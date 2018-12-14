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

static wifi_8710bx_at_cmd_t wifi_cmd;

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
  if(err_code > 0){
  log_debug("size:%d.\r\n",err_code);
  return;
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
  
  case  WIFI_ERR_SEND_TIMEOUT:
  log_error("WIFI ERR CODE:%d cmd send timeout.\r\n",err_code);   
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
  
  case  WIFI_ERR_SEND_NO_SPACE:
  log_error("WIFI ERR CODE:%d serial send no space.\r\n",err_code);   
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

/* 函数名：wifi_8710bx_err_code_add
*  功能：  添加比对的错误码
*  参数：  err_head 错误头指针 
*  参数：  err_code 错误节点 
*  返回：  无
*/
static void wifi_8710bx_err_code_add(wifi_8710bx_err_code_t **err_head,wifi_8710bx_err_code_t *err_code)
{
    wifi_8710bx_err_code_t **err_node;
 
    if (*err_head == NULL){
        *err_head = err_code;
        return;
    }
    err_node = err_head;
    while((*err_node)->next){
    err_node = (wifi_8710bx_err_code_t **)(&(*err_node)->next);   
    }
    (*err_node)->next = err_code;
}


/* 函数名：wifi_8710bx_at_cmd_send
*  功能：  串口发送
*  参数：  send 发送的数据 
*  参数：  size 数据大小 
*  参数：  timeout 发送超时 
*  返回：  WIFI_ERR_OK：成功 其他：失败
*/
static int wifi_8710bx_at_cmd_send(const char *send,const uint16_t size,const uint16_t timeout)
{
 int rc;
 uint16_t write_total = 0;
 uint16_t remain_total = size;
 uint16_t write_timeout = timeout;
 int write_len;
 /*清空serial buffer*/
 serial_flush(wifi_8710bx_serial_handle);
 do{
 write_len = serial_write(wifi_8710bx_serial_handle,(uint8_t *)send + write_total,remain_total);
 if(write_len == -1){
 log_error("at cmd write err.\r\n");
 return WIFI_ERR_SERIAL_SEND;  
 }
 write_total += write_len;
 remain_total -= write_len;
 write_timeout -- ;
 osDelay(1);
 }while(remain_total > 0 && write_timeout > 0);  
  
 if(remain_total > 0){
 return WIFI_ERR_SEND_TIMEOUT;
 }

 rc = serial_complete(wifi_8710bx_serial_handle,write_timeout) ;
 if (rc > 0){
 return WIFI_ERR_SEND_TIMEOUT;
 }
 
 if (rc < 0){
 return WIFI_ERR_SERIAL_SEND;
 }
 
 return WIFI_ERR_OK;
}

/* 函数名：wifi_8710bx_at_cmd_recv
*  功能：  串口接收
*  参数：  recv 数据buffer 
*  参数：  size 数据大小 
*  参数：  timeout 接收超时 
*  返回：  读到的一帧数据量 其他：失败
*/
#define  GSM_M6312_SELECT_TIMEOUT               10
static int wifi_8710bx_at_cmd_recv(char *recv,const uint16_t size,const uint32_t timeout)
{
 int select_size;
 uint16_t read_total = 0;
 int read_size;
 uint32_t read_timeout = timeout;

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
 return read_total;
 }

/* 函数名：wifi_8710bx_cmd_check_response
*  功能：  检查回应
*  参数：  rsp 回应数组 
*  参数：  err_head 错误头 
*  参数：  complete 回应是否结束 
*  返回：  WIFI_ERR_OK：成功 其他：失败 
*/
static int wifi_8710bx_cmd_check_response(const char *rsp,wifi_8710bx_err_code_t *err_head,bool *complete)
{
    wifi_8710bx_err_code_t *err_node;
    log_warning("*\r\n");
    err_node = err_head;
    while(err_node){
        if (strstr(rsp,err_node->str)){
        *complete = true;
        return err_node->code;
        }
        err_node = err_node->next;
    }
    return WIFI_ERR_OK;     
}
  
/* 函数名：wifi_8710bx_at_cmd_excute
*  功能：  wifi at命令执行
*  参数：  cmd 命令指针 
*  返回：  返回：  WIFI_ERR_OK：成功 其他：失败 
*/  
static int wifi_8710bx_at_cmd_excute(wifi_8710bx_at_cmd_t *at_cmd)
{
    int rc;
    int recv_size;
    uint32_t start_time,cur_time,time_left;
    
    rc = wifi_8710bx_at_cmd_send(at_cmd->send,at_cmd->send_size,at_cmd->send_timeout);
    if (rc != WIFI_ERR_OK){
        return rc;
    }
    
    start_time = osKernelSysTick();
    time_left = at_cmd->recv_timeout;
    while (time_left){
    recv_size = wifi_8710bx_at_cmd_recv(at_cmd->recv + at_cmd->recv_size,WIFI_8710BX_RECV_BUFFER_SIZE - at_cmd->recv_size,time_left);
    if (recv_size < 0){
    return recv_size; 
    }
    
    rc = wifi_8710bx_cmd_check_response(at_cmd->recv + at_cmd->recv_size,at_cmd->err_head,&at_cmd->complete);
    at_cmd->recv_size += recv_size;
    if(at_cmd->complete){
    return rc;
    }
    cur_time = osKernelSysTick();
    time_left = at_cmd->recv_timeout > cur_time - start_time ? at_cmd->recv_timeout - (cur_time - start_time) : 0;
    }
    
    return WIFI_ERR_UNKNOW;
}


/* 函数名：wifi_8710bx_reset
*  功能：  wifi软件复位 
*  参数：  无
*  返回：  WIFI_ERR_OK：成功 其他：失败
*/
int wifi_8710bx_reset(void)
{
 int rc;
 wifi_8710bx_err_code_t ok,err;
 
 osMutexWait(wifi_mutex,osWaitForever);
 
 memset(&wifi_cmd,0,sizeof(wifi_cmd));
 strcpy(wifi_cmd.send,"ATSR\r\n");
 wifi_cmd.send_size = strlen(wifi_cmd.send);
 wifi_cmd.send_timeout = 5;
 wifi_cmd.recv_timeout = 1000;
 ok.str = "[ATSR] OK";
 ok.code = WIFI_ERR_OK;
 ok.next = NULL;
 err.str = "[ATSR] ERROR";
 err.code = WIFI_ERR_CMD_ERR;
 err.next = NULL;
 
 wifi_8710bx_err_code_add(&wifi_cmd.err_head,&ok);
 wifi_8710bx_err_code_add(&wifi_cmd.err_head,&err);
 
 rc = wifi_8710bx_at_cmd_excute(&wifi_cmd);

 wifi_8710bx_print_err_info(wifi_cmd.send,rc);
 osMutexRelease(wifi_mutex);
 return rc; 
}

/* 函数名：wifi_8710bx_set_mode
*  功能：  设置工作模式 
*  参数：  mode 模式 station or ap
*  返回：  WIFI_ERR_OK：成功 其他：失败
*/
int wifi_8710bx_set_mode(const wifi_8710bx_mode_t mode)
{
 int rc;
 wifi_8710bx_err_code_t ok,err;
 
 osMutexWait(wifi_mutex,osWaitForever);
 
 memset(&wifi_cmd,0,sizeof(wifi_cmd));
 strcpy(wifi_cmd.send,mode == WIFI_8710BX_STATION_MODE ? "ATPW=1\r\n" : "ATPW=2\r\n");
 wifi_cmd.send_size = strlen(wifi_cmd.send);
 wifi_cmd.send_timeout = 5;
 wifi_cmd.recv_timeout = 1000;
 ok.str = "[ATPW] OK";
 ok.code = WIFI_ERR_OK;
 ok.next = NULL;
 err.str = "[ATPW] ERROR";
 err.code = WIFI_ERR_CMD_ERR;
 err.next = NULL;
 
 wifi_8710bx_err_code_add(&wifi_cmd.err_head,&ok);
 wifi_8710bx_err_code_add(&wifi_cmd.err_head,&err);
 
 rc = wifi_8710bx_at_cmd_excute(&wifi_cmd);

 wifi_8710bx_print_err_info(wifi_cmd.send,rc);
 osMutexRelease(wifi_mutex);
 return rc; 
}

/* 函数名：wifi_8710bx_set_echo
*  功能：  设置回显模式 
*  参数：  echo 回显值 打开或者关闭
*  返回：  WIFI_ERR_OK：成功 其他：失败
*/
int wifi_8710bx_set_echo(const wifi_8710bx_echo_t echo)
{
 int rc;
 wifi_8710bx_err_code_t ok,err;
 
 osMutexWait(wifi_mutex,osWaitForever);
 
 memset(&wifi_cmd,0,sizeof(wifi_cmd));
 strcpy(wifi_cmd.send,echo == WIFI_8710BX_ECHO_ON ? "ATSE=1\r\n" : "ATSE=0\r\n");
 wifi_cmd.send_size = strlen(wifi_cmd.send);
 wifi_cmd.send_timeout = 5;
 wifi_cmd.recv_timeout = 1000;
 ok.str = "[ATSE] OK";
 ok.code = WIFI_ERR_OK;
 ok.next = NULL;
 err.str = "[ATSE] ERROR";
 err.code = WIFI_ERR_CMD_ERR;
 err.next = NULL;
 
 wifi_8710bx_err_code_add(&wifi_cmd.err_head,&ok);
 wifi_8710bx_err_code_add(&wifi_cmd.err_head,&err);
 
 rc = wifi_8710bx_at_cmd_excute(&wifi_cmd);

 wifi_8710bx_print_err_info(wifi_cmd.send,rc);
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
 wifi_8710bx_err_code_t ok,err;
 
 osMutexWait(wifi_mutex,osWaitForever);
 
 memset(&wifi_cmd,0,sizeof(wifi_cmd));
 snprintf(wifi_cmd.send,WIFI_8710BX_SEND_BUFFER_SIZE,"ATPA=%s,%s,%d,%d\r\n",ap->ssid,ap->passwd,ap->chn,(int)ap->hidden);
 wifi_cmd.send_size = strlen(wifi_cmd.send);
 wifi_cmd.send_timeout = 5;
 wifi_cmd.recv_timeout = 1000;
 ok.str = "[ATPA] OK";
 ok.code = WIFI_ERR_OK;
 ok.next = NULL;
 err.str = "[ATPA] ERROR";
 err.code = WIFI_ERR_CMD_ERR;
 err.next = NULL;
 
 wifi_8710bx_err_code_add(&wifi_cmd.err_head,&ok);
 wifi_8710bx_err_code_add(&wifi_cmd.err_head,&err);
 
 rc = wifi_8710bx_at_cmd_excute(&wifi_cmd);

 wifi_8710bx_print_err_info(wifi_cmd.send,rc);
 osMutexRelease(wifi_mutex);
 return rc; 
}

/* 函数名：wifi_8710bx_get_ap_rssi
*  功能：  扫描出指定ap的rssi值 
*  参数：  ssid ap的ssid名称
*  参数：  rssi ap的rssi的指针
*  返回：  WIFI_ERR_OK：成功 其他：失败
*/
int wifi_8710bx_get_ap_rssi(const char *ssid,int *rssi)
{
 int rc;
 char *tolon_str,*ssid_str;
 wifi_8710bx_err_code_t ok,err;
 
 osMutexWait(wifi_mutex,osWaitForever);
 
 memset(&wifi_cmd,0,sizeof(wifi_cmd));
 strcpy(wifi_cmd.send,"ATWS\r\n");
 wifi_cmd.send_size = strlen(wifi_cmd.send);
 wifi_cmd.send_timeout = 5;
 wifi_cmd.recv_timeout = 10000;
 ok.str = "[ATWS] OK";
 ok.code = WIFI_ERR_OK;
 ok.next = NULL;
 err.str = "[ATWS] ERROR";
 err.code = WIFI_ERR_CMD_ERR;
 err.next = NULL;
 
 wifi_8710bx_err_code_add(&wifi_cmd.err_head,&ok);
 wifi_8710bx_err_code_add(&wifi_cmd.err_head,&err);
 
 rc = wifi_8710bx_at_cmd_excute(&wifi_cmd);
 
 if(rc == WIFI_ERR_OK){
 ssid_str = strstr(wifi_cmd.recv,ssid);
 if(ssid_str == NULL){
 /*没有发现指定ssid rssi默认为0*/
 *rssi = 0;
 goto err_exit;
 }
 /*ssid后第3个tolon是rssi值*/
 for(uint8_t i = 0;i < 3; i ++){
 tolon_str = strstr(ssid_str,",");
 if(tolon_str == NULL){
 rc = WIFI_ERR_UNKNOW;
 goto err_exit; 
 }
 ssid_str = tolon_str + 1; 
 }
 *rssi = atoi(ssid_str);
 }
 
err_exit:
 wifi_8710bx_print_err_info(wifi_cmd.send,rc);
 osMutexRelease(wifi_mutex);
 return rc;
}

/* 函数名：wifi_8710bx_connect_ap
*  功能：  连接指定的ap
*  参数：  ssid ap的ssid名称
*  参数：  passwd ap的passwd
*  返回：  WIFI_ERR_OK：成功 其他：失败
*/
int wifi_8710bx_connect_ap(const char *ssid,const char *passwd)
{
 int rc;
 wifi_8710bx_err_code_t ok,err;
 
 osMutexWait(wifi_mutex,osWaitForever);
 
 memset(&wifi_cmd,0,sizeof(wifi_cmd));
 snprintf(wifi_cmd.send,WIFI_8710BX_SEND_BUFFER_SIZE,"ATPN=%s,%s\r\n",ssid,passwd);
 wifi_cmd.send_size = strlen(wifi_cmd.send);
 wifi_cmd.send_timeout = 5;
 wifi_cmd.recv_timeout = 10000;
 ok.str = "[ATPN] OK";
 ok.code = WIFI_ERR_OK;
 ok.next = NULL;
 err.str = "[ATPN] ERROR";
 err.code = WIFI_ERR_CMD_ERR;
 err.next = NULL;
 
 wifi_8710bx_err_code_add(&wifi_cmd.err_head,&ok);
 wifi_8710bx_err_code_add(&wifi_cmd.err_head,&err);
 
 rc = wifi_8710bx_at_cmd_excute(&wifi_cmd);
 
 wifi_8710bx_print_err_info(wifi_cmd.send,rc);
 osMutexRelease(wifi_mutex);
 return rc;
}

/* 函数名：wifi_8710bx_disconnect_ap
*  功能：  断开指定的ap连接
*  参数：  无
*  返回：  WIFI_ERR_OK：成功 其他：失败
*/
int wifi_8710bx_disconnect_ap(void)
{
 int rc;
 wifi_8710bx_err_code_t ok,err;
 
 osMutexWait(wifi_mutex,osWaitForever);
 
 memset(&wifi_cmd,0,sizeof(wifi_cmd));
 snprintf(wifi_cmd.send,WIFI_8710BX_SEND_BUFFER_SIZE,"ATWD\r\n");
 wifi_cmd.send_size = strlen(wifi_cmd.send);
 wifi_cmd.send_timeout = 5;
 wifi_cmd.recv_timeout = 2000;
 ok.str = "[ATWD] OK";
 ok.code = WIFI_ERR_OK;
 ok.next = NULL;
 err.str = "[ATWD] ERROR";
 err.code = WIFI_ERR_CMD_ERR;
 err.next = NULL;
 
 wifi_8710bx_err_code_add(&wifi_cmd.err_head,&ok);
 wifi_8710bx_err_code_add(&wifi_cmd.err_head,&err);
 
 rc = wifi_8710bx_at_cmd_excute(&wifi_cmd);
 
 wifi_8710bx_print_err_info(wifi_cmd.send,rc);
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
 log_debug("dump wifi info ok.\r\n");
 return WIFI_ERR_OK;    
}

/* 函数名：wifi_8710bx_get_wifi_device
*  功能：  获取wifi设备信息
*  参数：  wifi_device 设备指针
*  返回：  WIFI_ERR_OK：成功 其他：失败
*/
int wifi_8710bx_get_wifi_device(wifi_8710bx_device_t *wifi_device)
{
 int rc;
 wifi_8710bx_err_code_t ok,err;
 
 osMutexWait(wifi_mutex,osWaitForever);
 
 memset(&wifi_cmd,0,sizeof(wifi_cmd));
 snprintf(wifi_cmd.send,WIFI_8710BX_SEND_BUFFER_SIZE,"ATW?\r\n");
 wifi_cmd.send_size = strlen(wifi_cmd.send);
 wifi_cmd.send_timeout = 5;
 wifi_cmd.recv_timeout = 10000;
 ok.str = "[ATW?] OK";
 ok.code = WIFI_ERR_OK;
 ok.next = NULL;
 err.str = "[ATW?] ERROR";
 err.code = WIFI_ERR_CMD_ERR;
 err.next = NULL;
 
 wifi_8710bx_err_code_add(&wifi_cmd.err_head,&ok);
 wifi_8710bx_err_code_add(&wifi_cmd.err_head,&err);
 
 rc = wifi_8710bx_at_cmd_excute(&wifi_cmd);
 if(rc == WIFI_ERR_OK){
 rc = wifi_8710bx_dump_wifi_device_info(wifi_cmd.recv,wifi_device);
 }
 
 wifi_8710bx_print_err_info(wifi_cmd.send,rc);
 osMutexRelease(wifi_mutex);
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

/* 函数名：wifi_8710bx_get_connection
*  功能：  获取连接信息
*  参数：  connection_list 连接列表指针
*  返回：  WIFI_ERR_OK：成功 其他：失败
*/
int wifi_8710bx_get_connection(wifi_8710bx_connection_list_t *connection_list)
{
 int rc;
 wifi_8710bx_err_code_t ok,err;
 
 osMutexWait(wifi_mutex,osWaitForever);
 
 memset(&wifi_cmd,0,sizeof(wifi_cmd));
 strcpy(wifi_cmd.send,"ATW?\r\n");
 wifi_cmd.send_size = strlen(wifi_cmd.send);
 wifi_cmd.send_timeout = 5;
 wifi_cmd.recv_timeout = 1000;
 ok.str = "[ATPI] OK";
 ok.code = WIFI_ERR_OK;
 ok.next = NULL;
 err.str = "[ATPI] ERROR";
 err.code = WIFI_ERR_CMD_ERR;
 err.next = NULL;
 
 wifi_8710bx_err_code_add(&wifi_cmd.err_head,&ok);
 wifi_8710bx_err_code_add(&wifi_cmd.err_head,&err);
 
 rc = wifi_8710bx_at_cmd_excute(&wifi_cmd);
 if(rc == WIFI_ERR_OK){
 rc = wifi_8710bx_dump_connection_info(wifi_cmd.recv,connection_list);
 }
 
 wifi_8710bx_print_err_info(wifi_cmd.send,rc);
 osMutexRelease(wifi_mutex);
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

pos = strstr(buffer,"con_id=") + strlen("con_id=");
if(pos == NULL){
return WIFI_ERR_UNKNOW;
}
conn_id = atoi(pos);
  
return conn_id;
}

/* 函数名：wifi_8710bx_open_server
*  功能：  创建服务端
*  参数：  port 服务端本地端口
*  参数：  protocol   服务端网络协议类型
*  返回：  连接句柄，大于WIFI_ERR_OK：成功 其他：失败
*/
int wifi_8710bx_open_server(const uint16_t port,const wifi_8710bx_net_protocol_t protocol)
{
 int rc;
 wifi_8710bx_err_code_t ok,err;
 
 osMutexWait(wifi_mutex,osWaitForever);
 
 memset(&wifi_cmd,0,sizeof(wifi_cmd));
 snprintf(wifi_cmd.send,WIFI_8710BX_SEND_BUFFER_SIZE,"ATPS=%s,%d\r\n",protocol == WIFI_8710BX_NET_PROTOCOL_TCP ? "0" : "1",port);
 wifi_cmd.send_size = strlen(wifi_cmd.send);
 wifi_cmd.send_timeout = 5;
 wifi_cmd.recv_timeout = 10000;
 ok.str = "[ATPS] OK";
 ok.code = WIFI_ERR_OK;
 ok.next = NULL;
 err.str = "[ATPS] ERROR";
 err.code = WIFI_ERR_CMD_ERR;
 err.next = NULL;
 
 wifi_8710bx_err_code_add(&wifi_cmd.err_head,&ok);
 wifi_8710bx_err_code_add(&wifi_cmd.err_head,&err);
 
 rc = wifi_8710bx_at_cmd_excute(&wifi_cmd);
 if(rc == WIFI_ERR_OK){
 rc = wifi_8710bx_dump_conn_id(wifi_cmd.recv);  
 }
 
 wifi_8710bx_print_err_info(wifi_cmd.send,rc);
 osMutexRelease(wifi_mutex);
 return rc;
}

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
 wifi_8710bx_err_code_t ok,err;
 
 osMutexWait(wifi_mutex,osWaitForever);
 
 memset(&wifi_cmd,0,sizeof(wifi_cmd));
 snprintf(wifi_cmd.send,WIFI_8710BX_SEND_BUFFER_SIZE,"ATPC=%s,%s,%d\r\n",protocol == WIFI_8710BX_NET_PROTOCOL_TCP ? "0" : "1",host,remote_port);
 wifi_cmd.send_size = strlen(wifi_cmd.send);
 wifi_cmd.send_timeout = 5;
 wifi_cmd.recv_timeout = 10000;
 ok.str = "[ATPC] OK";
 ok.code = WIFI_ERR_OK;
 ok.next = NULL;
 err.str = "[ATPC] ERROR";
 err.code = WIFI_ERR_CMD_ERR;
 err.next = NULL;
 
 wifi_8710bx_err_code_add(&wifi_cmd.err_head,&ok);
 wifi_8710bx_err_code_add(&wifi_cmd.err_head,&err);
 
 rc = wifi_8710bx_at_cmd_excute(&wifi_cmd);
 if(rc == WIFI_ERR_OK){
 rc = wifi_8710bx_dump_conn_id(wifi_cmd.recv);  
 }
 
 wifi_8710bx_print_err_info(wifi_cmd.send,rc);
 osMutexRelease(wifi_mutex);
 return rc;
}

/* 函数名：wifi_8710bx_close
*  功能：  关闭客户或者服务端域      
*  参数：  conn_id     连接句柄
*  返回：  WIFI_ERR_OK：成功 其他：失败
*/
int wifi_8710bx_close(const int conn_id)
{
 int rc;
 wifi_8710bx_err_code_t ok,err;
 
 osMutexWait(wifi_mutex,osWaitForever);
 
 memset(&wifi_cmd,0,sizeof(wifi_cmd));
 snprintf(wifi_cmd.send,WIFI_8710BX_SEND_BUFFER_SIZE,"ATPD=%d\r\n",conn_id);
 wifi_cmd.send_size = strlen(wifi_cmd.send);
 wifi_cmd.send_timeout = 5;
 wifi_cmd.recv_timeout = 2000;
 ok.str = "[ATPD] OK";
 ok.code = WIFI_ERR_OK;
 ok.next = NULL;
 err.str = "[ATPD] ERROR";
 err.code = WIFI_ERR_CMD_ERR;
 err.next = NULL;
 
 wifi_8710bx_err_code_add(&wifi_cmd.err_head,&ok);
 wifi_8710bx_err_code_add(&wifi_cmd.err_head,&err);
 
 rc = wifi_8710bx_at_cmd_excute(&wifi_cmd);

 wifi_8710bx_print_err_info(wifi_cmd.send,rc);
 osMutexRelease(wifi_mutex);
 return rc;
}

/* 函数名：wifi_8710bx_send
*  功能：  发送数据
*  参数：  conn_id 连接句柄
*  参数：  data    数据buffer
*  参数：  size    需要发送的数量
*  返回：  >=0：成功发送的数据 其他：失败
*/
int wifi_8710bx_send(int conn_id,const char *data,const int size)
{
 int rc;
 int used_size,space_size,send_size;
 
 wifi_8710bx_err_code_t ok,err;
 
 osMutexWait(wifi_mutex,osWaitForever);
 
 memset(&wifi_cmd,0,sizeof(wifi_cmd));
 snprintf(wifi_cmd.send,WIFI_8710BX_SEND_BUFFER_SIZE,"ATPT=%d,%d:",size,conn_id);
 used_size = strlen(wifi_cmd.send);
 space_size = WIFI_8710BX_SEND_BUFFER_SIZE - used_size;
 send_size = space_size < size ? space_size :size;
 memcpy(wifi_cmd.send + used_size,data,send_size);
 wifi_cmd.send_size = used_size + send_size;
 wifi_cmd.send_timeout = send_size / 8 + 10;
 wifi_cmd.recv_timeout = 10000;
 ok.str = "[ATPT] OK";
 ok.code = WIFI_ERR_OK;
 ok.next = NULL;
 err.str = "[ATPT] ERROR";
 err.code = WIFI_ERR_CMD_ERR;
 err.next = NULL;
 
 wifi_8710bx_err_code_add(&wifi_cmd.err_head,&ok);
 wifi_8710bx_err_code_add(&wifi_cmd.err_head,&err);
 
 rc = wifi_8710bx_at_cmd_excute(&wifi_cmd);
 if(rc == WIFI_ERR_OK){
 rc = send_size;
 }

 wifi_8710bx_print_err_info(wifi_cmd.send,rc);
 osMutexRelease(wifi_mutex);
 return rc;
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
 int rc;
 int data_size;
 char *size_str,*data_start_str; 
 wifi_8710bx_err_code_t ok,err;
 
 osMutexWait(wifi_mutex,osWaitForever);
 
 memset(&wifi_cmd,0,sizeof(wifi_cmd));
 snprintf(wifi_cmd.send,WIFI_8710BX_SEND_BUFFER_SIZE,"ATPR=%d,%d\r\n",conn_id,size);
 wifi_cmd.send_size = strlen(wifi_cmd.send);
 wifi_cmd.send_timeout = 5;
 wifi_cmd.recv_timeout = 2000;
 ok.str = "[ATPR] OK";
 ok.code = WIFI_ERR_OK;
 ok.next = NULL;
 err.str = "[ATPR] ERROR";
 err.code = WIFI_ERR_CMD_ERR;
 err.next = NULL;
 
 wifi_8710bx_err_code_add(&wifi_cmd.err_head,&ok);
 wifi_8710bx_err_code_add(&wifi_cmd.err_head,&err);
 
 rc = wifi_8710bx_at_cmd_excute(&wifi_cmd);
 if(rc == WIFI_ERR_OK){
 size_str = strstr(wifi_cmd.recv,"[ATPR] OK") + strlen("[ATPR] OK") + 1;
 data_size = atoi(size_str);
 data_size = data_size > size ? size : data_size ;
 if(data_size > 0){
 data_start_str = strstr(size_str,":");
 if(data_start_str){
 memcpy(buffer,data_start_str + 1,data_size);
 }else{
 rc = WIFI_ERR_UNKNOW;
 goto err_exit;
 }
 }
 rc = data_size;  
 }
 
err_exit:
 wifi_8710bx_print_err_info(wifi_cmd.send,rc);
 osMutexRelease(wifi_mutex);
 return rc;
}
