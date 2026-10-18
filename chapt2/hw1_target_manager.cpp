#include <cstdio>
#include <vector>
#include <algorithm>
#include <optional>
#include <cmath>

// 目标结构体（数据 + 方法）
struct Target {
    int color;
    float x, y, z;
    float confidence;

    float distance() const {
        return std::sqrt(x * x + y * y + z * z);
    }

    void print() const {
        printf("[目标] 颜色:%d 坐标:(%.2f, %.2f, %.2f) 距离:%.2f米 置信度:%.0f%%\n",
               color, x, y, z, distance(), confidence * 100);
    }
};

// 目标管理器：用 std::vector 存所有识别到的目标
class TargetManager {
public:
    void add(const Target &t) {
        targets.push_back(t);
    }

    // 返回距离相机最近的目标。
    // 容器为空时返回空 optional，绝不访问 targets[0]（否则越界崩溃）。
    std::optional<Target> get_nearest() {
        if (targets.empty()) {
            return std::nullopt;
        }
        std::sort(targets.begin(), targets.end(),
            [](const Target &a, const Target &b) {
                return a.distance() < b.distance();
            });
        return targets.front();
    }

    void print_all() const {
        for (const Target &t : targets) {
            t.print();
        }
    }

private:
    std::vector<Target> targets;
};

int main() {
    printf("=== 作业1：用类封装目标管理器 ===\n");

    TargetManager manager;
    manager.add({1, 0.5f, 1.2f, 3.0f, 0.95f});  // 红色，距离 ~3.31
    manager.add({2, 1.0f, 0.8f, 1.5f, 0.80f});  // 蓝色，距离 ~2.02
    manager.add({1, 0.2f, 0.3f, 0.5f, 0.99f});  // 红色，距离 ~0.62（最近）
    manager.add({3, 2.0f, 2.0f, 2.0f, 0.60f});  // 黄色，距离 ~3.46

    printf("--- 所有目标 ---\n");
    manager.print_all();

    printf("\n--- 最近目标 ---\n");
    if (auto nearest = manager.get_nearest()) {
        nearest->print();
    } else {
        printf("没有检测到目标\n");
    }

    // 防御性编程：空容器时不该崩溃
    printf("\n--- 空容器测试 ---\n");
    TargetManager empty_manager;
    if (auto n = empty_manager.get_nearest()) {
        n->print();
    } else {
        printf("空容器 get_nearest() 返回空，没有越界崩溃。\n");
    }

    return 0;
}
