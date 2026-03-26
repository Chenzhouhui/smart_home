# STM32F103 FreeRTOS LED + LCD 联调示例

## 项目简介

这是一个基于 **STM32F103C8** 的 FreeRTOS 联调工程，用于演示：

- FreeRTOS 任务创建与调度
- 不同优先级任务的切换行为
- 使用 GPIO 控制板载 LED（PC13）
- 使用 SPI1 驱动 ST7735 LCD（128x128）
- 在多任务环境下进行 LCD 刷屏与系统节拍协同验证

工程当前包含 LED 闪烁任务与 LCD 测试任务，适合作为学习 **FreeRTOS + 外设驱动并发运行** 的实践样例，后续将继续进行更新。

## 主要功能

- 在 `freertos_demo()` 中创建 `start_task`，并启动调度器。
- `start_task` 再创建两个用户任务：
	- `task1`（优先级 2）：点亮 LED，并延时 200 tick
	- `task2`（优先级 3）：熄灭 LED，并延时 300 tick
- LED 引脚为 `GPIOC Pin 13`（常见 Blue Pill 板载 LED 引脚，低电平点亮）。

## 目录说明

- `User/`
	- `main.c`：主函数入口，初始化 LED 并启动 FreeRTOS 示例
	- `freertos_demo.c`：任务创建、启动调度器、示例任务逻辑
	- `FreeRTOSConfig.h`：FreeRTOS 配置（系统 tick、堆大小、中断优先级等）
- `Hardware/`
	- `LED.c/.h`：LED GPIO 初始化和开关控制接口
- `FreeRTOS/`
	- FreeRTOS 内核源码
- `Library/`、`System/`、`Start/`
	- STM32 标准外设库、系统启动与底层支持代码
- `Project.uvprojx`
	- Keil MDK 工程文件

## 运行效果

程序上电后启动 FreeRTOS，LED 会在两个任务的控制下按不同延时交替亮灭。

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
