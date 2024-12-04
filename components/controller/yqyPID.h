#ifndef __YQY_PID_H
#define __YQY_PID_H

#include "main.h"

typedef struct
{
	float kp,ki,kd;
	float error,target,lastMeasure;
	float pout,iout,dout,out;
	
}yqyPid_t;

extern yqyPid_t YQY_PitchMotor,YQY_PitchMotorAng;
extern yqyPid_t YQY_YawMotor,YQY_YawMotorAng;

float YQY_Fliter(float Input);
float YQY_FliterYaw(float Input);
void YQY_PID_Init(yqyPid_t* YQYpid);
float YQY_PID_Cal(yqyPid_t* YQYpid,float measure);


#endif      //__YQY_PID_H
