/**
  ****************************(C) COPYRIGHT 2019 DJI****************************
  * @file       can_receive.c/h
  * @brief      there is CAN interrupt function  to receive motor data,
  *             and CAN send function to send motor current to control motor.
  *             ???CAN??????,??????,CAN??????????????.
  * @note       
  * @history
  *  Version    Date            Author          Modification
  *  V1.0.0     Dec-26-2018     RM              1. done
  *  V1.1.0     Nov-11-2019     RM              1. support hal lib
  *
  @verbatim
  ==============================================================================

  ==============================================================================
  @endverbatim
  ****************************(C) COPYRIGHT 2019 DJI****************************
  */

#include "CAN_receive.h"

#include "cmsis_os.h"

#include "main.h"
#include "bsp_rng.h"


#include "detect_task.h"

extern CAN_HandleTypeDef hcan1;
extern CAN_HandleTypeDef hcan2;
//motor data read
#define get_motor_measure(ptr, data)                                    \
    {                                                                   \
        (ptr)->last_ecd = (ptr)->ecd;                                   \
        (ptr)->ecd = (uint16_t)((data)[0] << 8 | (data)[1]);            \
        (ptr)->speed_rpm = (uint16_t)((data)[2] << 8 | (data)[3]);      \
        (ptr)->given_current = (uint16_t)((data)[4] << 8 | (data)[5]);  \
        (ptr)->temperate = (data)[6];                                   \
    }
/*
motor data,  0:chassis motor1 3508;1:chassis motor3 3508;2:chassis motor3 3508;3:chassis motor4 3508;
4:yaw gimbal motor 6020;5:pitch gimbal motor 6020;6:trigger motor 2006;
????, 0:????1 3508??,  1:????2 3508??,2:????3 3508??,3:????4 3508??;
4:yaw???? 6020??; 5:pitch???? 6020??; 6:???? 2006??*/
motor_measure_t motor_chassis[6];

static CAN_TxHeaderTypeDef  gimbal_tx_message;
static uint8_t              gimbal_can_send_data[8];
static CAN_TxHeaderTypeDef  chassis_tx_message;
static uint8_t              chassis_can_send_data[8];
	
		
//ma 
 power_measure_t power_data;

static CAN_TxHeaderTypeDef trigger_tx_message;
static uint8_t             trigger_can_send_data[8];
static CAN_TxHeaderTypeDef  fric_tx_message;
static uint8_t              fric_can_send_data[8];
		
static motor_measure_t motor_fric[3];
static motor_measure_t motor_switch;
		
/**
  * @brief          hal CAN fifo call back, receive motor data
  * @param[in]      hcan, the point to CAN handle
  * @retval         none
  */
/**
  * @brief          hal?CAN????,??????
  * @param[in]      hcan:CAN????
  * @retval         none
  */
void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
    CAN_RxHeaderTypeDef rx_header;
	  CAN_RxHeaderTypeDef rx_header2;
    uint8_t rx_data[8];
	  uint8_t rx_data2[8];
	
	if(hcan==&hcan1)
	{
    HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &rx_header, rx_data);
    switch (rx_header.StdId)
    {
       
       	case 0x209:
				{
					get_motor_measure(&motor_chassis[4],rx_data);
				  break;
				}
				case 0x20a:                                         //Pitch��
				{
					get_motor_measure(&motor_chassis[5],rx_data);
				  break;
				}

				case 0x203:
				{
					get_motor_measure(&motor_fric[2],rx_data);
					break;
				}
				

        default:
        {
            break;
        }	
    }
	}
	if(hcan == &hcan2)
	{
		HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &rx_header2, rx_data2);
		switch(rx_header2.StdId)
		{
			case 0x211:
			{
				get_power_measure(&power_data,rx_data2);
				break;
			}
		  	case 0x201:
				{
					get_motor_measure(&motor_fric[0],rx_data2);
					break;
				}
				case 0x202:
				{
					get_motor_measure(&motor_fric[1],rx_data2);
					break;
				}
				case 0x203:
				{
					get_motor_measure(&motor_fric[2],rx_data2);
					break;
				}
				case 0x204:
				{
					get_motor_measure(&motor_switch,rx_data2);
					break;
				}
			default:
			break;
		}
	}
}

//��ó�������ʣ���ѹ     //ma 
void get_power_measure(power_measure_t *ptr,uint8_t *data1)
{
	uint16_t data2[4];	
	data2[0] = data1[1]<<8 | data1[0];
	data2[1] = data1[3]<<8 | data1[2];
	data2[2] = data1[5]<<8 | data1[4];
	data2[3] = data1[7]<<8 | data1[6];
	
	ptr->InputVot = (float)data2[0]/100.f;
	ptr->CapVot = (float)data2[1]/100.f;
	ptr->Input_Current = (float)data2[2]/100.f;
	ptr->Target_Power = (float)data2[3]/100.f;
}


//�������ݳ�繦������
void power_current_set( uint16_t new_power )
{
	uint32_t send_mail_box;

	CAN_TxHeaderTypeDef  power_tx_message;
	uint8_t              power_can_send_data[8];

	power_tx_message.StdId = 0x210;
	power_tx_message.IDE   = CAN_ID_STD;
	power_tx_message.RTR   = CAN_RTR_DATA;
	power_tx_message.DLC   = 0x02;
	power_can_send_data[0] = (uint8_t)(new_power >> 8 );
	power_can_send_data[1] = (uint8_t)(new_power &  0xFF);
//	power_can_send_data[2] = 0x00;
//	power_can_send_data[3] = (save_flg == 0x01);			

	HAL_CAN_AddTxMessage(&hcan2, &power_tx_message, power_can_send_data, &send_mail_box);
	
}




/**
  * @brief          send control current of motor (0x205, 0x206, 0x207, 0x208)
  * @param[in]      yaw: (0x205) 6020 motor control current, range [-30000,30000] 
  * @param[in]      pitch: (0x206) 6020 motor control current, range [-30000,30000]
  * @param[in]      shoot: (0x207) 2006 motor control current, range [-10000,10000]
  * @param[in]      rev: (0x208) reserve motor control current
  * @retval         none
  */
/**
  * @brief          ????????(0x205,0x206,0x207,0x208)
  * @param[in]      yaw: (0x205) 6020??????, ?? [-30000,30000]
  * @param[in]      pitch: (0x206) 6020??????, ?? [-30000,30000]
  * @param[in]      shoot: (0x207) 2006??????, ?? [-10000,10000]
  * @param[in]      rev: (0x208) ??,??????
  * @retval         none
  */
void CAN_cmd_gimbal(int16_t yaw, int16_t pitch, int16_t rev1)
{
    uint32_t send_mail_box;
    gimbal_tx_message.StdId = 0x2FF;
    gimbal_tx_message.IDE = CAN_ID_STD;
    gimbal_tx_message.RTR = CAN_RTR_DATA;
    gimbal_tx_message.DLC = 0x08;
    gimbal_can_send_data[0] = (yaw >> 8);
    gimbal_can_send_data[1] = yaw;
    gimbal_can_send_data[2] = (pitch >> 8);
    gimbal_can_send_data[3] = pitch;
    gimbal_can_send_data[4] = (rev1 >> 8);
    gimbal_can_send_data[5] = rev1;
//    gimbal_can_send_data[6] = (rev2 >> 8);
//    gimbal_can_send_data[7] = rev2;
    HAL_CAN_AddTxMessage(&hcan1, &gimbal_tx_message, gimbal_can_send_data, &send_mail_box);
}

/**
  * @brief          send CAN packet of ID 0x700, it will set chassis motor 3508 to quick ID setting
  * @param[in]      none
  * @retval         none
  */
/**
  * @brief          ??ID?0x700?CAN?,????3508????????ID
  * @param[in]      none
  * @retval         none
  */
void CAN_cmd_chassis_reset_ID(void)
{
    uint32_t send_mail_box;
    chassis_tx_message.StdId = 0x700;
    chassis_tx_message.IDE = CAN_ID_STD;
    chassis_tx_message.RTR = CAN_RTR_DATA;
    chassis_tx_message.DLC = 0x08;
    chassis_can_send_data[0] = 0;
    chassis_can_send_data[1] = 0;
    chassis_can_send_data[2] = 0;
    chassis_can_send_data[3] = 0;
    chassis_can_send_data[4] = 0;
    chassis_can_send_data[5] = 0;
    chassis_can_send_data[6] = 0;
    chassis_can_send_data[7] = 0;

    HAL_CAN_AddTxMessage(&CHASSIS_CAN, &chassis_tx_message, chassis_can_send_data, &send_mail_box);
}





/**
  * @brief          send control current of motor (0x201, 0x202, 0x203, 0x204)
  * @param[in]      motor1: (0x201) 3508 motor control current, range [-16384,16384] 
  * @param[in]      motor2: (0x202) 3508 motor control current, range [-16384,16384] 
  * @param[in]      motor3: (0x203) 3508 motor control current, range [-16384,16384] 
  * @param[in]      motor4: (0x204) 3508 motor control current, range [-16384,16384] 
  * @retval         none
  */
/**
  * @brief          ????????(0x201,0x202,0x203,0x204)
  * @param[in]      motor1: (0x201) 3508??????, ?? [-16384,16384]
  * @param[in]      motor2: (0x202) 3508??????, ?? [-16384,16384]
  * @param[in]      motor3: (0x203) 3508??????, ?? [-16384,16384]
  * @param[in]      motor4: (0x204) 3508??????, ?? [-16384,16384]
  * @retval         none
  */
void CAN_cmd_chassis(int16_t motor1, int16_t motor2, int16_t motor3, int16_t motor4)
{
    uint32_t send_mail_box;
    chassis_tx_message.StdId = CAN_CHASSIS_ALL_ID;
    chassis_tx_message.IDE = CAN_ID_STD;
    chassis_tx_message.RTR = CAN_RTR_DATA;
    chassis_tx_message.DLC = 0x08;
    chassis_can_send_data[0] = motor1 >> 8;
    chassis_can_send_data[1] = motor1;
    chassis_can_send_data[2] = motor2 >> 8;
    chassis_can_send_data[3] = motor2;
    chassis_can_send_data[4] = motor3 >> 8;
    chassis_can_send_data[5] = motor3;
    chassis_can_send_data[6] = motor4 >> 8;
    chassis_can_send_data[7] = motor4;

    HAL_CAN_AddTxMessage(&CHASSIS_CAN, &chassis_tx_message, chassis_can_send_data, &send_mail_box);
}

//ma
//Ħ���ֵ��
void CAN_cmd_fric(int16_t fric1, int16_t fric2, int16_t trigger, int16_t rev2)
{
    uint32_t send_mail_box;
    fric_tx_message.StdId = 0x200;
    fric_tx_message.IDE = CAN_ID_STD;
    fric_tx_message.RTR = CAN_RTR_DATA;
    fric_tx_message.DLC = 0x08;
    fric_can_send_data[0] = (fric1 >> 8);
    fric_can_send_data[1] = fric1;
    fric_can_send_data[2] = (fric2 >> 8);
    fric_can_send_data[3] = fric2;
    fric_can_send_data[4] = (trigger >> 8);
    fric_can_send_data[5] = trigger;
    fric_can_send_data[6] = (rev2 >> 8);
    fric_can_send_data[7] = rev2;
    HAL_CAN_AddTxMessage(&hcan2, &fric_tx_message, fric_can_send_data, &send_mail_box);
}
void CAN_cmd_trigger(int16_t trigger)
{
    uint32_t send_mail_box;
    trigger_tx_message.StdId = 0x200;
    trigger_tx_message.IDE = CAN_ID_STD;
    trigger_tx_message.RTR = CAN_RTR_DATA;
    trigger_tx_message.DLC = 0x08;
    trigger_can_send_data[4] = (trigger >> 8);
    trigger_can_send_data[5] = trigger;
    HAL_CAN_AddTxMessage(&hcan1, &trigger_tx_message, trigger_can_send_data, &send_mail_box);
}
/**
  * @brief          return the yaw 6020 motor data point
  * @param[in]      none
  * @retval         motor data point
  */
/**
  * @brief          ??yaw 6020??????
  * @param[in]      none
  * @retval         ??????
  */
const motor_measure_t *get_yaw_gimbal_motor_measure_point(void)
{
    return &motor_chassis[4];
}

/**
  * @brief          return the pitch 6020 motor data point
  * @param[in]      none
  * @retval         motor data point
  */
/**
  * @brief          ??pitch 6020??????
  * @param[in]      none
  * @retval         ??????
  */
const motor_measure_t *get_pitch_gimbal_motor_measure_point(void)
{
    return &motor_chassis[5];
}


/**
  * @brief          return the trigger 2006 motor data point
  * @param[in]      none
  * @retval         motor data point
  */
/**
  * @brief          ?????? 2006??????
  * @param[in]      none
  * @retval         ??????
  */
const motor_measure_t *get_trigger_motor_measure_point(void)
{
    return &motor_fric[2];
}


/**
  * @brief          return the chassis 3508 motor data point
  * @param[in]      i: motor number,range [0,3]
  * @retval         motor data point
  */
/**
  * @brief          ?????? 3508??????
  * @param[in]      i: ????,??[0,3]
  * @retval         ??????
  */
const motor_measure_t *get_chassis_motor_measure_point(uint8_t i)
{
    return &motor_chassis[(i & 0x03)];
}

//ma 
//���س�����������ָ��
const power_measure_t *get_power_measure_point(void)
{
	return &power_data;
}
//Ħ���ֵ�����ݷ���ָ��
const motor_measure_t *get_fric1_motor_measure_point(void)
{
    return &motor_fric[0];
}

const motor_measure_t *get_fric2_motor_measure_point(void)
{
    return &motor_fric[1];
}
const motor_measure_t *get_switch_motor_measure_point(void)
{
	return &motor_switch;
}