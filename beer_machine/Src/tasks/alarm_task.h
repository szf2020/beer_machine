#ifndef  __ALARM_TASK_H__
#define  __ALARM_TASK_H__
#include "stdint.h"
#include "stdbool.h"

#ifdef  __cplusplus
#define ALARM_TASK_BEGIN  extern "C" {
#define ALARM_TASK_END    }
#else
#define ALARM_TASK_BEGIN  
#define ALARM_TASK_END   
#endif


ALARM_TASK_BEGIN


extern osThreadId   alarm_task_handle;
extern osMessageQId alarm_task_msg_q_id;

void alarm_task(void const *argument);


typedef enum
{
ALARM_TASK_MSG_TIMER_TIMEOUT,
ALARM_TASK_MSG_TEMPERATURE,
ALARM_TASK_MSG_PRESSURE,
ALARM_TASK_MSG_CAPACITY
}alarm_task_msg_type_t;


typedef struct
{
uint32_t type:8;
uint32_t alarm:8;
uint32_t reserved:16;
}alarm_task_msg_t;




ALARM_TASK_END

#endif