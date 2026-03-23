#ifndef DS1804_H
#define DS1804_H

#include "main.h"

typedef struct {
    GPIO_TypeDef* UD_GPIO_Port;
    uint16_t UD_Pin;
    GPIO_TypeDef* CS_GPIO_Port;
    uint16_t CS_Pin;
    GPIO_TypeDef* INC_GPIO_Port;
    uint16_t INC_Pin;
} DS1804_HandleTypeDef;

// Function prototypes
void DS1804_reset_to_zero(DS1804_HandleTypeDef* hds1804);
void DS1804_Init(DS1804_HandleTypeDef* hds1804);
void DS1804_set_direction(DS1804_HandleTypeDef* hds1804, uint8_t direction);
void DS1804_step(DS1804_HandleTypeDef* hds1804, uint16_t steps, uint32_t delay_us);
void DS1804_get_position(DS1804_HandleTypeDef* hds1804, uint8_t* position);
void DS1804_set_position(DS1804_HandleTypeDef* hds1804, uint8_t position);

#endif // DS1804_H