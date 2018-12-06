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
#define  TRANSFER_ENCODE_TYPE_STR             "Transfer-Encoding: "
#define  TRANSFER_ENCODE_TYPE_END_STR         "\r\n"
#define  TRANSFER_ENCODE_CHUNKED_STR          "chunked"
#define  TRANSFER_ENCODE_TYPE_STR_LEN         40
#define  CONTENT_LENGTH_STR                   "Content-Length: "


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
static int http_client_parse_response(const char *buffer,http_client_response_t *res)
{
  char *header;
  char *body;
  char *pos;
  char encode_type_str[TRANSFER_ENCODE_TYPE_STR_LEN];
  char *encode_type;
  char *content_len;
  char *data_start;
  int  header_size;
  int  encode_type_size;
  
  
  if(res == NULL ||buffer == NULL) {
  log_error("http buffer is null.\r\n");
  return -1;
  }
  
  if (buffer == NULL) {
  log_debug("Buffer shouldn't be NULL\n");
  return -1;
  }else if (buffer[0] != '\0') {

  /*计算回应头开始位置*/
  header = strstr(buffer,HEADER_START0_STR);
  if(header == NULL ){
  header = strstr(buffer,HEADER_START1_STR);
  if(header == NULL){
  log_debug("not find version in res buffer.\r\n");
  return -1;
  }
  }
  pos = header + strlen(HEADER_START0_STR);
  res->status_code = atoi(pos); 
  
  /*计算回应头结束位置*/  
  pos = strstr(buffer,HEADER_END_STR); 
  if(pos == NULL){
  log_error("http res no header end str.\r\n");
  return -1;
  }
  body = pos + strlen(HEADER_END_STR);
  header_size = body - header;
  header[header_size - 2] = '\0'; 
  
  /*是否带有content-lenght*/
  pos = strstr(header,CONTENT_LENGTH_STR);
  if(pos != NULL){
  log_debug("http res has content len.\r\n");
  content_len = pos + strlen(CONTENT_LENGTH_STR);
  res->body_size = atoi(content_len);
  data_start = body;         
  }else{
  /*不带有长度信息，寻找编码类型*/
  /*找到编码类型位置*/
  pos = strstr(header,TRANSFER_ENCODE_TYPE_STR);
  if(pos == NULL){
  log_error("http res no TRANSFER_ENCODE_TYPE_STR.\r\n");
  res->body_size = 0;
  return -1;
  } 
  encode_type = pos + strlen(TRANSFER_ENCODE_TYPE_STR);
  /*找到这行的EOF*/
  pos = strstr(encode_type,TRANSFER_ENCODE_TYPE_END_STR);
  encode_type_size = pos - encode_type > TRANSFER_ENCODE_TYPE_STR_LEN - 1 ? TRANSFER_ENCODE_TYPE_STR_LEN - 1 : pos - encode_type;
  memcpy(encode_type_str,encode_type,encode_type_size);
  encode_type_str[encode_type_size] = '\0';
  /*只解析CHUNKED编码类型*/
  if(strcmp(encode_type_str,TRANSFER_ENCODE_CHUNKED_STR) != 0){
  log_error("http encode is not CHUNKED.\r\n");
  }
  log_debug("http encode is chunked.\r\n");
  /*找到chunked size位置*/
  res->body_size = strtol(body,NULL,16);  
  pos = strstr(body,CHUNKED_SIZE_END_STR);
  if(pos == NULL){
  log_error("http res no CHUNKED_SIZE_END_STR.\r\n");
  return -1;
  }    
  data_start = pos + strlen(CHUNKED_SIZE_END_STR);
  }
  
  /*保证数据大小是可靠的*/
  if(res->body_size > res->body_buffer_size){
  log_error("response body size:%d is largeer than buffer size:%d.\r\n",res->body_size,res->body_buffer_size);
  return -1;
  }

  memcpy(res->body,data_start,res->body_size);
  res->body[res->body_size] = '\0';
  }
  log_debug("http parse res ok.\r\n");
  return 0;
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

/*http 请求*/
static int http_client_request(const char *method,const char *url,http_client_request_t *req,http_client_response_t *res)
{
 int start_time,cur_time;
 int rc;
 int req_len;
 char *http_buffer;

 http_client_connection_t http_connection;
 
 http_connection.handle = HTTP_CLIENT_CONNECTION_HANDLE;
 http_connection.connected = false;
 http_connection.successed = false;
 res->status_code = -1;
 
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
 rc = connection_send(http_connection.handle,http_buffer,req_len);
 if(rc != 0){
 log_error("http send err.\r\n");
 goto err_handler;
 }
 
 /*清空http buffer 等待接收数据*/
 memset(http_buffer,0,HTTP_BUFFER_SIZE);
 /*循环接收数据*/
 start_time = osKernelSysTick();
 do{
 rc = connection_recv(http_connection.handle,http_buffer,HTTP_BUFFER_SIZE);
 if(rc < 0){
 log_error("http recv err.\r\n");
 goto err_handler;
 }else if(rc == 0){
 osDelay(10);
 }else if(rc > 0){
 /*解析数据*/
 rc = http_client_parse_response(http_buffer,res);
 if(rc != 0){
 log_error("http parse res err.\r\n");
 goto err_handler;
 }
 log_debug("http parse res success.\r\nstatus code:%d.\r\nbody_size:%d\r\n",res->status_code,res->body_size);
 break; 
 }
 cur_time = osKernelSysTick();
 }while(res->timeout > (cur_time - start_time) );
 

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
