# 第三章笔记：CMake 构建 与 VSCode 断点调试

> 用法：文字总结 + 语法速查片段。可复用 CMake / launch.json 模板在 `templates/`，作业工程在 `demo/`。

## 1. 为什么需要 CMake

**一句话**：手敲 g++ 只适合单文件；文件一多、要接外部库时，用 CMakeLists.txt 描述项目，让 CMake 生成底层编译指令。

三个痛点：① 命令太长；② 无增量编译（改一个文件全部重编）；③ 换机器库路径不同就报错。

## 2. 编译两阶段 + 两类报错

**一句话**：`.cpp` →（编译）→ `.o` →（链接）→ 可执行程序。报错就两类，先分清再看。

| 阶段 | 报错 | 原因 | 解法 |
|---|---|---|---|
| 编译期 | `fatal error: xxx.hpp: No such file or directory` | 找不到头文件 | `target_include_directories` 加搜索路径 |
| 链接期 | `undefined reference to ...` | 找不到函数实现 / 库 | 把 .cpp 加进 `add_executable`，或 `target_link_libraries` |

## 3. 规范目录结构

**一句话**：头文件放 `include/`，实现放 `src/`，编译产物全进 `build/`（外部构建）。

```
demo/
├── CMakeLists.txt
├── include/   # .hpp / .h
├── src/       # .cpp
└── build/     # 中间文件 + 可执行程序，出问题整个删掉重来，不碰源码
```

## 4. 现代 CMakeLists 五段式

**一句话**：最低版本 → 项目名 → C++ 标准 → 生成 target → 给 target 绑 include 路径。

```cmake
cmake_minimum_required(VERSION 3.16)
project(my_project VERSION 1.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_executable(app src/main.cpp src/xxx.cpp)
target_include_directories(app PRIVATE include)
```

现代构建命令（不用 cd，自动适配底层工具）：

```bash
cmake -B build            # 配置并生成 build/
cmake --build build -j    # 增量编译，-j 全核并行
./build/app               # 运行
```

## 5. 引入第三方库（OpenCV）

```cmake
find_package(OpenCV REQUIRED)                        # 找不到就报错停止
target_link_libraries(app PRIVATE ${OpenCV_LIBS})     # 链接
```

## 6. VSCode 断点调试

**一句话**：遇到逻辑 Bug / 崩溃，断点比 printf 直接。

- 选 Kit：`Ctrl+Shift+P` → `CMake: Select a Kit` → GCC/G++。
- ⚠️ **构建类型别写死**：`Debug` 无优化有符号 / `Release` -O3 无符号 / `RelWithDebInfo` -O2 有符号。用 `if(NOT CMAKE_BUILD_TYPE)` 设默认值。
- 打断点：行号左侧单击红点。
- 启动：底部小虫子图标，或 F5（需 `.vscode/launch.json`）。

| 快捷键 | 作用 |
|---|---|
| F5 | 继续到下一断点 |
| F10 | 单步跳过（不进入函数） |
| F11 | 单步进入（跳进函数内部） |
| Shift+F11 | 单步跳出（跑完当前函数返回上一层） |
| Shift+F5 | 停止调试 |

- 观察：Variables 看局部变量（vector 可展开）、Watch 输入表达式实时算、Call Stack 看崩在哪行谁调的。

---

## 语法速查片段

**构建类型默认值（别写死）**
```cmake
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "Build type" FORCE)
endif()
```

**launch.json 核心（program 别写死路径）**
```json
{
  "type": "cppdbg",
  "request": "launch",
  "program": "${command:cmake.launchTargetPath}",
  "MIMode": "gdb"
}
```

---

## 避坑清单（本章经典坑）

- [ ] `CMakeLists.txt` 大小写写错 / 放错目录 → CMake 找不到。
- [ ] 分不清编译期 vs 链接期错误 → 乱改一通。先看报错是「找不到头文件」还是「undefined reference」。
- [ ] 新增了 `.cpp` 忘加进 `add_executable` → undefined reference。
- [ ] 头文件在 `include/` 却没写 `target_include_directories` → 找不到头文件。
- [ ] 用了第三方库忘 `target_link_libraries` → undefined reference。
- [ ] 写死 `set(CMAKE_BUILD_TYPE Debug)` → 换 Release 打包时被覆盖。
- [ ] Release 构建下断点调试 → 变量值看不到 / 断点不生效（符号被剥离）。
- [ ] 改了 CMakeLists 或新增文件后没重新 `cmake -B build` → 编译没生效。
- [ ] 编译忘加 `-j` → 文件多了之后慢。
- [ ] launch.json 的 `program` 写死绝对路径 → 换目标就错，用 `${command:cmake.launchTargetPath}`。

---

## 思考与反思

1. **CMake 是「描述式」，不是「命令式」**：你描述项目长什么样，它替你生成编译命令。所以换机器、换平台，只要脚本不变，构建方式就一致。
2. **编译和链接是两件事**：报错先判断「找不到头文件」（编译期）还是「找不到实现」（链接期），定位速度直接翻倍。
3. **外部构建的价值**：`build/` 是纯产物，出问题删 `build/` 重来，源码永远干净——这就是「能把项目彻底复原」的底气。
4. **断点调试看「某一刻的状态」，printf 看「时间线上的输出」**，两者互补：崩溃用断点 + 调用堆栈，时序/频率问题用 printf。

> 留给自己填：这一章哪个概念卡得最久？____
