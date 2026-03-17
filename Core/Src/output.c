#include "output.h"

#define BR_GPIO_Port GPIOB  /* assuming backup relays are connected to GPIOB pins 3-8 */

static Command_Button_Map command_button_map[] = {
        {"RL1", RL1},
        {"RL2", RL2},
        {"RL3", RL3},
        {"RL4", RL4},
        {"RL5", RL5},
        {"RL6", RL6},
        {"RL7", RL7},
        {"RL8", RL8},
        {"RL9", RL9},
        {"RL10", RL10},
        {"RL11", RL11},
        {"RL12", RL12},
        {"RL13", RL13},
        {"RL14", RL14},
        {"BR1", BR1},
        {"BR2", BR2},
        {"BR3", BR3},
        {"BR4", BR4},
        {"BR5", BR5},
        {"BR6", BR6}
};

void press_button(I2C_HandleTypeDef* hi2c, Button_TypeDef button, uint32_t hold_time_ms)
{
    if(button < BR1 && button >= RL1)
    {
        pcf8575_write_pin(hi2c, button, 1);
    } else if(button >= BR1 && button <= BR6)
    {
        HAL_GPIO_WritePin(BR_GPIO_Port, GPIO_PIN_3 << (button - BR1), GPIO_PIN_SET);
    }
}

void release_button(I2C_HandleTypeDef* hi2c, Button_TypeDef button)
{
    if(button < BR1 && button >= RL1)
    {
        pcf8575_write_pin(hi2c, button, 0);
    } else if(button >= BR1 && button <= BR6)
    {
        HAL_GPIO_WritePin(BR_GPIO_Port, GPIO_PIN_3 << (button - BR1), GPIO_PIN_RESET);
    }
}

void set_potentiometer(DS1804_HandleTypeDef* hds1804, uint8_t position)
{
    DS1804_set_position(hds1804, position);
}

// void read_inputs(uint16_t *input_report, uint8_t *report_index) {
//     uint16_t input_state = 0;
//     input_state |= ((HAL_GPIO_ReadPin(IN1_GPIO_Port, IN1_Pin) == GPIO_PIN_RESET) << 0);
//     input_state |= ((HAL_GPIO_ReadPin(IN2_GPIO_Port, IN2_Pin) == GPIO_PIN_RESET) << 1);
//     input_state |= ((HAL_GPIO_ReadPin(IN3_GPIO_Port, IN3_Pin) == GPIO_PIN_RESET) << 2);
//     input_state |= ((HAL_GPIO_ReadPin(IN4_GPIO_Port, IN4_Pin) == GPIO_PIN_RESET) << 3);
//     input_state |= ((HAL_GPIO_ReadPin(IN5_GPIO_Port, IN5_Pin) == GPIO_PIN_RESET) << 4);
//     input_state |= ((HAL_GPIO_ReadPin(IN6_GPIO_Port, IN6_Pin) == GPIO_PIN_RESET) << 5);
//     input_state |= ((HAL_GPIO_ReadPin(IN7_GPIO_Port, IN7_Pin) == GPIO_PIN_RESET) << 6);
//     input_state |= ((HAL_GPIO_ReadPin(IN8_GPIO_Port, IN8_Pin) == GPIO_PIN_RESET) << 7);
//     input_state |= ((HAL_GPIO_ReadPin(IN9_GPIO_Port, IN9_Pin) == GPIO_PIN_RESET) << 8);
//     input_state |= ((HAL_GPIO_ReadPin(IN10_GPIO_Port, IN10_Pin) == GPIO_PIN_RESET) << 9);
//     input_state |= ((HAL_GPIO_ReadPin(IN11_GPIO_Port, IN11_Pin) == GPIO_PIN_RESET) << 10);
//     input_state |= ((HAL_GPIO_ReadPin(IN12_GPIO_Port, IN12_Pin) == GPIO_PIN_RESET) << 11);
//     input_state |= ((HAL_GPIO_ReadPin(IN13_GPIO_Port, IN13_Pin) == GPIO_PIN_RESET) << 12);
//     input_state |= ((HAL_GPIO_ReadPin(IN14_GPIO_Port, IN14_Pin) == GPIO_PIN_RESET) << 13);
//     input_state |= ((HAL_GPIO_ReadPin(IN15_GPIO_Port, IN15_Pin) == GPIO_PIN_RESET) << 14);
//     input_state |= ((HAL_GPIO_ReadPin(IN16_GPIO_Port, IN16_Pin) == GPIO_PIN_RESET) << 15);
//     input_report[*report_index] = input_state;
//     input_state_report[*report_index] = input_state;  /* store the input state in the global report buffer for later processing */
//     if((*report_index) < 99) {
//         _index++;
//         (*report_index)++;
//     } else {
//         _index = 0; /* reset index for next round of input state collection */
//         *report_index = 0;  /* reset index if buffer is full */
//     }
// }
