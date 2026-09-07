# Reflow Plate (加热台) 项目

本项目是一个基于 GD32E230C8T6 的小型智能加热台/回流焊平台，使用 PlatformIO 环境开发。

## 硬件配置

*   **MCU**: GD32E230C8T6
*   **加热控制 (SSR)**: PB8
*   **旋钮编码器 (EC11)**:
    *   A相: PA1
    *   B相: PA2
    *   按键: PA0
*   **温度传感器 (MAX6675)**:
    *   CS: PB10
    *   SO(MISO): PA6
    *   SCK: PA5
*   **TFT 屏幕**:
    *   CS: PA4
    *   SDA(MOSI): PA7
    *   SCK: PA5
    *   RS/DC: PB0
    *   RES: PB1
*   **蜂鸣器 (无源 + S8050驱动)**: PA8 (使用 TIMER0_CH0 产生 PWM)

## 软件架构

*   使用原生 C 语言编写。
*   **纯异步轮询调度器 (Cooperative Scheduler)**: 模仿飞控，没有阻塞 `delay()`。由 `SysTick` 提供 1ms 时间基准，在主循环中分发 `tasks`。
*   **硬件无关的 PID 算法**: 带积分抗饱和，已完成本地 `native` 单测验证。

## 烧录方式

GD32E230 支持两种烧录方式：

### 方式一：串口 ISP 烧录（推荐，无需调试器）

通过 Type-C 直连，利用芯片内置 bootloader 通过串口烧录。

**BOOT0 跳帽说明：**

| BOOT0 | 模式 | 说明 |
|-------|------|------|
| 0 (GND) | 普通模式 | 从内部 Flash 运行程序 |
| 1 (3.3V) | ISP 模式 | 进入内置 bootloader，等待串口烧录 |

**烧录步骤：**

```powershell
# 1. 将 BOOT0 跳帽接到 1 (3.3V)，拔插 Type-C 重新上电
# 2. 在 reflow_plate 目录下执行：
python flash_gd32.py COM6

# 或指定端口和固件路径：
python flash_gd32.py COM3 .pio/build/gd32e230c8t6/firmware.bin
```

**3. 烧录完成后，将 BOOT0 跳帽改回 0 (GND)，拔插 Type-C 重新上电，程序即开始运行。**

> 串口参数：115200 baud, 8 data bits, Even parity, 1 stop bit (8E1)

### 方式二：GD-Link 调试器烧录

需要连接 GD-Link 调试器到板子 SWD 接口。

```powershell
cd reflow_plate
pio run -e gd32e230c8t6 -t upload
```

### 常见问题

*   **`No response - check BOOT0 and reset`**: BOOT0 没接 1 或没重新上电，拔插 Type-C 后重试
*   **收到 `0x1F` (NACK)**: bootloader 状态异常，彻底断电（拔 Type-C 等 3 秒）后重新上电
*   **屏幕不亮**: 检查 ST7735 背光引脚是否接 3.3V，确认 RES(PB1)、DC(PB0) 引脚连接正确

## 开发指令

*   **编译**: `pio run -e gd32e230c8t6`
*   **运行本地单测 (针对 PID 等逻辑)**: `pio test -e native`
