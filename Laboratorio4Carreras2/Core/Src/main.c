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
#include "stdint.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef enum
{
    ESTADO_IDLE = 0,
    ESTADO_CUENTA_REGRESIVA,
    ESTADO_CARRERA
} estado_juego_t;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define META               4
#define CUENTA_INICIAL     5
#define DEBOUNCE_MS        200U
#define COUNTDOWN_STEP_MS  1000U
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/

/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
volatile uint8_t  contador_j1       = 0;
volatile uint8_t  contador_j2       = 0;
volatile uint32_t ultimo_tick_j1    = 0;
volatile uint32_t ultimo_tick_j2    = 0;
volatile uint32_t ultimo_tick_start = 0;

volatile uint8_t  carrera_activa = 0;
volatile uint8_t  ganador        = 0;

volatile estado_juego_t game_state    = ESTADO_IDLE;
volatile uint8_t  countdown_value     = CUENTA_INICIAL;
volatile uint32_t countdown_last_tick = 0;
volatile uint8_t  inicio_solicitado   = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */
static void LED_Update_J1(uint8_t valor);
static void LED_Update_J2(uint8_t valor);
static void Display_Digit(uint8_t digito);
static void Display_Apagar(void);
static void Reset_Game(void);
static void Check_Winner(void);
static void Countdown_Task(void);
/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
static void LED_Update_J1(uint8_t valor)
{
    /* "Termómetro": se prenden los primeros N LEDs, sin apagar los anteriores */
    uint8_t mascara = (valor >= 4) ? 0x0F : (uint8_t)((1u << valor) - 1u);
    HAL_GPIO_WritePin(GPIOB, LED_J1_0_Pin, (mascara & 0x01) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOB, LED_J1_1_Pin, (mascara & 0x02) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOB, LED_J1_2_Pin, (mascara & 0x04) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOB, LED_J1_3_Pin, (mascara & 0x08) ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

static void LED_Update_J2(uint8_t valor)
{
    uint8_t mascara = (valor >= 4) ? 0x0F : (uint8_t)((1u << valor) - 1u);
    HAL_GPIO_WritePin(GPIOB, LED_J2_0_Pin, (mascara & 0x01) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOB, LED_J2_1_Pin, (mascara & 0x02) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOB, LED_J2_2_Pin, (mascara & 0x04) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOB, LED_J2_3_Pin, (mascara & 0x08) ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

static void Display_Digit(uint8_t digito)
{
    static const uint8_t patrones[10][7] = {
        {1,1,1,1,1,1,0}, /* 0 */
        {0,1,1,0,0,0,0}, /* 1 */
        {1,1,0,1,1,0,1}, /* 2 */
        {1,1,1,1,0,0,1}, /* 3 */
        {0,1,1,0,0,1,1}, /* 4 */
        {1,0,1,1,0,1,1}, /* 5 */
        {1,0,1,1,1,1,1}, /* 6 */
        {1,1,1,0,0,0,0}, /* 7 */
        {1,1,1,1,1,1,1}, /* 8 */
        {1,1,1,1,0,1,1}, /* 9 */
    };
    if (digito > 9) return;
    const uint8_t *p = patrones[digito];
    HAL_GPIO_WritePin(GPIOC, SEG_A_Pin, p[0] ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOC, SEG_B_Pin, p[1] ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOC, SEG_C_Pin, p[2] ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOC, SEG_D_Pin, p[3] ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOC, SEG_E_Pin, p[4] ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOC, SEG_F_Pin, p[5] ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOC, SEG_G_Pin, p[6] ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

static void Display_Apagar(void)
{
    HAL_GPIO_WritePin(GPIOC, SEG_A_Pin|SEG_B_Pin|SEG_C_Pin|SEG_D_Pin
                            |SEG_E_Pin|SEG_F_Pin|SEG_G_Pin, GPIO_PIN_RESET);
}

static void Reset_Game(void)
{
    contador_j1    = 0;
    contador_j2    = 0;
    ganador        = 0;
    carrera_activa = 0;
    LED_Update_J1(0);
    LED_Update_J2(0);
    Display_Apagar();
}

static void Check_Winner(void)
{
    if (game_state != ESTADO_CARRERA) return;

    if (contador_j1 >= META)
    {
        ganador = 1;
        carrera_activa = 0;
        LED_Update_J1(0x0F);
        LED_Update_J2(0x00);
        Display_Digit(1);
        game_state = ESTADO_IDLE;
    }
    else if (contador_j2 >= META)
    {
        ganador = 2;
        carrera_activa = 0;
        LED_Update_J2(0x0F);
        LED_Update_J1(0x00);
        Display_Digit(2);
        game_state = ESTADO_IDLE;
    }
}

static void Countdown_Task(void)
{
    if (game_state != ESTADO_CUENTA_REGRESIVA) return;

    uint32_t ahora = HAL_GetTick();
    if ((ahora - countdown_last_tick) < COUNTDOWN_STEP_MS) return;
    countdown_last_tick = ahora;

    if (countdown_value > 0)
    {
        countdown_value--;
        Display_Digit(countdown_value);
    }
    else
    {
        Display_Apagar();
        carrera_activa = 1;
        game_state = ESTADO_CARRERA;
    }
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    uint32_t ahora = HAL_GetTick();

    if (GPIO_Pin == BTN_START_Pin)
    {
        if ((ahora - ultimo_tick_start) > DEBOUNCE_MS)
        {
            ultimo_tick_start = ahora;
            inicio_solicitado = 1;
        }
        return;
    }

    if (!carrera_activa) return;

    if (GPIO_Pin == BTN_J1_Pin)
    {
        if ((ahora - ultimo_tick_j1) > DEBOUNCE_MS)
        {
            ultimo_tick_j1 = ahora;
            if (contador_j1 < META)
            {
                contador_j1++;
                LED_Update_J1(contador_j1);
            }
        }
    }
    else if (GPIO_Pin == BTN_J2_Pin)
    {
        if ((ahora - ultimo_tick_j2) > DEBOUNCE_MS)
        {
            ultimo_tick_j2 = ahora;
            if (contador_j2 < META)
            {
                contador_j2++;
                LED_Update_J2(contador_j2);
            }
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
  /* USER CODE BEGIN 2 */
  Reset_Game();
  /* USER CODE END 2 */

  /* Initialize leds */

  /* Initialize USER push-button, will be used to trigger an interrupt each time it's pressed.*/

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {

    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
	  if (inicio_solicitado)
	  {
	      inicio_solicitado = 0;
	      if (game_state == ESTADO_IDLE)
	      {
	          Reset_Game();
	          game_state          = ESTADO_CUENTA_REGRESIVA;
	          countdown_value     = CUENTA_INICIAL;
	          countdown_last_tick = HAL_GetTick();
	          Display_Digit(countdown_value);
	      }
	  }

	  Countdown_Task();
	  Check_Winner();
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
  HAL_GPIO_WritePin(GPIOC, SEG_A_Pin|SEG_B_Pin|SEG_C_Pin|SEG_D_Pin
                          |SEG_E_Pin|SEG_F_Pin|SEG_G_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, LED_J1_0_Pin|LED_J1_1_Pin|LED_J1_2_Pin|LED_J1_3_Pin
                          |LED_J2_0_Pin|LED_J2_1_Pin|LED_J2_2_Pin|LED_J2_3_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pins : BTN_J1_Pin BTN_J2_Pin */
  GPIO_InitStruct.Pin = BTN_J1_Pin|BTN_J2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : SEG_A_Pin SEG_B_Pin SEG_C_Pin SEG_D_Pin
                           SEG_E_Pin SEG_F_Pin SEG_G_Pin */
  GPIO_InitStruct.Pin = SEG_A_Pin|SEG_B_Pin|SEG_C_Pin|SEG_D_Pin
                          |SEG_E_Pin|SEG_F_Pin|SEG_G_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : USART_TX_Pin USART_RX_Pin */
  GPIO_InitStruct.Pin = USART_TX_Pin|USART_RX_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF7_USART2;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : BTN_START_Pin */
  GPIO_InitStruct.Pin = BTN_START_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(BTN_START_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : LED_J1_0_Pin LED_J1_1_Pin LED_J1_2_Pin LED_J1_3_Pin
                           LED_J2_0_Pin LED_J2_1_Pin LED_J2_2_Pin LED_J2_3_Pin */
  GPIO_InitStruct.Pin = LED_J1_0_Pin|LED_J1_1_Pin|LED_J1_2_Pin|LED_J1_3_Pin
                          |LED_J2_0_Pin|LED_J2_1_Pin|LED_J2_2_Pin|LED_J2_3_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI0_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI0_IRQn);

  HAL_NVIC_SetPriority(EXTI1_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI1_IRQn);

  HAL_NVIC_SetPriority(EXTI4_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI4_IRQn);

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
