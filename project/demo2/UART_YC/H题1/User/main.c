#include "bsp.h"
#include "Serial.h"
#include "KEY.h"
#include "SerialK230.h"
#include "OLED.h"

//int i = 0;
//int Run = 0;

//extern int k230_w;
//extern int k230_h;
//extern int k230_flag;
//extern uint16_t diatance;
//extern u8 out;
//uint64_t area;
//int taskchoise;

int main(void)
{
	/* 模块初始化 */
	Timer_Init();
	Serial_Init();	 // 涓插彛鍒濆鍖栵紙jetson锛?
	CanMotor_Init(); // 鐢垫満鍒濆鍖?
	Serial62_Init(); // JY62鍒濆鍖?
	Serial_Init_K();
//	GY56_Init();
//	PID_Ctrl_Init(); // PID鍒濆鍖?
	Key_Init();
	Delay_s(1);
	fmq_Init();

	while (1)
	{
		if (Serial_GetRxFlag() == 1)
		{

		}
		
	}
}

void TIM1_UP_IRQHandler(void)
{
	static uint16_t count0, count0_flag = 0;
	if (TIM_GetITStatus(TIM1, TIM_IT_Update) == SET)
	{

		Key_Tick();

		TIM_ClearITPendingBit(TIM1, TIM_IT_Update);
	}
}
