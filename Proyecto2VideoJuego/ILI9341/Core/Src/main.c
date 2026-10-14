/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2024 STMicroelectronics.
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
#include "fatfs.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "ili9341.h"
#include "gfx.h"
#include "screens.h"
#include "controller.h"
#include "padtest.h"
#include "sound.h"
#include "game.h"
#include "storage.h"
#include "sprites.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef enum { ST_MENU, ST_PLAY, ST_WIN, ST_OVER, ST_PADTEST, ST_GALLERY, ST_RECORDS, ST_CREDITS } GameState;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define PADTEST_AL_INICIO  0     /* 1 = arranca en la pantalla de prueba del mando */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
SPI_HandleTypeDef hspi1;

TIM_HandleTypeDef htim3;

UART_HandleTypeDef huart5;
UART_HandleTypeDef huart3;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_SPI1_Init(void);
static void MX_USART3_UART_Init(void);
static void MX_TIM3_Init(void);
static void MX_UART5_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
static uint8_t btn_poll(void)
{
    static uint8_t down = 0;
    static uint32_t t0 = 0;
    uint8_t pressed = (HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_13) == GPIO_PIN_RESET);

    if (pressed && !down) { down = 1; t0 = HAL_GetTick(); }
    else if (!pressed && down) {
        down = 0;
        uint32_t dt = HAL_GetTick() - t0;
        if (dt < 30) return 0;                 // antirrebote
        return (dt >= 500) ? 2 : 1;
    }
    return 0;
}

/* Muestra el menu con su musica */
static void go_menu(uint8_t sel, uint8_t level)
{
    screens_menu_full(sel, level);
    sound_music(MUS_TITLE);
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
  MX_SPI1_Init();
  MX_USART3_UART_Init();
  MX_TIM3_Init();
  MX_FATFS_Init();
  MX_UART5_Init();
  /* USER CODE BEGIN 2 */
  LCD_Init();
  pad_init();                    // empieza a recibir el mando por USART3
  sound_init();                  // buzzer pasivo en PC6 (TIM3_CH1)

  GameState state = ST_MENU;
  uint8_t  sel = 0;              // opcion del menu
  uint8_t  level = 1;            // nivel elegido
  uint8_t  pend = 0;             // evento pendiente de B1 (1 corto, 2 largo)
  uint8_t  pend_btn = 0;         // botones del mando presionados y sin atender
  uint16_t best[NUM_LEVELS] = {0};   // records (en RAM; luego se guardan en la SD)
  uint32_t last_status = 0;
  uint32_t last_draw = 0;
  uint32_t last_frame = 0;

  GameStats stats = {0};             // partidas, victorias, derrotas (STATS.TXT)
  static char creditos[480];         // texto de CREDITOS.TXT
  uint8_t creditos_ok = 0;

  /* Carga de la SD: graficos, portada, fondo, niveles, records y estadisticas */
  LCD_Clear(0x0000);
  LCD_Print("CARGANDO SD...", 104, 112, 1, 0xFFFF, 0x0000);
  storage_init();
  if (!storage_load_sprites()) {     // graficos: SD -> RAM
    sprites_from_flash();            // respaldo (si SPRITES_EN_FLASH = 1)
#if !SPRITES_EN_FLASH
    LCD_Print("INSERTA LA MEMORIA SD", 76, 140, 1, 0xF800, 0x0000);
    LCD_Print("(LOS GRAFICOS ESTAN AHI)", 64, 156, 1, 0xF800, 0x0000);
    HAL_Delay(3000);
#endif
  }
  storage_load_records(best, NUM_LEVELS);
  storage_load_stats(&stats);
  creditos_ok = (uint8_t)storage_load_text("CREDITOS.TXT", creditos, sizeof(creditos));

  /* Portada (imagen de 320x240 leida de la SD): 3 s o hasta presionar un boton */
  if (storage_show_image("PORTADA.BIN")) {
    uint32_t t0 = HAL_GetTick();
    while (HAL_GetTick() - t0 < 3000) {
      pad_poll();
      if (btn_poll() || pad_take_presses()) break;
    }
  }

#if PADTEST_AL_INICIO
  state = ST_PADTEST;
  padtest_full();
#else
  go_menu(sel, level);
#endif
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
	while (1) {



    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
		sound_update();                                 // avanza la musica / efectos (no bloquea)

		uint8_t ev = btn_poll();                        // boton B1 de la Nucleo
		if (ev) pend = ev;

		pad_poll();                                     // procesa lo que llego del mando
		pend_btn |= pad_take_presses();                 // se acumulan hasta que se atienden
		const PadState *pad = pad_get();

		/* Estado del juego al mando cada 100 ms (fase y poder disponible) */
		if (HAL_GetTick() - last_status >= 100) {
		    last_status = HAL_GetTick();
		    uint8_t st = 0, fl = 0;
		    if (state == ST_PLAY)         { st = game_status_state(); fl = game_status_flags(); }
		    else if (state == ST_WIN)     st = 4;
		    else if (state == ST_OVER)    st = 5;
		    else if (state == ST_PADTEST) fl = (pad->btn & PAD_BTN_POWER) ? 1 : 0;
		    pad_send_status(st, fl);
		}

		switch (state) {

		/* ---------------- Menu ---------------- */
		case ST_MENU: {
		    int8_t nav = pad_nav_y();                   // joystick / cruceta arriba-abajo
		    uint8_t old = sel;
		    if (nav != 0)       sel = (sel + MENU_ITEMS + nav) % MENU_ITEMS;
		    else if (pend == 1) sel = (sel + 1) % MENU_ITEMS;   // B1 corto: siguiente opcion
		    if (sel != old) { screens_menu_select(old, sel); sound_sfx(SFX_MOVE); }

		    if (pend_btn & PAD_BTN_MENU) {              // clic / Start: prueba del mando
		        sound_music(MUS_NONE);
		        padtest_full();
		        state = ST_PADTEST;
		    } else if (pend_btn & PAD_BTN_POWER) {      // PODER: galeria de personajes
		        sound_sfx(SFX_OK);
		        screens_gallery();
		        state = ST_GALLERY;
		    } else if (pend == 2 || (pend_btn & PAD_BTN_FIRE)) {   // B1 largo o DISPARO: aceptar
		        sound_sfx(SFX_OK);
		        if (sel == 0) {                         // JUGAR
		            game_start(level);
		            sound_music(MUS_LEVEL);
		            last_frame = HAL_GetTick();
		            state = ST_PLAY;
		        } else if (sel == 1) {                  // cambiar de NIVEL
		            level = (level % NUM_LEVELS) + 1;
		            screens_menu_level(level, sel);
		        } else if (sel == 2) {                  // RECORDS
		            screens_records(best, stats.partidas, stats.victorias, stats.derrotas);
		            state = ST_RECORDS;
		        } else {                                // CREDITOS
		            screens_credits(creditos_ok ? creditos : NULL);
		            state = ST_CREDITS;
		        }
		    }
		    pend = 0; pend_btn = 0;
		    break;
		}

		/* ---------------- Jugando ---------------- */
		case ST_PLAY:
		    if (HAL_GetTick() - last_frame >= GAME_FRAME_MS) {   // ~30 cuadros por segundo
		        last_frame = HAL_GetTick();
		        GameInput gi = {
		            pad->jx, pad->jy, pad->connected, pend,
		            (uint8_t)((pend_btn & PAD_BTN_FIRE)  != 0),
		            (uint8_t)((pend_btn & PAD_BTN_POWER) != 0),
		            (uint8_t)((pend_btn & PAD_BTN_MENU)  != 0)
		        };
		        pend = 0; pend_btn = 0;

		        GameResult r = game_update(&gi);
		        if (r == GAME_WIN) {
		            uint16_t sc = game_score();
		            uint8_t rec = (sc > best[level - 1]);
		            if (rec) {
		                best[level - 1] = sc;
		                storage_save_records(best, NUM_LEVELS);   // se guarda en la SD
		            }
		            screens_win(game_stars(), sc, rec);
		            sound_music(MUS_WIN);
		            stats.partidas++; stats.victorias++; stats.puntos += sc;
		            storage_save_stats(&stats);             // STATS.TXT en la SD
		            state = ST_WIN;
		        } else if (r == GAME_LOSE) {
		            screens_gameover(game_score());
		            sound_music(MUS_LOSE);
		            stats.partidas++; stats.derrotas++; stats.puntos += game_score();
		            storage_save_stats(&stats);
		            state = ST_OVER;
		        } else if (r == GAME_QUIT) {
		            go_menu(sel, level);
		            state = ST_MENU;
		        }
		    }
		    break;

		/* ---------------- Fin de nivel ---------------- */
		case ST_WIN:
		case ST_OVER:
		    if (pend || (pend_btn & (PAD_BTN_FIRE | PAD_BTN_MENU))) {
		        if (state == ST_WIN) level = (level % NUM_LEVELS) + 1;   // siguiente nivel
		        go_menu(sel, level);
		        state = ST_MENU;
		    }
		    pend = 0; pend_btn = 0;
		    break;

		/* ---------------- Records, creditos y galeria ---------------- */
		case ST_RECORDS:
		case ST_CREDITS:
		case ST_GALLERY:
		    if (pend || (pend_btn & (PAD_BTN_MENU | PAD_BTN_FIRE))) {
		        go_menu(sel, level);
		        state = ST_MENU;
		    }
		    pend = 0; pend_btn = 0;
		    break;

		/* ---------------- Prueba del mando ---------------- */
		case ST_PADTEST:
		    /* prueba del sonido desde el mando: cada boton toca un efecto */
		    if (pend_btn & PAD_BTN_FIRE)  sound_sfx(SFX_LAUNCH);
		    if (pend_btn & PAD_BTN_POWER) sound_sfx(SFX_BOOM);
		    if (pend == 1)                sound_sfx(SFX_PIG);

		    if (pend == 2 || (pend_btn & PAD_BTN_MENU)) {   // B1 largo o clic: volver al menu
		        go_menu(sel, level);
		        state = ST_MENU;
		    } else if (HAL_GetTick() - last_draw >= 40) {   // ~25 refrescos por segundo
		        last_draw = HAL_GetTick();
		        padtest_update(pad);
		    }
		    pend = 0; pend_btn = 0;
		    break;
		}
	}                                                   // <- cierra el while(1)
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
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 80;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
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
  * @brief SPI1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI1_Init(void)
{

  /* USER CODE BEGIN SPI1_Init 0 */

  /* USER CODE END SPI1_Init 0 */

  /* USER CODE BEGIN SPI1_Init 1 */

  /* USER CODE END SPI1_Init 1 */
  /* SPI1 parameter configuration*/
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI1_Init 2 */

  /* USER CODE END SPI1_Init 2 */

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
  htim3.Init.Prescaler = 79;
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period = 65535;
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
  sConfigOC.Pulse = 0;
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
  * @brief UART5 Initialization Function
  * @param None
  * @retval None
  */
static void MX_UART5_Init(void)
{

  /* USER CODE BEGIN UART5_Init 0 */

  /* USER CODE END UART5_Init 0 */

  /* USER CODE BEGIN UART5_Init 1 */

  /* USER CODE END UART5_Init 1 */
  huart5.Instance = UART5;
  huart5.Init.BaudRate = 115200;
  huart5.Init.WordLength = UART_WORDLENGTH_8B;
  huart5.Init.StopBits = UART_STOPBITS_1;
  huart5.Init.Parity = UART_PARITY_NONE;
  huart5.Init.Mode = UART_MODE_TX;
  huart5.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart5.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart5) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN UART5_Init 2 */

  /* USER CODE END UART5_Init 2 */

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
  if (HAL_UART_Init(&huart3) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART3_Init 2 */

  /* USER CODE END USART3_Init 2 */

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
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, LCD_RST_Pin|LCD_D1_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, LCD_RD_Pin|LCD_WR_Pin|LCD_RS_Pin|LCD_D7_Pin
                          |LCD_D0_Pin|LCD_D2_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, LCD_CS_Pin|LCD_D6_Pin|LCD_D3_Pin|LCD_D5_Pin
                          |LCD_D4_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(SD_SS_GPIO_Port, SD_SS_Pin, GPIO_PIN_SET);

  /*Configure GPIO pins : LCD_RST_Pin LCD_D1_Pin */
  GPIO_InitStruct.Pin = LCD_RST_Pin|LCD_D1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : LCD_RD_Pin LCD_WR_Pin LCD_RS_Pin LCD_D7_Pin
                           LCD_D0_Pin LCD_D2_Pin */
  GPIO_InitStruct.Pin = LCD_RD_Pin|LCD_WR_Pin|LCD_RS_Pin|LCD_D7_Pin
                          |LCD_D0_Pin|LCD_D2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : LCD_CS_Pin LCD_D6_Pin LCD_D3_Pin LCD_D5_Pin
                           LCD_D4_Pin SD_SS_Pin */
  GPIO_InitStruct.Pin = LCD_CS_Pin|LCD_D6_Pin|LCD_D3_Pin|LCD_D5_Pin
                          |LCD_D4_Pin|SD_SS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

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
	while (1) {
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
