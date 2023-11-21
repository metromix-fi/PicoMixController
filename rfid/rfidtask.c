//
// Created by jesse on 10.11.23.
//


#include <pico/printf.h>
#include "FreeRTOS.h"
#include "task.h"
#include "rfidtask.h"

#include "string.h"
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "mfrc522.h"
#include "rfid-spi.h"
#include "queue.h"
#include "utils/GlobalState.h"

#define MISO 16
#define CS 17
#define SCKL 18
#define MOSI 19
//#define RESET 10 // TODO: change (conflict with rotary encoder)


#define SPI_PORT spi0

#define DEBUG  False
#define OK  0
#define NOTAGERR  1
#define ERR  2

#define REQIDL  0x26
#define REQALL  0x52
#define AUTHENT1A  0x60
#define AUTHENT1B  0x61

#define PICC_ANTICOLL1  0x93
#define PICC_ANTICOLL2  0x95
#define PICC_ANTICOLL3  0x97


//mifare
#define BitFramingReg   0x0D


// Private functions


// Public functions

void setup_rfid_gpios(void) {
    // init gpio pins for SPI communication
    gpio_set_function(MISO, GPIO_FUNC_SPI);
    gpio_set_function(SCKL, GPIO_FUNC_SPI);
    gpio_set_function(MOSI, GPIO_FUNC_SPI);

    // init chip select pin
    gpio_init(CS);
    gpio_set_dir(CS, GPIO_OUT);
    gpio_put(CS, 1); // Set high for inactive

    // Initialize the reset pin, if used
//    gpio_init(RESET);
//    gpio_set_dir(RESET, GPIO_OUT);
//    gpio_put(RESET, 1);  // Set high assuming active-low reset

     spi_init(SPI_PORT, 9600);
}





// mfrc522.c
void MFRC522WriteRegister(MFRC522Driver *mfrc522p, uint8_t addr, uint8_t val) {
    uint8_t data[2];
    data[0] = (addr << 1) & 0x7E;
    data[1] = val;
    gpio_put(CS, 0);
    spi_write_blocking(SPI_PORT, data, 2);
    while(spi_is_busy(SPI_PORT)) {}
    gpio_put(CS, 1);
}

uint8_t MFRC522ReadRegister(MFRC522Driver *mfrc522p, uint8_t addr) {
    uint8_t data[2];
    uint8_t rx_data[2];
    data[0] = ((addr << 1) & 0x7E) | 0x80;
    data[1] = 0xff;
    gpio_put(CS, 0);
//    spi_write_blocking(SPI_PORT, data, 2);
    spi_read_blocking(SPI_PORT, data[0], rx_data, 2);
    while(spi_is_busy(SPI_PORT)) {}
    gpio_put(CS, 1);

//    spi_
    return rx_data[1];
}

// Returns length of received data
void MFRC522ReadRegisterBuffer(MFRC522Driver *mfrc522p, uint8_t addr, uint8_t count, uint8_t *values) {
    uint8_t data[2];
    uint8_t rx_data[count + 1];
    data[0] = ((addr << 1) & 0x7E) | 0x80;
    data[1] = 0xff;
    gpio_put(CS, 0);
//    spi_write_blocking(SPI_PORT, data, 2);
    spi_read_blocking(SPI_PORT, data[0], rx_data, count + 1);
    while(spi_is_busy(SPI_PORT)) {}
    gpio_put(CS, 1);

    for (uint8_t i = 0; i < count; i++) {
        values[i] = rx_data[i + 1];
    }

//    return rx_data[1];
}





// OWN CODE
//static void write_rfid(uint8_t *data, uint8_t len) {
//    gpio_put(CS, 0);
//    spi_write_blocking(SPI_PORT, data, len);
//    gpio_put(CS, 1);
//}
//
//static void read_rfid(uint8_t *data, uint8_t len) {
//    gpio_put(CS, 0);
//    spi_read_blocking(SPI_PORT, 0, data, len);
//    gpio_put(CS, 1);
//}
//
//uint8_t	mfrc522_request(uint8_t req_mode, uint8_t * tag_type)
//{
//    uint8_t  status;
//    uint32_t backBits;//The received data bits
//
//    write_rfid(BitFramingReg, 0x07);//TxLastBists = BitFramingReg[2..0]	???
//
//    tag_type[0] = req_mode;
//    status = mfrc522_to_card(Transceive_CMD, tag_type, 1, tag_type, &backBits);
//
//    if ((status != CARD_FOUND) || (backBits != 0x10))
//    {
//        status = ERROR;
//    }
//
//    return status;
//}
//
//
//
//bool get_card_uuid(mfrc522_t *reader, uint8_t *uuid, uint8_t *uuid_len) {
//    // Buffer for commands and responses
//    uint8_t buffer[10];
//    // Start with a standard REQUEST command to detect card presence
//    buffer[0] = MFRC522_CMD_REQUEST;
//    mfrc522_transceive(reader, buffer, 1, buffer, sizeof(buffer));
//
//    // Check if a card responded to the REQUEST
//    if (buffer[0] == SOME_STATUS_CODE_INDICATING_SUCCESS) {
//        // Now perform the anticollision sequence to get the card's UUID
//        buffer[0] = MFRC522_CMD_ANTICOLL;
//        buffer[1] = 0x20; // Parameter for the anticollision command
//        mfrc522_transceive(reader, buffer, 2, buffer, sizeof(buffer));
//
//        // If anticollision was successful, the card's UUID will be in the buffer
//        // Copy the UUID to the output variable and set the length
//        memcpy(uuid, buffer, 5);  // The UUID is typically 4 or 7 bytes, here we assume 4 for simplicity
//        *uuid_len = 5;
//        return true;
//    } else {
//        // No card responded, or an error occurred
//        return false;
//    }
//}
//
//uint8_t* select_tag_sn(uint8_t *uuid, uint8_t *len) {
//
//    uint8_t buffer[9];
//    uint8_t buffer_size = sizeof(buffer);
//    // Clear previous SN
//    memset(sn, 0, *sn_len);
//
//    // Send the ANTICOLLISION command with the appropriate cascade level to the RFID reader
//    // This example assumes the use of PICC_ANTICOLL1, which is a common first step.
//    buffer[0] = PICC_ANTICOLL1;
//    buffer[1] = 0x20; // A typical parameter for the anticollision command
//
//    write_rfid(buffer, 2); // 'write_rfid' is a function you'd need to define for SPI communication
//
//    // The 'read_rfid' function needs to handle the timing and reading process
//    read_rfid(buffer, buffer_size);
//
//    // Check for errors and validity of the response
//    // This would involve checking the status return byte and possibly the CRC
//
//    // If valid, copy the serial number to the provided buffer
//    if (buffer[0] == OK /* OK is a status code, need to be checked */) {
//        memcpy(sn, buffer + 1, buffer_size - 1); // Copy the serial number
//        *sn_len = buffer_size - 1;
//        return true;
//    } else {
//        return false;
//    }
//
//    return uuid;
//}




// Public functions

_Noreturn void rfid_task(void *pvParameters) {

    vTaskDelay(1000);


    MFRC522Driver mfrc522p;
    MFRC522ObjectInit(&mfrc522p);
    MFRC522Start(&mfrc522p, NULL);

    struct MifareUID card_id;
    char tag_type[2] = {0x00};

//    MIFARE_Status_t status = MifareRequest(&mfrc522p, PICC_REQIDL, &tag_type, 2);

    while (1) {
//        MFRC522Selftest(&mfrc522p);
//        vTaskDelay(1000);

        MIFARE_Status_t status = MifareCheck(&mfrc522p, &card_id);
//        MIFARE_Status_t status = MifareRequest(&mfrc522p, PICC_REQIDL, (uint8_t *) &tag_type, 2);
        if (status == MI_OK) {
            printf("\n\nrfidtask.c: MI_OK\n");
            printf("Card detected: %x, %x\n", tag_type[0], tag_type[1]);
            printf("Card: %x, %x\n\n", card_id.size, card_id.bytes);

        } else if (status == MI_NOTAGERR) {
            printf("\n\nrfidtask.c: MI_NOTAGERR");
            printf("No card detected\n\n");
        } else { // MI_ERR
            printf("\n\nrfidtask.c: Error: %d\n", status);
            printf("Tag type: %x, %x\n", tag_type[0], tag_type[1]);
            printf("Card: %x, %x\n\n", card_id.size, card_id.bytes);
        }
        xQueueSendToBack(globalStruct.rfidQueue, &tag_type, portMAX_DELAY);

        vTaskDelay(2000);
    }
}
