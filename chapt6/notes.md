# 第六章笔记：工业相机接入（驱动 SDK 封装 + std::thread 双缓冲防积压）

> 用法：文字总结 + 语法速查片段。核心类 `PingPongFrameBuffer.hpp`，可运行示例 `hw1_multithread.cpp`。
> 这一章解决的是上一章的遗留工程问题：坐标算得再准，拿到的却是几百毫秒前的旧画面，机械臂照样抓空 —— 所以先解决**实时性**。

## 1. 为什么比赛不能用普通 USB 摄像头

**一句话**：免驱 USB 摄像头是"为视频通话设计的"，三个物理差异在赛场上全是致命伤。

| | 普通免驱摄像头 | 工业相机 |
|---|---|---|
| 快门方式 | 卷帘快门（果冻效应） | 全局快门（刚性不变形） |
| 曝光控制 | 自动（忽明忽暗） | 手动微秒级锁定（如 3000μs） |
| 帧率/接口 | 30 FPS，USB UVC | 100~200 FPS，定制驱动 |

- **卷帘快门的果冻效应**：逐行扫描感光，第 1 行和第 1080 行之间存在时间差。车体高速运动时，竖直道具被拍成倾斜平行四边形 → 上一章的长宽比和 PnP 解算全失准。
- **自动曝光是颜色阈值的死敌**：镜头从暗处扫向射灯，芯片自动降亮度；扫向暗处又自动提亮 → HSV 数值剧烈漂移，调好的阈值全失效。工业相机必须锁死：**曝光时间、增益、白平衡**。

## 2. 工业相机 SDK 通用六步模式

**一句话**：各家 API 函数名不同，底层工作流程完全一致。

```
枚举设备 → 打开句柄 → 设置曝光/增益 → 开启取流 → 循环抓图 → 停止释放
```

| 阶段 | 海康威视 MVS | 迈德威视 MindVision |
|---|---|---|
| 枚举 | `MV_CC_EnumDevices` | `CameraEnumerateDevice` |
| 打开 | `MV_CC_CreateHandle` + `OpenDevice` | `CameraInit` |
| 设曝光 | `MV_CC_SetFloatValue("ExposureTime", 3000)` | `CameraSetExposureTime(h, 3000)` |
| 取流 | `MV_CC_StartGrabbing` | `CameraPlay` |
| 抓图 | `MV_CC_GetImageBuffer` | `CameraGetImageBuffer` |
| 释放 | `MV_CC_FreeImageBuffer` | `CameraReleaseImageBuffer` |

原始字节流通常是 Bayer / 裸 RGB，用 SDK 自带的转换函数或 `cv::cvtColor` 转成 `cv::Mat (BGR)` 即可复用前面的算法。

## 3. 致命陷阱：SDK 裸指针 + cv::Mat 浅拷贝 = 悬垂指针

**一句话**：`pBufAddr` 只在 `GetImageBuffer` 和 `FreeImageBuffer` 之间有效，先 Free 再包装 cv::Mat，指针就成了悬垂指针。

- 驱动底层维护固定数量的环形内存池，Free 之后硬件 DMA 会在几毫秒内把下一帧直接覆盖进这块内存；
- 此时 `raw_img` 的 data 指针变成悬垂指针，算法线程读到的是被硬件践踏一半的撕裂画面，甚至段错误。

**安全范式**：在 Free 之前完成深拷贝（`cv::cvtColor` 会自动给输出分配独立内存），再送进队列。

## 4. 积压延迟：单线程的死穴 + 驱动硬件队列

**一句话**：硬件 100 FPS 采图、算法 30 FPS 处理，速度不匹配 → 缓冲队列越堆越满 → 你拿到的永远是几十帧前的旧画面。

两个层级都要治，缺一不可：

- **应用层**：单线程 `取帧→处理→取帧`，算法忙时采图被阻塞。
- **驱动层**（更隐蔽的真凶）：
  - Linux V4L2 默认 4 帧环形缓冲 → `cap.set(cv::CAP_PROP_BUFFERSIZE, 1);`
  - 海康 MVS 默认 10~30 帧 FIFO → `MV_CC_SetGrabStrategy(handle, MV_GrabStrategy_LatestImagesOnly);`
  - 迈德威视 → 丢帧模式设"保留最新帧"，抓取缓冲帧数设 1。

核心原则：**驱动层把蓄水池关到最小 + 应用层多线程全力抽水**。

## 5. 零拷贝双缓冲：为什么持锁 clone() 是灾难

**一句话**：100 FPS 1080P 下，生产者每秒 100 次 `clone()` = 620 MB/s 内存写入，消费者再加 186 MB/s，锁被几兆字节拷贝长期占住，帧率腰斩。

解法是 `std::swap`：

- `cv::Mat` 头部只有几十字节（尺寸/步进/指针），像素在远端堆内存；
- `std::swap(write_buffer_, new_frame)` 锁内只换头部 + 引用计数，**像素一动不动，耗时 < 10ns**；
- 旧帧被 swap 自然覆盖 = 只保留最新一帧。

## 6. PingPongFrameBuffer 类

**一句话**：生产者 `update()` 交换最新帧，消费者 `getLatest()` 接管最新帧；旧帧被 swap 覆盖 = 天然丢历史。

```cpp
PingPongFrameBuffer buffer;

// 采图线程：
buffer.update(raw_frame);           // 零拷贝存入最新帧

// 算法线程：
cv::Mat frame;
if (buffer.getLatest(frame)) {      // 只拿最新帧，旧帧已丢
    process(frame);
}
```

## 附：上位机选型（顺带一提）

- 传统视觉（抓流、格式转换、形态学、轮廓筛选）吃 **CPU 单核 + 内存带宽**，GPU 帮不上忙；
- 实用选择：x86 迷你主机（AMD 7840H / 8845H 等，8 核 16 线程），工业相机 100 FPS 取流 CPU 占用通常 < 20%，原生 Ubuntu 装库秒级。

---

## 语法速查片段

**多线程三件套**
```cpp
#include <thread>
#include <mutex>
#include <atomic>
std::atomic<bool> is_running(true);         // 线程间共享的开关
std::thread t(func, std::ref(obj));         // 起后台线程
t.join();                                   // 主线程等它结束
```

**锁（作用域自动解锁）**
```cpp
std::mutex mtx;
std::lock_guard<std::mutex> lock(mtx);      // 构造加锁，出作用域自动解锁
```

**条件变量（消费者等待 / 生产者唤醒）**
```cpp
std::condition_variable cv;
std::unique_lock<std::mutex> lock(mtx);
cv.wait_for(lock, std::chrono::milliseconds(50), [&]{ return ready; });  // 最多等 50ms
cv.notify_one();                            // 唤醒一个等待者
```

**零拷贝交换 cv::Mat**
```cpp
std::swap(write_buffer_, new_frame);        // 只换头部+引用计数，像素不动
```

**V4L2 驱动层防积压**
```cpp
cap.set(cv::CAP_PROP_BUFFERSIZE, 1);        // 内核环形缓冲压到 1 帧
```

---

## 避坑清单（本章经典坑）

- [ ] 比赛用普通免驱 USB 摄像头 → 果冻效应 + 自动曝光，画面和阈值全崩。
- [ ] 曝光/增益/白平衡没锁死 → 换场景 HSV 漂移，阈值失效。
- [ ] SDK 裸指针先 `FreeImageBuffer` 再包装 cv::Mat → 悬垂指针，花屏/段错误。
- [ ] 在锁内做 `frame.clone()` 深拷贝 → 高帧率下带宽爆炸、锁竞争帧率腰斩。
- [ ] 忘设驱动层队列（V4L2 BUFFERSIZE / MVS GrabStrategy）→ 应用层多线程也救不了旧帧堆积。
- [ ] 单线程 `取帧→处理→取帧` → 算法忙时采图被阻塞，画面慢半拍。
- [ ] 拿历史帧做 PnP 发给机械臂 → 坐标是几百 ms 前的，直接抓空。
- [ ] 低算力 Jetson 跑工业相机取流 → CPU 单核 + 内存带宽不够，掉帧卡死。

---

## 思考与反思

1. **实时性比精度更先决**：坐标再准，拿到的是 300ms 前的旧画面，机械臂也抓空。先解决"最新帧"，再谈"准"。
2. **问题本质是速度不匹配**：硬件 100 FPS vs 算法 30 FPS，靠"丢弃历史帧"而不是"追上流水线"来解耦 —— 旧帧毫无价值。
3. **两个层级都要防积压**：驱动层把缓冲池关到 1，应用层多线程 + 零拷贝交换，缺一不可。
4. **cv::Mat 引用计数是零拷贝的基石**：头部和数据分离，swap 只动头部，像素安全由引用计数保证。
5. **到这一章，实时 3D 坐标 (X,Y,Z) 已经能稳定拿到**，下一步只剩把它发给单片机 —— 串口，下一章见。

> 留给自己填：这一章哪个概念卡得最久？____
