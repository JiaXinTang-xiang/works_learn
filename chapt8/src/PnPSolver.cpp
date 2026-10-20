// PnPSolver.cpp —— 4 个像素角点 + 标定内参 + 道具物理 3D 模型 -> 空间坐标
#include "PnPSolver.hpp"
#include <cmath>   // std::atan2

PnPSolver::PnPSolver(double fx, double fy, double cx, double cy,
                     const std::vector<double>& dist_coeffs,
                     float width_meters, float height_meters) {
    camera_matrix_ = (cv::Mat_<double>(3, 3) <<
        fx, 0.0, cx,
        0.0, fy, cy,
        0.0, 0.0, 1.0);

    dist_coeffs_ = cv::Mat(dist_coeffs, true);

    // 道具在自身坐标系下的物理 3D 角点（顺时针: 左上, 右上, 右下, 左下）
    float half_w = width_meters / 2.0f;
    float half_h = height_meters / 2.0f;
    object_points_ = {
        cv::Point3f(-half_w, -half_h, 0.0f),
        cv::Point3f( half_w, -half_h, 0.0f),
        cv::Point3f( half_w,  half_h, 0.0f),
        cv::Point3f(-half_w,  half_h, 0.0f)
    };
}

PoseResult PnPSolver::solve(const std::vector<cv::Point2f>& image_points) {
    PoseResult res;
    if (image_points.size() != 4) return res;

    cv::Mat rvec, tvec;
    // 使用 SOLVEPNP_IPPE 针对平面四边形解算，性能高且精度稳定
    bool ok = cv::solvePnP(object_points_, image_points, camera_matrix_, dist_coeffs_,
                           rvec, tvec, false, cv::SOLVEPNP_IPPE);

    if (ok) {
        res.is_valid = true;
        res.x = static_cast<float>(tvec.at<double>(0));
        res.y = static_cast<float>(tvec.at<double>(1));
        res.z = static_cast<float>(tvec.at<double>(2));

        // 将旋转向量转换为旋转矩阵计算偏航角
        cv::Mat R;
        cv::Rodrigues(rvec, R);
        // 绕相机 Y 轴（垂直向下）的偏航角
        double yaw_rad = std::atan2(R.at<double>(0, 2), R.at<double>(2, 2));
        res.yaw = static_cast<float>(yaw_rad * 180.0 / CV_PI);
    }
    return res;
}
