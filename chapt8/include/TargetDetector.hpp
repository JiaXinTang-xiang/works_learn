// TargetDetector.hpp —— 颜色空间与几何特征提取器（第 6、7 章）
#pragma once
#include <cstdint>
#include <opencv2/opencv.hpp>
#include <vector>

struct DetectionResult {
    bool is_detected = false;
    uint8_t target_type = 0;             // 1: 红色道具, 2: 蓝色道具
    std::vector<cv::Point2f> corners;    // 顺时针排列的 4 个四边形角点 (左上/右上/右下/左下)
    float confidence = 0.0f;
};

class TargetDetector {
public:
    TargetDetector();
    DetectionResult detect(const cv::Mat& bgr_frame, bool is_red_target = true);

private:
    cv::Mat hsv_img_;
    cv::Mat mask_img_;
    cv::Mat kernel_morph_;

    void sortQuadCorners(std::vector<cv::Point2f>& corners);
};
