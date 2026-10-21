/**
  ******************************************************************************
  * @file           : host_uart.c
  * @brief          : 上位机串口协议桥实现（工创赛协议，STM32F407 HAL 库）
  *
  * 接收：由 drv_uart.cpp 的 DMA+空闲中断驱动，把字节喂给 Host_UART_OnBytes，
  *       协议层切出合法帧后立即解析到 vision_data（或标记回环）。
  * 发送：HAL_UART_Transmit 阻塞发送（返回即完成，栈上缓冲安全）。
  ******************************************************************************
  */
#include "host_uart.h"

/* Private variables ---------------------------------------------------------*/

static UART_HandleTypeDef *s_huart = NULL;
static volatile uint8_t s_echo_ready = 0;   /* 收到 0x7F 回环帧，主循环回发 */

/* Private function prototypes -----------------------------------------------*/

static void Host_UART_ProcessByte(uint8_t Byte);

/**
 * @brief  保存串口句柄（供发送接口使用），不启动 DMA
 */
void Host_UART_Init(UART_HandleTypeDef *huart)
{
    s_huart = huart;
}

/**
 * @brief  处理单个接收字节：喂协议层状态机，切出帧后分发或标记回环
 * @note   在 drv_uart 接收回调（中断/DMA 上下文）里调用，只做轻量操作。
 */
static void Host_UART_ProcessByte(uint8_t Byte)
{
    if (Proto_ParseByte(Byte))
    {
        if (g_rx_cmd == CMD_ECHO)
        {
            /* 回环自测：不在此阻塞发送，置标志由主循环 Host_UART_Poll 处理 */
            s_echo_ready = 1;
        }
        else
        {
            Proto_Dispatch(g_rx_cmd, g_rx_payload, g_rx_len);
        }
    }
}

/**
 * @brief  把一段接收到的字节喂给协议层
 */
void Host_UART_OnBytes(const uint8_t *Data, uint16_t Length)
{
    uint16_t i;

    if ((Data == NULL) || (Length == 0))
    {
        return;
    }

    for (i = 0; i < Length; i++)
    {
        Host_UART_ProcessByte(Data[i]);
    }
}

/**
 * @brief  主循环轮询：处理回环自测帧（0x7F），返回 1 表示回环了一帧
 */
uint8_t Host_UART_Poll(void)
{
    if (s_echo_ready == 0)
    {
        return 0;
    }

    s_echo_ready = 0;
    Host_UART_SendFrame(CMD_ECHO, g_rx_payload, g_rx_len);
    return 1;
}

/* 发送接口 ------------------------------------------------------------------*/

/**
 * @brief  阻塞发送一段字节
 */
void Host_UART_SendArray(const uint8_t *Data, uint16_t Length)
{
    if (s_huart == NULL)
    {
        return;
    }

    HAL_UART_Transmit(s_huart, (uint8_t *)Data, Length, 0xFFFF);
}

/**
 * @brief  打包并发送一帧上行帧（下位机 -> 上位机，SOF 0xA6）
 */
void Host_UART_SendFrame(uint8_t Cmd, const uint8_t *Payload, uint8_t Length)
{
    uint8_t buf[6 + PROTO_MAX_PAYLOAD];
    uint16_t frame_len;

    frame_len = Proto_BuildUplink(buf, Cmd, Payload, Length);
    Host_UART_SendArray(buf, frame_len);
}

/**
 * @brief  发送任务触发帧
 */
void Host_UART_SendTask(uint8_t Task, uint8_t Target)
{
    TaskPayload_t t;

    t.task   = Task;
    t.target = Target;
    Host_UART_SendFrame(CMD_TASK, (const uint8_t *)&t, sizeof(t));
}

/**
 * @brief  发送里程计帧（mm）
 */
void Host_UART_SendOdom(int32_t X, int32_t Y)
{
    OdomPayload_t o;

    o.x = X;
    o.y = Y;
    Host_UART_SendFrame(CMD_ODOM, (const uint8_t *)&o, sizeof(o));
}

/**
 * @brief  发送陀螺仪帧（0.01°）
 */
void Host_UART_SendGyro(int16_t Yaw, int16_t Pitch, int16_t Roll)
{
    GyroPayload_t g;

    g.yaw   = Yaw;
    g.pitch = Pitch;
    g.roll  = Roll;
    Host_UART_SendFrame(CMD_GYRO, (const uint8_t *)&g, sizeof(g));
}
