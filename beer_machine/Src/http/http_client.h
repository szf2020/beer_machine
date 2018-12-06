/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * Martin d'Allens <martin.dallens@gmail.com> wrote this file. As long as you retain
 * this notice you can do whatever you want with this stuff. If we meet some day,
 * and you think this stuff is worth it, you can buy me a beer in return.
 * ----------------------------------------------------------------------------
 */

#ifndef  __HTTP_CLIENT_H__
#define  __HTTP_CLIENT_H__


typedef struct
{
uint32_t start;
uint32_t size;
bool valid;
}http_client_range_request_t;

typedef struct
{
const char *sn;
const char *sign;
const char *source;
const char *timestamp; 
}http_client_param_request_t;

typedef struct
{
const char *body;
uint16_t body_size;
http_client_range_request_t range;
http_client_param_request_t param;
}http_client_request_t;


typedef struct 
{
int      timeout;
char     *body;
uint16_t body_size;
uint16_t body_buffer_size;
int16_t  status_code;
}http_client_response_t;

#define  HTTP_CLIENT_MALLOC(x)           pvPortMalloc((x))
#define  HTTP_CLIENT_FREE(x)             vPortFree((x))
#define  HTTP_BUFFER_SIZE                1600

/* 函数名：http_client_get
*  功能：  http GET
*  参数：  url 资源定位
*  参数：  req 请求参数指针
*  参数：  res 回应指针
*  返回：  0：成功 其他：失败
*/
int http_client_get(const char *url,http_client_request_t *req,http_client_response_t *res);
/* 函数名：http_client_post
*  功能：  http POST
*  参数：  url 资源定位
*  参数：  req 请求参数指针
*  参数：  res 回应指针
*  返回：  0：成功 其他：失败
*/
int http_client_post(const char *url,http_client_request_t *req,http_client_response_t *res);
/* 函数名：http_client_download
*  功能：  http下载接口
*  参数：  url 资源定位
*  参数：  req 请求参数指针
*  参数：  res 回应指针
*  返回：  0：成功 其他：失败
*/
int http_client_download(const char *url,http_client_request_t *req,http_client_response_t *res);


#endif
