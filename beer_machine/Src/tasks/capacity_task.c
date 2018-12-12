#include "cmsis_os.h"
#include "serial.h"
#include "tasks_init.h"
#include "alarm_task.h"
#include "pressure_task.h"
#include "display_task.h"
#include "capacity_task.h"
#include "log.h"
#define  LOG_MODULE_LEVEL    LOG_LEVEL_DEBUG
#define  LOG_MODULE_NAME     "[capacity]"

osThreadId   capacity_task_handle;
osMessageQId capacity_task_msg_q_id;

extern int capacity_serial_handle;
extern serial_hal_driver_t capacity_serial_driver;


typedef struct
{
uint16_t high;
float    value;
int8_t   dir;
uint8_t  err_cnt;
bool     alarm;
bool     change;
bool     blink;
}beer_capacity_t;

static beer_capacity_t beer_capacity;

/* 函数名：gsm_m6312_serial_hal_init
*  功能：  m6312 2g模块执行硬件和串口初始化
*  参数：  无 
*  返回：  GSM_ERR_OK：成功 其他：失败
*/
static int capacity_serial_hal_init(void)
{
	int rc ;
    rc = serial_create(&capacity_serial_handle,CAPACITY_BUFFER_SIZE,CAPACITY_BUFFER_SIZE);
    if (rc != 0) {
    	log_error("capacity create serial hal err.\r\n");
    	return -1;
    }
    log_debug("capacity create serial hal ok.\r\n");
   
    rc = serial_register_hal_driver(capacity_serial_handle,&capacity_serial_driver);
	if (rc != 0) {
      log_error("capacity register serial hal driver err.\r\n");
      return -1;
    }
	log_debug("capacity register serial hal driver ok.\r\n");
	rc = serial_open(capacity_serial_handle,CAPACITY_SERIAL_PORT,CAPACITY_SERIAL_BAUDRATES,CAPACITY_SERIAL_DATA_BITS,CAPACITY_SERIAL_STOP_BITS);
	if (rc != 0) {
    	log_error("capacity open serial hal err.\r\n");
    	return -1;
  	}
	log_debug("capacity open serial hal ok.\r\n");
    
    return 0;
}

#define  CAPACITY_TASK_SENSOR_ERR_VALUE            0xFFFF


static uint16_t capacity_task_get_high(void)
{
  int select_size,read_size,read_total = 0;
  char buffer[4];
  uint16_t high;
  
  /*默认工作时间间隔2S 设置4s超时*/
  select_size = serial_select(capacity_serial_handle,4000);
  
  if(select_size <= 0){
  return CAPACITY_TASK_SENSOR_ERR_VALUE;
  }
  /*读到数据处理*/
  do{
  read_size = serial_read(capacity_serial_handle,(uint8_t *) buffer + read_total,select_size); 
  if(read_size < 0){
  return CAPACITY_TASK_SENSOR_ERR_VALUE;
  }
  read_total +=read_size; 
  select_size = serial_select(capacity_serial_handle,100);
  if(select_size < 0){
  return CAPACITY_TASK_SENSOR_ERR_VALUE;  
  }
  }while(select_size != 0);
  
  /*协议头错误 或者校验错误*/
  if (read_total != 4 || buffer[0] != 0xff || (( buffer[0] + buffer[1] + buffer[2]) & 0xff) != buffer[3]){
  log_error("液位传感器协议数据错误.\r\n");
  return CAPACITY_TASK_SENSOR_ERR_VALUE;  
  }
  /*故障清除*/
  beer_capacity.err_cnt = 0;  
  high = (buffer[1] * 256 + buffer[2]) & 0xFFFF;
  if(high == 0){
  log_warning("液位传感器没检测到液位.\r\n");
  }  
  
  log_debug("当前液位：%d mm.\r\n",high);
  return high;
}



void capacity_task(void const *argument)
{
  osStatus status;
  uint16_t high;
  pressure_task_msg_t pressure_msg;
  display_task_msg_t  display_msg;
  alarm_task_msg_t    alarm_msg; 

  /*串口驱动初始化*/
  capacity_serial_hal_init();
  /*等待任务同步*/
  xEventGroupSync(tasks_sync_evt_group_hdl,TASKS_SYNC_EVENT_CAPACITY_TASK_RDY,TASKS_SYNC_EVENT_ALL_TASKS_RDY,osWaitForever);
  log_debug("capacity task sync ok.\r\n");
  /*上电默认值*/
  beer_capacity.value = 88;
  
  while(1){
  high = capacity_task_get_high(); 
  /*如果液位高度没有变化就继续*/
  if(high == beer_capacity.high){
  continue;
  }
  
  /*如果传感器异常一定时间打开报警和闪烁*/
  if(high == CAPACITY_TASK_SENSOR_ERR_VALUE){
  beer_capacity.err_cnt ++;
  if( beer_capacity.err_cnt >= CAPACITY_TASK_ERR_CNT){
  beer_capacity.err_cnt = 0;
  beer_capacity.change = true;
  beer_capacity.high = high;
  beer_capacity.value = CAPACITY_TASK_ERR_VALUE;
  beer_capacity.alarm = true;
  beer_capacity.blink = true;  
  }
  }else{
  high > beer_capacity.high ? beer_capacity.dir ++ : beer_capacity.dir --;
  if(beer_capacity.dir > CAPACITY_TASK_DIR_CHANGE_CNT || beer_capacity.dir < -CAPACITY_TASK_DIR_CHANGE_CNT ){
  beer_capacity.change = true;
  /*容量传感器无异常关闭报警*/
  beer_capacity.alarm = false;
  beer_capacity.high = high;
  beer_capacity.value = beer_capacity.high * S / 1000000.0 ;/*单位L*/   
  /*如果低于警告值就打开闪烁*/
  if((uint8_t)beer_capacity.value <= CAPACITY_TASK_CAPACITY_WARNING_VALUE && beer_capacity.blink == false){
  beer_capacity.blink = true;
  /*如果高于警告值就关闭闪烁*/
  }else if((uint8_t)beer_capacity.value > CAPACITY_TASK_CAPACITY_WARNING_VALUE && beer_capacity.blink == true){
  beer_capacity.blink = false;
  }
  }
  }
  
  if(beer_capacity.change == true){
  beer_capacity.change = false;
  /*发送显示消息*/
  display_msg.type = DISPLAY_TASK_MSG_CAPACITY;
  display_msg.value = (uint16_t)beer_capacity.value;  
  display_msg.blink = beer_capacity.blink;  
  status = osMessagePut(display_task_msg_q_id,*(uint32_t*)&display_msg,CAPACITY_TASK_PUT_MSG_TIMEOUT);
  if(status != osOK){
  log_error("capacity put display task msg err:%d.\r\n",status);
  } 
  
  display_msg.type = DISPLAY_TASK_MSG_CAPACITY_LEVEL;
  if(beer_capacity.value ==  CAPACITY_TASK_ERR_VALUE){
  display_msg.value = 5;  
  }else{
  display_msg.value = (uint16_t)beer_capacity.value / 4;    
  }
  display_msg.blink = beer_capacity.blink;  
  status = osMessagePut(display_task_msg_q_id,*(uint32_t*)&display_msg,CAPACITY_TASK_PUT_MSG_TIMEOUT);
  if(status != osOK){
  log_error("capacity put display task msg err:%d.\r\n",status);
  } 
  
  
  /*发送报警消息*/   
  alarm_msg.type = ALARM_TASK_MSG_CAPACITY;
  alarm_msg.alarm = (uint16_t)beer_capacity.alarm;  
  status = osMessagePut(alarm_task_msg_q_id,*(uint32_t*)&alarm_msg,CAPACITY_TASK_PUT_MSG_TIMEOUT);
  if(status != osOK){
  log_error("capacity put alarm task msg err:%d.\r\n",status);
  } 
  
  /*发送压力消息*/  
  pressure_msg.type = PRESSURE_TASK_MSG_CAPACITY;
  pressure_msg.value = (uint16_t)beer_capacity.value;  
  status = osMessagePut(pressure_task_msg_q_id,*(uint32_t*)&pressure_msg,CAPACITY_TASK_PUT_MSG_TIMEOUT);
  if(status != osOK){
  log_error("capacity put pressure task msg err:%d.\r\n",status);
  } 
  
  }  
  }
  
}