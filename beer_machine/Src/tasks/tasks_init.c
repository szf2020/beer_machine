#include "cmsis_os.h"
#include "comm_utils.h"
#include "tasks_init.h"
#include "log.h"
#define  LOG_MODULE_LEVEL    LOG_LEVEL_DEBUG
#define  LOG_MODULE_NAME     "[init]"

EventGroupHandle_t tasks_sync_evt_group_hdl;

/* 函数名：tasks_init
*  功能：  所有任务参数初始化 
*  参数：  无
*  返回：  0：成功 其他：失败
*/
int tasks_init(void)
{
 tasks_sync_evt_group_hdl=xEventGroupCreate(); 
 log_assert(tasks_sync_evt_group_hdl);
  
 return 0;
}