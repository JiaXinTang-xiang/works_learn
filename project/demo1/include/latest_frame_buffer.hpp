// latest_frame_buffer.hpp —— 容量为 1 的最新帧单槽双缓冲(生产者-消费者)
//
// 思想来自 chapt6 的 PingPongFrameBuffer，但改成"交换所有权"语义：
//   - 生产者 publish(): 把新帧换入槽位，同时拿回上一帧(可复用作下一次转换缓冲)，
//     锁内只 swap 两个 cv::Mat 头部(几十字节)，不拷贝像素，耗时纳秒级。
//   - 消费者 tryTakeLatest(): 把最新帧换出(接管所有权)，旧帧天然被覆盖丢弃。
//
// 关键收益：海康 SDK 裸缓冲区在 MV_CC_FreeImageBuffer() 后不能再用，所以采图线程
// 必须先把它转成独立 BGR cv::Mat 再 publish；这里"只保留一帧、丢弃旧帧、算法不积压"，
// 正是 chapt6 的核心收益。
#pragma once

#include <opencv2/core.hpp>
#include <mutex>

class LatestFrameBuffer {
public:
    // 生产者：换入新帧(接管其所有权)，返回槽内上一帧供生产者复用。
    // 调用后传入的 frame 会变为空 Mat。
    cv::Mat publish(cv::Mat& frame) {
        std::lock_guard<std::mutex> lk(mtx_);
        cv::Mat old;
        cv::swap(old, slot_);       // old = 上一帧(可能为空)
        cv::swap(slot_, frame);     // slot_ 接管新帧，frame 变为空
        has_new_ = true;
        return old;
    }

    // 消费者：非阻塞地取走最新帧(接管所有权)。无新帧返回 false。
    bool tryTakeLatest(cv::Mat& out) {
        std::lock_guard<std::mutex> lk(mtx_);
        if (!has_new_) return false;
        cv::swap(out, slot_);       // out 接管最新帧，slot_ 清空
        has_new_ = false;
        return true;
    }

    // 清空槽位(退出时调用)。
    void reset() {
        std::lock_guard<std::mutex> lk(mtx_);
        slot_.release();
        has_new_ = false;
    }

private:
    cv::Mat slot_;
    bool has_new_ = false;
    std::mutex mtx_;
};
