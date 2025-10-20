#include <rc522.h>
#include "main.h"
#include "servo.h"
#include "delay.h"
#include "lcd_parallel.h"
#include <string.h>
#include <stdio.h>
#include "lcd_config.h"
#include "gpio.h"
#include "74hc595.h"
#include "rgb.h"
#include <stdbool.h>

/* Private function prototypes */
void SystemClock_Config(void);

/* Constants for the parking system */
#define MAX_VEHICLES_INSIDE    4       /* Maximum number of vehicles allowed in the parking lot. */
#define BARRIER_CLOSED_ANGLE   0.0f    /* Servo angle when the barrier is closed. */
#define BARRIER_OPEN_ANGLE     75.0f   /* Servo angle when the barrier is open. */
#define AUTHORIZED_TIMEOUT     10000   /* Timeout in ms to wait for a vehicle after card authorization. */
#define PASSAGE_TIMEOUT        15000   /* Timeout in ms for a vehicle to pass through the gate. */
#define DELAY_BEFORE_CLOSING   2000    /* Delay in ms after a vehicle has passed before closing the barrier. */

/* GPIO definitions for IR sensors */
#define ENTRY_IR_PORT         GPIOA
#define ENTRY_IR_PIN          1
#define EXIT_IR_PORT          GPIOA
#define EXIT_IR_PIN           2

/* Enum for vehicle direction */
typedef enum {
    DIR_NONE, DIR_ENTRY, DIR_EXIT
} VehicleDirection_t;

/* Enum for barrier state machine */
typedef enum {
    STATE_CLOSED,                       /* Barrier is fully closed. */
    STATE_AUTHORIZED_WAITING_VEHICLE,   /* Card is authorized, waiting for vehicle to approach IR sensor. */
    STATE_OPENING,                      /* Barrier is in the process of opening. */
    STATE_OPEN_WAITING_PASSAGE,         /* Barrier is open, waiting for the vehicle to pass completely. */
    STATE_CLOSING,                      /* Barrier is in the process of closing. */
    STATE_WAIT_BEFORE_CLOSING           /* Wait for a short period after the vehicle has passed before closing. */
} BarrierState_t;

/* Array of authorized RFID card UIDs */
const uint8_t VALID_UIDS[][4] = {
        { 0xD3, 0xA7, 0xB1, 0x28 },
        { 0x23, 0xB8, 0x16, 0x2D },
        { 0x93, 0x71, 0x8D, 0x0C },
        { 0x23, 0xA2, 0x5C, 0xFA }
};

/* Calculate the number of valid UIDs */
const uint8_t NUM_VALID_UIDS = sizeof(VALID_UIDS) / sizeof(VALID_UIDS[0]);

/* Database to store UIDs of vehicles currently inside */
uint8_t vehicles_inside_db[MAX_VEHICLES_INSIDE][4];
/* Counter for the number of vehicles inside */
uint8_t vehicle_count = 0;
/* Current direction of the vehicle (entry or exit) */
volatile VehicleDirection_t current_direction = DIR_NONE;

/* Current state of the barrier state machine */
volatile BarrierState_t currentState = STATE_CLOSED;
/* Servo configuration struct */
Servo_Config_t barrierServo;
/* Stores the UID of the last scanned card */
uint8_t current_uid[4];

/* Variables for MFRC522 communication (currently unused) */
uint8_t card_uid[5];
uint8_t card_type[2];

/* Timestamp for state change timeouts */
uint32_t state_change_timestamp;
/* Flag to indicate if a vehicle is currently passing through the IR sensors */
volatile bool vehicle_is_passing = false;

/**
 * @brief Checks if a given UID is in the list of authorized UIDs.
 * @param uid Pointer to the UID array to check.
 * @return true if the card is authorized, false otherwise.
 */
bool is_card_authorized(uint8_t *uid) {
    for (uint8_t i = 0; i < NUM_VALID_UIDS; i++) {
        if (memcmp(uid, VALID_UIDS[i], 4) == 0)
            return true;
    }
    return false;
}

/**
 * @brief Finds the index of a vehicle's UID in the database.
 * @param uid Pointer to the UID array to find.
 * @return The index of the vehicle if found, -1 otherwise.
 */
int find_vehicle_index(uint8_t *uid) {
    for (int i = 0; i < vehicle_count; i++) {
        if (memcmp(uid, vehicles_inside_db[i], 4) == 0) {
            return i; /* founded */
        }
    }
    return -1; /* not found */
}

/**
 * @brief Adds a vehicle's UID to the database.
 * @param uid Pointer to the UID array to add.
 * @return true if the vehicle was added successfully, false if the parking is full.
 */
bool add_vehicle(uint8_t *uid) {
    if (vehicle_count >= MAX_VEHICLES_INSIDE) {
        return false; /* Parking is full */
    }
    memcpy(vehicles_inside_db[vehicle_count], uid, 4);
    vehicle_count++;
    return true;
}

/**
 * @brief Removes a vehicle from the database by its index.
 * @param index The index of the vehicle to remove.
 */
void remove_vehicle(int index) {
    if (index < 0 || index >= vehicle_count)
        return;

    /* Replace the removed vehicle with the last one in the array */
    for (int i = 0; i < 4; i++) {
        vehicles_inside_db[index][i] = vehicles_inside_db[vehicle_count - 1][i];
    }
    vehicle_count--;
}

/**
 * @brief Configures GPIO pins for LCD and IR sensors.
 */
void GPIO_pinsConfig(void) {
    /* Enable GPIOA and GPIOB clocks */
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_GPIOBEN;

    /* Configure LCD Pins as Output Push-Pull */
    gpio_config_t lcd_pins[] = { { GPIOB, 1, GPIO_DRIVER_MODE_OUTPUT,
            GPIO_DRIVER_OUTPUT_PUSH_PULL, GPIO_DRIVER_SPEED_MEDIUM,
            GPIO_DRIVER_NO_PULL }, { GPIOB, 0, GPIO_DRIVER_MODE_OUTPUT,
            GPIO_DRIVER_OUTPUT_PUSH_PULL, GPIO_DRIVER_SPEED_MEDIUM,
            GPIO_DRIVER_NO_PULL }, { GPIOA, 7, GPIO_DRIVER_MODE_OUTPUT,
            GPIO_DRIVER_OUTPUT_PUSH_PULL, GPIO_DRIVER_SPEED_MEDIUM,
            GPIO_DRIVER_NO_PULL }, { GPIOA, 6, GPIO_DRIVER_MODE_OUTPUT,
            GPIO_DRIVER_OUTPUT_PUSH_PULL, GPIO_DRIVER_SPEED_MEDIUM,
            GPIO_DRIVER_NO_PULL }, { GPIOB, 10, GPIO_DRIVER_MODE_OUTPUT,
            GPIO_DRIVER_OUTPUT_PUSH_PULL, GPIO_DRIVER_SPEED_MEDIUM,
            GPIO_DRIVER_NO_PULL }, { GPIOB, 2, GPIO_DRIVER_MODE_OUTPUT,
            GPIO_DRIVER_OUTPUT_PUSH_PULL, GPIO_DRIVER_SPEED_MEDIUM,
            GPIO_DRIVER_NO_PULL } };
    for (int i = 0; i < sizeof(lcd_pins) / sizeof(gpio_config_t); i++) {
        GPIO_Init(&lcd_pins[i]);
    }

    /* Configure Entry IR sensor pin as input with pull-up */
    gpio_config_t entry_ir_sensor = { ENTRY_IR_PORT, ENTRY_IR_PIN,
            GPIO_DRIVER_MODE_INPUT, GPIO_DRIVER_OUTPUT_PUSH_PULL,
            GPIO_DRIVER_SPEED_MEDIUM, GPIO_DRIVER_PULL_UP };
    GPIO_Init(&entry_ir_sensor);

    /* Configure Exit IR sensor pin as input with pull-up */
    gpio_config_t exit_ir_sensor = { EXIT_IR_PORT, EXIT_IR_PIN,
            GPIO_DRIVER_MODE_INPUT, GPIO_DRIVER_OUTPUT_PUSH_PULL,
            GPIO_DRIVER_SPEED_MEDIUM, GPIO_DRIVER_PULL_UP };
    GPIO_Init(&exit_ir_sensor);

}

/**
 * @brief Checks if the entry IR sensor is blocked.
 * @return true if blocked, false otherwise.
 */
bool Entry_IR_IsBlocked(void) {
    /* The sensor output is active low, so we check for a low level. */
    return !(ENTRY_IR_PORT->IDR & (1 << ENTRY_IR_PIN));
}

/**
 * @brief Checks if the exit IR sensor is blocked.
 * @return true if blocked, false otherwise.
 */
bool Exit_IR_IsBlocked(void) {
    /* The sensor output is active low, so we check for a low level. */
    return !(EXIT_IR_PORT->IDR & (1 << EXIT_IR_PIN));
}

/**
 * @brief Main application entry point.
 * @retval int
 */
int main(void) {

    /* Initialize HAL, System Clock, and custom delay library. */
    HAL_Init();
    SystemClock_Config();
    Delay_Init();

    /* Configure Servo motor on TIM2 Channel 1, GPIOA Pin 0. */
    barrierServo.timer = TIM2;
    barrierServo.channel = 1;
    barrierServo.gpio_port = GPIOA;
    barrierServo.gpio_pin = 0;

    /* Initialize peripherals. */
    GPIO_pinsConfig();
    LCD_Init();
    Servo_Init(&barrierServo);
    Servo_SetAngle(&barrierServo, BARRIER_CLOSED_ANGLE); /* Start with barrier closed. */
    LCD_Clear();
    HC595_Init();
    RGB_Init();
    delay_ms(100); /* Wait for peripherals to stabilize. */
    MFRC522_Init();

    /* Main application loop. */
    while (1) {
        /* Update LCD with the number of free parking slots. */
        LCD_setCursor(0, 0);
        char buffer[32];
        snprintf(buffer, sizeof(buffer), "Free slot: %d",
                (MAX_VEHICLES_INSIDE - vehicle_count));
        LCD_Write(buffer);
        /* Display the number of vehicles inside on the 7-segment display. */
        HC595_DisplayNumber(vehicle_count);

        /* Update RGB LED based on parking availability. */
        if (vehicle_count == MAX_VEHICLES_INSIDE) {
            RGB_SetColor(255, 0, 0); /* Red: Full */
        } else if (vehicle_count == 0) {
            RGB_SetColor(0, 255, 0); /* Green: Empty */
        } else {
            RGB_SetColor(0, 0, 255); /* Blue: Available slots (changed from yellow for clarity) */
        }

        /* Barrier control state machine. */
        switch (currentState) {
        case STATE_CLOSED:
            LCD_setCursor(0, 1);
            LCD_Write("Gate Closed     ");
            /* Check for a present RFID card. */
            if (MFRC522_Request(PICC_REQIDL, current_uid) == MI_OK) {
                /* Perform anti-collision to get the card's UID. */
                if (MFRC522_Anticoll(current_uid) == MI_OK) {
                    /* Check if the card is authorized. */
                    if (is_card_authorized(current_uid)) {
                        int vehicle_idx = find_vehicle_index(current_uid);
                        if (vehicle_idx == -1) { /* Vehicle wants to enter. */
                            if (vehicle_count < MAX_VEHICLES_INSIDE) {
                                current_direction = DIR_ENTRY;
                                currentState = STATE_AUTHORIZED_WAITING_VEHICLE;
                                state_change_timestamp = Get_Ms_Ticks();
                                LCD_setCursor(0, 1);
                                LCD_Write("Gate Opened     ");
                            } else { /* Parking is full. */
                                LCD_setCursor(0, 1);
                                LCD_Write("Parking is full!");
                                delay_ms(1500);
                            }
                        } else { /* Vehicle wants to exit. */
                            current_direction = DIR_EXIT;
                            currentState = STATE_AUTHORIZED_WAITING_VEHICLE;
                            state_change_timestamp = Get_Ms_Ticks();
                            LCD_setCursor(0, 1);
                            LCD_Write("Gate Opened     ");
                        }
                    } else { /* Card not authorized. */
                        LCD_setCursor(0, 1);
                        LCD_Write("Access Denied!  ");
                        delay_ms(1500);
                    }
                }
            }
            break;

        case STATE_AUTHORIZED_WAITING_VEHICLE:
            /* Wait for the vehicle to trigger the corresponding IR sensor. */
            if ((current_direction == DIR_ENTRY && Entry_IR_IsBlocked())
                    || (current_direction == DIR_EXIT && Exit_IR_IsBlocked())) {
                currentState = STATE_OPENING; /* Vehicle detected, start opening. */
            }

            /* Timeout if the vehicle doesn't appear. */
            if (Get_Ms_Ticks() - state_change_timestamp > AUTHORIZED_TIMEOUT) {
                currentState = STATE_CLOSED; /* Cancel the request. */
                current_direction = DIR_NONE;
            }
            break;

        case STATE_OPENING:
            LCD_setCursor(0, 1);
            LCD_Write("Gate Opening... ");
            Servo_SetAngle(&barrierServo, BARRIER_OPEN_ANGLE);
            delay_ms(500); /* Give servo time to move. */
            currentState = STATE_OPEN_WAITING_PASSAGE;
            state_change_timestamp = Get_Ms_Ticks();
            vehicle_is_passing = false; /* Reset the passing flag. */
            break;

        case STATE_OPEN_WAITING_PASSAGE:
            LCD_setCursor(0, 1);
            LCD_Write("Please pass...  ");

            /* Stage 1: Wait for the vehicle to start passing. */
            if (!vehicle_is_passing) {
                if ((current_direction == DIR_ENTRY && Entry_IR_IsBlocked())
                        || (current_direction == DIR_EXIT && Exit_IR_IsBlocked())) {
                    vehicle_is_passing = true; /* Confirm the vehicle has started passing. */
                }
            }

            /* Stage 2: If the vehicle has started, wait for it to pass completely. */
            if (vehicle_is_passing) {
                /* The condition for complete passage is that both sensors are clear. */
                if (!Entry_IR_IsBlocked() && !Exit_IR_IsBlocked()) {
                    /* Update the vehicle database. */
                    if (current_direction == DIR_ENTRY) {
                        add_vehicle(current_uid);
                    } else if (current_direction == DIR_EXIT) {
                        remove_vehicle(find_vehicle_index(current_uid));
                    }
                    currentState = STATE_WAIT_BEFORE_CLOSING;
                    state_change_timestamp = Get_Ms_Ticks();
                }
            }

            /* Timeout check: if the car takes too long, close if safe. */
            if (Get_Ms_Ticks() - state_change_timestamp > PASSAGE_TIMEOUT) {
                if (!Entry_IR_IsBlocked() && !Exit_IR_IsBlocked()) {
                    currentState = STATE_CLOSING;
                }
            }
            break;
        case STATE_WAIT_BEFORE_CLOSING:
            LCD_setCursor(0, 1);
            LCD_Write("Vehicle passed! ");

            /* Wait for the configured delay before starting to close. */
            if (Get_Ms_Ticks() - state_change_timestamp > DELAY_BEFORE_CLOSING) {
                currentState = STATE_CLOSING;
            }
            break;
        case STATE_CLOSING:
            /* Safety check: only close if there are no obstructions. */
            if (!Entry_IR_IsBlocked() && !Exit_IR_IsBlocked()) {
                LCD_setCursor(0, 1);
                LCD_Write("Gate Closing... ");
                Servo_SetAngle(&barrierServo, BARRIER_CLOSED_ANGLE);
                delay_ms(500); /* Give servo time to move. */
                current_direction = DIR_NONE;
                currentState = STATE_CLOSED;
            }
            break;
        }
        delay_ms(50); /* Small delay to prevent busy-waiting. */
    }
}

/**
 * @brief System Clock Configuration
 * @retval None
 */
void SystemClock_Config(void) {
    RCC_OscInitTypeDef RCC_OscInitStruct = { 0 };
    RCC_ClkInitTypeDef RCC_ClkInitStruct = { 0 };

    /** Configure the main internal regulator output voltage
     */
    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);

    /** Initializes the RCC Oscillators according to the specified parameters
     * in the RCC_OscInitTypeDef structure.
     */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLM = 25;
    RCC_OscInitStruct.PLL.PLLN = 168;
    RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
    RCC_OscInitStruct.PLL.PLLQ = 4;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
        Error_Handler();
    }

    /** Initializes the CPU, AHB and APB buses clocks
     */
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
            | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK) {
        Error_Handler();
    }
}

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void) {
    /* User can add his own implementation to report the HAL error return state */
    __disable_irq();
    while (1) {
    }
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  * where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
}
#endif /* USE_FULL_ASSERT */
