// #include "library.h"
// #include "AP.h"
// #include "Sta.h"
// #include "appMain.h"
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
//     ESP_ERROR_CHECK(esp_netif_init());
//     ESP_ERROR_CHECK(esp_event_loop_create_default());

//     // Mở NVS để đọc giá trị wifi_mode
//     nvs_handle_t nvs_handle;
//     ESP_ERROR_CHECK(nvs_open("storage", NVS_READWRITE, &nvs_handle));

//     int32_t wifi_mode = 1;
//     esp_err_t err = nvs_get_i32(nvs_handle, "wifi_mode", &wifi_mode);
//     if (err == ESP_ERR_NVS_NOT_FOUND) {
//         // Nếu không tìm thấy wifi_mode trong NVS, gán giá trị mặc định là 1 (AP Mode)
//         wifi_mode = 1;
//         ESP_LOGI(TAG, "WiFi mode not found in NVS, defaulting to AP mode");

//         // Lưu giá trị mặc định vào NVS để sử dụng sau này
//         ESP_ERROR_CHECK(nvs_set_i32(nvs_handle, "wifi_mode", wifi_mode));
//         ESP_ERROR_CHECK(nvs_commit(nvs_handle));
//     } else if (err != ESP_OK) {
//         ESP_LOGE(TAG, "Error (%s) reading wifi_mode from NVS!", esp_err_to_name(err));
//         wifi_mode = 1; // Default to AP Mode in case of error
//     } else {
//         ESP_LOGI(TAG, "Read WiFi mode from NVS: %ld", wifi_mode); // In ra giá trị wifi_mode
//     }

//     // Đóng NVS sau khi đọc xong
//     nvs_close(nvs_handle);

//     // Đọc SSID và Password từ NVS
//     char ssid[32] = {0};
//     char password[64] = {0};
//     size_t ssid_len = sizeof(ssid);
//     size_t password_len = sizeof(password);

//     ESP_ERROR_CHECK(nvs_open("storage", NVS_READONLY, &nvs_handle));
//     esp_err_t ssid_err = nvs_get_str(nvs_handle, "ssid", ssid, &ssid_len);
//     esp_err_t pass_err = nvs_get_str(nvs_handle, "password", password, &password_len);

//     if (ssid_err != ESP_OK || pass_err != ESP_OK) {
//         ESP_LOGW(TAG, "SSID or Password not found in NVS. Switching to AP Mode.");
//         wifi_mode = 1; // Chuyển sang AP Mode nếu không tìm thấy SSID hoặc Password
//     } else {
//         ESP_LOGI(TAG, "Read SSID: %s, Password: %s", ssid, password); // In ra giá trị SSID và Password
//     }

//     nvs_close(nvs_handle); // Đóng NVS sau khi đọc xong

//     // Khởi động ESP32 dựa trên chế độ wifi_mode đã đọc được từ NVS
//     if (wifi_mode == 0) {
//         // Chế độ STA Mode
//         if (strlen(ssid) > 0 && strlen(password) > 0) {
//             ESP_LOGI(TAG, "Starting in STA Mode with SSID: %s", ssid);
//             wifi_init_sta(ssid, password); // Khởi động chế độ STA với SSID và Password từ NVS
//         } else {
//             ESP_LOGE(TAG, "Cannot start in STA Mode: SSID or Password not set. Starting in AP Mode.");
//             wifi_mode = 1; // Chuyển sang AP Mode nếu không có SSID hoặc Password
//             wifi_init_softap();
//             start_webserver();
//         }
//     } else if (wifi_mode == 1) {
//         // Chế độ AP Mode
//         ESP_LOGI(TAG, "Starting in AP Mode");
//         wifi_init_softap(); // Khởi động chế độ AP
//         start_webserver();  // Khởi động web server để chọn chế độ
//     } else if (wifi_mode == 2) {
//         ESP_LOGI(TAG, "Starting in AP + Bluetooth Mode");
//         wifi_init_softap();  // Khởi động chế độ AP
//         start_webserver();   // Khởi động web server
//         app_ble_start();    // Khởi động Bluetooth

//         // Đăng ký callback nhận dữ liệu từ BLE
//         app_ble_set_data_recv_callback(ble_data_received_callback);

//         // Thông báo người dùng gửi SSID và Password qua BLE
//         const char ble_resp[] = "Bluetooth started. Please send SSID and Password via BLE.";
//         app_ble_send_data((uint8_t*) ble_resp, strlen(ble_resp));
//          // Kiểm tra nếu đã nhận đủ SSID và Password
//     if (is_ssid_received && is_password_received) {
//         ESP_LOGI(TAG, "Received SSID and Password. Switching to STA Mode...");

//         // Lưu Wi-Fi mode thành 0 (STA Mode)
//         nvs_handle_t nvs_handle;
//         esp_err_t err = nvs_open("storage", NVS_READWRITE, &nvs_handle);
//         if (err == ESP_OK) {
//             // Cập nhật chế độ Wi-Fi thành STA Mode (0)
//             err = nvs_set_i32(nvs_handle, "wifi_mode", 0);
//             if (err == ESP_OK) {
//                 ESP_LOGI(TAG, "Wi-Fi mode updated to STA (0).");
//             } else {
//                 ESP_LOGE(TAG, "Failed to update Wi-Fi mode. Error: %s", esp_err_to_name(err));
//             }

//             // Commit thay đổi vào NVS
//             err = nvs_commit(nvs_handle);
//             if (err == ESP_OK) {
//                 ESP_LOGI(TAG, "NVS commit successful. Restarting in STA Mode.");
//             } else {
//                 ESP_LOGE(TAG, "Failed to commit NVS changes. Error: %s", esp_err_to_name(err));
//             }

//             // Đóng NVS
//             nvs_close(nvs_handle);

//             // Gửi phản hồi BLE
//             char success_msg[] = "Switching to STA Mode...";
//             app_ble_send_data((uint8_t *)success_msg, strlen(success_msg));

//             // Dừng BLE trước khi khởi động lại
//             app_ble_stop();

//             // Đợi một chút rồi khởi động lại thiết bị để chuyển sang STA
//             vTaskDelay(2000 / portTICK_PERIOD_MS); // Delay 2 giây
//             esp_restart();
//         } else {
//             ESP_LOGE(TAG, "Failed to open NVS. Error: %s", esp_err_to_name(err));
//         }
//     }
//     } else {
//         // Chế độ không hợp lệ, mặc định chuyển về AP Mode
//         ESP_LOGW(TAG, "Unknown mode, defaulting to AP Mode");
//         wifi_mode = 1;
//         wifi_init_softap();
//         start_webserver();
//     }

//     // Lưu lại chế độ Wi-Fi vào NVS nếu đã thay đổi
//     ESP_ERROR_CHECK(nvs_open("storage", NVS_READWRITE, &nvs_handle));
//     ESP_ERROR_CHECK(nvs_set_i32(nvs_handle, "wifi_mode", wifi_mode));
//     ESP_ERROR_CHECK(nvs_commit(nvs_handle));
//     nvs_close(nvs_handle);
// }
#include "library.h"
#include "AP.h"
#include "Sta.h"
#include "appMain.h"
#include "bluetooth.h"

extern const char *TAG;
int32_t wifi_mode = 1; // 0: STA Mode, 1: AP Mode, 2: AP + Bluetooth Mode

// Các biến để theo dõi trạng thái Wi-Fi AP và STA đã khởi tạo chưa
bool ap_initialized = false;
bool sta_initialized = false;

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

    int32_t wifi_mode = 1; // Bắt đầu luôn bằng AP Mode
    esp_err_t err = nvs_get_i32(nvs_handle, "wifi_mode", &wifi_mode);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "Read WiFi mode from NVS: %ld", wifi_mode); // In ra giá trị wifi_mode
    } else {
        ESP_LOGW(TAG, "WiFi mode not found in NVS, defaulting to AP mode");
        wifi_mode = 1;
    }

    nvs_close(nvs_handle); // Đóng NVS sau khi đọc xong

    // Khởi động ESP32 dựa trên chế độ wifi_mode đã đọc được từ NVS
    if (wifi_mode == 0) {
        // Chế độ STA Mode
        if (!sta_initialized) {
            char ssid[32] = {0};
            char password[64] = {0};
            size_t ssid_len = sizeof(ssid);
            size_t password_len = sizeof(password);

            ESP_ERROR_CHECK(nvs_open("storage", NVS_READONLY, &nvs_handle));
            esp_err_t ssid_err = nvs_get_str(nvs_handle, "ssid", ssid, &ssid_len);
            esp_err_t pass_err = nvs_get_str(nvs_handle, "password", password, &password_len);
            nvs_close(nvs_handle); // Đóng NVS sau khi đọc xong

            if (ssid_err == ESP_OK && pass_err == ESP_OK) {
                ESP_LOGI(TAG, "Starting in STA Mode with SSID: %s", ssid);
                wifi_init_sta(ssid, password); // Khởi động chế độ STA với SSID và Password từ NVS
                sta_initialized = true;  // Đánh dấu STA đã được khởi tạo
            } else {
                ESP_LOGE(TAG, "Failed to retrieve SSID/Password. Switching back to AP Mode.");
                wifi_mode = 1; // Chuyển sang AP Mode nếu không có SSID hoặc Password
                if (!ap_initialized) {
                    wifi_init_softap(); // Khởi động AP nếu chưa khởi tạo
                    ap_initialized = true;  // Đánh dấu AP đã được khởi tạo
                }
                start_webserver();
            }
        }
    } else if (wifi_mode == 1) {
        // Chế độ AP Mode
        ESP_LOGI(TAG, "Starting in AP Mode");
        if (!ap_initialized) {
            wifi_init_softap(); // Khởi động chế độ AP nếu chưa khởi tạo
            ap_initialized = true;  // Đánh dấu AP đã được khởi tạo
        }
        start_webserver();  // Khởi động web server để chọn chế độ
   
}else if (wifi_mode == 2) {
    // Chế độ AP + Bluetooth Mode
    ESP_LOGI(TAG, "Starting in AP + Bluetooth Mode");

    if (!ap_initialized) {
        wifi_init_softap();  // Khởi động chế độ AP nếu chưa khởi tạo
        ap_initialized = true;  // Đánh dấu AP đã được khởi tạo
    }
    
    start_webserver();   // Khởi động web server
    app_ble_start();    // Khởi động Bluetooth

    // Đăng ký callback nhận dữ liệu từ BLE
    app_ble_set_data_recv_callback(ble_data_received_callback);

    // Thông báo người dùng gửi SSID và Password qua BLE
    const char ble_resp[] = "Bluetooth started. Please send SSID and Password via BLE.";
    app_ble_send_data((uint8_t*) ble_resp, strlen(ble_resp));

    // Liên tục kiểm tra trạng thái nhận BLE
    while (true) {
        // Kiểm tra nếu đã nhận đầy đủ SSID và Password
        if (is_ssid_received && is_password_received) {
            ESP_LOGI(TAG, "SSID and Password received via BLE. Switching to STA Mode...");

            // Đọc lại SSID và Password từ NVS
            char ssid[32] = {0};
            char password[64] = {0};
            size_t ssid_len = sizeof(ssid);
            size_t password_len = sizeof(password);

            nvs_handle_t nvs_handle;
            ESP_ERROR_CHECK(nvs_open("storage", NVS_READONLY, &nvs_handle));
            ESP_ERROR_CHECK(nvs_get_str(nvs_handle, "ssid", ssid, &ssid_len));
            ESP_ERROR_CHECK(nvs_get_str(nvs_handle, "password", password, &password_len));
            nvs_close(nvs_handle);

            // Dừng chế độ AP trước khi chuyển sang STA Mode
            ESP_LOGI(TAG, "Stopping AP mode...");
            esp_err_t err = esp_wifi_stop();
            if (err == ESP_OK) {
                ESP_LOGI(TAG, "AP mode stopped successfully.");
            } else {
                ESP_LOGE(TAG, "Failed to stop AP mode: %s", esp_err_to_name(err));
            }

            // Lưu chế độ Wi-Fi thành STA Mode
            ESP_LOGI(TAG, "Updating WiFi mode to STA in NVS...");
            ESP_ERROR_CHECK(nvs_open("storage", NVS_READWRITE, &nvs_handle));
            ESP_ERROR_CHECK(nvs_set_i32(nvs_handle, "wifi_mode", 0));
            ESP_ERROR_CHECK(nvs_commit(nvs_handle));
            nvs_close(nvs_handle);

            // Gửi phản hồi BLE
            char success_msg[] = "Credentials saved. Restarting to connect to WiFi...";
            app_ble_send_data((uint8_t *)success_msg, strlen(success_msg));

            // Đợi một chút để đảm bảo BLE đã gửi thông báo
            vTaskDelay(1000 / portTICK_PERIOD_MS);

            // Khởi động lại thiết bị để chuyển sang STA Mode
            ESP_LOGI(TAG, "Restarting device to apply STA Mode...");
            esp_restart();  // Khởi động lại thiết bị

            // Thoát vòng lặp sau khi khởi động lại
            break;
        }

        // Kiểm tra mỗi giây
        vTaskDelay(1000 / portTICK_PERIOD_MS); // Delay 1 giây
    }
}

    else {
        // Nếu không chọn chế độ hợp lệ, quay về AP Mode
        ESP_LOGW(TAG, "Unknown WiFi mode, defaulting to AP Mode");

        if (!ap_initialized) {
            wifi_init_softap(); // Khởi động chế độ AP nếu chưa khởi tạo
            ap_initialized = true;  // Đánh dấu AP đã được khởi tạo
        }
        start_webserver();
    }
}
