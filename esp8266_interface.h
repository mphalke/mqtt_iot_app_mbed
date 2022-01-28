#ifndef ESP8266_INTERFACE_H_
#define ESP8266_INTERFACE_H_

/* ESP8266 Wifi AT commands */
#define ESP826_WIFI_RESET_CMD		"AT+RST\r\n"
#define ESP826_WIFI_AT_TEST_CMD		"AT\r\n"
#define ESP826_WIFI_CWMODE_CMD		"AT+CWMODE= "
#define ESP826_WIFI_CIPMUX_CMD		"AT+CIPMUX=0\r\n"
#define ESP826_WIFI_CIFSR_CMD		"AT+CIFSR\r\n"

typedef enum {
	ESP8266_WIFI_SUCCESS,
	ESP8266_WIFI_FAILURE
} esp8266_wifi_status;

typedef enum {
	ESP8266_WIFI_STATION_MODE,
	ESP8266_WIFI_SOFT_AP_MODE,
	ESP8266_WIFI_STATION_SOFT_AP_MODE,
} esp8266_wifi_mode;

typedef enum {
	TCP,
	UDP,
	SSL,
} esp8266_wifi_protocol;

esp8266_wifi_status esp8266_wifi_setup(const char *ssid, const char *password,
				       const char *mqtt_broker_ip, const char *mqtt_broker_port);
esp8266_wifi_status esp8266_send_connect_msg(const char *mqtt_connect_msg,
		int msg_len);
esp8266_wifi_status esp8266_send_subscribe_msg(const char *mqtt_subscribe_msg,
		int msg_len, char *mqtt_response, int resp_len);
esp8266_wifi_status esp8266_send_publish_msg(const char *mqtt_publish_msg,
		int msg_len);

#endif
