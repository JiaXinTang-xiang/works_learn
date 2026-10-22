#include "ti_msp_dl_config.h"
#include "bsp.h"

/* 运行模式 */
uint8_t RunFlag = 0;        /* 0=停车  1=直行  2=循迹 */

/* 目标速度 */
int32_t left_pwm  = 20;     /* 左轮目标, 按键可调 */
int32_t right_pwm = 20;     /* 右轮目标, 按键可调 */

/* 实测速度 */
int32_t left_speed;         /* 左轮实测 */
int32_t right_speed;        /* 右轮实测 */

/* 云台电机(CAN) */
QD4310_t Motor_0;

/* 视觉模块 */
uint8_t  RxState;
uint16_t center_x, center_y, area_value;
float    ypr[3];



/* 主函数 */
int main(void)
{
    SYSCFG_DL_init();
    while (DL_MCAN_OPERATION_MODE_NORMAL != DL_MCAN_getOpMode(MCAN0_INST)) {}

    Vision_init();
	uart0_init(115200);
	Wireless_init(115200);
	Host_UART_Init();          /* 上位机串口协议（Communication=UART1，115200） */
    timerg6_pwm_rgb_init();
		
    delay_ms(100);
    IMU_init();
    SPI1_CS_IMU(1);
    SPI1_CS_IMU(1);
    delay_ms(100); 
		
    QD4310_Init(&Motor_0, 0x00);
    QD4310_Enable(&Motor_0);
    DL_GPIO_clearPins(FMQ_PORT, FMQ_BBB_PIN);
    TimerA1_init();
    Motor_Init();
	OLED_Init();
	OLED_ShowString(1,1,"Hello World!");

    while (1)
	{
		IMU_getYawPitchRoll(ypr);
		Host_UART_Poll();   /* 处理上位机下发的帧（回环回发 / 解析到 vision_data） */
	}
}









