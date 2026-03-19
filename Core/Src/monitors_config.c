#include "monitors_config.h"
#include "input.h"
#include <string.h>
#include <stdlib.h>  // for strtol
#include <stdio.h>   // for snprintf

Monitor monitors_arr[MONITORS_NUM_MAX];

static const char *g_json_str = NULL;

extern monitor_event_t monitors_event[EVENTS_HISTORY_MAX];
extern int events_index;
extern int events_count;

void monitors_set_json(const char *json_str) {
    g_json_str = json_str;
}

void monitor_set_state_event() {
    for(uint8_t i = 0; i < 16; i++) {
        update_monitor_state(&monitors_arr[i]);
    }
}

static uint32_t find_seq_number(jsmntok_t *tokens, int token_count) {
    uint32_t seq = 0;
    for (int i = 1; i < token_count; i++) {
        if (jsoneq(g_json_str, &tokens[i], "seq") == 0) {
            uint64_t val;
            json_parse_uint64(g_json_str, &tokens[i+1], &val);
            seq = (uint32_t)val;
            return seq;
        }
    }
}

uint8_t handle_monitors_config(jsmntok_t *tokens, int token_count, char *response, size_t response_size) {
    if (!g_json_str || !tokens || token_count <= 0) {
        return -1;
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

    // Tìm key "monitors"
    int monitors_idx = -1;
    for (int i = 1; i < r; i++) {
        if (jsoneq(g_json_str, &tokens[i], "monitors") == 0 && tokens[i+1].type == JSMN_ARRAY) {
            monitors_idx = i + 1;  // token của array
            break;
        }
    }
    if (monitors_idx == -1) {
        //snprintf(response, response_size, "{\"cmd\":\"error\",\"seq\":%u,\"data\":null,\"events\":[],\"pending\":0}", seq);
        return -1;
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
                if (mon->type == MON_LED) mon->cfg.led.pin = (uint8_t)val;
                else if (mon->type == MON_BUZZER) mon->cfg.buzzer.pin = (uint8_t)val;
                else if (mon->type == MON_RELAY) mon->cfg.relay.pin = (uint8_t)val;
            } else if (jsoneq(g_json_str, &tokens[key_idx], "pin_r") == 0) {
                uint64_t val;
                json_parse_uint64(g_json_str, &tokens[val_idx], &val);
                mon->cfg.led_mc.pin_r = (uint8_t)val;
            } else if (jsoneq(g_json_str, &tokens[key_idx], "pin_g") == 0) {
                uint64_t val;
                json_parse_uint64(g_json_str, &tokens[val_idx], &val);
                mon->cfg.led_mc.pin_g = (uint8_t)val;
            } else if (jsoneq(g_json_str, &tokens[key_idx], "pin_b") == 0) {
                uint64_t val;
                json_parse_uint64(g_json_str, &tokens[val_idx], &val);
                mon->cfg.led_mc.pin_b = (uint8_t)val;
            } else if (jsoneq(g_json_str, &tokens[key_idx], "pin_orange") == 0) {
                uint64_t val;
                json_parse_uint64(g_json_str, &tokens[val_idx], &val);
                mon->cfg.led_mc.pin_orange = (uint8_t)val;
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
    snprintf(response, response_size, "{\"cmd\":\"ok\",\"seq\":%lu,\"data\":null,\"events\":[],\"pending\":0}", seq);
    return arr_size;  // số monitors parsed
}

uint8_t handle_ping(jsmntok_t *tokens, int token_count, char *response, size_t response_size) {
    if (!g_json_str || !tokens || token_count <= 0) {
        return -1;
    }

    int r = token_count;

    // Tìm "seq"
    uint32_t seq = 0;
    uint32_t pending = 0;
    const char *events_str = "[]";
    char events_buf[128] = {0};

    for (int i = 1; i < r; i++) {
        if (jsoneq(g_json_str, &tokens[i], "seq") == 0 && (i + 1) < r) {
            uint64_t val;
            if (json_parse_uint64(g_json_str, &tokens[i + 1], &val)) {
                seq = (uint32_t)val;
            }
        } else if (jsoneq(g_json_str, &tokens[i], "pending") == 0 && (i + 1) < r) {
            uint64_t val;
            if (json_parse_uint64(g_json_str, &tokens[i + 1], &val)) {
                pending = (uint32_t)val;
            }
        } else if (jsoneq(g_json_str, &tokens[i], "events") == 0 && (i + 1) < r) {
            int start = tokens[i + 1].start;
            int end = tokens[i + 1].end;
            int len = end - start;
            if (len > 0 && len < (int)sizeof(events_buf)) {
                memcpy(events_buf, g_json_str + start, len);
                events_buf[len] = '\0';
                events_str = events_buf;
            }
        }
    }

    // count configured monitors (non-empty id signifies an entry)
    int num_monitors = 0;
    for (int i = 0; i < 16; i++) {
        if (monitors_arr[i].id[0] != '\0') {
            num_monitors++;
        }
    }

    uint32_t uptime_ms = HAL_GetTick();

    snprintf(response, response_size,
             "{\"cmd\":\"ok\",\"seq\":%lu,\"data\":{\"uptime_ms\":%lu,\"fw_version\":\"%s\",\"num_monitors\":%d},\"events\":%s,\"pending\":%lu}",
             seq, uptime_ms, FIRMWARE_VERSION, num_monitors, events_str, pending);

    return 0;
}

static int state_to_code(json_monitor_state_t state) {
    return (int)state;
}

static float get_confidence(json_monitor_state_t state) {
    if (state == DETECTING || state == UNKNOWN) return 0.0f;
    if (state >= BLINK_1HZ && state <= BLINK_3_PER_MIN) return 0.94f; // blink states
    return 1.0f;
}

static const char* monitor_type_to_str(MonitorType type) {
    switch (type) {
        case MON_LED: return "led";
        case MON_LED_MULTICOLOR: return "led_multicolor";
        case MON_BUZZER: return "buzzer";
        case MON_RELAY: return "relay";
        default: return "unknown";
    }
}

uint8_t handle_read_pattern(jsmntok_t *tokens, int token_count, char *response, size_t response_size) {
    if (!g_json_str || !tokens || token_count <= 0) {
        return -1;
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
        return -1;
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
        return -1;
    }

    int ids_size = tokens[ids_idx].size;
    if (ids_size > 16) ids_size = 16; // limit

    // Build results
    char results_buf[1024] = {0};
    strcpy(results_buf, "[");
    int results_count = 0;
    int ids_current = ids_idx + 1;
    for (int m = 0; m < ids_size; m++) {
        if (ids_current >= r || tokens[ids_current].type != JSMN_STRING) break;
        char id_str[32];
        json_parse_string(g_json_str, &tokens[ids_current], id_str);

        // Find monitor
        Monitor *mon = NULL;
        for (int i = 0; i < 16; i++) {
            if (strcmp(monitors_arr[i].id, id_str) == 0) {
                mon = &monitors_arr[i];
                break;
            }
        }
        if (mon) {
            if (results_count > 0) strcat(results_buf, ",");
            char result[256];
            uint32_t ts_ms = HAL_GetTick();
            float conf = get_confidence(mon->state);
            snprintf(result, sizeof(result), "{\"id\":\"%s\",\"state\":\"%s\",\"state_code\":%d,\"confidence\":%.2f,\"ts_ms\":%lu}",
                     mon->id, state_to_str(mon->state), state_to_code(mon->state), conf, ts_ms);
            strcat(results_buf, result);
            results_count++;
        }
        ids_current++;
    }
    strcat(results_buf, "]");

    // Get latest event
    char events_buf[256] = "[]";
    if (events_count > 0) {
        int latest_idx = (events_index - 1 + EVENTS_HISTORY_MAX) % EVENTS_HISTORY_MAX;
        monitor_event_t *event = &monitors_event[latest_idx];
        // Find monitor type
        const char *type_str = "unknown";
        for (int i = 0; i < 16; i++) {
            if (strcmp(monitors_arr[i].id, event->monitor_id) == 0) {
                type_str = monitor_type_to_str(monitors_arr[i].type);
                break;
            }
        }
        snprintf(events_buf, sizeof(events_buf), "[{\"id\":\"%s\",\"type\":\"%s\",\"state\":\"%s\",\"prev\":\"%s\",\"ts_ms\":%lu}]",
                 event->monitor_id, type_str, state_to_str(event->current_state), state_to_str(event->prev_state), event->event_time);
    }

    // Build response
    snprintf(response, response_size, "{\"cmd\":\"ok\",\"seq\":%lu,\"data\":{\"results\":%s},\"events\":%s,\"pending\":0}",
             seq, results_buf, events_buf);

    return 0;
}

uint8_t handle_read_snapshot(jsmntok_t *tokens, int token_count, char *response, size_t response_size) {
    if (!g_json_str || !tokens || token_count <= 0) {
        return -1;
    }
    int r = token_count;

    // Tìm "seq"
    uint32_t seq = find_seq_number(tokens, r);

    // lấy tick ms
    uint32_t uptime_ms = HAL_GetTick();

    // kiểm tra hàng chờ event xem có overflow không
    char buf_overflow_string[5];
}