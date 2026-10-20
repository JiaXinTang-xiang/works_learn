/**
 ******************************************************************************
 * @file    Communication_task.c
 * @brief   通信任务回调函数
 *          - USART1 (huart1): Serialplot 串口绘图 / 命令控制
 *          - USART3 (huart3): SBUS 遥控器协议解析
 ******************************************************************************
 */

/* Includes ------------------------------------------------------------------*/
#include "communication_task.h"
#include <string.h>

/* Exported variables --------------------------------------------------------*/
Struct_Vision_Data Vision_Data = {0};

/* Private variables ---------------------------------------------------------*/
static uint8_t Vision_Rx_Buffer[VISION_FRAME_DATA_LENGTH];
static uint8_t Vision_Rx_State = 0;
static uint8_t Vision_Rx_Index = 0;

/* Private function declarations ---------------------------------------------*/
static uint16_t Vision_Read_Uint16(const uint8_t *Data);
static float Vision_Read_Float(const uint8_t *Data);
static void Vision_Parse_Frame(void);
static void Vision_Input_Byte(uint8_t Data);

/**
 * @brief  从字节数组读取一个小端 uint16_t 数据
 */
static uint16_t Vision_Read_Uint16(const uint8_t *Data)
{
    return (uint16_t)Data[0] | ((uint16_t)Data[1] << 8);
}

/**
 * @brief  从字节数组读取一个小端 float 数据
 * @note   Jetson 和 STM32 均使用 IEEE754 单精度浮点格式
 */
static float Vision_Read_Float(const uint8_t *Data)
{
    float Value;

    memcpy(&Value, Data, sizeof(float));
    return Value;
}

/**
 * @brief  解析一帧完整的视觉数据
 * @note   帧体格式: CMD(2) + FLAGS(2) + 12个float(48)
 */
static void Vision_Parse_Frame(void)
{
    uint16_t Command;
    uint8_t i;

    Command = Vision_Read_Uint16(&Vision_Rx_Buffer[0]);
    Vision_Data.Last_Command = Command;
    Vision_Data.Last_Flags = Vision_Read_Uint16(&Vision_Rx_Buffer[2]);

    switch (Command)
    {
        case VISION_CMD_OUTER_RECT:
            for (i = 0; i < 8; i++)
            {
                Vision_Data.Outer_Rect[i / 2][i % 2] =
                    Vision_Read_Float(&Vision_Rx_Buffer[4 + i * 4]);
            }
            Vision_Data.Outer_Valid = 1;
            break;

        case VISION_CMD_INNER_RECT:
            for (i = 0; i < 8; i++)
            {
                Vision_Data.Inner_Rect[i / 2][i % 2] =
                    Vision_Read_Float(&Vision_Rx_Buffer[4 + i * 4]);
            }
            Vision_Data.Inner_Valid = 1;
            break;

        case VISION_CMD_RED_POINT:
            Vision_Data.Red_X = Vision_Read_Float(&Vision_Rx_Buffer[4]);
            Vision_Data.Red_Y = Vision_Read_Float(&Vision_Rx_Buffer[8]);
            /* 上位机使用 (0, 0) 表示当前没有检测到红色激光点 */
            Vision_Data.Red_Valid =
                ((Vision_Data.Red_X != 0.0f) || (Vision_Data.Red_Y != 0.0f));
            break;

        default:
            Vision_Data.Error_Count++;
            return;
    }

    Vision_Data.Frame_Count++;
}

/**
 * @brief  输入一个串口字节并完成帧同步
 * @note   可处理半帧、粘包以及帧前存在无效数据的情况
 */
static void Vision_Input_Byte(uint8_t Data)
{
    switch (Vision_Rx_State)
    {
        case 0:
            if (Data == VISION_FRAME_HEAD)
            {
                Vision_Rx_State = 1;
            }
            break;

        case 1:
            if (Data == VISION_FRAME_DATA_LENGTH)
            {
                Vision_Rx_Index = 0;
                Vision_Rx_State = 2;
            }
            else
            {
                Vision_Data.Error_Count++;
                /* 连续收到 0xA5 时，保留它作为下一次帧头 */
                Vision_Rx_State = (Data == VISION_FRAME_HEAD) ? 1 : 0;
            }
            break;

        case 2:
            Vision_Rx_Buffer[Vision_Rx_Index++] = Data;
            if (Vision_Rx_Index >= VISION_FRAME_DATA_LENGTH)
            {
                Vision_Parse_Frame();
                Vision_Rx_Index = 0;
                Vision_Rx_State = 0;
            }
            break;

        default:
            Vision_Rx_Index = 0;
            Vision_Rx_State = 0;
            break;
    }
}

/**
 * @brief  UART 串口接收 DMA 空闲中断回调 (USART1)
 * @param  Buffer  接收数据缓冲区
 * @param  Length  接收数据长度
 * @note   接收 Jetson 发送的内外矩形坐标和红色激光点坐标
 */
void UART_Serialplot_Call_Back(uint8_t *Buffer, uint16_t Length)
{	
//    if (Length == 0) return;
	
    uint16_t i;

    if ((Buffer == 0) || (Length == 0))
    {
        return;
    }

    Vision_Data.Byte_Count += Length;
    for (i = 0; i < Length; i++)
    {
        Vision_Input_Byte(Buffer[i]);
    }
}


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

    /*
     * 注意: 如果没找到有效帧 (半帧/噪声), 不做任何处理,
     * 等待下一帧自动恢复同步.
     */
}

/************************ COPYRIGHT(C) USTC-ROBOWALKER **************************/
