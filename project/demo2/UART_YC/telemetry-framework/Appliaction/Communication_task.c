/**
 ******************************************************************************
 * @file    Communication_task.c
 * @brief   通信任务回调函数
 *          - USART3 (huart3): SBUS 遥控器协议解析
 *          - 视觉串口协议已迁到 boards/host_uart.c（工创赛协议）
 ******************************************************************************
 */

/* Includes ------------------------------------------------------------------*/
#include "communication_task.h"

/**
 * @brief  SBUS 数据接收回调 (USART3 — DMA 空闲中断)
 * @param  Buffer  接收数据缓冲区
 * @param  Length  本次接收到的数据长度
 *
 * @note   SBUS 帧为 25 字节固定长度, 以 0x0F 开头、0x00 结尾.
 *         由于使用 DMA 空闲中断 (Idle Line Detection), 通常一帧
 *         刚好触发一次回调. 此处额外增加帧同步搜索, 处理:
 *         - 意外粘包 (缓冲区含多帧)
 *         - 首次同步 (半帧丢弃)
 *         - 数据错位后自动恢复同步
 *
 *         信号正常时:
 *         - SBUS_CH.ConnectState = 1
 *         - SBUS_CH.CH1 ~ CH16 更新为最新通道值
 *
 *         信号丢失/失控保护时:
 *         - SBUS_CH.ConnectState = 0
 *         - 通道值保持上次有效值不变
 */
void SBUS_Data_Call_Back(uint8_t *Buffer, uint16_t Length)
{
    uint16_t i;

    /* ── 长度不足一帧, 直接丢弃 ───────────────────────── */
    if (Length < SBUS_FRAME_SIZE)
    {
        return;
    }

    /*
     * ── 帧同步搜索 ─────────────────────────────────────
     * 在缓冲区中搜索完整的 SBUS 帧 (0x0F ... 0x00)
     * 从后往前搜: SBUS 帧间隔 (~4~7ms) 远小于 DMA 空闲检测时间,
     * 所以缓冲区末尾的最新帧是最有效的.
     *
     *   搜索策略: 找到最后一对有效的 0x0F 和 0x00 (相隔24字节)
     *   这样即使前面有半帧也自动跳过.
     */
    for (i = Length - SBUS_FRAME_SIZE; i > 0; i--)
    {
        if ((Buffer[i] == SBUS_START_BYTE) &&
            (Buffer[i + SBUS_FRAME_SIZE - 1] == SBUS_END_BYTE))
        {
            /* 找到有效帧, 调用解析函数 */
            SBUS_Update(&Buffer[i]);
            return;
        }
    }

    /* 检查缓冲区第 0 字节是否是帧头 (兜底) */
    if ((Buffer[0] == SBUS_START_BYTE) &&
        (Length >= SBUS_FRAME_SIZE) &&
        (Buffer[SBUS_FRAME_SIZE - 1] == SBUS_END_BYTE))
    {
        SBUS_Update(&Buffer[0]);
    }
}

/************************ COPYRIGHT(C) USTC-ROBOWALKER **************************/
