//
// Created by jason on 25-10-16.
//
#include "QD4310.h"

// 限制函数，用于替代C++的std::clamp
static float QD4310_Clamp(float value, float min, float max) {
    if (value < min) return min;
    if (value > max) return max;
    return value;
}

// 初始化QD4310电机结构体
void QD4310_Init(QD4310_t *motor, uint8_t id) {
    motor->id = id;
    motor->enabled = false;
    motor->speed = 0.0f;
    motor->angle = 0.0f;
    motor->current = 0.0f;
}


// 发送命令到电机
void QD4310_SendCommand(QD4310_t *motor, QD4310_Command_t cmd, int16_t value) {
    DL_MCAN_TxBufElement txMsg;
    // 数据：命令 + 低字节 + 高字节
    txMsg.data[0] = (uint8_t)cmd;
    txMsg.data[1] = (uint8_t)(value & 0xFF);
    txMsg.data[2] = (uint8_t)((value >> 8) & 0xFF);
    
     // 标准ID：0x400 + ID，必须左移18位
    txMsg.id   = ((uint32_t)(0x400 + motor->id)) << 18U;
    txMsg.xtd  = 0;          // 标准ID
    txMsg.rtr  = 0;          // 数据帧
    txMsg.dlc  = 3;          // 长度3
    txMsg.fdf  = 0;
    txMsg.brs  = 0;

    // 发送
    DL_MCAN_writeMsgRam(MCAN0_INST, DL_MCAN_MEM_TYPE_BUF, 0, &txMsg);
    DL_MCAN_TXBufAddReq(MCAN0_INST, 0);

}


// 更新电机状态
void QD4310_Update(QD4310_t *motor, const uint8_t feedback[8]) {
    motor->enabled = feedback[0] & 0x01;

    // 重构int16_t值从字节数组
    int16_t current_raw = (int16_t)((feedback[3] << 8) | feedback[2]);
    motor->current = (float)current_raw * 10.0f / 32767.0f; // INT16_MAX = 32767

    int16_t speed_raw = (int16_t)((feedback[5] << 8) | feedback[4]);
    motor->speed = (float)speed_raw * 1000.0f / 32767.0f;

    uint16_t angle_raw = (uint16_t)((feedback[7] << 8) | feedback[6]);
    // motor->angle = (float)angle_raw * QD4310_TWO_PI / 65535.0f; // UINT16_MAX = 65535
    motor->angle = (float)angle_raw * 360.0f / 65535.0f; 
}


// 使能电机
void QD4310_Enable(QD4310_t *motor) {
    QD4310_SendCommand(motor, QD4310_CMD_ENABLE, 0x0000);
}

// 失能电机
void QD4310_Disable(QD4310_t *motor) {
    QD4310_SendCommand(motor, QD4310_CMD_DISABLE, 0x0000);
}

// 设置电机角度
void QD4310_SetAngle(QD4310_t *motor, float angle) {
	angle = QD4310_Clamp(angle, 0.0f, 360.0f) * QD4310_TWO_PI / 360.0f;
	int16_t angle_value = (int16_t)(angle / QD4310_TWO_PI * 65535.0f);
	QD4310_SendCommand(motor, QD4310_CMD_ANGLE, angle_value);
}

// 设置电机转速
void QD4310_SetSpeed(QD4310_t *motor, float speed) {
    speed = QD4310_Clamp(speed, QD4310_MIN_SPEED, QD4310_MAX_SPEED);
    int16_t speed_value = (int16_t)(speed / 1000.0f * 32767.0f);
    QD4310_SendCommand(motor, QD4310_CMD_SPEED, speed_value);
}

// 设置电机低速
void QD4310_SetLowSpeed(QD4310_t *motor, float speed) {
    speed = QD4310_Clamp(speed, QD4310_MIN_SPEED, QD4310_MAX_SPEED);
    int16_t speed_value = (int16_t)(speed / 1000.0f * 32767.0f);
    QD4310_SendCommand(motor, QD4310_CMD_LOW_SPEED, speed_value);
}

// 设置电机电流
void QD4310_SetCurrent(QD4310_t *motor, float current) {
    current = QD4310_Clamp(current, QD4310_MIN_CURRENT, QD4310_MAX_CURRENT);
    int16_t current_value = (int16_t)(current / 10.0f * 32767.0f);
    QD4310_SendCommand(motor, QD4310_CMD_CURRENT, current_value);
}