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
#define MQTT_TOPIC_SUB  "$sys/6S26Zkf84P/ESP8266/thing/property/set"

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
#define TASK5_PRIO      6                   /* 任务优先级*/
#define TASK5_STK_SIZE  1024                /* 任务堆栈大小 */
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
volatile uint8_t gLED_State = 0;
volatile uint8_t gLED_CloudCtrl = 0;
volatile uint8_t gLED_CloudState = 0;
volatile uint8_t gWiFi_Connected = 0;
volatile uint8_t gMQTT_Connected = 0;

static int find_pattern(const uint8_t *buf, uint16_t len, const char *pat)
{
    uint16_t i;
    uint16_t j;
    uint16_t patLen = (uint16_t)strlen(pat);

    if ((buf == 0) || (pat == 0) || (patLen == 0) || (len < patLen))
    {
        return 0;
    }

    for (i = 0; i <= (uint16_t)(len - patLen); i++)
    {
        for (j = 0; j < patLen; j++)
        {
            if (buf[i + j] != (uint8_t)pat[j])
            {
                break;
            }
        }
        if (j == patLen)
        {
            return 1;
        }
    }

    return 0;
}

static uint8_t mqtt_decode_remaining_len(const uint8_t *buf,
                                         uint16_t len,
                                         uint16_t start,
                                         uint32_t *remainLen,
                                         uint16_t *usedBytes)
{
    uint32_t mul = 1;
    uint32_t val = 0;
    uint16_t idx = start;
    uint16_t cnt = 0;
    uint8_t byte;

    if ((buf == 0) || (remainLen == 0) || (usedBytes == 0) || (start >= len))
    {
        return 0;
    }

    do
    {
        if (idx >= len)
        {
            return 0;
        }

        byte = buf[idx++];
        cnt++;
        val += (uint32_t)(byte & 0x7F) * mul;
        mul *= 128;

        if (cnt > 4)
        {
            return 0;
        }
    } while (byte & 0x80);

    *remainLen = val;
    *usedBytes = cnt;
    return 1;
}

static void process_raw_mqtt_puback(const uint8_t *buf, uint16_t len)
{
    uint16_t i;

    if (!OneNet_MQTT_IsRawMode())
    {
        return;
    }

    for (i = 0; i < len; i++)
    {
        uint8_t header = buf[i];
        uint8_t qos = (uint8_t)((header >> 1) & 0x03);
        uint32_t remainLen;
        uint16_t lenBytes;
        uint32_t pktEnd;
        uint32_t pos;
        uint16_t topicLen;
        uint16_t packetId;
        uint8_t ack[4];

        if ((header & 0xF0) != 0x30)
        {
            continue;
        }

        if (qos != 1)
        {
            continue;
        }

        if (!mqtt_decode_remaining_len(buf, len, (uint16_t)(i + 1), &remainLen, &lenBytes))
        {
            continue;
        }

        pktEnd = (uint32_t)i + 1U + (uint32_t)lenBytes + remainLen;
        if (pktEnd > len)
        {
            continue;
        }

        pos = (uint32_t)i + 1U + (uint32_t)lenBytes;
        if (pos + 2U > pktEnd)
        {
            continue;
        }

        topicLen = (uint16_t)(((uint16_t)buf[pos] << 8) | buf[pos + 1U]);
        pos += 2U;
        if (pos + topicLen + 2U > pktEnd)
        {
            continue;
        }

        pos += topicLen;
        packetId = (uint16_t)(((uint16_t)buf[pos] << 8) | buf[pos + 1U]);

        ack[0] = 0x40;
        ack[1] = 0x02;
        ack[2] = (uint8_t)(packetId >> 8);
        ack[3] = (uint8_t)(packetId & 0xFF);
        ESP8266_SendRaw(ack, 4);

        i = (uint16_t)(pktEnd - 1U);
    }
}

static void process_cloud_led_cmd(void)
{
    uint8_t rxBuf[ESP8266_RX_BUFFER_SIZE];
    uint16_t len;
    uint8_t ledChanged = 0;
    uint8_t cmdSeen = 0;

    len = ESP8266_CopyRxBuffer(rxBuf, sizeof(rxBuf));
    if (len == 0)
    {
        return;
    }

    process_raw_mqtt_puback(rxBuf, len);

    if (find_pattern(rxBuf, len, "\"LED\":true") || find_pattern(rxBuf, len, "\"LED\": true"))
    {
        cmdSeen = 1;
        if ((gLED_CloudCtrl == 0) || (gLED_CloudState != 1))
        {
            ledChanged = 1;
        }
        gLED_CloudCtrl = 1;
        gLED_CloudState = 1;
    }
    else if (find_pattern(rxBuf, len, "\"LED\":false") || find_pattern(rxBuf, len, "\"LED\": false"))
    {
        cmdSeen = 1;
        if ((gLED_CloudCtrl == 0) || (gLED_CloudState != 0))
        {
            ledChanged = 1;
        }
        gLED_CloudCtrl = 1;
        gLED_CloudState = 0;
    }

    if (ledChanged)
    {
        printf("Cloud LED cmd -> %s\r\n", gLED_CloudState ? "ON" : "OFF");
    }

    if (cmdSeen)
    {
        ESP8266_ClearRxBuffer();
    }
}

static uint8_t cloud_link_is_lost(const char *rsp)
{
    if (rsp == 0)
    {
        return 0;
    }

    if ((strstr(rsp, "CLOSED") != 0) ||
        (strstr(rsp, "WIFI DISCONNECT") != 0) ||
        (strstr(rsp, "+MQTTDISCONNECTED") != 0))
    {
        return 1;
    }

    return 0;
}



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
        if(gLED_CloudCtrl)
        {
            if(gLED_CloudState)
            {
                LED_ON();
                gLED_State = 1;
            }
            else
            {
                LED_OFF();
                gLED_State = 0;
            }
        }
        else
        {
            /* 仅保留云端控制，注释掉人体检测自动开灯逻辑
            else if(gHC_SR501_State)
            {
                LED_ON();
                gLED_State = 1;
            }
            */
            LED_OFF();
            gLED_State = 0;

        }
        vTaskDelay(1000);
    }
}

/**
 * @brief       task5
 * @param       pvParameters : 传入参数(未用到)
 * @retval      无
 */
//static char payload[512];
void task5(void *pvParameters)
{
    static char payload[256];
    uint16_t report_cnt = 0;
    uint16_t ping_cnt = 0;
    uint8_t mqttSubscribed = 0;
    uint8_t publish_fail_cnt = 0;
    uint32_t report_id = 1;
    const char *rsp;
    int temp10;
    int humi10;
    int lux10;
    int tempInt;
    int tempFrac;
    int humiInt;
    int humiFrac;
    int luxInt;
    int luxFrac;
    float temperatureSnap;
    float humiditySnap;
    float luxSnap;
    uint8_t ledStateSnap;
    uint8_t mq7DoSnap;
    uint8_t mq4DoSnap;
    uint8_t humanSnap;
    const char *ledBool;
    const char *coBool;
    const char *ch4Bool;
    const char *humanBool;

    ESP8266_Init(115200);

    while(1)
    {
        if((WIFI_SSID[0] == '\0') || (WIFI_PASSWORD[0] == '\0'))
        {
            gWiFi_Connected = 0;
            gMQTT_Connected = 0;
            mqttSubscribed = 0;
            report_cnt = 0;
            ping_cnt = 0;
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
                mqttSubscribed = 0;
                report_cnt = 0;
                ping_cnt = 0;
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
                mqttSubscribed = 0;
                report_cnt = 0;
                ping_cnt = 0;
                publish_fail_cnt = 0;
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

        if(!mqttSubscribed)
        {
            if(OneNet_MQTT_Subscribe(MQTT_TOPIC_SUB, 0))
            {
                mqttSubscribed = 1;
                printf("MQTT subscribe ok: %s\r\n", MQTT_TOPIC_SUB);
            }
            else
            {
                gMQTT_Connected = 0;
                mqttSubscribed = 0;
                report_cnt = 0;
                ping_cnt = 0;
                printf("MQTT subscribe fail\r\n");
                vTaskDelay(1000);
                continue;
            }
        }

        rsp = ESP8266_GetRxBuffer();
        if(cloud_link_is_lost(rsp))
        {
            if(strstr(rsp, "WIFI DISCONNECT") != 0)
            {
                gWiFi_Connected = 0;
            }
            gMQTT_Connected = 0;
            mqttSubscribed = 0;
            report_cnt = 0;
            ping_cnt = 0;
            publish_fail_cnt = 0;
            printf("MQTT link lost, reconnect\r\n");
            ESP8266_ClearRxBuffer();
            vTaskDelay(500);
            continue;
        }

        process_cloud_led_cmd();

        if(ESP8266_GetRxLength() > (ESP8266_RX_BUFFER_SIZE * 3 / 4))
        {
            ESP8266_ClearRxBuffer();
        }

        ping_cnt++;
        if(ping_cnt >= 200)
        {
            ping_cnt = 0;
            if(!OneNet_MQTT_Ping())
            {
                gMQTT_Connected = 0;
                mqttSubscribed = 0;
                report_cnt = 0;
                publish_fail_cnt = 0;
                printf("MQTT ping fail, reconnect\r\n");
                vTaskDelay(500);
                continue;
            }
        }

        report_cnt++;
        if(report_cnt < 50)
        {
            vTaskDelay(100);
            continue;
        }
        report_cnt = 0;

        taskENTER_CRITICAL();
        temperatureSnap = gAHT20_Temperature;
        humiditySnap = gAHT20_Humidity;
        luxSnap = gBH1750_Lux;
        ledStateSnap = gLED_State;
        mq7DoSnap = gMQ7_DO;
        mq4DoSnap = gMQ4_DO;
        humanSnap = gHC_SR501_State;
        taskEXIT_CRITICAL();

        ledBool = ledStateSnap ? "true" : "false";
        coBool = mq7DoSnap ? "true" : "false";
        ch4Bool = mq4DoSnap ? "true" : "false";
        humanBool = humanSnap ? "true" : "false";

        temp10 = (int)(temperatureSnap * 10.0f);
        humi10 = (int)(humiditySnap * 10.0f);
        lux10 = (int)(luxSnap * 10.0f);

        tempInt = temp10 / 10;
        tempFrac = temp10 % 10;
        if (tempFrac < 0)
        {
            tempFrac = -tempFrac;
        }

        humiInt = humi10 / 10;
        humiFrac = humi10 % 10;
        if (humiFrac < 0)
        {
            humiFrac = -humiFrac;
        }

        luxInt = lux10 / 10;
        luxFrac = lux10 % 10;
        if (luxFrac < 0)
        {
            luxFrac = -luxFrac;
        }

        snprintf(payload,
             sizeof(payload),
             "{\"id\":\"%lu\",\"version\":\"1.0\",\"params\":{\"temp\":{\"value\":%d.%d},\"humi\":{\"value\":%d.%d},\"lux\":{\"value\":%d.%d},\"LED\":{\"value\":%s},\"CO\":{\"value\":%s},\"CH4\":{\"value\":%s},\"human\":{\"value\":%s}}}",
             (unsigned long)report_id,
             tempInt,
             tempFrac,
             humiInt,
             humiFrac,
             luxInt,
             luxFrac,
             ledBool,
             coBool,
             ch4Bool,
             humanBool);

        if(!OneNet_MQTT_Publish(MQTT_TOPIC_PUB, payload, 0, 0))
        {
            rsp = ESP8266_GetRxBuffer();
            publish_fail_cnt++;
            printf("MQTT publish fail(%d/3)\r\n", publish_fail_cnt);
            printf("PUB rsp: %s\r\n", rsp);

            if(cloud_link_is_lost(rsp))
            {
                if(strstr(rsp, "WIFI DISCONNECT") != 0)
                {
                    gWiFi_Connected = 0;
                }
                gMQTT_Connected = 0;
                mqttSubscribed = 0;
                report_cnt = 0;
                ping_cnt = 0;
                publish_fail_cnt = 0;
                printf("MQTT reconnect by link lost\r\n");
                ESP8266_ClearRxBuffer();
                vTaskDelay(500);
                continue;
            }

            if(publish_fail_cnt >= 3)
            {
                gMQTT_Connected = 0;
                mqttSubscribed = 0;
                report_cnt = 0;
                ping_cnt = 0;
                publish_fail_cnt = 0;
                printf("MQTT reconnect triggered\r\n");
            }

            vTaskDelay(1000);
            continue;
        }
        else
        {
            publish_fail_cnt = 0;
            report_id++;
            printf("MQTT pub: %s\r\n", payload);
        }

        vTaskDelay(100);
    }
}
