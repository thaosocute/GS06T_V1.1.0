#include "monitors_config.h"
#include "input.h"
#include <string.h>
#include <stdlib.h>  // for strtol
#include <stdio.h>   // for snprintf

Monitor monitors_arr[MONITORS_NUM_MAX];
Relay_output_TypeDef relay_output_arr[RELAY_OUTPUT_NUM_MAX];

static const char *g_json_str = NULL;

char error_reason[32];

extern monitor_event_t monitors_event[EVENTS_HISTORY_MAX];
extern int events_index;
extern int events_count;

extern I2C_HandleTypeDef hi2c3;
extern volatile uint32_t tim6_tick_ms;

static inline Input_TypeDef sanitize_input_pin(uint64_t val) {
    if (val >= 1 && val <= 16) {
        return (Input_TypeDef)val;
    }
    return 0;  // 0 nghĩa chưa gán/unassigned
}

void monitors_set_json(const char *json_str) {
    g_json_str = json_str;
}

void monitor_set_state_event() {
    for(uint8_t i = 0; i < 16; i++) {
        update_monitor_state(&monitors_arr[i]);
    }
}

uint32_t find_seq_number(jsmntok_t *tokens, int token_count) {
    uint32_t seq = 0;
    for (int i = 1; i < token_count; i++) {
        if (jsoneq(g_json_str, &tokens[i], "seq") == 0) {
            uint64_t val;
            json_parse_uint64(g_json_str, &tokens[i+1], &val);
            seq = (uint32_t)val;
        }
    }
    return seq;
}

static char* monitor_type_to_str(MonitorType type) {
    switch (type) {
        case MON_LED: return "led";
        case MON_LED_MULTICOLOR: return "led_multicolor";
        case MON_BUZZER: return "buzzer";
        case MON_RELAY: return "relay";
        default: return "unknown";
    }
}

static uint8_t relay_name_to_button(const char *relay_id, Button_TypeDef *button) {
    if (!relay_id || !button) return 0;

    if (strcmp(relay_id, "RL1") == 0) { *button = RL1; return 1; }
    if (strcmp(relay_id, "RL2") == 0) { *button = RL2; return 1; }
    if (strcmp(relay_id, "RL3") == 0) { *button = RL3; return 1; }
    if (strcmp(relay_id, "RL4") == 0) { *button = RL4; return 1; }
    if (strcmp(relay_id, "RL5") == 0) { *button = RL5; return 1; }
    if (strcmp(relay_id, "RL6") == 0) { *button = RL6; return 1; }
    if (strcmp(relay_id, "RL7") == 0) { *button = RL7; return 1; }
    if (strcmp(relay_id, "RL8") == 0) { *button = RL8; return 1; }
    if (strcmp(relay_id, "RL9") == 0) { *button = RL9; return 1; }
    if (strcmp(relay_id, "RL10") == 0) { *button = RL10; return 1; }
    if (strcmp(relay_id, "RL11") == 0) { *button = RL11; return 1; }
    if (strcmp(relay_id, "RL12") == 0) { *button = RL12; return 1; }
    if (strcmp(relay_id, "RL13") == 0) { *button = RL13; return 1; }
    if (strcmp(relay_id, "RL14") == 0) { *button = RL14; return 1; }
    if (strcmp(relay_id, "BR1") == 0) { *button = BR1; return 1; }
    if (strcmp(relay_id, "BR2") == 0) { *button = BR2; return 1; }
    if (strcmp(relay_id, "BR3") == 0) { *button = BR3; return 1; }
    if (strcmp(relay_id, "BR4") == 0) { *button = BR4; return 1; }
    if (strcmp(relay_id, "BR5") == 0) { *button = BR5; return 1; }
    if (strcmp(relay_id, "BR6") == 0) { *button = BR6; return 1; }

    return 0;
}

// static int get_latest_event(monitor_event_t *events_arr, char *buf, size_t buf_size) {
//     // Get latest event
//     if (events_count > 0) {
//         int latest_idx = (events_index - 1 + EVENTS_HISTORY_MAX) % EVENTS_HISTORY_MAX;
//         monitor_event_t *event = &monitors_event[latest_idx];
//         if(event->monitor_id[0] != '\0') {
//             events_index--;
//             events_count--;
//         }
//         // Find monitor type
//         char *type_str = "unknown";
//         for (int i = 0; i < 16; i++) {
//             if (strcmp(monitors_arr[i].id, event->monitor_id) == 0) {
//                 type_str = monitor_type_to_str(monitors_arr[i].type);
//                 break;
//             }
//         }
//         snprintf(buf, buf_size, "[{\"id\":\"%s\",\"type\":\"%s\",\"state\":\"%s\",\"prev\":\"%s\",\"ts_ms\":%lu}]",
//                  event->monitor_id, type_str, state_to_str(event->current_state), state_to_str(event->prev_state), event->event_time);
    
//         return 0; 
//     } else {
//         return -1;
//     }
// }

static void build_events_json(char *out_buf, size_t out_buf_size, int events_to_take) {
    if (!out_buf || out_buf_size == 0) return;
    if (events_to_take > events_count) events_to_take = events_count;
    if (events_to_take > 5) events_to_take = 5;
    if (events_to_take <= 0) {
        snprintf(out_buf, out_buf_size, "[]");
        return;
    }

    size_t used = 0;
    size_t remain = out_buf_size;

    // Ghi "["
    int n = snprintf(out_buf + used, remain, "[");
    if (n < 0 || (size_t)n >= remain) goto fail;
    used += (size_t)n;
    remain = out_buf_size - used;

    for (int i = 0; i < events_to_take; i++) {
        int idx = (events_index - 1 - i + EVENTS_HISTORY_MAX) % EVENTS_HISTORY_MAX;
        monitor_event_t *event = &monitors_event[idx];

        // Tìm type của monitor
        const char *type_str = "unknown";
        for (int j = 0; j < MONITORS_NUM_MAX; j++) {
            if (monitors_arr[j].id[0] != '\0' &&
                strcmp(monitors_arr[j].id, event->monitor_id) == 0) {
                type_str = monitor_type_to_str(monitors_arr[j].type);
                break;
            }
        }

        // Thêm dấu "," giữa các object
        if (i > 0) {
            if (remain < 2) goto close;
            out_buf[used++] = ',';
            out_buf[used] = '\0';
            remain = out_buf_size - used;
        }

        // Ghi JSON object
        n = snprintf(out_buf + used, remain,
                     "{\"id\":\"%s\",\"type\":\"%s\","
                     "\"state\":\"%s\",\"prev\":\"%s\",\"ts_ms\":%lu}",
                     event->monitor_id, type_str,
                     state_to_str(event->current_state),
                     state_to_str(event->prev_state),
                     (unsigned long)event->event_time);
        if (n < 0 || (size_t)n >= remain) goto close;
        used += (size_t)n;
        remain = out_buf_size - used;
    }

close:
    // Ghi "]"
    if (remain >= 2) {
        out_buf[used++] = ']';
        out_buf[used] = '\0';
    } else {
        snprintf(out_buf, out_buf_size, "[]");
    }

    // Cập nhật lại buffer state
    events_count -= events_to_take;
    if (events_count < 0) events_count = 0;
    events_index = (events_index - events_to_take + EVENTS_HISTORY_MAX) % EVENTS_HISTORY_MAX;
    return;

fail:
    snprintf(out_buf, out_buf_size, "[]");
}

static Relay_output_TypeDef *relay_output_get(Button_TypeDef button) {
    if (button == 0) return NULL;
    uint8_t idx = (uint8_t)button - 1;
    if (idx >= RELAY_OUTPUT_NUM_MAX) return NULL;
    return &relay_output_arr[idx];
}

static void relay_output_step(Relay_output_TypeDef *ro, uint32_t now_ms) {
    if (!ro) return;
    if (ro->state == RELAY_IDLE || ro->state == RELAY_ON) return;
    if (ro->deadline_ms == 0 || (int32_t)(now_ms - ro->deadline_ms) < 0) return;

    /* Phase 1 of pulse: release button */
    if (ro->state == RELAY_PULSE) {
        release_button(&hi2c3, ro->button);

        /* Simple pulse (not part of sequence) */
        if (!ro->in_seq) {
            ro->state = RELAY_IDLE;
            ro->deadline_ms = 0;
            return;
        }

        /* Sequence pulse: check if more gaps left */
        if (ro->seq_pos >= ro->seq_count - 1) {
            ro->state = RELAY_IDLE;
            ro->in_seq = 0;
            ro->deadline_ms = 0;
            return;
        }

        /* Transition to gap phase */
        uint32_t gap = ro->gap_ms[ro->seq_pos];
        ro->state = RELAY_GAP;
        ro->deadline_ms = now_ms + gap;
        return;
    }

    /* Phase 2 of sequence: gap done, press button again */
    if (ro->state == RELAY_GAP) {
        ro->seq_pos++;
        if (ro->seq_pos >= ro->seq_count) {
            ro->state = RELAY_IDLE;
            ro->in_seq = 0;
            ro->deadline_ms = 0;
            return;
        }

        press_button(&hi2c3, ro->button);
        ro->state = RELAY_PULSE;
        ro->deadline_ms = now_ms + ro->pulse_ms[ro->seq_pos];
        return;
    }
}

void relay_output_timer_tick(void) {
    uint32_t now_ms = tim6_tick_ms;
    for (uint8_t i = 0; i < RELAY_OUTPUT_NUM_MAX; i++) {
        relay_output_step(&relay_output_arr[i], now_ms);
    }
}

void set_relay_output_button(){
    for(uint8_t i = 0; i < RELAY_OUTPUT_NUM_MAX; i++){
        relay_output_arr[i].button = (Button_TypeDef)(i+1);
        relay_output_arr[i].state = RELAY_IDLE;
        relay_output_arr[i].deadline_ms = 0;
        relay_output_arr[i].in_seq = 0;
        relay_output_arr[i].seq_pos = 0;
        relay_output_arr[i].seq_count = 0;
        for (uint8_t j = 0; j < RELAY_PULSE_SEQ_MAX_STEPS; j++) {
            relay_output_arr[i].pulse_ms[j] = 0;
            relay_output_arr[i].gap_ms[j] = 0;
        }
    }
}

void handle_error(json_err_t error, uint32_t seq, char *response, size_t response_size) {
    if(error == ERR_NONE){
        return;
    }
    char err_code[20];
    strcpy(err_code, json_err_to_code(error));

    // tạo message string
    char *msg_string = (char *)malloc(64);
    if(error == ERR_INVALID_CMD){ 
        snprintf(msg_string, 64, "command %s invalid", error_reason);
        memset(error_reason, 0, sizeof(error_reason));
    }
    if(error == ERR_INVALID_ID) {
        snprintf(msg_string, 64, "%s not found", error_reason);
        memset(error_reason, 0, sizeof(error_reason));
    }
    if(error == ERR_INVALID_DATA) {
        snprintf(msg_string, 64, "%s not found or wrong data", error_reason);
        memset(error_reason, 0, sizeof(error_reason));
    }
    if(error == ERR_JSON_PARSE) {
        snprintf(msg_string, 64, "Unexpected symbol");
    }
    if(error == ERR_PULSE_RANGE) {
        snprintf(msg_string, 64, "pulse_ms %s out of range [10,5000]", error_reason);
        memset(error_reason, 0, sizeof(error_reason));
    }
    if(error == ERR_BUSY) {
        snprintf(msg_string, 64, "%s busy", error_reason);
        memset(error_reason, 0, sizeof(error_reason));
    }
    if(error == ERR_HW_FAULT) {
        snprintf(msg_string, 64, "hardware fault");
        memset(error_reason, 0, sizeof(error_reason));
    }

    // lấy các sự kiện (events)
    int events_to_take = events_count;
    if (events_to_take > 5) events_to_take = 5;

    size_t events_buf_size = 32 + (size_t)events_to_take * 140;
    if (events_buf_size < 128) events_buf_size = 128;
    char *events_buf = (char *)malloc(events_buf_size);
    if (!events_buf) {
        return;
    }
    build_events_json(events_buf, events_buf_size, events_to_take);

    int pending_events = events_count;

    // tạo response
    snprintf(response, response_size, "{\"cmd\":\"error\",\"seq\":%ld,\"data\":{\"code\":\"%s\",\"msg\":\"%s\"},\"events\":%s,\"pending\":%d}\r\n", 
        seq, err_code, msg_string, events_buf, pending_events);
    
    free(events_buf);
    free(msg_string);
}

json_err_t handle_monitors_config(jsmntok_t *tokens, int token_count, char *response, size_t response_size) {
    if (!g_json_str || !tokens || token_count <= 0) {
        return ERR_JSON_PARSE;
    }
    int r = token_count;

    uint8_t found = 0;
    // Tìm "seq"
    uint32_t seq = 0;
    
    for (int i = 1; i < r; i++) {
        if (jsoneq(g_json_str, &tokens[i], "seq") == 0) {
            uint64_t val;
            json_parse_uint64(g_json_str, &tokens[i+1], &val);
            seq = (uint32_t)val;
            found = 1;
            break;
        }
    }

    if(!found) {
        snprintf(error_reason, 16, "seq");
        return ERR_INVALID_DATA;
    } 
    // Tìm key "monitors"
    int monitors_idx = -1;
    for (int i = 1; i < r; i++) {
        if (jsoneq(g_json_str, &tokens[i], "monitors") == 0 && tokens[i+1].type == JSMN_ARRAY) {
            monitors_idx = i + 1;  // token của array
            break;
        }
    }
    if (monitors_idx == -1) {
        snprintf(error_reason, 16, "monitors");
        return ERR_INVALID_DATA;
    }

    int arr_size = tokens[monitors_idx].size;
    if (arr_size > 16) arr_size = 16;  // giới hạn mảng

    int current = monitors_idx + 1;  // token đầu tiên của array
    for (int m = 0; m < arr_size; m++) {
        if (current >= r || tokens[current].type != JSMN_OBJECT) break;
        int obj_size = tokens[current].size;  // số pairs
        int obj_start = current + 1;  // token đầu tiên của pairs

        Monitor *mon = &monitors_arr[m];
        memset(mon, 0, sizeof(Monitor));  // reset

        for (int k = 0; k < obj_size; k++) {
            int key_idx = obj_start + k * 2;
            int val_idx = obj_start + k * 2 + 1;
            if (key_idx >= r || val_idx >= r) break;

            if (jsoneq(g_json_str, &tokens[key_idx], "id") == 0) {
                json_parse_string(g_json_str, &tokens[val_idx], mon->id);
            } else if (jsoneq(g_json_str, &tokens[key_idx], "type") == 0) {
                char type_str[20];
                json_parse_string(g_json_str, &tokens[val_idx], type_str);
                if (strcmp(type_str, "led") == 0) mon->type = MON_LED;
                else if (strcmp(type_str, "led_multicolor") == 0) mon->type = MON_LED_MULTICOLOR;
                else if (strcmp(type_str, "buzzer") == 0) mon->type = MON_BUZZER;
                else if (strcmp(type_str, "relay") == 0) mon->type = MON_RELAY;
            } else if (jsoneq(g_json_str, &tokens[key_idx], "mode") == 0) {
                char mode_str[10];
                json_parse_string(g_json_str, &tokens[val_idx], mode_str);
                if (strcmp(mode_str, "pattern") == 0) mon->mode = MON_MODE_PATTERN;
                else if (strcmp(mode_str, "event") == 0) mon->mode = MON_MODE_EVENT;
            } else if (jsoneq(g_json_str, &tokens[key_idx], "observe_ms") == 0) {
                uint64_t val;
                json_parse_uint64(g_json_str, &tokens[val_idx], &val);
                mon->timing.observe_ms = (uint32_t)val;
            } else if (jsoneq(g_json_str, &tokens[key_idx], "debounce_ms") == 0) {
                uint64_t val;
                json_parse_uint64(g_json_str, &tokens[val_idx], &val);
                mon->timing.debounce_ms = (uint16_t)val;
            } else if (jsoneq(g_json_str, &tokens[key_idx], "pin") == 0) {
                uint64_t val;
                json_parse_uint64(g_json_str, &tokens[val_idx], &val);
                if (mon->type == MON_LED) mon->cfg.led.pin = sanitize_input_pin(val);
                else if (mon->type == MON_BUZZER) mon->cfg.buzzer.pin = sanitize_input_pin(val);
                else if (mon->type == MON_RELAY) mon->cfg.relay.pin = sanitize_input_pin(val);
            } else if (jsoneq(g_json_str, &tokens[key_idx], "pin_r") == 0) {
                uint64_t val;
                json_parse_uint64(g_json_str, &tokens[val_idx], &val);
                mon->cfg.led_mc.pin_r = sanitize_input_pin(val);
            } else if (jsoneq(g_json_str, &tokens[key_idx], "pin_g") == 0) {
                uint64_t val;
                json_parse_uint64(g_json_str, &tokens[val_idx], &val);
                mon->cfg.led_mc.pin_g = sanitize_input_pin(val);
            } else if (jsoneq(g_json_str, &tokens[key_idx], "pin_b") == 0) {
                uint64_t val;
                json_parse_uint64(g_json_str, &tokens[val_idx], &val);
                mon->cfg.led_mc.pin_b = sanitize_input_pin(val);
            } else if (jsoneq(g_json_str, &tokens[key_idx], "pin_orange") == 0) {
                uint64_t val;
                json_parse_uint64(g_json_str, &tokens[val_idx], &val);
                mon->cfg.led_mc.pin_orange = sanitize_input_pin(val);
            } else if (jsoneq(g_json_str, &tokens[key_idx], "active_state") == 0) {
                char state_str[10];
                json_parse_string(g_json_str, &tokens[val_idx], state_str);
                if (strcmp(state_str, "closed") == 0) mon->cfg.relay.active_state = ACTIVE_CLOSED;
                else if (strcmp(state_str, "open") == 0) mon->cfg.relay.active_state = ACTIVE_OPEN;
            }
        }

        // Chuyển sang object tiếp theo
        current += 1 + obj_size * 2;
    }

    // Tạo response thành công
    snprintf(response, response_size, "{\"cmd\":\"ok\",\"seq\":%lu,\"data\":null,\"events\":[],\"pending\":0}\r\n", seq);
    return ERR_NONE;  // số monitors parsed
}

json_err_t handle_ping(jsmntok_t *tokens, int token_count, char *response, size_t response_size) {
    if (!g_json_str || !tokens || token_count <= 0) {
        return ERR_JSON_PARSE;
    }

    int r = token_count;

    // Tìm "seq"
    uint8_t found = 0;
    uint32_t seq = 0;
    for (int i = 1; i < r; i++) {
        if (jsoneq(g_json_str, &tokens[i], "seq") == 0 && (i + 1) < r) {
            uint64_t val;
            if (json_parse_uint64(g_json_str, &tokens[i + 1], &val)) {
                seq = (uint32_t)val;
                found = 1;
            }
        } 
    }
    if(!found) {
        snprintf(error_reason, 32, "seq");
        return ERR_INVALID_DATA;
    }

    // count configured monitors (non-empty id signifies an entry)
    int num_monitors = 0;
    for (int i = 0; i < MONITORS_NUM_MAX; i++) {
        if (monitors_arr[i].id[0] != '\0') {
            num_monitors++;
        }
    }

    uint32_t uptime_ms = HAL_GetTick();

    // lấy các sự kiện (events)
    int events_to_take = events_count;
    if (events_to_take > 5) events_to_take = 5;

    size_t events_buf_size = 32 + (size_t)events_to_take * 140;
    if (events_buf_size < 128) events_buf_size = 128;
    char *events_buf = malloc(events_buf_size);
    if (!events_buf) {
        return ERR_INVALID_CMD;
    }
    build_events_json(events_buf, events_buf_size, events_to_take);

    int pending_events = events_count;

    snprintf(response, response_size,
             "{\"cmd\":\"ok\",\"seq\":%lu,\"data\":{\"uptime_ms\":%lu,\"fw_version\":\"%s\",\"num_monitors\":%d},\"events\":%s,\"pending\":%d}\r\n",
             seq, uptime_ms, FIRMWARE_VERSION, num_monitors, events_buf, pending_events);
    free(events_buf);
    return ERR_NONE;
}

static int state_to_code(json_monitor_state_t state) {
    return (int)state;
}

static float get_confidence(json_monitor_state_t state) {
    if (state == DETECTING || state == UNKNOWN) return 0.0f;
    if (state >= BLINK_1HZ && state <= BLINK_3_PER_MIN) return 0.94f; // blink states
    return 1.0f;
}

json_err_t handle_read_pattern(jsmntok_t *tokens, int token_count, char *response, size_t response_size) {
    if (!g_json_str || !tokens || token_count <= 0) {
        return ERR_JSON_PARSE;
    }
    int r = token_count;

    // Tìm "seq"
    uint32_t seq = 0;
    for (int i = 1; i < r; i++) {
        if (jsoneq(g_json_str, &tokens[i], "seq") == 0) {
            uint64_t val;
            json_parse_uint64(g_json_str, &tokens[i+1], &val);
            seq = (uint32_t)val;
            break;
        }
    }

    // Tìm "data"
    int data_idx = -1;
    for (int i = 1; i < r; i++) {
        if (jsoneq(g_json_str, &tokens[i], "data") == 0 && tokens[i+1].type == JSMN_OBJECT) {
            data_idx = i + 1;
            break;
        }
    }
    if (data_idx == -1) {
        return ERR_INVALID_DATA;
    }

    // Trong data, tìm "ids"
    int ids_idx = -1;
    int data_size = tokens[data_idx].size;
    int data_start = data_idx + 1;
    for (int k = 0; k < data_size; k++) {
        int key_idx = data_start + k * 2;
        int val_idx = data_start + k * 2 + 1;
        if (key_idx >= r || val_idx >= r) break;
        if (jsoneq(g_json_str, &tokens[key_idx], "ids") == 0 && tokens[val_idx].type == JSMN_ARRAY) {
            ids_idx = val_idx;
            break;
        }
    }
    if (ids_idx == -1) {
        return ERR_INVALID_DATA;
    }

    int ids_size = tokens[ids_idx].size;
    if (ids_size > 16) ids_size = 16; // limit

    int max_results = ids_size;
    size_t results_buf_size = 64 + (size_t)max_results * 128;
    if (results_buf_size < 256) results_buf_size = 256;
    char *results_buf = malloc(results_buf_size);
    if (!results_buf) {
        return ERR_INVALID_DATA;
    }
    size_t results_used = 0;
    size_t results_remain = results_buf_size;
    int n = snprintf(results_buf + results_used, results_remain, "[");
    if (n < 0 || (size_t)n >= results_remain) {
        free(results_buf);
        return ERR_INVALID_DATA;
    }
    results_used += (size_t)n;
    results_remain = results_buf_size - results_used;

    int results_count = 0;
    int ids_current = ids_idx + 1;
    for (int m = 0; m < ids_size; m++) {
        if (ids_current >= r || tokens[ids_current].type != JSMN_STRING) break;
        char id_str[32];
        json_parse_string(g_json_str, &tokens[ids_current], id_str);

        Monitor *mon = NULL;
        for (int i = 0; i < MONITORS_NUM_MAX; i++) {
            if (strcmp(monitors_arr[i].id, id_str) == 0) {
                mon = &monitors_arr[i];
                break;
            }
        }
        if (mon) {
            if (results_count > 0) {
                if (results_used + 1 < results_buf_size) {
                    results_buf[results_used++] = ',';
                    results_buf[results_used] = '\0';
                    results_remain = results_buf_size - results_used;
                }
            }
            uint32_t ts_ms = HAL_GetTick();
            float conf = get_confidence(mon->state);
            n = snprintf(results_buf + results_used, results_remain,
                         "{\"id\":\"%s\",\"state\":\"%s\",\"state_code\":%d,\"confidence\":%.2f,\"ts_ms\":%lu}\r\n",
                         mon->id, state_to_str(mon->state), state_to_code(mon->state), conf, (unsigned long)ts_ms);
            if (n < 0 || (size_t)n >= results_remain) {
                break;
            }
            results_used += (size_t)n;
            results_remain = results_buf_size - results_used;
            results_count++;
        }
        ids_current++;
    }
    if (results_used + 2 < results_buf_size) {
        results_buf[results_used++] = ']';
        results_buf[results_used] = '\0';
    } else {
        snprintf(results_buf, results_buf_size, "[]");
    }

    // size_t events_buf_size = 192;
    // char *events_buf = malloc(events_buf_size);
    // if (!events_buf) {
    //     free(results_buf);
    //     return ERR_INVALID_DATA;
    // }
    // if (get_latest_event(monitors_event, events_buf, events_buf_size) != 0) {
    //     strcpy(events_buf, "[]");
    // }
    // uint8_t pending_events = events_count - 1;
    // lấy các sự kiện (events)
    int events_to_take = events_count;
    if (events_to_take > 5) events_to_take = 5;

    size_t events_buf_size = 32 + (size_t)events_to_take * 140;
    if (events_buf_size < 128) events_buf_size = 128;
    char *events_buf = (char *)malloc(events_buf_size);
    if (!events_buf) {
        return ERR_INVALID_CMD;
    }
    build_events_json(events_buf, events_buf_size, events_to_take);

    int pending_events = events_count;

    // Build response
    snprintf(response, response_size, "{\"cmd\":\"ok\",\"seq\":%lu,\"data\":{\"results\":%s},\"events\":%s,\"pending\":%d}\r\n",
             seq, results_buf, events_buf, pending_events);
    free(results_buf);
    free(events_buf);
    return ERR_NONE;
}



json_err_t handle_read_snapshot(jsmntok_t *tokens, int token_count, char *response, size_t response_size) {
    if (!g_json_str || !tokens || token_count <= 0) {
        return ERR_INVALID_CMD;
    }

    // Tìm "seq"
    uint32_t seq = find_seq_number(tokens, token_count);

    // lấy tick ms
    uint32_t uptime_ms = HAL_GetTick();

    // Tính số object cần ghi
    int num_objects = 0;
    for (int i = 0; i < MONITORS_NUM_MAX; i++) {
        if (monitors_arr[i].id[0] != '\0') num_objects++;
    }

    size_t objects_buf_size = 32 + (size_t)num_objects * 96;
    if (objects_buf_size < 256) objects_buf_size = 256;
    char *objects_buf = (char *)malloc(objects_buf_size);
    if (!objects_buf) {
        return ERR_INVALID_CMD;
    }
    objects_buf[0] = '\0';
    size_t obj_remain = objects_buf_size;
    size_t obj_used = 0;

    int object_count = 0;
    for (int i = 0; i < MONITORS_NUM_MAX; i++) {
        if (monitors_arr[i].id[0] == '\0') continue;
        if (object_count > 0) {
            if (obj_used + 1 < objects_buf_size) {
                objects_buf[obj_used++] = ',';
                objects_buf[obj_used] = '\0';
                obj_remain = objects_buf_size - obj_used;
            }
        }
        const char *type_str = monitor_type_to_str(monitors_arr[i].type);
        const char *state_str = state_to_str(monitors_arr[i].state);
        int n = snprintf(objects_buf + obj_used, obj_remain,
                         "{\"id\":\"%s\",\"type\":\"%s\",\"state\":\"%s\",\"state_code\":%d}",
                         monitors_arr[i].id, type_str, state_str, state_to_code(monitors_arr[i].state));
        if (n < 0 || (size_t)n >= obj_remain) {
            break;
        }
        obj_used += (size_t)n;
        obj_remain = objects_buf_size - obj_used;
        object_count++;
    }
    
    int events_to_take = events_count;
    if (events_to_take > 5) events_to_take = 5;

    size_t events_buf_size = 32 + (size_t)events_to_take * 140;
    if (events_buf_size < 128) events_buf_size = 128;
    char *events_buf = (char *)malloc(events_buf_size);
    if (!events_buf) {
        free(objects_buf);
        return ERR_INVALID_CMD;
    }

    build_events_json(events_buf, events_buf_size, events_to_take);
    int pending_events = events_count;

    const char *overflow_str = (events_count >= EVENTS_HISTORY_MAX) ? "true" : "false";

    //build response
    snprintf(response, response_size,
             "{\"cmd\":\"ok\",\"seq\":%lu,\"data\":{\"ts_ms\":%lu,\"buf_overflow\":%s,\"objects\":[%s]},\"events\":%s,\"pending\":%d}\r\n",
             (unsigned long)seq, (unsigned long)uptime_ms, overflow_str, objects_buf, events_buf, pending_events);

    free(objects_buf);
    free(events_buf);
    return ERR_NONE;
}

json_err_t handle_poll(jsmntok_t *tokens, int token_count, char *response, size_t response_size) {
    if (!g_json_str || !tokens || token_count <= 0) {
        return ERR_INVALID_CMD;
    }

    // Tìm "seq"
    uint32_t seq = find_seq_number(tokens, token_count);

    // tính queue depth
    int queue_depth = 0;
    queue_depth = events_count;

    // lấy các sự kiện (events)
    int events_to_take = events_count;
    if (events_to_take > 5) events_to_take = 5;

    size_t events_buf_size = 32 + (size_t)events_to_take * 140;
    if (events_buf_size < 128) events_buf_size = 128;
    char *events_buf = (char *)malloc(events_buf_size);
    if (!events_buf) {
        return ERR_INVALID_CMD;
    }
    build_events_json(events_buf, events_buf_size, events_to_take);

    int pending_events = events_count;
    snprintf(response, response_size, "{\"cmd\":\"ok\",\"seq\":%ld,\"data\":{\"queue_depth\":%d},\"events\":%s,\"pending\":%d}\r\n", 
            seq, queue_depth, events_buf, pending_events);
    free(events_buf);
    return ERR_NONE;
}

json_err_t handle_flush_events(jsmntok_t *tokens, int token_count, char *response, size_t response_size) {
     if (!g_json_str || !tokens || token_count <= 0) {
        return ERR_INVALID_CMD;
    }

    // Tìm "seq"
    uint32_t seq = find_seq_number(tokens, token_count);

    // lấy các sự kiện (events)
    int events_to_take = events_count;
    if (events_to_take > 5) events_to_take = 5;

    size_t events_buf_size = 32 + (size_t)events_to_take * 140;
    if (events_buf_size < 128) events_buf_size = 128;
    char *events_buf = (char *)malloc(events_buf_size);
    if (!events_buf) {
        return ERR_INVALID_CMD;
    }
    build_events_json(events_buf, events_buf_size, events_to_take);

    int pending_events = events_count;
    snprintf(response, response_size, "{\"cmd\":\"ok\",\"seq\":%ld,\"data\":null,\"events\":%s,\"pending\":%d}\r\n", 
            seq, events_buf, pending_events);
    free(events_buf);
    return ERR_NONE;
}

json_err_t handle_relay_set(jsmntok_t *tokens, int token_count, char *response, size_t response_size) {
    if (!g_json_str || !tokens || token_count <= 0) {
        return ERR_INVALID_CMD;
    }

    uint32_t seq = find_seq_number(tokens, token_count);

    // Tìm "data"
    int data_idx = -1;
    for (int i = 1; i < token_count; i++) {
        if (jsoneq(g_json_str, &tokens[i], "data") == 0 && tokens[i+1].type == JSMN_OBJECT) {
            data_idx = i + 1;
            break;
        }
    }
    if (data_idx == -1) {
        return ERR_INVALID_CMD;
    }

    // Trong data, tìm relay_id và state
    int relay_id_idx = -1;
    int state_idx = -1;
    int data_size = tokens[data_idx].size;
    int data_start = data_idx + 1;
    for (int k = 0; k < data_size; k++) {
        int key_idx = data_start + k * 2;
        int val_idx = data_start + k * 2 + 1;
        if (key_idx >= token_count || val_idx >= token_count) break;

        if (jsoneq(g_json_str, &tokens[key_idx], "relay_id") == 0 && tokens[val_idx].type == JSMN_STRING) {
            relay_id_idx = val_idx;
        } else if (jsoneq(g_json_str, &tokens[key_idx], "state") == 0 && tokens[val_idx].type == JSMN_PRIMITIVE) {
            state_idx = val_idx;
        }
    }

    if (relay_id_idx == -1 || state_idx == -1) {
        return ERR_INVALID_CMD;
    }

    char relay_id[32] = {0};
    json_parse_string(g_json_str, &tokens[relay_id_idx], relay_id);

    int state_len = tokens[state_idx].end - tokens[state_idx].start;
    const char *state_ptr = g_json_str + tokens[state_idx].start;
    uint8_t relay_on;
    if (state_len == 4 && strncmp(state_ptr, "true", 4) == 0) {
        relay_on = 1;
    } else if (state_len == 5 && strncmp(state_ptr, "false", 5) == 0) {
        relay_on = 0;
    } else {
        return ERR_INVALID_DATA;
    }

    Button_TypeDef button;
    if (!relay_name_to_button(relay_id, &button)) {
        return ERR_INVALID_DATA;
    }

    Relay_output_TypeDef *ro = relay_output_get(button);
    if (!ro) {
        return ERR_INVALID_DATA;
    }

    __disable_irq();
    int busy = (ro->state == RELAY_PULSE || ro->state == RELAY_GAP || ro->in_seq);
    __enable_irq();
    if (busy) {
        return ERR_BUSY;
    }

    if (relay_on) {
        press_button(&hi2c3, button);
    } else {
        release_button(&hi2c3, button);
    }

    __disable_irq();
    ro->state = relay_on ? RELAY_ON : RELAY_IDLE;
    ro->deadline_ms = 0;
    ro->in_seq = 0;
    ro->seq_pos = 0;
    ro->seq_count = 0;
    __enable_irq();

    // lấy các sự kiện (events)
    int events_to_take = events_count;
    if (events_to_take > 5) events_to_take = 5;

    size_t events_buf_size = 32 + (size_t)events_to_take * 140;
    if (events_buf_size < 128) events_buf_size = 128;
    char *events_buf = (char *)malloc(events_buf_size);
    if (!events_buf) {
        return ERR_INVALID_CMD;
    }
    build_events_json(events_buf, events_buf_size, events_to_take);

    int pending_events = events_count;

    snprintf(response, response_size,
             "{\"cmd\":\"ok\",\"seq\":%lu,\"data\":null,\"events\":%s,\"pending\":%d}\r\n",
             (unsigned long)seq, events_buf, pending_events);

    free(events_buf);
    return ERR_NONE;
}

json_err_t handle_relay_pulse(jsmntok_t *tokens, int token_count, char *response, size_t response_size){
    if (!g_json_str || !tokens || token_count <= 0) {
        return ERR_INVALID_CMD;
    }
    uint32_t seq = find_seq_number(tokens, token_count);

    // Tìm "data"
    int data_idx = -1;
    for (int i = 1; i < token_count; i++) {
        if (jsoneq(g_json_str, &tokens[i], "data") == 0 && tokens[i+1].type == JSMN_OBJECT) {
            data_idx = i + 1;
            break;
        }
    }
    if (data_idx == -1) {
        return ERR_INVALID_CMD;
    }

    // Trong data, tìm relay_id và pulse_ms
    int relay_id_idx = -1;
    int pulse_ms_idx = -1;
    int data_size = tokens[data_idx].size;
    int data_start = data_idx + 1;
    for (int k = 0; k < data_size; k++) {
        int key_idx = data_start + k * 2;
        int val_idx = data_start + k * 2 + 1;
        if (key_idx >= token_count || val_idx >= token_count) break;

        if (jsoneq(g_json_str, &tokens[key_idx], "relay_id") == 0 && tokens[val_idx].type == JSMN_STRING) {
            relay_id_idx = val_idx;
        } else if (jsoneq(g_json_str, &tokens[key_idx], "pulse_ms") == 0 && tokens[val_idx].type == JSMN_PRIMITIVE) {
            pulse_ms_idx = val_idx;
        }
    }

    if (relay_id_idx == -1 || pulse_ms_idx == -1) {
        return ERR_INVALID_CMD;
    }

    char relay_id[32] = {0};
    json_parse_string(g_json_str, &tokens[relay_id_idx], relay_id);

    uint64_t pulse_ms_u64 = 0;
    if (!json_parse_uint64(g_json_str, &tokens[pulse_ms_idx], &pulse_ms_u64)) {
        return ERR_INVALID_DATA;
    }
    if (pulse_ms_u64 == 0 || pulse_ms_u64 > 60000U) {
        return ERR_PULSE_RANGE;
    }
    uint32_t pulse_ms = (uint32_t)pulse_ms_u64;

    Button_TypeDef button;
    if (!relay_name_to_button(relay_id, &button)) {
        return ERR_INVALID_DATA;
    }

    Relay_output_TypeDef *ro = relay_output_get(button);
    if (!ro) {
        return ERR_INVALID_DATA;
    }

    __disable_irq();
    int busy = (ro->state != RELAY_IDLE);
    __enable_irq();
    if (busy) {
        return ERR_BUSY;
    }

    press_button(&hi2c3, button);

    __disable_irq();
    ro->state = RELAY_PULSE;
    ro->deadline_ms = tim6_tick_ms + pulse_ms;
    ro->in_seq = 0;
    ro->seq_pos = 0;
    ro->seq_count = 0;
    __enable_irq();

    // lấy các sự kiện (events)
    int events_to_take = events_count;
    if (events_to_take > 5) events_to_take = 5;

    size_t events_buf_size = 32 + (size_t)events_to_take * 140;
    if (events_buf_size < 128) events_buf_size = 128;
    char *events_buf = (char *)malloc(events_buf_size);
    if (!events_buf) {
        return ERR_INVALID_CMD;
    }
    build_events_json(events_buf, events_buf_size, events_to_take);

    int pending_events = events_count;

    snprintf(response, response_size,
             "{\"cmd\":\"ok\",\"seq\":%lu,\"data\":null,\"events\":%s,\"pending\":%d}\r\n",
             (unsigned long)seq, events_buf, pending_events);

    free(events_buf);
    return ERR_NONE;
}

json_err_t handle_relay_pulse_seq(jsmntok_t *tokens, int token_count, char *response, size_t response_size){
    if (!g_json_str || !tokens || token_count <= 0) {
        return ERR_INVALID_CMD;
    }

    uint32_t seq = find_seq_number(tokens, token_count);

    // Tìm "data"
    int data_idx = -1;
    for (int i = 1; i < token_count; i++) {
        if (jsoneq(g_json_str, &tokens[i], "data") == 0 && tokens[i+1].type == JSMN_OBJECT) {
            data_idx = i + 1;
            break;
        }
    }
    if (data_idx == -1) {
        return ERR_INVALID_CMD;
    }

    int relay_id_idx = -1;
    int sequence_idx = -1;
    int data_size = tokens[data_idx].size;
    int data_start = data_idx + 1;
    for (int k = 0; k < data_size; k++) {
        int key_idx = data_start + k * 2;
        int val_idx = data_start + k * 2 + 1;
        if (key_idx >= token_count || val_idx >= token_count) break;

        if (jsoneq(g_json_str, &tokens[key_idx], "relay_id") == 0 && tokens[val_idx].type == JSMN_STRING) {
            relay_id_idx = val_idx;
        } else if (jsoneq(g_json_str, &tokens[key_idx], "sequence") == 0 && tokens[val_idx].type == JSMN_ARRAY) {
            sequence_idx = val_idx;
        }
    }

    if (relay_id_idx == -1 || sequence_idx == -1) {
        return ERR_INVALID_CMD;
    }

    char relay_id[32] = {0};
    json_parse_string(g_json_str, &tokens[relay_id_idx], relay_id);

    Button_TypeDef button;
    if (!relay_name_to_button(relay_id, &button)) {
        return ERR_INVALID_DATA;
    }

    int seq_count = tokens[sequence_idx].size;
    if (seq_count <= 0) {
        return ERR_INVALID_DATA;
    }
    if (seq_count > RELAY_PULSE_SEQ_MAX_STEPS) {
        return ERR_PULSE_RANGE;
    }

    uint32_t pulse_list[RELAY_PULSE_SEQ_MAX_STEPS] = {0};
    uint32_t gap_list[RELAY_PULSE_SEQ_MAX_STEPS] = {0};

    int current = sequence_idx + 1;
    for (int s = 0; s < seq_count; s++) {
        if (current >= token_count || tokens[current].type != JSMN_OBJECT) {
            return ERR_INVALID_DATA;
        }

        int obj_size = tokens[current].size;
        int obj_start = current + 1;
        uint8_t has_pulse = 0;
        uint8_t has_gap = 0;

        for (int k = 0; k < obj_size; k++) {
            int key_idx = obj_start + k * 2;
            int val_idx = obj_start + k * 2 + 1;
            if (key_idx >= token_count || val_idx >= token_count) break;

            if (jsoneq(g_json_str, &tokens[key_idx], "pulse_ms") == 0 && tokens[val_idx].type == JSMN_PRIMITIVE) {
                uint64_t v = 0;
                if (!json_parse_uint64(g_json_str, &tokens[val_idx], &v) || v == 0 || v > 60000U) {
                    return ERR_PULSE_RANGE;
                }
                pulse_list[s] = (uint32_t)v;
                has_pulse = 1;
            } else if (jsoneq(g_json_str, &tokens[key_idx], "gap_ms") == 0 && tokens[val_idx].type == JSMN_PRIMITIVE) {
                uint64_t v = 0;
                if (!json_parse_uint64(g_json_str, &tokens[val_idx], &v) || v > 60000U) {
                    return ERR_PULSE_RANGE;
                }
                gap_list[s] = (uint32_t)v;
                has_gap = 1;
            }
        }

        if (!has_pulse || !has_gap) {
            return ERR_INVALID_DATA;
        }

        current += 1 + obj_size * 2;
    }

    Relay_output_TypeDef *ro = relay_output_get(button);
    if (!ro) {
        return ERR_INVALID_DATA;
    }

    __disable_irq();
    int busy = (ro->state != RELAY_IDLE);
    __enable_irq();
    if (busy) {
        return ERR_BUSY;
    }

    /* Setup sequence parameters within critical section */
    __disable_irq();
    ro->in_seq = 1;
    ro->seq_pos = 0;
    ro->seq_count = (uint8_t)seq_count;
    for (int i = 0; i < seq_count; i++) {
        ro->pulse_ms[i] = pulse_list[i];
        ro->gap_ms[i] = gap_list[i];
    }
    __enable_irq();

    press_button(&hi2c3, button);

    __disable_irq();
    ro->state = RELAY_PULSE;
    ro->deadline_ms = tim6_tick_ms + ro->pulse_ms[0];
    __enable_irq();

    // lấy các sự kiện (events)
    int events_to_take = events_count;
    if (events_to_take > 5) events_to_take = 5;

    size_t events_buf_size = 32 + (size_t)events_to_take * 140;
    if (events_buf_size < 128) events_buf_size = 128;
    char *events_buf = (char *)malloc(events_buf_size);
    if (!events_buf) {
        return ERR_INVALID_CMD;
    }
    build_events_json(events_buf, events_buf_size, events_to_take);

    int pending_events = events_count;

    snprintf(response, response_size,
             "{\"cmd\":\"ok\",\"seq\":%lu,\"data\":null,\"events\":%s,\"pending\":%d}\r\n",
             (unsigned long)seq, events_buf, pending_events);

    free(events_buf);
    return ERR_NONE;
}

json_err_t handle_reset(jsmntok_t *tokens, int token_count, char *response, size_t response_size) {
    if (!g_json_str || !tokens || token_count <= 0) {
        return ERR_INVALID_CMD;
    }

    uint32_t seq = find_seq_number(tokens, token_count);

    // Tìm "data"
    int data_idx = -1;
    for (int i = 1; i < token_count; i++) {
        if (jsoneq(g_json_str, &tokens[i], "data") == 0 && (i + 1) < token_count) {
            data_idx = i + 1;
            break;
        }
    }
    if (data_idx == -1) {
        return ERR_INVALID_CMD;
    }

    // reset command yêu cầu data:null
    int data_len = tokens[data_idx].end - tokens[data_idx].start;
    if (tokens[data_idx].type != JSMN_PRIMITIVE ||
        data_len != 4 ||
        strncmp(g_json_str + tokens[data_idx].start, "null", 4) != 0) {
        return ERR_INVALID_DATA;
    }

    uint32_t relays_cleared = 0;
    for (int i = 0; i < RELAY_OUTPUT_NUM_MAX; i++) {
        Relay_output_TypeDef *ro = &relay_output_arr[i];
        uint8_t was_active = (ro->state == RELAY_PULSE || ro->state == RELAY_GAP || ro->in_seq);

        ro->state = RELAY_IDLE;
        ro->in_seq = 0;
        ro->seq_pos = 0;
        ro->seq_count = 0;
        ro->deadline_ms = 0;

        /* Always release output to ensure relay tắt */
        release_button(&hi2c3, ro->button);

        if (was_active) {
            relays_cleared++;
        }
    }

    int queue_cleared = events_count;
    events_count = 0;
    events_index = 0;

    snprintf(response, response_size,
             "{\"cmd\":\"ok\",\"seq\":%lu,\"data\":{\"relays_cleared\":%lu,\"queue_cleared\":%u},\"events\":[],\"pending\":0}",
             (unsigned long)seq, relays_cleared, queue_cleared);

    return ERR_NONE;

}