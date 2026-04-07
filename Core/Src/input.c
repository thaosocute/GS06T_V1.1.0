#include "input.h"
#include <stdlib.h>
#include <string.h>

monitor_event_t monitors_event[EVENTS_HISTORY_MAX];

uint32_t input_queue[INPUT_QUEUE_SIZE];
int queue_head = 0;
int queue_tail = 0;
int queue_count = 0;
uint16_t queue_total_samples = 0;

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
static inline uint32_t queue_get(int i) {
    return input_queue[(queue_head - 1 - i + INPUT_QUEUE_SIZE) % INPUT_QUEUE_SIZE];
}

static inline uint8_t get_pin(int sample_idx, uint8_t pin) {
    // Input_TypeDef: 1..32, bit position trong queue là 0..31
    if (pin == 0 || pin > 32) return 0;
    return (queue_get(sample_idx) >> (pin - 1)) & 1;
}

/* đếm số samples cần dùng theo observe_ms */
static inline uint16_t samples_for(uint32_t observe_ms) {
    uint16_t n = observe_ms / SAMPLE_RATE_MS;
    return n;
}

PinAnalysis analyze_pin(uint8_t n_samples, uint8_t pin) {
    PinAnalysis a = {0};

    /* giới hạn không vượt quá số sample thực có */
    if (n_samples > queue_count)
        n_samples = queue_count;

    // phân tích xung
    for (uint8_t i = 0; i < n_samples; i++) {
        uint8_t val = get_pin(i, pin);
        if (val) a.high_count++;
        else     a.low_count++;
        if (i > 0 && val != get_pin(i - 1, pin))
            a.transitions++;
    }

    a.pulse_num = a.transitions/2;

    uint32_t total = a.high_count + a.low_count;
    if (total == 0) return a;

    a.duty_cycle = (float)a.high_count / total;

    if (a.transitions >= 2) {
        float duration_s = (n_samples * SAMPLE_RATE_MS) / 1000.0f;
        a.freq_hz = (a.transitions / 2.0f) / duration_s;
    }

    return a;
}

static json_monitor_state_t infer_from_analysis(MonitorType type, PinAnalysis *a) {
    /* so khớp a->freq_hz với các mức BLINK_* */
    if (type == MON_LED || type == MON_LED_MULTICOLOR) {
        float freq = a->freq_hz;
        /* tín hiệu tĩnh */
        if (a->transitions == 0) {
            a->confident = 0.9f;
            return (a->duty_cycle >= 0.85f) ? ON : OFF;
        }
        if (freq >= 6.0f && freq <= 10.0f) {
            a->confident = 1 - abs(freq - 8.0f)/2.0f;
            return BLINK_8HZ;
        }
        if (freq >= 4.5f && freq <= 5.5f) {
            a->confident = 1 - abs(freq - 5.0f)/0.5f;
            return BLINK_5HZ;
        }
        if (freq >= 3.5f && freq <= 4.5f) {
            a->confident = 1 - abs(freq - 4.0f)/0.5f;
            return BLINK_4HZ;
        }
        if (freq >= 1.5f && freq <= 2.5f) {
            a->confident =1 - abs(freq - 2.0f)/0.5f;
            return BLINK_2HZ;
        }
        if (freq >= 0.5f && freq <= 1.5f) {
            a->confident = 1 - abs(freq - 1.0f)/0.5f;
            return BLINK_1HZ;
        }
        if (freq >= 0.2f && freq <= 0.3f) {
            a->confident = 1 - abs(freq - 0.25f)/0.05f;
            return BLINK_0_25HZ;
        }
        if(a->pulse_num == 1) {
            a->confident = 0.9f;
            return BLINK_1_PER_MIN;
        } else if (a->pulse_num == 2) {
            a->confident = 0.9f;
            return BLINK_2_PER_MIN;
        } else if (a->pulse_num == 3) {
            a->confident = 0.9f;
            return BLINK_3_PER_MIN;
        }
    } 

    if(type == MON_BUZZER) {
        if(a->duty_cycle >= 0.6f) {
            a->confident = 0.8f;
            return ALARM_CONTINUOUS;
        } else if (a->duty_cycle > 0.45f && a->duty_cycle < 0.6f) {
            a->confident = abs((a->duty_cycle - 0.5f)/0.05);
            return BEEP_1_PER_0S5_8S;
        } else if (a->duty_cycle > 0.2f && a->duty_cycle <=0.45f) {
            return BEEP_3_PER_1S5;
        } else if (a->duty_cycle <= 0.01f) {
            a->confident = 0.99f;
            return OFF;
        }
        if(a->pulse_num == 1){
            a->confident = 0.9f;
            return BEEP_1_PER_5S;
        }
    }
    return UNKNOWN;
}
void push_input(uint32_t val) {
    input_queue[queue_head] = val;
    queue_head = (queue_head + 1) % INPUT_QUEUE_SIZE;
    if (queue_count < INPUT_QUEUE_SIZE) {
        queue_count++;
    } else {
        queue_tail = (queue_tail + 1) % INPUT_QUEUE_SIZE;
    }
    queue_total_samples++;
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
    uint16_t n = samples_for(monitor->timing.observe_ms);
    uint16_t debounce_n = samples_for(monitor->timing.debounce_ms);

    /* nếu chưa đủ samples cho observe, set DETECTING */
    if (queue_total_samples < n) {
        monitor->state = DETECTING;
        return;
    }

    switch (monitor->type) {

        case MON_LED: {
            switch (monitor->mode) {
                case MON_MODE_PATTERN: {
                    PinAnalysis a = analyze_pin(n, monitor->cfg.led.pin);
                    monitor->state = infer_from_analysis(MON_LED, &a);
                    monitor->confident = a.confident;
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
                        // tĩnh: sử dụng infer_from_analysis để lấy trạng thái ON/OFF cho từng pin, rồi tổng hợp
                        json_monitor_state_t r_state = infer_from_analysis(MON_LED_MULTICOLOR, &a_r);
                        json_monitor_state_t g_state = infer_from_analysis(MON_LED_MULTICOLOR, &a_g);
                        json_monitor_state_t b_state = infer_from_analysis(MON_LED_MULTICOLOR, &a_b);
                        json_monitor_state_t o_state = infer_from_analysis(MON_LED_MULTICOLOR, &a_o);
                        
                        uint8_t r = (r_state == ON);
                        uint8_t g = (g_state == ON);
                        uint8_t b = (b_state == ON);
                        uint8_t o = (o_state == ON);
                        
                        uint8_t active = r + g + b + o;
                        if (r && b && !g && !o) {
                            monitor->confident = a_r.confident * a_b.confident;
                            monitor->state = ON_PURPLE;
                        } else if (active > 2) {
                            monitor->state = UNKNOWN;
                        } else if (r && !g && !b && !o) {
                            monitor->confident = a_r.confident;
                            monitor->state = ON;
                        } else if (g && !r && !b && !o) {
                            monitor->confident = a_g.confident;
                            monitor->state = ON;  // no dedicated green code, using ON
                        } else if (b && !r && !g && !o) {
                            monitor->confident = a_b.confident;
                            monitor->state = ON_BLUE;
                        } else if (o && !r && !g && !b) {
                            monitor->confident = a_o.confident;
                            monitor->state = ON_ORANGE;
                        } else if (active == 0) {
                            monitor->confident = 0.9f;  // average confident
                            monitor->state = OFF;
                        } else {
                            monitor->state = UNKNOWN;
                        }
                    } else {
                        // động, chỉ 1 led hoạt động tại 1 thời điểm
                        uint8_t blinking_pins = 0;
                        if (a_r.transitions > 0) blinking_pins++;
                        if (a_g.transitions > 0) blinking_pins++;
                        if (a_b.transitions > 0) blinking_pins++;
                        if (a_o.transitions > 0) blinking_pins++;
                        if (blinking_pins > 2) {
                            monitor->state = UNKNOWN;
                        } else if (blinking_pins == 2) {
                            if (a_r.transitions > 0 && a_b.transitions > 0 && a_g.transitions == 0 && a_o.transitions == 0) {
                                json_monitor_state_t red_blink = infer_from_analysis(MON_LED_MULTICOLOR, &a_r);
                                json_monitor_state_t blue_blink = infer_from_analysis(MON_LED_MULTICOLOR, &a_b);
                                monitor->confident = a_r.confident * a_b.confident;
                                if (red_blink == blue_blink) {
                                    if (red_blink == BLINK_1HZ) {
                                        monitor->state = BLINK_PURPLE_1HZ;
                                    } else if (red_blink == BLINK_5HZ) {
                                        monitor->state = BLINK_PURPLE_5HZ;
                                    } else {
                                        monitor->state = UNKNOWN;
                                    }
                                } else {
                                    monitor->state = UNKNOWN;
                                }
                            } else {
                                monitor->state = UNKNOWN;
                            }
                        } else {
                            PinAnalysis *active_a = NULL;
                            if (a_r.transitions > 0) active_a = &a_r;
                            else if (a_b.transitions > 0) active_a = &a_b;
                            else if (a_o.transitions > 0) active_a = &a_o;
                            else if (a_g.transitions > 0) active_a = &a_g;
                            if (active_a) {
                                json_monitor_state_t blink = infer_from_analysis(MON_LED_MULTICOLOR, active_a);
                                monitor->confident = active_a->confident;
                                if (active_a == &a_r) {
                                    if (blink == BLINK_1HZ) monitor->state = BLINK_RED_1HZ;
                                    else if (blink == BLINK_5HZ) monitor->state = BLINK_RED_5HZ;
                                    else monitor->state = UNKNOWN;
                                } else if (active_a == &a_b) {
                                    if (blink == BLINK_1HZ) monitor->state = BLINK_BLUE_1HZ;
                                    else if (blink == BLINK_5HZ) monitor->state = BLINK_BLUE_5HZ;
                                    else if (blink == BLINK_0_25HZ) monitor->state = BLINK_BLUE_0_25HZ;
                                    else monitor->state = UNKNOWN;
                                } else if (active_a == &a_o) {
                                    if (blink == BLINK_1HZ) monitor->state = BLINK_ORANGE_1HZ;
                                    else if (blink == BLINK_5HZ) monitor->state = BLINK_ORANGE_5HZ;
                                    else monitor->state = UNKNOWN;
                                } else if (active_a == &a_g) {
                                    if (blink == BLINK_1HZ) monitor->state = BLINK_GREEN_1HZ;
                                    else monitor->state = UNKNOWN;
                                } else {
                                    monitor->state = UNKNOWN;
                                }
                            } else {
                                monitor->state = UNKNOWN;
                            }
                        }
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
                        uint8_t active = r + g + b + o;
                        if (active > 1) {
                            monitor->state = UNKNOWN;
                        } else if (r) {
                            monitor->state = ON;
                        } else if (g) {
                            monitor->state = ON; // no dedicated green code
                        } else if (b) {
                            monitor->state = ON_BLUE;
                        } else if (o) {
                            monitor->state = ON_ORANGE;
                        } else if (r && b) {
                            monitor->state = ON_PURPLE;
                        } else {
                            monitor->state = OFF;
                        }
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
                    monitor->confident = a.confident;
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
                monitor->confident = 0;
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