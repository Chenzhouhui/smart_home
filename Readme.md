# STM32F103 FreeRTOS 多传感器监测与 OneNET 上云项目

## 项目简介

这是一个基于 **STM32F103C8 + FreeRTOS** 的综合物联网实践工程，已实现本地多传感器采集、LCD 实时显示、串口调试输出、WiFi 联网与 OneNET MQTT 数据上报。

当前工程核心能力包括：

- FreeRTOS 多任务并发调度与优先级分层
- SPI1 驱动 ST7735 LCD 单页实时监控
- AHT20 / BH1750 / MQ-4 / MQ-7 / HC-SR501 多传感器协同采集
- USART1 调试串口收发与 `printf` 重定向
- ESP8266（USART2）AT 通信、WiFi 连接与 MQTT 上云
- OneNET 平台鉴权、连接、周期 JSON 上报与连接故障诊断

## 主要功能

- 在 `freertos_demo()` 中创建 `start_task` 并启动调度器，随后创建 `task1~task5`：
	- `task1`：系统行为辅助任务（LED 联动/预留）
	- `task2`：LCD 显示任务（`temp/humi/lux/mq4/mq7/human/wifi`）
	- `task3`：MQ 传感器采样与 ppm 估算
	- `task4`：人体红外采样与联动控制
	- `task5`：ESP8266 联网、MQTT 连接、周期上报
- 传感器数据按固定周期打包 JSON，通过 MQTT 发布至 OneNET。
- 提供 MQTT 连接失败诊断信息（错误码、ESP 回包、CONNACK 码）。

## 目录说明

- `User/`
	- `main.c`：主函数入口，初始化基础外设并启动 FreeRTOS
	- `freertos_demo.c`：任务创建、采集/显示/联网任务主逻辑
	- `FreeRTOSConfig.h`：FreeRTOS 配置（系统 tick、堆大小、中断优先级等）
- `Hardware/`
	- `LED.c/.h`：LED GPIO 控制
	- `lcd.c/.h`、`GUI.c/.h`：LCD 与图形显示驱动
	- `AHT20.c/.h`、`MyI2C.c/.h`：温湿度采集与软件 I2C
	- `MQ.c/.h`：MQ-4/MQ-7 采样与 ppm 估算
	- `HC_SR501.c/.h`：人体红外状态采集
	- `ESP8266.c/.h`：AT 通信、WiFi 接入
	- `onenet.c/.h`：MQTT 连接与发布封装
- `System/`
	- `UART.c/.h`：USART1 收发与 `printf` 重定向
- `FreeRTOS/`
	- FreeRTOS 内核源码
- `Library/`、`System/`、`Start/`
	- STM32 标准外设库、系统启动与底层支持代码
- `Project.uvprojx`
	- Keil MDK 工程文件

## 运行效果

程序上电后启动 FreeRTOS，设备会自动连接 WiFi 与 OneNET MQTT，并周期上报传感器数据。串口可看到 `MQTT connected` 与 `MQTT pub: {...}` 日志，LCD 单页实时显示当前环境与状态信息。

## 开发记录

### 2026-03-26

今日完成了 LCD 与 FreeRTOS 联调和问题排查，主要工作如下：

- LCD 驱动适配到当前连线（`PA5/PA7` 为 `SPI1` 时钟/数据，`PA4` 片选，`PB12` 数据命令，`PB13` 复位，`PB14` 背光）。
- 将 LCD 写数据路径统一为 `MySPI`（`SPI1`），不再使用原始驱动中的 `SPI2` 调用。
- 在 `task2` 中加入 LCD 功能测试逻辑（清屏、画框、字符串、计数显示），用于验证屏幕驱动是否正常。
- 排查到“加上 LCD 任务后 LED 不闪”的关键风险点：
	- `LCD_Init()` 内部调用 `Delay_ms()`；
	- `System/Delay.c` 直接重配 `SysTick` 寄存器；
	- 在 FreeRTOS 运行期间调用该延时，可能干扰系统节拍（tick）。
- 对任务调度逻辑进行了联调验证：
	- 检查并修正过 `start_task` 临界区与自删除顺序问题（先退出临界区再删除任务）；
	- 检查过 `task2` 优先级与阻塞行为，避免高优先级任务长期占用 CPU。
- 编译与烧录链路验证通过（Keil 构建/下载正常）。

后续建议：

- 将 `Delay_ms()` 改为 FreeRTOS 运行态与裸机态分离实现（运行态优先用 `vTaskDelay`），从根本上避免 SysTick 冲突。

### 2026-03-27

今日完成了多传感器任务整合、人体红外联动、串口重定向、ESP8266 联网与 OneNET MQTT 上云联调，主要工作如下：

- 完成 MQ 模块驱动与任务接入：
	- 新增 `MQ-4` / `MQ-7` 的 `DO`（数字）与 `AO`（ADC）采集；
	- 基于灵敏度曲线完成 `AO -> ppm` 估算接口；
	- 在 FreeRTOS 中增加独立采样任务，数据并入 LCD 显示。
- LCD 显示界面重构为单页小字体监控：
	- 统一按 `name:value` 风格显示 `temp/humi/lux/mq4/mq7/human/wifi`；
	- 取消翻页，便于现场调试观察。
- 完成 `HC-SR501` 人体红外接入与联动：
	- 新增红外采样任务；
	- 增加原始值/稳定值判定逻辑优化；
	- LED 控制改为“人体检测结果联动亮灭”。
- 完成串口模块重构：
	- 实现 `USART1` 收发、环形接收缓冲、文本行读取；
	- 完成 `printf` 重定向，串口日志可直接输出运行状态。
- 完成 `ESP8266` AT 驱动与联网流程：
	- 实现 `USART2` AT 指令通道、响应等待、WiFi 连接；
	- 新增联网任务并调整任务优先级，避免采样/显示与联网任务冲突。
- 完成 `onenet` 模块封装与 MQTT 上云：
	- 增加 MQTT 连接/发布/订阅兼容接口；
	- 支持 OneNET 参数配置（`host/clientId/username/token/topic`）；
	- 对不支持 `AT+MQTT` 的固件增加 `TCP + 原始 MQTT 报文` 兜底方案；
	- 增加失败诊断日志（错误码、ESP 回包、CONNACK 码）。
- 联调结果：
	- 设备成功连接 WiFi；
	- MQTT 成功连接 OneNET；
	- 传感器 JSON 数据可周期上报（串口可见 `MQTT connected` 与 `MQTT pub: {...}` 日志）。
