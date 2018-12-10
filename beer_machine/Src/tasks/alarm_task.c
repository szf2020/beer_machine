#include "cmsis_os.h"
#include "tasks_init.h"
#include "temperature_task.h"
#include "pressure_task.h"
#include "beer_machine.h"
#include "alarm_task.h"
#include "log.h"
#define  LOG_MODULE_LEVEL    LOG_LEVEL_ERROR
#define  LOG_MODULE_NAME     "[alarm]"

osThreadId   alarm_task_handle;
osMessageQId alarm_task_msg_q_id;
osTimerId    alarm_timer_id;

typedef struct
{
bool alarm;              
}alarm_unit_t;

typedef struct
{
bool         is_pwr_on;
alarm_unit_t temperature;
alarm_unit_t pressure;
alarm_unit_t capacity;
}alarm_t;

static  alarm_t  alarm;


/*蜂鸣器通电断电时间 单位:ms*/
#define  ALARM_TASK_TIMER_TIMEOUT             1000

#define  ALARM_TASK_PUT_MSG_TIMEOUT           5
#define  ALARM_TASK_MSG_WAIT_TIMEOUT          osWaitForever


static void alarm_timer_expired(void const *argument);

static void alarm_buzzer_pwr_turn_on();
static void alarm_buzzer_pwr_turn_off();




static void alarm_buzzer_pwr_turn_on()
{
 bsp_buzzer_ctrl_on();
}

static void alarm_buzzer_pwr_turn_off()
{
 bsp_buzzer_ctrl_off();
}

static void alarm_timer_init()
{
 osTimerDef(alarm_timer,alarm_timer_expired);
 alarm_timer_id=osTimerCreate(osTimer(alarm_timer),osTimerPeriodic,0);
 log_assert(alarm_timer_id);
}

static void alarm_timer_start(void)
{
 osTimerStart(alarm_timer_id,ALARM_TASK_TIMER_TIMEOUT);  
 log_debug("报警定时器启动.\r\n");
}

static void alarm_timer_expired(void const *argument)
{
 osStatus status;
 alarm_task_msg_t alarm_timeout_msg;
 
 log_debug("报警定时器到达.\r\n");
 alarm_timeout_msg.type = ALARM_TASK_MSG_TIMER_TIMEOUT;
 status = osMessagePut(alarm_task_msg_q_id,*(uint32_t*)&alarm_timeout_msg,ALARM_TASK_PUT_MSG_TIMEOUT);  
 if(status !=osOK){
 log_error("put alarm timeout msg error:%d\r\n",status); 
 } 
}

void alarm_task(void const *argument)
{
  osEvent  os_msg;
  alarm_task_msg_t msg;
  
  osMessageQDef(alarm_msg_q,6,uint32_t);
  alarm_task_msg_q_id = osMessageCreate(osMessageQ(alarm_msg_q),alarm_task_handle);
  log_assert(alarm_task_msg_q_id);

  /*等待任务同步*/
  xEventGroupSync(tasks_sync_evt_group_hdl,TASKS_SYNC_EVENT_ALARM_TASK_RDY,TASKS_SYNC_EVENT_ALL_TASKS_RDY,osWaitForever);
  log_debug("alarm task sync ok.\r\n");
  
  alarm_buzzer_pwr_turn_off();
  alarm.is_pwr_on = false;
  
  alarm_timer_init();
  alarm_timer_start();

  while(1){ 
  os_msg = osMessageGet(alarm_task_msg_q_id,ALARM_TASK_MSG_WAIT_TIMEOUT); 
  if(os_msg.status == osEventMessage){
  
  msg = *(alarm_task_msg_t*)&os_msg.value.v;

  /*温度报警消息处理*/ 
  if(msg.type == ALARM_TASK_MSG_TEMPERATURE){
  alarm.temperature.alarm = msg.alarm;
  }
  
  /*压力报警消息处理*/ 
  if(msg.type == ALARM_TASK_MSG_PRESSURE){
  alarm.pressure.alarm = msg.alarm;
  }
  
  /*液位报警消息处理*/ 
  if(msg.type == ALARM_TASK_MSG_CAPACITY){
  alarm.capacity.alarm = msg.alarm;
  }
  
  /*液位报警消息处理*/ 
  if(msg.type == ALARM_TASK_MSG_TIMER_TIMEOUT){
  /*如果有任意一个报警状态存在 就继续操作蜂鸣器*/
  /*  
  if(alarm.temperature.alarm == true || alarm.pressure.alarm == true || alarm.capacity.alarm == true){
  if(alarm.is_pwr_on == true){
  alarm.is_pwr_on = false;
  alarm_buzzer_pwr_turn_off();
  }else{
  alarm.is_pwr_on = true;
  alarm_buzzer_pwr_turn_on();
  }
  }else if(alarm.is_pwr_on != false){
  alarm.is_pwr_on = false;
  alarm_buzzer_pwr_turn_off();     
  }
 */
  }

  }
  }
}