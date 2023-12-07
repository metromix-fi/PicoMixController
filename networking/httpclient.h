#include <sys/cdefs.h>
//
// Created by jesse on 06.11.23.
//

#ifndef METROMIX_HTTPCLIENT_H
#define METROMIX_HTTPCLIENT_H
#include "FreeRTOS.h"
#include "task.h"

typedef enum NetworkEvent {
    NETWORK_EVENT_AUTHENTICATE,
} NetworkEvent;

typedef struct NetworkData {
    NetworkEvent event;
    char *data;
    uint32_t dataLength;
} NetworkData;

int network_setup();

_Noreturn void networkTask(void *param);

#endif //METROMIX_HTTPCLIENT_H
