#ifndef __AHT20_H
#define __AHT20_H


void AHT20_WriteHalfword(uint8_t Command, uint8_t Data0, uint8_t Data1);
void AHT20_WriteCommand(uint8_t Command);
uint8_t AHT20_ReadState(void);
void AHT20_WriteInit(void);
void AHT20_WriteMeasure(void);
void AHT20_WriteReset(void);
float* AHT20_ReadTemperatureHumidity(void);
void AHT20_Init(void);
void BH1750_Init(void);
float BH1750_ReadLux(void);



#endif
