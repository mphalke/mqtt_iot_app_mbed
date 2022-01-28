
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "esp8266_mqtt_parser.h"
#include "MQTTPacket.h"
#include "MQTTConnect.h"

static char mqtt_msg_buffer[500];

typedef enum {
	MQTT_MSG_SUBSCRIBE,
	MQTT_MSG_PUBLISH
} mqtt_msg_types;

typedef struct {
	uint8_t link_id;
	uint8_t version;
	uint8_t *name;
	uint16_t keep_alive;
	uint8_t *username;
	uint8_t *password;
} esp8266_wifi_mqtt_connect_config;

esp8266_wifi_mqtt_connect_config mqtt_connect_confg = {
	.link_id = 1,
	.version = 4,
	.name = (uint8_t *)"esp8266",
	.keep_alive = 1000,
	.username = (uint8_t *)"",
	.password = (uint8_t *)"",
};

typedef struct {
	uint8_t *mqtt_data;
	int mqtt_data_len;
	uint8_t dup_flag;
	uint16_t packet_id;
	uint8_t *topic[3];
	int qos[3];
	int counts;
} esp8266_mqtt_subscribe_config;

typedef struct {
	uint8_t *mqtt_data;
	int mqtt_data_len;
	uint8_t dup_flag;
	uint16_t packet_id;
	uint8_t *topic;
	int qos;
	int retained_flag;
} esp8266_mqtt_publish_config;

mqtt_msg_types mqtt_msg_type;

esp8266_mqtt_subscribe_config event_subscribe = {
	.mqtt_data = NULL,
	.mqtt_data_len = 0,
	.packet_id = 1,
	.dup_flag = 0,
	.qos = { 0, 0, 0 },
	.topic = { (uint8_t *)"event/logger", (uint8_t *)"event/temperature", (uint8_t *)"event/voltage" },
	.counts = 3,
};

typedef enum {
	LOGGER,
	TEMPERATURE,
	VOLTAGE
} publish_topics;

publish_topics publish_topic;

esp8266_mqtt_publish_config logger_publish = {
	.mqtt_data = NULL,
	.mqtt_data_len = 0,
	.packet_id = 0,
	.qos = 0,
	.retained_flag = 0,
	.topic = (uint8_t *)"event/logger"
};

void esp8266_parse_mqtt_msgs(void)
{
	switch (mqtt_msg_type) {
	case MQTT_MSG_SUBSCRIBE:
		esp8266_mqt_msg_subscribe();
		mqtt_msg_type = MQTT_MSG_PUBLISH;
		break;

	case MQTT_MSG_PUBLISH:
		esp8266_mqt_msg_publish();
		mqtt_msg_type = MQTT_MSG_SUBSCRIBE;
		break;

	default:
		break;
	}
}

esp8266_wifi_status esp8266_mqtt_msg_connect(void)
{
	int data_len;
	esp8266_wifi_status status;
	MQTTPacket_connectData data = MQTTPacket_connectData_initializer;

	data.MQTTVersion = mqtt_connect_confg.version;
	data.clientID.cstring = (char *)mqtt_connect_confg.name;
	data.keepAliveInterval = mqtt_connect_confg.keep_alive;
	data.cleansession = 1u;
	data.username.cstring = (char *)mqtt_connect_confg.username;
	data.password.cstring = (char *)mqtt_connect_confg.password;

	/* Serialize the mqtt packet */
	data_len = MQTTSerialize_connect((unsigned char *)mqtt_msg_buffer,
					 sizeof(mqtt_msg_buffer), &data);

	status = esp8266_send_connect_msg(mqtt_msg_buffer, data_len);
	if (status != ESP8266_WIFI_SUCCESS) {
		return status;
	}

	return ESP8266_WIFI_SUCCESS;
}

esp8266_wifi_status esp8266_mqt_msg_subscribe(void)
{
	int data_len;
	char resp_payload[20];
	esp8266_wifi_status status;

	MQTTString topic_filter[] = {
		{ (char *)event_subscribe.topic[0], {0, NULL} },
		{ (char *)event_subscribe.topic[1], {0, NULL} },
		{ (char *)event_subscribe.topic[2], {0, NULL} },
	};

	memset(mqtt_msg_buffer, 0, sizeof(mqtt_msg_buffer));

	/* Serialize the mqtt packet */
	data_len = MQTTSerialize_subscribe((unsigned char *)mqtt_msg_buffer, 500,
					   event_subscribe.dup_flag, event_subscribe.packet_id,
					   event_subscribe.counts, topic_filter, event_subscribe.qos);

	status = esp8266_send_subscribe_msg(mqtt_msg_buffer, data_len, resp_payload,
					    sizeof(resp_payload));
	if (status != ESP8266_WIFI_SUCCESS) {
		return status;
	}

	/* Parse the response and determine the subscribed topic */
	if (strstr(resp_payload, "logger")) {
		publish_topic = LOGGER;
	} else if (strstr(resp_payload, "temperature")) {
		publish_topic = TEMPERATURE;
	} else if (strstr(resp_payload, "voltage")) {
		publish_topic = VOLTAGE;
	} else {
		// do nothing
	}

	return ESP8266_WIFI_SUCCESS;
}

char *my_data[5] = { "100", "200", "300", "400", "500", };
u_int8_t my_data_indx;
MQTTString topicString;

esp8266_wifi_status esp8266_mqt_msg_publish(void)
{
	int data_len;
	esp8266_wifi_status status;

	switch (publish_topic) {
	case LOGGER:
		for (uint8_t i=0; i< 200; i++) {
			memset(mqtt_msg_buffer, 0, sizeof(mqtt_msg_buffer));

			/* Serialize the mqtt packet */
			topicString.cstring = (char *)logger_publish.topic;
			logger_publish.mqtt_data = (uint8_t *)my_data[my_data_indx];
			logger_publish.mqtt_data_len = strlen(my_data[my_data_indx]);
			my_data_indx++;
			if (my_data_indx > 5)
				my_data_indx = 0;

			data_len = MQTTSerialize_publish((unsigned char *)mqtt_msg_buffer,
							 500, 0u, logger_publish.qos, 0u,
							 logger_publish.packet_id, topicString,
							 (unsigned char *)logger_publish.mqtt_data,
							 logger_publish.mqtt_data_len);

			status = esp8266_send_publish_msg(mqtt_msg_buffer, data_len);
			if (status != ESP8266_WIFI_SUCCESS) {
				return status;
			}
		}
		break;

	case TEMPERATURE:
		break;

	case VOLTAGE:
		break;
	}

	return ESP8266_WIFI_SUCCESS;
}
