#include "FreeRTOS.h"
#include "task.h"
#include "cmsis_os.h"
#include "string.h"
#include "socket.h"
#include "socket_manage_task.h"
#include "http_post_task.h"
#include "display_task.h"
#include "tasks_init.h"
#include "log.h"
#define  LOG_MODULE_LEVEL    LOG_LEVEL_DEBUG
#define  LOG_MODULE_NAME     "[net_manage]"


osThreadId  socket_manage_task_handle;

osMessageQId socket_manage_task_msg_q_id;

static osTimerId    wifi_query_timer_id;
static osTimerId    gsm_query_timer_id;


#define  SOCKET_MANAGE_TASK_QUERY_WIFI_TIMEOUT    4000
#define  SOCKET_MANAGE_TASK_QUERY_GSM_TIMEOUT     4000

static char lac[8];
static char ci[8];
#define  SOCKET_MANAGE_TASK_ERR_CNT_MAX          10
#define  SSID                                    "wkxboot"
#define  PASSWD                                  "wkxboot6666"

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
 osTimerStart(wifi_query_timer_id,SOCKET_MANAGE_TASK_QUERY_WIFI_TIMEOUT);  
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
osMessagePut(socket_manage_task_msg_q_id,SOCKET_MANAGE_TASK_MSG_QUERY_WIFI_STATUS,0);
}


static void gsm_query_timer_init()
{
 osTimerDef(gsm_query_timer,gsm_query_timer_expired);
 gsm_query_timer_id=osTimerCreate(osTimer(gsm_query_timer),osTimerOnce,0);
 log_assert(gsm_query_timer_id);
}

static void gsm_query_timer_start(void)
{
 osTimerStart(gsm_query_timer_id,SOCKET_MANAGE_TASK_QUERY_GSM_TIMEOUT);  
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
osMessagePut(socket_manage_task_msg_q_id,SOCKET_MANAGE_TASK_MSG_QUERY_GSM_STATUS,0);
}

void socket_manage_task(void const *argument)
{
 int rc;
 int wifi_level;
 osEvent  os_event;
 osStatus status;
 uint8_t wifi_err_cnt;
 uint8_t gsm_err_cnt;
 socket_manage_task_msg_t msg;
 display_task_msg_t display_msg;

 /*上电处理流程*/
 /*网络连接初始化*/
 socket_init();
 /*wifi 复位*/
 socket_module_wifi_reset();
 /*gsm 复位*/
 socket_module_gsm_reset();
  /*配置wifi网路*/
 socket_module_config_wifi(SSID,PASSWD);
 
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
  os_event = osMessageGet(socket_manage_task_msg_q_id,osWaitForever);
  if(os_event.status == osEventMessage){
  msg = (socket_manage_task_msg_t)os_event.value.v; 
 
 
  /*如果收到检查WIFI网络状态*/
  if(msg == SOCKET_MANAGE_TASK_MSG_QUERY_WIFI_STATUS){ 
  rc = socket_module_query_wifi_status(&wifi_level);
  if(rc != 0){
  wifi_err_cnt ++;
  if(wifi_err_cnt > SOCKET_MANAGE_TASK_ERR_CNT_MAX){
  wifi_err_cnt = 0;
  socket_module_wifi_reset();  
  }
  }else{
  wifi_err_cnt = 0;
  }
 
  display_msg.type = DISPLAY_TASK_MSG_WIFI;
  if(wifi_level == 0){
  display_msg.value = 3;
  display_msg.blink = true;    
  }else{
  display_msg.value = wifi_level;
  display_msg.blink = false;         
  }
  status = osMessagePut(display_task_msg_q_id,*(uint32_t*)&display_msg,SOCKET_MANAGE_TASK_PUT_MSG_TIMEOUT);
  if(status !=osOK){
  log_error("put compress display msg error:%d\r\n",status);
  } 
  
 /*重新启动wifi检查定时器*/
  wifi_query_timer_start(); 
 }
  
  
  /*如果收到检查GSM网络状态*/
  if(msg == SOCKET_MANAGE_TASK_MSG_QUERY_GSM_STATUS){ 
  rc = socket_module_query_gsm_status(lac,ci);
  if(rc != 0){
  gsm_err_cnt ++;
  if(gsm_err_cnt >= SOCKET_MANAGE_TASK_ERR_CNT_MAX){
  gsm_err_cnt = 0;
  socket_module_gsm_reset();  
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