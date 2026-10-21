/**
  ******************************************************************************
  * @file           : host_uart.h
  * @brief          : 上位机串口协议桥（工创赛协议，STM32F407 HAL 库）
  *
  * 与 f407_hal 模板的区别：本工程已用 drv_uart.cpp 作为统一 UART 传输层
  * （DMA + 空闲中断），所以这里【不】重定义 HAL 弱函数，也不自行启动 DMA。
  * 只需把 Host_UART_OnBytes 注册成 drv_uart 的回调即可：
  *     UART_Init(&huart1, Host_UART_OnBytes, UART_BUFFER_SIZE);
  * 发送走本文件的阻塞发送接口（Host_UART_SendXxx）。
  ******************************************************************************
  */
#ifndef HOST_UART_H
#define HOST_UART_H

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"
#include "uart_protocol.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Exported functions --------------------------------------------------------*/

/**
 * @brief  保存串口句柄（供发送接口使用），不启动 DMA
 * @param  huart  已由 CubeMX 初始化好的 UART 句柄（如 &huart1）
 * @note   接收由 drv_uart 的 UART_Init(huart, Host_UART_OnBytes, ...) 启动。
 */
void Host_UART_Init(UART_HandleTypeDef *huart);

/**
 * @brief  主循环轮询：处理回环自测帧（0x7F），返回 1 表示回环了一帧
 * @note   普通视觉帧在接收回调里已自动解析到 vision_data，无需轮询。
 */
uint8_t Host_UART_Poll(void);

/**
 * @brief  把一段接收到的字节喂给协议层（注册成 drv_uart 的回调）
 * @param  Data   接收缓冲区指针
 * @param  Length 字节数
 */
void Host_UART_OnBytes(const uint8_t *Data, uint16_t Length);

/* 发送接口 ------------------------------------------------------------------*/

/** 阻塞发送一段字节 */
void Host_UART_SendArray(const uint8_t *Data, uint16_t Length);

/** 打包并发送一帧上行帧（下位机 -> 上位机，SOF 0xA6） */
void Host_UART_SendFrame(uint8_t Cmd, const uint8_t *Payload, uint8_t Length);

/** 任务触发（task=任务码, target=目标码） */
void Host_UART_SendTask(uint8_t Task, uint8_t Target);

/** 里程计 x/y（单位 mm） */
void Host_UART_SendOdom(int32_t X, int32_t Y);

/** 陀螺仪 yaw/pitch/roll（单位 0.01°） */
void Host_UART_SendGyro(int16_t Yaw, int16_t Pitch, int16_t Roll);

#ifdef __cplusplus
}
#endif

#endif /* HOST_UART_H */
