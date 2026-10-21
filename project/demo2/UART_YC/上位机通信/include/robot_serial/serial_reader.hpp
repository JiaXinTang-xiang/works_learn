// serial_reader.hpp —— 后台串口接收线程（不依赖 ROS）
//
// 串口接收是"持续阻塞等待"的过程，绝不能放在主线程 / 视觉线程里（会把它卡住最多
// 一个读超时）。本类把「读字节 + 流式解包」跑在独立 std::thread 里，切出完整一帧
// 就回调，效果等同 CH3 文档里 SerialDriverNode::receiveLoop()，只是不拖 ROS 进来。
//
// 职责边界（与 serial_port / protocol 分层一致）：
//   SerialPort        —— 纯字节传输（打开/关闭/读/写）
//   StreamPacketParser—— 字节流 -> 帧（命令字 + 载荷）
//   SerialReader      —— 本类：把上面两者装进一个后台线程 + 回调
//
// 用法：
//   SerialPort port;
//   port.openPort("/dev/ttyUSB0");
//
//   SerialReader reader(port);
//   reader.setCallback([](uint8_t cmd, const std::vector<uint8_t>& payload) {
//       // 在这里处理下位机帧：CMD_TASK / CMD_ODOM / CMD_GYRO ...
//   });
//   reader.start();
//   // ... 主循环照常跑 ...
//   reader.stop();   // 或让析构自动停
//
// 注意：
//   - 回调在接收线程内执行：不要阻塞它，也不要抛异常；碰共享变量自己加锁。
//   - payload 是 const 引用，仅在回调内有效，要保留请拷贝。
//   - SerialReader 不拥有串口：要保证 port 在 reader 存活期间有效，且 start 前先 openPort。
//   - 本类只解「下位机 -> 上位机」帧（SOF 0xA6）。上行帧发下位机用 packFrame + writeAll。
#pragma once

#include <atomic>
#include <chrono>
#include <cstdint>
#include <functional>
#include <thread>
#include <utility>
#include <vector>

#include "robot_serial/protocol.hpp"
#include "robot_serial/serial_port.hpp"

namespace robot_serial {

// 切出一帧合法上行帧时触发的回调：cmd = 命令字，payload = 载荷（仅回调内有效）
using FrameCallback = std::function<void(uint8_t cmd, const std::vector<uint8_t>& payload)>;

class SerialReader {
public:
    // 绑定一个已打开（或稍后打开）的串口；不拥有它，生命周期由调用方保证
    explicit SerialReader(SerialPort& port) : port_(port) {}

    ~SerialReader() { stop(); }

    SerialReader(const SerialReader&) = delete;
    SerialReader& operator=(const SerialReader&) = delete;

    // 注册帧回调（start 前或后都可以，单线程下随时可换）
    void setCallback(FrameCallback cb) { callback_ = std::move(cb); }

    // 启动后台接收线程；幂等：已启动则直接返回
    void start() {
        if (running_.load(std::memory_order_acquire)) return;
        running_.store(true, std::memory_order_release);
        thread_ = std::thread([this] { run(); });
    }

    // 停止并等待线程退出；最多阻塞约一个读超时（termios VMIN=0/VTIME=1 -> ~100ms）
    void stop() {
        if (!running_.load(std::memory_order_acquire)) return;
        running_.store(false, std::memory_order_release);
        if (thread_.joinable()) thread_.join();
    }

    bool running() const { return running_.load(std::memory_order_acquire); }

private:
    void run() {
        uint8_t buf[256];
        StreamPacketParser parser;

        while (running_.load(std::memory_order_acquire)) {
            // readBytes 无数据时最多阻塞 ~100ms（VMIN=0/VTIME=1），正好当停摆检查周期
            int n = port_.readBytes(buf, sizeof(buf));
            if (n < 0) {
                // 读错误（断线 / 端口被关）：短暂退避后重试，避免忙等
                std::this_thread::sleep_for(std::chrono::milliseconds(20));
                continue;
            }

            for (int i = 0; i < n; ++i) {
                uint8_t cmd = 0;
                std::vector<uint8_t> payload;
                if (parser.parseByte(buf[i], cmd, payload) && callback_) {
                    callback_(cmd, payload);
                }
            }
        }
    }

    SerialPort& port_;
    FrameCallback callback_;
    std::thread thread_;
    std::atomic<bool> running_{false};
};

}  // namespace robot_serial
