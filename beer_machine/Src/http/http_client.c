/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * Martin d'Allens <martin.dallens@gmail.com> wrote this file. As long as you retain
 * this notice you can do whatever you want with this stuff. If we meet some day,
 * and you think this stuff is worth it, you can buy me a beer in return.
 * ----------------------------------------------------------------------------
 */

// FIXME: sprintf->snprintf everywhere.
#include "stdbool.h"
#include "stddef.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "stdint.h"
#include "socket.h"
#include "http_client.h"
#include "comm_utils.h"
#include "cmsis_os.h"
#include "log.h"
#define  LOG_MODULE_LEVEL    LOG_LEVEL_DEBUG
#define  LOG_MODULE_NAME     "[http_client]"


#define  HTTP_CLIENT_ASSERT_NULL_POINTER(x)     \
if((x) == NULL){                                \
return -1;                                      \
}

#define  HEAD_START0_STR                    "HTTP/1.0 "
#define  HEAD_START1_STR                    "HTTP/1.1 "
#define  HEAD_END_STR                       "\r\n\r\n"


static int http_client_parse_url(const char *url,char *host,uint16_t *port,char *path)
{
  char *pos;
  char *host_str;
  char *port_str;
  char *path_str;
  uint8_t host_str_len;
  uint8_t path_str_len;
  
  if(url == NULL ||host == NULL ||port == NULL||path == NULL){
  log_error("null pointer.\r\n");
  } 
  /*找到host*/
  pos = strstr(url,"http://");
  HTTP_CLIENT_ASSERT_NULL_POINTER(pos);
  host_str = pos + strlen("http://");
  pos = strstr(host_str,":");
  HTTP_CLIENT_ASSERT_NULL_POINTER(pos);
  host_str_len = pos - host_str;
  if(host_str_len > HTTP_CLIENT_HOST_STR_LEN){
  log_error("host str len:%d is too large.\r\n",host_str_len);
  return -1;
  }
  memcpy(host,host_str,host_str_len);
  host[host_str_len]= '\0';
  /*找到port*/
  port_str =  pos + strlen(":");
  *port = strtol(port_str,NULL,10);
  /*找到path*/
  path_str = strstr(port_str,"/");
  HTTP_CLIENT_ASSERT_NULL_POINTER(path_str);
  path_str_len = strlen(path_str);
  if(path_str_len > HTTP_CLIENT_PATH_STR_LEN){
  log_error("path str len:%d is too large.\r\n",path_str_len);
  return -1;
  } 
  strcpy(path,path_str);
  
  return 0; 
}


 
static int http_client_build_head(char *buffer,const char *method,http_client_context_t *context,int size)
{
  int head_size;
  const char *http_version = "HTTP/1.1";
  snprintf(buffer,size,
        /*method----path----http_version*/
         "%s "     "%s "  "%s\r\n"
        /*host----port*/
		 "Host: %s:%d\r\n"
		 "Connection: keep-alive\r\n"
		 "User-Agent: wkxboot-gsm-wifi.\r\n"
         /*content type*/
		 "Content-Type: %s%s\r\n"
		 "Content-Length: %d\r\n"
         "Range: bytes=%d-%d\r\n"
		 "\r\n",
		 method,context->path,http_version,
         context->host,context->port,
         context->content_type,
         context->is_form_data ? context->boundary : "",
         context->user_data_size,
         context->range_start,context->range_start + context->range_size -1);
         head_size = strlen(buffer);
         log_debug("head:%s\r\n",buffer);        
  
 return head_size;     
}

typedef struct
{
bool up;
uint32_t start;
uint32_t value;
}http_client_timer_t;

static int http_client_timer_init(http_client_timer_t *timer,uint32_t timeout,bool up)
{
if(timer == NULL){
return -1;
}

timer->up = up;
timer->start = osKernelSysTick();  
timer->value = timeout;  

return 0;
}

static int http_client_timer_value(http_client_timer_t *timer)
{
uint32_t time_elapse;

if(timer == NULL){
return -1;
}  
time_elapse = osKernelSysTick() - timer->start; 

if(timer->up == true){
return  timer->value > time_elapse ? time_elapse : timer->value;
}

return  timer->value > time_elapse ? timer->value - time_elapse : 0; 
}


static int http_client_recv_head(http_client_context_t *context,char *buffer,uint16_t buffer_size,const uint32_t timeout)
{
  int rc;
  int recv_total = 0;
  char *tmp_ptr,*encode_type;
  int encode_type_size;
  http_client_timer_t timer;
 
  http_client_timer_init(&timer,timeout,false);
 
  /*获取http head*/
  do{
  rc = socket_recv(context->handle,buffer + recv_total,1,http_client_timer_value(&timer));
  if(rc < 0){
  log_error("http header recv err.\r\n");
  return -1;
  }
 
  if(rc != 1){
  /*超时返回*/
  log_error("recv head timeout.\r\n");
  return -1;  
  }
 
  recv_total += 1;
  buffer[recv_total] = '\0';
  /*尝试解析head*/
  if((strstr(buffer,HEAD_START0_STR) || strstr(buffer,HEAD_START1_STR)) &&  strstr(buffer,HEAD_END_STR)){
  log_debug("find header.\r\n"); 
  context->head_size = strstr(buffer,HEAD_END_STR) + strlen(HEAD_END_STR) - buffer;
  /*找到content size*/
  if (NULL != (tmp_ptr = strstr(buffer, "Content-Length"))) {
  context->content_size = atoi(tmp_ptr + strlen("Content-Length: "));
  context->is_chunked = false;
  log_debug("content not chunk. size is %d.\r\n",context->content_size);  
  return 0;
  }else if (NULL != (tmp_ptr = strstr(buffer, "Transfer-Encoding: "))) {
  encode_type_size = strlen("Chunked");
  encode_type =(char *) tmp_ptr + strlen("Transfer-Encoding: ");
  if ((! memcmp(encode_type, "Chunked", encode_type_size))
   || (! memcmp(encode_type, "chunked", encode_type_size))) {
  context->content_size = 0;/*this mean data is chunked*/   
  context->is_chunked = true;
  log_debug("content is chunked.\r\n"); 
  return 0;
  }
  }else{
  log_error("header is invalid.\r\n");
  return -1;/*will stop parse*/
  }
  } 
  }while( recv_total < buffer_size - 1);

  log_error("recv head buffer is overflow.\r\n");
  return -1;
 }
 

#define  CHUNK_CODE_SIZE_MAX    5
static int http_client_recv_chunk_size(http_client_context_t *context,uint32_t timeout)
{
  int rc;
  int recv_total = 0;
  char chunk_code[CHUNK_CODE_SIZE_MAX];
  http_client_timer_t timer;
  
  http_client_timer_init(&timer,timeout,false);
  context->chunk_size = 0;
  /*获取chunk code*/
  do{
  rc = socket_recv(context->handle,chunk_code + recv_total,1,http_client_timer_value(&timer));
  if(rc < 0){
  log_error("http chunk code recv err.\r\n");
  return -1;
  }
 
  if(rc == 0){
  /*超时返回*/
  log_error("recv chunk code timeout.\r\n");
  return -1;  
  }
  recv_total += 1;
  if(recv_total > 2){
  chunk_code[recv_total] = '\0';
  
  /*尝试解析chunk code*/
  if(chunk_code[recv_total - 2] == '\r' && chunk_code[recv_total - 1] == '\n'){
  /*编码是16进制hex*/
  context->chunk_size = strtol(chunk_code,NULL,16);
  log_debug("find chunk code:%x.\r\n",context->chunk_size); 
  return 0;
  }
  }
  }while(recv_total < CHUNK_CODE_SIZE_MAX - 1);
                                                          
  log_error("recv chunk code not find .\r\n");                                                        
  return -1;                                                         
}                                                 
 

static int http_client_recv_chun_tail(http_client_context_t *context,uint32_t timeout)
{
  int rc;
  http_client_timer_t timer;
  char tail[2];
  
  http_client_timer_init(&timer,timeout,false);  
  
  rc = socket_recv(context->handle,tail ,2,http_client_timer_value(&timer));
  if(rc < 0){
  log_error("http chunk code recv err.\r\n");
  return -1;
  }
 
  if(rc != 2){
  /*超时返回*/
  log_error("recv chunk code timeout.\r\n");
  return -1;  
  }
  
  if(tail[0] != '\r' || tail[1] != '\n'){
    
    
  /*超时返回*/
  log_error("chunk format err.\r\n");
  return -1;  
  } 
  return 0;  
  }
  



static int http_client_recv_chunk(http_client_context_t *context,uint32_t timeout)
{
  int rc;
  http_client_timer_t timer;
  
  http_client_timer_init(&timer,timeout,false);
  
  context->content_size = 0;
  do{
  rc = http_client_recv_chunk_size(context,http_client_timer_value(&timer));
  if(rc != 0){
  return -1;   
  }
  if(context->chunk_size == 0){
  /*接收chunk tail*/
  rc = http_client_recv_chun_tail(context,http_client_timer_value(&timer));
  if(rc != 0){
  return -1;   
  }
  
  context->rsp_buffer[context->content_size] = '\0';
  log_debug("last chunk.\r\n");
  return 0; 
  }
  if(context->chunk_size > context->rsp_buffer_size - context->content_size - 1){
  log_error("chunk size:%d large than free buffer size:%d.\r\n",context->chunk_size,context->rsp_buffer_size - context->content_size - 1);
  return -1;   
  }
  
  rc = socket_recv(context->handle,context->rsp_buffer + context->content_size,context->chunk_size,http_client_timer_value(&timer));
  if(rc < 0){
  log_error("http chunk code recv err.\r\n");
  return -1;
  }
 
  if(rc != context->chunk_size){
  /*超时返回*/
  log_error("recv chunk code timeout.\r\n");
  return -1;  
  }
  /*接收chunk tail*/
  rc = http_client_recv_chun_tail(context,http_client_timer_value(&timer));
  if(rc != 0){
  return -1;   
  }
  /*更新content size*/
  context->content_size +=context->chunk_size;
  
  }while(context->content_size);/*消除编译警告*/
  /*不可到达*/
  return -1;
}

/*http 请求*/
static int http_client_request(const char *method,http_client_context_t *context)
{
 int rc;
 int head_size;
 char *http_buffer;
 http_client_timer_t timer;
 
 http_client_timer_init(&timer,context->timeout,false);

 /*申请http缓存*/
 http_buffer = HTTP_CLIENT_MALLOC(HTTP_BUFFER_SIZE);
 if(http_buffer == NULL){
 log_error("http malloc buffer err.\r\n");
 return -1;
 }
 /*解析url*/
 rc = http_client_parse_url(context->url,context->host,&context->port,context->path);
 if(rc != 0){
 log_error("http client parse url error.\r\n");
 goto err_handler;
 }
 /*构建http head缓存*/
 head_size = http_client_build_head(http_buffer,method,context,HTTP_BUFFER_SIZE);
 if(head_size < 0){
 log_error("http build head err.\r\n");
 goto err_handler;
 }
 /*组合body*/
 //strcat(http_buffer,context->user_data);
 //head_size +=strlen(context->user_data);
 /*http 连接*/
 rc = socket_connect(context->host,context->port,SOCKET_PROTOCOL_TCP);
 if(rc < 0){
 goto err_handler;
 }
 context->handle = rc;
 context->connected = true;
 
 /*http head发送*/
 rc = socket_send(context->handle,http_buffer,head_size,http_client_timer_value(&timer));
 if(rc != head_size){
 log_error("http head send err.\r\n");
 goto err_handler;
 }

 /*http user data发送*/
 
 rc = socket_send(context->handle,context->user_data,context->user_data_size,http_client_timer_value(&timer));
 if(rc != context->user_data_size){
 log_error("http user data send err.\r\n");
 goto err_handler;
 }

 
 /*清空http buffer 等待接收数据*/
 memset(http_buffer,0,HTTP_BUFFER_SIZE);

 rc = http_client_recv_head(context,http_buffer,HTTP_BUFFER_SIZE,http_client_timer_value(&timer));
 /*接收head失败 返回*/
 if(rc != 0){
 goto err_handler;
 }

 /*编码是chunk处理*/
 if(context->is_chunked == true){
 rc = http_client_recv_chunk(context,http_client_timer_value(&timer));
 if(rc != 0){
 goto err_handler;  
 }
 }else{/*不是chunk处理*/
 if(context->content_size > context->rsp_buffer_size - 1){
 log_error("content size:%d large than free buffer size:%d.\r\n",context->content_size,context->rsp_buffer_size);   
 return -1;
 }
 rc = socket_recv(context->handle,context->rsp_buffer,context->content_size,http_client_timer_value(&timer));
 if(rc < 0){
 log_error("content recv err.\r\n");
 goto err_handler;
 }
 if(rc != context->content_size){
 log_error("content recv timeout.\r\n");
 goto err_handler; 
 }
 }
 rc =0;
 /*错误处理*/ 
err_handler:
 /*释放http 缓存*/
 HTTP_CLIENT_FREE(http_buffer);
 if(context->connected == true){
 socket_disconnect(context->handle);
 context->connected = false;
 }
 
 
 return rc;
}

/* 函数名：http_client_get
*  功能：  http GET
*  参数：  context 资源上下文
*  返回：  0：成功 其他：失败
*/
int http_client_get(http_client_context_t *context)
{
  if(context == NULL ){
  log_error("null pointer.\r\n");
  return -1;
  } 
 return  http_client_request("GET",context);
}

/* 函数名：http_client_post
*  功能：  http GET
*  参数：  context 资源上下文
*  返回：  0：成功 其他：失败
*/
int http_client_post(http_client_context_t *context)
{
  if(context == NULL ){
  log_error("null pointer.\r\n");
  return -1;
  } 

 return  http_client_request("POST",context);
}

/* 函数名：http_client_download
*  功能：  http GET
*  参数：  context 资源上下文
*  返回：  0：成功 其他：失败
*/
int http_client_download(http_client_context_t *context)
{
  if(context == NULL ){
  log_error("null pointer.\r\n");
  return -1;
  }  
 return  http_client_request("POST",context);
}
