#include "stm32f10x.h"                  // Device header
#include "AHT20.h"
#include "MyI2C.h"

#define AHT20_ADDRESS 0x70  // 从机地址+写

static void AHT20_LocalDelayMs(uint32_t ms)
{
    volatile uint32_t i,j;
    for(i = 0; i < ms; i++)
    {
        for(j = 0; j < 8000; j++)
        {
            __NOP();
        }
    }
}
/********************************************************************
* 参数: 命令，数据0，数据1
* 函数功能: 通用 AHT20写入 [命令+两字节]
********************************************************************/
void AHT20_WriteHalfword(uint8_t Command, uint8_t Data0, uint8_t Data1)
{
    MyI2C_Start();//开始
    
    MyI2C_SendByte(AHT20_ADDRESS);//从机地址+写
    MyI2C_ReceiveAck();//应答
    
    MyI2C_SendByte(Command);//命令
    MyI2C_ReceiveAck();//应答
    
    MyI2C_SendByte(Data0);//写入Data0
    MyI2C_ReceiveAck();//应答
    
    MyI2C_SendByte(Data1);//写入Data1
    MyI2C_ReceiveAck();//应答
    
    MyI2C_Stop();//停止
}

/********************************************************************
* 参数: 命令
* 函数功能: 通用 AHT20写入 [命令]
********************************************************************/
void AHT20_WriteCommand(uint8_t Command)
{
    MyI2C_Start();    //开始
    
    MyI2C_SendByte(AHT20_ADDRESS);//从机地址+写
    MyI2C_ReceiveAck();//应答
    
    MyI2C_SendByte(Command);//命令
    MyI2C_ReceiveAck();//应答
    
    MyI2C_Stop();//停止
}

/********************************************************************
* 参数: 无
* 函数功能: 读取 AHT20 状态
********************************************************************/
uint8_t AHT20_ReadState(void)
{
    uint8_t State = 0;
    
    MyI2C_Start();//开始
    
    MyI2C_SendByte(AHT20_ADDRESS | 0x01);//从机地址+读
    MyI2C_ReceiveAck();//接收应答
    
    State = MyI2C_ReceiveByte();//接收状态
    MyI2C_SendAck(1);//发送非应答
    
    MyI2C_Stop();//停止
    return State;
}

/********************************************************************
* 参数: 无
* 函数功能: AHT20 写入[初始化 寄存器指令]
********************************************************************/
void AHT20_WriteInit(void)
{
    AHT20_WriteHalfword(0xBE, 0x08, 0x00);
}

/********************************************************************
* 参数: 无
* 函数功能: 写入[温湿度触发测量指令]给AHT20
********************************************************************/
void AHT20_WriteMeasure(void)
{
    AHT20_WriteHalfword(0xAC, 0x33, 0x00);
}

/********************************************************************
* 参数: 无
* 函数功能: 软复位AHT20
********************************************************************/
void AHT20_WriteReset(void)
{
    AHT20_WriteCommand(0xBA);
}

///********************************************************************
//* 函数功能: 幂函数（num, power）
//********************************************************************/
//uint32_t AHT20_Pow(uint32_t Num, uint32_t Power)
//{
//    uint32_t Result = 1;
//    while(Power--)
//    {
//        Result *= Num;
//    }
//    return Result;
//}

/********************************************************************
* 参数: 无
* 函数功能: 发送测量命令，读取温度、湿度
* 返回值: TH数组
********************************************************************/
float* AHT20_ReadTemperatureHumidity(void)
{
    uint8_t Array[6] = {0};         //定义一个6字节数据的数组 存放字节
    //uint32_t Temp = 0;              //定义32位数据 暂时存放20位数据
    //uint32_t Result[2] = {0};       //定义一个2位数据的数组 存放数据结果
    static float TH[2] = {0};       //静态变量，返回最终数值
    
    AHT20_WriteMeasure();//发送测量命令
    AHT20_LocalDelayMs(80);
    
    MyI2C_Start();//开始
    
    MyI2C_SendByte(AHT20_ADDRESS | 0x01);    //从机地址+读
    MyI2C_ReceiveAck();
    
    Array[0] = MyI2C_ReceiveByte();          //状态字
    MyI2C_SendAck(0);//发送应答
    
    Array[1] = MyI2C_ReceiveByte();          //湿度
    MyI2C_SendAck(0);//发送应答
    Array[2] = MyI2C_ReceiveByte();          //湿度
    MyI2C_SendAck(0);//发送应答
    Array[3] = MyI2C_ReceiveByte();          //湿度和温度
    MyI2C_SendAck(0);//发送应答
    Array[4] = MyI2C_ReceiveByte();          //温度
    MyI2C_SendAck(0);//发送应答
    Array[5] = MyI2C_ReceiveByte();          //温度
    MyI2C_SendAck(1);//不发送应答，结束
    MyI2C_Stop();
	
	/*此部分代码用于调试
	OLED_ShowString(3, 1, "H:");
    OLED_ShowHexNum(3, 3, Array[1], 2);  // 湿度字节 H1
    OLED_ShowHexNum(3, 6, Array[2], 2);  // 湿度字节 H2
    
    // 在OLED第4行显示混合字节和温度字节
    OLED_ShowString(4, 1, "T:");
    OLED_ShowHexNum(4, 3, Array[4], 2);  // 温度字节 T1
    OLED_ShowHexNum(4, 6, Array[5], 2);  // 温度字节 T2
    
    // 同时把状态字节和混合字节也显示出来
    OLED_ShowString(3, 9, "S:");
    OLED_ShowHexNum(3, 11, Array[0], 2); // 状态字节
    OLED_ShowString(4, 9, "M:");
    OLED_ShowHexNum(4, 11, Array[3], 2); // 混合字节 (包含部分湿度和温度)
	
    */
    if((Array[0] & 0x80) == 0x00) //如果测量完成 bit[7]=0，进行数据处理
    {
		uint32_t rawHumidity = 0;
		uint32_t rawTemperature = 0;

		// 正确地处理湿度数据（20位）
		rawHumidity = ((uint32_t)Array[1] << 12) | ((uint32_t)Array[2] << 4) | ((uint32_t)Array[3] >> 4);
		
		// 正确地处理温度数据（20位）
		rawTemperature = (((uint32_t)Array[3] & 0x0F) << 16) | ((uint32_t)Array[4] << 8) | ((uint32_t)Array[5]);
    
		// 转换为实际值
		// 使用 (1UL << 20) 
		TH[0] = (float)rawHumidity / (1UL << 20) * 100;      // 湿度
		TH[1] = (float)rawTemperature / (1UL << 20) * 200 - 50; // 温度

    }
    else
    {
        return (float*)0x00000000; // 测量未完成，返回空指针
    }
    return TH;
}

/********************************************************************
* 参数: 无
* 函数功能: AHT20初始化
********************************************************************/
void AHT20_Init(void)
{
    uint8_t State = 0;

    MyI2C_Init();
    AHT20_LocalDelayMs(40);

    //1. 上电读取状态
    State = AHT20_ReadState();//读取状态
    //2. 判断状态位 bit[3] 0x04/未初始化寄存器 0x0C/已初始化寄存器
    if((State & 0x0C) != 0x0C)//不等于0000 0100，进行初始化寄存器
    {
        AHT20_WriteInit();//如果第三位是0，则初始化寄存器
        AHT20_LocalDelayMs(10);
    }
}

/********************************************************************
* 参数: 无
* 函数功能: BH1750初始化（ADDR接GND,地址0x23）
********************************************************************/
void BH1750_Init(void)
{
    MyI2C_Init();

    MyI2C_Start();
    MyI2C_SendByte(0x46);         // 0x23写地址
    MyI2C_ReceiveAck();
    MyI2C_SendByte(0x01);         // Power On
    MyI2C_ReceiveAck();
    MyI2C_Stop();

    AHT20_LocalDelayMs(10);

    MyI2C_Start();
    MyI2C_SendByte(0x46);         // 0x23写地址
    MyI2C_ReceiveAck();
    MyI2C_SendByte(0x10);         // Continuously H-Resolution Mode
    MyI2C_ReceiveAck();
    MyI2C_Stop();

    AHT20_LocalDelayMs(180);
}

/********************************************************************
* 参数: 无
* 函数功能: 读取BH1750光照（lx）
********************************************************************/
float BH1750_ReadLux(void)
{
    uint8_t high, low;
    uint16_t raw;

    MyI2C_Start();
    MyI2C_SendByte(0x47);         // 0x23读地址
    MyI2C_ReceiveAck();

    high = MyI2C_ReceiveByte();
    MyI2C_SendAck(0);
    low = MyI2C_ReceiveByte();
    MyI2C_SendAck(1);
    MyI2C_Stop();

    raw = ((uint16_t)high << 8) | low;
    return (float)raw / 1.2f;
}

