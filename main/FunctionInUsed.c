#include"library.h"
#include"FunctionInUsed.h"

const char *TAG = "wifi"; // sử dụng một TAG cho ESP để có thể chuyển đổi giữa hai mode
int s_retry_num = 0;
static uint8_t mode =0;

QueueHandle_t mode_queue;
EventGroupHandle_t s_wifi_event_group;
static bool uart_initialized = false;
// Cập nhật hàm wifi_event_handler để xử lý cả hai chế độ AP và STA
void wifi_event_handler(void* arg, esp_event_base_t event_base,
                                    int32_t event_id, void* event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STACONNECTED) {
        wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
        ESP_LOGI(TAG, "station "MACSTR" join, AID=%d",
                 MAC2STR(event->mac), event->aid);
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
        ESP_LOGI(TAG, "station "MACSTR" leave, AID=%d, reason=%d",
                 MAC2STR(event->mac), event->aid, event->reason);
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START){
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED){
        if (s_retry_num < EXAMPLE_ESP_MAXIMUM_RETRY) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "retry to connect to the AP");
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
        ESP_LOGI(TAG,"connect to the AP fail");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

//HÀM xử lý tín hiệu từ UART
// Initialize UART only when needed
void initialize_uart_if_needed(void) {
    if (!uart_initialized) {
        const uart_config_t uart_config = {
            .baud_rate = 115200,
            .data_bits = UART_DATA_8_BITS,
            .parity    = UART_PARITY_DISABLE,
            .stop_bits = UART_STOP_BITS_1,
            .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        };
        uart_param_config(UART_NUM_1, &uart_config);
        uart_driver_install(UART_NUM_1, BUF_SIZE * 2, 0, 0, NULL, 0);
        uart_initialized = true;
    }
}

void uart_event_task(void *arg) {
    uint8_t *data = (uint8_t *) malloc(BUF_SIZE);
    while (1) {
        int len = uart_read_bytes(UART_NUM, data, BUF_SIZE, 20 / portTICK_PERIOD_MS);
        if (len > 0) {
            data[len] = '\0';  // Kết thúc chuỗi
            ESP_LOGI("UART", "Received DATA: %s", data);

            // Xử lý ký tự Y/y hoặc N/n
            if (data[0] == 'Y' || data[0] == 'y') {
                // Gửi thông báo đến queue là chế độ kết nối WiFi
                mode = 1; // 1 là chế độ STA
                xQueueSend(mode_queue, &mode, portMAX_DELAY);
            } else if (data[0] == 'N' || data[0] == 'n') {
                // Gửi thông báo đến queue là chế độ AP
                mode = 2; // 2 là chế độ AP
                xQueueSend(mode_queue, &mode, portMAX_DELAY);
            }
        }
        vTaskDelay(100 / portTICK_PERIOD_MS);  // Tránh làm bận CPU
    }
    free(data);
    // Đóng UART sau khi hoàn thành nhiệm vụ
    ESP_LOGI(TAG, "Đóng UART để giải phóng tài nguyên.");
    uart_driver_delete(UART_NUM_1);  // Gỡ bỏ UART driver
    uart_initialized = false;
    vTaskDelete(NULL);  // Kết thúc task
}
