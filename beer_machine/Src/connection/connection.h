#ifndef  __CONNECTION_H__
#define  __CONNECTION_H__

#include "cmsis_os.h"
#include "wifi_8710bx.h"
#include "gsm_m6312.h"
 
typedef enum
{
 CONNECTION_PROTOCOL_TCP =0,
 CONNECTION_PROTOCOL_UDP
}connection_protocol_t;


typedef struct
{
 connection_protocol_t protocol;
 char *host;
 uint16_t remote_port;
 uint16_t local_port;
}connection_info_t;

typedef enum
{
 CONNECTION_STATUS_READY = 1,
 CONNECTION_STATUS_NOT_READY
}connection_status_t;



/* 函数名：connection_config_wifi
*  功能：  配置wifi 连接的ssid和密码
*  参数：  ssid 热点名称
*  参数：  passwd 密码 
*  返回：  0：成功 其他：失败
*/
int connection_config_wifi(const char *ssid,const char *passwd);


/* 函数名：函数名：connection_get_ap_rssi
*  功能：  获取AP的rssi值
*  参数：  rssi ap rssi指针
*  返回：  0  成功 其他：失败
*/ 
int connection_get_ap_rssi(const char *ssid,int *rssi);

/* 函数名：函数名：connection_query_wifi_status
*  功能：  询问WiFi是否就绪
*  返回：  0  成功 其他：失败
*/ 
int connection_query_wifi_status();

/* 函数名：函数名：connection_query_gsm_status
*  功能：  询问gsm是否就绪
*  参数：  无
*  返回：  0 成功 其他：失败
*/ 
int connection_query_gsm_status();

/* 函数名：函数名：connection_wifi_reset
*  功能：  复位WiFi
*  返回：  0  成功 其他：失败
*/ 
int connection_wifi_reset();
/* 函数名：函数名：connection_gsm_reset
*  功能：  复位WiFi
*  返回：  0  成功 其他：失败
*/ 
int connection_gsm_reset();

/* 函数名：connection_init
*  功能：  网络连接初始化
*  参数：  无
*  返回：  0：成功 其他：失败
*/
int connection_init();

/* 函数名：connection_connect
*  功能：  建立网络连接
*  参数：  host 主机域名
*  参数：  remote_port 主机端口
*  参数：  protocol   连接协议
*  返回：  >=0 成功连接句柄 其他：失败
*/ 
int connection_connect(const char *host,uint16_t remote_port,connection_protocol_t protocol);

/* 函数名：connection_disconnect
*  功能：  断开网络连接
*  参数：  connection_info 连接参数
*  返回：  0：成功 其他：失败
*/ 
int connection_disconnect(int conenction_handle);
/* 函数名：connection_send
*  功能：  网络发送
*  参数：  connection_handle 连接句柄
*  参数：  buffer 发送缓存
*  参数：  length 缓存长度
*  参数    timeout 发送超时
*  返回：  >=0 成功发送的数据 其他：失败
*/ 
int connection_send(int connection_handle,const char *buffer,int length,uint32_t timeout);

/* 函数名：connection_recv
*  功能：  网络接收
*  参数：  connection_handle 连接句柄
*  参数：  buffer 接收缓存
*  参数：  buffer_size 缓存长度
*  参数：  timeout 接收超时
*  返回：  >=0:成功接收的长度 其他：失败
*/ 
int connection_recv(int connection_handle,char *buffer,int buffer_size,uint32_t timeout);
#endif