#ifndef __MQ_H
#define __MQ_H

#include "stm32f10x.h"

void MQ_Init(void);

uint8_t MQ4_ReadDO(void);
uint8_t MQ7_ReadDO(void);

uint16_t MQ4_ReadAO(void);
uint16_t MQ7_ReadAO(void);

uint16_t MQ4_ReadAO_Avg(uint8_t times);
uint16_t MQ7_ReadAO_Avg(uint8_t times);

float MQ4_ReadPPM_FromAO(uint16_t adc);
float MQ7_ReadPPM_FromAO(uint16_t adc);

void MQ4_SetR0(float r0_kohm);
void MQ7_SetR0(float r0_kohm);
float MQ4_GetR0(void);
float MQ7_GetR0(void);


#endif

