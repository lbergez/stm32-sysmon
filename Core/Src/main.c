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
#include <string.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

// rewrite enums for less generic wording, could cause problems down the line///////////////////////////////

typedef struct {
  volatile uint32_t MODER, OTYPER, OSPEEDR, PUPDR, IDR, ODR, BSRR, LCKR, AFRL, AFRH;
} gpio;

typedef struct {
  volatile uint32_t CR, PLLCFGR, CFGR, CIR, AHB1RSTR, AHB2RSTR, AHB3RSTR,
      RESERVED0, APB1RSTR, APB2RSTR, RESERVED1, RESERVED2, AHB1ENR, AHB2ENR,
      AHB3ENR, RESERVED3, APB1ENR, APB2ENR, RESERVED4, RESERVED5, AHB1LPENR,
      AHB2LPENR, AHB3LPENR, RESERVED6, APB1LPENR, APB2LPENR, RESERVED7,
      RESERVED8, BDCR, CSR, RESERVED9, RESERVED10, SSCGR, PLLI2SCFGR,
      PLLSAICFGR, DCKCFGR, CKGATENR, DCKCFGR2;
} rcc;

typedef struct {
	volatile uint32_t SPI_CR1, SPI_CR2, SPI_SR, SPI_DR, SPI_CRCPR, SPI_RXCRCR,
	SPI_TXCRCR, SPI_I2SCFGR, SPI_I2SPR;
} my_spi;

typedef struct {
	volatile uint32_t SR, DR, BRR, CR1, CR2, CR3, GTPR;
} usartt;

enum {GPIO_INPUT_MODE, GPIO_OUTPUT_MODE, GPIO_AF_MODE, GPIO_ANALOG_MODE};
enum {LOW_SPEED, MEDIUM_SPEED, FAST_SPEED, HIGH_SPEED};
enum {NONE, PULLUP, PULLDOWN, RESERVED};
enum {PUSH_PULL, OPEN_DRAIN};

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
#define MY_SPI2 ((my_spi *) 0x40003800)

// RGB565 colors (5 bits red, 6 green, 5 blue)
#define BLACK 0x0000
#define WHITE 0xFFFF
#define GRAY  0x8410
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/


/* USER CODE BEGIN PV */

// 5x7 font for printable ascii (' ' to '~'), standard public domain glcd font
// each character is 5 bytes = 5 columns, left to right
// in each byte bit 0 is the TOP row and bit 6 is the bottom row (bit 7 unused)
// ex 'L' = 0x7F,0x40,0x40,0x40,0x40 -> first column all on, then only the bottom row on
static const uint8_t font5x7[95][5] = {
	{0x00,0x00,0x00,0x00,0x00}, // space
	{0x00,0x00,0x5F,0x00,0x00}, // !
	{0x00,0x07,0x00,0x07,0x00}, // "
	{0x14,0x7F,0x14,0x7F,0x14}, // #
	{0x24,0x2A,0x7F,0x2A,0x12}, // $
	{0x23,0x13,0x08,0x64,0x62}, // %
	{0x36,0x49,0x55,0x22,0x50}, // &
	{0x00,0x05,0x03,0x00,0x00}, // '
	{0x00,0x1C,0x22,0x41,0x00}, // (
	{0x00,0x41,0x22,0x1C,0x00}, // )
	{0x08,0x2A,0x1C,0x2A,0x08}, // *
	{0x08,0x08,0x3E,0x08,0x08}, // +
	{0x00,0x50,0x30,0x00,0x00}, // ,
	{0x08,0x08,0x08,0x08,0x08}, // -
	{0x00,0x60,0x60,0x00,0x00}, // .
	{0x20,0x10,0x08,0x04,0x02}, // /
	{0x3E,0x51,0x49,0x45,0x3E}, // 0
	{0x00,0x42,0x7F,0x40,0x00}, // 1
	{0x42,0x61,0x51,0x49,0x46}, // 2
	{0x21,0x41,0x45,0x4B,0x31}, // 3
	{0x18,0x14,0x12,0x7F,0x10}, // 4
	{0x27,0x45,0x45,0x45,0x39}, // 5
	{0x3C,0x4A,0x49,0x49,0x30}, // 6
	{0x01,0x71,0x09,0x05,0x03}, // 7
	{0x36,0x49,0x49,0x49,0x36}, // 8
	{0x06,0x49,0x49,0x29,0x1E}, // 9
	{0x00,0x36,0x36,0x00,0x00}, // :
	{0x00,0x56,0x36,0x00,0x00}, // ;
	{0x08,0x14,0x22,0x41,0x00}, // <
	{0x14,0x14,0x14,0x14,0x14}, // =
	{0x00,0x41,0x22,0x14,0x08}, // >
	{0x02,0x01,0x51,0x09,0x06}, // ?
	{0x32,0x49,0x79,0x41,0x3E}, // @
	{0x7E,0x11,0x11,0x11,0x7E}, // A
	{0x7F,0x49,0x49,0x49,0x36}, // B
	{0x3E,0x41,0x41,0x41,0x22}, // C
	{0x7F,0x41,0x41,0x22,0x1C}, // D
	{0x7F,0x49,0x49,0x49,0x41}, // E
	{0x7F,0x09,0x09,0x09,0x01}, // F
	{0x3E,0x41,0x49,0x49,0x7A}, // G
	{0x7F,0x08,0x08,0x08,0x7F}, // H
	{0x00,0x41,0x7F,0x41,0x00}, // I
	{0x20,0x40,0x41,0x3F,0x01}, // J
	{0x7F,0x08,0x14,0x22,0x41}, // K
	{0x7F,0x40,0x40,0x40,0x40}, // L
	{0x7F,0x02,0x0C,0x02,0x7F}, // M
	{0x7F,0x04,0x08,0x10,0x7F}, // N
	{0x3E,0x41,0x41,0x41,0x3E}, // O
	{0x7F,0x09,0x09,0x09,0x06}, // P
	{0x3E,0x41,0x51,0x21,0x5E}, // Q
	{0x7F,0x09,0x19,0x29,0x46}, // R
	{0x46,0x49,0x49,0x49,0x31}, // S
	{0x01,0x01,0x7F,0x01,0x01}, // T
	{0x3F,0x40,0x40,0x40,0x3F}, // U
	{0x1F,0x20,0x40,0x20,0x1F}, // V
	{0x3F,0x40,0x38,0x40,0x3F}, // W
	{0x63,0x14,0x08,0x14,0x63}, // X
	{0x07,0x08,0x70,0x08,0x07}, // Y
	{0x61,0x51,0x49,0x45,0x43}, // Z
	{0x00,0x7F,0x41,0x41,0x00}, // [
	{0x02,0x04,0x08,0x10,0x20}, // back slash
	{0x00,0x41,0x41,0x7F,0x00}, // ]
	{0x04,0x02,0x01,0x02,0x04}, // ^
	{0x40,0x40,0x40,0x40,0x40}, // _
	{0x00,0x01,0x02,0x04,0x00}, // `
	{0x20,0x54,0x54,0x54,0x78}, // a
	{0x7F,0x48,0x44,0x44,0x38}, // b
	{0x38,0x44,0x44,0x44,0x20}, // c
	{0x38,0x44,0x44,0x48,0x7F}, // d
	{0x38,0x54,0x54,0x54,0x18}, // e
	{0x08,0x7E,0x09,0x01,0x02}, // f
	{0x0C,0x52,0x52,0x52,0x3E}, // g
	{0x7F,0x08,0x04,0x04,0x78}, // h
	{0x00,0x44,0x7D,0x40,0x00}, // i
	{0x20,0x40,0x44,0x3D,0x00}, // j
	{0x7F,0x10,0x28,0x44,0x00}, // k
	{0x00,0x41,0x7F,0x40,0x00}, // l
	{0x7C,0x04,0x18,0x04,0x78}, // m
	{0x7C,0x08,0x04,0x04,0x78}, // n
	{0x38,0x44,0x44,0x44,0x38}, // o
	{0x7C,0x14,0x14,0x14,0x08}, // p
	{0x08,0x14,0x14,0x18,0x7C}, // q
	{0x7C,0x08,0x04,0x04,0x08}, // r
	{0x48,0x54,0x54,0x54,0x20}, // s
	{0x04,0x3F,0x44,0x40,0x20}, // t
	{0x3C,0x40,0x40,0x20,0x7C}, // u
	{0x1C,0x20,0x40,0x20,0x1C}, // v
	{0x3C,0x40,0x30,0x40,0x3C}, // w
	{0x44,0x28,0x10,0x28,0x44}, // x
	{0x0C,0x50,0x50,0x50,0x3C}, // y
	{0x44,0x64,0x54,0x4C,0x44}, // z
	{0x00,0x08,0x36,0x41,0x00}, // {
	{0x00,0x00,0x7F,0x00,0x00}, // |
	{0x00,0x41,0x36,0x08,0x00}, // }
	{0x08,0x04,0x08,0x10,0x08}, // ~
};

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
  g->BSRR = (1U << PINNO(pin)) << (val ? 0 : 16);  // = and not |= bc/ BSRR is atomic
}

static inline bool gpio_read(uint16_t pin) {
  gpio *g = GPIO(PINBANK(pin));
  return g->IDR & (1U << PINNO(pin));
}

// (mode & 3U) is used to make sure that whatever mode is passed into the function is only two bits wide
// saving us from potential memory leakage with numbers larger than two bits long
static inline void gpio_speed(uint16_t pin, uint8_t mode) {
	gpio *g = GPIO(PINBANK(pin));
	g->OSPEEDR &= ~(3U << (PINNO(pin) * 2));
	g->OSPEEDR |= ((mode & 3U) << PINNO(pin) * 2);
}

static inline void gpio_set_pull(uint16_t pin, uint8_t mode) {
	gpio *g = GPIO(PINBANK(pin));
	g->PUPDR &= ~(3U << (PINNO(pin) * 2));
	g->PUPDR |= ((mode & 3U) << (PINNO(pin) * 2));
}

void gpio_set_otyper(uint16_t pin, uint8_t mode) {
	gpio *g = GPIO(PINBANK(pin));
	g->OTYPER &= ~(1U << (PINNO(pin)));
	g->OTYPER |= ((1U & mode) << PINNO(pin));
}

void gpio_set_af(uint16_t pin, uint8_t af_num) {
	gpio *gpio_port = GPIO(PINBANK(pin));
	gpio_set_mode_x(pin, GPIO_AF_MODE);
	if (PINNO(pin) < 8) {
		gpio_port->AFRL &= ~(0xFU << (PINNO(pin) * 4));
		gpio_port->AFRL |= ((af_num & 0xFU) << (PINNO(pin) * 4));
	} else {
		gpio_port->AFRH &= ~(0xFU << ((PINNO(pin) - 8) * 4));
		gpio_port->AFRH |= ((af_num & 0xFU) << ((PINNO(pin) - 8)) * 4);
	}
}

void uart_setup(uint32_t baud) { // come up with func for baud to get rid of baud pass in setup, not bad actually oculd hardcode baud val
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

void uart_write_byte (uint32_t byte) {
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

// prints a 32-bit value as hex, e.g. 0x0000034C
void uart_write_hex(uint32_t val) {
	const char digits[] = "0123456789ABCDEF";
	uart_write_string("0x");
	for (int shift = 28; shift >= 0; shift -= 4) {
		uart_write_byte(digits[(val >> shift) & 0xF]);
	}
	uart_write_string("\r\n");
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

static inline void spin(volatile uint32_t count) {
  while (count--) asm("nop");
}

static void delay_ms(uint32_t ms) {
    spin(21000 * ms);
}

void spi_setup() {

	// enable clock
	MY_RCC->APB1ENR |= (BIT(14)); // spi
	MY_RCC->AHB1ENR |= (BIT(0) | BIT(1)) | (BIT(2)); // gpio a, b, c
	// set af pins
	gpio_set_af(PIN('C', 1), 7);
	gpio_set_af(PIN('C', 2), 5);
	gpio_set_af(PIN('B', 10), 5);
	// set floating and pull-up
	gpio_set_pull(PIN('C', 1), NONE);
	gpio_set_pull(PIN('C', 2), PULLUP);
	gpio_set_pull(PIN('B', 10), NONE);
	// set push-pull, should be redundant but whatever
	gpio_set_otyper(PIN('C', 1), PUSH_PULL);
	gpio_set_otyper(PIN('C', 2), PUSH_PULL);
	gpio_set_otyper(PIN('B', 10), PUSH_PULL);
	// set ospeeder
	gpio_speed(PIN('C', 1), HIGH_SPEED);
	gpio_speed(PIN('B', 10), HIGH_SPEED);

	// put before set_mode so that CS is always 1 and doesn't set 0 before gpio_write
	gpio_write(PIN('B', 6), 1);
	gpio_write(PIN('A', 9), 1);

	gpio_set_mode_x(PIN('B', 6), GPIO_OUTPUT_MODE);
	gpio_set_mode_x(PIN('A', 8), GPIO_OUTPUT_MODE);
	gpio_set_mode_x(PIN('A', 9), GPIO_OUTPUT_MODE);

	// configure SPI control register 1 (SPI_CR1)
	MY_SPI2->SPI_CR1 = BIT(2) | (1U << 3) | BIT(8) | BIT(9) | BIT(6);
}

uint8_t spi_write_byte(uint8_t byte) {
	while ((MY_SPI2->SPI_SR & (1 << 1)) == 0);
	MY_SPI2->SPI_DR = byte;
	while ((MY_SPI2->SPI_SR & (1)) == 0);

	uint8_t return_byte = MY_SPI2->SPI_DR;
	return return_byte;
}

void spi_busy_wait() {
	while (!(MY_SPI2->SPI_SR & (1 << 7)) == 0);
}

// PB6->CS, PA8->DC, PA9->RST

void display_command(uint8_t cmd, uint8_t param[], uint8_t length) {
	gpio_write(PIN('B', 6), 0);  // set CS low
	gpio_write(PIN('A', 8), 0); // set DC low bc/ low = command
	spi_write_byte(cmd);
	spi_busy_wait();
	gpio_write(PIN('A', 8), 1); // set DC high bc/ high = data
	for (int i = 0; i < length; i++) {
		spi_write_byte(param[i]);
		spi_busy_wait();
	}
	gpio_write(PIN('A', 8), 0);  // DC low is our default
	gpio_write(PIN('B', 6), 1);  // set CS high
}

void display_init() {
	uint8_t col_mod_16bit[] = {0x55};
	uint8_t madctl[] = {0x88};  // MY (flip rows so 0,0 is by the header pins) + BGR

	// reset the display
	gpio_write(PIN('A', 9), 0);
	delay_ms(1);
	gpio_write(PIN('A', 9), 1);
	delay_ms(120);

	display_command(0x11, NULL, 0); // sleep out
	delay_ms(5);
	display_command(0x3A, col_mod_16bit, 1);
	display_command(0x36, madctl, 1);
	display_command(0x29, NULL, 0);
}


// C will only take the first 8 bits for each parameter, so we have to shift higher bits to bottom 8
void display_window_set(int x_start, int x_end, int y_start, int y_end) {
	uint8_t caset[] = {x_start >> 8, x_start, x_end >> 8, x_end};
	uint8_t paset[] = {y_start >> 8, y_start, y_end >> 8, y_end};
	display_command(0x2A, caset, 4);
	display_command(0x2B, paset, 4);
}


// tried to do a spi_command but since i need to iterate, I can instead send the command byte and since the command
// doesn't have a parameter limit, I can spam the parameters then
void display_fill_area(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t color) {
	display_window_set(x, x + width - 1, y, y + height - 1);
	gpio_write(PIN('B', 6), 0);  // set CS low
	gpio_write(PIN('A', 8), 0);  // DC low: command (it's already low by default, but don't rely on that)
	spi_write_byte(0x2C);
	spi_busy_wait();
	gpio_write(PIN('A', 8), 1); // set DC high bc/ high = data
	for (int i = 0; i < width * height; i++) {
		spi_write_byte(color >> 8); // high bit first
		spi_write_byte(color);
		spi_busy_wait();
	}
	gpio_write(PIN('A', 8), 0);  // set DC low, back to default
	gpio_write(PIN('B', 6), 1);  // set CS back high
}

void display_draw_character(uint16_t x, uint16_t y, char c, uint16_t color, uint16_t bg, uint8_t scale) {
	if (c < ' ' || c > '~') c = '?';
	const uint8_t *glyph = font5x7[c - ' '];

	uint16_t width = 6 * scale;
	uint16_t length = 8 * scale;
	display_window_set(x, x + width - 1, y, y + length - 1);

	gpio_write(PIN('B', 6), 0);
	gpio_write(PIN('A', 8), 0);
	spi_write_byte(0x2C);
	spi_busy_wait();
	gpio_write(PIN('A', 8), 1);

	for (int i = 0; i < length; i++) {
		uint16_t scaled_row = i / scale;
		uint16_t pixel = bg;
		for (int j = 0; j < width; j++) {
			uint16_t scaled_col = j / scale;
			if (scaled_col < 5 && scaled_row < 7 && ((glyph[scaled_col] >> scaled_row) & 1)){
				pixel = color;
			} else {
				pixel = bg;
			}
			spi_write_byte(pixel >> 8);
			spi_write_byte(pixel);
		}
	}
	spi_busy_wait();
	gpio_write(PIN('A', 8), 0);  // set DC low, back to default
	gpio_write(PIN('B', 6), 1);  // set CS back high
}

void display_draw_string(uint16_t x, uint16_t y, char *string, uint16_t color, uint16_t bg, uint8_t scale) {
	uint16_t string_length = strlen(string);
	for (int i = 0; string[i] != '\0' && i < string_length; i++) {
		display_draw_character(x + (i * 6 * scale), y , string[i], color, bg, scale);
	}
}

void uart_read_line(char *buf, int max_length) {
	int len = 0;
	while (1) {
		uint8_t byte = uart_read_byte();
		if (byte == '\n') {
			break;
		} else if (byte == '\r') {
			continue;
		} else if (len < max_length - 1) {
			buf[len] = byte;
			len++;
		}
	}
	buf[len] = '\0';
}


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
  SystemClock_Config();



  /* USER CODE BEGIN 2 */
  uart_setup(115200);
  spi_setup();
  display_init();

  display_fill_area(0, 0, 240, 320, BLACK);  // clear the screen

  // title and buffer line below
  display_draw_string(10, 12, "SYSMON", WHITE, BLACK, 3);
  display_fill_area(10, 42, 220, 1, GRAY);

  // labels for data
  display_draw_string(10, 56, "CPU", GRAY, BLACK, 2);
  display_draw_string(10, 84, "RAM", GRAY, BLACK, 2);
  display_draw_string(10, 112, "MEM", GRAY, BLACK, 2);
  display_draw_string(10, 140, "GPU", GRAY, BLACK, 2);
  display_draw_string(10, 168, "UP", GRAY, BLACK, 2);

  // default to --
  display_draw_string(106, 56, "--", WHITE, BLACK, 2);
  display_draw_string(106, 84, "--", WHITE, BLACK, 2);
  display_draw_string(106, 112, "--", WHITE, BLACK, 2);
  display_draw_string(106, 140, "--", WHITE, BLACK, 2);
  display_draw_string(106, 168, "--", WHITE, BLACK, 2);

  char line[64];
  char text[20];
  int cpu, ram, used_mb, total_mb, gpu, uptime;



  /* USER CODE END 2 */

  while (1)
  {
    /* USER CODE BEGIN WHILE */

	  // pc sends cpu,ram,used_mb,total_mb,gpu,uptime
	  // cpu ram and gpu are x10 and gpu = -1 means no gpu
	  uart_read_line(line, 64);

	  // skip if sscanf doesn't read 6 bc/ that means the line got cutoff and shouldn't be read
	  if (sscanf(line, "%d,%d,%d,%d,%d,%d", &cpu, &ram, &used_mb, &total_mb, &gpu, &uptime) != 6) {
		  continue;
	  }

	  display_fill_area(106, 56, 134, 16, BLACK);
	  sprintf(text, "%d.%d%%", cpu / 10, cpu % 10);
	  display_draw_string(106, 56, text, WHITE, BLACK, 2);

	  display_fill_area(106, 84, 134, 16, BLACK);
	  sprintf(text, "%d.%d%%", ram / 10, ram % 10);
	  display_draw_string(106, 84, text, WHITE, BLACK, 2);


	  int used_gb = used_mb * 10 / 1024;
	  int total_gb = total_mb * 10 / 1024;
	  display_fill_area(106, 112, 134, 16, BLACK);
	  sprintf(text, "%d.%d/%d.%dG", used_gb / 10, used_gb % 10, total_gb / 10, total_gb % 10);
	  display_draw_string(106, 112, text, WHITE, BLACK, 2);

	  display_fill_area(106, 140, 134, 16, BLACK);
	  if (gpu < 0) {
		  display_draw_string(106, 140, "N/A", WHITE, BLACK, 2);
	  } else {
		  sprintf(text, "%d.%d%%", gpu / 10, gpu % 10);
		  display_draw_string(106, 140, text, WHITE, BLACK, 2);
	  }


	  int hours = uptime / 3600;
	  int minutes = (uptime / 60) % 60;
	  int seconds = uptime % 60;
	  display_fill_area(106, 168, 134, 16, BLACK);
	  sprintf(text, "%02d:%02d:%02d", hours, minutes, seconds);
	  display_draw_string(106, 168, text, WHITE, BLACK, 2);

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
