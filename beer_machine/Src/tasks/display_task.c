#include "cmsis_os.h"
#include "led.h"
#include "tasks_init.h"
#include "adc_task.h"
#include "compressor_task.h"
#include "alarm_task.h"
#include "pressure_task.h"
#include "temperature_task.h"
#include "display_task.h"
#include "log.h"
#define  LOG_MODULE_LEVEL    LOG_LEVEL_DEBUG
#define  LOG_MODULE_NAME     "[display]"

osThreadId   display_task_handle;
osMessageQId display_task_msg_q_id;
osTimerId    display_timer_id;


typedef struct
{
display_unit_t temperature;
display_unit_t pressure;
display_unit_t capacity;
display_unit_t wifi;
display_unit_t compressor;
bool is_turn_on;
bool is_update;
}display_t;

static display_t display = {
.wifi.value = 1,
.wifi.blink = true
};

static void display_timer_expired(void const *argument);

static void display_timer_init()
{
 osTimerDef(display_timer,display_timer_expired);
 display_timer_id=osTimerCreate(osTimer(display_timer),osTimerPeriodic,0);
 log_assert(display_timer_id);
}

static void display_timer_start(void)
{
 osTimerStart(display_timer_id,DISPLAY_TASK_BLINK_TIMER_TIMEOUT);  
 log_debug("显示定时器开始.\r\n");
}
/*
static void display_timer_stop(void)
{
 osTimerStop(display_timer_id);  
 log_debug("显示定时器停止.\r\n");  
}
*/
static void display_timer_expired(void const *argument)
{
 osStatus status;
 display_task_msg_t blink_msg;
 blink_msg.type = DISPLAY_TASK_MSG_BLINK;
 status = osMessagePut(display_task_msg_q_id,*(uint32_t*)&blink_msg,DISPLAY_TASK_PUT_MSG_TIMEOUT);
 if(status !=osOK){
 log_error("put blink msg error:%d\r\n",status); 
 }
}

 

void display_task(void const *argument)
{
 osEvent os_msg;
 display_task_msg_t msg;
 
 /*等待显示芯片上电稳定*/;
 osDelay(200);
 led_display_init();
 
 led_display_temperature_unit(LED_DISPLAY_ON);
 led_display_pressure_unit(LED_DISPLAY_ON);
 led_display_capacity_unit(LED_DISPLAY_ON);
 
 led_display_temperature_icon(LED_DISPLAY_ON);
 led_display_pressure_icon(LED_DISPLAY_ON);
 led_display_capacity_icon_frame(LED_DISPLAY_ON);
 
 led_display_pressure_point(LED_DISPLAY_ON);
 led_display_capacity_icon_level(5);
 

 led_display_wifi_icon(LED_DISPLAY_ON);
 led_display_compressor_icon(LED_DISPLAY_ON,LED_DISPLAY_ON);
 led_display_brand_icon(LED_DISPLAY_ON);
 
 /*依次显示 8-7....0.*/
 for(int8_t dis=88;dis >=0 ;dis-=11){
 led_display_temperature(dis);
 led_display_pressure(dis);
 led_display_capacity(dis);
 
 led_display_refresh();
 osDelay(150);
 }
  /*默认显示容积0L*/
  led_display_capacity(display.capacity.value);
  led_display_capacity_icon_level(display.capacity.value / 4);
 
  osMessageQDef(display_msg_q,8,uint32_t);
  display_task_msg_q_id = osMessageCreate(osMessageQ(display_msg_q),display_task_handle);
  log_assert(display_task_msg_q_id);
  /*等待任务同步*/
  xEventGroupSync(tasks_sync_evt_group_hdl,TASKS_SYNC_EVENT_DISPLAY_TASK_RDY,TASKS_SYNC_EVENT_ALL_TASKS_RDY,osWaitForever);
  log_debug("display task sync ok.\r\n");
  display_timer_init();
  display_timer_start();
  
  while(1){
  os_msg = osMessageGet(display_task_msg_q_id,DISPLAY_TASK_MSG_WAIT_TIMEOUT);
  if(os_msg.status == osEventMessage){
  msg = *(display_task_msg_t*)&os_msg.value.v;
  /*温度消息*/
  if(msg.type == DISPLAY_TASK_MSG_TEMPERATURE){
  display.temperature.value = msg.value;
  display.temperature.blink = msg.blink;   
  led_display_temperature(display.temperature.value);
  display.is_update = true;
  }
  /*压力消息*/
  if(msg.type == DISPLAY_TASK_MSG_PRESSURE){
  display.pressure.value = msg.value;   
  display.pressure.blink = msg.blink;
  /*闪烁的时候把小数点去掉*/
  if(display.pressure.blink == true){
  led_display_pressure_point(LED_DISPLAY_OFF);
  }else{
  led_display_pressure_point(LED_DISPLAY_ON);  
  }
  led_display_pressure(display.pressure.value);
  display.is_update = true;
  }
  /*容量消息*/
  if(msg.type == DISPLAY_TASK_MSG_CAPACITY){
  display.capacity.value = msg.value;
  display.capacity.blink = msg.blink; 
  led_display_capacity(display.capacity.value);
  led_display_capacity_icon_level(display.capacity.value / 4);
  display.is_update = true;
  }
  /*wifi消息*/
  if(msg.type == DISPLAY_TASK_MSG_WIFI){
  display.wifi.value = msg.value; 
  display.wifi.blink = msg.blink; 
  led_display_wifi_icon(display.wifi.value);
  display.is_update = true;
  }
  
  /*压缩机消息*/
  if(msg.type == DISPLAY_TASK_MSG_COMPRESSOR){
  display.compressor.blink = msg.blink; 
  led_display_compressor_icon(LED_DISPLAY_ON,LED_DISPLAY_ON);
  display.is_update = true;
  }
  
  /*闪烁消息*/
  if(msg.type == DISPLAY_TASK_MSG_BLINK){
  if(display.is_turn_on == true){
  /*闪烁进入灯关闭阶段*/
  display.is_turn_on = false;
  
  if(display.temperature.blink == true){
  led_display_temperature(LED_NULL_VALUE);
  display.is_update = true;
  }
  
  if(display.pressure.blink == true){
  led_display_pressure(LED_NULL_VALUE);
  display.is_update = true;
  }
  
  if(display.capacity.blink == true){
  led_display_capacity(LED_NULL_VALUE);
  led_display_capacity_icon_level(LED_DISPLAY_OFF);  
  display.is_update = true;
  }
  
  if(display.wifi.blink == true){
  led_display_wifi_icon(LED_DISPLAY_OFF);
  display.is_update = true;
  }
  
  if(display.compressor.blink == true){
  led_display_compressor_icon(LED_DISPLAY_OFF,LED_DISPLAY_OFF);
  display.is_update = true;
  }

  }else{
  /*闪烁进入灯打开阶段*/
  display.is_turn_on = true;
  
  if(display.temperature.blink == true){
  led_display_temperature(display.temperature.value);
  display.is_update = true;
  }
  
  if(display.pressure.blink == true){
  led_display_pressure(display.pressure.value);
  display.is_update = true;
  }
  
  if(display.capacity.blink == true){
  led_display_capacity(display.capacity.value);
  led_display_capacity_icon_level(display.capacity.value / 4);
  display.is_update = true;
  }
  
  if(display.wifi.blink == true){
  led_display_wifi_icon(display.wifi.value);
  display.is_update = true;
  }
  
  if(display.compressor.blink == true){
  led_display_compressor_icon(LED_DISPLAY_ON,LED_DISPLAY_ON);  
  display.is_update = true;
  }
  }
  } 
  
  /*更新上述消息内容到芯片*/
  if(display.is_update == true){
  display.is_update = false;  
  /*刷新到芯片RAM*/
  led_display_refresh(); 
  
  } 
  }
  }
}
   
   
  
