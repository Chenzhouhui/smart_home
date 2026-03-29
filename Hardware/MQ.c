#include "stm32f10x.h" // Device header 
#include "MQ.h"
/******************************************************************************************************
1)MQ-4甲烷传感器：
VCC --> 5V
GND --> GND
 DO --> PB0(输入/中断,需3.3V)
------------------------------------------------
2)MQ-7一氧化碳传感器：
VCC --> 5V
GND --> GND
DO --> PB1(输入/中断,需3.3V)
******************************************************************************************************/

void MQ_Init(void)
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);

	GPIO_InitTypeDef GPIO_InitStructure;

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
}

uint8_t MQ4_ReadDO(void)
{
	return (uint8_t)GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_0);
}

uint8_t MQ7_ReadDO(void)
{
	return (uint8_t)GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_1);
}







