# RC 上位机 · 基础篇 总结

> 从「C 语言变量」到「机器人自主抓取定位 Demo」，八章走完一条完整的上位机工程师成长路径。
> 每一章都是 **notes.md（知识总结）+ homework.md（真实产出记录）+ 可运行代码** 三件套。

---

## 一、这套笔记在做什么

目标只有一个：**从零培养出能独立写出 RC/Robocon 机器人视觉上位机的全栈能力**——从 C/C++ 语言、CMake 构建，到图像处理、空间几何解算、多线程、串口通信，最后拼成一个赛场上无人值守的抓取定位系统。

这不是「调包侠」路线：每一层都往 Linux 底层、内存、坐标系、软硬件握手这些真东西上踩。

---

## 二、八章知识地图

| 章 | 主题 | 核心知识点 | 产出代码 |
|----|------|-----------|----------|
| [chapt1](chapt1/) | C 语言速通 | 变量 / 指针 / 结构体 / 内存模型 | — |
| [chapt2](chapt2/) | 现代 C++ | 引用 / RAII / STL / 智能指针 | — |
| [chapt3](chapt3/) | CMake + 调试 | 构建脚本 / VSCode 断点 / gdb | — |
| [chapt4](chapt4/) | 视觉流水线 | HSV 取色 / 轮廓提取 / 目标结构 | 检测器 |
| [chapt5](chapt5/) | 2D 像素 → 3D 空间 | 小孔成像 / 相机标定 / solvePnP | `calibrate/` `charuco/` `pnp/` |
| [chapt6](chapt6/) | 工业相机 + 多线程 | SDK 封装 / 双缓冲 / 防积压 | `hw1_multithread` |
| [chapt7](chapt7/) | 串口通信 | termios 原始模式 / 二进制帧 / CRC8 | `SerialPort.hpp` `hw1/hw2` |
| [chapt8](chapt8/) | 结业整合 | 四模块组装 / 坐标系对齐 / headless / 丢失防护 | `prop_vision_system` |

> 章节号与书不完全一一对应（书的 CH6~CH11 被压缩进 chapt4~chapt8），按本仓库的 chapt1~8 文件夹来记即可。

---

## 三、贯穿全书的一条数据流

所有章节不是孤立的，它们拼起来就是一条完整的视觉链路：

```
物理相机
   │  chapt4  HSV 取色 + 轮廓 → 找到目标（像素角点）
   │  chapt5  标定内参 K + solvePnP → 相机系 3D 坐标
   │  chapt6  工业相机 SDK + 多线程双缓冲 → 不积压、不掉帧
   │  chapt7  打包二进制帧 + CRC8 → 可靠写入串口
   ▼
电控单片机 STM32 → 底盘电机 / 机械臂
   │  chapt8  把上面全部模块组装 + 解决坐标系对齐等最后一公里
   ▼
自主抓取
```

一句话记住整本书：**「取帧 → 找目标 → 解坐标 → 封包下发」**。

---

## 四、贯穿全书的五大方法论

这些不是某个章节的知识点，而是每一章反复出现、越到后面越重要的「元能力」：

1. **资源管理（RAII）**：chapt2 的智能指针 → chapt6 的线程/缓冲 → chapt7 的串口 fd，统一思想是「析构自动回收，永远不漏、不二次释放」。
2. **内存安全**：chapt1 的指针 → chapt6 的零拷贝 `std::swap` 双缓冲 → chapt7 的 `#pragma pack(1)` + `memcpy` 解包（严禁指针强转，防 ARM 非对齐 HardFault）。内存是 RC 上位机翻车重灾区。
3. **坐标系与约定**：chapt5 相机内参是「像素→相机系」的桥梁，chapt8 外参是「相机系→底盘系」的桥梁。单位（米/度）、轴向极性（yaw 取反）必须全程统一，否则电控拿到反坐标。
4. **防御式编程**：chapt7 CRC8 防噪声、chapt8 目标丢失发归零包、串口 `write` 返回 -1 重连、headless 保护 GUI——赛场上没有「正常情况」，代码要为最坏情况兜底。
5. **模块化分层**：chapt3 的 CMake 目标依赖 → chapt8 的四模块正交拆分 + header-only。反对把几百行堆进 main.cpp 的「意大利面条」。

---

## 五、各章索引

每章三件套：`notes.md`（知识）+ `homework.md`（产出）+ 代码。

- [chapt1 · C 语言速通](chapt1/notes.md) — 变量 → 指针 → 结构体
- [chapt2 · 现代 C++](chapt2/notes.md) — 引用 / RAII / STL / 智能指针
- [chapt3 · CMake + 调试](chapt3/notes.md) — 构建脚本 / 断点
- [chapt4 · 视觉流水线](chapt4/notes.md) — HSV 取色 → 轮廓 → 目标
- [chapt5 · 2D→3D](chapt5/notes.md) — 小孔成像 / 标定 / solvePnP
- [chapt6 · 工业相机 + 多线程](chapt6/notes.md) — 双缓冲防积压
- [chapt7 · 串口通信](chapt7/notes.md) — termios / 二进制帧 / CRC8
- [chapt8 · 结业整合](chapt8/notes.md) — 四模块组装 + 三大盲区

---

## 六、结业之后（进阶篇预告）

基础版是「规则管道」：HSV + solvePnP + UART。它的三个短板，正好对应三条进阶路线：

| 短板 | 进阶方向 |
|------|----------|
| 目标移动时按「当前位置」抓必空（50~100ms 延时） | 状态估计：扩展卡尔曼滤波 EKF + 运动预测 |
| 强反光 / 破损道具让 HSV 大面积失效 | 深度学习：YOLOv8/YOLO11 + TensorRT 边缘加速 |
| 多传感器（激光雷达/双目/IMU）协同 | 分布式：ROS 2 节点化 + Pub/Sub |

**基础篇的终点不是像素，而是钢铁战车在赛场上的每一次精准起落。**
