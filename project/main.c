#include "main.h"
#include "string.h"
#include "freertos.h"
#include "task.h"
#include "semphr.h"
#include "queue.h"
#include "timers.h"
#include <stdint.h>
#include "stm32f7xx_hal.h"

#if defined ( __ICCARM__ ) /*!< IAR Compiler */
#pragma location=0x2004c000
ETH_DMADescTypeDef  DMARxDscrTab[ETH_RX_DESC_CNT]; /* Ethernet Rx DMA Descriptors */
#pragma location=0x2004c0a0
ETH_DMADescTypeDef  DMATxDscrTab[ETH_TX_DESC_CNT]; /* Ethernet Tx DMA Descriptors */

#elif defined ( __CC_ARM )  /* MDK ARM Compiler */

__attribute__((at(0x2004c000))) ETH_DMADescTypeDef  DMARxDscrTab[ETH_RX_DESC_CNT]; /* Ethernet Rx DMA Descriptors */
__attribute__((at(0x2004c0a0))) ETH_DMADescTypeDef  DMATxDscrTab[ETH_TX_DESC_CNT]; /* Ethernet Tx DMA Descriptors */

#elif defined ( __GNUC__ ) /* GNU Compiler */
ETH_DMADescTypeDef DMARxDscrTab[ETH_RX_DESC_CNT] __attribute__((section(".RxDecripSection"))); /* Ethernet Rx DMA Descriptors */
ETH_DMADescTypeDef DMATxDscrTab[ETH_TX_DESC_CNT] __attribute__((section(".TxDecripSection")));   /* Ethernet Tx DMA Descriptors */
#endif

ETH_TxPacketConfig TxConfig;
ETH_HandleTypeDef heth;
UART_HandleTypeDef huart3;
PCD_HandleTypeDef hpcd_USB_OTG_FS;

void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_ETH_Init(void);
static void MX_USART3_UART_Init(void);
static void MX_USB_OTG_FS_PCD_Init(void);
void StartDefaultTask(void *argument);

char font8x8[10][8] = {
  { 0x7E, 0x83, 0x85, 0x89, 0x91, 0xA1, 0xC1, 0x7E},
  { 0xF8, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0xFF},
  { 0xFF, 0x01, 0x01, 0x01, 0x7F, 0x80, 0x80, 0xFF},
  { 0xFF, 0x01, 0x01, 0x01, 0xFF, 0x01, 0x01, 0xFF},
  { 0x81, 0x81, 0x81, 0x81, 0xFF, 0x01, 0x01, 0x01},
  { 0xFF, 0x80, 0x80, 0x80, 0xFF, 0x01, 0x01, 0xFF},
  { 0xFF, 0x80, 0x80, 0x80, 0xFF, 0x81, 0x81, 0xFF},
  { 0xFF, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01},
  { 0xFF, 0x81, 0x81, 0x81, 0xFF, 0x81, 0x81, 0xFF},
  { 0xFF, 0x81, 0x81, 0x81, 0xFF, 0x01, 0x01, 0x01}
};

char font7x7[10][8] = {
  { 0x7C, 0x86, 0x8A, 0x92, 0xA2, 0xC2, 0x7C, 0x00},
  { 0xF0, 0x10, 0x10, 0x10, 0x10, 0x10, 0xFE, 0x00},
  { 0xFE, 0x02, 0x02, 0x7E, 0x80, 0x80, 0xFE, 0x00},
  { 0xFE, 0x02, 0x02, 0xFE, 0x02, 0x02, 0xFE, 0x00},
  { 0x82, 0x82, 0x82, 0xFE, 0x02, 0x02, 0x02, 0x00},
  { 0xFE, 0x80, 0x80, 0xFE, 0x02, 0x02, 0xFC, 0x00},
  { 0xFE, 0x80, 0x80, 0xFE, 0x82, 0x82, 0xFE, 0x00},
  { 0xFE, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x00},
  { 0xFE, 0x82, 0x82, 0xFE, 0x82, 0x82, 0xFE, 0x00},
  { 0xFE, 0x82, 0x82, 0xFE, 0x82, 0x82, 0xFE, 0x00}
};

char displayFont[8][72];

#define STACK_SIZE 128
#define QUEUE_LENGTH 8

QueueHandle_t fontQueue = NULL;
SemaphoreHandle_t rowSemPtr = NULL;
TimerHandle_t rowTimer;
TimerHandle_t colTimer;

char student_id[8] = "20181536";
int displayMode = 0;
int colIdx = 0;
int timestamp = 0;

void rowTimerCallback(TimerHandle_t xTimer);
void colTimerCallback(TimerHandle_t xTimer);
void fontInitTask(void * NotUsed);
void rowShiftTask(void * NotUsed);

int main(void)
{
  HAL_Init();
  SystemClock_Config();
  MX_GPIO_Init();
  MX_ETH_Init();
  MX_USART3_UART_Init();
  MX_USB_OTG_FS_PCD_Init();

  rowTimer = xTimerCreate("RowTimer", pdMS_TO_TICKS(1), pdTRUE, (void *)0 , rowTimerCallback);
  colTimer = xTimerCreate("ColTimer1", pdMS_TO_TICKS(1000 / 18), pdTRUE, (void *)0, colTimerCallback);

  fontQueue = xQueueCreate(QUEUE_LENGTH, sizeof(char (*)[8]));
  rowSemPtr = xSemaphoreCreateBinary();

  xTaskCreate(fontInitTask, "fontInitTask", STACK_SIZE, NULL, tskIDLE_PRIORITY + 3, NULL);
  xTaskCreate(rowShiftTask, "rowShiftTask", STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, NULL);

  xTimerStart(rowTimer, 0);
  xTimerStart(colTimer, 0);

  for(int num = 0; num < 8; num++) {
	  for(int row = 0; row < 8; row++) {
		  for(int col = 0; col < 8; col++) {
			  displayFont[row][num * 8 + col] = font8x8[student_id[num] - '0'][row] & (0x80 >> col);
  		  }
      }
  }

  vTaskStartScheduler();

  while (1)
  {
  }
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
	BaseType_t xTaskWokenByReceive = pdFALSE;

	if(GPIO_Pin == USER_Btn_Pin) {
		displayMode = (displayMode + 1) % 3;
		if(displayMode == 0) colIdx = 0;
		else if(displayMode == 1) colIdx = 64;
		char (*fontPtr)[8] = (displayMode == 0) ? font8x8 : font7x7;
		xQueueSendFromISR(fontQueue, &fontPtr, &xTaskWokenByReceive);
		timestamp = 0;
	}
}

void rowTimerCallback(TimerHandle_t xTimer) {
	xSemaphoreGive(rowSemPtr);
}

void colTimerCallback(TimerHandle_t xTimer) {
	timestamp++;
	if(displayMode == 0) {
		if(timestamp % 18 == 0) colIdx = (colIdx + 8) % 72;
	}
	else if(displayMode == 1) {
		if(timestamp % 2 == 0) colIdx = (colIdx + 1) % 72;
	}
	else if(displayMode == 2) {
		colIdx = (colIdx + 1) % 72;
	}
}

void fontInitTask(void * NotUsed) {
	char (*fontPtr)[8];

	while(1) {
		if(xQueueReceive(fontQueue, &fontPtr, portMAX_DELAY) == pdTRUE) {
			for(int num = 0; num < 8; num++) {
				for(int row = 0; row < 8; row++) {
					for(int col = 0; col < 8; col++) {
						displayFont[row][num * 8 + col] = fontPtr[student_id[num] - '0'][row] & (0x80 >> col);
					}
				}
			}
		}
	}
}

void rowShiftTask(void * NotUsed) {
	int row = 0;
	int pin[8] = {GPIO_PIN_0, GPIO_PIN_1, GPIO_PIN_2, GPIO_PIN_3, GPIO_PIN_4, GPIO_PIN_5, GPIO_PIN_6, GPIO_PIN_7};

	while(1) {
		if(xSemaphoreTake(rowSemPtr, portMAX_DELAY) == pdTRUE) {
			for(int i = 0; i < 8; i++) {
				if(i == row)
					HAL_GPIO_WritePin(GPIOD, pin[i], GPIO_PIN_RESET);
				else
					HAL_GPIO_WritePin(GPIOD, pin[i], GPIO_PIN_SET);
			}

			HAL_GPIO_WritePin(GPIOE, GPIO_PIN_8, displayFont[row][(colIdx) % 72]);
			HAL_GPIO_WritePin(GPIOE, GPIO_PIN_9, displayFont[row][(colIdx + 1) % 72]);
		    HAL_GPIO_WritePin(GPIOE, GPIO_PIN_10, displayFont[row][(colIdx + 2) % 72]);
			HAL_GPIO_WritePin(GPIOE, GPIO_PIN_11, displayFont[row][(colIdx + 3) % 72]);
			HAL_GPIO_WritePin(GPIOE, GPIO_PIN_12, displayFont[row][(colIdx + 4) % 72]);
			HAL_GPIO_WritePin(GPIOE, GPIO_PIN_13, displayFont[row][(colIdx + 5) % 72]);
			HAL_GPIO_WritePin(GPIOE, GPIO_PIN_14, displayFont[row][(colIdx + 6) % 72]);
			HAL_GPIO_WritePin(GPIOE, GPIO_PIN_15, displayFont[row][(colIdx + 7) % 72]);

			row = (row + 1) % 8;
		}
	}
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure LSE Drive Capability
  */
  HAL_PWR_EnableBkUpAccess();

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_BYPASS;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 4;
  RCC_OscInitStruct.PLL.PLLN = 72;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 3;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief ETH Initialization Function
  * @param None
  * @retval None
  */
static void MX_ETH_Init(void)
{

  /* USER CODE BEGIN ETH_Init 0 */

  /* USER CODE END ETH_Init 0 */

   static uint8_t MACAddr[6];

  /* USER CODE BEGIN ETH_Init 1 */

  /* USER CODE END ETH_Init 1 */
  heth.Instance = ETH;
  MACAddr[0] = 0x00;
  MACAddr[1] = 0x80;
  MACAddr[2] = 0xE1;
  MACAddr[3] = 0x00;
  MACAddr[4] = 0x00;
  MACAddr[5] = 0x00;
  heth.Init.MACAddr = &MACAddr[0];
  heth.Init.MediaInterface = HAL_ETH_RMII_MODE;
  heth.Init.TxDesc = DMATxDscrTab;
  heth.Init.RxDesc = DMARxDscrTab;
  heth.Init.RxBuffLen = 1524;

  /* USER CODE BEGIN MACADDRESS */

  /* USER CODE END MACADDRESS */

  if (HAL_ETH_Init(&heth) != HAL_OK)
  {
    Error_Handler();
  }

  memset(&TxConfig, 0 , sizeof(ETH_TxPacketConfig));
  TxConfig.Attributes = ETH_TX_PACKETS_FEATURES_CSUM | ETH_TX_PACKETS_FEATURES_CRCPAD;
  TxConfig.ChecksumCtrl = ETH_CHECKSUM_IPHDR_PAYLOAD_INSERT_PHDR_CALC;
  TxConfig.CRCPadCtrl = ETH_CRC_PAD_INSERT;
  /* USER CODE BEGIN ETH_Init 2 */

  /* USER CODE END ETH_Init 2 */

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
  huart3.Init.BaudRate = 115200;
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
  * @brief USB_OTG_FS Initialization Function
  * @param None
  * @retval None
  */
static void MX_USB_OTG_FS_PCD_Init(void)
{

  /* USER CODE BEGIN USB_OTG_FS_Init 0 */

  /* USER CODE END USB_OTG_FS_Init 0 */

  /* USER CODE BEGIN USB_OTG_FS_Init 1 */

  /* USER CODE END USB_OTG_FS_Init 1 */
  hpcd_USB_OTG_FS.Instance = USB_OTG_FS;
  hpcd_USB_OTG_FS.Init.dev_endpoints = 6;
  hpcd_USB_OTG_FS.Init.speed = PCD_SPEED_FULL;
  hpcd_USB_OTG_FS.Init.dma_enable = DISABLE;
  hpcd_USB_OTG_FS.Init.phy_itface = PCD_PHY_EMBEDDED;
  hpcd_USB_OTG_FS.Init.Sof_enable = ENABLE;
  hpcd_USB_OTG_FS.Init.low_power_enable = DISABLE;
  hpcd_USB_OTG_FS.Init.lpm_enable = DISABLE;
  hpcd_USB_OTG_FS.Init.vbus_sensing_enable = ENABLE;
  hpcd_USB_OTG_FS.Init.use_dedicated_ep1 = DISABLE;
  if (HAL_PCD_Init(&hpcd_USB_OTG_FS) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USB_OTG_FS_Init 2 */

  /* USER CODE END USB_OTG_FS_Init 2 */

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
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOG_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, LD1_Pin|LD3_Pin|LD2_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(USB_PowerSwitchOn_GPIO_Port, USB_PowerSwitchOn_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : USER_Btn_Pin */
  GPIO_InitStruct.Pin = USER_Btn_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(USER_Btn_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : LD1_Pin LD3_Pin LD2_Pin */
  GPIO_InitStruct.Pin = LD1_Pin|LD3_Pin|LD2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : USB_PowerSwitchOn_Pin */
  GPIO_InitStruct.Pin = USB_PowerSwitchOn_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(USB_PowerSwitchOn_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : USB_OverCurrent_Pin */
  GPIO_InitStruct.Pin = USB_OverCurrent_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(USB_OverCurrent_GPIO_Port, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

  HAL_NVIC_SetPriority(EXTI15_10_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
//void StartDefaultTask(void *argument)
//{
//  /* USER CODE BEGIN 5 */
//  /* Infinite loop */
//  for(;;)
//  {
//    osDelay(1);
//  }
//  /* USER CODE END 5 */
//}

/**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called  when TIM4 interrupt took place, inside
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  /* USER CODE BEGIN Callback 0 */

  /* USER CODE END Callback 0 */
  if (htim->Instance == TIM4) {
    HAL_IncTick();
  }
  /* USER CODE BEGIN Callback 1 */

  /* USER CODE END Callback 1 */
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
