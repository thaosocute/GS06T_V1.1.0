#ifndef MAX3485_H_
#define MAX3485_H_

#include "main.h"

typedef struct
{
    UART_HandleTypeDef *huart;
    GPIO_TypeDef *DE_Port;
    uint16_t DE_Pin;
} MAX3485_HandleTypeDef;

void max3485_init(MAX3485_HandleTypeDef *hmax3485, UART_HandleTypeDef *huart, GPIO_TypeDef *DE_Port, uint16_t DE_Pin);
void max3485_transmit(MAX3485_HandleTypeDef *hmax3485, uint8_t *pData, uint16_t Size, uint32_t Timeout);
void max3485_receive(MAX3485_HandleTypeDef *hmax3485, uint8_t *pData, uint16_t Size, uint32_t Timeout);

#endif /* MAX3485_H_ */