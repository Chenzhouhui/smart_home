/*****************************************************************
 *  ESP8266-01S WiFiģ�飺
    VCC --> 3.3V(>=500mA)
    GND --> GND
    TX --> PA3(USART2_RX)
    RX --> PA2(USART2_TX)
 ****************************************************************/
#include "stm32f10x.h"                  // Device header
#include "ESP8266.h"
#include "misc.h"
#include <stdio.h>
#include <string.h>

static volatile uint16_t espRxIndex = 0;
static volatile uint8_t espRxBuffer[ESP8266_RX_BUFFER_SIZE];

static void ESP8266_SoftDelayMs(uint32_t ms)
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

static uint8_t ESP8266_BufferContains(const char *target)
{
   uint16_t len;

   if (target == 0)
   {
      return 1;
   }

   len = espRxIndex;
   if (len == 0)
   {
      return 0;
   }

   ((char *)espRxBuffer)[len] = '\0';
   if (strstr((char *)espRxBuffer, target) != 0)
   {
      return 1;
   }
   return 0;
}

static uint8_t ESP8266_WaitFor(const char *expect, uint32_t timeoutMs)
{
   while (timeoutMs--)
   {
      if (ESP8266_BufferContains(expect))
      {
         return 1;
      }
      if (ESP8266_BufferContains("ERROR"))
      {
         return 0;
      }
      ESP8266_SoftDelayMs(1);
   }
   return 0;
}

void ESP8266_Init(uint32_t baudrate)
{
   GPIO_InitTypeDef GPIO_InitStructure;
   USART_InitTypeDef USART_InitStructure;
   NVIC_InitTypeDef NVIC_InitStructure;

   RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
   RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);

   GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
   GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
   GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
   GPIO_Init(GPIOA, &GPIO_InitStructure);

   GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
   GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
   GPIO_Init(GPIOA, &GPIO_InitStructure);

   USART_InitStructure.USART_BaudRate = baudrate;
   USART_InitStructure.USART_WordLength = USART_WordLength_8b;
   USART_InitStructure.USART_StopBits = USART_StopBits_1;
   USART_InitStructure.USART_Parity = USART_Parity_No;
   USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
   USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
   USART_Init(USART2, &USART_InitStructure);

   USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);

   NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;
   NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 7;
   NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
   NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
   NVIC_Init(&NVIC_InitStructure);

   USART_Cmd(USART2, ENABLE);

   ESP8266_ClearRxBuffer();
}

void ESP8266_ClearRxBuffer(void)
{
   uint16_t i;
   for (i = 0; i < ESP8266_RX_BUFFER_SIZE; i++)
   {
      espRxBuffer[i] = 0;
   }
   espRxIndex = 0;
}

void ESP8266_SendByte(uint8_t data)
{
   USART_SendData(USART2, data);
   while (USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET);
}

void ESP8266_SendString(const char *str)
{
   while (*str != '\0')
   {
      ESP8266_SendByte((uint8_t)*str);
      str++;
   }
}

uint8_t ESP8266_SendAT(const char *cmd, const char *expect, uint32_t timeoutMs)
{
   ESP8266_ClearRxBuffer();
   ESP8266_SendString(cmd);
   ESP8266_SendString("\r\n");
   return ESP8266_WaitFor(expect, timeoutMs);
}

uint8_t ESP8266_Test(void)
{
   return ESP8266_SendAT("AT", "OK", 1000);
}

uint8_t ESP8266_SetModeStation(void)
{
   if (!ESP8266_SendAT("AT+CWMODE=1", "OK", 1000))
   {
      return 0;
   }
   return 1;
}

uint8_t ESP8266_ConnectWiFi(const char *ssid, const char *password, uint32_t timeoutMs)
{
   char cmd[160];

   if (!ESP8266_Test())
   {
      return 0;
   }

   ESP8266_SendAT("ATE0", "OK", 1000);

   if (!ESP8266_SetModeStation())
   {
      return 0;
   }

   snprintf(cmd, sizeof(cmd), "AT+CWJAP=\"%s\",\"%s\"", ssid, password);
   if (!ESP8266_SendAT(cmd, "WIFI GOT IP", timeoutMs))
   {
      if (!ESP8266_SendAT(cmd, "OK", timeoutMs))
      {
         return 0;
      }
   }

   return 1;
}

uint8_t ESP8266_StartTCP(const char *host, uint16_t port)
{
   char cmd[96];
   snprintf(cmd, sizeof(cmd), "AT+CIPSTART=\"TCP\",\"%s\",%u", host, port);
   if (ESP8266_SendAT(cmd, "CONNECT", 5000))
   {
      return 1;
   }
   if (ESP8266_SendAT(cmd, "ALREADY CONNECTED", 1000))
   {
      return 1;
   }
   return 0;
}

uint8_t ESP8266_SendText(const char *text)
{
   char cmd[32];
   uint16_t len = (uint16_t)strlen(text);

   snprintf(cmd, sizeof(cmd), "AT+CIPSEND=%u", len);
   if (!ESP8266_SendAT(cmd, ">", 2000))
   {
      return 0;
   }

   ESP8266_ClearRxBuffer();
   ESP8266_SendString(text);

   if (!ESP8266_WaitFor("SEND OK", 3000))
   {
      return 0;
   }
   return 1;
}

uint8_t ESP8266_SendRaw(const uint8_t *data, uint16_t len)
{
   char cmd[32];
   uint16_t i;
   uint32_t timeoutMs = 8000;

   if ((data == 0) || (len == 0))
   {
      return 0;
   }

   snprintf(cmd, sizeof(cmd), "AT+CIPSEND=%u", len);
   if (!ESP8266_SendAT(cmd, ">", 3000))
   {
      return 0;
   }

   ESP8266_ClearRxBuffer();
   for (i = 0; i < len; i++)
   {
      ESP8266_SendByte(data[i]);
   }

   while (timeoutMs--)
   {
      if (ESP8266_BufferContains("SEND OK"))
      {
         return 1;
      }
      if (ESP8266_BufferContains("Recv"))
      {
         return 1;
      }
      if (ESP8266_BufferContains("ERROR") || ESP8266_BufferContains("SEND FAIL"))
      {
         return 0;
      }
      ESP8266_SoftDelayMs(1);
   }

   return 0;
}

const char *ESP8266_GetRxBuffer(void)
{
   uint16_t len = espRxIndex;
   if (len >= (ESP8266_RX_BUFFER_SIZE - 1))
   {
      len = ESP8266_RX_BUFFER_SIZE - 1;
   }
   ((char *)espRxBuffer)[len] = '\0';
   return (const char *)espRxBuffer;
}

uint16_t ESP8266_GetRxLength(void)
{
   return espRxIndex;
}

uint16_t ESP8266_CopyRxBuffer(uint8_t *dst, uint16_t maxLen)
{
   uint16_t len;
   uint16_t i;

   if ((dst == 0) || (maxLen == 0))
   {
      return 0;
   }

   len = espRxIndex;
   if (len > maxLen)
   {
      len = maxLen;
   }

   for (i = 0; i < len; i++)
   {
      dst[i] = espRxBuffer[i];
   }
   return len;
}

void USART2_IRQHandler(void)
{
   uint8_t data;

   if (USART_GetITStatus(USART2, USART_IT_RXNE) != RESET)
   {
      data = (uint8_t)USART_ReceiveData(USART2);

      if (espRxIndex < (ESP8266_RX_BUFFER_SIZE - 1))
      {
         espRxBuffer[espRxIndex++] = data;
         espRxBuffer[espRxIndex] = '\0';
      }
      else
      {
         /* ?????????????????????? */
      }

      USART_ClearITPendingBit(USART2, USART_IT_RXNE);
   }
}


