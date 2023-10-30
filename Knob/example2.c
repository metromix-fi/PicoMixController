#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "ssd1306.h"

#define SLEEPTIME 10

void setup_gpios(void);
void displayNumber(ssd1306_t *disp, int value);

// Define GPIO pins for the rotary encoder
#define ROTARY_A 10
#define ROTARY_B 11

// Variables to keep track of the displayed number and encoder state
int displayedNumber = 1;
int encoderState = 0;
int lastEncoderState = 0;

int main() {
    stdio_init_all();

    //printf("configuring pins...\n");
    setup_gpios();

    //printf("jumping to display...\n");

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
    }

    return 0;
}

void setup_gpios(void) {
    // Configure GPIO pins for the rotary encoder as inputs
    gpio_init(ROTARY_A);
    gpio_set_dir(ROTARY_A, GPIO_IN);
    gpio_disable_pulls(ROTARY_A);

    gpio_init(ROTARY_B);
    gpio_set_dir(ROTARY_B, GPIO_IN);
    gpio_disable_pulls(ROTARY_B);
}

void displayNumber(ssd1306_t *disp, int value) {
    // Display the current number on the OLED screen
    char buf[8];
    snprintf(buf, sizeof(buf), "%d", value);
    ssd1306_clear(disp);
    ssd1306_draw_string(disp, 8, 24, 2, buf);
    ssd1306_show(disp);

    // Update the displayed number based on the encoder state
    int stateA = gpio_get(ROTARY_A);
    int stateB = gpio_get(ROTARY_B);

    if (stateA != lastEncoderState) {
        if (stateB != stateA) {
            // Clockwise rotation
            displayedNumber++;
        } else {
            // Counterclockwise rotation
            displayedNumber--;
        }
    }

    lastEncoderState = stateA;

    sleep_ms(SLEEPTIME);
}
