#ifndef INPUT_H
#define INPUT_H

#define EVENTS_HISTORY_MAX 16
#define INPUT_QUEUE_SIZE 100
#define SAMPLE_RATE_MS     50

#include "json_cmd.h"
#include "monitors_config.h"

typedef struct {
    char monitor_id[32];
    json_monitor_state_t current_state;
    json_monitor_state_t prev_state;
    uint32_t event_time;
    uint32_t pending_time;
} monitor_event_t;

typedef struct {
    uint32_t high_count;    /* số sample đọc được HIGH    */
    uint32_t low_count;     /* số sample đọc được LOW     */
    uint32_t transitions;   /* số lần thay đổi mức        */
    uint8_t pulse_num;      /* số xung*/
    float    freq_hz;       /* tần số ước tính            */
    float    duty_cycle;    /* tỉ lệ HIGH / tổng          */
    float    confident;
} PinAnalysis;

void push_input(uint16_t val);
void update_monitor_state(Monitor *monitor);

extern monitor_event_t monitors_event[EVENTS_HISTORY_MAX];
extern int events_index;
extern int events_count;


#endif //INPUT_H