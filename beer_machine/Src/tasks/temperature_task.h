#ifndef  __TEMPERATURE_TASK_H__
#define  __TEMPERATURE_TASK_H__
#include "stdint.h"


#ifdef  __cplusplus
#define TEMPERATURE_TASK_BEGIN  extern "C" {
#define TEMPERATURE_TASK_END    }
#else
#define TEMPERATURE_TASK_BEGIN  
#define TEMPERATURE_TASK_END   
#endif


TEMPERATURE_TASK_BEGIN

extern osThreadId   temperature_task_handle;
extern osMessageQId temperature_task_msg_q_id;

void temperature_task(void const *argument);



#define  TEMPERATURE_TASK_TEMPERATURE_CHANGE_CNT             2
#define  TEMPERATURE_TASK_TEMPERATURE_CHANGE_NEAGTIVE_CNT   -2

#define  TEMPERATURE_SENSOR_ADC_VALUE_MAX          4095/*温度AD转换最大数值*/      
#define  TEMPERATURE_SENSOR_BYPASS_RES_VALUE       2000/*温度AD转换旁路电阻值*/  
#define  TEMPERATURE_SENSOR_REFERENCE_VOLTAGE      3.3 /*温度传感器参考电压 单位:V*/
#define  TEMPERATURE_SENSOR_SUPPLY_VOLTAGE         3.3 /*温度传感器供电电压 单位:V*/


#define  TEMPERATURE_TASK_MSG_WAIT_TIMEOUT         osWaitForever
#define  TEMPERATURE_TASK_PUT_MSG_TIMEOUT          5  /*发送消息超时时间*/

#define  TR_MAP_IDX_MIN                            3  /*显示最小值t_r_map[3 ][1] r=10394 -9摄氏度*/ 
#define  TR_MAP_IDX_MAX                            64 /*显示最大值t_r_map[64][1] r=672   52摄氏度*/ 
#define  TEMPERATURE_COMPENSATION_VALUE            -2 /*温度补偿值,因为温度传感器位置温度与桶内实际温度有误差*/

#define  TEMPERATURE_ERR_VALUE_OVER_HIGH           (0xe * 10 + 0)/*温度显示过高错误代码   e0*/
#define  TEMPERATURE_ERR_VALUE_OVER_LOW            (0xe * 10 + 1)/*温度显示过低错误代码   e1*/
#define  TEMPERATURE_ERR_VALUE_SENSOR              (0xe * 10 + 2)/*温度显示传感器错误代码 e2*/

typedef enum
{
TEMPERATURE_TASK_MSG_ADC_COMPLETED
}temperature_task_msg_type_t;

typedef struct
{
uint32_t type:8;
uint32_t value:16;
uint32_t reserved:8;
}temperature_task_msg_t;


TEMPERATURE_TASK_END

#endif