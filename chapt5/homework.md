# 第五章作业记录（PnP：2D 像素 → 3D 坐标）

> 记录原则：代码在哪 → 怎么构建 → 实际输出 → 观察到的现象。
> 硬件环节（棋盘格标定、实物测距）需你亲手做；数学部分已用无摄像头验证。

---

## 无摄像头验证（我已跑通，证明 PnP 数学正确）

- **代码**：`pnp/test_pnp.cpp`（合成一个 0.1m 方块在已知位姿 → 投影出 2D 点 → 打乱顺序 → 极角排序还原 → `solvePnP` 解算）
- **构建**：`cmake -B build && cmake --build build -j && ./build/test_pnp`
- **实际输出**：

```
真实  tvec: X=0.1000 Y=-0.0500 Z=0.8000 米
解算  tvec: X=0.1000 Y=-0.0500 Z=0.8000 米
距离: 真实 0.8078 米 vs 解算 0.8078 米
绝对误差: X=0.00000 Y=0.00000 Z=0.00000 米
```

- **观察**：
  - 误差 0.00000 米（无噪声理想数据下应精确还原），证明两条：① `sortQuadCorners` 能把打乱的点还原成 TL/TR/BR/BL；② `solvePnP`(SOLVEPNP_IPPE) + 相机系坐标约定正确。
  - 坐标约定确认：X 向右正、Y 向下正、Z 向前正。真实 (0.1, -0.05, 0.8) = 偏右 0.1m、偏上 0.05m（Y 负）、前方 0.8m，与解算一致。

---

## 作业 1：相机标定（需你亲手做，需要标定板 + 摄像头）

标定有两套程序，**推荐方案 B（ChArUco）**：板子不用整块入镜、角点带唯一 ID、更好拍。

### 方案 A：普通棋盘格（`calibrate/`）

1. 打印 [calibrate/chessboard_9x6.png](calibrate/chessboard_9x6.png)（9×6 内角点 = 10×7 方格），贴平整硬板。⚠️ 严禁用屏幕显示。
2. `cmake -B build && cmake --build build -j && ./build/calibrate`
   - ⚠️ 棋盘格**必须整块清晰入镜**（`findChessboardCorners` 缺一角就识别不到）；
   - 变换距离角度，按 `c` 捕获 ≥12 张，按 `s` 计算。

### 方案 B：ChArUco（`charuco/`，推荐）

1. 用你的 DICT_4X4_50 打印板（9×6 方格），或打印 [charuco/charuco_board_9x6_4x4_50.png](charuco/charuco_board_9x6_4x4_50.png)。
2. 量两个边长，填进 [charuco/charuco_calibrate.cpp](charuco/charuco_calibrate.cpp) 顶部：`squareLength`（方格边长，米）、`markerLength`（黑标记边长，米）。
3. `cmake -B build && cmake --build build -j && ./build/charuco_calibrate`
   - 板子**部分入镜即可**（≥20 个角点，共 40 个），按 `c` 捕获，按 `s` 计算。

### 标定结果记录

两方案都会生成 `camera_params.yaml`，把它复制到 `pnp/` 目录。

```
方案: ____（A 普通棋盘格 / B ChArUco）
重投影误差: ____ 像素（< 0.5 合格）
K = ____
```

---

## 作业 2：实物测距验证（需你亲手做，需要 10cm 方块道具）

1. 把 `pnp/main.cpp` 里的 `lower_yellow/upper_yellow` 换成你自己调好的阈值，`setTargetSize(0.10f, 0.10f)` 改成你道具的真实尺寸。
2. 运行 `./pnp/build/pnp`（注意要在有 `camera_params.yaml` 的目录下运行）。
3. 把方块放在镜头前 **0.5 米** 和 **1.0 米**，记录屏幕上的 Z 与实测距离，填表：

| 实测距离 | 屏幕 Z | 误差 | 旋转道具时 Yaw 是否平滑 |
|---|---|---|---|
| 0.5 m | ____ | ____ | ____ |
| 1.0 m | ____ | ____ | ____ |

---

## 思考题（上一章的答案）

上一章问「像素 (u,v) 怎么变空间 (X,Y,Z)」，这一章的答案是：

> 单目一个像素点背后是一条射线，深度信息靠**先验**补回：已知道具真实尺寸（如 10cm）+ 相机内参 K（标定得到）+ `solvePnP`，用四边形的透视变形唯一解出 (X,Y,Z) 和姿态角。

- 我的理解（用自己的话写一遍）：____
