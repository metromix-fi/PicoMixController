//
// Created by jesse on 08.11.23.
//


#include "networking/httpclient.h"
#include "input/inputhandler.h"
#include "task.h"
#include "FreeRTOSConfig.h"
#include "FreeRTOS.h"
#include "hardware/i2c.h"
#include "pico/stdlib.h"
#include <string.h>
#include <stdint.h>
#include <stdio.h>

#include "BMSPA_font.h"
#include "crackers_font.h"
#include "bubblesstandard_font.h"
#include "acme_5_outlines_font.h"
#include "image.h"
#include "ssd1306.h"

#include "animationtask.h"
#include "queue.h"
#include "utils/GlobalState.h"
#include "utils/helper.h"
#include "rfid/rfidtask.h"
#include "toftask/toftask.h"


#define SLEEPTIME 25


const uint8_t num_chars_per_disp[] = {7, 7, 7, 5};
const uint8_t *fonts[4] = {acme_font, bubblesstandard_font, crackers_font, BMSPA_font};

static void draw_number(ssd1306_t *disp, uint32_t x, uint32_t y, uint32_t scale, int value) {
    // Display the current number on the OLED screen
    char buf[8];
    snprintf(buf, sizeof(buf), "%d", value);
    ssd1306_draw_string(disp, x, y, scale, buf);
}

static void display_img(ssd1306_t *disp) {
    ssd1306_bmp_show_image(disp, image_data, image_size);
    ssd1306_show(disp);
}

void setup_display_gpios(void) {
    i2c_init(i2c1, 400000);
    gpio_set_function(14, GPIO_FUNC_I2C);
    gpio_set_function(15, GPIO_FUNC_I2C);
    gpio_pull_up(14);
    gpio_pull_up(15);
}


// used as main application loop
_Noreturn void animationTask(void *param) {

    // ALL INITS
    printf("configuring pins...\n");
    initializeGlobalStruct();
//    setup_display_gpios();
//    setup_input_gpios();
//    setup_rfid_gpios();
//    setup_tof();


    const int amount_drinks = 2;
    const char *words[] = {"Vodka Cola", "Rum Cola"};
    const int liquid1[] = {DRINK_1, DRINK_2};
    const int liquid2[] = {DRINK_3, DRINK_3};

    ssd1306_t disp;
    disp.external_vcc = false;
    ssd1306_init(&disp, 128, 64, 0x3C, i2c1);
    ssd1306_clear(&disp);
    ssd1306_draw_string_with_font(&disp, 8, 24, 1, acme_font, "Hello World!");
    ssd1306_show(&disp);

    printf("ANIMATION!\n");

//    char buf[8];

    // Input
    InputEvent rotary_input = -1;
    MifareUID rfid_state;

    // State
    bool needs_update = true;
    MenuState state = DRINK_SELECT;
    CocktailConfig config;
    config.drink1 = DRINK_1;
    config.drink2 = DRINK_2;
    config.size = SMALL;
    config.mixture[0] = 50;
    config.mixture[1] = 50;

    // drink select
    uint8_t drink = 0;
    uint8_t last_drink = 0;

    // idle counter
    int idle_counter = 0;

    // tof distance
    uint16_t distance = 0;

    TickType_t ticks_to_wait = 10000;
    for (;;) {

        // get input
        if (!needs_update) {
            if (xQueueReceive(globalStruct.rotaryEncoderQueue, &rotary_input, ticks_to_wait) == pdFALSE) {
                state = IDLE;
                ticks_to_wait = 64;
            } else {
                if (idle_counter != 0) {
                    idle_counter = 0;
                    state = DRINK_SELECT;
                    ticks_to_wait = 30000;
                }
            }
        } else {
            needs_update = false;
        }

        // Break out of nested switch after state change with goto (show next display without waiting for input)
        // different actions for different states
        switch (state) {
            case DRINK_SELECT:
//                printf("DRINK_SELECT\n");

                switch (rotary_input) {
                    case CW_ROTATION:
                        drink = mod(drink + 1, amount_drinks);
                        break;
                    case CCW_ROTATION:
                        drink = mod(drink - 1, amount_drinks);
                        break;
                    case PUSH:
                        // confirm drink and change state
                        config.drink1 = drink;
                        state = SIZE_SELECT;
                        rotary_input = -1;
                        needs_update = true;
                }

                if (!needs_update) {
                    last_drink = drink;
                    ssd1306_clear(&disp);
                    ssd1306_draw_string_with_font(&disp, 8, 24, 1, acme_font, words[drink]);
                    ssd1306_show(&disp);
                }

                break;
            case SIZE_SELECT:
//                printf("SIZE_SELECT\n");
                switch (rotary_input) {
                    case CW_ROTATION:
                        config.size = (config.size + 1) % 3;
                        break;
                    case CCW_ROTATION:
                        config.size = (config.size - 1) % 3;
                        break;
                    case PUSH:
                        // confirm size and change state
                        state = MIXTURE_SELECT;
                        rotary_input = -1;
                        needs_update = true;
                }

                if (!needs_update) {
                    ssd1306_clear(&disp);
                    ssd1306_draw_string_with_font(&disp, 8, 24, 1, acme_font, "Size");
                    ssd1306_draw_string_with_font(&disp, 8, 48, 1, acme_font,
                                                  config.size == SMALL ? "Small" : config.size == MEDIUM ? "Medium"
                                                                                                         : "Large");
                    ssd1306_show(&disp);
                }

                break;
            case MIXTURE_SELECT:
//                printf("MIXTURE_SELECT\n");
                switch (rotary_input) {
                    case CW_ROTATION:
                        config.mixture[0] = clamp(config.mixture[0] + 1, 0, 100);
                        config.mixture[1] = clamp(config.mixture[1] - 1, 0, 100);
                        break;
                    case CCW_ROTATION:
                        config.mixture[0] = clamp(config.mixture[0] - 1, 0, 100);
                        config.mixture[1] = clamp(config.mixture[1] + 1, 0, 100);
                        break;
                    case PUSH:
                        // confirm mixture and change state
                        state = AUTHENTICATION;
                        rotary_input = -1;
                        needs_update = true;
                }

                if (!needs_update) {
                    // display percentage determined by config.mixture
                    ssd1306_clear(&disp);
                    ssd1306_draw_string_with_font(&disp, 8, 8, 1, acme_font, "Mixture");
                    draw_number(&disp, 8, 24, 1, config.mixture[0]);
                    // ssd1306
                    draw_number(&disp, 8, 48, 1, config.mixture[1]);
                    ssd1306_show(&disp);
                }

                break;
            case AUTHENTICATION:
                printf("AUTHENTICATION\n");


                ssd1306_clear(&disp);
                ssd1306_draw_string_with_font(&disp, 8, 24, 1, acme_font, "Authenticate RFID");
                ssd1306_show(&disp);

                for (int i = 0; i < 10; ++i) {
                    // Notify RFID_Task to start measuring
                    xTaskNotifyGive(globalStruct.rfidTaskHandle);

                    // TODO: use queue from wifi and not rfid directly (rfid -> wifi(get auth info from server) -> animation)
                    if (xQueueReceive(globalStruct.rfidQueue, &rfid_state, portMAX_DELAY)) {
                        if (rfid_state.size == 0) {
                            vTaskDelay(1000);
                            continue;
                        } else {
                            printf("Success!\n");
                            printf("rfid_state: %x, %04X\n", rfid_state.size, rfid_state.bytes);
                            state = POURING;
                            needs_update = true;
                        }
                    }
                }

                if (!needs_update) {
                    state = ERROR;
                }

                break;
            case ERROR:
                printf("ERROR\n");
                ssd1306_clear(&disp);
                ssd1306_draw_string_with_font(&disp, 8, 24, 1, acme_font, "Error");
                ssd1306_show(&disp);

                vTaskDelay(3000);
                state = DRINK_SELECT;
                break;
            case POURING:
                printf("POURING\n");

                // Notify ToF_Task to start measuring
                xTaskNotifyGive(globalStruct.tofTaskHandle);

                xQueueReceive(globalStruct.tofQueue, &distance, portMAX_DELAY);
                if (distance > MAX_CUP_DISTANCE) {
                    state = NO_CUP;
                    needs_update = true;
                }

                if (!needs_update) {

                    ssd1306_clear(&disp);
                    ssd1306_draw_string_with_font(&disp, 8, 24, 1, acme_font, "Pouring");
                    ssd1306_show(&disp);

                    // TODO:Show timer progress

                    for (int i = 0; i < 10; ++i) {
                        // Notify ToF_Task to start measuring
                        xTaskNotifyGive(globalStruct.tofTaskHandle);

                        xQueueReceive(globalStruct.tofQueue, &distance, portMAX_DELAY);
                        if (distance > MAX_CUP_DISTANCE) {
                            state = NO_CUP;
                            needs_update = true;
                        } else {
                            ssd1306_clear(&disp);
                            ssd1306_draw_string_with_font(&disp, 8, 24, 1, acme_font, "Pouring");
                            draw_number(&disp, 8, 48, 1, 10 - i);
                            ssd1306_show(&disp);
                            vTaskDelay(DISTANCE_MEASURE_DELAY);
                        }
                    }

                    state = DONE;
                    needs_update = true;
                    break;
                }

            case NO_CUP:
                printf("NO_CUP\n");

                ssd1306_clear(&disp);
                ssd1306_draw_string_with_font(&disp, 8, 24, 1, acme_font, "Place Cup");
                ssd1306_show(&disp);

                for (int i = 0; i < 10; ++i) {
                    // Notify ToF_Task to start measuring
                    xTaskNotifyGive(globalStruct.tofTaskHandle);

                    xQueueReceive(globalStruct.tofQueue, &distance, portMAX_DELAY);
                    if (distance <= MAX_CUP_DISTANCE) {
                        state = POURING;
                        needs_update = true;
                    } else {
                        vTaskDelay(DISTANCE_MEASURE_DELAY);
                    }
                }

                if (!needs_update) {
                    state = ERROR;
                }

                break;
            case DONE:
                printf("DONE\n");

                ssd1306_clear(&disp);
                ssd1306_draw_string_with_font(&disp, 8, 24, 1, acme_font, "Done");
                ssd1306_show(&disp);

                vTaskDelay(5000);
                state = DRINK_SELECT;
                needs_update = true;
                break;

            case IDLE:
                // TODO: Play animation
                idle_counter++;
                ssd1306_clear(&disp);
                ssd1306_draw_string_with_font(&disp, 8, 24, 1, acme_font, "Idle");
                draw_number(&disp, 8, 48, 1, idle_counter);
                ssd1306_show(&disp);

                break;
        }
    }
}
