/**
 * Timer_task.c — 定时器中断(RunFlag判断 + 调度)
 *
 * 100Hz: 根据RunFlag决定停车/直行/循迹→速度闭环→电机输出
 * 50Hz:  按键处理
 */
#include "Timer_task.h"


volatile uint32_t nowtime = 0;
extern float ypr[3];
extern uint8_t  RunFlag;
extern int32_t  left_pwm, right_pwm;
extern int QH[11], BLK_CNT;    // QH[路口号]=方向 BLK_CNT黑线计数

int aaa=0;

void TimerA1_INST_IRQHandler(void)
{
    static uint32_t count1;
    static uint32_t count2;

    switch (DL_TimerA_getPendingInterrupt(TimerA1_INST)) {
    case DL_TIMERA_IIDX_LOAD:
        nowtime++;
        count1++;
        count2++;
        Key_Tick();

		if(nowtime%1000==0) LED1_toggle;	// ICM45686时钟基准
	
        /* ===== 100Hz: 电机控制 ===== */
        if (count1 >= 100)
        {
			count1 = 0;
            int32_t LeftOut, RightOut;

            if (RunFlag == 0)                                   /* 停车 */
            {                
                LeftOut  = 0;
                RightOut = 0;
            }
            else if (RunFlag == 1)                              /* 直行 */
            {
								LeftOut  = left_pwm;
                RightOut = right_pwm;
//				xunji(left_pwm, right_pwm, &LeftOut, &RightOut);
                xunji_PID(left_pwm, right_pwm, &LeftOut, &RightOut);							
            }
            else if (RunFlag == 2)                              /* 循迹 */
            {
                xunji_PID(left_pwm, right_pwm, &LeftOut, &RightOut);
            }

            Motor_SpeedOut(LeftOut, RightOut);  /* 速度闭环→电机输出 */
        }

        /* ===== 50Hz: 按键处理 ===== */
        if (count2 >= 500)
        {
            count2 = 0;
            uint8_t Numm = Key_GetNum();
					if (Numm == 1)																							//板子上的按键
            {
                RunFlag++;
                if (RunFlag == 3) RunFlag = 0;
            }
            if (Numm == 2)																					  //PB0
            {

            }
            if (Numm == 3)																						//PB1
            {

            }

        }
        break;
    default: break;
    }
}

void TimerA1_init(void) {
    NVIC_ClearPendingIRQ(TimerA1_INST_INT_IRQN);
    NVIC_EnableIRQ(TimerA1_INST_INT_IRQN);
}
