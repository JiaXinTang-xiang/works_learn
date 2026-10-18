// main.cpp —— 实时检测：摄像头 + 绘制旋转矩形 + 中心点
#include <opencv2/opencv.hpp>
#include <iostream>
#include "TargetDetector.hpp"

int main() {
    cv::VideoCapture cap(0);
    if (!cap.isOpened()) {
        std::cerr << "无法打开摄像头！" << std::endl;
        return -1;
    }

    TargetDetector detector;

    // 填入上一章通过调参窗口记录下的颜色阈值（此处以黄色道具为例）
    cv::Scalar lower_yellow(20, 100, 100);
    cv::Scalar upper_yellow(40, 255, 255);

    cv::Mat frame;
    while (true) {
        cap >> frame;
        if (frame.empty()) break;

        // 执行检测
        std::vector<TargetObject> targets = detector.detect(frame, lower_yellow, upper_yellow);

        // 在原图上绘制检测结果
        for (const auto& obj : targets) {
            // 1. 画旋转矩形四条边（绿色）
            for (int i = 0; i < 4; i++) {
                cv::line(frame, obj.corners[i], obj.corners[(i + 1) % 4],
                         cv::Scalar(0, 255, 0), 2);
            }
            // 2. 标记中心点（红色实心圆）
            cv::circle(frame, obj.center, 5, cv::Scalar(0, 0, 255), -1);
            // 3. 打印中心像素坐标 (u, v)
            std::string text = "(" + std::to_string((int)obj.center.x) + ", "
                                   + std::to_string((int)obj.center.y) + ")";
            cv::putText(frame, text, obj.center + cv::Point2f(10, -10),
                        cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 255, 255), 1);
        }

        cv::imshow("Target Detection", frame);

        char key = (char)cv::waitKey(30);
        if (key == 'q' || key == 27) break;
    }

    cap.release();
    cv::destroyAllWindows();
    return 0;
}
