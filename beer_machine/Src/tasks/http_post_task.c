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
#include "md5.h"
#include "utils_httpc.h"
#include "stdarg.h"
#include "log.h"
#define  LOG_MODULE_LEVEL    LOG_LEVEL_DEBUG
#define  LOG_MODULE_NAME     "[net]"


osThreadId  http_post_task_handle;

osMessageQId http_post_task_msg_q_id;


#define  HTTP_POST_DATA_STR    "wkxboot"

//http_client_request_t req;
//http_client_response_t res;

//char http_post_response[100];
 
int utils_build_sign(char *sign,const int cnt,...)//const char *url_origin,const char *sn,const uint32_t timestamp,const char *source)
{ 
#define  SIGN_SRC_BUFFER_SIZE_MAX               300
 int size = 0;
 char sign_src[SIGN_SRC_BUFFER_SIZE_MAX] = { 0 };
 char md5_hex[16];
 char md5_str[33];
 va_list ap;
 char *temp;

 va_start(ap, cnt);
 /*组合MD5的源数据,根据输入数据*/  
 for(uint8_t i=0; i < cnt; i++){
 temp = va_arg(ap,char*);
 /*保证数据不溢出*/
 if(size + strlen(temp) >= SIGN_SRC_BUFFER_SIZE_MAX){
 return -1;
 }
 size += strlen(temp);
 strcat(sign_src,temp);
 if(i < cnt){
 strcat(sign_src,"&");
 }
 }
 
 /*第1次MD5*/
 md5(sign_src,strlen(sign_src),md5_hex);
 /*把字节装换成HEX字符串*/
 bytes_to_hex_str(md5_hex,md5_str,16);
 /*第2次MD5*/
 md5(md5_str,32,md5_hex);
 bytes_to_hex_str(md5_hex,md5_str,16);
 strcpy(sign,md5_str);
 
 return 0;
}

static int utils_build_url(char *url,const int size,const char *origin_url,const char *sn,const char *sign,const char *source,const char *timestamp)
{
snprintf(url,size,"%s?sn=%s&sign=%s&source=%s&timestamp=%s",origin_url,sn,sign,source,timestamp);
if(strlen(url) == size - 1){
log_error("url size:%d too large.\r\n",size - 1); 
return -1;
}
return 0;
}
 

static http_client_context_t context;

void http_post_task(void const *argument)
{
 int rc;
 uint32_t time;
 char http_post_response[200]={0};
 char url[200] = { 0 };
 char sign[33];
 char timestamp[14];
 const char *sn = "129DP12399787777";
 const char *source = "coolbeer";
 const char *msg= "{\"errorCode\":2010,\"errorMsg\":\"null\"}";
 const char *url2 = "http://mh1597193030.uicp.cn:35787/device/log/submit";//
 
 //osMessageQDef(http_post_task_msg_q,3,uint32_t);
 
 //http_post_task_msg_q_id = osMessageCreate(osMessageQ(http_post_task_msg_q),http_post_task_handle);
 //log_assert(http_post_task_msg_q_id); 
 
  /*等待任务同步*/
 xEventGroupSync(tasks_sync_evt_group_hdl,TASKS_SYNC_EVENT_HTTP_POST_TASK_RDY,TASKS_SYNC_EVENT_ALL_TASKS_RDY,osWaitForever);
 log_debug("http post task sync ok.\r\n");
 

while(1){
osDelay(5000);

//const char *url1 = "http://syll-test.mymlsoft.com/common/service.execute.json";//:8083

retry_ntp:
 rc = ntp_sync_time(&time);
 if(rc != 0){
 osDelay(1000);
 goto retry_ntp;
 }
 
 memset(sign,0,33);
 memset(url,0,200);
 memset(timestamp,0,14);
 snprintf(timestamp,14,"%d",time);
 
 utils_build_sign(sign,5,/*errorCOde*/"2010",/*errorMsg*/"null",sn,source,timestamp);
 utils_build_url(url,200,url2,sn,sign,source,timestamp);
 
 context.range_size = 200;
 context.range_start = 0;
 context.rsp_buffer = http_post_response;
 context.rsp_buffer_size = 200;
 context.url = url;
 context.timeout = 5000;
 context.user_data = (char *)msg;
 context.user_data_size = strlen(msg);

 rc = http_client_post(&context);

 if(rc != 0){
 log_error("http send err.5s retry...\r\n");
 }else{
 log_debug("http send ok.\r\n");   
 } 
}
}
