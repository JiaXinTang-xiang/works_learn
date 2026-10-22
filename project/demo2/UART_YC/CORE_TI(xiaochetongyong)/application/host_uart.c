/**
 ******************************************************************************
 * host_uart.c —— 上位机串口硬件层实现（TI MSPM0 DriverLib）
 *
 * 对应 SysConfig 的 Communication 串口（UART1）。外设配置已在
 * SYSCFG_DL_Communication_init() 里完成（见 User/ti_msp_dl_config.c）：
 *   115200 8N1、RX 中断已开、NVIC 优先级 1。
 * 这里只补充：NVIC 使能 + 协议层状态机复位 + 收发字节。
 ******************************************************************************
 */
#include "host_uart.h"

/* 接收完成标志：ISR 置位，主循环 Host_UART_Poll 里处理并清零 */
static volatile uint8_t rx_ready = 0;

void Host_UART_Init(void)
{
    NVIC_ClearPendingIRQ(Communication_INST_INT_IRQN);
    NVIC_EnableIRQ(Communication_INST_INT_IRQN);
    Proto_Init();
}

void Host_UART_SendByte(uint8_t byte)
{
    while (DL_UART_Main_isBusy(Communication_INST))
    {
    }
    DL_UART_Main_transmitData(Communication_INST, byte);
}

void Host_UART_SendArray(const uint8_t *data, uint16_t len)
{
    uint16_t i;
    for (i = 0; i < len; i++)
    {
        Host_UART_SendByte(data[i]);
    }
}

void Host_UART_SendFrame(uint8_t cmd, const uint8_t *payload, uint8_t len)
{
    uint8_t  buf[6 + PROTO_MAX_PAYLOAD];
    uint16_t frame_len;

    frame_len = Proto_BuildUplink(buf, cmd, payload, len);
    Host_UART_SendArray(buf, frame_len);
}

void Host_UART_SendTask(uint8_t task, uint8_t target)
{
    TaskPayload_t t;
    t.task   = task;
    t.target = target;
    Host_UART_SendFrame(CMD_TASK, (const uint8_t *)&t, sizeof(t));
}

void Host_UART_SendOdom(int32_t x, int32_t y)
{
    OdomPayload_t o;
    o.x = x;
    o.y = y;
    Host_UART_SendFrame(CMD_ODOM, (const uint8_t *)&o, sizeof(o));
}

void Host_UART_SendGyro(int16_t yaw, int16_t pitch, int16_t roll)
{
    GyroPayload_t g;
    g.yaw   = yaw;
    g.pitch = pitch;
    g.roll  = roll;
    Host_UART_SendFrame(CMD_GYRO, (const uint8_t *)&g, sizeof(g));
}

uint8_t Host_UART_Poll(void)
{
    if (rx_ready == 0)
        return 0;
    rx_ready = 0;

    if (g_rx_cmd == CMD_ECHO)
    {
        /* 回环自测：原样回发（联调用，走主循环不在中断里阻塞发送） */
        Host_UART_SendFrame(CMD_ECHO, g_rx_payload, g_rx_len);
    }
    else
    {
        /* 正常业务帧：解析到 vision_data */
        Proto_Dispatch(g_rx_cmd, g_rx_payload, g_rx_len);
    }
    return 1;
}

/* Communication 串口(UART1)接收中断：只读字节喂协议层，不在此阻塞发送 */
void Communication_INST_IRQHandler(void)
{
    switch (DL_UART_Main_getPendingInterrupt(Communication_INST))
    {
        case DL_UART_MAIN_IIDX_RX:
        {
            uint8_t byte = DL_UART_Main_receiveData(Communication_INST);
            if (Proto_ParseByte(byte))
            {
                rx_ready = 1;
            }
            break;
        }
        default:
            break;
    }
}
