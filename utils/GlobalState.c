//
// Created by jesse on 08.11.23.
//
#include "FreeRTOS.h"
#include "semphr.h"
#include "queue.h"
#include "GlobalState.h"
#include "input/inputhandler.h"
#include "rfid/mfrc522.h"

GlobalStruct_t globalStruct;

void initializeGlobalStruct() {
    // Initialize queues
    globalStruct.rotaryEncoderQueue = xQueueCreate(32, sizeof(InputEvent));
    globalStruct.rfidQueue = xQueueCreate(8 , sizeof(MifareUID));
    globalStruct.tofQueue = xQueueCreate(8, sizeof(uint16_t));
    globalStruct.pouringProgressQueue = xQueueCreate(8, sizeof(uint8_t));
}
