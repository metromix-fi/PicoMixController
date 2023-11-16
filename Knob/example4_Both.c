#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/gpio.h"
#include "ssd1306.h"

#define SLEEPTIME 10

void setup_gpios(void);
void displayNumber(ssd1306_t *disp, int value);
void handleEncoderA(uint gpio, uint32_t events, uint32_t *lastTick);
//void handleEncoderB(uint gpio, uint32_t events, uint32_t *lastTick);

// Define GPIO pins for the rotary encoder
#define ROTARY_A 10
#define ROTARY_B 11
#define ROTARY_PUSH 12

// Variables to keep track of the displayed number
int displayedNumber = 1;

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
        // Display the current number on the OLED screen
        displayNumber(&disp, displayedNumber);
        sleep_ms(SLEEPTIME);
    }

    return 0;
}

void displayNumber(ssd1306_t *disp, int value) {
    // Display the current number on the OLED screen
    char buf[8];
    snprintf(buf, sizeof(buf), "%d", value);
    ssd1306_clear(disp);
    ssd1306_draw_string(disp, 8, 24, 2, buf);
    ssd1306_show(disp);
}

void handleEncoderA(uint gpio, uint32_t events, uint32_t *lastTick) {
    if (events & GPIO_IRQ_EDGE_RISE) {
        if (gpio_get(ROTARY_B) == 0) {
            // Clockwise rotation
            displayedNumber++;
            printf("Clockwise Rotation. Value: %d\n", displayedNumber);
        } else {
            // Counterclockwise rotation
            displayedNumber--;
            printf("Counterclockwise Rotation. Value: %d\n", displayedNumber);
        }
    } else if (events & GPIO_IRQ_EDGE_FALL) {
        *lastTick = time_us_32();
    }
}

