# works_learn

## 用黑色空心等边三角形验证测距

`triangle_pnp` 是新增的三角形检测程序，不改变原来的黄色方块 `pnp`。
打印源图为 `../charuco/image.png`，实物外面三条边**各长 9.6 cm**，高度约 8.314 cm。
请测量黑色外轮廓的边长，不是纸张宽度；等比例打印、贴平，确认三边相同。
内三角形三条边**各长 5.6 cm**；使用实测尺寸直接建模，不从图片比例估算。
假定内外两三角形同心、同向。可通过 `--side=0.096 --inner-side=0.056` 调整实测边长（米）。

方法：二值化 → 内外轮廓 → 三条边中段直线拟合 → 交点构成 6 个顶点 → 平面 PnP 多候选 + LM 精化。
在所有正深度候选中选择重投影 RMS 最小的结果。平面靶标仍可能存在位姿歧义，等边图案还有 120° 对称，
本程序用于距离验证，不报告唯一的朝向。目标需基本无遮挡、边缘清晰、内孔可见，避免过远/极端倾斜。

在工作区根目录运行：

```bash
cmake -S chapt5/pnp -B chapt5/pnp/build
cmake --build chapt5/pnp/build -j2
ctest --test-dir chapt5/pnp/build --output-on-failure
./chapt5/pnp/build/triangle_pnp
```

默认直接读取 `chapt5/charuco/camera_params.yaml` 的最新参数，**不是 pnp 目录中的旧参数**。
默认相机路径为当前外接 HJ 相机的 by-id，可用 `--camera=/dev/video2` 等指定正确设备。
仅适用于与标定一致的相机、焦距、裁剪及输出模式。分辨率必须与 YAML 相同，不会强制拉伸图像。
程序向相机请求标定分辨率，并验证实际帧尺寸。相同分辨率并不自动保证相同相机/模式。
按 `p` 在终端打印当前结果，`q` 或 Esc 退出。

已知到三角形重心的直线距离为 50 cm 时：

```bash
./chapt5/pnp/build/triangle_pnp --distance=0.50
```

打印当前结果时会输出实际距离与测距误差。`--distance` 单位米，是从相机光心到**外三角形重心**的
斜距，与 `norm(tvec)` 比较，不是沿桌面距离或到纸边的距离。实际量尺难以精确定位光心，需注意系统偏差。
`Z` 单独表示沿相机光轴的深度，目标偏离中心时与斜距不同。

已有**同一标定相机**拍的 640×480 照片：

```bash
./chapt5/pnp/build/triangle_pnp --image=/绝对路径/实拍照片.png --distance=0.50 --output=/tmp/triangle_result.png
```

输出图路径必须不存在，以免覆盖原照片；图中绿色是检测边线、红点是检测角点、紫圈是模型重投影。
打印源图只能检查图案检测，不能拿来测距（程序会拒绝对源文件求位姿）：

```bash
./chapt5/pnp/build/triangle_pnp --detect-only --image=chapt5/charuco/image.png
```

建议在 30/50/80 cm 等适合清晰检测的距离，分别放在画面中心和边缘验证。
例如目标测距误差要求为 3%，应确认各位置的实测误差满足要求；3% 不是通用“标定合格线”。
重投影 RMS 小仅表示当前 6 点与模型吻合，不能独立证明焦距或绝对距离正确。
测试用模拟投影验证算法实现，不验证真实摄像头的标定精度。
