#include "PID.h"


extern uint8_t RunFlag;
#define LimitMax(input, max)   \
    {                          \
        if (input > max)       \
        {                      \
            input = max;       \
        }                      \
        else if (input < -max) \
        {                      \
            input = -max;      \
        }                      \
    }

/**
  * @brief          pid struct data init
  * @param[out]     pid: PID struct data point
  * @param[in]      mode: PID_POSITION: normal pid
  *                 PID_DELTA: delta pid
  * @param[in]      PID: 0: kp, 1: ki, 2:kd
  * @param[in]      max_out: pid max out
  * @param[in]      max_iout: pid max iout
  * @retval         none
  */
/**
  * @brief          pid struct data init
  * @param[out]     pid: PID结构数据指针
  * @param[in]      mode: PID_POSITION:普通PID
  *                 PID_DELTA: 差分PID
  * @param[in]      PID: 0: kp, 1: ki, 2:kd
  * @param[in]      max_out: pid最大输出
  * @param[in]      max_iout: pid最大积分输出
  * @retval         none
  */
void PID_init(pid_type_def *pid, uint8_t mode, const fp32 PID[3], fp32 max_out, fp32 max_iout)
{
    if (pid == NULL || PID == NULL)
    {
        return;
    }
    pid->mode = mode;
    pid->Kp = PID[0];
    pid->Ki = PID[1];
    pid->Kd = PID[2];
    pid->max_out = max_out;
    pid->max_iout = max_iout;
    pid->Dbuf[0] = pid->Dbuf[1] = pid->Dbuf[2] = 0.0f;
    pid->error[0] = pid->error[1] = pid->error[2] = pid->Pout = pid->Iout = pid->Dout = pid->out = 0.0f;
}

/**
  * @brief          pid calculate
  * @param[out]     pid: PID struct data point
  * @param[in]      ref: feedback data
  * @param[in]      set: set point
  * @retval         pid out
  */
/**
  * @brief          pid计算
  * @param[out]     pid: PID结构数据指针
  * @param[in]      ref: 反馈数据
  * @param[in]      set: 设定值
  * @retval         pid输出
  */
fp32 PID_calc(pid_type_def *pid, fp32 ref, fp32 set)
{
    if (pid == NULL)
    {
        return 0.0f;
    }

    pid->error[2] = pid->error[1];
    pid->error[1] = pid->error[0];
    pid->set = set;
    pid->fdb = ref;
    pid->error[0] = set - ref;
    if (pid->mode == PID_POSITION)
    {
        pid->Pout = pid->Kp * pid->error[0];
        pid->Iout += pid->Ki * pid->error[0];
        pid->Dbuf[2] = pid->Dbuf[1];
        pid->Dbuf[1] = pid->Dbuf[0];
        pid->Dbuf[0] = (pid->error[0] - pid->error[1]);
        pid->Dout = pid->Kd * pid->Dbuf[0];
        LimitMax(pid->Iout, pid->max_iout);
        pid->out = pid->Pout + pid->Iout + pid->Dout;
        LimitMax(pid->out, pid->max_out);
    }
    else if (pid->mode == PID_DELTA)
    {
        pid->Pout = pid->Kp * (pid->error[0] - pid->error[1]);
        pid->Iout = pid->Ki * pid->error[0];
        pid->Dbuf[2] = pid->Dbuf[1];
        pid->Dbuf[1] = pid->Dbuf[0];
        pid->Dbuf[0] = (pid->error[0] - 2.0f * pid->error[1] + pid->error[2]);
        pid->Dout = pid->Kd * pid->Dbuf[0];
        pid->out += pid->Pout + pid->Iout + pid->Dout;
        LimitMax(pid->out, pid->max_out);
    }
    return pid->out;
}

/**
  * @brief          pid out clear
  * @param[out]     pid: PID struct data point
  * @retval         none
  */
/**
  * @brief          pid 输出清除
  * @param[out]     pid: PID结构数据指针
  * @retval         none
  */
void PID_clear(pid_type_def *pid)
{
    if (pid == NULL)
    {
        return;
    }

    pid->error[0] = pid->error[1] = pid->error[2] = 0.0f;
    pid->Dbuf[0] = pid->Dbuf[1] = pid->Dbuf[2] = 0.0f;
    pid->out = pid->Pout = pid->Iout = pid->Dout = 0.0f;
    pid->fdb = pid->set = 0.0f;
}

/**
  * @brief           平衡车直立环pid
  * @param Angle     Pitch角度（前倾为正）
  * @param Gyro_y    俯仰角速度（前倾方向为负，需要与角度匹配）
  * @param Med       机械中值（静止时的角度）
  * @param Kp        比例系数
  * @param Kd        微分系数
  * @param max_out   输出限幅
  * @retval          pid输出
  */
fp32 Vertical(float Angle, float Gyro_y, float Med,float Kp, float Kd, float max_out,int OutOffset) 
{
    float angle_error = Angle - Med;
    float pwm_out;
    
    // PD 控制
    pwm_out = Kp * angle_error + Kd * Gyro_y;
    
	//	//增加优化 -- 输出偏移
	if(pwm_out > 0 ){pwm_out += OutOffset;}
	else if(pwm_out < 0 ){pwm_out -= OutOffset;}
	
	
    // 输出限幅
    if(pwm_out > max_out) pwm_out = max_out;
    if(pwm_out < -max_out) pwm_out = -max_out;
    
    return pwm_out;
}


extern float ypr[3]; 
/*****************  
速度环PI控制器：Kp*Ek + Ki*∫Ek dt
增加：积分分离、积分衰减、输出限幅
******************/
int Velocity(int Target, int encoder_left, int encoder_right, float Velocity_Kp, float Velocity_Ki)
{
    static int PWM_out;
    static float Encoder_Err, Encoder_S, EnC_Err_Lowout, EnC_Err_Lowout_last;
    float a = 0.7f;
    
    // 1. 计算速度偏差
    Encoder_Err = (float)((encoder_left + encoder_right) - Target);
    
    // 2. 低通滤波
    EnC_Err_Lowout = (1-a) * Encoder_Err + a * EnC_Err_Lowout_last;
    EnC_Err_Lowout_last = EnC_Err_Lowout;
    
    // 3. 积分（位移）
    Encoder_S += EnC_Err_Lowout;
    
    // 4. 积分限幅（减小到合适的值）
    if(Encoder_S > 5000) Encoder_S = 5000;      // 3000 → 300
    if(Encoder_S < -5000) Encoder_S = -5000;
    
    // 5. 积分衰减（防止振荡的关键！）
    Encoder_S = Encoder_S * 0.998f;
    
    // 6. 停车或角度过大时清零积分
    if(RunFlag == 0 || ypr[2] > 45 || ypr[2]< -45) {
        Encoder_S = 0;
        EnC_Err_Lowout = 0;
        EnC_Err_Lowout_last = 0;
    }
    
    // 7. PI计算
    PWM_out = (int)(Velocity_Kp * EnC_Err_Lowout + Velocity_Ki * Encoder_S);
    
    // 8. 输出限幅
    if(PWM_out > 50) PWM_out = 50;
    if(PWM_out < -50) PWM_out = -50;
    
    return PWM_out;
}


/*****************  
转向环：速度差PID控制器（P：速度差误差，I：速度差误差积分，D：角速度阻尼）
输入：左右轮速度差，目标转向速度差
******************/
int Turn(int gyro_Z, int actual_speed_diff, int target_speed_diff, 
         float Turn_Kp, float Turn_Ki, float Turn_Kd)
{
    static int PWM_out;
    static float SpeedDiff_Err;
    static float SpeedDiff_Integral = 0;
    static float SpeedDiff_Err_Filtered = 0;
    static float SpeedDiff_Err_Last = 0;
    float a = 0.3f;  // 低通滤波系数
    
    // 1. 计算速度差误差（目标 - 实际）
    SpeedDiff_Err = (float)(target_speed_diff - actual_speed_diff);
    
    // 2. 低通滤波（滤除噪声）
    SpeedDiff_Err_Filtered = (1-a) * SpeedDiff_Err + a * SpeedDiff_Err_Last;
    SpeedDiff_Err_Last = SpeedDiff_Err_Filtered;
    
    // 3. 积分分离：误差大时停止积分
    if(fabsf(SpeedDiff_Err_Filtered) < 50) {  // 误差小于50时积分
        SpeedDiff_Integral += SpeedDiff_Err_Filtered * 0.05f;  // dt=0.05s
    }
    
    // 4. 积分限幅
    if(SpeedDiff_Integral > 500) SpeedDiff_Integral = 500;
    if(SpeedDiff_Integral < -500) SpeedDiff_Integral = -500;
    
    // 5. 积分衰减（防止饱和）
    SpeedDiff_Integral = SpeedDiff_Integral * 0.998f;
    
    // 6. 停车时清积分（需要外部RunFlag标志）
    if(RunFlag == 0) {
        SpeedDiff_Integral = 0;
        SpeedDiff_Err_Last = 0;
        SpeedDiff_Err_Filtered = 0;
    }
    
    // 7. PID计算：P*速度差误差 + I*积分 + D*角速度（阻尼）
    PWM_out = (int)(Turn_Kp * SpeedDiff_Err_Filtered 
                  + Turn_Ki * SpeedDiff_Integral 
                  + Turn_Kd * gyro_Z);
    
    // 8. 输出限幅（控制转向电机的PWM差值）
    if(PWM_out > 500) PWM_out = 500;
    if(PWM_out < -500) PWM_out = -500;
    
    return PWM_out;
}