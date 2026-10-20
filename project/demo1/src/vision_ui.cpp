// vision_ui.cpp —— 可视化与交互实现
#include "vision_ui.hpp"

#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>

#include <algorithm>
#include <cstdio>
#include <ctime>
#include <iostream>

#include "config.hpp"

VisionUi::VisionUi(RedDetector& detector) : det_(detector) {}

VisionUi::~VisionUi() {
    destroy();
}

void VisionUi::setup(const std::string& windowName) {
    windowName_ = windowName;
    cv::namedWindow(windowName_, cv::WINDOW_AUTOSIZE);
    cv::setMouseCallback(windowName_, &VisionUi::onMouse, this);

    std::cout << "\n操作说明:" << std::endl;
    std::cout << "  Q/ESC - 退出        S    - 保存当前帧" << std::endl;
    std::cout << "  M    - 红色掩膜      P    - 形态学结果" << std::endl;
    std::cout << "  D    - 全部调试窗口  R    - 重置点击点" << std::endl;
    std::cout << "  E / e - 曝光增/减(±5000us)  G / g - 增益增/减(±1dB)" << std::endl;
    std::cout << "  + / - - 调整最小面积  [ / ] - 调整圆形度" << std::endl;
    std::cout << "  ; / ' - 调整闭运算核  , / . - 调整开运算核" << std::endl;
    std::cout << "  1    - 打印当前参数  鼠标点击 - 获取 HSV 信息" << std::endl;
    std::cout << "=" << std::string(58, '=') << std::endl;
}

void VisionUi::setCurrentFrame(const cv::Mat& bgr) {
    currentFrame_ = bgr;   // 浅拷贝，保持数据存活即可(HSV 分析用)
}

void VisionUi::drawDetection(cv::Mat& frame, const RedDetectionResult& r) const {
    if (!r.detected || r.radius <= 0) return;

    // 轮廓
    if (!r.contour.empty()) {
        std::vector<std::vector<cv::Point>> cs{r.contour};
        cv::drawContours(frame, cs, -1, cv::Scalar(0, 255, 0), 2);
    }

    cv::Point c(static_cast<int>(r.center.x), static_cast<int>(r.center.y));

    // 圆心(实心) + 最小外接圆
    cv::circle(frame, c, 5, cv::Scalar(0, 255, 255), -1);
    cv::circle(frame, c, r.radius, cv::Scalar(255, 0, 0), 2);

    // 十字线
    int cross = std::max(10, r.radius / 2);
    cv::line(frame, cv::Point(c.x - cross, c.y), cv::Point(c.x + cross, c.y),
             cv::Scalar(0, 255, 255), 1);
    cv::line(frame, cv::Point(c.x, c.y - cross), cv::Point(c.x, c.y + cross),
             cv::Scalar(0, 255, 255), 1);

    // 标注
    char label[64];
    std::snprintf(label, sizeof(label), "Center:(%d,%d) R:%d", c.x, c.y, r.radius);
    cv::putText(frame, label, cv::Point(c.x + 15, c.y - 15),
                cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 255, 255), 1);
    std::snprintf(label, sizeof(label), "Area:%.0f", r.area);
    cv::putText(frame, label, cv::Point(c.x + 15, c.y + 10),
                cv::FONT_HERSHEY_SIMPLEX, 0.45, cv::Scalar(255, 255, 255), 1);
}

void VisionUi::drawHud(cv::Mat& frame, const RedDetectionResult& r,
                       double fps) const {
    const char* status = r.detected ? "DETECTED" : "SEARCHING...";
    cv::putText(frame, std::string("Status: ") + status, cv::Point(10, 25),
                cv::FONT_HERSHEY_SIMPLEX, 0.6,
                r.detected ? cv::Scalar(0, 255, 0) : cv::Scalar(0, 0, 255), 2);

    char buf[64];
    std::snprintf(buf, sizeof(buf), "FPS: %.1f", fps);
    cv::putText(frame, buf, cv::Point(10, 90),
                cv::FONT_HERSHEY_SIMPLEX, 0.45, cv::Scalar(0, 255, 255), 1);

    // 点击点标记
    if (hasClicked && clickedPoint.x >= 0) {
        cv::circle(frame, clickedPoint, 5, cv::Scalar(0, 0, 255), -1);
        std::snprintf(buf, sizeof(buf), "Click:(%d,%d)", clickedPoint.x, clickedPoint.y);
        cv::putText(frame, buf, cv::Point(10, 60),
                    cv::FONT_HERSHEY_SIMPLEX, 0.4, cv::Scalar(0, 0, 255), 1);
    }

    // 底部提示
    int hintY = frame.rows - 55;
    cv::putText(frame,
                "Q:Quit S:Save M:Mask P:Morph D:Debug 1:Params E/e:Exp G/g:Gain",
                cv::Point(5, hintY), cv::FONT_HERSHEY_SIMPLEX, 0.3,
                cv::Scalar(255, 255, 255), 1);
    std::snprintf(buf, sizeof(buf),
                  "Area>%.0f Circ>%.2f CloseK%d OpenK%d",
                  det_.minArea, det_.minCircularity, det_.closeKernel, det_.openKernel);
    cv::putText(frame, buf, cv::Point(5, hintY + 15), cv::FONT_HERSHEY_SIMPLEX, 0.3,
                cv::Scalar(180, 180, 180), 1);
    std::snprintf(buf, sizeof(buf),
                  "Red1:[0-%d,S>%.0f,V>%.0f] Red2:[%d-180,S>%.0f,V>%.0f]",
                  static_cast<int>(det_.upperRed1[0]), det_.lowerRed1[1], det_.lowerRed1[2],
                  static_cast<int>(det_.lowerRed2[0]), det_.lowerRed2[1], det_.lowerRed2[2]);
    cv::putText(frame, buf, cv::Point(5, hintY + 30), cv::FONT_HERSHEY_SIMPLEX, 0.28,
                cv::Scalar(180, 180, 180), 1);
}

void VisionUi::render(const cv::Mat& result, const RedDetectionResult& r) {
    cv::imshow(windowName_, result);

    if (showMask || showAll) {
        cv::Mat maskColor;
        if (!r.mask.empty()) cv::cvtColor(r.mask, maskColor, cv::COLOR_GRAY2BGR);
        cv::imshow("mask", maskColor);
        maskWinCreated_ = true;
    } else if (maskWinCreated_) {
        cv::destroyWindow("mask");
        maskWinCreated_ = false;
    }

    if (showMorph || showAll) {
        cv::Mat morphDisplay;
        if (!r.mask.empty()) {
            cv::cvtColor(r.mask, morphDisplay, cv::COLOR_GRAY2BGR);
            // 形态学结果窗口额外画上当前最优轮廓
            if (!r.contour.empty()) {
                std::vector<std::vector<cv::Point>> cs{r.contour};
                cv::drawContours(morphDisplay, cs, -1, cv::Scalar(0, 255, 0), 2);
            }
        }
        cv::imshow("morph", morphDisplay);
        morphWinCreated_ = true;
    } else if (morphWinCreated_) {
        cv::destroyWindow("morph");
        morphWinCreated_ = false;
    }
}

bool VisionUi::handleKey(int key) {
    if (key == -1) return false;

    if (key == 'q' || key == 'Q' || key == 27) {
        std::cout << "用户退出" << std::endl;
        return true;
    }
    switch (key) {
        case 's': case 'S':
            saveRequested = true;
            break;
        case 'r': case 'R':
            hasClicked = false;
            clickedPoint = cv::Point(-1, -1);
            std::cout << "已重置点击点" << std::endl;
            break;
        case 'm': case 'M':
            showMask = !showMask;
            std::cout << "红色掩膜显示: " << (showMask ? "开" : "关") << std::endl;
            break;
        case 'p': case 'P':
            showMorph = !showMorph;
            std::cout << "形态学结果显示: " << (showMorph ? "开" : "关") << std::endl;
            break;
        case 'd': case 'D':
            showAll = !showAll;
            showMask = showAll;
            showMorph = showAll;
            std::cout << "所有调试显示: " << (showAll ? "开" : "关") << std::endl;
            break;
        case '+': case '=':
            det_.minArea = std::min(50000.0, det_.minArea + 100.0);
            std::cout << "最小轮廓面积: " << det_.minArea << std::endl;
            break;
        case '-':
            det_.minArea = std::max(50.0, det_.minArea - 100.0);
            std::cout << "最小轮廓面积: " << det_.minArea << std::endl;
            break;
        case '[':
            det_.minCircularity = std::max(0.1, det_.minCircularity - 0.05);
            std::cout << "最小圆形度: " << det_.minCircularity << std::endl;
            break;
        case ']':
            det_.minCircularity = std::min(1.0, det_.minCircularity + 0.05);
            std::cout << "最小圆形度: " << det_.minCircularity << std::endl;
            break;
        case ';':
            det_.closeKernel = std::max(1, det_.closeKernel - 2);
            std::cout << "闭运算核大小: " << det_.closeKernel << std::endl;
            break;
        case '\'':
            det_.closeKernel = std::min(31, det_.closeKernel + 2);
            std::cout << "闭运算核大小: " << det_.closeKernel << std::endl;
            break;
        case ',':
            det_.openKernel = std::max(1, det_.openKernel - 2);
            std::cout << "开运算核大小: " << det_.openKernel << std::endl;
            break;
        case '.':
            det_.openKernel = std::min(31, det_.openKernel + 2);
            std::cout << "开运算核大小: " << det_.openKernel << std::endl;
            break;
        case '1':
            printParams();
            break;
        default:
            break;
    }
    return false;
}

void VisionUi::saveImage(const cv::Mat& frame) {
    char name[64];
    std::snprintf(name, sizeof(name), "cap_detect_%ld.jpg", static_cast<long>(std::time(nullptr)));
    if (cv::imwrite(name, frame)) {
        std::cout << "已保存: " << name << std::endl;
    } else {
        std::cout << "保存失败: " << name << std::endl;
    }
}

void VisionUi::destroy() {
    cv::destroyAllWindows();
    maskWinCreated_ = false;
    morphWinCreated_ = false;
}

void VisionUi::onMouse(int event, int x, int y, int, void* userdata) {
    if (event != cv::EVENT_LBUTTONDOWN) return;
    VisionUi* self = static_cast<VisionUi*>(userdata);
    self->clickedPoint = cv::Point(x, y);
    self->hasClicked = true;
    self->printHsvInfo(x, y, cfg::HSV_REGION_SIZE);
}

void VisionUi::printHsvInfo(int x, int y, int regionSize) const {
    if (currentFrame_.empty()) return;
    int h = currentFrame_.rows;
    int w = currentFrame_.cols;
    if (x < 0 || x >= w || y < 0 || y >= h) return;

    cv::Mat hsv;
    cv::cvtColor(currentFrame_, hsv, cv::COLOR_BGR2HSV);

    cv::Vec3b p = hsv.at<cv::Vec3b>(y, x);
    std::cout << "\n=== 点击位置(" << x << "," << y << ") HSV: "
              << "H=" << static_cast<int>(p[0])
              << " S=" << static_cast<int>(p[1])
              << " V=" << static_cast<int>(p[2]) << " ===" << std::endl;

    int half = regionSize / 2;
    int y1 = std::max(0, y - half), y2 = std::min(h, y + half + 1);
    int x1 = std::max(0, x - half), x2 = std::min(w, x + half + 1);
    cv::Mat region = hsv(cv::Range(y1, y2), cv::Range(x1, x2));

    std::vector<cv::Mat> ch;
    cv::split(region, ch);
    double hMin, hMax, sMin, sMax, vMin, vMax;
    cv::minMaxLoc(ch[0], &hMin, &hMax);
    cv::minMaxLoc(ch[1], &sMin, &sMax);
    cv::minMaxLoc(ch[2], &vMin, &vMax);
    cv::Scalar mean = cv::mean(region);
    int hMean = static_cast<int>(mean[0]);
    int sMean = static_cast<int>(mean[1]);
    int vMean = static_cast<int>(mean[2]);

    std::cout << "区域HSV范围(" << regionSize << "x" << regionSize << "):" << std::endl;
    std::cout << "  H: " << hMin << "-" << hMax << " (均值:" << hMean << ")" << std::endl;
    std::cout << "  S: " << sMin << "-" << sMax << " (均值:" << sMean << ")" << std::endl;
    std::cout << "  V: " << vMin << "-" << vMax << " (均值:" << vMean << ")" << std::endl;

    if (hMean < 15 || hMean > 155) {
        std::cout << "\n检测到红色，建议双范围阈值:" << std::endl;
        if (hMean < 15) {
            std::cout << "  lower_red1 = [" << 0 << ", " << std::max(0.0, sMin - 20)
                      << ", " << std::max(0.0, vMin - 20) << "]" << std::endl;
            std::cout << "  upper_red1 = [" << std::min(180.0, hMax + 10)
                      << ", 255, 255]" << std::endl;
            std::cout << "  lower_red2 = [156, " << std::max(0.0, sMin - 20)
                      << ", " << std::max(0.0, vMin - 20) << "]" << std::endl;
            std::cout << "  upper_red2 = [180, 255, 255]" << std::endl;
        } else {
            std::cout << "  lower_red1 = [" << 0 << ", " << std::max(0.0, sMin - 20)
                      << ", " << std::max(0.0, vMin - 20) << "]" << std::endl;
            std::cout << "  upper_red1 = [" << std::min(180.0, hMax - 160 + 10)
                      << ", 255, 255]" << std::endl;
            std::cout << "  lower_red2 = [" << std::max(0.0, hMin - 10)
                      << ", " << std::max(0.0, sMin - 20)
                      << ", " << std::max(0.0, vMin - 20) << "]" << std::endl;
            std::cout << "  upper_red2 = [180, 255, 255]" << std::endl;
        }
    }
    std::cout << std::string(50, '=') << std::endl;
}

void VisionUi::printParams() const {
    std::cout << "\n========== 当前参数 ==========" << std::endl;
    std::cout << "HSV Red1: H[0-" << static_cast<int>(det_.upperRed1[0])
              << "] S[" << det_.lowerRed1[1] << "-255] V[" << det_.lowerRed1[2] << "-255]"
              << std::endl;
    std::cout << "HSV Red2: H[" << static_cast<int>(det_.lowerRed2[0])
              << "-180] S[" << det_.lowerRed2[1] << "-255] V[" << det_.lowerRed2[2] << "-255]"
              << std::endl;
    std::cout << "高斯模糊核: " << det_.gaussianSize << std::endl;
    std::cout << "闭运算核: " << det_.closeKernel << "  开运算核: " << det_.openKernel << std::endl;
    std::cout << "轮廓面积: " << det_.minArea << "-" << det_.maxArea << std::endl;
    std::cout << "最小圆形度: " << det_.minCircularity << std::endl;
    std::cout << "调试窗口: mask=" << showMask << " morph=" << showMorph
              << " all=" << showAll << std::endl;
    std::cout << "==============================" << std::endl;
}
