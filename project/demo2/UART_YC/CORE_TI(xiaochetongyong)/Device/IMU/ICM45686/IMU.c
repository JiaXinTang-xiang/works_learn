
//////////////////////////////////// LWJK MCU Team //////////////////////////////////////

#include "IMU.h"
#include "inv_imu_driver.h"
#include <stdio.h>
// #include "eeprom.h"

/* XYZ structure */

/* Accel: north direction accel in accel-meter frame */
/* Accel: west direction accel in accel-meter frame */
xyz_f_t north, west;
volatile float exInt, eyInt, ezInt;       // integral error
volatile float q0, q1, q2, q3;            // quaternion
volatile float integralFBhand, handdiff;
volatile uint32_t lastUpdate, now;         // integration period calc, unit us
volatile float yaw[5] = {0, 0, 0, 0, 0};  // yaw filter buffer
int16_t Ax_offset = 0, Ay_offset = 0;
float TTangles_gyro[7];                    // TT-filtered angles

float Angle_Final[3];                      // X-axis tilt angle
float Kp = 10.0f;

extern volatile uint32_t nowtime;
void MadgwickAHRSupdate(float gx, float gy, float gz, float ax, float ay, float az, float mx, float my, float mz);

extern int bsp_IcmGetRawData(float accel_mg[3], float gyro_dps[3], float *temp_degc);

// Fast inverse square-root
/***************************** Implementation *****************************************
 * Function:    float invSqrt(float x)
 * Description: Fast computation of 1/Sqrt(x)
 * Input:       Value to compute
 * Output:      Result
 *************************************************************************************/
float invSqrt1(float x)
{
	float halfx = 0.5f * x;
	float y = x;
	long i = *(long *)&y;
	i = 0x5f3759df - (i >> 1);
	y = *(float *)&i;
	y = y * (1.5f - (halfx * y * y));
	return y;
}

extern int setup_imu(int use_ln, int accel_en, int gyro_en);

/***************************** Implementation *****************************************
 * Function:    void IMU_init(void)
 * Description: Initialize IMU sensor
 *              Initialize sensor parameters
 *              Initialize quaternion
 *              Set control parameters
 *              Record system time
 * Input:       None
 * Output:      None
 *************************************************************************************/
void IMU_init(void)
{
	// while(!ICM_Init());    // Initialize ICM42688
	if (0x00 == setup_imu(1, 1, 1))
	{
		// Initial_Timer3();
		//  initialize quaternion
		q0 = 1.0f;               // initial quaternion
		q1 = 0.0f;
		q2 = 0.0f;
		q3 = 0.0f;
		exInt = 0.0;
		eyInt = 0.0;
		ezInt = 0.0;
		lastUpdate = nowtime;    // record time
		now = nowtime;
		return;
	}
	printf("IMU ERROR!!\r\n");
}

float gyro_offset[3] = {0};
int CalCount = 0;
static double Gyro_fill[3][300];
static double Gyro_total[3];
static double sqrGyro_total[3];
static int GyroinitFlag = 0;
static int GyroCount = 0;

// Variance formula: S^2 = (X1^2 + X2^2 + ... + Xn^2)/n - X_avg^2
// Function:    calVariance
// Description: Compute variance
// Input:       data[]  --- data buffer for variance calculation
//              length  --- data length
// Output:      sqrResult[] --- variance result
//              avgResult[] --- mean result
void calGyroVariance(float data[], int length, float sqrResult[], float avgResult[])
{
	int i;
	double tmplen;
	if (GyroinitFlag == 0)
	{
		for (i = 0; i < 3; i++)
		{
			Gyro_fill[i][GyroCount] = data[i];
			Gyro_total[i] += data[i];
			sqrGyro_total[i] += data[i] * data[i];
			sqrResult[i] = 100;
			avgResult[i] = 0;
		}
	}
	else
	{
		for (i = 0; i < 3; i++)
		{
			Gyro_total[i] -= Gyro_fill[i][GyroCount];
			sqrGyro_total[i] -= Gyro_fill[i][GyroCount] * Gyro_fill[i][GyroCount];
			Gyro_fill[i][GyroCount] = data[i];
			Gyro_total[i] += Gyro_fill[i][GyroCount];
			sqrGyro_total[i] += Gyro_fill[i][GyroCount] * Gyro_fill[i][GyroCount];
		}
	}
	GyroCount++;
	if (GyroCount >= length)
	{
		GyroCount = 0;
		GyroinitFlag = 1;
		Kp = 0.5f;
	}
	if (GyroinitFlag == 0)
	{
		return;
	}
	tmplen = length;
	for (i = 0; i < 3; i++)
	{
		avgResult[i] = (float)(Gyro_total[i] / tmplen);
		sqrResult[i] = (float)((sqrGyro_total[i] - Gyro_total[i] * Gyro_total[i] / tmplen) / tmplen);
	}
}

/***************************** Implementation *****************************************
 * Function:    void IMU_getValues(float * values)
 * Description: Get current accel, gyro, temperature values
 * Input:       Pointer to result array
 * Output:      None
 *************************************************************************************/
void IMU_getValues(float *values)
{
	float accgyroval[7];
	//	icm42688RealData_t accval;
	//	icm42688RealData_t gyroval;

	float sqrResult_gyro[3];
	float avgResult_gyro[3];

	// Get current accel and gyro ADC values
	bsp_IcmGetRawData(accgyroval, &accgyroval[3], &accgyroval[6]);
	TTangles_gyro[0] = accgyroval[0];
	TTangles_gyro[1] = accgyroval[1];
	TTangles_gyro[2] = accgyroval[2];
	TTangles_gyro[3] = accgyroval[3];
	TTangles_gyro[4] = accgyroval[4];
	TTangles_gyro[5] = accgyroval[5];
	TTangles_gyro[6] = accgyroval[6];

	calGyroVariance(&TTangles_gyro[3], 100, sqrResult_gyro, avgResult_gyro);

	// Only allow dynamic calibration if power-on calibration hasn't been done
	if (sqrResult_gyro[0] < 0.02f && sqrResult_gyro[1] < 0.02f && sqrResult_gyro[2] < 0.02f && CalCount >= 99)
	{
		gyro_offset[0] = avgResult_gyro[0];
		gyro_offset[1] = avgResult_gyro[1];
		gyro_offset[2] = avgResult_gyro[2];
		exInt = 0;
		eyInt = 0;
		ezInt = 0;
		CalCount = 0;
	}
	else if (CalCount < 100)
	{
		CalCount++;
	}
	values[0] = accgyroval[0];
	values[1] = accgyroval[1];
	values[2] = accgyroval[2];
	values[3] = accgyroval[3] - gyro_offset[0];
	values[4] = accgyroval[4] - gyro_offset[1];
	values[5] = accgyroval[5] - gyro_offset[2];

	// Gyro range converted to 1000 dps, 32.8 corresponds to 1 dps
}

/***************************** Implementation *****************************************
 * Function:    void IMU_AHRSupdate
 * Description: Run AHRS algorithm, update quaternion
 * Input:       Current sensor values
 * Output:      None
 *************************************************************************************/
// proportional gain governs rate of convergence to accelerometer/magnetometer
#define Ki 0.001f // integral gain governs rate of convergence of gyroscope biases

void IMU_AHRSupdate(float gx, float gy, float gz, float ax, float ay, float az, float mx, float my, float mz)
{
	float norm;
	// float hx, hy, hz, bx, bz;
	float vx, vy, vz; //, wx, wy, wz;
	float ex, ey, ez, halfT;
	float tempq0, tempq1, tempq2, tempq3;

	// Pre-compute quaternion products
	float q0q0 = q0 * q0;
	float q0q1 = q0 * q1;
	float q0q2 = q0 * q2;
	float q0q3 = q0 * q3;
	float q1q1 = q1 * q1;
	float q1q2 = q1 * q2;
	float q1q3 = q1 * q3;
	float q2q2 = q2 * q2;
	float q2q3 = q2 * q3;
	float q3q3 = q3 * q3;

	////================================================================================
	//	//20160323v0.4.6
	//	// Variables for extended Kalman filter
	//	// Placeholder for extended EKF research
	//	static int s_InitTickCount=0;
	//	float an[3]={0,0,0};
	//	float Cb2n[3*3]={0};
	//
	////================================================================================
	//

	now = nowtime;            // get current time
	if (now < lastUpdate)
	{                         // timer overflow handling
		halfT = ((float)(now + (0xffff - lastUpdate)) / 20000.0f);
	}
	else
	{
		halfT = ((float)(now - lastUpdate) / 20000.0f);
	}
	lastUpdate = now;         // update time

	norm = invSqrt1(ax * ax + ay * ay + az * az);
	ax = ax * norm;
	ay = ay * norm;
	az = az * norm;
	// Convert accelerometer 3D vector to unit vector

	norm = invSqrt1(mx * mx + my * my + mz * mz);
	mx = mx * norm;
	my = my * norm;
	mz = mz * norm;

	/*
	Rotation matrix from quaternion: elements of direction cosine matrix.
	Based on Euler angle definition, transform from body frame to navigation frame.
	vx/vy/vz are the current Euler angle (quaternion-based) gravity vector
	in the body coordinate frame.
	*/

	// estimated direction of gravity and flux (v and w)
	vx = 2 * (q1q3 - q0q2);
	vy = 2 * (q0q1 + q2q3);
	vz = q0q0 - q1q1 - q2q2 + q3q3;

	/* Acceleration in north direction in accelerometer X axis */
	north.x = 1 - 2 * (q3 * q3 + q2 * q2);
	/* Acceleration in north direction in accelerometer Y axis */
	north.y = 2 * (-q0 * q3 + q1 * q2);
	/* Acceleration in north direction in accelerometer Z axis */
	north.z = 2 * (+q0 * q2 - q1 * q3);
	/* Acceleration in west direction in accelerometer X axis */
	west.x = 2 * (+q0 * q3 + q1 * q2);
	/* Acceleration in west direction in accelerometer Y axis */
	west.y = 1 - 2 * (q3 * q3 + q1 * q1);
	/* Acceleration in west direction in accelerometer Z axis */
	west.z = 2 * (-q0 * q1 + q2 * q3);

	/*
	axyz: accelerometer readings in body frame (actual measured gravity).
	vxyz: gravity vector estimated from gyro integration (predicted attitude).
	exyz: cross product error between measured and estimated gravity.
	This error represents the gyro drift error in body frame coordinates.
	*/

	// error is sum of cross product between reference direction of fields and direction measured by sensors
	ex = (ay * vz - az * vy);
	ey = (az * vx - ax * vz);
	ez = (ax * vy - ay * vx);

	if (ex != 0.0f && ey != 0.0f && ez != 0.0f)
	{
		exInt = exInt + ex * Ki * halfT;
		eyInt = eyInt + ey * Ki * halfT;
		ezInt = ezInt + ez * Ki * halfT;

		// PI controller corrects gyro bias
		gx = gx + Kp * ex + exInt;
		gy = gy + Kp * ey + eyInt;
		gz = gz + Kp * ez + ezInt;
	}

	// Quaternion differential equation
	tempq0 = q0 + (-q1 * gx - q2 * gy - q3 * gz) * halfT;
	tempq1 = q1 + (q0 * gx + q2 * gz - q3 * gy) * halfT;
	tempq2 = q2 + (q0 * gy - q1 * gz + q3 * gx) * halfT;
	tempq3 = q3 + (q0 * gz + q1 * gy - q2 * gx) * halfT;

	// Quaternion normalization
	norm = invSqrt1(tempq0 * tempq0 + tempq1 * tempq1 + tempq2 * tempq2 + tempq3 * tempq3);
	q0 = tempq0 * norm;
	q1 = tempq1 * norm;
	q2 = tempq2 * norm;
	q3 = tempq3 * norm;
}

/***************************** Implementation *****************************************
 * Function:    void IMU_getQ(float * q)
 * Description: Retrieve quaternion, return current quaternion values
 * Input:       Pointer to quaternion result array
 * Output:      None
 *************************************************************************************/
float mygetqval[9];  // buffer for transfer variables
void IMU_getQ(float *q)
{
	IMU_getValues(mygetqval);
	// Convert gyro values to radians per second
	// Accelerometer and magnetometer are already in correct units (no ADC conversion needed)
	IMU_AHRSupdate(mygetqval[3] * M_PI / 180, mygetqval[4] * M_PI / 180, mygetqval[5] * M_PI / 180,
				   mygetqval[0] / 1000.0f, mygetqval[1] / 1000.0f, mygetqval[2] / 1000.0f,
				   mygetqval[6], mygetqval[7], mygetqval[8]);

	q[0] = q0;  // return current values
	q[1] = q1;
	q[2] = q2;
	q[3] = q3;
}

/***************************** Implementation *****************************************
 * Function:    void IMU_getYawPitchRoll(float * angles)
 * Description: Compute quaternion, return current attitude angles
 * Input:       Pointer to attitude angle result array
 * Output:      None
 *************************************************************************************/
void IMU_getYawPitchRoll(float *angles)
{
	float q[4];                                  // quaternion
	volatile float gx = 0.0, gy = 0.0, gz = 0.0; // estimated gravity direction
	IMU_getQ(q);                                 // get full quaternion

	angles[0] = -atan2(2 * q[1] * q[2] + 2 * q[0] * q[3], -2 * q[2] * q[2] - 2 * q[3] * q[3] + 1) * 180 / M_PI;  // yaw
	angles[1] = -asin(-2 * q[1] * q[3] + 2 * q[0] * q[2]) * 180 / M_PI;                                             // pitch
	angles[2] = atan2(2 * q[2] * q[3] + 2 * q[0] * q[1], -2 * q[1] * q[1] - 2 * q[2] * q[2] + 1) * 180 / M_PI;     // roll
	// if(angles[0]<0)angles[0]+=360.0f;  // convert -+180 to 0-360 range
}

void IMU_TT_getgyro(float *zsjganda)
{
	zsjganda[0] = TTangles_gyro[0];
	zsjganda[1] = TTangles_gyro[1];
	zsjganda[2] = TTangles_gyro[2];
	zsjganda[3] = TTangles_gyro[3];
	zsjganda[4] = TTangles_gyro[4];
	zsjganda[5] = TTangles_gyro[5];
	zsjganda[6] = TTangles_gyro[6];
}

void MPU6050_InitAng_Offset(void)
{
}
//------------------End of File----------------------------
