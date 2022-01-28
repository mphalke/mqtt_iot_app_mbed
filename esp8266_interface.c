#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "esp8266_interface.h"
#include "esp8266_transport.h"
#include "platform.h"

#define ESP8266_DEBUG
#define CMD_LEN(x)	sizeof(x)

volatile static char esp8266_data_buffer[1000];
volatile char esp8266_cmd[50];
volatile uint16_t esp8266_data_indx = 0;
volatile bool esp8266_resp_complete = false;
volatile char expected_resp[50] = { };
volatile uint8_t expected_resp_sz = 0;
volatile uint8_t expected_resp_indx = 0;

static esp8266_wifi_status esp8266_wifi_reset(void);
static esp8266_wifi_status esp8266_wifi_at_test(void);
static esp8266_wifi_status esp8266_wifi_mode_set(esp8266_wifi_mode mode);
static esp8266_wifi_status esp8266_wifi_set_ap(const char *ssid,
		const char *password);
static esp8266_wifi_status esp8266_wifi_single_conn(void);
static esp8266_wifi_status esp_8266_wifi_connect_server(const char *protocol,
		const char *remote_ip, const char *remote_port);
static esp8266_wifi_status esp8266_send_command(const char *command,
		uint16_t len);

/*
 *@brief ESP8266 UART Rx interrupt callback function
 *@return none
 **/
static void esp8266_uart_callback(void)
{
	char recv_ch;
	esp8266_uart_read(&recv_ch, 1);
	esp8266_data_buffer[esp8266_data_indx++] = recv_ch;

	if (!esp8266_resp_complete) {
		if (recv_ch == expected_resp[expected_resp_indx]) {
			expected_resp_indx++;
			if (expected_resp_indx >= expected_resp_sz) {
				esp8266_resp_complete = true;
				expected_resp_indx = 0;
				esp8266_data_indx = 0;
			}
		} else {
			if (expected_resp_indx > 0) {
				expected_resp_indx = 0;
			}
		}
	}
}

/*
 *@brief ESP8266 WiFi setup function
 *@return Wifi setup status (success/failure)
 **/
esp8266_wifi_status esp8266_wifi_setup(const char *ssid, const char *password,
				       const char *mqtt_broker_ip, const char *mqtt_broker_port)
{
	esp8266_uart_rx_callback_register(esp8266_uart_callback);

	printf("%s", "Resetting esp8266\r\n");
	if (esp8266_wifi_reset() == ESP8266_WIFI_SUCCESS) {
		printf("%s", "esp8266 reset success...\r\n");
	} else {
		printf("%s", "esp8266 reset failure!!\r\n");
		return ESP8266_WIFI_FAILURE;
	}

	printf("%s", "\r\nTesting esp8266 AT command\r\n");
	if (esp8266_wifi_at_test() == ESP8266_WIFI_SUCCESS) {
		printf("%s", "esp8266 AT test success...\r\n");
	} else {
		printf("%s", "esp8266 AT test failure!!\r\n");
		return ESP8266_WIFI_FAILURE;
	}

	printf("%s", "\r\nSetting esp8266 wifi mode\r\n");
	if (esp8266_wifi_mode_set(ESP8266_WIFI_STATION_MODE) == ESP8266_WIFI_SUCCESS) {
		printf("%s", "esp8266 wifi mode set success...\r\n");
	} else {
		printf("%s", "esp8266 wifi mode set failure!!\r\n");
		return ESP8266_WIFI_FAILURE;
	}

	printf("%s", "\r\nSetting esp8266 wifi AP\r\n");
	if (esp8266_wifi_set_ap(ssid, password) == ESP8266_WIFI_SUCCESS) {
		printf("%s", "esp8266 wifi AP set success...\r\n");
	} else {
		printf("%s", "esp8266 wifi AP set failure!!\r\n");
		return ESP8266_WIFI_FAILURE;
	}

	printf("%s", "\r\nSetting esp8266 single mode\r\n");
	if (esp8266_wifi_single_conn() == ESP8266_WIFI_SUCCESS) {
		printf("%s", "esp8266 wifi single mode set success...\r\n");
	} else {
		printf("%s", "esp8266 wifi single mode failure!!\r\n");
		return ESP8266_WIFI_FAILURE;
	}

	printf("%s", "\r\nConnecting to esp8266 wifi remote server\r\n");
	if (esp_8266_wifi_connect_server("TCP", mqtt_broker_ip,
					 mqtt_broker_port) == ESP8266_WIFI_SUCCESS) {
		printf("%s", "esp8266 wifi server connect success...\r\n");
	} else {
		printf("%s", "esp8266 wifi server connect failure!!\r\n");
		return ESP8266_WIFI_FAILURE;
	}

	printf("esp8266 wifi connect success...\r\n");
	return ESP8266_WIFI_SUCCESS;
}

esp8266_wifi_status esp8266_send_connect_msg(const char *mqtt_connect_msg,
		int msg_len)
{
	char len[4];
	esp8266_wifi_status status;

	memset(esp8266_data_buffer, 0, sizeof(esp8266_data_buffer));
	memset(esp8266_cmd, 0, sizeof(esp8266_cmd));

	/* Store the expected response and size for next command */
	strcpy((char *)expected_resp, ">");
	expected_resp_sz = strlen((char *)expected_resp);
	esp8266_resp_complete = false;
	expected_resp_indx = 0;
	esp8266_data_indx = 0;

	/* Form a command payload and its size */
	strcpy(esp8266_cmd, "AT+CIPSEND=");
	sprintf(len, "%d", msg_len);
	strcat(esp8266_cmd, len);
	strcat(esp8266_cmd, (const char *)"\r\n");

	/* Send command over esp8266 client */
	if (esp8266_send_command(esp8266_cmd,
				 strlen(esp8266_cmd)) != ESP8266_WIFI_SUCCESS) {
		return ESP8266_WIFI_FAILURE;
	}

	/* Wait until response is received */
	while (!esp8266_resp_complete) ;

	/* Store the expected response and size for next command */
	memset(esp8266_data_buffer, 0, sizeof(esp8266_data_buffer));
	strcpy((char *)expected_resp, "OK\r\n");
	expected_resp_sz = strlen((char *)expected_resp);
	esp8266_resp_complete = false;
	expected_resp_indx = 0;
	esp8266_data_indx = 0;

	/* Send the mqtt packet */
	status = esp8266_send_command(mqtt_connect_msg, msg_len);
	if (status != ESP8266_WIFI_SUCCESS) {
		return status;
	}

	/* Wait until response is received */
	while (!esp8266_resp_complete) ;

	return ESP8266_WIFI_SUCCESS;
}

esp8266_wifi_status esp8266_send_subscribe_msg(const char *mqtt_subscribe_msg,
		int msg_len, char *mqtt_response, int resp_len)
{
	char len[4];
	esp8266_wifi_status status;

	memset(esp8266_data_buffer, 0, sizeof(esp8266_data_buffer));
	memset(esp8266_cmd, 0, sizeof(esp8266_cmd));

	/* Store the expected response and size for next command */
	strcpy((char *)expected_resp, ">");
	expected_resp_sz = strlen((char *)expected_resp);
	esp8266_resp_complete = false;
	expected_resp_indx = 0;
	esp8266_data_indx = 0;

	/* Form a command payload and its size */
	strcpy(esp8266_data_buffer, "AT+CIPSEND=");
	sprintf(len, "%d", msg_len);
	strcat(esp8266_data_buffer, len);
	strcat(esp8266_data_buffer, (const char *)"\r\n");

	/* Send command over esp8266 client */
	status = esp8266_send_command(esp8266_data_buffer, strlen(esp8266_data_buffer));
	if (status != ESP8266_WIFI_SUCCESS) {
		return status;
	}

	/* Wait until response is received */
	while (!esp8266_resp_complete) ;

	/* Store the expected response and size for next command */
	memset(esp8266_data_buffer, 0, sizeof(esp8266_data_buffer));
	strcpy((char *)expected_resp, "OK\r\n");
	expected_resp_sz = strlen((char *)expected_resp);
	esp8266_resp_complete = false;
	expected_resp_indx = 0;
	esp8266_data_indx = 0;

	/* Send the mqtt packet */
	status = esp8266_send_command(mqtt_subscribe_msg, msg_len);
	if (status != ESP8266_WIFI_SUCCESS) {
		return status;
	}

	/* Wait until response is received */
	while (!esp8266_resp_complete) ;

	/* Store the expected response and size for next command */
	memset(esp8266_data_buffer, 0, sizeof(esp8266_data_buffer));
	strcpy((char *)expected_resp, "event/");
	expected_resp_sz = strlen((char *)expected_resp);
	esp8266_resp_complete = false;
	expected_resp_indx = 0;
	esp8266_data_indx = 0;

	/* Wait until response is received */
	while (!esp8266_resp_complete) ;

	delay_ms(10);
	memcpy(mqtt_response, esp8266_data_buffer, resp_len);

	return ESP8266_WIFI_SUCCESS;
}

esp8266_wifi_status esp8266_send_publish_msg(const char *mqtt_publish_msg,
		int msg_len)
{
	char len[4];
	esp8266_wifi_status status;

	memset(esp8266_data_buffer, 0, sizeof(esp8266_data_buffer));
	memset(esp8266_cmd, 0, sizeof(esp8266_cmd));

	/* Store the expected response and size for next command */
	strcpy((char *)expected_resp, "OK");
	expected_resp_sz = strlen((char *)expected_resp);
	esp8266_resp_complete = false;
	expected_resp_indx = 0;
	esp8266_data_indx = 0;

	/* Form a command payload and its size */
	strcpy(esp8266_data_buffer, "AT+CIPSEND=");
	sprintf(len, "%d", msg_len);
	strcat(esp8266_data_buffer, len);
	strcat(esp8266_data_buffer, (const char *)"\r\n");

	/* Send command over esp8266 client */
	status = esp8266_send_command(esp8266_data_buffer, strlen(esp8266_data_buffer));
	if (status != ESP8266_WIFI_SUCCESS) {
		return status;
	}

	/* Wait until response is received */
	while (!esp8266_resp_complete) ;

	/* Store the expected response and size for next command */
	memset(esp8266_data_buffer, 0, sizeof(esp8266_data_buffer));
	strcpy((char *)expected_resp, "OK\r\n");
	expected_resp_sz = strlen((char *)expected_resp);
	esp8266_resp_complete = false;
	expected_resp_indx = 0;
	esp8266_data_indx = 0;

	/* Send the mqtt packet */
	status = esp8266_send_command(mqtt_publish_msg, msg_len);
	if (status != ESP8266_WIFI_SUCCESS) {
		return status;
	}

	/* Wait until response is received */
	while (!esp8266_resp_complete) ;

	/* Store the expected response and size for next command */
	memset(esp8266_data_buffer, 0, sizeof(esp8266_data_buffer));
	strcpy((char *)expected_resp, "event/");
	expected_resp_sz = strlen((char *)expected_resp);
	esp8266_resp_complete = false;
	expected_resp_indx = 0;
	esp8266_data_indx = 0;

	/* Wait until response is received */
	while (!esp8266_resp_complete) ;

	return ESP8266_WIFI_SUCCESS;
}

/*
 *@brief ESP8266 WiFi remote reset function
 *@return Wifi reset status (success/failure)
 **/
static esp8266_wifi_status esp8266_wifi_reset(void)
{
	memset(esp8266_data_buffer, 0, sizeof(esp8266_data_buffer));

	/* Store the expected response and size for next command */
	strcpy((char *)expected_resp, "OK");
	expected_resp_sz = strlen((char *)expected_resp);
	esp8266_resp_complete = false;
	expected_resp_indx = 0;
	esp8266_data_indx = 0;

	/* Send the reset command */
	if (esp8266_send_command(ESP826_WIFI_RESET_CMD,
				 CMD_LEN(ESP826_WIFI_RESET_CMD)) != ESP8266_WIFI_SUCCESS) {
		return ESP8266_WIFI_FAILURE;
	}

	/* Wait until command is not complete */
	while (!esp8266_resp_complete) ;

	/* Wifi module reset delay_ms */
	delay_ms(5000);

#if defined(ESP8266_DEBUG)
	printf("%s", esp8266_data_buffer);
	delay_ms(2000);
#endif

	return ESP8266_WIFI_SUCCESS;
}

/*
 *@brief ESP8266 WiFi AT command test function
 *@return Wifi AT test status (success/failure)
 **/
static esp8266_wifi_status esp8266_wifi_at_test(void)
{
	memset(esp8266_data_buffer, 0, sizeof(esp8266_data_buffer));

	/* Store the expected response and size for next command */
	strcpy((char *)expected_resp, "OK");
	expected_resp_sz = strlen((char *)expected_resp);
	esp8266_resp_complete = false;
	expected_resp_indx = 0;
	esp8266_data_indx = 0;

	/* Send the AT test command */
	if (esp8266_send_command(ESP826_WIFI_AT_TEST_CMD,
				 CMD_LEN(ESP826_WIFI_AT_TEST_CMD)) != ESP8266_WIFI_SUCCESS) {
		return ESP8266_WIFI_FAILURE;
	}

	/* Wait until command is not complete */
	while (!esp8266_resp_complete) ;

#if defined(ESP8266_DEBUG)
	printf("%s", esp8266_data_buffer);
	delay_ms(2000);
#endif

	return ESP8266_WIFI_SUCCESS;
}

/*
 *@brief ESP8266 WiFi connection type set function
 *@return Wifi single connection set status (success/failure)
 **/
static esp8266_wifi_status esp8266_wifi_single_conn(void)
{
	memset(esp8266_data_buffer, 0, sizeof(esp8266_data_buffer));

	/* Store the expected response and size for next command */
	strcpy((char *)expected_resp, "OK");
	expected_resp_sz = strlen((char *)expected_resp);
	esp8266_resp_complete = false;
	expected_resp_indx = 0;
	esp8266_data_indx = 0;

	/* Send the single connection set command */
	if (esp8266_send_command(ESP826_WIFI_CIPMUX_CMD,
				 CMD_LEN(ESP826_WIFI_CIPMUX_CMD)) != ESP8266_WIFI_SUCCESS) {
		return ESP8266_WIFI_FAILURE;
	}

	/* Wait until command is not complete */
	while (!esp8266_resp_complete) ;

#if defined(ESP8266_DEBUG)
	printf("%s", esp8266_data_buffer);
	delay_ms(2000);
#endif

	return ESP8266_WIFI_SUCCESS;
}

/*
 *@brief ESP8266 WiFi mode set function
 *@return Wifi mode set status (success/failure)
 **/
static esp8266_wifi_status esp8266_wifi_mode_set(esp8266_wifi_mode mode)
{
	char esp8266_cmd[] = "AT+CWMODE= \r\n";

	switch (mode) {
	case ESP8266_WIFI_STATION_MODE:
		esp8266_cmd[strlen(ESP826_WIFI_CWMODE_CMD)-1] = '1';
		break;

	case ESP8266_WIFI_SOFT_AP_MODE:
		esp8266_cmd[strlen(ESP826_WIFI_CWMODE_CMD)-1] = '2';
		break;

	case ESP8266_WIFI_STATION_SOFT_AP_MODE:
		esp8266_cmd[strlen(ESP826_WIFI_CWMODE_CMD)-1] = '3';
		break;
	}

	memset(esp8266_data_buffer, 0, sizeof(esp8266_data_buffer));

	/* Store the expected response and size for next command */
	strcpy((char *)expected_resp, "OK");
	expected_resp_sz = strlen((char *)expected_resp);
	esp8266_resp_complete = false;
	expected_resp_indx = 0;
	esp8266_data_indx = 0;

	/* Send the mode change command */
	if(esp8266_send_command(esp8266_cmd,
				CMD_LEN(esp8266_cmd)) != ESP8266_WIFI_SUCCESS) {
		return ESP8266_WIFI_FAILURE;
	}

	/* Wait until command is not complete */
	while (!esp8266_resp_complete) ;

#if defined(ESP8266_DEBUG)
	printf("%s", esp8266_data_buffer);
	delay_ms(2000);
#endif

	return ESP8266_WIFI_SUCCESS;
}

/*
 *@brief ESP8266 WiFi access point set function
 *@return Wifi AP set status (success/failure)
 **/
static esp8266_wifi_status esp8266_wifi_set_ap(const char *ssid,
		const char *password)
{
	memset(esp8266_data_buffer, 0, sizeof(esp8266_data_buffer));

	/* Store the expected response and size for next command */
	strcpy((char *)expected_resp, "OK");
	expected_resp_sz = strlen((char *)expected_resp);
	esp8266_resp_complete = false;
	expected_resp_indx = 0;
	esp8266_data_indx = 0;

	/* Form the command fields */
	strcpy(esp8266_data_buffer, "AT+CWJAP=");
	strcat(esp8266_data_buffer, (const char *)"\"");
	strcat(esp8266_data_buffer, ssid);
	strcat(esp8266_data_buffer, (const char *)"\",\"");
	strcat(esp8266_data_buffer, password);
	strcat(esp8266_data_buffer, (const char *)"\"");
	strcat(esp8266_data_buffer, (const char *)"\r\n");

	/* Send the AP set command */
	if (esp8266_send_command(esp8266_data_buffer,
				 strlen(esp8266_data_buffer)) != ESP8266_WIFI_SUCCESS) {
		return ESP8266_WIFI_FAILURE;
	}

	/* Wait until command is not complete */
	while (!esp8266_resp_complete) ;

#if defined(ESP8266_DEBUG)
	printf("%s", esp8266_data_buffer);
	delay_ms(2000);
#endif

	return ESP8266_WIFI_SUCCESS;
}

/*
 *@brief ESP8266 WiFi server connect function
 *@return Wifi server connect status (success/failure)
 **/
static esp8266_wifi_status esp_8266_wifi_connect_server(const char *protocol,
		const char *remote_ip, const char *remote_port)
{
	memset(esp8266_data_buffer, 0, sizeof(esp8266_data_buffer));

	/* Store the expected response and size for next command */
	strcpy((char *)expected_resp, "OK");
	expected_resp_sz = strlen((char *)expected_resp);
	esp8266_resp_complete = false;
	expected_resp_indx = 0;
	esp8266_data_indx = 0;

	/* Form the command fields */
	strcpy(esp8266_data_buffer, "AT+CIPSTART=");
	strcat(esp8266_data_buffer, (const char *)"\"");
	strcat(esp8266_data_buffer, protocol);
	strcat(esp8266_data_buffer, (const char *)"\",\"");
	strcat(esp8266_data_buffer, remote_ip);
	strcat(esp8266_data_buffer, (const char *)"\",");
	strcat(esp8266_data_buffer, remote_port);
	strcat(esp8266_data_buffer, (const char *)"\r\n");

	/* Send the server connection establish command */
	if (esp8266_send_command(esp8266_data_buffer,
				 strlen(esp8266_data_buffer)) != ESP8266_WIFI_SUCCESS) {
		return ESP8266_WIFI_FAILURE;
	}

	/* Wait until command is not complete */
	while (!esp8266_resp_complete) ;

#if defined(ESP8266_DEBUG)
	printf("%s", esp8266_data_buffer);
	delay_ms(2000);
#endif

	return ESP8266_WIFI_SUCCESS;
}

static esp8266_wifi_status esp8266_send_command(const char *command,
		uint16_t len)
{
	if (esp8266_uart_write(command, len) < 0) {
		return ESP8266_WIFI_FAILURE;
	}

	return ESP8266_WIFI_SUCCESS;
}
