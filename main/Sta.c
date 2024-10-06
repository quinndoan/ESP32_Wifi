#include "Sta.h"

extern int s_retry_num;
extern const char *TAG; // Sử dụng một TAG cho ESP để có thể chuyển đổi giữa hai mode
extern EventGroupHandle_t s_wifi_event_group;

// Hàm để khởi tạo WiFi ở chế độ STA
void wifi_init_sta(const char* ssid, const char* password) {
    // Khởi tạo lại số lần thử kết nối
    s_retry_num = 0;

    // Tạo event group nếu chưa được tạo
    if (s_wifi_event_group == NULL) {
        s_wifi_event_group = xEventGroupCreate();
    }

    // Tạo giao diện mạng STA
    esp_netif_create_default_wifi_sta();

    // Cấu hình WiFi
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    // Đăng ký xử lý sự kiện cho WiFi và IP
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        NULL));

    // Cấu hình thông tin WiFi
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = "",  // Đảm bảo rằng các chuỗi được khởi tạo
            .password = "",
            .threshold.authmode = WIFI_AUTH_WPA2_PSK, // Đặt ngưỡng bảo mật
        },
    };

    // Sao chép SSID và Password vào cấu hình WiFi
    strncpy((char*)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid) - 1);
    wifi_config.sta.ssid[sizeof(wifi_config.sta.ssid) - 1] = '\0'; // Đảm bảo chuỗi kết thúc bằng '\0'
    strncpy((char*)wifi_config.sta.password, password, sizeof(wifi_config.sta.password) - 1);
    wifi_config.sta.password[sizeof(wifi_config.sta.password) - 1] = '\0'; // Đảm bảo chuỗi kết thúc bằng '\0'

    // Thiết lập chế độ WiFi là STA
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config)); // Đặt cấu hình WiFi cho chế độ STA
    ESP_ERROR_CHECK(esp_wifi_start()); // Bắt đầu WiFi

    // In ra log
    ESP_LOGI(TAG, "wifi_init_sta finished. SSID: %s, Password: %s", ssid, password);
}
