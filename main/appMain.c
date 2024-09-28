#include "library.h"
#include "AP.h"
#include "Sta.h"
#include "appMain.h"
#include "FunctionInUsed.h"
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

    // Read WiFi mode from NVS
    nvs_handle_t nvs_handle;
    ESP_ERROR_CHECK(nvs_open("storage", NVS_READWRITE, &nvs_handle));
    esp_err_t err = nvs_get_i32(nvs_handle, "wifi_mode", &wifi_mode);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        // Default to AP mode if not found
        wifi_mode = 1; // AP mode
        ESP_LOGI(TAG, "WiFi mode not found in NVS, defaulting to AP mode");

        // Save default mode to NVS
        ESP_ERROR_CHECK(nvs_set_i32(nvs_handle, "wifi_mode", wifi_mode));
        ESP_ERROR_CHECK(nvs_commit(nvs_handle));
    } else if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error (%s) reading wifi_mode from NVS!", esp_err_to_name(err));
    }
    nvs_close(nvs_handle);

    // Read SSID and Password from NVS
    char ssid[32] = {0};
    char password[64] = {0};
    size_t ssid_len = sizeof(ssid);
    size_t password_len = sizeof(password);

    ESP_ERROR_CHECK(nvs_open("storage", NVS_READONLY, &nvs_handle));
    esp_err_t ssid_err = nvs_get_str(nvs_handle, "ssid", ssid, &ssid_len);
    esp_err_t pass_err = nvs_get_str(nvs_handle, "password", password, &password_len);

    if (ssid_err != ESP_OK) {
        ESP_LOGW(TAG, "SSID not found in NVS");
    }

    if (pass_err != ESP_OK) {
        ESP_LOGW(TAG, "Password not found in NVS");
    }

    nvs_close(nvs_handle);

    // Start based on saved Wi-Fi mode
    if (wifi_mode == 0) {
        // STA Mode
        ESP_LOGI(TAG, "Starting in STA Mode");
        wifi_init_sta(ssid, password);
    } else if (wifi_mode == 1) {
        // AP Mode
        ESP_LOGI(TAG, "Starting in AP Mode");
        wifi_init_softap();
        start_webserver(); // Giao diện chọn chế độ và nhập thông tin
    } else if (wifi_mode == 2) {
        // AP + Bluetooth Mode
        ESP_LOGI(TAG, "Starting in AP + Bluetooth Mode");
        wifi_init_softap();
        start_webserver();
        init_bluetooth(); // Khởi động Bluetooth
    }
}