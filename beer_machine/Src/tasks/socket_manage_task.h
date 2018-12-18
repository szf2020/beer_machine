#ifndef  __SOCKET_MANAGE_TASK_H__
#define  __SOCKET_MANAGE_TASK_H__




extern osThreadId  socket_manage_task_handle;
extern osMessageQId socket_manage_task_msg_q_id;

void socket_manage_task(void const * argument);




typedef enum
{
SOCKET_MANAGE_TASK_MSG_QUERY_WIFI_STATUS,
SOCKET_MANAGE_TASK_MSG_QUERY_GSM_STATUS,
}socket_manage_task_msg_t;


#define  SOCKET_MANAGE_TASK_PUT_MSG_TIMEOUT         5







#endif