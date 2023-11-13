//
// Created by jesse on 10.11.23.
//

#ifndef METROMIX_RFIDTASK_H
#define METROMIX_RFIDTASK_H

#include "mfrc522.h"

_Noreturn void rfid_task(void *pvParameters);

void MFRC522WriteRegister(MFRC522Driver *mfrc522p, uint8_t addr, uint8_t val);

uint8_t MFRC522ReadRegister(MFRC522Driver *mfrc522p, uint8_t addr);

void setup_rfid_gpios(void);
#endif //METROMIX_RFIDTASK_H
