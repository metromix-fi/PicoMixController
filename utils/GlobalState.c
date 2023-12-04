//
// Created by jesse on 08.11.23.
//
#include <stdbool.h>
#include "FreeRTOS.h"
#include "semphr.h"
#include "queue.h"
#include "GlobalState.h"
#include "input/inputhandler.h"
#include "rfid/mfrc522.h"
#include "pumps/pumptask.h"

GlobalStruct_t globalStruct;

void initializeGlobalStruct() {
    // Initialize queues
    globalStruct.rotaryEncoderQueue = xQueueCreate(32, sizeof(InputEvent));
    globalStruct.rfidQueue = xQueueCreate(8 , sizeof(MifareUID));
    globalStruct.authenticationQueue = xQueueCreate(8, sizeof(bool));
    globalStruct.tofQueue = xQueueCreate(8, sizeof(uint16_t));
    globalStruct.pumpControllerQueue = xQueueCreate(8, sizeof(PumpData));
    globalStruct.pumpTask1Queue = xQueueCreate(8, sizeof(PumpData));
    globalStruct.pumpTask2Queue = xQueueCreate(8, sizeof(PumpData));
}
