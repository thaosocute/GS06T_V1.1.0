#ifndef INPUT_OUTPUT_H
#define INPUT_OUTPUT_H

#include "main.h"

typedef enum {
    // main relays
    RL1 = 1,
    RL2,
    RL3,
    RL4,
    RL5,
    RL6,
    RL7,
    RL8,
    RL9,
    RL10,
    RL11,
    RL12,
    RL13,
    RL14,
    // back up relays
    BR1,
    BR2,
    BR3,
    BR4,
    BR5,
    BR6,
} Button_TypeDef;

typedef struct {
    const char* cmd_name;
    Button_TypeDef button;
} Command_Button_Map;

typedef enum {
    IN1 = 1,
    IN2,
    IN3,
    IN4,
    IN5,
    IN6,
    IN7,
    IN8,
    IN9,
    IN10,
    IN11,
    IN12,
    IN13,
    IN14,
    IN15,
    IN16
} Input_TypeDef;

typedef struct {
    const char* cmd_name;
    Input_TypeDef input;
} Command_Input_Map;

#endif /* INPUT_OUTPUT_H */