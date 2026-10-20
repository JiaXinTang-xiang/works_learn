// PnPSolver.hpp —— 空间小孔成像与 solvePnP 解算器（第 8 章）
#pragma once
#include <opencv2/opencv.hpp>
#include <vector>

struct PoseResult {
    bool is_valid = false;
    float x = 0.0f;      // 相机系 X (米, 向右)
    float y = 0.0f;      // 相机系 Y (米, 向下)
    float z = 0.0f;      // 相机系 Z (米, 向前)
    float yaw = 0.0f;    // 绕垂直轴旋转角度 (度)
};

class PnPSolver {
public:
    PnPSolver(double fx, double fy, double cx, double cy,
              const std::vector<double>& dist_coeffs,
              float width_meters, float height_meters);

    PoseResult solve(const std::vector<cv::Point2f>& image_points);

private:
    cv::Mat camera_matrix_;
    cv::Mat dist_coeffs_;
    std::vector<cv::Point3f> object_points_;
};
