# 第二章笔记：现代 C++（引用 / RAII / STL / 智能指针）

> 用法：文字总结 + 语法速查片段（只放 1~3 行、扫一眼能想起来的）。完整可复用框架在 `templates/`，作业代码在 `hw*.cpp`。

## 1. 引用（Reference）

**一句话**：引用 = 变量的别名，绑定后不能改、不存在空引用，底层近似指针的语法糖。

- `int &ref = a;` 之后 `ref` 和 `a` 就是同一个东西。
- 函数想改外部变量 → 传引用，写起来和普通变量一样：

```cpp
void detect(float &out_x, float &out_y) { out_x = 1.5f; out_y = 2.3f; }
detect(x, y);   // 调用时不用 &，函数里不用 *
```

**传参黄金法则**（高频避坑）：

| 参数类型 | 写法 |
|---|---|
| 基本类型 int/float/bool | 直接传值 |
| 大对象 struct/string/vector | `const T &` |
| cv::Mat 图像 | `const cv::Mat &` |

## 2. 类与 RAII（本章最核心的思想）

**一句话**：class = 数据 + 方法；RAII = 资源在构造获取、析构释放，作用域结束自动释放。**永远不要在裸代码里写 new/delete 或 malloc/free。**

- 构造函数 `ClassName(...)`：创建时自动初始化；析构函数 `~ClassName()`：销毁时自动清理。
- 独占资源要禁拷贝，防止 double free / double close：

```cpp
SerialPort(const SerialPort &) = delete;
SerialPort &operator=(const SerialPort &) = delete;
```

- RAII 适用一切资源：内存、文件、串口、相机连接——绑到对象上，让作用域自动管理。

## 3. STL 容器

**一句话**：忘了 C 的 malloc 数组和 char[]，全换 STL。

- `std::vector<T>`：动态数组，`push_back` / `size()` / `[i]` / 范围 for，自动扩容自动释放。替代 90% 动态数组。
- `std::string`：`+` 拼接、`==` 比较、`.find()`，告别 strcmp / 缓冲区溢出。
- `std::pair` / `std::tuple`：打包多返回值，C++17 结构化绑定：

```cpp
auto [u, v] = get_pixel_center();       // pair
auto [x, y, z] = get_3d_position();     // tuple
```

- `std::queue<T>`：FIFO（相机线程塞帧、算法线程取帧）。⚠️ 非线程安全，多线程要加 `std::mutex`。
- `std::map<K, V>`：键值映射，`color_names[1] = "红色";`

## 4. 语法糖

**一句话**：写更少代码做更多事。

- `auto`：编译器推导类型（编译期就确定，不是弱类型）。
- 范围 for：`for (const Target &t : targets)` 只读、`for (float &d : ds)` 可改。
- Lambda：`[捕获](参数){函数体}`，`[]` 不捕获 / `[&]` 按引用全捕获 / `[=]` 按值全捕获 / `[&x]` 只捕一个。最常用给 sort 传比较规则：

```cpp
std::sort(v.begin(), v.end(),
    [](const Target &a, const Target &b){ return a.distance() < b.distance(); });
```

## 5. 智能指针

**一句话**：RAII 版的指针，对象销毁自动 delete。

- `std::unique_ptr` 独占（首选，不能拷贝只能移动）：`auto p = std::make_unique<T>();`
- `std::shared_ptr` 共享（引用计数，归零才释放）：`auto p = std::make_shared<T>();`
- 热路径只读：传 `const std::shared_ptr<T>&` 或解引用后的 `const T&`，避免原子引用计数开销。

## 6. include 与命名空间

- C 头 `.h` → C++ `<c...>`：`<cstdio>` `<cmath>` `<cstdint>`；C++ 独有 `<vector>` `<string>` `<memory>` `<algorithm>`。
- ⚠️ 别在 `.h` 头文件里 `using namespace std;`，会污染所有包含它的文件。.cpp 里随便。

## 7. C vs C++ 编译

| | C | C++ |
|---|---|---|
| 后缀 | .c | .cpp |
| 编译器 | gcc | g++ |
| 字符串 | char[] + strcmp | std::string |
| 动态数组 | malloc/free | std::vector |
| 内存 | 手动 free | RAII + 智能指针 |

```bash
g++ -std=c++17 main.cpp target.cpp -o app
```

---

## 避坑清单（本章经典坑）

- [ ] 引用声明时没初始化 → 编译错（引用必须绑定、不能空、不能改绑）。
- [ ] 大对象传参忘 `const &` → 每次调用深拷贝整段数据。
- [ ] 返回局部变量的引用 → 悬垂引用，和 C 返回栈指针一样送命。
- [ ] 独占资源（串口/文件）忘写 `= delete` → 拷贝导致 double close。
- [ ] 热路径里 `shared_ptr` 按值传 → 每次原子引用计数增减 + 内存屏障开销。
- [ ] 多线程直接用 `std::queue` → 数据竞争崩溃，要 `std::mutex` 封装。
- [ ] 头文件里 `using namespace std` → 污染命名空间。
- [ ] 用了结构化绑定 / optional 却忘 `-std=c++17` → 编译错。

---

## 思考与反思

1. **从 C 到 C++ 的转变就一句话**：从"手动记得释放"到"让作用域自动释放"。RAII 是这章所有东西的根——vector、string、智能指针、SerialPort 全部是 RAII。
2. **传参法则背后是两个词**：避免拷贝 + 明确所有权。基本类型传值（本来就快），大对象 `const&`（不拷贝），引用（明确"我要改你"）。
3. **越界崩溃是可防的**：`get_nearest` 里的 `empty()` 判断 + `std::optional`，把"没有目标"从崩溃变成正常返回值，这才是比赛现场能活的代码。

> 留给自己填：这一章哪个概念卡得最久？____
