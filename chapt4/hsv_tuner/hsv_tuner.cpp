// 文件名: hsv_tuner.cpp
#include <opencv2/opencv.hpp>
#include <iostream>

struct HsvThresholds {
    int h_min = 0;
    int h_max = 180;
    int s_min = 50;
    int s_max = 255;
    int v_min = 50;
    int v_max = 255;
} thresholds;

void on_trackbar(int, void*) {}

int main(int argc, char** argv) {
    cv::VideoCapture cap(0);
    if (!cap.isOpened()) {
        std::cerr << "无法打开摄像头！请检查设备连接或权限。" << std::endl;
        return -1;
    }

    // 尝试锁定相机曝光和白平衡（若硬件驱动支持）
    cap.set(cv::CAP_PROP_AUTO_EXPOSURE, 0.25);
    cap.set(cv::CAP_PROP_AUTO_WB, 0);

    const std::string control_win = "HSV Tuner Controls";
    const std::string result_win  = "Threshold Result";
    cv::namedWindow(control_win, cv::WINDOW_AUTOSIZE);
    cv::namedWindow(result_win,  cv::WINDOW_AUTOSIZE);

    cv::createTrackbar("H Min", control_win, &thresholds.h_min, 180, on_trackbar);
    cv::createTrackbar("H Max", control_win, &thresholds.h_max, 180, on_trackbar);
    cv::createTrackbar("S Min", control_win, &thresholds.s_min, 255, on_trackbar);
    cv::createTrackbar("S Max", control_win, &thresholds.s_max, 255, on_trackbar);
    cv::createTrackbar("V Min", control_win, &thresholds.v_min, 255, on_trackbar);
    cv::createTrackbar("V Max", control_win, &thresholds.v_max, 255, on_trackbar);

    std::cout << "调参工具已启动：" << std::endl;
    std::cout << " - 按键盘 'r' 键：切换 常规模式 / 红色双段跨界模式" << std::endl;
    std::cout << " - 按键盘 's' 键：导出当前 C++ 配置代码" << std::endl;
    std::cout << " - 按键盘 'q' 或 ESC：退出程序" << std::endl;

    cv::Mat frame, hsv, mask;
    bool is_red_mode = false; // 是否开启红色跨 0 点双段模式

    while (true) {
        cap >> frame;
        if (frame.empty()) break;

        // 1. 滑动条数值倒挂保护：防止 Min > Max 导致画面意外全黑
        if (thresholds.h_min > thresholds.h_max) {
            cv::setTrackbarPos("H Max", control_win, thresholds.h_min);
            thresholds.h_max = thresholds.h_min;
        }
        if (thresholds.s_min > thresholds.s_max) {
            cv::setTrackbarPos("S Max", control_win, thresholds.s_min);
            thresholds.s_max = thresholds.s_min;
        }
        if (thresholds.v_min > thresholds.v_max) {
            cv::setTrackbarPos("V Max", control_win, thresholds.v_min);
            thresholds.v_max = thresholds.v_min;
        }

        // 2. 预处理去噪
        cv::GaussianBlur(frame, frame, cv::Size(3, 3), 0);
        cv::cvtColor(frame, hsv, cv::COLOR_BGR2HSV);

        // 3. 根据模式进行二值化
        if (!is_red_mode) {
            // 常规单区间模式 (适用于蓝、黄、绿等绝大多数单色道具)
            cv::Scalar lower(thresholds.h_min, thresholds.s_min, thresholds.v_min);
            cv::Scalar upper(thresholds.h_max, thresholds.s_max, thresholds.v_max);
            cv::inRange(hsv, lower, upper, mask);
        } else {
            // 红色双段跨界模式：[0, H_min] 并上 [H_max, 180]
            cv::Mat mask1, mask2;
            cv::inRange(hsv, cv::Scalar(0, thresholds.s_min, thresholds.v_min),
                             cv::Scalar(thresholds.h_min, thresholds.s_max, thresholds.v_max), mask1);
            cv::inRange(hsv, cv::Scalar(thresholds.h_max, thresholds.s_min, thresholds.v_min),
                             cv::Scalar(180, thresholds.s_max, thresholds.v_max), mask2);
            cv::bitwise_or(mask1, mask2, mask);
        }

        // 4. 在原图画面上提示当前模式
        std::string mode_str = is_red_mode ? "Mode: RED Dual-Range [0,Hmin] U [Hmax,180]"
                                           : "Mode: Normal [Hmin, Hmax]";
        cv::putText(frame, mode_str, cv::Point(20, 30), cv::FONT_HERSHEY_SIMPLEX, 0.6,
                    is_red_mode ? cv::Scalar(0, 0, 255) : cv::Scalar(0, 255, 0), 2);
        cv::putText(frame, "Press 'r' to toggle mode, 's' to export", cv::Point(20, 60),
                    cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(255, 255, 0), 1);

        cv::imshow(control_win, frame);
        cv::imshow(result_win,  mask);

        char key = (char)cv::waitKey(30);
        if (key == 'q' || key == 27) {
            break;
        } else if (key == 'r') {
            is_red_mode = !is_red_mode;
            std::cout << "[模式切换] " << (is_red_mode ? "已切换至 红色双段模式" : "已切换至 常规单区间模式") << std::endl;
        } else if (key == 's') {
            std::cout << "\n--- 当前调定参数代码 ---" << std::endl;
            if (!is_red_mode) {
                std::cout << "cv::Scalar lower(" << thresholds.h_min << ", "
                          << thresholds.s_min << ", " << thresholds.v_min << ");" << std::endl;
                std::cout << "cv::Scalar upper(" << thresholds.h_max << ", "
                          << thresholds.s_max << ", " << thresholds.v_max << ");" << std::endl;
                std::cout << "cv::inRange(hsv, lower, upper, mask);" << std::endl;
            } else {
                std::cout << "// 红色双段提取代码:" << std::endl;
                std::cout << "cv::Mat mask1, mask2, red_mask;" << std::endl;
                std::cout << "cv::inRange(hsv, cv::Scalar(0, " << thresholds.s_min << ", " << thresholds.v_min << "), "
                          << "cv::Scalar(" << thresholds.h_min << ", " << thresholds.s_max << ", " << thresholds.v_max << "), mask1);" << std::endl;
                std::cout << "cv::inRange(hsv, cv::Scalar(" << thresholds.h_max << ", " << thresholds.s_min << ", " << thresholds.v_min << "), "
                          << "cv::Scalar(180, " << thresholds.s_max << ", " << thresholds.v_max << "), mask2);" << std::endl;
                std::cout << "cv::bitwise_or(mask1, mask2, red_mask);" << std::endl;
            }
            std::cout << "------------------------\n" << std::endl;
        }
    }

    cap.release();
    cv::destroyAllWindows();
    return 0;
}
