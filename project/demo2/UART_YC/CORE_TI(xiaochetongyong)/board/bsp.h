#ifndef BSP_H
#define BSP_H

/******************系统头文件***************/
#include <ti/devices/msp/msp.h>
#include <ti/driverlib/driverlib.h>
#include <ti/driverlib/m0p/dl_core.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdbool.h>
#include "math.h"
#include "ti_msp_dl_config.h"
/*******************************************/
#define u8 unsigned char
#define u32 unsigned int
#define u16  uint16_t
#define fp32 float
#define fp64 double
#ifndef bool_t
typedef uint8_t bool_t;
#endif

static inline void delay_ms(uint32_t ms)
{
    delay_cycles(80000 * ms);
}
static inline void delay_us(uint32_t us)
{
    delay_cycles(80 * us);
}
/******************用户自定义头文件*********/

/* board（板卡驱动层） */
#include "drv_can.h"
#include "bsp_led.h"
#include "bsp_key.h"
#include "bsp_RGB.h"
#include "bsp_buzzer.h"

/* application（应用层） */
#include "EDC_task.h"
#include "Timer_task.h"
#include "Xunji_task.h"
#include "Vision_task.h"
#include "Wireless_task.h"
#include "TypecUART_task.h"
#include "host_uart.h"

/* Algorithm（算法层） */
#include "pid.h"

/* Hardware（其它硬件类） */
#include "OLED.h"
#include "TB6612.h"

/* IMU（陀螺仪类） */
#include "spi1.h"
#include "IMU.h"
#include "inv_imu_driver.h"

/* Motor（电机类） */
#include "QD4310.h"

/*******************************************/

#endif