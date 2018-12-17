#ifndef  __CAPACITY_TASK_H__
#define  __CAPACITY_TASK_H__


extern osThreadId   capacity_task_handle;
void capacity_task(void const *argument);


#define  CAPACITY_BUFFER_SIZE                16

#define  CAPACITY_SERIAL_PORT                3
#define  CAPACITY_SERIAL_BAUDRATES           9600
#define  CAPACITY_SERIAL_DATA_BITS           8
#define  CAPACITY_SERIAL_STOP_BITS           1



#define       R         130.0   /*mm 啤酒桶半径*/   
#define       S         53093.0 /*mm2 啤酒桶横截面积*/

#define  CAPACITY_TASK_DIR_CHANGE_CNT          5 /*容量值确认次数*/
#define  CAPACITY_TASK_ERR_CNT                 10 /*故障确认次数*/
#define  CAPACITY_TASK_CAPACITY_BLINK_VALUE    5 /*5升*/       
#define  CAPACITY_TASK_PUT_MSG_TIMEOUT         5 

#define  CAPACITY_TASK_ERR_VALUE               (0xe * 10 + 2) /*液位传感器故障 代码E2*/




#endif