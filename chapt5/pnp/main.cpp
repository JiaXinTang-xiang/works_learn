// main.cpp —— 联调：检测道具 → 角点提取排序 → PnP 解算 → 绘制
// 用法：
//   ./pnp              摄像头实时检测（橙黄色方块）
//   ./pnp image.png    处理单张图片，打印位姿并保存标注图
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

// 对一帧做检测 + PnP，把结果画到 out 上，返回是否解算成功
static bool processFrame(const cv::Mat& frame, PnPSolver& solver,
                         const cv::Scalar& lo, const cv::Scalar& hi, const cv::Mat& kernel,
                         cv::Mat& out, PoseResult& pose) {
    cv::Mat hsv, mask;
    cv::cvtColor(frame, hsv, cv::COLOR_BGR2HSV);
    cv::inRange(hsv, lo, hi, mask);
    cv::morphologyEx(mask, mask, cv::MORPH_OPEN, kernel);
    cv::morphologyEx(mask, mask, cv::MORPH_CLOSE, kernel);

    out = frame.clone();

    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(mask, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    bool solved = false;
    for (const auto& cnt : contours) {
        if (cv::contourArea(cnt) < 300.0) continue;

        auto raw = getQuadCorners(cnt);
        auto sorted = sortQuadCorners(raw);

        PoseResult p;
        if (!solver.estimatePose(sorted, p)) continue;

        solved = true;
        pose = p;
        for (int i = 0; i < 4; i++)
            cv::line(out, sorted[i], sorted[(i + 1) % 4], cv::Scalar(0, 255, 0), 2);

        char info[160];
        snprintf(info, sizeof(info), "X:%.3f Y:%.3f Z:%.3f dist:%.3f m",
                 p.tvec[0], p.tvec[1], p.tvec[2], p.distance);
        cv::putText(out, info, sorted[0] + cv::Point2f(-50, -15),
                    cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 255, 255), 2);
    }
    return solved;
}

int main(int argc, char** argv) {
    PnPSolver solver;
    if (!solver.loadCameraParams("camera_params.yaml")) {
        std::cerr << "未找到 camera_params.yaml，请先运行标定！" << std::endl;
        return -1;
    }

    // 道具真实尺寸：9.6cm x 9.6cm 方块（最外边 9.6cm）
    solver.setTargetSize(0.096f, 0.096f);

    cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3, 3));
    // 橙黄色道具（H 10~40 覆盖橙黄段）
    cv::Scalar lower_yellow(10, 100, 100);
    cv::Scalar upper_yellow(40, 255, 255);

    // ===== 图片模式：./pnp image.png =====
    if (argc == 2) {
        cv::Mat frame = cv::imread(argv[1]);
        if (frame.empty()) {
            std::cerr << "读不到图片: " << argv[1] << std::endl;
            return -1;
        }
        std::cout << "图片尺寸: " << frame.cols << "x" << frame.rows << std::endl;

        // 内参是按 640x480 标定的：尺寸不一致就缩放，但宽高比变了会引入误差
        if (frame.cols != 640 || frame.rows != 480) {
            std::cerr << "警告：图片不是 640x480，已缩放到 640x480。\n"
                      << "      内参 K 是按 640x480 标的，只有同一相机 640x480 拍的图结果才有效；\n"
                      << "      手机/其它相机拍的照片，这套内参不适用，距离结果无意义！" << std::endl;
            cv::resize(frame, frame, cv::Size(640, 480));
        }

        cv::Mat out;
        PoseResult pose;
        if (processFrame(frame, solver, lower_yellow, upper_yellow, kernel, out, pose)) {
            std::cout << "\n=== PnP 结果 ===\n"
                      << "X  (右 +) : " << pose.tvec[0] << " m\n"
                      << "Y  (下 +) : " << pose.tvec[1] << " m\n"
                      << "Z  (正前方): " << pose.tvec[2] << " m\n"
                      << "直线距离  : " << pose.distance << " m\n"
                      << "yaw   : " << pose.yaw << " deg\n"
                      << "pitch : " << pose.pitch << " deg\n"
                      << "roll  : " << pose.roll << " deg\n";
            std::string outPath = std::string(argv[1]) + ".annotated.png";
            cv::imwrite(outPath, out);
            std::cout << "标注图已存: " << outPath << std::endl;
        } else {
            std::cerr << "没检测到橙黄色方块。请确认物体颜色和 HSV 范围一致（当前 H 10~40）。" << std::endl;
        }
        return 0;
    }

    // ===== 摄像头模式 =====
    cv::VideoCapture cap("/dev/v4l/by-id/usb-HJ_USB_2.0_Camera_HJ_USB_2.0_Camera_SN0001-video-index0", cv::CAP_V4L2);
    if (!cap.isOpened()) cap.open(2, cv::CAP_V4L2);
    if (!cap.isOpened()) return -1;

    cap.set(cv::CAP_PROP_FRAME_WIDTH,  640);
    cap.set(cv::CAP_PROP_FRAME_HEIGHT, 480);

    cv::Mat frame;
    while (true) {
        cap >> frame;
        if (frame.empty()) break;

        cv::Mat out;
        PoseResult pose;
        processFrame(frame, solver, lower_yellow, upper_yellow, kernel, out, pose);

        cv::imshow("3D Pose", out);
        if (cv::waitKey(30) == 27) break;
    }
    return 0;
}
