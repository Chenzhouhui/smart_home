#include "stm32f10x.h" // Device header
#include "freertos_demo.h"
#include "Delay.h"
#include "LED.h"
int main(void)
{
	LED_Init();
	
	while (1)
	{
		freertos_demo();
	}
}

