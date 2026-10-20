// config.hpp —— 全局参数配置中心
//
// 作用：把相机、HSV、形态学、轮廓过滤、显示等所有可调参数集中到一个文件，
//       便于调参和移植。运行时部分参数(面积/圆形度/形态学核)仍可用键盘实时调整，
//       见 vision_ui.cpp 的快捷键。
#pragma once

#include <opencv2/core.hpp>

namespace cfg {

// ---------------------------------------------------------------------------
// 相机 (Hikvision Camera)
// ---------------------------------------------------------------------------
inline constexpr int    CAMERA_DEVICE_INDEX  = 0;      // 枚举到的第几个设备(0=第一个)
inline constexpr double CAMERA_EXPOSURE_US   = 50000.0;// 曝光时间(微秒)。5000 在本环境会黑屏，先加大到可见，
                                                        // 再按现场光照用键盘 e/E 微调后锁死(否则 HSV 会漂移)
inline constexpr double CAMERA_GAIN          = 12.0;   // 增益(dB)，与参考 Python set_gain(12) 一致
inline constexpr double CAMERA_FRAMERATE     = 0.0;    // 采集帧率，<=0 不设置(避免与较长曝光冲突，让相机自定)
inline constexpr unsigned int GRAB_TIMEOUT_MS = 1000;  // 取流超时(毫秒)

// ---------------------------------------------------------------------------
// HSV 红色双区间 (Red Dual Range)
// 红色在色相环两端，需两段合并。初值取自 red_cap_detect.py 实测瓶盖数据。
// ---------------------------------------------------------------------------
inline const cv::Scalar LOWER_RED1{0,   145, 114};
inline const cv::Scalar UPPER_RED1{28,  255, 255};
inline const cv::Scalar LOWER_RED2{167, 145, 114};
inline const cv::Scalar UPPER_RED2{180, 255, 255};

// ---------------------------------------------------------------------------
// 形态学 (Morphology)
// ---------------------------------------------------------------------------
inline constexpr int GAUSSIAN_SIZE = 5;   // 高斯模糊核(奇数)
inline constexpr int CLOSE_KERNEL  = 5;   // 闭运算核，连接断裂区域
inline constexpr int OPEN_KERNEL   = 3;   // 开运算核，去小噪点

// ---------------------------------------------------------------------------
// 轮廓过滤 (Contour Filtering)
// ---------------------------------------------------------------------------
inline constexpr double MIN_AREA         = 300.0;   // 最小轮廓面积(像素²)
inline constexpr double MAX_AREA         = 80000.0; // 最大轮廓面积(像素²)
inline constexpr double MIN_CIRCULARITY  = 0.60;    // 最小圆形度 4πA/P²

// ---------------------------------------------------------------------------
// 显示 (Display)
// ---------------------------------------------------------------------------
inline constexpr int MAX_DISPLAY_WIDTH = 640;  // 宽>该值时等比缩小后再检测/显示(0=不缩放)
                                              // 注意：串口坐标使用缩放后的显示坐标系

// ---------------------------------------------------------------------------
// 鼠标 HSV 取样 (Mouse HSV Sampling)
// ---------------------------------------------------------------------------
inline constexpr int HSV_REGION_SIZE = 5;     // 点击处分析区域边长(奇数)

} // namespace cfg
