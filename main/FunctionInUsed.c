// #include"library.h"
// #include"FunctionInUsed.h"

// extern const char *TAG; // sử dụng một TAG cho ESP để có thể chuyển đổi giữa hai mode
// int s_retry_num = 5;

// EventGroupHandle_t s_wifi_event_group;
// // Cập nhật hàm wifi_event_handler để xử lý cả hai chế độ AP và STA
// void wifi_event_handler(void* arg, esp_event_base_t event_base,
//                                     int32_t event_id, void* event_data) {
//     if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STACONNECTED) {
//         wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
//         ESP_LOGI(TAG, "station "MACSTR" join, AID=%d",
//                  MAC2STR(event->mac), event->aid);
//     } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STADISCONNECTED) {
//         wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
//         ESP_LOGI(TAG, "station "MACSTR" leave, AID=%d, reason=%d",
//                  MAC2STR(event->mac), event->aid, event->reason);
//     } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START){
//         esp_wifi_connect();
//     } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED){
//         if (s_retry_num < EXAMPLE_ESP_MAXIMUM_RETRY) {
//             esp_wifi_connect();
//             s_retry_num++;
//             ESP_LOGI(TAG, "retry to connect to the AP");
//         } else {
//             xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
//         }
//         ESP_LOGI(TAG,"connect to the AP fail");
//     } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
//         ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
//         ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
//         s_retry_num = 0;
//         xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
//     }
// }

#include "library.h"
#include "FunctionInUsed.h"

extern const char *TAG; // Sử dụng một TAG chung cho ESP
int s_retry_num = 0; // Khởi tạo số lần thử kết nối lại

EventGroupHandle_t s_wifi_event_group;

// Hàm xử lý sự kiện Wi-Fi
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
                xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT); // Đánh dấu kết nối thất bại
            }
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        // Khi STA nhận được IP từ AP
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "Got IP: " IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0; // Reset lại số lần thử
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT); // Đánh dấu kết nối thành công
    }
}
