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
#include <stdio.h>
#include <string.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef struct
{
    uint16_t frequency_hz;  /* 0 significa silencio */
    uint16_t duration_ms;
} note_t;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
DAC_HandleTypeDef hdac;
DMA_HandleTypeDef hdma_dac1;

TIM_HandleTypeDef htim3;
TIM_HandleTypeDef htim6;

UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_TIM3_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_DAC_Init(void);
static void MX_TIM6_Init(void);
/* USER CODE BEGIN PFP */
static void UART_Print(const char *msg);
static void Show_Menu(void);
static uint8_t UART_ReadCommand(uint32_t timeout_ms);
static void PWM_SetFrequency(uint16_t freq_hz);
static void PWM_Stop(void);
static void Play_Note(uint16_t freq_hz, uint16_t duration_ms);
static void Play_ImperialMarch(void);
static void DAC_SetFrequency(uint16_t freq_hz);
static void DAC_Silence(void);
static void DAC_Play_Note(uint16_t freq_hz, uint16_t duration_ms);
static void Play_HesAPirate(void);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
#define TIM3_COUNTER_CLOCK_HZ  1000000U

/* Frecuencias de las notas musicales en Hz */
#define NOTE_F4    349U
#define NOTE_FS4   370U
#define NOTE_GS4   415U
#define NOTE_A4    440U
#define NOTE_AS4   466U
#define NOTE_C5    523U
#define NOTE_CS5   554U
#define NOTE_E5    659U
#define NOTE_F5    698U
#define NOTE_REST  0U

#define AUDIO1_TOTAL_MS  10000U

static const note_t imperial_march[] =
{
    {NOTE_A4,   500}, {NOTE_A4,   500}, {NOTE_A4,   500},
    {NOTE_F4,   350}, {NOTE_REST,  50}, {NOTE_C5,   150},
    {NOTE_A4,   500}, {NOTE_F4,   350}, {NOTE_REST,  50},
    {NOTE_C5,   150}, {NOTE_A4,   650},

    {NOTE_REST, 100},

    {NOTE_E5,   500}, {NOTE_E5,   500}, {NOTE_E5,   500},
    {NOTE_F5,   350}, {NOTE_REST,  50}, {NOTE_C5,   150},
    {NOTE_GS4,  500}, {NOTE_F4,   350}, {NOTE_REST,  50},
    {NOTE_C5,   150}, {NOTE_A4,   650},

    {NOTE_REST, 200}
};

#define IMPERIAL_MARCH_LEN \
    (sizeof(imperial_march) / sizeof(imperial_march[0]))

static void UART_Print(const char *msg)
{
    HAL_UART_Transmit(
        &huart2,
        (uint8_t *)msg,
        (uint16_t)strlen(msg),
        HAL_MAX_DELAY
    );
}

static void Show_Menu(void)
{
    UART_Print(
        "\r\n"
        "=========================================\r\n"
        "   Laboratorio 7 \r\n"
        "=========================================\r\n"
        " [1] Reproducir Cancion 1 \r\n"
    	" [2] Reproducir Cancion 2 \r\n"
        "-----------------------------------------\r\n"
        " Ingrese un comando y presione ENTER\r\n"
        "=========================================\r\n"
        "> "
    );
}

static uint8_t UART_ReadCommand(uint32_t timeout_ms)
{
    uint8_t rx = 0;

    if (HAL_UART_Receive(&huart2, &rx, 1, timeout_ms) == HAL_OK)
    {
        return rx;
    }

    return 0;
}

static void PWM_Stop(void)
{
    HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_1);
}

static void PWM_SetFrequency(uint16_t freq_hz)
{
    uint32_t arr;

    if (freq_hz == NOTE_REST)
    {
        PWM_Stop();
        return;
    }

    arr = (TIM3_COUNTER_CLOCK_HZ / freq_hz) - 1U;

    __HAL_TIM_SET_AUTORELOAD(&htim3, arr);
    __HAL_TIM_SET_COMPARE(
        &htim3,
        TIM_CHANNEL_1,
        (arr + 1U) / 2U
    );
    __HAL_TIM_SET_COUNTER(&htim3, 0U);

    HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);
}

static void Play_Note(uint16_t freq_hz, uint16_t duration_ms)
{
    PWM_SetFrequency(freq_hz);
    HAL_Delay(duration_ms);
}

static void Play_ImperialMarch(void)
{
    uint32_t start;
    uint32_t idx = 0;

    UART_Print(
        "\r\nReproduciendo Audio 1 (Imperial March)...\r\n"
    );

    start = HAL_GetTick();

    while ((HAL_GetTick() - start) < AUDIO1_TOTAL_MS)
    {
        Play_Note(
            imperial_march[idx].frequency_hz,
            imperial_march[idx].duration_ms
        );

        idx = (idx + 1U) % IMPERIAL_MARCH_LEN;
    }

    PWM_Stop();

    UART_Print("Reproduccion finalizada.\r\n");
}
/* ---------- Audio 2: He's a Pirate (DAC por DDS) ---------- */

static const uint8_t sine_wave_lut[32] = {
    128, 153, 177, 199, 218, 234, 245, 253, 255, 253, 245, 234, 218, 199, 177, 153,
    128, 103, 79, 57, 38, 22, 11, 3, 1, 3, 11, 22, 38, 57, 79, 103
};
#define SINE_LUT_SIZE  32U

/* Reloj de TIM6: mismo APB1 x2 que TIM3 en este proyecto -> 84 MHz.
 * Si cambias SystemClock_Config(), recalcula este valor. */
#define TIM6_CLOCK_HZ   84000000UL

static uint8_t dac_playing = 0;

#define NOTE_B4   494U
#define NOTE_C5b  523U
#define NOTE_D5   587U
#define NOTE_E5   659U
#define NOTE_FS5  740U
#define NOTE_G5   784U
#define NOTE_A5   880U

#define AUDIO2_TOTAL_MS  10000U

static const note_t hes_a_pirate[] = {
    {NOTE_E5,   180}, {NOTE_G5,   180}, {NOTE_A5,   360},
    {NOTE_REST,  60},

    {NOTE_A5,   180}, {NOTE_G5,   180}, {NOTE_FS5,  360},
    {NOTE_E5,   180}, {NOTE_D5,   180}, {NOTE_E5,   360},
    {NOTE_REST,  60},

    {NOTE_FS5,  180}, {NOTE_A5,   180}, {NOTE_G5,   360},
    {NOTE_FS5,  180}, {NOTE_E5,   180}, {NOTE_D5,   360},
    {NOTE_REST,  60},

    {NOTE_E5,   180}, {NOTE_G5,   180}, {NOTE_A5,   360},
    {NOTE_B4,   180}, {NOTE_D5,   180}, {NOTE_E5,   360},
    {NOTE_REST, 120},

    {NOTE_FS5,  180}, {NOTE_G5,   180}, {NOTE_A5,   360},
    {NOTE_G5,   180}, {NOTE_FS5,  180}, {NOTE_E5,   360},
    {NOTE_D5,   180}, {NOTE_E5,   180}, {NOTE_FS5,  360},
    {NOTE_REST, 120}
};
#define HES_A_PIRATE_LEN (sizeof(hes_a_pirate) / sizeof(hes_a_pirate[0]))

static void DAC_SetFrequency(uint16_t freq_hz)
{
    uint32_t update_rate = (uint32_t)freq_hz * SINE_LUT_SIZE;
    uint32_t arr = (TIM6_CLOCK_HZ / update_rate) - 1U;

    __HAL_TIM_SET_AUTORELOAD(&htim6, arr);
    __HAL_TIM_SET_COUNTER(&htim6, 0U);

    if (!dac_playing) {
        HAL_TIM_Base_Start(&htim6);
        HAL_DAC_Start_DMA(&hdac, DAC_CHANNEL_1, (uint32_t *)sine_wave_lut,
                           SINE_LUT_SIZE, DAC_ALIGN_8B_R);
        dac_playing = 1;
    }
}

static void DAC_Silence(void)
{
    if (dac_playing) {
        HAL_DAC_Stop_DMA(&hdac, DAC_CHANNEL_1);
        HAL_TIM_Base_Stop(&htim6);
        dac_playing = 0;
    }
    HAL_DAC_SetValue(&hdac, DAC_CHANNEL_1, DAC_ALIGN_8B_R, 128);
    HAL_DAC_Start(&hdac, DAC_CHANNEL_1);
}

static void DAC_Play_Note(uint16_t freq_hz, uint16_t duration_ms)
{
    if (freq_hz == 0) {
        DAC_Silence();
    } else {
        DAC_SetFrequency(freq_hz);
    }
    HAL_Delay(duration_ms);
}

static void Play_HesAPirate(void)
{
    UART_Print("\r\nReproduciendo Audio 2 (He's a Pirate)...\r\n");
    uint32_t start = HAL_GetTick();
    uint32_t idx = 0;

    while ((HAL_GetTick() - start) < AUDIO2_TOTAL_MS) {
        DAC_Play_Note(hes_a_pirate[idx].frequency_hz, hes_a_pirate[idx].duration_ms);
        idx = (idx + 1) % HES_A_PIRATE_LEN;
    }
    DAC_Silence();
    HAL_DAC_Stop(&hdac, DAC_CHANNEL_1);
    UART_Print("Reproduccion finalizada.\r\n");
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
  MX_TIM3_Init();
  MX_USART2_UART_Init();
  MX_DMA_Init();
  MX_DAC_Init();
  MX_TIM6_Init();
  /* USER CODE BEGIN 2 */
  PWM_Stop();
  Show_Menu();
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
	  uint8_t cmd = UART_ReadCommand(20);

	      if (cmd == '1')
	      {
	          Play_ImperialMarch();
	          Show_Menu();
	      }
	      else if (cmd == '2')
	      {
	          Play_HesAPirate();
	          Show_Menu();
	      }
	      else if ((cmd != 0) && (cmd != '\r') && (cmd != '\n'))
	      {
	          UART_Print("\r\nComando no valido.\r\n");
	          Show_Menu();
	      }

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
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 16;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
  RCC_OscInitStruct.PLL.PLLQ = 2;
  RCC_OscInitStruct.PLL.PLLR = 2;
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
  * @brief DAC Initialization Function
  * @param None
  * @retval None
  */
static void MX_DAC_Init(void)
{

  /* USER CODE BEGIN DAC_Init 0 */

  /* USER CODE END DAC_Init 0 */

  DAC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN DAC_Init 1 */

  /* USER CODE END DAC_Init 1 */

  /** DAC Initialization
  */
  hdac.Instance = DAC;
  if (HAL_DAC_Init(&hdac) != HAL_OK)
  {
    Error_Handler();
  }

  /** DAC channel OUT1 config
  */
  sConfig.DAC_Trigger = DAC_TRIGGER_T6_TRGO;
  sConfig.DAC_OutputBuffer = DAC_OUTPUTBUFFER_ENABLE;
  if (HAL_DAC_ConfigChannel(&hdac, &sConfig, DAC_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN DAC_Init 2 */

  /* USER CODE END DAC_Init 2 */

}

/**
  * @brief TIM3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM3_Init(void)
{

  /* USER CODE BEGIN TIM3_Init 0 */

  /* USER CODE END TIM3_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM3_Init 1 */

  /* USER CODE END TIM3_Init 1 */
  htim3.Instance = TIM3;
  htim3.Init.Prescaler = 83;
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period = 999;
  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  if (HAL_TIM_Base_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim3, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 500;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM3_Init 2 */

  /* USER CODE END TIM3_Init 2 */
  HAL_TIM_MspPostInit(&htim3);

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
  htim6.Init.Prescaler = 0;
  htim6.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim6.Init.Period = 999;
  htim6.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim6) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_UPDATE;
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
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA1_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA1_Stream5_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Stream5_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Stream5_IRQn);

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
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : B1_Pin */
  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : LD2_Pin */
  GPIO_InitStruct.Pin = LD2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LD2_GPIO_Port, &GPIO_InitStruct);

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
