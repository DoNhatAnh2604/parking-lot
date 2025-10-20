#ifndef INC_RC522_H_
#define INC_RC522_H_

#include "stm32f4xx.h"
#include <stdint.h>

/*------------- PIN DEFINITIONS -------------*/
/* Change these definitions to match your circuit schematic. */
#define MFRC522_SPI_INSTANCE        SPI2
#define MFRC522_SPI_RCC_REG         RCC->APB1ENR
#define MFRC522_SPI_RCC_EN          RCC_APB1ENR_SPI2EN

/* SPI pins on Port B */
#define MFRC522_SCK_PORT            GPIOB
#define MFRC522_SCK_PIN             13
#define MFRC522_MISO_PORT           GPIOB
#define MFRC522_MISO_PIN            14
#define MFRC522_MOSI_PORT           GPIOB
#define MFRC522_MOSI_PIN            15

/* Chip Select (CS or SDA) and Reset (RST) pins */
#define MFRC522_CS_PORT             GPIOB
#define MFRC522_CS_PIN              12
#define MFRC522_RST_PORT            GPIOB
#define MFRC522_RST_PIN             9

/* GPIO Clock Enable */
#define MFRC522_GPIO_RCC_REG        RCC->AHB1ENR
#define MFRC522_GPIOA_RCC_EN        0 /* GPIOA not used in this configuration */
#define MFRC522_GPIOB_RCC_EN        RCC_AHB1ENR_GPIOBEN

/*------------- CONSTANTS -------------*/
/* Maximum length of the array for data transfer */
#define MAX_LEN 16

/* MFRC522 commands (datasheet chapter 10) */
#define PCD_IDLE              0x00
#define PCD_AUTHENT           0x0E
#define PCD_RECEIVE           0x08
#define PCD_TRANSMIT          0x04
#define PCD_TRANSCEIVE        0x0C
#define PCD_RESETPHASE        0x0F
#define PCD_CALCCRC           0x03

/* PICC commands (PICC = Proximity Integrated Circuit Card) */
#define PICC_REQIDL           0x26
#define PICC_REQALL           0x52
#define PICC_ANTICOLL         0x93
#define PICC_SElECTTAG        0x93
#define PICC_AUTHENT1A        0x60
#define PICC_AUTHENT1B        0x61
#define PICC_READ             0x30
#define PICC_WRITE            0xA0
#define PICC_DECREMENT        0xC0
#define PICC_INCREMENT        0xC1
#define PICC_RESTORE          0xC2
#define PICC_TRANSFER         0xB0
#define PICC_HALT             0x50

/* Status codes */
#define MI_OK                 0
#define MI_NOTAGERR           1
#define MI_ERR                2

/* MFRC522 registers (datasheet chapter 9) */
/* Page 0: Command and Status */
#define CommandReg            0x01
#define CommIEnReg            0x02
#define DivlEnReg             0x03
#define CommIrqReg            0x04
#define DivIrqReg             0x05
#define ErrorReg              0x06
#define Status1Reg            0x07
#define Status2Reg            0x08
#define FIFODataReg           0x09
#define FIFOLevelReg          0x0A
#define WaterLevelReg         0x0B
#define ControlReg            0x0C
#define BitFramingReg         0x0D
#define CollReg               0x0E
/* Page 1: Command */
#define ModeReg               0x11
#define TxModeReg             0x12
#define RxModeReg             0x13
#define TxControlReg          0x14
#define TxAutoReg             0x15
#define TxSelReg              0x16
#define RxSelReg              0x17
#define RxThresholdReg        0x18
#define DemodReg              0x19
#define MifareReg             0x1C
#define SerialSpeedReg        0x1F
/* Page 2: Configuration */
#define CRCResultRegH         0x21
#define CRCResultRegL         0x22
#define ModWidthReg           0x24
#define RFCfgReg              0x26
#define GsNReg                0x27
#define CWGsPReg              0x28
#define ModGsPReg             0x29
#define TModeReg              0x2A
#define TPrescalerReg         0x2B
#define TReloadRegH           0x2C
#define TReloadRegL           0x2D
#define TCounterValueRegH     0x2E
#define TCounterValueRegL     0x2F
/* Page 3: Test Registers */
#define TestSel1Reg           0x31
#define TestSel2Reg           0x32
#define TestPinEnReg          0x33
#define TestPinValueReg       0x34
#define TestBusReg            0x35
#define AutoTestReg           0x36
#define VersionReg            0x37
#define AnalogTestReg         0x38
#define TestDAC1Reg           0x39
#define TestDAC2Reg           0x3A
#define TestADCReg            0x3B

/*------------- FUNCTION PROTOTYPES -------------*/
void MFRC522_Init(void);
uint8_t MFRC522_Request(uint8_t reqMode, uint8_t *TagType);
uint8_t MFRC522_Anticoll(uint8_t *serNum);
uint8_t MFRC522_SelectTag(uint8_t *serNum);
uint8_t MFRC522_Auth(uint8_t authMode, uint8_t BlockAddr, uint8_t *Sectorkey, uint8_t *serNum);
uint8_t MFRC522_Read(uint8_t blockAddr, uint8_t *recvData);
uint8_t MFRC522_Write(uint8_t blockAddr, uint8_t *writeData);
void MFRC522_Halt(void);
void MFRC522_Reset(void);

#endif /* INC_RC522_H_ */
