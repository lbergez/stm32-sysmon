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
#include <stdlib.h>
#include <stdbool.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef struct {
  volatile uint32_t MODER, OTYPER, OSPEEDR, PUPDR, IDR, ODR, BSRR, LCKR, AFRL, AFRH;
} gpio;

enum {GPIO_INPUT_MODE, GPIO_OUTPUT_MODE, GPIO_AF_MODE, GPIO_ANALOG_MODE};

typedef struct {
  volatile uint32_t CR, PLLCFGR, CFGR, CIR, AHB1RSTR, AHB2RSTR, AHB3RSTR,
      RESERVED0, APB1RSTR, APB2RSTR, RESERVED1, RESERVED2, AHB1ENR, AHB2ENR,
      AHB3ENR, RESERVED3, APB1ENR, APB2ENR, RESERVED4, RESERVED5, AHB1LPENR,
      AHB2LPENR, AHB3LPENR, RESERVED6, APB1LPENR, APB2LPENR, RESERVED7,
      RESERVED8, BDCR, CSR, RESERVED9, RESERVED10, SSCGR, PLLI2SCFGR,
      PLLSAICFGR, DCKCFGR, CKGATENR, DCKCFGR2;
} rcc;

typedef struct {
	volatile uint32_t SR, DR, BRR, CR1, CR2, CR3, GTPR;
} usartt;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define TIMEOUT 100

#define BIT(x) (1UL << (x))
#define PIN(bank, num) (((bank) - 'A') << 8 | (num))
#define PINNO(pin) ((pin) & 0xFF)
#define PINBANK(pin) ((pin) >> 8)
#define GPIO(bank) ((gpio *) (0x40020000 + (0x400 * (bank))))
#define USART2 ((usartt *) (0x40004400))
#define MY_RCC ((rcc *) 0x40023800)
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/


/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);

/* USER CODE BEGIN PFP */
static inline void gpio_set_mode_x(uint16_t pin, uint8_t mode) {
  gpio *g = GPIO(PINBANK(pin));
  uint8_t n = PINNO(pin);
  g->MODER &= ~(3U << (n * 2));
  g->MODER |= (mode & 3U) << (n * 2);
}



static inline void gpio_write(uint16_t pin, bool val) {
  gpio *g = GPIO(PINBANK(pin));
  g->BSRR |= (1U << PINNO(pin)) << (val ? 0 : 16);
}

static inline bool gpio_read(uint16_t pin) {
  gpio *g = GPIO(PINBANK(pin));
  return g->IDR & (1U << PINNO(pin));
}

enum {LOW_SPEED, MEDIUM_SPEED, FAST_SPEED, HIGH_SPEED};


// (mode & 3U) is used to make sure that whatever mode is passed into the function is only two bits wide
// saving us from potential memory leakage with numbers larger than two bits long
static inline void gpio_speed(uint16_t pin, uint8_t mode) {
	gpio *g = GPIO(PINBANK(pin));
	g->OSPEEDR &= ~(3U << (PINNO(pin) * 2));
	g->OSPEEDR |= ((mode & 3U) << PINNO(pin) * 2);
}

enum {NONE, PULLUP, PULLDOWN, RESERVED};

static inline void gpio_set_pull(uint16_t pin, uint8_t mode) {
	gpio *g = GPIO(PINBANK(pin));
	g->PUPDR &= ~(3U << (PINNO(pin) * 2));
	g->PUPDR |= ((mode & 3U) << (PINNO(pin) * 2));
}

static inline void spin(volatile uint32_t count) {
  while (count--) asm("nop");
}
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
void gpio_set_af(uint16_t pin, uint8_t af_num) {
	gpio *gpio_port = GPIO(PINBANK(pin));
	gpio_set_mode_x(pin, GPIO_AF_MODE);
	if (PINNO(pin) < 8) {
		gpio_port->AFRL &= ~(0xFU << (PINNO(pin) * 4));
		gpio_port->AFRL |= ((af_num) << (PINNO(pin) * 4));
	} else {
		gpio_port->AFRH &= ~(0xFU << ((PINNO(pin) - 8) * 4));
		gpio_port->AFRH |= ((af_num) << ((PINNO(pin) - 8)) * 4);
	}
}




void uart_setup(uint16_t baud) { // come up with func for baud to get rid of baud pass in setup, not bad actually oculd hardcode baud val
	gpio *gpioa = GPIO(0);

	// set clock for gpio and usart
	MY_RCC->APB1ENR |= (1U << 17);
	MY_RCC->AHB1ENR |= BIT(0);
	// gpio AF config
	uint16_t tx_pin = PIN('A', 2);
	uint16_t rx_pin = PIN('A', 3);
	gpio_set_mode_x(tx_pin, GPIO_AF_MODE);
	gpio_set_mode_x(rx_pin, GPIO_AF_MODE);
	// set AF pins
	gpioa->AFRL &= ~((0xFU << 8) | (0xFU << 12));
	gpioa->AFRL |= (7U << 8) | (7U << 12);
	// set high speed
	gpioa->OSPEEDR |= (3U << 4) | (3U << 6);
	//BRR calc
	USART2->BRR = 42000000 / baud; // 42MHz, 115200 baud
	// enable UE,TE,RE
	USART2->CR1 |= (1U << 2) | (1U << 3) | (1U << 13);
}

void uart_write_byte (uint8_t byte) {
	while ((USART2->SR & (1 << 7)) == 0);
	USART2->DR = byte;
}

// in C it's always good to pass both the string and the string length to stop
// potential errors with no null character being present
void uart_write_string (char* string) {
	int stringlength = strlen(string);
	for (int i = 0; string[i] != '\0' && i < stringlength; i++) {
		uart_write_byte(string[i]);
	}
}

bool uart_available () {
	if ((USART2->SR & (1 << 5)) != 0) { // try to define all the bits for the sr registers for like 1 shifted by 5, can be changed to a define -> makes code more readable
		return true;
	} else {
		return false;
	}
}

uint8_t uart_read_byte(void) {
	while (uart_available() == false);
	return USART2->DR;
}





int main(void)
{
  SystemClock_Config();



  /* USER CODE BEGIN 2 */

  uart_setup();

  gpio *gpioa = GPIO(0);
  usartt *usart = USART2;
  uint16_t main_uart = PIN('A', 2);
  gpio_set_mode_x(main_uart, GPIO_AF_MODE);
  gpioa->AFRL &= ~(0xFU << 8);
  gpioa->AFRL |= (7U << 8);
  gpioa->OSPEEDER |= (2U << 4);
  usart->BRR = 364;
  usart->CR1 |= (1U << 13) | (1U << 3);
  while (!(usart->SR & (1U << 7)));
  usart->DR = 'A';















  uint16_t led = PIN('A', 5);
  MY_RCC->AHB1ENR |= BIT(PINBANK(led));
  gpio_set_mode_x(led, GPIO_OUTPUT_MODE);
  /* USER CODE END 2 */

  while (1)
  {
    /* USER CODE BEGIN WHILE */
    gpio_write(led, true);
    spin(99999);
    gpio_write(led, false);
    spin(99999);
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    /* USER CODE END 3 */
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
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */


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

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI15_10_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin){
	HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);
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
