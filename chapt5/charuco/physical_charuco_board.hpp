#pragma once

#include <opencv2/aruco/charuco.hpp>
#include <algorithm>
#include <cmath>
#include <limits>

// 以 my_charuco.jpg 中的实物为准。将照片转 180 度，按 ID 从小到大看：
//   黑  0 黑  1 黑  2 黑  3 黑
//    4 黑  5 黑  6 黑  7 黑  8
//   黑  9 黑 10 黑 11 黑 12 黑
//   13 黑 14 黑 15 黑 16 黑 17
//   黑 18 黑 19 黑 20 黑 21 黑
//   22 黑 23 黑 24 黑 25 黑 26
// 坐标原点为上述方向的左上角，x 向右、y 向下；标记角点按其编码的
// TL/TR/BR/BL 排列。不能仅反转 ids：标记角点方向及邻接表也必须匹配。
inline cv::Ptr<cv::aruco::CharucoBoard> createPhysicalCharucoBoard(
    float squareLength, float markerLength,
    const cv::Ptr<cv::aruco::Dictionary>& dictionary) {
    constexpr int columns = 9;
    constexpr int rows = 6;
    auto board = cv::aruco::CharucoBoard::create(
        columns, rows, squareLength, markerLength, dictionary);
    board->objPoints.clear();
    board->ids.clear();
    const float margin = (squareLength - markerLength) / 2;
    for (int row = 0; row < rows; ++row) {
        for (int col = 0; col < columns; ++col) {
            if ((row + col) % 2 == 0) continue;
            const float x = col * squareLength + margin;
            const float y = row * squareLength + margin;
            board->ids.push_back(static_cast<int>(board->ids.size()));
            board->objPoints.push_back({
                {x, y, 0}, {x + markerLength, y, 0},
                {x + markerLength, y + markerLength, 0}, {x, y + markerLength, 0}});
        }
    }

    // OpenCV 4.5.4 的默认邻接表属于旧布局，改变 objPoints 后必须重建。
    board->nearestMarkerIdx.assign(board->chessboardCorners.size(), {});
    board->nearestMarkerCorners.assign(board->chessboardCorners.size(), {});
    const double tolerance = squareLength * squareLength * 1e-4;
    for (size_t i = 0; i < board->chessboardCorners.size(); ++i) {
        const auto& corner = board->chessboardCorners[i];
        double minimum = std::numeric_limits<double>::max();
        for (const auto& marker : board->objPoints) {
            cv::Point3f center;
            for (const auto& point : marker) center += point;
            center *= 0.25f;
            minimum = std::min(minimum, static_cast<double>((center - corner).dot(center - corner)));
        }
        for (size_t j = 0; j < board->objPoints.size(); ++j) {
            const auto& marker = board->objPoints[j];
            cv::Point3f center;
            for (const auto& point : marker) center += point;
            center *= 0.25f;
            if (std::abs((center - corner).dot(center - corner) - minimum) > tolerance) continue;
            int closest = 0;
            for (int k = 1; k < 4; ++k) {
                if (cv::norm(marker[k] - corner) < cv::norm(marker[closest] - corner)) closest = k;
            }
            board->nearestMarkerIdx[i].push_back(static_cast<int>(j));
            board->nearestMarkerCorners[i].push_back(closest);
        }
    }
    // 此适配用于检测/标定；4.5.4 的 board->draw() 仍使用旧棋盘底色，勿用来打印。
    return board;
}
