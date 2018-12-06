#ifndef  __CONNECTION_MANAGE_TASK_H__
#define  __CONNECTION_MANAGE_TASK_H__




extern osThreadId  connection_manage_task_handle;

void connection_manage_task(void const * argument);




typedef enum
{
CONNECTION_MANAGE_TASK_MSG_QUERY_WIFI_STATUS,
CONNECTION_MANAGE_TASK_MSG_QUERY_GSM_STATUS,
}connection_manage_msg_t;










#endif