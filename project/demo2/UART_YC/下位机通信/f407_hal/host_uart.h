/**
  ******************************************************************************
  * @file           : host_uart.h
  * @brief          : 上位机串口硬件层（STM32F407 HAL 库）
  *
  * 只负责：USART 的 DMA+空闲中断接收、收发字节、把收到的字节喂给协议层、
  *         把要发的帧打包发出去。协议层 uart_protocol.c 无需任何改动。
  * 默认视觉串口使用 USART1（Jetson <-> STM32），115200 8N1，与协议文档一致。
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

/* Exported macros -----------------------------------------------------------*/

/** DMA 空闲接收缓冲区长度（够装多帧即可，上电按此大小申请） */
#define HOST_UART_RX_BUF_SIZE  128U

/* Exported functions --------------------------------------------------------*/

/**
 * @brief  初始化上位机串口：保存句柄并启动 DMA + 空闲中断接收
 * @param  huart  已由 CubeMX 初始化好的 UART 句柄（如 &huart1）
 * @note   需保证 MX_DMA_Init() 与 MX_USARTx_UART_Init() 已先调用，
 *         且该串口在 CubeMX 里开启了 RX DMA（接收依赖空闲中断）。
 */
void Host_UART_Init(UART_HandleTypeDef *huart);

/**
 * @brief  主循环轮询：处理回环自测帧（0x7F），返回 1 表示回环了一帧
 * @note   普通视觉帧在接收回调里已自动解析到 vision_data，无需轮询；
 *         只有回环回发需要放主循环（用阻塞发送，避免在中断里阻塞）。
 */
uint8_t Host_UART_Poll(void);

/**
 * @brief  把一段接收到的字节喂给协议层
 * @param  Data   接收缓冲区指针
 * @param  Length 字节数
 * @note   内部逐字节驱动状态机，切出合法帧即分发/标记回环。
 *         若工程里已用 telemetry-framework 的 drv_uart.cpp，可把它
 *         直接注册成回调：UART_Init(&huart1, Host_UART_OnBytes, size)。
 */
void Host_UART_OnBytes(const uint8_t *Data, uint16_t Length);

/* 发送接口 ------------------------------------------------------------------*/

/**
 * @brief  阻塞发送一段字节
 * @param  Data   数据指针
 * @param  Length 字节数
 * @note   用 HAL_UART_Transmit 阻塞发送：返回即发送完成，栈上缓冲安全。
 *         若要高频零阻塞发送，再换 HAL_UART_Transmit_DMA + 静态发送缓冲。
 */
void Host_UART_SendArray(const uint8_t *Data, uint16_t Length);

/**
 * @brief  打包并发送一帧上行帧（下位机 -> 上位机，SOF 0xA6）
 * @param  Cmd     命令字
 * @param  Payload 载荷指针
 * @param  Length  载荷字节数
 */
void Host_UART_SendFrame(uint8_t Cmd, const uint8_t *Payload, uint8_t Length);

/* 便捷发送：任务触发 / 里程计 / 陀螺仪 */

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
