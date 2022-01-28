#include "esp8266_transport.h"
#include "platform.h"

esp8266_wifi_status esp8266_uart_read(char *buffer, uint16_t bytes)
{
	if (uart_read(ESP8266_UART, buffer, bytes) != PLATFORM_SUCCESS)
		return ESP8266_WIFI_FAILURE;
	
	return ESP8266_WIFI_SUCCESS;
}

esp8266_wifi_status esp8266_uart_write(const char *buffer, uint16_t bytes)
{
	if (uart_write(ESP8266_UART, buffer, bytes) != PLATFORM_SUCCESS)
		return ESP8266_WIFI_FAILURE;
	
	return ESP8266_WIFI_SUCCESS;
}

void esp8266_uart_rx_callback_register(void *callback)
{
	uart_rx_callback_register(ESP8266_UART, callback);
}

#if 0
esp8266_wifi_status esp_8266_mqtt_publish(void)
{
	topicString.cstring = (char *)mqtt_publish_config.topic;

	memset(esp8266_wifi_buffer, 0, sizeof(esp8266_wifi_buffer));
	memset(cmd, 0, sizeof(cmd));
	memset(mqtt_data_buffer, 0, sizeof(mqtt_data_buffer));
	strcpy((char *)expected_cmd, "OK");
	expected_cmd_sz = strlen((char *)expected_cmd);
	esp8266_cmd_complete = false;
	expected_cmd_indx = 0;
	esp8266_data_indx = 0;

	/* Serialize the mqtt packet */
	mqtt_publish_config.mqtt_data = (uint8_t *)my_data[my_data_indx++];
	if (my_data_indx > 5)
		my_data_indx = 0;
	
	data_len = MQTTSerialize_publish((unsigned char *)mqtt_data_buffer,
		500,
		0u,
		mqtt_publish_config.qos,
		0u,
		mqtt_publish_config.packet_id,
		topicString,
		(unsigned char *)mqtt_publish_config.mqtt_data,
		mqtt_publish_config.mqtt_data_len);

	/* Form CIPSEND command */
	strcpy(cmd, "AT+CIPSEND=");
	sprintf(len, "%d", data_len);
	strcat(cmd, len);
	strcat(cmd, (const char *)"\r\n");

	/* Send the CIPSEND command */
	if (esp8266_send_command(cmd,
		strlen(cmd)) != ESP8266_WIFI_SUCCESS) {
		return ESP8266_WIFI_FAILURE;
	}

	/* Wait until command is not complete */
	while (!esp8266_cmd_complete) ;

	memset(esp8266_wifi_buffer, 0, sizeof(esp8266_wifi_buffer));

	strcpy((char *)expected_cmd, "OK");
	expected_cmd_sz = strlen((char *)expected_cmd);
	esp8266_cmd_complete = false;
	expected_cmd_indx = 0;
	esp8266_data_indx = 0;

	/* Send the mqtt packet */
	if (esp8266_send_command(mqtt_data_buffer,
		data_len) != ESP8266_WIFI_SUCCESS) {
		return ESP8266_WIFI_FAILURE;
	}

	/* Wait until command is not complete */
	while (!esp8266_cmd_complete) ;

#if defined(ESP8266_DEBUG)
	//printf("%s", esp8266_wifi_buffer);
#endif

	return ESP8266_WIFI_SUCCESS;
}
#endif
