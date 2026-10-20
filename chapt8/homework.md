# 第八章作业记录（CH11：结业实战 —— 道具抓取定位 Demo）

> 记录原则：**贴真实终端输出，不贴理想输出**。没跑通就写"卡在哪"。
>
> 这是结业终局战，三个试炼层层递进：先**编译**，再**软硬件闭环**，最后**真机抓取**。每一步都依赖上一步。

---

## 试炼 1：组装全套代码并编译（无需硬件）

```bash
cmake -B build -S .
cmake --build build -j$(nproc)
```

**验收标准**：链接出 `./build/prop_vision_node`，无报错。

<details>
<summary>我的输出（复制到这里）</summary>

```
（贴真实终端输出）
```
</details>

> 结构说明：四个模块里 `PingPongFrameBuffer.hpp`（复用 chapt6）和 `SerialPort.hpp`（复用 chapt7）是 header-only，`TargetDetector` / `PnPSolver` 有 .cpp。所以 `add_executable` 里只列 main.cpp + 两个 .cpp。

---

## 试炼 2：软硬件全闭环联调

**目标**：上位机通过 USB-TTL 连单片机，在镜头前移动道具，单片机用串口打印收到的物理坐标 (X_r, Y_r, Z_r)。

**准备**：
1. 把 main.cpp 里的**相机内参**换成你 chapt5 标定的真实 K 值（`920/920/640/360` 是占位假值）；
2. 把**相机下俯角** `CAMERA_PITCH_DEG`、**道具尺寸** `0.15f` 改成实际值；
3. 相机改成你的实际设备（by-id 路径或索引）；
4. 串口 `/dev/ttyUSB0` 连单片机（或先短接 TX/RX 用 chapt7 的回环思路自查）。

**带窗口调试运行**：
```bash
./build/prop_vision_node --gui
```

**验收标准**：镜头前放道具 → 窗口里画出绿色四边形 + 左上角显示 X/Y/Z/Yaw → 单片机端收到一帧帧 18 字节二进制包、CRC8 校验通过、坐标随道具移动而变。

<details>
<summary>我的输出（复制到这里）</summary>

```
（贴单片机端收到的串口数据 / 上位机终端输出）
```
</details>

---

## 试炼 3：第一次视觉引导自主抓取

**目标**：配合机械臂同学，在物理世界完成第一次抓取。

**验收标准**：机械臂根据视觉坐标稳定伸到道具上方并抓取成功。记录：抓取成功率、误差（厘米级）、暴露出来的问题（光照？遮挡？延时？）。

<details>
<summary>我的记录（复制到这里）</summary>

```
（成功/失败次数、误差、发现的问题、下次怎么改）
```
</details>

---

## 附加：开机自启部署（可选，真上车才需要）

1. 改 `robot_vision.service` 里的 `User=` 和 `ExecStart=` 为你的用户名和二进制绝对路径；
2. `sudo cp robot_vision.service /etc/systemd/system/`；
3. `sudo systemctl daemon-reload && sudo systemctl enable --now robot_vision`；
4. `sudo systemctl status robot_vision` 看到 `active (running)` 即成功。

> 上车前务必用 `./build/prop_vision_node`（不带 `--gui`）跑一遍，确认 headless 下不崩（cv::imshow 已用开关保护）。
