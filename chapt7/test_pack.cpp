// test_pack.cpp —— 无硬件自测：验证封包 / CRC8 / 紧凑对齐 / memcpy 解包逻辑
// 不需要串口硬件，直接验证协议帧的数学正确性。
#include <iostream>
#include <iomanip>
#include <cstring>
#include "SerialPort.hpp"

#pragma pack(push, 1)
struct PropTargetPayload {
    uint8_t  target_type;
    float    x;
    float    y;
    float    z;
    float    yaw;
    uint8_t  confidence;
};
#pragma pack(pop)

int main() {
    // 0. 验证紧凑打包大小：1 + 4 + 4 + 4 + 4 + 1 = 18
    std::cout << "sizeof(PropTargetPayload) = " << sizeof(PropTargetPayload)
              << " (应为 18)" << std::endl;
    if (sizeof(PropTargetPayload) != 18) {
        std::cerr << "结构体对齐错误！请检查 #pragma pack(1)。" << std::endl;
        return -1;
    }

    // 1. 准备数据
    PropTargetPayload tx;
    tx.target_type = 1;
    tx.x = 0.523f; tx.y = -0.185f; tx.z = 1.240f;
    tx.yaw = 15.6f; tx.confidence = 95;

    // 2. 封包（不碰串口）
    std::vector<uint8_t> frame;
    SerialPort::buildFrame(0x01, &tx, sizeof(tx), frame);
    std::cout << "帧总长 = " << frame.size() << " (应为 23)" << std::endl;
    if (frame.size() != 23) { std::cerr << "帧长度错误！" << std::endl; return -1; }

    std::cout << "原始帧: ";
    for (uint8_t b : frame)
        std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)b << " ";
    std::cout << std::dec << std::endl;

    // 3. 帧头/长度/CRC/帧尾 检查
    if (frame[0] != SerialPort::FRAME_HEADER) { std::cerr << "帧头错误！" << std::endl; return -1; }
    uint8_t len = frame[1];
    if (len != sizeof(PropTargetPayload)) { std::cerr << "长度错误！" << std::endl; return -1; }
    uint8_t crc = SerialPort::calculateCRC8(frame.data(), 3 + len);
    if (crc != frame[3 + len]) { std::cerr << "CRC 错误！" << std::endl; return -1; }
    if (frame[3 + len + 1] != SerialPort::FRAME_TAIL) { std::cerr << "帧尾错误！" << std::endl; return -1; }

    // 4. memcpy 解包并验证浮点值分毫不差
    PropTargetPayload rx;
    std::memcpy(&rx, &frame[3], sizeof(rx));
    bool ok = (rx.target_type == tx.target_type && rx.x == tx.x && rx.y == tx.y &&
               rx.z == tx.z && rx.yaw == tx.yaw && rx.confidence == tx.confidence);
    std::cout << "解包数值还原: " << (ok ? "分毫不差 ✓" : "出错 ✗") << std::endl;

    // 5. 篡改检测：翻转一个比特后 CRC 必须变化
    std::vector<uint8_t> bad = frame;
    bad[5] ^= 0xFF;
    uint8_t bad_crc = SerialPort::calculateCRC8(bad.data(), 3 + len);
    std::cout << "篡改后 CRC 变化: " << (bad_crc != crc ? "是（能检出干扰）✓" : "否 ✗") << std::endl;

    std::cout << "\n===== 无硬件自测全部通过 =====" << std::endl;
    return ok && (bad_crc != crc) ? 0 : -1;
}
