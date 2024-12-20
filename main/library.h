#ifndef LIBRARY_H_
#define LIBRARY_H_
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_mac.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_system.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "esp_http_server.h"
#include "driver/gpio.h"
#include "nvs.h"


#define EXAMPLE_ESP_WIFI_SSID      CONFIG_ESP_WIFI_SSID
#define EXAMPLE_ESP_WIFI_PASS      CONFIG_ESP_WIFI_PASSWORD
#define EXAMPLE_ESP_WIFI_CHANNEL   CONFIG_ESP_WIFI_CHANNEL
#define EXAMPLE_MAX_STA_CONN       CONFIG_ESP_MAX_STA_CONN
#define MIN(x,y) ((x) <(y) ? (x) :(y))   // thêm định nghĩa MIN
#define EXAMPLE_ESP_MAXIMUM_RETRY  10   // Maximum retry count

#define GPIO_BUTTON_PIN 18  // thêm define cho chân GPIO dùng cho Button

static const char *TAG = "WiFi_Mode_Switch";
extern int32_t wifi_mode; // 0: STA mode, 1: AP mode

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

#define UART_NUM UART_NUM_1
#define BUF_SIZE (1024)

#endif