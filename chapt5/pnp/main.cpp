// main.cpp —— 联调：摄像头检测道具 → 角点提取排序 → PnP 解算 → 绘制
#include <opencv2/opencv.hpp>
#include <iostream>
#include <vector>
#include <algorithm>
#include <cmath>
#include "PnPSolver.hpp"

// 多边形拟合提取 4 个角点（透视下 minAreaRect 对边平行会失真，用 approxPolyDP）
std::vector<cv::Point2f> getQuadCorners(const std::vector<cv::Point>& cnt) {
    std::vector<cv::Point> approx;
    double peri = cv::arcLength(cnt, true);
    cv::approxPolyDP(cnt, approx, 0.03 * peri, true);

    std::vector<cv::Point2f> corners;
    if (approx.size() == 4 && cv::isContourConvex(approx)) {
        for (const auto& pt : approx) corners.push_back(cv::Point2f(pt.x, pt.y));
    } else {
        // 噪点导致拟合点数不对时，降级用 minAreaRect 兜底
        cv::RotatedRect r = cv::minAreaRect(cnt);
        cv::Point2f pts[4];
        r.points(pts);
        corners.assign(pts, pts + 4);
    }
    return corners;
}

// 极角法排序：输出严格 TL, TR, BR, BL（无论物体如何旋转）
std::vector<cv::Point2f> sortQuadCorners(const std::vector<cv::Point2f>& pts) {
    if (pts.size() != 4) return pts;
    cv::Point2f center(0, 0);
    for (const auto& p : pts) center += p;
    center *= 0.25f;

    std::vector<cv::Point2f> sorted = pts;
    std::sort(sorted.begin(), sorted.end(), [center](const cv::Point2f& a, const cv::Point2f& b) {
        return std::atan2(a.y - center.y, a.x - center.x) <
               std::atan2(b.y - center.y, b.x - center.x);
    });

    int tl_idx = 0;
    double min_dist = 1e9;
    for (int i = 0; i < 4; ++i) {
        double d = sorted[i].x * sorted[i].x + sorted[i].y * sorted[i].y;
        if (d < min_dist) {
            min_dist = d;
            tl_idx = i;
        }
    }

    std::vector<cv::Point2f> result(4);
    for (int i = 0; i < 4; ++i) {
        result[i] = sorted[(tl_idx + i) % 4];
    }
    return result;
}

int main() {
    // 打开外接 USB 相机（by-id 持久路径，插拔不变）；失败回退到 index 2
    cv::VideoCapture cap("/dev/v4l/by-id/usb-Sonix_Technology_Co.__Ltd._USB_2.0_Camera-video-index0");
    if (!cap.isOpened()) cap.open(2);
    if (!cap.isOpened()) return -1;

    PnPSolver solver;
    if (!solver.loadCameraParams("camera_params.yaml")) {
        std::cerr << "未找到 camera_params.yaml，请先运行标定！" << std::endl;
        return -1;
    }

    // 设置道具真实尺寸：0.1m x 0.1m (10cm 方块)
    solver.setTargetSize(0.10f, 0.10f);

    cv::Mat frame, hsv, mask;
    cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3, 3));

    // 寻找橙黄色道具（H 下探到 10 覆盖橙色段，上探到 40 覆盖黄色段）
    cv::Scalar lower_yellow(10, 100, 100);
    cv::Scalar upper_yellow(40, 255, 255);

    while (true) {
        cap >> frame;
        if (frame.empty()) break;

        cv::cvtColor(frame, hsv, cv::COLOR_BGR2HSV);
        cv::inRange(hsv, lower_yellow, upper_yellow, mask);
        cv::morphologyEx(mask, mask, cv::MORPH_OPEN, kernel);
        cv::morphologyEx(mask, mask, cv::MORPH_CLOSE, kernel);

        std::vector<std::vector<cv::Point>> contours;
        cv::findContours(mask, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

        for (const auto& cnt : contours) {
            if (cv::contourArea(cnt) < 300.0) continue;

            // 1. 多边形拟合提取角点
            auto raw_corners = getQuadCorners(cnt);

            // 2. 鲁棒极角排序
            auto sorted_corners = sortQuadCorners(raw_corners);

            // 3. PnP 解算
            PoseResult pose;
            if (solver.estimatePose(sorted_corners, pose)) {
                for (int i = 0; i < 4; i++) {
                    cv::line(frame, sorted_corners[i], sorted_corners[(i + 1) % 4], cv::Scalar(0, 255, 0), 2);
                }

                char info[128];
                snprintf(info, sizeof(info), "X:%.2fm Y:%.2fm Z:%.2fm Yaw:%.1f deg",
                         pose.tvec[0], pose.tvec[1], pose.tvec[2], pose.yaw);
                cv::putText(frame, info, sorted_corners[0] + cv::Point2f(-50, -15),
                            cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 255, 255), 2);
            }
        }

        cv::imshow("3D Pose", frame);
        if (cv::waitKey(30) == 27) break;
    }

    return 0;
}
