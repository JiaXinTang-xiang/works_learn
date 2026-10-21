/**
 ******************************************************************************
 * host_uart.c —— 上位机串口硬件层实现（STM32F103 标准外设库）
 *
 * 换串口/引脚：只改本文件里的 Init 与 IRQHandler（USART1 -> USART2 等），
 * 协议层 uart_protocol.c 无需任何改动。
 ******************************************************************************
 */
#include "host_uart.h"

/* 接收完成标志：ISR 置位，主循环 Host_UART_Poll 里处理并清零 */
static volatile uint8_t rx_ready = 0;

void Host_UART_Init(void)
{
    GPIO_InitTypeDef  GPIO_InitStructure;
    USART_InitTypeDef USART_InitStructure;
    NVIC_InitTypeDef  NVIC_InitStructure;

    /* 开启 USART1 与 GPIOA 时钟 */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1 | RCC_APB2Periph_GPIOA, ENABLE);

    /* PA9 复用推挽输出(TX) */
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_9;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    /* PA10 上拉输入(RX) */
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_InitStructure.GPIO_Pin  = GPIO_Pin_10;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    /* USART 115200 8N1 */
    USART_InitStructure.USART_BaudRate            = 115200;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode                = USART_Mode_Tx | USART_Mode_Rx;
    USART_InitStructure.USART_Parity              = USART_Parity_No;
    USART_InitStructure.USART_StopBits            = USART_StopBits_1;
    USART_InitStructure.USART_WordLength          = USART_WordLength_8b;
    USART_Init(USART1, &USART_InitStructure);

    /* 开启 RXNE 接收中断 */
    USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);

    /* NVIC 中断（若工程已在别处配置过分组，可删下面这行） */
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
    NVIC_InitStructure.NVIC_IRQChannel                   = USART1_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelCmd                = ENABLE;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority        = 1;
    NVIC_Init(&NVIC_InitStructure);

    USART_Cmd(USART1, ENABLE);
}

void Host_UART_SendByte(uint8_t byte)
{
    USART_SendData(USART1, byte);
    while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
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

/* USART1 接收中断：只读字节喂给协议层，置位标志，不在此做阻塞发送 */
void USART1_IRQHandler(void)
{
    if (USART_GetITStatus(USART1, USART_IT_RXNE) == SET)
    {
        uint8_t byte = (uint8_t)USART_ReceiveData(USART1);

        if (Proto_ParseByte(byte))
        {
            rx_ready = 1;
        }

        USART_ClearITPendingBit(USART1, USART_IT_RXNE);
    }
}
