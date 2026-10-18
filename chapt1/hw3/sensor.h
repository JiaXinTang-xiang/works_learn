#ifndef SENSOR_H    // 头文件保护宏，防止重复包含
#define SENSOR_H

#include <stdint.h>

// 默认对齐结构体（用于对比，作业1）
typedef struct {
    uint8_t  sensor_id;    // 1 字节
    double   temperature;  // 8 字节
    uint8_t  status;       // 1 字节
} SensorDataDefault;

// 紧凑对齐结构体（跨设备通信必须强制 1 字节对齐）
#pragma pack(push, 1)
typedef struct {
    uint8_t  sensor_id;
    double   temperature;
    uint8_t  status;
} SensorDataPacked;
#pragma pack(pop)

// 函数声明（原型）——实现放在 sensor.c 里
void print_sensor_sizes(void);        // 作业1：打印两种结构体大小
void print_float_bytes(float value);  // 作业2：逐字节打印浮点数

#endif // SENSOR_H
