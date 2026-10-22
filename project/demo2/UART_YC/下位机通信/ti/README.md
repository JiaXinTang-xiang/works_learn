# 下位机通信模板（TI MSPM0 DriverLib）

与上位机 [protocol.md](../../上位机通信/protocol.md) 对应的下位机串口模块，两层结构。
**协议层与 [f103_biao](../f103_biao) / [f407_hal](../f407_hal) 完全一致**（同一份源码），
只有硬件层不同（TI MSPM0 DriverLib vs STM32 标准库 / HAL）：

| 文件 | 层 | 作用 |
|---|---|---|
| `uart_protocol.h/.c` | 协议层（纯逻辑） | CRC16、帧打包/解包状态机、命令分发，**无任何 TI/DriverLib 依赖**，可原样搬到任何平台 |
| `host_uart.h/.c` | 硬件层（DriverLib） | 对应 SysConfig 的 Communication 串口（UART1），收发字节、把帧喂给协议层 |

> 目标芯片：MSPM0G3519（MSPM0G351x 系列，Cortex-M0+）。对应工程里的
> `CORE_TI(xiaochetongyong)`，Communication 串口 = UART1（PB4=TX / PB5=RX @ 115200）。

## 1. 接到现有 MSPM0 工程

1. SysConfig 里确认「Communication」串口存在且为 115200 8N1、打开 RX 中断
   （默认生成 `SYSCFG_DL_Communication_init()`，外设/引脚/波特率/中断都配好了）。
2. 把 `uart_protocol.c`、`host_uart.c` 加入工程（Keil 里 Add Existing Files），
   并把本目录加进头文件搜索路径。
3. `main.c` 里初始化 + 主循环轮询：

```c
#include "host_uart.h"

int main(void)
{
    SYSCFG_DL_init();     // 里面已经跑过 SYSCFG_DL_Communication_init()
    /* 其它初始化... */
    Host_UART_Init();     // 使能 NVIC 接收中断 + 复位协议状态机

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

只改 SysConfig：把「Communication」串口换成目标 UART 并重新生成
`ti_msp_dl_config.h/.c`。`host_uart.c` 用的是 `Communication_INST` 宏，
换完后无需改动。

## 5. 注意事项

- **CRC16 与上位机同算法**（Modbus 0xA001 / 初值 0xFFFF），两端必须一致，任一比特翻转整帧丢弃。
- **紧凑打包**：载荷结构体 `#pragma pack(push, 1)`，还原用 `memcpy`，勿指针强转（防 Cortex-M0+ 非对齐访问）。
- **外设配置在 SysConfig**：`host_uart.c` 不配 GPIO/波特率，这些都在 `ti_msp_dl_config.c` 里。
  若改波特率/引脚/中断，去 SysConfig 改，别在 `host_uart.c` 里硬编。
- **回环联调**：上位机发 `0x7F`，下位机收到后原样回发（`Host_UART_Poll` 里处理）。
- **接收中断只读字节**：`Communication_INST_IRQHandler` 里只喂协议层、置标志，
  发送走主循环，避免在中断里阻塞等 `DL_UART_Main_isBusy`。
- **发送是阻塞的**：`Host_UART_SendByte` 用 busy-wait 等上字节发完，帧很短、50~100Hz 足够。
- 心跳 `0x00` 预留未实现；里程计 OPS 未装先用测试变量占位。
