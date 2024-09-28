#ifndef WIFI_HANDLE_H_
#define WIFI_HANDLE_H_

#include"library.h"

void wifi_event_handler(void* arg, esp_event_base_t event_base,
                                    int32_t event_id, void* event_data);
// void uart_event_task(void *arg);
// void initialize_uart_if_needed(void);
#endif