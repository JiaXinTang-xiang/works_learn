#include <stdio.h>
#include <stdint.h>

// 默认对齐（不加 #pragma pack）
typedef struct {
    uint8_t  sensor_id;    // 1 字节
    double   temperature;  // 8 字节
    uint8_t  status;       // 1 字节
} SensorDataDefault;

// 强制单字节紧凑对齐   pragma pack
#pragma pack(push, 1)
typedef struct {
    uint8_t  sensor_id;    // 1 字节
    double   temperature;  // 8 字节
    uint8_t  status;       // 1 字节
} SensorDataPacked;
#pragma pack(pop)

int main() {
    printf("=== 作业1：结构体字节对齐验证 ===\n");
    printf("成员大小之和: %zu + %zu + %zu = %zu 字节\n",
           sizeof(uint8_t), sizeof(double), sizeof(uint8_t),
           sizeof(uint8_t) + sizeof(double) + sizeof(uint8_t));
    printf("默认对齐 sizeof(SensorDataDefault) = %zu 字节\n",
           sizeof(SensorDataDefault));
    printf("紧凑对齐 sizeof(SensorDataPacked)  = %zu 字节\n",
           sizeof(SensorDataPacked));
    return 0;
}
