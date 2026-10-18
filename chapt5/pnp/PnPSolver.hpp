// PnPSolver.hpp —— 位姿解算类：2D 角点 + 3D 尺寸 + 内参 → (X,Y,Z) + 欧拉角
#pragma once
#include <opencv2/opencv.hpp>
#include <vector>
#include <string>

struct PoseResult {
    cv::Vec3d tvec;  // 平移向量 (x, y, z) 单位: 米
    double distance; // 空间欧式直线距离
    double yaw;      // 偏航角 (绕 Yc 轴)
    double pitch;    // 俯仰角 (绕 Xc 轴)
    double roll;     // 翻滚角 (绕 Zc 轴)
};

class PnPSolver {
public:
    PnPSolver() = default;

    bool loadCameraParams(const std::string& yaml_path) {
        cv::FileStorage fs(yaml_path, cv::FileStorage::READ);
        if (!fs.isOpened()) return false;

        fs["camera_matrix"] >> camera_matrix_;
        fs["dist_coeffs"]   >> dist_coeffs_;
        fs.release();
        return true;
    }

    // 设定 3D 物理尺寸，顺序严格为: 左上, 右上, 右下, 左下
    void setTargetSize(float width_m, float height_m) {
        float half_w = width_m / 2.0f;
        float half_h = height_m / 2.0f;

        object_points_.clear();
        object_points_.push_back(cv::Point3f(-half_w, -half_h, 0.0f)); // 0: TL
        object_points_.push_back(cv::Point3f( half_w, -half_h, 0.0f)); // 1: TR
        object_points_.push_back(cv::Point3f( half_w,  half_h, 0.0f)); // 2: BR
        object_points_.push_back(cv::Point3f(-half_w,  half_h, 0.0f)); // 3: BL
    }

    bool estimatePose(const std::vector<cv::Point2f>& sorted_image_points, PoseResult& result) {
        if (sorted_image_points.size() != 4 || object_points_.size() != 4) return false;

        cv::Vec3d rvec;
        // SOLVEPNP_IPPE 适合平面四边形靶标
        bool success = cv::solvePnP(object_points_, sorted_image_points,
                                    camera_matrix_, dist_coeffs_,
                                    rvec, result.tvec,
                                    false, cv::SOLVEPNP_IPPE);

        if (!success) return false;

        result.distance = cv::norm(result.tvec);

        // 旋转矩阵转相机系欧拉角
        cv::Mat R;
        cv::Rodrigues(rvec, R);
        result.pitch = std::asin(-R.at<double>(1, 2)) * 180.0 / CV_PI;
        result.yaw   = std::atan2(R.at<double>(0, 2), R.at<double>(2, 2)) * 180.0 / CV_PI;
        result.roll  = std::atan2(R.at<double>(1, 0), R.at<double>(1, 1)) * 180.0 / CV_PI;

        return true;
    }

private:
    cv::Mat camera_matrix_;
    cv::Mat dist_coeffs_;
    std::vector<cv::Point3f> object_points_;
};
