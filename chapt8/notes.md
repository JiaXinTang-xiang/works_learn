# 第八章笔记（CH11：结业实战 —— 组装第一个道具抓取定位 Demo）

> 本章不是新知识，而是**把前面 10 章的积木拼成一部引擎**。核心增量只有三个「赛场最后一公里」问题：坐标系对齐、无显示器运行、跟踪丢失防护。其余全部是复习。

---

## 1. 完整工程拓扑与四层架构（一句话）
**别把几百行堆进 main.cpp（意大利面条）**，按职责拆成四个正交模块，用轻量数据结构低耦合交互：

```
物理摄像头 → [采图线程 Producer] → PingPongFrameBuffer(双缓冲)
                                     ↓ 按需取最新帧
                              [算法主线程 Consumer]
   ├─ TargetDetector: HSV→形态学→轮廓→四边形顶点
   ├─ PnPSolver:     3D点+2D点→solvePnP→相机系坐标
   ├─ 坐标系转换:     相机系→底盘中心系(外参)
   └─ SerialPort:    打包18字节帧+CRC8→写串口
                                     ↓
                              [电控单片机 STM32]
```

四个模块正好对应前四章：PingPongFrameBuffer（CH9 多线程）、TargetDetector（CH6/7 图像）、PnPSolver（CH8 标定/PnP）、SerialPort（CH10 串口）。

## 2. 目标检测器 TargetDetector（一句话）
**HSV 阈值切出目标 → 形态学开闭去噪 → findContours 找最大轮廓 → approxPolyDP 拟合四边形 → 凸包排成顺时针**。红色要开**双段阈值**（H 在 0~10 和 170~180 两段，因为红色在色相环上跨了 0°）。

## 3. PnP 解算器（一句话）
**道具的物理 3D 角点（半宽/半高）+ 图像 2D 角点 + 标定内参 K + 畸变系数 → `cv::solvePnP` → 相机系坐标**。平面四边形用 `SOLVEPNP_IPPE`（专为平面目标设计，快且稳）；`Rodrigues` 把旋转向量转矩阵，再用 `atan2` 提偏航角。

## 4. 盲区一：相机系 vs 底盘系（外参转换，本章最硬核）
**PnP 解出的坐标是「相机系」，电控要的是「底盘系」，两者轴向和原点都不一样，直接发必炸。**

- **轴向相反**：相机 X 向右、Y 向下、Z 向前；底盘 X 车头向前、Y 车身向左、Z 垂直向上。所以要做 `y = -x`、`z = -y` 这种映射。
- **俯仰角补偿**：相机 100% 是向下倾斜安装的（15°~35°），光轴 Z_c 斜向下戳地，**不是水平前向距离**。要绕 X 轴旋转 θ 把视线「摆平」：
  ```
  y_level = y·cosθ - z·sinθ
  z_level = y·sinθ + z·cosθ
  ```
  不补偿的话测距误差 10%~25%，机械臂砸地或抓空。
- **Yaw 极性取反（控制死穴）**：相机系正 yaw 是向右转，底盘系（REP 103）正 yaw 是向左转。**不取反就是正反馈发散**——小车右偏一点，算法命令它继续右转，越转越快原地画圈。铁律：`send.yaw = -pose.yaw`。

## 5. 盲区二：无显示器（Headless）运行（一句话）
**赛场上位机没有显示器，`cv::imshow` + `cv::waitKey` 在无图形桌面环境下会直接抛异常崩溃**。加个 `--gui` 开关：调试时开窗口，上车时静默运行。

## 6. 盲区三：跟踪丢失与通信失联防护（一句话）
**丢了目标不能「不发」或「发旧坐标」，必须持续发「置信度 0 的归零包」**；单片机连续收到 3 帧零包就中止动作、转入搜索。串口 `write` 返回 -1 说明 USB 震松了，要重连重试，不能抛异常闪退。

## 7. 现代 CMake 与 Header-only（一句话）
**用 `target_include_directories` / `target_link_libraries` 把依赖挂在「目标」上，而不是全局**。`PingPongFrameBuffer.hpp` 和 `SerialPort.hpp` 是 header-only（实现全写在头文件里），所以 `add_executable` 里**只列 .cpp**，不列它们——加了反而会重复定义。

## 8. systemd 开机自启（一句话）
**开机自动拉起的标准做法是 systemd 服务**：写 `.service` 文件（`Restart=always` 崩溃自愈），`systemctl enable --now` 启用。这就是汽车行车电脑的「无人值守」能力。

## 9. 进阶路线图（一句话）
基础版是「规则管道」（HSV + solvePnP + UART）；下一步三座山：**状态估计（EKF 追踪预测）**、**深度学习（YOLO + TensorRT）**、**分布式（ROS 2）**。

---

## 语法速查片段

```cpp
// ① 平面四边形 PnP（IPPE）
cv::Mat rvec, tvec;
cv::solvePnP(object_pts, image_pts, K, dist, rvec, tvec, false, cv::SOLVEPNP_IPPE);
cv::Mat R; cv::Rodrigues(rvec, R);
double yaw = std::atan2(R.at<double>(0,2), R.at<double>(2,2));   // 绕 Y 轴偏航

// ② 俯仰角补偿（绕相机 X 轴旋转 θ）
float c = std::cos(theta), s = std::sin(theta);
float y_level = pose.y * c - pose.z * s;   // 摆平后的水平深度
float z_level = pose.y * s + pose.z * c;   // 摆平后的高度

// ③ 相机系 → 底盘系映射（轴向 + 极性 + 偏置）
pkt.x   = z_level + L;    // 前向
pkt.y   = -x_level;       // 左向（相机右正→底盘左正，取反）
pkt.z   = -y_level + H;   // 高度（相机下正→取反+离地高）
pkt.yaw = -pose.yaw;      // 偏航严格取反！

// ④ 红色双段 HSV 阈值
cv::inRange(hsv, cv::Scalar(0,120,70),  cv::Scalar(10,255,255),  m1);
cv::inRange(hsv, cv::Scalar(170,120,70),cv::Scalar(180,255,255), m2);
cv::bitwise_or(m1, m2, mask);

// ⑤ 凸包顺时针排序四边形角点
std::vector<cv::Point2f> hull;
cv::convexHull(corners, hull, true /* 顺时针 */);

// ⑥ 目标丢失：发全零归零包
send_packet = {0};   // target_type=0, confidence=0, 坐标全 0

// ⑦ systemd 服务（重启自愈）
//   Restart=always
//   RestartSec=2s
//   sudo systemctl enable --now robot_vision
```

---

## 避坑清单（本章经典坑）

1. **直接发 PnP 坐标、不做外参转换**：电控拿到的是相机系，全是反的。
2. **Yaw 不取反**：正反馈发散，小车原地画圈越转越快。
3. **忽略俯仰角**：拿 Z_c 当水平前向距离，测距偏 10%~25%。
4. **内参用占位假值**（`920/920/640/360`）：必须填 chapt5 标定的真实 K 和畸变。
5. **`cv::imshow` 在车上 headless 运行**：无显示器直接 `cv::Exception` 崩溃。
6. **目标丢失不发零包**：单片机重复抓旧坐标，抓空烧舵机。
7. **`write` 返回值不检查**：USB 震松返回 -1，程序不知道还继续发。
8. **把四个模块全堆进 main.cpp**：改一处牵一发动全身。
9. **header-only 模块也写进 `add_executable`**：重复定义链接报错。
10. **道具尺寸单位错**：`width_meters` 是米，别把毫米塞进去。

---

## 思考与反思

- **为什么「软件」和「教学代码」的区别在盲区里，不在算法里？** 教学代码只验证算法对不对；赛场软件要解决「坐标发给谁、在什么坐标系、没了目标怎么办、没屏幕会不会崩、断电会不会起」这些工程问题。三大盲区才是从 Demo 到能上车的分水岭。
- **坐标系对齐的本质是什么？** 是「约定」。相机和底盘是两个独立的物理部件，谁也不会自动迁就谁。上位机的职责就是**把数据翻译成底盘听得懂的语言（米制 + 标准极性）**，这也是为什么前面 CH10 协议帧里要规定好每个字段的单位。
- **这套流水线的天花板在哪？** HSV 阈值怕反光和变色，PnP 怕遮挡和丢角点，串口怕干扰。所以进阶方向（EKF 预测遮挡、YOLO 抗光照、ROS2 多传感器）正好是给这三个短板打补丁——看懂基础版的短板，才理解进阶篇为什么存在。

> 留给自己填：你标定的真实内参 fx/fy/cx/cy 是多少？相机实际下俯角多少度？填进 main.cpp 里跑起来才对得上。
