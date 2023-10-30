#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/gpio.h"
#include "ssd1306.h"

#define SLEEPTIME 10
#define PUSH_TIME_MS 2000

void setup_gpios(void);
void displayNumber(ssd1306_t *disp, int value);

// Define GPIO pins for the rotary encoder and push button
#define ROTARY_A 10
#define ROTARY_B 11
#define ROTARY_PUSH 12

// Variables to keep track of the displayed number and button state
int displayedNumber = 1;
bool isPushed = false;
uint32_t pushStartTime = 0;

int main() {
    stdio_init_all();

    setup_gpios();

    // Initialize the OLED display
    i2c_init(i2c1, 400000);
    gpio_set_function(14, GPIO_FUNC_I2C);
    gpio_set_function(15, GPIO_FUNC_I2C);
    gpio_pull_up(14);
    gpio_pull_up(15);

    ssd1306_t disp;
    disp.external_vcc = false;
    ssd1306_init(&disp, 128, 64, 0x3C, i2c1);
    ssd1306_clear(&disp);

    while (1) {
        if (isPushed) {
            // Display "PUSHING" for 2 seconds
            uint32_t currentTime = time_us_32();
            if (currentTime - pushStartTime < PUSH_TIME_MS * 1000) {
                ssd1306_clear(&disp);
                ssd1306_draw_string(&disp, 8, 24, 2, "PUSHING");
                ssd1306_show(&disp);
            } else {
                isPushed = false;
                displayedNumber = 1;
            }
        } else {
            // Display the current number on the OLED screen
            displayNumber(&disp, displayedNumber);
            sleep_ms(SLEEPTIME);
        }
    }

    return 0;
}

void setup_gpios(void) {
    // Configure GPIO pins for the rotary encoder and push button as inputs
    gpio_init(ROTARY_A);
    gpio_set_dir(ROTARY_A, GPIO_IN);
    gpio_pull_up(ROTARY_A);

    gpio_init(ROTARY_B);
    gpio_set_dir(ROTARY_B, GPIO_IN);
    gpio_pull_up(ROTARY_B);

    gpio_init(ROTARY_PUSH);
    gpio_set_dir(ROTARY_PUSH, GPIO_IN);
    gpio_pull_up(ROTARY_PUSH);
}

void displayNumber(ssd1306_t *disp, int value) {
    // Display the current number on the OLED screen
    char buf[8];
    snprintf(buf, sizeof(buf), "%d", value);
    ssd1306_clear(disp);
    ssd1306_draw_string(disp, 8, 24, 2, buf);
    ssd1306_show(disp);
}
