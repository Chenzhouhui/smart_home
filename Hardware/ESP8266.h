/*****************************************************************
 *  ESP8266-01S WiFiģ�飺
    VCC --> 3.3V(>=500mA)
    GND --> GND
    TX --> PA3(USART2_RX)
    RX --> PA2(USART2_TX)
 ****************************************************************/
#ifndef __ESP8266_H
#define __ESP8266_H

#include "stm32f10x.h"

#define ESP8266_RX_BUFFER_SIZE 512

void ESP8266_Init(uint32_t baudrate);
void ESP8266_ClearRxBuffer(void);

void ESP8266_SendByte(uint8_t data);
void ESP8266_SendString(const char *str);

uint8_t ESP8266_SendAT(const char *cmd, const char *expect, uint32_t timeoutMs);
uint8_t ESP8266_Test(void);
uint8_t ESP8266_SetModeStation(void);
uint8_t ESP8266_ConnectWiFi(const char *ssid, const char *password, uint32_t timeoutMs);

uint8_t ESP8266_StartTCP(const char *host, uint16_t port);
uint8_t ESP8266_SendText(const char *text);
uint8_t ESP8266_SendRaw(const uint8_t *data, uint16_t len);
const char *ESP8266_GetRxBuffer(void);
uint16_t ESP8266_GetRxLength(void);
uint16_t ESP8266_CopyRxBuffer(uint8_t *dst, uint16_t maxLen);

#endif
