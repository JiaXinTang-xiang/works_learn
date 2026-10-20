# 红色瓶盖识别（海康工业相机 · C++ 多线程版）

基于海康威视 MVS SDK + OpenCV 的红色目标识别程序。以 chapt6 的 **std::thread 双缓冲防积压** 为框架，
把 [参考/视觉代码带注释.py](参考/视觉代码带注释.py) 的海康取流能力 与
[参考/red_cap_detect.py](参考/red_cap_detect.py) 的红色识别、鼠标 HSV 取样、键盘调试
整合成一个可移植的 C++ 模块化工程。

## 1. 目录结构

```
demo1/
├── CMakeLists.txt              # 构建配置(MVS SDK / OpenCV / Threads)
├── README.md
├── include/
│   ├── config.hpp              # 全局参数中心(相机/HSV/形态学/显示)
│   ├── latest_frame_buffer.hpp # 容量=1 的最新帧单槽双缓冲(头文件实现)
│   ├── hik_camera.hpp          # 海康 SDK 封装
│   ├── red_detector.hpp        # 红色检测核心算法
│   └── vision_ui.hpp           # 窗口/鼠标/键盘/调试
└── src/
    ├── main.cpp                # 组装与生命周期
    ├── hik_camera.cpp
    ├── red_detector.cpp
    └── vision_ui.cpp
```

## 2. 依赖与本机环境

| 依赖 | 版本/路径 | 说明 |
|---|---|---|
| Ubuntu | x86_64 | 本机 6.8.0-138 |
| OpenCV | 4.5.4 | `pkg-config` 已可用 |
| 海康 MVS SDK | `/opt/MVS` | 头文件 `/opt/MVS/include/MvCameraControl.h`，库 `/opt/MVS/lib/64/libMvCameraControl.so` |
| cmake / g++ | ≥3.16 / C++17 | — |

> SDK 路径若不同，改 [CMakeLists.txt](CMakeLists.txt) 里的 `MVS_ROOT`。

## 3. 编译

```bash
cd /home/jiaxintang/桌面/work/project/demo1
cmake -B build
cmake --build build -j
```

产物：`build/red_cap_detect`。CMake 已把 `/opt/MVS/lib/64` 写入 rpath，通常可直接运行；
若仍报找不到 `libMvCameraControl.so`，执行一次：

```bash
source /opt/MVS/set_env.sh          # 或 export LD_LIBRARY_PATH=/opt/MVS/lib/64:$LD_LIBRARY_PATH
```

## 4. 运行

```bash
./build/red_cap_detect
```

启动流程：枚举海康设备 → 打开 → 设曝光/增益/帧率 → 开始取流 → 起采图线程 → 主线程识别/显示。
无相机时会打印明确错误并退出。

## 5. 相机参数（重要）

| 参数 | 位置 | 默认 | 说明 |
|---|---|---|---|
| 设备索引 | `config.hpp` `CAMERA_DEVICE_INDEX` | 0 | 第 0 个设备 |
| 曝光 | `CAMERA_EXPOSURE_US` | 5000 μs | **必须锁死**，否则 HSV 漂移 |
| 增益 | `CAMERA_GAIN` | 0 | |
| 帧率 | `CAMERA_FRAMERATE` | 60 | ≤0 用相机默认 |
| 像素格式 | 运行时打印 | — | 输出统一转 BGR8，无需手动改 |

取流使用 `MV_CC_GetImageBuffer` + `MV_CC_FreeImageBuffer`，并在释放前通过
`MV_CC_ConvertPixelTypeEx` 转成独立 BGR 帧（避免 SDK 裸缓冲悬垂指针）。启动会打印当前像素格式
（如 `BayerRG8`），若画面颜色异常先核对这里。

## 6. 调参指南

### 6.1 红色 HSV 阈值标定（鼠标取样）

程序运行后，**鼠标左键点击画面上的红色瓶盖**，终端会打印该点及邻域的 HSV 范围与建议阈值：

```
=== 点击位置(320,240) HSV: H=3 S=241 V=29 ===
区域HSV范围(5x5):
  H: 1-7 (均值:3)
  S: 211-255 (均值:241)
  V: 21-41 (均值:29)
检测到红色，建议双范围阈值: ...
```

把建议值填回 [config.hpp](include/config.hpp) 的 `LOWER_RED1/UPPER_RED1/LOWER_RED2/UPPER_RED2`。

### 6.2 运行时键盘调参

| 按键 | 作用 |
|---|---|
| `Q` / `ESC` | 退出 |
| `S` | 保存当前帧为 `cap_detect_<时间戳>.jpg` |
| `M` | 切换红色掩膜窗口 |
| `P` | 切换形态学结果窗口 |
| `D` | 全部调试窗口开/关 |
| `R` | 清除点击点 |
| `+` / `-` | 调最小轮廓面积(±100) |
| `[` / `]` | 调最小圆形度(±0.05) |
| `;` / `'` | 调闭运算核(±2) |
| `,` / `.` | 调开运算核(±2) |
| `1` | 打印当前所有参数 |

## 7. 常见问题

| 现象 | 排查 |
|---|---|
| 枚举不到设备 | 检查供电、网线/USB；GigE 需设同网段 IP；权限不足用 `sudo` 或加 `udev` 规则 |
| 打开失败 | 是否被其他程序(如 MVS 客户端)占用；GigE 会退回只读 Monitor 模式 |
| 画面颜色错误/偏色 | 核对启动打印的像素格式，本程序已统一转 BGR8，一般无需改 |
| 识别不到红色 | 用鼠标取样重新标定 HSV；调大 `MIN_AREA`/调小 `MIN_CIRCULARITY`；确认曝光已锁死 |
| 找不到 `libMvCameraControl.so` | `source /opt/MVS/set_env.sh` 或导出 `LD_LIBRARY_PATH` |

## 8. 移植说明（模块划分）

- **采图**：`hik_camera` + `latest_frame_buffer` + 采图线程，负责“只保留最新一帧、丢弃旧帧”，
  与算法完全解耦 —— 换相机品牌只需重写 `hik_camera`。
- **算法**：`red_detector`，纯图像进、结果出，不碰相机/UI —— 换检测目标只需替换本模块。
- **调试**：`vision_ui`，窗口/鼠标/键盘全部集中 —— 落地部署时可直接裁剪。
- **参数**：`config.hpp` 一处改全量参数。
