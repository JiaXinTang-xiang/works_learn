# 下位机通信模板（STM32F103 标准外设库）

与上位机 [protocol.md](../../上位机通信/protocol.md) 对应的下位机串口模块，两层结构：

| 文件 | 层 | 作用 |
|---|---|---|
| `uart_protocol.h/.c` | 协议层（纯逻辑） | CRC16、帧打包/解包状态机、命令分发，**无任何硬件寄存器**，可原样搬到 F407/HAL 或任何平台 |
| `host_uart.h/.c` | 硬件层 | USART1 初始化、收发字节、RX 中断、把帧喂给协议层 |

## 1. 接到现有 F103 工程

1. 把 `uart_protocol.c`、`host_uart.c` 加入工程（Keil 里 Add Existing Files）；
2. 把本目录加进头文件搜索路径（Options → C/C++ → Include Paths）；
3. `main.c` 里初始化 + 主循环轮询：

```c
#include "host_uart.h"

int main(void)
{
    /* 其它初始化... */
    Proto_Init();      // 复位协议状态机（可选，上电默认就是 0）
    Host_UART_Init();  // USART1 115200 8N1

    while (1)
    {
        Host_UART_Poll();   // 处理收到的帧（回环回发 / 解析到 vision_data）
        /* 你的业务循环... */
    }
}
```

## 2. 发送数据（下位机 → 上位机，SOF 0xA6）

```c
Host_UART_SendTask(0x03, 0x00);        // 任务触发：色环
Host_UART_SendOdom(odom_x, odom_y);    // 里程计 x/y（mm，OPS 未装先用测试变量）
Host_UART_SendGyro(yaw, pitch, roll);  // 陀螺仪（0.01°）
```

## 3. 读取收到的视觉数据（上位机 → 下位机）

收包后自动解析到全局 `vision_data`，带有效标志位，业务层直接读：

```c
extern VisionData_t vision_data;   // 已在 uart_protocol.h 声明

if (vision_data.tray_valid) {
    int16_t cx = vision_data.tray_x;
    int16_t cy = vision_data.tray_y;
}
// vision_data.qr_valid / material_valid / ring_valid 同理
```

## 4. 换串口 / 引脚

只改 [host_uart.c](host_uart.c)：
- `RCC_APB2PeriphClockCmd` / `GPIO_Init` / `USART_Init` / `USART_ITConfig` / `USART_Cmd` 里的外设；
- 中断函数名 `USART1_IRQHandler` 换成对应串口（如 USART2/3 还要把时钟从 APB2 改成 APB1）。

## 5. 注意事项

- **CRC16 与上位机同算法**（Modbus 0xA001 / 初值 0xFFFF），两端必须一致，任一比特翻转整帧丢弃。
- **紧凑打包**：载荷结构体 `#pragma pack(push, 1)`，还原用 `memcpy`，勿指针强转（防 Cortex-M 非对齐 HardFault）。
- **回环联调**：上位机发 `0x7F`，下位机收到后原样回发（`Host_UART_Poll` 里处理），用于两端对答案。
- 若 Keil 对 `#pragma pack(push, 1)` 报错（极老编译器），换成 `__packed`（ARMCC）或 `__attribute__((packed))`（GCC）。
- 心跳 `0x00` 预留未实现；里程计 OPS 未装先用测试变量占位。
