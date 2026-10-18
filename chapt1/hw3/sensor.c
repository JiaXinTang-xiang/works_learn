#include <stdio.h>
#include "sensor.h"

void print_sensor_sizes(void) {
    printf("成员大小之和: %zu + %zu + %zu = %zu 字节\n",
           sizeof(uint8_t), sizeof(double), sizeof(uint8_t),
           sizeof(uint8_t) + sizeof(double) + sizeof(uint8_t));
    printf("默认对齐 sizeof(SensorDataDefault) = %zu 字节\n",
           sizeof(SensorDataDefault));
    printf("紧凑对齐 sizeof(SensorDataPacked)  = %zu 字节\n",
           sizeof(SensorDataPacked));
}

void print_float_bytes(float value) {
    uint8_t *byte_ptr = (uint8_t *)&value;
    printf("float %.4f 在内存中的 4 个字节: ", value);
    for (int i = 0; i < 4; i++) {
        printf("%02X ", byte_ptr[i]);
    }
    printf("\n");
}
