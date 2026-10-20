// red_detector.cpp —— 红色瓶盖检测核心算法实现
#include "red_detector.hpp"

#include <opencv2/imgproc.hpp>

#include <algorithm>
#include <cmath>

RedDetectionResult RedDetector::detect(const cv::Mat& bgr) const {
    RedDetectionResult r;
    if (bgr.empty()) return r;

    // 1. 高斯模糊降噪
    cv::Mat blurred;
    cv::GaussianBlur(bgr, blurred, cv::Size(gaussianSize, gaussianSize), 0);

    // 2. 转 HSV
    cv::Mat hsv;
    cv::cvtColor(blurred, hsv, cv::COLOR_BGR2HSV);

    // 3. 双范围红色掩膜(红色在色相环两端)
    cv::Mat mask1, mask2, maskRed;
    cv::inRange(hsv, lowerRed1, upperRed1, mask1);
    cv::inRange(hsv, lowerRed2, upperRed2, mask2);
    cv::bitwise_or(mask1, mask2, maskRed);

    // 4. 闭运算：连接断裂的红色区域
    cv::Mat kClose = cv::getStructuringElement(cv::MORPH_RECT,
                                               cv::Size(closeKernel, closeKernel));
    cv::Mat maskClosed;
    cv::morphologyEx(maskRed, maskClosed, cv::MORPH_CLOSE, kClose);

    // 5. 开运算：去除小噪点
    cv::Mat kOpen = cv::getStructuringElement(cv::MORPH_RECT,
                                              cv::Size(openKernel, openKernel));
    cv::Mat maskClean;
    cv::morphologyEx(maskClosed, maskClean, cv::MORPH_OPEN, kOpen);

    // 6. 查找轮廓
    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(maskClean, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    double bestScore = 0.0;
    for (const auto& cnt : contours) {
        double area = cv::contourArea(cnt);
        if (area < minArea || area > maxArea) continue;

        double perimeter = cv::arcLength(cnt, true);
        if (perimeter <= 0.0) continue;

        double circularity = 4.0 * CV_PI * area / (perimeter * perimeter);
        if (circularity < minCircularity) continue;

        cv::Point2f c;
        float radius = 0.0f;
        cv::minEnclosingCircle(cnt, c, radius);

        // 综合评分：圆形度越高越好，面积适中越好(与 Python 一致)
        double areaScore = std::min(area / 5000.0, 5000.0 / std::max(area, 1.0));
        double score = circularity * 0.7 + areaScore * 0.3;

        if (score > bestScore) {
            bestScore = score;
            r.detected = true;
            r.center = c;
            r.radius = static_cast<int>(radius);
            r.area = area;
            r.circularity = circularity;
            r.contour = cnt;
        }
    }

    r.mask = maskClean;
    return r;
}
