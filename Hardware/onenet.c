#include "onenet.h"
#include "ESP8266.h"
#include <stdio.h>
#include <string.h>

char MQTT_ClientID[100];
char MQTT_UserName[100];
char MQTT_PassWord[200];
static uint8_t g_OneNetMqttLastError = ONENET_MQTT_ERR_NONE;
static uint8_t g_MqttUseRawTcp = 0;
static volatile int8_t g_MqttConnAckCode = -1;
static uint8_t g_mqttPktBuf[512];
static uint8_t g_mqttTmpBuf[ESP8266_RX_BUFFER_SIZE];
static char g_mqttCmdBuf[512];

static void OneNet_DelayMs(uint32_t ms)
{
	volatile uint32_t i;
	while (ms--)
	{
		for (i = 0; i < 8000; i++)
		{
			__NOP();
		}
	}
}

static uint16_t MQTT_EncodeLength(uint8_t *buf, uint32_t length)
{
	uint16_t count = 0;
	do
	{
		uint8_t byte = length % 128;
		length /= 128;
		if (length > 0)
		{
			byte |= 0x80;
		}
		buf[count++] = byte;
	} while (length > 0);
	return count;
}

static uint8_t MQTT_WaitSubAck(uint16_t packetId, uint32_t timeoutMs)
{
	uint16_t len;
	uint16_t i;
	uint8_t idMsb = (uint8_t)(packetId >> 8);
	uint8_t idLsb = (uint8_t)(packetId & 0xFF);

	while (timeoutMs > 0)
	{
		len = ESP8266_CopyRxBuffer(g_mqttTmpBuf, sizeof(g_mqttTmpBuf));
		if (len >= 5)
		{
			for (i = 0; i + 4 < len; i++)
			{
				if ((g_mqttTmpBuf[i] == 0x90) &&
					(g_mqttTmpBuf[i + 1] == 0x03) &&
					(g_mqttTmpBuf[i + 2] == idMsb) &&
					(g_mqttTmpBuf[i + 3] == idLsb))
				{
					return 1;
				}
			}
		}

		if (strstr(ESP8266_GetRxBuffer(), "CLOSED") != 0)
		{
			return 0;
		}

		OneNet_DelayMs(20);
		if (timeoutMs >= 20)
		{
			timeoutMs -= 20;
		}
		else
		{
			timeoutMs = 0;
		}
	}

	return 0;
}

static void MQTT_WriteString(uint8_t *buf, uint16_t *index, const char *str)
{
	uint16_t len = (uint16_t)strlen(str);
	buf[(*index)++] = (uint8_t)(len >> 8);
	buf[(*index)++] = (uint8_t)(len & 0xFF);
	memcpy(&buf[*index], str, len);
	*index += len;
}

static uint8_t MQTT_TryGetConnAckCode(int8_t *ackCode)
{
	uint16_t len;
	uint16_t i;

	if (ackCode == 0)
	{
		return 0;
	}

	len = ESP8266_CopyRxBuffer(g_mqttTmpBuf, sizeof(g_mqttTmpBuf));
	if (len < 4)
	{
		return 0;
	}

	for (i = 0; i + 3 < len; i++)
	{
		if ((g_mqttTmpBuf[i] == 0x20) && (g_mqttTmpBuf[i + 1] == 0x02) && (g_mqttTmpBuf[i + 2] == 0x00))
		{
			*ackCode = (int8_t)g_mqttTmpBuf[i + 3];
			return 1;
		}
	}
	return 0;
}

static uint8_t OneNet_MQTT_ConnectRaw(const char *host,
									  uint16_t port,
									  const char *clientId,
									  const char *username,
									  const char *password,
									  uint16_t keepAliveSec)
{
	uint8_t lenField[4];
	uint16_t idx = 0;
	uint16_t remainLen;
	uint16_t lenBytes;
	uint16_t clientLen = (uint16_t)strlen(clientId);
	uint16_t userLen = (uint16_t)strlen(username);
	uint16_t passLen = (uint16_t)strlen(password);
	uint8_t retry;
	int8_t ackCode = -1;

	if (!ESP8266_StartTCP(host, port))
	{
		return 0;
	}

	remainLen = (uint16_t)(10 + 2 + clientLen + 2 + userLen + 2 + passLen);
	g_mqttPktBuf[idx++] = 0x10;
	lenBytes = MQTT_EncodeLength(lenField, remainLen);
	memcpy(&g_mqttPktBuf[idx], lenField, lenBytes);
	idx += lenBytes;

	MQTT_WriteString(g_mqttPktBuf, &idx, "MQTT");
	g_mqttPktBuf[idx++] = 0x04;
	g_mqttPktBuf[idx++] = 0xC2;
	g_mqttPktBuf[idx++] = (uint8_t)(keepAliveSec >> 8);
	g_mqttPktBuf[idx++] = (uint8_t)(keepAliveSec & 0xFF);

	MQTT_WriteString(g_mqttPktBuf, &idx, clientId);
	MQTT_WriteString(g_mqttPktBuf, &idx, username);
	MQTT_WriteString(g_mqttPktBuf, &idx, password);

	ESP8266_ClearRxBuffer();
	if (!ESP8266_SendRaw(g_mqttPktBuf, idx))
	{
		return 0;
	}

	for (retry = 0; retry < 80; retry++)
	{
		if (MQTT_TryGetConnAckCode(&ackCode))
		{
			g_MqttConnAckCode = ackCode;
			if (ackCode == 0)
			{
				return 1;
			}
			return 0;
		}
		if (strstr(ESP8266_GetRxBuffer(), "CLOSED") != 0)
		{
			return 0;
		}
		OneNet_DelayMs(100);
	}
	return 0;
}

uint8_t OneNet_MQTT_GetLastError(void)
{
	return g_OneNetMqttLastError;
}

int8_t OneNet_MQTT_GetConnAckCode(void)
{
	return g_MqttConnAckCode;
}

uint8_t OneNet_MQTT_IsRawMode(void)
{
	return g_MqttUseRawTcp;
}

void mqtt_login_init(char *ProductKey, char *DeviceName, char *DeviceSecret)
{
	if ((ProductKey == 0) || (DeviceName == 0) || (DeviceSecret == 0))
	{
		MQTT_ClientID[0] = '\0';
		MQTT_UserName[0] = '\0';
		MQTT_PassWord[0] = '\0';
		return;
	}

	snprintf(MQTT_ClientID, sizeof(MQTT_ClientID), "%s", DeviceName);
	snprintf(MQTT_UserName, sizeof(MQTT_UserName), "%s", ProductKey);
	snprintf(MQTT_PassWord, sizeof(MQTT_PassWord), "%s", DeviceSecret);
}

void mqtt_init(void)
{
	MQTT_ClientID[0] = '\0';
	MQTT_UserName[0] = '\0';
	MQTT_PassWord[0] = '\0';
	mqtt_disconnect();
}

uint8_t mqtt_connect(char *ClientID, char *Username, char *Password)
{
	if (OneNet_MQTT_Connect("broker.emqx.io", 1883, ClientID, Username, Password, 120))
	{
		return 0;
	}
	return 1;
}

uint8_t mqtt_subscribe_topic(char *topic, uint8_t qos, uint8_t whether)
{
	char cmd[192];

	if (topic == 0)
	{
		return 1;
	}

	if (whether)
	{
		if (OneNet_MQTT_Subscribe(topic, qos))
		{
			return 0;
		}
		return 1;
	}

	snprintf(cmd, sizeof(cmd), "AT+MQTTUNSUB=0,\"%s\"", topic);
	if (ESP8266_SendAT(cmd, "OK", 2000))
	{
		return 0;
	}
	return 1;
}

uint8_t mqtt_publish_data(char *topic, char *message, uint8_t qos)
{
	if (OneNet_MQTT_Publish(topic, message, qos, 0))
	{
		return 0;
	}
	return 1;
}

uint8_t mqtt_receive_handle(uint8_t *data_received, Mqtt_RxData_Type *rx_data)
{
	uint8_t *p;
	uint8_t encodeByte = 0;
	uint32_t multiplier = 1;
	uint32_t remainingLen = 0;
	uint8_t qosLevel = 0;

	if ((data_received == 0) || (rx_data == 0))
	{
		return 1;
	}

	p = data_received;
	memset(rx_data, 0, sizeof(Mqtt_RxData_Type));

	if ((*p != 0x30) && (*p != 0x32) && (*p != 0x34))
	{
		return 2;
	}

	if (*p != 0x30)
	{
		qosLevel = 1;
	}

	p++;
	do
	{
		encodeByte = *p++;
		remainingLen += (encodeByte & 0x7F) * multiplier;
		multiplier *= 128;
		if (multiplier > (128UL * 128UL * 128UL))
		{
			return 3;
		}
	} while ((encodeByte & 0x80) != 0);

	rx_data->topic_len = (uint16_t)(*p++ << 8);
	rx_data->topic_len |= *p++;
	if (rx_data->topic_len >= sizeof(rx_data->topic))
	{
		return 4;
	}
	memcpy(rx_data->topic, p, rx_data->topic_len);
	rx_data->topic[rx_data->topic_len] = '\0';
	p += rx_data->topic_len;

	if (qosLevel != 0)
	{
		p += 2;
	}

	rx_data->payload_len = (uint16_t)(remainingLen - rx_data->topic_len - 2 - (qosLevel ? 2 : 0));
	if (rx_data->payload_len >= sizeof(rx_data->payload))
	{
		return 5;
	}
	memcpy(rx_data->payload, p, rx_data->payload_len);
	rx_data->payload[rx_data->payload_len] = '\0';

	return 0;
}

void mqtt_send_heart(void)
{
	ESP8266_SendAT("AT+MQTTPING=0", "OK", 2000);
}

void mqtt_disconnect(void)
{
	OneNet_MQTT_Disconnect();
}

uint8_t OneNet_MQTT_Connect(const char *host,
							uint16_t port,
							const char *clientId,
							const char *username,
							const char *password,
							uint16_t keepAliveSec)
{
	const char *rsp;

	g_OneNetMqttLastError = ONENET_MQTT_ERR_NONE;
	g_MqttUseRawTcp = 0;
	g_MqttConnAckCode = -1;

	if ((host == 0) || (clientId == 0) || (username == 0) || (password == 0))
	{
		g_OneNetMqttLastError = ONENET_MQTT_ERR_PARAM;
		return 0;
	}

	snprintf(g_mqttCmdBuf,
			 sizeof(g_mqttCmdBuf),
			 "AT+MQTTUSERCFG=0,1,\"%s\",\"%s\",\"%s\",0,0,\"\"",
			 clientId,
			 username,
			 password);
	if (!ESP8266_SendAT(g_mqttCmdBuf, "OK", 2000))
	{
		if (!ESP8266_SendAT("AT+MQTTUSERCFG=0,1,\"\",\"\",\"\",0,0,\"\"", "OK", 2000))
		{
			if (!OneNet_MQTT_ConnectRaw(host, port, clientId, username, password, keepAliveSec))
			{
				g_OneNetMqttLastError = ONENET_MQTT_ERR_CONN;
				return 0;
			}
			g_MqttUseRawTcp = 1;
			g_OneNetMqttLastError = ONENET_MQTT_ERR_NONE;
			return 1;
		}

		snprintf(g_mqttCmdBuf,
				 sizeof(g_mqttCmdBuf),
				 "AT+MQTTLONGCLIENTID=0,\"%s\"",
				 clientId);
		if (!ESP8266_SendAT(g_mqttCmdBuf, "OK", 2000))
		{
			g_OneNetMqttLastError = ONENET_MQTT_ERR_USERCFG;
			return 0;
		}

		snprintf(g_mqttCmdBuf,
				 sizeof(g_mqttCmdBuf),
				 "AT+MQTTLONGUSERNAME=0,\"%s\"",
				 username);
		if (!ESP8266_SendAT(g_mqttCmdBuf, "OK", 2000))
		{
			g_OneNetMqttLastError = ONENET_MQTT_ERR_USERCFG;
			return 0;
		}

		snprintf(g_mqttCmdBuf,
				 sizeof(g_mqttCmdBuf),
				 "AT+MQTTLONGPASSWORD=0,\"%s\"",
				 password);
		if (!ESP8266_SendAT(g_mqttCmdBuf, "OK", 3000))
		{
			if (!OneNet_MQTT_ConnectRaw(host, port, clientId, username, password, keepAliveSec))
			{
				g_OneNetMqttLastError = ONENET_MQTT_ERR_CONN;
				return 0;
			}
			g_MqttUseRawTcp = 1;
			g_OneNetMqttLastError = ONENET_MQTT_ERR_NONE;
			return 1;
		}
	}

	snprintf(g_mqttCmdBuf,
			 sizeof(g_mqttCmdBuf),
			 "AT+MQTTCONNCFG=0,%u,0,\"\",\"\",0,0",
			 keepAliveSec);
	if (!ESP8266_SendAT(g_mqttCmdBuf, "OK", 3000))
	{
		g_OneNetMqttLastError = ONENET_MQTT_ERR_CONNCFG;
		return 0;
	}

	snprintf(g_mqttCmdBuf,
			 sizeof(g_mqttCmdBuf),
			 "AT+MQTTCONN=0,\"%s\",%u,1",
			 host,
			 port);
	if (!ESP8266_SendAT(g_mqttCmdBuf, "OK", 20000))
	{
		rsp = ESP8266_GetRxBuffer();
		if ((rsp == 0) || (strstr(rsp, "+MQTTCONNECTED") == 0))
		{
			g_OneNetMqttLastError = ONENET_MQTT_ERR_CONN;
			return 0;
		}
	}

	g_OneNetMqttLastError = ONENET_MQTT_ERR_NONE;
	return 1;
}

uint8_t OneNet_MQTT_Publish(const char *topic, const char *payload, uint8_t qos, uint8_t retain)
{
	uint8_t lenField[4];
	uint16_t idx = 0;
	uint16_t remainLen;
	uint16_t lenBytes;
	uint16_t topicLen;
	uint16_t payloadLen;

	if ((topic == 0) || (payload == 0))
	{
		return 0;
	}

	if (qos > 2)
	{
		qos = 0;
	}
	if (retain > 1)
	{
		retain = 0;
	}

	if (g_MqttUseRawTcp)
	{
		topicLen = (uint16_t)strlen(topic);
		payloadLen = (uint16_t)strlen(payload);
		remainLen = (uint16_t)(2 + topicLen + payloadLen);
		g_mqttPktBuf[idx++] = (uint8_t)(0x30 | ((qos & 0x03) << 1) | (retain ? 0x01 : 0x00));
		lenBytes = MQTT_EncodeLength(lenField, remainLen);
		memcpy(&g_mqttPktBuf[idx], lenField, lenBytes);
		idx += lenBytes;
		g_mqttPktBuf[idx++] = (uint8_t)(topicLen >> 8);
		g_mqttPktBuf[idx++] = (uint8_t)(topicLen & 0xFF);
		memcpy(&g_mqttPktBuf[idx], topic, topicLen);
		idx += topicLen;
		memcpy(&g_mqttPktBuf[idx], payload, payloadLen);
		idx += payloadLen;

		if (!ESP8266_SendRaw(g_mqttPktBuf, idx))
		{
			return 0;
		}
		return 1;
	}

	snprintf(g_mqttCmdBuf,
			 sizeof(g_mqttCmdBuf),
			 "AT+MQTTPUB=0,\"%s\",\"%s\",%u,%u",
			 topic,
			 payload,
			 qos,
			 retain);

	if (!ESP8266_SendAT(g_mqttCmdBuf, "OK", 5000))
	{
		return 0;
	}

	return 1;
}

uint8_t OneNet_MQTT_Subscribe(const char *topic, uint8_t qos)
{
	uint8_t lenField[4];
	uint16_t idx = 0;
	uint16_t topicLen;
	uint16_t remainLen;
	uint16_t lenBytes;
	static uint16_t s_packetId = 1;
	uint16_t packetId;

	if (topic == 0)
	{
		return 0;
	}

	if (qos > 2)
	{
		qos = 0;
	}

	if (g_MqttUseRawTcp)
	{
		topicLen = (uint16_t)strlen(topic);
		remainLen = (uint16_t)(2 + 2 + topicLen + 1);
		packetId = s_packetId++;

		g_mqttPktBuf[idx++] = 0x82;
		lenBytes = MQTT_EncodeLength(lenField, remainLen);
		memcpy(&g_mqttPktBuf[idx], lenField, lenBytes);
		idx += lenBytes;

		g_mqttPktBuf[idx++] = (uint8_t)(packetId >> 8);
		g_mqttPktBuf[idx++] = (uint8_t)(packetId & 0xFF);
		g_mqttPktBuf[idx++] = (uint8_t)(topicLen >> 8);
		g_mqttPktBuf[idx++] = (uint8_t)(topicLen & 0xFF);
		memcpy(&g_mqttPktBuf[idx], topic, topicLen);
		idx += topicLen;
		g_mqttPktBuf[idx++] = qos;

		ESP8266_ClearRxBuffer();
		if (!ESP8266_SendRaw(g_mqttPktBuf, idx))
		{
			return 0;
		}

		if (!MQTT_WaitSubAck(packetId, 3000))
		{
			return 0;
		}

		return 1;
	}

	snprintf(g_mqttCmdBuf,
			 sizeof(g_mqttCmdBuf),
			 "AT+MQTTSUB=0,\"%s\",%u",
			 topic,
			 qos);

	if (!ESP8266_SendAT(g_mqttCmdBuf, "OK", 5000))
	{
		return 0;
	}

	return 1;
}

uint8_t OneNet_MQTT_Ping(void)
{
	if (g_MqttUseRawTcp)
	{
		uint8_t pkt[2] = {0xC0, 0x00};
		if (!ESP8266_SendRaw(pkt, 2))
		{
			return 0;
		}
		return 1;
	}

	if (!ESP8266_SendAT("AT+MQTTPING=0", "OK", 3000))
	{
		return 0;
	}

	return 1;
}

void OneNet_MQTT_Disconnect(void)
{
	if (g_MqttUseRawTcp)
	{
		uint8_t pkt[2] = {0xE0, 0x00};
		ESP8266_SendRaw(pkt, 2);
	}
	else
	{
		ESP8266_SendAT("AT+MQTTCLEAN=0", "OK", 2000);
	}
}

