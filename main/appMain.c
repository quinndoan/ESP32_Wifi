#include "library.h"
#include "AP.h"
#include "Sta.h"
#include "appMain.h"
#include "bluetooth.h"

extern const char *TAG;
int32_t wifi_mode = 1; // 0: STA Mode, 1: AP Mode, 2: AP + Bluetooth Mode

void app_main(void) {
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Initialize esp_netif và event loop
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // Mở NVS để đọc giá trị wifi_mode
    nvs_handle_t nvs_handle;
    ESP_ERROR_CHECK(nvs_open("storage", NVS_READWRITE, &nvs_handle));

    int32_t wifi_mode = 1;
    esp_err_t err = nvs_get_i32(nvs_handle, "wifi_mode", &wifi_mode);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        // Nếu không tìm thấy wifi_mode trong NVS, gán giá trị mặc định là 1 (AP Mode)
        wifi_mode = 1;
        ESP_LOGI(TAG, "WiFi mode not found in NVS, defaulting to AP mode");

        // Lưu giá trị mặc định vào NVS để sử dụng sau này
        ESP_ERROR_CHECK(nvs_set_i32(nvs_handle, "wifi_mode", wifi_mode));
        ESP_ERROR_CHECK(nvs_commit(nvs_handle));
    } else if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error (%s) reading wifi_mode from NVS!", esp_err_to_name(err));
        wifi_mode = 1; // Default to AP Mode in case of error
    } else {
        ESP_LOGI(TAG, "Read WiFi mode from NVS: %ld", wifi_mode); // In ra giá trị wifi_mode
    }

    // Đóng NVS sau khi đọc xong
    nvs_close(nvs_handle);

    // Đọc SSID và Password từ NVS
    char ssid[32] = {0};
    char password[64] = {0};
    size_t ssid_len = sizeof(ssid);
    size_t password_len = sizeof(password);

    ESP_ERROR_CHECK(nvs_open("storage", NVS_READONLY, &nvs_handle));
    esp_err_t ssid_err = nvs_get_str(nvs_handle, "ssid", ssid, &ssid_len);
    esp_err_t pass_err = nvs_get_str(nvs_handle, "password", password, &password_len);

    if (ssid_err != ESP_OK || pass_err != ESP_OK) {
        ESP_LOGW(TAG, "SSID or Password not found in NVS. Switching to AP Mode.");
        wifi_mode = 1; // Chuyển sang AP Mode nếu không tìm thấy SSID hoặc Password
    } else {
        ESP_LOGI(TAG, "Read SSID: %s, Password: %s", ssid, password); // In ra giá trị SSID và Password
    }

    nvs_close(nvs_handle); // Đóng NVS sau khi đọc xong

    // Khởi động ESP32 dựa trên chế độ wifi_mode đã đọc được từ NVS
    if (wifi_mode == 0) {
        // Chế độ STA Mode
        if (strlen(ssid) > 0 && strlen(password) > 0) {
            ESP_LOGI(TAG, "Starting in STA Mode with SSID: %s", ssid);
            wifi_init_sta(ssid, password); // Khởi động chế độ STA với SSID và Password từ NVS
        } else {
            ESP_LOGE(TAG, "Cannot start in STA Mode: SSID or Password not set. Starting in AP Mode.");
            wifi_mode = 1; // Chuyển sang AP Mode nếu không có SSID hoặc Password
            wifi_init_softap();
            start_webserver();
        }
    } else if (wifi_mode == 1) {
        // Chế độ AP Mode
        ESP_LOGI(TAG, "Starting in AP Mode");
        wifi_init_softap(); // Khởi động chế độ AP
        start_webserver();  // Khởi động web server để chọn chế độ
    } else if (wifi_mode == 2) {
        // Chế độ AP + Bluetooth Mode
        ESP_LOGI(TAG, "Starting in AP + Bluetooth Mode");
        wifi_init_softap();  // Khởi động chế độ AP
        start_webserver();   // Khởi động web server
        app_ble_start();    // Khởi động Bluetooth
    } else {
        // Chế độ không hợp lệ, mặc định chuyển về AP Mode
        ESP_LOGW(TAG, "Unknown mode, defaulting to AP Mode");
        wifi_mode = 1;
        wifi_init_softap();
        start_webserver();
    }

    // Lưu lại chế độ Wi-Fi vào NVS nếu đã thay đổi
    ESP_ERROR_CHECK(nvs_open("storage", NVS_READWRITE, &nvs_handle));
    ESP_ERROR_CHECK(nvs_set_i32(nvs_handle, "wifi_mode", wifi_mode));
    ESP_ERROR_CHECK(nvs_commit(nvs_handle));
    nvs_close(nvs_handle);
}