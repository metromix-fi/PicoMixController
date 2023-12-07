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
#include "pumps/pumptask.h"
#include "buzzer/buzzertask.h"


#define SLEEPTIME 25

void set_menu_state(CocktailState *cocktailState, MenuState new, ssd1306_t *disp);

const uint8_t num_chars_per_disp[] = {7, 7, 7, 5};
const uint8_t *fonts[4] = {acme_font, bubblesstandard_font, crackers_font, BMSPA_font};

static void draw_number(ssd1306_t *disp, uint32_t x, uint32_t y, uint32_t scale, int value) {
    // Display the current number on the OLED screen
    char buf[8];
    snprintf(buf, sizeof(buf), "%d", value);
    ssd1306_draw_string(disp, x, y, scale, buf);
}

void draw_dotted_line(ssd1306_t *p, int32_t x1, int32_t y1, int32_t x2, int32_t y2) {
    if (x1 > x2) {
        swap(&x1, &x2);
        swap(&y1, &y2);
    }

    if (x1 == x2) {
        if (y1 > y2)
            swap(&y1, &y2);
        for (int32_t i = y1; i <= y2; ++i)
            ssd1306_clear_pixel(p, x1, i);
        return;
    }

    float m = (float) (y2 - y1) / (float) (x2 - x1);

    for (int32_t i = x1; i <= x2; i += 2) {
        float y = m * (float) (i - x1) + (float) y1;
        ssd1306_clear_pixel(p, i, (uint32_t) y);
    }
}


/* Display images */
static void display_drink_select(ssd1306_t *disp, uint8_t drink) {
    ssd1306_clear(disp);
    switch (drink) {
        case DRINK_1:
            ssd1306_bmp_show_image(disp, left_image, image_size);
            break;
        case DRINK_2:
            ssd1306_bmp_show_image(disp, middle_image, image_size);
            break;
        case DRINK_3:
            ssd1306_bmp_show_image(disp, right_image, image_size);
            break;
        default:
            break;
    }
    ssd1306_show(disp);
}

static void display_size_select(ssd1306_t *disp, uint8_t size) {
    ssd1306_clear(disp);
    switch (size) {
        case SMALL:
            ssd1306_bmp_show_image(disp, small_image, image_size);
            break;
        case LARGE:
            ssd1306_bmp_show_image(disp, large_image, image_size);
            break;
        default:
            break;
    }
    ssd1306_show(disp);
}

static void display_auth(ssd1306_t *disp) {
    ssd1306_clear(disp);
    ssd1306_bmp_show_image(disp, auth_image, image_size);
    ssd1306_show(disp);
}

static void display_error(ssd1306_t *disp) {
    ssd1306_clear(disp);
    ssd1306_bmp_show_image(disp, error_image, image_size);
    ssd1306_show(disp);
}

static void display_nocup(ssd1306_t *disp) {
    ssd1306_clear(disp);
    ssd1306_bmp_show_image(disp, nocup_image, image_size);
    ssd1306_show(disp);
}

static void display_done(ssd1306_t *disp) {
    ssd1306_clear(disp);
    ssd1306_bmp_show_image(disp, done_image, image_size);
    ssd1306_show(disp);
}

static void display_mixture(ssd1306_t *disp, int mixture[]) {
    ssd1306_clear(disp);
    ssd1306_bmp_show_image(disp, mixture_image, image_size);
    draw_number(disp, 72, 24, 1, mixture[0]);
    draw_number(disp, 72, 48, 1, mixture[1]);
    for (int i = 0; i < 40; i++) {
        ssd1306_draw_line(disp, CUP_BASE_X - 9, 59 - i, CUP_BASE_X + 29, 59 - i);
    }
    for (int i = 0; i < mixture[0] * 0.4; i++) {
        draw_dotted_line(disp, CUP_BASE_X - 9, 59 - i, CUP_BASE_X + 29, 59 - i);
    }
    ssd1306_show(disp);
}

static void update_mixture(ssd1306_t *disp, int mixtureArr[]) {

    ssd1306_clear(disp);
    ssd1306_bmp_show_image(disp, mixture_image, image_size);
    draw_number(disp, 72, 24, 1, mixtureArr[0]);
    draw_number(disp, 72, 48, 1, mixtureArr[1]);

    int mixture = mixtureArr[0] * 0.4f;
    uint8_t y = 58 - mixture;
    if (mixture == 0) {
        ssd1306_draw_line(disp, CUP_BASE_X, y, CUP_BASE_X + 20, y);
    } else if (mixture == 1) {
        ssd1306_draw_line(disp, CUP_BASE_X - 2, y, CUP_BASE_X + 22, y);
    } else if (mixture == 2) {
        ssd1306_draw_line(disp, CUP_BASE_X - 3, y, CUP_BASE_X + 23, y);
    } else if (mixture == 3) {
        ssd1306_draw_line(disp, CUP_BASE_X - 4, y, CUP_BASE_X + 24, y);
    } else if (mixture == 4 || mixture == 5) {
        ssd1306_draw_line(disp, CUP_BASE_X - 5, y, CUP_BASE_X + 25, y);
    } else if (y >= 6 && y < 9) {
        ssd1306_draw_line(disp, CUP_BASE_X - 6, y, CUP_BASE_X + 26, y);
    } else if (mixture >= 9 && mixture < 13) {
        ssd1306_draw_line(disp, CUP_BASE_X - 7, y, CUP_BASE_X + 27, y);
    } else if (mixture >= 13 && mixture < 17) {
        ssd1306_draw_line(disp, CUP_BASE_X - 8, y, CUP_BASE_X + 28, y);
    } else if (mixture >= 17 && mixture < 40) {
        ssd1306_draw_line(disp, CUP_BASE_X - 9, y, CUP_BASE_X + 29, y);
    } else {
        // full
    }
}

static void update_mixture_dotted(ssd1306_t *disp, uint8_t mixture) {
    uint8_t mixtureIndex = mixture - 1;
    uint8_t y = 59 - mixture;
    if (mixtureIndex <= 0) {
        // empty
    } else if (mixtureIndex == 1) {
        draw_dotted_line(disp, CUP_BASE_X - 2, y, CUP_BASE_X + 22, y);
    } else if (mixtureIndex == 2) {
        draw_dotted_line(disp, CUP_BASE_X - 3, y, CUP_BASE_X + 23, y);
    } else if (mixtureIndex == 3) {
        draw_dotted_line(disp, CUP_BASE_X - 4, y, CUP_BASE_X + 24, y);
    } else if (mixtureIndex == 4 || mixtureIndex == 5) {
        draw_dotted_line(disp, CUP_BASE_X - 5, y, CUP_BASE_X + 25, y);
    } else if (y >= 6 && y < 9) {
        draw_dotted_line(disp, CUP_BASE_X - 6, y, CUP_BASE_X + 26, y);
    } else if (mixtureIndex >= 9 && mixtureIndex < 13) {
        draw_dotted_line(disp, CUP_BASE_X - 7, y, CUP_BASE_X + 27, y);
    } else if (mixtureIndex >= 13 && mixtureIndex < 17) {
        draw_dotted_line(disp, CUP_BASE_X - 8, y, CUP_BASE_X + 28, y);
    } else {
        draw_dotted_line(disp, CUP_BASE_X - 9, y, CUP_BASE_X + 29, y);
    }
}

void display_idle(ssd1306_t *disp, int idle_counter) {
    int frame = idle_counter % 2;

    ssd1306_clear(disp);
//    draw_number(disp, 36, 48, 1, idle_counter);
    if (frame == 0) {
        ssd1306_bmp_show_image(disp, idle2_image, image_size);
    } else if (frame == 1)  {
        ssd1306_bmp_show_image(disp, idle1_image, image_size);
    }
    ssd1306_show(disp);
}

/* -- End Images -- */

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


    const int amount_drinks = 3;
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

    // Input
//    InputEvent rotary_input = -1;
    MifareUID rfid_state;

    // State
//    bool needs_update = true;
//    MenuState state = DRINK_SELECT;

    CocktailState cocktailState;
    set_menu_state(&cocktailState, DRINK_SELECT, &disp);

    // idle counter
    int idle_counter = 0;

    // tof distance
    uint16_t distance = 0;

    TickType_t ticks_to_wait = 10000;

    //pump command
    PumpData pumpData;

    // buzzer event
    BuzzerEvent buzzerEvent;

    for (;;) {

        // get input
        if (!cocktailState.needsUpdate) {
            if (xQueueReceive(globalStruct.rotaryEncoderQueue, &cocktailState.inputEvent, ticks_to_wait) ==
                pdFALSE) {
                set_menu_state(&cocktailState, IDLE, &disp);
                ticks_to_wait = 512;
            } else {
                if (idle_counter != 0) {
                    idle_counter = 0;
                    set_menu_state(&cocktailState, DRINK_SELECT, &disp);
                    ticks_to_wait = 30000;
                }
            }
        } else {
            cocktailState.needsUpdate = false;
        }

        // Break out of nested switch after cocktailState change with goto (show next display without waiting for input)
        // different actions for different states
        switch (cocktailState.menuState) {
            case DRINK_SELECT:
                switch (cocktailState.inputEvent) {
                    case CW_ROTATION:
                        cocktailState.selectedDrink = mod(cocktailState.selectedDrink = cocktailState.selectedDrink + 1, amount_drinks);
                        break;
                    case CCW_ROTATION:
                        cocktailState.selectedDrink = mod(cocktailState.selectedDrink = cocktailState.selectedDrink - 1, amount_drinks);
                        break;
                    case PUSH:
                        // confirm drink and change cocktailState
                        set_menu_state(&cocktailState, SIZE_SELECT, &disp);
                        cocktailState.inputEvent = -1;
                }

                if (!cocktailState.needsUpdate) {
//                    ssd1306_draw_string_with_font(&disp, 8, 24, 1, acme_font, words[drink]);
                    display_drink_select(&disp, cocktailState.selectedDrink);
                }

                break;
            case SIZE_SELECT:
//                printf("SIZE_SELECT\n");
                switch (cocktailState.inputEvent) {
                    case CW_ROTATION:
                        cocktailState.size = mod(cocktailState.size + 1, 2);
                        break;
                    case CCW_ROTATION:
                        cocktailState.size = mod(cocktailState.size - 1, 2);
                        break;
                    case PUSH:
                        // confirm size and change cocktailState
                        set_menu_state(&cocktailState, MIXTURE_SELECT, &disp);
                        cocktailState.inputEvent = -1;
                }

                if (!cocktailState.needsUpdate) {
//                    ssd1306_clear(&disp);
//                    ssd1306_draw_string_with_font(&disp, 8, 24, 1, acme_font, "Size");
//                    ssd1306_draw_string_with_font(&disp, 8, 48, 1, acme_font,
//                                                  cocktailState.size == SMALL ? "Small" : "Large");
//                    ssd1306_show(&disp);
                    display_size_select(&disp, cocktailState.size);
                }

                break;
            case MIXTURE_SELECT:
                switch (cocktailState.inputEvent) {
                    case CW_ROTATION:
                        cocktailState.mixture[0] = clamp(cocktailState.mixture[0] + 1, 0, 100);
                        cocktailState.mixture[1] = clamp(cocktailState.mixture[1] - 1, 0, 100);
                        break;
                    case CCW_ROTATION:
                        cocktailState.mixture[0] = clamp(cocktailState.mixture[0] - 1, 0, 100);
                        cocktailState.mixture[1] = clamp(cocktailState.mixture[1] + 1, 0, 100);
                        break;
                    case PUSH:
                        // confirm mixture and change cocktailState
//                        set_menu_state(&cocktailState, AUTHENTICATION, &disp);
                        set_menu_state(&cocktailState, POURING, &disp);
                        cocktailState.inputEvent = -1;
                }

                if (!cocktailState.needsUpdate) {
                    // display percentage determined by cocktailState.mixture
//                    ssd1306_clear(&disp);
//                    ssd1306_draw_string_with_font(&disp, 8, 8, 1, acme_font, "Mixture");
                    update_mixture(&disp, cocktailState.mixture);
                    update_mixture_dotted(&disp, (uint8_t) (cocktailState.mixture[0] * 0.4));
//                    draw_number(&disp, 8, 24, 1, cocktailState.mixture[0]);
                    // ssd1306
//                    draw_number(&disp, 8, 48, 1, cocktailState.mixture[1]);
                    ssd1306_show(&disp);
                }

                break;
            case AUTHENTICATION:
                display_auth(&disp);


                char body[80];
                sprintf(body, "drink=%d&size=%d&liquid1=%d&liquid2=%d", cocktailState.selectedDrink,
                        cocktailState.size, cocktailState.mixture[0],
                        cocktailState.mixture[1]);
                NetworkData data;
                data.event = NETWORK_EVENT_AUTHENTICATE;
                data.data = body;
                data.dataLength = strlen(body);
//                xTaskNotifyGive(globalStruct.networkTaskHandle);
                xQueueSendToBack(globalStruct.networkQueue, &data, portMAX_DELAY);

                bool auth = false;
                xQueueReceive(globalStruct.authenticationQueue, &auth, portMAX_DELAY);

                printf("auth: %d\n", auth);

                if (!auth) {
                    set_menu_state(&cocktailState, ERROR, &disp);
                } else {
                    set_menu_state(&cocktailState, POURING, &disp);
                }
                break;
            case ERROR:
                display_error(&disp);

                vTaskDelay(3000);
                set_menu_state(&cocktailState, DRINK_SELECT, &disp);
                break;
            case POURING:
                printf("POURING\n");
//                uint8_t sizePrescaler = cocktailState.size == SMALL ? 1 : 2;
                TickType_t pouringTicksToWaitBase =
                        cocktailState.size == SMALL ? TIME_TO_POUR_SMALL : TIME_TO_POUR_LARGE;
                TickType_t pouringTicksToWait1 = pouringTicksToWaitBase * cocktailState.mixture[0] * cocktailState.progress / 10;
                TickType_t pouringTicksToWait2 = pouringTicksToWaitBase * cocktailState.mixture[1]  * cocktailState.progress / 10;

                TickType_t progressInterval =
                        pouringTicksToWait1 > pouringTicksToWait2 ? pouringTicksToWait1 : pouringTicksToWait2;
                progressInterval /= (cocktailState.progress);

                // Notify ToF_Task to start measuring and check distance once
                xTaskNotifyGive(globalStruct.tofTaskHandle);

                xQueueReceive(globalStruct.tofQueue, &distance, portMAX_DELAY);
                if (distance > MAX_CUP_DISTANCE) {
                    set_menu_state(&cocktailState, NO_CUP, &disp);
                }

                // start pouring
                if (!cocktailState.needsUpdate) {

                    ssd1306_clear(&disp);

                    switch (cocktailState.selectedDrink) {
                        case DRINK_1:
                            pumpData.command = PUMP_ON;
                            pumpData.pumpNumber = PUMP_1;
                            pumpData.timeToPour = pouringTicksToWait1;
                            xQueueSendToBack(globalStruct.pumpControllerQueue, &pumpData, portMAX_DELAY);

                            pumpData.command = PUMP_ON;
                            pumpData.pumpNumber = PUMP_2;
                            pumpData.timeToPour = pouringTicksToWait2;
                            xQueueSendToBack(globalStruct.pumpControllerQueue, &pumpData, portMAX_DELAY);
                            break;
                        case DRINK_2:
                            pumpData.command = PUMP_ON;
                            pumpData.pumpNumber = PUMP_1;
                            pumpData.timeToPour = pouringTicksToWait1;
                            xQueueSendToBack(globalStruct.pumpControllerQueue, &pumpData, portMAX_DELAY);

                            pumpData.command = PUMP_ON;
                            pumpData.pumpNumber = PUMP_3;
                            pumpData.timeToPour = pouringTicksToWait2;
                            xQueueSendToBack(globalStruct.pumpControllerQueue, &pumpData, portMAX_DELAY);
                            break;
                        case DRINK_3:
                            pumpData.command = PUMP_ON;
                            pumpData.pumpNumber = PUMP_2;
                            pumpData.timeToPour = pouringTicksToWait1;
                            xQueueSendToBack(globalStruct.pumpControllerQueue, &pumpData, portMAX_DELAY);

                            pumpData.command = PUMP_ON;
                            pumpData.pumpNumber = PUMP_3;
                            pumpData.timeToPour = pouringTicksToWait2;
                            xQueueSendToBack(globalStruct.pumpControllerQueue, &pumpData, portMAX_DELAY);
                            break;
                    }

                    while (cocktailState.progress > 0) {
                        // Notify ToF_Task to start measuring

                        xTaskNotifyGive(globalStruct.tofTaskHandle);
                        xQueueReceive(globalStruct.tofQueue, &distance, portMAX_DELAY);
                        if (distance > MAX_CUP_DISTANCE) {
                            pumpData.command = PUMP_OFF;
                            xQueueSendToBack(globalStruct.pumpControllerQueue, &pumpData, portMAX_DELAY);
                            set_menu_state(&cocktailState, NO_CUP, &disp);
                            break;
                        } else {
                            ssd1306_clear(&disp);
                            ssd1306_draw_string_with_font(&disp, 8, 24, 1, acme_font, "Pouring");
                            draw_number(&disp, 8, 48, 1, cocktailState.progress);
                            ssd1306_show(&disp);
//                            vTaskDelay(DISTANCE_MEASURE_DELAY);
                            cocktailState.progress--;
                        }

                        vTaskDelay(progressInterval);
                    }

                    if (cocktailState.progress == 0) {
//                        vTaskDelay(1000);
                        set_menu_state(&cocktailState, DONE, &disp);
                    }
                }
                break;

            case NO_CUP:
                printf("NO_CUP\n");

                display_nocup(&disp);

                for (int i = 0; i < 20; ++i) {
                    // Notify ToF_Task to start measuring
                    xTaskNotifyGive(globalStruct.tofTaskHandle);

                    xQueueReceive(globalStruct.tofQueue, &distance, portMAX_DELAY);
                    if (distance <= MAX_CUP_DISTANCE) {
                        set_menu_state(&cocktailState, POURING, &disp);
                    } else {
                        vTaskDelay(DISTANCE_MEASURE_DELAY);
                    }
                }

                if (!cocktailState.needsUpdate) {
                    set_menu_state(&cocktailState, ERROR, &disp);
                }

                break;
            case DONE:
                // Activate buzzer melody
                buzzerEvent = MELODY_1;
                xQueueSendToBack(globalStruct.buzzerQueue, &buzzerEvent, portMAX_DELAY);

                display_done(&disp);

                vTaskDelay(5000);
                set_menu_state(&cocktailState, DRINK_SELECT, &disp);
                break;

            case IDLE:
                // TODO: Play animation
                idle_counter++;
                display_idle(&disp, idle_counter);
//                ssd1306_clear(&disp);
//                ssd1306_draw_string_with_font(&disp, 8, 24, 1, acme_font, "Idle");
//                draw_number(&disp, 8, 48, 1, idle_counter);
//                ssd1306_show(&disp);
                cocktailState.needsUpdate = false;


                break;
        }
    }
}

void set_menu_state(CocktailState *cocktailState, MenuState new, ssd1306_t *disp) {
    switch (new) {
        case DRINK_SELECT:
            printf("DRINK_SELECT\n");
            cocktailState->inputEvent = -1;
            cocktailState->needsUpdate = true;
            cocktailState->selectedDrink = DRINK_1;
            cocktailState->size = SMALL;
            cocktailState->mixture[0] = 50;
            cocktailState->mixture[1] = 50;
            cocktailState->auth = false;
            cocktailState->menuState = DRINK_SELECT;
            break;
        case SIZE_SELECT:
            printf("SIZE_SELECT\n");
            cocktailState->menuState = SIZE_SELECT;
            break;
        case MIXTURE_SELECT:
            printf("MIXTURE_SELECT\n");
            display_mixture(disp, &cocktailState->mixture[0]);
            cocktailState->menuState = MIXTURE_SELECT;
            break;
        case AUTHENTICATION:
            printf("AUTHENTICATION\n");
            cocktailState->menuState = AUTHENTICATION;
            break;
        case ERROR:
            printf("ERROR\n");
            cocktailState->menuState = ERROR;
            break;
        case POURING:
            printf("POURING\n");
            if (cocktailState->menuState != NO_CUP) {
                cocktailState->progress = 10;
            }
            cocktailState->menuState = POURING;
            break;
        case NO_CUP:
            printf("NO_CUP\n");
            cocktailState->menuState = NO_CUP;
            break;
        case DONE:
            printf("DONE\n");
            cocktailState->menuState = DONE;
            break;
        case IDLE:
//            printf("IDLE\n");
            cocktailState->menuState = IDLE;
            break;


    }
    cocktailState->needsUpdate = true;
}
