#include <rc522.h>

/*---------- PRIVATE FUNCTION PROTOTYPES ----------*/
static void MFRC522_GPIO_Init(void);
static void MFRC522_SPI_Init(void);
static uint8_t RC522_SPI_Transfer(uint8_t data);
static void Write_MFRC522(uint8_t addr, uint8_t val);
static uint8_t Read_MFRC522(uint8_t addr);
static void SetBitMask(uint8_t reg, uint8_t mask);
static void ClearBitMask(uint8_t reg, uint8_t mask);
static void AntennaOn(void);
static void AntennaOff(void);
static void CalulateCRC(uint8_t *pIndata, uint8_t len, uint8_t *pOutData);
static uint8_t MFRC522_ToCard(uint8_t command, uint8_t *sendData, uint8_t sendLen, uint8_t *backData, uint16_t *backLen);


/*---------- GPIO CONTROL MACROS ----------*/
/* Macro to pull the Chip Select (CS) pin low. */
#define CS_LOW()      (MFRC522_CS_PORT->BSRR = (1U << (MFRC522_CS_PIN + 16)))
/* Macro to pull the Chip Select (CS) pin high. */
#define CS_HIGH()     (MFRC522_CS_PORT->BSRR = (1U << MFRC522_CS_PIN))
/* Macro to pull the Reset (RST) pin low. */
#define RST_LOW()     (MFRC522_RST_PORT->BSRR = (1U << (MFRC522_RST_PIN + 16)))
/* Macro to pull the Reset (RST) pin high. */
#define RST_HIGH()    (MFRC522_RST_PORT->BSRR = (1U << MFRC522_RST_PIN))


/**
 * @brief Initializes GPIO pins for MFRC522 SPI communication.
 */
static void MFRC522_GPIO_Init(void) {
    /* 1. Enable GPIO clocks for ports A and B. */
    MFRC522_GPIO_RCC_REG |= (MFRC522_GPIOA_RCC_EN | MFRC522_GPIOB_RCC_EN);

    /* 2. Configure SPI pins (SCK, MISO, MOSI) as Alternate Function AF5 for SPI2. */

    /* SCK Pin (PB13) configuration */
    MFRC522_SCK_PORT->MODER &= ~(0x03 << (MFRC522_SCK_PIN * 2)); /* Clear mode bits */
    MFRC522_SCK_PORT->MODER |= (0x02 << (MFRC522_SCK_PIN * 2));  /* Set to Alternate Function mode */
    MFRC522_SCK_PORT->AFR[1] |= (5U << ((MFRC522_SCK_PIN - 8) * 4)); /* Select AF5 */

    /* MISO Pin (PB14) configuration */
    MFRC522_MISO_PORT->MODER &= ~(0x03 << (MFRC522_MISO_PIN * 2));
    MFRC522_MISO_PORT->MODER |= (0x02 << (MFRC522_MISO_PIN * 2));
    MFRC522_MISO_PORT->AFR[1] |= (5U << ((MFRC522_MISO_PIN - 8) * 4));

    /* MOSI Pin (PB15) configuration */
    MFRC522_MOSI_PORT->MODER &= ~(0x03 << (MFRC522_MOSI_PIN * 2));
    MFRC522_MOSI_PORT->MODER |= (0x02 << (MFRC522_MOSI_PIN * 2));
    MFRC522_MOSI_PORT->AFR[1] |= (5U << ((MFRC522_MOSI_PIN - 8) * 4));

    /* Set high speed for all SPI pins */
    MFRC522_SCK_PORT->OSPEEDR |= (0x03 << (MFRC522_SCK_PIN * 2));
    MFRC522_MISO_PORT->OSPEEDR |= (0x03 << (MFRC522_MISO_PIN * 2));
    MFRC522_MOSI_PORT->OSPEEDR |= (0x03 << (MFRC522_MOSI_PIN * 2));

    /* 3. Configure CS pin (PB12) as General Purpose Output. */
    MFRC522_CS_PORT->MODER &= ~(0x03 << (MFRC522_CS_PIN * 2));
    MFRC522_CS_PORT->MODER |= (0x01 << (MFRC522_CS_PIN * 2));
    MFRC522_CS_PORT->OSPEEDR |= (0x03 << (MFRC522_CS_PIN * 2));

    /* 4. Configure RST pin (PB9) as General Purpose Output. */
    MFRC522_RST_PORT->MODER &= ~(0x03 << (MFRC522_RST_PIN * 2));
    MFRC522_RST_PORT->MODER |= (0x01 << (MFRC522_RST_PIN * 2));
    MFRC522_RST_PORT->OSPEEDR |= (0x03 << (MFRC522_RST_PIN * 2));
}

/**
 * @brief Initializes the SPI2 peripheral for MFRC522 communication.
 */
static void MFRC522_SPI_Init(void) {
    /* 1. Enable SPI2 clock. */
    MFRC522_SPI_RCC_REG |= MFRC522_SPI_RCC_EN;

    /* 2. Configure SPI_CR1 register. */
    MFRC522_SPI_INSTANCE->CR1 = 0; /* Clear all settings */
    MFRC522_SPI_INSTANCE->CR1 |= SPI_CR1_MSTR; /* Set to Master mode */
    MFRC522_SPI_INSTANCE->CR1 |= (0x05 << SPI_CR1_BR_Pos); /* Set Baud rate to APB1_CLK/64. Note: APB1 is 42MHz, so SPI clk ~0.65MHz */
    MFRC522_SPI_INSTANCE->CR1 |= SPI_CR1_SSM | SPI_CR1_SSI; /* Software slave management enabled */

    /* 3. Configure SPI_CR2 register (default is fine). */
    MFRC522_SPI_INSTANCE->CR2 = 0;

    /* 4. Enable SPI peripheral. */
    MFRC522_SPI_INSTANCE->CR1 |= SPI_CR1_SPE;
}


/**
 * @brief Sends and receives one byte over SPI.
 * @param data The byte to send.
 * @return The byte received.
 */
static uint8_t RC522_SPI_Transfer(uint8_t data) {
    /* Wait until transmit buffer is empty (TXE flag is set). */
    while (!(MFRC522_SPI_INSTANCE->SR & SPI_SR_TXE));
    MFRC522_SPI_INSTANCE->DR = data;

    /* Wait until receive buffer is not empty (RXNE flag is set). */
    while (!(MFRC522_SPI_INSTANCE->SR & SPI_SR_RXNE));

    /* Return the received data from the data register. */
    return (uint8_t)MFRC522_SPI_INSTANCE->DR;
}


/**
 * @brief Writes a byte to a specific MFRC522 register.
 * @param addr The register address.
 * @param val The value to write.
 */
static void Write_MFRC522(uint8_t addr, uint8_t val) {
    CS_LOW();
    /* Address format for write: 0XXXXXX0 (MSB=0 for write, LSB=0). */
    RC522_SPI_Transfer((addr << 1) & 0x7E);
    RC522_SPI_Transfer(val);
    CS_HIGH();
}

/**
 * @brief Reads a byte from a specific MFRC522 register.
 * @param addr The register address.
 * @return The value read from the register.
 */
static uint8_t Read_MFRC522(uint8_t addr) {
    uint8_t val;
    CS_LOW();
    /* Address format for read: 1XXXXXX0 (MSB=1 for read, LSB=0). */
    RC522_SPI_Transfer(((addr << 1) & 0x7E) | 0x80);
    val = RC522_SPI_Transfer(0x00); /* Send a dummy byte to receive data. */
    CS_HIGH();
    return val;
}

/**
 * @brief Sets a bit mask in a specific MFRC522 register.
 * @param reg The register address.
 * @param mask The bit mask to set.
 */
static void SetBitMask(uint8_t reg, uint8_t mask) {
    uint8_t tmp;
    tmp = Read_MFRC522(reg);
    Write_MFRC522(reg, tmp | mask);
}

/**
 * @brief Clears a bit mask in a specific MFRC522 register.
 * @param reg The register address.
 * @param mask The bit mask to clear.
 */
static void ClearBitMask(uint8_t reg, uint8_t mask) {
    uint8_t tmp;
    tmp = Read_MFRC522(reg);
    Write_MFRC522(reg, tmp & (~mask));
}

/**
 * @brief Turns the antenna on.
 */
static void AntennaOn(void) {
    uint8_t temp = Read_MFRC522(TxControlReg);
    if (!(temp & 0x03)) {
        SetBitMask(TxControlReg, 0x03);
    }
}

/**
 * @brief Turns the antenna off.
 */
static void AntennaOff(void) {
    ClearBitMask(TxControlReg, 0x03);
}

/**
 * @brief Performs a soft reset of the MFRC522 module.
 */
void MFRC522_Reset(void) {
    Write_MFRC522(CommandReg, PCD_RESETPHASE);
}

/*------------- PUBLIC FUNCTIONS -------------*/

/**
 * @brief Initializes the MFRC522 module.
 */
void MFRC522_Init(void) {
    MFRC522_GPIO_Init();
    MFRC522_SPI_Init();

    CS_HIGH();
    RST_HIGH();

    MFRC522_Reset();

    /* Configure timer settings. */
    Write_MFRC522(TModeReg, 0x8D);      /* TAuto=1, TPrescaler=... */
    Write_MFRC522(TPrescalerReg, 0x3E); /* TPrescaler=... */
    Write_MFRC522(TReloadRegL, 30);
    Write_MFRC522(TReloadRegH, 0);

    Write_MFRC522(TxAutoReg, 0x40);    /* Set 100% ASK modulation. */
    Write_MFRC522(ModeReg, 0x3D);     /* Set CRC initial value to 0x6363. */

    AntennaOn();
}

/**
 * @brief Communicates with a PICC (card).
 * @param command The command to execute (e.g., PCD_TRANSCEIVE).
 * @param sendData Pointer to the data to send.
 * @param sendLen Length of the data to send.
 * @param backData Pointer to the buffer to store received data.
 * @param backLen Pointer to store the length of received data in bits.
 * @return Status of the communication (MI_OK, MI_ERR, MI_NOTAGERR).
 */
static uint8_t MFRC522_ToCard(uint8_t command, uint8_t *sendData, uint8_t sendLen, uint8_t *backData, uint16_t *backLen) {
    uint8_t status = MI_ERR;
    uint8_t irqEn = 0x00;
    uint8_t waitIRq = 0x00;
    uint8_t lastBits;
    uint8_t n;
    uint16_t i;

    /* Set interrupt enable and wait flags based on the command. */
    switch (command) {
        case PCD_AUTHENT:
            irqEn = 0x12; /* Enable error and idle interrupts */
            waitIRq = 0x10; /* Wait for idle interrupt */
            break;
        case PCD_TRANSCEIVE:
            irqEn = 0x77; /* Enable all interrupts */
            waitIRq = 0x30; /* Wait for Rx or Idle interrupt */
            break;
        default:
            break;
    }

    /* Configure communication registers. */
    Write_MFRC522(CommIEnReg, irqEn | 0x80); /* Enable IRQ pin */
    ClearBitMask(CommIrqReg, 0x80);        /* Clear all interrupt request bits */
    SetBitMask(FIFOLevelReg, 0x80);        /* Flush the FIFO buffer */
    Write_MFRC522(CommandReg, PCD_IDLE);   /* Cancel current command */

    /* Write data to the FIFO buffer. */
    for (i = 0; i < sendLen; i++) {
        Write_MFRC522(FIFODataReg, sendData[i]);
    }

    /* Execute the command. */
    Write_MFRC522(CommandReg, command);
    if (command == PCD_TRANSCEIVE) {
        SetBitMask(BitFramingReg, 0x80); /* Start the transmission */
    }

    /* Wait for the command to complete or timeout. */
    i = 2000; /* Set timeout */
    do {
        n = Read_MFRC522(CommIrqReg);
        i--;
    } while ((i != 0) && !(n & 0x01) && !(n & waitIRq));

    ClearBitMask(BitFramingReg, 0x80); /* Stop the transmission */

    /* Check the result. */
    if (i != 0) {
        if (!(Read_MFRC522(ErrorReg) & 0x1B)) { /* Check for errors */
            status = MI_OK;
            if (n & irqEn & 0x01) {
                status = MI_NOTAGERR;
            }

            if (command == PCD_TRANSCEIVE) {
                n = Read_MFRC522(FIFOLevelReg);
                lastBits = Read_MFRC522(ControlReg) & 0x07;
                if (lastBits) {
                    *backLen = (n - 1) * 8 + lastBits;
                } else {
                    *backLen = n * 8;
                }

                if (n == 0) n = 1;
                if (n > MAX_LEN) n = MAX_LEN;

                /* Read the received data from FIFO. */
                for (i = 0; i < n; i++) {
                    backData[i] = Read_MFRC522(FIFODataReg);
                }
            }
        } else {
            status = MI_ERR;
        }
    }
    return status;
}

/**
 * @brief Finds cards in the antenna field and returns their type.
 * @param reqMode The request mode (e.g., PICC_REQIDL for idle cards).
 * @param TagType Pointer to a buffer to store the card type (ATQA).
 * @return Status of the operation (MI_OK or MI_ERR).
 */
uint8_t MFRC522_Request(uint8_t reqMode, uint8_t *TagType) {
    uint8_t status;
    uint16_t backBits; /* The length of the received data in bits */
    Write_MFRC522(BitFramingReg, 0x07); /* TxLastBists = 7 */
    TagType[0] = reqMode;
    status = MFRC522_ToCard(PCD_TRANSCEIVE, TagType, 1, TagType, &backBits);
    if ((status != MI_OK) || (backBits != 0x10)) { /* ATQA is 2 bytes (16 bits) */
        status = MI_ERR;
    }
    return status;
}

/**
 * @brief Anti-collision procedure to get the UID of a card.
 * @param serNum Pointer to a buffer to store the 4-byte serial number (UID) and 1-byte BCC.
 * @return Status of the operation (MI_OK or MI_ERR).
 */
uint8_t MFRC522_Anticoll(uint8_t *serNum) {
    uint8_t status;
    uint8_t i;
    uint8_t serNumCheck = 0;
    uint16_t unLen;

    Write_MFRC522(BitFramingReg, 0x00); /* TxLastBists = 0 */
    serNum[0] = PICC_ANTICOLL;
    serNum[1] = 0x20;
    status = MFRC522_ToCard(PCD_TRANSCEIVE, serNum, 2, serNum, &unLen);

    if (status == MI_OK) {
        /* Check the BCC (Block Check Character). */
        for (i = 0; i < 4; i++) {
            serNumCheck ^= serNum[i];
        }
        if (serNumCheck != serNum[i]) {
            status = MI_ERR;
        }
    }
    return status;
}

/**
 * @brief Calculates the CRC of data using the MFRC522's internal hardware.
 * @param pIndata Pointer to the input data.
 * @param len Length of the input data.
 * @param pOutData Pointer to a 2-byte array to store the CRC result.
 */
static void CalulateCRC(uint8_t *pIndata, uint8_t len, uint8_t *pOutData) {
    uint8_t i, n;
    ClearBitMask(DivIrqReg, 0x04); /* Clear CRCIRq interrupt request bit */
    SetBitMask(FIFOLevelReg, 0x80); /* Flush FIFO buffer */

    /* Write data to FIFO. */
    for (i = 0; i < len; i++) {
        Write_MFRC522(FIFODataReg, *(pIndata + i));
    }
    Write_MFRC522(CommandReg, PCD_CALCCRC); /* Start CRC calculation */

    /* Wait for CRC calculation to complete. */
    i = 0xFF;
    do {
        n = Read_MFRC522(DivIrqReg);
        i--;
    } while ((i != 0) && !(n & 0x04));

    /* Read CRC result. */
    pOutData[0] = Read_MFRC522(CRCResultRegL);
    pOutData[1] = Read_MFRC522(CRCResultRegH);
}

/**
 * @brief Selects a specific card using its serial number.
 * @param serNum Pointer to the card's 5-byte serial number (UID+BCC).
 * @return The SAK (Select Acknowledge) byte if successful, 0 otherwise.
 */
uint8_t MFRC522_SelectTag(uint8_t *serNum) {
    uint8_t i;
    uint8_t status;
    uint8_t size;
    uint16_t recvBits;
    uint8_t buffer[9];

    buffer[0] = PICC_SElECTTAG;
    buffer[1] = 0x70; /* Cascade Level 1, 7 bytes total (CMD + NVB + UID + BCC + CRC) */
    for (i = 0; i < 5; i++) {
        buffer[i + 2] = *(serNum + i);
    }
    CalulateCRC(buffer, 7, &buffer[7]);
    status = MFRC522_ToCard(PCD_TRANSCEIVE, buffer, 9, buffer, &recvBits);

    if ((status == MI_OK) && (recvBits == 0x18)) { /* SAK is 1 byte, CRC is 2 bytes = 24 bits */
        size = buffer[0];
    } else {
        size = 0;
    }
    return size;
}

/**
 * @brief Authenticates a memory block of a MIFARE Classic card.
 * @param authMode The authentication mode (PICC_AUTHENT1A or PICC_AUTHENT1B).
 * @param BlockAddr The address of the block to authenticate.
 * @param Sectorkey Pointer to the 6-byte sector key.
 * @param serNum Pointer to the 4-byte card serial number.
 * @return Status of the authentication (MI_OK or MI_ERR).
 */
uint8_t MFRC522_Auth(uint8_t authMode, uint8_t BlockAddr, uint8_t *Sectorkey, uint8_t *serNum) {
    uint8_t status;
    uint16_t recvBits;
    uint8_t i;
    uint8_t buff[12];

    /* Format the authentication request command. */
    buff[0] = authMode;
    buff[1] = BlockAddr;
    for (i = 0; i < 6; i++) {
        buff[i + 2] = *(Sectorkey + i);
    }
    for (i = 0; i < 4; i++) {
        buff[i + 8] = *(serNum + i);
    }
    status = MFRC522_ToCard(PCD_AUTHENT, buff, 12, buff, &recvBits);

    if ((status != MI_OK) || (!(Read_MFRC522(Status2Reg) & 0x08))) {
        status = MI_ERR;
    }
    return status;
}

/**
 * @brief Reads 16 bytes from a block on a MIFARE Classic card.
 * @param blockAddr The address of the block to read.
 * @param recvData Pointer to a buffer to store the 16 bytes of data.
 * @return Status of the read operation (MI_OK or MI_ERR).
 */
uint8_t MFRC522_Read(uint8_t blockAddr, uint8_t *recvData) {
    uint8_t status;
    uint16_t unLen;
    recvData[0] = PICC_READ;
    recvData[1] = blockAddr;
    CalulateCRC(recvData, 2, &recvData[2]);
    status = MFRC522_ToCard(PCD_TRANSCEIVE, recvData, 4, recvData, &unLen);

    if ((status != MI_OK) || (unLen != 0x90)) { /* Received data should be 18 bytes (144 bits) = 16 data + 2 CRC */
        status = MI_ERR;
    }
    return status;
}

/**
 * @brief Writes 16 bytes to a block on a MIFARE Classic card.
 * @param blockAddr The address of the block to write.
 * @param writeData Pointer to the 16 bytes of data to write.
 * @return Status of the write operation (MI_OK or MI_ERR).
 */
uint8_t MFRC522_Write(uint8_t blockAddr, uint8_t *writeData) {
    uint8_t status;
    uint16_t recvBits;
    uint8_t i;
    uint8_t buff[18];

    /* First part of write operation: send write command and block address. */
    buff[0] = PICC_WRITE;
    buff[1] = blockAddr;
    CalulateCRC(buff, 2, &buff[2]);
    status = MFRC522_ToCard(PCD_TRANSCEIVE, buff, 4, buff, &recvBits);

    if ((status != MI_OK) || (recvBits != 4) || ((buff[0] & 0x0F) != 0x0A)) {
        status = MI_ERR;
    }

    /* Second part: if first part was successful, send the data. */
    if (status == MI_OK) {
        for (i = 0; i < 16; i++) {
            buff[i] = *(writeData + i);
        }
        CalulateCRC(buff, 16, &buff[16]);
        status = MFRC522_ToCard(PCD_TRANSCEIVE, buff, 18, buff, &recvBits);
        if ((status != MI_OK) || (recvBits != 4) || ((buff[0] & 0x0F) != 0x0A)) {
            status = MI_ERR;
        }
    }
    return status;
}

/**
 * @brief Puts the selected card into a HALT state.
 */
void MFRC522_Halt(void) {
    uint16_t unLen;
    uint8_t buff[4];
    buff[0] = PICC_HALT;
    buff[1] = 0;
    CalulateCRC(buff, 2, &buff[2]);
    MFRC522_ToCard(PCD_TRANSCEIVE, buff, 4, buff, &unLen);
}
