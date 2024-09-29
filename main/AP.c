#include "AP.h"
#include "bluetooth.h"
#include "appMain.h"
extern int32_t wifi_mode;
extern const char *TAG; // sử dụng một TAG cho ESP để có thể chuyển đổi giữa hai mode
extern EventGroupHandle_t s_wifi_event_group;
// extern bool is_ssid_received;
// extern bool is_password_received;

void wifi_init_softap(void) {
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        NULL));

    wifi_config_t wifi_config = {
    .ap = {
        .ssid = EXAMPLE_ESP_WIFI_SSID,
        .ssid_len = strlen(EXAMPLE_ESP_WIFI_SSID),
        .channel = EXAMPLE_ESP_WIFI_CHANNEL,
        .password = EXAMPLE_ESP_WIFI_PASS,
        .max_connection = 4,
        .authmode = WIFI_AUTH_WPA2_PSK, // Security dùng WPA2
        .pmf_cfg = {
            .required = false,
        },
    },
};

    if (strlen(EXAMPLE_ESP_WIFI_PASS) == 0) {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "wifi_init_softap finished. SSID:%s password:%s channel:%d",
             EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS, EXAMPLE_ESP_WIFI_CHANNEL);
}

esp_err_t root_get_handler(httpd_req_t *req) {
    const char resp[] = "<!DOCTYPE html><html><head>"
                        "<style>"
                        "body {"
                        "  background-image: url('https://drive.google.com/uc?export=view&id=1IBvc80BpTBpE8Awo_3ZvqtV9WdnF_zwq');"
                        "  background-size: cover;"
                        "  background-position: center;"
                        "  font-family: Arial, sans-serif;"
                        "  display: flex;"
                        "  justify-content: center;"
                        "  align-items: center;"
                        "  height: 100vh;"
                        "  margin: 0;"
                        "} "
                        ".container {"
                        "  background-color: rgba(255, 255, 255, 0.8);"
                        "  padding: 20px;"
                        "  border-radius: 10px;"
                        "  box-shadow: 0 0 10px rgba(0, 0, 0, 0.1);"
                        "} "
                        "h1 {"
                        "  text-align: center;"
                        "  color: #333;"
                        "} "
                        "input[type='text'], input[type='password'] {"
                        "  width: 100%;"
                        "  padding: 12px 20px;"
                        "  margin: 8px 0;"
                        "  display: inline-block;"
                        "  border: 1px solid #ccc;"
                        "  border-radius: 4px;"
                        "  box-sizing: border-box;"
                        "} "
                        "input[type='submit'], button {"
                        "  width: 100%;"
                        "  background-color: #4CAF50;"
                        "  color: white;"
                        "  padding: 14px 20px;"
                        "  margin: 8px 0;"
                        "  border: none;"
                        "  border-radius: 4px;"
                        "  cursor: pointer;"
                        "} "
                        "input[type='submit']:hover, button:hover {"
                        "  background-color: #45a049;"
                        "} "
                        "#ssid-input, #password-input {"
                        "  display: none;"
                        "} "
                        "</style>"
                        "<script>"
                        "function toggleInputs() {"
                        "  var mode = document.getElementById('mode-select').value;"
                        "  if (mode == '0') {"  // Chỉ hiện SSID và Password khi mode là 0
                        "    document.getElementById('ssid-input').style.display = 'block';"
                        "    document.getElementById('password-input').style.display = 'block';"
                        "  } else {"
                        "    document.getElementById('ssid-input').style.display = 'none';"
                        "    document.getElementById('password-input').style.display = 'none';"
                        "  }"
                        "  if (mode == '2') {"
                        "    // Nếu chế độ là AP + Bluetooth, thông báo cho người dùng gửi SSID và Password qua BLE."
                        "    alert('Please send SSID and Password via Bluetooth');"
                        "  }"
                        "}"
                        "window.onload = function() {"
                        "  toggleInputs();"
                        "};"
                        "</script>"
                        "</head><body>"
                        "<div class=\"container\">"
                        "<h1>AIThingsLab</h1>"
                        "<form action=\"/setup\" method=\"post\">"
                        "Mode: <select id=\"mode-select\" name=\"mode\" onchange=\"toggleInputs()\">"
                        "<option value=\"0\">STA Mode</option>" // Cập nhật giá trị là 0 cho STA Mode
                        "<option value=\"1\">AP Mode</option>"
                        "<option value=\"2\">AP Mode + Bluetooth</option>"
                        "</select><br>"
                        "<div id=\"ssid-input\">SSID: <input type=\"text\" name=\"ssid\" placeholder=\"Enter SSID\"></div>"
                        "<div id=\"password-input\">Password: <input type=\"password\" name=\"password\" placeholder=\"Enter Password\"></div>"
                        "<input type=\"submit\" value=\"Submit\">"
                        "</form></div></body></html>";
    httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

// esp_err_t setup_post_handler(httpd_req_t *req) {
//     char buf[200];
//     int ret, remaining = req->content_len;

//     // Đọc dữ liệu từ yêu cầu POST
//     while (remaining > 0) {
//         if ((ret = httpd_req_recv(req, buf, MIN(remaining, sizeof(buf)))) <= 0) {
//             if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
//                 continue;
//             }
//             return ESP_FAIL;
//         }
//         remaining -= ret;
//     }

//     buf[req->content_len] = '\0'; // Kết thúc chuỗi

//     // Phân tích dữ liệu nhận được từ biểu mẫu
//     char ssid[32] = {0};
//     char password[64] = {0};
//     int mode = 1;

//     // Scan mode trước
//     int scanned = sscanf(buf, "mode=%d&ssid=%[^&]&password=%s", &mode, ssid, password);
//     if (scanned < 1) { // Kiểm tra ít nhất phải có mode được phân tích
//         ESP_LOGE(TAG, "Failed to parse mode. Input data: %s", buf);
//         const char resp[] = "Failed to parse mode. Please try again.";
//         httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
//         return ESP_FAIL;
//     }


//     ESP_LOGI("SETUP", "Received SSID: %s, Password: %s, Mode: %d", ssid, password, mode);

//     // Lưu chế độ và thông tin Wi-Fi vào NVS
//     nvs_handle_t nvs_handle;
//     esp_err_t err = nvs_open("storage", NVS_READWRITE, &nvs_handle);
//     if (err != ESP_OK) {
//         ESP_LOGE("SETUP", "Failed to open NVS handle! Error: %s", esp_err_to_name(err));
//         const char resp[] = "Failed to open NVS. Please try again.";
//         httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
//         return ESP_FAIL;
//     }

//     // Lưu chế độ vào NVS
//     err = nvs_set_i32(nvs_handle, "wifi_mode", mode);
//     if (err != ESP_OK) {
//         ESP_LOGE("SETUP", "Failed to set WiFi mode in NVS! Error: %s", esp_err_to_name(err));
//     }

//     // Nếu chế độ là AP + Bluetooth
//     if (mode == 2) {
//         ESP_LOGI(TAG, "AP + Bluetooth mode selected. Starting Bluetooth...");

//         // Khởi động Bluetooth và gửi yêu cầu người dùng gửi SSID và Password
//         app_ble_start();
//         const char ble_resp[] = "Bluetooth started. Please send SSID and Password via BLE.";
//         app_ble_send_data((uint8_t*) ble_resp, strlen(ble_resp));
        
//         // Cài đặt callback để nhận dữ liệu từ BLE
//         app_ble_set_data_recv_callback(ble_data_received_callback);
        
//         // Gửi phản hồi đến client
//         httpd_resp_send(req, "Please send SSID and Password via Bluetooth.", HTTPD_RESP_USE_STRLEN);
        
//         // Đóng NVS
//         nvs_close(nvs_handle);
        
//         return ESP_OK;
//     }


//     // Nếu có SSID và Password, lưu chúng vào NVS
//     if (strlen(ssid) > 0 && strlen(password) > 0) {
//         err = nvs_set_str(nvs_handle, "ssid", ssid);
//         if (err != ESP_OK) {
//             ESP_LOGE("SETUP", "Failed to set SSID in NVS! Error: %s", esp_err_to_name(err));
//             nvs_close(nvs_handle);
//             const char resp[] = "Failed to save SSID. Please try again.";
//             httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
//             return ESP_FAIL;
//         }

//         err = nvs_set_str(nvs_handle, "password", password);
//         if (err != ESP_OK) {
//             ESP_LOGE("SETUP", "Failed to set Password in NVS! Error: %s", esp_err_to_name(err));
//             nvs_close(nvs_handle);
//             const char resp[] = "Failed to save Password. Please try again.";
//             httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
//             return ESP_FAIL;
//         }
//     }

//     // Commit thay đổi để lưu vào NVS
//     err = nvs_commit(nvs_handle);
//     if (err != ESP_OK) {
//         ESP_LOGE("SETUP", "Failed to commit changes in NVS! Error: %s", esp_err_to_name(err));
//         nvs_close(nvs_handle);
//         const char resp[] = "Failed to save settings. Please try again.";
//         httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
//         return ESP_FAIL;
//     }

//     // Đóng NVS
//     nvs_close(nvs_handle);

//     // Gửi phản hồi thành công đến client
//     const char resp[] = "WiFi settings have been updated. Change to STA mode...";
//     httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);

//     // Đợi một chút rồi khởi động lại thiết bị để áp dụng cài đặt mới
//     vTaskDelay(1000 / portTICK_PERIOD_MS); // Chờ 1 giây để hoàn thành phản hồi HTTP
//     esp_restart(); // Khởi động lại thiết bị để áp dụng chế độ mới

//     return ESP_OK;
// }


// Khởi tạo web server
void start_webserver(void) {
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    httpd_uri_t root = {
        .uri = "/",
        .method = HTTP_GET,
        .handler = root_get_handler,
        .user_ctx = NULL
    };

    httpd_uri_t setup = {
        .uri = "/setup",
        .method = HTTP_POST,
        .handler = setup_post_handler,
        .user_ctx = NULL
    };


    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_register_uri_handler(server, &root);
        httpd_register_uri_handler(server, &setup);
    }
}

esp_err_t setup_post_handler(httpd_req_t *req) {
    char buf[200];
    int ret, remaining = req->content_len;

    // Đọc dữ liệu từ yêu cầu POST
    while (remaining > 0) {
        if ((ret = httpd_req_recv(req, buf, MIN(remaining, sizeof(buf)))) <= 0) {
            if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
                continue;
            }
            return ESP_FAIL;
        }
        remaining -= ret;
    }

    buf[req->content_len] = '\0'; // Kết thúc chuỗi an toàn

    int mode = 0; // Biến để lưu chế độ hoạt động

    // Phân tích dữ liệu POST nhận được
    if (sscanf(buf, "mode=%d", &mode) != 1) {
        ESP_LOGE(TAG, "Failed to parse mode. Input data: %s", buf);
        const char resp[] = "Failed to parse mode. Please try again.";
        httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Parsed Mode: %d", mode);

    // Mở NVS (Non-Volatile Storage) để lưu cài đặt Wi-Fi
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open("storage", NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open NVS handle! Error: %s", esp_err_to_name(err));
        const char resp[] = "Failed to open NVS. Please try again.";
        httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
        return ESP_FAIL;
    }

    // Lưu chế độ Wi-Fi vào NVS
    err = nvs_set_i32(nvs_handle, "wifi_mode", mode);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set WiFi mode in NVS! Error: %s", esp_err_to_name(err));
    }

    // Kiểm tra nếu chế độ là AP + Bluetooth
    if (mode == 2) {
        ESP_LOGI(TAG, "AP + Bluetooth mode selected. Starting Bluetooth...");

        // Khởi động Bluetooth và gửi yêu cầu người dùng gửi SSID và Password qua BLE
        app_ble_start();
        
        // Reset trạng thái nhận dữ liệu BLE
        is_ssid_received = false;
        is_password_received = false;

        // Đăng ký callback nhận dữ liệu từ BLE
        app_ble_set_data_recv_callback(ble_data_received_callback);

        // Gửi phản hồi đến client
        httpd_resp_send(req, "Please send SSID and Password via Bluetooth.", HTTPD_RESP_USE_STRLEN);
        
        // Đóng NVS
        nvs_close(nvs_handle);
        
        return ESP_OK;
    }

    // Đóng NVS sau khi lưu chế độ
    nvs_close(nvs_handle);

    // Gửi phản hồi thành công đến client
    const char resp[] = "Mode updated. Ready to receive SSID and Password.";
    httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);

    return ESP_OK;
}
