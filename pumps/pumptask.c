//
// Created by jesse on 04.12.23.
//

#include "pumptask.h"
#include <pico/printf.h>
#include <hardware/gpio.h>
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "utils/GlobalState.h"

typedef struct PumpState {
    uint16_t progress;
} PumpState;

void setup_pump_gpios(void) {
    gpio_init(PUMP1_PIN);
    gpio_set_dir(PUMP1_PIN, GPIO_OUT);

    gpio_init(PUMP2_PIN);
    gpio_set_dir(PUMP2_PIN, GPIO_OUT);

    gpio_init(PUMP3_PIN);
    gpio_set_dir(PUMP3_PIN, GPIO_OUT);
}

void switch_current_queue(QueueHandle_t *currentTaskQueue) {
    if (*currentTaskQueue == globalStruct.pumpTask1Queue) {
        *currentTaskQueue = globalStruct.pumpTask2Queue;
    } else {
        *currentTaskQueue = globalStruct.pumpTask1Queue;
    }
}

_Noreturn void pumpControllerTask(void *pvParameters) {
    setup_pump_gpios();
    vTaskDelay(500);
    printf("pumpControllerTask started\n");

    QueueHandle_t currentTaskQueue = globalStruct.pumpTask1Queue;

    PumpData pumpData;
    while (true) {
        xQueueReceive(globalStruct.pumpControllerQueue, &pumpData, portMAX_DELAY);
        switch (pumpData.command) {
            case PUMP_OFF:
                xQueueSendToBack(globalStruct.pumpTask1Queue, &pumpData, portMAX_DELAY);
                xQueueSendToBack(globalStruct.pumpTask2Queue, &pumpData, portMAX_DELAY);
                break;
            case PUMP_ON:
                xQueueSendToBack(currentTaskQueue, &pumpData, portMAX_DELAY);
                switch_current_queue(&currentTaskQueue);
                break;
        }
    }
}


_Noreturn void pumpTask(void *pvParameters) {
    PumpTaskIndex *pumpTaskIndex = (PumpTaskIndex *) pvParameters;
    QueueHandle_t pumpTaskQueue;
    PumpData pumpData;

    vTaskDelay(500);

    if (*pumpTaskIndex == TASK_1) {
        printf("pumpTask 1 started\n");
        pumpTaskQueue = globalStruct.pumpTask1Queue;
    } else {
        printf("pumpTask 2 started\n");
        pumpTaskQueue = globalStruct.pumpTask2Queue;
    }

    while (true) {
        xQueueReceive(pumpTaskQueue, &pumpData, portMAX_DELAY);
        if (pumpData.command == PUMP_ON) {
            printf("Pump %d on\n", pumpData.pumpNumber);

            switch (pumpData.pumpNumber) {
                case PUMP_1: {
                    gpio_put(PUMP1_PIN, 1);
                    xQueueReceive(pumpTaskQueue, &pumpData, pumpData.timeToPour);
                    gpio_put(PUMP1_PIN, 0);
                    break;
                }
                case PUMP_2: {
                    gpio_put(PUMP2_PIN, 1);
                    xQueueReceive(pumpTaskQueue, &pumpData, pumpData.timeToPour);
                    gpio_put(PUMP2_PIN, 0);
                    break;
                }
                case PUMP_3: {
                    gpio_put(PUMP3_PIN, 1);
                    xQueueReceive(pumpTaskQueue, &pumpData, pumpData.timeToPour);
                    gpio_put(PUMP3_PIN, 0);
                    break;
                }
            }
        }
    }
}