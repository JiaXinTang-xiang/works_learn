// TargetDetector.hpp —— 视觉流水线核心：HSV 取色 → 形态学 → 轮廓 → 几何筛选
// 这是 chapt4 颜色提取 + 本章几何特征提取的整合形态。
#pragma once
#include <opencv2/opencv.hpp>
#include <vector>

// 识别到的目标道具
struct TargetObject {
    cv::Point2f center;          // 像素中心坐标 (u, v)
    cv::RotatedRect rect;        // 最小外接旋转矩形
    cv::Point2f corners[4];      // 矩形的 4 个角点
    double area;                 // 轮廓面积
};

class TargetDetector {
public:
    TargetDetector() {
        // 创建 3x3 矩形结构元（形态学卷积核）
        kernel_ = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3, 3));
    }

    // 传入一帧图像与当前 HSV 阈值范围，返回所有识别到的道具
    std::vector<TargetObject> detect(
        const cv::Mat& bgr_img,
        const cv::Scalar& lower_hsv,
        const cv::Scalar& upper_hsv
    ) {
        std::vector<TargetObject> results;
        if (bgr_img.empty()) return results;

        // 1. 转 HSV
        cv::Mat hsv;
        cv::cvtColor(bgr_img, hsv, cv::COLOR_BGR2HSV);

        // 2. 阈值分割得到二值 Mask
        cv::Mat mask;
        cv::inRange(hsv, lower_hsv, upper_hsv, mask);

        // 3. 形态学开闭运算修补（开去噪、闭填洞）
        cv::morphologyEx(mask, mask, cv::MORPH_OPEN, kernel_);
        cv::morphologyEx(mask, mask, cv::MORPH_CLOSE, kernel_);

        // 4. 轮廓提取（只取最外层轮廓）
        std::vector<std::vector<cv::Point>> contours;
        cv::findContours(mask, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

        double frame_area = bgr_img.cols * bgr_img.rows;

        // 5. 遍历做几何筛选
        for (const auto& cnt : contours) {
            double area = cv::contourArea(cnt);
            // 过滤极小噪点与极大背景（按画面比例，换分辨率也不失效）
            if (area < frame_area * 0.001 || area > frame_area * 0.4) continue;

            cv::RotatedRect r_rect = cv::minAreaRect(cnt);

            float w = r_rect.size.width;
            float h = r_rect.size.height;
            float max_dim = std::max(w, h);
            float min_dim = std::min(w, h);
            float aspect_ratio = max_dim / (min_dim + 1e-5f);

            // 正方形/规则道具长宽比通常不超过 1.6，细长条反光剔除
            if (aspect_ratio > 1.6f) continue;

            TargetObject obj;
            obj.center = r_rect.center;
            obj.rect = r_rect;
            obj.area = area;
            r_rect.points(obj.corners);

            results.push_back(obj);
        }

        return results;
    }

private:
    cv::Mat kernel_;
};
