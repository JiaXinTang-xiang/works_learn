//
// Created by Admin on 2025/12/19.
//

#ifndef __PID_H
#define __PID_H
#include "bsp.h"
#define fp32 float

enum PID_MODE
{
    PID_POSITION = 0,
    PID_DELTA
};

typedef struct
{
  uint8_t mode; //PID模式:位置式或增量式

  //PID 三参数
  fp32 Kp;
  fp32 Ki;
  fp32 Kd;

  fp32 max_out;  //最大输出
  fp32 max_iout; //最大积分输出

  //输入输出
  fp32 set;	   //设定值
  fp32 fdb;	   //反馈值

  //PID各项输出
  fp32 out;	   //最终输出
  fp32 Pout;	   //比例
  fp32 Iout;	   //积分
  fp32 Dout;	   //微分

  //历史数据缓冲区（实现滤波和差分）
  fp32 Dbuf[3];  //微分缓冲区 0最新 1上一次 2上上次
  fp32 error[3]; //误差缓冲区 0最新 1上一次 2上上次

} pid_type_def;
/**
  * @brief          pid struct data init
  * @param[out]     pid: PID struct data point
  * @param[in]      mode: PID_POSITION: normal pid
  *                       PID_DELTA: delta pid
  * @param[in]      PID: 0: kp, 1: ki, 2:kd
  * @param[in]      max_out: pid max out
  * @param[in]      max_iout: pid max iout
  * @retval         none
  */
/**
  * @brief          pid struct data init
  * @param[out]     pid: PID结构数据指针
  * @param[in]      mode: PID_POSITION:普通PID
  *                       PID_DELTA: 差分PID
  * @param[in]      PID: 0: kp, 1: ki, 2:kd
  * @param[in]      max_out: pid最大输出
  * @param[in]      max_iout: pid最大积分输出
  * @retval         none
  */
extern void PID_init(pid_type_def *pid, uint8_t mode, const fp32 PID[3], fp32 max_out, fp32 max_iout);

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
extern fp32 PID_calc(pid_type_def *pid, fp32 ref, fp32 set);

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
extern void PID_clear(pid_type_def *pid);

/**
  * @brief          平衡车直立环pid
  * @param[Angle]   Pitch角度
  * @param[Angle]   角速度
  * @param[Angle]   机械中值
  * @param[Angle]   Kp
  * @param[Angle]   Kd
  * @param[Angle]   输出限幅
  * @retval         pid输出
  */
fp32 Vertical(float Angle, float Gyro_y, float Med,float Kp, float Kd, float max_out,int OutOffset); 
int Velocity(int Target,int encoder_left,int encoder_right,float Velocity_Kp,float Velocity_Ki);
int Turn(int gyro_Z, int actual_speed_diff, int target_speed_diff, 
         float Turn_Kp, float Turn_Ki, float Turn_Kd);
	
#endif 
