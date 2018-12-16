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
#include "utils_httpc.h"
#include "log.h"
#define  LOG_MODULE_LEVEL    LOG_LEVEL_DEBUG
#define  LOG_MODULE_NAME     "[net]"


osThreadId  http_post_task_handle;

osMessageQId http_post_task_msg_q_id;


#define  HTTP_POST_DATA_STR    "wkxboot"

//http_client_request_t req;
//http_client_response_t res;

//char http_post_response[100];
 
int http_client_build_url(char *url,const char *url_origin,const char *sn,const uint32_t timestamp,const char *source)
{ 
#define  HTTP_CLIENT_URL_SIZE_MAX            200
 char timestamp_str[12];
 char _md5[16];
 char _md5_str[32];
  
 /*组合sn + timestamp + source作为MD5的源数据*/  
 strcpy(url,sn);
 snprintf(timestamp_str,12,"%d",timestamp);
 strcat(url,timestamp_str);
 strcat(url,source); 
  
 /*第1次MD5*/
 md5(url,strlen(url),_md5);
 /*把字节装换成HEX字符串*/
 bytes_to_hex_str(_md5,_md5_str,16);
 /*第2次MD5*/
 md5(_md5_str,32,_md5);
 bytes_to_hex_str(_md5,_md5_str,16);
 
 snprintf(url,HTTP_CLIENT_URL_SIZE_MAX,"%ssn=%s&sign=%s&source=%s&timestamp=%s",url_origin,sn,_md5_str,source,timestamp_str);
 return 0;
}
 
 
void http_post_task(void const *argument)
{
 int rc;
 uint32_t time;
 char http_post_response[200]={0};
 char url[200];
 
 const char *sn = "129DP12399787777";
 const char *source = "coolbeer";
 const char *msg= "{\"yewei\":1.0,\"yali\":200}";
 const char *url2 = "http://mh1597193030.uicp.cn/device/log/submit?";//:35787
 
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
 
http_client_build_url(url,url2,sn,time,source);

httpclient_t http_client;
httpclient_data_t   httpc_data;

memset(&httpc_data,0,sizeof(httpc_data));
memset(&http_client,0,sizeof(http_client));

http_client.header = NULL;
http_client.auth_user = NULL;
http_client.net.handle = 0;

httpc_data.is_more = 1;
httpc_data.post_content_type = "application/Json";
httpc_data.post_buf = (char *)msg;
httpc_data.post_buf_len = strlen(msg);
httpc_data.response_buf =http_post_response;
httpc_data.response_buf_len = 200;
    
rc = iotx_post(&http_client,url,35787,NULL,&httpc_data);

 if(rc != 0){
 log_error("http send err.5s retry...\r\n");
 }else{
 log_debug("http send ok.\r\n");   
 rc = httpclient_recv_response(&http_client, 5000, &httpc_data);
 if(rc == 0){
 log_debug("http recv ok.res:%s\r\n",http_post_response); 
 }else{
 log_error("http recv err.:%s\r\n"); 
 }
 }
 httpclient_close(&http_client);
 
 } 
}
