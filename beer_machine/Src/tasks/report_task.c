#include "FreeRTOS.h"
#include "task.h"
#include "cmsis_os.h"
#include "string.h"
#include "socket.h"
#include "http_client.h"
#include "report_task.h"
#include "ntp.h"
#include "tasks_init.h"
#include "utils.h"
#include "cJSON.h"
#include "log.h"
#define  LOG_MODULE_LEVEL    LOG_LEVEL_DEBUG
#define  LOG_MODULE_NAME     "[net]"


osThreadId  report_task_handle;
osMessageQId report_task_msg_q_id;


#define  URL_LOG               "http://mh1597193030.uicp.cn:35787/device/log/submit"
#define  URL_ACTIVE            "http://mh1597193030.uicp.cn:35787/device/info/active"
#define  URL_FAULT             "http://mh1597193030.uicp.cn:35787/device/info/active"
#define  BOUNDARY              "##wkxboot##"
#define  KEY                   "meiling-beer"
#define  SOURCE                "coolbeer"
#define  MODEL                 "pijiuji"
#define  FW_VERSION            "v1.0.0"

typedef struct
{
const char *code;
const char *msg;
}hal_fault_t;


typedef struct
{
int16_t temperature;
uint8_t capacity;
uint8_t pressure;
report_location_t location;
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

static report_information_t report_info;
static uint32_t time_offset;

/*温度传感器故障*/
const static hal_fault_t t_sensor_fault = {"2010","null"}; 
/*压力传感器故障*/
const static hal_fault_t p_sensor_fault = {"2011","null"}; 
/*液位传感器故障*/
const static hal_fault_t c_sensor_fault = {"2012","null"}; 

static uint32_t get_utc()
{
 return osKernelSysTick() + time_offset;
}

 static char *report_task_build_log_json_str(report_information_t *info)
 {
  char *json_str;
  cJSON *log_json;
  cJSON *location_array,*location1,*location2,*location3;
  log_json = cJSON_CreateObject();
  cJSON_AddStringToObject(log_json,"source",info->source);
  cJSON_AddNumberToObject(log_json,"pressure",info->pressure / 10.0);
  cJSON_AddNumberToObject(log_json,"capacity",info->capacity);
  cJSON_AddNumberToObject(log_json,"temp",info->temperature);
  /*location数组*/
  cJSON_AddItemToObject(log_json,"location",location_array = cJSON_CreateArray());
  cJSON_AddItemToArray(location_array,location1 = cJSON_CreateObject());
  cJSON_AddStringToObject(location1,"lac",info->location.value[0].lac);
  cJSON_AddStringToObject(location1,"cid",info->location.value[0].cid);
  cJSON_AddItemToArray(location_array,location2 = cJSON_CreateObject());
  cJSON_AddStringToObject(location2,"lac",info->location.value[1].lac);
  cJSON_AddStringToObject(location2,"cid",info->location.value[1].cid);
  cJSON_AddItemToArray(location_array,location3 = cJSON_CreateObject());
  cJSON_AddStringToObject(location3,"lac",info->location.value[2].lac);
  cJSON_AddStringToObject(location3,"cid",info->location.value[2].cid);
  json_str = cJSON_PrintUnformatted(log_json);
  cJSON_Delete(log_json);
  return json_str;
 }
 
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
  
 static int report_task_report_active(const char *url_origin,report_information_t *info)
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
 context.content_type = "application/Json";//"multipart/form-data; boundary=";
 
 rc = http_client_post(&context);
 cJSON_free(req);
 
 if(rc != 0){
 log_error("report active err.\r\n");  
 return -1;
 }
 
 log_debug("report active ok.\r\n");  
 return 0;
 }
  


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
   

 
 
 

void report_task(void const *argument)
{
 int rc;
 osEvent os_event;
 osStatus status;
 report_task_msg_t *msg;

// char *msg_active= "{\"model\":\"jiuji\",\"sn\":\"129DP12399787777\",\"firmware\":\"1.0.1\",\"simId\":\"112233445566\",\"wifiMac\":\"aa:bb:cc:dd:ee:ff\"}";

 osMessageQDef(report_task_msg_q,10,report_task_msg_t*);
 report_task_msg_q_id = osMessageCreate(osMessageQ(report_task_msg_q),report_task_handle);
 log_assert(report_task_msg_q_id); 
 
 /*等待任务同步*/
 xEventGroupSync(tasks_sync_evt_group_hdl,TASKS_SYNC_EVENT_REPORT_TASK_RDY,TASKS_SYNC_EVENT_ALL_TASKS_RDY,osWaitForever);
 log_debug("report task sync ok.\r\n");
 //snprintf(msg_log,200,"{\"source\":\"coolbeer\",\"pressure\":\"1.1\",\"capacity\":\"1\",\"temp\":4,\"location\":{\"lac\":%s,\"ci\":%s}}",reg.location.lac,reg.location.ci);
 /*初始化运行数据*/
 report_info.key = KEY;
 report_info.source = SOURCE;
 report_info.model = MODEL;
 report_info.boundary = BOUNDARY;
 report_info.url_log = URL_LOG;
 report_info.url_active = URL_ACTIVE;
 report_info.url_fault = URL_FAULT;

 while(1){

 os_event = osMessageGet(report_task_msg_q_id,osWaitForever);
 if(os_event.status == osEventMessage){
 msg = (report_task_msg_t *)os_event.value.v; 
 
 /*温度消息*/
 if(msg->type == REPORT_TASK_MSG_TEMPERATURE){
 report_info.temperature = msg->temperature;
 }
 /*压力消息*/
 if(msg->type == REPORT_TASK_MSG_PRESSURE){
 report_info.pressure = msg->pressure;
 }
 
 /*容积消息*/
 if(msg->type == REPORT_TASK_MSG_CAPACITY){
 report_info.capacity = msg->capacity;
 }
 /*位置消息*/
 if(msg->type == REPORT_TASK_MSG_LOCATION){
 report_info.location = msg->location;
 }
 
 /*数据上报消息*/
 if(msg->type == REPORT_TASK_MSG_REPORT_LOG){
 report_task_report_log(URL_LOG,&report_info);
  
 }
 
 /*开机配置获取消息*/
 if(msg->type == REPORT_TASK_MSG_CONFIG_DEVICE){
  
 }



                       

}
}
}
