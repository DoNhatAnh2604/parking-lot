#ifndef INC_RGB_H_
#define INC_RGB_H_

#include "stm32f4xx.h"

/* @brief Initializes GPIO pins and Timer for RGB LED PWM control. */
void RGB_Init(void);

/* @brief Sets the color of the RGB LED.
 * @param r Red component (0-255).
 * @param g Green component (0-255).
 * @param b Blue component (0-255).
 */
void RGB_SetColor(uint8_t r, uint8_t g, uint8_t b);

#endif /* INC_RGB_H_ */
