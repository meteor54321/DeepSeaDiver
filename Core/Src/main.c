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

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
TIM_HandleTypeDef htim2;

UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */
#include <stdio.h>
#include <string.h>

// 全域共用變數（跨 ISR 必加 volatile）[cite: 6, 19, 26]
volatile uint32_t ticks = 0;         // 毫秒計時 (TIM3 或 SysTick)[cite: 35]
volatile uint8_t  btn_pressed = 0;   // B1 觸發旗標[cite: 35]
volatile uint32_t t_last_btn = 0;    // 50ms 去彈跳時間戳[cite: 35]

// 環形緩衝區 (W06)[cite: 25, 26]
#define RX_BUF_SIZE 64
volatile char rx_buf[RX_BUF_SIZE];
volatile uint8_t rx_head = 0;
volatile uint8_t rx_tail = 0;

// 潛水員狀態機變數[cite: 35, 40]
int depth = 10;            // 初始深度 10m
int o2 = 100;              // 氧氣殘量 0~100%[cite: 35, 40]
uint32_t last_sec = 0;     // 每秒計時[cite: 35]
enum { MODE_DIVING, MODE_SOS, MODE_DECOMPRESS } mode = MODE_DIVING;
int decomp_sec = 0;        // 減壓倒數秒數[cite: 40]
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_TIM2_Init(void);
static void MX_USART2_UART_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
// UART 傳送單一字元[cite: 24, 25]
void uart_putchar(char c) {
    while (!(USART2->ISR & USART_ISR_TXE_TXFNF)); // 等待 TDR 空[cite: 24, 25]
    USART2->TDR = c;                               // 寫入傳送暫存器[cite: 24]
}

// UART 傳送字串[cite: 24, 35]
void uart_puts(const char *s) {
    while (*s) uart_putchar(*s++);
}

// 供 printf 重導向至 USART2[cite: 28]
int __io_putchar(int ch) {
    uart_putchar((char)ch);
    return ch;
}

// W05 EXTI 中斷服務常式（B1 PC13）[cite: 14, 19]
void EXTI15_10_IRQHandler(void) {
    if (EXTI->PR1 & EXTI_PR1_PIF13) {
        EXTI->PR1 = EXTI_PR1_PIF13; // 寫 1 清除 (rc_w1)[cite: 16, 19]
        // 50 ms 非阻塞軟體去彈跳[cite: 35]
        if (ticks - t_last_btn >= 50) {
            btn_pressed = 1;
            t_last_btn = ticks;
        }
    }
}

// W06 USART2 中斷服務常式[cite: 25, 26]
void USART2_IRQHandler(void) {
    if (USART2->ISR & USART_ISR_RXNE_RXFNE) {
        char c = USART2->RDR; // 讀取 RDR 自動清旗標[cite: 24, 25]
        uint8_t next_head = (rx_head + 1) & (RX_BUF_SIZE - 1); // 64-byte 快速運算[cite: 25, 26]
        if (next_head != rx_tail) {
            rx_buf[rx_head] = c;
            rx_head = next_head;
        }
    }
}
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
  MX_TIM2_Init();
  MX_USART2_UART_Init();
  /* USER CODE BEGIN 2 */
  TIM2->PSC = 169;       // 170MHz / (169+1) = 1 MHz 計數節拍[cite: 29, 30]
      TIM2->ARR = 999;       // 週期 1000 次 = 1 kHz 頻率[cite: 29, 30]
      TIM2->CCR1 = 1000;     // 初始佔空比 100%（全亮）[cite: 30]
      TIM2->CR1 |= TIM_CR1_ARPE;       // 啟用 ARR 預載防毛刺[cite: 31, 32]
      TIM2->CCMR1 |= TIM_CCMR1_OC1PE;  // 啟用 CCR1 預載防毛刺[cite: 31, 32]
      TIM2->CCER |= TIM_CCER_CC1E;     // 啟用通道 1 輸出[cite: 30, 32]
      TIM2->CR1 |= TIM_CR1_CEN;        // 開啟計時器計數[cite: 29, 30]

      // 啟用 USART2 接收中斷[cite: 25, 26]
      USART2->CR1 |= USART_CR1_RXNEIE_RXFNEIE;

      uart_puts("\r\n=== Deep Sea Diver System Online ===\r\n");
  /* USER CODE END 2 */
      char cmd_line[32];
      int cmd_idx = 0;
  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */
	  ticks = HAL_GetTick(); // 使用系統 1ms tick[cite: 35]

	// 1. 處理 B1 按鍵中斷事件（下潛加深）[cite: 35, 40]
	if (btn_pressed) {
	btn_pressed = 0;
	if (mode == MODE_DIVING) {
	depth += 10; // 每按一次深度 +10m[cite: 40]
	printf(">> Dive Deeper: Depth = %dm\r\n", depth);
	}
    /* USER CODE BEGIN 3 */
	while (rx_tail != rx_head) {
	        char ch = rx_buf[rx_tail];
	        rx_tail = (rx_tail + 1) & (RX_BUF_SIZE - 1);

	        if (ch == '\r' || ch == '\n') {
	            if (cmd_idx > 0) {
	                cmd_line[cmd_idx] = '\0';
	                // 指令解析[cite: 27, 40]
	                if (strcmp(cmd_line, "surface") == 0) {
	                    mode = MODE_DECOMPRESS;
	                    decomp_sec = 3; // 啟動 3 秒減壓艙[cite: 40]
	                    printf(">> Surface Requested: Starting 3s Decompression...\r\n");
	                } else if (strcmp(cmd_line, "low") == 0) {
	                    o2 = 15; // 快速測試指令[cite: 40]
	                }
	                cmd_idx = 0;
	            }
	        } else if (cmd_idx < 31) {
	            cmd_line[cmd_idx++] = ch;
	        }
	    }

	    // 3. 每秒（1000 ms）週期性狀態處理[cite: 35, 40]
	    if (ticks - last_sec >= 1000) {
	        last_sec = ticks;

	        if (mode == MODE_DIVING) {
	            // 水深決定耗氧率：10m 扣 2%，之後每多 10m 多扣 3%[cite: 40]
	            int burn_rate = 2 + ((depth - 10) / 10) * 3;
	            o2 -= burn_rate;
	            if (o2 <= 0) o2 = 0;

	            printf("[DIVE] Depth: %2dm | O2: %3d%% | Burn: %2d%%/s\r\n", depth, o2, burn_rate);

	            // 判斷是否低於 20% 進入 SOS[cite: 40]
	            if (o2 <= 20) {
	                mode = MODE_SOS;
	                printf(">> [ALERT] Oxygen Critical! SOS Triggered!\r\n");
	            }
	        }
	        else if (mode == MODE_DECOMPRESS) {
	            printf("[DECOMP] Safe Chamber: %ds remaining...\r\n", decomp_sec);
	            decomp_sec--;
	            if (decomp_sec <= 0) {
	                mode = MODE_DIVING;
	                depth = 10;
	                o2 = 100;
	                printf(">> Resurfaced Safely! O2 Refilled to 100%%.\r\n");
	            }
	        }
	    }

	    // 4. W07 PWM 多態調光控制（直接修改 CCR1 暫存器）[cite: 30, 31, 32]
	    if (mode == MODE_DIVING) {
	        TIM2->CCR1 = o2 * 10; // 0~100% 等比對應 CCR1 (0~1000)[cite: 30, 32]
	    }
	    else if (mode == MODE_SOS) {
	        // 每 200ms 快閃（非阻塞）[cite: 40]
	        TIM2->CCR1 = ((ticks / 200) % 2) ? 1000 : 0;
	    }
	    else if (mode == MODE_DECOMPRESS) {
	        // 平滑呼吸燈：利用三角波在 1000ms 內往返 0~1000[cite: 31, 32]
	        int phase = ticks % 1000;
	        TIM2->CCR1 = (phase < 500) ? (phase * 2) : (2000 - phase * 2);

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
  HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1_BOOST);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = RCC_PLLM_DIV4;
  RCC_OscInitStruct.PLL.PLLN = 85;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
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
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief TIM2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM2_Init(void)
{

  /* USER CODE BEGIN TIM2_Init 0 */

  /* USER CODE END TIM2_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 0;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 4294967295;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */
  HAL_TIM_MspPostInit(&htim2);

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
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  huart2.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart2.Init.ClockPrescaler = UART_PRESCALER_DIV1;
  huart2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetTxFifoThreshold(&huart2, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetRxFifoThreshold(&huart2, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_DisableFifoMode(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

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
  __HAL_RCC_GPIOF_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin : PC13 */
  GPIO_InitStruct.Pin = GPIO_PIN_13;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI15_10_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

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
#ifdef USE_FULL_ASSERT
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
