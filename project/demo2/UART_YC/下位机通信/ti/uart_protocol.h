/**
 ******************************************************************************
 * uart_protocol.h —— 串口帧协议层（纯逻辑，与硬件无关，可移植到任何 MCU）
 *
 * 帧格式（与上位机 protocol.md 一致）：
 *   上位机 -> 下位机: 0xA5 | LEN | CMD | PAYLOAD(LEN) | CRC16(2,小端) | 0x5A
 *   下位机 -> 上位机: 0xA6 | LEN | CMD | PAYLOAD(LEN) | CRC16(2,小端) | 0x5A
 * CRC16 覆盖 [LEN][CMD][PAYLOAD]（不含 SOF/CRC/EOF）。
 * 所有多字节字段小端；载荷结构体 1 字节紧凑打包 + memcpy 还原，严禁指针强转。
 ******************************************************************************
 */
#ifndef __UART_PROTOCOL_H
#define __UART_PROTOCOL_H

#include <stdint.h>

/* ---- 帧常量 ---- */
#define PROTO_SOF_DOWN    0xA5   /* 上位机 -> 下位机 */
#define PROTO_SOF_UP      0xA6   /* 下位机 -> 上位机 */
#define PROTO_EOF         0x5A
#define PROTO_MAX_PAYLOAD 255

/* ---- 命令字：上位机 -> 下位机 ---- */
#define CMD_QR            0x10   /* 二维码 */
#define CMD_TRAY          0x11   /* 物料盘 */
#define CMD_MATERIAL      0x12   /* 物料 */
#define CMD_RING          0x13   /* 色环 */
#define CMD_ECHO          0x7F   /* 回环自测 */

/* ---- 命令字：下位机 -> 上位机 ---- */
#define CMD_TASK          0x01   /* 任务触发 */
#define CMD_ODOM          0x20   /* 里程计 */
#define CMD_GYRO          0x21   /* 陀螺仪 */

/* ---- 载荷结构体（紧凑打包，字段与 protocol.md 一一对应）---- */
#pragma pack(push, 1)
typedef struct { uint8_t  qr[6]; }               QrPayload_t;       /* 6 位数字，和==12 */
typedef struct { int16_t  x, y; }                TrayPayload_t;     /* 像素 */
typedef struct { uint8_t  color; int16_t x, y; } MaterialPayload_t; /* color:1红 2绿 3蓝 */
typedef struct { int16_t  mid_x, mid_y, z; }     RingPayload_t;     /* z=左环Y-右环Y */
typedef struct { uint8_t  task, target; }        TaskPayload_t;
typedef struct { int32_t  x, y; }                OdomPayload_t;     /* mm */
typedef struct { int16_t  yaw, pitch, roll; }    GyroPayload_t;     /* 0.01° */
#pragma pack(pop)

/* ---- 下位机解析后的视觉数据（供业务层读取）---- */
typedef struct {
    uint8_t  qr[6];
    int16_t  tray_x, tray_y;
    uint8_t  material_color;
    int16_t  material_x, material_y;
    int16_t  ring_mid_x, ring_mid_y, ring_z;
    uint8_t  qr_valid, tray_valid, material_valid, ring_valid;
    uint16_t last_cmd;
} VisionData_t;

extern VisionData_t vision_data;

/* 最近一帧解析出的原始帧（cmd/payload/len），供硬件层做回环等处理 */
extern uint8_t  g_rx_cmd;
extern uint8_t  g_rx_payload[PROTO_MAX_PAYLOAD];
extern uint8_t  g_rx_len;

/* CRC16/Modbus：多项式 0xA001，初值 0xFFFF */
uint16_t CRC16_Calc(const uint8_t *data, uint16_t len);

/* 复位协议状态机与数据 */
void Proto_Init(void);

/* 解包状态机：每收到一个字节喂一次；返回 1 表示切出一帧合法下行帧 */
uint8_t Proto_ParseByte(uint8_t byte);

/* 打包上行帧到 buf（不含发送），返回帧长；buf 至少 6+len 字节 */
uint16_t Proto_BuildUplink(uint8_t *buf, uint8_t cmd, const uint8_t *payload, uint8_t len);

/* 按 cmd 把 payload 解析到 vision_data（memcpy 防非对齐访问） */
void Proto_Dispatch(uint8_t cmd, const uint8_t *payload, uint8_t len);

#endif
