#include "cmsis_os.h"
#include "tasks_init.h"
#include "adc_task.h"
#include "alarm_task.h"
#include "pressure_task.h"
#include "display_task.h"
#include "capacity_task.h"
#include "log.h"
#define  LOG_MODULE_LEVEL    LOG_LEVEL_DEBUG
#define  LOG_MODULE_NAME     "[pressure]"

osThreadId   pressure_task_handle;
osMessageQId pressure_task_msg_q_id;


typedef struct
{
uint8_t value;
bool    change;
bool    blink;
bool    alarm;
bool    point;
int8_t  dir;
uint8_t capacity;
}pressure_t;

static pressure_t  pressure;


static uint8_t get_pressure(uint16_t adc)
{
 float v,p;
 if(adc == ADC_TASK_ADC_ERR_VALUE){
 return PRESSURE_ERR_VALUE_SENSOR;
 }
 v= (adc * PRESSURE_SENSOR_REFERENCE_VOLTAGE)/PRESSURE_SENSOR_ADC_VALUE_MAX;   
 p= (v - PRESSURE_SENSOR_OUTPUT_VOLTAGE_MIN) * (PRESSURE_SENSOR_INPUT_PA_MAX - PRESSURE_SENSOR_INPUT_PA_MIN)/(PRESSURE_SENSOR_OUTPUT_VOLTAGE_MAX - PRESSURE_SENSOR_OUTPUT_VOLTAGE_MIN) + PRESSURE_SENSOR_INPUT_PA_MIN;
 p= (p / PA_VALUE_PER_1KG_CM2) * 10;

 log_one_line("v:%d mv p:%d.%d kg/cm2.",(uint16_t)(v*1000),(uint8_t)p / 10,(uint8_t)p % 10);
 /*减去外部大气压*/
 if(p >= PRESSURE_VALUE_IN_KG_CM2_ERR_MAX ){
 return PRESSURE_ERR_VALUE_OVER_HIGH;  
 }
 
 if(p <= PRESSURE_VALUE_IN_KG_CM2_ERR_MIN ){
 return PRESSURE_ERR_VALUE_OVER_LOW;  
 }
 
 p = p < PRESSURE_VALUE_STANDARD_ATM ? 0 : p - PRESSURE_VALUE_STANDARD_ATM;
 return (uint8_t)p;
}

void pressure_task(void const *argument)
{
  uint8_t p;
  uint16_t adc;
  osEvent os_msg;
  osStatus status;
  
  pressure_task_msg_t msg;
  display_task_msg_t  display_msg;
  alarm_task_msg_t    alarm_msg; 
  
  osMessageQDef(pressure_msg_q,6,uint32_t);
  pressure_task_msg_q_id = osMessageCreate(osMessageQ(pressure_msg_q),pressure_task_handle);
  log_assert(pressure_task_msg_q_id);
  
  /*等待任务同步*/
  xEventGroupSync(tasks_sync_evt_group_hdl,TASKS_SYNC_EVENT_PRESSURE_TASK_RDY,TASKS_SYNC_EVENT_ALL_TASKS_RDY,osWaitForever);
  log_debug("pressure task sync ok.\r\n");
  /*上电默认值*/
  pressure.value = 88;
  
  while(1){
  os_msg = osMessageGet(pressure_task_msg_q_id,PRESSURE_TASK_MSG_WAIT_TIMEOUT);
  if(os_msg.status == osEventMessage){
  msg = *(pressure_task_msg_t*)&os_msg.value.v;
  
   /*压力ADC完成消息处理*/
  if(msg.type == PRESSURE_TASK_MSG_ADC_COMPLETED){
  adc = msg.value;
  p = get_pressure(adc); 
  /*如果压力值无变化 继续*/
  if(p == pressure.value){
  continue;  
  }
  /*如果压力值有变化处理下面步骤*/
  if(p == PRESSURE_ERR_VALUE_SENSOR || p == PRESSURE_ERR_VALUE_OVER_LOW || p == PRESSURE_ERR_VALUE_OVER_HIGH){   
  pressure.dir = 0;
  pressure.value = p;
  pressure.change = true;
  /*压力异常闪烁和报警不显示小数点*/
  pressure.alarm = true;
  pressure.blink = true;
  pressure.point = false;
  log_error("pressure err.code:0x%2x.\r\n",pressure.value);
  }else {
  if(p > pressure.value){
  pressure.dir += 1;    
  }else if(p < pressure.value){
  pressure.dir -= 1;      
  }  
  /*当满足条件时 接受数据变化*/
  if(pressure.dir > PRESSURE_TASK_PRESSURE_CHANGE_CNT ||
     pressure.dir < -PRESSURE_TASK_PRESSURE_CHANGE_CNT){
  pressure.dir = 0;
  pressure.value = p;
  pressure.change = true; 
  /*压力正常范围不报警，显示小数点*/
  pressure.point = true;
  pressure.alarm = false;
  /*压力低于报警值 同时桶内有酒闪烁*/
  if( pressure.value <= PRESSURE_VALUE_IN_KG_CM2_WARNING_MIN || pressure.value >= PRESSURE_VALUE_IN_KG_CM2_WARNING_MAX){
  pressure.blink = true;
  /*恢复正常 解除报警*/
  }else if (pressure.value > PRESSURE_VALUE_IN_KG_CM2_WARNING_MIN && pressure.value < PRESSURE_VALUE_IN_KG_CM2_WARNING_MAX){
  pressure.blink = false;
  }
  }
  }
  }
      
  /*容量变化消息处理*/
  if(msg.type == PRESSURE_TASK_MSG_CAPACITY){
  pressure.capacity = (uint8_t)msg.value;  
  }    
  
      
  /*所有消息引起的状态变化处理*/
  if(pressure.change == true){
  log_debug("pressure change. dir :%d  value:%d kg/cm2.\r\n" ,pressure.dir,p);  
  pressure.change = false;
  /*显示消息*/
  display_msg.type = DISPLAY_TASK_MSG_PRESSURE;
  display_msg.value = pressure.value;
  display_msg.blink = pressure.blink;
  status = osMessagePut(display_task_msg_q_id,*(uint32_t*)&display_msg,PRESSURE_TASK_PUT_MSG_TIMEOUT);  
  if(status !=osOK){
  log_error("put display msg error:%d\r\n",status); 
  } 
  
  display_msg.type = DISPLAY_TASK_MSG_PRESSURE_POINT;
  display_msg.value = pressure.point == true ? 1 : 0 ;
  display_msg.blink = pressure.blink;
  status = osMessagePut(display_task_msg_q_id,*(uint32_t*)&display_msg,PRESSURE_TASK_PUT_MSG_TIMEOUT);  
  if(status !=osOK){
  log_error("put display point msg error:%d\r\n",status); 
  }
  
  /*报警消息*/
  alarm_msg.type = ALARM_TASK_MSG_PRESSURE;
  alarm_msg.alarm = pressure.alarm;
  status = osMessagePut(alarm_task_msg_q_id,*(uint32_t*)&alarm_msg,PRESSURE_TASK_PUT_MSG_TIMEOUT);  
  if(status !=osOK){
  log_error("put alarm msg error:%d\r\n",status); 
  } 
  }
    
  
 }
 }
}