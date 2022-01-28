#ifndef ESP8266_TRANSPORT_H_
#define ESP8266_TRANSPORT_H_

#include <stdint.h>
#include "esp8266_interface.h"

esp8266_wifi_status esp8266_uart_read(char *buffer, uint16_t bytes);
esp8266_wifi_status esp8266_uart_write(const char *buffer, uint16_t bytes);
void esp8266_uart_rx_callback_register(void *callback);

#endif
