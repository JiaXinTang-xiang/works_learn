# 以实物板为准的 ChArUco 标定

当前程序匹配 `my_charuco.jpg` 中的实物：9×6 方格、`DICT_5X5_50`、27 个标记。
`physical_charuco_board.hpp` 显式定义标记的位置、编码角点方向及角点邻接表，
不再依赖 OpenCV 4.5.4 的默认旧布局。板子可以任意旋转，不需要转动实物来适配代码。

## 编译、检查

在本目录运行：

```bash
cmake -S . -B build
cmake --build build -j2
ctest --test-dir build --output-on-failure
./build/charuco_calibrate --check-image my_charuco.jpg
```

离线检查不会打开相机或覆盖 `camera_params.yaml`。可在输入图片后追加输出 PNG 路径，保存角点标注图。
单图平面残差只用来检查此照片的布局对应关系，不是相机标定误差。

## 重新标定

1. 用尺子测量实物的方格边长和标记黑色外框边长，填写程序的 `squareLength`、`markerLength`。
   现有 0.024 m、0.018 m 是原代码的值，不能从照片确定实际米制尺寸。
2. 运行 `./build/charuco_calibrate`，按 `c` 采集多位置、多距离、多倾角的清晰图像，建议 20～30 帧。
3. 按 `s` 计算。输出写到当前工作目录的 `camera_params.yaml`，包括分辨率、整体 RMS、板子参数。
4. 旧参数来自旧模型，必须重新采集计算；本次代码修改不会自动修正旧 YAML。
   RMS 小不等于一定正确，还需检查参数稳定性，并用未参与标定的新图和已知尺寸物体验证。

目录中的 `charuco_board_9x6_4x4_50.png` 属于另一块板，不适用于当前实物模型。
OpenCV 4.5.4 的 `board->draw()` 仍按旧棋盘底色绘图，不能用于重印此适配模型。
