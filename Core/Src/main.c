/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* Compatibility typedef for CMSIS-RTOS v2 */
typedef StaticTask_t osStaticThreadDef_t;

#include "max3485.h"
#include "output.h"
#include "input.h"
#include "jsmn.h"
#include "monitors_config.h"
#include "json_cmd.h"
#include  <string.h>     /* for strlen in UART transmit */
#include  <stdio.h>      /* for sprintf in UART transmit */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
typedef StaticTask_t osStaticThreadDef_t;
typedef StaticSemaphore_t osStaticSemaphoreDef_t;
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define TOKENS_NUM 192
#define RX_BUFFER_SIZE_MAX 2048
#define RESPONSE_SIZE_MAX 2048

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c3;

TIM_HandleTypeDef htim6;

UART_HandleTypeDef huart2;
UART_HandleTypeDef huart3;
DMA_HandleTypeDef hdma_usart3_rx;

/* Definitions for RS485_cmd */
osThreadId_t RS485_cmdHandle;
uint32_t RS485_cmdBuffer[ 4096 ];
osStaticThreadDef_t RS485_cmdControlBlock;
const osThreadAttr_t RS485_cmd_attributes = {
  .name = "RS485_cmd",
  .cb_mem = &RS485_cmdControlBlock,
  .cb_size = sizeof(RS485_cmdControlBlock),
  .stack_mem = &RS485_cmdBuffer[0],
  .stack_size = sizeof(RS485_cmdBuffer),
  .priority = (osPriority_t) osPriorityAboveNormal,
};
/* Definitions for Update_input */
osThreadId_t Update_inputHandle;
uint32_t RS485_reportBuffer[ 512 ];
osStaticThreadDef_t RS485_reportControlBlock;
const osThreadAttr_t Update_input_attributes = {
  .name = "Update_input",
  .cb_mem = &RS485_reportControlBlock,
  .cb_size = sizeof(RS485_reportControlBlock),
  .stack_mem = &RS485_reportBuffer[0],
  .stack_size = sizeof(RS485_reportBuffer),
  .priority = (osPriority_t) osPriorityBelowNormal,
};
/* Definitions for myBinarySem01 */
osSemaphoreId_t myBinarySem01Handle;
osStaticSemaphoreDef_t myBinarySem01ControlBlock;
const osSemaphoreAttr_t myBinarySem01_attributes = {
  .name = "myBinarySem01",
  .cb_mem = &myBinarySem01ControlBlock,
  .cb_size = sizeof(myBinarySem01ControlBlock),
};
/* USER CODE BEGIN PV */
MAX3485_HandleTypeDef hmax3485_1;
MAX3485_HandleTypeDef hmax3485_2;

uint8_t rx_buffer[RX_BUFFER_SIZE_MAX];
uint8_t rx_data;  /* buffer for UART receive interrupt */
uint16_t rx_index = 0;
// uint8_t Rx_flag = 0;
uint16_t timer = 0;
uint16_t Countimer = 0;
volatile uint32_t tim6_tick_ms = 0;
volatile uint8_t tim6_pulse_active = 0;
volatile uint8_t tim6_pulse_button = 0;
volatile uint32_t tim6_pulse_end_tick = 0;
volatile uint8_t tim6_pulse_seq_active = 0;
volatile uint8_t tim6_pulse_seq_button = 0;
volatile uint8_t tim6_pulse_seq_phase = 0; /* 1: pulse on, 2: gap off */
volatile uint8_t tim6_pulse_seq_index = 0;
volatile uint8_t tim6_pulse_seq_count = 0;
volatile uint32_t tim6_pulse_seq_deadline = 0;
volatile uint32_t tim6_pulse_seq_pulse_ms[RELAY_PULSE_SEQ_MAX_STEPS] = {0};
volatile uint32_t tim6_pulse_seq_gap_ms[RELAY_PULSE_SEQ_MAX_STEPS] = {0};
volatile uint8_t tim6_press_req = 0;
volatile uint8_t tim6_release_req = 0;

char response[RESPONSE_SIZE_MAX];

Button_TypeDef button1 = RL1;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_I2C3_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_USART3_UART_Init(void);
static void MX_TIM6_Init(void);
void StartRS485CmdTask(void *argument);
void StartUpdate_input(void *argument);

/* USER CODE BEGIN PFP */
void json_err_handle(json_err_t* err);

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_I2C3_Init();
  MX_USART2_UART_Init();
  MX_USART3_UART_Init();
  MX_TIM6_Init();
  /* USER CODE BEGIN 2 */
  HAL_TIM_Base_Start_IT(&htim6);
  HAL_GPIO_WritePin(RS485_DE1_GPIO_Port, RS485_DE1_Pin, GPIO_PIN_RESET);  /* ensure DE1 is low for receive mode */
  HAL_GPIO_WritePin(RS485_DE2_GPIO_Port, RS485_DE2_Pin, GPIO_PIN_RESET);  /* ensure DE2 is low for receive mode */
  
  HAL_UARTEx_ReceiveToIdle_DMA(&huart3, rx_buffer, RX_BUFFER_SIZE_MAX);

  HAL_GPIO_WritePin(BR_I1_GPIO_Port, BR_I1_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(BR_I2_GPIO_Port, BR_I2_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(BR_I3_GPIO_Port, BR_I3_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(BR_I4_GPIO_Port, BR_I4_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(BR_I5_GPIO_Port, BR_I5_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(BR_I6_GPIO_Port, BR_I6_Pin, GPIO_PIN_RESET);

  pcf8575_reset_state(&hi2c3);
  max3485_init(&hmax3485_2, &huart3, RS485_DE2_GPIO_Port, RS485_DE2_Pin);
  max3485_init(&hmax3485_1, &huart2, RS485_DE1_GPIO_Port, RS485_DE1_Pin);

  pcf8575_write_pin(&hi2c3, 0, 0);
  HAL_GPIO_WritePin(LED_STT_GPIO_Port, LED_STT_Pin, GPIO_PIN_SET);

  HAL_GPIO_WritePin(EN1_GPIO_Port, EN1_Pin, GPIO_PIN_SET); 
  HAL_GPIO_WritePin(EN2_GPIO_Port, EN2_Pin, GPIO_PIN_SET);
  
  /* USER CODE END 2 */

  /* Init scheduler */
  osKernelInitialize();

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* Create the semaphores(s) */
  /* creation of myBinarySem01 */
  myBinarySem01Handle = osSemaphoreNew(1, 1, &myBinarySem01_attributes);

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of RS485_cmd */
  RS485_cmdHandle = osThreadNew(StartRS485CmdTask, NULL, &RS485_cmd_attributes);

  /* creation of Update_input */
  Update_inputHandle = osThreadNew(StartUpdate_input, NULL, &Update_input_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

  /* Start scheduler */
  osKernelStart();

  /* We should never get here as control is now taken by the scheduler */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure LSE Drive Capability
  */
  HAL_PWR_EnableBkUpAccess();
  __HAL_RCC_LSEDRIVE_CONFIG(RCC_LSEDRIVE_LOW);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSE|RCC_OSCILLATORTYPE_MSI;
  RCC_OscInitStruct.LSEState = RCC_LSE_ON;
  RCC_OscInitStruct.MSIState = RCC_MSI_ON;
  RCC_OscInitStruct.MSICalibrationValue = 0;
  RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_6;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_MSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }

  /** Enable MSI Auto calibration
  */
  HAL_RCCEx_EnableMSIPLLMode();
}

/**
  * @brief I2C3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C3_Init(void)
{

  /* USER CODE BEGIN I2C3_Init 0 */

  /* USER CODE END I2C3_Init 0 */

  /* USER CODE BEGIN I2C3_Init 1 */

  /* USER CODE END I2C3_Init 1 */
  hi2c3.Instance = I2C3;
  hi2c3.Init.Timing = 0x00100D14;
  hi2c3.Init.OwnAddress1 = 0;
  hi2c3.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c3.Init.DualAddressMode = I2C_DUALADDRESS_ENABLE;
  hi2c3.Init.OwnAddress2 = 0;
  hi2c3.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
  hi2c3.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c3.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c3) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Analogue filter
  */
  if (HAL_I2CEx_ConfigAnalogFilter(&hi2c3, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Digital filter
  */
  if (HAL_I2CEx_ConfigDigitalFilter(&hi2c3, 0) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C3_Init 2 */

  /* USER CODE END I2C3_Init 2 */

}

/**
  * @brief TIM6 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM6_Init(void)
{

  /* USER CODE BEGIN TIM6_Init 0 */

  /* USER CODE END TIM6_Init 0 */

  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM6_Init 1 */

  /* USER CODE END TIM6_Init 1 */
  htim6.Instance = TIM6;
  htim6.Init.Prescaler = 3;
  htim6.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim6.Init.Period = 999;
  htim6.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim6) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim6, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM6_Init 2 */

  /* USER CODE END TIM6_Init 2 */

}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 9600;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  huart2.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * @brief USART3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART3_UART_Init(void)
{

  /* USER CODE BEGIN USART3_Init 0 */

  /* USER CODE END USART3_Init 0 */

  /* USER CODE BEGIN USART3_Init 1 */

  /* USER CODE END USART3_Init 1 */
  huart3.Instance = USART3;
  huart3.Init.BaudRate = 9600;
  huart3.Init.WordLength = UART_WORDLENGTH_8B;
  huart3.Init.StopBits = UART_STOPBITS_1;
  huart3.Init.Parity = UART_PARITY_NONE;
  huart3.Init.Mode = UART_MODE_TX_RX;
  huart3.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart3.Init.OverSampling = UART_OVERSAMPLING_16;
  huart3.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart3.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart3) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART3_Init 2 */

  /* USER CODE END USART3_Init 2 */

}

/**
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA1_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA1_Channel3_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel3_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel3_IRQn);

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
/* USER CODE BEGIN MX_GPIO_Init_1 */
/* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, LED_STT_Pin|EN1_Pin|EN2_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, RS485_DE1_Pin|DS1804Z_UD_Pin|DS1804Z_CS_Pin|DS1804Z_INC_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, RS485_DE2_Pin|BR_I1_Pin|BR_I2_Pin|BR_I3_Pin
                          |BR_I4_Pin|BR_I5_Pin|BR_I6_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pins : LED_STT_Pin EN1_Pin EN2_Pin */
  GPIO_InitStruct.Pin = LED_STT_Pin|EN1_Pin|EN2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pin : RS485_DE1_Pin */
  GPIO_InitStruct.Pin = RS485_DE1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(RS485_DE1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : DS1804Z_UD_Pin DS1804Z_CS_Pin DS1804Z_INC_Pin */
  GPIO_InitStruct.Pin = DS1804Z_UD_Pin|DS1804Z_CS_Pin|DS1804Z_INC_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : RS485_DE2_Pin */
  GPIO_InitStruct.Pin = RS485_DE2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(RS485_DE2_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : IN1_Pin IN3_Pin IN4_Pin IN5_Pin */
  GPIO_InitStruct.Pin = IN1_Pin|IN3_Pin|IN4_Pin|IN5_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pins : IN2_Pin IN6_Pin */
  GPIO_InitStruct.Pin = IN2_Pin|IN6_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pins : IN7_Pin IN8_Pin */
  GPIO_InitStruct.Pin = IN7_Pin|IN8_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pins : IN9_Pin IN11_Pin */
  GPIO_InitStruct.Pin = IN9_Pin|IN11_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : IN10_Pin IN12_Pin */
  GPIO_InitStruct.Pin = IN10_Pin|IN12_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : IN13_Pin IN14_Pin IN15_Pin */
  GPIO_InitStruct.Pin = IN13_Pin|IN14_Pin|IN15_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : IN16_Pin */
  GPIO_InitStruct.Pin = IN16_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(IN16_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : BR_I1_Pin BR_I2_Pin BR_I3_Pin BR_I4_Pin
                           BR_I5_Pin BR_I6_Pin */
  GPIO_InitStruct.Pin = BR_I1_Pin|BR_I2_Pin|BR_I3_Pin|BR_I4_Pin
                          |BR_I5_Pin|BR_I6_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance != TIM6) return;

  tim6_tick_ms++;

  if (tim6_pulse_active && (int32_t)(tim6_tick_ms - tim6_pulse_end_tick) >= 0) {
    release_button(&hi2c3, (Button_TypeDef)tim6_pulse_button);
    tim6_pulse_active = 0;
  }

  if (tim6_pulse_seq_active && (int32_t)(tim6_tick_ms - tim6_pulse_seq_deadline) >= 0) {
    if (tim6_pulse_seq_phase == 1) {
      release_button(&hi2c3, (Button_TypeDef)tim6_pulse_seq_button);

      if (tim6_pulse_seq_index >= tim6_pulse_seq_count - 1) {
        tim6_pulse_seq_active = 0;
        tim6_pulse_seq_phase = 0;
      } else {
        uint32_t gap_ms = tim6_pulse_seq_gap_ms[tim6_pulse_seq_index];
        tim6_pulse_seq_phase = 2;
        tim6_pulse_seq_deadline = tim6_tick_ms + gap_ms;
      }
    } else if (tim6_pulse_seq_phase == 2) {
      tim6_pulse_seq_index++;
      if (tim6_pulse_seq_index >= tim6_pulse_seq_count) {
        tim6_pulse_seq_active = 0;
        tim6_pulse_seq_phase = 0;
      } else {
        press_button(&hi2c3, (Button_TypeDef)tim6_pulse_seq_button);
        tim6_pulse_seq_phase = 1;
        tim6_pulse_seq_deadline = tim6_tick_ms + tim6_pulse_seq_pulse_ms[tim6_pulse_seq_index];
      }
    }
  }

    // Countimer++;

    // if (Countimer == 1) {
    // tim6_press_req = 1;
    // } else if (Countimer == 200) {
    // tim6_release_req = 1;
    // } else if (Countimer >= 400) {
    //     Countimer = 0;
    // }
}
/* Callback functions*/
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
  if(huart->Instance == USART3){
		rx_index = Size;
		timer = 0;
	}
}

//functions
void json_err_handle(json_err_t* err){
  if(*err == ERR_NONE){
    return;
  }
  char err_code[20];
  strcpy(err_code, json_err_to_code(*err));
  max3485_transmit(&hmax3485_2, (uint8_t*)err_code, strlen(err_code), HAL_MAX_DELAY);
}

//void handle_monitors_config();
//void handle_relay_set();
//void handle_relay_pulse();
//void handle_relay_pulse_seq();

/* USER CODE END 4 */

/* USER CODE BEGIN Header_StartRS485CmdTask */
/**
  * @brief  Function implementing the RS485_cmd thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartRS485CmdTask */
void StartRS485CmdTask(void *argument)
{
  /* USER CODE BEGIN 5 */
  uint8_t ret;
  /* Infinite loop */
  for(;;)
  {
    if(timer >= 500 && rx_index > 0)  /* check if a complete command is received */
    {
      timer = 0;
      HAL_UART_DMAStop(&huart3);
      jsmn_parser parser;
      jsmntok_t tokens[TOKENS_NUM];
      json_err_t error = ERR_NONE;
      char cmd[32];
      jsmn_init(&parser);
      ret = jsmn_parse(&parser, (const char *)rx_buffer, strlen((const char *)rx_buffer), tokens, TOKENS_NUM);
      if(ret < 0){
        error = ERR_JSON_PARSE;
      } else {
        for(uint8_t i = 1; i < ret; i++) {
          if(tokens[i].type == JSMN_STRING){
            // kiểm tra nếu token là "cmd"
            if(jsoneq((const char *)rx_buffer, &tokens[i], "cmd") == 0) {
              // lấy giá trị của "cmd"
              jsmntok_t *cmd_token = &tokens[i + 1];
              snprintf(cmd, sizeof(cmd), "%.*s", cmd_token->end - cmd_token->start, (const char *)rx_buffer + cmd_token->start);
            }
          }
        }
        json_cmd_t num;
        num = json_cmd_from_str(cmd);
        switch(num) {
          case CMD_UNKNOWN:
            error = ERR_INVALID_CMD;
            break;
          case CMD_MONITOR_CONFIG: {
            monitors_set_json((const char*)rx_buffer);
            error = handle_monitors_config(tokens, ret, response, sizeof(response));
            if (error == ERR_NONE) {
              max3485_transmit(&hmax3485_2, (uint8_t*)response, strlen(response), HAL_MAX_DELAY);
              memset(response, 0, sizeof(response));
            }
            break;
          }
          case CMD_READ_PATTERN:
            monitors_set_json((const char*)rx_buffer);
            error = handle_read_pattern(tokens, ret, response, sizeof(response));
            if(error == ERR_NONE) {
              max3485_transmit(&hmax3485_2, (uint8_t*)response, strlen(response), HAL_MAX_DELAY);
              memset(response, 0, sizeof(response));
              release_button(&hi2c3, button1);
            }
            break;
          case CMD_READ_SNAPSHOT:
            monitors_set_json((const char*)rx_buffer);
            error = handle_read_snapshot(tokens, ret, response, sizeof(response));
            if(error == ERR_NONE) {
              max3485_transmit(&hmax3485_2, (uint8_t*)response, strlen(response), HAL_MAX_DELAY);
              memset(response, 0, sizeof(response));
            }
            break;
          case CMD_POLL:
            monitors_set_json((const char*)rx_buffer);
            error = handle_poll(tokens, ret, response, sizeof(response));
            if(error == ERR_NONE) {
              max3485_transmit(&hmax3485_2, (uint8_t*)response, strlen(response), HAL_MAX_DELAY);
              memset(response, 0, sizeof(response));
            }
            break;
          case CMD_FLUSH_EVENTS:
            break;
          case CMD_PING:
            monitors_set_json((const char*)rx_buffer);
            error = handle_ping(tokens, ret, response, sizeof(response));
            if(error == ERR_NONE) {
              max3485_transmit(&hmax3485_2, (uint8_t*)response, strlen(response), HAL_MAX_DELAY);
              memset(response, 0, sizeof(response));
              press_button(&hi2c3, button1);
            }
            break;
          case CMD_RELAY_SET:
            monitors_set_json((const char*)rx_buffer);
            error = handle_relay_set(tokens, ret, response, sizeof(response));
            if(error == ERR_NONE) {
              max3485_transmit(&hmax3485_2, (uint8_t*)response, strlen(response), HAL_MAX_DELAY);
              memset(response, 0, sizeof(response));
            }
            break;
          case CMD_RELAY_PULSE:
          monitors_set_json((const char*)rx_buffer);  
          error = handle_relay_pulse(tokens, ret, response, sizeof(response));
            if(error == ERR_NONE) {
              max3485_transmit(&hmax3485_2, (uint8_t*)response, strlen(response), HAL_MAX_DELAY);
              memset(response, 0, sizeof(response));
            }
            break;
          case CMD_RELAY_PULSE_SEQ:
          monitors_set_json((const char*)rx_buffer);  
          error = handle_relay_pulse_seq(tokens, ret, response, sizeof(response));
            if(error == ERR_NONE) {
              max3485_transmit(&hmax3485_2, (uint8_t*)response, strlen(response), HAL_MAX_DELAY);
              memset(response, 0, sizeof(response));
            }
            break;
          case CMD_RESET:
              monitors_set_json((const char*)rx_buffer);  
              error = handle_reset(tokens, ret, response, sizeof(response));
              if(error == ERR_NONE) {
              max3485_transmit(&hmax3485_2, (uint8_t*)response, strlen(response), HAL_MAX_DELAY);
              memset(response, 0, sizeof(response));
            }
            break;
        }
      }
      json_err_handle(&error);
      error = ERR_NONE;
      memset(rx_buffer, 0, RX_BUFFER_SIZE_MAX);
      rx_index = 0;
      HAL_UARTEx_ReceiveToIdle_DMA(&huart3, rx_buffer, RX_BUFFER_SIZE_MAX);
    }
    osDelay(200);
  }
  /* USER CODE END 5 */
}

/* USER CODE BEGIN Header_StartUpdate_input */
/**
* @brief Function implementing the Update_input thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartUpdate_input */
void StartUpdate_input(void *argument)
{
  /* USER CODE BEGIN StartUpdate_input */
  /* Infinite loop */
  for(;;)
  {
    // if (tim6_press_req) {
    //   __disable_irq();
    //   tim6_press_req = 0;
    //   __enable_irq();
    //   press_button(&hi2c3, RL1);
    // }

    // if (tim6_release_req) {
    //   __disable_irq();
    //   tim6_release_req = 0;
    //   __enable_irq();
    //   release_button(&hi2c3, RL1);
    // }

//    uint16_t input_state = 0;
//    input_state |= (HAL_GPIO_ReadPin(IN1_GPIO_Port, IN1_Pin) == GPIO_PIN_RESET) ? (1 << 0) : 0;
//    input_state |= (HAL_GPIO_ReadPin(IN2_GPIO_Port, IN2_Pin) == GPIO_PIN_RESET) ? (1 << 1) : 0;
//    input_state |= (HAL_GPIO_ReadPin(IN3_GPIO_Port, IN3_Pin) == GPIO_PIN_RESET) ? (1 << 2) : 0;
//    input_state |= (HAL_GPIO_ReadPin(IN4_GPIO_Port, IN4_Pin) == GPIO_PIN_RESET) ? (1 << 3) : 0;
//    input_state |= (HAL_GPIO_ReadPin(IN5_GPIO_Port, IN5_Pin) == GPIO_PIN_RESET) ? (1 << 4) : 0;
//    input_state |= (HAL_GPIO_ReadPin(IN6_GPIO_Port, IN6_Pin) == GPIO_PIN_RESET) ? (1 << 5) : 0;
//    input_state |= (HAL_GPIO_ReadPin(IN7_GPIO_Port, IN7_Pin) == GPIO_PIN_RESET) ? (1 << 6) : 0;
//    input_state |= (HAL_GPIO_ReadPin(IN8_GPIO_Port, IN8_Pin) == GPIO_PIN_RESET) ? (1 << 7) : 0;
//    input_state |= (HAL_GPIO_ReadPin(IN9_GPIO_Port, IN9_Pin) == GPIO_PIN_RESET) ? (1 << 8) : 0;
//    input_state |= (HAL_GPIO_ReadPin(IN10_GPIO_Port, IN10_Pin) == GPIO_PIN_RESET) ? (1 << 9) : 0;
//    input_state |= (HAL_GPIO_ReadPin(IN11_GPIO_Port, IN11_Pin) == GPIO_PIN_RESET) ? (1 << 10) : 0;
//    input_state |= (HAL_GPIO_ReadPin(IN12_GPIO_Port, IN12_Pin) == GPIO_PIN_RESET) ? (1 << 11) : 0;
//    input_state |= (HAL_GPIO_ReadPin(IN13_GPIO_Port, IN13_Pin) == GPIO_PIN_RESET) ? (1 << 12) : 0;
//    input_state |= (HAL_GPIO_ReadPin(IN14_GPIO_Port, IN14_Pin) == GPIO_PIN_RESET) ? (1 << 13) : 0;
//    input_state |= (HAL_GPIO_ReadPin(IN15_GPIO_Port, IN15_Pin) == GPIO_PIN_RESET) ? (1 << 14) : 0;
//    input_state |= (HAL_GPIO_ReadPin(IN16_GPIO_Port, IN16_Pin) == GPIO_PIN_RESET) ? (1 << 15) : 0;
//    push_input(input_state);
//    count++;
//    if(count >= 25){
//      count = 0;
//      monitor_set_state_event();
//      HAL_GPIO_TogglePin(LED_STT_GPIO_Port, LED_STT_Pin);
//    }
//	  press_button(&hi2c3, button1);
//    osDelay(300);
//    release_button(&hi2c3, button1);
//    osDelay(300);
  }
  /* USER CODE END StartUpdate_input */
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
