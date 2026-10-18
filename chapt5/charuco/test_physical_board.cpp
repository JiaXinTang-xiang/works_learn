#include "physical_charuco_board.hpp"
#include <opencv2/opencv.hpp>
#include <iostream>
#include <stdexcept>

static void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

int main(int argc, char** argv) {
    try {
        require(argc == 2, "Pass my_charuco.jpg as the test fixture");
        auto dictionary = cv::aruco::getPredefinedDictionary(cv::aruco::DICT_5X5_50);
        auto board = createPhysicalCharucoBoard(0.024f, 0.018f, dictionary);
        require(board->ids.size() == 27 && board->chessboardCorners.size() == 40,
                "Incorrect board dimensions");
        require(cv::norm(board->objPoints[0][0] - cv::Point3f(.027f, .003f, 0)) < 1e-6 &&
                cv::norm(board->objPoints[4][0] - cv::Point3f(.003f, .027f, 0)) < 1e-6,
                "IDs 0 and 4 must follow the physical board, not the legacy layout");
        for (const auto& neighbors : board->nearestMarkerIdx)
            require(neighbors.size() == 2, "Each chessboard corner needs two neighboring markers");

        cv::Mat original = cv::imread(argv[1], cv::IMREAD_GRAYSCALE);
        require(!original.empty(), "Cannot load test photograph");
        auto parameters = cv::aruco::DetectorParameters::create();
        parameters->cornerRefinementMethod = cv::aruco::CORNER_REFINE_SUBPIX;
        for (int variant = 0; variant < 6; ++variant) {
            cv::Mat image;
            if (variant == 0) image = original;
            else if (variant <= 3) cv::rotate(original, image, variant - 1);
            else if (variant == 4) cv::resize(original, image, cv::Size(), .5, .5, cv::INTER_AREA);
            else image = original(cv::Rect(original.cols / 6, original.rows / 12,
                                           original.cols * 2 / 3, original.rows * 5 / 6));
            std::vector<std::vector<cv::Point2f>> markers;
            std::vector<int> markerIds, cornerIds;
            std::vector<cv::Point2f> corners, objects, projected;
            cv::aruco::detectMarkers(image, dictionary, markers, markerIds, parameters);
            require(!markerIds.empty(), "No markers found in photograph");
            int count = cv::aruco::interpolateCornersCharuco(
                markers, markerIds, image, board, corners, cornerIds);
            require(count >= 20, "Too few corners in photograph variant");
            for (int id : cornerIds) {
                const auto& p = board->chessboardCorners[id];
                objects.emplace_back(p.x, p.y);
            }
            cv::Mat inliers;
            cv::Mat h = cv::findHomography(objects, corners, cv::RANSAC, 3, inliers);
            require(!h.empty(), "Cannot estimate planar layout");
            cv::perspectiveTransform(objects, projected, h);
            double maxResidual = 0;
            for (size_t i = 0; i < corners.size(); ++i)
                maxResidual = std::max(maxResidual, cv::norm(corners[i] - projected[i]));
            // 只针对这张近似平面、低畸变的测试照片；不是通用标定质量阈值。
            require(maxResidual < 3, "Corner IDs do not match the photographed board geometry");
            std::cout << "variant " << variant << ": " << count
                      << " corners, maximum planar residual " << maxResidual << " px\n";
        }
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
