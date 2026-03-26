#include "stm32f10x.h" // Device header	
#include "MySPI.h"

/* 
    1.44 TFT 彩屏：
    VCC --> 3.3V(建议)
    GND --> GND
    CS --> PA4
    RESET --> PB13(GPIO输出)
    RS(DC) --> PB12(GPIO输出)
    SDI(MOSI) --> PA7(SPI1_MOSI)
    SCK --> PA5(SPI1_SCK)
    LED --> PB14(TIM1_CH2 PWM调光)
 */
void MySPI_W_SS(uint8_t BitValue)
{
    GPIO_WriteBit(GPIOA, GPIO_Pin_4, (BitAction)BitValue);
}
void MySPI_Init(void)
{
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1, ENABLE);
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5 | GPIO_Pin_7; // SCK和MOSI
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP; // 复用推挽 
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4; // CS
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; // 推挽输出    
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    SPI_InitTypeDef SPI_InitStructure;
    SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
    SPI_InitStructure.SPI_Mode = SPI_Mode_Master;
    SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;
    SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low;
    SPI_InitStructure.SPI_CPHA = SPI_CPHA_1Edge;
    SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;
    SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_16;
    SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;
    SPI_InitStructure.SPI_CRCPolynomial = 7;
    SPI_Init(SPI1, &SPI_InitStructure);
    SPI_Cmd(SPI1, ENABLE);
    MySPI_W_SS(1); 
}

void MySPI_Start(void)
{
    MySPI_W_SS(0); 
}
void MySPI_Stop(void)
{
    MySPI_W_SS(1); 
}

uint8_t MySPI_WriteByte(SPI_TypeDef* SPIx,uint8_t Byte)
{
    while (SPI_I2S_GetFlagStatus(SPIx, SPI_I2S_FLAG_TXE) == RESET); // 等待发送缓冲区空
    SPI_I2S_SendData(SPIx, Byte); // 发送数据
    while (SPI_I2S_GetFlagStatus(SPIx, SPI_I2S_FLAG_RXNE) == RESET); // 等待接收缓冲区非空
    return SPI_I2S_ReceiveData(SPIx); // 返回接收到的数据
}

