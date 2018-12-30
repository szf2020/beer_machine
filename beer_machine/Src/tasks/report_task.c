#include "FreeRTOS.h"
#include "task.h"
#include "flash_utils.h"
#include "cmsis_os.h"
#include "ntp.h"
#include "http_client.h"
#include "report_task.h"
#include "temperature_task.h"
#include "pressure_task.h"
#include "compressor_task.h"
#include "bootloader_if.h"
#include "firmware_version.h"
#include "net_task.h"
#include "tasks_init.h"
#include "utils.h"
#include "cJSON.h"
#include "device_config.h"
#include "log.h"
#define  LOG_MODULE_LEVEL    LOG_LEVEL_DEBUG
#define  LOG_MODULE_NAME     "[net]"


osThreadId  report_task_handle;
osMessageQId report_task_msg_q_id;
osMessageQId report_task_location_msg_q_id;
osMessageQId report_task_net_hal_info_msg_q_id;


osTimerId    active_timer_id;
osTimerId    log_timer_id;
osTimerId    fault_timer_id;


typedef enum
{
  HAL_FAULT_STATUS_FAULT = 0,
  HAL_FAULT_STATUS_FAULT_CLEAR,
}hal_fault_status_t;
  
typedef struct
{
  char code[4];
  char msg[6];
  char time[14];
  char status[2];
}report_fault_t;
 
typedef struct
{
  int16_t temperature;
  uint8_t capacity;
  uint8_t pressure;
  net_location_t location;
}report_log_t;

typedef struct
{
  char sn[20];
  char sim_id[20];
  char wifi_mac[20];
  char *fw_version;
  bool is_active;
}report_active_t;


typedef enum
{
  DEVICE_CONFIG_STATUS_VALID = 0x20180000U,
  DEVICE_CONFIG_STATUS_INVALID
}device_config_status_t;

typedef enum
{
  COMPRESSOR_LOCK = 0x20190000U,
  COMPRESSOR_UNLOCK
}compressor_lock_t;

/*设备运行配置信息表*/
typedef struct
{
  uint32_t report_log_interval;
  uint16_t temperature_high;
  uint16_t temperature_low;
  uint16_t pressure_high;
  uint16_t pressure_low;
  compressor_lock_t lock;
  device_config_status_t status;
}device_config_t;

typedef struct
{
  uint32_t read;
  uint32_t write;
  report_fault_t fault[REPORT_TASK_FAULT_QUEUE_SIZE];
}hal_fault_queue_t;


static hal_fault_queue_t fault_queue;

static uint32_t time_offset;
static report_active_t report_active;
static report_log_t    report_log;
static uint8_t active_event;

static device_config_t device_default_config = {
  
.temperature_low = DEFAULT_LOW_TEMPERATURE,
.temperature_high = DEFAULT_HIGH_TEMPERATURE,
.temperature_low = DEFAULT_LOW_PRESSURE,
.temperature_high = DEFAULT_HIGH_PRESSURE,
.report_log_interval = DEFAULT_REPORT_LOG_INTERVAL,
.lock = DEFAULT_REPORT_LOCK_STATUS,
.status = DEVICE_CONFIG_STATUS_VALID

};


static void report_task_active_timer_expired(void const *argument);
static void report_task_log_timer_expired(void const *argument);
static void report_task_fault_timer_expired(void const *argument);


/*激活定时器 复用为同步时间定时器*/
static void report_task_active_timer_init(uint8_t *event)
{
  osTimerDef(active_timer,report_task_active_timer_expired);
  active_timer_id = osTimerCreate(osTimer(active_timer),osTimerOnce,event);
  log_assert(active_timer_id);
}
/*启动激活定时器*/
static void report_task_start_active_timer(uint32_t timeout,uint8_t event)
{
  active_event = active_event;
  osTimerStart(active_timer_id,timeout);  
}
/*激活定时器到达处理*/
static void report_task_active_timer_expired(void const *argument)
{
  osStatus status; 
  report_task_msg_t msg;
  
  msg.type = *(uint8_t *)argument;
  
  status = osMessagePut(report_task_msg_q_id,*(uint32_t *)&msg,REPORT_TASK_PUT_MSG_TIMEOUT);
  if(status != osOK){
     log_error("put active msg error:%d\r\n",status);
  }
}
/*日志定时器*/
static void report_task_log_timer_init()
{
  osTimerDef(log_timer,report_task_log_timer_expired);
  log_timer_id = osTimerCreate(osTimer(log_timer),osTimerOnce,0);
  log_assert(log_timer_id);
}
/*启动日志定时器*/
static void report_task_start_log_timer(uint32_t timeout)
{
  osTimerStart(log_timer_id,timeout);  
}
/*日志定时器到达处理*/
static void report_task_log_timer_expired(void const *argument)
{
  osStatus status; 
  report_task_msg_t msg;
  
  msg.type = REPORT_TASK_MSG_REPORT_LOG;
  
  status = osMessagePut(report_task_msg_q_id,*(uint32_t *)&msg,REPORT_TASK_PUT_MSG_TIMEOUT);
  if(status != osOK){
     log_error("put log msg error:%d\r\n",status);
  }
}
/*故障定时器*/
static void report_task_fault_timer_init()
{
  osTimerDef(fault_timer,report_task_fault_timer_expired);
  fault_timer_id = osTimerCreate(osTimer(fault_timer),osTimerOnce,0);
  log_assert(fault_timer_id);
}
/*启动故障定时器*/
static void report_task_start_fault_timer(uint32_t timeout)
{
  osTimerStart(fault_timer_id,timeout);  
}

/*故障定时器到达处理*/
static void report_task_fault_timer_expired(void const *argument)
{
  osStatus status; 
  report_task_msg_t msg;
  
  msg.type = REPORT_TASK_MSG_REPORT_FAULT;
  
  status = osMessagePut(report_task_msg_q_id,*(uint32_t *)&msg,REPORT_TASK_PUT_MSG_TIMEOUT);
  if(status != osOK){
     log_error("put fault msg error:%d\r\n",status);
  }
}


/*获取同步后的本地UTC*/
static uint32_t report_task_get_utc()
{
 return osKernelSysTick() + time_offset;
}

  
/*构造日志数据json*/
static char *report_task_build_log_json_str(report_log_t *log)
{
  char *json_str;
  cJSON *log_json;
  cJSON *location_array,*location1,*location2,*location3,*location4;
  log_json = cJSON_CreateObject();
  cJSON_AddStringToObject(log_json,"source",SOURCE);
  cJSON_AddNumberToObject(log_json,"pressure",log->pressure / 10.0);
  cJSON_AddNumberToObject(log_json,"capacity",log->capacity);
  cJSON_AddNumberToObject(log_json,"temp",log->temperature);
  /*location数组*/
  cJSON_AddItemToObject(log_json,"location",location_array = cJSON_CreateArray());
  
  cJSON_AddItemToArray(location_array,location1 = cJSON_CreateObject());
  cJSON_AddStringToObject(location1,"lac",log->location.base_main.lac);
  cJSON_AddStringToObject(location1,"cid",log->location.base_main.ci);
  cJSON_AddStringToObject(location1,"rssi",log->location.base_main.rssi);
  
  cJSON_AddItemToArray(location_array,location2 = cJSON_CreateObject());
  cJSON_AddStringToObject(location2,"lac",log->location.base_assist1.lac);
  cJSON_AddStringToObject(location2,"cid",log->location.base_assist1.ci);
  cJSON_AddStringToObject(location2,"rssi",log->location.base_assist1.rssi);
  
  cJSON_AddItemToArray(location_array,location3 = cJSON_CreateObject());
  cJSON_AddStringToObject(location3,"lac",log->location.base_assist2.lac);
  cJSON_AddStringToObject(location3,"cid",log->location.base_assist2.ci);
  cJSON_AddStringToObject(location3,"rssi",log->location.base_assist2.rssi);
  
  cJSON_AddItemToArray(location_array,location4 = cJSON_CreateObject());
  cJSON_AddStringToObject(location4,"lac",log->location.base_assist3.lac);
  cJSON_AddStringToObject(location4,"cid",log->location.base_assist3.ci);
  cJSON_AddStringToObject(location4,"rssi",log->location.base_assist3.rssi);
  
  json_str = cJSON_PrintUnformatted(log_json);
  cJSON_Delete(log_json);
  return json_str;
 }
 

/*执行日志数据上报*/
 static int report_task_report_log(const char *url_origin,report_log_t *log,const char *sn)
 {
  //utils_build_sign(sign,5,/*errorCOde*/"2010",/*errorMsg*/"",sn,source,timestamp);
 int rc;
 uint32_t timestamp;
 http_client_context_t context;
 char timestamp_str[14] = { 0 };
 char sign_str[33] = { 0 };
 char *req;
 char rsp[200] = { 0 };
 char url[200] = { 0 };
 /*计算时间戳字符串*/
 timestamp = report_task_get_utc();
 snprintf(timestamp_str,14,"%d",timestamp);
 /*计算sign*/
 utils_build_sign(sign_str,4,KEY,sn,SOURCE,timestamp_str);
 /*构建新的url*/
 utils_build_url(url,200,url_origin,sn,sign_str,SOURCE,timestamp_str);  
   
 req = report_task_build_log_json_str(log);
 
 context.range_size = 200;
 context.range_start = 0;
 context.rsp_buffer = rsp;
 context.rsp_buffer_size = 200;
 context.url = url;
 context.timeout = 10000;
 context.user_data = (char *)req;
 context.user_data_size = strlen(req);
 context.boundary = BOUNDARY;
 context.is_form_data = false;
 context.content_type = "application/Json";//"multipart/form-data; boundary=";
 
 rc = http_client_post(&context);
 cJSON_free(req);
 
 if(rc != 0){
    log_error("report log err.\r\n");  
    return -1;
 }
 
 log_debug("report log ok.\r\n");  
 return 0;
 }
 
 
 
 /*构造激活信息数据json*/
 static char *report_task_build_active_json_str(report_active_t *active)
 {
  char *json_str;
  cJSON *active_json;
  active_json = cJSON_CreateObject();
  cJSON_AddStringToObject(active_json,"model",MODEL);
  cJSON_AddStringToObject(active_json,"sn",active->sn);
  cJSON_AddStringToObject(active_json,"firmware",active->fw_version);
  cJSON_AddStringToObject(active_json,"simId",active->sim_id);
  cJSON_AddStringToObject(active_json,"wifiMac",active->wifi_mac);
  json_str = cJSON_PrintUnformatted(active_json);
  cJSON_Delete(active_json);
  return json_str;
 }



/*解析激活返回的信息数据json*/
static int report_task_parse_active_rsp_json(char *json_str ,device_config_t *config)
{
  cJSON *active_rsp_json;
  cJSON *temp,*data,*run_config,*temperature,*pressure;
  
  log_debug("parse active rsp.\r\n");
  active_rsp_json = cJSON_Parse(json_str);
  if(active_rsp_json == NULL){
     log_error("rsp is not json.\r\n");
     return -1;  
  }
  /*检查code值 200成功*/
  temp = cJSON_GetObjectItem(active_rsp_json,"code");
  if(!cJSON_IsNumber(temp) || temp->valuestring == NULL){
     log_error("code is not num.\r\n");
     goto err_exit;  
  }
               
  log_debug("code:%s\r\n", temp->valuestring);
  if(strcmp(temp->valuestring,"200") != 0){
     log_error("active rsp err code:%d.\r\n",temp->valuestring); 
     goto err_exit;  
  }   
  /*检查success值 true or false*/
  temp = cJSON_GetObjectItem(active_rsp_json,"success");
  if(!cJSON_IsBool(temp) || !cJSON_IsTrue(temp)){
     log_error("success is not bool or is not true.\r\n");
     goto err_exit;  
  }
  log_debug("success:%s\r\n",temp->valuestring); 
  
  /*检查data */
  data = cJSON_GetObjectItem(active_rsp_json,"data");
  if(!cJSON_IsObject(data) || data->valuestring == NULL ){
     log_error("data is not obj or value is null.\r\n");
     goto err_exit;  
  }
  /*检查runConfig */ 
  run_config = cJSON_GetObjectItem(data,"runConfig");
  if(!cJSON_IsObject(run_config) || run_config->valuestring == NULL ){
     log_error("run config is not obj or value is null.\r\n");
     goto err_exit;  
  }
  /*检查log submit interval*/
  temp = cJSON_GetObjectItem(run_config,"logSubmitDur");
  if(!cJSON_IsNumber(temp) || temp->valuestring == NULL){
     log_error("logSubmitDur is not num or is null.\r\n");
     goto err_exit;  
  }
  config->report_log_interval = atoi(temp->valuestring);
  
  /*检查lock*/
  temp = cJSON_GetObjectItem(run_config,"lock");
  if(!cJSON_IsBool(temp)){ 
     log_error("lock is not bool.\r\n");
     goto err_exit;  
  }
  
  if(cJSON_IsTrue(temp)){
  config->lock = COMPRESSOR_LOCK;
  }else{
  config->lock = COMPRESSOR_UNLOCK;
  }

  /*检查temperature */ 
  temperature = cJSON_GetObjectItem(run_config,"temp");
  if(!cJSON_IsObject(temperature) || temperature->valuestring == NULL ){
     log_error("temperature is not obj or value is null.\r\n");
     goto err_exit;  
  }
  /*检查temperature min */ 
  temp = cJSON_GetObjectItem(temperature,"min");
  if(!cJSON_IsNumber(temp) || temp->valuestring == NULL){
     log_error("t min is not num or is null.\r\n");
     goto err_exit;  
  }
  config->temperature_low = atoi(temp->valuestring);
 /*检查temperature max */ 
  temp = cJSON_GetObjectItem(temperature,"max");
  if(!cJSON_IsNumber(temp) || temp->valuestring == NULL){
     log_error("t max is not num or is null.\r\n");
     goto err_exit;  
  }
  config->temperature_high = atoi(temp->valuestring);
  
  /*检查pressure*/ 
  pressure = cJSON_GetObjectItem(run_config,"pressure");
  if(!cJSON_IsObject(pressure) || pressure->valuestring == NULL ){
     log_error("pressure is not obj or value is null.\r\n");
     goto err_exit;  
  }
  /*检查pressure min */ 
  temp = cJSON_GetObjectItem(pressure,"min");
  if(!cJSON_IsNumber(temp) || temp->valuestring == NULL){
     log_error("p min is not num or is null.\r\n");
     goto err_exit;  
  }
  config->pressure_low = atoi(temp->valuestring);
 /*检查pressure max */ 
  temp = cJSON_GetObjectItem(pressure,"max");
  if(!cJSON_IsNumber(temp) || temp->valuestring == NULL){
     log_error("p max is not num or is null.\r\n");
     goto err_exit;  
  }
  config->pressure_high = atoi(temp->valuestring);
  
  log_debug("active rsp [lock:%d infointerval:%d. t_low:%d t_high:%d p_low:%d p_high:%d.\r\n",
            config->lock,config->temperature_low,config->temperature_high,config->pressure_low,config->pressure_high);

err_exit:
  cJSON_Delete(active_rsp_json);
  return -1;
}



/*执行激活信息数据上报*/  
static int report_task_report_active(const char *url_origin,report_active_t *active,device_config_t *config)
{
   //utils_build_sign(sign,5,/*errorCOde*/"2010",/*errorMsg*/"",sn,source,timestamp);
 int rc;
 uint32_t timestamp;
 http_client_context_t context;
 char timestamp_str[14] = { 0 };
 char sign_str[33] = { 0 };
 char *req;
 char rsp[200] = { 0 };
 char url[200] = { 0 };
 /*计算时间戳字符串*/
 timestamp = report_task_get_utc();
 snprintf(timestamp_str,14,"%d",timestamp);
 /*计算sign*/
 utils_build_sign(sign_str,4,KEY,active->sn,SOURCE,timestamp_str);
 /*构建新的url*/
 utils_build_url(url,200,url_origin,active->sn,sign_str,SOURCE,timestamp_str);  
   
 req = report_task_build_active_json_str(active);

 
 context.range_size = 200;
 context.range_start = 0;
 context.rsp_buffer = rsp;
 context.rsp_buffer_size = 200;
 context.url = url;
 context.timeout = 10000;
 context.user_data = (char *)req;
 context.user_data_size = strlen(req);
 context.boundary = BOUNDARY;
 context.is_form_data = false;
 context.content_type = "application/Json";
 
 rc = http_client_post(&context);
 cJSON_free(req);
 
 if(rc != 0){
    log_error("report active err.\r\n");  
    return -1;
 }
 
 rc = report_task_parse_active_rsp_json(context.rsp_buffer,config);
 if(rc != 0){
    log_error("json parse active rsp error.\r\n");  
    return -1;
 }
 log_debug("report active ok.\r\n");  
 return 0;
 }
  

/*构造故障信息json*/
static void report_task_build_fault_form_data_str(char *form_data,const uint16_t size,const report_fault_t *fault)
{
 form_data_t err_code;
 form_data_t err_msg;
 form_data_t err_time;
 form_data_t err_status;
 
 err_code.name = "errorCode";
 err_code.value = fault->code;
 
 err_msg.name = "errorMsg";
 err_msg.value = fault->msg;
 
 err_time.name = "errorTime";
 err_time.value = fault->time;
 
 err_status.name = "state";
 err_status.value = fault->status;
 
 utils_build_form_data(form_data,size,BOUNDARY,4,&err_code,&err_msg,&err_time,&err_status);
 }
  

/*解析故障上报返回的信息数据json*/
static int report_task_parse_fault_rsp_json(char *json_str)
{
  cJSON *fault_rsp_json;
  cJSON *temp;
  
  log_debug("parse fault rsp.\r\n");
  fault_rsp_json = cJSON_Parse(json_str);
  if(fault_rsp_json == NULL){
     log_error("rsp is not json.\r\n");
     return -1;  
  }
  /*检查code值 200成功*/
  temp = cJSON_GetObjectItem(fault_rsp_json,"code");
  if(!cJSON_IsNumber(temp) || temp->valuestring == NULL){
     log_error("code is not num.\r\n");
     goto err_exit;  
  }
               
  log_debug("code:%s\r\n", temp->valuestring);
  if(strcmp(temp->valuestring,"200") != 0){
     log_error("active rsp err code:%d.\r\n",temp->valuestring); 
     goto err_exit;  
  } 
  return 0;
  
err_exit:
  cJSON_Delete(fault_rsp_json);
  return -1;
}

 /*执行故障信息数据上报*/
static int report_task_report_fault(const char *url_origin,report_fault_t *fault,const char *sn)
{
   //utils_build_sign(sign,5,/*errorCOde*/"2010",/*errorMsg*/"",sn,source,timestamp);
  int rc;
  uint32_t timestamp;
  http_client_context_t context;
 
  char timestamp_str[14] = { 0 };
  char sign_str[33] = { 0 };
  char req[200];
  char rsp[200] = { 0 };
  char url[200] = { 0 };

  /*计算时间戳字符串*/
  timestamp = report_task_get_utc();
  snprintf(timestamp_str,14,"%d",timestamp);
  /*计算sign*/
  utils_build_sign(sign_str,4,fault->code,fault->msg,fault->time,fault->status,KEY,sn,SOURCE,timestamp_str);
  /*构建新的url*/
  utils_build_url(url,200,url_origin,sn,sign_str,SOURCE,timestamp_str);  
   
  report_task_build_fault_form_data_str(req,200,fault);

 
  context.range_size = 200;
  context.range_start = 0;
  context.rsp_buffer = rsp;
  context.rsp_buffer_size = 200;
  context.url = url;
  context.timeout = 10000;
  context.user_data = (char *)req;
  context.user_data_size = strlen(req);
  context.boundary = BOUNDARY;
  context.is_form_data = true;
  context.content_type = "multipart/form-data; boundary=";
 
  rc = http_client_post(&context);
 
 if(rc != 0){
    log_error("report fault err.\r\n");  
    return -1;
 }
 rc = report_task_parse_fault_rsp_json(context.rsp_buffer);
 if(rc != 0){
    log_error("json parse fault rsp error.\r\n");  
    return -1;
 }
 log_debug("report fault ok.\r\n");  
 return 0;
}

/*根据重试次数获取重试延时时间*/
static int report_task_retry_delay(uint8_t retry)
{
  if(retry == 0){
     return REPORT_TASK_RETRY1_DELAY;
  }else if(retry == 1){
     return REPORT_TASK_RETRY2_DELAY;
  }
  
  return REPORT_TASK_RETRY3_DELAY; 
}

/*读取保存在ENV中的设备配置参数*/
static int report_task_get_env_device_config(device_config_t *config)
{
  int rc;
  bootloader_env_t env;
  
  rc = bootloader_get_env(&env);
  /*读取env失败*/
  if(rc != 0 ){
     return -1;
   }
  
  /*读取env成功*/
  *config = *(device_config_t *)&env.reserved[0];
  return 0;
}

/*保存新的设备配置参数在ENV中*/
static int report_task_save_env_device_config(device_config_t *config)
{
  int rc;
  bootloader_env_t env;
  device_config_t  config_env;
  
  rc = bootloader_get_env(&env);
  /*读取env失败*/
  if(rc != 0 ){
     return -1;
   }
  
  /*读取env成功*/
  config_env = *(device_config_t *)&env.reserved[0];
  if(memcmp(&config_env,config,sizeof(device_config_t)) != 0 ){
    *(device_config_t*)&env.reserved[0] = *config;
    rc = bootloader_save_env(&env);
    /*保存env失败*/
    if(rc != 0 ){
       return -1;
    }  
  }  
  
  return 0;
}
  
  
 

/*向对应任务分发工作参数*/
static int report_task_dispatch_device_config(device_config_t *config)
{
  osStatus status;
   
  temperature_task_msg_t temperature_msg;
  pressure_task_msg_t   pressure_msg;
  compressor_task_msg_t compressor_msg;
   
  if(config->lock == COMPRESSOR_LOCK){
     compressor_msg.type = COMPRESSOR_TASK_MSG_LOCK;
  }else{
     compressor_msg.type = COMPRESSOR_TASK_MSG_UNLOCK;
  }   
   
  status = osMessagePut(compressor_task_msg_q_id,*(uint32_t *)&compressor_msg,REPORT_TASK_PUT_MSG_TIMEOUT);
  if(status != osOK){
     log_error("report task put compressor msg err.\r\n"); 
  }
   
  temperature_msg.type = TEMPERATURE_TASK_MSG_CONFIG;
  temperature_msg.value = (config->temperature_high << 8 | config->temperature_low) & 0xFFFF;
  status = osMessagePut(compressor_task_msg_q_id,*(uint32_t *)&temperature_msg,REPORT_TASK_PUT_MSG_TIMEOUT);
  if(status != osOK){
     log_error("report task put compressor msg err.\r\n"); 
  }
   
  pressure_msg.type = PRESSURE_TASK_MSG_CONFIG;
  pressure_msg.value = (config->pressure_high << 8 | config->pressure_low) & 0xFFFF;
  status = osMessagePut(compressor_task_msg_q_id,*(uint32_t *)&pressure_msg,REPORT_TASK_PUT_MSG_TIMEOUT);
  if(status != osOK){
     log_error("report task put pressure msg err.\r\n"); 
  }  
  
  return 0;
}

/*等待网络硬件消息，用来激活设备*/
static int report_task_get_net_hal_info(char *mac,char *sim_id,uint32_t timeout)
{
  osEvent os_event;
  net_hal_information_t *net_hal_info;
 
  /*处理网络模块硬件消息*/
  os_event = osMessageGet(report_task_net_hal_info_msg_q_id,timeout);
  if(os_event.status == osEventMessage){
     net_hal_info = (net_hal_information_t*)os_event.value.v; 
     strcpy(mac,net_hal_info->mac);
     strcpy(sim_id,net_hal_info->sim_id);
     return 0;
  }  
 
  return -1;
}


/*读取flash中的SN*/
static void report_task_get_sn(char *sn)
{
  int rc;
  
  uint32_t sn_str[SN_LEN / 4 + 1];
  
  flash_utils_init();
  rc = flash_utils_read(sn_str,SN_ADDR,SN_LEN / 4 + 1);
  if(rc != 0){
     sn[0] = 0; 
  }else{
     memcpy(sn,(char *)sn_str,SN_LEN);
     sn[SN_LEN] = '\0';  
  }
}
/*读取版本号*/
static void report_task_get_firmware_version(char **fw_version)
{
  *fw_version = FIRMWARE_VERSION_STR;
}

/*构造故障参数*/
static void report_task_build_fault(report_fault_t *fault,const char *code,const char *msg,const uint32_t time,const int status)
{
  char time_str[14];
  char status_str[2];
        
  snprintf(time_str,14,"%d",time);
  snprintf(status_str,2,"%d",status);
        
  strcpy(fault->code,code); 
  strcpy(fault->msg,msg); 
  strcpy(fault->time,time_str); 
  strcpy(fault->status,status_str); 
}

/*把一个故障参数放进故障参数队列*/      
static int report_task_put_fault_to_queue(hal_fault_queue_t *queue,report_fault_t *fault)
{      
  if(queue->write - queue->read >= REPORT_TASK_FAULT_QUEUE_SIZE){
     log_error("fault queue is full.\r\n");
     return -1;
  }
  queue->fault[queue->write & (REPORT_TASK_FAULT_QUEUE_SIZE - 1)] = *fault;
  queue->write++;    
  
  return 0;    
}

/*从故障参数队列取出一个故障参数*/         
int report_task_get_fault_from_queue(hal_fault_queue_t *queue,report_fault_t *fault)
{     
  if(queue->read >= queue->write){
     log_warning("fault queue is null.\r\n");
     return -1;
   }
       
  *fault = queue->fault[queue->read & (REPORT_TASK_FAULT_QUEUE_SIZE - 1)];
   queue->read++;
     
   return 0;    
}
         
/*同步UTC时间*/
static int report_task_sync_utc(uint32_t *offset)
{
  int rc;
  uint32_t cur_time;
  
  rc = ntp_sync_time(&cur_time);
  if(rc != 0){
     return -1;
  }
  
  *offset = cur_time - osKernelSysTick();
  
  return 0; 
}


/*上报任务*/
void report_task(void const *argument)
{
 int rc;
 int log_retry = 0,fault_retry = 0,active_retry = 0;
 osEvent os_event;
 report_task_msg_t msg;
 device_config_t config;

//char *msg_active= "{\"model\":\"jiuji\",\"sn\":\"129DP12399787777\",\"firmware\":\"1.0.1\",\"simId\":\"112233445566\",\"wifiMac\":\"aa:bb:cc:dd:ee:ff\"}";
 /*定时器初始化*/
 report_task_active_timer_init(&active_event);
 report_task_log_timer_init();
 report_task_fault_timer_init();
 
 /*分发默认的开机配置参数*/
 rc = report_task_get_env_device_config(&config);
 if(rc != 0  || config.status != DEVICE_CONFIG_STATUS_VALID){
    config = device_default_config;
 }
 
 report_task_dispatch_device_config(&config);

 report_task_get_firmware_version(&report_active.fw_version);
 report_task_get_sn(report_active.sn);
 
 /*等待任务同步*/
 /*
 xEventGroupSync(tasks_sync_evt_group_hdl,TASKS_SYNC_EVENT_REPORT_TASK_RDY,TASKS_SYNC_EVENT_ALL_TASKS_RDY,osWaitForever);
 log_debug("report task sync ok.\r\n");
 //snprintf(msg_log,200,"{\"source\":\"coolbeer\",\"pressure\":\"1.1\",\"capacity\":\"1\",\"temp\":4,\"location\":{\"lac\":%s,\"ci\":%s}}",reg.location.lac,reg.location.ci);
 */
 
 while(1){
 osDelay(REPORT_TASK_GET_MSG_INTERVAL);
 
 os_event = osMessageGet(report_task_msg_q_id,0);
 if(os_event.status == osEventMessage){
    msg = *(report_task_msg_t*)&os_event.value.v; 
    
    /*获取网络硬件消息*/
    if(msg.type == REPORT_TASK_MSG_NET_HAL_INFO){ 
       rc = report_task_get_net_hal_info(report_active.wifi_mac,report_active.sim_id,0);
       /*获取失败 设备离线 无法激活和进行下面的步骤 继续尝试*/
       if(rc != 0){
          log_error("report task get net hal info timeout.%d S later retry.",REPORT_TASK_RETRY_DELAY / 1000);
          report_task_start_active_timer(REPORT_TASK_RETRY_DELAY,REPORT_TASK_MSG_NET_HAL_INFO); 
       }else{
          report_task_start_active_timer(0,REPORT_TASK_MSG_SYNC_UTC); 
       }
    }
    /*同步UTC消息*/
    if(msg.type == REPORT_TASK_MSG_SYNC_UTC){ 
       rc = report_task_sync_utc(&time_offset);
       if(rc != 0){
          log_error("report task sync utc timeout.%d S later retry.",REPORT_TASK_RETRY_DELAY / 1000);
          report_task_start_active_timer(REPORT_TASK_RETRY_DELAY,REPORT_TASK_MSG_SYNC_UTC); 
       }else{
          log_warning("report task sync utc ok.\r\n");
          /*为了后面的定时同步时间*/
          if(report_active.is_active == false){
             report_task_start_active_timer(0,REPORT_TASK_MSG_ACTIVE);
          }else{
             report_task_start_active_timer(REPORT_TASK_SYNC_UTC_DELAY,REPORT_TASK_MSG_SYNC_UTC); 
          }
       }
    }
     /*设备激活消息*/
    if(msg.type == REPORT_TASK_MSG_ACTIVE){ 
       rc = report_task_report_active(URL_ACTIVE,&report_active,&config);
       if(rc != 0){
          log_error("report task active timeout.%d S later retry.",report_task_retry_delay(active_retry) / 1000);
          report_task_start_active_timer(report_task_retry_delay(active_retry),REPORT_TASK_MSG_ACTIVE); 
          if(++active_retry >= 3){
             active_retry = 0;
          }
       }else{
         active_retry = 0;
         report_active.is_active = true;
         log_warning("device active ok.\r\n");
         
         /*分发激活后的配置参数*/
         report_task_dispatch_device_config(&config);
         /*比较保存新激活的参数，*/
         report_task_save_env_device_config(&config);
 
         /*打开启日志上报定时器*/
         report_task_start_log_timer(config.report_log_interval);
         /*激活后，开启定时作为同步时间定时器*/
         report_task_start_active_timer(REPORT_TASK_SYNC_UTC_DELAY,REPORT_TASK_MSG_SYNC_UTC);
       }
    }
    
    
    /*温度消息*/
    if(msg.type == REPORT_TASK_MSG_TEMPERATURE){
       report_log.temperature = msg.value;
    }
    /*压力消息*/
    if(msg.type == REPORT_TASK_MSG_PRESSURE){
       report_log.pressure = msg.value;
    }
 
    /*容积消息*/
    if(msg.type == REPORT_TASK_MSG_CAPACITY){
       report_log.capacity = msg.value;
    }
 
    /*温度传感器故障消息*/
    if(msg.type == REPORT_TASK_MSG_TEMPERATURE_SENSOR_FAULT){    
       report_fault_t fault;
       report_task_build_fault(&fault,"2010","null",report_task_get_utc(),msg.value == 1 ? HAL_FAULT_STATUS_FAULT : HAL_FAULT_STATUS_FAULT_CLEAR);
       report_task_put_fault_to_queue(&fault_queue,&fault);  
    }
 
    /*压力传感器故障消息*/
    if(msg.type == REPORT_TASK_MSG_PRESSURE_SENSOR_FAULT){
       report_fault_t fault;     
       report_task_build_fault(&fault,"2011","null",report_task_get_utc(),msg.value == 1 ? HAL_FAULT_STATUS_FAULT : HAL_FAULT_STATUS_FAULT_CLEAR);
       report_task_put_fault_to_queue(&fault_queue,&fault);
     }
 
    /*液位传感器故障消息*/
    if(msg.type == REPORT_TASK_MSG_CAPACITY_SENSOR_FAULT){
       report_fault_t fault;
       report_task_build_fault(&fault,"2012","null",report_task_get_utc(),msg.value == 1 ? HAL_FAULT_STATUS_FAULT : HAL_FAULT_STATUS_FAULT_CLEAR);
       report_task_put_fault_to_queue(&fault_queue,&fault); 
     }
 
    /*数据上报消息*/
    if(msg.type == REPORT_TASK_MSG_REPORT_LOG){
      /*只有设备激活才可以上报数据*/
      if(report_active.is_active == true){       
         rc = report_task_report_log(URL_LOG,&report_log,report_active.sn);
         if(rc != 0){
            log_error("report task report log timeout.%d S later retry.",report_task_retry_delay(log_retry) / 1000);
            report_task_start_log_timer(report_task_retry_delay(log_retry)); 
            if(++log_retry >= 3){
               log_retry = 0;
            }
         }else{
         /*重置日志上报定时器*/
           report_task_start_log_timer(report_task_retry_delay(log_retry)); 
           log_retry = 0;
         }
      }
   }
    
   /*故障上报消息*/
   if(msg.type == REPORT_TASK_MSG_REPORT_FAULT){
      /*只有设备激活才可以上报故障*/
      if(report_active.is_active == true){    
         report_fault_t report_fault;
         /*取出故障信息*/
         rc = report_task_get_fault_from_queue(&fault_queue,&report_fault);
         if(rc == 0){
            rc = report_task_report_fault(URL_LOG,&report_fault,report_active.sn);
            if(rc != 0){
               log_error("report task report fault timeout.%d S later retry.",report_task_retry_delay(fault_retry) / 1000);
               report_task_start_fault_timer(report_task_retry_delay(fault_retry)); 
               if(++fault_retry >= 3){
                  fault_retry = 0;
               }
            }else{
              fault_retry = 0;
            }
          }
       }
   }
   }
 
   /*处理位置信息队列消息*/
   os_event = osMessageGet(report_task_location_msg_q_id,0);
   if(os_event.status == osEventMessage){
      report_log.location  = *(net_location_t*)os_event.value.v; 
   }                     
 
  }
}
