// hw2_corrupt_test.cpp —— 校验防护测试：故意翻转一个比特，验证 CRC8 能拦下
// 使用前：同样短接 USB-TTL 的 TX/RX。与 hw1 唯一区别是解包前篡改了一个字节。
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

int main(int argc, char** argv) {
    std::string port_name = "/dev/ttyUSB0";
    if (argc >= 2) port_name = argv[1];

    SerialPort serial;
    if (!serial.openPort(port_name, 115200)) {
        std::cerr << "打开串口失败！" << std::endl;
        return -1;
    }

    PropTargetPayload tx_data;
    tx_data.target_type = 1;
    tx_data.x = 0.523f; tx_data.y = -0.185f; tx_data.z = 1.240f;
    tx_data.yaw = 15.6f; tx_data.confidence = 95;

    serial.sendPacket(0x01, &tx_data, sizeof(tx_data));

    constexpr size_t EXPECTED = 1 + 1 + 1 + sizeof(PropTargetPayload) + 1 + 1;
    uint8_t rx_raw[128] = {0};
    int bytes_read = serial.readExact(rx_raw, EXPECTED, 100);
    if (bytes_read != static_cast<int>(EXPECTED)) {
        std::cerr << "未收到完整帧！" << std::endl;
        return -1;
    }

    // ★ 关键：解包前故意翻转 Payload 里第 3 个字节（x 的中间字节）的一个位，
    //   模拟电磁干扰造成的比特翻转。
    rx_raw[5] ^= 0xFF;
    std::cout << "[已注入干扰] 翻转了 rx_raw[5] 的比特，接下来应触发 CRC8 校验失败..." << std::endl;

    if (rx_raw[0] != SerialPort::FRAME_HEADER) { std::cerr << "帧头错误！" << std::endl; return -1; }
    uint8_t rx_len = rx_raw[1];

    uint8_t calc_crc = SerialPort::calculateCRC8(rx_raw, 3 + rx_len);
    uint8_t recv_crc = rx_raw[3 + rx_len];
    if (calc_crc != recv_crc) {
        std::cerr << "CRC8 校验失败！数据已被篡改或受到噪声干扰！" << std::endl;
        std::cerr << "  (计算值 0x" << std::hex << (int)calc_crc
                  << " != 接收值 0x" << (int)recv_crc << std::dec << ")，本帧被正确丢弃。" << std::endl;
        return -1;   // 整包丢弃，绝不解包
    }

    // 走到这里说明校验被绕过（正常不会发生）
    std::cerr << "错误：篡改后 CRC 竟然通过了，检查代码！" << std::endl;
    return -1;
}
