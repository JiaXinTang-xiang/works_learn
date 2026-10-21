# 下位机通信模板（STM32F407 HAL 库）

与上位机 [protocol.md](../../上位机通信/protocol.md) 对应的下位机串口模块，两层结构。
**协议层与 [f103_biao](../f103_biao) 完全一致**，只有硬件层不同（HAL 库 vs 标准库）：

| 文件 | 层 | 作用 |
|---|---|---|
| `uart_protocol.h/.c` | 协议层（纯逻辑） | CRC16、帧打包/解包状态机、命令分发，**无任何 HAL 依赖**，可原样搬到任何平台 |
| `host_uart.h/.c` | 硬件层（HAL） | USART 的 DMA+空闲中断接收、收发字节、把帧喂给协议层 |

## 1. 接到现有 F407 HAL 工程（CubeMX）

1. CubeMX 里配好视觉串口（默认 USART1），**并给该串口打开 RX DMA**（`ReceiveToIdle_DMA` 依赖它）；生成 MDK 工程。
2. 把 `uart_protocol.c`、`host_uart.c` 加入工程，并把本目录加进头文件搜索路径。
3. `main.c` 里初始化：

```c
#include "host_uart.h"

int main(void)
{
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_DMA_Init();          // 必须先初始化 DMA
    MX_USART1_UART_Init();  // 再初始化 USART

    Proto_Init();           // 复位协议状态机（可选，上电默认就是 0）
    Host_UART_Init(&huart1); // 启动 DMA 空闲接收

    while (1)
    {
        Host_UART_Poll();   // 只处理回环帧(0x7F)；视觉帧已在回调里解析
        /* 你的业务循环... */
    }
}
```

> 普通视觉帧在 `HAL_UARTEx_RxEventCallback`（接收回调）里**已经自动解析到 `vision_data`**，
> 主循环无需再处理。`Host_UART_Poll()` 只为回环自测服务。

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

只改 `main.c` 的调用点：把 `Host_UART_Init(&huart1)` 换成目标句柄（如 `&huart3`），
并在 CubeMX 里给对应串口打开 RX DMA。协议层与硬件层代码都不用动。

## 5. 注意事项

- **CRC16 与上位机同算法**（Modbus 0xA001 / 初值 0xFFFF），两端必须一致，任一比特翻转整帧丢弃。
- **紧凑打包**：载荷结构体 `#pragma pack(push, 1)`，还原用 `memcpy`，勿指针强转（防 Cortex-M4 非对齐访问）。
- **接收依赖 DMA**：`HAL_UARTEx_ReceiveToIdle_DMA` 需要 CubeMX 里给该串口开 RX DMA，
  且 `MX_DMA_Init()` 要在 `MX_USARTx_UART_Init()` 之前调用。
- **弱回调冲突**：`host_uart.c` 重定义了 HAL 弱函数 `HAL_UARTEx_RxEventCallback` / `HAL_UART_ErrorCallback`。
  如果工程里已经用了 telemetry-framework 的 `drv_uart.cpp`（也定义了这两个函数），**二选一**：
  - 删掉 `drv_uart.cpp`，只用 `host_uart.c`；或
  - 保留 `drv_uart.cpp`，把 `host_uart.c` 里这两个弱函数删掉，改用
    `UART_Init(&huart1, Host_UART_OnBytes, HOST_UART_RX_BUF_SIZE)` 注册回调。
- **发送是阻塞的**：`Host_UART_SendArray` 用 `HAL_UART_Transmit`（返回即发送完成，栈上缓冲安全）。
  帧很短、频率 50~100Hz 完全够；以后若要零阻塞高频发送，再换 `HAL_UART_Transmit_DMA` + **静态**发送缓冲（注意 DMA 发送时源缓冲必须保持有效，别用栈变量）。
- **与 f103_biao 的差异**：协议层完全相同；硬件层一个是标准库逐字节中断，一个是 HAL 库 DMA+空闲中断。
  f103 版在 `Host_UART_Poll()` 里做分发，f407 版在接收回调里直接分发（DMA 一坨字节里可能含多帧，逐帧即时分发才不丢帧），`Host_UART_Poll()` 只剩回环功能。
- 心跳 `0x00` 预留未实现；里程计 OPS 未装先用测试变量占位。
