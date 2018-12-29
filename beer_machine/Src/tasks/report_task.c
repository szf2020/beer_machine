#include "FreeRTOS.h"
#include "task.h"
#include "flash_utils.h"
#include "cmsis_os.h"
#include "ntp.h"
#include "http_client.h"
#include "report_task.h"
#include "pressure_task.h"
#include "compressor_task.h"
#include "bootloader_if.h"
#include "firmware_version.h"
#include "net_task.h"
#include "tasks_init.h"
#include "utils.h"
#include "cJSON.h"
#include "log.h"
#define  LOG_MODULE_LEVEL    LOG_LEVEL_DEBUG
#define  LOG_MODULE_NAME     "[net]"


osThreadId  report_task_handle;
osMessageQId report_task_msg_q_id;
osMessageQId report_task_location_msg_q_id;
osMessageQId report_task_net_hal_info_msg_q_id;

osTimerId    report_log_timer_id;
osTimerId    report_fault_timer_id;

typedef struct
{
char     *code;
char     *msg;
uint32_t time;
bool     is_fault;
bool     is_report;
}hal_fault_t;


typedef struct
{
int16_t temperature;
uint8_t capacity;
uint8_t pressure;
hal_fault_t t_sensor_fault;
hal_fault_t p_sensor_fault;
hal_fault_t c_sensor_fault;
net_location_t location;
char sn[20];
char sim_id[20];
char wifi_mac[20];
char *fw_version;
char *source;
char *model;
char *key;
char *boundary;
char *url_log;
char *url_active;
char *url_fault;
}report_information_t;

typedef enum
{
DEVICE_WORK_PARAM_STATUS_VALID = 0x20180000U,
DEVICE_WORK_PARAM_STATUS_INVALID
}device_work_param_status_t;

typedef enum
{
COMPRESSOR_LOCK = 0x20190000U,
COMPRESSOR_UNLOCK
}compressor_lock_t;

typedef struct
{
uint32_t report_log_interval;
uint16_t temperature_high;
uint16_t temperature_low;
uint16_t pressure_high;
uint16_t pressure_low;
compressor_lock_t lock;
device_work_param_status_t status;
}device_work_param_t;



static report_information_t report_info;
static uint32_t time_offset;

static void report_log_timer_expired(void const *argument);
static void report_fault_timer_expired(void const *argument);


/*获取同步后的本地UTC*/
static uint32_t get_utc()
{
 return osKernelSysTick() + time_offset;
}


/*日志数据上报定时器*/
static void report_log_timer_init()
{
 osTimerDef(report_log_timer,report_log_timer_expired);
 report_log_timer_id=osTimerCreate(osTimer(report_log_timer),osTimerOnce,0);
 log_assert(report_log_timer_id);
}

static void report_log_timer_start(uint32_t timeout)
{
 osTimerStart(report_log_timer_id,timeout);  
}
/*
static void report_log_timer_stop(void)
{
 osTimerStop(report_log_timer_id);  
}
*/

static void report_log_timer_expired(void const *argument)
{
 osStatus status;
 report_task_msg_t msg;
 /*发送日志数据上报消息*/
 msg.type = REPORT_TASK_MSG_REPORT_LOG;
 status = osMessagePut(report_task_msg_q_id,*(uint32_t*)&msg,REPORT_TASK_PUT_MSG_TIMEOUT);
 if(status !=osOK){
 log_error("put report log msg error:%d\r\n",status);
 } 
}

/*故障数据上报定时器*/
static void report_fault_timer_init()
{
 osTimerDef(report_fault_timer,report_fault_timer_expired);
 report_fault_timer_id=osTimerCreate(osTimer(report_fault_timer),osTimerOnce,0);
 log_assert(report_fault_timer_id);
}

static void report_fault_timer_start(uint32_t timeout)
{
 osTimerStart(report_fault_timer_id,timeout);  
}
/*
static void report_fault_timer_stop(void)
{
 osTimerStop(report_fault_timer_id);  
}
*/
static void report_fault_timer_expired(void const *argument)
{
 osStatus status;
 report_task_msg_t msg;
 /*发送日志数据上报消息*/
 msg.type = REPORT_TASK_MSG_REPORT_FAULT;
 status = osMessagePut(report_task_msg_q_id,*(uint32_t*)&msg,REPORT_TASK_PUT_MSG_TIMEOUT);
 if(status !=osOK){
    log_error("put report fault msg error:%d\r\n",status);
 } 
}

/*构造日志数据json*/
static char *report_task_build_log_json_str(report_information_t *info)
{
  char *json_str;
  cJSON *log_json;
  cJSON *location_array,*location1,*location2,*location3,*location4;
  log_json = cJSON_CreateObject();
  cJSON_AddStringToObject(log_json,"source",info->source);
  cJSON_AddNumberToObject(log_json,"pressure",info->pressure / 10.0);
  cJSON_AddNumberToObject(log_json,"capacity",info->capacity);
  cJSON_AddNumberToObject(log_json,"temp",info->temperature);
  /*location数组*/
  cJSON_AddItemToObject(log_json,"location",location_array = cJSON_CreateArray());
  
  cJSON_AddItemToArray(location_array,location1 = cJSON_CreateObject());
  cJSON_AddStringToObject(location1,"lac",info->location.base_main.lac);
  cJSON_AddStringToObject(location1,"cid",info->location.base_main.ci);
  cJSON_AddStringToObject(location1,"rssi",info->location.base_main.rssi);
  
  cJSON_AddItemToArray(location_array,location2 = cJSON_CreateObject());
  cJSON_AddStringToObject(location2,"lac",info->location.base_assist1.lac);
  cJSON_AddStringToObject(location2,"cid",info->location.base_assist1.ci);
  cJSON_AddStringToObject(location2,"rssi",info->location.base_assist1.rssi);
  
  cJSON_AddItemToArray(location_array,location3 = cJSON_CreateObject());
  cJSON_AddStringToObject(location3,"lac",info->location.base_assist2.lac);
  cJSON_AddStringToObject(location3,"cid",info->location.base_assist2.ci);
  cJSON_AddStringToObject(location3,"rssi",info->location.base_assist2.rssi);
  
  cJSON_AddItemToArray(location_array,location4 = cJSON_CreateObject());
  cJSON_AddStringToObject(location4,"lac",info->location.base_assist3.lac);
  cJSON_AddStringToObject(location4,"cid",info->location.base_assist3.ci);
  cJSON_AddStringToObject(location4,"rssi",info->location.base_assist3.rssi);
  
  json_str = cJSON_PrintUnformatted(log_json);
  cJSON_Delete(log_json);
  return json_str;
 }
 

/*执行日志数据上报*/
 static int report_task_report_log(const char *url_origin,report_information_t *info)
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
 timestamp = get_utc();
 snprintf(timestamp_str,14,"%d",timestamp);
 /*计算sign*/
 utils_build_sign(sign_str,4,info->key,info->sn,info->source,timestamp_str);
 /*构建新的url*/
 utils_build_url(url,200,url_origin,info->sn,sign_str,info->source,timestamp_str);  
   
 req = report_task_build_log_json_str(info);
 
 context.range_size = 200;
 context.range_start = 0;
 context.rsp_buffer = rsp;
 context.rsp_buffer_size = 200;
 context.url = url;
 context.timeout = 10000;
 context.user_data = (char *)req;
 context.user_data_size = strlen(req);
 context.boundary = info->boundary;
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
 static char *report_task_build_active_json_str(report_information_t *info)
 {
  char *json_str;
  cJSON *active_json;
  active_json = cJSON_CreateObject();
  cJSON_AddStringToObject(active_json,"model",info->model);
  cJSON_AddStringToObject(active_json,"sn",info->sn);
  cJSON_AddStringToObject(active_json,"firmware",info->fw_version);
  cJSON_AddStringToObject(active_json,"simId",info->sim_id);
  cJSON_AddStringToObject(active_json,"wifiMac",info->wifi_mac);
  json_str = cJSON_PrintUnformatted(active_json);
  cJSON_Delete(active_json);
  return json_str;
 }
 /*解析激活返回的信息数据json*/
static int report_task_parse_active_rsp_json(char *json_str ,device_work_param_t *work_param)
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
  work_param->report_log_interval = atoi(temp->valuestring);
  
  /*检查lock*/
  temp = cJSON_GetObjectItem(run_config,"lock");
  if(!cJSON_IsBool(temp)){ 
     log_error("lock is not bool.\r\n");
     goto err_exit;  
  }
  
  if(cJSON_IsTrue(temp)){
  work_param->lock = COMPRESSOR_LOCK;
  }else{
  work_param->lock = COMPRESSOR_UNLOCK;
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
  work_param->temperature_low = atoi(temp->valuestring);
 /*检查temperature max */ 
  temp = cJSON_GetObjectItem(temperature,"max");
  if(!cJSON_IsNumber(temp) || temp->valuestring == NULL){
     log_error("t max is not num or is null.\r\n");
     goto err_exit;  
  }
  work_param->temperature_high = atoi(temp->valuestring);
  
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
  work_param->pressure_low = atoi(temp->valuestring);
 /*检查pressure max */ 
  temp = cJSON_GetObjectItem(pressure,"max");
  if(!cJSON_IsNumber(temp) || temp->valuestring == NULL){
     log_error("p max is not num or is null.\r\n");
     goto err_exit;  
  }
  work_param->pressure_high = atoi(temp->valuestring);
  
  log_debug("active rsp [lock:%d infointerval:%d. t_low:%d t_high:%d p_low:%d p_high:%d.\r\n",
            work_param->lock,work_param->temperature_low,work_param->temperature_high,work_param->pressure_low,work_param->pressure_high);

err_exit:
  cJSON_Delete(active_rsp_json);
  return -1;
}



/*执行激活信息数据上报*/  
static int report_task_report_active(const char *url_origin,report_information_t *info,device_work_param_t *work_param)
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
 timestamp = get_utc();
 snprintf(timestamp_str,14,"%d",timestamp);
 /*计算sign*/
 utils_build_sign(sign_str,4,info->key,info->sn,info->source,timestamp_str);
 /*构建新的url*/
 utils_build_url(url,200,url_origin,info->sn,sign_str,info->source,timestamp_str);  
   
 req = report_task_build_active_json_str(info);

 
 context.range_size = 200;
 context.range_start = 0;
 context.rsp_buffer = rsp;
 context.rsp_buffer_size = 200;
 context.url = url;
 context.timeout = 10000;
 context.user_data = (char *)req;
 context.user_data_size = strlen(req);
 context.boundary = info->boundary;
 context.is_form_data = false;
 context.content_type = "application/Json";
 
 rc = http_client_post(&context);
 cJSON_free(req);
 
 if(rc != 0){
    log_error("report active err.\r\n");  
    return -1;
 }
 
 rc = report_task_parse_active_rsp_json(context.rsp_buffer,work_param);
 if(rc != 0){
    log_error("json parse active rsp error.\r\n");  
    return -1;
 }
 log_debug("report active ok.\r\n");  
 return 0;
 }
  

/*构造故障信息json*/
static void report_task_build_fault_form_data_str(char *form_data,const uint16_t size,const char *code,const char *msg,report_information_t *info)
{
 form_data_t err_code;
 form_data_t err_msg;

 err_code.name = "errorCode";
 err_code.value = code;
 
 err_msg.name = "errorMsg";
 err_msg.value = msg;
 
 utils_build_form_data(form_data,size,info->boundary,2,&err_code,&err_msg);
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
static int report_task_report_fault(const char *url_origin,hal_fault_t *hal_fault,report_information_t *info)
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
 timestamp = get_utc();
 snprintf(timestamp_str,14,"%d",timestamp);
 /*计算sign*/
 utils_build_sign(sign_str,4,hal_fault->code,hal_fault->msg,info->key,info->sn,info->source,timestamp_str);
 /*构建新的url*/
 utils_build_url(url,200,url_origin,info->sn,sign_str,info->source,timestamp_str);  
   
 report_task_build_fault_form_data_str(req,200,hal_fault->code,hal_fault->msg,info);

 
 context.range_size = 200;
 context.range_start = 0;
 context.rsp_buffer = rsp;
 context.rsp_buffer_size = 200;
 context.url = url;
 context.timeout = 10000;
 context.user_data = (char *)req;
 context.user_data_size = strlen(req);
 context.boundary = info->boundary;
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

   
/*同步网络时间服务器*/
static void report_task_sync_utc()
{
 int rc;
 uint32_t cur_utc;  
 do{
 rc = ntp_sync_time(&cur_utc);
 if(rc != 0){
    osDelay(10000);
 }
 }while(rc != 0);
 
 time_offset = cur_utc - osKernelSysTick();
 log_debug("sync utc ok.\r\n");
 }


/*开机设备激活*/
static void report_task_device_active(report_information_t *report_info,device_work_param_t *work_param)
{
 int rc;
 uint8_t  device_active_retry = REPORT_TASK_DEVICE_ACTIVE_RETRY;
 bootloader_env_t env;
 device_work_param_t param_temp,param_default,param_active;
 
 param_default.temperature_low = COMPRESSOR_STOP_TEMPERATURE;
 param_default.temperature_high = COMPRESSOR_WORK_TEMPERATURE;
 param_default.report_log_interval = REPORT_TASK_REPORT_LOG_INTERVAL;
 param_default.status = DEVICE_WORK_PARAM_STATUS_VALID;
 
 do{
 rc = report_task_report_active(URL_ACTIVE,report_info,&param_active);   
 if(rc != 0){
    osDelay(REPORT_TASK_RETRY_INTERVAL);
 }
 }while(rc != 0 && -- device_active_retry > 0);

  rc = bootloader_get_env(&env);
  if(rc != 0 ){
     if(device_active_retry == 0){
        param_temp = param_default;
     }else{
        param_temp = param_active;
     }
  }else{
     param_temp = *(device_work_param_t *)&env.reserved[0];   
     if(param_temp.status != DEVICE_WORK_PARAM_STATUS_VALID){ 
        param_temp = param_default;
     }
     /*激活成功，使用新参数*/
     if(device_active_retry > 0){
        if( memcmp(&param_active, &param_temp,sizeof(device_work_param_t))){      
          *(device_work_param_t *)&env.reserved[0] = param_active;
          bootloader_save_env(&env); 
          param_temp = param_active;     
         }
     }
  }

  *work_param = param_temp; 
  log_warning("device work param t_high:%d t_low:%d report_interval:%d.\r\n",param_temp.temperature_high,param_temp.temperature_low,param_temp.report_log_interval);  
}

/*向对应任务分发工作参数*/
static int report_task_dispatch_work_param(device_work_param_t *work_param)
{
   osStatus status;
   
   compressor_task_msg_t compressor_msg;
   pressure_task_msg_t   pressure_msg;
   
   if(work_param->lock == COMPRESSOR_LOCK){
      compressor_msg.type = COMPRESSOR_TASK_MSG_LOCK;
   }else{
      compressor_msg.type = COMPRESSOR_TASK_MSG_UNLOCK;
   }   
   
   status = osMessagePut(compressor_task_msg_q_id,*(uint32_t *)&compressor_msg,REPORT_TASK_PUT_MSG_TIMEOUT);
   if(status != osOK){
      log_error("report task put compressor msg err.\r\n"); 
   }
   
   compressor_msg.type = COMPRESSOR_TASK_MSG_WORK_PARAM;
   compressor_msg.value = (work_param->temperature_high << 8 | work_param->temperature_low) & 0xFFFF;
   status = osMessagePut(compressor_task_msg_q_id,*(uint32_t *)&compressor_msg,REPORT_TASK_PUT_MSG_TIMEOUT);
   if(status != osOK){
      log_error("report task put compressor msg err.\r\n"); 
   }
   
   pressure_msg.type = COMPRESSOR_TASK_MSG_WORK_PARAM;
   pressure_msg.value = (work_param->pressure_high << 8 | work_param->pressure_low) & 0xFFFF;
   status = osMessagePut(compressor_task_msg_q_id,*(uint32_t *)&pressure_msg,REPORT_TASK_PUT_MSG_TIMEOUT);
   if(status != osOK){
      log_error("report task put pressure msg err.\r\n"); 
   }  
   
  return 0;
}

static void report_task_get_net_hal_info(char *mac,char *sim_id)
{
 osEvent os_event;
 net_hal_information_t *net_hal_info;
 
 /*处理网络模块硬件消息*/
 os_event = osMessageGet(report_task_net_hal_info_msg_q_id,osWaitForever);
 if(os_event.status == osEventMessage){
   net_hal_info = (net_hal_information_t*)os_event.value.v; 
   strcpy(mac,net_hal_info->mac);
   strcpy(sim_id,net_hal_info->sim_id);
 }  
}

#define  SN_LEN                     18
#define  SN_ADDR                    0x803FF00

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

static void report_task_get_firmware_version(char **fw_version)
{
  *fw_version = FIRMWARE_VERSION_STR;
}



void report_task(void const *argument)
{
 int rc;

 osEvent os_event;
 osStatus status;
 report_task_msg_t msg;
 device_work_param_t work_param;
 

// char *msg_active= "{\"model\":\"jiuji\",\"sn\":\"129DP12399787777\",\"firmware\":\"1.0.1\",\"simId\":\"112233445566\",\"wifiMac\":\"aa:bb:cc:dd:ee:ff\"}";
 report_log_timer_init();
 report_fault_timer_init();

 /*初始化运行数据*/
 report_info.key = KEY;
 report_info.source = SOURCE;
 report_info.model = MODEL;
 report_info.boundary = BOUNDARY;
 
 report_task_get_firmware_version(&report_info.fw_version);
 report_task_get_sn(report_info.sn);
 
 report_task_get_net_hal_info(report_info.wifi_mac,report_info.sim_id);


 
 /*等待任务同步*/
 xEventGroupSync(tasks_sync_evt_group_hdl,TASKS_SYNC_EVENT_REPORT_TASK_RDY,TASKS_SYNC_EVENT_ALL_TASKS_RDY,osWaitForever);
 log_debug("report task sync ok.\r\n");
 //snprintf(msg_log,200,"{\"source\":\"coolbeer\",\"pressure\":\"1.1\",\"capacity\":\"1\",\"temp\":4,\"location\":{\"lac\":%s,\"ci\":%s}}",reg.location.lac,reg.location.ci);


 report_task_sync_utc();
 
 report_task_device_active(&report_info,&work_param);
 report_task_dispatch_work_param(&work_param);
 

 
 /*开机配置获取激活消息*/
 report_log_timer_start(REPORT_TASK_REPORT_LOG_INTERVAL);
 
 while(1){
 osDelay(REPORT_TASK_GET_MSG_INTERVAL);
 
 os_event = osMessageGet(report_task_msg_q_id,0);
 if(os_event.status == osEventMessage){
 msg = *(report_task_msg_t*)&os_event.value.v; 
 


 
 /*温度消息*/
 if(msg.type == REPORT_TASK_MSG_TEMPERATURE){
    report_info.temperature = msg.temperature;
 }
 /*压力消息*/
 if(msg.type == REPORT_TASK_MSG_PRESSURE){
    report_info.pressure = msg.pressure;
 }
 
 /*容积消息*/
 if(msg.type == REPORT_TASK_MSG_CAPACITY){
    report_info.capacity = msg.capacity;
 }
 
 /*温度传感器故障消息*/
 if(msg.type == REPORT_TASK_MSG_TEMPERATURE_SENSOR_FAULT){
   if(report_info.t_sensor_fault.is_fault == false){
     report_info.t_sensor_fault.is_fault = true;
     report_info.t_sensor_fault.msg = "null";
     report_info.t_sensor_fault.code = "2010";
     report_info.t_sensor_fault.time = get_utc();
     status = osMessagePut(report_task_location_msg_q_id,REPORT_TASK_MSG_REPORT_FAULT,REPORT_TASK_PUT_MSG_TIMEOUT);
     if(status != osOK){
        log_error("put report temperature fault msg error:%d\r\n",status);
     }   
 }
 }
 
 /*压力传感器故障消息*/
 if(msg.type == REPORT_TASK_MSG_PRESSURE_SENSOR_FAULT){
    if(report_info.p_sensor_fault.is_fault == false){
    report_info.p_sensor_fault.is_fault = true;
    report_info.p_sensor_fault.msg = "null";
    report_info.p_sensor_fault.code = "2011";
    report_info.p_sensor_fault.time = get_utc();
    status = osMessagePut(report_task_location_msg_q_id,REPORT_TASK_MSG_REPORT_FAULT,REPORT_TASK_PUT_MSG_TIMEOUT);
    if(status != osOK){
      log_error("put report pressure fault msg error:%d\r\n",status);
   }  
 }
 }
 
 /*液位传感器故障消息*/
 if(msg.type == REPORT_TASK_MSG_CAPACITY_SENSOR_FAULT){
    if(report_info.c_sensor_fault.is_fault == false){
    report_info.c_sensor_fault.is_fault = true;
    report_info.c_sensor_fault.msg = "null";
    report_info.c_sensor_fault.code = "2012";
    report_info.c_sensor_fault.time = get_utc();
    status = osMessagePut(report_task_location_msg_q_id,REPORT_TASK_MSG_REPORT_FAULT,REPORT_TASK_PUT_MSG_TIMEOUT);
    if(status != osOK){
       log_error("put report capacity fault msg error:%d\r\n",status);
   }   
 }
 }
 
  /*数据上报消息*/
 if(msg.type == REPORT_TASK_MSG_REPORT_LOG){
    rc = report_task_report_log(URL_LOG,&report_info);
    if(rc != 0){
    log_error("report log fail.retry %ds later.\r\n",REPORT_TASK_RETRY_INTERVAL);  
    report_log_timer_start(REPORT_TASK_RETRY_INTERVAL);
    }
  
 }
  /*故障上报消息*/
 if(msg.type == REPORT_TASK_MSG_REPORT_FAULT){
     /*温度传感器故障上报*/
    if(report_info.t_sensor_fault.is_fault == true && report_info.t_sensor_fault.is_report == false){
    rc = report_task_report_fault(URL_LOG,&report_info.t_sensor_fault,&report_info);
    if(rc != 0){
    log_error("report log fail.retry %ds later.\r\n",REPORT_TASK_RETRY_INTERVAL);  
    report_fault_timer_start(REPORT_TASK_RETRY_INTERVAL);
    }else{
    report_info.t_sensor_fault.is_fault = false;
    report_info.t_sensor_fault.is_report = true;  
    }
    }
    /*压力传感器故障上报*/
    if(report_info.p_sensor_fault.is_fault == true && report_info.p_sensor_fault.is_report == false){
    rc = report_task_report_fault(URL_LOG,&report_info.p_sensor_fault,&report_info);
    if(rc != 0){
    log_error("report log fail.retry %ds later.\r\n",REPORT_TASK_RETRY_INTERVAL);  
    report_fault_timer_start(REPORT_TASK_RETRY_INTERVAL);
    }else{
    report_info.p_sensor_fault.is_fault = false;
    report_info.p_sensor_fault.is_report = true;  
    }
    }
    /*液位传感器故障上报*/
    if(report_info.c_sensor_fault.is_fault == true && report_info.c_sensor_fault.is_report == false){
    rc = report_task_report_fault(URL_LOG,&report_info.c_sensor_fault,&report_info);
    if(rc != 0){
    log_error("report log fail.retry %ds later.\r\n",REPORT_TASK_RETRY_INTERVAL);  
    report_fault_timer_start(REPORT_TASK_RETRY_INTERVAL);
    }else{
    report_info.c_sensor_fault.is_fault = false;
    report_info.c_sensor_fault.is_report = true;  
    }
    }    
 }
 }
 
 /*处理位置信息队列消息*/
 os_event = osMessageGet(report_task_location_msg_q_id,0);
 if(os_event.status == osEventMessage){
    report_info.location  = *(net_location_t*)os_event.value.v; 
 }                     

 
 
}
}
