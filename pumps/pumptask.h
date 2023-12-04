//
// Created by jesse on 04.12.23.
//

#ifndef METROMIX_PUMPTASK_H
#define METROMIX_PUMPTASK_H

#include <stdint-gcc.h>

#define IN1_PIN 26

#define TIME_TO_POUR_LARGE 1000
#define TIME_TO_POUR_SMALL 500

typedef enum PumpIndex {
    PUMP_1 = 0,
    PUMP_2,
    PUMP_3,
    ALL_PUMPS,
} PumpIndex;

//typedef struct PumpControllerCommand {
//    PumpIndex pumpIndex1;
//    PumpIndex pumpIndex2;
//    TickType_t timeToPour;
//};

typedef enum PumpCommand {
    PUMP_ON = 0,
    PUMP_OFF,
} PumpCommand;

typedef struct PumpData {
    PumpCommand command;
    uint8_t pumpNumber;
    uint16_t timeToPour;
} PumpData;

typedef enum PumpTaskIndex {
    TASK_1 = 0,
    TASK_2,
} PumpTaskIndex;

_Noreturn void pumpControllerTask(void *pvParameters);

_Noreturn void pumpTask(void *pvParameters);

#endif //METROMIX_PUMPTASK_H
