/**
 ******************************************************************************
 * @file    Communication_task.h
 * @brief   通信任务回调函数声明
 *          - 视觉串口协议已迁到 boards/host_uart.h/.c（工创赛协议）
 *          - 本文件只保留 SBUS 遥控器接收回调
 ******************************************************************************
 */

#ifndef COMMUNICATION_TASK_H
#define COMMUNICATION_TASK_H

/* Includes ------------------------------------------------------------------*/
#include "bsp.h"

/* Exported functions --------------------------------------------------------*/

/**
 * @brief  USART3 SBUS 接收回调 (DMA 空闲中断)
 * @param  Buffer  接收数据缓冲区
 * @param  Length  接收数据长度 (通常为 25 字节 SBUS 帧)
 * @note   自动搜索帧同步, 调用 SBUS_Update() 解析通道数据
 */
void SBUS_Data_Call_Back(uint8_t *Buffer, uint16_t Length);

#endif /* COMMUNICATION_TASK_H */
