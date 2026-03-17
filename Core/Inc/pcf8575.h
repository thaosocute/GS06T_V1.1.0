#ifndef __PCF8575_H
#define __PCF8575_H

#include "main.h"
void pcf8575_i2c_start(void);
void pcf8575_i2c_stop(void);
uint8_t pcf8575_i2c_wait_ack(void);
void pcf8575_write_pin(I2C_HandleTypeDef *hi2c, uint8_t pin, uint8_t level);
void pcf8575_reset_state(I2C_HandleTypeDef *hi2c);

#endif /* __PCF8575_H */