#include "servo.h"

/**
 * @brief  Gets the alternate function mapping for a given timer.
 * @param  timer Pointer to the TIM_TypeDef peripheral.
 * @return The alternate function value (0-15). Returns 0xFF if timer is not supported.
 * @note   This is specific to STM32F401xx. Check the datasheet for other MCUs.
 */
static uint8_t _servo_get_gpio_af(TIM_TypeDef *timer) {
    if (timer == TIM1 || timer == TIM2) {
        return 1; /* AF1 */
    } else if (timer == TIM3 || timer == TIM4 || timer == TIM5) {
        return 2; /* AF2 */
    } else if (timer == TIM9 || timer == TIM10 || timer == TIM11) {
        return 3; /* AF3 */
    }
    return 0xFF; /* Unsupported timer */
}

/**
 * @brief Initializes the GPIO and Timer for a specific servo motor.
 * @param config Pointer to the Servo_Config_t structure for the servo.
 */
void Servo_Init(Servo_Config_t *config) {
    if (!config || !config->timer || !config->gpio_port) {
        return;
    }

    /* 1. Enable GPIO and Timer Clocks */
    /* Enable GPIO Port Clock */
    if (config->gpio_port == GPIOA) {
        RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    } else if (config->gpio_port == GPIOB) {
        RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;
    } else if (config->gpio_port == GPIOC) {
        RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN;
    } /* Add other ports if needed (GPIOD, etc.) */

    /* Enable Timer Clock */
    if (config->timer == TIM2 || config->timer == TIM3 || config->timer == TIM4 || config->timer == TIM5) {
        RCC->APB1ENR |= (1 << ((uint32_t)config->timer - (uint32_t)TIM2) / 0x400);
    } else if (config->timer == TIM1 || config->timer == TIM9 || config->timer == TIM10 || config->timer == TIM11) {
        /* A bit more complex for APB2 timers due to their register layout */
        if(config->timer == TIM1)  RCC->APB2ENR |= RCC_APB2ENR_TIM1EN;
        if(config->timer == TIM9)  RCC->APB2ENR |= RCC_APB2ENR_TIM9EN;
        if(config->timer == TIM10) RCC->APB2ENR |= RCC_APB2ENR_TIM10EN;
        if(config->timer == TIM11) RCC->APB2ENR |= RCC_APB2ENR_TIM11EN;
    }

    /* 2. Configure GPIO Pin as Alternate Function */
    /* Set mode to Alternate Function (10) */
    config->gpio_port->MODER &= ~(0x03UL << (config->gpio_pin * 2));
    config->gpio_port->MODER |= (0x02UL << (config->gpio_pin * 2));

    /* Set speed to high speed (10) */
    config->gpio_port->OSPEEDR &= ~(0x03UL << (config->gpio_pin * 2));
    config->gpio_port->OSPEEDR |= (0x02UL << (config->gpio_pin * 2));

    /* Configure Alternate Function register */
    uint8_t af_value = _servo_get_gpio_af(config->timer);
    if (af_value == 0xFF) return; /* Stop if timer not supported */

    if (config->gpio_pin < 8) {
        config->gpio_port->AFR[0] &= ~(0x0FUL << (config->gpio_pin * 4));
        config->gpio_port->AFR[0] |= ((uint32_t)af_value << (config->gpio_pin * 4));
    } else {
        config->gpio_port->AFR[1] &= ~(0x0FUL << ((config->gpio_pin - 8) * 4));
        config->gpio_port->AFR[1] |= ((uint32_t)af_value << ((config->gpio_pin - 8) * 4));
    }

    /* 3. Configure Timer for 50Hz PWM */
    /**
     * --- Calculation for 50Hz PWM ---
     * Target Frequency = 50 Hz (20ms period)
     * Assuming Timer Clock (from APB1) is 42 MHz for TIM2-5.
     * Timer_Tick_Frequency = Timer_Clock / (Prescaler + 1)
     * We want a 1MHz tick frequency (1us resolution) for easy pulse width control.
     * Prescaler = (Timer_Clock / 1,000,000) - 1
     * Prescaler = (42,000,000 / 1,000,000) - 1 = 41
     * PWM_Frequency = Timer_Tick_Frequency / (AutoReloadRegister + 1)
     * 50 Hz = 1,000,000 / (ARR + 1)
     * ARR = (1,000,000 / 50) - 1 = 20,000 - 1 = 19999
     */

    config->timer->PSC = 83;
    config->timer->ARR = 19999;

    /* 4. Configure PWM Channel */
    volatile uint32_t *ccmr = (config->channel <= 2) ? &config->timer->CCMR1 : &config->timer->CCMR2;
    uint8_t channel_offset = (config->channel % 2 == 1) ? 0 : 8;

    /* Clear previous settings for the channel */
    *ccmr &= ~(0xFFUL << channel_offset);

    /* Set PWM Mode 1 (OCxM = 110) and enable Preload (OCxPE = 1) */
    *ccmr |= (0x68UL << channel_offset);

    /* Enable the channel output */
    config->timer->CCER |= (1UL << ((config->channel - 1) * 4));

    /* 5. Enable Timer */
    /* Enable Auto-Reload Preload (ARPE) */
    config->timer->CR1 |= TIM_CR1_ARPE;

    /* For advanced timers (TIM1), enable main output */
    if(config->timer == TIM1) {
        config->timer->BDTR |= TIM_BDTR_MOE;
    }

    /* Generate an update event to load the prescaler and ARR */
    config->timer->EGR |= TIM_EGR_UG;

    /* Clear the update flag */
    config->timer->SR &= ~TIM_SR_UIF;

    /* Enable the counter */
    config->timer->CR1 |= TIM_CR1_CEN;

    /* Set initial position to 90 degrees (1.5ms pulse) */
    Servo_SetAngle(config, 90.0f);
}

/**
 * @brief De-initializes the timer associated with a servo.
 * @param config Pointer to the Servo_Config_t structure for the servo.
 */
void Servo_DeInit(Servo_Config_t *config) {
    if (!config || !config->timer) {
        return;
    }
    /* Disable the timer counter */
    config->timer->CR1 &= ~TIM_CR1_CEN;
    /* Note: Disabling clocks is omitted for simplicity, as other servos might */
}

/**
 * @brief Sets the angle of the servo motor.
 * @param config Pointer to the Servo_Config_t structure for the servo.
 * @param angle The desired angle, from 0.0 to 180.0 degrees.
 */
void Servo_SetAngle(Servo_Config_t *config, float angle) {
    /* Clamp the angle to the valid range [0, 180] */
    if (angle < 0.0f) angle = 0.0f;
    if (angle > 180.0f) angle = 180.0f;

    /* Linearly map the angle (0-180) to the pulse width (MIN-MAX) */
    uint16_t pulse_width = SERVO_MIN_PULSE_WIDTH_US +
        (uint16_t)(angle / 180.0f * (SERVO_MAX_PULSE_WIDTH_US - SERVO_MIN_PULSE_WIDTH_US));

    Servo_SetPulseWidth_us(config, pulse_width);
}

/**
 * @brief Sets the raw pulse width for the servo motor.
 * @param config Pointer to the Servo_Config_t structure for the servo.
 * @param pulse_width_us The desired pulse width in microseconds.
 */
void Servo_SetPulseWidth_us(Servo_Config_t *config, uint16_t pulse_width_us) {
    if (!config || !config->timer) {
        return;
    }

    /* The CCRx value directly corresponds to the pulse width in microseconds */
    /* due to the 1MHz timer tick frequency setup in Servo_Init. */
    switch (config->channel) {
        case 1:
            config->timer->CCR1 = pulse_width_us;
            break;
        case 2:
            config->timer->CCR2 = pulse_width_us;
            break;
        case 3:
            config->timer->CCR3 = pulse_width_us;
            break;
        case 4:
            config->timer->CCR4 = pulse_width_us;
            break;
    }
}
