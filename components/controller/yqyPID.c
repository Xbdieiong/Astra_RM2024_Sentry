#include "yqyPID.h"

//大疆代码太繁琐了，自己写个pid
#define YQY_I_SEP		0							//积分分离
#define YQY_I_MAX		6000						//积分限幅
#define YQY_OUT_MAX	24000						//输出限幅

yqyPid_t YQY_PitchMotor,YQY_PitchMotorAng;
yqyPid_t YQY_YawMotor,YQY_YawMotorAng;

#define LimitMax(input, max)    \
{                               \
    if (input > max)            \
    {                           \
        input = max;            \
    }                           \
    else if (input < -max)      \
    {                           \
        input = -max;           \
    }                           \
}


void YQY_PID_Init(yqyPid_t* YQYpid)
{
	YQYpid -> kp = 6;
	YQYpid -> ki = 0.5;
	YQYpid -> kd = 0.0;
}
float YQY_Fliter(float Input)
{
  static float output, lastOutput;
  output = Input * 0.3 + lastOutput * 0.7;
  lastOutput = output;
  return output;
}
float YQY_FliterYaw(float Input)
{
  static float output, lastOutput;
  output = Input * 0.3 + lastOutput * 0.7;
  lastOutput = output;
  return output;
}
float YQY_PID_Cal(yqyPid_t* YQYpid, float measure)
{
	YQYpid->error = YQYpid->target - measure;
	
	YQYpid->pout = YQYpid->error * YQYpid->kp;
	if((YQYpid->error > YQY_I_SEP) || (YQYpid->error < -YQY_I_SEP))
		YQYpid->iout += YQYpid->error * YQYpid->ki;
	YQYpid->dout = (measure - YQYpid->lastMeasure) * YQYpid->kd;
	YQYpid->out = YQYpid->pout + YQYpid->iout + YQYpid->dout;
	
	LimitMax(YQYpid->iout, YQY_I_MAX);		//积分限幅
	LimitMax(YQYpid->out, YQY_OUT_MAX);		//输出限幅
	
	YQYpid->lastMeasure = measure;				//更新上次测量值
  return YQYpid->out;
}

