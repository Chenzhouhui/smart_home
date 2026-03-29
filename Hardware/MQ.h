#ifndef __MQ_H
#define __MQ_H

#include "stm32f10x.h"

void MQ_Init(void);

uint8_t MQ4_ReadDO(void);
uint8_t MQ7_ReadDO(void);


#endif

