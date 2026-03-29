#ifndef __ONENET_H
#define __ONENET_H

#include "stm32f10x.h"

typedef struct
{
	char topic[128];
	uint16_t topic_len;
	char payload[256];
	uint16_t payload_len;
} Mqtt_RxData_Type;

extern char MQTT_ClientID[100];
extern char MQTT_UserName[100];
extern char MQTT_PassWord[200];

void mqtt_login_init(char *ProductKey, char *DeviceName, char *DeviceSecret);
void mqtt_init(void);
uint8_t mqtt_connect(char *ClientID, char *Username, char *Password);
uint8_t mqtt_subscribe_topic(char *topic, uint8_t qos, uint8_t whether);
uint8_t mqtt_publish_data(char *topic, char *message, uint8_t qos);
uint8_t mqtt_receive_handle(uint8_t *data_received, Mqtt_RxData_Type *rx_data);
void mqtt_send_heart(void);
void mqtt_disconnect(void);

#define ONENET_MQTT_ERR_NONE         0
#define ONENET_MQTT_ERR_PARAM        1
#define ONENET_MQTT_ERR_USERCFG      2
#define ONENET_MQTT_ERR_CONNCFG      3
#define ONENET_MQTT_ERR_CONN         4

uint8_t OneNet_MQTT_GetLastError(void);
int8_t OneNet_MQTT_GetConnAckCode(void);
uint8_t OneNet_MQTT_IsRawMode(void);

uint8_t OneNet_MQTT_Connect(const char *host,
							uint16_t port,
							const char *clientId,
							const char *username,
							const char *password,
							uint16_t keepAliveSec);

uint8_t OneNet_MQTT_Publish(const char *topic, const char *payload, uint8_t qos, uint8_t retain);
uint8_t OneNet_MQTT_Subscribe(const char *topic, uint8_t qos);
uint8_t OneNet_MQTT_Ping(void);
void OneNet_MQTT_Disconnect(void);

#endif

