// test_pnp.cpp —— 无摄像头验证：已知位姿投影出 2D 点，再经角点排序 + solvePnP 还原，对比误差
#include <opencv2/opencv.hpp>
#include <cstdio>
#include <vector>
#include <algorithm>
#include <cmath>

// 极角排序（与 main.cpp 中一致）
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
        if (d < min_dist) { min_dist = d; tl_idx = i; }
    }
    std::vector<cv::Point2f> result(4);
    for (int i = 0; i < 4; ++i) result[i] = sorted[(tl_idx + i) % 4];
    return result;
}

int main() {
    // 1. 虚拟相机内参（640x480，焦距 500 像素，主点 (320,240)）
    cv::Mat K = (cv::Mat_<double>(3, 3) <<
        500, 0, 320,
        0, 500, 240,
        0, 0, 1);
    cv::Mat dist = cv::Mat::zeros(5, 1, CV_64F); // 无畸变

    // 2. 目标 0.1m 方块，真实位姿 (X=0.1, Y=-0.05, Z=0.8) 米，绕 Y 轴 15°
    std::vector<cv::Point3f> obj = {
        {-0.05f, -0.05f, 0}, {0.05f, -0.05f, 0},   // TL, TR
        {0.05f,  0.05f, 0}, {-0.05f,  0.05f, 0}    // BR, BL
    };
    cv::Vec3d gt_tvec(0.1, -0.05, 0.8);
    cv::Vec3d gt_rvec(0, 15.0 * CV_PI / 180.0, 0);

    // 3. 投影得到 2D 像素点（正序）
    std::vector<cv::Point2f> img;
    cv::projectPoints(obj, gt_rvec, gt_tvec, K, dist, img);

    // 4. 打乱顺序，测试 sortQuadCorners 能否还原 TL/TR/BR/BL
    std::vector<cv::Point2f> shuffled = {img[2], img[0], img[3], img[1]};
    auto sorted = sortQuadCorners(shuffled);

    // 5. solvePnP 还原（与 PnPSolver 一致的 SOLVEPNP_IPPE）
    cv::Vec3d rvec, tvec;
    cv::solvePnP(obj, sorted, K, dist, rvec, tvec, false, cv::SOLVEPNP_IPPE);

    printf("真实  tvec: X=%.4f Y=%.4f Z=%.4f 米\n", gt_tvec[0], gt_tvec[1], gt_tvec[2]);
    printf("解算  tvec: X=%.4f Y=%.4f Z=%.4f 米\n", tvec[0], tvec[1], tvec[2]);
    printf("距离: 真实 %.4f 米 vs 解算 %.4f 米\n", cv::norm(gt_tvec), cv::norm(tvec));
    printf("绝对误差: X=%.5f Y=%.5f Z=%.5f 米\n",
           std::abs(tvec[0] - gt_tvec[0]),
           std::abs(tvec[1] - gt_tvec[1]),
           std::abs(tvec[2] - gt_tvec[2]));

    return 0;
}
