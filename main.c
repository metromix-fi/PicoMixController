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

#include "networking/httpclient.h"

#include "display/ssd1306.h"
#include "display/image.h"
#include "display/acme_5_outlines_font.h"
#include "display/bubblesstandard_font.h"
#include "display/crackers_font.h"
#include "display/BMSPA_font.h"

const uint8_t num_chars_per_disp[] = {7, 7, 7, 5};
const uint8_t *fonts[4] = {acme_font, bubblesstandard_font, crackers_font, BMSPA_font};

#define SLEEPTIME 25

void setup_gpios(void);

_Noreturn void animationTask(void *param);

int main() {
    stdio_init_all();

    printf("configuring pins...\n");
    setup_gpios();

    printf("jumping to animationTask...\n");

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

    // Display
    TaskHandle_t animation_task_handle = NULL;
    xTaskCreate(animationTask,
                "animationTask",
                1024,
                NULL,
                tskIDLE_PRIORITY + 1,
                &animation_task_handle
                );

    vTaskStartScheduler();

    return 0;
}


void setup_gpios(void) {
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

    for (;;) {
        for (int y = 0; y < 31; ++y) {
            ssd1306_draw_line(&disp, 0, y, 127, y);
            ssd1306_show(&disp);
            vTaskDelay(SLEEPTIME);
            ssd1306_clear(&disp);
        }

        for (int y = 0, i = 1; y >= 0; y += i) {
            ssd1306_draw_line(&disp, 0, 31 - y, 127, 31 + y);
            ssd1306_draw_line(&disp, 0, 31 + y, 127, 31 - y);
            ssd1306_show(&disp);
            vTaskDelay(SLEEPTIME);
            ssd1306_clear(&disp);
            if (y == 32) i = -1;
        }

        for (int i = 0; i < sizeof(words) / sizeof(char *); ++i) {
            ssd1306_draw_string(&disp, 8, 24, 2, words[i]);
            ssd1306_show(&disp);
            vTaskDelay(800);
            ssd1306_clear(&disp);
        }

        for (int y = 31; y < 63; ++y) {
            ssd1306_draw_line(&disp, 0, y, 127, y);
            ssd1306_show(&disp);
            vTaskDelay(SLEEPTIME);
            ssd1306_clear(&disp);
        }

        for (size_t font_i = 0; font_i < sizeof(fonts) / sizeof(fonts[0]); ++font_i) {
            uint8_t c = 32;
            while (c <= 126) {
                uint8_t i = 0;
                for (; i < num_chars_per_disp[font_i]; ++i) {
                    if (c > 126)
                        break;
                    buf[i] = c++;
                }
                buf[i] = 0;

                ssd1306_draw_string_with_font(&disp, 8, 24, 2, fonts[font_i], buf);
                ssd1306_show(&disp);
                vTaskDelay(800);
                ssd1306_clear(&disp);
            }
        }

        ssd1306_bmp_show_image(&disp, image_data, image_size);
        ssd1306_show(&disp);
        printf("finished animation loop\n");
        vTaskDelay(2000);
    }
}
