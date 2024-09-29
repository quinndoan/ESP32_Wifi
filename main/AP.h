#ifndef AP_H_
#define AP_H_

#include"library.h"
#include"WifiHandle.h"
#include"Sta.h"
extern bool is_ssid_received;  // Khai báo extern cho is_ssid_received
extern bool is_password_received;  
void wifi_init_softap(void);
esp_err_t root_get_handler(httpd_req_t *req);
esp_err_t setup_post_handler(httpd_req_t *req);
void start_webserver(void);
esp_err_t mode_post_handler(httpd_req_t *req);
#endif 