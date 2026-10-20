/**
 ******************************************************************************
 * @file    Communication_task.h
 * @brief   通信任务回调函数声明
 ******************************************************************************
 */

#ifndef COMMUNICATION_TASK_H
#define COMMUNICATION_TASK_H

/* Includes ------------------------------------------------------------------*/
#include "bsp.h"

/* Vision protocol definitions ----------------------------------------------*/
#define VISION_FRAME_HEAD          0xA5U
#define VISION_FRAME_DATA_LENGTH   0x34U
#define VISION_FRAME_SIZE          54U

#define VISION_CMD_OUTER_RECT      0x0102U
#define VISION_CMD_INNER_RECT      0x0103U
#define VISION_CMD_RED_POINT       0x0104U

/* Exported types ------------------------------------------------------------*/
typedef struct
{
    float Outer_Rect[4][2];
    float Inner_Rect[4][2];

    float Red_X;
    float Red_Y;

    uint16_t Last_Command;
    uint16_t Last_Flags;

    uint8_t Outer_Valid;
    uint8_t Inner_Valid;
    uint8_t Red_Valid;

    /* 以下计数值便于在 Keil Debug 中检查通信状态 */
    uint32_t Byte_Count;
    uint32_t Frame_Count;
    uint32_t Error_Count;
} Struct_Vision_Data;

/* Exported variables --------------------------------------------------------*/
extern Struct_Vision_Data Vision_Data;

/* Exported functions --------------------------------------------------------*/

/**
 * @brief  USART1 Serialplot 接收回调
 * @param  Buffer  接收数据缓冲区
 * @param  Length  接收数据长度
 */
void UART_Serialplot_Call_Back(uint8_t *Buffer, uint16_t Length);

/**
 * @brief  USART3 SBUS 接收回调 (DMA 空闲中断)
 * @param  Buffer  接收数据缓冲区
 * @param  Length  接收数据长度 (通常为 25 字节 SBUS 帧)
 * @note   自动搜索帧同步, 调用 SBUS_Update() 解析通道数据
 */
void SBUS_Data_Call_Back(uint8_t *Buffer, uint16_t Length);

#endif /* COMMUNICATION_TASK_H */

/************************ COPYRIGHT(C) USTC-ROBOWALKER **************************/
