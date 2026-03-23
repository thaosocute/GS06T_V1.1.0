#include "max3485.h"

void max3485_init(MAX3485_HandleTypeDef *hmax3485, UART_HandleTypeDef *huart, GPIO_TypeDef *DE_Port, uint16_t DE_Pin)
{
    hmax3485->huart = huart;
    hmax3485->DE_Port = DE_Port;
    hmax3485->DE_Pin = DE_Pin;
    HAL_GPIO_WritePin(hmax3485->DE_Port, hmax3485->DE_Pin, GPIO_PIN_RESET);
}

void max3485_transmit(MAX3485_HandleTypeDef *hmax3485, uint8_t *pData, uint16_t Size, uint32_t Timeout)
{
    HAL_GPIO_WritePin(hmax3485->DE_Port, hmax3485->DE_Pin, GPIO_PIN_SET); 
    HAL_UART_Transmit(hmax3485->huart, pData, Size, Timeout);
    HAL_GPIO_WritePin(hmax3485->DE_Port, hmax3485->DE_Pin, GPIO_PIN_RESET); 
}

void max3485_receive(MAX3485_HandleTypeDef *hmax3485, uint8_t *pData, uint16_t Size, uint32_t Timeout)
{
    HAL_GPIO_WritePin(hmax3485->DE_Port, hmax3485->DE_Pin, GPIO_PIN_RESET); 
    HAL_UART_Receive(hmax3485->huart, pData, Size, Timeout);
}
