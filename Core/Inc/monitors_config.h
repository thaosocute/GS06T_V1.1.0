#ifndef MONITOR_CONFIG_H
#define MONITOR_CONFIG_H

#include "main.h"
#include "jsmn.h"
#include "json_cmd.h"
#include <stdint.h>
#include <stddef.h>

#define MONITORS_NUM_MAX 16

/* ── Enums ─────────────────────────────────────────────── */

typedef enum {
    MON_LED,
    MON_LED_MULTICOLOR,
    MON_BUZZER,
    MON_RELAY
} MonitorType;

typedef enum {
    MON_MODE_PATTERN,
    MON_MODE_EVENT
} MonitorMode;

typedef enum {
    ACTIVE_CLOSED,
    ACTIVE_OPEN
} ActiveState;

/* ── Config theo từng loại ─────────────────────────────── */

typedef struct {
    uint8_t pin;
} LedConfig;

typedef struct {
    uint8_t pin_r;
    uint8_t pin_b;
    uint8_t pin_orange;   /* chỉ O2 dùng, còn lại = 0 */
    uint8_t pin_g;
} LedMcConfig;

typedef struct {
    uint8_t pin;
} BuzzerConfig;

typedef struct {
    uint8_t     pin;
    ActiveState active_state;
} RelayConfig;

/* ── Tagged union ──────────────────────────────────────── */

typedef union {
    LedConfig   led;
    LedMcConfig led_mc;
    BuzzerConfig buzzer;
    RelayConfig relay;
} MonitorConfig;

/* ── Struct chính ──────────────────────────────────────── */

// typedef struct {
//     char          id[32];
//     MonitorType   type;
//     MonitorMode   mode;
//     uint32_t      observe_ms;   /* 0 nếu mode = EVENT */
//     uint16_t      debounce_ms;  /* 0 nếu mode = PATTERN */
//     MonitorConfig cfg;
// } Monitor;

typedef struct {
    char          id[32];
    MonitorType   type;
    MonitorMode   mode;

    union {
        uint32_t observe_ms;   /* dùng khi mode = PATTERN */
        uint16_t debounce_ms;  /* dùng khi mode = EVENT   */
    } timing;

    MonitorConfig cfg;
    json_monitor_state_t state;
} Monitor;

/// Set the current JSON buffer for handlers.
/// The token array passed to handlers must refer to this buffer.
void monitors_set_json(const char *json_str);
void monitor_set_state_event();

json_err_t handle_monitors_config(jsmntok_t *tokens, int token_count, char *response, size_t response_size);
json_err_t handle_ping(jsmntok_t *tokens, int token_count, char *response, size_t response_size);
json_err_t handle_read_pattern(jsmntok_t *tokens, int token_count, char *response, size_t response_size);
json_err_t handle_read_snapshot(jsmntok_t *tokens, int token_count, char *response, size_t response_size);
json_err_t handle_poll(jsmntok_t *tokens, int token_count, char *response, size_t response_size);
json_err_t handle_flush_events(jsmntok_t *tokens, int token_count, char *response, size_t response_size);

#endif //MONITOR_CONFIG_H