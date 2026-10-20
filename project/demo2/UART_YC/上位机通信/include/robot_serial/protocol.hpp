// protocol.hpp —— 串口帧协议（命令定义 / 打包 / 流式解包）
//
// 帧格式（双向，帧头区分方向）：
//   上位机 -> 下位机: 0xA5 | LEN | CMD | PAYLOAD(LEN) | CRC16(2,小端) | 0x5A
//   下位机 -> 上位机: 0xA6 | LEN | CMD | PAYLOAD(LEN) | CRC16(2,小端) | 0x5A
// CRC16 覆盖 [LEN][CMD][PAYLOAD]（不含 SOF/CRC/EOF）。
// 所有多字节字段小端；载荷结构体 #pragma pack(1) + memcpy，严禁指针强转（防非对齐 HardFault）。
#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "robot_serial/crc16.hpp"

namespace robot_serial {

// ---- 帧常量 ----
inline constexpr uint8_t SOF_DOWN = 0xA5;   // 上位机 -> 下位机
inline constexpr uint8_t SOF_UP   = 0xA6;   // 下位机 -> 上位机
inline constexpr uint8_t EOF_BYTE = 0x5A;
inline constexpr size_t  MAX_PAYLOAD = 255;

// ---- 命令字：上位机 -> 下位机 ----
inline constexpr uint8_t CMD_QR       = 0x10;  // 二维码
inline constexpr uint8_t CMD_TRAY     = 0x11;  // 物料盘
inline constexpr uint8_t CMD_MATERIAL = 0x12;  // 物料
inline constexpr uint8_t CMD_RING     = 0x13;  // 色环
inline constexpr uint8_t CMD_ECHO     = 0x7F;  // 回环自测

// ---- 命令字：下位机 -> 上位机 ----
inline constexpr uint8_t CMD_TASK = 0x01;  // 任务触发
inline constexpr uint8_t CMD_ODOM = 0x20;  // 里程计
inline constexpr uint8_t CMD_GYRO = 0x21;  // 陀螺仪

// ---- 载荷结构体（紧凑打包，字段见 protocol.md） ----
#pragma pack(push, 1)
struct QrPayload       { uint8_t qr[6]; };                      // 6 位数字，和==12
struct TrayPayload     { int16_t x; int16_t y; };               // 像素
struct MaterialPayload { uint8_t color; int16_t x; int16_t y; }; // color: 1红 2绿 3蓝
struct RingPayload     { int16_t mid_x; int16_t mid_y; int16_t z; };  // z = 左环Y - 右环Y
struct TaskPayload     { uint8_t task; uint8_t target; };
struct OdomPayload     { int32_t x; int32_t y; };               // mm
struct GyroPayload     { int16_t yaw; int16_t pitch; int16_t roll; }; // 0.01°
#pragma pack(pop)

// ---- 打包：构建上位机 -> 下位机完整帧（含 CRC16 与 EOF） ----
inline std::vector<uint8_t> packFrame(uint8_t cmd, const void* payload, uint8_t len) {
    std::vector<uint8_t> frame;
    frame.reserve(static_cast<size_t>(len) + 6);
    frame.push_back(SOF_DOWN);
    frame.push_back(len);
    frame.push_back(cmd);
    const uint8_t* p = static_cast<const uint8_t*>(payload);
    for (uint8_t i = 0; i < len; ++i) frame.push_back(p[i]);

    // CRC16 覆盖 [LEN][CMD][PAYLOAD] = frame[1 .. 2+len]
    uint16_t crc = CRC16::calc(frame.data() + 1, frame.size() - 1);
    frame.push_back(static_cast<uint8_t>(crc & 0xFF));
    frame.push_back(static_cast<uint8_t>(crc >> 8));
    frame.push_back(EOF_BYTE);
    return frame;
}

// ---- 解包：从下位机 -> 上位机字节流中逐字节切出合法帧 ----
// 逐字节有限状态机，不怕分包/粘包/前导噪声，只要总线上出现 0xA6 即自动对齐。
class StreamPacketParser {
public:
    // 每读到一个字节喂一次；返回 true 表示切出一帧合法帧（out_cmd / out_payload）
    bool parseByte(uint8_t byte, uint8_t& out_cmd, std::vector<uint8_t>& out_payload);

private:
    enum class State { SOF, LEN, CMD, PAYLOAD, CRC_LO, CRC_HI, TAIL };
    State state_ = State::SOF;
    uint8_t len_ = 0;
    uint8_t cmd_ = 0;
    uint8_t crc_lo_ = 0;
    std::vector<uint8_t> payload_;
    std::vector<uint8_t> crc_buf_;  // [LEN][CMD][PAYLOAD]，用于算 CRC16
};

inline bool StreamPacketParser::parseByte(uint8_t byte, uint8_t& out_cmd,
                                          std::vector<uint8_t>& out_payload) {
    switch (state_) {
        case State::SOF:
            if (byte == SOF_UP) {
                state_ = State::LEN;
                crc_buf_.clear();
            }
            break;

        case State::LEN:
            len_ = byte;
            if (len_ > MAX_PAYLOAD) {   // 长度非法，丢弃重找帧头
                state_ = State::SOF;
                break;
            }
            crc_buf_.push_back(byte);
            state_ = State::CMD;
            break;

        case State::CMD:
            cmd_ = byte;
            crc_buf_.push_back(byte);
            payload_.clear();
            state_ = (len_ == 0) ? State::CRC_LO : State::PAYLOAD;
            break;

        case State::PAYLOAD:
            payload_.push_back(byte);
            crc_buf_.push_back(byte);
            if (payload_.size() == len_) state_ = State::CRC_LO;
            break;

        case State::CRC_LO:
            crc_lo_ = byte;
            state_ = State::CRC_HI;
            break;

        case State::CRC_HI: {
            uint16_t recv = static_cast<uint16_t>(crc_lo_) | (static_cast<uint16_t>(byte) << 8);
            uint16_t calc = CRC16::calc(crc_buf_.data(), crc_buf_.size());
            state_ = (recv == calc) ? State::TAIL : State::SOF;  // 校验不过，整帧丢弃
            break;
        }

        case State::TAIL:
            state_ = State::SOF;
            if (byte == EOF_BYTE) {
                out_cmd = cmd_;
                out_payload = payload_;
                return true;
            }
            break;
    }
    return false;
}

}  // namespace robot_serial
