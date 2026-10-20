// test_loopback.cpp —— 硬件回环自测
//
// 用一根杜邦线把 USB-TTL 模块的 TX 与 RX 短接，程序发送帧后自己读回并全链路校验：
//   测试1：上位机 -> 下位机帧（物料盘 0x11）—— 打包 -> 发送 -> 读回 -> 逐字节比对
//   测试2：下位机 -> 上位机帧（任务触发 0x01）—— 构造 -> 发送 -> 读回 -> 流式状态机解包
//
// 编译：cmake -B build && cmake --build build -j
// 运行：./build/test_loopback [串口设备路径]    （默认 /dev/ttyTHS1）
#include <chrono>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <thread>
#include <vector>

#include "robot_serial/protocol.hpp"
#include "robot_serial/serial_port.hpp"

using namespace robot_serial;

static void hexDump(const uint8_t* data, size_t len) {
    for (size_t i = 0; i < len; ++i) {
        std::cout << std::hex << std::setw(2) << std::setfill('0')
                  << static_cast<int>(data[i]) << ' ';
    }
    std::cout << std::dec << std::endl;
}

int main(int argc, char** argv) {
    std::string port = (argc > 1) ? argv[1] : "/dev/ttyTHS1";

    SerialPort serial;
    if (!serial.openPort(port, 115200)) {
        std::cerr << "打开失败。请检查：设备路径 / 权限(sudo usermod -aG dialout $USER) / 是否短接 TX-RX。"
                  << std::endl;
        return -1;
    }

    // ---- 测试1：上位机 -> 下位机 物料盘帧，打包 + 回环逐字节比对 ----
    {
        TrayPayload tray{320, 240};
        auto frame = packFrame(CMD_TRAY, &tray, sizeof(tray));
        std::cout << "[测试1] 发送物料盘帧 (" << frame.size() << " 字节): ";
        hexDump(frame.data(), frame.size());

        if (!serial.writeAll(frame.data(), frame.size())) {
            std::cerr << "[测试1] 发送失败" << std::endl;
            return -1;
        }

        std::vector<uint8_t> rx(frame.size());
        int n = serial.readExact(rx.data(), rx.size(), 200);
        std::cout << "[测试1] 回环收到 " << n << " 字节: ";
        hexDump(rx.data(), static_cast<size_t>(n));

        if (n == static_cast<int>(frame.size()) &&
            std::memcmp(rx.data(), frame.data(), frame.size()) == 0) {
            std::cout << "[测试1] ✅ 打包 + 传输 + CRC16 通过（回环逐字节一致）" << std::endl;
        } else {
            std::cout << "[测试1] ❌ 回环数据不一致，检查接线 / 波特率 / termios" << std::endl;
            return -1;
        }
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(100));  // 等上一帧回显彻底清空

    // ---- 测试2：下位机 -> 上位机 任务触发帧，流式状态机解包 ----
    {
        TaskPayload task{0x03, 0x00};  // 任务 = 色环，目标 = 无
        // 手工构造上行帧（SOF 0xA6）
        std::vector<uint8_t> frame;
        frame.push_back(SOF_UP);
        frame.push_back(sizeof(TaskPayload));
        frame.push_back(CMD_TASK);
        const uint8_t* p = reinterpret_cast<const uint8_t*>(&task);
        for (size_t i = 0; i < sizeof(TaskPayload); ++i) frame.push_back(p[i]);
        uint16_t crc = CRC16::calc(frame.data() + 1, frame.size() - 1);
        frame.push_back(static_cast<uint8_t>(crc & 0xFF));
        frame.push_back(static_cast<uint8_t>(crc >> 8));
        frame.push_back(EOF_BYTE);

        std::cout << "[测试2] 发送任务触发帧 (" << frame.size() << " 字节): ";
        hexDump(frame.data(), frame.size());

        if (!serial.writeAll(frame.data(), frame.size())) {
            std::cerr << "[测试2] 发送失败" << std::endl;
            return -1;
        }

        std::vector<uint8_t> rx(frame.size());
        int n = serial.readExact(rx.data(), rx.size(), 200);

        StreamPacketParser parser;
        uint8_t cmd = 0;
        std::vector<uint8_t> payload;
        bool ok = false;
        for (int i = 0; i < n; ++i) {
            if (parser.parseByte(rx[i], cmd, payload)) {
                ok = true;
                break;
            }
        }

        if (ok && cmd == CMD_TASK && payload.size() == sizeof(TaskPayload)) {
            TaskPayload got;
            std::memcpy(&got, payload.data(), sizeof(got));  // memcpy 防非对齐访问
            std::cout << "[测试2] ✅ 状态机解包通过: cmd=0x" << std::hex
                      << static_cast<int>(cmd) << std::dec
                      << " task=" << static_cast<int>(got.task)
                      << " target=" << static_cast<int>(got.target) << std::endl;
        } else {
            std::cout << "[测试2] ❌ 状态机解包失败" << std::endl;
            return -1;
        }
    }

    std::cout << "\n===== 全链路自测通过，通信链路正常 =====" << std::endl;
    return 0;
}
