// main.cpp —— 生产者-消费者主线程调度与软硬件闭环（结业终局整合）
#include <iostream>
#include <string>
#include <thread>
#include <atomic>
#include <chrono>
#include <cstdio>    // std::snprintf
#include <cmath>     // std::cos / std::sin
#include <cstdint>   // uint8_t

#include "PingPongFrameBuffer.hpp"
#include "TargetDetector.hpp"
#include "PnPSolver.hpp"
#include "SerialPort.hpp"

#pragma pack(push, 1)
struct PropTargetPacket {
    uint8_t  target_type; // 0: 丢失, 1: 红色, 2: 蓝色
    float    x;           // 机器人坐标系 X (米, 前方)
    float    y;           // 机器人坐标系 Y (米, 左方)
    float    z;           // 机器人坐标系 Z (米, 上方)
    float    yaw;         // 目标偏航角度 (度)
    uint8_t  confidence;  // 置信度 (0~100)
};
#pragma pack(pop)

std::atomic<bool> is_running(true);

// 采图工作子线程（生产者）
void cameraCaptureTask(PingPongFrameBuffer& buffer) {
    // 改成你的相机：默认索引 0；若多相机，用 /dev/v4l/by-id/... 路径更稳
    cv::VideoCapture cap(0);
    if (!cap.isOpened()) {
        std::cerr << "[相机线程] 无法打开摄像头！" << std::endl;
        is_running = false;
        return;
    }

    // 关键硬件队列限制：驱动层只保留 1 帧，彻底防积压
    cap.set(cv::CAP_PROP_BUFFERSIZE, 1);
    cap.set(cv::CAP_PROP_FPS, 60);

    cv::Mat frame;
    while (is_running) {
        cap >> frame;
        if (frame.empty()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
            continue;
        }
        // 零拷贝指针交换放入最新缓冲区
        buffer.update(frame);
    }

    cap.release();
    std::cout << "[相机线程] 安全退出。" << std::endl;
}

int main(int argc, char** argv) {
    // 检查是否开启 GUI 窗口（默认车载无屏幕运行）
    bool show_gui = false;
    for (int i = 1; i < argc; ++i) {
        if (std::string(argv[i]) == "--gui") show_gui = true;
    }

    // 1. 初始化模块
    PingPongFrameBuffer frame_buffer;
    TargetDetector detector;

    // 初始化 PnP 解算器（根据相机真实内参填入，假设道具尺寸 0.15m x 0.15m）
    PnPSolver pnp(920.0, 920.0, 640.0, 360.0, {0.0, 0.0, 0.0, 0.0, 0.0}, 0.15f, 0.15f);

    SerialPort serial;
    std::string serial_dev = "/dev/ttyUSB0";
    if (!serial.openPort(serial_dev, 115200)) {
        std::cerr << "[警告] 串口 " << serial_dev << " 打开失败，仅开启纯视觉计算模式！" << std::endl;
    }

    // 2. 启动采图后台线程
    std::thread cap_thread(cameraCaptureTask, std::ref(frame_buffer));

    cv::Mat current_frame;
    PropTargetPacket send_packet = {0};

    std::cout << "[主控系统] 机器人视觉追踪系统启动成功！" << std::endl;
    if (show_gui) std::cout << "[主控系统] GUI 调试窗口已激活 (按 ESC 退出)" << std::endl;

    while (is_running) {
        // 3. 从双缓冲队列获取最新鲜的一帧（零拷贝接管所有权）
        if (!frame_buffer.getLatest(current_frame)) {
            // 无新帧时让出 CPU 休眠 1ms，避免空转自旋导致 CPU 单核 100% 满载发热
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            continue;
        }

        // 4. 执行颜色与几何检测
        DetectionResult det = detector.detect(current_frame, true /* 识别红色道具 */);

        if (det.is_detected) {
            // 5. 空间姿态与距离解算
            PoseResult pose = pnp.solve(det.corners);

            if (pose.is_valid) {
                // 6. 相机坐标系 -> 机器人底盘本体坐标系转换
                //   （假设相机在车架上向下倾斜 20 度安装，前向偏置 0.10m，离地基准高度 0.25m）
                constexpr float CAMERA_PITCH_DEG = 20.0f;
                constexpr float theta_rad = CAMERA_PITCH_DEG * 3.1415926535f / 180.0f;
                float cos_p = std::cos(theta_rad);
                float sin_p = std::sin(theta_rad);

                // 旋转补偿向下俯仰角
                float x_level = pose.x;
                float y_level = pose.y * cos_p - pose.z * sin_p;
                float z_level = pose.y * sin_p + pose.z * cos_p;

                send_packet.target_type = det.target_type;
                send_packet.x = z_level + 0.10f; // 车头向前: 等效水平深度 + 前向安装距
                send_packet.y = -x_level;        // 车身向左: 相机向右为正，底盘向左为正，取负
                send_packet.z = -y_level + 0.25f;// 离地高度: 相机向下为正，取负后加上离地基准高
                send_packet.yaw = -pose.yaw;     // 关键：偏航角严格取反，符合底盘坐标系极性规范
                send_packet.confidence = static_cast<uint8_t>(det.confidence);

                if (show_gui) {
                    // 可视化：绘制 4 个角点与物理坐标
                    for (size_t i = 0; i < 4; ++i) {
                        cv::line(current_frame, det.corners[i], det.corners[(i + 1) % 4],
                                 cv::Scalar(0, 255, 0), 2);
                    }
                    char info_str[128];
                    std::snprintf(info_str, sizeof(info_str), "X:%.2fm Y:%.2fm Z:%.2fm Yaw:%.1fdeg",
                                  send_packet.x, send_packet.y, send_packet.z, send_packet.yaw);
                    cv::putText(current_frame, info_str, cv::Point(30, 50),
                                cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(0, 255, 255), 2);
                }
            } else {
                // 解算失败，降级为丢失状态
                send_packet = {0};
            }
        } else {
            // 视野内未检测到目标：发送安全归零包
            send_packet = {0};
        }

        // 7. 打包并通过串口发送至电控单片机（命令字 0x01）
        if (serial.isOpened()) {
            serial.sendPacket(0x01, &send_packet, sizeof(send_packet));
        }

        // 8. 调试窗口展示
        if (show_gui) {
            cv::imshow("Robot Vision Pipeline", current_frame);
            if (cv::waitKey(1) == 27) { // 按 ESC 键主动退出
                is_running = false;
                break;
            }
        }
    }

    // 9. 安全退出与资源回收
    if (cap_thread.joinable()) {
        cap_thread.join();
    }
    serial.closePort();
    if (show_gui) cv::destroyAllWindows();

    std::cout << "[主控系统] 视觉程序安全退出。" << std::endl;
    return 0;
}
