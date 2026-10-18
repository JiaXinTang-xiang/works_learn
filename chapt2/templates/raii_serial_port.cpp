// ====================================================================
// 可复用骨架：RAII 串口封装
// 用法：把这个类拷进你的工程，把注释里的模拟代码换成真实系统调用即可。
//   fd = ::open(device, O_RDWR | O_NOCTTY);
//   ::close(fd);
//   ::write(fd, data, len);
// ====================================================================
#include <cstdio>
#include <cstdint>

class SerialPort {
public:
    int fd;  // Linux 下串口用文件描述符打开

    // 构造函数：对象一创建就打开串口（RAII：资源获取即初始化）
    SerialPort(const char *device) {
        printf("打开串口: %s\n", device);
        // fd = ::open(device, O_RDWR | O_NOCTTY);  // 真实代码
        fd = 42;  // 模拟
    }

    // 析构函数：对象销毁时自动关闭串口（RAII：资源随作用域释放）
    ~SerialPort() {
        printf("关闭串口 fd=%d\n", fd);
        // ::close(fd);  // 真实代码
    }

    // 工业级规范：独占资源禁止拷贝！
    // 防止 SerialPort p2 = p1 导致同一个 fd 被析构两次（Double Close 灾难）
    SerialPort(const SerialPort &) = delete;
    SerialPort &operator=(const SerialPort &) = delete;

    void send(const uint8_t *data, int len) {
        printf("发送 %d 字节: ", len);
        for (int i = 0; i < len; i++) {
            printf("%02X ", data[i]);
        }
        printf("\n");
        // ::write(fd, data, len);  // 真实代码
    }
};

int main() {
    {
        SerialPort serial("/dev/ttyUSB0");  // 构造：自动打开
        uint8_t packet[] = {0xA5, 0x01, 0x02};
        serial.send(packet, 3);
    }  // 出作用域，析构自动关闭串口，不用手动 close

    printf("串口已被自动关闭（析构函数自动调用）\n");
    return 0;
}
