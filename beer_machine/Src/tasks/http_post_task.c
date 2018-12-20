#include "FreeRTOS.h"
#include "task.h"
#include "cmsis_os.h"
#include "string.h"
#include "socket.h"
#include "http_client.h"
#include "http_post_task.h"
#include "ntp.h"
#include "tasks_init.h"
#include "utils.h"
#include "utils_httpc.h"
#include "log.h"
#define  LOG_MODULE_LEVEL    LOG_LEVEL_DEBUG
#define  LOG_MODULE_NAME     "[net]"


osThreadId  http_post_task_handle;

osMessageQId http_post_task_msg_q_id;


#define  HTTP_POST_DATA_STR    "wkxboot"

static http_client_context_t context;

void http_post_task(void const *argument)
{
 int rc;
 gsm_m6312_register_t reg;
 uint32_t time;
 char http_post_response[200]={0};
 char url[200] = { 0 };
 char sign[33];
 char timestamp[14];
 const char *sn = "129DP12399787777";
 const char *source = "coolbeer";
 char msg_log[200];
// char *msg_active= "{\"model\":\"jiuji\",\"sn\":\"129DP12399787777\",\"firmware\":\"1.0.1\",\"simId\":\"112233445566\",\"wifiMac\":\"aa:bb:cc:dd:ee:ff\"}";
 const char *url2 = "http://mh1597193030.uicp.cn:35787/device/log/submit";///info/active";//
 const char *boundary = "##########wkxboot";
 const char *key = "meiling-beer";
 //osMessageQDef(http_post_task_msg_q,3,uint32_t);
 
 //http_post_task_msg_q_id = osMessageCreate(osMessageQ(http_post_task_msg_q),http_post_task_handle);
 //log_assert(http_post_task_msg_q_id); 
 
  /*等待任务同步*/
 xEventGroupSync(tasks_sync_evt_group_hdl,TASKS_SYNC_EVENT_HTTP_POST_TASK_RDY,TASKS_SYNC_EVENT_ALL_TASKS_RDY,osWaitForever);
 log_debug("http post task sync ok.\r\n");
 

while(1){
osDelay(10000);

//const char *url1 = "http://syll-test.mymlsoft.com/common/service.execute.json";//:8083

retry:
 strcpy(reg.location.lac,"null");
 strcpy(reg.location.ci,"null");;
 rc = gsm_m6312_get_reg_location(&reg);

 if(rc != 0){
 osDelay(1000);  
 goto retry;
 }
 memset(msg_log,0,200);
 snprintf(msg_log,200,"{\"source\":\"coolbeer\",\"pressure\":\"1.1\",\"capacity\":\"1\",\"temp\":4,\"location\":{\"lac\":%s,\"ci\":%s}}",reg.location.lac,reg.location.ci);
 rc = ntp_sync_time(&time);
 if(rc != 0){
 osDelay(1000);
 goto retry;
 }
 
 memset(sign,0,33);
 memset(url,0,200);
 memset(timestamp,0,14);
 //memset(msg,0,200);
 memset(http_post_response,0,200);
 snprintf(timestamp,14,"%d",time);
 
 //utils_build_sign(sign,5,/*errorCOde*/"2010",/*errorMsg*/"",sn,source,timestamp);
 
 utils_build_sign(sign,4,key,sn,source,timestamp);
 utils_build_url(url,200,url2,sn,sign,source,timestamp);
 /*
 form_data_t err_code;
 form_data_t err_msg;
 
 err_code.name = "errorCode";
 err_code.value = "2010";
 
 err_msg.name = "errorMsg";
 err_msg.value = "";
 */
 
 //utils_build_form_data(msg,300,boundary,2,&err_code,&err_msg);
                       
                       
 context.range_size = 200;
 context.range_start = 0;
 context.rsp_buffer = http_post_response;
 context.rsp_buffer_size = 200;
 context.url = url;
 context.timeout = 10000;
 context.user_data = (char *)msg_log;
 context.user_data_size = strlen(msg_log);
 context.boundary = (char*)boundary;
 context.is_form_data = false;
 context.content_type = "application/Json";//"multipart/form-data; boundary=";
 rc = http_client_post(&context);

 if(rc != 0){
 log_error("http send err.5s retry...\r\n");
 osDelay(5000);
 }else{
 log_debug("http send ok.\r\n");  
 printf("res:\r\n%s.\r\n",http_post_response);
 } 
}
}
