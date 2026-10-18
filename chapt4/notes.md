# 第四、五章笔记：视觉流水线（HSV 取色 → 轮廓 → 目标结构）

> 这两章是一条连续流水线：CH4 把彩色图变成黑白 mask，CH5 把 mask 变成结构化目标。
> 核心可复用骨架是 `detector/TargetDetector.hpp`（它吸收了原 `templates/color_extract.cpp`），调参工具在 `hsv_tuner/`。

## 0. 一张图看懂流水线

```
BGR 帧
  │ ① cvtColor BGR→HSV
  ▼
HSV 图
  │ ② inRange 阈值分割
  ▼
mask（黑白图）
  │ ③ 形态学开/闭运算
  ▼
干净 mask
  │ ④ findContours
  ▼
轮廓集合
  │ ⑤ 面积 / 长宽比 / 旋转矩形筛选
  ▼
TargetObject（中心 u,v + 4 角点 + 面积）
  │ 下一章 solvePnP
  ▼
(X, Y, Z)
```

## 1. 数字图像的本质 + cv::Mat

**一句话**：计算机眼里没有"物体"，只有一张数字矩阵。Mat = 栈上矩阵头（几十字节）+ 堆上像素数据（MB 级）。

- 灰度图单通道 0~255；彩色图三通道 **BGR**（不是 RGB！纯红 = `Scalar(0,0,255)`）。
- 浅拷贝陷阱：`Mat b = a;` 只复制矩阵头，共用一个 `data`，改 b 连累 a。独立副本用 `a.clone()`。
- 像素访问：`img.at<cv::Vec3b>(y, x)`（注意 y,x 顺序，`p[2]` 才是 R）；整图遍历用 `img.ptr<uint8_t>(row)`。

## 2. 色彩空间与取色（HSV + inRange）

**一句话**：BGR 三通道和光照强耦合、阈值脆弱；HSV 把「颜色」(H) 和「亮度」(V) 解耦，赛场更稳。

- ⚠️ OpenCV 里 **H 是 0~180**（实际 0~360 除以 2）；S、V 是 0~255。常见 H：0/180 红、30 黄、60 绿、100~120 蓝。
- 取色三步：`cvtColor` → `inRange` → mask。
- ⚠️ 红色跨 0 点：[0,10] ∪ [170,180]，要两段 `inRange` + `bitwise_or`。
- ⚠️ 上场前锁死 AWB/AEC：`cap.set(cv::CAP_PROP_AUTO_EXPOSURE, 0.25); cap.set(cv::CAP_PROP_AUTO_WB, 0);`

## 3. 预处理：滤波与 ROI

**一句话**：二值化前先降噪、先裁无关区域，稳定性 + 速度双提升。

- `GaussianBlur`（高斯噪声）/ `medianBlur`（椒盐噪声），核大小必须奇数。
- ROI 零拷贝切片 `img(cv::Rect(x,y,w,h))`；⚠️ 局部坐标要加回 `roi_x/roi_y` 才是全图坐标。

## 4. 形态学去噪：开运算 / 闭运算

**一句话**：开运算（先腐蚀后膨胀）去孤立白噪点；闭运算（先膨胀后腐蚀）填内部黑洞。

- 腐蚀 erode：核里有一个黑 → 中心变黑（白色瘦一圈，吃掉小噪点）。
- 膨胀 dilate：核里有一个白 → 中心变白（白色胖一圈，填平小洞）。

```cpp
cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3, 3));
cv::morphologyEx(mask, mask, cv::MORPH_OPEN, kernel);   // 开：去噪
cv::morphologyEx(mask, mask, cv::MORPH_CLOSE, kernel);  // 闭：填洞
```

## 5. 轮廓提取 findContours

**一句话**：把每个连通的白色区域变成一串边界点。

```cpp
std::vector<std::vector<cv::Point>> contours;
cv::findContours(mask, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
```

- `RETR_EXTERNAL`：只取最外层轮廓（快、够用，忽略内部孔洞）。
- `CHAIN_APPROX_SIMPLE`：直线只存端点，省内存。
- 验证：`cv::drawContours(frame, contours, -1, cv::Scalar(0,255,0), 2);`

## 6. 几何筛选：面积 / 旋转矩形 / 长宽比

**一句话**：findContours 会找出几十上百个色块，用几何规则踢掉假目标（反光条、背景板）。

```cpp
double area = cv::contourArea(cnt);
double frame_area = img.cols * img.rows;
if (area < frame_area * 0.001 || area > frame_area * 0.4) continue; // 面积（按比例，抗分辨率变化）

cv::RotatedRect r = cv::minAreaRect(cnt);   // 最小外接旋转矩形
float ratio = std::max(r.size.width, r.size.height) /
              (std::min(r.size.width, r.size.height) + 1e-5f);
if (ratio > 1.6f) continue;                  // 长宽比

r.center;          // 像素中心 (u, v)
r.points(corners); // 4 个角点
```

- ⚠️ 直立矩形 `boundingRect` 对斜放物体虚大、长宽比失真；用 `minAreaRect`（带角度、贴合）。
- ⚠️ `minAreaRect` 对边永远平行，下一章 PnP 要的是一般四边形（近大远小），届时用多边形拟合升级。

## 7. 整合：TargetDetector 类

**一句话**：把 ①~⑤ 全装进一个类，输入帧 + 阈值，输出 `std::vector<TargetObject>`。

```cpp
TargetDetector detector;
std::vector<TargetObject> targets = detector.detect(frame, lower, upper);
// 每个 TargetObject: .center 中心 / .corners[4] 角点 / .rect 旋转矩形 / .area 面积
```

## 8. GUI 调参工具（阈值从哪来）

**一句话**：`detect()` 要的 lower/upper 阈值不是拍脑袋写的，用 `hsv_tuner` 现场拖滑块调出来，按 `s` 导出。

- 三个关键设计：① Min>Max 倒挂保护；② `r` 键切红色双段模式；③ `s` 键导出 C++ 代码。

---

## 语法速查片段

**BGR→HSV→inRange→形态学→轮廓 五步**
```cpp
cv::cvtColor(bgr, hsv, cv::COLOR_BGR2HSV);
cv::inRange(hsv, lower, upper, mask);
cv::morphologyEx(mask, mask, cv::MORPH_OPEN, kernel);   // + MORPH_CLOSE
cv::findContours(mask, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
cv::RotatedRect r = cv::minAreaRect(contours[i]);
```

**红色双段（跨 0 点）**
```cpp
cv::inRange(hsv, cv::Scalar(0, s_min, v_min),   cv::Scalar(10,  s_max, v_max), m1);
cv::inRange(hsv, cv::Scalar(170, s_min, v_min), cv::Scalar(180, s_max, v_max), m2);
cv::bitwise_or(m1, m2, red_mask);
```

**读单像素（y,x 顺序，p[2] 才是红）**
```cpp
cv::Vec3b p = img.at<cv::Vec3b>(y, x);   // B=p[0] G=p[1] R=p[2]
```

---

## 避坑清单（本章经典坑）

- [ ] BGR 顺序记反 → 红蓝颠倒。纯红 = `Scalar(0,0,255)`。
- [ ] `Mat b = a;` 误当深拷贝 → 改 b 连 a 一起改。独立副本用 `clone()`。
- [ ] `at<Vec3b>(y, x)` 写成 `(x, y)` → 取错像素或越界。
- [ ] H 范围按 0~360 写 → 其实 OpenCV 是 0~180（除以 2）。
- [ ] 红色只用一个 inRange → 只抠到半个红，忘双段 + bitwise_or。
- [ ] 忘关相机 AWB/AEC → 阈值随环境漂移。
- [ ] ROI 局部坐标忘加回 `roi_x/roi_y` → 后续 PnP 解算错位。
- [ ] 开/闭运算顺序记反：开=先蚀后胀（去噪），闭=先胀后蚀（填洞）。
- [ ] `findContours` 用了 `RETR_TREE` → 取到内部孔洞轮廓，多出一堆假目标。
- [ ] 面积阈值写死像素值 → 换高分辨率相机失效，用 `frame_area` 比例。
- [ ] 斜放物体用 `boundingRect` → 长宽比失真，用 `minAreaRect`。
- [ ] 长宽比分母忘 `+1e-5f` → 宽/高为 0 时除零。

---

## 思考与反思

1. **从「像素」到「结构」的跃迁**：mask 还只是"哪些点是白的"，`TargetObject` 已经是"中心在哪、多大、什么姿态"的结构化数据。这就是视觉算法的价值所在——把矩阵变成可用的信息。
2. **鲁棒性来自「选对表示」+「用先验过滤」**：选 HSV 是选对表示；面积/长宽比筛选用的是"道具大概多大、什么形状"的先验知识。比例阈值比绝对值抗环境变化。
3. **封装让流水线可复用**：`TargetDetector` 的接口是"进图出目标"，下一章 PnP 直接调用，不用动内部——这就是前面学的类/封装在真实项目里的落地。
4. **几何筛选的极限**：minAreaRect 只能给"平行对边"的矩形，真实投影是"近大远小"的四边形，下一章要升级。

> 留给自己填：这一章哪个概念卡得最久？____
