#ifndef APPMAIN_H_
#define APPMAIN_H_

#include"library.h"
// Cập nhật hàm wifi_event_handler để xử lý cả hai chế độ AP và STA
void app_main(void);
bool check_saved_wifi(char *ssid, char *password);

#endif
