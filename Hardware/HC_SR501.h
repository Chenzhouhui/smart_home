 #ifndef __HC_SR501_H
 #define __HC_SR501_H

/********************************************************************************
    HC-SR501人体红外传感器：
    VCC --> 5V
    GND --> GND
    OUT --> PB10(输入/中断)
 *******************************************************************************/

#include "stm32f10x.h" // Device header

void HC_SR501_Init(void);
uint8_t HC_SR501_ReadState(void);

#endif



