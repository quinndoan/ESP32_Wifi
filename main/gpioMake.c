// #include "gpioMake.h"

// extern int32_t wifi_mode;
// bool is_button_pressed()
// {
//     static int button_state = 1; // Giữ trạng thái hiện tại của nút (1 là không nhấn, 0 là nhấn)
//     int current_state = gpio_get_level(GPIO_BUTTON_PIN);
//     if (current_state != button_state)
//     {
//         button_state = current_state;
//         if (button_state == 0) // Chỉ khi nút được nhấn mới trả về true
//         {
//             // Đợi một khoảng thời gian ngắn để tránh rung
//             vTaskDelay(pdMS_TO_TICKS(50));
//             if (gpio_get_level(GPIO_BUTTON_PIN) == 0)
//             {
//                 return true;
//             }
//         }
//     }
//     return false;
// }

// void handle_button_press()
// {
//     wifi_mode = !wifi_mode; // Chuyển đổi chế độ
//     // Lấy SSID và Password từ NVS
//     char ssid[32] = {0};
//     char password[64] = {0};
//     size_t ssid_len = sizeof(ssid);
//     size_t password_len = sizeof(password);
//     nvs_handle_t nvs_handle;
//     ESP_ERROR_CHECK(nvs_open("storage", NVS_READONLY, &nvs_handle));
//     esp_err_t ssid_err = nvs_get_str(nvs_handle, "ssid", ssid, &ssid_len);
//     esp_err_t pass_err = nvs_get_str(nvs_handle, "password", password, &password_len);

//     ESP_ERROR_CHECK(nvs_open("storage", NVS_READWRITE, &nvs_handle));
//     ESP_ERROR_CHECK(nvs_set_i32(nvs_handle, "wifi_mode", wifi_mode));
//     ESP_ERROR_CHECK(nvs_commit(nvs_handle));
//     nvs_close(nvs_handle);

//     if (wifi_mode == 0 && ssid_err == ESP_OK && pass_err == ESP_OK)
//     {
//         ESP_LOGI(TAG, "Switching to STA Mode...");
//         wifi_init_sta(ssid, password);
//     }
//     else
//     {
//         ESP_LOGI(TAG, "Switching to AP Mode...");
//         wifi_init_softap();
//     }

    
// }
