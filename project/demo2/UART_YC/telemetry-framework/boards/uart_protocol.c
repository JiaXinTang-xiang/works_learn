/**
 ******************************************************************************
 * uart_protocol.c —— 串口帧协议层实现（纯逻辑，无任何硬件寄存器操作）
 ******************************************************************************
 */
#include "uart_protocol.h"
#include <string.h>

VisionData_t vision_data;

uint8_t g_rx_cmd = 0;
uint8_t g_rx_payload[PROTO_MAX_PAYLOAD];
uint8_t g_rx_len = 0;

/* 解包状态机（静态变量，跨中断调用保持） */
static uint8_t  s_state = 0;     /* 0:SOF 1:LEN 2:CMD 3:PAYLOAD 4:CRC_LO 5:CRC_HI 6:EOF */
static uint8_t  s_len = 0;
static uint8_t  s_index = 0;
static uint8_t  s_crc_lo = 0;
static uint16_t s_crc = 0xFFFF;

/* 单字节更新 CRC16（供逐字节接收时增量计算） */
static uint16_t CRC16_Update(uint16_t crc, uint8_t byte)
{
    uint8_t i;
    crc ^= byte;
    for (i = 0; i < 8; i++)
    {
        if (crc & 0x0001)
            crc = (crc >> 1) ^ 0xA001;
        else
            crc >>= 1;
    }
    return crc;
}

uint16_t CRC16_Calc(const uint8_t *data, uint16_t len)
{
    uint16_t i;
    uint16_t crc = 0xFFFF;
    for (i = 0; i < len; i++)
    {
        crc = CRC16_Update(crc, data[i]);
    }
    return crc;
}

void Proto_Init(void)
{
    s_state = 0;
    s_len = 0;
    s_index = 0;
    s_crc = 0xFFFF;
    g_rx_cmd = 0;
    g_rx_len = 0;
    memset((void *)&vision_data, 0, sizeof(vision_data));
}

uint8_t Proto_ParseByte(uint8_t byte)
{
    switch (s_state)
    {
        case 0:    /* SOF：等待 0xA5 */
            if (byte == PROTO_SOF_DOWN)
                s_state = 1;
            break;

        case 1:    /* LEN */
            s_len = byte;
            if (s_len > PROTO_MAX_PAYLOAD)
            {
                s_state = 0;   /* 长度非法，丢弃重找帧头 */
                break;
            }
            s_crc = CRC16_Update(0xFFFF, byte);   /* CRC 覆盖 LEN */
            s_state = 2;
            break;

        case 2:    /* CMD */
            g_rx_cmd = byte;
            s_crc = CRC16_Update(s_crc, byte);    /* 覆盖 CMD */
            s_index = 0;
            s_state = (s_len == 0) ? 4 : 3;
            break;

        case 3:    /* PAYLOAD */
            g_rx_payload[s_index] = byte;
            s_index++;
            s_crc = CRC16_Update(s_crc, byte);    /* 覆盖 PAYLOAD */
            if (s_index >= s_len)
                s_state = 4;
            break;

        case 4:    /* CRC16 低字节 */
            s_crc_lo = byte;
            s_state = 5;
            break;

        case 5:    /* CRC16 高字节 */
            if ((((uint16_t)s_crc_lo) | ((uint16_t)byte << 8)) == s_crc)
                s_state = 6;
            else
                s_state = 0;   /* 校验失败，整帧丢弃重找帧头 */
            break;

        case 6:    /* EOF */
            s_state = 0;
            if (byte == PROTO_EOF)
            {
                g_rx_len = s_len;
                return 1;
            }
            break;
    }
    return 0;
}

uint16_t Proto_BuildUplink(uint8_t *buf, uint8_t cmd, const uint8_t *payload, uint8_t len)
{
    uint16_t i;
    uint16_t crc;

    buf[0] = PROTO_SOF_UP;
    buf[1] = len;
    buf[2] = cmd;
    for (i = 0; i < len; i++)
    {
        buf[3 + i] = payload[i];
    }

    /* CRC16 覆盖 [LEN][CMD][PAYLOAD] = buf[1 .. 2+len] */
    crc = CRC16_Calc(&buf[1], 2 + len);

    buf[3 + len]     = (uint8_t)(crc & 0xFF);
    buf[3 + len + 1] = (uint8_t)(crc >> 8);
    buf[3 + len + 2] = PROTO_EOF;

    return (uint16_t)(6 + len);
}

void Proto_Dispatch(uint8_t cmd, const uint8_t *payload, uint8_t len)
{
    vision_data.last_cmd = cmd;

    switch (cmd)
    {
        case CMD_QR:
        {
            if (len == sizeof(QrPayload_t))
            {
                memcpy(vision_data.qr, payload, 6);
                vision_data.qr_valid = 1;
            }
            break;
        }

        case CMD_TRAY:
        {
            TrayPayload_t t;
            if (len == sizeof(TrayPayload_t))
            {
                memcpy(&t, payload, sizeof(t));
                vision_data.tray_x = t.x;
                vision_data.tray_y = t.y;
                vision_data.tray_valid = 1;
            }
            break;
        }

        case CMD_MATERIAL:
        {
            MaterialPayload_t m;
            if (len == sizeof(MaterialPayload_t))
            {
                memcpy(&m, payload, sizeof(m));
                vision_data.material_color = m.color;
                vision_data.material_x = m.x;
                vision_data.material_y = m.y;
                vision_data.material_valid = 1;
            }
            break;
        }

        case CMD_RING:
        {
            RingPayload_t r;
            if (len == sizeof(RingPayload_t))
            {
                memcpy(&r, payload, sizeof(r));
                vision_data.ring_mid_x = r.mid_x;
                vision_data.ring_mid_y = r.mid_y;
                vision_data.ring_z = r.z;
                vision_data.ring_valid = 1;
            }
            break;
        }

        default:
            /* 未知命令 / 回环(CMD_ECHO)由硬件层处理 */
            break;
    }
}
