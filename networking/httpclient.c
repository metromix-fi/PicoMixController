#include <sys/cdefs.h>
//
// Created by jesse on 06.11.23.
//
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "httpclient.h"

#define TLS_CLIENT_SERVER        "192.168.122.226"
#define TLS_CLIENT_HTTP_REQUEST  "GET /api HTTP/1.1\r\n" \
                                 "Host: " TLS_CLIENT_SERVER"\r\n" \
                                 "Connection: close\r\n" \
                                 "\r\n"
#define TLS_CLIENT_TIMEOUT_SECS  15

extern bool run_tls_client_test(const uint8_t *cert, size_t cert_len, const char *server, const char *request, int timeout);

_Noreturn void networkTask(void *param) {
    printf("networkTask\n");

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
        for (;;) {
            printf("Connecting to server: " TLS_CLIENT_SERVER);
            bool pass = run_tls_client_test(NULL, 0, TLS_CLIENT_SERVER, TLS_CLIENT_HTTP_REQUEST,
                                            TLS_CLIENT_TIMEOUT_SECS);
            if (pass) {
                printf("Test passed\n");
            } else {
                printf("Test failed\n");
            }
            /* sleep a bit to let usb stdio write out any buffer to host */
            vTaskDelay(1000);
        }
    }
    cyw43_arch_deinit();
    printf("All done\n");
    return;

}