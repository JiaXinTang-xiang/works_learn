// PingPongFrameBuffer.hpp —— 零拷贝乒乓双缓冲（生产者-消费者）
//
// 作用：采图线程(生产者) 与 算法线程(消费者) 之间传递"最新一帧"。
//       用 std::swap 交换 cv::Mat 头部指针，锁内只碰几十字节的头信息，不拷贝像素。
#pragma once
#include <opencv2/opencv.hpp>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <utility>   // std::swap

class PingPongFrameBuffer {
public:
    // 生产者调用：零拷贝指针交换，锁内仅交换头信息，耗时纳秒级
    void update(cv::Mat& new_frame) {
        if (new_frame.empty()) return;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            // 只交换头部 + 引用计数，像素内存一动不动
            std::swap(write_buffer_, new_frame);
            // 覆盖旧帧 = 天然丢弃历史帧（只保留最新一帧）
            has_new_frame_ = true;
        }
        cv_.notify_one();   // 唤醒可能在等待的消费者
    }

    // 消费者调用：接管最新帧的所有权，同样纳秒级
    bool getLatest(cv::Mat& out_frame) {
        std::unique_lock<std::mutex> lock(mutex_);
        // 没有新帧时最多等 50ms，避免死锁空挂
        cv_.wait_for(lock, std::chrono::milliseconds(50), [this] { return has_new_frame_; });
        if (!has_new_frame_) return false;   // 超时仍没拿到帧

        out_frame = write_buffer_;           // 浅拷贝接管（cv::Mat 引用计数 +1）
        has_new_frame_ = false;              // 取走后标记为已读
        return true;
    }

private:
    cv::Mat write_buffer_;
    bool has_new_frame_ = false;
    std::mutex mutex_;              // 保护指针交换的轻量互斥锁
    std::condition_variable cv_;    // 条件变量，避免消费者空转占满 CPU
};
