/**
 * bsp_tb6612.c — TB6612电机驱动 + 速度闭环
 *
 * AO=左轮  BO=右轮
 *
 * 速度闭环原理:
 *   1. 读编码器(QEI_0/QEI_1) → 实测速度(脉冲/10ms)
 *   2. 前馈 = target × FF  → 基准PWM
 *   3. PI修正 = PID(target − actual) → 误差补偿
 *   4. PWM = 前馈 + PI → 限幅 ±999 → AO/BO输出
 */
#include "tb6612.h"
#include "PID.h"

/* ===== 编码器方向: 读负数的改1 ===== */
#define ENCODER_L_INVERT  1
#define ENCODER_R_INVERT  0

/* ===== PI参数 & 速度变量(main.c定义) ===== */
float Speed_FF = 0.0f;        													//前馈系数
float Speed_Kp = 3.0f;																	//速度环KP
float Speed_Ki = 0.65f;																	//速度环KI
extern int32_t left_speed, right_speed;   							//实际速度（编码器测的）

static pid_type_def pid_l, pid_r;     									//左右轮各自的PID结构体（存误差，积分）
static uint32_t last_l, last_r;       									//存上一次的编码器计数值（用来算10ms差量）

/* =================== 底层PWM控制 =================== */

void TB6612_Motor_Stop(void)   													//AIN1_OUT(1); AIN2_OUT(1);的时候电机不转，这个是停车代码
{
    AIN1_OUT(1); AIN2_OUT(1);
    BIN1_OUT(1); BIN2_OUT(1);
    DL_TimerA_setCaptureCompareValue(PWM_0_INST, 0, GPIO_PWM_0_C0_IDX);  // 左 PWM 占空比 = 0
    DL_TimerA_setCaptureCompareValue(PWM_0_INST, 0, GPIO_PWM_0_C1_IDX);	 // 右 PWM 占空比 = 0
}

void AO_Control(uint8_t enable, int32_t speed)   				//接AO的电机控制
{
	if(speed > 999) speed = 999;               						//输出限幅（这里是输出pwm所以是999）
    if(speed < -999) speed = -999;

    if(enable == 0 || speed == 0) {											//enable=0和speed=0就是速度为零
        AIN1_OUT(1); AIN2_OUT(1);
        DL_TimerA_setCaptureCompareValue(PWM_0_INST, 0, GPIO_PWM_0_C0_IDX);
        return;
    }
    if(speed > 0) { AIN1_OUT(0); AIN2_OUT(1); }   			//判断speed正负正反转
    else          { AIN1_OUT(1); AIN2_OUT(0); }    

    DL_TimerA_setCaptureCompareValue(PWM_0_INST, speed > 0 ? speed : -speed, GPIO_PWM_0_C0_IDX);
}

void BO_Control(uint8_t enable, int32_t speed)   				//接AO的电机控制
{
    if(speed > 999) speed = 999;												//输出限幅（这里是输出pwm所以是999）
    if(speed < -999) speed = -999;

    if(enable == 0 || speed == 0) {											//enable=0和speed=0就是速度为零
        BIN1_OUT(1); BIN2_OUT(1);
        DL_TimerA_setCaptureCompareValue(PWM_0_INST, 0, GPIO_PWM_0_C1_IDX);				// PWM 清零
        return;
    }
    if(speed > 0) { BIN1_OUT(0); BIN2_OUT(1); }					//判断speed正负正反转
    else          { BIN1_OUT(1); BIN2_OUT(0); }

    DL_TimerA_setCaptureCompareValue(PWM_0_INST, speed > 0 ? speed : -speed, GPIO_PWM_0_C1_IDX);// 取 speed 的绝对值  
}

/* =================== 速度闭环 =================== */

void Motor_Init(void)
{
    float p[3] = {Speed_Kp, Speed_Ki, 0.0f};    				// Kd 填 0，只用 PI 不用微分
    PID_init(&pid_l, PID_POSITION, p, 999, 900);				// 左轮 PID
    PID_init(&pid_r, PID_POSITION, p, 999, 900);				// 右轮 PID
    last_l = DL_TimerG_getTimerCount(QEI_1_INST); 				 /* 读左轮编码器的值QEI_1 */
    last_r = DL_TimerG_getTimerCount(QEI_0_INST); 				 /* 读右轮编码器的值QEI_1 */
		//提前采样编码器的当前值，否则第一次 Motor_SpeedOut 调用时 cur - last 会得到一个巨大的假值，电机会抽搐一下。
}

/*
 * Motor_SpeedOut — 目标速度 → 编码器PI → PWM输出(100Hz调用)
 *
 * l_target/r_target: 目标速度(编码器脉冲/10ms)
 * 内部完成: 读编码器→算实测速度→PI控制→AO/BO输出
 */
void Motor_SpeedOut(int32_t l_target, int32_t r_target)
{
    uint32_t cur_l = DL_TimerG_getTimerCount(QEI_1_INST);// 读左编码器当前值
    uint32_t cur_r = DL_TimerG_getTimerCount(QEI_0_INST);// 读右编码器当前值
    /* 1. 实测速度 = 本次-上次 (int16_t自动处理16位溢出) */
    left_speed  = (int16_t)(cur_l - last_l);						// 本次 - 上次 = 10ms 内走的脉冲数
    right_speed = (int16_t)(cur_r - last_r);
    last_l = cur_l;																			// 更新基准点
    last_r = cur_r;
	
	
// 如果接线反了，正转测到负数，取反就正了
//#if 是编译期判断，不是运行期。
//ENCODER_L_INVERT 在第 16 行定为 1，所以编译器编译时会把 left_speed = -left_speed; 这行直接编进去；
//ENCODER_R_INVERT 是 0，右轮那行直接不编译。这是零开销的方向修正。

#if ENCODER_L_INVERT
    left_speed = -left_speed;
#endif
#if ENCODER_R_INVERT
    right_speed = -right_speed;
#endif


    /* 2. 同步Debug调参 & 目标为零时清掉积分防止残留 */
    pid_l.Kp = Speed_Kp;  pid_l.Ki = Speed_Ki;
    pid_r.Kp = Speed_Kp;  pid_r.Ki = Speed_Ki;

//参数：l_target 和 r_target 来自 Timer_task.c，是 xunji 函数计算出的左右轮目标速度

    if (l_target == 0) PID_clear(&pid_l);    // 左侧目标=0 → 清掉积分，防止残留
    if (r_target == 0) PID_clear(&pid_r);		 // 右侧目标=0 → 清掉积分，防止残留

    /* 3. 前馈 + PI修正 → PWM */
    int32_t ff_l   = (int32_t)((float)l_target * Speed_FF); // 前馈：目标×系数 = 基准PWM
    int32_t ff_r   = (int32_t)((float)r_target * Speed_FF);
    int32_t corr_l = (int32_t)PID_calc(&pid_l, (float)left_speed,  (float)l_target);
    int32_t corr_r = (int32_t)PID_calc(&pid_r, (float)right_speed, (float)r_target);

    int32_t pwm_l = ff_l + corr_l;
    int32_t pwm_r = ff_r + corr_r;

    if      (pwm_l >  999) pwm_l =  999;
    else if (pwm_l < -999) pwm_l = -999;
    if      (pwm_r >  999) pwm_r =  999;
    else if (pwm_r < -999) pwm_r = -999;

    /* 4. 输出到电机 */
    AO_Control(1, pwm_l);
    BO_Control(1, pwm_r);
}
