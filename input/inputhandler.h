#include <sys/cdefs.h>
//
// Created by jesse on 06.11.23.
//

#ifndef METROMIX_INPUTHANDLER_H
#define METROMIX_INPUTHANDLER_H

void setup_input_gpios(void);

_Noreturn void input_handler_task(void *pvParameters);

typedef enum InputEvent {
    CW_ROTATION = 0,
    CCW_ROTATION,
    PUSH,
} InputEvent;

#endif //METROMIX_INPUTHANDLER_H
