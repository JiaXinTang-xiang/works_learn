// hw1_multithread.cpp —— 多线程取流：采图线程 + 算法线程，验证"防积压"
#include <opencv2/opencv.hpp>
#include <iostream>
#include <thread>
#include <atomic>
#include <chrono>
#include "PingPongFrameBuffer.hpp"

std::atomic<bool> is_running(true);   // 线程间共享的退出开关

// 采图工作线程（生产者）
void captureThreadFunc(PingPongFrameBuffer& buffer) {
    // 打开摄像头（by-id 持久路径，插拔不变），失败回退 index 0
    cv::VideoCapture cap("/dev/v4l/by-id/usb-Sonix_Technology_Co.__Ltd._HD_Webcam-video-index0");
    if (!cap.isOpened()) cap.open(0);
    if (!cap.isOpened()) {
        std::cerr << "[采图线程] 无法打开摄像头！" << std::endl;
        is_running = false;
        return;
    }

    // 关键：Linux V4L2 内核环形缓冲区压到 1 帧，杜绝硬件层积压
    cap.set(cv::CAP_PROP_BUFFERSIZE, 1);
    cap.set(cv::CAP_PROP_FPS, 60);

    std::cout << "[采图线程] 启动成功，正在高速取流..." << std::endl;

    cv::Mat raw_frame;
    int capture_count = 0;
    auto last_time = std::chrono::steady_clock::now();

    while (is_running) {
        cap >> raw_frame;
        if (raw_frame.empty()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
            continue;
        }

        buffer.update(raw_frame);   // 零拷贝送入双缓冲
        capture_count++;

        auto now = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::seconds>(now - last_time).count() >= 1) {
            std::cout << "[采图线程] 采集帧率: " << capture_count << " FPS" << std::endl;
            capture_count = 0;
            last_time = now;
        }
    }

    cap.release();
    std::cout << "[采图线程] 安全退出。" << std::endl;
}

int main() {
    PingPongFrameBuffer frame_buffer;
    std::thread capture_thread(captureThreadFunc, std::ref(frame_buffer));

    cv::Mat current_frame;
    int process_count = 0;
    auto last_time = std::chrono::steady_clock::now();

    std::cout << "[算法主线程] 启动成功，按 ESC 退出。" << std::endl;

    while (is_running) {
        // 1. 只取最新一帧（纳秒级零拷贝接管）
        if (!frame_buffer.getLatest(current_frame)) continue;

        // 2. 模拟耗时算法（作业 2 改这里：25ms → 50ms / 100ms）
        std::this_thread::sleep_for(std::chrono::milliseconds(50));

        // 3. 画个标注，证明显示的是当前帧
        cv::putText(current_frame, "Real-time Live", cv::Point(30, 40),
                    cv::FONT_HERSHEY_SIMPLEX, 0.8, cv::Scalar(0, 255, 0), 2);
        cv::imshow("Multi-thread Vision", current_frame);
        process_count++;

        auto now = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::seconds>(now - last_time).count() >= 1) {
            std::cout << "[算法主线程] 处理帧率: " << process_count << " FPS" << std::endl;
            process_count = 0;
            last_time = now;
        }

        if (cv::waitKey(1) == 27) {   // ESC 退出
            is_running = false;
            break;
        }
    }

    if (capture_thread.joinable()) capture_thread.join();
    cv::destroyAllWindows();
    std::cout << "程序完全退出。" << std::endl;
    return 0;
}
