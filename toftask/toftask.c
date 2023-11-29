//
// Created by jesse on 20.11.23.
//

#include "toftask.h"
#include <pico/printf.h>
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "pico/cyw43_arch.h"
#include "FreeRTOS.h"
#include "task.h"
#include "VL53L1X-C-API-Pico/library/public/include/VL53L1X_api.h"
#include "VL53L1X-C-API-Pico/library/public/include/VL53L1X_types.h"
#include "queue.h"
#include "utils/GlobalState.h"

#define I2C_DEV_ADDR 0x29

int setup_tof() {

    printf("init tof\n");
    // Init I2C
    if (VL53L1X_I2C_Init(I2C_DEV_ADDR, i2c0) < 0) {
        printf("Error initializing sensor.\n");
        return 0;
    }

    return 1;
}

_Noreturn void tof_task(void *pvParameters) {

    VL53L1X_Status_t status;
    VL53L1X_Result_t results;

    // INIT
    uint8_t sensorState;
    do {
        status += VL53L1X_BootState(I2C_DEV_ADDR, &sensorState);
        vTaskDelay(2);
//        VL53L1X_WaitMs(I2C_DEV_ADDR, 2);
    } while (sensorState == 0);
    printf("Sensor booted.\n");

    status = VL53L1X_SensorInit(I2C_DEV_ADDR);
    status += VL53L1X_SetDistanceMode(I2C_DEV_ADDR, 1);
    status += VL53L1X_SetTimingBudgetInMs(I2C_DEV_ADDR, 100);
    status += VL53L1X_SetInterMeasurementInMs(I2C_DEV_ADDR, 100);
    status += VL53L1X_StartRanging(I2C_DEV_ADDR);
    // END INIT

    bool first_range = true;
    while (true) {

        // Wait for notification
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        printf("tof task\n");

        // Read from stdin (blocking)
//        getc(stdin);
        // Wait until we have new data
        uint8_t dataReady;
        do {
            status = VL53L1X_CheckForDataReady(I2C_DEV_ADDR, &dataReady);
//            VL53L1X_WaitMs(I2C_DEV_ADDR, 1);
            vTaskDelay(1);
        } while (dataReady == 0);

        // Read and display result
        status += VL53L1X_GetResult(I2C_DEV_ADDR, &results);
        printf("Status = %2d, dist = %5d, Ambient = %2d, Signal = %5d, #ofSpads = %5d\n",
               results.status, results.distance, results.ambient, results.sigPerSPAD, results.numSPADs);

        xQueueSendToBack(globalStruct.tofQueue, &results.distance, 0);

        // Clear the sensor for a new measurement
        status += VL53L1X_ClearInterrupt(I2C_DEV_ADDR);
        if (first_range) {  // Clear twice on first measurement
            status += VL53L1X_ClearInterrupt(I2C_DEV_ADDR);
            first_range = false;
        }

//        vTaskDelay(1000);
    }
}