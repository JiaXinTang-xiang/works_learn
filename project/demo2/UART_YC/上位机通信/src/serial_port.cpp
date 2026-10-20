// serial_port.cpp —— Linux 串口驱动实现（termios Raw Mode）
#include "robot_serial/serial_port.hpp"

#include <cerrno>
#include <chrono>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <thread>
#include <termios.h>
#include <unistd.h>

namespace robot_serial {

SerialPort::~SerialPort() {
    closePort();
}

bool SerialPort::openPort(const std::string& device_path, int baudrate) {
    closePort();

    // O_NOCTTY: 不作为控制终端；O_NDELAY: 非阻塞打开，稍后改回
    fd_ = ::open(device_path.c_str(), O_RDWR | O_NOCTTY | O_NDELAY);
    if (fd_ < 0) {
        std::cerr << "[SerialPort] 打开失败: " << device_path
                  << " (" << std::strerror(errno) << ")" << std::endl;
        return false;
    }

    // 改回阻塞模式（配合 VMIN/VTIME 实现读超时）
    ::fcntl(fd_, F_SETFL, 0);

    struct termios tty;
    if (::tcgetattr(fd_, &tty) != 0) {
        std::cerr << "[SerialPort] 读取 termios 配置失败" << std::endl;
        closePort();
        return false;
    }

    // Raw Mode：关闭行缓冲 / 字符翻译 / 信号 / 回显（二进制帧字节不许被内核改写）
    ::cfmakeraw(&tty);

    int speed = baudrateConstant(baudrate);
    ::cfsetispeed(&tty, speed);
    ::cfsetospeed(&tty, speed);

    // 8N1
    tty.c_cflag |= (CLOCAL | CREAD);   // 忽略调制解调器状态线，启用接收器
    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= CS8;                // 8 数据位
    tty.c_cflag &= ~PARENB;            // 无奇偶校验
    tty.c_cflag &= ~CSTOPB;            // 1 停止位
    tty.c_cflag &= ~CRTSCTS;           // 关硬件流控

    // 读超时：VMIN=0, VTIME=1 => 最多挂起 100ms 后返回 0（防止掉线时 read 永久卡死）
    tty.c_cc[VMIN] = 0;
    tty.c_cc[VTIME] = 1;

    ::tcflush(fd_, TCIOFLUSH);
    if (::tcsetattr(fd_, TCSANOW, &tty) != 0) {
        std::cerr << "[SerialPort] 应用 termios 配置失败" << std::endl;
        closePort();
        return false;
    }

    std::cout << "[SerialPort] " << device_path << " 打开成功, 波特率 " << baudrate << std::endl;
    return true;
}

void SerialPort::closePort() {
    if (fd_ >= 0) {
        ::close(fd_);
        fd_ = -1;
    }
}

bool SerialPort::writeAll(const uint8_t* data, size_t len) {
    if (fd_ < 0 || data == nullptr) return false;
    size_t written = 0;
    while (written < len) {
        ssize_t n = ::write(fd_, data + written, len - written);
        if (n < 0) {
            if (errno == EINTR) continue;   // 被信号打断则重试
            return false;
        }
        written += static_cast<size_t>(n);
    }
    return true;
}

int SerialPort::readBytes(uint8_t* buffer, size_t max_len) {
    if (fd_ < 0) return -1;
    return static_cast<int>(::read(fd_, buffer, max_len));
}

int SerialPort::readExact(uint8_t* buffer, size_t target_len, int timeout_ms) {
    if (fd_ < 0) return -1;
    size_t got = 0;
    auto start = std::chrono::steady_clock::now();

    while (got < target_len) {
        ssize_t n = ::read(fd_, buffer + got, target_len - got);
        if (n > 0) {
            got += static_cast<size_t>(n);
        } else if (n < 0 && errno != EINTR) {
            break;
        }
        auto now = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count() > timeout_ms) {
            break;
        }
        std::this_thread::sleep_for(std::chrono::microseconds(200));
    }
    return static_cast<int>(got);
}

int SerialPort::baudrateConstant(int baudrate) {
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

}  // namespace robot_serial
