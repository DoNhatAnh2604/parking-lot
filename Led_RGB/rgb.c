#include "rgb.h"

/* Pin and GPIO definitions for the RGB LED */
#define RED_PIN     8       /* PA8  - TIM1_CH1 */
#define GREEN_PIN   9       /* PA9  - TIM1_CH2 */
#define BLUE_PIN    10      /* PA10 - TIM1_CH3 */
#define RGB_GPIO    GPIOA

/* PWM configuration constants */
#define PWM_TARGET_HZ   1000U     /* Target PWM frequency: ~1 kHz */
#define PWM_ARR         999U      /* Auto-reload register value for 1 kHz at a specific timer clock */

/**
 * @brief Simple software delay function.
 * @param ms The number of milliseconds to delay.
 * @note This is a blocking delay and its accuracy depends on the system clock.
 */
static inline void delay_ms(volatile uint32_t ms) {
    while (ms--) for (volatile uint32_t i=0; i<14000; ++i) __NOP(); /* Loop calibrated for ~1ms at 84MHz */
}

/**
 * @brief Computes the timer prescaler value.
 * @param timclk_hz The timer's input clock frequency in Hz.
 * @param fpwm_hz The desired PWM frequency in Hz.
 * @param arr The auto-reload register value.
 * @return The calculated prescaler value (PSC register value).
 */
static uint16_t compute_psc(uint32_t timclk_hz, uint32_t fpwm_hz, uint32_t arr) {
    /* Formula: PSC = (timclk_hz / (fpwm_hz * (arr + 1))) - 1 */
    uint32_t denom = fpwm_hz * (arr + 1U);
    if (!denom) denom = 1U; /* Avoid division by zero */
    uint32_t psc = (timclk_hz + denom - 1U) / denom; /* Calculate with ceiling division */
    if (psc == 0) psc = 1;
    return (uint16_t)(psc - 1U);
}

/**
 * @brief Scales an 8-bit color value (0-255) to the timer's ARR range (0-PWM_ARR).
 * @param v The 8-bit color value.
 * @return The scaled value for the CCR register.
 */
static inline uint16_t scale8_to_arr(uint8_t v) {
    /* Maps 0..255 -> 0..PWM_ARR with rounding */
    uint32_t x = (uint32_t)v * (uint32_t)PWM_ARR + 127U;
    return (uint16_t)(x / 255U);
}

/**
 * @brief Initializes GPIO and TIM1 for RGB LED PWM control.
 */
void RGB_Init(void) {
    /* Enable clocks for GPIOA and TIM1 */
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    RCC->APB2ENR |= RCC_APB2ENR_TIM1EN;

    /* Configure pins PA8, PA9, PA10 as Alternate Function */
    RGB_GPIO->MODER &= ~((3U << (RED_PIN * 2)) | (3U << (GREEN_PIN * 2))
            | (3U << (BLUE_PIN * 2))); /* Clear mode bits */
    RGB_GPIO->MODER |= ((2U << (RED_PIN * 2)) | (2U << (GREEN_PIN * 2))
            | (2U << (BLUE_PIN * 2))); /* Set to Alternate Function mode */
    /* Select AF1 (TIM1) for the pins */
    RGB_GPIO->AFR[1] &= ~((0xFU << ((RED_PIN - 8) * 4))
            | (0xFU << ((GREEN_PIN - 8) * 4)) | (0xFU << ((BLUE_PIN - 8) * 4)));
    RGB_GPIO->AFR[1] |= ((1U << ((RED_PIN - 8) * 4))
            | (1U << ((GREEN_PIN - 8) * 4)) | (1U << ((BLUE_PIN - 8) * 4)));

    /* Configure pin output speed to medium */
    RGB_GPIO->OSPEEDR &= ~((3U << (RED_PIN * 2)) | (3U << (GREEN_PIN * 2))
            | (3U << (BLUE_PIN * 2)));
    RGB_GPIO->OSPEEDR |= ((2U << (RED_PIN * 2)) | (2U << (GREEN_PIN * 2))
            | (2U << (BLUE_PIN * 2)));

    /* Configure TIM1 for PWM generation */
    uint32_t timclk_hz = 168000000U; /* APB2 Timer Clock is 168 MHz for F411 */
    TIM1->PSC = compute_psc(timclk_hz, PWM_TARGET_HZ, PWM_ARR);
    TIM1->ARR = PWM_ARR;

    /* Reset duty cycles (Capture/Compare Registers) */
    TIM1->CCR1 = 0; /* Red channel */
    TIM1->CCR2 = 0; /* Green channel */
    TIM1->CCR3 = 0; /* Blue channel */

    /* Configure PWM mode 1 and enable preload for all channels */
    /* CH1/CH2 are in CCMR1, CH3/CH4 are in CCMR2 */

    /* Channel 1 (Red) */
    TIM1->CCMR1 &= ~TIM_CCMR1_OC1M;
    TIM1->CCMR1 |= (6U << TIM_CCMR1_OC1M_Pos); /* 110: PWM mode 1 */
    TIM1->CCMR1 |= TIM_CCMR1_OC1PE; /* Enable preload */
    /* Channel 2 (Green) */
    TIM1->CCMR1 &= ~TIM_CCMR1_OC2M;
    TIM1->CCMR1 |= (6U << TIM_CCMR1_OC2M_Pos);
    TIM1->CCMR1 |= TIM_CCMR1_OC2PE;
    /* Channel 3 (Blue) */
    TIM1->CCMR2 &= ~TIM_CCMR2_OC3M;
    TIM1->CCMR2 |= (6U << TIM_CCMR2_OC3M_Pos);
    TIM1->CCMR2 |= TIM_CCMR2_OC3PE;

    /* Set output polarity to active high and enable channels */
    TIM1->CCER &= ~(TIM_CCER_CC1P | TIM_CCER_CC2P | TIM_CCER_CC3P); /* Active high */
    TIM1->CCER |= (TIM_CCER_CC1E | TIM_CCER_CC2E | TIM_CCER_CC3E); /* Enable outputs */

    /* Advanced timers like TIM1 require the Main Output Enable (MOE) bit to be set. */
    TIM1->BDTR |= TIM_BDTR_MOE;

    /* Enable Auto-Reload Preload, generate an update event to load registers, and enable the counter. */
    TIM1->CR1 |= TIM_CR1_ARPE;
    TIM1->EGR |= TIM_EGR_UG;
    TIM1->CR1 |= TIM_CR1_CEN;
}

/**
 * @brief Sets the color of the RGB LED.
 * @param r Red component (0-255).
 * @param g Green component (0-255).
 * @param b Blue component (0-255).
 */
void RGB_SetColor(uint8_t r, uint8_t g, uint8_t b) {
    TIM1->CCR1 = scale8_to_arr(r); /* Set Red duty cycle */
    TIM1->CCR2 = scale8_to_arr(g); /* Set Green duty cycle */
    TIM1->CCR3 = scale8_to_arr(b); /* Set Blue duty cycle */
}
