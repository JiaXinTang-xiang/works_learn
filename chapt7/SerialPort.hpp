// SerialPort.hpp —— Linux 串口 RAII 封装（termios Raw Mode + 二进制帧封包 + CRC8）
//
// 关键点：
//   1. termios 原始模式（cfmakeraw）剔除行缓冲/字符翻译/信号/回显；
//   2. 二进制帧 [Header][Len][Cmd][Payload][CRC8][Tail] 定长封包；
//   3. CRC8 校验防电磁干扰比特翻转；
//   4. RAII 管理文件描述符，析构自动 close。
#pragma once
#include <iostream>
#include <string>
#include <vector>
#include <cstring>      // memcpy, strerror
#include <cstdint>      // uint8_t
#include <cerrno>       // errno
#include <chrono>
#include <thread>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>

class SerialPort {
public:
    static constexpr uint8_t FRAME_HEADER = 0xA5;
    static constexpr uint8_t FRAME_TAIL   = 0x5A;

    // CRC8 校验（多项式 0x31），发送端与接收端统一复用
    static uint8_t calculateCRC8(const uint8_t* data, size_t length) {
        uint8_t crc = 0x00;
        for (size_t i = 0; i < length; ++i) {
            crc ^= data[i];
            for (int j = 0; j < 8; ++j) {
                if (crc & 0x80) crc = (crc << 1) ^ 0x31;
                else            crc <<= 1;
            }
        }
        return crc;
    }

    // 把载荷打包成一帧（不碰串口，方便无硬件自测）
    static void buildFrame(uint8_t cmd_id, const void* payload, uint8_t payload_len,
                           std::vector<uint8_t>& out) {
        out.resize(3 + payload_len + 2);
        out[0] = FRAME_HEADER;
        out[1] = payload_len;
        out[2] = cmd_id;
        std::memcpy(&out[3], payload, payload_len);
        out[3 + payload_len]     = calculateCRC8(out.data(), 3 + payload_len); // 校验 Header+Len+Cmd+Payload
        out[3 + payload_len + 1] = FRAME_TAIL;
    }

    SerialPort() : fd_(-1) {}

    // RAII：析构时自动关闭
    ~SerialPort() { closePort(); }

    // 禁止拷贝（防止 fd 被二次关闭），允许移动
    SerialPort(const SerialPort&) = delete;
    SerialPort& operator=(const SerialPort&) = delete;
    SerialPort(SerialPort&& other) noexcept : fd_(other.fd_) { other.fd_ = -1; }

    // 打开并配置串口为 8N1 Raw Mode
    bool openPort(const std::string& device_path, int baudrate = 115200) {
        closePort();

        fd_ = open(device_path.c_str(), O_RDWR | O_NOCTTY | O_NDELAY);
        if (fd_ < 0) {
            std::cerr << "[SerialPort] 无法打开 " << device_path
                      << " (" << strerror(errno) << ")" << std::endl;
            return false;
        }
        fcntl(fd_, F_SETFL, 0);   // 恢复为阻塞/超时读取模式

        struct termios tty;
        if (tcgetattr(fd_, &tty) != 0) { closePort(); return false; }

        cfmakeraw(&tty);          // Raw Mode：清空行缓冲/字符翻译/信号/回显

        speed_t speed = getBaudrateConstant(baudrate);
        cfsetispeed(&tty, speed);
        cfsetospeed(&tty, speed);

        tty.c_cflag |= (CLOCAL | CREAD);
        tty.c_cflag &= ~CSIZE;
        tty.c_cflag |= CS8;       // 8 数据位
        tty.c_cflag &= ~PARENB;   // 无奇偶校验
        tty.c_cflag &= ~CSTOPB;   // 1 停止位
        tty.c_cflag &= ~CRTSCTS;  // 关闭硬件流控

        tty.c_cc[VMIN]  = 0;      // 不设最小读取字节数
        tty.c_cc[VTIME] = 1;      // 超时 1 * 100ms = 100ms

        tcflush(fd_, TCIOFLUSH);
        if (tcsetattr(fd_, TCSANOW, &tty) != 0) { closePort(); return false; }

        std::cout << "[SerialPort] " << device_path << " 打开成功，波特率 " << baudrate << std::endl;
        return true;
    }

    bool isOpened() const { return fd_ >= 0; }

    void closePort() {
        if (fd_ >= 0) { close(fd_); fd_ = -1; }
    }

    // 封包并写入物理串口
    bool sendPacket(uint8_t cmd_id, const void* payload, uint8_t payload_len) {
        if (fd_ < 0 || payload == nullptr || payload_len == 0) return false;
        std::vector<uint8_t> tx;
        buildFrame(cmd_id, payload, payload_len, tx);
        ssize_t n = write(fd_, tx.data(), tx.size());
        return n == static_cast<ssize_t>(tx.size());
    }

    // 单次读取原始字节（供流式状态机逐字节消费）
    int readBytes(uint8_t* buffer, size_t max_len) {
        if (fd_ < 0) return -1;
        return static_cast<int>(read(fd_, buffer, max_len));
    }

    // 循环读满 target_len 字节，或超时（防流式分包导致的半截包）
    int readExact(uint8_t* buffer, size_t target_len, int timeout_ms = 100) {
        if (fd_ < 0) return -1;
        size_t total = 0;
        auto start = std::chrono::steady_clock::now();
        while (total < target_len) {
            ssize_t n = read(fd_, buffer + total, target_len - total);
            if (n > 0) total += n;
            auto now = std::chrono::steady_clock::now();
            if (std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count() > timeout_ms)
                break;
            std::this_thread::sleep_for(std::chrono::microseconds(500));
        }
        return static_cast<int>(total);
    }

private:
    int fd_;

    static speed_t getBaudrateConstant(int baudrate) {
        switch (baudrate) {
            case 9600:   return B9600;
            case 19200:  return B19200;
            case 38400:  return B38400;
            case 57600:  return B57600;
            case 115200: return B115200;
            case 230400: return B230400;
            case 460800: return B460800;
            case 921600: return B921600;
            default:     return B115200;
        }
    }
};
