//#include <sys/stat.h>
//#include <sys/cdefs.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"

#include "FreeRTOS.h"
#include "FreeRTOSConfig.h"
#include "task.h"

#include "input/inputhandler.h"

#include "networking/httpclient.h"

#include "display/animationtask.h"
#include "display/renderer.h"

#include "utils/GlobalState.h"

#include "rfid/rfidtask.h"
#include "toftask/toftask.h"

#define PICO_TIME_DEFAULT_ALARM_POOL_DISABLED = 1;


int main() {
    printf("Initializing...\n");
    stdio_init_all();

    // inits are in animation task now
//    printf("configuring pins...\n");
    network_setup();
    setup_display_gpios();
    setup_input_gpios();
    setup_rfid_gpios();
    setup_tof();


    printf("Creating tasks...\n");

    // Networking
    TaskHandle_t networking_task_handle = NULL;
    xTaskCreate(
                networkTask,
                "networking task",
                1024,
                NULL,
                tskIDLE_PRIORITY + 1,
                &networking_task_handle
            );
    globalStruct.networkTaskHandle = networking_task_handle;

    // Display
    TaskHandle_t animation_task_handle = NULL;
    xTaskCreate(animationTask,
                "animationTask",
                1024,
                NULL,
                tskIDLE_PRIORITY + 1,
                &animation_task_handle
                );

// Rendering
//    TaskHandle_t rendering_task_handle = NULL;
//    xTaskCreate(
//            renderingTask,
//            "rendering task",
//            1024,
//            NULL,
//            tskIDLE_PRIORITY + 3,
//            &rendering_task_handle
//    );

    // RFID
    TaskHandle_t rfid_task_handle = NULL;
    xTaskCreate(
            rfid_task,
            "rfid task",
            1024,
            NULL,
            tskIDLE_PRIORITY + 2,
            &rfid_task_handle
    );
    globalStruct.rfidTaskHandle = rfid_task_handle;

// Time of Flight
    TaskHandle_t tof_task_handle = NULL;
    xTaskCreate(
            tof_task,
            "tof task",
            1024,
            NULL,
            tskIDLE_PRIORITY + 2,
            &tof_task_handle
    );
    globalStruct.tofTaskHandle = tof_task_handle;


    vTaskStartScheduler();

    return 0;
}


