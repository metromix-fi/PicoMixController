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
#include "utils/GlobalState.h"


int main() {
    printf("Initializing...\n");
    stdio_init_all();
    initializeGlobalStruct();

    printf("configuring pins...\n");
    setup_display_gpios();
    setup_input_gpios();

    printf("Creating tasks...\n");

    // Networking
//    TaskHandle_t networking_task_handle = NULL;
//    xTaskCreate(
//                networkTask,
//                "networking task",
//                1024,
//                NULL,
//                tskIDLE_PRIORITY + 1,
//                &networking_task_handle
//            );

    // Display
    TaskHandle_t animation_task_handle = NULL;
    xTaskCreate(animationTask,
                "animationTask",
                1024,
                NULL,
                tskIDLE_PRIORITY + 3,
                &animation_task_handle
                );

    vTaskStartScheduler();

    return 0;
}


