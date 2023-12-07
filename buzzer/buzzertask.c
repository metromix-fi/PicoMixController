//
// Created by jesse on 07.12.23.
//

#include "buzzertask.h"
#include <pico/printf.h>
#include <hardware/gpio.h>
#include <hardware/pwm.h>
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "timers.h"
#include "utils/GlobalState.h"

const int notes[] = {
        31,
        33,
        35,
        37,
        39,
        41,
        44,
        46,
        49,
        52,
        55,
        58,
        62,
        65,
        69,
        73,
        78,
        82,
        87,
        93,
        98,
        104,
        110,
        117,
        123,
        131,
        139,
        147,
        156,
        165,
        175,
        185,
        196,
        208,
        220,
        233,
        247,
        262,
        277,
        294,
        311,
        330,
        349,
        370,
        392,
        415,
        440,
        466,
        494,
        523,
        554,
        587,
        622,
        659,
        698,
        740,
        784,
        831,
        880,
        932,
        988,
        1047,
        1109,
        1175,
        1245,
        1319,
        1397,
        1480,
        1568,
        1661,
        1760,
        1865,
        1976,
        2093,
        2217,
        2349,
        2489,
        2637,
        2794,
        2960,
        3136,
        3322,
        3520,
        3729,
        3951,
        4186,
        4435,
        4699,
        4978
};

void setup_buzzer_gpios(void) {
    gpio_init(BUZZER_PIN);
    gpio_set_function(BUZZER_PIN, GPIO_FUNC_PWM);
//    gpio_set_dir(BUZZER_PIN, GPIO_OUT);
}

// Function to set the frequency of the PWM signal
void set_frequency(uint gpio, uint frequency) {
    uint slice_num = pwm_gpio_to_slice_num(gpio);
    uint clock_div = 1250; // Clock divider to adjust frequency range

    // Calculate the wrap value based on the desired frequency
    uint wrap = (125000000 / (clock_div * frequency)) - 1;

    pwm_set_clkdiv(slice_num, clock_div);
    pwm_set_wrap(slice_num, wrap);
    pwm_set_gpio_level(gpio, wrap / 2); // 50% duty cycle
    pwm_set_enabled(slice_num, true);
}

// Function to play a note for a certain duration
void play_note(uint frequency, TickType_t duration) {
    set_frequency(BUZZER_PIN, frequency);
    vTaskDelay(duration / portTICK_PERIOD_MS);
    pwm_set_enabled(pwm_gpio_to_slice_num(BUZZER_PIN), false);
}

_Noreturn void buzzerTask(void *pvParameters) {
    setup_buzzer_gpios();
    vTaskDelay(500);
    printf("buzzerTask started\n");

    BuzzerEvent buzzerEvent;

    int melody1[] = {C3,E3,G3,B3,C4,E4,G4,G4,E4,P,
            C3,E3,G3,B3,C4,E4,A4,A4,E4,P,
            D3,F3,AS3,D4,F4,B4,B4,C5,C5,P,C5,C5}
    ;

    while (true) {
        xQueueReceive(globalStruct.buzzerQueue, &buzzerEvent, portMAX_DELAY);

        switch (buzzerEvent) {
            case MELODY_1:
                for (int i = 0; i < sizeof(melody1); i++) {
                    if (melody1[i] != P) {
                        play_note(melody1[i -1], 125);
                    } else {
                        vTaskDelay(125 / portTICK_PERIOD_MS);
                    }
                }
                break;
        }
    }
}