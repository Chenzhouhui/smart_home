#include "freertos_demo.h"
/*FreeRTOS*********************************************************************************************/
#include "FreeRTOS.h"
#include "task.h"
#include <string.h>

/*其他头文件********************************************************************************************/
#include "stdio.h"
#include "UART.h"
#include "LED.h"
#include "lcd.h"
#include "gui.h"
#include "pic.h"
#include "AHT20.h"
#include "MQ.h"
#include "HC_SR501.h"
#include "ESP8266.h"
#include "onenet.h"
/******************************************************************************************************/
/*FreeRTOS配置*/

#define WIFI_SSID       "640"
#define WIFI_PASSWORD   "12345678"

#define MQTT_HOST       "mqtts.heclouds.com"
#define MQTT_PORT       1883
#define MQTT_CLIENT_ID  "ESP8266"
#define MQTT_USERNAME   "6S26Zkf84P"
#define MQTT_PASSWORD   "version=2018-10-31&res=products%2F6S26Zkf84P%2Fdevices%2FESP8266&et=1815951631&method=md5&sign=oKnA0gT%2FA9cyFOj41SoTPw%3D%3D"
#define MQTT_TOPIC_PUB  "$sys/6S26Zkf84P/ESP8266/thing/property/post"

/* START_TASK 任务 配置
 * 包括: 任务句柄 任务优先级 堆栈大小 创建任务
 */
#define START_TASK_PRIO 6                   /* 任务优先级(仅创建任务后删除) */
#define START_STK_SIZE  128                 /* 任务堆栈大小 */
TaskHandle_t            StartTask_Handler;  /* 任务句柄 */
void start_task(void *pvParameters);        /* 任务函数 */

/* TASK1 任务 配置
 * 包括: 任务句柄 任务优先级 堆栈大小 创建任务
 */
#define TASK1_PRIO      2                   /* 任务优先级 */
#define TASK1_STK_SIZE  128                 /* 任务堆栈大小 */
TaskHandle_t            Task1Task_Handler;  /* 任务句柄 */
void task1(void *pvParameters);             /* 任务函数 */

/* TASK2 任务 配置
 * 包括: 任务句柄 任务优先级 堆栈大小 创建任务
 */
#define TASK2_PRIO      3                   /* 任务优先级 */
#define TASK2_STK_SIZE  128                 /* 任务堆栈大小 */
TaskHandle_t            Task2Task_Handler;  /* 任务句柄 */
void task2(void *pvParameters);             /* 任务函数 */

/* TASK3 任务 配置
 * 包括: 任务句柄 任务优先级 堆栈大小 创建任务
 */
#define TASK3_PRIO      4                   /* 任务优先级 */
#define TASK3_STK_SIZE  128                 /* 任务堆栈大小 */
TaskHandle_t            Task3Task_Handler;  /* 任务句柄 */
void task3(void *pvParameters);             /* 任务函数 */

/* TASK4 任务 配置
 * 包括: 任务句柄 任务优先级 堆栈大小 创建任务
 */
#define TASK4_PRIO      5                   /* 任务优先级 */
#define TASK4_STK_SIZE  128                 /* 任务堆栈大小 */
TaskHandle_t            Task4Task_Handler;  /* 任务句柄 */
void task4(void *pvParameters);             /* 任务函数 */

/* TASK5 任务 配置
 * 包括: 任务句柄 任务优先级 堆栈大小 创建任务
 */
#define TASK5_PRIO      1                   /* 任务优先级(联网任务最低) */
#define TASK5_STK_SIZE  256                 /* 任务堆栈大小 */
TaskHandle_t            Task5Task_Handler;  /* 任务句柄 */
void task5(void *pvParameters);             /* 任务函数 */

/******************************************************************************************************/

/* LCD刷屏时使用的颜色 */
volatile float gAHT20_Humidity = 0;
volatile float gAHT20_Temperature = 0;
volatile uint8_t gAHT20_WorkState = 0;
volatile float gBH1750_Lux = 0;
volatile uint8_t gBH1750_WorkState = 0;
volatile uint16_t gMQ4_AO = 0;
volatile uint16_t gMQ7_AO = 0;
volatile uint8_t gMQ4_DO = 0;
volatile uint8_t gMQ7_DO = 0;
volatile float gMQ4_PPM = 0;
volatile float gMQ7_PPM = 0;
volatile uint8_t gHC_SR501_State = 0;
volatile uint8_t gWiFi_Connected = 0;
volatile uint8_t gMQTT_Connected = 0;



/**
 * @brief       FreeRTOS例程入口函数
 * @param       无
 * @retval      无
 */
void freertos_demo(void)
{
	LCD_Init();

    
    xTaskCreate((TaskFunction_t )start_task,            /* 任务函数 */
                (const char*    )"start_task",          /* 任务名称 */
                (uint16_t       )START_STK_SIZE,        /* 任务堆栈大小 */
                (void*          )NULL,                  /* 传入给任务函数的参数 */
                (UBaseType_t    )START_TASK_PRIO,       /* 任务优先级 */
                (TaskHandle_t*  )&StartTask_Handler);   /* 任务句柄 */
    vTaskStartScheduler();
}

/**
 * @brief       start_task
 * @param       pvParameters : 传入参数(未用到)
 * @retval      无
 */
void start_task(void *pvParameters)
{
    taskENTER_CRITICAL();           /* 进入临界区 */
    /* 创建任务1 */
    xTaskCreate((TaskFunction_t )task1,
                (const char*    )"task1",
                (uint16_t       )TASK1_STK_SIZE,
                (void*          )NULL,
                (UBaseType_t    )TASK1_PRIO,
                (TaskHandle_t*  )&Task1Task_Handler);
    /* 创建任务2 */
    xTaskCreate((TaskFunction_t )task2,
                (const char*    )"task2",
                (uint16_t       )TASK2_STK_SIZE,
                (void*          )NULL,
                (UBaseType_t    )TASK2_PRIO,
                (TaskHandle_t*  )&Task2Task_Handler);
    /* 创建任务3 */
    xTaskCreate((TaskFunction_t )task3,
                (const char*    )"task3",
                (uint16_t       )TASK3_STK_SIZE,
                (void*          )NULL,
                (UBaseType_t    )TASK3_PRIO,
                (TaskHandle_t*  )&Task3Task_Handler);
    /* 创建任务4 */
    xTaskCreate((TaskFunction_t )task4,
                (const char*    )"task4",
                (uint16_t       )TASK4_STK_SIZE,
                (void*          )NULL,
                (UBaseType_t    )TASK4_PRIO,
                (TaskHandle_t*  )&Task4Task_Handler);
    /* 创建任务5 */
    xTaskCreate((TaskFunction_t )task5,
                (const char*    )"task5",
                (uint16_t       )TASK5_STK_SIZE,
                (void*          )NULL,
                (UBaseType_t    )TASK5_PRIO,
                (TaskHandle_t*  )&Task5Task_Handler);

    taskEXIT_CRITICAL();            /* 退出临界区 */
    vTaskDelete(StartTask_Handler); /* 删除开始任务 */
}

/**
 * @brief       task1
 * @param       pvParameters : 传入参数(未用到)
 * @retval      无
 */
void task1(void *pvParameters)
{

    
    while(1)
    {
        
//        LED_ON();
//        vTaskDelay(200);
//        LED_OFF();
//        vTaskDelay(200);
        vTaskDelay(100);
  
    }
}

/**
 * @brief       task2
 * @param       pvParameters : 传入参数(未用到)
 * @retval      无
 */
void task2(void *pvParameters)
{
    float *TH;
    int temp10;
    int humidity10;
    int temp_abs10;
    int lux10;
    uint32_t mq4ppm;
    uint32_t mq7ppm;

    AHT20_Init();
    BH1750_Init();
    LCD_Clear(WHITE);
    POINT_COLOR = BLACK;

    while(1)
    {
        TH = AHT20_ReadTemperatureHumidity();
        if(TH != 0)
        {
            gAHT20_Humidity = TH[0];
            gAHT20_Temperature = TH[1];
            gAHT20_WorkState = 1;
        }
        else
        {
            gAHT20_WorkState = 0;
        }

        gBH1750_Lux = BH1750_ReadLux();
        if(gBH1750_Lux >= 0)
        {
            gBH1750_WorkState = 1;
        }
        else
        {
            gBH1750_WorkState = 0;
        }

        mq4ppm = (uint32_t)gMQ4_PPM;
        mq7ppm = (uint32_t)gMQ7_PPM;

        LCD_Fill(0,0,127,127,WHITE);

        if(gAHT20_WorkState)
        {
            humidity10 = (int)(gAHT20_Humidity * 10);
            temp10 = (int)(gAHT20_Temperature * 10);
            temp_abs10 = temp10;
            if(temp_abs10 < 0)
            {
                temp_abs10 = -temp_abs10;
            }

            LCD_ShowString(2,4,12,(u8 *)"temp:",1);
            if(temp10 < 0)
            {
                LCD_ShowString(38,4,12,(u8 *)"-",1);
            }
            else
            {
                LCD_ShowString(38,4,12,(u8 *)" ",1);
            }
            LCD_ShowNum(44,4,temp_abs10 / 10,2,12);
            LCD_ShowString(56,4,12,(u8 *)".",1);
            LCD_ShowNum(62,4,temp_abs10 % 10,1,12);

            LCD_ShowString(2,20,12,(u8 *)"humi:",1);
            LCD_ShowNum(38,20,humidity10 / 10,2,12);
            LCD_ShowString(50,20,12,(u8 *)".",1);
            LCD_ShowNum(56,20,humidity10 % 10,1,12);
        }
        else
        {
            LCD_ShowString(2,4,12,(u8 *)"temp:?",1);
            LCD_ShowString(2,20,12,(u8 *)"humi:?",1);
        }

        if(gBH1750_WorkState)
        {
            lux10 = (int)(gBH1750_Lux * 10.0f);
            LCD_ShowString(2,36,12,(u8 *)"lux:",1);
            LCD_ShowNum(30,36,lux10 / 10,5,12);
            LCD_ShowString(60,36,12,(u8 *)".",1);
            LCD_ShowNum(66,36,lux10 % 10,1,12);
        }
        else
        {
            LCD_ShowString(2,36,12,(u8 *)"lux:?",1);
        }

        LCD_ShowString(2,52,12,(u8 *)"mq4:",1);
        LCD_ShowNum(30,52,mq4ppm,5,12);
        LCD_ShowString(66,52,12,(u8 *)"ppm",1);

        LCD_ShowString(2,68,12,(u8 *)"mq7:",1);
        LCD_ShowNum(30,68,mq7ppm,5,12);
        LCD_ShowString(66,68,12,(u8 *)"ppm",1);

        LCD_ShowString(2,84,12,(u8 *)"mq4do:",1);
        LCD_ShowNum(44,84,gMQ4_DO,1,12);
        LCD_ShowString(60,84,12,(u8 *)"mq7do:",1);
        LCD_ShowNum(102,84,gMQ7_DO,1,12);

        LCD_ShowString(2,100,12,(u8 *)"human:",1);
        if(gHC_SR501_State)
        {
            LCD_ShowString(38,100,12,(u8 *)"yes",1);
        }
        else
        {
            LCD_ShowString(38,100,12,(u8 *)"no",1);
        }

        LCD_ShowString(70,100,12,(u8 *)"wifi:",1);
        if(gWiFi_Connected)
        {
            LCD_ShowString(100,100,12,(u8 *)"OK",1);
        }
        else
        {
            LCD_ShowString(100,100,12,(u8 *)"FAIL",1);
        }

        vTaskDelay(500);
    }
}

/**
 * @brief       task3
 * @param       pvParameters : 传入参数(未用到)
 * @retval      无
 */
void task3(void *pvParameters)
{
    MQ_Init();

    while(1)
    {
        gMQ4_AO = MQ4_ReadAO_Avg(8);
        gMQ7_AO = MQ7_ReadAO_Avg(8);
        gMQ4_DO = MQ4_ReadDO();
        gMQ7_DO = MQ7_ReadDO();
        gMQ4_PPM = MQ4_ReadPPM_FromAO(gMQ4_AO);
        gMQ7_PPM = MQ7_ReadPPM_FromAO(gMQ7_AO);

        vTaskDelay(300);
    }
}

/**
 * @brief       task4
 * @param       pvParameters : 传入参数(未用到)
 * @retval      无
 */
void task4(void *pvParameters)
{
    HC_SR501_Init();
    
    while(1)
    {
        gHC_SR501_State = HC_SR501_ReadState();
 //       printf("%d", gHC_SR501_State);
		if(gHC_SR501_State)
        {
            LED_ON();
        }
        else
        {
            LED_OFF();

        }
        vTaskDelay(1000);
    }
}

/**
 * @brief       task5
 * @param       pvParameters : 传入参数(未用到)
 * @retval      无
 */
void task5(void *pvParameters)
{
    char payload[160];

    ESP8266_Init(115200);

    while(1)
    {
        if((WIFI_SSID[0] == '\0') || (WIFI_PASSWORD[0] == '\0'))
        {
            gWiFi_Connected = 0;
            gMQTT_Connected = 0;
            printf("Set WIFI_SSID/WIFI_PASSWORD in freertos_demo.c\r\n");
            vTaskDelay(5000);
            continue;
        }

        if(!gWiFi_Connected)
        {
            if(ESP8266_ConnectWiFi(WIFI_SSID, WIFI_PASSWORD, 15000))
            {
                gWiFi_Connected = 1;
                printf("ESP8266 WiFi connected\r\n");
            }
            else
            {
                gWiFi_Connected = 0;
                gMQTT_Connected = 0;
                printf("ESP8266 WiFi connect fail\r\n");
                vTaskDelay(3000);
                continue;
            }
        }

        if(!gMQTT_Connected)
        {
            if(OneNet_MQTT_Connect(MQTT_HOST,
                                   MQTT_PORT,
                                   MQTT_CLIENT_ID,
                                   MQTT_USERNAME,
                                   MQTT_PASSWORD,
                                   120))
            {
                gMQTT_Connected = 1;
                printf("MQTT connected\r\n");
            }
            else
            {
                gMQTT_Connected = 0;
                printf("MQTT connect fail, err=%d\r\n", OneNet_MQTT_GetLastError());
                printf("CONNACK code=%d\r\n", OneNet_MQTT_GetConnAckCode());
                printf("ESP rsp: %s\r\n", ESP8266_GetRxBuffer());
                if(strstr(ESP8266_GetRxBuffer(), "Recv") != 0)
                {
                    printf("Tip: CONNECT sent, check OneNET auth/topic/client params\r\n");
                }
                vTaskDelay(3000);
                continue;
            }
        }

        snprintf(payload,
                 sizeof(payload),
                 "{\"temp\":%.1f,\"humi\":%.1f,\"lux\":%.1f,\"mq4\":%.0f,\"mq7\":%.0f,\"human\":%u}",
                 gAHT20_Temperature,
                 gAHT20_Humidity,
                 gBH1750_Lux,
                 gMQ4_PPM,
                 gMQ7_PPM,
                 gHC_SR501_State);

        if(!OneNet_MQTT_Publish(MQTT_TOPIC_PUB, payload, 0, 0))
        {
            gMQTT_Connected = 0;
            printf("MQTT publish fail\r\n");
        }
        else
        {
            printf("MQTT pub: %s\r\n", payload);
        }

        vTaskDelay(5000);
    }
}
