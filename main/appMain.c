#include "library.h"
#include "AP.h"
#include "Sta.h"
#include"appMain.h"
#include "gpioMake.h"
extern const char *TAG; // sử dụng một TAG cho ESP để có thể chuyển đổi giữa hai mode
int32_t wifi_mode = 0;

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

    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_DISABLE; // Không cần ngắt
    io_conf.mode = GPIO_MODE_INPUT;        // Cấu hình làm đầu vào
    io_conf.pin_bit_mask = (1ULL << GPIO_BUTTON_PIN); // Gán GPIO cho nút nhấn
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE; // Bật pull-up
    gpio_config(&io_conf);

    nvs_handle_t nvs_handle;
    ESP_ERROR_CHECK(nvs_open("storage", NVS_READWRITE, &nvs_handle));

    esp_err_t err = nvs_get_i32(nvs_handle, "wifi_mode", &wifi_mode);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        // Nếu không tìm thấy khóa "wifi_mode", thiết lập chế độ mặc định là AP mode
        wifi_mode = 1; // Đặt mặc định là AP mode
        ESP_LOGI(TAG, "WiFi mode not found in NVS, defaulting to AP mode");

        // Lưu chế độ mặc định vào NVS
        ESP_ERROR_CHECK(nvs_set_i32(nvs_handle, "wifi_mode", wifi_mode));
        ESP_ERROR_CHECK(nvs_commit(nvs_handle));
    } else if (err != ESP_OK) {
        // Nếu có lỗi khác, báo lỗi
        ESP_LOGE(TAG, "Error (%s) reading wifi_mode from NVS!", esp_err_to_name(err));
    }

    nvs_close(nvs_handle);

    char ssid[32] = {0};
    char password[64] = {0};
    size_t ssid_len = sizeof(ssid);
    size_t password_len = sizeof(password);

    ESP_ERROR_CHECK(nvs_open("storage", NVS_READONLY, &nvs_handle));
    esp_err_t ssid_err = nvs_get_str(nvs_handle, "ssid", ssid, &ssid_len);
    esp_err_t pass_err = nvs_get_str(nvs_handle, "password", password, &password_len);

    // appMain.c
    if (ssid_err != ESP_OK) {
        ESP_LOGW(TAG, "SSID not found in NVS");
    }

    if (pass_err != ESP_OK) {
        ESP_LOGW(TAG, "Password not found in NVS");
    }

    nvs_close(nvs_handle);

    // Khởi động chế độ ban đầu dựa trên giá trị lưu trữ trong NVS
    if (wifi_mode == 0)
    {
        ESP_LOGI(TAG, "Starting in STA Mode");
        wifi_init_sta(ssid, password);
    }
    else
    {
        ESP_LOGI(TAG, "Starting in AP Mode");
        wifi_init_softap();
        start_webserver();
    }

    // Vòng lặp kiểm tra trạng thái nút nhấn
    while (true)
    {
        if (is_button_pressed())
        {
            ESP_LOGI(TAG, "Button pressed, switching Wi-Fi mode");
            handle_button_press();
        }
        vTaskDelay(pdMS_TO_TICKS(100)); // Kiểm tra trạng thái nút mỗi 100ms
    }
}
