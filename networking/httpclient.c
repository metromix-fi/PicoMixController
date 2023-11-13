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

#define TLS_CLIENT_SERVER        "192.168.150.226"

#define TLS_CLIENT_TIMEOUT_SECS  15

extern bool run_tls_client_test(const uint8_t *cert, size_t cert_len, const char *server, const char *request, int timeout);

_Noreturn void networkTask(void *param) {
    printf("networkTask\n");

    char request[] = "POST /api HTTP/1.1\r\n" \
                                 "Host: " TLS_CLIENT_SERVER"\r\n" \
                                 "Content-Type: text/plain\r\n" \
                                 "Content-Length: 2\r\n" \
                                 "Connection: close\r\n" \
                                 "\r\n";

    for(;;) {

        if (cyw43_arch_init()) {
            printf("failed to initialise\n");
            vTaskDelay(1000);
            continue;
        }
        cyw43_arch_enable_sta_mode();

        if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 30000)) {
            printf("failed to connect\n");
            printf("SSID: " WIFI_SSID);
            printf("PW: " WIFI_PASSWORD);

            cyw43_arch_deinit();
            vTaskDelay(1000);
            continue;
        }

        char rfid_state[2] = "00";

        for (;;) {
            xQueueReceive(globalStruct.rfidQueue, &rfid_state, portMAX_DELAY);

            printf("Connecting to server: " TLS_CLIENT_SERVER);
            bool pass = run_tls_client_test(NULL, 0, TLS_CLIENT_SERVER, strcat(request, rfid_state),
                                            TLS_CLIENT_TIMEOUT_SECS);
            if (pass) {
                printf("Test passed\n");
            } else {
                printf("Test failed\n");
            }
            /* sleep a bit to let usb stdio write out any buffer to host */
            vTaskDelay(10000);
        }
    }
}