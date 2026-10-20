// crc16.hpp —— CRC16/Modbus 查表校验
//
// 多项式 0xA001、初值 0xFFFF、查表法（256 项表在编译期生成，避免手写 256 个魔数出错）。
// 上位机与下位机（STM32）共用同一算法：任一比特翻转都会导致校验失败，整帧丢弃。
#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace robot_serial {
namespace detail {

// 编译期生成查表：c 右移时低位为 1 则异或 0xA001（反射多项式）。
// 放在 CRC16 之前定义，避免类内 static constexpr 初始化时类体尚未完整的问题。
constexpr std::array<uint16_t, 256> makeCrc16Table() {
    std::array<uint16_t, 256> t{};
    for (uint32_t i = 0; i < 256; ++i) {
        uint16_t c = static_cast<uint16_t>(i);
        for (int j = 0; j < 8; ++j) {
            c = (c & 1) ? static_cast<uint16_t>(0xA001 ^ (c >> 1))
                        : static_cast<uint16_t>(c >> 1);
        }
        t[i] = c;
    }
    return t;
}

}  // namespace detail

class CRC16 {
public:
    // 计算 data[0..len) 的 CRC16/Modbus
    static uint16_t calc(const uint8_t* data, size_t len) {
        uint16_t crc = 0xFFFF;
        for (size_t i = 0; i < len; ++i) {
            crc = static_cast<uint16_t>((crc >> 8) ^ table_[(crc ^ data[i]) & 0xFF]);
        }
        return crc;
    }

private:
    // C++17 起 constexpr static 数据成员隐式 inline，无需在 .cpp 中再定义
    static constexpr std::array<uint16_t, 256> table_ = detail::makeCrc16Table();
};

}  // namespace robot_serial
