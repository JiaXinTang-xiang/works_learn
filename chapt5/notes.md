# 第五章笔记：2D 像素 → 3D 空间（小孔成像 / 标定 / solvePnP）

> 用法：文字总结 + 语法速查片段。核心类 `pnp/PnPSolver.hpp`，标定工具 `calibrate/`。
> 这一章回答了上一章作业的思考题：像素 (u,v) 怎么变空间 (X,Y,Z)。

## 1. 为什么单个像素点定不了 3D

**一句话**：投影是一次降维，一个像素点背后是一条无限延伸的射线，分不清"近处小物体"还是"远处大物体"。

破局关键 = **先验知识**：RC 比赛道具的物理尺寸是已知的（如方块边长 100mm）。已知真实尺寸 + 它在画面上的投影变形，就能唯一解出位置和姿态。

## 2. 四大坐标系 + 小孔成像

**一句话**：世界(米) →[R,t]→ 相机(米) →[透视除法]→ 图像(毫米) →[K]→ 像素。

- **内参矩阵 K**（相机固有，焦距环没动就固定不变）：

```
K = [ fx  0  cx ]    fx/fy: 焦距(像素)  cx/cy: 主点偏移(≈画面中心)
    [  0 fy  cy ]
    [  0  0   1 ]
```

- **畸变**：径向(桶形/枕形，k1 k2 k3) + 切向(p1 p2)，`distCoeffs = (k1,k2,p1,p2,k3)`。广角镜头(>80°)边缘桶形畸变明显，要先 `undistort`。

## 3. 相机标定（棋盘格获取 K）

**一句话**：拍 15~25 张不同角度棋盘格照片 → `findChessboardCorners` + `calibrateCamera` → 存 YAML。

- ⚠️ **严禁用手机/平板屏幕显示棋盘格**：玻璃反光 + 像素缩放，角点精度崩。
- 重投影误差 < 0.5 像素为合格。

## 4. 角点提取：为什么不用 minAreaRect

**一句话**：透视投影下方形道具是"近大远小"的梯形，对边不平行；minAreaRect 强行平行对边会抹平透视特征，角度误差大。

正确用多边形拟合：

```cpp
cv::approxPolyDP(cnt, approx, 0.03 * cv::arcLength(cnt, true), true);
// approx.size()==4 且 isContourConvex → 用这 4 个真实角点；否则降级 minAreaRect 兜底
```

## 5. 角点排序：极角排序法

**一句话**：PnP 要求 3D 点和 2D 点**严格一一对应**（TL,TR,BR,BL）。简单按 y 排序在物体转成菱形时会崩。

极角排序三步：①算质心 ②按 `atan2` 极角顺时针排环 ③找离图像原点(0,0)最近的点作 TL 起点。

## 6. solvePnP：核心解算

**一句话**：输入 3D 角点(米) + 2D 角点(像素) + K + 畸变 → 输出 tvec(位置) + rvec(姿态)。

- **tvec = (Xc, Yc, Zc)**：X 光轴向右为正、Y 光轴向下为正、Z 正前方深度。可直接发给下位机。
- **rvec → 相机系欧拉角**（⚠️ 别套航空系公式，轴向会颠倒）：

```cpp
cv::Rodrigues(rvec, R);
pitch = asin(-R(1,2));                  // 绕 Xc
yaw   = atan2(R(0,2), R(2,2));          // 绕 Yc
roll  = atan2(R(1,0), R(1,1));          // 绕 Zc
```

## 7. PnPSolver 类

**一句话**：把「载入内参 → 设目标尺寸 → 解算」封装成一个类。

```cpp
PnPSolver solver;
solver.loadCameraParams("camera_params.yaml");
solver.setTargetSize(0.10f, 0.10f);     // 3D 角点顺序 TL,TR,BR,BL
PoseResult pose;
solver.estimatePose(sorted_corners, pose);
// pose.tvec (X,Y,Z) / pose.distance / pose.yaw / pose.pitch / pose.roll
```

## 8. IPPE 平面二义性

**一句话**：平面靶标正对相机或较远时，微小噪声会让 yaw 在正负间突变（双解）。

工程解法：单帧结果不直接发，过一层时序平滑滤波（滑动平均 / EKF），剔除孤立奇异解。

---

## 语法速查片段

**投影 + 解算（互为逆过程）**
```cpp
cv::projectPoints(obj3d, rvec, tvec, K, dist, img2d);   // 3D→2D
cv::solvePnP(obj3d, img2d, K, dist, rvec, tvec, false, cv::SOLVEPNP_IPPE); // 2D→3D
```

**多边形拟合 + 凸性判断**
```cpp
cv::approxPolyDP(cnt, approx, 0.03 * cv::arcLength(cnt, true), true);
bool is_quad = (approx.size() == 4 && cv::isContourConvex(approx));
```

**相机系欧拉角（度）**
```cpp
double pitch = std::asin(-R.at<double>(1,2)) * 180.0 / CV_PI;
double yaw   = std::atan2(R.at<double>(0,2), R.at<double>(2,2)) * 180.0 / CV_PI;
double roll  = std::atan2(R.at<double>(1,0), R.at<double>(1,1)) * 180.0 / CV_PI;
```

---

## 避坑清单（本章经典坑）

- [ ] 以为单点能定深度 → 必须靠已知物理尺寸 + 投影变形。
- [ ] 标定用手机/平板屏幕显示棋盘格 → 反光 + 像素缩放，精度崩。
- [ ] 标定帧数 < 12 张 → 解算不稳，多拍几个角度。
- [ ] 直接拿 `minAreaRect` 的角点送 PnP → 透视下对边平行失真，角度误差大。用 `approxPolyDP`。
- [ ] 角点用简单 y/x 排序 → 物体转成菱形时顺序崩、坐标跳变。用极角排序。
- [ ] 3D 点和 2D 点顺序不对应 → 解算出负坐标/跳变。TL,TR,BR,BL 严格对齐。
- [ ] 相机系欧拉角套航空系公式 → pitch/yaw 轴向颠倒。
- [ ] 忘标定 / 焦距环被拧 → K 失效，全程乱码。
- [ ] 广角镜头不 `undistort` → 边缘角点偏差大。
- [ ] IPPE 单帧 yaw 突变 → 直接发下位机抖，要时序滤波。

---

## 思考与反思

1. **深度信息靠"先验"补回来**：单目丢失的深度，用"我知道这个方块是 10cm"补上。这是单目视觉最核心的思想——没有先验就没有尺度。
2. **坐标系是灵魂**：4 个坐标系层层转换，K 是相机的"光学指纹"，测一次长期用。搞错一个轴向，全盘皆错。
3. **几何鲁棒性藏在细节里**：极角排序防菱形崩溃、approxPolyDP 保透视特征、时序滤波压 IPPE 双解——都是"极端几何情况"的防御。
4. **到这一章，流水线闭环了**：`BGR帧 → HSV → mask → 轮廓 → 角点 → PnP → (X,Y,Z)`，像素终于变成了机械臂能用的米制坐标。

> 留给自己填：这一章哪个概念卡得最久？____
