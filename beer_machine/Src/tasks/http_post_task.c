#include "FreeRTOS.h"
#include "task.h"
#include "cmsis_os.h"
#include "string.h"
#include "connection.h"
#include "http_client.h"
#include "http_post_task.h"
#include "ntp.h"
#include "tasks_init.h"
#include "utils.h"
#include "md5.h"
#include "log.h"
#define  LOG_MODULE_LEVEL    LOG_LEVEL_DEBUG
#define  LOG_MODULE_NAME     "[net]"


osThreadId  http_post_task_handle;

osMessageQId http_post_task_msg_q_id;


#define  HTTP_POST_DATA_STR    "wkxboot"

http_client_request_t req;
http_client_response_t res;

 char http_post_src_buffer[100];
 char http_post_src_hex_buffer[100];
 char md5_hex[16];
 char md5_hex_str[33];
 
void http_post_task(void const *argument)
{
 int rc;
 uint32_t time;
 char http_post_response[200]={0};

 char timestamp_str[12]={0};
 const char *sn = "129DP12399787777";
 const char *source = "coolbeer";
 const char *msg= "{\"yewei\":1.0,\"yali\":200}";
 const char *url2 = "http://mh1597193030.uicp.cn:35787/device/log/submit?";
 
 //osMessageQDef(http_post_task_msg_q,3,uint32_t);
 
 //http_post_task_msg_q_id = osMessageCreate(osMessageQ(http_post_task_msg_q),http_post_task_handle);
 //log_assert(http_post_task_msg_q_id); 
 
  /*等待任务同步*/
 xEventGroupSync(tasks_sync_evt_group_hdl,TASKS_SYNC_EVENT_HTTP_POST_TASK_RDY,TASKS_SYNC_EVENT_ALL_TASKS_RDY,osWaitForever);
 log_debug("http post task sync ok.\r\n");
while(1){
osDelay(5000);

//const char *url = "http://syll-test.mymlsoft.com:8083/common/service.execute.json";

retry_ntp:
 rc = ntp_sync_time(&time);
 if(rc != 0){
 osDelay(1000);
 goto retry_ntp;
 }


 memset(http_post_src_hex_buffer,0,100);
 memset(http_post_src_buffer,0,100);
 
 strcat(http_post_src_buffer,sn);
 snprintf(timestamp_str,12,"%d",time);
 strcat(http_post_src_buffer,timestamp_str);
 strcat(http_post_src_buffer,source);

 /*第一次MD5*/
 md5(http_post_src_buffer,strlen(http_post_src_buffer),md5_hex);
 /*把字节装换成HEX字符串*/
 bytes_to_hex_str(md5_hex,md5_hex_str,16);
 strcpy(http_post_src_hex_buffer,md5_hex_str);
 md5(md5_hex_str,32,md5_hex);
 bytes_to_hex_str(md5_hex,md5_hex_str,16);

 
 req.body = msg;
 req.body_size = strlen(msg);
 req.range.start = 0;
 req.range.size = 199;
 req.param.sn = sn;
 req.param.sign = md5_hex_str;
 req.param.source = source;
 req.param.timestamp = timestamp_str;
 res.body_buffer_size = 199;
 res.body = http_post_response;
 res.timeout = 20000;


 rc = http_client_post(url2,&req,&res);
 if(rc != 0){
 log_error("http post err.5s retry...\r\n");
 }
 log_debug("http post ok.res:%s\r\n",res.body); 
 } 
}
