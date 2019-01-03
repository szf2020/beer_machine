#include "cmsis_os.h"
#include "comm_utils.h"
#include "tasks_init.h"
#include "alarm_task.h"
#include "compressor_task.h"
#include "display_task.h"
#include "pressure_task.h"
#include "net_task.h"
#include "temperature_task.h"
#include "report_task.h"
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
 /*创建同步事件组*/
 tasks_sync_evt_group_hdl=xEventGroupCreate(); 
 log_assert(tasks_sync_evt_group_hdl);
 /*创建报警消息队列*/
 osMessageQDef(alarm_msg_q,10,uint32_t);
 alarm_task_msg_q_id = osMessageCreate(osMessageQ(alarm_msg_q),alarm_task_handle);
 log_assert(alarm_task_msg_q_id);
 /*创建压缩机消息队列*/
 osMessageQDef(compressor_msg_q,8,uint32_t);
 compressor_task_msg_q_id = osMessageCreate(osMessageQ(compressor_msg_q),compressor_task_handle);
 log_assert(compressor_task_msg_q_id);
 /*创建显示消息队列*/
 osMessageQDef(display_msg_q,8,uint32_t);
 display_task_msg_q_id = osMessageCreate(osMessageQ(display_msg_q),display_task_handle);
 log_assert(display_task_msg_q_id);
 /*创建压力消息队列*/
 osMessageQDef(pressure_msg_q,8,uint32_t);
 pressure_task_msg_q_id = osMessageCreate(osMessageQ(pressure_msg_q),pressure_task_handle);
 log_assert(pressure_task_msg_q_id);
 /*创建网络管理消息队列*/
 osMessageQDef(net_task_msg_q,4,uint32_t);
 net_task_msg_q_id = osMessageCreate(osMessageQ(net_task_msg_q),net_task_handle);
 log_assert(net_task_msg_q_id);
 /*创建温度消息队列*/
 osMessageQDef(temperature_msg_q,4,uint32_t);
 temperature_task_msg_q_id = osMessageCreate(osMessageQ(temperature_msg_q),temperature_task_handle);
 log_assert(temperature_task_msg_q_id);
 
 /*信息上报消息队列*/
 osMessageQDef(report_task_msg_q,20,report_task_msg_t*);
 report_task_msg_q_id = osMessageCreate(osMessageQ(report_task_msg_q),report_task_handle);
 log_assert(report_task_msg_q_id); 
 /*信息上报位置消息队列*/
 osMessageQDef(report_task_location_msg_q,4,net_location_t*);
 report_task_location_msg_q_id = osMessageCreate(osMessageQ(report_task_location_msg_q),report_task_handle);
 log_assert(report_task_location_msg_q_id); 
 
 /*网络硬件消息队列*/
 osMessageQDef(report_task_net_hal_info_msg_q,4,net_hal_information_t*);
 report_task_net_hal_info_msg_q_id = osMessageCreate(osMessageQ(report_task_net_hal_info_msg_q),report_task_handle);
 log_assert(report_task_net_hal_info_msg_q_id); 
 
 
 return 0;
}