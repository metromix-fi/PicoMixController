//
// Created by jesse on 08.11.23.
//
#include "FreeRTOS.h"
#include "semphr.h"
#include "queue.h"
#include "GlobalState.h"

GlobalStruct_t globalStruct;

void initializeGlobalStruct() {
    // Initialize queues
    globalStruct.rotaryEncoderQueue = xQueueCreate(1024, sizeof(int8_t));
}
