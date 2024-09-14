#ifndef AP_H_
#define AP_H_

#include"library.h"
#include"FunctionInUsed.h"
void wifi_init_softap(void);
static esp_err_t root_get_handler(httpd_req_t *req);
static esp_err_t setup_post_handler(httpd_req_t *req);
void start_webserver(void);
#endif 