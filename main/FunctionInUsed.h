#ifndef FUNCTION_IN_USED_H_
#define FUNCTION_IN_USED_H_

#include"library.h"

void wifi_event_handler(void* arg, esp_event_base_t event_base,
                                    int32_t event_id, void* event_data);
// void uart_event_task(void *arg);
// void initialize_uart_if_needed(void);
#endif