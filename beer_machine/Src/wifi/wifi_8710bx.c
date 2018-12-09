#include "comm_utils.h"
#include "at_cmd.h"
#include "wifi_8710bx.h"
#include "serial.h"
#include "cmsis_os.h"
#include "log.h"
#define  LOG_MODULE_LEVEL    LOG_LEVEL_DEBUG
#define  LOG_MODULE_NAME     "[wifi]"

extern int wifi_8710bx_serial_handle;
extern serial_hal_driver_t wifi_8710bx_serial_driver;
static osMutexId wifi_mutex;

#define  WIFI_8710BX_MALLOC(x)      pvPortMalloc((x))
#define  WIFI_8710BX_FREE(x)        vPortFree((x))


/* 函数名：wifi_8710bx_hal_init
*  功能：  串口驱动初始化
*  参数：  无
*  返回：  WIFI_ERR_OK：成功 其他：失败
*/
int wifi_8710bx_hal_init()
{
int rc;
rc = serial_create(&wifi_8710bx_serial_handle,WIFI_8710BX_BUFFER_SIZE,WIFI_8710BX_BUFFER_SIZE);
if(rc != 0){
log_error("wifi create serial hal err.\r\n");
return WIFI_ERR_HAL_INIT;
}
log_debug("wifi create serial hal ok.\r\n");
rc = serial_register_hal_driver(wifi_8710bx_serial_handle,&wifi_8710bx_serial_driver);
if(rc != 0){
log_error("wifi register serial hal driver err.\r\n");
return WIFI_ERR_HAL_INIT;
}
log_debug("wifi register serial hal driver ok.\r\n");

rc = serial_open(wifi_8710bx_serial_handle,2,115200,8,1);
if(rc != 0){
log_error("wifi open serial hal err.\r\n");
return WIFI_ERR_HAL_INIT;
}
log_debug("wifi open serial hal ok.\r\n");

osMutexDef(wifi_mutex);
wifi_mutex = osMutexCreate(osMutex(wifi_mutex));
if(wifi_mutex == NULL){
log_error("create wifi mutex err.\r\n");
return WIFI_ERR_HAL_INIT;
}
osMutexRelease(wifi_mutex);
log_debug("create wifi mutex ok.\r\n");

return WIFI_ERR_OK; 
}

/* 函数名：wifi_8710bx_print_err_info
*  功能：  打印错误信息
*  参数：  info 额外信息 
*  参数：  err_code 错误码 
*  返回：  无
*/
static void wifi_8710bx_print_err_info(const char *info,const int err_code)
{
  if(info){
  log_debug("%s\r\n",info);
  }
  switch(err_code)
  {
  case  WIFI_ERR_OK:
  log_debug("WIFI ERR CODE:%d cmd OK.\r\n",err_code);
  break;

  case  WIFI_ERR_MALLOC_FAIL:
  log_error("WIFI ERR CODE:%d malloc fail.\r\n",err_code);
  break;
    
  case  WIFI_ERR_CMD_ERR:
  log_error("WIFI ERR CODE:%d cmd execute err.\r\n",err_code);
  break;  
  
  case  WIFI_ERR_CMD_TIMEOUT:
  log_error("WIFI ERR CODE:%d cmd execute timeout.\r\n",err_code);   
  break;
  
  case  WIFI_ERR_RSP_TIMEOUT:
  log_error("WIFI ERR CODE:%d cmd rsp timeout.\r\n",err_code);   
  break;
  
  case  WIFI_ERR_SERIAL_SEND:
  log_error("WIFI ERR CODE:%d serial send err.\r\n",err_code);   
  break;
  
  case  WIFI_ERR_SERIAL_RECV:
  log_error("WIFI ERR CODE:%d serial recv err.\r\n",err_code);   
  break;
 
  case  WIFI_ERR_RECV_NO_SPACE:
  log_error("WIFI ERR CODE:%d serial recv no space.\r\n",err_code);   
  break;
  
  case  WIFI_ERR_SOCKET_ALREADY_CONNECT:
  log_error("WIFI ERR CODE:%d socket alread connect.\r\n",err_code);   
  break;
  
  case  WIFI_ERR_CONNECT_FAIL:
  log_error("WIFI ERR CODE:%d socket connect fail.\r\n",err_code);   
  break;
  
  case  WIFI_ERR_SOCKET_SEND_FAIL:
  log_error("WIFI ERR CODE:%d socket send fail.\r\n",err_code);   
  break;
  
  case  WIFI_ERR_SOCKET_DISCONNECT:
  log_error("WIFI ERR CODE:%d socket disconnect err .\r\n",err_code);   
  break;
  
  case  WIFI_ERR_HAL_GPIO:
  log_error("WIFI ERR CODE:%d hal gpio err.\r\n",err_code);   
  break;
  
  case  WIFI_ERR_HAL_INIT:
  log_error("WIFI ERR CODE:%d hal init err.\r\n",err_code);   
  break;
  
  case  WIFI_ERR_UNKNOW:
  log_error("WIFI ERR CODE:%d unknow err.\r\n",err_code);   
  break;
  
  default:
  log_error("WIFI ERR CODE:%d invalid err code.\r\n",err_code);   
  }
}

/* 函数名：wifi_8710bx_at_cmd_send
*  功能：  串口发送
*  参数：  send 发送的数据 
*  参数：  size 数据大小 
*  参数：  timeout 发送超时 
*  返回：  WIFI_ERR_OK：成功 其他：失败
*/
static int wifi_8710bx_at_cmd_send(const char *send,const uint16_t size,const uint32_t timeout)
{
 uint16_t write_total = 0;
 uint16_t remain_total = size;
 uint32_t write_timeout = timeout;
 int write_len;
 /*清空serial buffer*/
 serial_flush(wifi_8710bx_serial_handle);
 do{
 write_timeout -- ;
 write_len = serial_write(wifi_8710bx_serial_handle,(uint8_t *)send + write_total,remain_total);
 if(write_len == -1){
 log_error("at cmd write err.\r\n");
 return WIFI_ERR_SERIAL_SEND;  
 }
 write_total += write_len;
 remain_total -= write_len;
 }while(remain_total > 0 && write_timeout > 0);  
  
 if(remain_total > 0){
 return WIFI_ERR_SERIAL_SEND;
 }

 if( serial_complete(wifi_8710bx_serial_handle,write_timeout) != 0){
 return WIFI_ERR_SERIAL_SEND;  ;
 }
 return WIFI_ERR_OK;
}

/* 函数名：wifi_8710bx_at_cmd_recv
*  功能：  串口接收
*  参数：  recv 数据buffer 
*  参数：  size 数据大小 
*  参数：  timeout 接收超时 
*  返回：  WIFI_ERR_OK：成功 其他：失败
*/
#define  GSM_M6312_SELECT_TIMEOUT            200
static int wifi_8710bx_at_cmd_recv(char *recv,const uint16_t size,const uint32_t timeout)
{
 int select_size;
 uint16_t read_total = 0;
 int read_size;
 uint32_t read_timeout = timeout;
 /*清空serial buffer*/
 serial_flush(wifi_8710bx_serial_handle); 
 select_size = serial_select(wifi_8710bx_serial_handle,read_timeout);
 if(select_size < 0){
 return WIFI_ERR_SERIAL_RECV;
 } 
 if(select_size == 0){
 return WIFI_ERR_RSP_TIMEOUT;  
 }
 do{
 if(read_total + select_size > size){
 return WIFI_ERR_RECV_NO_SPACE;
 }
 read_size = serial_read(wifi_8710bx_serial_handle,(uint8_t *)recv + read_total,select_size); 
 if(read_size < 0){
 return WIFI_ERR_SERIAL_RECV;  
 }
 read_total += read_size;
 select_size = serial_select(wifi_8710bx_serial_handle,GSM_M6312_SELECT_TIMEOUT);
 if(select_size < 0){
 return WIFI_ERR_SERIAL_RECV;
 } 
 }while(select_size != 0);
 recv[read_total] = '\0';
 return WIFI_ERR_OK;
 }


#define  WIFI_8710BX_RESET_SEND_TIMEOUT              5
#define  WIFI_8710BX_RESET_RSP_TIMEOUT               1000
/* 函数名：wifi_8710bx_reset
*  功能：  wifi软件复位 
*  参数：  无
*  返回：  WIFI_ERR_OK：成功 其他：失败
*/
int wifi_8710bx_reset(void)
{
 int rc;
 const char *at_cmd = "ATSR\r\n";
 char rsp[20] = { 0 };

 osMutexWait(wifi_mutex,osWaitForever);
 
 rc = wifi_8710bx_at_cmd_send(at_cmd,strlen(at_cmd),WIFI_8710BX_RESET_SEND_TIMEOUT);
 if(rc != WIFI_ERR_OK){
 goto err_exit;
 }
 
 rc = wifi_8710bx_at_cmd_recv(rsp,20,WIFI_8710BX_RESET_RSP_TIMEOUT);
 if(rc != WIFI_ERR_OK){
 goto err_exit;
 }
 
 if(strstr(rsp,"[ATSR] ERROR")){
 rc = WIFI_ERR_CMD_ERR;
 goto err_exit;
 }
 if(strstr(rsp,"[ATSR] OK") == NULL){
 rc = WIFI_ERR_UNKNOW;
 goto err_exit;
 }
 rc = WIFI_ERR_OK;
  
err_exit:
 wifi_8710bx_print_err_info(at_cmd,rc);
 osMutexRelease(wifi_mutex);
 return rc; 
}

#define  WIFI_8710BX_SET_MODE_SEND_TIMEOUT            5
#define  WIFI_8710BX_SET_MODE_RSP_TIMEOUT             2000
/* 函数名：wifi_8710bx_set_mode
*  功能：  设置工作模式 
*  参数：  mode 模式 station or ap
*  返回：  WIFI_ERR_OK：成功 其他：失败
*/
int wifi_8710bx_set_mode(const wifi_8710bx_mode_t mode)
{
 int rc;
 char *at_cmd;
 char rsp[20] = { 0 };

 osMutexWait(wifi_mutex,osWaitForever);
 if(mode == WIFI_8710BX_STATION_MODE){
 at_cmd = "ATPW=1\r\n";
 }else{
 at_cmd = "ATPW=2\r\n";
 }
 rc = wifi_8710bx_at_cmd_send(at_cmd,strlen(at_cmd),WIFI_8710BX_SET_MODE_SEND_TIMEOUT);
 if(rc != WIFI_ERR_OK){
 goto err_exit;
 }
 
 rc = wifi_8710bx_at_cmd_recv(rsp,20,WIFI_8710BX_SET_MODE_RSP_TIMEOUT);
 if(rc != WIFI_ERR_OK){
 goto err_exit;
 }
 
 if(strstr(rsp,"[ATPW] ERROR")){
 rc = WIFI_ERR_CMD_ERR;
 goto err_exit;
 }
 if(strstr(rsp,"[ATPW] OK") == NULL){
 rc = WIFI_ERR_UNKNOW;
 goto err_exit;
 }
 rc = WIFI_ERR_OK;
  
err_exit:
 wifi_8710bx_print_err_info(at_cmd,rc);
 osMutexRelease(wifi_mutex);
 return rc; 
}

#define  WIFI_8710BX_SET_ECHO_SEND_TIMEOUT           5
#define  WIFI_8710BX_SET_ECHO_RSP_TIMEOUT            2000
/* 函数名：wifi_8710bx_set_echo
*  功能：  设置回显模式 
*  参数：  echo 回显值 打开或者关闭
*  返回：  WIFI_ERR_OK：成功 其他：失败
*/
int wifi_8710bx_set_echo(const wifi_8710bx_echo_t echo)
{
 int rc;
 char *at_cmd;
 char rsp[20] = { 0 };

 osMutexWait(wifi_mutex,osWaitForever);
 if(echo == WIFI_8710BX_ECHO_ON){
 at_cmd = "ATSE=1\r\n";
 }else{
 at_cmd = "ATSE=0\r\n";
 }
 rc = wifi_8710bx_at_cmd_send(at_cmd,strlen(at_cmd),WIFI_8710BX_SET_ECHO_SEND_TIMEOUT);
 if(rc != WIFI_ERR_OK){
 goto err_exit;
 }
 
 rc = wifi_8710bx_at_cmd_recv(rsp,20,WIFI_8710BX_SET_ECHO_RSP_TIMEOUT);
 if(rc != WIFI_ERR_OK){
 goto err_exit;
 }
 
 if(strstr(rsp,"[ATSE] ERROR")){
 rc = WIFI_ERR_CMD_ERR;
 goto err_exit;
 }
 if(strstr(rsp,"[ATSE] OK") == NULL){
 rc = WIFI_ERR_UNKNOW;
 goto err_exit;
 }
 rc = WIFI_ERR_OK;
  
err_exit:
 wifi_8710bx_print_err_info(at_cmd,rc);
 osMutexRelease(wifi_mutex);
 return rc;
}
#define  WIFI_8710BX_CONFIG_AP_SEND_TIMEOUT           5
#define  WIFI_8710BX_CONFIG_AP_RSP_TIMEOUT            2000
/* 函数名：wifi_8710bx_config_ap
*  功能：  配置AP参数 
*  参数：  ap AP参数结构指针
*  返回：  WIFI_ERR_OK：成功 其他：失败
*/
int wifi_8710bx_config_ap(const wifi_8710bx_ap_config_t *ap)
{
 int rc;
 char at_cmd[64] = { 0 };
 char rsp[20] = { 0 };
 
 osMutexWait(wifi_mutex,osWaitForever);
 
 snprintf(at_cmd,64,"ATPA=%s,%s,%d,%d\r\n",ap->ssid,ap->passwd,ap->chn,(int)ap->hidden);
 rc = wifi_8710bx_at_cmd_send(at_cmd,strlen(at_cmd),WIFI_8710BX_CONFIG_AP_SEND_TIMEOUT);
 if(rc != WIFI_ERR_OK){
 goto err_exit;
 }
 
 rc = wifi_8710bx_at_cmd_recv(rsp,20,WIFI_8710BX_CONFIG_AP_RSP_TIMEOUT);
 if(rc != WIFI_ERR_OK){
 goto err_exit;
 }
 
 if(strstr(rsp,"[ATPA] ERROR")){
 rc = WIFI_ERR_CMD_ERR;
 goto err_exit;
 }
 if(strstr(rsp,"[ATPA] OK") == NULL){
 rc = WIFI_ERR_UNKNOW;
 goto err_exit;
 }
 rc = WIFI_ERR_OK;
  
err_exit:
 wifi_8710bx_print_err_info(at_cmd,rc);
 osMutexRelease(wifi_mutex);
 return rc;
}

#define  WIFI_8710BX_GET_AP_RSSI_SEND_TIMEOUT           5
#define  WIFI_8710BX_GET_AP_RSSI_RSP_TIMEOUT            5000
#define  WIFI_8710BX_GET_RSSI_SIZE                      2500
/* 函数名：wifi_8710bx_get_ap_rssi
*  功能：  扫描出指定ap的rssi值 
*  参数：  ssid ap的ssid名称
*  参数：  rssi ap的rssi的指针
*  返回：  WIFI_ERR_OK：成功 其他：失败
*/
int wifi_8710bx_get_ap_rssi(const char *ssid,int *rssi)
{
 int rc;
 const char *at_cmd = "ATWS\r\n";
 char *rsp ="";
 char *ssid_str;
 char *tolon_str;
 
 rsp = WIFI_8710BX_MALLOC(WIFI_8710BX_GET_RSSI_SIZE);
 if(rsp == NULL){
 return WIFI_ERR_MALLOC_FAIL;
 }
 
 osMutexWait(wifi_mutex,osWaitForever);
 
 rc = wifi_8710bx_at_cmd_send(at_cmd,strlen(at_cmd),WIFI_8710BX_GET_AP_RSSI_SEND_TIMEOUT);
 if(rc != WIFI_ERR_OK){
 goto err_exit;
 }
 memset(rsp,0,WIFI_8710BX_GET_RSSI_SIZE);
 rc = wifi_8710bx_at_cmd_recv(rsp,WIFI_8710BX_GET_RSSI_SIZE,WIFI_8710BX_GET_AP_RSSI_RSP_TIMEOUT);
 if(rc != WIFI_ERR_OK){
 goto err_exit;
 }
 
 if(strstr(rsp,"[ATWS] ERROR")){
 rc = WIFI_ERR_CMD_ERR;
 goto err_exit;
 }
 if(strstr(rsp,"[ATWS] OK") == NULL){
 rc = WIFI_ERR_UNKNOW;
 goto err_exit;
 }
 
 ssid_str = strstr(rsp,ssid);
 if(ssid_str == NULL){
 /*没有发现指定ssid rssi默认为0*/
 *rssi = 0;
 rc = WIFI_ERR_OK;
 goto err_exit;
 }
 /*ssid后第3个tolon是rssi值*/
 for(uint8_t i = 0;i < 3; i ++){
 tolon_str= strstr(ssid_str,",");
 if(tolon_str == NULL){
 rc = WIFI_ERR_UNKNOW;
 goto err_exit; 
 }
 ssid_str = tolon_str + 1; 
 }
 *rssi = atoi(ssid_str);
 rc = WIFI_ERR_OK;
  
err_exit:
 wifi_8710bx_print_err_info(at_cmd,rc);
 wifi_8710bx_print_err_info(rsp,rc);
 osMutexRelease(wifi_mutex);
 WIFI_8710BX_FREE(rsp);
 return rc;
}

#define  WIFI_8710BX_CONNECT_AP_SEND_TIMEOUT         5
#define  WIFI_8710BX_CONNECT_AP_RSP_TIMEOUT          8000

/* 函数名：wifi_8710bx_connect_ap
*  功能：  连接指定的ap
*  参数：  ssid ap的ssid名称
*  参数：  passwd ap的passwd
*  返回：  WIFI_ERR_OK：成功 其他：失败
*/
int wifi_8710bx_connect_ap(const char *ssid,const char *passwd)
{
 int rc;
 char at_cmd[80] = { 0 };
 char rsp[20] = { 0 };
 
 osMutexWait(wifi_mutex,osWaitForever);
 
 snprintf(at_cmd,80,"ATPN=%s,%s\r\n",ssid,passwd);
 rc = wifi_8710bx_at_cmd_send(at_cmd,strlen(at_cmd),WIFI_8710BX_CONNECT_AP_SEND_TIMEOUT);
 if(rc != WIFI_ERR_OK){
 goto err_exit;
 }

 rc = wifi_8710bx_at_cmd_recv(rsp,WIFI_8710BX_GET_RSSI_SIZE,WIFI_8710BX_CONNECT_AP_RSP_TIMEOUT);
 if(rc != WIFI_ERR_OK){
 goto err_exit;
 }
 
 if(strstr(rsp,"[ATPN] ERROR")){
 rc = WIFI_ERR_CMD_ERR;
 goto err_exit;
 }
 
 if(strstr(rsp,"[ATPN] OK") == NULL){
 rc = WIFI_ERR_UNKNOW;
 goto err_exit;
 }
 rc = WIFI_ERR_OK;
  
err_exit:
 wifi_8710bx_print_err_info(at_cmd,rc);
 osMutexRelease(wifi_mutex);
 return rc;
}

#define  WIFI_8710BX_DISCONNECT_AP_SEND_TIMEOUT      5
#define  WIFI_8710BX_DISCONNECT_AP_RSP_TIMEOUT       2000

/* 函数名：wifi_8710bx_disconnect_ap
*  功能：  断开指定的ap连接
*  参数：  无
*  返回：  WIFI_ERR_OK：成功 其他：失败
*/
int wifi_8710bx_disconnect_ap(void)
{
 int rc;
 const char *at_cmd = "ATWD\r\n";
 char rsp[20] = { 0 };
 
 osMutexWait(wifi_mutex,osWaitForever);
 
 rc = wifi_8710bx_at_cmd_send(at_cmd,strlen(at_cmd),WIFI_8710BX_DISCONNECT_AP_SEND_TIMEOUT);
 if(rc != WIFI_ERR_OK){
 goto err_exit;
 }
 rc = wifi_8710bx_at_cmd_recv(rsp,WIFI_8710BX_GET_RSSI_SIZE,WIFI_8710BX_DISCONNECT_AP_RSP_TIMEOUT);
 if(rc != WIFI_ERR_OK){
 goto err_exit;
 }
 
 if(strstr(rsp,"[ATWD] ERROR")){
 rc = WIFI_ERR_CMD_ERR;
 goto err_exit;
 }
 
 if(strstr(rsp,"[ATWD] OK") == NULL){
 rc = WIFI_ERR_UNKNOW;
 goto err_exit;
 }
 rc = WIFI_ERR_OK;
  
err_exit:
 wifi_8710bx_print_err_info(at_cmd,rc);
 osMutexRelease(wifi_mutex);
 return rc;
}

 
/* 函数名：wifi_8710bx_dump_wifi_device_info
*  功能：  dump wifi设备信息
*  参数：  buffer  源buffer
*  参数：  wifi_device wifi设备指针
*  返回：  WIFI_ERR_OK：成功 其他：失败
*/
static int wifi_8710bx_dump_wifi_device_info(const char *buffer,wifi_8710bx_device_t *wifi_device)
{
 char *pos;
 int  size;
 
 if(strstr(buffer,"AP")){
 wifi_device->mode = WIFI_8710BX_AP_MODE; 
 }else if(strstr(buffer,"STA")){
 wifi_device->mode = WIFI_8710BX_STATION_MODE;  
 }else{
 return WIFI_ERR_UNKNOW;
 }

 /*找到第1个break位置 mode*/
 pos = strstr(buffer,",");
 if(pos == NULL){
 return WIFI_ERR_UNKNOW;
 }
 buffer = pos + 1;
 /*找到第2个break位置 ssid*/
 pos = strstr(buffer,",");
 if(pos == NULL){
 return WIFI_ERR_UNKNOW;
 }
 size = pos - buffer > WIFI_8710BX_SSID_STR_LEN ? WIFI_8710BX_SSID_STR_LEN : pos - buffer;
 memcpy(wifi_device->ap.ssid,buffer,size);
 wifi_device->ap.ssid[size] = '\0';
/*找到chn位置*/
 buffer = pos + 1;
 wifi_device->ap.chn = atoi(buffer);
 
 /*找到第3个break位置 chn*/
 pos = strstr(buffer,",");
 if(pos == NULL){
 return WIFI_ERR_UNKNOW;
 }
 /*找到第4个break位置 sec*/
 buffer = pos + 1;
 pos = strstr(buffer,",");
 if(pos == NULL){
 return WIFI_ERR_UNKNOW;
 }
 size = pos - buffer > WIFI_8710BX_SEC_STR_LEN ? WIFI_8710BX_SEC_STR_LEN : pos - buffer;
 memcpy(wifi_device->ap.sec,buffer,size);
 wifi_device->ap.sec[size] = '\0';
 /*找到第5个break位置 passwd*/
 buffer = pos + 1;
 pos = strstr(buffer,",");
 if(pos == NULL){
 return WIFI_ERR_UNKNOW;
 }
 size = pos - buffer > WIFI_8710BX_PASSWD_STR_LEN ? WIFI_8710BX_PASSWD_STR_LEN : pos - buffer;
 memcpy(wifi_device->ap.passwd,buffer,size);
 wifi_device->ap.passwd[size] = '\0'; 
 /*找到第6个break位置 mac*/
 buffer = pos + 1;
 pos = strstr(buffer,",");
 if(pos == NULL){
 return WIFI_ERR_UNKNOW;
 }
 size = pos - buffer > WIFI_8710BX_MAC_STR_LEN ? WIFI_8710BX_MAC_STR_LEN : pos - buffer;
 memcpy(wifi_device->device.mac,buffer,size);
 wifi_device->device.mac[size] = '\0';
 
 /*找到第7个break位置 ip*/
 buffer = pos + 1;
 pos = strstr(buffer,",");
 if(pos == NULL){
 return WIFI_ERR_UNKNOW;
 }
 size = pos - buffer > WIFI_8710BX_IP_STR_LEN ? WIFI_8710BX_IP_STR_LEN : pos - buffer;
 memcpy(wifi_device->device.ip,buffer,size);
 wifi_device->device.ip[size] = '\0';

 /*找到gateway位置*/
 buffer = pos + 1;
 pos = strstr(buffer,"\r\n");
 if(pos == NULL){
 return WIFI_ERR_UNKNOW;
 }
 size = pos - buffer > WIFI_8710BX_IP_STR_LEN ? WIFI_8710BX_IP_STR_LEN : pos - buffer;
 memcpy(wifi_device->device.gateway,buffer,size);
 wifi_device->device.gateway[size]= '\0';
 log_debug("dump wifi infi ok.\r\n");
 return WIFI_ERR_OK;    
}

#define  WIFI_8710BX_GET_WIFI_DEVICE_SEND_TIMEOUT        5
#define  WIFI_8710BX_GET_WIFI_DEVICE_RSP_TIMEOUT         2000
#define  WIFI_8710BX_GET_WIFI_DEVICE_SIZE                200
/* 函数名：wifi_8710bx_get_wifi_device
*  功能：  获取wifi设备信息
*  参数：  wifi_device 设备指针
*  返回：  WIFI_ERR_OK：成功 其他：失败
*/
int wifi_8710bx_get_wifi_device(wifi_8710bx_device_t *wifi_device)
{
 int rc;
 const char *at_cmd = "ATW?\r\n";
 char *rsp ="";
 
 rsp = WIFI_8710BX_MALLOC(WIFI_8710BX_GET_WIFI_DEVICE_SIZE);
 if(rsp == NULL){
 return WIFI_ERR_MALLOC_FAIL;
 }
 
 osMutexWait(wifi_mutex,osWaitForever);
 
 rc = wifi_8710bx_at_cmd_send(at_cmd,strlen(at_cmd),WIFI_8710BX_GET_WIFI_DEVICE_SEND_TIMEOUT);
 if(rc != WIFI_ERR_OK){
 goto err_exit;
 }
 
 memset(rsp,0,WIFI_8710BX_GET_WIFI_DEVICE_SIZE);
 rc = wifi_8710bx_at_cmd_recv(rsp,WIFI_8710BX_GET_WIFI_DEVICE_SIZE,WIFI_8710BX_GET_WIFI_DEVICE_RSP_TIMEOUT);
 if(rc != WIFI_ERR_OK){
 goto err_exit;
 }
 
 if(strstr(rsp,"[ATW?] ERROR")){
 rc = WIFI_ERR_CMD_ERR;
 goto err_exit;
 }
 
 if(strstr(rsp,"[ATW?] OK") == NULL){
 rc = WIFI_ERR_UNKNOW;
 goto err_exit;
 }
 
 rc = wifi_8710bx_dump_wifi_device_info(rsp,wifi_device);

err_exit:
 wifi_8710bx_print_err_info(at_cmd,rc);
 osMutexRelease(wifi_mutex);
 WIFI_8710BX_FREE(rsp);
 return rc;
}

/* 函数名：wifi_8710bx_dump_connection_info
*  功能：  服务端的连接信息
*  参数：  connect 连接指针
*  参数：  buffer  源buffer
*  返回：  WIFI_ERR_OK：成功 其他：失败
*/
static int wifi_8710bx_dump_connection_info(const char *buffer,wifi_8710bx_connection_list_t *connection_list)
{
#define  TEMP_BUFFER_SIZE                   8

 char *pos;
 int  size;
 char temp_buffer[TEMP_BUFFER_SIZE + 1];
 
 connection_list->cnt = 0;
 for(int8_t i = 0;i< WIFI_8710BX_CONNECTION_CNT_MAX;i++){
 /*找到conn id标志*/
 pos = strstr(buffer,"con_id:"); 
 if(pos == NULL){
 log_debug("dump conn info completed.\r\n");
 return WIFI_ERR_OK;
 }
 buffer = pos + strlen("con_id:");
 connection_list->connection[i].conn_id = atoi(buffer);
 
 /*找到第1个break位置*/
 pos = strstr(buffer,",");
 if(pos == NULL){
 return WIFI_ERR_UNKNOW;
 }
 buffer = pos +1;
 /*找到第2个break位置*/
 pos = strstr(buffer,",");
 if(pos == NULL){
 return WIFI_ERR_UNKNOW;
 }
 size = pos - buffer > TEMP_BUFFER_SIZE ? TEMP_BUFFER_SIZE : pos - buffer;
 memcpy(temp_buffer,buffer,size);
 temp_buffer[size] = '\0';
 if(strcmp(temp_buffer,"server") == 0){
 connection_list->connection[i].type = WIFI_8710BX_CONNECTION_TYPE_SERVER;
 }else if(strcmp(temp_buffer,"seed") == 0){
 connection_list->connection[i].type = WIFI_8710BX_CONNECTION_TYPE_SEED;
 }else {
 connection_list->connection[i].type = WIFI_8710BX_CONNECTION_TYPE_CLIENT;
 }
/*找到第3个break位置*/
 buffer = pos + 1;
 pos = strstr(buffer,",");
 if(pos == NULL){
 return WIFI_ERR_UNKNOW;
 }
 size = pos - buffer > TEMP_BUFFER_SIZE ? TEMP_BUFFER_SIZE : pos - buffer;
 memcpy(temp_buffer,buffer,size);
 temp_buffer[size] = '\0';
 if(strcmp(temp_buffer,"tcp") == 0){
 connection_list->connection[i].protocol = WIFI_8710BX_NET_PROTOCOL_TCP;
 }else {
 connection_list->connection[i].protocol = WIFI_8710BX_NET_PROTOCOL_UDP;
 }
 /*找到address位置*/
 buffer = pos + 1;
 pos = strstr(buffer,"address:");
 if(pos == NULL){
 return WIFI_ERR_UNKNOW;
 }
 buffer = pos + strlen("address:");
 /*找到第4个break位置*/
 pos = strstr(buffer,",");
 if(pos == NULL){
 return WIFI_ERR_UNKNOW;
 }
 size = pos - buffer > WIFI_8710BX_IP_STR_LEN ? WIFI_8710BX_IP_STR_LEN : pos - buffer;
 memcpy(connection_list->connection[i].ip,buffer,size);
 connection_list->connection[i].ip[size] = '\0';
 /*找到port位置*/
 pos = strstr(buffer,"port:");
 if(pos == NULL){
 return WIFI_ERR_UNKNOW;
 }
 buffer = pos + strlen("port:");
 connection_list->connection[i].port = atoi(buffer);

 /*找到socket位置*/
 pos = strstr(buffer,"socket:");
 if(pos == NULL){
 return WIFI_ERR_UNKNOW;
 }
 buffer = pos + strlen("socket:");
 connection_list->connection[i].socket_id = atoi(buffer);
 connection_list->cnt = i + 1;
 }
 
 return WIFI_ERR_OK; 
}

#define  WIFI_8710BX_GET_CONNECTION_SEND_TIMEOUT       5
#define  WIFI_8710BX_GET_CONNECTION_RSP_TIMEOUT        1000
#define  WIFI_8710BX_GET_CONNECTION_SIZE               500

/* 函数名：wifi_8710bx_get_connection
*  功能：  获取连接信息
*  参数：  connection_list 连接列表指针
*  返回：  WIFI_ERR_OK：成功 其他：失败
*/
int wifi_8710bx_get_connection(wifi_8710bx_connection_list_t *connection_list)
{
 int rc;
 const char *at_cmd = "ATPI\r\n";
 char *rsp ="";
 
 rsp = WIFI_8710BX_MALLOC(WIFI_8710BX_GET_CONNECTION_SIZE);
 if(rsp == NULL){
 return WIFI_ERR_MALLOC_FAIL;
 }
 
 osMutexWait(wifi_mutex,osWaitForever);
 
 rc = wifi_8710bx_at_cmd_send(at_cmd,strlen(at_cmd),WIFI_8710BX_GET_CONNECTION_SEND_TIMEOUT);
 if(rc != WIFI_ERR_OK){
 goto err_exit;
 }
 
 memset(rsp,0,WIFI_8710BX_GET_CONNECTION_SIZE);
 rc = wifi_8710bx_at_cmd_recv(rsp,WIFI_8710BX_GET_CONNECTION_SIZE,WIFI_8710BX_GET_CONNECTION_RSP_TIMEOUT);
 if(rc != WIFI_ERR_OK){
 goto err_exit;
 }
 
 if(strstr(rsp,"[ATPI] ERROR")){
 rc = WIFI_ERR_CMD_ERR;
 goto err_exit;
 }
 
 if(strstr(rsp,"[ATPI] OK") == NULL){
 rc = WIFI_ERR_UNKNOW;
 goto err_exit;
 }
 
 rc = wifi_8710bx_dump_connection_info(rsp,connection_list);

err_exit:
 wifi_8710bx_print_err_info(at_cmd,rc);
 osMutexRelease(wifi_mutex);
 WIFI_8710BX_FREE(rsp);
 return rc;
}


/* 函数名：wifi_8710bx_dump_conn_id
*  功能：  获取连接id
*  参数：  buffer 源数据buffer
*  返回：  WIFI_ERR_OK：成功 其他：失败
*/
static int wifi_8710bx_dump_conn_id(const char *buffer)
{
char *pos;
int  conn_id;

pos = strstr(buffer,"conn_id=") + strlen("conn_id=");
if(pos == NULL){
return WIFI_ERR_UNKNOW;
}
conn_id = atoi(pos);
  
return conn_id;
}

#define  WIFI_8710BX_OPEN_SERVER_SEND_TIMEOUT     5
#define  WIFI_8710BX_OPEN_SERVER_RSP_TIMEOUT      5000
/* 函数名：wifi_8710bx_open_server
*  功能：  创建服务端
*  参数：  port 服务端本地端口
*  参数：  protocol   服务端网络协议类型
*  返回：  连接句柄，大于WIFI_ERR_OK：成功 其他：失败
*/
int wifi_8710bx_open_server(const uint16_t port,const wifi_8710bx_net_protocol_t protocol)
{
 int rc;
 char at_cmd[30] = { 0 };
 char rsp[20] = { 0 };
 char *protocol_str;
 
 osMutexWait(wifi_mutex,osWaitForever);
 
 if(protocol == WIFI_8710BX_NET_PROTOCOL_TCP){
 protocol_str = "0";
 }else {
 protocol_str = "1";
 }
 snprintf(at_cmd,30,"ATPS=%s,%d\r\n",protocol_str,port);
 rc = wifi_8710bx_at_cmd_send(at_cmd,strlen(at_cmd),WIFI_8710BX_OPEN_SERVER_SEND_TIMEOUT);
 if(rc != WIFI_ERR_OK){
 goto err_exit;
 }
 
 rc = wifi_8710bx_at_cmd_recv(rsp,20,WIFI_8710BX_OPEN_SERVER_RSP_TIMEOUT);
 if(rc != WIFI_ERR_OK){
 goto err_exit;
 }
 
 if(strstr(rsp,"[ATPS] ERROR")){
 rc = WIFI_ERR_CMD_ERR;
 goto err_exit;
 }
 
 if(strstr(rsp,"[ATPS] OK") == NULL){
 rc = WIFI_ERR_UNKNOW;
 goto err_exit;
 }
 
 rc = wifi_8710bx_dump_conn_id(rsp);
err_exit:
 wifi_8710bx_print_err_info(at_cmd,rc);
 osMutexRelease(wifi_mutex);
 return rc;  
}


#define  WIFI_8710BX_OPEN_CLIENTR_SEND_TIMEOUT          5
#define  WIFI_8710BX_OPEN_CLIENT_RSP_TIMEOUT            15000
/* 函数名：wifi_8710bx_open_client
*  功能：  创建客户端
*  参数：  host        服务端域名
*  参数：  remote_port 服务端端口
*  参数：  local_port  本地端口
*  参数：  protocol    网络协议类型
*  返回：  连接句柄，大于WIFI_ERR_OK：成功 其他：失败
*/
int wifi_8710bx_open_client(const char *host,const uint16_t remote_port,const uint16_t local_port,const wifi_8710bx_net_protocol_t protocol)
{
 int rc;
 char at_cmd[100] = { 0 };
 char rsp[20] = { 0 };
 char *protocol_str;
 
 osMutexWait(wifi_mutex,osWaitForever);
 
 if(protocol == WIFI_8710BX_NET_PROTOCOL_TCP){
 protocol_str = "0";
 }else {
 protocol_str = "1";
 }
 snprintf(at_cmd,100,"ATPC=%s,%s,%d,%d\r\n",protocol_str,host,remote_port,local_port);
 rc = wifi_8710bx_at_cmd_send(at_cmd,strlen(at_cmd),WIFI_8710BX_OPEN_CLIENTR_SEND_TIMEOUT);
 if(rc != WIFI_ERR_OK){
 goto err_exit;
 }
 
 rc = wifi_8710bx_at_cmd_recv(rsp,20,WIFI_8710BX_OPEN_CLIENT_RSP_TIMEOUT);
 if(rc != WIFI_ERR_OK){
 goto err_exit;
 }
 
 if(strstr(rsp,"[ATPC] ERROR")){
 rc = WIFI_ERR_CMD_ERR;
 goto err_exit;
 }
 
 if(strstr(rsp,"[ATPC] OK") == NULL){
 rc = WIFI_ERR_UNKNOW;
 goto err_exit;
 }
 
 rc = wifi_8710bx_dump_conn_id(rsp);
err_exit:
 wifi_8710bx_print_err_info(at_cmd,rc);
 osMutexRelease(wifi_mutex);
 return rc;
}  

#define  WIFI_8710BX_CLOSE_SEND_TIMEOUT       5
#define  WIFI_8710BX_CLOSE_RSP_TIMEOUT        1000
/* 函数名：wifi_8710bx_close
*  功能：  关闭客户或者服务端域      
*  参数：  conn_id     连接句柄
*  返回：  WIFI_ERR_OK：成功 其他：失败
*/
int wifi_8710bx_close(const int conn_id)
{
 int rc;
 char at_cmd[20] = { 0 };
 char rsp[20] = { 0 };
 
 osMutexWait(wifi_mutex,osWaitForever);
 
 snprintf(at_cmd,20,"ATPD=%d\r\n",conn_id);
 rc = wifi_8710bx_at_cmd_send(at_cmd,strlen(at_cmd),WIFI_8710BX_CLOSE_SEND_TIMEOUT);
 if(rc != WIFI_ERR_OK){
 goto err_exit;
 }
 
 rc = wifi_8710bx_at_cmd_recv(rsp,20,WIFI_8710BX_CLOSE_RSP_TIMEOUT);
 if(rc != WIFI_ERR_OK){
 goto err_exit;
 }
 
 if(strstr(rsp,"[ATPD] ERROR")){
 rc = WIFI_ERR_CMD_ERR;
 goto err_exit;
 }
 
 if(strstr(rsp,"[ATPD] OK") == NULL){
 rc = WIFI_ERR_UNKNOW;
 goto err_exit;
 }
 rc = WIFI_ERR_OK;
 
err_exit:
 wifi_8710bx_print_err_info(at_cmd,rc);
 osMutexRelease(wifi_mutex);
 return rc;
} 

#define  WIFI_8710BX_SEND_SEND_TIMEOUT            5
#define  WIFI_8710BX_DATA_SEND_TIMEOUT            200
#define  WIFI_8710BX_CLOSE_RSP_TIMEOUT            1000
/* 函数名：wifi_8710bx_send
*  功能：  发送数据
*  参数：  conn_id 连接句柄
*  参数：  data    数据buffer
*  参数：  size    需要发送的数量
*  返回：  WIFI_ERR_OK：成功 其他：失败
*/
int wifi_8710bx_send(int conn_id,const char *data,const int size)
{
 int rc;
 char at_cmd[20] = { 0 };
 char rsp[20] = { 0 };
 
 osMutexWait(wifi_mutex,osWaitForever);
 
 snprintf(at_cmd,20,"ATPT=%d,%d:",size,conn_id);
 rc = wifi_8710bx_at_cmd_send(at_cmd,strlen(at_cmd),WIFI_8710BX_SEND_SEND_TIMEOUT);
 
 if(rc != WIFI_ERR_OK){
 goto err_exit;
 }
 rc = wifi_8710bx_at_cmd_send(data,size,WIFI_8710BX_DATA_SEND_TIMEOUT);
 
 if(rc != WIFI_ERR_OK){
 goto err_exit;
 }
 
 rc = wifi_8710bx_at_cmd_recv(rsp,20,WIFI_8710BX_CLOSE_RSP_TIMEOUT);
 if(rc != WIFI_ERR_OK){
 goto err_exit;
 }
 
 if(strstr(rsp,"[ATPT] ERROR")){
 rc = WIFI_ERR_CMD_ERR;
 goto err_exit;
 }
 
 if(strstr(rsp,"[ATPT] OK") == NULL){
 rc = WIFI_ERR_UNKNOW;
 goto err_exit;
 }
 rc = WIFI_ERR_OK;
 
err_exit:
 wifi_8710bx_print_err_info(at_cmd,rc);
 osMutexRelease(wifi_mutex);
 return rc;
} 


#define  WIFI_8710BX_RECV_SEND_TIMEOUT              5
#define  WIFI_8710BX_RECV_RSP_TIMEOUT               1000
#define  WIFI_8710BX_RECV_DATA_SIZE                 1700
/* 函数名：wifi_8710bx_recv
*  功能：  接收数据
*  参数：  buffer_dst  目的buffer
*  参数：  buffer_src  源buffer
*  参数：  buffer_size 目的buffer大小
*  返回：  >= 0：接收到的数据量 其他：失败
*/
int wifi_8710bx_recv(const int conn_id,char *buffer,int size)
{
 int rc;
 char at_cmd[20] = { 0 };
 char *rsp= "";
 char *size_str;
 char *data_start_str;
 char data_size;
 
 rsp = WIFI_8710BX_MALLOC(WIFI_8710BX_RECV_DATA_SIZE);
 if(rsp == NULL){
 return WIFI_ERR_MALLOC_FAIL;
 }
 osMutexWait(wifi_mutex,osWaitForever);
 
 snprintf(at_cmd,20,"ATPR=%d,%d\r\n",conn_id,size);
 rc = wifi_8710bx_at_cmd_send(at_cmd,strlen(at_cmd),WIFI_8710BX_RECV_SEND_TIMEOUT);
 
 if(rc != WIFI_ERR_OK){
 goto err_exit;
 }
 
 memset(rsp,0,WIFI_8710BX_RECV_DATA_SIZE);
 rc = wifi_8710bx_at_cmd_recv(rsp,20,WIFI_8710BX_RECV_RSP_TIMEOUT);
 if(rc != WIFI_ERR_OK){
 goto err_exit;
 }
 
 if(strstr(rsp,"[ATPR] ERROR")){
 rc = WIFI_ERR_CMD_ERR;
 goto err_exit;
 }
 
 if(strstr(rsp,"[ATPR] OK") == NULL){
 rc = WIFI_ERR_UNKNOW;
 goto err_exit;
 }
 
 size_str = strstr(rsp,"[ATPR] OK") + strlen("[ATPR] OK") + 1;
 data_size = atoi(size_str);
 
 data_size = data_size > size ? size : data_size ;
 if(data_size > 0){
 data_start_str = strstr(rsp,":");
 if(data_start_str){
 memcpy(buffer,data_start_str + 1,data_size);
 }else{
 rc = WIFI_ERR_UNKNOW;
 goto err_exit;
 }
 }
 rc = data_size;
 
err_exit:
 wifi_8710bx_print_err_info(at_cmd,rc);
 osMutexRelease(wifi_mutex);
 WIFI_8710BX_FREE(rsp);
 return rc;
} 
