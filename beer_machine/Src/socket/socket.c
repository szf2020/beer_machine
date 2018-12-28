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
#define  BUFFER_SIZE                          1024 /*必须是2的x次方*/
#define  SOCKET_GSM_HANDLE_BASE               10
#define  SOCKET_GSM_HANDLE_MAX                5

#define  SOCKET_WIFI_RSSI_OF_LEVEL_1         -90
#define  SOCKET_WIFI_RSSI_OF_LEVEL_2         -80
#define  SOCKET_WIFI_RSSI_OF_LEVEL_3         -60


#if  (((BUFFER_SIZE) - 1) & (BUFFER_SIZE)) != 0
#error "BUFFER SIZE must be 2^X !!!"
#endif


static void socket_gsm_handle_init();
static int socket_buffer_init();

typedef struct
{
 int value;
 bool alive;
}socket_gsm_handle_t;


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
bool base_initialized;
bool gprs_initialized;
socket_status_t status;
socket_gsm_handle_t   handle[SOCKET_GSM_HANDLE_MAX];
}gsm;

socket_buffer_t buffer[BUFFER_CNT];
osMutexId mutex;
}socket_manage_t;



static socket_manage_t socket_manage;


/* 函数名：socket_manage_wifi_init
*  功能：  网络管理wifi初始化
*  参数：  无
*  返回：  0：成功 其他：失败
*/
static int socket_manage_wifi_init()
{
 socket_manage.wifi.initialized = false;
 socket_manage.wifi.status = SOCKET_STATUS_NOT_READY; 

 return 0;
}

/* 函数名：socket_manage_gsm_init
*  功能：  网络管理gsm初始化
*  参数：  无
*  返回：  0：成功 其他：失败
*/
static int socket_manage_gsm_init()
{
 socket_manage.gsm.base_initialized = false;
 socket_manage.gsm.gprs_initialized = false;
 socket_manage.gsm.status = SOCKET_STATUS_NOT_READY;
 
 return 0;
}


/* 函数名：socket_init
*  功能：  网络连接初始化
*  参数：  无
*  返回：  0：成功 其他：失败
*/
int socket_init()
{
  
 if(wifi_8710bx_serial_hal_init() !=0 || gsm_m6312_serial_hal_init() != 0){
 log_assert(0);
 }
 
 osMutexDef(socket_mutex);
 socket_manage.mutex = osMutexCreate(osMutex(socket_mutex));
 if(socket_manage.mutex == NULL) {
 log_error("create socket mutex err.\r\n");
 return -1;
 }
 osMutexRelease(socket_manage.mutex);
 log_debug("create socket mutex ok.\r\n");
 
 socket_gsm_handle_init();
 socket_buffer_init();
 
 log_debug("socket init ok.\r\n");

 return 0;
}


/* 函数名：socket_module_config_wifi
*  功能：  配置wifi 连接的ssid和密码
*  参数：  ssid 热点名称
*  参数：  passwd 密码 
*  返回：  0：成功 其他：失败
*/
int socket_module_config_wifi(const char *ssid,const char *passwd)
{
 strcpy(socket_manage.wifi.connect.ssid ,ssid);
 strcpy(socket_manage.wifi.connect.passwd,passwd);
 socket_manage.wifi.config =true;
 return 0; 
}

/* 函数名：函数名：socket_module_query_wifi_level
*  功能：  询问WiFi的level值
*  参数：  rssi值指针
*  返回：  0  成功 其他：失败
*/ 
static int socket_module_query_wifi_level(int *level)
{
 int rc;
 /*尝试扫描ssid*/
 rc = wifi_8710bx_get_ap_rssi(socket_manage.wifi.connect.ssid,&socket_manage.wifi.rssi);
 /*执行失败或者没有扫描到wifi*/
 if(rc != 0){
 return -1;
 }
 /*没有发现ssid*/  
 if(socket_manage.wifi.rssi == 0){
 *level = 0;
 return 0;
 }
 /*设置wifi信号等级*/
 /*rssi   - 90 == level 1*/
 /*rssi   - 80 == level 2*/
 /*rssi   - 60 == level 3*/
 if(socket_manage.wifi.rssi >= SOCKET_WIFI_RSSI_OF_LEVEL_3){
 *level = 3; 
 }else if(socket_manage.wifi.rssi >= SOCKET_WIFI_RSSI_OF_LEVEL_2){
 *level = 2; 
 }else {
 *level = 1;  
 }
 
 return 0; 
}


/* 函数名：函数名：socket_module_gsm_base_init
*  功能：  gsm模块基本参数初始化
*  参数：  无
*  返回：  0 成功 其他：失败
*/ 
static int socket_module_gsm_base_init(void)
{
 int rc;
 /*关闭回显*/
 rc = gsm_m6312_set_echo(GSM_M6312_ECHO_OFF); 
 if(rc != 0){
 goto err_exit;
 }
 /*关闭信息主动上报*/
 rc = gsm_m6312_set_report(GSM_M6312_REPORT_OFF);
 if(rc != 0){
 goto err_exit;
 }  
  
 /*设置SIM卡注册主动回应位置信息*/
 rc = gsm_m6312_set_reg_echo(GSM_M6312_REG_ECHO_ON);
 if(rc != 0){
 goto err_exit; 
 } 
 return 0;
 
err_exit:
  return rc;
}

/* 函数名：函数名：socket_module_gsm_gprs_init
*  功能：  gsm网络初始化
*  参数：  无
*  返回：  0 成功 其他：失败
*/ 
static int socket_module_gsm_gprs_init(void)
{
 int rc;
 operator_name_t operator_name;
 gsm_m6312_apn_t apn;
 
 /*去除附着网络*/
 rc = gsm_m6312_set_attach_status(GSM_M6312_NOT_ATTACH); 
 if(rc != 0){
 goto err_exit;
 }
 
 /*设置多连接*/
 rc = gsm_m6312_set_connect_mode(GSM_M6312_CONNECT_MODE_MULTIPLE); 
 if(rc != 0){
 goto err_exit;
 }
 /*设置运营商格式*/
 /*
 rc = gsm_m6312_set_auto_operator_format(GSM_M6312_OPERATOR_FORMAT_SHORT_NAME);
 if(rc != 0){
 goto err_exit;
 }
*/
 /*设置接收缓存*/
 rc = gsm_m6312_config_recv_buffer(GSM_M6312_RECV_BUFFERE); 
 if(rc != 0){
 goto err_exit;
 }
 
 /*获取运营商*/
 rc = gsm_m6312_get_operator(&operator_name);
 if(rc != 0){
 goto err_exit;
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
 goto err_exit;
 }

 /*附着网络*/
 rc = gsm_m6312_set_attach_status(GSM_M6312_ATTACH); 
 if(rc != 0){
 goto err_exit;
 }
 
 /*激活网络*/
 rc = gsm_m6312_set_active_status(GSM_M6312_ACTIVE); 
 if(rc != 0){
 goto err_exit;
 }
 log_debug("socket gsm gprs init ok.\r\n");
 return 0;

err_exit:
  log_error("gsm init err.\r\n");

 return rc;
}

/* 函数名：函数名：socket_module_query_gsm_status
*  功能：  询问gsm是否就绪包括位置信息等
*  参数：  location位置信息指针
*  返回：  0 成功 其他：失败
*/ 
int socket_module_query_gsm_status(socket_location_t *location)
{
 int rc;
 sim_card_status_t sim_card_status;
 gsm_m6312_register_t register_info;
 gsm_m6312_assist_base_t assist_base;
 
 /*默认为空*/
 memset(location,0,sizeof(socket_location_t));
 
 /*获取sim卡状态*/
 rc = gsm_m6312_get_sim_card_status(&sim_card_status);
 if(rc != 0){
 goto err_exit; 
 }
 
 /*sim卡是否就位*/
 if(sim_card_status == SIM_CARD_STATUS_NO_SIM_CARD){
 socket_manage.gsm.base_initialized = false;
 socket_manage.gsm.gprs_initialized = false;   
 socket_manage.gsm.status = SOCKET_STATUS_NOT_READY;
 log_debug("SIM card not ready.\r\n");
 return 0; 
 }
 /*gsm基础参数是否初始化*/ 
 if(socket_manage.gsm.base_initialized == false){
 rc = socket_module_gsm_base_init();
 if(rc != 0){
 goto err_exit;
 }  
 socket_manage.gsm.base_initialized = true;
 }
 
 /*SIM卡是否注册*/  
 rc = gsm_m6312_get_reg_location(&register_info);
 if(rc != 0){
 goto err_exit; 
 }
/*没有注册直接返回*/ 
 if(register_info.status == GSM_M6312_STATUS_NO_REGISTER){
 log_debug("sim not register.\r\n");
 return 0;
 }
 /*注册后就有位置信息*/
 strcpy(location->base[0].lac,register_info.base.lac);
 strcpy(location->base[0].ci,register_info.base.ci);
 /*获取主机站信号强度信息*/
 rc = gsm_m6312_get_rssi(location->base[0].rssi);
 if(rc != 0){
 goto err_exit; 
 }
 /*获取辅助基站信息*/
 rc = gsm_m6312_get_assist_base_info(&assist_base);
 if(rc != 0){
 goto err_exit; 
 }
 for(uint8_t i = 0 ;i < 3 ;i++){
 strcpy(location->base[i + 1].lac,assist_base.base[i].lac);
 strcpy(location->base[i + 1].ci,assist_base.base[i].ci);
 strcpy(location->base[i + 1].rssi,assist_base.base[i].rssi);
 }
 
 /*gsm模块gprs参数初始化*/  
 if(socket_manage.gsm.gprs_initialized == false){
 rc = socket_module_gsm_gprs_init();
 if(rc != 0){
 goto err_exit;
 }
 socket_manage.gsm.gprs_initialized = true;
 socket_manage.gsm.status = SOCKET_STATUS_READY;
 }
 return 0;
 
err_exit:
 return rc;   
}

/* 函数名：socket_module_wifi_init
*  功能：  初始化wifi模块参数
*  参数：  无
*  返回：  0 成功 其他：失败
*/ 
static int socket_module_wifi_init()
{
 int rc;
 rc = wifi_8710bx_set_echo(WIFI_8710BX_ECHO_OFF);
 if(rc != 0){
 goto err_exit;
 }
 rc = wifi_8710bx_set_mode(WIFI_8710BX_STATION_MODE);
 if(rc != 0){
 goto err_exit;
 }
 log_debug("socket wifi init ok.\r\n");
 return 0;
 
err_exit:
  log_error("socket wifi init err.\r\n");

  return rc;
}

/* 函数名：函数名：socket_module_query_wifi_status
*  功能：  询问WiFi状态
*  参数：  wifi_level wifi强度 
*  返回：  0  成功 其他：失败
*/ 
int socket_module_query_wifi_status(int *wifi_level)
{
 int rc;
 wifi_8710bx_device_t wifi;
 
 /*如果没有初始化参数*/
 if(socket_manage.wifi.initialized == false){
 rc = socket_module_wifi_init();
 if(rc != 0){
 goto err_exit;
 }
 socket_manage.wifi.initialized = true;
 }
 
 /*wifi没有配网直接返回*/
 if(socket_manage.wifi.config == false){
 *wifi_level = 0;/*代表没有发现*/ 
 log_debug("wifi not config.\r\n");
 return 0;
 }
 /*扫描是否存在配置的ap ssid并获取强度值*/
 rc = socket_module_query_wifi_level(wifi_level); 
 if(rc != 0){
 goto err_exit;
 }
 
 /*是否连接上*/ 
 rc = wifi_8710bx_get_wifi_device(&wifi);
 if(rc != 0){
 goto err_exit;
 }
 if(strcmp(socket_manage.wifi.connect.ssid,wifi.ap.ssid)== 0){
 /*现在是连接的状态*/
 socket_manage.wifi.status = SOCKET_STATUS_READY;
 }else {
 /*现在是断开的状态*/
 socket_manage.wifi.status = SOCKET_STATUS_NOT_READY;
 /*连接到了别的网络或者未连接*/
 rc = wifi_8710bx_disconnect_ap();
 if(rc != 0){
 goto err_exit;
 }

 if(*wifi_level == 0){/*没有发现ssid*/
 return 0; 
 }
 /*尝试连接ssid*/
 rc = wifi_8710bx_connect_ap(socket_manage.wifi.connect.ssid,socket_manage.wifi.connect.passwd);  
 /*连接成功*/
 if(rc != 0){
 goto err_exit;
 }
 socket_manage.wifi.status = SOCKET_STATUS_READY;
 }
 
 err_exit:
 return rc;
}

/* 函数名：函数名：socket_module_wifi_reset
*  功能：  复位WiFi
*  返回：  0  成功 其他：失败
*/ 
int socket_module_wifi_reset()
{
 socket_manage_wifi_init();
 if(wifi_8710bx_reset() != 0){
 return -1;
 }
 return 0; 
}

/* 函数名：函数名：socket_module_gsm_reset
*  功能：  复位WiFi
*  返回：  0  成功 其他：失败
*/ 
int socket_module_gsm_reset()
{
 socket_manage_gsm_init(); 
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

/* 函数名：socket_gsm_handle_init
*  功能：  gsm句柄池初始化
*  参数：  无 
*  返回：  无
*/ 
static void socket_gsm_handle_init()
{
 for(uint8_t i = 0;i < SOCKET_GSM_HANDLE_MAX; i++){
 socket_manage.gsm.handle[i].alive = false;
 socket_manage.gsm.handle[i].value = i ;      
 }
}
/* 函数名：socket_malloc_gsm_handle
*  功能：  gsm句柄申请
*  参数：  无 
*  返回： >=0 成功 其他：失败
*/ 
static int socket_malloc_gsm_handle()
{
 int rc;
 osMutexWait(socket_manage.mutex,osWaitForever);
 for(uint8_t i = 0;i < SOCKET_GSM_HANDLE_MAX; i++){
 if(socket_manage.gsm.handle[i].alive == false){
 socket_manage.gsm.handle[i].alive = true;
 rc = socket_manage.gsm.handle[i].value; 
 goto exit;
 }
 }
 rc = -1; 
 log_error("gsm has no invalid handle.\r\n"); 
exit: 
 osMutexRelease(socket_manage.mutex);
 return rc; 
}
/* 函数名：socket_free_gsm_handle
*  功能：  gsm句柄释放
*  参数：  无 
*  返回：  0：成功 其他：失败
*/ 
static int socket_free_gsm_handle(int handle)
{
 int rc;
 osMutexWait(socket_manage.mutex,osWaitForever);
 
 for(uint8_t i = 0;i < SOCKET_GSM_HANDLE_MAX; i++){
 if(socket_manage.gsm.handle[i].value == handle && socket_manage.gsm.handle[i].alive == true){
  socket_manage.gsm.handle[i].alive = false;      
  rc = 0;
  goto exit;
 }
 }
 rc = -1;
 log_error("gsm free handle:%d invalid.\r\n",handle); 
exit:
 osMutexRelease(socket_manage.mutex); 
 return rc;
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
  int rc;
  osMutexWait(socket_manage.mutex,osWaitForever);
 
  for(uint8_t i = 0; i< BUFFER_CNT; i++){
    if( socket_manage.buffer[i].valid == false){
    socket_manage.buffer[i].socket = handle;
    socket_manage.buffer[i].valid = true;
    rc = 0; 
    goto exit;
    }
  }
  rc = -1;
  log_error("gsm malloc buffer handle:%d fail no more valid buffer.\r\n",handle);  
exit:
 osMutexRelease(socket_manage.mutex); 
 return rc;   
}
/* 函数名：socket_free_buffer
*  功能：  释放socket接收缓存
*  参数：  handle： socket句柄 
*  返回：  0：成功 其他：失败
*/ 
static int socket_free_buffer(int handle)
{
  int rc;
  osMutexWait(socket_manage.mutex,osWaitForever);
  
  for(uint8_t i = 0; i< BUFFER_CNT; i++){
    if(socket_manage.buffer[i].socket == handle && socket_manage.buffer[i].valid == true){
    socket_manage.buffer[i].valid = false; 
    socket_manage.buffer[i].socket = 0;
    socket_manage.buffer[i].read = socket_manage.buffer[i].write;
    rc = 0; 
    goto exit;
    }
  }
  rc = -1;
  log_error("gsm free buffer handle:%d invalid.\r\n",handle);
exit:
 osMutexRelease(socket_manage.mutex); 
 return rc;   
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