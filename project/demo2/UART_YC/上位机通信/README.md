# 上位机串口通信模块（Jetson ↔ STM32）

按 CH10（软硬件握手）框架重构的上位机串口层：termios Raw Mode + CRC16 + 流式状态机 + 硬件回环自测。
协议详见 [protocol.md](protocol.md)。

## 目录结构

```
上位机通信/
├── protocol.md                      # 与电控约定的串口协议（命根子，两端都照它写）
├── CMakeLists.txt
├── include/robot_serial/
│   ├── crc16.hpp                    # CRC16/Modbus 查表（编译期生成表）
│   ├── serial_port.hpp              # 串口驱动（RAII，纯字节流，不关心协议）
│   ├── protocol.hpp                 # 帧常量/命令字/载荷结构体/打包/流式解包
│   └── serial_reader.hpp            # 后台接收线程（std::thread + 流式解包 + 回调）
├── src/serial_port.cpp              # 串口驱动实现（termios Raw Mode）
└── test_loopback.cpp                # 硬件回环自测
```

分层：`protocol`（帧）依赖 `crc16`；`serial_port` 是纯传输层，上层业务不直接碰串口 —— 换硬件只改 `serial_port`。
`serial_reader` 把「读字节 + 解包」装进独立线程，避免阻塞主/视觉线程。

## 编译

```bash
cd 上位机通信
cmake -B build
cmake --build build -j
```

## 自测（硬件回环，无需下位机）

用一根**母对母杜邦线把 USB-TTL 模块自己的 TX 和 RX 短接**，然后：

```bash
./build/test_loopback /dev/ttyTHS1     # 默认就是 /dev/ttyTHS1，可省
```

通过即说明：termios Raw Mode 没改字节、打包/CRC16 正确、流式状态机能解出上行帧。
若报 `Permission denied`：`sudo usermod -aG dialout $USER` 后**注销重登**（别用 `newgrp`，只对当前终端生效）。

## 发一帧真实数据（示例）

```cpp
#include "robot_serial/serial_port.hpp"
#include "robot_serial/protocol.hpp"
using namespace robot_serial;

SerialPort serial;
serial.openPort("/dev/ttyTHS1", 115200);

TrayPayload tray{320, 240};                       // 物料盘中心像素坐标
auto frame = packFrame(CMD_TRAY, &tray, sizeof(tray));
serial.writeAll(frame.data(), frame.size());      // 发出 [0xA5 04 11 ... 0x5A]
```

接收侧用 `StreamPacketParser` 逐字节喂入，解出 `cmd` 与 `payload` 后按命令字 `memcpy` 还原成对应结构体。

## 后台接收线程（推荐，不依赖 ROS）

串口接收是持续阻塞的，别放在主线程/视觉线程里。用 `SerialReader` 开一个后台线程，
切出完整上行帧（0xA6）就回调：

```cpp
#include "robot_serial/serial_port.hpp"
#include "robot_serial/protocol.hpp"
#include "robot_serial/serial_reader.hpp"
using namespace robot_serial;

SerialPort port;
port.openPort("/dev/ttyTHS1", 115200);

SerialReader reader(port);
reader.setCallback([](uint8_t cmd, const std::vector<uint8_t>& payload) {
    switch (cmd) {
        case CMD_TASK: { TaskPayload t; memcpy(&t, payload.data(), sizeof(t)); /* ... */ break; }
        case CMD_ODOM: { OdomPayload o; memcpy(&o, payload.data(), sizeof(o)); /* ... */ break; }
        case CMD_GYRO: { GyroPayload g; memcpy(&g, payload.data(), sizeof(g)); /* ... */ break; }
    }
});
reader.start();

// ... 主循环 / 视觉照常跑，互不阻塞 ...
reader.stop();   // 或析构自动停
```

回调在**接收线程内**执行：不要阻塞它、不要抛异常，碰共享变量自己加锁；`payload` 是
`const` 引用，仅在回调内有效，要保留请拷贝。

## 待办

- [ ] 下位机侧按同一份 [protocol.md](protocol.md) 实现（能发能收 + CRC16 + 0xA5/0xA6 分方向 + 0x7F 回环）。
- [ ] 里程计 OPS 未装，先用测试变量。
- [ ] 心跳 `0x00` 预留，后续做"上位机卡死 → 下位机急停"看门狗。
