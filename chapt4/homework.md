# 第四、五章作业记录（视觉流水线：取色 + 几何特征提取）

> 记录原则：代码在哪 → 怎么构建 → 实际输出 → 观察到的现象。
> 这两章是一条流水线：CH4 调色、CH5 检测。代码整合进 `detector/`。

---

## 作业 A：HSV 调参工具（CH4）

- **工程**：`hsv_tuner/`
- **构建**：`cmake -B build && cmake --build build -j && ./build/hsv_tuner`
- **实际构建输出**：`Found OpenCV 4.5.4`，链接 `hsv_tuner` 成功。
- **系统摄像头**：`/dev/video0`、`/dev/video1`。
- **实操（需你亲自做）**：运行调参工具，拿纯色物体对准镜头，拖 6 个滑块让物体成白色实心块，按 `s` 导出阈值代码，复制下来填到下面的检测程序里。

```
// 粘贴 s 键导出的阈值代码到这里
```

---

## 作业 B：TargetDetector 检测程序（CH5）

- **工程**：`detector/`（`TargetDetector.hpp` + `main.cpp` + `test_detector.cpp`）
- **构建**：`cmake -B build && cmake --build build -j`
- **实际构建输出**：

```
-- Found OpenCV: /usr (found version "4.5.4")
[ 50%] Building CXX object .../detector.dir/main.cpp.o
[ 50%] Building CXX object .../test_detector.dir/test_detector.cpp.o
[ 75%] Linking CXX executable detector
[100%] Linking CXX executable test_detector
[100%] Built target test_detector
[100%] Built target detector
```

- **无摄像头验证**（`./build/test_detector`，合成一个 80×80、转 30° 的黄色方块）：

```
检测到 1 个目标（期望 1）
中心: (320.0, 240.0)  面积: 6496  旋转角: 29.7°
```

- **观察**：
  - 中心 (320.0, 240.0) 与合成方块的真实中心**精确一致** → 整条流水线（HSV→形态学→轮廓→旋转矩形→中心）正确。
  - 面积 6496 ≈ 6400（+1.5%）：形态学闭运算把边缘略扩了一圈，属预期。
  - 旋转角 29.7° ≈ 30°（−0.3°）：旋转多边形光栅化成像素时，边缘有 ±1 像素的离散误差，属正常。
  - 这说明 `minAreaRect` 对斜放物体确实"贴合旋转"，而不是像 `boundingRect` 那样虚大。

- **实时实操（需你亲自做）**：把 `main.cpp` 里的 `lower_yellow/upper_yellow` 换成作业 A 导出的阈值，运行 `./build/detector`，拿物体在镜头前倾斜，观察绿色旋转矩形是否贴合旋转。

---

## 思考题（接下一章）

现在拿到的是**像素坐标 (u, v)**，但机械臂要的是**三维空间距离 (X, Y, Z)**。像素怎么变空间坐标？带着这个问题进入下一章 `solvePnP`。

- 我的初步想法：____
