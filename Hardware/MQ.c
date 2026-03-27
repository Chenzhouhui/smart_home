#include "stm32f10x.h" // Device header 
#include "MQ.h"
#include <math.h>
/******************************************************************************************************
1)MQ-4甲烷传感器：
VCC --> 5V
GND --> GND
 DO --> PB0(输入/中断,需3.3V)
AO --> PA0(ADC1_IN0,需分压)
------------------------------------------------
2)MQ-7一氧化碳传感器：
VCC --> 5V
GND --> GND
DO --> PB1(输入/中断,需3.3V)
AO --> PA1(ADC1_IN1,需分压)
******************************************************************************************************/

#define ADC_FULL_SCALE               4095.0f
#define ADC_REF_VOLTAGE              3.3f

/* RL按你给的资料默认值设置，可根据你模块板上电阻实测修改 */
#define MQ4_RL_KOHM                  20.0f
#define MQ7_RL_KOHM                  10.0f

/* 这两个R0是默认值，建议后续按标气或已知环境再标定 */
static float g_mq4_r0_kohm = 10.0f;
static float g_mq7_r0_kohm = 10.0f;

/*
 * MQ-4（按你给的曲线，目标气体CH4）
 * 采用两点近似：
 *   200ppm -> Rs/R0≈1.6
 *  1000ppm -> Rs/R0≈1.0
 * log10(Rs/R0) = m*log10(ppm) + b
 */
#define MQ4_CH4_M                    (-0.291069f)
#define MQ4_CH4_B                    (0.873207f)

/*
 * MQ-7（按你给的曲线，目标气体CO）
 * 采用两点近似：
 *   100ppm -> Rs/R0≈1.0
 *  1000ppm -> Rs/R0≈0.5
 */
#define MQ7_CO_M                     (-0.301030f)
#define MQ7_CO_B                     (0.602060f)

static float MQ_ADC_ToVoltage(uint16_t adc)
{
	return ((float)adc / ADC_FULL_SCALE) * ADC_REF_VOLTAGE;
}

static float MQ_Voltage_ToRs(float vout, float rl_kohm)
{
	if (vout <= 0.001f)
	{
		vout = 0.001f;
	}
	if (vout >= (ADC_REF_VOLTAGE - 0.001f))
	{
		vout = ADC_REF_VOLTAGE - 0.001f;
	}
	return rl_kohm * (ADC_REF_VOLTAGE - vout) / vout;
}

static float MQ_Ratio_ToPPM(float ratio, float m, float b)
{
	float log_ratio;
	float log_ppm;

	if (ratio <= 0.0001f)
	{
		ratio = 0.0001f;
	}

	log_ratio = log10f(ratio);
	log_ppm = (log_ratio - b) / m;
	return powf(10.0f, log_ppm);
}

static uint16_t MQ_ReadADC_Channel(uint8_t channel)
{
	ADC_RegularChannelConfig(ADC1, channel, 1, ADC_SampleTime_239Cycles5);
	ADC_SoftwareStartConvCmd(ADC1, ENABLE);
	while (ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC) == RESET);
	return ADC_GetConversionValue(ADC1);
}

void MQ_Init(void)
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB | RCC_APB2Periph_ADC1, ENABLE);

	GPIO_InitTypeDef GPIO_InitStructure;

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	ADC_DeInit(ADC1);

	ADC_InitTypeDef ADC_InitStructure;
	ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;
	ADC_InitStructure.ADC_ScanConvMode = DISABLE;
	ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;
	ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
	ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
	ADC_InitStructure.ADC_NbrOfChannel = 1;
	ADC_Init(ADC1, &ADC_InitStructure);

	ADC_Cmd(ADC1, ENABLE);

	ADC_ResetCalibration(ADC1);
	while (ADC_GetResetCalibrationStatus(ADC1) == SET);
	ADC_StartCalibration(ADC1);
	while (ADC_GetCalibrationStatus(ADC1) == SET);
}

uint8_t MQ4_ReadDO(void)
{
	return (uint8_t)GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_0);
}

uint8_t MQ7_ReadDO(void)
{
	return (uint8_t)GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_1);
}

uint16_t MQ4_ReadAO(void)
{
	return MQ_ReadADC_Channel(ADC_Channel_0);
}

uint16_t MQ7_ReadAO(void)
{
	return MQ_ReadADC_Channel(ADC_Channel_1);
}

uint16_t MQ4_ReadAO_Avg(uint8_t times)
{
	uint32_t sum = 0;
	uint8_t i;
	if (times == 0)
	{
		times = 1;
	}
	for (i = 0; i < times; i++)
	{
		sum += MQ4_ReadAO();
	}
	return (uint16_t)(sum / times);
}

uint16_t MQ7_ReadAO_Avg(uint8_t times)
{
	uint32_t sum = 0;
	uint8_t i;
	if (times == 0)
	{
		times = 1;
	}
	for (i = 0; i < times; i++)
	{
		sum += MQ7_ReadAO();
	}
	return (uint16_t)(sum / times);
}

float MQ4_ReadPPM_FromAO(uint16_t adc)
{
	float vout = MQ_ADC_ToVoltage(adc);
	float rs = MQ_Voltage_ToRs(vout, MQ4_RL_KOHM);
	float ratio = rs / g_mq4_r0_kohm;
	float ppm = MQ_Ratio_ToPPM(ratio, MQ4_CH4_M, MQ4_CH4_B);

	if (ppm < 0.0f)
	{
		ppm = 0.0f;
	}
	return ppm;
}

float MQ7_ReadPPM_FromAO(uint16_t adc)
{
	float vout = MQ_ADC_ToVoltage(adc);
	float rs = MQ_Voltage_ToRs(vout, MQ7_RL_KOHM);
	float ratio = rs / g_mq7_r0_kohm;
	float ppm = MQ_Ratio_ToPPM(ratio, MQ7_CO_M, MQ7_CO_B);

	if (ppm < 0.0f)
	{
		ppm = 0.0f;
	}
	return ppm;
}

void MQ4_SetR0(float r0_kohm)
{
	if (r0_kohm > 0.01f)
	{
		g_mq4_r0_kohm = r0_kohm;
	}
}

void MQ7_SetR0(float r0_kohm)
{
	if (r0_kohm > 0.01f)
	{
		g_mq7_r0_kohm = r0_kohm;
	}
}

float MQ4_GetR0(void)
{
	return g_mq4_r0_kohm;
}

float MQ7_GetR0(void)
{
	return g_mq7_r0_kohm;
}







