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
#include "adc.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <string.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define ADC_MITAD               2048U
#define RX_BUFFER_SIZE          32U
#define INTERVALO_JOYSTICK_MS   300U
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
/* Control 1: potenciómetros */
uint32_t valor_pot1 = 0;
uint32_t valor_pot2 = 0;

char mensaje_uart[80];

/* Control 2: recepción desde el ATmega328P */
volatile uint8_t rx_byte = 0;
volatile char rx_buffer[RX_BUFFER_SIZE];
volatile uint8_t rx_index = 0;
volatile uint8_t rx_linea_lista = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
static uint32_t Leer_ADC(uint32_t canal);
static void Procesar_Accion_Control2(const char *linea);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
static uint32_t Leer_ADC(uint32_t canal)
{
    ADC_ChannelConfTypeDef configuracion = {0};
    uint32_t resultado = 0;

    configuracion.Channel = canal;
    configuracion.Rank = 1;
    configuracion.SamplingTime = ADC_SAMPLETIME_56CYCLES;

    if (HAL_ADC_ConfigChannel(
            &hadc1,
                       &configuracion
        ) != HAL_OK)
    {
        Error_Handler();
    }

    if (HAL_ADC_Start(&hadc1) != HAL_OK)
    {
        Error_Handler();
    }

    if (HAL_ADC_PollForConversion(
            &hadc1,
            50
        ) != HAL_OK)
    {
        HAL_ADC_Stop(&hadc1);
        Error_Handler();
    }

    resultado = HAL_ADC_GetValue(&hadc1);

    HAL_ADC_Stop(&hadc1);

    return resultado;
}
static void Procesar_Accion_Control2(const char *linea)
{
    const char *accion = NULL;
    char mensaje_control2[50];

    if (linea == NULL || linea[0] == '\0')
    {
        return;
    }

    switch (linea[0])
    {
        case 'U':
        case 'u':
            accion = "Arriba";
            break;

        case 'D':
        case 'd':
            accion = "Abajo";
            break;

        case 'R':
        case 'r':
            accion = "Derecha";
            break;

        case 'L':
        case 'l':
            accion = "Izquierda";
            break;

        case 'A':
        case 'a':
            accion = "Accion A";
            break;

        case 'B':
        case 'b':
            accion = "Accion B";
            break;

        default:
            accion = NULL;
            break;
    }

    if (accion != NULL)
    {
        int longitud = snprintf(
            mensaje_control2,
            sizeof(mensaje_control2),
            "Control 2 (ATmega): %s\r\n",
            accion
        );

        if (longitud > 0)
        {
            HAL_UART_Transmit(
                &huart2,
                (uint8_t *)mensaje_control2,
                (uint16_t)longitud,
                100
            );
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
  MX_ADC1_Init();
  MX_USART2_UART_Init();
  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */
  const char mensaje_inicio[] =
      "\r\n"
      "========================================\r\n"
      "Consola de videojuegos iniciada\r\n"
      "Control 1: potenciometros por ADC1\r\n"
      "Control 2: ATmega328P por USART1\r\n"
      "Salida PC: USART2 a 115200 baudios\r\n"
      "========================================\r\n\r\n";

  HAL_UART_Transmit(
      &huart2,
      (uint8_t *)mensaje_inicio,
      (uint16_t)strlen(mensaje_inicio),
      100
  );

  if (HAL_UART_Receive_IT(
          &huart1,
          (uint8_t *)&rx_byte,
          1
      ) != HAL_OK)
  {
      const char error_rx[] =
          "ERROR iniciando recepcion USART1\r\n";

      HAL_UART_Transmit(
          &huart2,
          (uint8_t *)error_rx,
          (uint16_t)strlen(error_rx),
          100
      );

      Error_Handler();
  }

  uint32_t ultimo_envio_joystick = HAL_GetTick();
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
	    uint32_t ahora = HAL_GetTick();

	    /*
	     * Control 1: leer los potenciómetros cada 300 ms.
	     */
	    if ((ahora - ultimo_envio_joystick)
	            >= INTERVALO_JOYSTICK_MS)
	    {
	        const char *estado_vertical;
	        const char *estado_horizontal;

	        ultimo_envio_joystick = ahora;

	        valor_pot1 =
	            Leer_ADC(ADC_CHANNEL_1);

	        valor_pot2 =
	            Leer_ADC(ADC_CHANNEL_4);

	        if (valor_pot1 >= ADC_MITAD)
	        {
	            estado_vertical = "Arriba";
	        }
	        else
	        {
	            estado_vertical = "Abajo";
	        }

	        if (valor_pot2 >= ADC_MITAD)
	        {
	            estado_horizontal = "Derecha";
	        }
	        else
	        {
	            estado_horizontal = "Izquierda";
	        }

	        int longitud = snprintf(
	            mensaje_uart,
	            sizeof(mensaje_uart),
	            "Control 1: %s / %s\r\n",
	            estado_vertical,
	            estado_horizontal
	        );

	        if (longitud > 0)
	        {
	            HAL_UART_Transmit(
	                &huart2,
	                (uint8_t *)mensaje_uart,
	                (uint16_t)longitud,
	                100
	            );
	        }
	    }


	    if (rx_linea_lista != 0)
	    {
	        Procesar_Accion_Control2(
	            (const char *)rx_buffer
	        );

	        rx_linea_lista = 0;
	        rx_index = 0;


	        HAL_UART_Receive_IT(
	            &huart1,
	            (uint8_t *)&rx_byte,
	            1
	        );
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

/* USER CODE BEGIN 4 */
void HAL_UART_RxCpltCallback(
    UART_HandleTypeDef *huart
)
{
    if (huart->Instance == USART1)
    {
        if (rx_byte == '\n')
        {

            if (rx_index > 0)
            {
                rx_buffer[rx_index] = '\0';
                rx_linea_lista = 1;

                return;
            }
        }
        else if (rx_byte != '\r')
        {

            if (rx_index < (RX_BUFFER_SIZE - 1U))
            {
                rx_buffer[rx_index] =
                    (char)rx_byte;

                rx_index++;
            }
            else
            {

                rx_index = 0;
            }
        }


        HAL_UART_Receive_IT(
            &huart1,
            (uint8_t *)&rx_byte,
            1
        );
    }
}


void HAL_UART_ErrorCallback(
    UART_HandleTypeDef *huart
)
{
    if (huart->Instance == USART1)
    {
        rx_index = 0;
        rx_linea_lista = 0;

        HAL_UART_Receive_IT(
            &huart1,
            (uint8_t *)&rx_byte,
            1
        );
    }
}
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
