#include "74hc595.h"

/* Segment patterns for digits 0-9 for a common cathode 7-segment display. */
/* (Segments are mapped as: g, f, e, d, c, b, a) */
static const uint8_t seg_cc[10] = {
    0x3F, /* 0 */ 0x06, /* 1 */ 0x5B, /* 2 */ 0x4F, /* 3 */ 0x66, /* 4 */
    0x6D, /* 5 */ 0x7D, /* 6 */ 0x07, /* 7 */ 0x7F, /* 8 */ 0x6F  /* 9 */
};

/**
 * @brief A short, blocking delay.
 * @param t The number of iterations to loop.
 */
static inline void delay_short(volatile uint32_t t) { while(t--) __NOP(); }

/**
 * @brief Initializes the GPIO pins used to control the 74HC595 shift register.
 */
void HC595_Init(void) {
    /* Enable GPIOB clock */
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;
    /* Configure SDI, SCLK, and LOAD pins as general purpose output */
    GPIOB->MODER &= ~((3U<<(SDI_PIN*2))|(3U<<(SCLK_PIN*2))|(3U<<(LOAD_PIN*2)));
    GPIOB->MODER |=  ((1U<<(SDI_PIN*2))|(1U<<(SCLK_PIN*2))|(1U<<(LOAD_PIN*2)));
    /* Initialize pin states to low */
    GPIOB->ODR &= ~((1U<<SDI_PIN)|(1U<<SCLK_PIN)|(1U<<LOAD_PIN));
}

/**
 * @brief Sends a single bit to the shift register's data input (SDI).
 * @param b The bit to send (1 or 0).
 */
static void sendBit(uint8_t b) {
    /* Set SDI pin high or low based on the bit value */
    if (b) GPIOB->ODR |=  (1U << SDI_PIN);
    else   GPIOB->ODR &= ~(1U << SDI_PIN);
    /* Pulse the clock (SCLK) to shift the bit in */
    GPIOB->ODR |=  (1U << SCLK_PIN);
    delay_short(20);
    GPIOB->ODR &= ~(1U << SCLK_PIN);
}

/**
 * @brief Sends a full byte of data to the shift register.
 * @param data The byte to send.
 */
void HC595_SendByte(uint8_t data) {
    /* Send each bit, starting from the most significant bit (MSB) */
    for (int i = 0; i < 8; i++) {
        sendBit((data & 0x80) != 0);
        data <<= 1;
    }
}

/**
 * @brief Latches the data from the shift register to the output register.
 */
void HC595_Latch(void) {
    /* Pulse the load/latch pin (LOAD) to make the sent data appear on the outputs */
    GPIOB->ODR |=  (1U << LOAD_PIN);
    delay_short(40);
    GPIOB->ODR &= ~(1U << LOAD_PIN);
}

/**
 * @brief Encodes a single digit (0-9) into its 7-segment pattern.
 * @param d The digit to encode.
 * @return The corresponding 7-segment code.
 */
static uint8_t encodeDigit(uint8_t d) {
    uint8_t code = seg_cc[d % 10];
#if LED_TYPE == COMMON_ANODE
    /* Invert the code for common anode displays */
    code = ~code;
#endif
    return code;
}

/**
 * @brief Displays a number up to 3 digits on the 7-segment displays.
 * @param num The number to display (0-999).
 */
void HC595_DisplayNumber(uint16_t num) {
    if (num > 999) num = 999;
    /* Extract hundreds, tens, and ones digits */
    uint8_t h = (num/100)%10;
    uint8_t t = (num/10)%10;
    uint8_t o = num%10;

    /* Define the blank pattern based on the LED type */
#if LED_TYPE == COMMON_CATHODE
    uint8_t blank = 0x00; /* All segments off */
#else
    uint8_t blank = 0xFF; /* All segments off (inverted) */
#endif

    /* Encode digits, blanking leading zeros */
    uint8_t bH = (num>=100) ? encodeDigit(h) : blank;
    uint8_t bT = (num>=10) ? encodeDigit(t) : blank;
    uint8_t bO = encodeDigit(o);

    /* Send the encoded bytes to the shift registers in the correct order */
#if ORDER_321 /* Order for digits: Hundreds, Tens, Ones */
    HC595_SendByte(bO);
    HC595_SendByte(bT);
    HC595_SendByte(bH);
#else /* Order for digits: Ones, Tens, Hundreds */
    HC595_SendByte(bH);
    HC595_SendByte(bT);
    HC595_SendByte(bO);
#endif

    /* Latch the data to display it */
    HC595_Latch();
}
