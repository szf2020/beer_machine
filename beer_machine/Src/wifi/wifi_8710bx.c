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

#define  WIFI_8710BX_ASSERT_NULL_POINTER(x)           \
if((x) == NULL){                                      \
return -1;                                            \
}



#define  WIFI_8710BX_CREATE_CONN_ID_STR              "con_id="
#define  WIFI_8710BX_CONNECT_CONN_ID_STR             "con_id:"
#define  WIFI_8710BX_CONNECT_SOCKET_ID_STR           "socket:"
#define  WIFI_8710BX_ADDRESS_STR                     "address:"
#define  WIFI_8710BX_PORT_STR                        "port:"
#define  WIFI_8710BX_CONNECT_SERVER_STR              "server"
#define  WIFI_8710BX_CONNECT_SEED_STR                "seed"
#define  WIFI_8710BX_CONNECT_CLIENT_STR              "client"
#define  WIFI_8710BX_CONNECT_TCP_STR                 "tcp" 
#define  WIFI_8710BX_CONNECT_UDP_STR                 "udp"
#define  WIFI_8710BX_MODE_STA_STR                    "STA"
#define  WIFI_8710BX_MODE_AP_STR                     "AP"
#define  WIFI_8710BX_AP_HEADER_AT_STR                "AP :"

#define  WIFI_8710BX_STATION_MODE_AT_STR             "1"
#define  WIFI_8710BX_AP_MODE_AT_STR                  "2"
#define  WIFI_8710BX_BOTH_MODE_AT_STR                "3"

#define  WIFI_8710BX_TCP_AT_STR                      "0"
#define  WIFI_8710BX_UDP_AT_STR                      "1"

#define  WIFI_8710BX_SSID_NO_HIDDEN_AT_STR           "0"
#define  WIFI_8710BX_SSID_HIDDEN_AT_STR              "1"

#define  WIFI_8710BX_SMALL_REQUEST_SIZE               100
#define  WIFI_8710BX_SMALL_RESPONSE_SIZE              100

#define  WIFI_8710BX_MIDDLE_REQUEST_SIZE              500
#define  WIFI_8710BX_MIDDLE_RESPONSE_SIZE             500

#define  WIFI_8710BX_LARGE_REQUEST_SIZE              1600
#define  WIFI_8710BX_LARGE_RESPONSE_SIZE             2500


#define  WIFI_8710BX_SET_MODE_AT_STR                      "ATPW="
#define  WIFI_8710BX_SET_MODE_RESPONSE_OK_AT_STR          "[ATPW] OK"
#define  WIFI_8710BX_SET_MODE_RESPONSE_ERR_AT_STR         "[ATPW] ERROR"
#define  WIFI_8710BX_SET_MODE_TIMEOUT                     5000

#define  WIFI_8710BX_SET_ECHO_AT_STR                       "ATSE="
#define  WIFI_8710BX_SET_ECHO_RESPONSE_OK_AT_STR           "[ATSE] OK"
#define  WIFI_8710BX_SET_ECHO_RESPONSE_ERR_AT_STR          "[ATSE] ERR"
#define  WIFI_8710BX_SET_ECHO_TIMEOUT                      1000
#define  WIFI_8710BX_ECHO_ON_AT_STR                        "1"
#define  WIFI_8710BX_ECHO_OFF_AT_STR                       "0"


#define  WIFI_8710BX_CONFIG_AP_AT_STR                      "ATPA="
#define  WIFI_8710BX_CONFIG_AP_RESPONSE_OK_AT_STR          "[ATPA] OK"
#define  WIFI_8710BX_CONFIG_AP_RESPONSE_ERR_AT_STR         "[ATPA] ERROR"
#define  WIFI_8710BX_CONFIG_AP_TIMEOUT                     5000




#define  WIFI_8710BX_SCAN_AP_AT_STR                        "ATWS"
#define  WIFI_8710BX_SCAN_AP_RESPONSE_OK_AT_STR            "[ATWS] OK"
#define  WIFI_8710BX_SCAN_AP_RESPONSE_ERR_AT_STR           "[ATWS] ERR"
#define  WIFI_8710BX_SCAN_AP_TIMEOUT                       10000

#define  WIFI_8710BX_CONNECT_AP_AT_STR                     "ATPN="
#define  WIFI_8710BX_CONNECT_AP_RESPONSE_OK_AT_STR         "[ATPN] OK"
#define  WIFI_8710BX_CONNECT_AP_RESPONSE_ERR_AT_STR        "[ATPN] ERROR"
#define  WIFI_8710BX_CONNECT_AP_TIMEOUT                    10000

#define  WIFI_8710BX_DISCONNECT_AP_AT_STR                  "ATWD"
#define  WIFI_8710BX_DISCONNECT_AP_RESPONSE_OK_AT_STR      "[ATWD] OK"
#define  WIFI_8710BX_DISCONNECT_AP_RESPONSE_ERR_AT_STR     "[ATWD] ERROR"
#define  WIFI_8710BX_DISCONNECT_AP_TIMEOUT                 5000

#define  WIFI_8710BX_GET_WIFI_DEVICE_AT_STR                "ATW?"
#define  WIFI_8710BX_GET_WIFI_DEVICE_RESPONSE_OK_AT_STR    "[ATW?] OK"
#define  WIFI_8710BX_GET_WIFI_DEVICE_RESPONSE_ERR_AT_STR   "[ATW?] ERROR"
#define  WIFI_8710BX_GET_WIFI_DEVICE_TIMEOUT               2000 

#define  WIFI_8710BX_GET_CONNECTION_AT_STR                 "ATPI"
#define  WIFI_8710BX_GET_CONNECTION_RESPONSE_OK_AT_STR     "[ATPI] OK"
#define  WIFI_8710BX_GET_CONNECTION_RESPONSE_ERR_AT_STR    "[ATPI] ERROR"
#define  WIFI_8710BX_GET_CONNECTION_TIMEOUT                2000

#define  WIFI_8710BX_OPEN_SERVER_AT_STR                     "ATPS="
#define  WIFI_8710BX_OPEN_SERVER_RESPONSE_OK_AT_STR         "[ATPS] OK"
#define  WIFI_8710BX_OPEN_SERVER_RESPONSE_ERR_AT_STR        "[ATPS] ERROR"
#define  WIFI_8710BX_OPEN_SERVER_TIMEOUT                    5000


#define  WIFI_8710BX_OPEN_CLIENT_AT_STR                     "ATPC="
#define  WIFI_8710BX_OPEN_CLIENT_RESPONSE_OK_AT_STR         "[ATPC] OK"
#define  WIFI_8710BX_OPEN_CLIENT_RESPONSE_ERR_AT_STR        "[ATPC] ERROR"
#define  WIFI_8710BX_OPEN_CLIENT_TIMEOUT                    15000
 


#define  WIFI_8710BX_CLOSE_AT_STR                           "ATPD="
#define  WIFI_8710BX_CLOSE_RESPONSE_OK_AT_STR               "[ATPD] OK"
#define  WIFI_8710BX_CLOSE_RESPONSE_ERR_AT_STR              "[ATPD] ERROR"
#define  WIFI_8710BX_CLOSE_TIMEOUT                          5000

#define  WIFI_8710BX_SEND_AT_STR                            "ATPT="
#define  WIFI_8710BX_SEND_RESPONSE_OK_AT_STR                "[ATPT] OK"
#define  WIFI_8710BX_SEND_RESPONSE_ERR_AT_STR               "[ATPT] ERROR"
#define  WIFI_8710BX_SEND_TIMEOUT                           10000

#define  WIFI_8710BX_RECV_AT_STR                            "ATPR="
#define  WIFI_8710BX_RECV_RESPONSE_OK_AT_STR                "[ATPR] OK"
#define  WIFI_8710BX_RECV_RESPONSE_ERR_AT_STR               "[ATPR] ERROR"
#define  WIFI_8710BX_RECV_TIMEOUT                           10000

#define  WIFI_8710BX_RESET_AT_STR                           "ATSR"
#define  WIFI_8710BX_RESET_RESPONSE_OK_AT_STR               "[ATSR] OK"
#define  WIFI_8710BX_RESET_RESPONSE_ERR_AT_STR              "[ATSR] ERROR"
#define  WIFI_8710BX_RESET_TIMEOUT                          1000




/*wifi 8710bx串口外设初始化*/
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

return 0; 
}

/* 函数名：wifi_8710bx_print_err_info
*  功能：  打印错误信息
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
  log_debug("GSM ERR CODE:%d cmd OK.\r\n",err_code);
  break;

  case  WIFI_ERR_MALLOC_FAIL:
  log_error("GSM ERR CODE:%d malloc fail.\r\n",err_code);
  break;
    
  case  WIFI_ERR_CMD_ERR:
  log_error("GSM ERR CODE:%d cmd execute err.\r\n",err_code);
  break;  
  
  case  WIFI_ERR_CMD_TIMEOUT:
  log_error("GSM ERR CODE:%d cmd execute timeout.\r\n",err_code);   
  break;
  
  case  WIFI_ERR_RSP_TIMEOUT:
  log_error("GSM ERR CODE:%d cmd rsp timeout.\r\n",err_code);   
  break;
  
  case  WIFI_ERR_SERIAL_SEND:
  log_error("GSM ERR CODE:%d serial send err.\r\n",err_code);   
  break;
  
  case  WIFI_ERR_SERIAL_RECV:
  log_error("GSM ERR CODE:%d serial recv err.\r\n",err_code);   
  break;
 
  case  WIFI_ERR_RECV_NO_SPACE:
  log_error("GSM ERR CODE:%d serial recv no space.\r\n",err_code);   
  break;
  
  case  WIFI_ERR_SOCKET_ALREADY_CONNECT:
  log_error("GSM ERR CODE:%d socket alread connect.\r\n",err_code);   
  break;
  
  case  WIFI_ERR_CONNECT_FAIL:
  log_error("GSM ERR CODE:%d socket connect fail.\r\n",err_code);   
  break;
  
  case  WIFI_ERR_SOCKET_SEND_FAIL:
  log_error("GSM ERR CODE:%d socket send fail.\r\n",err_code);   
  break;
  
  case  WIFI_ERR_SOCKET_DISCONNECT:
  log_error("GSM ERR CODE:%d socket disconnect err .\r\n",err_code);   
  break;
  
  case  WIFI_ERR_HAL_GPIO:
  log_error("GSM ERR CODE:%d hal gpio err.\r\n",err_code);   
  break;
  
  case  WIFI_ERR_HAL_INIT:
  log_error("GSM ERR CODE:%d hal init err.\r\n",err_code);   
  break;
  
  case  WIFI_ERR_UNKNOW:
  log_error("GSM ERR CODE:%d unknow err.\r\n",err_code);   
  break;
  
  default:
  log_error("GSM ERR CODE:%d invalid err code.\r\n",err_code);   
  }
}



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

#define  GSM_M6312_SELECT_TIMEOUT          5
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
 
 return WIFI_ERR_OK;
 }


#define  WIFI_8710BX_RESET_SEND_TIMEOUT              5
#define  WIFI_8710BX_RESET_RSP_TIMEOUT               1000
/* 函数名：wifi_8710bx_reset
*  功能：  wifi软件复位 
*  参数：  无
*  返回：  0：成功 其他：失败
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
*  返回：  0：成功 其他：失败
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
*  返回：  0：成功 其他：失败
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
*  返回：  0：成功 其他：失败
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
*  返回：  0：成功 其他：失败
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
*  返回：  0：成功 其他：失败
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
*  返回：  0：成功 其他：失败
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
*  返回：  0：成功 其他：失败
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
*  返回：  0：成功 其他：失败
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
 rc = wifi_8710bx_at_cmd_recv(rsp,WIFI_8710BX_GET_RSSI_SIZE,WIFI_8710BX_GET_WIFI_DEVICE_RSP_TIMEOUT);
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
*  返回：  0：成功 其他：失败
*/
static int wifi_8710bx_dump_connection_info(const char *buffer,wifi_8710bx_connection_list_t *connection_list)
{
#define  TEMP_BUFFER_SIZE                   8

 char *pos;
 int  size;
 char temp_buffer[TEMP_BUFFER_SIZE + 1];
 
 WIFI_8710BX_ASSERT_NULL_POINTER(buffer); 
 WIFI_8710BX_ASSERT_NULL_POINTER(connection_list);
 
 connection_list->cnt = 0;
 for(int8_t i = 0;i< WIFI_8710BX_CONNECTION_CNT_MAX;i++){
 /*找到conn id标志*/
 pos = strstr(buffer,WIFI_8710BX_CONNECT_CONN_ID_STR); 
 if(pos == NULL){
 log_debug("dump conn info completed.\r\n");
 return 0;
 }
 buffer = pos + strlen(WIFI_8710BX_CONNECT_CONN_ID_STR);
 connection_list->connection[i].conn_id = atoi(buffer);
 
 /*找到第1个break位置*/
 pos = strstr(buffer,AT_CMD_BREAK_STR);
 WIFI_8710BX_ASSERT_NULL_POINTER(pos);
 buffer = pos + strlen(AT_CMD_BREAK_STR);
 /*找到第2个break位置*/
 pos = strstr(buffer,AT_CMD_BREAK_STR);
 WIFI_8710BX_ASSERT_NULL_POINTER(pos);
 size = pos - buffer > TEMP_BUFFER_SIZE ? TEMP_BUFFER_SIZE : pos - buffer;
 memcpy(temp_buffer,buffer,size);
 temp_buffer[size] = '\0';
 if(strcmp(temp_buffer,WIFI_8710BX_CONNECT_SERVER_STR) == 0){
 connection_list->connection[i].type = WIFI_8710BX_CONNECTION_TYPE_SERVER;
 }else if(strcmp(temp_buffer,WIFI_8710BX_CONNECT_SEED_STR) == 0){
 connection_list->connection[i].type = WIFI_8710BX_CONNECTION_TYPE_SEED;
 }else if(strcmp(temp_buffer,WIFI_8710BX_CONNECT_CLIENT_STR) == 0){
 connection_list->connection[i].type = WIFI_8710BX_CONNECTION_TYPE_CLIENT;
 }else{
 return -1;
 }
/*找到第3个break位置*/
 buffer = pos + strlen(AT_CMD_BREAK_STR);
 pos = strstr(buffer,AT_CMD_BREAK_STR);
 WIFI_8710BX_ASSERT_NULL_POINTER(pos);
 size = pos - buffer > TEMP_BUFFER_SIZE ? TEMP_BUFFER_SIZE : pos - buffer;
 memcpy(temp_buffer,buffer,size);
 temp_buffer[size] = '\0';
 if(strcmp(temp_buffer,WIFI_8710BX_CONNECT_TCP_STR) == 0){
 connection_list->connection[i].protocol = WIFI_8710BX_NET_PROTOCOL_TCP;
 }else if(strcmp(temp_buffer,WIFI_8710BX_CONNECT_UDP_STR) == 0){
 connection_list->connection[i].protocol = WIFI_8710BX_NET_PROTOCOL_UDP;
 }else{
 return -1;
 }
 /*找到address位置*/
 buffer = pos + strlen(AT_CMD_BREAK_STR);
 pos = strstr(buffer,WIFI_8710BX_ADDRESS_STR);
 WIFI_8710BX_ASSERT_NULL_POINTER(pos);
 buffer = pos + strlen(WIFI_8710BX_ADDRESS_STR);
 /*找到第4个break位置*/
 pos = strstr(buffer,AT_CMD_BREAK_STR);
 WIFI_8710BX_ASSERT_NULL_POINTER(pos);
 size = pos - buffer > WIFI_8710BX_IP_STR_LEN ? WIFI_8710BX_IP_STR_LEN : pos - buffer;
 memcpy(connection_list->connection[i].ip,buffer,size);
 connection_list->connection[i].ip[size] = '\0';
 /*找到port位置*/
 pos = strstr(buffer,WIFI_8710BX_PORT_STR);
 WIFI_8710BX_ASSERT_NULL_POINTER(pos);
 buffer = pos + strlen(WIFI_8710BX_PORT_STR);
 connection_list->connection[i].port = atoi(buffer);

 /*找到socket位置*/
 pos = strstr(buffer,WIFI_8710BX_CONNECT_SOCKET_ID_STR);
 WIFI_8710BX_ASSERT_NULL_POINTER(pos);
 buffer = pos + strlen(WIFI_8710BX_CONNECT_SOCKET_ID_STR);
 connection_list->connection[i].socket_id = atoi(buffer);
 connection_list->cnt = i + 1;
 }
 
 return 0; 
}


/* 函数名：wifi_8710bx_get_connection
*  功能：  获取连接信息
*  参数：  connection_list 连接列表指针
*  返回：  0：成功 其他：失败
*/
int wifi_8710bx_get_connection(wifi_8710bx_connection_list_t *connection_list)
{
 int at_rc;
 int connection_rc;
 at_cmd_t cmd;  
 
 osMutexWait(wifi_mutex,osWaitForever);
 cmd.request = (char *)WIFI_8710BX_MALLOC(WIFI_8710BX_SMALL_REQUEST_SIZE);
 cmd.response = (char *)WIFI_8710BX_MALLOC(WIFI_8710BX_MIDDLE_RESPONSE_SIZE);
 
 if(cmd.request == NULL ||cmd.response == NULL){
 log_debug("malloc fail.\r\n");
 connection_rc = -1;
 goto err_handler;
 }
 memset(cmd.request,0,WIFI_8710BX_SMALL_REQUEST_SIZE);
 memset(cmd.response,0,WIFI_8710BX_MIDDLE_RESPONSE_SIZE);
 
 cmd.response_status_pos = AT_CMD_RESPONSE_STATUS_POS_IN_HEAD;
 cmd.response_max = WIFI_8710BX_MIDDLE_RESPONSE_SIZE - 1;
 
 strcpy(cmd.request,WIFI_8710BX_GET_CONNECTION_AT_STR);
 strcat(cmd.request,AT_CMD_EOF_STR);

 cmd.request_size = strlen(cmd.request);
 cmd.response_ok = WIFI_8710BX_GET_CONNECTION_RESPONSE_OK_AT_STR; 
 cmd.response_err = WIFI_8710BX_GET_CONNECTION_RESPONSE_ERR_AT_STR; 
 cmd.timeout = WIFI_8710BX_GET_CONNECTION_TIMEOUT;
 
 at_rc = at_cmd_execute(wifi_8710bx_serial_handle,&cmd);
 if(at_rc != 0 ){
 connection_rc = -1;
 goto err_handler;
 }else{
 connection_rc = wifi_8710bx_dump_connection_info(cmd.response,connection_list);
 }
           
err_handler:
 if(connection_rc == 0){
 log_debug("get connection ok.\r\n");
 }else{
 log_error("get connection err.\r\n");
 }
 WIFI_8710BX_FREE(cmd.request);
 WIFI_8710BX_FREE(cmd.response);
 osMutexRelease(wifi_mutex);
 return connection_rc;
}

/* 函数名：wifi_8710bx_dump_conn_id
*  功能：  获取连接id
*  参数：  buffer 源数据buffer
*  返回：  0：成功 其他：失败
*/
static int wifi_8710bx_dump_conn_id(const char *buffer)
{
char *pos;
int  conn_id;
log_assert(buffer);

pos = strstr(buffer,WIFI_8710BX_CREATE_CONN_ID_STR) + strlen(WIFI_8710BX_CREATE_CONN_ID_STR);
WIFI_8710BX_ASSERT_NULL_POINTER(pos);
conn_id = atoi(pos);
  
return conn_id;
}


/* 函数名：wifi_8710bx_open_server
*  功能：  创建服务端
*  参数：  port 服务端本地端口
*  参数：  protocol   服务端网络协议类型
*  返回：  连接句柄，大于0：成功 其他：失败
*/
int wifi_8710bx_open_server(const uint16_t port,const wifi_8710bx_net_protocol_t protocol)
{
 int conn_id;
 int at_rc;
 at_cmd_t cmd;  
 
 osMutexWait(wifi_mutex,osWaitForever);
 cmd.request = (char *)WIFI_8710BX_MALLOC(WIFI_8710BX_SMALL_REQUEST_SIZE);
 cmd.response = (char *)WIFI_8710BX_MALLOC(WIFI_8710BX_SMALL_RESPONSE_SIZE);
 
 if(cmd.request == NULL ||cmd.response == NULL){
 log_debug("malloc fail.\r\n");
 goto err_handler;
 }
 memset(cmd.request,0,WIFI_8710BX_SMALL_REQUEST_SIZE);
 memset(cmd.response,0,WIFI_8710BX_SMALL_RESPONSE_SIZE);
 
 cmd.response_status_pos = AT_CMD_RESPONSE_STATUS_POS_IN_HEAD;
 cmd.response_max = WIFI_8710BX_SMALL_RESPONSE_SIZE;
 
 strcpy(cmd.request,WIFI_8710BX_OPEN_SERVER_AT_STR);  
 if(protocol == WIFI_8710BX_NET_PROTOCOL_TCP){
 strcat(cmd.request,WIFI_8710BX_TCP_AT_STR);
 }else if(protocol == WIFI_8710BX_NET_PROTOCOL_UDP){
 strcat(cmd.request,WIFI_8710BX_UDP_AT_STR);  
 }else{
 log_error("service net error:%d.\r\n",protocol);
 conn_id = -1;
 goto err_handler;   
 }
 strcat(cmd.request,AT_CMD_BREAK_STR);
 sprintf(cmd.request + strlen(cmd.request),"%d",port);
 strcat(cmd.request,AT_CMD_EOF_STR);
 
 cmd.request_size = strlen(cmd.request);
 cmd.response_ok = WIFI_8710BX_OPEN_SERVER_RESPONSE_OK_AT_STR; 
 cmd.response_err = WIFI_8710BX_OPEN_SERVER_RESPONSE_ERR_AT_STR; 
 cmd.timeout = WIFI_8710BX_OPEN_SERVER_TIMEOUT;
 
 at_rc = at_cmd_execute(wifi_8710bx_serial_handle,&cmd);
 if(at_rc != 0 ){
 conn_id = -1;
 goto err_handler;
 }else{ 
 conn_id = wifi_8710bx_dump_conn_id(cmd.response);
 }
 
err_handler:
 if(conn_id > 0){
 log_debug("open server ok.\r\n");
 }else{
 log_error("open server err.\r\n");  
 }
 WIFI_8710BX_FREE(cmd.request);
 WIFI_8710BX_FREE(cmd.response);
 osMutexRelease(wifi_mutex);
 return conn_id;
}

/* 函数名：wifi_8710bx_open_client
*  功能：  创建客户端
*  参数：  host        服务端域名
*  参数：  remote_port 服务端端口
*  参数：  local_port  本地端口
*  参数：  protocol    网络协议类型
*  返回：  连接句柄，大于0：成功 其他：失败
*/
int wifi_8710bx_open_client(const char *host,const uint16_t remote_port,const uint16_t local_port,const wifi_8710bx_net_protocol_t protocol)
{
 int conn_id;
 int at_rc;
 at_cmd_t cmd;  
 
 osMutexWait(wifi_mutex,osWaitForever); 
 cmd.request = (char *)WIFI_8710BX_MALLOC(WIFI_8710BX_SMALL_REQUEST_SIZE);
 cmd.response = (char *)WIFI_8710BX_MALLOC(WIFI_8710BX_SMALL_RESPONSE_SIZE);
 
 if(cmd.request == NULL ||cmd.response == NULL){
 log_debug("malloc fail.\r\n");
 conn_id = -1;
 goto err_handler;
 }
 memset(cmd.request,0,WIFI_8710BX_SMALL_REQUEST_SIZE);
 memset(cmd.response,0,WIFI_8710BX_SMALL_RESPONSE_SIZE);
 
 cmd.response_status_pos = AT_CMD_RESPONSE_STATUS_POS_IN_HEAD;
 cmd.response_max = WIFI_8710BX_SMALL_RESPONSE_SIZE;
 
 strcpy(cmd.request,WIFI_8710BX_OPEN_CLIENT_AT_STR);
 if(protocol == WIFI_8710BX_NET_PROTOCOL_TCP){
 strcat(cmd.request,WIFI_8710BX_TCP_AT_STR);
 }else if(protocol == WIFI_8710BX_NET_PROTOCOL_UDP){
 strcat(cmd.request,WIFI_8710BX_UDP_AT_STR);  
 }else{
 log_error("service net error:%d.\r\n",protocol);
 conn_id = -1;
 goto err_handler;   
 }
 
 strcat(cmd.request,AT_CMD_BREAK_STR);
 strcat(cmd.request,host);
 strcat(cmd.request,AT_CMD_BREAK_STR);
 sprintf(cmd.request + strlen(cmd.request),"%d",remote_port);
 strcat(cmd.request,AT_CMD_BREAK_STR);
 sprintf(cmd.request + strlen(cmd.request),"%d",local_port);
 strcat(cmd.request,AT_CMD_EOF_STR);
 
 cmd.request_size = strlen(cmd.request);
 cmd.response_ok = WIFI_8710BX_OPEN_CLIENT_RESPONSE_OK_AT_STR; 
 cmd.response_err = WIFI_8710BX_OPEN_CLIENT_RESPONSE_ERR_AT_STR; 
 cmd.timeout = WIFI_8710BX_OPEN_CLIENT_TIMEOUT; 

 at_rc = at_cmd_execute(wifi_8710bx_serial_handle,&cmd);
 if(at_rc != 0 ){
 conn_id = -1;
 goto err_handler;
 }else{
 conn_id = wifi_8710bx_dump_conn_id(cmd.response); 
 }

err_handler:
 if(conn_id > 0){
 log_debug("open client ok.\r\n");
 }else{
 log_debug("open client err.\r\n");  
 }
 WIFI_8710BX_FREE(cmd.request);
 WIFI_8710BX_FREE(cmd.response);
 osMutexRelease(wifi_mutex);
 return conn_id;
}  


/* 函数名：wifi_8710bx_close
*  功能：  关闭客户或者服务端域      
*  参数：  conn_id     连接句柄
*  返回：  0：成功 其他：失败
*/
int wifi_8710bx_close(const int conn_id)
{
 int at_rc;
 int close_rc;
 at_cmd_t cmd;  
 
 osMutexWait(wifi_mutex,osWaitForever);
 cmd.request = (char *)WIFI_8710BX_MALLOC(WIFI_8710BX_SMALL_REQUEST_SIZE);
 cmd.response = (char *)WIFI_8710BX_MALLOC(WIFI_8710BX_SMALL_RESPONSE_SIZE);
 
 if(cmd.request == NULL ||cmd.response == NULL){
 log_debug("malloc fail.\r\n");
 close_rc = -1;
 goto err_handler;
 }
 memset(cmd.request,0,WIFI_8710BX_SMALL_REQUEST_SIZE);
 memset(cmd.response,0,WIFI_8710BX_SMALL_RESPONSE_SIZE);
 
 cmd.response_status_pos = AT_CMD_RESPONSE_STATUS_POS_IN_HEAD;
 cmd.response_max = WIFI_8710BX_SMALL_RESPONSE_SIZE - 1;
 
 strcpy(cmd.request,WIFI_8710BX_CLOSE_AT_STR);
 sprintf(cmd.request + strlen(cmd.request),"%d",conn_id);
 strcat(cmd.request,AT_CMD_EOF_STR);
 
 cmd.request_size = strlen(cmd.request);
 cmd.response_ok = WIFI_8710BX_CLOSE_RESPONSE_OK_AT_STR; 
 cmd.response_err = WIFI_8710BX_CLOSE_RESPONSE_ERR_AT_STR; 
 cmd.timeout = WIFI_8710BX_CLOSE_TIMEOUT; 

 at_rc = at_cmd_execute(wifi_8710bx_serial_handle,&cmd);
 if(at_rc != 0 ){
 close_rc = -1;
 goto err_handler;
 }else{
 close_rc = 0;
 }

err_handler:
 if(close_rc == 0){
 log_debug("close ok.\r\n");
 }else{
 log_error("close err.\r\n");
 }
 WIFI_8710BX_FREE(cmd.request);
 WIFI_8710BX_FREE(cmd.response);
 osMutexRelease(wifi_mutex);
 return close_rc;
} 



/* 函数名：wifi_8710bx_send
*  功能：  发送数据
*  参数：  conn_id 连接句柄
*  参数：  data    数据buffer
*  参数：  size    需要发送的数量
*  返回：  0：成功 其他：失败
*/
int wifi_8710bx_send(int conn_id,const char *data,const int size)
{
 int at_rc;
 int send_rc;
 int unused_size;
 int used_size;
 at_cmd_t cmd;  
 
 osMutexWait(wifi_mutex,osWaitForever);
 cmd.request = (char *)WIFI_8710BX_MALLOC(WIFI_8710BX_LARGE_REQUEST_SIZE);
 cmd.response = (char *)WIFI_8710BX_MALLOC(WIFI_8710BX_SMALL_RESPONSE_SIZE);
 
 if(cmd.request == NULL ||cmd.response == NULL){
 log_debug("malloc fail.\r\n");
 send_rc = -1;
 goto err_handler;
 }
 memset(cmd.request,0,WIFI_8710BX_LARGE_REQUEST_SIZE);
 memset(cmd.response,0,WIFI_8710BX_SMALL_RESPONSE_SIZE);
 
 cmd.response_status_pos = AT_CMD_RESPONSE_STATUS_POS_IN_HEAD;
 cmd.response_max = WIFI_8710BX_SMALL_RESPONSE_SIZE - 1;
 
 strcpy(cmd.request,WIFI_8710BX_SEND_AT_STR);
 sprintf(cmd.request + strlen(cmd.request),"%d",size);
 strcat(cmd.request,AT_CMD_BREAK_STR);
 sprintf(cmd.request + strlen(cmd.request),"%d",conn_id);
 strcat(cmd.request,AT_CMD_TOLON_STR);
 used_size = strlen(cmd.request);
 
 unused_size = WIFI_8710BX_LARGE_REQUEST_SIZE - used_size - strlen(AT_CMD_EOF_STR) - 1;
 if(unused_size < size){
 log_error("at cmd request buffer has no space.\r\n");
 send_rc = -1;
 goto err_handler; 
 }
 memcpy(cmd.request + used_size,data,size);
 memcpy(cmd.request+ used_size + size,AT_CMD_EOF_STR,strlen(AT_CMD_EOF_STR));
 
 cmd.request_size = used_size + size + strlen(AT_CMD_EOF_STR);
 cmd.response_ok = WIFI_8710BX_SEND_RESPONSE_OK_AT_STR; 
 cmd.response_err = WIFI_8710BX_SEND_RESPONSE_ERR_AT_STR; 
 cmd.timeout = WIFI_8710BX_SEND_TIMEOUT; 

 at_rc = at_cmd_execute(wifi_8710bx_serial_handle,&cmd);
 if(at_rc != 0 ){
 send_rc = -1;
 goto err_handler;
 }else{
 send_rc = 0;
 }
 
err_handler:
 if(send_rc == 0){
 log_debug("send ok.\r\n");  
 }else{
 log_error("send err.\r\n"); 
 }
 WIFI_8710BX_FREE(cmd.request);
 WIFI_8710BX_FREE(cmd.response);
 osMutexRelease(wifi_mutex);
 return send_rc;
} 


/* 函数名：wifi_8710bx_dump_recv_data
*  功能：  dump接收到的数据
*  参数：  buffer_dst  目的buffer
*  参数：  buffer  源buffer
*  参数：  size 目的buffer大小
*  返回：  0：成功 其他：失败
*/
static int wifi_8710bx_dump_recv_data(char *buffer_dst,const char *buffer_src,int size)
{
 char *pos;
 int  recv_size;
 int  conn_id;
 int  min;
 log_assert(buffer_src); 
 log_assert(buffer_dst);  
 /*找到接收开始标志*/
 pos = strstr(buffer_src,WIFI_8710BX_RECV_RESPONSE_OK_AT_STR); 
 WIFI_8710BX_ASSERT_NULL_POINTER(pos);
 buffer_src = pos;
 /*找到第1个break位置*/
 pos = strstr(buffer_src,AT_CMD_BREAK_STR);
 WIFI_8710BX_ASSERT_NULL_POINTER(pos);
 /*找到数据size位置*/
 buffer_src = pos + strlen(AT_CMD_BREAK_STR);
 recv_size = atoi(buffer_src);
 /*找到第2个break位置*/
 pos = strstr(buffer_src,AT_CMD_BREAK_STR);
 WIFI_8710BX_ASSERT_NULL_POINTER(pos);
 /*找到conn id位置*/
 buffer_src = pos + strlen(AT_CMD_BREAK_STR);
 conn_id = atoi(buffer_src);
 /*找到数据域分割符位置*/
 pos = strstr(buffer_src,AT_CMD_TOLON_STR);
 WIFI_8710BX_ASSERT_NULL_POINTER(pos);
 /*找到数据域位置*/
 buffer_src = pos + strlen(AT_CMD_TOLON_STR);
 /*copy data*/
 min = size <= recv_size ? size : recv_size;
 memcpy(buffer_dst,buffer_src,min);
 buffer_dst[min] = '\0';
 (void)conn_id;
 return recv_size; 
}


/* 函数名：wifi_8710bx_recv
*  功能：  接收数据
*  参数：  buffer_dst  目的buffer
*  参数：  buffer_src  源buffer
*  参数：  buffer_size 目的buffer大小
*  返回：  >= 0：接收到的数据量 其他：失败
*/
int wifi_8710bx_recv(const int conn_id,char *buffer,int size)
{
 int at_rc;
 int recv_size;
 at_cmd_t cmd;  
 
 osMutexWait(wifi_mutex,osWaitForever);
 cmd.request = (char *)WIFI_8710BX_MALLOC(WIFI_8710BX_SMALL_REQUEST_SIZE);
 cmd.response = (char *)WIFI_8710BX_MALLOC(WIFI_8710BX_LARGE_RESPONSE_SIZE);
 
 if(cmd.request == NULL ||cmd.response == NULL){
 log_debug("malloc fail.\r\n");
 recv_size = -1;
 goto err_handler;
 }
 memset(cmd.request,0,WIFI_8710BX_SMALL_REQUEST_SIZE);
 memset(cmd.response,0,WIFI_8710BX_LARGE_RESPONSE_SIZE );
 
 cmd.response_status_pos = AT_CMD_RESPONSE_STATUS_POS_IN_HEAD;
 cmd.response_max = WIFI_8710BX_LARGE_RESPONSE_SIZE - 1;
 
 strcpy(cmd.request,WIFI_8710BX_RECV_AT_STR);
 sprintf(cmd.request + strlen(cmd.request),"%d",conn_id);
 strcat(cmd.request,AT_CMD_BREAK_STR);
 sprintf(cmd.request + strlen(cmd.request),"%d",size);
 strcat(cmd.request,AT_CMD_EOF_STR);
 
 cmd.request_size = strlen(cmd.request);
 cmd.response_ok = WIFI_8710BX_RECV_RESPONSE_OK_AT_STR; 
 cmd.response_err = WIFI_8710BX_RECV_RESPONSE_ERR_AT_STR; 
 cmd.timeout = WIFI_8710BX_RECV_TIMEOUT; 

 at_rc = at_cmd_execute(wifi_8710bx_serial_handle,&cmd);
 if(at_rc != 0 ){
 recv_size = -1;
 goto err_handler;
 }

 recv_size = wifi_8710bx_dump_recv_data(buffer,cmd.response,size);

err_handler:
 if(recv_size >= 0){
 log_debug("recv ok.\r\n");
 }else{
 log_error("recv err.\r\n");  
 }
 WIFI_8710BX_FREE(cmd.request);
 WIFI_8710BX_FREE(cmd.response);
 osMutexRelease(wifi_mutex);
 return recv_size;
} 






