#ifndef  __SOCKET_MANAGE_TASK_H__
#define  __SOCKET_MANAGE_TASK_H__




extern osThreadId  socket_manage_task_handle;

void socket_manage_task(void const * argument);




typedef enum
{
SOCKET_MANAGE_TASK_MSG_QUERY_WIFI_STATUS,
SOCKET_MANAGE_TASK_MSG_QUERY_GSM_STATUS,
}socket_manage_msg_t;










#endif