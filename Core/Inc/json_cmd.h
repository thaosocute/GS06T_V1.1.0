// json_cmd.h
#pragma once
#include <stdint.h>
#include <stdbool.h>

#define FIRMWARE_VERSION "1.0.0"

typedef enum {
    CMD_UNKNOWN = 0,
    CMD_MONITOR_CONFIG,
    CMD_READ_PATTERN,
    CMD_READ_SNAPSHOT,
    CMD_POLL,
    CMD_FLUSH_EVENTS,
    CMD_PING,
    CMD_RELAY_SET,
    CMD_RELAY_PULSE,
    CMD_RELAY_PULSE_SEQ,
    CMD_RESET
} json_cmd_t;

typedef enum {
    ERR_NONE = 0,
    ERR_INVALID_CMD,
    ERR_INVALID_ID,
    ERR_INVALID_DATA,
    ERR_JSON_PARSE,
    ERR_PULSE_RANGE,
    ERR_BUSY,
    ERR_HW_FAULT
} json_err_t;

typedef enum {
    // mã chung
    OFF = 0,
    ON = 1,
    DETECTING = 98,
    UNKNOWN = 99,
    // mã led đơn màu
    BLINK_1HZ = 10,
    BLINK_2HZ = 11,
    BLINK_4HZ = 12,
    BLINK_5HZ,
    BLINK_8HZ,
    BLINK_0_25HZ,
    BLINK_1_PER_MIN,
    BLINK_2_PER_MIN, 
    BLINK_3_PER_MIN,
    // mã led đa màu
    ON_BLUE = 30,
    ON_ORANGE,
    ON_PURPLE,
    BLINK_BLUE_1HZ,
    BLINK_BLUE_5HZ,
    BLINK_BLUE_0_25HZ,
    BLINK_RED_1HZ,
    BLINK_RED_5HZ,
    BLINK_ORANGE_1HZ,
    BLINK_ORANGE_5HZ,
    BLINK_PURPLE_1HZ,
    BLINK_PURPLE_5HZ,
    // mã buzzer
    BEEP_ONCE = 50,
    BEEP_ONCE_EXIT = 51,
    BEEP_1_PER_5S,
    ALARM_CONTINUOUS,
    BEEP_3_PER_1S5,
    BEEP_1_PER_0S5_8S,
    // mã relay output DUT
    OPEN = 60,
    CLOSE
} json_monitor_state_t;


const char *json_cmd_to_str(json_cmd_t cmd);
json_cmd_t json_cmd_from_str(const char *s);
const char *json_err_to_code(json_err_t err);
const char *state_to_str(json_monitor_state_t state);