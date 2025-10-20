#ifndef __SERVO_H
#define __SERVO_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f4xx.h"
#include <stdint.h>

/**
 * @brief Defines the minimum and maximum pulse width for the servo in microseconds.
 * These values typically correspond to 0 and 180 degrees, respectively.
 * Standard values are 1000us (1ms) for 0 degrees and 2000us (2ms) for 180 degrees.
 * You might need to tune these for your specific servo.
 */
#define SERVO_MIN_PULSE_WIDTH_US 414
#define SERVO_MAX_PULSE_WIDTH_US 2571

/**
 * @brief Structure to hold the configuration for a single servo motor.
 * @note  The user must initialize this structure with the correct GPIO and Timer
 * peripherals for the servo connection.
 */
typedef struct {
    TIM_TypeDef  *timer;       /*!< Pointer to the Timer peripheral (e.g., TIM2, TIM3). */
    uint8_t      channel;      /*!< Timer channel (1-4) connected to the servo. */
    GPIO_TypeDef *gpio_port;   /*!< Pointer to the GPIO port of the servo pin (e.g., GPIOA, GPIOB). */
    uint8_t      gpio_pin;     /*!< GPIO pin number (0-15) connected to the servo. */
} Servo_Config_t;

/**
 * @brief Initializes the GPIO and Timer for a specific servo motor.
 * @param config Pointer to the Servo_Config_t structure for the servo.
 * @note  This function configures the timer to produce a 50Hz PWM signal,
 * which is standard for servo control. It assumes the APB1 clock for the
 * timer is 42MHz (standard on STM32F401 when HCLK is 84MHz).
 */
void Servo_Init(Servo_Config_t *config);

/**
 * @brief De-initializes the timer associated with a servo to save power.
 * @param config Pointer to the Servo_Config_t structure for the servo.
 */
void Servo_DeInit(Servo_Config_t *config);

/**
 * @brief Sets the angle of the servo motor.
 * @param config Pointer to the Servo_Config_t structure for the servo.
 * @param angle The desired angle, from 0.0 to 180.0 degrees. Values outside
 * this range will be clamped.
 */
void Servo_SetAngle(Servo_Config_t *config, float angle);

/**
 * @brief Sets the raw pulse width for the servo motor.
 * @param config Pointer to the Servo_Config_t structure for the servo.
 * @param pulse_width_us The desired pulse width in microseconds. This function
 * is useful for fine-tuning and calibration. It is recommended
 * to keep the value between 500 and 2500.
 */
void Servo_SetPulseWidth_us(Servo_Config_t *config, uint16_t pulse_width_us);


#ifdef __cplusplus
}
#endif

#endif // __SERVO_H
