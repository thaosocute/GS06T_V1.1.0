#include "input.h"
#include <string.h>

monitor_event_t monitors_event[EVENTS_HISTORY_MAX];

uint16_t input_queue[INPUT_QUEUE_SIZE];
int queue_head = 0;
int queue_tail = 0;
int queue_count = 0;

input_state_t input_state[16];

int events_index = 0;
int events_count = 0;

static void add_monitor_event(const char *monitor_id, json_monitor_state_t current_state, json_monitor_state_t prev_state) {
    monitor_event_t event;
    strcpy(event.monitor_id, monitor_id);
    event.current_state = current_state;
    event.prev_state = prev_state;
    event.event_time = HAL_GetTick();
    event.pending_time = 0; // có thể set theo nhu cầu
    monitors_event[events_index] = event;
    events_index = (events_index + 1) % EVENTS_HISTORY_MAX;
    if (events_count < EVENTS_HISTORY_MAX) {
        events_count++;
    }
}

/*
 * i = 0 → mới nhất
 * i = 1 → trước đó 50ms
 * i = queue_count-1 → cũ nhất
 */
static inline uint16_t queue_get(int i) {
    return input_queue[(queue_head - 1 - i + INPUT_QUEUE_SIZE) % INPUT_QUEUE_SIZE];
}

static inline uint8_t get_pin(int sample_idx, uint8_t pin) {
    return (queue_get(sample_idx) >> pin) & 1;
}

/* đếm số samples cần dùng theo observe_ms */
static inline uint8_t samples_for(uint32_t observe_ms) {
    uint32_t n = observe_ms / SAMPLE_RATE_MS;
    return (uint8_t)(n > INPUT_QUEUE_SIZE ? INPUT_QUEUE_SIZE : n);
}

static PinAnalysis analyze_pin(uint8_t n_samples, uint8_t pin) {
    PinAnalysis a = {0};

    /* giới hạn không vượt quá số sample thực có */
    if (n_samples > queue_count)
        n_samples = queue_count;

    for (uint8_t i = 0; i < n_samples; i++) {
        uint8_t val = get_pin(i, pin);
        if (val) a.high_count++;
        else     a.low_count++;
        if (i > 0 && val != get_pin(i - 1, pin))
            a.transitions++;
    }

    uint32_t total = a.high_count + a.low_count;
    if (total == 0) return a;

    a.duty_cycle = (float)a.high_count / total;

    if (a.transitions >= 2) {
        float duration_s = (n_samples * SAMPLE_RATE_MS) / 1000.0f;
        a.freq_hz = (a.transitions / 2.0f) / duration_s;
    }

    return a;
}

static json_monitor_state_t infer_from_analysis(MonitorType type, const PinAnalysis *a) {
    /* tín hiệu tĩnh */
    if (a->transitions == 0) {
        return (a->duty_cycle >= 0.99f) ? ON : OFF;
    }

    /* so khớp a->freq_hz với các mức BLINK_* */
    float freq = a->freq_hz;
    if (freq >= 7.5f && freq <= 8.5f) return BLINK_8HZ;
    if (freq >= 4.5f && freq <= 5.5f) return BLINK_5HZ;
    if (freq >= 3.5f && freq <= 4.5f) return BLINK_4HZ;
    if (freq >= 1.5f && freq <= 2.5f) return BLINK_2HZ;
    if (freq >= 0.5f && freq <= 1.5f) return BLINK_1HZ;
    if (freq >= 0.2f && freq <= 0.3f) return BLINK_0_25HZ;
    if (freq >= 0.016f && freq <= 0.017f) return BLINK_1_PER_MIN;  // ~1/60 Hz
    if (freq >= 0.032f && freq <= 0.035f) return BLINK_2_PER_MIN;  // ~2/60 Hz
    if (freq >= 0.048f && freq <= 0.052f) return BLINK_3_PER_MIN;  // ~3/60 Hz

    return UNKNOWN;
}
void push_input(uint16_t val) {
    input_queue[queue_head] = val;
    queue_head = (queue_head + 1) % INPUT_QUEUE_SIZE;
    if (queue_count < INPUT_QUEUE_SIZE) {
        queue_count++;
    } else {
        queue_tail = (queue_tail + 1) % INPUT_QUEUE_SIZE;
    }
}

static uint8_t is_stable(uint8_t pin, uint8_t debounce_samples) {
    if (debounce_samples == 0) return 1;  // no debounce
    uint8_t val = get_pin(0, pin);
    for (uint8_t i = 1; i < debounce_samples && i < queue_count; i++) {
        if (get_pin(i, pin) != val) return 0;
    }
    return 1;
}

void update_monitor_state(Monitor *monitor) {
    json_monitor_state_t prev_state = monitor->state;
    uint8_t n = samples_for(monitor->timing.observe_ms);
    uint8_t debounce_n = samples_for(monitor->timing.debounce_ms);

    /* nếu chưa đủ samples cho observe, set DETECTING */
    if (queue_count < n) {
        monitor->state = DETECTING;
        return;
    }

    switch (monitor->type) {

        case MON_LED: {
            switch (monitor->mode) {
                case MON_MODE_PATTERN: {
                    PinAnalysis a = analyze_pin(n, monitor->cfg.led.pin);
                    monitor->state = infer_from_analysis(MON_LED, &a);
                    break;
                }
                case MON_MODE_EVENT: {
                    if (!is_stable(monitor->cfg.led.pin, debounce_n)) {
                        monitor->state = DETECTING;
                    } else {
                        uint8_t val = get_pin(0, monitor->cfg.led.pin);
                        monitor->state = val ? ON : OFF;
                    }
                    break;
                }
            }
            break;
        }

        case MON_LED_MULTICOLOR: {
            switch (monitor->mode) {
                case MON_MODE_PATTERN: {
                    PinAnalysis a_r = analyze_pin(n, monitor->cfg.led_mc.pin_r);
                    PinAnalysis a_g = analyze_pin(n, monitor->cfg.led_mc.pin_g);
                    PinAnalysis a_b = analyze_pin(n, monitor->cfg.led_mc.pin_b);
                    PinAnalysis a_o = analyze_pin(n, monitor->cfg.led_mc.pin_orange);
                    /* kết hợp: nếu tất cả tĩnh, suy màu; nếu có động, suy BLINK */
                    uint8_t has_blink = (a_r.transitions > 0) || (a_g.transitions > 0) || (a_b.transitions > 0) || (a_o.transitions > 0);
                    if (!has_blink) {
                        // tĩnh
                        uint8_t r = a_r.duty_cycle >= 0.99f;
                        uint8_t g = a_g.duty_cycle >= 0.99f;
                        uint8_t b = a_b.duty_cycle >= 0.99f;
                        uint8_t o = a_o.duty_cycle >= 0.99f;
                        if (r && g && b) monitor->state = ON_PURPLE;  // giả sử
                        else if (r && b) monitor->state = ON_PURPLE;
                        else if (r && o) monitor->state = ON_ORANGE;
                        else if (b) monitor->state = ON_BLUE;
                        else if (r) monitor->state = ON;  // RED
                        else monitor->state = OFF;
                    } else {
                        // động, suy từ freq trung bình
                        float avg_freq = (a_r.freq_hz + a_g.freq_hz + a_b.freq_hz + a_o.freq_hz) / 4.0f;
                        json_monitor_state_t blink = infer_from_analysis(MON_LED_MULTICOLOR, &(PinAnalysis){0,0,0,0.0f,avg_freq});
                        // map sang BLINK_*
                        if (blink == BLINK_1HZ) monitor->state = BLINK_BLUE_1HZ;  // giả sử
                        else monitor->state = UNKNOWN;
                    }
                    break;
                }
                case MON_MODE_EVENT: {
                    if (!is_stable(monitor->cfg.led_mc.pin_r, debounce_n) ||
                        !is_stable(monitor->cfg.led_mc.pin_g, debounce_n) ||
                        !is_stable(monitor->cfg.led_mc.pin_b, debounce_n) ||
                        !is_stable(monitor->cfg.led_mc.pin_orange, debounce_n)) {
                        monitor->state = DETECTING;
                    } else {
                        uint8_t r = get_pin(0, monitor->cfg.led_mc.pin_r);
                        uint8_t g = get_pin(0, monitor->cfg.led_mc.pin_g);
                        uint8_t b = get_pin(0, monitor->cfg.led_mc.pin_b);
                        uint8_t o = get_pin(0, monitor->cfg.led_mc.pin_orange);
                        if (r && b) monitor->state = ON_PURPLE;
                        else if (r && o) monitor->state = ON_ORANGE;
                        else if (b) monitor->state = ON_BLUE;
                        else if (r) monitor->state = ON;  // RED
                        else monitor->state = OFF;
                    }
                    break;
                }
            }
            break;
        }

        case MON_BUZZER: {
            switch (monitor->mode) {
                case MON_MODE_PATTERN: {
                    PinAnalysis a = analyze_pin(n, monitor->cfg.buzzer.pin);
                    monitor->state = infer_from_analysis(MON_BUZZER, &a);
                    break;
                }
                case MON_MODE_EVENT: {
                    if (!is_stable(monitor->cfg.buzzer.pin, debounce_n)) {
                        monitor->state = DETECTING;
                    } else {
                        uint8_t val = get_pin(0, monitor->cfg.buzzer.pin);
                        monitor->state = val ? BEEP_ONCE : OFF;
                    }
                    break;
                }
            }
            break;
        }

        case MON_RELAY: {
            if (!is_stable(monitor->cfg.relay.pin, debounce_n)) {
                monitor->state = DETECTING;
            } else {
                uint8_t val = get_pin(0, monitor->cfg.relay.pin);
                if (monitor->cfg.relay.active_state == ACTIVE_CLOSED)
                    monitor->state = val ? CLOSE : OPEN;
                else
                    monitor->state = val ? OPEN : CLOSE;
            }
            break;
        }
    }

    if (monitor->state != prev_state) {
        add_monitor_event(monitor->id, monitor->state, prev_state);
    }
}