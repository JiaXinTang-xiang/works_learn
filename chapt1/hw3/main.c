#include <stdio.h>
#include "sensor.h"

int main() {
    printf("=== 作业3：多文件编译（sensor.h + sensor.c + main.c） ===\n");
    print_sensor_sizes();      // 作业1 的内容
    print_float_bytes(1.234f); // 作业2 的内容
    return 0;
}
