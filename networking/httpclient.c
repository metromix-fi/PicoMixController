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

#define TLS_CLIENT_SERVER        "192.168.162.226"
#define CONTENT_LENGTH           "44"

#define TLS_CLIENT_TIMEOUT_SECS  25

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
                                 "Content-Length: " CONTENT_LENGTH"\r\n" \
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
//        size_t request_len = strlen(request);
        size_t body_len;
        NetworkData networkData;
        bool auth;
        uint request_len;

        for (;;) {
            body_len = 56;
            auth = false;
            request_len = strlen(request);

            xQueueReceive(globalStruct.networkQueue, &networkData, portMAX_DELAY);
//            ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
            printf("network task\n");

            printf("NetworkData: %lu, %s\n", networkData.dataLength, networkData.data);

//            body_len += networkData.dataLength;

            if (networkData.event == NETWORK_EVENT_AUTHENTICATE) {
                printf("NETWORK_EVENT_AUTHENTICATE\n");

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
            }

//            char request[128];
//            snprintf(request, 128,
//                     "POST /api HTTP/1.1\r\n"
//                     "Host: %zu\r\n"
//                     "Content-Type: text/plain\r\n"
//                     "Content-Length: %d\r\n"
//                     "Connection: close\r\n"
//                     "\r\n"
//                     "%s"
//                     TLS_CLIENT_SERVER, body_len + 14, *networkData.data);
//            uint request_len = strlen(request);
//            body_len += rfid_state.size + 1;
            size_t new_size = request_len + body_len + 12;
            char *new_request = pvPortMalloc(new_size);
//            char *new_request = pvPortMalloc(256);
            if (new_request == NULL) {
                printf("Failed to allocate memory for request\n");
                continue;
            }

//            printf("New Request1: \n\r%s", new_request);
            // Copy the old request
            strcpy(new_request, request);

            // append networkData.data to new_request
            strcat(new_request, networkData.data);

            // append rfid to new_request
            strcat(new_request, "&rfid=");
//            // Append each byte of rfid as hex
            for (int i = 0; i < rfid_state.size; i++) {
                sprintf(new_request + strlen(new_request) + i * 2, "%02X", rfid_state.bytes[i]);
            }

            // Null terminate the string
//            new_request[new_size - 1] = '\0';

            printf("New Request2: \n\r%s", new_request);
            printf("SIZE: \n\r%zu", new_size - request_len);

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
            vPortFree(new_request);
        }
    }
}