/**
 ******************************************************************************
 * host_uart.h —— 上位机串口硬件层（STM32F103 标准外设库）
 *
 * 只负责：USART 初始化、收发字节、把收到的字节喂给协议层、把要发的帧发出去。
 * 默认使用 USART1（PA9=TX, PA10=RX）@ 115200，与 H题1 的视觉串口一致。
 ******************************************************************************
 */
#ifndef __HOST_UART_H
#define __HOST_UART_H

#include "stm32f10x.h"
#include "uart_protocol.h"

/* 串口初始化（USART1 115200 8N1 + RXNE 中断） */
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
