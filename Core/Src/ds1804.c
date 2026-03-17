#include "ds1804.h"

static volatile uint32_t gs_fac_us = 0;
static volatile uint8_t position = 0;  /* variable to keep track of current position of the potentiometer */

static void delay_us(uint32_t us)
{
    uint32_t ticks;
    uint32_t told;
    uint32_t tnow;
    uint32_t tcnt;
    uint32_t reload;
    
    /* set the used param */
    tcnt = 0;
    reload = SysTick->LOAD;
    ticks = us * gs_fac_us;
    told = SysTick->VAL;
    
    /* delay */
    while (1)
    {
        tnow = SysTick->VAL;
        if (tnow != told)
        {
            if (tnow < told)
            {
                tcnt += told - tnow;
            }
            else 
            {
                tcnt += reload - tnow + told;
            }
            told = tnow;
            if (tcnt >= ticks)
            {
                break;
            }
        }
    }
}

void DS1804_reset_to_zero(DS1804_HandleTypeDef* hds1804) {
    DS1804_set_direction(hds1804, 0);  /* set direction to decrement */
    DS1804_step(hds1804, 100, 10);  /* step down a large number of steps to ensure we reach zero */
    position = 0;  /* reset position tracking variable */   
}

void DS1804_Init(DS1804_HandleTypeDef* hds1804) {
    // Initialize GPIO pins for UD, CS, and INC
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    // Initialize UD pin
    GPIO_InitStruct.Pin = hds1804->UD_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(hds1804->UD_GPIO_Port, &GPIO_InitStruct);

    // Initialize CS pin
    GPIO_InitStruct.Pin = hds1804->CS_Pin;
    HAL_GPIO_Init(hds1804->CS_GPIO_Port, &GPIO_InitStruct);

    // Initialize INC pin
    GPIO_InitStruct.Pin = hds1804->INC_Pin;
    HAL_GPIO_Init(hds1804->INC_GPIO_Port, &GPIO_InitStruct);

    // Set initial state
    HAL_GPIO_WritePin(hds1804->CS_GPIO_Port, hds1804->CS_Pin, GPIO_PIN_SET);  // CS high (inactive)

    DS1804_reset_to_zero(hds1804);  // Reset the potentiometer to zero position
}

void DS1804_set_direction(DS1804_HandleTypeDef* hds1804, uint8_t direction) {
    if (direction == 0) {
        HAL_GPIO_WritePin(hds1804->UD_GPIO_Port, hds1804->UD_Pin, GPIO_PIN_RESET); // Set direction to decrement
    } else {
        HAL_GPIO_WritePin(hds1804->UD_GPIO_Port, hds1804->UD_Pin, GPIO_PIN_SET);   // Set direction to increment
    }
}

void DS1804_step(DS1804_HandleTypeDef* hds1804, uint16_t steps, uint32_t us) {
    // Activate the chip by pulling CS low
    HAL_GPIO_WritePin(hds1804->CS_GPIO_Port, hds1804->CS_Pin, GPIO_PIN_RESET);
    
    for (uint16_t i = 0; i < steps; i++) {
        // Generate a pulse on the INC pin
        HAL_GPIO_WritePin(hds1804->INC_GPIO_Port, hds1804->INC_Pin, GPIO_PIN_SET);   // INC high
        delay_us(us);  // Delay for the specified time
        HAL_GPIO_WritePin(hds1804->INC_GPIO_Port, hds1804->INC_Pin, GPIO_PIN_RESET); // INC low
        delay_us(us);  // Delay for the specified time
    }
    
    // Deactivate the chip by pulling CS high
    HAL_GPIO_WritePin(hds1804->CS_GPIO_Port, hds1804->CS_Pin, GPIO_PIN_SET);
}

void DS1804_get_position(DS1804_HandleTypeDef* hds1804, uint8_t* _position) {
    *_position = position;
}

void DS1804_set_position(DS1804_HandleTypeDef* hds1804, uint8_t new_position) {
    if (new_position > position) {
        DS1804_set_direction(hds1804, 1);  // Set direction to increment
        DS1804_step(hds1804, new_position - position, 100);  // Step up to the new position
    } else if (new_position < position) {
        DS1804_set_direction(hds1804, 0);  // Set direction to decrement
        DS1804_step(hds1804, position - new_position, 100);  // Step down to the new position
    }
    position = new_position;  // Update the current position variable
}