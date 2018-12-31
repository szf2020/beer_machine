#include "cmsis_os.h"
#include "tasks_init.h"
#include "beer_machine.h"
#include "compressor_task.h"
#include "display_task.h"
#include "alarm_task.h"
#include "device_config.h"
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
  COMPRESSOR_STATUS_RDY=0,        /*正常关机后的就绪状态*/
  COMPRESSOR_STATUS_RDY_CONTINUE, /*长时间工作停机后的就绪状态*/
  COMPRESSOR_STATUS_REST,         /*长时间工作后的停机状态*/
  COMPRESSOR_STATUS_WORK,         /*压缩机工作状态*/
  COMPRESSOR_STATUS_WAIT,         /*两次开机之间的状态*/
  COMPRESSOR_STATUS_WAIT_CONTINUE /*异常时两次开机之间的状态*/
}compressor_status_t;

typedef struct
{
  compressor_status_t status;
  int8_t temperature;
  int8_t temperature_work;
  int8_t temperature_stop;
  bool   lock;
  bool   status_change;
}compressor_t;

static compressor_t compressor ={
.temperature_stop = DEFAULT_COMPRESSOR_LOW_TEMPERATURE,
.temperature_work = DEFAULT_COMPRESSOR_HIGH_TEMPERATURE,
.status = COMPRESSOR_STATUS_RDY_CONTINUE
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
  status = osMessagePut(compressor_task_msg_q_id,*(uint32_t*)&work_timeout_msg,COMPRESSOR_TASK_PUT_MSG_TIMEOUT);
  if(status !=osOK){
     log_error("compressor put work timeout msg error:%d\r\n",status);
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
  status = osMessagePut(compressor_task_msg_q_id,*(uint32_t*)&wait_timeout_msg,COMPRESSOR_TASK_PUT_MSG_TIMEOUT);
  if(status !=osOK){
     log_error("compressor put wait timeout msg error:%d\r\n",status);
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
  status = osMessagePut(compressor_task_msg_q_id,*(uint32_t*)&rest_timeout_msg,COMPRESSOR_TASK_PUT_MSG_TIMEOUT);
  if(status !=osOK){
     log_error("compressor put rest timeout msg error:%d\r\n",status);
  } 
}

static int compressor_task_send_temperature_msg_to_self()
{  
  osStatus status;
  compressor_task_msg_t msg;
  /*构建温度消息*/
  if((uint8_t)compressor.temperature == ALARM_TASK_TEMPERATURE_ERR_VALUE){
     msg.type = COMPRESSOR_TASK_MSG_TEMPERATURE_ERR;
  }else{
     msg.type = COMPRESSOR_TASK_MSG_TEMPERATURE_VALUE;
  }
  msg.value = compressor.temperature;
  status = osMessagePut(compressor_task_msg_q_id,*(uint32_t*)&msg,COMPRESSOR_TASK_PUT_MSG_TIMEOUT);
  if(status !=osOK){
     log_error("compressor put t msg error:%d\r\n",status);
     return -1;
   }
  
  return 0;
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
  
  display_task_msg_t    display_msg;
  compressor_task_msg_t msg;

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
  os_msg = osMessageGet(compressor_task_msg_q_id,osWaitForever);
  if(os_msg.status == osEventMessage){    
    
     msg = *(compressor_task_msg_t*)&os_msg.value.v;
  
     /*LOCK配置消息*/
     if(msg.type == COMPRESSOR_TASK_MSG_LOCK_CONFIG){
        /*锁定消息*/
        if(msg.value == ALARM_TASK_COMPRESSOR_LOCK_VALUE && compressor.lock == false){
          compressor.lock = true;
          log_debug("LOCK.....关压缩机.\r\n");
          compressor.status = COMPRESSOR_STATUS_RDY_CONTINUE;
          /*关闭压缩机*/
          compressor_pwr_turn_off();  
          compressor.status_change = true;
          /*关闭所有定时器*/ 
          compressor_work_timer_stop();  
          compressor_wait_timer_stop();  
          compressor_rest_timer_stop();
 
        } else if (msg.value == ALARM_TASK_COMPRESSOR_UNLOCK_VALUE && compressor.lock == true){ 
          /*解锁消息*/
          compressor.lock = false;
          compressor_task_send_temperature_msg_to_self();
       }
     }
                   
     /*压缩机温度配置消息处理*/
     if(msg.type == COMPRESSOR_TASK_MSG_TEMPERATURE_CONFIG){ 
       /*缓存温度配置值*/
        compressor.temperature_work = (int8_t)msg.reserved >> 8;
        compressor.temperature_stop = (int8_t)msg.reserved & 0xFF;
        compressor_task_send_temperature_msg_to_self();
     } 
  
  /*温度消息处理*/
  if(msg.type == COMPRESSOR_TASK_MSG_TEMPERATURE_VALUE){ 
     /*缓存温度值*/
     compressor.temperature = (int8_t)msg.value; 
     if(compressor.lock == false){

     if(compressor.temperature <= compressor.temperature_stop  && compressor.status == COMPRESSOR_STATUS_WORK){
        log_warning("温度:%d 低于关机温度.\r\n",compressor.temperature);
        compressor.status = COMPRESSOR_STATUS_WAIT;
        log_warning("compressor change status to wait.\r\n");  
        log_warning("关压缩机.\r\n");
        compressor.status_change = true;
        /*关闭压缩机和工作定时器*/
        compressor_pwr_turn_off(); 
        compressor_work_timer_stop();
        /*打开等待定时器*/ 
        compressor_wait_timer_start(); 
     }else if((compressor.temperature >= compressor.temperature_work  && compressor.status == COMPRESSOR_STATUS_RDY )  || \
             (compressor.temperature > compressor.temperature_stop   && compressor.status == COMPRESSOR_STATUS_RDY_CONTINUE )){
        /*温度大于开机温度，同时是正常关机状态时，开机*/
        /*或者温度大于关机温度，同时是超时关机或者异常关机状态时，继续开机*/
  
        log_warning("温度:%d 高于开机温度.\r\n",compressor.temperature);
        log_warning("开压缩机.\r\n");
        compressor.status = COMPRESSOR_STATUS_WORK;
        log_warning("compressor change status to work.\r\n");  
        compressor.status_change = true;
        /*打开压缩机*/
        compressor_pwr_turn_on();
        /*打开工作定时器*/ 
        compressor_work_timer_start(); 
     }else{
        log_warning("压缩机状态:%d 无需处理.\r\n",compressor.status);     
     } 
     }
   }
   /*温度错误消息处理*/
  if(msg.type == COMPRESSOR_TASK_MSG_TEMPERATURE_ERR){ 
     compressor.temperature = msg.value;
     if(compressor.lock == false){

     if(compressor.status == COMPRESSOR_STATUS_WORK){
        /*温度异常时，如果在工作,就变更为wait continue状态*/
        compressor.status = COMPRESSOR_STATUS_WAIT_CONTINUE;
        log_warning("温度错误.code:%d.\r\n",compressor.temperature); 
        log_warning("compressor change status to wait continue.\r\n");  
        log_warning("关压缩机.\r\n");
        compressor.status_change = true;
        /*关闭压缩机和工作定时器*/
        compressor_pwr_turn_off(); 
        compressor_work_timer_stop();
        /*打开等待定时器*/ 
        compressor_wait_timer_start();  
     }else{
        log_warning("压缩机状态:%d 无需处理.\r\n",compressor.status);  
     }
     }
  }
  
  /*压缩机工作超时消息*/
  if(msg.type == COMPRESSOR_TASK_MSG_WORK_TIMEOUT){
     if(compressor.lock == false){

     if(compressor.status == COMPRESSOR_STATUS_WORK){
        log_warning("关压缩机.\r\n");
        compressor.status = COMPRESSOR_STATUS_REST;
        log_warning("compressor change status to rest.\r\n");  
        /*关闭压缩机和工作定时器*/
        compressor_pwr_turn_off();  
        compressor.status_change = true;
        /*打开休息定时器*/ 
        compressor_rest_timer_start(); 
      }else{
        log_warning("压缩机状态:%d 无需处理.\r\n",compressor.status);  
      }
     }
   }
 
  /*压缩机等待超时消息*/
  if(msg.type == COMPRESSOR_TASK_MSG_WAIT_TIMEOUT){
     if(compressor.lock == false){

     if(compressor.status == COMPRESSOR_STATUS_WAIT || compressor.status == COMPRESSOR_STATUS_WAIT_CONTINUE){
        if(compressor.status == COMPRESSOR_STATUS_WAIT){
           compressor.status = COMPRESSOR_STATUS_RDY;
           log_warning("compressor change status to rdy.\r\n");  
        }else{
           compressor.status = COMPRESSOR_STATUS_RDY_CONTINUE;
           log_warning("compressor change status to rdy continue.\r\n");  
        }
        /*构建温度消息*/
        compressor_task_send_temperature_msg_to_self();
    }else{
      log_warning("压缩机状态:%d 无需处理.\r\n",compressor.status);     
    }
   }
 }
 
  /*压缩机休息超时消息*/
  if(msg.type == COMPRESSOR_TASK_MSG_REST_TIMEOUT){
     if(compressor.lock == false){

     if(compressor.status == COMPRESSOR_STATUS_REST){
        compressor.status = COMPRESSOR_STATUS_RDY_CONTINUE;
        log_warning("compressor change status to rdy continue.\r\n");
        
        compressor_task_send_temperature_msg_to_self();
     }else{
       log_warning("压缩机状态:%d 无需处理.\r\n",compressor.status);     
     }
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

