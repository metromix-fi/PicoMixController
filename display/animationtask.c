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


#define SLEEPTIME 25


const uint8_t num_chars_per_disp[] = {7, 7, 7, 5};
const uint8_t *fonts[4] = {acme_font, bubblesstandard_font, crackers_font, BMSPA_font};

static void displayNumber(ssd1306_t *disp, int value) {
    // Display the current number on the OLED screen
    char buf[8];
    snprintf(buf, sizeof(buf), "%d", value);
    ssd1306_clear(disp);
    ssd1306_draw_string(disp, 8, 24, 2, buf);
    ssd1306_show(disp);
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

_Noreturn void animationTask(void *param) {
    const char *words[] = {"SSD1306", "DISPLAY", "DRIVER"};

    ssd1306_t disp;
    disp.external_vcc = false;
    ssd1306_init(&disp, 128, 64, 0x3C, i2c1);
    ssd1306_clear(&disp);

    printf("ANIMATION!\n");

    char buf[8];

    int rotary_input = 0;
    uint8_t state = 0;
    for (;;) {
        state = state + rotary_input;
//        uint8_t i = (state / 8) % 3;
        displayNumber(&disp, state);
//        display_img(&disp);
//        ssd1306_draw_string(&disp, 8, 24, 2, words[i]);
        ssd1306_show(&disp);
//        vTaskDelay(800);
//        ssd1306_clear(&disp);W

        xQueueReceive(globalStruct.rotaryEncoderQueue, &rotary_input, portMAX_DELAY);
    }
}
