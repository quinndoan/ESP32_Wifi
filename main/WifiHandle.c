#include "library.h"
#include "WifiHandle.h"
#include "AP.h"
#include "Sta.h"

extern const char *TAG; // Sử dụng một TAG chung cho ESP
int s_retry_num = 0; // Khởi tạo số lần thử kết nối lại

EventGroupHandle_t s_wifi_event_group;

void wifi_event_handler(void* arg, esp_event_base_t event_base,
                        int32_t event_id, void* event_data) {
    if (event_base == WIFI_EVENT) {
        if (event_id == WIFI_EVENT_AP_STACONNECTED) {
            wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
            ESP_LOGI(TAG, "station "MACSTR" join, AID=%d",
                     MAC2STR(event->mac), event->aid);
        } else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
            wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
            ESP_LOGI(TAG, "station "MACSTR" leave, AID=%d, reason=%d",
                     MAC2STR(event->mac), event->aid, event->reason);
        } else if (event_id == WIFI_EVENT_STA_START) {
            // Khi STA bắt đầu, cố gắng kết nối Wi-Fi
            ESP_LOGI(TAG, "Wi-Fi STA started, attempting to connect...");
            esp_wifi_connect();
        } else if (event_id == WIFI_EVENT_STA_DISCONNECTED) {
            if (s_retry_num < EXAMPLE_ESP_MAXIMUM_RETRY) {
                esp_wifi_connect();
                s_retry_num++;
                ESP_LOGI(TAG, "Retrying to connect to the AP, attempt #%d", s_retry_num);
            } else {
                ESP_LOGE(TAG, "Failed to connect to the AP, maximum retries reached");

                // Thay đổi WiFi mode trong NVS sang AP mode
                nvs_handle_t nvs_handle;
                esp_err_t err = nvs_open("storage", NVS_READWRITE, &nvs_handle);
                if (err == ESP_OK) {
                    int32_t wifi_mode = 1; // Chuyển sang chế độ AP
                    err = nvs_set_i32(nvs_handle, "wifi_mode", wifi_mode);
                    if (err == ESP_OK) {
                        ESP_LOGI(TAG, "WiFi mode set to AP mode in NVS");
                    } else {
                        ESP_LOGE(TAG, "Failed to set WiFi mode in NVS: %s", esp_err_to_name(err));
                    }
                    nvs_commit(nvs_handle);
                    nvs_close(nvs_handle);
                } else {
                    ESP_LOGE(TAG, "Failed to open NVS: %s", esp_err_to_name(err));
                }
                // Chuyển sang chế độ AP
                ESP_LOGI(TAG, "Switching to AP mode...");
                wifi_init_softap(); // Bắt đầu AP
                start_webserver(); // Hàm khởi động AP mode và Web Server
                //xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT); // Đánh dấu kết nối thất bại
            }
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        // Khi STA nhận được IP từ AP
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "Got IP: " IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0; // Reset lại số lần thử
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT); // Đánh dấu kết nối thành công
         for (int i = 0; i < 10000; i++) {
        vTaskDelay(pdMS_TO_TICKS(10)); // Đợi khoảng 10 ms x 1000 lần ~ 10 giây
    }

    // Dừng Wi-Fi hiện tại
    ESP_ERROR_CHECK(esp_wifi_stop());

    // Chuyển wifi_mode về 1 (chế độ AP)
    nvs_handle_t nvs_handle;
    ESP_ERROR_CHECK(nvs_open("storage", NVS_READWRITE, &nvs_handle));
    wifi_mode = 1;
    ESP_ERROR_CHECK(nvs_set_i32(nvs_handle, "wifi_mode", wifi_mode));
    ESP_ERROR_CHECK(nvs_commit(nvs_handle));
    nvs_close(nvs_handle);
    ESP_LOGI(TAG, "Switched to AP mode after delay.");

    // Khởi động lại Wi-Fi ở chế độ AP
     wifi_init_softap(); // Bắt đầu AP
    start_webserver();
    }
}