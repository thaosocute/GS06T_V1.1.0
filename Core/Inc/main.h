/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32l4xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define LED_STT_Pin GPIO_PIN_13
#define LED_STT_GPIO_Port GPIOC
#define EN1_Pin GPIO_PIN_2
#define EN1_GPIO_Port GPIOC
#define EN2_Pin GPIO_PIN_3
#define EN2_GPIO_Port GPIOC
#define RS485_DE1_Pin GPIO_PIN_4
#define RS485_DE1_GPIO_Port GPIOA
#define DS1804Z_UD_Pin GPIO_PIN_5
#define DS1804Z_UD_GPIO_Port GPIOA
#define DS1804Z_CS_Pin GPIO_PIN_6
#define DS1804Z_CS_GPIO_Port GPIOA
#define DS1804Z_INC_Pin GPIO_PIN_7
#define DS1804Z_INC_GPIO_Port GPIOA
#define RS485_DE2_Pin GPIO_PIN_0
#define RS485_DE2_GPIO_Port GPIOB
#define IN1_Pin GPIO_PIN_1
#define IN1_GPIO_Port GPIOB
#define IN2_Pin GPIO_PIN_2
#define IN2_GPIO_Port GPIOB
#define IN3_Pin GPIO_PIN_10
#define IN3_GPIO_Port GPIOB
#define IN4_Pin GPIO_PIN_11
#define IN4_GPIO_Port GPIOB
#define IN5_Pin GPIO_PIN_12
#define IN5_GPIO_Port GPIOB
#define IN6_Pin GPIO_PIN_13
#define IN6_GPIO_Port GPIOB
#define IN7_Pin GPIO_PIN_14
#define IN7_GPIO_Port GPIOB
#define IN8_Pin GPIO_PIN_15
#define IN8_GPIO_Port GPIOB
#define IN9_Pin GPIO_PIN_6
#define IN9_GPIO_Port GPIOC
#define IN10_Pin GPIO_PIN_7
#define IN10_GPIO_Port GPIOC
#define IN11_Pin GPIO_PIN_8
#define IN11_GPIO_Port GPIOC
#define IN12_Pin GPIO_PIN_9
#define IN12_GPIO_Port GPIOC
#define IN13_Pin GPIO_PIN_8
#define IN13_GPIO_Port GPIOA
#define IN14_Pin GPIO_PIN_9
#define IN14_GPIO_Port GPIOA
#define IN15_Pin GPIO_PIN_10
#define IN15_GPIO_Port GPIOA
#define IN16_Pin GPIO_PIN_11
#define IN16_GPIO_Port GPIOA
#define IN17_Pin GPIO_PIN_12
#define IN17_GPIO_Port GPIOA
#define IN18_Pin GPIO_PIN_2
#define IN18_GPIO_Port GPIOD
#define BR_I1_Pin GPIO_PIN_3
#define BR_I1_GPIO_Port GPIOB
#define BR_I2_Pin GPIO_PIN_4
#define BR_I2_GPIO_Port GPIOB
#define BR_I3_Pin GPIO_PIN_5
#define BR_I3_GPIO_Port GPIOB
#define BR_I4_Pin GPIO_PIN_6
#define BR_I4_GPIO_Port GPIOB
#define BR_I5_Pin GPIO_PIN_7
#define BR_I5_GPIO_Port GPIOB
#define BR_I6_Pin GPIO_PIN_8
#define BR_I6_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
