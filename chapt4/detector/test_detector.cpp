// test_detector.cpp —— 无摄像头验证：合成一个旋转黄色方块，验证 TargetDetector 能定位中心
#include <opencv2/opencv.hpp>
#include <cstdio>
#include "TargetDetector.hpp"

int main() {
    // 640x480 黑底，画一个 80x80 的黄色旋转方块，中心在 (320, 240)，转 30°
    cv::Mat img(480, 640, CV_8UC3, cv::Scalar(0, 0, 0));
    cv::RotatedRect rect(cv::Point2f(320, 240), cv::Size2f(80, 80), 30);
    cv::Point2f pts[4];
    rect.points(pts);
    std::vector<cv::Point> poly;
    for (int i = 0; i < 4; i++) poly.push_back(pts[i]);
    cv::fillConvexPoly(img, poly, cv::Scalar(0, 255, 255)); // 黄色 BGR=(0,255,255)

    TargetDetector detector;
    cv::Scalar lower(20, 100, 100), upper(40, 255, 255); // 黄色阈值

    auto targets = detector.detect(img, lower, upper);

    printf("检测到 %zu 个目标（期望 1）\n", targets.size());
    for (const auto& t : targets) {
        printf("中心: (%.1f, %.1f)  面积: %.0f  旋转角: %.1f°\n",
               t.center.x, t.center.y, t.area, t.rect.angle);
    }
    // 期望：中心 ≈ (320, 240)，面积 ≈ 6400，旋转角 ≈ 30°
    return 0;
}
