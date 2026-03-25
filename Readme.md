# STM32F103 FreeRTOS LED 任务切换示例

## 项目简介

这是一个基于 **STM32F103C8** 的 FreeRTOS 入门工程，用于演示：

- FreeRTOS 任务创建与调度
- 不同优先级任务的切换行为
- 使用 GPIO 控制板载 LED（PC13）

工程通过两个任务交替控制 LED 的亮灭，适合作为学习 RTOS 基本概念的最小示例。

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

## 编译与烧录

### 方式 1：使用 Keil MDK 图形界面

1. 用 Keil 打开 `Project.uvprojx`
2. 选择目标 `Target 1`
3. 执行 Build
4. 连接 ST-Link 后 Download 到开发板

### 方式 2：命令行调用 Keil UV4

可参考如下命令：

```powershell
C:\Keil5\UV4\UV4.exe -f Project.uvprojx -j0 -t "Target 1" -o .vscode\uv4.log
```

## 适用场景

- FreeRTOS 初学者练习任务调度
- STM32 工程模板验证
- 后续扩展按键、中断、串口、队列/信号量等 RTOS 功能

## 说明

本项目定位为教学/实验示例，代码简洁优先。如用于实际产品，请补充：

- 异常处理与断言
- 任务栈溢出检测
- 内存使用评估
- 驱动与应用层模块化设计
