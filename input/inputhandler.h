#include <sys/cdefs.h>
//
// Created by jesse on 06.11.23.
//

#ifndef METROMIX_INPUTHANDLER_H
#define METROMIX_INPUTHANDLER_H

void setup_input_gpios(void);

_Noreturn void input_handler_task(void *pvParameters);

#endif //METROMIX_INPUTHANDLER_H
