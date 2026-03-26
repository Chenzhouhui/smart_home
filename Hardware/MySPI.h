#include "stm32f10x.h" // Device header
#ifndef _MYSPI_H_
#define _MYSPI_H_

void MySPI_W_SS(uint8_t BitValue);
void MySPI_Init(void);
void MySPI_Start(void);
void MySPI_Stop(void);
uint8_t MySPI_WriteByte(SPI_TypeDef* SPIx,uint8_t Byte);



#endif
