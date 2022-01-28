#ifndef PLATFORM_H_
#define PLATFORM_H_

#ifdef __cplusplus
extern "C"
{
#endif

typedef enum {
	ESP8266_UART,
	CONSOLE_UART
} uart_types;

typedef enum {
	PLATFORM_SUCCESS,
	PLATFORM_FAILURE
} platform_status;

typedef void uart_rx_callback(void);

void delay_ms(uint32_t delay);
platform_status uart_read(uart_types uart_type, char *buffer, uint16_t bytes);
platform_status uart_write(uart_types uart_type, const char *buffer,
			   uint16_t bytes);
void uart_rx_callback_register(uart_types uart_type,
			       uart_rx_callback *callback);

#ifdef __cplusplus
}
#endif

#endif
