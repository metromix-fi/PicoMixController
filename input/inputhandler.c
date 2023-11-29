//
// Created by jesse on 06.11.23.
//

#include <sys/cdefs.h>
#include "inputhandler.h"
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/gpio.h"

#include "FreeRTOS.h"
#include "task.h"

#include "utils/GlobalState.h"
#include "timers.h"

// Define GPIO pins for the rotary encoder
#define ROTARY_A 10
#define ROTARY_B 11
#define ROTARY_PUSH 12

void input_isr(uint gpio, uint32_t events);

/* GOBAL STATE */
//GlobalStruct_t globalStruct;


static void enable_interrupts() {
    // Activate the interrupts
    gpio_set_irq_enabled_with_callback(ROTARY_A, GPIO_IRQ_EDGE_RISE, true, &input_isr);
//    gpio_set_irq_enabled_with_callback(ROTARY_A, GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE, true, &input_isr);
//    gpio_set_irq_enabled(ROTARY_B, GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE, true);
    gpio_set_irq_enabled(ROTARY_PUSH, GPIO_IRQ_EDGE_RISE, true);
}

// TODO: Needs enabling/disabling during the ISR?
static void disable_interrupts() {
    // Deactivate the interrupts
    gpio_set_irq_enabled(ROTARY_A, GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE, false);
    gpio_set_irq_enabled(ROTARY_B, GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE, false);
    gpio_set_irq_enabled(ROTARY_PUSH, GPIO_IRQ_EDGE_RISE, false);
}

static void disable_rotary_push_interrupt() {
    // Deactivate the interrupt
    gpio_set_irq_enabled(ROTARY_PUSH, GPIO_IRQ_EDGE_RISE, false);
}

static int64_t enable_rotary_push_interrupt() {
    // Enable the interrupt
    gpio_set_irq_enabled(ROTARY_PUSH, GPIO_IRQ_EDGE_RISE, true);
    return 0;
}

static void enable_rotary_push_interrupt_rtos() {
    // Enable the interrupt
    gpio_set_irq_enabled(ROTARY_PUSH, GPIO_IRQ_EDGE_RISE, true);
}


void setup_input_gpios(void) {
    // Configure GPIO pins for the rotary encoder as inputs
    gpio_init(ROTARY_A);
    gpio_set_dir(ROTARY_A, GPIO_IN);
    gpio_pull_up(ROTARY_A);

    gpio_init(ROTARY_B);
    gpio_set_dir(ROTARY_B, GPIO_IN);
    gpio_pull_up(ROTARY_B);


    gpio_init(ROTARY_PUSH);
    gpio_set_dir(ROTARY_PUSH, GPIO_IN);
    gpio_pull_up(ROTARY_PUSH);

    enable_interrupts();
}


void input_isr(uint gpio, uint32_t events) {

//    disable_interrupts();
    static unsigned char lastBState = 0;
    static unsigned char lastAState = 0;

    InputEvent value = 0;

    BaseType_t pxHigherPriorityTaskWoken = pdFALSE;

    // printf("ISR triggered on GPIO %d\n", gpio);

    // Low Resolution Rotary Encoder
    if (gpio == ROTARY_A) {
        if(gpio_get(ROTARY_B)) {
            value = CCW_ROTATION;
            xQueueSendFromISR(globalStruct.rotaryEncoderQueue, &value, &pxHigherPriorityTaskWoken);
        } else if(!gpio_get(ROTARY_B)) {
            value = CW_ROTATION;
            xQueueSendFromISR(globalStruct.rotaryEncoderQueue, &value, &pxHigherPriorityTaskWoken);
        }
    }

// High Resolution Rotary Encoder
//    if (gpio == ROTARY_A) {
//        unsigned char currentAState = events & GPIO_IRQ_EDGE_RISE;
//        // printf("Current A State: %d\n", currentAState);
//        if (currentAState != lastAState) { // If there is a change in the state
//            if (lastBState == 0 && currentAState == GPIO_IRQ_EDGE_RISE) {
//                /* Send a message to the queue */
//                value = CW_ROTATION;
//                xQueueSendFromISR(globalStruct.rotaryEncoderQueue, &value, NULL);
//                // printf("Clockwise Rotation. Value: %d\n", displayedNumber);
//            } else if (lastBState == GPIO_IRQ_EDGE_RISE && currentAState == GPIO_IRQ_EDGE_RISE) {
//                /* Send a message to the queue */
//                value = CCW_ROTATION;
//                xQueueSendFromISR(globalStruct.rotaryEncoderQueue, &value, NULL);
//                // printf("Counter-clockwise Rotation. Value: %d\n", displayedNumber);
//            }
//        }
//        lastAState = currentAState;
//    } else if (gpio == ROTARY_B) {
//        unsigned char currentBState = events & GPIO_IRQ_EDGE_RISE;
//        // printf("Current B State: %d\n", currentBState);
//        if (currentBState != lastBState) { // If there is a change in the state
//            if (lastAState == 0 && currentBState == GPIO_IRQ_EDGE_RISE) {
//                /* Send a message to the queue */
//                value = CCW_ROTATION;
//                xQueueSendFromISR(globalStruct.rotaryEncoderQueue, &value, NULL);
//
//            } else if (lastAState == GPIO_IRQ_EDGE_RISE && currentBState == GPIO_IRQ_EDGE_RISE) {
//                /* Send a message to the queue */
//                value = CW_ROTATION;
//                xQueueSendFromISR(globalStruct.rotaryEncoderQueue, &value, NULL);
//            }
//        }
//        lastBState = currentBState;
//    }

    else { // ROTARY_PUSH
        // printf("Push button pressed\n");
        disable_rotary_push_interrupt();
        value = PUSH;
        xQueueSendFromISR(globalStruct.rotaryEncoderQueue, &value, &pxHigherPriorityTaskWoken);

//        add_alarm_in_ms(50, enable_rotary_push_interrupt, NULL, false);
        TimerHandle_t xTimer = xTimerCreate(
                "RotaryPushTimer",                  // Name of the timer
                pdMS_TO_TICKS(50),      // Timer period in ticks (1000 ms in this example)
                pdFALSE,                  // uxAutoReload pdFALSE for one-shot, pdTRUE for periodic
                (void *) 0,               // Optional identifier
                enable_rotary_push_interrupt_rtos            // Callback function
        );
        xTimerStart(xTimer, 0);
    }

    portYIELD_FROM_ISR(pxHigherPriorityTaskWoken);
    //enable_interrupts();
}