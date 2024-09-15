#include "library.h"
#include "AP.h"
#include "Sta.h"
#include"appMain.h"

extern const char *TAG; // sử dụng một TAG cho ESP để có thể chuyển đổi giữa hai mode
//static int s_retry_num = 0;
//static EventGroupHandle_t s_wifi_event_group;

// Cập nhật hàm app_main
void app_main(void) {
    //Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Initialize esp_netif và event loop chỉ một lần
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // ESP_LOGI(TAG, "ESP_WIFI_MODE_AP");
    // wifi_init_softap();

    // // Khoi dong server
    // start_webserver();

    // Khai báo các biến để lưu SSID và password
    char ssid[32] = {0};
    char password[64] = {0};
    size_t ssid_len = sizeof(ssid);
    size_t password_len = sizeof(password);

    // Mở NVS và đọc SSID và password
    nvs_handle_t nvs_handle;
    ESP_ERROR_CHECK(nvs_open("storage", NVS_READONLY, &nvs_handle));
    ESP_ERROR_CHECK(nvs_get_str(nvs_handle, "ssid", ssid, &ssid_len));
    ESP_ERROR_CHECK(nvs_get_str(nvs_handle, "password", password, &password_len));
    nvs_close(nvs_handle);

    
    // Kiểm tra nếu SSID và password đã được thiết lập
    if (strlen(ssid) > 0 && strlen(password) > 0) {
        ESP_LOGI(TAG, "Connecting to WiFi SSID: %s", ssid);
        wifi_init_sta(ssid, password); // Khởi tạo chế độ STA và kết nối WiFi
    } else {
        ESP_LOGI(TAG, "No WiFi credentials found, starting AP mode");
        wifi_init_softap(); // Bắt đầu ở chế độ AP để người dùng có thể nhập thông tin WiFi
        start_webserver(); // Khởi động server web để người dùng nhập SSID và password
    }
}
