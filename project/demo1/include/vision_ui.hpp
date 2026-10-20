// vision_ui.hpp —— 可视化与交互(窗口、鼠标 HSV 分析、键盘调试、调试窗口)
//
// 功能(与 red_cap_detect.py 对齐)：
//   - 主窗口 + 调试窗口(mask / morph)管理
//   - 鼠标左键点击输出该点 HSV 信息并给出建议阈值
//   - 键盘快捷调参(面积/圆形度/形态学核)与保存/退出
#pragma once

#include <opencv2/core.hpp>
#include <string>

#include "red_detector.hpp"

class VisionUi {
public:
    explicit VisionUi(RedDetector& detector);
    ~VisionUi();

    // 创建主窗口并挂鼠标回调。
    void setup(const std::string& windowName);

    // 记录当前帧(浅拷贝)用于鼠标 HSV 分析；必须在绘制污染图像之前调用。
    void setCurrentFrame(const cv::Mat& bgr);

    // 绘制检测结果(轮廓/圆心/最小外接圆/十字线/标注)。
    void drawDetection(cv::Mat& frame, const RedDetectionResult& r) const;

    // 绘制状态栏(FPS/底部提示/当前参数)。
    void drawHud(cv::Mat& frame, const RedDetectionResult& r,
                 double fps) const;

    // imshow 主窗口 + 调试窗口(受 showMask/showMorph/showAll 控制)。
    void render(const cv::Mat& result, const RedDetectionResult& r);

    // 处理一个按键。返回 true 表示请求退出。
    bool handleKey(int key);

    // 保存当前图像到工作目录。
    void saveImage(const cv::Mat& frame);

    // 销毁所有窗口。
    void destroy();

    // 点击点与调试开关状态。
    bool showMask = false;
    bool showMorph = false;
    bool showAll = false;
    bool saveRequested = false;
    cv::Point clickedPoint{-1, -1};
    bool hasClicked = false;

private:
    static void onMouse(int event, int x, int y, int flags, void* userdata);
    void printHsvInfo(int x, int y, int regionSize) const;
    void printParams() const;

    RedDetector& det_;          // 用于运行时调参
    cv::Mat currentFrame_;      // 供鼠标 HSV 分析的干净帧
    std::string windowName_;
    bool maskWinCreated_ = false;
    bool morphWinCreated_ = false;
};
