#include "AP.h"

extern const char *TAG; // sử dụng một TAG cho ESP để có thể chuyển đổi giữa hai mode
extern EventGroupHandle_t s_wifi_event_group;

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
        .max_connection = EXAMPLE_MAX_STA_CONN,
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


// // Xử lý yêu cầu HTTP POST để cung cấp trang cấu hình WiFi
// esp_err_t root_get_handler(httpd_req_t *req) {
//     const char resp[] = "<!DOCTYPE html><html><body><form action=\"/setup\" method=\"post\">"
//                         "SSID: <input type=\"text\" name=\"ssid\"><br>"
//                         "Password: <input type=\"password\" name=\"password\"><br>"
//                         "<input type=\"submit\" value=\"Submit\"></form></body></html>";
//     httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
//     return ESP_OK;
// }

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
                        "}"
                        "h1 {"
                        "  text-align: center;"
                        "  color: #333;"
                        "}"
                        "input[type='text'], input[type='password'] {"
                        "  width: 100%;"
                        "  padding: 12px 20px;"
                        "  margin: 8px 0;"
                        "  display: inline-block;"
                        "  border: 1px solid #ccc;"
                        "  border-radius: 4px;"
                        "  box-sizing: border-box;"
                        "}"
                        "input[type='submit'] {"
                        "  width: 100%;"
                        "  background-color: #4CAF50;"
                        "  color: white;"
                        "  padding: 14px 20px;"
                        "  margin: 8px 0;"
                        "  border: none;"
                        "  border-radius: 4px;"
                        "  cursor: pointer;"
                        "}"
                        "input[type='submit']:hover {"
                        "  background-color: #45a049;"
                        "}"
                        "</style>"
                        "</head><body>"
                        "<div class=\"container\">"
                        "<h1>AIThingsLab</h1>"
                        "<form action=\"/setup\" method=\"post\">"
                        "SSID: <input type=\"text\" name=\"ssid\" placeholder=\"Enter SSID\"><br>"
                        "Password: <input type=\"password\" name=\"password\" placeholder=\"Enter Password\"><br>"
                        "<input type=\"submit\" value=\"Submit\">"
                        "</form></div></body></html>";
    httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}


// Xử lý yêu cầu HTTP POST để nhận và lưu thông tin WiFi mới
esp_err_t setup_post_handler(httpd_req_t *req) {
    char buf[100];
    int ret, remaining = req->content_len;

    while (remaining > 0) {
        if ((ret = httpd_req_recv(req, buf, MIN(remaining, sizeof(buf)))) <= 0) {
            if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
                continue;
            }
            return ESP_FAIL;
        }
        remaining -= ret;
    }

    buf[req->content_len] = '\0';

    char ssid[32] = {0};
    char password[64] = {0};
    sscanf(buf, "ssid=%[^&]&password=%s", ssid, password);

    // Lưu SSID và mật khẩu vào NVS
    nvs_handle_t nvs_handle;
    ESP_ERROR_CHECK(nvs_open("storage", NVS_READWRITE, &nvs_handle));
    ESP_ERROR_CHECK(nvs_set_str(nvs_handle, "ssid", ssid));
    ESP_ERROR_CHECK(nvs_set_str(nvs_handle, "password", password));
    ESP_ERROR_CHECK(nvs_commit(nvs_handle));
    nvs_close(nvs_handle);

    const char resp[] = "WiFi settings have been updated. Rebooting the device now...";
    httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);

    vTaskDelay(1000 / portTICK_PERIOD_MS); // Đợi 1 giây trước khi reboot để đảm bảo người dùng thấy thông báo
    esp_restart(); // Khởi động lại ESP32 để áp dụng thông tin WiFi mới

    return ESP_OK;
}

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
