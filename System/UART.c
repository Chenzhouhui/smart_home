/*
 留出一个串口来调试:USART1 PA9/PA10
 */
#include "stm32f10x.h"                  // Device header
#include "UART.h"
#include "misc.h"
#include <stdio.h>

static volatile uint8_t uartRxBuffer[UART_RX_BUFFER_SIZE];
static volatile uint16_t uartRxHead = 0;
static volatile uint16_t uartRxTail = 0;

static uint16_t UART_NextIndex(uint16_t index)
{
	index++;
	if (index >= UART_RX_BUFFER_SIZE)
	{
		index = 0;
	}
	return index;
}

void UART_Init(uint32_t baudrate)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_USART1, ENABLE);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	USART_InitStructure.USART_BaudRate = baudrate;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_Parity = USART_Parity_No;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;
	USART_Init(USART1, &USART_InitStructure);

	USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);

	NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 7;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);

	USART_Cmd(USART1, ENABLE);
}

void UART_SendByte(uint8_t byte)
{
	USART_SendData(USART1, byte);
	while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
}

void UART_SendArray(const uint8_t *array, uint16_t length)
{
	uint16_t i;
	for (i = 0; i < length; i++)
	{
		UART_SendByte(array[i]);
	}
}

void UART_SendString(const char *str)
{
	while (*str != '\0')
	{
		UART_SendByte((uint8_t)*str);
		str++;
	}
}

uint16_t UART_Available(void)
{
	if (uartRxHead >= uartRxTail)
	{
		return (uint16_t)(uartRxHead - uartRxTail);
	}
	return (uint16_t)(UART_RX_BUFFER_SIZE - uartRxTail + uartRxHead);
}

uint8_t UART_ReadByte(uint8_t *byte)
{
	if (uartRxHead == uartRxTail)
	{
		return 0;
	}

	*byte = uartRxBuffer[uartRxTail];
	uartRxTail = UART_NextIndex(uartRxTail);
	return 1;
}

uint16_t UART_Read(uint8_t *buffer, uint16_t length)
{
	uint16_t count = 0;
	while ((count < length) && UART_ReadByte(&buffer[count]))
	{
		count++;
	}
	return count;
}

uint8_t UART_ReadLine(char *line, uint16_t maxLen)
{
	uint16_t count = 0;
	uint16_t localTail = uartRxTail;
	uint8_t data;

	if (maxLen < 2)
	{
		return 0;
	}

	while (localTail != uartRxHead)
	{
		data = uartRxBuffer[localTail];
		localTail = UART_NextIndex(localTail);

		if (data == '\r')
		{
			continue;
		}

		if (data == '\n')
		{
			uartRxTail = localTail;
			line[count] = '\0';
			return 1;
		}

		if (count < (maxLen - 1))
		{
			line[count++] = (char)data;
		}
	}

	return 0;
}

void USART1_IRQHandler(void)
{
	uint16_t nextHead;
	uint8_t data;

	if (USART_GetITStatus(USART1, USART_IT_RXNE) != RESET)
	{
		data = (uint8_t)USART_ReceiveData(USART1);
		nextHead = UART_NextIndex(uartRxHead);

		if (nextHead != uartRxTail)
		{
			uartRxBuffer[uartRxHead] = data;
			uartRxHead = nextHead;
		}

		USART_ClearITPendingBit(USART1, USART_IT_RXNE);
	}
}

#ifndef __MICROLIB
#pragma import(__use_no_semihosting)
struct __FILE
{
	int handle;
};

FILE __stdout;

void _sys_exit(int x)
{
	(void)x;
}
#endif

int fputc(int ch, FILE *f)
{
	(void)f;
	UART_SendByte((uint8_t)ch);
	return ch;
}


