#ifndef  __COMPRESSOR_TASK_H__
#define  __COMPRESSOR_TASK_H__
#include "stdint.h"


#ifdef  __cplusplus
#define COMPRESSOR_TASK_BEGIN  extern "C" {
#define COMPRESSOR_TASK_END    }
#else
#define COMPRESSOR_TASK_BEGIN  
#define COMPRESSOR_TASK_END   
#endif


COMPRESSOR_TASK_BEGIN

extern osThreadId   compressor_task_handle;
extern osMessageQId compressor_task_msg_q_id;

void compressor_task(void const *argument);



#define  COMPRESSOR_WORK_TEMPERATURE           5 /*开压缩机温度单位:摄氏度*/
#define  COMPRESSOR_STOP_TEMPERATURE           1 /*关压缩机温度单位:摄氏度*/

#define  COMPRESSOR_TASK_WORK_TIMEOUT          (120*60*1000) /*连续工作时间单位:ms*/
#define  COMPRESSOR_TASK_REST_TIMEOUT          (10*60*1000)  /*连续工作时间后的休息时间单位:ms*/
#define  COMPRESSOR_TASK_WAIT_TIMEOUT          (5*60*1000)   /*2次开机的等待时间 单位:ms*/

#define  COMPRESSOR_TASK_PUT_MSG_TIMEOUT       5             /*发送消息超时时间 单位:ms*/

#define  COMPRESSOT_TASK_WAIT_RDY_TIMEOUT      (2*60*1000)   /*压缩机上电后等待就绪的时间 单位:ms*/

typedef enum
{
COMPRESSOR_TASK_MSG_TEMPERATURE,
COMPRESSOR_TASK_MSG_WORK_TIMEOUT,
COMPRESSOR_TASK_MSG_WAIT_TIMEOUT,
COMPRESSOR_TASK_MSG_REST_TIMEOUT,
COMPRESSOR_TASK_MSG_LOCK,
COMPRESSOR_TASK_MSG_UNLOCK
}compressor_task_msg_type_t;


typedef struct
{
uint32_t type:8;
uint32_t value:16;
uint32_t reserved:8;
}compressor_task_msg_t;

COMPRESSOR_TASK_END

#endif





