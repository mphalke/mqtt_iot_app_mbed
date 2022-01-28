#include <stdint.h>
#include <mbed.h>

#include "platform.h"

/* Create the ESP8266 UART instance */
UnbufferedSerial esp8266(D1, D0, 115200);
BufferedSerial pc(USBTX, USBRX, 115200);

#ifdef __cplusplus
extern "C"
{
#endif

platform_status uart_read(uart_types uart_type, char *buffer, uint16_t bytes)
{
	switch (uart_type) {
	case ESP8266_UART:
		if (esp8266.read(buffer, bytes) < 0)
			return PLATFORM_FAILURE;
		break;
		
	case CONSOLE_UART:
		// redirected to printf()/scanf()
	default :
		break;
	}
	
	return PLATFORM_SUCCESS;
}

platform_status uart_write(uart_types uart_type, const char *buffer, uint16_t bytes)
{
	switch (uart_type) {
	case ESP8266_UART:
		if (esp8266.write(buffer, bytes) < 0)
			return PLATFORM_FAILURE;
		break;
		
	case CONSOLE_UART:
		// redirected to printf()/scanf()
	default:
		break;
	}
	
	return PLATFORM_SUCCESS;
}

void uart_rx_callback_register(uart_types uart_type, uart_rx_callback *callback)
{
	switch (uart_type) {
	case ESP8266_UART:
		esp8266.attach(callback, UnbufferedSerial::RxIrq);
		break;
		
	case CONSOLE_UART:
	default:
		break;
	}
}

void delay_ms(uint32_t delay)
{
	HAL_Delay(delay);
}

#ifdef __cplusplus
}
#endif
