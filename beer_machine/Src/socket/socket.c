#include "stdbool.h"
#include "stddef.h"
#include "stdint.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "comm_utils.h"
#include "socket.h"
#include "cmsis_os.h"
#include "log.h"
#define  LOG_MODULE_LEVEL    LOG_LEVEL_DEBUG
#define  LOG_MODULE_NAME     "[socket]"


#define  BUFFER_CNT                           2
#define  BUFFER_SIZE                          1024
#define  SOCKET_GSM_HANDLE_BASE               10


static void socket_gsm_handle_init();
static int socket_buffer_init();
typedef struct
{
bool valid;
uint8_t socket;
char buffer[BUFFER_SIZE];
uint32_t read;
uint32_t write;
}socket_buffer_t;

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
socket_status_t status;
}wifi;

struct
{
bool initialized;
socket_status_t status;
}gsm;

socket_buffer_t buffer[BUFFER_CNT];
}socket_manage_t;



static socket_manage_t socket_manage;



/* 函数名：socket_init
*  功能：  网络连接初始化
*  参数：  无
*  返回：  0：成功 其他：失败
*/
int socket_init()
{
  
 if(wifi_8710bx_hal_init() !=0 || gsm_m6312_serial_hal_init() != 0){
 return -1;
 }
 
 if( socket_wifi_reset() != 0){
 return -1;
 }
 
 if(socket_gsm_reset() != 0){
 return -1;
 }
 
 socket_manage.wifi.config = false;
 socket_manage.wifi.initialized = false;
 socket_manage.wifi.status = SOCKET_STATUS_NOT_READY;
 strcpy(socket_manage.wifi.connect.ssid ,"wkxboot");
 strcpy(socket_manage.wifi.connect.passwd, "wkxboot6666");
 socket_manage.wifi.config =true;
 
 socket_manage.gsm.initialized = false;
 socket_manage.gsm.status = SOCKET_STATUS_NOT_READY;
 socket_gsm_handle_init();
 socket_buffer_init();
 
 log_debug("socket init ok.\r\n");

 return 0;
}

/* 函数名：socket_config_wifi
*  功能：  配置wifi 连接的ssid和密码
*  参数：  ssid 热点名称
*  参数：  passwd 密码 
*  返回：  0：成功 其他：失败
*/
int socket_config_wifi(const char *ssid,const char *passwd)
{
 return 0; 
}


/* 函数名：函数名：socket_get_ap_rssi
*  功能：  获取AP的rssi值
*  参数：  rssi ap rssi指针
*  返回：  0  成功 其他：失败
*/ 
int socket_get_ap_rssi(const char *ssid,int *rssi)
{
 int rc;

 /*获取rssi*/
 rc = wifi_8710bx_get_ap_rssi(ssid,rssi);
 if(rc == 0){
 log_debug("socket get rssi:%d ok.\r\n",*rssi);
 }else{
 log_error("socket get rssi err.\r\n");  
 }

 return rc;   
}


/* 函数名：函数名：socket_gsm_init
*  功能：  gsm网络初始化
*  参数：  无
*  返回：  0 成功 其他：失败
*/ 
static int socket_gsm_init(void)
{
 int rc;
 operator_name_t operator_name;
 gsm_m6312_apn_t apn;
 
 /*关闭回显*/
 rc = gsm_m6312_set_echo(GSM_M6312_ECHO_OFF); 
 if(rc != 0){
 goto err_handler;
 }
 /*关闭信息主动上报*/
 rc = gsm_m6312_set_report(GSM_M6312_REPORT_OFF);
 if(rc != 0){
 goto err_handler;
 }
  /*打开注册和位置主动上报*/
 rc = gsm_m6312_set_reg_echo(GSM_M6312_REG_ECHO_ON);
 if(rc != 0){
 goto err_handler;
 }
 
 /*去除附着网络*/
 rc = gsm_m6312_set_attach_status(GSM_M6312_NOT_ATTACH); 
 if(rc != 0){
 goto err_handler;
 }
 
 /*设置多连接*/
 rc = gsm_m6312_set_connect_mode(GSM_M6312_CONNECT_MODE_MULTIPLE); 
 if(rc != 0){
 goto err_handler;
 }
 /*设置运营商格式*/
 /*
 rc = gsm_m6312_set_auto_operator_format(GSM_M6312_OPERATOR_FORMAT_SHORT_NAME);
 if(rc != 0){
 goto err_handler;
 }
*/
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
 apn =  GSM_M6312_APN_CMNET;
 }else{
 apn = GSM_M6312_APN_UNINET;
 }

 /*设置apn*/
 rc = gsm_m6312_set_apn(apn); 
 if(rc != 0){
 goto err_handler;
 }

 /*附着网络*/
 rc = gsm_m6312_set_attach_status(GSM_M6312_ATTACH); 
 if(rc != 0){
 goto err_handler;
 }
 
 /*激活网络*/
 rc = gsm_m6312_set_active_status(GSM_M6312_ACTIVE); 


err_handler:
  if(rc == 0){
  log_debug("gsm init ok.\r\n");
  }else{
  log_error("gsm init err.\r\n");
  }

 return rc;
}

/* 函数名：函数名：socket_query_gsm_status
*  功能：  询问gsm是否就绪
*  参数：  无
*  返回：  0 成功 其他：失败
*/ 
int socket_query_gsm_status()
{
 int rc;
 sim_card_status_t sim_card_status;

 /*获取sim卡状态*/
 rc = gsm_m6312_get_sim_card_status(&sim_card_status);
 if(rc == 0){
 /*sim卡是否就位*/
 if(sim_card_status == SIM_CARD_STATUS_READY){
 if(socket_manage.gsm.initialized == false){
 rc = socket_gsm_init();
 if(rc == 0){
 socket_manage.gsm.initialized = true;
 socket_manage.gsm.status = SOCKET_STATUS_READY;
 }
 }
 } else{
 socket_manage.gsm.initialized = false;
 socket_manage.gsm.status = SOCKET_STATUS_NOT_READY;
 } 
 }
 return rc;   
}

/* 函数名：socket_wifi_init
*  功能：  初始化wifi网络
*  参数：  无
*  返回：  0 成功 其他：失败
*/ 
static int socket_wifi_init()
{
 int rc;
 rc = wifi_8710bx_set_echo(WIFI_8710BX_ECHO_OFF);
 if(rc != 0){
 goto err_handler;
 }
 rc = wifi_8710bx_set_mode(WIFI_8710BX_STATION_MODE);

err_handler:
  if(rc == 0){
  log_debug("socket wifi init ok.\r\n");
  }else{
  log_error("socket wifi init err.\r\n");
  }
  
  return rc;
}

/* 函数名：函数名：socket_query_wifi_status
*  功能：  询问WiFi是否就绪
*  返回：  0  成功 其他：失败
*/ 
int socket_query_wifi_status()
{
 int rc;
 wifi_8710bx_device_t wifi;
 /*wifi没有配网直接返回*/
 if(socket_manage.wifi.config == false){
 log_debug("wifi not config.\r\n");
 return 0;
 }
 /*wifi没有初始化*/
 if(socket_manage.wifi.initialized == false){
 rc = socket_wifi_init();
 if(rc != 0){
 return -1;
 }
 socket_manage.wifi.initialized = true;
 }
 
 if(socket_manage.wifi.initialized == false){
 log_debug("wifi param not init.\r\n");
 return 0;
 }
 
 rc = wifi_8710bx_get_wifi_device(&wifi);
 if(rc != 0){
 return -1;
 }
 
 if(strcmp(socket_manage.wifi.connect.ssid,wifi.ap.ssid)== 0){
 /*现在是连接的状态*/
 socket_manage.wifi.status = SOCKET_STATUS_READY;
 return 0;
 }
 if(strlen(wifi.ap.ssid) != 0){
 log_debug("wifi cur connect ap:%s.will disconnect.\r\n", wifi.ap.ssid);
 wifi_8710bx_disconnect_ap();
 }
 /*现在是断开的状态*/
 socket_manage.wifi.status = SOCKET_STATUS_NOT_READY;
 
 /*尝试连接ssid*/
 if(socket_manage.wifi.rssi != 0){
 rc = wifi_8710bx_connect_ap(socket_manage.wifi.connect.ssid,socket_manage.wifi.connect.passwd);  
 /*连接成功*/
 if(rc != 0){
 return -1;
 }
 socket_manage.wifi.status = SOCKET_STATUS_READY;
 }

 return 0;   
}

#define  SOCKET_LEVEL_1         -90
#define  SOCKET_LEVEL_2         -80
#define  SOCKET_LEVEL_3         -60
/* 函数名：函数名：socket_query_wifi_level
*  功能：  询问WiFi的level值
*  参数：  rssi值指针
*  返回：  0  成功 其他：失败
*/ 
int socket_query_wifi_level(int *level)
{
 int rc;
 /*尝试扫描ssid*/
 rc = wifi_8710bx_get_ap_rssi(socket_manage.wifi.connect.ssid,&socket_manage.wifi.rssi);
 /*执行失败或者没有扫描到wifi 默认返回 0*/
 if(rc != 0 || socket_manage.wifi.rssi == 0){
 *level = 0;
 return 0;
 }
 /*设置wifi信号等级*/
 /*rssi   - 90 == level 1*/
 /*rssi   - 80 == level 2*/
 /*rssi   - 60 == level 3*/
 if(socket_manage.wifi.rssi >= SOCKET_LEVEL_3){
 *level = 3; 
 }else if(socket_manage.wifi.rssi >= SOCKET_LEVEL_2){
 *level = 2; 
 }else {
 *level = 1;  
 }
 
 return 0; 
}
/* 函数名：函数名：socket_wifi_reset
*  功能：  复位WiFi
*  返回：  0  成功 其他：失败
*/ 
int socket_wifi_reset()
{
 if(wifi_8710bx_reset() != 0){
 return -1;
 }
 return 0; 
}

/* 函数名：函数名：socket_gsm_reset
*  功能：  复位WiFi
*  返回：  0  成功 其他：失败
*/ 
int socket_gsm_reset()
{
 
 if(gsm_m6312_pwr_off() != 0){
 log_error("gsm pwr off err.\r\n");
 return -1;
 }
 if(gsm_m6312_pwr_on() != 0){
 log_error("gsm pwr on err.\r\n");
 return -1;
 
}

 return 0; 
}

typedef struct
{
 int value;
 bool alive;
}socket_handle_t;

#define  GSM_HANDLE_MAX             5

socket_handle_t socket_gsm_handle[GSM_HANDLE_MAX];

/* 函数名：socket_gsm_handle_init
*  功能：  gsm句柄池初始化
*  参数：  无 
*  返回：  无
*/ 
static void socket_gsm_handle_init()
{
 for(uint8_t i = 0;i < GSM_HANDLE_MAX; i++){
 socket_gsm_handle[i].alive = false;
 socket_gsm_handle[i].value = i ;      
 }
}
/* 函数名：socket_malloc_gsm_handle
*  功能：  gsm句柄申请
*  参数：  无 
*  返回：  返回：  0：成功 其他：失败
*/ 
static int socket_malloc_gsm_handle()
{
 for(uint8_t i = 0;i < GSM_HANDLE_MAX; i++){
 if(socket_gsm_handle[i].alive == false){
 socket_gsm_handle[i].alive = true;
 return socket_gsm_handle[i].value;      
 }
 }
 log_error("gsm has no invalid handle.\r\n");   
 return -1; 
}
/* 函数名：socket_free_gsm_handle
*  功能：  gsm句柄释放
*  参数：  无 
*  返回：  0：成功 其他：失败
*/ 
static int socket_free_gsm_handle(int handle)
{
 for(uint8_t i = 0;i < GSM_HANDLE_MAX; i++){
 if(socket_gsm_handle[i].value == handle && socket_gsm_handle[i].alive == true){
  socket_gsm_handle[i].alive = false;      
  return 0;
 }
}
return -1;
}
/* 函数名：socket_buffer_init
*  功能：  socket接收缓存初始化
*  参数：  无 
*  返回：  0：成功 其他：失败
*/ 
static int socket_buffer_init()
{
 for(uint8_t i = 0; i< BUFFER_CNT; i++){
 memset(&socket_manage.buffer[i],0,sizeof(socket_buffer_t));  
 }
 return 0;
}
/* 函数名：socket_malloc_buffer
*  功能：  申请socket接收缓存
*  参数：  handle： socket句柄 
*  返回：  0：成功 其他：失败
*/ 
static int socket_malloc_buffer(int handle)
{
  for(uint8_t i = 0; i< BUFFER_CNT; i++){
    if( socket_manage.buffer[i].valid == false){
    socket_manage.buffer[i].socket = handle;
    socket_manage.buffer[i].valid = true;
    return 0; 
    }
  }
  
  return -1;  
}
/* 函数名：socket_free_buffer
*  功能：  释放socket接收缓存
*  参数：  handle： socket句柄 
*  返回：  0：成功 其他：失败
*/ 
static int socket_free_buffer(int handle)
{
  for(uint8_t i = 0; i< BUFFER_CNT; i++){
    if(socket_manage.buffer[i].socket == handle && socket_manage.buffer[i].valid == true){
    socket_manage.buffer[i].valid = false; 
    socket_manage.buffer[i].socket = 0;
    socket_manage.buffer[i].read = socket_manage.buffer[i].write;
    return 0; 
    }
  }
  log_error("can not free buffer.handle:%d \r\n",handle);
  return -1;  
}


/* 函数名：socket_seek_buffer_idx
*  功能：  根据句柄查找接收缓存idx
*  参数：  handle： socket句柄 
*  返回：  0：成功 其他：失败
*/ 
static int socket_seek_buffer_idx(int handle)
{

  for(uint8_t i = 0; i< BUFFER_CNT; i++){
  if(socket_manage.buffer[i].socket == handle && socket_manage.buffer[i].valid == true){
  return i; 
  }
  }
  log_error("can seek free buffer idx.handle:%d \r\n",handle);
  return -1;  
}

/* 函数名：socket_get_used_size
*  功能：  根据句柄获取接收缓存已经接收的数据量
*  参数：  handle： socket句柄 
*  返回：  0：成功 其他：失败
*/ 
static int socket_get_used_size(int handle)
{
  int rc;
  rc = socket_seek_buffer_idx(handle);
  if(rc < 0){
  return -1; 
  }
  return socket_manage.buffer[rc].write - socket_manage.buffer[rc].read;    
}

/* 函数名：socket_get_free_size
*  功能：  根据句柄获取接收缓存可使用的空间
*  参数：  handle： socket句柄 
*  返回：  0：成功 其他：失败
*/ 
static int socket_get_free_size(int handle)
{ 
  int rc;
  rc = socket_seek_buffer_idx(handle);
  if(rc < 0){
  return -1; 
  }
  
 return BUFFER_SIZE - (socket_manage.buffer[rc].write - socket_manage.buffer[rc].read);   
}

/* 函数名：socket_write_buffer
*  功能：  根据句柄向对应接收缓存写入数据
*  参数：  handle： socket句柄 
*  参数：  buffer： 数据位置 
*  参数：  size：   数据大小 
*  返回：  0：成功 其他：失败
*/
static int socket_write_buffer(int handle,const char *buffer,const int size)
{
  int rc ;
  rc = socket_seek_buffer_idx(handle);
  if(rc < 0){
  return -1;
  }
  
  for(int i= 0; i< size ; i ++){
  socket_manage.buffer[rc].buffer[socket_manage.buffer[rc].write & (BUFFER_SIZE - 1)] = buffer[i];
  socket_manage.buffer[rc].write ++;
  }
  return 0;
 }

/* 函数名：socket_read_buffer
*  功能：  根据句柄从对应接收缓存读出数据
*  参数：  handle： socket句柄 
*  参数：  buffer： 数据位置 
*  参数：  size：   数据大小 
*  返回：  0：成功 其他：失败
*/     
 static int socket_read_buffer(int handle,char *buffer,const int size)
{
  int rc ;
  rc = socket_seek_buffer_idx(handle);
  if(rc < 0){
  return -1;
  }
  
  for(int i = 0 ; i < size ; i++){
  buffer[i] = socket_manage.buffer[rc].buffer[socket_manage.buffer[rc].read & (BUFFER_SIZE - 1)];
  socket_manage.buffer[rc].read ++; 
  }
  return 0;  
}
  

/* 函数名：socket_connect
*  功能：  建立网络连接
*  参数：  host 主机域名
*  参数：  remote_port 主机端口
*  参数：  local_port 本地端口
*  参数：  protocol   连接协议
*  返回：  >=0 成功连接句柄 其他：失败
*/ 
int socket_connect(const char *host,uint16_t remote_port,socket_protocol_t protocol)
{
 int rc ;
 int handle;
 
 wifi_8710bx_net_protocol_t wifi_net_protocol;
 gsm_m6312_net_protocol_t  gsm_net_protocol;
 
 if(protocol == SOCKET_PROTOCOL_TCP){
 wifi_net_protocol = WIFI_8710BX_NET_PROTOCOL_TCP;
 gsm_net_protocol = GSM_M6312_NET_PROTOCOL_TCP;
 }else{
 wifi_net_protocol = WIFI_8710BX_NET_PROTOCOL_UDP;
 gsm_net_protocol = GSM_M6312_NET_PROTOCOL_UDP;
 }

 /*只要wifi连接上了，始终优先*/
 if(socket_manage.wifi.status == SOCKET_STATUS_READY ){
 rc = wifi_8710bx_open_client(host,remote_port,wifi_net_protocol);
 }else{
 rc = -1;
 log_warning("wifi is not ready.retry gsm.\r\n");      
 }  

 /*wifi连接失败，选择gsm*/
 if(rc < 0){ 
 if(socket_manage.gsm.status == SOCKET_STATUS_READY){
 handle = socket_malloc_gsm_handle();
 if(handle < 0){
 return -1;    
 }
 rc = gsm_m6312_open_client(handle,gsm_net_protocol,host,remote_port);
 if(rc == handle){
 rc =  handle + SOCKET_GSM_HANDLE_BASE;
 log_debug("gsm connect ok handle:%d.\r\n",handle);    
 }else{
 socket_free_gsm_handle(handle);
 log_error("gsm handle :%d connect fail.\r\n",handle);    
 }
 }else{
 log_debug("gsm is not ready.\r\n");    
 }
 }else{
 log_debug("wifi connect ok handle:%d.\r\n",rc);    
 }
 
 /*申请buffer*/
 if(rc > 0 ){
 socket_malloc_buffer(rc);
 }
 return rc;  
}

/* 函数名：socket_disconnect
*  功能：  断开网络连接
*  参数：  socket_info 连接参数
*  返回：  0：成功 其他：失败
*/ 
int socket_disconnect(const int socket_handle)
{
 int rc ;

 /*此连接handle是GSM网络*/
 if(socket_handle >= SOCKET_GSM_HANDLE_BASE){
 rc = gsm_m6312_close_client(socket_handle - SOCKET_GSM_HANDLE_BASE);
 if(rc != 0){
 log_error("gsm handle:%d disconnect err.\r\n",socket_handle - SOCKET_GSM_HANDLE_BASE);  
 }
 socket_free_gsm_handle(socket_handle - SOCKET_GSM_HANDLE_BASE);
 }else { 
 /*此连接handle是WIFI网络*/
 rc = wifi_8710bx_close(socket_handle);
 if(rc != 0){
 log_error("wifi handle:%d disconnect err.\r\n",socket_handle);  
 }
 }
 /*释放buffer*/
 socket_free_buffer(socket_handle);
 return rc;
}


/* 函数名：socket_send
*  功能：  网络发送
*  参数：  socket_handle 连接句柄
*  参数：  buffer 发送缓存
*  参数：  size 长度
*  参数    timeout 发送超时
*  返回：  >=0 成功发送的数据 其他：失败
*/ 
int socket_send(const int socket_handle,const char *buffer,int size,uint32_t timeout)
{
 int send_total = 0,send_size = 0,send_left = size;
 uint32_t start_time,cur_time;
 
 start_time = osKernelSysTick();
 cur_time = start_time;
 /*此连接handle是GSM网络*/
 while(send_left > 0 && cur_time - start_time < timeout){
   
 if(socket_handle >= SOCKET_GSM_HANDLE_BASE ){
 if(socket_manage.gsm.status == SOCKET_STATUS_READY){
 send_size = gsm_m6312_send(socket_handle - SOCKET_GSM_HANDLE_BASE,buffer + send_total,send_left); 
 }
 }else{ 
 /*此连接handle是WIFI网络*/
 if(socket_manage.wifi.status == SOCKET_STATUS_READY){
 send_size = wifi_8710bx_send(socket_handle,buffer + send_total,send_left);
 }
 }
 
 if(send_size >= 0){
 send_total +=send_size;
 send_left -=send_size;
 }else{
 return -1;  
 }
 cur_time = osKernelSysTick();
 }
 log_debug("send data size:%d .\r\n",send_total);
 return send_total;
}


static  char hal_socket_recv_buffer[BUFFER_SIZE];
/* 函数名：socket_recv
*  功能：  网络接收
*  参数：  socket_handle 连接句柄
*  参数：  buffer 接收缓存
*  参数：  size 长度
*  参数：  timeout 接收超时
*  返回：  >=0:成功接收的长度 其他：失败
*/ 
int socket_recv(const int socket_handle,char *buffer,int size,uint32_t timeout)
{
 uint32_t start_time,cur_time;
 int buffer_size,free_size,read_size,recv_size = 0;;

 if(size == 0){
 return 0;
 }
 
 start_time = osKernelSysTick();
 cur_time = start_time;
 read_size = 0;
 
 while(read_size != size && cur_time - start_time < timeout){
 buffer_size = socket_get_used_size(socket_handle);
 if(buffer_size < 0){
 return -1;
 }
 /*先从缓存读出*/
 if(buffer_size + read_size >= size){
 socket_read_buffer(socket_handle,buffer, size - read_size);
 return size;
 }

 /*如果本地缓存不够 先读出所有本地缓存*/
 socket_read_buffer(socket_handle,buffer + read_size,buffer_size);
 read_size += buffer_size;
 
 free_size = socket_get_free_size(socket_handle);

 /*再读出硬件所有缓存*/
 /*此连接handle是GSM网络*/
 if(socket_handle >= SOCKET_GSM_HANDLE_BASE){
 if(socket_manage.gsm.status == SOCKET_STATUS_READY){
 recv_size = gsm_m6312_recv(socket_handle - SOCKET_GSM_HANDLE_BASE,hal_socket_recv_buffer,free_size);
 }
 }else{
 /*此连接handle是WIFI网络*/
 if(socket_manage.wifi.status == SOCKET_STATUS_READY){
 recv_size = wifi_8710bx_recv(socket_handle,hal_socket_recv_buffer,free_size);
 }
 }
 
 if(recv_size >= 0){
 socket_write_buffer(socket_handle,hal_socket_recv_buffer,recv_size);
 }else{
 return -1;  
 }
 cur_time = osKernelSysTick();
 }
 
 log_debug("read data size:%d .\r\n",read_size);
 return read_size;
}