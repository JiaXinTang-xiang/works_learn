// charuco_calibrate.cpp —— 使用 my_charuco.jpg 实物布局（DICT_5X5_50）
//
// 和 calibrate.cpp 的普通棋盘格相比，ChArUco 有三大好处：
//   1. 板子不用整块入镜 —— 只要有一小片（≥ minCornersToCapture 个角点）就能捕获；
//   2. 角点带唯一 ID，顺序不会乱，标定更稳；
//   3. 部分遮挡/反光也不怕，只要还看得到几个标记。
#include <opencv2/opencv.hpp>
#include <opencv2/aruco.hpp>
#include <opencv2/aruco/charuco.hpp>   // ChArUco 板/角点插值/标定函数在此头文件
#include <iostream>
#include <vector>
#include "physical_charuco_board.hpp"

int main(int argc, char** argv) {
    if (argc != 1 && !(argc == 3 && std::string(argv[1]) == "--check-image") &&
        !(argc == 4 && std::string(argv[1]) == "--check-image")) {
        std::cerr << "用法: " << argv[0] << " [--check-image 输入图片 [检测结果图片]]" << std::endl;
        return 1;
    }
    // ================= 1. 板子参数（用尺子量你的打印板，改成实际值） =================
    // 注意：squaresX/squaresY 是「方格数」，不是角点数！
    const int   squaresX      = 9;                  // 方格列数
    const int   squaresY      = 6;                  // 方格行数
    const float squareLength  = 0.024f;             // 每个方格边长（米），自己量！
    const float markerLength  = 0.018f;             // 方格内 ArUco 标记边长（米），自己量！
    const int   dictName = cv::aruco::DICT_5X5_50;

    // 每帧至少检测到多少 ChArUco 角点才允许捕获。
    // 9×6 方格 → 共 (9-1)×(6-1) = 40 个角点；要求 ≥ 一半即可，板子可部分出镜。
    const int minCornersToCapture = 20;
    // 至少攒多少帧才开始标定（建议 10 帧以上，角度/距离尽量拉开）。
    const int minFramesToCalibrate = 10;

    // ================= 2. 创建 ChArUco 板 + 检测器 =================
    cv::Ptr<cv::aruco::Dictionary> dictionary =
        cv::aruco::getPredefinedDictionary(dictName);
    cv::Ptr<cv::aruco::CharucoBoard> board =
        createPhysicalCharucoBoard(squareLength, markerLength, dictionary);

    cv::Ptr<cv::aruco::DetectorParameters> detectorParams =
        cv::aruco::DetectorParameters::create();
    // 让 detectMarkers 也做一次标记角点亚像素精化，后面 charuco 角点会更准
    detectorParams->cornerRefinementMethod = cv::aruco::CORNER_REFINE_SUBPIX;

    // 离线检查实物照片，不打开相机，也不修改已有标定参数。
    if (argc >= 3) {
        cv::Mat input = cv::imread(argv[2]);
        if (input.empty()) {
            std::cerr << "无法读取图片: " << argv[2] << std::endl;
            return 1;
        }
        cv::Mat gray;
        cv::cvtColor(input, gray, cv::COLOR_BGR2GRAY);
        std::vector<std::vector<cv::Point2f>> markers;
        std::vector<int> markerIds, cornerIds;
        std::vector<cv::Point2f> corners;
        cv::aruco::detectMarkers(gray, dictionary, markers, markerIds, detectorParams);
        int count = markerIds.empty() ? 0 : cv::aruco::interpolateCornersCharuco(
            markers, markerIds, gray, board, corners, cornerIds);
        std::cout << "实物布局: 9x6 / DICT_5X5_50 / 27 markers\n"
                  << "标记数: " << markerIds.size() << "，ChArUco 角点数: " << count << std::endl;
        if (!markerIds.empty()) cv::aruco::drawDetectedMarkers(input, markers, markerIds);
        if (count > 0) cv::aruco::drawDetectedCornersCharuco(input, corners, cornerIds);
        if (count >= 4) {
            std::vector<cv::Point2f> objectPoints;
            for (int id : cornerIds) {
                const auto& point = board->chessboardCorners[id];
                objectPoints.emplace_back(point.x, point.y);
            }
            cv::Mat inliers;
            cv::Mat homography = cv::findHomography(objectPoints, corners, cv::RANSAC, 3, inliers);
            if (!homography.empty()) {
                std::vector<cv::Point2f> projected;
                cv::perspectiveTransform(objectPoints, projected, homography);
                double maxResidual = 0;
                for (size_t i = 0; i < corners.size(); ++i)
                    maxResidual = std::max(maxResidual, cv::norm(corners[i] - projected[i]));
                std::cout << "平面布局一致点(3px): " << cv::countNonZero(inliers) << "/" << count
                          << "，最大残差: " << maxResidual << " px\n"
                          << "注意：这是单图布局检查，不是相机标定误差，也不保证多帧标定质量。" << std::endl;
            }
        }
        if (argc == 4 && !cv::imwrite(argv[3], input)) return 1;
        return count >= minCornersToCapture ? 0 : 2;
    }

    // ================= 3. 打开外接 USB 相机 =================
    // 用 CAP_V4L2 显式指定后端：默认的 GStreamer 后端会把 /dev/v4l/... 当 URI 解析、
    // 打一堆 warning 还打不开；V4L2 后端原生认设备路径。
    cv::VideoCapture cap("/dev/v4l/by-id/usb-HJ_USB_2.0_Camera_HJ_USB_2.0_Camera_SN0001-video-index0", cv::CAP_V4L2);
    if (!cap.isOpened()) cap.open(2, cv::CAP_V4L2);
    if (!cap.isOpened()) {
        std::cerr << "无法打开摄像头！" << std::endl;
        return -1;
    }

    // 固定分辨率 640x480：内参 K 的 fx/fy/cx/cy 都是按像素算的，标定用多大分辨率，
    // 后面 PnP/视觉处理就必须用同样的分辨率。这里标 640x480，下游也要 640x480。
    cap.set(cv::CAP_PROP_FRAME_WIDTH,  640);
    cap.set(cv::CAP_PROP_FRAME_HEIGHT, 480);

    std::vector<std::vector<cv::Point2f>> allCharucoCorners;  // 每帧的角点（2D 像素）
    std::vector<std::vector<int>>        allCharucoIds;       // 每帧的角点 ID
    cv::Size imageSize;

    cv::Mat frame, gray;
    std::cout << "ChArUco 标定程序启动！" << std::endl;
    std::cout << "使用 my_charuco.jpg 实物布局：9x6 / DICT_5X5_50（不是旧版 PNG 布局）。" << std::endl;
    std::cout << "把棋盘格对准镜头（无需整块入镜，部分可见即可）：" << std::endl;
    std::cout << "  按 'c' 捕获当前帧 | 按 's' 开始计算并保存 | 按 'q' 退出" << std::endl;
    std::cout << "（注意：请先点击弹出的窗口，再按键盘！）" << std::endl;

    bool warnedLayoutMismatch = false;  // 「检测到标记却插值不出角点」只提示一次

    // ================= 采集覆盖引导 =================
    // 追踪板子在画面里的位置(3x3)、距离(近/中/远)、面内旋转(3档)，实时提示还缺哪一块。
    bool posCovered[3][3] = {{false}};
    bool sizeCovered[3]   = {false, false, false};
    bool angleCovered[3]  = {false, false, false};

    auto updateCoverage = [&](const std::vector<cv::Point2f>& corners, const cv::Size& img) {
        if (corners.size() < 4) return;
        cv::Point2f c(0, 0);
        for (const auto& p : corners) c += p;
        c *= 1.0f / (float)corners.size();
        int gx = std::min(2, (int)(c.x / img.width * 3.0f));
        int gy = std::min(2, (int)(c.y / img.height * 3.0f));
        posCovered[gy][gx] = true;

        cv::Rect bb = cv::boundingRect(corners);
        double ratio = (double)(bb.width * bb.height) / (double)(img.width * img.height);
        if (ratio < 0.12)      sizeCovered[0] = true;  // 远/小
        else if (ratio < 0.35) sizeCovered[1] = true;  // 中
        else                   sizeCovered[2] = true;  // 近/大

        cv::RotatedRect rr = cv::minAreaRect(corners);
        double ang = rr.angle;
        if (rr.size.width < rr.size.height) ang += 90.0f;
        if (ang > 45.0f)  ang -= 90.0f;
        if (ang < -45.0f) ang += 90.0f;
        double a = std::abs(ang);
        if      (a < 15.0f) angleCovered[2] = true;  // 接近水平
        else if (a < 30.0f) angleCovered[1] = true;  // 中等倾斜
        else                angleCovered[0] = true;  // 大角度倾斜
    };

    while (true) {
        cap >> frame;
        if (frame.empty()) break;

        if (imageSize.empty()) {
            imageSize = frame.size();
            if (imageSize != cv::Size(640, 480)) {
                std::cerr << "警告：相机实际分辨率是 " << imageSize.width << "x" << imageSize.height
                          << "，不是 640x480！(cap.set 可能没生效，相机不支持该设置)\n"
                          << "内参按像素计算，若继续标定，下游就必须用 "
                          << imageSize.width << "x" << imageSize.height << "，否则结果不匹配。" << std::endl;
            }
        }
        if (frame.size() != imageSize) {
            std::cerr << "相机分辨率发生变化，停止标定；请固定分辨率后重新采集。" << std::endl;
            return 1;
        }
        cv::cvtColor(frame, gray, cv::COLOR_BGR2GRAY);

        // 4. 检测 ArUco 标记
        std::vector<int> markerIds;
        std::vector<std::vector<cv::Point2f>> markerCorners, rejected;
        cv::aruco::detectMarkers(gray, dictionary, markerCorners, markerIds,
                                 detectorParams, rejected);

        // 5. 由标记插值出 ChArUco 角点（带唯一 ID）
        std::vector<cv::Point2f> charucoCorners;
        std::vector<int>         charucoIds;
        int nCorners = 0;
        if (!markerIds.empty()) {
            nCorners = cv::aruco::interpolateCornersCharuco(
                markerCorners, markerIds, gray, board, charucoCorners, charucoIds);
        }

        // interpolateCornersCharuco 已执行角点亚像素精化；不再用固定窗口重复拉动角点。

        // 覆盖统计 + 清晰度（Laplacian 方差，越高越清晰）
        updateCoverage(charucoCorners, frame.size());
        double sharpness = 0.0;
        if (nCorners > 0) {
            cv::Rect bb = cv::boundingRect(charucoCorners);
            bb &= cv::Rect(0, 0, gray.cols, gray.rows);
            if (bb.width > 4 && bb.height > 4) {
                cv::Mat lap;
                cv::Laplacian(gray(bb), lap, CV_64F);
                cv::Scalar mu, sd;
                cv::meanStdDev(lap, mu, sd);
                sharpness = sd.val[0] * sd.val[0];
            }
        }
        bool sharp = (sharpness > 50.0);  // 阈值可调：<50 一般偏糊，对焦后再按 c

        // 7. 画图 + 实时状态提示
        cv::Mat display = frame.clone();
        if (!markerIds.empty())
            cv::aruco::drawDetectedMarkers(display, markerCorners, markerIds);
        if (nCorners > 0)
            cv::aruco::drawDetectedCornersCharuco(display, charucoCorners, charucoIds);
        // 诊断：红色框 = 看到了方块但被字典拒绝（区分「没看到板子」和「字典不匹配」）
        if (!rejected.empty())
            cv::aruco::drawDetectedMarkers(display, rejected, cv::noArray(), cv::Scalar(0, 0, 255));

        // 少量孤立标记、模糊或遮挡也可能导致无法插值，不能仅凭此判断版本不兼容。
        if (!markerIds.empty() && nCorners == 0 && !warnedLayoutMismatch) {
            std::cout << "检测到 " << markerIds.size() << " 个标记，却插值不出角点！" << std::endl
                      << "  → 当前模型匹配 my_charuco.jpg 实物，请检查相邻标记可见性、清晰度和板子尺寸。" << std::endl;
            warnedLayoutMismatch = true;
        }

        // 8. 能不能拍照，直接大字画在图像顶部（绿=可以按 c，橙=还不行）
        bool enough = (nCorners >= minCornersToCapture);
        cv::Scalar bannerColor = enough ? cv::Scalar(0, 180, 0) : cv::Scalar(0, 120, 210);
        std::string bannerText = enough
            ? "READY - press 'c' to capture  [corners " + std::to_string(nCorners) + "]"
            : "WAIT - corners " + std::to_string(nCorners) + "/" + std::to_string(minCornersToCapture) +
              "  (move closer / straighten)";

        int bannerH = 46;
        cv::rectangle(display, cv::Point(0, 0), cv::Point(display.cols, bannerH), bannerColor, cv::FILLED);
        cv::putText(display, bannerText, cv::Point(12, 33), cv::FONT_HERSHEY_SIMPLEX, 0.85,
                    cv::Scalar(255, 255, 255), 2);

        // 顶部横幅下方补一行小字：已捕获帧数 + 标记数
        std::string sub = "Captured: " + std::to_string(allCharucoCorners.size()) +
                          "  |  Markers: " + std::to_string(markerIds.size());
        cv::putText(display, sub, cv::Point(12, bannerH + 24), cv::FONT_HERSHEY_SIMPLEX, 0.55,
                    cv::Scalar(0, 255, 255), 2);

        // 覆盖引导 + 清晰度
        int posCount = 0;
        for (int r = 0; r < 3; ++r)
            for (int cc = 0; cc < 3; ++cc) posCount += posCovered[r][cc];
        int sizeCount  = sizeCovered[0] + sizeCovered[1] + sizeCovered[2];
        int angleCount = angleCovered[0] + angleCovered[1] + angleCovered[2];
        std::string cov = "Coverage: pos " + std::to_string(posCount) + "/9  dist " +
                          std::to_string(sizeCount) + "/3  tilt " + std::to_string(angleCount) + "/3";
        cv::putText(display, cov, cv::Point(12, bannerH + 46), cv::FONT_HERSHEY_SIMPLEX, 0.5,
                    cv::Scalar(0, 255, 0), 1);
        std::string shrp = "Sharpness: " + std::to_string((int)sharpness) + (sharp ? "  OK" : "  BLUR!");
        cv::putText(display, shrp, cv::Point(12, bannerH + 64), cv::FONT_HERSHEY_SIMPLEX, 0.5,
                    sharp ? cv::Scalar(0, 255, 0) : cv::Scalar(0, 0, 255), 1);

        cv::imshow("ChArUco Calibration", display);

        // 8. 按键处理（失败时打印原因，不再静默）
        char key = (char)cv::waitKey(30);
        if (key == 'c') {
            if (enough && sharp) {
                allCharucoCorners.push_back(charucoCorners);
                allCharucoIds.push_back(charucoIds);
                std::cout << "已捕获第 " << allCharucoCorners.size()
                          << " 帧（角点数 " << nCorners << "，清晰度 " << (int)sharpness << "）" << std::endl;
            } else if (!enough) {
                std::cout << "未捕获：当前仅 " << nCorners << " 个角点"
                          << "（需 >= " << minCornersToCapture << "），把板子拿近/摆正一点。" << std::endl;
            } else {
                std::cout << "未捕获：画面偏糊（清晰度 " << (int)sharpness << " < 50），对焦/拿稳后再按 c。" << std::endl;
            }
        } else if (key == 's') {
            if ((int)allCharucoCorners.size() < minFramesToCalibrate) {
                std::cout << "当前仅 " << allCharucoCorners.size() << " 帧，不足 "
                          << minFramesToCalibrate << " 帧，请继续按 'c' 多角度捕获。" << std::endl;
                continue;
            }
            break;
        } else if (key == 'q' || key == 27) {
            cap.release();
            cv::destroyAllWindows();
            return 0;
        }
    }

    if (static_cast<int>(allCharucoCorners.size()) < minFramesToCalibrate) {
        std::cerr << "采集已中断，帧数不足；不计算，也不覆盖已有参数。" << std::endl;
        return 1;
    }

    // ================= 9. 运行标定算法（自动剔除离群帧）=================
    std::cout << "正在运行 ChArUco 标定算法（共 " << allCharucoCorners.size() << " 帧）..." << std::endl;
    cv::Mat cameraMatrix, distCoeffs;
    std::vector<cv::Mat> rvecs, tvecs;

    // 逐帧误差计算（复用：剔除循环 + 最终展示）
    auto frameErrors = [&](const cv::Mat& K, const cv::Mat& D,
                           const std::vector<cv::Mat>& rv, const std::vector<cv::Mat>& tv) {
        std::vector<double> errs(allCharucoCorners.size());
        for (size_t i = 0; i < allCharucoCorners.size(); ++i) {
            std::vector<cv::Point3f> obj;
            obj.reserve(allCharucoIds[i].size());
            for (int id : allCharucoIds[i]) obj.push_back(board->chessboardCorners[id]);
            std::vector<cv::Point2f> proj;
            cv::projectPoints(obj, rv[i], tv[i], K, D, proj);
            errs[i] = cv::norm(allCharucoCorners[i], proj, cv::NORM_L2) / std::sqrt((double)proj.size());
        }
        return errs;
    };

    // 离群帧（糊/抖/反光）会拖垮最小二乘解：反复剔除误差最大的帧并重新求解
    const double maxFrameErr = 1.5;   // 单帧误差超过此值就剔除（可调）
    int removed = 0;
    double repError = 0.0;
    while (true) {
        repError = cv::aruco::calibrateCameraCharuco(
            allCharucoCorners, allCharucoIds, board, imageSize,
            cameraMatrix, distCoeffs, rvecs, tvecs);

        std::vector<double> errs = frameErrors(cameraMatrix, distCoeffs, rvecs, tvecs);
        double worstErr = -1.0;
        size_t worstIdx = 0;
        for (size_t i = 0; i < errs.size(); ++i)
            if (errs[i] > worstErr) { worstErr = errs[i]; worstIdx = i; }

        // 最差帧已达标，或再剔就低于最小帧数 → 打印结果并结束
        if (worstErr <= maxFrameErr || (int)allCharucoCorners.size() <= minFramesToCalibrate) {
            std::cout << "\n逐帧重投影误差（px，剔除后）:\n";
            for (size_t i = 0; i < errs.size(); ++i) {
                std::cout << "  帧" << i << ": " << errs[i] << " px (" << allCharucoCorners[i].size() << " 角点)";
                if (errs[i] > maxFrameErr) std::cout << "  <-- 仍偏大";
                std::cout << std::endl;
            }
            break;
        }

        allCharucoCorners.erase(allCharucoCorners.begin() + worstIdx);
        allCharucoIds.erase(allCharucoIds.begin() + worstIdx);
        ++removed;
        std::cout << "  剔除离群帧（误差 " << worstErr << " px），剩余 " << allCharucoCorners.size() << " 帧" << std::endl;
    }
    if (removed > 0)
        std::cout << "共剔除 " << removed << " 个离群帧。" << std::endl;

    std::cout << "标定完成！重投影 RMS: " << repError << " 像素（< 0.5 可作参考，仍需独立验证）" << std::endl;
    std::cout << "内参矩阵 K:\n" << cameraMatrix << std::endl;
    std::cout << "畸变系数:\n" << distCoeffs << std::endl;

    cv::FileStorage fs("camera_params.yaml", cv::FileStorage::WRITE);
    fs << "camera_matrix" << cameraMatrix;
    fs << "dist_coeffs" << distCoeffs;
    fs << "image_width" << imageSize.width << "image_height" << imageSize.height;
    fs << "reprojection_rms" << repError;
    fs << "frame_count" << static_cast<int>(allCharucoCorners.size());
    fs << "board_layout" << "my_charuco_photo_9x6_5x5_row_major_black_top_left";
    fs << "dictionary" << "DICT_5X5_50";
    fs << "squares_x" << squaresX << "squares_y" << squaresY;
    fs << "square_length" << squareLength << "marker_length" << markerLength;
    fs.release();
    std::cout << "参数已保存到 camera_params.yaml（键名与 pnp 程序一致，可直接复用）" << std::endl;

    return 0;
}
