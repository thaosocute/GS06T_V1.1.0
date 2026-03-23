#ifndef CMD_PROCESS_H
#define CMD_PROCESS_H
#include "main.h"
#include "pcf8575.h"
#include "max3485.h"
#include "ds1804.h"
#include "input_output.h"

/* simulate a short press when only a count is specified */
#define DEFAULT_PRESS_HOLD_MS 100

void press_button(I2C_HandleTypeDef* hi2c, Button_TypeDef button);
void release_button(I2C_HandleTypeDef* hi2c, Button_TypeDef button);
void set_potentiometer(DS1804_HandleTypeDef* hds1804, uint8_t position);

#endif /* CMD_PROCESS_H */