// TargetDetector.cpp —— 从 BGR 图像提取目标道具的外轮廓四边形顶点
#include "TargetDetector.hpp"
#include <algorithm>   // std::rotate

TargetDetector::TargetDetector() {
    kernel_morph_ = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(5, 5));
}

void TargetDetector::sortQuadCorners(std::vector<cv::Point2f>& corners) {
    if (corners.size() != 4) return;

    // 1. 利用凸包算法保证 4 个角点按顺时针闭环排列
    //   （彻底规避 atan2 在 +-pi 处的跳变割裂）
    std::vector<cv::Point2f> hull;
    cv::convexHull(corners, hull, true /* clockwise: 顺时针 */);
    if (hull.size() != 4) return;
    corners = hull;

    // 2. 寻找距离图像原点 (0, 0) 最近的点作为左上角起算点
    //   （注：适用于小倾角正向放置道具；若道具存在较大角度的菱形偏转，
    //     可结合长宽比或额外标记锁定唯一朝向）
    int top_left_idx = 0;
    float min_dist_sq = 1e9f;
    for (int i = 0; i < 4; ++i) {
        float dist_sq = corners[i].x * corners[i].x + corners[i].y * corners[i].y;
        if (dist_sq < min_dist_sq) {
            min_dist_sq = dist_sq;
            top_left_idx = i;
        }
    }
    std::rotate(corners.begin(), corners.begin() + top_left_idx, corners.end());
}

DetectionResult TargetDetector::detect(const cv::Mat& bgr_frame, bool is_red_target) {
    DetectionResult result;
    if (bgr_frame.empty()) return result;

    // 1. 转换至 HSV 颜色空间
    cv::cvtColor(bgr_frame, hsv_img_, cv::COLOR_BGR2HSV);

    // 2. 颜色阈值分割（以红色双段阈值为例）
    if (is_red_target) {
        cv::Mat mask1, mask2;
        cv::inRange(hsv_img_, cv::Scalar(0, 120, 70), cv::Scalar(10, 255, 255), mask1);
        cv::inRange(hsv_img_, cv::Scalar(170, 120, 70), cv::Scalar(180, 255, 255), mask2);
        cv::bitwise_or(mask1, mask2, mask_img_);
    } else {
        // 蓝色目标
        cv::inRange(hsv_img_, cv::Scalar(100, 120, 70), cv::Scalar(124, 255, 255), mask_img_);
    }

    // 3. 形态学开闭运算消除噪点
    cv::morphologyEx(mask_img_, mask_img_, cv::MORPH_OPEN, kernel_morph_);
    cv::morphologyEx(mask_img_, mask_img_, cv::MORPH_CLOSE, kernel_morph_);

    // 4. 轮廓提取
    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(mask_img_, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    // 5. 寻找最大满足几何特征的轮廓
    double max_area = 0;
    int best_contour_idx = -1;

    for (size_t i = 0; i < contours.size(); ++i) {
        double area = cv::contourArea(contours[i]);
        if (area < 800.0) continue; // 面积太小视为杂波

        if (area > max_area) {
            max_area = area;
            best_contour_idx = i;
        }
    }

    if (best_contour_idx == -1) return result;

    // 6. 多边形拟合逼近四边形角点
    std::vector<cv::Point> approx_curve;
    double peri = cv::arcLength(contours[best_contour_idx], true);
    cv::approxPolyDP(contours[best_contour_idx], approx_curve, 0.03 * peri, true);

    if (approx_curve.size() == 4) {
        result.corners.clear();
        for (const auto& p : approx_curve) {
            result.corners.emplace_back(p.x, p.y);
        }
        sortQuadCorners(result.corners);
        result.is_detected = true;
        result.target_type = is_red_target ? 1 : 2;
        result.confidence = 95.0f;
    }

    return result;
}
