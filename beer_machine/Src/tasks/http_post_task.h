#ifndef  __HTTP_POST_TASK_H__
#define  __HTTP_POST_TASK_H__



extern osThreadId  http_post_task_handle;
extern osMessageQId http_post_task_msg_q_id;



void http_post_task(void const * argument);

















#endif