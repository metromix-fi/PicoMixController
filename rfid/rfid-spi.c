//
// Created by jesse on 10.11.23.
//

#include "rfid-spi.h"

#include "FreeRTOS.h"
#include "task.h"

#include "string.h"
#include "pico/stdlib.h"
#include "hardware/spi.h"


#define MISO 16
#define CS 17
#define SCKL 18
#define MOSI 19
#define RESET 10 // TODO: change


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

static void setup_rfid_gpios(void) {
    // init gpio pins for SPI communication
    gpio_set_function(MISO, GPIO_FUNC_SPI);
    gpio_set_function(SCKL, GPIO_FUNC_SPI);
    gpio_set_function(MOSI, GPIO_FUNC_SPI);

    // init chip select pin
    gpio_init(CS);
    gpio_set_dir(CS, GPIO_OUT);
    gpio_put(CS, 1); // Set high for inactive

    // Initialize the reset pin, if used
    gpio_init(RESET);
    gpio_set_dir(RESET, GPIO_OUT);
    gpio_put(RESET, 1);  // Set high assuming active-low reset

    uint baudrate = spi_init(SPI_PORT, 9600);
}



// LIB https://github.com/litehacker/rfrc522-spi-c/blob/master/rfid-spi.c#L1203

/**
 * Writes a byte to the specified register in the MFRC522 chip.
 * The interface is described in the datasheet section 8.1.2.
 */
void
write_mfrc522(uint8_t adr, uint8_t val)
{
    gpio_put(CS, 0);
    spi_write_blocking(SPI_PORT, &adr, 1);
    gpio_put(CS, 1);
}

/**
 * Writes a number of bytes to the specified register in the MFRC522 chip.
 * The interface is described in the datasheet section 8.1.2.
 */
void
write_bytes_mfrc522( uint8_t reg, uint8_t count, uint8_t *values)
{
    gpio_put(CS, false);
    spi_write_blocking(SPI_PORT, &reg, 1);
    spi_write_blocking(SPI_PORT, values, count);
    gpio_put(CS, true);
} // End PCD_WriteRegister()

static uint8_t read_mfrc522(uint8_t reg) {
    uint8_t ret = 0x0;
    gpio_put(CS, false);
    spi_read_blocking(SPI_PORT, ret, &reg, 1);
    gpio_put(CS, true);
}

/**
 * Clears the bits given in mask from register reg.
 */
void
pcd_clear_register_bit_mask(pcd_register reg, uint8_t mask)
{
    uint8_t tmp;
    tmp = read_mfrc522(reg);
    write_mfrc522(reg, tmp & (~mask));    // clear bit mask
} // End pcd_clear_register_bit_mask()

/**
 * Sets the bits given in mask in register reg.
 */
void
pcd_set_register_bit_mask(uint8_t reg, uint8_t mask)
{
    uint8_t tmp;
    tmp = read_mfrc522(reg);
    write_mfrc522(reg, tmp | mask);     // set bit mask
} // End pcd_set_register_bit_mask()

/**
 * Reads a number of bytes from the specified register in the MFRC522 chip.
 * The interface is described in the datasheet section 8.1.2.
 */
void read_fifo_mfrc522( pcd_register reg, uint8_t count, uint8_t *values, uint8_t rx_align)
{
    if (count == 0)
    {
        return;
    }
    //Serial.print(F("Reading "));  Serial.print(count); Serial.println(F(" bytes from register."));
    uint8_t index = 0;             // Index in values array.
    uint8_t ret = 0;
//    GPIO_CLR_PIN(GPIO_A_BASE, GPIO_PIN_6);
    gpio_put(CS, false);
    count--;                // One read is performed outside of the loop

    if (rx_align) {    // Only update bit positions rx_align..7 in values[0]
        // Create bit mask for bit positions rx_align..7
        uint8_t mask = (0xFF << rx_align) & 0xFF;
        // Read value and tell that we want to read the same address again.
        uint8_t value;
        ret = read_mfrc522(reg);
        value=ret;
        // Apply mask to both current value of values[0] and the new data in value.
        values[0] = (values[0] & ~mask) | (value & mask);
        index++;
    }

    while (index < count) {
        ret = read_mfrc522(reg);
        values[index] = ret;  // Read value and tell that we want to read the same address again.
        index++;
    }

    ret = read_mfrc522(reg);
    values[index] = ret;  // Read value and tell that we want to read the same address again.

//    GPIO_SET_PIN(GPIO_A_BASE, GPIO_PIN_6);    // Release slave again
    gpio_put(CS, true);

} // End PCD_ReadRegister()

/**
 * Transfers data to the MFRC522 FIFO, executes a command, waits for completion and transfers data back from the FIFO.
 * CRC validation can only be done if back_data and back_len are specified.
 *
 * @return STATUS_OK on success, STATUS_??? otherwise.
 */
status_code
pcd_communicate_tith_picc(uint8_t command, uint8_t wait_irq, uint8_t *send_data, uint8_t send_len, uint8_t *back_data, uint8_t *back_len, uint8_t *valid_bits, uint8_t rx_align, bool check_crc)
{
    // Prepare values for BitFramingReg
    uint8_t txLastBits = valid_bits ? *valid_bits : 0;
    uint8_t bitFraming = (rx_align << 4) + txLastBits;    // rx_align = BitFramingReg[6..4]. TxLastBits = BitFramingReg[2..0]

    write_mfrc522(CommandReg, PCD_Idle);                  // Stop any active command.
    write_mfrc522(ComIrqReg, 0x7F);                       // Clear all seven interrupt request bits
    write_mfrc522(FIFOLevelReg, 0x80);                    // FlushBuffer = 1, FIFO initialization
    write_bytes_mfrc522(FIFODataReg, send_len, send_data);  // Write send_data to the FIFO
    write_mfrc522(BitFramingReg, bitFraming);             // Bit adjustments
    write_mfrc522(CommandReg, command);                   // Execute the command

    if (command == PCD_Transceive)
    {
        pcd_set_register_bit_mask(BitFramingReg, 0x80);        // StartSend=1, transmission of data starts
    }

    // Wait for the command to complete.
    // In pcd_initialization() we set the TAuto flag in TModeReg. This means the timer automatically starts when the PCD stops transmitting.
    // Each iteration of the do-while-loop takes 17.86μs.
    // TODO check/modify for other architectures than Arduino Uno 16bit

    uint16_t i;
    for (i = 2000; i > 0; i--)
    {
        uint8_t n = read_mfrc522(ComIrqReg); // ComIrqReg[7..0] bits are: Set1 TxIRq RxIRq IdleIRq HiAlertIRq LoAlertIRq ErrIRq TimerIRq
        if (n & wait_irq)
        {          // One of the  that signal success has been set.
            break;
        }
        if (n & 0x01)
        {           // Timer interrupt - nothing received in 25ms
            return STATUS_TIMEOUT;
        }
    }

    // 35.7ms and nothing happend. Communication with the MFRC522 might be down.
    if (i == 0)
    {
        return STATUS_TIMEOUT;
    }

    // Stop now if any errors except collisions were detected.
    uint8_t errorRegValue = read_mfrc522(ErrorReg); // ErrorReg[7..0] bits are: WrErr TempErr reserved BufferOvfl CollErr CRCErr ParityErr ProtocolErr
    if (errorRegValue & 0x13)
    {  // BufferOvfl ParityErr ProtocolErr
        return STATUS_ERROR;
    }

    uint8_t _valid_bits = 0;

    // If the caller wants data back, get it from the MFRC522.
    if (back_data && back_len)
    {
        uint8_t n = read_mfrc522(FIFOLevelReg);  // Number of bytes in the FIFO
        if (n > *back_len)
        {
            return STATUS_NO_ROOM;
        }
        *back_len = n;                     // Number of bytes returned
        read_fifo_mfrc522(FIFODataReg, n, back_data, rx_align);  // Get received data from FIFO
        _valid_bits = read_mfrc522(ControlReg) & 0x07;   // RxLastBits[2:0] indicates the number of valid bits in the last received byte. If this value is 000b, the whole byte is valid.
        if (valid_bits)
        {
            *valid_bits = _valid_bits;
            //printf(" *valid_bits = %x _valid_bits = %x\n",*valid_bits, _valid_bits);

        }
    }
    // Tell about collisions
    if (errorRegValue & 0x08)
    {   // CollErr
        return STATUS_COLLISION;
    }

//    // Perform CRC_A validation if requested.
//    if (back_data && back_len && check_crc)
//    {
//        // In this case a MIFARE Classic NAK is not OK.
//        if (*back_len == 1 && _valid_bits == 4)
//        {
//            return STATUS_MIFARE_NACK;
//        }
//        // We need at least the CRC_A value and all 8 bits of the last byte must be received.
//        if (*back_len < 2 || _valid_bits != 0)
//        {
//            return STATUS_CRC_WRONG;
//        }
//        // Verify CRC_A - do our own calculation and store the control in controlBuffer.
//        uint8_t controlBuffer[2];
//        status_code status = pcd_calculate_crc(&back_data[0], *back_len - 2, &controlBuffer[0]);
//        if (status != STATUS_OK)
//        {
//            return status;
//        }
//        if ((back_data[*back_len - 2] != controlBuffer[0]) || (back_data[*back_len - 1] != controlBuffer[1]))
//        {
//            return STATUS_CRC_WRONG;
//        }
//    }

    return STATUS_OK;
} // End pcd_communicate_tith_picc()


/**
 * Executes the Transceive command.
 * CRC validation can only be done if back_data and back_len are specified.
 *
 * @return STATUS_OK on success, STATUS_??? otherwise.
 */
//status_code
//pcd_transceive_data(uint8_t *send_data, uint8_t send_len, uint8_t *back_data, uint8_t *back_len, uint8_t *valid_bits, uint8_t rx_align, bool check_crc)
//{
//    uint8_t wait_irq = 0x30;    // RxIRq and IdleIRq
//    uint8_t result = pcd_communicate_tith_picc(PCD_Transceive, wait_irq, send_data, send_len, back_data, back_len, valid_bits, rx_align, check_crc);
//    return static_cast<status_code>(result);
//} // End pcd_transceive_data()


/**
 * Transmits REQA or WUPA commands.
 * Beware: When two PICCs are in the field at the same time I often get STATUS_TIMEOUT - probably due do bad antenna design.
 *
 * @return STATUS_OK on success, STATUS_??? otherwise.
 */
status_code
picc_reqa_or_wupa( uint8_t command, uint8_t *bufferATQA, uint8_t *bufferSize)
{
    uint8_t valid_bits;
    status_code status;

    if (bufferATQA == NULL || *bufferSize < 2)
    {  // The ATQA response is 2 bytes long.
        //printf("picc_reqa_or_wupa > STATUS_NO_ROOM: %x\n", STATUS_NO_ROOM);
        return STATUS_NO_ROOM;
    }

    pcd_clear_register_bit_mask(CollReg, 0x80);    // ValuesAfterColl=1 => Bits received after collision are cleared.
    valid_bits = 7;                  // For REQA and WUPA we need the short frame format - transmit only 7 bits of the last (and only) byte. TxLastBits = BitFramingReg[2..0]
    status = pcd_transceive_data(&command, 1, bufferATQA, bufferSize, &valid_bits, 0, false);
    if (status != STATUS_OK)
    {
        //printf("picc_reqa_or_wupa > status: %x\n", status);
        return status;
    }
    if (*bufferSize != 2 || valid_bits != 0)
    {   // ATQA must be exactly 16 bits.
//        printf("STATUS_ERROR: %x\n", STATUS_ERROR);
        return STATUS_ERROR;
    }
    return STATUS_OK;
} // End picc_reqa_or_wupa()


/**
 * Transmits a REQuest command, Type A. Invites PICCs in state IDLE to go to READY and prepare for anticollision or selection. 7 bit frame.
 * Beware: When two PICCs are in the field at the same time I often get STATUS_TIMEOUT - probably due do bad antenna design.
 *
 * @return STATUS_OK on success, STATUS_??? otherwise.
 */
status_code
picc_request_a( uint8_t *bufferATQA,uint8_t *bufferSize)
{
    return picc_reqa_or_wupa(PICC_CMD_REQA, bufferATQA, bufferSize);
} // End picc_request_a()