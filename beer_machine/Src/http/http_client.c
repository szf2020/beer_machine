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
#include "connection.h"
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

#define  HEADER_START0_STR                    "HTTP/1.0 "
#define  HEADER_START1_STR                    "HTTP/1.1 "
#define  HEADER_END_STR                       "\r\n\r\n"
#define  CHUNKED_SIZE_END_STR                 "\r\n"
#define  TRANSFER_ENCODE_TYPE_STR             "Transfer-Encoding: chunked\r\n"

#define  HTTP_CLIENT_HOST_STR_LEN             50
#define  HTTP_CLIENT_PATH_STR_LEN             50
typedef struct
{
bool connected;
bool success;
}http_client_request_status_t;

typedef struct
{
 int handle;
 char host[HTTP_CLIENT_HOST_STR_LEN];
 uint16_t port;
 char path[HTTP_CLIENT_PATH_STR_LEN];
 bool connected;
 bool successed;
}http_client_connection_t;





/*http回应的数据中找到数据*/
static int http_client_parse_response_head(const char *head,int *head_size,int *body_size)
{
  char *tmp_ptr,*encode_type;
  int encode_type_size;
  
  /*回应头存在*/
  if((strstr(head,HEADER_START0_STR) || strstr(head,HEADER_START1_STR)) &&  strstr(head,HEADER_END_STR)){
  log_debug("find header.\r\n"); 
  *head_size = strstr(head,HEADER_END_STR) + strlen(HEADER_END_STR) - head;
 /*找到body size*/
  if (NULL != (tmp_ptr = strstr(head, "Content-Length"))) {
  *body_size = atoi(tmp_ptr + strlen("Content-Length: "));
  return 0;
  }else if (NULL != (tmp_ptr = strstr(head, "Transfer-Encoding"))) {
  encode_type_size = strlen("Chunked");
  encode_type =(char *) head + strlen("Transfer-Encoding: ");
  if ((! memcmp(encode_type, "Chunked", encode_type_size))
   || (! memcmp(encode_type, "chunked", encode_type_size))) {
  *body_size = 0;/*this mean data is chunked*/   
  return 0;
  }
  }else{
  log_error("header is invalid.\r\n");
  return -2;/*will stop parse*/
  }
  }
  
  log_debug("header is continue...\r\n");   
  return -1;
 }

static int  http_client_parse_response_chunk_size(const char *chunk_str)
{  

}

    
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


 
static int http_client_build_request(char *buffer,const char *method,const http_client_connection_t *connection,http_client_request_t *req,int size)
{
  int req_size;
  const char *http_version = "HTTP/1.1";
  
         snprintf(buffer,size,
        /*method-----path--------------------------------------http_version*/
         "%s "  "%ssn=%s&sign=%s&source=%s&timestamp=%s"  " %s\r\n"
		 "Host: %s:%d\r\n"
		 "Connection: keep-alive\r\n"
		 "User-Agent: wkxboot-gsm-wifi.\r\n"
		 "Content-Type: application/Json\r\n"
		 "Content-Length: %d\r\n"
         "Range: bytes=%d-%d\r\n"
		 "\r\n",
		 method,connection->path,req->param.sn,req->param.sign,req->param.source,req->param.timestamp,http_version,
         connection->host,connection->port,
         req->body_size,
         req->range.start,req->range.start + req->range.size -1
         );
  req_size = strlen(buffer);
  log_debug("head_size:%d\r\nheader:\r\n%s\r\n",req_size,buffer);
  /*拷贝body*/
  if(req->body != NULL){
  if(req->body_size > size - req_size){
  log_error("http no space to copy body data.\r\n"); 
  return -1;  
  }
  memcpy(buffer + req_size,req->body,req->body_size);
  req_size += req->body_size;
  }
  return req_size;     
}

#define  HTTP_CLIENT_CONNECTION_HANDLE         1
#define  HTTP_CLIENT_HEADER_RECV_SIZE          32

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

if(timer->up == true){
time_elapse = osKernelSysTick() - timer->value; 
return  timer->value > time_elapse ? time_elapse : timer->value;
}

return  timer->value > time_elapse ? timer->value - time_elapse : 0; 
}


static int http_client_recv_head(int handle,char *header,int *head_size,int *body_size,const uint32_t timeout)
{
 int rc;
 int recv_total = 0;
 http_client_timer_t timer;
 
 http_client_timer_init(&timer,timeout,false);
 
 /*获取http header*/
 do{
 rc = connection_recv(handle,header + recv_total,HTTP_CLIENT_HEADER_RECV_SIZE,http_client_timer_value(&timer));
 if(rc < 0){
 log_error("http header recv err.\r\n");
 return -1;
 }
 
 if(rc == 0){
 /*超时返回*/
 log_error("recv head timeout.\r\n");
 return -1;  
 }
 
 if(rc > 0){
 recv_total += rc;
 /*尝试解析header*/
 rc = http_client_parse_response_head(header,head_size,body_size);
 if(rc == -2){
 return -1;  
 }
 if(rc == 0){
 log_debug("http parse header success.header size:%d body_size:%d.\r\n",*head_size,*body_size);
 /*返回已经接收的body size */
 return recv_total - *head_size;
 }
 }
 
 }while(rc > 0);
 
 return -1;
}

 
static int http_client_check_chunk_size(char *buffer)
{
 int chunk_size;
 
 /*发现chunk size hex*/
 if(strstr(buffer,"\r\n")){
 chunk_size = strtol(buffer,NULL,16);
 return chunk_size;
 }
 /*没有chunk size hex*/
 log_error("cand not find chun size hex.\r\n");
 return -1; 
 }
 
/*http 请求*/
static int http_client_request(const char *method,const char *url,http_client_request_t *req,http_client_response_t *res)
{
 int rc;
 int req_len;

 char *http_buffer;
 int recv_total = 0;
 int head_size;
 int body_size,body_size_left;
 char *body_start,*body_size_str;
 bool chunked =false;
 
 http_client_connection_t http_connection;
 http_client_timer_t      timer;
 http_connection.handle = HTTP_CLIENT_CONNECTION_HANDLE;
 http_connection.connected = false;
 http_connection.successed = false;
 res->status_code = -1;
 
 http_client_timer_init(&timer,res->timeout,false);

 
 /*申请http缓存*/
 http_buffer = HTTP_CLIENT_MALLOC(HTTP_BUFFER_SIZE);
 if(http_buffer == NULL){
 log_error("http malloc buffer err.\r\n");
 return -1;
 }
 /*解析url*/
 rc = http_client_parse_url(url,http_connection.host,&http_connection.port,http_connection.path);
 if(rc != 0){
 log_error("http client parse url error.\r\n");
 goto err_handler;
 }
 /*构建http缓存*/
 req_len = http_client_build_request(http_buffer,method,&http_connection,req,HTTP_BUFFER_SIZE);
 if(req_len < 0){
 log_error("http build req err.\r\n");
 goto err_handler;
 }

 /*http 连接*/
 rc = connection_connect(http_connection.handle,http_connection.host,http_connection.port,1234,CONNECTION_PROTOCOL_TCP);
 if(rc < 0){
 goto err_handler;
 }
 http_connection.handle = rc;
 http_connection.connected = true;
 
 /*http 发送*/
 rc = connection_send(http_connection.handle,http_buffer,req_len,http_client_timer_value(&timer));
 if(rc != req_len){
 log_error("http send err.\r\n");
 goto err_handler;
 }


 /*清空http buffer 等待接收数据*/
 memset(http_buffer,0,HTTP_BUFFER_SIZE);

 rc = http_client_recv_head(http_connection.handle,http_buffer,&head_size,&body_size,http_client_timer_value(&timer));
 /*解析head失败 返回*/
 if(rc < 0){
 goto err_handler;
 }
 
 body_start = (char *)http_buffer + head_size;
 /*data is chunked 获取chunk size*/
 if(body_size == 0){
 chunk_size_start = body_start;  
 chunked = true;

/*已经接收的chunk size*/
 chunk_size_recv = rc;
/*接收剩余chunk 数据*/
 do{
 /*计算全部chunk size*/
 chunk_data_size = http_client_check_chunk_size(chunk_size_start,&chunk_data_start,&chunk_data_recv);
 if(chunk_data_size > 0){
 if(chunk_data_size > chunk_data_recv){
 chunk_data_left = chunk_data_size - chunk_data_recv);
 rc = connection_recv(http_connection.handle,chunk_size_start + chunk_size_recv,chunk_data_left,http_client_timer_value(&timer));
 if(rc < 0){
 log_error("chunk data recv err.\r\n");
 goto err_handle;
 }
 
 if(rc != chunk_size_left){
 log_error("chunk recv timeout.\r\n");
 goto err_handle; 
 }
 
 log_debug("1 chunk recved.\r\n");
 /*复制回应*/
 memcpy(res->response + body_size,chunk_data_start);
 body_size += chunk_size;

 }else if(chunk_size == 0){ 
 log_debug("recv 1ast chunk.\r\n");   
 break;
 }
 /*接收chunk size hex*/
 rc = connection_recv(http_connection.handle,body_start + body_size,CHUNK_RECV_SIZE,http_client_timer_value(&timer));
 if(rc < 0){
 log_error("chunk size hex recv err.\r\n");
 goto err_handle;
 }
 chunk_size_start = 
 }while(chunk_size > 0 );
 
 /*data is not chunked 获取剩余body size*/
 }else{
 body_size_left = body_size - rc;  
   
   
 }
 }
 
 
 

 /*错误处理*/ 
err_handler:
 /*释放http 缓存*/
 HTTP_CLIENT_FREE(http_buffer);
 if(http_connection.connected == true){
 rc = connection_disconnect(http_connection.handle);
 if(rc != 0){
 log_error("http disconnect res err.\r\n");
 }
 }
 if(res->status_code == 200 || res->status_code == 206 ){
 return 0;
 }
 return -1;
}

/* 函数名：http_client_get
*  功能：  http GET
*  参数：  url 资源定位
*  参数：  req 请求参数指针
*  参数：  res 回应指针
*  返回：  0：成功 其他：失败
*/
int http_client_get(const char *url,http_client_request_t *req,http_client_response_t *res)
{
  if(url == NULL || req  == NULL || res == NULL){
  log_error("null pointer.\r\n");
  return -1;
  } 
 return  http_client_request("GET",url,req,res);
}

/* 函数名：http_client_post
*  功能：  http POST
*  参数：  url 资源定位
*  参数：  req 请求参数指针
*  参数：  res 回应指针
*  返回：  0：成功 其他：失败
*/
int http_client_post(const char *url,http_client_request_t *req,http_client_response_t *res)
{
  if(url == NULL || req  == NULL || res == NULL){
  log_error("null pointer.\r\n");
  return -1;
  } 
 return  http_client_request("POST",url,req,res);
}

/* 函数名：http_client_download
*  功能：  http下载接口
*  参数：  url 资源定位
*  参数：  req 请求参数指针
*  参数：  res 回应指针
*  返回：  0：成功 其他：失败
*/
int http_client_download(const char *url,http_client_request_t *req,http_client_response_t *res)
{
  if(url == NULL || req  == NULL || res == NULL){
  log_error("null pointer.\r\n");
  return -1;
  } 
 return  http_client_request("POST",url,req,res);
}
