#ifndef INC_74HC595_H_
#define INC_74HC595_H_

#include "stm32f4xx.h"

/* Pin definitions for 74HC595 connection on GPIOB */
#define SDI_PIN   4  /* Serial Data In (DS) */
#define SCLK_PIN  5  /* Shift Register Clock (SHCP) */
#define LOAD_PIN  6  /* Storage Register Clock / Latch (STCP) */

/* 7-segment display type configuration */
#define COMMON_CATHODE  0
#define COMMON_ANODE    1
#define LED_TYPE COMMON_ANODE /* Set the type of 7-segment display used */

/* Order in which digits are shifted out. Set to 1 if the first byte sent corresponds to the 'ones' digit. */
#define ORDER_321 1

/* @brief Initializes GPIO pins for controlling the 74HC595. */
void HC595_Init(void);

/* @brief Sends one byte of data to the shift register. */
void HC595_SendByte(uint8_t data);

/* @brief Latches the data from the shift register to the output pins. */
void HC595_Latch(void);

/* @brief Displays a number (0-999) on the 7-segment displays. */
void HC595_DisplayNumber(uint16_t num);

#endif /* INC_74HC595_H_ */
