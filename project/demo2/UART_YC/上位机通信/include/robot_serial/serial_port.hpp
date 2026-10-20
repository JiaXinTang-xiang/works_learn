// serial_port.hpp —— Linux 串口驱动（termios Raw Mode + RAII）
//
// 纯字节流传输层：只负责打开/关闭/读写，不关心任何协议帧。
// 协议帧的打包/解包在 protocol.hpp，上层业务永远不直接碰串口 —— 换硬件只改本层。
#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

namespace robot_serial {

class SerialPort {
public:
    SerialPort() = default;
    ~SerialPort();

    // 禁止拷贝（避免文件描述符被二次关闭）
    SerialPort(const SerialPort&) = delete;
    SerialPort& operator=(const SerialPort&) = delete;

    // 打开并配置串口（Raw Mode + 8N1 + 100ms 读超时）
    bool openPort(const std::string& device_path, int baudrate = 115200);
    void closePort();
    bool isOpened() const { return fd_ >= 0; }

    // 写一整段字节（阻塞写满）
    bool writeAll(const uint8_t* data, size_t len);

    // 单次读（有多少读多少，返回实际字节数，<0 表示错误）
    int readBytes(uint8_t* buffer, size_t max_len);

    // 循环读满 target_len 或达到超时（防范流式分包，只读半包）
    int readExact(uint8_t* buffer, size_t target_len, int timeout_ms = 100);

private:
    int fd_ = -1;

    // 波特率数值 -> termios 常量
    static int baudrateConstant(int baudrate);
};

}  // namespace robot_serial
