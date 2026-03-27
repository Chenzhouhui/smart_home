/********************************************************************************
    HC-SR501人体红外传感器：
    VCC --> 5V
    GND --> GND
    OUT --> PB10(输入/中断)
 *******************************************************************************/

#include "stm32f10x.h"
#include "HC_SR501.h"

void HC_SR501_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPD;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
}

uint8_t HC_SR501_ReadState(void)
{
    return (uint8_t)GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_10);
}

