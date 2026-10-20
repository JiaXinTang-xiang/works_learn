// hik_camera.cpp —— 海康威视 MVS SDK 封装实现
#include "hik_camera.hpp"

#include <MvCameraControl.h>

#include <cstring>
#include <iostream>

#include "config.hpp"

namespace {

// 把 SDK 的 unsigned char[] 字段安全转成 std::string(遇到 '\0' 截断)。
std::string decodeStr(const unsigned char* p, size_t maxLen) {
    std::string s;
    s.reserve(maxLen);
    for (size_t i = 0; i < maxLen; ++i) {
        if (p[i] == '\0') break;
        s.push_back(static_cast<char>(p[i]));
    }
    return s;
}

// 像素格式枚举值 → 可读名字(用于日志排查颜色错误)。
std::string pixelTypeName(unsigned int v) {
    switch (v) {
        case PixelType_Gvsp_Mono8:       return "Mono8";
        case PixelType_Gvsp_BayerRG8:    return "BayerRG8";
        case PixelType_Gvsp_BayerGR8:    return "BayerGR8";
        case PixelType_Gvsp_BayerGB8:    return "BayerGB8";
        case PixelType_Gvsp_BayerBG8:    return "BayerBG8";
        case PixelType_Gvsp_RGB8_Packed: return "RGB8_Packed";
        case PixelType_Gvsp_BGR8_Packed: return "BGR8_Packed";
        case PixelType_Gvsp_YUV422_Packed: return "YUV422_Packed";
        default: return "0x" + [v] {
            char buf[16];
            std::snprintf(buf, sizeof(buf), "%08X", v);
            return std::string(buf);
        }();
    }
}

} // namespace

HikCamera::~HikCamera() {
    close();
}

std::vector<std::string> HikCamera::listDevices() {
    std::vector<std::string> names;

    MV_CC_DEVICE_INFO_LIST devList;
    std::memset(&devList, 0, sizeof(devList));
    int ret = MV_CC_EnumDevices(MV_GIGE_DEVICE | MV_USB_DEVICE, &devList);
    if (ret != 0) {
        std::cerr << "[相机] 枚举设备失败，错误码 0x" << std::hex << ret << std::dec << std::endl;
        return names;
    }
    if (devList.nDeviceNum == 0) return names;

    for (unsigned int i = 0; i < devList.nDeviceNum; ++i) {
        MV_CC_DEVICE_INFO* info = devList.pDeviceInfo[i];
        if (info == nullptr) continue;

        std::string name;
        if (info->nTLayerType == MV_GIGE_DEVICE) {
            MV_GIGE_DEVICE_INFO& g = info->SpecialInfo.stGigEInfo;
            name = decodeStr(g.chUserDefinedName, sizeof(g.chUserDefinedName));
            if (name.empty()) name = decodeStr(g.chModelName, sizeof(g.chModelName));
            std::string sn = decodeStr(g.chSerialNumber, sizeof(g.chSerialNumber));
            if (!sn.empty()) name += " SN:" + sn;
            // IP 按 SDK 的小端字节序还原成点分十进制
            char ip[32];
            std::snprintf(ip, sizeof(ip), " (%d.%d.%d.%d)",
                          (g.nCurrentIp >> 0) & 0xFF, (g.nCurrentIp >> 8) & 0xFF,
                          (g.nCurrentIp >> 16) & 0xFF, (g.nCurrentIp >> 24) & 0xFF);
            name = "GigE-" + std::to_string(i) + ": " + name + ip;
        } else if (info->nTLayerType == MV_USB_DEVICE) {
            MV_USB3_DEVICE_INFO& u = info->SpecialInfo.stUsb3VInfo;
            name = decodeStr(u.chUserDefinedName, sizeof(u.chUserDefinedName));
            if (name.empty()) name = decodeStr(u.chModelName, sizeof(u.chModelName));
            std::string sn = decodeStr(u.chSerialNumber, sizeof(u.chSerialNumber));
            if (!sn.empty()) name += " SN:" + sn;
            name = "USB-" + std::to_string(i) + ": " + name;
        } else {
            name = "Unknown-" + std::to_string(i);
        }
        names.push_back(name);
    }
    return names;
}

bool HikCamera::open(int deviceIndex) {
    // 1. 枚举
    MV_CC_DEVICE_INFO_LIST devList;
    std::memset(&devList, 0, sizeof(devList));
    int ret = MV_CC_EnumDevices(MV_GIGE_DEVICE | MV_USB_DEVICE, &devList);
    if (ret != 0 || devList.nDeviceNum == 0) {
        std::cerr << "[相机] 未找到设备或枚举失败(0x" << std::hex << ret << std::dec << ")" << std::endl;
        return false;
    }
    if (deviceIndex < 0 || deviceIndex >= static_cast<int>(devList.nDeviceNum)) {
        std::cerr << "[相机] 设备索引 " << deviceIndex << " 无效，共 " << devList.nDeviceNum
                  << " 个设备" << std::endl;
        return false;
    }

    // 2. 创建句柄
    ret = MV_CC_CreateHandle(&handle_, devList.pDeviceInfo[deviceIndex]);
    if (ret != 0) {
        std::cerr << "[相机] 创建句柄失败 0x" << std::hex << ret << std::dec << std::endl;
        return false;
    }

    // 3. 打开设备(先独占，失败退回只读)
    ret = MV_CC_OpenDevice(handle_, MV_ACCESS_Exclusive, 0);
    if (ret != 0) {
        ret = MV_CC_OpenDevice(handle_, MV_ACCESS_Monitor, 0);
        if (ret != 0) {
            std::cerr << "[相机] 打开设备失败 0x" << std::hex << ret << std::dec << std::endl;
            MV_CC_DestroyHandle(handle_);
            handle_ = nullptr;
            return false;
        }
        std::cout << "[相机] 以只读(Monitor)模式打开" << std::endl;
    }
    opened_ = true;

    // 4. 记录设备名与像素格式
    MV_CC_DEVICE_INFO* info = devList.pDeviceInfo[deviceIndex];
    if (info->nTLayerType == MV_GIGE_DEVICE) {
        MV_GIGE_DEVICE_INFO& g = info->SpecialInfo.stGigEInfo;
        deviceName_ = decodeStr(g.chUserDefinedName, sizeof(g.chUserDefinedName));
        if (deviceName_.empty()) deviceName_ = decodeStr(g.chModelName, sizeof(g.chModelName));
        std::string sn = decodeStr(g.chSerialNumber, sizeof(g.chSerialNumber));
        if (!sn.empty()) deviceName_ += " SN:" + sn;
    } else if (info->nTLayerType == MV_USB_DEVICE) {
        MV_USB3_DEVICE_INFO& u = info->SpecialInfo.stUsb3VInfo;
        deviceName_ = decodeStr(u.chUserDefinedName, sizeof(u.chUserDefinedName));
        if (deviceName_.empty()) deviceName_ = decodeStr(u.chModelName, sizeof(u.chModelName));
        std::string sn = decodeStr(u.chSerialNumber, sizeof(u.chSerialNumber));
        if (!sn.empty()) deviceName_ += " SN:" + sn;
    }

    MVCC_ENUMVALUE pix;
    std::memset(&pix, 0, sizeof(pix));
    if (MV_CC_GetEnumValue(handle_, "PixelFormat", &pix) == 0) {
        char buf[16];
        std::snprintf(buf, sizeof(buf), "%08X", pix.nCurValue);
        pixelFormatHex_ = buf;
        std::cout << "[相机] 当前像素格式: " << pixelFormatHex_
                  << " (" << pixelTypeName(pix.nCurValue) << ")" << std::endl;
    }

    // 5. 关触发(连续采集模式)
    setTriggerOff();

    // 6. GigE 相机设置包大小(与参考 Python 一致)
    if (info->nTLayerType == MV_GIGE_DEVICE) {
        MV_CC_SetIntValue(handle_, "GevSCPSPacketSize", 1500);
    }

    // 7. 防积压：优先 LatestImagesOnly，失败回退 SetImageNodeNum(1)
    ret = MV_CC_SetGrabStrategy(handle_, MV_GrabStrategy_LatestImagesOnly);
    if (ret != 0) {
        MV_CC_SetImageNodeNum(handle_, 1);
        std::cout << "[相机] 最新帧策略不支持，回退为节点数=1" << std::endl;
    } else {
        std::cout << "[相机] 已设置最新帧抓取策略(LatestImagesOnly)" << std::endl;
    }

    std::cout << "[相机] 设备连接成功: " << deviceName_ << std::endl;
    return true;
}

void HikCamera::close() {
    if (handle_ == nullptr) return;
    if (grabbing_) stopGrabbing();
    if (opened_) {
        MV_CC_CloseDevice(handle_);
        opened_ = false;
    }
    MV_CC_DestroyHandle(handle_);
    handle_ = nullptr;
}

bool HikCamera::startGrabbing() {
    if (!opened_) return false;
    int ret = MV_CC_StartGrabbing(handle_);
    if (ret == 0) {
        grabbing_ = true;
        return true;
    }
    std::cerr << "[相机] 开始采集失败 0x" << std::hex << ret << std::dec << std::endl;
    return false;
}

void HikCamera::stopGrabbing() {
    if (handle_ != nullptr && grabbing_) {
        MV_CC_StopGrabbing(handle_);
        grabbing_ = false;
    }
}

bool HikCamera::grabToBgr(cv::Mat& dst) {
    if (!opened_ || !grabbing_) return false;

    MV_FRAME_OUT frame;
    std::memset(&frame, 0, sizeof(frame));

    int ret = MV_CC_GetImageBuffer(handle_, &frame, cfg::GRAB_TIMEOUT_MS);
    if (ret != 0) {
        // 超时(MV_E_NODATA 等)视为本帧无数据，由调用方休眠重试，不刷屏报错。
        return false;
    }

    int w = frame.stFrameInfo.nWidth;
    int h = frame.stFrameInfo.nHeight;

    // 准备 BGR 输出缓冲(同尺寸时 create 直接复用，不重复分配)。
    dst.create(h, w, CV_8UC3);

    // 在 FreeImageBuffer 之前完成像素转换(转为独立 BGR 内存)。
    MV_CC_PIXEL_CONVERT_PARAM_EX param;
    std::memset(&param, 0, sizeof(param));
    param.nWidth        = static_cast<unsigned int>(w);
    param.nHeight       = static_cast<unsigned int>(h);
    param.enSrcPixelType = frame.stFrameInfo.enPixelType;
    param.pSrcData      = frame.pBufAddr;
    param.nSrcDataLen   = frame.stFrameInfo.nFrameLen;
    param.enDstPixelType = PixelType_Gvsp_BGR8_Packed;
    param.pDstBuffer    = dst.data;
    param.nDstBufferSize = static_cast<unsigned int>(w * h * 3);

    ret = MV_CC_ConvertPixelTypeEx(handle_, &param);

    // 无论转换成败，都必须释放 SDK 缓冲，否则取流会卡死。
    MV_CC_FreeImageBuffer(handle_, &frame);

    if (ret != 0) {
        std::cerr << "[相机] 像素转换失败 0x" << std::hex << ret << std::dec << std::endl;
        return false;
    }

    width_ = w;
    height_ = h;
    return true;
}

bool HikCamera::setTriggerOff() {
    if (!opened_) return false;
    return MV_CC_SetEnumValue(handle_, "TriggerMode", MV_TRIGGER_MODE_OFF) == 0;
}

bool HikCamera::setExposureUs(double us) {
    if (!opened_) return false;
    int ret = MV_CC_SetFloatValue(handle_, "ExposureTime", static_cast<float>(us));
    if (ret == 0) {
        std::cout << "[相机] 曝光时间已设置: " << us << " us" << std::endl;
        return true;
    }
    std::cerr << "[相机] 设置曝光失败 0x" << std::hex << ret << std::dec << std::endl;
    return false;
}

bool HikCamera::setGain(double gain) {
    if (!opened_) return false;
    return MV_CC_SetFloatValue(handle_, "Gain", static_cast<float>(gain)) == 0;
}

bool HikCamera::setFramerate(double fps) {
    if (!opened_ || fps <= 0.0) return true;
    MV_CC_SetBoolValue(handle_, "AcquisitionFrameRateEnable", true);
    return MV_CC_SetFloatValue(handle_, "AcquisitionFrameRate", static_cast<float>(fps)) == 0;
}
