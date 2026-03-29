#include "stm32f10x.h" // Device header
#include "freertos_demo.h"
#include "Delay.h"
#include "LED.h"
#include "lcd.h"
#include "AHT20.h"
#include "MQ.h"
#include "HC_SR501.h"
#include "UART.h"
#include <stdio.h>
int main(void)
{
	LED_Init();
	LCD_Init();
    AHT20_Init();
	//MQ_Init();
	HC_SR501_Init();
    UART_Init(115200);
    printf("System start, UART1 @115200\r\n");
	while (1)
	{
		freertos_demo();
	}
}

