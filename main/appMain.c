// #include "library.h"
// #include "AP.h"
// #include "Sta.h"
// #include"appMain.h"

// extern const char *TAG; // sử dụng một TAG cho ESP để có thể chuyển đổi giữa hai mode
// extern EventGroupHandle_t s_wifi_event_group;
// extern QueueHandle_t mode_queue;

// // hàm kiểm tra xem dữ liệu đã có trong NVS chưa
// bool check_saved_wifi(char *ssid, char *password){
//   nvs_handle_t nvs_handle;
//   esp_err_t err;
//   size_t ssid_len = 32;
//   size_t password_len = 64;
  
//   // Open NVS
//   err = nvs_open("storage", NVS_READONLY, &nvs_handle);
//   if (err != ESP_OK){
//     ESP_LOGI(TAG, "Can't open NVS: %s",esp_err_to_name(err));
//     return false;
//   }
//   // Handle SSID and password from NVS
//   do {
//     err = nvs_get_str(nvs_handle, "ssid", ssid, &ssid_len);
//     if (err != ESP_OK){
//       ESP_LOGI(TAG,"SSID Error: %s", esp_err_to_name(err));
//       break;
//     }
//     err = nvs_get_str(nvs_handle, "password", password, &password_len);
//     if (err != ESP_OK){
//       ESP_LOGI(TAG, "Password Error: %s", esp_err_to_name(err));
//       break;
//     }
//     nvs_close(nvs_handle);
//     return true;
//   }while(0);
  
//     // Đóng NVS nếu có lỗi
//     nvs_close(nvs_handle);
//     return false;
// }

// void app_main(void) {
//     // Initial NVS, Wifi, Event_Loop...
//     esp_err_t ret = nvs_flash_init();
//     if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND){
//       ESP_ERROR_CHECK(nvs_flash_erase());
//       ret = nvs_flash_init();
//     }
//     ESP_ERROR_CHECK(ret);

//     ESP_ERROR_CHECK(esp_netif_init());
//     ESP_ERROR_CHECK(esp_event_loop_create_default());

//     const uart_config_t uart_config = {
//         .baud_rate = 115200,
//         .data_bits = UART_DATA_8_BITS,
//         .parity    = UART_PARITY_DISABLE,
//         .stop_bits = UART_STOP_BITS_1,
//         .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
//     };
//     uart_param_config(UART_NUM_1, &uart_config);
//     uart_driver_install(UART_NUM_1, BUF_SIZE * 2, 0, 0, NULL, 0);

//     // Tạo hàng đợi cho chế độ (STA hoặc AP)
//     mode_queue = xQueueCreate(10, sizeof(uint8_t));

//     // Tạo task UART để lắng nghe dữ liệu từ người dùng
//     xTaskCreate(uart_event_task, "uart_event_task", 2048, NULL, 10, NULL);

//    char ssid[32] = {0};
//     char password[64] = {0};
//     if (check_saved_wifi(ssid, password)) {
//         ESP_LOGI(TAG, "WiFi saved, Send Data though UART...");

//         uint8_t mode;
//         // Chờ người dùng nhập từ UART để quyết định chế độ
//         if (xQueueReceive(mode_queue, &mode, portMAX_DELAY)) {
//             if (mode == 1) {  // Nếu người dùng chọn STA
//                 ESP_LOGI(TAG, "Connecting: %s", ssid);
//                 wifi_init_sta(ssid, password);  // Khởi tạo chế độ STA với thông tin đã lưu

//                 // Chờ kết quả kết nối
//                 EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
//                                                        WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
//                                                        pdFALSE, // Không xóa các bit này
//                                                        pdFALSE, // Không cần tất cả các bit
//                                                        portMAX_DELAY);

//                 if (bits & WIFI_CONNECTED_BIT) {
//                     ESP_LOGI(TAG, "Connected Wifi Sucessfully");
//                     return;  // Giữ kết nối và thoát ra nếu kết nối thành công
//                 } else if (bits & WIFI_FAIL_BIT) {
//                     ESP_LOGI(TAG, "Failed to connect");
//                 }
//             }
//         }
//     } else {
//         ESP_LOGI(TAG, "No Saved Wifi, switch to AP mode");
//     }

//     // Step 3: Chuyển sang chế độ AP để nhập thông tin WiFi mới
//     ESP_LOGI(TAG, "ESP_WIFI_MODE_AP");
//     wifi_init_softap();  // Khởi tạo chế độ AP
//     start_webserver();   // Mở web server để nhập thông tin WiFi

//     // Step 4: Quay lại chế độ STA với thông tin WiFi mới
//     if (check_saved_wifi(ssid, password)) {
//         ESP_LOGI(TAG, "Connecting to new Wifi: %s", ssid);
//         wifi_init_sta(ssid, password);  // Khởi tạo chế độ STA với WiFi mới

//         EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
//                                                WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
//                                                pdFALSE, // Không xóa các bit này
//                                                pdFALSE, // Không cần tất cả các bit
//                                                portMAX_DELAY);

//         if (bits & WIFI_CONNECTED_BIT) {
//             ESP_LOGI(TAG, "Connected new Wifi Sucessully");
//         } else if (bits & WIFI_FAIL_BIT) {
//             ESP_LOGI(TAG, "Failed to connect new Wifi, check your Wifi Information");
//         }
//     }
// }

#include "library.h"
#include "AP.h"
#include "Sta.h"
#include "appMain.h"

extern const char *TAG; // sử dụng một TAG cho ESP để có thể chuyển đổi giữa hai mode
extern EventGroupHandle_t s_wifi_event_group;
extern QueueHandle_t mode_queue;
extern uint8_t mode;

extern bool uart_initialized;

// Hàm kiểm tra xem dữ liệu WiFi đã có trong NVS chưa
bool check_saved_wifi(char *ssid, char *password) {
    nvs_handle_t nvs_handle;
    esp_err_t err;
    size_t ssid_len = 32;
    size_t password_len = 64;

    // Open NVS
    err = nvs_open("storage", NVS_READONLY, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGI(TAG, "Can't open NVS: %s", esp_err_to_name(err));
        return false;
    }

    // Handle SSID and password from NVS
    err = nvs_get_str(nvs_handle, "ssid", ssid, &ssid_len);
    if (err != ESP_OK) {
        ESP_LOGI(TAG, "SSID Error: %s", esp_err_to_name(err));
        nvs_close(nvs_handle);
        return false;
    }
    err = nvs_get_str(nvs_handle, "password", password, &password_len);
    if (err != ESP_OK) {
        ESP_LOGI(TAG, "Password Error: %s", esp_err_to_name(err));
        nvs_close(nvs_handle);
        return false;
    }
    nvs_close(nvs_handle);
    return true;
}

void app_main(void) {
    // Step 1: Khởi tạo NVS, WiFi, Event Loop
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // Tạo hàng đợi cho chế độ (STA hoặc AP)
    mode_queue = xQueueCreate(10, sizeof(uint8_t));

    char ssid[32] = {0};
    char password[64] = {0};
    if (check_saved_wifi(ssid, password)) {
        ESP_LOGI(TAG, "WiFi saved, waiting for user input through UART...");

        // Chỉ khởi tạo UART khi cần
        initialize_uart_if_needed();

        // Tạo task UART để lắng nghe dữ liệu từ người dùng
        xTaskCreate(uart_event_task, "uart_event_task", 2048, NULL, 10, NULL);

        uint8_t mode;
        // Chờ người dùng nhập từ UART để quyết định chế độ
        if (xQueueReceive(mode_queue, &mode, portMAX_DELAY)) {
            if (mode == 1) {  // Nếu người dùng chọn STA
                ESP_LOGI(TAG, "Connecting: %s", ssid);
                wifi_init_sta(ssid, password);  // Khởi tạo chế độ STA với thông tin đã lưu

                // Chờ kết quả kết nối
                EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
                                                       WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                                                       pdFALSE, // Không xóa các bit này
                                                       pdFALSE, // Không cần tất cả các bit
                                                       portMAX_DELAY);

                if (bits & WIFI_CONNECTED_BIT) {
                    ESP_LOGI(TAG, "Connected Wifi Sucessfully");
                    return;  // Giữ kết nối và thoát ra nếu kết nối thành công
                } else if (bits & WIFI_FAIL_BIT) {
                    ESP_LOGI(TAG, "Failed to connect");
                }
            }
        }
    } else {
        ESP_LOGI(TAG, "No Saved Wifi, switching to AP mode");
    }

    // Step 3: Chuyển sang chế độ AP để nhập thông tin WiFi mới
    ESP_LOGI(TAG, "ESP_WIFI_MODE_AP");
    wifi_init_softap();  // Khởi tạo chế độ AP
    start_webserver();   // Mở web server để nhập thông tin WiFi

    // Step 4: Quay lại chế độ STA với thông tin WiFi mới
    if (check_saved_wifi(ssid, password)) {
        ESP_LOGI(TAG, "Connecting to new Wifi: %s", ssid);
        wifi_init_sta(ssid, password);  // Khởi tạo chế độ STA với WiFi mới

        EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
                                               WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                                               pdFALSE, // Không xóa các bit này
                                               pdFALSE, // Không cần tất cả các bit
                                               portMAX_DELAY);

        if (bits & WIFI_CONNECTED_BIT) {
            ESP_LOGI(TAG, "Connected new Wifi Sucessully");
        } else if (bits & WIFI_FAIL_BIT) {
            ESP_LOGI(TAG, "Failed to connect new Wifi, check your Wifi Information");
        }
    }
}
