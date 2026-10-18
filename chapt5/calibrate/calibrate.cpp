// 文件名: calibrate.cpp —— 相机标定：棋盘格获取内参 + 畸变系数
#include <opencv2/opencv.hpp>
#include <iostream>
#include <vector>

int main() {
    // 棋盘格内部角点数 (列数, 行数)
    cv::Size board_size(9, 6);
    // 每个方格的真实物理边长 (单位: 米)
    float square_size = 0.025f;

    // 打开外接 USB 相机（by-id 持久路径，插拔不变）；失败回退到 index 2
    cv::VideoCapture cap("/dev/v4l/by-id/usb-Sonix_Technology_Co.__Ltd._USB_2.0_Camera-video-index0");
    if (!cap.isOpened()) cap.open(2);
    if (!cap.isOpened()) {
        std::cerr << "无法打开摄像头！" << std::endl;
        return -1;
    }

    std::vector<cv::Point3f> obj_single_pattern;
    for (int i = 0; i < board_size.height; i++) {
        for (int j = 0; j < board_size.width; j++) {
            obj_single_pattern.push_back(cv::Point3f(j * square_size, i * square_size, 0.0f));
        }
    }

    std::vector<std::vector<cv::Point3f>> object_points;
    std::vector<std::vector<cv::Point2f>> image_points;

    cv::Mat frame, gray;
    std::cout << "标定程序启动！将棋盘格对准镜头：" << std::endl;
    std::cout << "按键盘 'c' 捕获当前帧，按 's' 开始计算并保存结果，按 'q' 退出。" << std::endl;

    while (true) {
        cap >> frame;
        if (frame.empty()) break;

        cv::cvtColor(frame, gray, cv::COLOR_BGR2GRAY);

        std::vector<cv::Point2f> corners;
        bool found = cv::findChessboardCorners(gray, board_size, corners,
                                               cv::CALIB_CB_ADAPTIVE_THRESH | cv::CALIB_CB_NORMALIZE_IMAGE);

        cv::Mat display_frame = frame.clone();
        if (found) {
            cv::drawChessboardCorners(display_frame, board_size, corners, found);
        }

        std::string status = "Captured: " + std::to_string(image_points.size()) + " frames";
        cv::putText(display_frame, status, cv::Point(20, 30), cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(0, 255, 0), 2);
        cv::imshow("Camera Calibration", display_frame);

        char key = (char)cv::waitKey(30);
        if (key == 'c' && found) {
            cv::cornerSubPix(gray, corners, cv::Size(11, 11), cv::Size(-1, -1),
                             cv::TermCriteria(cv::TermCriteria::EPS + cv::TermCriteria::COUNT, 30, 0.01));

            image_points.push_back(corners);
            object_points.push_back(obj_single_pattern);
            std::cout << "已成功捕获第 " << image_points.size() << " 组标定数据！" << std::endl;
        } else if (key == 's') {
            if (image_points.size() < 12) {
                std::cout << "当前捕获帧数少于 12 张，请继续按 'c' 捕获更多角度。" << std::endl;
                continue;
            }
            break;
        } else if (key == 'q' || key == 27) {
            cap.release();
            return 0;
        }
    }

    std::cout << "正在运行标定算法..." << std::endl;
    cv::Mat camera_matrix, dist_coeffs;
    std::vector<cv::Mat> rvecs, tvecs;

    double rep_error = cv::calibrateCamera(object_points, image_points, gray.size(),
                                           camera_matrix, dist_coeffs, rvecs, tvecs);

    std::cout << "标定完成！重投影误差: " << rep_error << " 像素 (通常 < 0.5 像素为合格)" << std::endl;

    cv::FileStorage fs("camera_params.yaml", cv::FileStorage::WRITE);
    fs << "camera_matrix" << camera_matrix;
    fs << "dist_coeffs" << dist_coeffs;
    fs.release();
    std::cout << "参数已保存到 camera_params.yaml" << std::endl;

    return 0;
}
