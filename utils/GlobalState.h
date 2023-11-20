//
// Created by jesse on 08.11.23.
//

#ifndef METROMIX_GLOBALSTATE_H
#define METROMIX_GLOBALSTATE_H

#include "FreeRTOS.h"
#include "semphr.h"
#include "queue.h"

typedef struct {
    QueueHandle_t rotaryEncoderQueue;
    QueueHandle_t rfidQueue;
    QueueHandle_t tofQueue;
    QueueHandle_t pouringProgressQueue;
} GlobalStruct_t;

extern GlobalStruct_t globalStruct;

void initializeGlobalStruct();

#endif //METROMIX_GLOBALSTATE_H
