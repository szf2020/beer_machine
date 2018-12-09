#include "FreeRTOS.h"
#include "task.h"
#include "cmsis_os.h"
#include "string.h"
#include "connection.h"
#include "connection_manage_task.h"
#include "http_post_task.h"
#include "tasks_init.h"
#include "log.h"
#define  LOG_MODULE_LEVEL    LOG_LEVEL_DEBUG
#define  LOG_MODULE_NAME     "[net_manage]"


osThreadId  connection_manage_task_handle;

osMessageQId connection_manage_task_msg_q_id;

static osTimerId    wifi_query_timer_id;
static osTimerId    gsm_query_timer_id;

#define  CONNECTION_MANAGE_TASK_NET_ERR_CNT      5



#define  CONNECTION_MANAGE_TASK_QUERY_WIFI_TIMEOUT    4000
#define  CONNECTION_MANAGE_TASK_QUERY_GSM_TIMEOUT     4000

static void wifi_query_timer_init(void);
static void wifi_query_timer_start(void);
//static void wifi_query_timer_stop(void);
static void wifi_query_timer_expired(void const *argument);

static void gsm_query_timer_init(void);
static void gsm_query_timer_start(void);
//static void gsm_query_timer_stop(void);
static void gsm_query_timer_expired(void const *argument);


static void wifi_query_timer_init(void)
{
 osTimerDef(wifi_query_timer,wifi_query_timer_expired);
 wifi_query_timer_id=osTimerCreate(osTimer(wifi_query_timer),osTimerOnce,0);
 log_assert(wifi_query_timer_id);
}

static void wifi_query_timer_start(void)
{
 osTimerStart(wifi_query_timer_id,CONNECTION_MANAGE_TASK_QUERY_WIFI_TIMEOUT);  
 log_debug("wifi net timer start.\r\n");
}
/*
static void wifi_query_timer_stop(void)
{
 osTimerStop(wifi_query_timer_id);  
 log_debug("wifi net timer stop.\r\n");  
}
*/
static void wifi_query_timer_expired(void const *argument)
{
osMessagePut(connection_manage_task_msg_q_id,CONNECTION_MANAGE_TASK_MSG_QUERY_WIFI_STATUS,0);
}


static void gsm_query_timer_init()
{
 osTimerDef(gsm_query_timer,gsm_query_timer_expired);
 gsm_query_timer_id=osTimerCreate(osTimer(gsm_query_timer),osTimerOnce,0);
 log_assert(gsm_query_timer_id);
}

static void gsm_query_timer_start(void)
{
 osTimerStart(gsm_query_timer_id,CONNECTION_MANAGE_TASK_QUERY_GSM_TIMEOUT);  
 log_debug("gsm net timer start.\r\n");
}
/*
static void gsm_query_timer_stop(void)
{
 osTimerStop(gsm_query_timer_id);  
 log_debug("wifi net timer stop.\r\n");  
}
*/
static void gsm_query_timer_expired(void const *argument)
{
osMessagePut(connection_manage_task_msg_q_id,CONNECTION_MANAGE_TASK_MSG_QUERY_GSM_STATUS,0);
}


#define  CONNECTION_MANAGE_TASK_ERR_CNT_MAX      10

void connection_manage_task(void const *argument)
{
 int rc;
 uint8_t wifi_err_cnt;
 uint8_t gsm_err_cnt;
 connection_manage_msg_t msg;
 osEvent  os_event;
 
 osMessageQDef(connection_manage_task_msg_q,4,uint32_t);
 
 connection_manage_task_msg_q_id = osMessageCreate(osMessageQ(connection_manage_task_msg_q),connection_manage_task_handle);
 log_assert(connection_manage_task_msg_q_id);

 /*网络连接初始化*/
 connection_init();
 /*等待任务同步*/
 xEventGroupSync(tasks_sync_evt_group_hdl,TASKS_SYNC_EVENT_NET_MANAGE_TASK_RDY,TASKS_SYNC_EVENT_ALL_TASKS_RDY,osWaitForever);
 log_debug("net manage task sync ok.\r\n");
 

 
 wifi_query_timer_init();
 gsm_query_timer_init();
 wifi_query_timer_start();
 gsm_query_timer_start();
 wifi_err_cnt = 0;
 gsm_err_cnt = 0;
 
 while(1){
 os_event = osMessageGet(connection_manage_task_msg_q_id,osWaitForever);
 if(os_event.status == osEventMessage){
 msg = (connection_manage_msg_t)os_event.value.v; 
 
 
  /*如果收到检查WIFI网络状态*/
  if(msg == CONNECTION_MANAGE_TASK_MSG_QUERY_WIFI_STATUS){ 
  rc = connection_query_wifi_status();
  if(rc != 0){
  wifi_err_cnt ++;
  if(wifi_err_cnt > CONNECTION_MANAGE_TASK_ERR_CNT_MAX){
  wifi_err_cnt = 0;
  connection_wifi_reset();  
  }
  }else{
  wifi_err_cnt = 0;
  }
 /*重新启动wifi检查定时器*/
  wifi_query_timer_start(); 
 }
  
  
  /*如果收到检查GSM网络状态*/
  if(msg == CONNECTION_MANAGE_TASK_MSG_QUERY_GSM_STATUS){ 
  rc = connection_query_gsm_status();
  if(rc != 0){
  gsm_err_cnt ++;
  if(gsm_err_cnt >= CONNECTION_MANAGE_TASK_ERR_CNT_MAX){
  gsm_err_cnt = 0;
  connection_gsm_reset();  
  }
  }else{
  gsm_err_cnt = 0;
  }
 /*重新启动GSM检查定时器*/
  gsm_query_timer_start(); 
 }
  
 
 
 }
 }
 }