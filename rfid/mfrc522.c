/**
 * @file    mfrc522.c
 * @brief   MFRC522 Driver code.
 *
 * @addtogroup MFRC522
 * @{
 */

#include "mfrc522.h"
#include "FreeRTOS.h"
#include "task.h"
#include <string.h>
#include <stdint.h>
#include <pico/printf.h>
#include <hardware/gpio.h>
#include <hardware/spi.h>

/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/

/* MFRC522 Commands */
#define PCD_IDLE                        0x00   //NO action; Cancel the current command
#define PCD_AUTHENT                     0x0E   //Authentication Key
#define PCD_RECEIVE                     0x08   //Receive Data
#define PCD_TRANSMIT                    0x04   //Transmit data
#define PCD_TRANSCEIVE                  0x0C   //Transmit and receive data,
#define PCD_RESETPHASE                  0x0F   //Reset
#define PCD_CALCCRC                     0x03   //CRC Calculate

/* Mifare_One card command word */
#define PICC_REQIDL                     0x26   // find the antenna area does not enter hibernation
#define PICC_REQALL                     0x52   // find all the cards antenna area
#define PICC_ANTICOLL_CL1               0x93   // anti-collision CL1
#define PICC_ANTICOLL_CL2               0x95   // anti-collision CL2
#define PICC_ANTICOLL_CL3               0x97   // anti-collision CL3
#define PICC_SElECTTAG                  0x93   // election card
#define PICC_AUTHENT1A                  0x60   // authentication key A
#define PICC_AUTHENT1B                  0x61   // authentication key B
#define PICC_READ                       0x30   // Read Block
#define PICC_WRITE                      0xA0   // write block
#define PICC_DECREMENT                  0xC0   // debit
#define PICC_INCREMENT                  0xC1   // recharge
#define PICC_RESTORE                    0xC2   // transfer block data to the buffer
#define PICC_TRANSFER                   0xB0   // save the data in the buffer
#define PICC_HALT                       0x50   // Sleep

/* MFRC522 Registers */
//Page 0: Command and Status
#define MifareREG_RESERVED00          0x00
#define MifareREG_COMMAND             0x01
#define MifareREG_COMM_IE_N           0x02
#define MifareREG_DIV1_EN             0x03
#define MifareREG_COMM_IRQ            0x04
#define MifareREG_DIV_IRQ             0x05
#define MifareREG_ERROR               0x06
#define MifareREG_STATUS1             0x07
#define MifareREG_STATUS2             0x08
#define MifareREG_FIFO_DATA           0x09
#define MifareREG_FIFO_LEVEL          0x0A
#define MifareREG_WATER_LEVEL         0x0B
#define MifareREG_CONTROL             0x0C
#define MifareREG_BIT_FRAMING         0x0D
#define MifareREG_COLL                0x0E
#define MifareREG_RESERVED01          0x0F
//Page 1: Command
#define MifareREG_RESERVED10          0x10
#define MifareREG_MODE                0x11
#define MifareREG_TX_MODE             0x12
#define MifareREG_RX_MODE             0x13
#define MifareREG_TX_CONTROL          0x14
#define MifareREG_TX_AUTO             0x15
#define MifareREG_TX_SELL             0x16
#define MifareREG_RX_SELL             0x17
#define MifareREG_RX_THRESHOLD        0x18
#define MifareREG_DEMOD               0x19
#define MifareREG_RESERVED11          0x1A
#define MifareREG_RESERVED12          0x1B
#define MifareREG_MIFARE              0x1C
#define MifareREG_RESERVED13          0x1D
#define MifareREG_RESERVED14          0x1E
#define MifareREG_SERIALSPEED         0x1F
//Page 2: CFG
#define MifareREG_RESERVED20          0x20
#define MifareREG_CRC_RESULT_M        0x21
#define MifareREG_CRC_RESULT_L        0x22
#define MifareREG_RESERVED21          0x23
#define MifareREG_MOD_WIDTH           0x24
#define MifareREG_RESERVED22          0x25
#define MifareREG_RF_CFG              0x26
#define MifareREG_GS_N                0x27
#define MifareREG_CWGS_PREG           0x28
#define MifareREG__MODGS_PREG         0x29
#define MifareREG_T_MODE              0x2A
#define MifareREG_T_PRESCALER         0x2B
#define MifareREG_T_RELOAD_H          0x2C
#define MifareREG_T_RELOAD_L          0x2D
#define MifareREG_T_COUNTER_VALUE_H   0x2E
#define MifareREG_T_COUNTER_VALUE_L   0x2F
//Page 3:TestRegister
#define MifareREG_RESERVED30          0x30
#define MifareREG_TEST_SEL1           0x31
#define MifareREG_TEST_SEL2           0x32
#define MifareREG_TEST_PIN_EN         0x33
#define MifareREG_TEST_PIN_VALUE      0x34
#define MifareREG_TEST_BUS            0x35
#define MifareREG_AUTO_TEST           0x36
#define MifareREG_VERSION             0x37
#define MifareREG_ANALOG_TEST         0x38
#define MifareREG_TEST_ADC1           0x39
#define MifareREG_TEST_ADC2           0x3A
#define MifareREG_TEST_ADC0           0x3B
#define MifareREG_RESERVED31          0x3C
#define MifareREG_RESERVED32          0x3D
#define MifareREG_RESERVED33          0x3E
#define MifareREG_RESERVED34          0x3F
//Dummy byte
#define MifareDUMMY                   0x00

#define MifareMAX_LEN                 16

/*===========================================================================*/
/* Driver exported variables.                                                */
/*===========================================================================*/

/*===========================================================================*/
/* OWN TYPES                                        */
/*===========================================================================*/


// A struct used for passing the UID of a PICC.
typedef struct {
    uint8_t 		size;			// Number of bytes in the UID. 4, 7 or 10.
    uint8_t 		uidByte[10];
    uint8_t 		sak;			// The SAK (Select acknowledge) byte returned from the PICC after successful selection.
} Uid;


/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/
static void MFRC522Reset(MFRC522Driver *mfrc522p) {
    MFRC522WriteRegister(mfrc522p, MifareREG_COMMAND, PCD_RESETPHASE);
    uint8_t count = 0;
    // TODO: needed?
    while (MFRC522ReadRegister(mfrc522p, MifareREG_COMMAND) & (1 << 4) && count < 3) {
        count++;
        vTaskDelay(50);
    };
}

static void MFRC522SetBitMask(MFRC522Driver *mfrc522p, uint8_t reg, uint8_t mask) {
    MFRC522WriteRegister(mfrc522p, reg, MFRC522ReadRegister(mfrc522p, reg) | mask);
}

static void MFRC522ClearBitMask(MFRC522Driver *mfrc522p, uint8_t reg, uint8_t mask) {
    MFRC522WriteRegister(mfrc522p, reg, MFRC522ReadRegister(mfrc522p, reg) & (~mask));
}

static void MFRC522AntennaOn(MFRC522Driver *mfrc522p) {
    uint8_t temp;

    temp = MFRC522ReadRegister(mfrc522p, MifareREG_TX_CONTROL);
    if (!(temp & 0x03)) {
        MFRC522SetBitMask(mfrc522p, MifareREG_TX_CONTROL, 0x03);
    }
}

static void MFRC522AntennaOff(MFRC522Driver *mfrc522p) {
    MFRC522ClearBitMask(mfrc522p, MifareREG_TX_CONTROL, 0x03);
}

static MIFARE_Status_t
MifareToPICC(MFRC522Driver *mfrc522p, uint8_t command, uint8_t *sendData, uint8_t sendLen, uint8_t *backData,
             uint8_t backDataLen, uint16_t *backLen) {

    MIFARE_Status_t status = MIFARE_ERR;
    uint8_t irqEn = 0x00;
    uint8_t waitIRq = 0x00;
    uint8_t lastBits;
    uint8_t n;
    uint16_t i;

//    printf("MifareToPICC\n command: %x\n, back", command);

    switch (command) {
        case PCD_AUTHENT: {
            irqEn = 0x12;
            waitIRq = 0x10;
            break;
        }
        case PCD_TRANSCEIVE: {
            irqEn = 0x77;
            waitIRq = 0x30;
            break;
        }
        default:
            break;
    }

    MFRC522WriteRegister(mfrc522p, MifareREG_COMM_IE_N, irqEn | 0x80);
    MFRC522ClearBitMask(mfrc522p, MifareREG_COMM_IRQ, 0x80);
    MFRC522SetBitMask(mfrc522p, MifareREG_FIFO_LEVEL, 0x80);

    MFRC522WriteRegister(mfrc522p, MifareREG_COMMAND, PCD_IDLE);

    //Writing data to the FIFO
    for (i = 0; i < sendLen; i++) {
        MFRC522WriteRegister(mfrc522p, MifareREG_FIFO_DATA, sendData[i]);
    }

    //Execute the command
    MFRC522WriteRegister(mfrc522p, MifareREG_COMMAND, command);
    if (command == PCD_TRANSCEIVE) {
        MFRC522SetBitMask(mfrc522p, MifareREG_BIT_FRAMING, 0x80);      //StartSend=1,transmission of data starts
    }

    //Waiting to receive data to complete
    i = 2000;   //i according to the clock frequency adjustment, the operator M1 card maximum waiting time 25ms???
    do {
        //CommIrqReg[7..0]
        //Set1 TxIRq RxIRq IdleIRq HiAlerIRq LoAlertIRq ErrIRq TimerIRq
        n = MFRC522ReadRegister(mfrc522p, MifareREG_COMM_IRQ);
//        printf("n0: %x\n", n);

        i--;
    } while ((i != 0) && !(n & 0x01) && !(n & waitIRq));

    MFRC522ClearBitMask(mfrc522p, MifareREG_BIT_FRAMING, 0x80);            //StartSend=0

//    printf("n: %x\n", n);

    if (i != 0) {
        if (!(MFRC522ReadRegister(mfrc522p, MifareREG_ERROR) & 0x1B)) {
            status = MIFARE_OK;
            if (n & irqEn & 0x01) {
//                printf("MIFARE_NOTAGERR\n");
                status = MIFARE_NOTAGERR;
            }

            if (command == PCD_TRANSCEIVE) {
                n = MFRC522ReadRegister(mfrc522p, MifareREG_FIFO_LEVEL);
//                printf("n2: %x\n", n);
                lastBits = MFRC522ReadRegister(mfrc522p, MifareREG_CONTROL) & 0x07;
//                printf("n3: %x\n", lastBits);
                if (lastBits) {
                    *backLen = (n - 1) * 8 + lastBits;
                } else {
                    *backLen = n * 8;
                }

                if (n == 0) {
                    n = 1;
                }
                if (n > MifareMAX_LEN) {
                    n = MifareMAX_LEN;
                }

                if (n <= backDataLen) {
//                    printf("reading data\n");
                    //Reading the received data in FIFO
//                    for (i = 0; i < n; i++) {
//                        backData[i] = MFRC522ReadRegister(mfrc522p, MifareREG_FIFO_DATA);
//                    }
                    MFRC522ReadRegisterBuffer(mfrc522p, MifareREG_FIFO_DATA, n, backData);
                } else {
                    status = MIFARE_ERR;
                }

            }
        } else {
            status = MIFARE_ERR;
        }
    }

//    printf("MifareToPICC status: %x\n", status);
    return status;
}

uint8_t MFRC522AntiCollisionLoop(MFRC522Driver *mfrc522p, uint8_t selcommand, uint8_t *serNum) {
    MIFARE_Status_t status;
    uint8_t serNumCheck = 0;
    uint16_t unLen;
    uint8_t cascadeLevel[5];
    uint8_t command[2];
    uint8_t SAK = 0xff;


    command[0] = selcommand; // SEL
    command[1] = 0x20; // NVB
//    MFRC522WriteRegister(mfrc522p ,MifareREG_COLL, 0x80);
    status = MifareToPICC(mfrc522p, PCD_TRANSCEIVE, command, 2, cascadeLevel, sizeof(cascadeLevel), &unLen);

    if (status == MIFARE_OK) {
        //calc bcc
        int8_t i = 0;
        for (i = 0; i < 4; i++) {
            serNumCheck ^= cascadeLevel[i];
        }

        if (serNumCheck != cascadeLevel[i]) {
            status = MIFARE_ERR;
            return SAK;
        }

        SAK = MifareSelectTag(mfrc522p, selcommand, cascadeLevel);
        if ((SAK & 0x04) == 0) {
            for (i = 0; i < 4; i++) {
                serNum[i] = cascadeLevel[i];
            }
        } else {
            serNum[0] = cascadeLevel[1];
            serNum[1] = cascadeLevel[2];
            serNum[2] = cascadeLevel[3];
        }
    }

    return SAK;
}

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   MFRC522 Driver initialization.
 * @note    This function is implicitly invoked by @p halInit(), there is
 *          no need to explicitly initialize the driver.
 *
 * @init
 */
void MFRC522Init(void) {

}

/**
 * @brief   Initializes the standard part of a @p MFRC522Driver structure.
 *
 * @param[out] mfrc522p     pointer to the @p MFRC522Driver object
 *
 * @init
 */
void MFRC522ObjectInit(MFRC522Driver *mfrc522p) {

    mfrc522p->state = MFRC522_STOP;
    mfrc522p->config = NULL;
}

/**
 * @brief   Configures and activates the MFRC522 peripheral.
 *
 * @param[in] MFRC522p      pointer to the @p MFRC522Driver object
 * @param[in] config    pointer to the @p MFRC522Config object
 *
 * @api
 */

void MFRC522Start(MFRC522Driver *mfrc522p, const MFRC522Config *config) {

//	osalDbgCheck((mfrc522p != NULL) && (config != NULL));

//	osalSysLock();
//	osalDbgAssert((mfrc522p->state == MFRC522_STOP) || (mfrc522p->state == MFRC522_READY),
//			"invalid state");
    mfrc522p->config = config;

//	osalSysUnlock();

    MFRC522Reset(mfrc522p);

    MFRC522WriteRegister(mfrc522p, MifareREG_T_MODE, 0x8D);
    MFRC522WriteRegister(mfrc522p, MifareREG_T_PRESCALER, 0x3E);
    MFRC522WriteRegister(mfrc522p, MifareREG_T_RELOAD_L, 30);
    MFRC522WriteRegister(mfrc522p, MifareREG_T_RELOAD_H, 0);

    /* 48dB gain */
    MFRC522WriteRegister(mfrc522p, MifareREG_RF_CFG, 0x70);

    MFRC522WriteRegister(mfrc522p, MifareREG_TX_AUTO, 0x40);
    MFRC522WriteRegister(mfrc522p, MifareREG_MODE, 0x3D);

    MFRC522AntennaOn(mfrc522p);

//	osalSysLock();
    mfrc522p->state = MFRC522_ACTIVE;
//	osalSysUnlock();
}

/**
 * @brief   Deactivates the MFRC522 peripheral.
 *
 * @param[in] MFRC522p      pointer to the @p MFRC522Driver object
 *
 * @api
 */
void MFRC522Stop(MFRC522Driver *mfrc522p) {

    osalDbgCheck(mfrc522p != NULL);

    osalSysLock();
    osalDbgAssert(mfrc522p->state == MFRC522_ACTIVE,
                  "invalid state");
    osalSysUnlock();

    MFRC522AntennaOff(mfrc522p);
    MFRC522Reset(mfrc522p);

    osalSysLock();
    mfrc522p->state = MFRC522_STOP;
    osalSysUnlock();
}

/**
 * @brief
 *
 * @param[in] MFRC522p      pointer to the @p MFRC522Driver object
 *
 * @api
 */
MIFARE_Status_t MifareRequest(MFRC522Driver *mfrc522p, uint8_t reqMode, uint8_t *tagType, uint8_t tagTypeLen) {
    MIFARE_Status_t status;
    uint16_t backBits;          //The received data bits
    uint8_t request[1];

    MFRC522WriteRegister(mfrc522p, MifareREG_COMMAND, PCD_IDLE); // Stop any command
    MFRC522WriteRegister(mfrc522p, MifareREG_COMM_IRQ, 0x7F);   // Clear all seven interrupt request bits
    MFRC522SetBitMask(mfrc522p, MifareREG_FIFO_LEVEL, 0x80);        // FlushBuffer = 1, FIFO initialization


    MFRC522WriteRegister(mfrc522p, MifareREG_BIT_FRAMING, 0x07);        //TxLastBists = BitFramingReg[2..0] ???

    request[0] = reqMode;
    status = MifareToPICC(mfrc522p, PCD_TRANSCEIVE, request, 1, tagType, tagTypeLen, &backBits);

//    printf("status: %x\n", status);
//    printf("backBits: %x\n", backBits);
    if ((status != MIFARE_OK) || (backBits != 0x10)) {
        status = MIFARE_ERR;
    }

    return status;
}


/**
 * @brief
 *
 * @param[in] MFRC522p
 *
 * @api
 */


MIFARE_Status_t MifareAnticoll(MFRC522Driver *mfrc522p, struct MifareUID *id) {

    uint8_t SAK = 0;

    MFRC522WriteRegister(mfrc522p, MifareREG_BIT_FRAMING, 0x00);        //TxLastBists = BitFramingReg[2..0]

    // cascade level 1
    SAK = MFRC522AntiCollisionLoop(mfrc522p, PICC_ANTICOLL_CL1, id->bytes);
    if ((SAK & 0x04) == 0) {
        id->size = 4;
        return MIFARE_OK;
    }

    SAK = MFRC522AntiCollisionLoop(mfrc522p, PICC_ANTICOLL_CL2, id->bytes + 3);
    if ((SAK & 0x04) == 0) {
        id->size = 7;
        return MIFARE_OK;
    }

    SAK = MFRC522AntiCollisionLoop(mfrc522p, PICC_ANTICOLL_CL3, id->bytes + 6);
    if ((SAK & 0x04) != 0) {
        return MIFARE_ERR;
    }

    id->size = 10;
    return MIFARE_OK;
}


/**
 * @brief
 *
 * MY OWN SELECT METHOD
 *
 * @param[in] MFRC522p
 *
 * @api
 */
MIFARE_Status_t PICC_SELECT(MFRC522Driver *mfrc522p,Uid *uid) {
    bool uidComplete;
    bool selectDone;
    bool useCascadeTag = false;
    uint8_t cascadeLevel = 1;
    MIFARE_Status_t result;
    uint8_t count;
    uint8_t checkBit;
    uint8_t index;
    uint8_t uidIndex;					// The first index in uid->uidByte[] that is used in the current Cascade Level.
    int8_t currentLevelKnownBits;		// The number of known UID bits in the current Cascade Level.
    uint8_t buffer[9];					// The SELECT/ANTICOLLISION commands uses a 7 byte standard frame + 2 bytes CRC_A
    uint8_t bufferUsed;				// The number of bytes used in the buffer, ie the number of bytes to transfer to the FIFO.
    uint8_t rxAlign;					// Used in BitFramingReg. Defines the bit position for the first bit received.
    uint8_t txLastBits;				// Used in BitFramingReg. The number of valid bits in the last transmitted byte.
    uint8_t *responseBuffer;
    uint8_t responseLength;

    // Description of buffer structure:
    //		Byte 0: SEL 				Indicates the Cascade Level: PICC_CMD_SEL_CL1, PICC_CMD_SEL_CL2 or PICC_CMD_SEL_CL3
    //		Byte 1: NVB					Number of Valid Bits (in complete command, not just the UID): High nibble: complete bytes, Low nibble: Extra bits.
    //		Byte 2: UID-data or CT		See explanation below. CT means Cascade Tag.
    //		Byte 3: UID-data
    //		Byte 4: UID-data
    //		Byte 5: UID-data
    //		Byte 6: BCC					Block Check Character - XOR of bytes 2-5
    //		Byte 7: CRC_A
    //		Byte 8: CRC_A
    // The BCC and CRC_A are only transmitted if we know all the UID bits of the current Cascade Level.
    //
    // Description of bytes 2-5: (Section 6.5.4 of the ISO/IEC 14443-3 draft: UID contents and cascade levels)
    //		UID size	Cascade level	Byte2	Byte3	Byte4	Byte5
    //		========	=============	=====	=====	=====	=====
    //		 4 bytes		1			uid0	uid1	uid2	uid3
    //		 7 bytes		1			CT		uid0	uid1	uid2
    //						2			uid3	uid4	uid5	uid6
    //		10 bytes		1			CT		uid0	uid1	uid2
    //						2			CT		uid3	uid4	uid5
    //						3			uid6	uid7	uid8	uid9

    // Sanity checks
//    if (validBits > 80) {
//        return STATUS_INVALID;
//    }

    // Prepare MFRC522
    MFRC522WriteRegister(mfrc522p ,MifareREG_COLL, 0x80);		// ValuesAfterColl=1 => Bits received after collision are cleared.

    // Repeat Cascade Level loop until we have a complete UID.
    uidComplete = false;
    while (!uidComplete) {
        // Set the Cascade Level in the SEL byte, find out if we need to use the Cascade Tag in byte 2.
        switch (cascadeLevel) {
            case 1:
                buffer[0] = PICC_ANTICOLL_CL1;
                uidIndex = 0;
//                useCascadeTag = validBits && uid->size > 4;	// When we know that the UID has more than 4 bytes
                break;

            case 2:
                buffer[0] = PICC_ANTICOLL_CL2;
                uidIndex = 3;
//                useCascadeTag = validBits && uid->size > 7;	// When we know that the UID has more than 7 bytes
                break;

            case 3:
                buffer[0] = PICC_ANTICOLL_CL3;
                uidIndex = 6;
                useCascadeTag = false;						// Never used in CL3.
                break;

            default:
                return MIFARE_ERR;
                break;
        }

        // How many UID bits are known in this Cascade Level?
        currentLevelKnownBits = 0;
//        currentLevelKnownBits = validBits - (8 * uidIndex);
//        if (currentLevelKnownBits < 0) {
//            currentLevelKnownBits = 0;
//        }
        // Copy the known bits from uid->uidByte[] to buffer[]
//        index = 2; // destination index in buffer[]
//        if (useCascadeTag) {
//            buffer[index++] = PICC_CMD_CT;
//        }
//        uint8_t bytesToCopy = currentLevelKnownBits / 8 + (currentLevelKnownBits % 8 ? 1 : 0); // The number of bytes needed to represent the known bits for this level.
//        if (bytesToCopy) {
//            uint8_t maxBytes = useCascadeTag ? 3 : 4; // Max 4 bytes in each Cascade Level. Only 3 left if we use the Cascade Tag
//            if (bytesToCopy > maxBytes) {
//                bytesToCopy = maxBytes;
//            }
//            for (count = 0; count < bytesToCopy; count++) {
//                buffer[index++] = uid->uidByte[uidIndex + count];
//            }
//        }
//         Now that the data has been copied we need to include the 8 bits in CT in currentLevelKnownBits
//        if (useCascadeTag) {
//            currentLevelKnownBits += 8;
//        }

        // Repeat anti collision loop until we can transmit all UID bits + BCC and receive a SAK - max 32 iterations.
        selectDone = false;
        while (!selectDone) {
            // Find out how many bits and bytes to send and receive.
            if (currentLevelKnownBits >= 32) { // All UID bits in this Cascade Level are known. This is a SELECT.
                //Serial.print(F("SELECT: currentLevelKnownBits=")); Serial.println(currentLevelKnownBits, DEC);
                buffer[1] = 0x70; // NVB - Number of Valid Bits: Seven whole bytes
                // Calculate BCC - Block Check Character
                buffer[6] = buffer[2] ^ buffer[3] ^ buffer[4] ^ buffer[5];
                // Calculate CRC_A
//                result = PCD_CalculateCRC(buffer, 7, &buffer[7]);
//                if (result != STATUS_OK) {
//                    return result;
//                }
                txLastBits		= 0; // 0 => All 8 bits are valid.
                bufferUsed		= 9;
                // Store response in the last 3 bytes of buffer (BCC and CRC_A - not needed after tx)
                responseBuffer	= &buffer[6];
                responseLength	= 3;
            }
            else { // This is an ANTICOLLISION.
                //Serial.print(F("ANTICOLLISION: currentLevelKnownBits=")); Serial.println(currentLevelKnownBits, DEC);
                txLastBits		= currentLevelKnownBits % 8;
                count			= currentLevelKnownBits / 8;	// Number of whole bytes in the UID part.
                index			= 2 + count;					// Number of whole bytes: SEL + NVB + UIDs
                buffer[1]		= (index << 4) + txLastBits;	// NVB - Number of Valid Bits
                bufferUsed		= index + (txLastBits ? 1 : 0);
                // Store response in the unused part of buffer
                responseBuffer	= &buffer[index];
                responseLength	= sizeof(buffer) - index;
            }

            // Set bit adjustments
            rxAlign = txLastBits;											// Having a separate variable is overkill. But it makes the next line easier to read.
            MFRC522WriteRegister(mfrc522p, MifareREG_BIT_FRAMING, (rxAlign << 4) + txLastBits);	// RxAlign = BitFramingReg[6..4]. TxLastBits = BitFramingReg[2..0]

            uint16_t backLen = 0;
            // Transmit the buffer and receive the response.
            result = MifareToPICC(mfrc522p,PCD_TRANSCEIVE, buffer, bufferUsed, responseBuffer, responseLength, &backLen);


            uint8_t errorRegValue = MFRC522ReadRegister(mfrc522p, MifareREG_ERROR);
            bool collision = errorRegValue & 0x08;
            // Tell about collisions
//            if (errorRegValue & 0x08) {		// CollErr
//                return STATUS_COLLISION;
//            }

//            result = PCD_TransceiveData(buffer, bufferUsed, responseBuffer, &responseLength, &txLastBits, rxAlign);
            if (collision) { // More than one PICC in the field => collision.
                uint8_t valueOfCollReg = MFRC522ReadRegister(mfrc522p ,MifareREG_COLL); // CollReg[7..0] bits are: ValuesAfterColl reserved CollPosNotValid CollPos[4:0]
                if (valueOfCollReg & 0x20) { // CollPosNotValid
                    printf("CollPosNotValid\n");
                    return MIFARE_ERR; // Without a valid collision position we cannot continue
                }
                uint8_t collisionPos = valueOfCollReg & 0x1F; // Values 0-31, 0 means bit 32.
                if (collisionPos == 0) {
                    collisionPos = 32;
                }
                if (collisionPos <= currentLevelKnownBits) { // No progress - should not happen
                    printf("No progress\n");
                    return MIFARE_ERR;
                }
                // Choose the PICC with the bit set.
                currentLevelKnownBits	= collisionPos;
                count			= currentLevelKnownBits % 8; // The bit to modify
                checkBit		= (currentLevelKnownBits - 1) % 8;
                index			= 1 + (currentLevelKnownBits / 8) + (count ? 1 : 0); // First byte is index 0.
                buffer[index]	|= (1 << checkBit);
            }
            else if (result != MIFARE_OK) {
                printf("result != MIFARE_OK\n");
                return result;
            }
            else { // STATUS_OK
                if (currentLevelKnownBits >= 32) { // This was a SELECT.
                    selectDone = true; // No more anticollision
                    // We continue below outside the while.
                }
                else { // This was an ANTICOLLISION.
                    // We now have all 32 bits of the UID in this Cascade Level
                    currentLevelKnownBits = 32;
                    // Run loop again to do the SELECT.
                }
            }
        } // End of while (!selectDone)

        // We do not check the CBB - it was constructed by us above.

        // Copy the found UID bytes from buffer[] to uid->uidByte[]
//        index			= (buffer[2] == PICC_CMD_CT) ? 3 : 2; // source index in buffer[]
//        bytesToCopy		= (buffer[2] == PICC_CMD_CT) ? 3 : 4;
//        for (count = 0; count < bytesToCopy; count++) {
//            uid->uidByte[uidIndex + count] = buffer[index++];
//        }

        // Check response SAK (Select Acknowledge)
//        if (responseLength != 3 || txLastBits != 0) { // SAK must be exactly 24 bits (1 byte + CRC_A).
//            return STATUS_ERROR;
//        }
        // Verify CRC_A - do our own calculation and store the control in buffer[2..3] - those bytes are not needed anymore.
//        result = PCD_CalculateCRC(responseBuffer, 1, &buffer[2]);
//        if (result != STATUS_OK) {
//            return result;
//        }
//        if ((buffer[2] != responseBuffer[1]) || (buffer[3] != responseBuffer[2])) {
//            return STATUS_CRC_WRONG;
//        }
//        if (responseBuffer[0] & 0x04) { // Cascade bit set - UID not complete yes
//            cascadeLevel++;
//        }
//        else {
//            uidComplete = true;
//            uid->sak = responseBuffer[0];
//        }
    } // End of while (!uidComplete)

    // Set correct uid->size
    uid->size = 3 * cascadeLevel + 1;

    return MIFARE_OK;
} // End PICC_Select()

/**
 * @brief
 *
 * @param[in] MFRC522p
 *
 * @api
 */
void MifareCalculateCRC(MFRC522Driver *mfrc522p, uint8_t *pIndata, uint8_t len, uint8_t *pOutData) {
    MFRC522ClearBitMask(mfrc522p, MifareREG_DIV_IRQ, 0x04);         //CRCIrq = 0
    MFRC522SetBitMask(mfrc522p, MifareREG_FIFO_LEVEL, 0x80);            //Clear the FIFO pointer

    //Writing data to the FIFO
    uint8_t i;
    for (i = 0; i < len; i++) {
        MFRC522WriteRegister(mfrc522p, MifareREG_FIFO_DATA, *(pIndata + i));
    }
    MFRC522WriteRegister(mfrc522p, MifareREG_COMMAND, PCD_CALCCRC);

    //Wait CRC calculation is complete
    uint8_t n;
    i = 0xFF;
    do {
        n = MFRC522ReadRegister(mfrc522p, MifareREG_DIV_IRQ);
        i--;
    } while ((i != 0) && !(n & 0x04));          //CRCIrq = 1

    //Read CRC calculation result
    pOutData[0] = MFRC522ReadRegister(mfrc522p, MifareREG_CRC_RESULT_L);
    pOutData[1] = MFRC522ReadRegister(mfrc522p, MifareREG_CRC_RESULT_M);
}

/**
 * @brief
 *
 * @param[in] MFRC522p
 *
 * @api
 */
uint8_t MifareSelectTag(MFRC522Driver *mfrc522p, uint8_t command, uint8_t *serNum) {
    uint8_t i;
    MIFARE_Status_t status;
    uint8_t SAK;
    uint16_t recvBits;
    uint8_t buffer[9];

    buffer[0] = command;
    buffer[1] = 0x70;
    for (i = 0; i < 5; i++) {
        buffer[i + 2] = *(serNum + i);
    }
    MifareCalculateCRC(mfrc522p, buffer, 7, &buffer[7]);     //??
    status = MifareToPICC(mfrc522p, PCD_TRANSCEIVE, buffer, 9, buffer, 9, &recvBits);

    if ((status == MIFARE_OK) && (recvBits == 0x18)) {
        SAK = buffer[0];
    } else {
        SAK = 0xff;
    }

    return SAK;
}

/**
 * @brief
 *
 * @param[in] MFRC522p
 *
 * @api
 */
MIFARE_Status_t
MifareAuth(MFRC522Driver *mfrc522p, uint8_t authMode, uint8_t BlockAddr, uint8_t *Sectorkey, uint8_t *serNum) {
    MIFARE_Status_t status;
    uint16_t recvBits;
    uint8_t i;
    uint8_t buff[12];

    //Verify the command block address + sector + password + card serial number
    buff[0] = authMode;
    buff[1] = BlockAddr;
    for (i = 0; i < 6; i++) {
        buff[i + 2] = *(Sectorkey + i);
    }
    for (i = 0; i < 4; i++) {
        buff[i + 8] = *(serNum + i);
    }
    status = MifareToPICC(mfrc522p, PCD_AUTHENT, buff, 12, buff, 12, &recvBits);

    if ((status != MIFARE_OK) || (!(MFRC522ReadRegister(mfrc522p, MifareREG_STATUS2) & 0x08))) {
        status = MIFARE_ERR;
    }

    return status;
}

/**
 * @brief
 *
 * @param[in] MFRC522p
 *
 * @api
 */
MIFARE_Status_t MifareRead(MFRC522Driver *mfrc522p, uint8_t blockAddr, uint8_t *recvData) {
    MIFARE_Status_t status;
    uint16_t unLen;

    recvData[0] = PICC_READ;
    recvData[1] = blockAddr;
    MifareCalculateCRC(mfrc522p, recvData, 2, &recvData[2]);
    status = MifareToPICC(mfrc522p, PCD_TRANSCEIVE, recvData, 4, recvData, 4, &unLen);

    if ((status != MIFARE_OK) || (unLen != 0x90)) {
        status = MIFARE_ERR;
    }

    return status;
}

/**
 * @brief
 *
 * @param[in] MFRC522p
 *
 * @api
 */
MIFARE_Status_t MifareWrite(MFRC522Driver *mfrc522p, uint8_t blockAddr, uint8_t *writeData) {
    MIFARE_Status_t status;
    uint16_t recvBits;
    uint8_t i;
    uint8_t buff[18];

    buff[0] = PICC_WRITE;
    buff[1] = blockAddr;
    MifareCalculateCRC(mfrc522p, buff, 2, &buff[2]);
    status = MifareToPICC(mfrc522p, PCD_TRANSCEIVE, buff, 4, buff, 18, &recvBits);

    if ((status != MIFARE_OK) || (recvBits != 4) || ((buff[0] & 0x0F) != 0x0A)) {
        status = MIFARE_ERR;
    }

    if (status == MIFARE_OK) {
        //Data to the FIFO write 16Byte
        for (i = 0; i < 16; i++) {
            buff[i] = *(writeData + i);
        }
        MifareCalculateCRC(mfrc522p, buff, 16, &buff[16]);
        status = MifareToPICC(mfrc522p, PCD_TRANSCEIVE, buff, 18, buff, 18, &recvBits);

        if ((status != MIFARE_OK) || (recvBits != 4) || ((buff[0] & 0x0F) != 0x0A)) {
            status = MIFARE_ERR;
        }
    }

    return status;
}

/**
 * @brief
 *
 * @param[in] MFRC522p
 *
 * @api
 */
MIFARE_Status_t MifareCheck(MFRC522Driver *mfrc522p, struct MifareUID *id) {
    MIFARE_Status_t status;
    uint8_t tagType[2];

    status = MifareRequest(mfrc522p, PICC_REQALL, tagType, 2);
    if (status == MIFARE_OK) {
        printf("Card detected: %x %x\n", tagType[0], tagType[1]);
        //Card detected
        //Anti-collision, return card serial number 4 bytes
        status = MifareAnticoll(mfrc522p, id);
//        Uid uid;
//
//        status = PICC_SELECT(mfrc522p, &uid);
    }
    MifareHalt(mfrc522p);          //Command card into hibernation

    return status;
}

MIFARE_Status_t MifareHalt(MFRC522Driver *mfrc522p) {
    uint16_t unLen;
    uint8_t buff[4];

    buff[0] = PICC_HALT;
    buff[1] = 0;
    MifareCalculateCRC(mfrc522p, buff, 2, &buff[2]);

    return MifareToPICC(mfrc522p, PCD_TRANSCEIVE, buff, 4, buff, 4, &unLen);
}

/** @} */



void MFRC522Selftest(MFRC522Driver *mfrc522p) {
    MFRC522Reset(mfrc522p);

    uint8_t reg = MifareREG_COMMAND;
    uint8_t data[25] = {0x00};
    // flush the FIFO buffer
    MFRC522WriteRegister(mfrc522p, MifareREG_FIFO_LEVEL, 0x80);

//    MFRC522WriteRegister(mfrc522p, MifareREG_COMMAND, 0x01);

    // write 25 empty bytes to fifo
//    gpio_put(CS, 0);
    for (int i = 0; i < 25; i++) {
        MFRC522WriteRegister(mfrc522p, MifareREG_FIFO_DATA, 0x00);
    }
//    spi_write_blocking(SPI_PORT, &reg, 1);
//    while(spi_is_busy(SPI_PORT)) {}
//    gpio_put(CS, 1);

//    gpio_put(CS, 0);
//    spi_write_blocking(SPI_PORT, tx_data, 25);
//    while(spi_is_busy(SPI_PORT)) {}
//    gpio_put(CS, 1);

    // transfer to internal buffer
    MFRC522WriteRegister(mfrc522p, MifareREG_COMMAND, 0x01);


//    MFRC522Reset(mfrc522p);

    // perform self test
    MFRC522WriteRegister(mfrc522p, MifareREG_AUTO_TEST, 0x09);

    // wrtie 00 to Fifo
    MFRC522WriteRegister(mfrc522p, MifareREG_FIFO_DATA, 0x00);

    //  Start the self test with the CalcCRC command.
    MFRC522WriteRegister(mfrc522p, MifareREG_COMMAND, PCD_CALCCRC);



    // wait until calculation is done
    uint8_t level = 0;
    do {
        level = MFRC522ReadRegister(mfrc522p, MifareREG_FIFO_LEVEL);
    } while (level < 64);

    // Stop calculating CRC for new content in the FIFO.
    MFRC522WriteRegister(mfrc522p, MifareREG_COMMAND, 0x00);

    // read the tx_data from the fifo
    uint8_t n;
    printf("Self test:\n");
//        n = MFRC522ReadRegister(mfrc522p, MifareREG_FIFO_DATA);
//        printf("%x \n", n);
    uint8_t tx_data[2];
    uint8_t rx_data[65];
    tx_data[0] = ((MifareREG_FIFO_DATA << 1) & 0x7E) | 0x80;
    tx_data[1] = 0xff;
    gpio_put(CS, 0);
//    spi_write_blocking(SPI_PORT, tx_data, 2);
    spi_read_blocking(SPI_PORT, tx_data[0], rx_data, 65);
    while (spi_is_busy(SPI_PORT)) {}
    gpio_put(CS, 1);

    for (int i = 1; i < 65; ++i) {
        printf("%x \n", rx_data[i]);
    }
}

//void MFRC522GetData(MFRC522Driver *mfrc522p, uint8_t *uid[], uint8_t *uid_len ) {
//    uint8_t tx_data[2];
//    uint8_t rx_data[65];
//    tx_data[0] = ((MifareREG_FIFO_DATA << 1) & 0x7E) | 0x80;
//    tx_data[1] = 0xff;
//
//
//    // Repeat anti collision loop until we can transmit all UID bits + BCC and receive a SAK - max 32 iterations.
//    bool selectDone = false;
//    while (!selectDone) {
//        // Find out how many bits and bytes to send and receive.
//        if (currentLevelKnownBits >= 32) { // All UID bits in this Cascade Level are known. This is a SELECT.
//            //Serial.print(F("SELECT: currentLevelKnownBits=")); Serial.println(currentLevelKnownBits, DEC);
//            buffer[1] = 0x70; // NVB - Number of Valid Bits: Seven whole bytes
//            // Calculate BCC - Block Check Character
//            buffer[6] = buffer[2] ^ buffer[3] ^ buffer[4] ^ buffer[5];
//            // Calculate CRC_A
//            result = PCD_CalculateCRC(buffer, 7, &buffer[7]);
//            if (result != STATUS_OK) {
//                return result;
//            }
//            txLastBits		= 0; // 0 => All 8 bits are valid.
//            bufferUsed		= 9;
//            // Store response in the last 3 bytes of buffer (BCC and CRC_A - not needed after tx)
//            responseBuffer	= &buffer[6];
//            responseLength	= 3;
//        }
//        else { // This is an ANTICOLLISION.
//            //Serial.print(F("ANTICOLLISION: currentLevelKnownBits=")); Serial.println(currentLevelKnownBits, DEC);
//            txLastBits		= currentLevelKnownBits % 8;
//            count			= currentLevelKnownBits / 8;	// Number of whole bytes in the UID part.
//            index			= 2 + count;					// Number of whole bytes: SEL + NVB + UIDs
//            buffer[1]		= (index << 4) + txLastBits;	// NVB - Number of Valid Bits
//            bufferUsed		= index + (txLastBits ? 1 : 0);
//            // Store response in the unused part of buffer
//            responseBuffer	= &buffer[index];
//            responseLength	= sizeof(buffer) - index;
//        }
//
//        // Set bit adjustments
//        rxAlign = txLastBits;											// Having a separate variable is overkill. But it makes the next line easier to read.
//        PCD_WriteRegister(BitFramingReg, (rxAlign << 4) + txLastBits);	// RxAlign = BitFramingReg[6..4]. TxLastBits = BitFramingReg[2..0]
//
//        // Transmit the buffer and receive the response.
//        result = PCD_TransceiveData(buffer, bufferUsed, responseBuffer, &responseLength, &txLastBits, rxAlign);
//        if (result == STATUS_COLLISION) { // More than one PICC in the field => collision.
//            uint8_t valueOfCollReg = PCD_ReadRegister(CollReg); // CollReg[7..0] bits are: ValuesAfterColl reserved CollPosNotValid CollPos[4:0]
//            if (valueOfCollReg & 0x20) { // CollPosNotValid
//                return STATUS_COLLISION; // Without a valid collision position we cannot continue
//            }
//            uint8_t collisionPos = valueOfCollReg & 0x1F; // Values 0-31, 0 means bit 32.
//            if (collisionPos == 0) {
//                collisionPos = 32;
//            }
//            if (collisionPos <= currentLevelKnownBits) { // No progress - should not happen
//                return STATUS_INTERNAL_ERROR;
//            }
//            // Choose the PICC with the bit set.
//            currentLevelKnownBits	= collisionPos;
//            count			= currentLevelKnownBits % 8; // The bit to modify
//            checkBit		= (currentLevelKnownBits - 1) % 8;
//            index			= 1 + (currentLevelKnownBits / 8) + (count ? 1 : 0); // First byte is index 0.
//            buffer[index]	|= (1 << checkBit);
//        }
//        else if (result != STATUS_OK) {
//            return result;
//        }
//        else { // STATUS_OK
//            if (currentLevelKnownBits >= 32) { // This was a SELECT.
//                selectDone = true; // No more anticollision
//                // We continue below outside the while.
//            }
//            else { // This was an ANTICOLLISION.
//                // We now have all 32 bits of the UID in this Cascade Level
//                currentLevelKnownBits = 32;
//                // Run loop again to do the SELECT.
//            }
//        }
//    } // End of while (!selectDone)
//}









