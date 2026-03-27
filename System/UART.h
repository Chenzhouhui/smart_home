/*
 USART1: PA9(TX) PA10(RX)
 */
#ifndef __UART_H
#define __UART_H

#include "stm32f10x.h"

#define UART_RX_BUFFER_SIZE 128

void UART_Init(uint32_t baudrate);

void UART_SendByte(uint8_t byte);
void UART_SendArray(const uint8_t *array, uint16_t length);
void UART_SendString(const char *str);

uint16_t UART_Available(void);
uint8_t UART_ReadByte(uint8_t *byte);
uint16_t UART_Read(uint8_t *buffer, uint16_t length);
uint8_t UART_ReadLine(char *line, uint16_t maxLen);

#endif
