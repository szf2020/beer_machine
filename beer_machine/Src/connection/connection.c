#include "stdbool.h"
#include "stddef.h"
#include "stdint.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "comm_utils.h"
#include "connection.h"
#include "cmsis_os.h"
#include "log.h"
#define  LOG_MODULE_LEVEL    LOG_LEVEL_DEBUG
#define  LOG_MODULE_NAME     "[connection]"




typedef struct
{
struct
{
bool initialized;
bool config;
int  rssi;
wifi_8710bx_ap_connect_t connect;
uint8_t ssid_len;
uint8_t passwd_len;
connection_status_t status;
}wifi;

struct
{
bool initialized;
connection_status_t status;
}gsm;
}connection_manage_t;


#define  CONNECTION_GSM_CONNECT_HANDLE_BASE               11
static connection_manage_t connection_manage;



/* 函数名：connection_init
*  功能：  网络连接初始化
*  参数：  无
*  返回：  0：成功 其他：失败
*/
int connection_init()
{
 if(wifi_8710bx_hal_init() !=0 || gsm_m6312_serial_hal_init() != 0){
 return -1;
 }
 if(connection_gsm_reset() != 0){
 return -1;
 }
 connection_manage.wifi.config = false;
 connection_manage.wifi.initialized = false;
 connection_manage.wifi.status = CONNECTION_STATUS_NOT_READY;
 strcpy(connection_manage.wifi.connect.ssid ,"wkxboot");
 strcpy(connection_manage.wifi.connect.passwd, "wkxboot6666");
 connection_manage.wifi.config =true;
 
 connection_manage.gsm.initialized = false;
 connection_manage.gsm.status = CONNECTION_STATUS_NOT_READY;
 
 log_debug("connection init ok.\r\n");

 return 0;
}

/* 函数名：connection_config_wifi
*  功能：  配置wifi 连接的ssid和密码
*  参数：  ssid 热点名称
*  参数：  passwd 密码 
*  返回：  0：成功 其他：失败
*/
int connection_config_wifi(const char *ssid,const char *passwd)
{
 return 0; 
}


/* 函数名：函数名：connection_get_ap_rssi
*  功能：  获取AP的rssi值
*  参数：  rssi ap rssi指针
*  返回：  0  成功 其他：失败
*/ 
int connection_get_ap_rssi(const char *ssid,int *rssi)
{
 int rc;

 /*获取rssi*/
 rc = wifi_8710bx_get_ap_rssi(ssid,rssi);
 if(rc == 0){
 log_debug("connection get rssi:%d ok.\r\n",*rssi);
 }else{
 log_error("connection get rssi err.\r\n");  
 }

 return rc;   
}


/* 函数名：函数名：connection_gsm_init
*  功能：  gsm网络初始化
*  参数：  无
*  返回：  0 成功 其他：失败
*/ 
static int connection_gsm_init(void)
{
 int rc;
 operator_name_t operator_name;
 gsm_gprs_apn_t apn;
  
 /*去除附着网络*/
 rc = gsm_m6312_gprs_set_attach_status(GSM_GPRS_NOT_ATTACH); 
 if(rc != 0){
 goto err_handler;
 }
 /*关闭回显*/
 rc = gsm_m6312_set_echo(GSM_M6312_ECHO_OFF); 
 if(rc != 0){
 goto err_handler;
 }
 
 /*设置多连接*/
 rc = gsm_m6312_gprs_set_connect_mode(GSM_M6312_GPRS_CONNECT_MODE_MULTIPLE); 
 if(rc != 0){
 goto err_handler;
 }
 
 /*设置接收缓存*/
 rc = gsm_m6312_config_recv_buffer(GSM_M6312_RECV_BUFFERE); 
 if(rc != 0){
 goto err_handler;
 }
 
 /*获取运营商*/
 rc = gsm_m6312_get_operator(&operator_name);
 if(rc != 0){
 goto err_handler;
 }
 /*赋值apn值*/
 if(operator_name == OPERATOR_CHINA_MOBILE){
 apn =  GSM_GPRS_APN_CMNET;
 }else{
 apn = GSM_GPRS_APN_UNINET;
 }
 /*设置apn*/
 rc = gsm_m6312_gprs_set_apn(1,apn); 
 if(rc != 0){
 goto err_handler;
 }
 /*附着网络*/
 rc = gsm_m6312_gprs_set_attach_status(GSM_GPRS_ATTACH); 
 if(rc != 0){
 goto err_handler;
 }
 /*激活网络*/
 rc = gsm_m6312_gprs_set_active_status(1,GSM_GPRS_ACTIVE); 
 
err_handler:
  if(rc == 0){
  log_debug("gsm init ok.\r\n");
  }else{
  log_error("gsm init err.\r\n");
  }

 return rc;
}
/* 函数名：函数名：connection_query_gsm_status
*  功能：  询问gsm是否就绪
*  参数：  无
*  返回：  0 成功 其他：失败
*/ 
int connection_query_gsm_status()
{
 int rc;
 sim_card_status_t sim_card_status;

 /*获取sim卡状态*/
 rc = gsm_m6312_is_sim_card_ready(&sim_card_status);
 if(rc == 0){
 /*sim卡是否就位*/
 if(sim_card_status == SIM_CARD_STATUS_READY){
 if(connection_manage.gsm.initialized == false){
 rc = connection_gsm_init();
 if(rc == 0){
 connection_manage.gsm.initialized = true;
 connection_manage.gsm.status = CONNECTION_STATUS_READY;
 }
 }else{
 connection_manage.gsm.status = CONNECTION_STATUS_NOT_READY;
 } 
 } 
 }
 return rc;   
}

/* 函数名：connection_wifi_init
*  功能：  初始化wifi网络
*  参数：  无
*  返回：  0 成功 其他：失败
*/ 
static int connection_wifi_init()
{
 int rc;
 rc = wifi_8710bx_set_echo(WIFI_8710BX_ECHO_OFF);
 if(rc != 0){
 goto err_handler;
 }
 rc = wifi_8710bx_set_mode(WIFI_8710BX_STATION_MODE);

err_handler:
  if(rc == 0){
  log_debug("connection wifi init ok.\r\n");
  }else{
  log_error("connection wifi init err.\r\n");
  }
  
  return rc;
}

/* 函数名：函数名：connection_query_wifi_status
*  功能：  询问WiFi是否就绪
*  返回：  0  成功 其他：失败
*/ 
int connection_query_wifi_status()
{
 int rc;
 wifi_8710bx_device_t wifi;
 /*wifi没有配网直接返回*/
 if(connection_manage.wifi.config == false){
 return 0;
 }
 /*wifi没有初始化*/
 if(connection_manage.wifi.initialized == false){
 rc = connection_wifi_init();
 if(rc != 0){
 return -1;
 }
 connection_manage.wifi.initialized = true;
 }
 rc = wifi_8710bx_get_wifi_device(&wifi);
 if(rc != 0){
 return -1;
 }
 if(strcmp(connection_manage.wifi.connect.ssid,wifi.ap.ssid)== 0){
 /*现在是连接的状态*/
 connection_manage.wifi.status = CONNECTION_STATUS_READY;
 return 0;
 }
 
 /*现在是断开的状态*/
 connection_manage.wifi.status = CONNECTION_STATUS_NOT_READY;
 /*尝试扫描ssid*/
 rc = wifi_8710bx_get_ap_rssi(connection_manage.wifi.connect.ssid,&connection_manage.wifi.rssi);
 if(rc != 0){
 return -1;
 }
 /*扫描到指定的ssid，可以连接*/
 if(connection_manage.wifi.rssi != 0){
 rc = wifi_8710bx_connect_ap(connection_manage.wifi.connect.ssid,connection_manage.wifi.connect.passwd);  
 /*连接成功*/
 if(rc != 0){
 return -1;
 }
 connection_manage.wifi.status = CONNECTION_STATUS_READY;
 }

 return 0;   
}

/* 函数名：函数名：connection_wifi_reset
*  功能：  复位WiFi
*  返回：  0  成功 其他：失败
*/ 
int connection_wifi_reset()
{
 if(wifi_8710bx_reset() != 0){
 return -1;
 }
 return 0; 
}

/* 函数名：函数名：connection_gsm_reset
*  功能：  复位WiFi
*  返回：  0  成功 其他：失败
*/ 
int connection_gsm_reset()
{
 if(gsm_m6312_pwr_off() != 0){
 return -1;
 }
 if(gsm_m6312_pwr_on() != 0){
 return -1;
 }
 return 0; 
}


/* 函数名：connection_connect
*  功能：  建立网络连接
*  参数：  handle 连接句柄
*  参数：  host 主机域名
*  参数：  remote_port 主机端口
*  参数：  local_port 本地端口
*  参数：  protocol   连接协议
*  返回：  >=0 成功连接句柄 其他：失败
*/ 
int connection_connect(int handle,const char *host,uint16_t remote_port,uint16_t local_port,connection_protocol_t protocol)
{
 int rc;
 wifi_8710bx_net_protocol_t wifi_net_protocol;
 gsm_m6312_net_protocol_t  gsm_net_protocol;
 
 if(protocol == CONNECTION_PROTOCOL_TCP){
 wifi_net_protocol = WIFI_8710BX_NET_PROTOCOL_TCP;
 gsm_net_protocol = GSM_M6312_NET_PROTOCOL_TCP;
 }else{
 wifi_net_protocol = WIFI_8710BX_NET_PROTOCOL_UDP;
 gsm_net_protocol = GSM_M6312_NET_PROTOCOL_UDP;
 }
 
 rc = -1;
 /*只要wifi连接上了，始终优先*/
 if(connection_manage.wifi.status == CONNECTION_STATUS_READY ){
 rc = wifi_8710bx_open_client(host,remote_port,local_port,wifi_net_protocol);
 }
 /*wifi连接失败，选择gsm*/
 if(rc <= 0){
 if(connection_manage.gsm.status == CONNECTION_STATUS_READY){
 rc = gsm_m6312_open_client(handle,gsm_net_protocol,host,remote_port);
 }
 }
 
 if(rc >= 0){
 log_debug("connection connect ok.\r\n");  
 }else{
 log_error("connection connect err.\r\n");    
 }
 
 return rc;  
}

/* 函数名：connection_disconnect
*  功能：  断开网络连接
*  参数：  connection_info 连接参数
*  返回：  0：成功 其他：失败
*/ 
int connection_disconnect(int connection_handle)
{
 int rc ;

 /*此连接handle是GSM网络*/
 if(connection_handle >= CONNECTION_GSM_CONNECT_HANDLE_BASE){
 connection_handle -=CONNECTION_GSM_CONNECT_HANDLE_BASE;
 rc = gsm_m6312_close_client(connection_handle);
 }else { 
 /*此连接handle是WIFI网络*/
 rc = wifi_8710bx_close(connection_handle);
 }
 
 if(rc == 0){
 log_debug("connection disconnect ok.\r\n");
 }else{
 log_error("connection disconnect err.\r\n");  
 }

 return rc;
}


/* 函数名：connection_send
*  功能：  网络发送
*  参数：  connection_handle 连接句柄
*  参数：  buffer 发送缓存
*  参数：  length 缓存长度
*  返回：  0：成功 其他：失败
*/ 
int connection_send(int connection_handle,const char *buffer,int length)
{
 int rc = -1;

 /*此连接handle是GSM网络*/
 if(connection_handle >= CONNECTION_GSM_CONNECT_HANDLE_BASE ){
 if(connection_manage.gsm.status == CONNECTION_STATUS_READY){
 connection_handle -=CONNECTION_GSM_CONNECT_HANDLE_BASE;
 rc = gsm_m6312_send(connection_handle,buffer,length); 
 }
 }else{ 
 /*此连接handle是WIFI网络*/
 if(connection_manage.wifi.status == CONNECTION_STATUS_READY){
 rc = wifi_8710bx_send(connection_handle,buffer,length);
 }
 }
 
 if(rc == 0){
 log_debug("connection send ok.\r\n");
 }else{
 log_error("connection send err.\r\n");
 }
 return rc;
}

/* 函数名：connection_recv
*  功能：  网络接收
*  参数：  connection_handle 连接句柄
*  参数：  buffer 接收缓存
*  参数：  buffer_size 缓存长度
*  返回：  >=0:成功接收的长度 其他：失败
*/ 
int connection_recv(int connection_handle,char *buffer,int buffer_size)
{
 int rc;
 int recv_size;
 
 if(buffer_size == 0){
 return 0;
 }

  rc = -1;
 /*此连接handle是GSM网络*/
 if(connection_handle >= CONNECTION_GSM_CONNECT_HANDLE_BASE){
 if(connection_manage.gsm.status == CONNECTION_STATUS_READY){
 connection_handle -=CONNECTION_GSM_CONNECT_HANDLE_BASE;
 rc = gsm_m6312_get_recv_buffer_size(connection_handle);
 if(rc < 0){
 return -1;
 }
 if(rc == 0){
 return 0;
 }
 recv_size = rc <= buffer_size ? rc : buffer_size;
 rc = gsm_m6312_recv(connection_handle,buffer,recv_size);
 }
 }else{
 /*此连接handle是WIFI网络*/
 if(connection_manage.wifi.status == CONNECTION_STATUS_READY){
 rc = wifi_8710bx_recv(connection_handle,buffer,buffer_size);
 }
 }
 
 if(rc >= 0){
 log_debug("connection recv ok.\r\n");
 }else{
 log_error("connection recv err.\r\n");   
 }
 
 return rc;
}