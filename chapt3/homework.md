# 第三章作业记录（CMake 构建 + VSCode 断点调试）

> 记录原则：代码在哪 → 怎么构建 → 实际输出 → 观察到的现象。

---

## 作业：搭建规范最小工程 demo

- **工程**：`demo/`（`CMakeLists.txt` + `include/math_utils.hpp` + `src/math_utils.cpp` + `src/main.cpp`）
- **构建命令**：

```bash
cmake -B build            # 配置并生成 build/
cmake --build build -j    # 增量编译
./build/demo              # 运行
```

- **实际输出**：

```
-- The CXX compiler identification is GNU 11.4.0
-- Detecting CXX compiler ABI info - done
-- Configuring done
-- Generating done
-- Build files have been written to: .../demo/build

[ 66%] Building CXX object CMakeFiles/demo.dir/src/math_utils.cpp.o
[ 66%] Building CXX object CMakeFiles/demo.dir/src/main.cpp.o
[100%] Linking CXX executable demo
[100%] Built target demo

点 (0.5, 1.2, 3.0) 到原点距离: 3.270 米
```

- **观察**：
  - `cmake -B build` 只生成构建脚本，不编译；`cmake --build build -j` 才真正编译链接。两个 `.cpp` 各自生成 `.o` 再链接成 `demo`，正好对应「编译 → 链接」两阶段。
  - 结果 3.270 = √(0.5² + 1.2² + 3.0²) = √10.69，正确。
  - 编译产物全在 `build/` 里，源码目录 `src/` `include/` 保持干净（外部构建）。

---

## 作业步骤 4：断点调试体验（需在 VSCode GUI 里手动完成）

我无法替你点鼠标，但环境已全部就绪，你只需按下面走一遍：

1. 已写好 `demo/.vscode/launch.json`（`program` 用的是 `${command:cmake.launchTargetPath}`，自动追踪 CMake 当前目标）。
2. 已确认构建类型为 `RelWithDebInfo`，且 gdb 能查到 `calculate_3d_dist` 的调试符号：

```
构建类型: CMAKE_BUILD_TYPE:STRING=RelWithDebInfo
All functions matching regular expression "calculate_3d_dist":
4:	float calculate_3d_dist(float, float, float);
```

**你要做的（体验一次就记住）**：
1. VSCode 打开 `chapt3/demo/` 目录；
2. `Ctrl+Shift+P` → `CMake: Select a Kit` → 选 GCC/G++；
3. 打开 `src/main.cpp`，在 `float dist = calculate_3d_dist(x, y, z);` 那一行行号左侧单击打红点；
4. 点底部小虫子图标（或按 F5）；
5. 停在断点后按 **F11** 跳进 `math_utils.cpp`，看左侧 Variables 里 `x=0.5, y=1.2, z=3.0`；
6. 按 **Shift+F5** 停止。

- **体验完补一句**：跳进函数后，你在 Variables 窗口看到了什么？____
