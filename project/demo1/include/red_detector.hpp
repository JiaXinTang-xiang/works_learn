// red_detector.hpp —— 红色瓶盖检测核心算法
//
// 算法流程(与 red_cap_detect.py 完全一致)：
//   高斯模糊 → BGR2HSV → 双区间红色掩膜 → 闭运算 → 开运算 → 轮廓 →
//   面积过滤 → 圆形度过滤 → 最小外接圆 → 综合评分取最优。
#pragma once

#include <opencv2/core.hpp>
#include <vector>

struct RedDetectionResult {
    bool detected = false;
    cv::Point2f center{0.0f, 0.0f};  // 最小外接圆圆心
    int radius = 0;                  // 最小外接圆半径
    double area = 0.0;               // 轮廓面积
    double circularity = 0.0;        // 圆形度
    std::vector<cv::Point> contour;  // 最优轮廓(用于绘制)
    cv::Mat mask;                    // 形态学后的红色掩膜(调试用)
};

class RedDetector {
public:
    // 运行时可调参数(初值由 main.cpp 从 config.hpp 注入，可用键盘实时改)。
    cv::Scalar lowerRed1, upperRed1, lowerRed2, upperRed2;
    int gaussianSize = 5;
    int closeKernel = 5;
    int openKernel = 3;
    double minArea = 300.0;
    double maxArea = 80000.0;
    double minCircularity = 0.60;

    // 对 BGR 图像执行检测。输入为空时返回 detected=false。
    RedDetectionResult detect(const cv::Mat& bgr) const;
};
