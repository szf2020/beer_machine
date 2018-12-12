#ifndef  __DISPLAY_TASK_H__
#define  __DISPLAY_TASK_H__
#include "stdint.h"
#include "stdbool.h"

#ifdef  __cplusplus
#define DISPLAY_TASK_BEGIN  extern "C" {
#define DISPLAY_TASK_END    }
#else
#define DISPLAY_TASK_BEGIN  
#define DISPLAY_TASK_END   
#endif


DISPLAY_TASK_BEGIN

extern osThreadId   display_task_handle;
extern osMessageQId display_task_msg_q_id;

void display_task(void const *argument);


#define  DISPLAY_TASK_MSG_WAIT_TIMEOUT         osWaitForever
#define  DISPLAY_TASK_PUT_MSG_TIMEOUT          5


#define  DISPLAY_TASK_BLINK_TIMER_TIMEOUT      250

typedef struct
{
uint16_t value;
bool     blink;
}display_unit_t;

typedef enum
{
DISPLAY_TASK_MSG_TEMPERATURE,
DISPLAY_TASK_MSG_PRESSURE,
DISPLAY_TASK_MSG_PRESSURE_POINT,
DISPLAY_TASK_MSG_CAPACITY,
DISPLAY_TASK_MSG_CAPACITY_LEVEL,
DISPLAY_TASK_MSG_WIFI,
DISPLAY_TASK_MSG_COMPRESSOR,
DISPLAY_TASK_MSG_BLINK
}display_task_msg_type_t;

typedef struct
{
uint32_t  type:8;
uint32_t  value:16;
uint32_t  blink:8;
}display_task_msg_t;


DISPLAY_TASK_END


#endif