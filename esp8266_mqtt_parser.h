#ifndef ESP8266_MQTT_PARSER_H_
#define ESP8266_MQTT_PARSER_H_

#include "esp8266_interface.h"

void esp8266_parse_mqtt_msgs(void);
esp8266_wifi_status esp8266_mqtt_msg_connect(void);
esp8266_wifi_status esp8266_mqt_msg_subscribe(void);
esp8266_wifi_status esp8266_mqt_msg_publish(void);

#endif
