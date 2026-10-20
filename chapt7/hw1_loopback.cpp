// hw1_loopback.cpp —— 串口硬件回环自测：发送 → 接收 → 解包 → 全校验
// 使用前：把 USB-TTL 模块的 TX 和 RX 用杜邦线短接（自发自收）。
#include <iostream>
#include <iomanip>
#include <cstring>
#include "SerialPort.hpp"

// 道具目标坐标载荷（#pragma pack(1) 强制 1 字节对齐，剔除编译器填充）
#pragma pack(push, 1)
struct PropTargetPayload {
    uint8_t  target_type;   // 1 字节：目标类别
    float    x;             // 4 字节：X (米)
    float    y;             // 4 字节：Y (米)
    float    z;             // 4 字节：Z (米)
    float    yaw;           // 4 字节：偏航角 (度)
    uint8_t  confidence;    // 1 字节：置信度 0~100
};
#pragma pack(pop)

int main(int argc, char** argv) {
    // 串口路径：默认 /dev/ttyUSB0，也可命令行传入，如 ./hw1_loopback /dev/ttyACM0
    std::string port_name = "/dev/ttyUSB0";
    if (argc >= 2) port_name = argv[1];

    SerialPort serial;
    if (!serial.openPort(port_name, 115200)) {
        std::cerr << "打开串口失败！请检查设备，或执行: sudo usermod -a -G dialout $USER" << std::endl;
        return -1;
    }
    std::cout << "已打开 " << port_name << "，开始回环自测（请先短接 TX/RX）..." << std::endl;

    // 1. 准备一帧模拟视觉解算数据
    PropTargetPayload tx_data;
    tx_data.target_type = 1;
    tx_data.x = 0.523f;
    tx_data.y = -0.185f;
    tx_data.z = 1.240f;
    tx_data.yaw = 15.6f;
    tx_data.confidence = 95;

    // 2. 封包发送（命令字 0x01）
    if (!serial.sendPacket(0x01, &tx_data, sizeof(tx_data))) {
        std::cerr << "发送失败！" << std::endl;
        return -1;
    }
    std::cout << "[发送成功] Payload " << sizeof(tx_data) << " 字节" << std::endl;

    // 3. 接收完整帧：1+1+1+18+1+1 = 23 字节
    constexpr size_t EXPECTED = 1 + 1 + 1 + sizeof(PropTargetPayload) + 1 + 1;
    uint8_t rx_raw[128] = {0};
    int bytes_read = serial.readExact(rx_raw, EXPECTED, 100);

    std::cout << "[接收] 收到 " << bytes_read << " 字节" << std::endl;
    if (bytes_read != static_cast<int>(EXPECTED)) {
        std::cerr << "未收到完整帧！请确认 TX/RX 是否已短接。" << std::endl;
        return -1;
    }

    // 打印原始十六进制帧
    std::cout << "原始帧: ";
    for (int i = 0; i < bytes_read; ++i)
        std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)rx_raw[i] << " ";
    std::cout << std::dec << std::endl;

    // 4. 帧头 + 长度检查
    if (rx_raw[0] != SerialPort::FRAME_HEADER) { std::cerr << "帧头错误！" << std::endl; return -1; }
    uint8_t rx_len = rx_raw[1];
    if (rx_len != sizeof(PropTargetPayload)) { std::cerr << "载荷长度不匹配！" << std::endl; return -1; }

    // 5. CRC8 + 帧尾检查（校验范围：Header + Len + Cmd + Payload）
    uint8_t calc_crc  = SerialPort::calculateCRC8(rx_raw, 3 + rx_len);
    uint8_t recv_crc  = rx_raw[3 + rx_len];
    uint8_t recv_tail = rx_raw[3 + rx_len + 1];
    if (calc_crc != recv_crc) { std::cerr << "CRC8 校验失败！数据被篡改或受干扰！" << std::endl; return -1; }
    if (recv_tail != SerialPort::FRAME_TAIL) { std::cerr << "帧尾错误！" << std::endl; return -1; }

    // 6. memcpy 安全解包（严禁指针强转，防 ARM 非对齐 HardFault）
    PropTargetPayload rx_data;
    std::memcpy(&rx_data, &rx_raw[3], sizeof(PropTargetPayload));

    // 7. 验证浮点数值
    std::cout << "\n===== 全校验通过，数据还原 =====" << std::endl;
    std::cout << "类别: " << (int)rx_data.target_type << std::endl;
    std::cout << "X: " << rx_data.x << " (原值 " << tx_data.x << ")" << std::endl;
    std::cout << "Y: " << rx_data.y << " (原值 " << tx_data.y << ")" << std::endl;
    std::cout << "Z: " << rx_data.z << " (原值 " << tx_data.z << ")" << std::endl;
    std::cout << "Yaw: " << rx_data.yaw << " (原值 " << tx_data.yaw << ")" << std::endl;
    std::cout << "置信度: " << (int)rx_data.confidence << " %" << std::endl;
    std::cout << "==============================" << std::endl;
    return 0;
}
