#include <sys/cdefs.h>
//
// Created by jesse on 06.11.23.
//
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "httpclient.h"
#include "utils/GlobalState.h"
#include "FreeRTOS.h"
#include "queue.h"
#include "rfid/mfrc522.h"

#define TLS_CLIENT_SERVER        "192.168.211.226"

#define TLS_CLIENT_TIMEOUT_SECS  15

extern bool
run_tls_client_test(const uint8_t *cert, size_t cert_len, const char *server, const char *request, int timeout);

int network_setup() {
    if (cyw43_arch_init()) {
        printf("failed to initialise\n");
        return 1;
    }
    cyw43_arch_enable_sta_mode();
}

_Noreturn void networkTask(void *param) {
    printf("networkTask\n");

    char request[] = "POST /api HTTP/1.1\r\n" \
                                 "Host: " TLS_CLIENT_SERVER"\r\n" \
                                 "Content-Type: text/plain\r\n" \
                                 "Content-Length: 8\r\n" \
                                 "Connection: close\r\n" \
                                 "\r\n";

    for (;;) {

//        if (cyw43_arch_init()) {
//            printf("failed to initialise\n");
//            vTaskDelay(1000);
//            continue;
//        }
//        cyw43_arch_enable_sta_mode();

        if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 30000)) {
            printf("failed to connect\n");
            printf("SSID: " WIFI_SSID);
            printf("PW: " WIFI_PASSWORD);

            cyw43_arch_deinit();
            vTaskDelay(1000);
            continue;
        }

        MifareUID rfid_state;
        size_t request_len = strlen(request);

        for (;;) {
//            xQueueReceive(globalStruct.rfidQueue, &rfid_state, portMAX_DELAY);
            ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
            printf("network task\n");

            bool auth = false;
            for (int i = 0; i < 10; ++i) {
                // Notify RFID_Task to start measuring
                xTaskNotifyGive(globalStruct.rfidTaskHandle);

                if (xQueueReceive(globalStruct.rfidQueue, &rfid_state, portMAX_DELAY)) {
                    if (rfid_state.size == 0) {
                        vTaskDelay(1000);
                        continue;
                    } else {
                        printf("Success!\n");
                        printf("rfid_state: %x, %06X\n", rfid_state.size, rfid_state.bytes);
                        break;
                    }
                }
            }
            if (rfid_state.size == 0) {
                printf("No RFID tag found\n");
                xQueueSendToBack(globalStruct.authenticationQueue, &auth, portMAX_DELAY);
                continue;
            }

            size_t new_size = request_len + rfid_state.size + 1;
            char *new_request = pvPortMalloc(new_size);

            if (new_request == NULL) {
                printf("Failed to allocate memory for request\n");
                continue;
            }

            strcpy(new_request, request);

            // Append each byte as hex
            for (int i = 0; i < rfid_state.size; i++) {
                sprintf(new_request + strlen(request) + i * 2, "%02X", rfid_state.bytes[i]);
            }

            // Null terminate the string
//            new_request[new_size - 1] = '\0';

            printf("New Request: \n\r%s", new_request);

            printf("Connecting to server: " TLS_CLIENT_SERVER);
            bool pass = run_tls_client_test(NULL, 0, TLS_CLIENT_SERVER, new_request,
                                            TLS_CLIENT_TIMEOUT_SECS);
            if (pass) {
                auth = true;
                printf("Test passed\n");
            } else {
                printf("Test failed\n");
            }

            xQueueSendToBack(globalStruct.authenticationQueue, &auth, portMAX_DELAY);
//            free(new_request);
            vPortFree(new_request);
        }
    }
}