# 第七章笔记（CH10：软硬件握手 —— Linux 串口、结构体内存打包与校验）

> 本章解决一个问题：**上位机（视觉 PC）算出来的目标坐标，怎么可靠地传给下位机（STM32/单片机）。**
> 载体是串口（UART），核心是四件事：① 把串口配成"透明字节流"（termios Raw Mode）；② 自定义二进制协议帧；③ 结构体打包（对齐 + 端序）；④ 加校验（CRC8）防噪声。

---

## 1. 硬件连接与设备节点（一句话）
**串口是一根"没有包概念的字节流"**：USB-TTL 转接板插上后 Linux 分配 `/dev/ttyUSB0`（CH340/CP2102）或 `/dev/ttyACM0`（带 CDC 的芯片），程序读写的其实是这一个字符设备文件；但串口**不会告诉你一帧从哪开始、到哪结束**，帧边界必须靠自定义协议自己切。

- 常见设备名：`ls /dev/ttyUSB* /dev/ttyACM*`
- 权限问题：默认只有 root 和 `dialout` 组能开串口 → `sudo usermod -a -G dialout $USER` 后**重新登录**才生效。
- 设备名会漂移：多插一个 USB 设备，`ttyUSB0` 可能变成 `ttyUSB1`，所以工业上用 `/dev/serial/by-id/...` 软链接定位。

## 2. termios 原始模式（为什么串口不能当普通文件读）
**默认串口是"终端模式"**：它会把 `\n` 翻译成 `\r\n`、把输入攒成一行才返回、还回显字符。这些对**人打字**友好，但对**二进制数据**是灾难——比如 `0x0A` 会被当换行符处理。所以必须 `cfmakeraw()` 全部关掉，让串口变成"读什么来什么"的透明管道。

| 设置 | 含义 | 关掉会怎样 |
|------|------|-----------|
| `cfmakeraw()` | 关行缓冲/字符翻译/信号/回显 | 二进制字节被篡改、读不到数据 |
| `CS8 / ~PARENB / ~CSTOPB` | 8 数据位 / 无校验 / 1 停止位 = **8N1** | 与下位机对不上，全是乱码 |
| `~CRTSCTS` | 关硬件流控 | 不接 RTS/CTS 线时会卡死 |
| `VMIN=0, VTIME=1` | 读超时 100ms | 永久阻塞，线程卡死 |

> 两个配置的坑：`cfsetispeed` 和 `cfsetospeed` 都要设（输入输出波特率分别设置）；`tcsetattr` 前先 `tcflush` 清掉缓冲里的残留垃圾字节。

## 3. 工业级二进制协议帧（为什么不用 ASCII）
**ASCII 字符串协议（如 `"X:0.523,Y:-0.185\n"`）人看得懂，但下位机解析慢、占用带宽、格式易错**。工业上直接传二进制帧：

```
[帧头 0xA5][长度 Len][命令 CmdID][载荷 Payload ...][CRC8][帧尾 0x5A]
  1 字节    1 字节    1 字节      Len 字节         1 字节  1 字节
```

- **帧头/帧尾**：定界符，定位一帧的起止。
- **长度 Len**：接收端知道 Payload 有多长（协议可扩展的关键）。
- **命令 CmdID**：区分帧类型（这是坐标、那是心跳包）。
- **CRC8**：校验 `Header+Len+Cmd+Payload`（帧尾不参与），噪声比特翻转必被发现。

## 4. 内存对齐与端序（结构体直接发串口的两大坑）
**C++ 结构体成员之间会插入"填充字节"对齐**。例如 `{ uint8_t a; float b; }` 里 `a` 和 `b` 之间默认塞 3 字节填充，`sizeof` 会从 5 变 8。直接 `write(&s, sizeof(s))` 会把填充垃圾一起发出去，下位机按 18 字节解析就错位了。

- `#pragma pack(push, 1)` / `#pragma pack(pop)`：强制 1 字节对齐，去掉所有填充。
- **端序**：x86 PC 是**小端**（低字节在前），STM32 通常也是小端，所以 PC↔单片机一般不用转；但若连大端设备，`float` 必须逐字节翻转。
- **解包唯一正确姿势是 `memcpy`**：`memcpy(&rx_struct, &buf[3], sizeof(rx_struct))`。**严禁** `(PropTargetPayload*)(&buf[3])` 强转——数组起始地址不一定 4 字节对齐，ARM 上解引用非对齐 `float*` 直接 HardFault 死机。

## 5. 校验算法（Sum8 vs CRC8）
**没有校验的帧 = 裸奔**。电机一开，串口线上的电磁干扰就能随机翻转比特，把坐标 `0.523` 变 `-18.3`。

- **Sum8**：所有字节相加取低 8 位。实现简单，但**偶数个互补错误查不出**（两个比特都翻可能和不变）。
- **CRC8（本章用，多项式 0x31）**：位运算滚动异或，一个比特翻转几乎必然改变结果，能检出绝大多数单比特/双比特错误。
- 校验逻辑：**接收端用同样算法重算一遍 CRC，和帧里携带的 CRC 比**；不一致 → 整帧丢弃，绝不解包。

## 6. SerialPort 类 + 回环自测
**RAII 封装文件描述符**：对象析构自动 `close(fd)`，不怕忘记关。类把"打开串口 / 封包 / CRC8 / 读字节"封装成可复用组件，后续视觉、通信模块直接 `#include "SerialPort.hpp"` 用。

- **回环自测**：把 USB-TTL 的 **TX 和 RX 用杜邦线短接**，程序发出去的数据立刻被自己读回来。这是无下位机时验证"封包/校验/解包"整条链路的最快方法——数据能自收自检通过，说明逻辑没错。
- `buildFrame` 抽成静态函数（不碰 fd）：**没有串口硬件也能单测封包/CRC 逻辑**（`test_pack.cpp`）。

## 7. 流式字节解析状态机 FSM（进阶）
**串口是流，`read()` 不保证一次读到整帧**。可能这次只读到半帧、下次多读了下半帧 + 下一帧的开头。所以真正的接收端不能"读完一帧再处理"，而是**逐字节喂给状态机**：

```
WAIT_HEADER ──收到0xA5──▶ WAIT_LEN ──存长度──▶ WAIT_PAYLOAD ──收满Len字节──▶ WAIT_CRC ──校验过──▶ WAIT_TAIL ──收到0x5A──▶ 交出一帧，回到 WAIT_HEADER
      ▲                                                                                                │
      └────────────────────────────── 任意一步失败，丢弃当前字节回到开头 ◀──────────────────────────────┘
```

状态机是本章"读者练习/进阶"部分，核心思想：**把"一次性解析"改造成"每来一个字节推进一步状态"**，天然抗分包、抗粘包。

---

## 语法速查片段

```cpp
// ① 打开串口 + 配成 8N1 Raw Mode
int fd = open("/dev/ttyUSB0", O_RDWR | O_NOCTTY | O_NDELAY);
fcntl(fd, F_SETFL, 0);                 // 恢复阻塞
struct termios tty;
tcgetattr(fd, &tty);
cfmakeraw(&tty);                       // 关键：关掉终端翻译
cfsetispeed(&tty, B115200);
cfsetospeed(&tty, B115200);
tty.c_cflag |= (CLOCAL | CREAD);
tty.c_cflag &= ~CSIZE;  tty.c_cflag |= CS8;   // 8 数据位
tty.c_cflag &= ~PARENB;                        // 无校验
tty.c_cflag &= ~CSTOPB;                        // 1 停止位
tty.c_cflag &= ~CRTSCTS;                       // 关硬件流控
tty.c_cc[VMIN] = 0;  tty.c_cc[VTIME] = 1;      // 100ms 超时
tcflush(fd, TCIOFLUSH);
tcsetattr(fd, TCSANOW, &tty);

// ② 紧凑结构体（1 字节对齐，去掉填充）
#pragma pack(push, 1)
struct Payload { uint8_t type; float x, y, z, yaw; uint8_t conf; };
#pragma pack(pop)

// ③ CRC8（多项式 0x31）
uint8_t crc8(const uint8_t* d, size_t n) {
    uint8_t crc = 0;
    for (size_t i = 0; i < n; ++i) {
        crc ^= d[i];
        for (int j = 0; j < 8; ++j)
            crc = (crc & 0x80) ? (crc << 1) ^ 0x31 : (crc << 1);
    }
    return crc;
}

// ④ 安全解包（严禁指针强转！）
Payload p;
std::memcpy(&p, &buf[3], sizeof(Payload));   // buf[3] 是 Payload 起点

// ⑤ 原始字节按十六进制打印（调试帧必备）
std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)byte;
```

---

## 避坑清单（本章经典坑）

1. **忘记 `cfmakeraw()`**：`\n`→`\r\n` 被翻译、读到一行为止，二进制帧直接被拆烂。
2. **`cfsetispeed`/`cfsetospeed` 只设一个**：收发波特率不一致，看似通了实际乱码。
3. **`VMIN=0, VTIME=0`**：变成非阻塞，`read()` 立刻返回 0，空转烧 CPU。
4. **权限没配**：`Permission denied` —— 加了 dialout 组必须**注销重新登录**才生效。
5. **设备名漂移**：拔插后 `ttyUSB0` 变了，用 `/dev/serial/by-id/` 定位更稳。
6. **用指针强转解包**：`(Payload*)buf` 非对齐解引用，x86 没事、ARM 上直接 HardFault。
7. **结构体没加 `pack(1)`**：编译器插入填充字节，`sizeof` 变大、下位机解析错位。
8. **`write()` 一次没写完**：串口 `write` 理论上可能短写，生产代码要循环写满（23 字节的小帧几乎不会，但要知道这点）。
9. **一次 `read()` 假设读到整帧**：流式传输会分包，必须用状态机或 `readExact` 循环读满。
10. **校验范围算错**：CRC 只覆盖 `Header+Len+Cmd+Payload`，帧尾 `0x5A` 不参与，别多算/少算。
11. **篡改测试放在校验之后**：`rx_raw[5] ^= 0xFF` 必须加在**解包前**，否则测了个寂寞。
12. **用 `==` 判断浮点相等**：解包验证用 `rx.x == tx.x` 可以（同一次 memcpy 往返），但跨端传输的浮点应比绝对值误差 `< 1e-4`。

---

## 思考与反思

- **为什么串口协议要自己定？** 因为串口只保证"字节顺序送达"，不保证"帧完整、帧边界、内容没被噪声翻比特"。帧头尾解决边界、Len 解决长度、CRC8 解决正确性——这三样加上去，一根裸线才变成一个可靠通道。
- **`buildFrame` 为什么要抽成静态函数、不碰 fd？** 把"纯逻辑"和"硬件 IO"拆开，逻辑就能**无硬件单测**（`test_pack.cpp`）。这是可测试性设计：凡是能脱离硬件验证的，就别绑死在硬件上。
- **本章和上一章（CH9）是怎么串起来的？** CH9 的多线程负责"视觉帧采集"，本章负责"结果下发"。完整的 RC 链路是：相机 → 工业相机驱动 → 图像处理/PNP 解算（坐标）→ **串口打包 + CRC8** → 下位机。串口就是视觉和电控之间的那根"握手线"。
- **为什么工业上宁可用难读的二进制，也不用好读的 ASCII？** 下位机是资源极少的单片机：ASCII 要 `printf/sscanf` 解析（慢、代码大、易错），二进制直接 `memcpy` 进结构体（快、省 Flash）。人的可读性让步于机器的可靠性。

> 留给自己填：串口设备名用 `ls /dev/ttyUSB*` 看到的是哪个？有没有遇到 `Permission denied`？
