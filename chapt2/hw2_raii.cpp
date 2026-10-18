#include <cstdio>
#include <memory>

// 模拟相机：构造时连接，析构时断开
class CameraSimulator {
public:
    CameraSimulator() {
        printf("相机已连接\n");
    }
    ~CameraSimulator() {
        printf("相机已断开\n");
    }
    void grab() {
        printf("  抓取一帧图像...\n");
    }
};

int main() {
    printf("=== 作业2：体验 RAII 与智能指针 ===\n");

    printf("--- 进入作用域 ---\n");
    {
        // 用 unique_ptr 独占管理，出作用域自动 delete，不用手动释放
        auto cam = std::make_unique<CameraSimulator>();
        cam->grab();
    } // cam 在此销毁，析构函数自动调用
    printf("--- 离开作用域（相机应已自动断开）---\n");

    // 对比：裸 new/delete 忘写 delete 就泄漏
    // CameraSimulator *raw = new CameraSimulator();
    // delete raw;  // 漏掉这行 = 内存泄漏

    return 0;
}
