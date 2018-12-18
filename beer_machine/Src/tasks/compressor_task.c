#include "cmsis_os.h"
#include "tasks_init.h"
#include "compressor_task.h"
#include "temperature_task.h"
#include "display_task.h"
#include "beer_machine.h"
#include "log.h"
#define  LOG_MODULE_LEVEL    LOG_LEVEL_DEBUG
#define  LOG_MODULE_NAME     "[compressor]"

osThreadId   compressor_task_handle;
osMessageQId compressor_task_msg_q_id;


osTimerId    compressor_work_timer_id;
osTimerId    compressor_wait_timer_id;
osTimerId    compressor_rest_timer_id;


typedef enum
{
COMPRESSOR_STATUS_RDY=0, /*就绪状态*/
COMPRESSOR_STATUS_REST,  /*长时间工作后停机的状态*/
COMPRESSOR_STATUS_WORK,  /*压缩机工作状态*/
COMPRESSOR_STATUS_WAIT   /*两次开机之间的状态*/
}compressor_status_t;

typedef enum
{
COMPRESSOR_UNLOCK,
COMPRESSOR_LOCK
}compressor_lock_t;



typedef struct
{
compressor_status_t status;
int16_t temperature;
int16_t temperature_work;
int16_t temperature_stop;
compressor_lock_t lock;
bool    status_change;
}compressor_t;

static compressor_t compressor ={
.temperature_stop = COMPRESSOR_STOP_TEMPERATURE,
.temperature_work = COMPRESSOR_WORK_TEMPERATURE
};

static void compressor_work_timer_init(void);
static void compressor_wait_timer_init(void);
static void compressor_rest_timer_init(void);

static void compressor_work_timer_start(void);
static void compressor_wait_timer_start(void);
static void compressor_rest_timer_start(void);


static void compressor_work_timer_stop(void);
/*
static void compressor_wait_timer_stop(void);
static void compressor_rest_timer_stop(void);
*/

static void compressor_work_timer_expired(void const *argument);
static void compressor_wait_timer_expired(void const *argument);
static void compressor_rest_timer_expired(void const *argument);

static void compressor_pwr_turn_on();
static void compressor_pwr_turn_off();


static void compressor_work_timer_init()
{
 osTimerDef(compressor_work_timer,compressor_work_timer_expired);
 compressor_work_timer_id=osTimerCreate(osTimer(compressor_work_timer),osTimerOnce,0);
 log_assert(compressor_work_timer_id);
}

static void compressor_work_timer_start(void)
{
 osTimerStart(compressor_work_timer_id,COMPRESSOR_TASK_WORK_TIMEOUT);  
 log_debug("压缩机工作定时器开始.\r\n");
}
static void compressor_work_timer_stop(void)
{
 osTimerStop(compressor_work_timer_id);  
 log_debug("压缩机工作定时器停止.\r\n");  
}

static void compressor_work_timer_expired(void const *argument)
{
 osStatus status;
 compressor_task_msg_t work_timeout_msg;
 /*工作超时 发送消息*/
 work_timeout_msg.type = COMPRESSOR_TASK_MSG_WORK_TIMEOUT;
 status = osMessagePut(compressor_task_msg_q_id,(uint32_t)&work_timeout_msg,COMPRESSOR_TASK_PUT_MSG_TIMEOUT);
 if(status !=osOK){
 log_error("put work timeout msg error:%d\r\n",status);
 } 
}


static void compressor_wait_timer_init()
{
 osTimerDef(compressor_wait_timer,compressor_wait_timer_expired);
 compressor_wait_timer_id=osTimerCreate(osTimer(compressor_wait_timer),osTimerOnce,0);
 log_assert(compressor_wait_timer_id);
}


static void compressor_wait_timer_start(void)
{
 osTimerStart(compressor_wait_timer_id,COMPRESSOR_TASK_WAIT_TIMEOUT);  
 log_debug("压缩机等待定时器开始.\r\n");
}


static void compressor_wait_timer_stop(void)
{
 osTimerStop(compressor_wait_timer_id);  
 log_debug("压缩机等待定时器停止.\r\n");  
}


static void compressor_wait_timer_expired(void const *argument)
{
 osStatus status;
 compressor_task_msg_t wait_timeout_msg;
 /*工作超时 发送消息*/
 wait_timeout_msg.type = COMPRESSOR_TASK_MSG_WAIT_TIMEOUT;
 status = osMessagePut(compressor_task_msg_q_id,(uint32_t)&wait_timeout_msg,COMPRESSOR_TASK_PUT_MSG_TIMEOUT);
 if(status !=osOK){
 log_error("put wait timeout msg error:%d\r\n",status);
 } 
}


static void compressor_rest_timer_init(void)
{
 osTimerDef(compressor_rest_timer,compressor_rest_timer_expired);
 compressor_rest_timer_id = osTimerCreate(osTimer(compressor_rest_timer),osTimerOnce,0);
 log_assert(compressor_rest_timer_id);
}

static void compressor_rest_timer_start(void)
{
 osTimerStart(compressor_rest_timer_id,COMPRESSOR_TASK_REST_TIMEOUT);
 log_debug("压缩机休息定时器开始.\r\n");
}


static void compressor_rest_timer_stop(void)
{
 osTimerStop(compressor_rest_timer_id);
 log_debug("压缩机休息定时器停止.\r\n");
}

static void compressor_rest_timer_expired(void const *argument)
{
 osStatus status;
 compressor_task_msg_t rest_timeout_msg;
 /*工作超时 发送消息*/
 rest_timeout_msg.type = COMPRESSOR_TASK_MSG_REST_TIMEOUT;
 status = osMessagePut(compressor_task_msg_q_id,(uint32_t)&rest_timeout_msg,COMPRESSOR_TASK_PUT_MSG_TIMEOUT);
 if(status !=osOK){
 log_error("put rest timeout msg error:%d\r\n",status);
 } 
}

/*压缩机打开*/
static void compressor_pwr_turn_on()
{
 bsp_compressor_ctrl_on(); 
}
/*压缩机关闭*/
static void compressor_pwr_turn_off()
{
 bsp_compressor_ctrl_off();  
}


void compressor_task(void const *argument)
{
  osEvent  os_msg;
  osStatus status;
  
  display_task_msg_t display_msg;
  compressor_task_msg_t msg;
  compressor_task_msg_t temperature_msg;
  
  /*开机先关闭压缩机*/
  compressor_pwr_turn_off();
  /*等待任务同步*/
  /*
  xEventGroupSync(tasks_sync_evt_group_hdl,TASKS_SYNC_EVENT_COMPRESSOR_TASK_RDY,TASKS_SYNC_EVENT_ALL_TASKS_RDY,osWaitForever);
  log_debug("compressor task sync ok.\r\n");
  */
  compressor_work_timer_init();
  compressor_wait_timer_init();
  compressor_rest_timer_init();
  
  osDelay(COMPRESSOT_TASK_WAIT_RDY_TIMEOUT);
  log_debug("压缩机上电待机完毕.\r\n");
 
  while(1){
  os_msg = osMessageGet(compressor_task_msg_q_id,COMPRESSOR_TASK_MSG_WAIT_TIMEOUT);
  if(os_msg.status == osEventMessage){    
    
  msg = *(compressor_task_msg_t*)&os_msg.value.v;
  
  /*如果压缩机被锁定了 就只处理UNLOCK消息*/
  if(compressor.lock == COMPRESSOR_LOCK){  
  if(msg.type == COMPRESSOR_TASK_MSG_UNLOCK){
  compressor.lock = COMPRESSOR_UNLOCK;   
  /*存入nv*/
  
  /*构建温度消息*/
  temperature_msg.type = COMPRESSOR_TASK_MSG_TEMPERATURE;
  temperature_msg.value = compressor.temperature;
  status = osMessagePut(compressor_task_msg_q_id,*(uint32_t*)&temperature_msg,COMPRESSOR_TASK_PUT_MSG_TIMEOUT);
  if(status !=osOK){
  log_error("put compressor temperature msg error:%d\r\n",status);
  } 
  }else{
  /*其他消息只处理温度信息*/
  /*温度消息处理*/
  if(msg.type == COMPRESSOR_TASK_MSG_TEMPERATURE){ 
  /*缓存温度值*/
  compressor.temperature = msg.value; 
  continue;  
  }
  }
  }
  
  /*如果压缩机LOCK消息*/
  if(msg.type == COMPRESSOR_TASK_MSG_LOCK){
  if(compressor.lock !=  COMPRESSOR_LOCK ){
  compressor.lock = COMPRESSOR_LOCK;
  /*存入nv*/
  log_debug("LOCK.....关压缩机.\r\n");
  compressor.status = COMPRESSOR_STATUS_RDY;
  /*关闭压缩机*/
  compressor_pwr_turn_off();  
  compressor.status_change = true;
  /*关闭所有定时器*/ 
  compressor_work_timer_stop();  
  compressor_wait_timer_stop();  
  compressor_rest_timer_stop();  
  }  
  }
  
  /*温度消息处理*/
  if(msg.type == COMPRESSOR_TASK_MSG_TEMPERATURE){ 
  /*缓存温度值*/
  compressor.temperature = msg.value; 
  
  if(compressor.temperature == TEMPERATURE_ERR_VALUE_SENSOR ){ 
  log_error("温度错误.code:%d.\r\n",compressor.temperature);
  if(compressor.status == COMPRESSOR_STATUS_WORK){
  log_debug("关压缩机.\r\n");
  compressor.status = COMPRESSOR_STATUS_WAIT;
  /*关闭压缩机*/
  compressor_pwr_turn_off();  
  compressor.status_change = true;
  /*打开等待定时器*/ 
  compressor_wait_timer_start();
  }
  }else if(compressor.temperature >= compressor.temperature_work  && compressor.status == COMPRESSOR_STATUS_RDY){
  log_debug("温度:%d 高于开机温度.\r\n",compressor.temperature);
  log_debug("开压缩机.\r\n");
  /*打开压缩机和工作定时器*/
  compressor.status = COMPRESSOR_STATUS_WORK;
  compressor_pwr_turn_on();
  compressor.status_change = true;
  compressor_work_timer_start(); 
  }else if(compressor.temperature <= compressor.temperature_stop && compressor.status == COMPRESSOR_STATUS_WORK){
  log_debug("温度:%d 低于关机温度.\r\n",compressor.temperature);
  log_debug("关压缩机.\r\n");
  compressor.status = COMPRESSOR_STATUS_WAIT;
  /*关闭压缩机和工作定时器*/
  compressor_pwr_turn_off(); 
  compressor.status_change = true;
  compressor_work_timer_stop();
  /*打开等待定时器*/ 
  compressor_wait_timer_start(); 
  }
  }
  
  /*压缩机工作超时消息*/
  if(msg.type == COMPRESSOR_TASK_MSG_WORK_TIMEOUT){
  if(compressor.status == COMPRESSOR_STATUS_WORK){
  log_debug("关压缩机.\r\n");
  compressor.status = COMPRESSOR_STATUS_REST;
  /*关闭压缩机和工作定时器*/
  compressor_pwr_turn_off();  
  compressor.status_change = true;
  /*打开休息定时器*/ 
  compressor_rest_timer_start(); 
  }else{
  log_warning("压缩机状态:%d 无需处理.\r\n",compressor.status);  
  }
  }
 
  /*压缩机等待超时消息*/
  if(msg.type == COMPRESSOR_TASK_MSG_WAIT_TIMEOUT){
  if(compressor.status == COMPRESSOR_STATUS_WAIT){
  compressor.status = COMPRESSOR_STATUS_RDY;
  /*构建温度消息*/
  temperature_msg.type = COMPRESSOR_TASK_MSG_TEMPERATURE;
  temperature_msg.value = compressor.temperature;
  status = osMessagePut(compressor_task_msg_q_id,*(uint32_t*)&temperature_msg,COMPRESSOR_TASK_PUT_MSG_TIMEOUT);
  if(status !=osOK){
  log_error("put compressor temperature msg error:%d\r\n",status);
  }   
  }else{
  log_warning("压缩机状态:%d 无需处理.\r\n",compressor.status);     
  }
 }
 
  /*压缩机休息超时消息*/
  if(msg.type == COMPRESSOR_TASK_MSG_REST_TIMEOUT){
  if(compressor.status == COMPRESSOR_STATUS_REST){
  compressor.status = COMPRESSOR_STATUS_RDY;
  /*构建温度消息*/
  temperature_msg.type = COMPRESSOR_TASK_MSG_TEMPERATURE;
  temperature_msg.value = compressor.temperature;
  status = osMessagePut(compressor_task_msg_q_id,*(uint32_t*)&temperature_msg,COMPRESSOR_TASK_PUT_MSG_TIMEOUT);
  if(status !=osOK){
  log_error("put compressor temperature msg error:%d\r\n",status);
  }   
  }else{
  log_warning("压缩机状态:%d 无需处理.\r\n",compressor.status);     
  }
 }
 
 /*处理状态变化显示*/
  if(compressor.status_change == true ){
  compressor.status_change = false;
  display_msg.type =  DISPLAY_TASK_MSG_COMPRESSOR;
  if(compressor.status == COMPRESSOR_STATUS_WORK){ 
  display_msg.blink = true;
  }else{
  display_msg.blink = false;  
  }
  status = osMessagePut(display_task_msg_q_id,*(uint32_t*)&display_msg,COMPRESSOR_TASK_PUT_MSG_TIMEOUT);
  if(status !=osOK){
  log_error("put compress display msg error:%d\r\n",status);
  }    
  }
 
 }
 }
}

