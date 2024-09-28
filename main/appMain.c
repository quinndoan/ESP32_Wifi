// #include "library.h"
// #include "AP.h"
// #include "Sta.h"
// #include "appMain.h"
// #include "FunctionInUsed.h"
// #include "bluetooth.h"

// extern const char *TAG;
// int32_t wifi_mode = 1; // 0: STA Mode, 1: AP Mode, 2: AP + Bluetooth Mode

// void app_main(void) {
//     // Initialize NVS
//     esp_err_t ret = nvs_flash_init();
//     if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
//         ESP_ERROR_CHECK(nvs_flash_erase());
//         ret = nvs_flash_init();
//     }
//     ESP_ERROR_CHECK(ret);

//     // Initialize esp_netif và event loop
//         ESP_ERROR_CHECK(esp_netif_init());
//         ESP_ERROR_CHECK(esp_event_loop_create_default());

//         // Read WiFi mode from NVS
//         nvs_handle_t nvs_handle;
//         ESP_ERROR_CHECK(nvs_open("storage", NVS_READWRITE, &nvs_handle));
//         esp_err_t err = nvs_get_i32(nvs_handle, "wifi_mode", &wifi_mode);
//         if (err == ESP_ERR_NVS_NOT_FOUND) {
//             // Default to AP mode if not found
//             wifi_mode = 1; // AP mode
//             ESP_LOGI(TAG, "WiFi mode not found in NVS, defaulting to AP mode");

//             // Save default mode to NVS
//             ESP_ERROR_CHECK(nvs_set_i32(nvs_handle, "wifi_mode", wifi_mode));
//             ESP_ERROR_CHECK(nvs_commit(nvs_handle));
//         } else if (err != ESP_OK) {
//             ESP_LOGE(TAG, "Error (%s) reading wifi_mode from NVS!", esp_err_to_name(err));
//         }
//         nvs_close(nvs_handle);

// // Read SSID and Password from NVS
//     char ssid[32] = {0};
//     char password[64] = {0};

//     if (wifi_mode ==0){     // only read if in STA mode
//         size_t ssid_len = sizeof(ssid);
//         size_t password_len = sizeof(password);

//         ESP_ERROR_CHECK(nvs_open("storage", NVS_READONLY, &nvs_handle));
//         esp_err_t ssid_err = nvs_get_str(nvs_handle, "ssid", ssid, &ssid_len);
//         esp_err_t pass_err = nvs_get_str(nvs_handle, "password", password, &password_len);

//         if (ssid_err != ESP_OK) {
//             ESP_LOGW(TAG, "SSID not found in NVS");
//         }

//         if (pass_err != ESP_OK) {
//             ESP_LOGW(TAG, "Password not found in NVS");
//         }

//         nvs_close(nvs_handle);

//     }



//      // Start based on saved Wi-Fi mode
//     if (wifi_mode == 0) {
//         // STA Mode
//         if (strlen(ssid) > 0 && strlen(password) > 0) { // Check if SSID and Password are available
//             ESP_LOGI(TAG, "Starting in STA Mode");
//             wifi_init_sta(ssid, password);
//         } else {
//             ESP_LOGE(TAG, "Cannot start in STA Mode: SSID or Password not set");
//         }
//     } else if (wifi_mode == 1) {
//         // AP Mode
//         ESP_LOGI(TAG, "Starting in AP Mode");
//         wifi_init_softap();
//         start_webserver(); // Giao diện chọn chế độ và nhập thông tin
//     } else if (wifi_mode == 2) {
//         // AP + Bluetooth Mode
//         ESP_LOGI(TAG, "Starting in AP + Bluetooth Mode");
//         wifi_init_softap();
//         start_webserver();
//         init_bluetooth(); // Khởi động Bluetooth
//     } else {
//         // Fallback to AP Mode if unknown mode is set
//         ESP_LOGW(TAG, "Unknown mode, defaulting to AP Mode");
//         wifi_init_softap();
//         start_webserver();
//     }
// }
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

    // Mở NVS để đọc giá trị wifi_mode
    nvs_handle_t nvs_handle;
    ESP_ERROR_CHECK(nvs_open("storage", NVS_READWRITE, &nvs_handle));

    // Đọc wifi_mode từ NVS
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
        ESP_LOGI(TAG, "Read WiFi mode from NVS: %d", wifi_mode); // In ra giá trị wifi_mode
    }

    // Đóng NVS sau khi đọc xong
    nvs_close(nvs_handle);

    // Đọc SSID và Password từ NVS nếu ở STA Mode
    char ssid[32] = {0};
    char password[64] = {0};

    if (wifi_mode == 0) { // Chỉ đọc SSID và Password khi ở STA Mode
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
    }

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
        init_bluetooth();    // Khởi động Bluetooth
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
