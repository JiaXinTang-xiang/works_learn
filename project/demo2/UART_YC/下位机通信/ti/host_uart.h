/**
 ******************************************************************************
 * host_uart.h —— 上位机串口硬件层（TI MSPM0 DriverLib）
 *
 * 对应 SysConfig 里的 Communication 串口（UART1，PB4=TX / PB5=RX @ 115200），
 * 也就是和下位机直连的「上位机（Jetson）串口」。
 *
 * 外设/引脚/波特率/RX 中断已由 SysConfig 生成的 SYSCFG_DL_Communication_init()
 * 配好（见 User/ti_msp_dl_config.c），本层只做：使能 NVIC、收发字节、把收到的
 * 字节喂给协议层、把要发的帧发出去。换串口只需改 SysConfig 里的 Communication。
 ******************************************************************************
 */
#ifndef __HOST_UART_H
#define __HOST_UART_H

#include "ti_msp_dl_config.h"
#include "uart_protocol.h"

/* 串口初始化（使能 NVIC 接收中断 + 复位协议状态机） */
void Host_UART_Init(void);

/* 主循环轮询：处理收到的一帧（回环回发 / 解析到 vision_data），返回 1 表示处理了一帧 */
uint8_t Host_UART_Poll(void);

/* 发送一个字节 / 一段字节数组 */
void Host_UART_SendByte(uint8_t byte);
void Host_UART_SendArray(const uint8_t *data, uint16_t len);

/* 打包并发送一帧上行帧（下位机 -> 上位机，SOF 0xA6） */
void Host_UART_SendFrame(uint8_t cmd, const uint8_t *payload, uint8_t len);

/* 便捷发送：任务触发 / 里程计 / 陀螺仪 */
void Host_UART_SendTask(uint8_t task, uint8_t target);
void Host_UART_SendOdom(int32_t x, int32_t y);
void Host_UART_SendGyro(int16_t yaw, int16_t pitch, int16_t roll);

#endif
