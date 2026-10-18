#include <stdio.h>
#include <stdint.h>

void print_float_bytes(float value) {
    uint8_t *byte_ptr = (uint8_t *)&value;  // 把 float 的地址当成逐字节的指针
    printf("float %.4f 在内存中的 4 个字节: ", value);
    for (int i = 0; i < 4; i++) {
        printf("%02X ", byte_ptr[i]);       // 逐个字节打印，%02X 至少两位、补零
    }
    printf("\n");
}

int main() {
    printf("=== 作业2：指针逐字节查看浮点数的内存表示 ===\n");
    float target_x = 1.234f;
    print_float_bytes(target_x);

    printf("IEEE 754 数学表示应为 0x3F9DF3B6，\n");
    printf("小端序（Little-Endian）下内存里顺序是颠倒的。\n");
    return 0;
}
