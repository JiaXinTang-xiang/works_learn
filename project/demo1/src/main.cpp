// main.cpp —— 红色瓶盖识别(海康工业相机版)程序入口
//
// 架构：海康 SDK 采图线程 → 转独立 BGR 帧 → 容量 1 的最新帧槽 → 主线程
//       红色检测 + 显示 + 鼠标 HSV 分析。思想来自 chapt6。
#include <opencv2/opencv.hpp>

#include <atomic>
#include <chrono>
#include <cstdio>
#include <iostream>
#include <thread>

#include "config.hpp"
#include "hik_camera.hpp"
#include "latest_frame_buffer.hpp"
#include "red_detector.hpp"
#include "vision_ui.hpp"

static std::atomic<bool> g_running{true};

// 采图线程：海康取流 → 转 BGR → 发布到最新帧槽(两个 scratch 交替复用，避免重复分配)。
static void captureThread(HikCamera& cam, LatestFrameBuffer& buf) {
    cv::Mat scratch[2];
    int idx = 0;
    int frames = 0;
    auto last = std::chrono::steady_clock::now();

    while (g_running) {
        if (!cam.grabToBgr(scratch[idx])) {
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
            continue;
        }
        scratch[idx] = buf.publish(scratch[idx]);  // 换入新帧，拿回上一帧复用
        idx = 1 - idx;
        ++frames;

        auto now = std::chrono::steady_clock::now();
        double sec = std::chrono::duration<double>(now - last).count();
        if (sec >= 1.0) {
            std::cout << "[采图线程] " << (frames / sec) << " FPS" << std::endl;
            frames = 0;
            last = now;
        }
    }
}

int main() {
    std::cout << std::string(60, '=') << std::endl;
    std::cout << "  红色瓶盖识别系统 (海康工业相机 + C++ 多线程)" << std::endl;
    std::cout << std::string(60, '=') << std::endl;

    // 1. 枚举并打开相机
    auto devices = HikCamera::listDevices();
    if (devices.empty()) {
        std::cerr << "[错误] 未找到海康相机。请检查：供电 / 网线或USB / 驱动 / 权限，然后重试。"
                  << std::endl;
        return 1;
    }
    std::cout << "枚举到 " << devices.size() << " 个设备:" << std::endl;
    for (size_t i = 0; i < devices.size(); ++i) {
        std::cout << "  [" << i << "] " << devices[i] << std::endl;
    }

    HikCamera cam;
    if (!cam.open(cfg::CAMERA_DEVICE_INDEX)) {
        std::cerr << "[错误] 打开相机失败。" << std::endl;
        return 1;
    }

    cam.setExposureUs(cfg::CAMERA_EXPOSURE_US);
    cam.setGain(cfg::CAMERA_GAIN);
    cam.setFramerate(cfg::CAMERA_FRAMERATE);
    if (!cam.startGrabbing()) {
        std::cerr << "[错误] 开始采集失败。" << std::endl;
        return 1;
    }

    // 2. 最新帧槽 + 采图线程
    LatestFrameBuffer frameBuf;
    std::thread capThread(captureThread, std::ref(cam), std::ref(frameBuf));

    // 3. 检测器 + UI
    RedDetector detector;
    detector.lowerRed1 = cfg::LOWER_RED1;
    detector.upperRed1 = cfg::UPPER_RED1;
    detector.lowerRed2 = cfg::LOWER_RED2;
    detector.upperRed2 = cfg::UPPER_RED2;
    detector.gaussianSize = cfg::GAUSSIAN_SIZE;
    detector.closeKernel = cfg::CLOSE_KERNEL;
    detector.openKernel = cfg::OPEN_KERNEL;
    detector.minArea = cfg::MIN_AREA;
    detector.maxArea = cfg::MAX_AREA;
    detector.minCircularity = cfg::MIN_CIRCULARITY;

    VisionUi ui(detector);
    ui.setup("Red Cap Detection");

    // 4. 主循环
    cv::Mat frame;
    int procCount = 0;
    double fps = 0.0;
    auto fpsT0 = std::chrono::steady_clock::now();

    std::cout << "[主线程] 启动成功，按 Q/ESC 退出。" << std::endl;

    while (g_running) {
        if (!frameBuf.tryTakeLatest(frame)) {
            // 暂无新帧(相机未就绪/超时)，仍要响应键盘。
            if (cv::waitKey(1) == 27) break;
            continue;
        }

        // 显示缩放(与参考 Python 一致：宽>阈值时等比缩小，串口坐标为缩放后坐标系)
        if (cfg::MAX_DISPLAY_WIDTH > 0 && frame.cols > cfg::MAX_DISPLAY_WIDTH) {
            double s = static_cast<double>(cfg::MAX_DISPLAY_WIDTH) / frame.cols;
            cv::resize(frame, frame, cv::Size(), s, s);
        }

        // 记录干净帧给鼠标 HSV 分析(浅拷贝，绘制在另一份 clone 上，不污染本帧)
        ui.setCurrentFrame(frame);

        RedDetectionResult r = detector.detect(frame);

        // 绘制(独立显示帧，保留原始帧供 HSV 分析)
        cv::Mat display = frame.clone();
        ui.drawDetection(display, r);
        ui.drawHud(display, r, fps);

        // FPS 统计
        ++procCount;
        double el = std::chrono::duration<double>(std::chrono::steady_clock::now() - fpsT0).count();
        if (el >= 0.5) {
            fps = procCount / el;
            procCount = 0;
            fpsT0 = std::chrono::steady_clock::now();
        }

        ui.render(display, r);

        int key = cv::waitKey(1);
        if (key != -1) {
            // 相机曝光/增益实时微调：画面黑就先按 E 加大曝光，直到看清为止。
            if (key == 'e' || key == 'E') {
                double step = (key == 'E') ? 5000.0 : -5000.0; // E 增亮 / e 减暗
                cam.setExposureUs(std::max(100.0, cam.exposureUs() + step));
            } else if (key == 'g' || key == 'G') {
                double step = (key == 'G') ? 1.0 : -1.0;       // G 增 / g 减
                cam.setGain(std::max(0.0, cam.gain() + step));
            }
            if (ui.handleKey(key)) break;   // Q/ESC 退出
            if (ui.saveRequested) {
                ui.saveImage(display);
                ui.saveRequested = false;
            }
        }
    }

    // 5. 清理
    g_running = false;
    if (capThread.joinable()) capThread.join();
    cam.stopGrabbing();
    cam.close();
    ui.destroy();
    std::cout << "程序结束。" << std::endl;
    return 0;
}
