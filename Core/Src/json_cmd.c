// json_cmd.c
#include "json_cmd.h"
#include <string.h>

const char *json_cmd_to_str(json_cmd_t cmd)
{
    switch (cmd) {
    case CMD_MONITOR_CONFIG: return "monitor_config";
    case CMD_READ_PATTERN:   return "read_pattern";
    case CMD_READ_SNAPSHOT:  return "read_snapshot";
    case CMD_POLL:           return "poll";
    case CMD_FLUSH_EVENTS:   return "flush_events";
    case CMD_RELAY_SET:      return "relay_set";
    case CMD_RELAY_PULSE:    return "relay_pulse";
    case CMD_RELAY_PULSE_SEQ:return "relay_pulse_seq";
    case CMD_PING:           return "ping";
    case CMD_RESET:          return "reset";
    default:                 return "unknown";
    }
}

json_cmd_t json_cmd_from_str(const char *s)
{
    if (!s) return CMD_UNKNOWN;
    if (strcmp(s, "monitor_config") == 0) return CMD_MONITOR_CONFIG;
    if (strcmp(s, "read_pattern")    == 0) return CMD_READ_PATTERN;
    if (strcmp(s, "read_snapshot")   == 0) return CMD_READ_SNAPSHOT;
    if (strcmp(s, "poll")            == 0) return CMD_POLL;
    if (strcmp(s, "flush_events")    == 0) return CMD_FLUSH_EVENTS;
    if (strcmp(s, "relay_set")       == 0) return CMD_RELAY_SET;
    if (strcmp(s, "relay_pulse")     == 0) return CMD_RELAY_PULSE;
    if (strcmp(s, "relay_pulse_seq") == 0) return CMD_RELAY_PULSE_SEQ;
    if (strcmp(s, "ping")            == 0) return CMD_PING;
    if (strcmp(s, "reset")           == 0) return CMD_RESET;
    return CMD_UNKNOWN;
}

const char *json_err_to_code(json_err_t err)
{
    switch (err) {
    case ERR_INVALID_CMD:  return "ERR_INVALID_CMD";
    case ERR_INVALID_ID:   return "ERR_INVALID_ID";
    case ERR_INVALID_DATA: return "ERR_INVALID_DATA";
    case ERR_JSON_PARSE:   return "ERR_JSON_PARSE";
    case ERR_PULSE_RANGE:  return "ERR_PULSE_RANGE";
    case ERR_BUSY:         return "ERR_BUSY";
    case ERR_HW_FAULT:     return "ERR_HW_FAULT";
    default:               return "ERR_NONE";
    }
}

const char* state_to_str(json_monitor_state_t state) {
    switch (state) {
        case OFF: return "off";
        case ON: return "on";
        case DETECTING: return "detecting";
        case UNKNOWN: return "unknown";
        case BLINK_1HZ: return "blink_1hz";
        case BLINK_2HZ: return "blink_2hz";
        case BLINK_4HZ: return "blink_4hz";
        case BLINK_5HZ: return "blink_5hz";
        case BLINK_8HZ: return "blink_8hz";
        case BLINK_0_25HZ: return "blink_0_25hz";
        case BLINK_1_PER_MIN: return "blink_1_per_min";
        case BLINK_2_PER_MIN: return "blink_2_per_min";
        case BLINK_3_PER_MIN: return "blink_3_per_min";
        case ON_BLUE: return "on_blue";
        case ON_ORANGE: return "on_orange";
        case ON_PURPLE: return "on_purple";
        case BLINK_BLUE_1HZ: return "blink_blue_1hz";
        case BLINK_BLUE_5HZ: return "blink_blue_5hz";
        case BLINK_BLUE_0_25HZ: return "blink_blue_0_25hz";
        case BLINK_RED_1HZ: return "blink_red_1hz";
        case BLINK_RED_5HZ: return "blink_red_5hz";
        case BLINK_ORANGE_1HZ: return "blink_orange_1hz";
        case BLINK_ORANGE_5HZ: return "blink_orange_5hz";
        case BLINK_PURPLE_1HZ: return "blink_purple_1hz";
        case BLINK_PURPLE_5HZ: return "blink_purple_5hz";
        case BEEP_ONCE: return "beep_once";
        case BEEP_ONCE_EXIT: return "beep_once_exit";
        case BEEP_1_PER_5S: return "beep_1_per_5s";
        case ALARM_CONTINUOUS: return "alarm_continuous";
        case BEEP_3_PER_1S5: return "beep_3_per_1s5";
        case BEEP_1_PER_0S5_8S: return "beep_1_per_0s5_8s";
        case OPEN: return "open";
        case CLOSE: return "closed";
        default: return "unknown";
    }
}