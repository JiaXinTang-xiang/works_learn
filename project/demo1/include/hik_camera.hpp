// hik_camera.hpp —— 海康威视 MVS SDK 封装
//
// 负责：枚举设备、创建/打开句柄、配置曝光/增益/帧率、取流、像素转换、释放。
// 取流采用 MV_CC_GetImageBuffer + MV_CC_FreeImageBuffer(主动取流，效率高于单帧超时接口)，
// 并在 FreeImageBuffer 之前把裸缓冲转成独立 BGR cv::Mat，避免悬垂指针。
#pragma once

#include <opencv2/core.hpp>
#include <string>
#include <vector>

class HikCamera {
public:
    HikCamera() = default;
    ~HikCamera();

    HikCamera(const HikCamera&) = delete;
    HikCamera& operator=(const HikCamera&) = delete;

    // 枚举设备，返回人类可读的名字列表(不打开相机)。
    static std::vector<std::string> listDevices();

    // 枚举 + 打开指定设备 + 基础配置(关触发、GigE 包大小、防积压策略)。
    bool open(int deviceIndex = 0);
    void close();

    bool startGrabbing();
    void stopGrabbing();

    // 取一帧并转换为 BGR8 的独立 cv::Mat。超时/失败返回 false(不会抛出)。
    bool grabToBgr(cv::Mat& dst);

    bool isOpen() const { return handle_ != nullptr && opened_; }
    int  width()  const { return width_; }
    int  height() const { return height_; }
    const std::string& deviceName()    const { return deviceName_; }
    const std::string& pixelFormatHex() const { return pixelFormatHex_; }

    // 参数设置(调用失败返回 false，但不会影响取流)。
    bool setTriggerOff();
    bool setExposureUs(double us);
    bool setGain(double gain);
    bool setFramerate(double fps);   // <=0 表示不设置

private:
    void* handle_ = nullptr;
    bool opened_ = false;
    bool grabbing_ = false;
    int width_ = 0;
    int height_ = 0;
    std::string deviceName_;
    std::string pixelFormatHex_;
};
