#include "beer_machine.h"
#include "serial.h"
#include "gsm_m6312.h"
#include "cmsis_os.h"
#include "log.h"
#define  LOG_MODULE_LEVEL    LOG_LEVEL_DEBUG
#define  LOG_MODULE_NAME     "[gsm]"

extern int gsm_m6312_serial_handle;
extern serial_hal_driver_t gsm_m6312_serial_driver;
static osMutexId gsm_mutex;


#define  GSM_M6312_MALLOC(x)           pvPortMalloc((x))
#define  GSM_M6312_FREE(x)             vPortFree((x))

#define  GSM_M6312_PWR_ON_DELAY        4000
#define  GSM_M6312_PWR_OFF_DELAY       10000


/* 函数名：gsm_m6312_pwr_on
*  功能：  m6312 2g模块开机
*  参数：  无 
*  返回：  0：成功 其他：失败
*/
int gsm_m6312_pwr_on(void)
{
	int rc = GSM_ERR_HAL_GPIO;
	uint32_t timeout = 0;
	if (bsp_get_gsm_pwr_status() == BSP_GSM_STATUS_PWR_ON) {
	return GSM_ERR_OK;
	}
	bsp_gsm_pwr_key_press();
	while (timeout < GSM_M6312_PWR_ON_DELAY) {
		osDelay(100);
		timeout +=100;
		if (bsp_get_gsm_pwr_status() == BSP_GSM_STATUS_PWR_ON) {
		rc = GSM_ERR_OK;
		break;
		}
	}
	bsp_gsm_pwr_key_release();  
	return rc;
}

/* 函数名：gsm_m6312_pwr_off
*  功能：  m6312 2g模块关机
*  参数：  无 
*  返回：  0：成功 其他：失败
*/

int gsm_m6312_pwr_off(void)
{
	int rc = GSM_ERR_HAL_GPIO;
	uint32_t timeout = 0;

	if (bsp_get_gsm_pwr_status() == BSP_GSM_STATUS_PWR_OFF) {
	return GSM_ERR_OK;
	}
	bsp_gsm_pwr_key_press();
	
	while(timeout < GSM_M6312_PWR_OFF_DELAY){
		osDelay(100);
		timeout +=100;
		if(bsp_get_gsm_pwr_status() == BSP_GSM_STATUS_PWR_OFF){
		rc = GSM_ERR_OK;
		break;
		}
	}
	bsp_gsm_pwr_key_release();  
	return rc; 
}


/* 函数名：gsm_m6312_serial_hal_init
*  功能：  m6312 2g模块执行硬件和串口初始化
*  参数：  无 
*  返回：  0：成功 其他：失败
*/
int gsm_m6312_serial_hal_init(void)
{
	int rc ;
    rc = serial_create(&gsm_m6312_serial_handle,GSM_M6312_BUFFER_SIZE,GSM_M6312_BUFFER_SIZE);
    if (rc != 0) {
    	log_error("m6312 create serial hal err.\r\n");
    	return GSM_ERR_HAL_INIT;
    }
    log_debug("m6312 create serial hal ok.\r\n");
   
    rc = serial_register_hal_driver(gsm_m6312_serial_handle,&gsm_m6312_serial_driver);
	if (rc != 0) {
      log_error("m6312 register serial hal driver err.\r\n");
      return GSM_ERR_HAL_INIT;
    }
	log_debug("m6312 register serial hal driver ok.\r\n");
	rc = serial_open(gsm_m6312_serial_handle,GSM_M6312_SERIAL_PORT,GSM_M6312_SERIAL_BAUDRATES,GSM_M6312_SERIAL_DATA_BITS,GSM_M6312_SERIAL_STOP_BITS);
	if (rc != 0) {
    	log_error("m6312 open serial hal err.\r\n");
    	return GSM_ERR_HAL_INIT;
  	}
	log_debug("m6312 open serial hal ok.\r\n");
	
	osMutexDef(gsm_mutex);
	gsm_mutex = osMutexCreate(osMutex(gsm_mutex));
	if (gsm_mutex == NULL) {
	log_error("create gsm mutex err.\r\n");
	return GSM_ERR_HAL_INIT;
	}
	osMutexRelease(gsm_mutex);
	log_debug("create gsm mutex ok.\r\n");

	return GSM_ERR_OK; 
}

/* 函数名：gsm_m6312_print_err_info
*  功能：  打印错误信息
*  参数：  err_code 错误码 
*  返回：  无
*/
static void gsm_m6312_print_err_info(const char *info,const int err_code)
{
  if(info){
  log_debug("%s\r\n",info);
  }
  switch(err_code)
  {
  case  GSM_ERR_OK:
  log_debug("GSM ERR CODE:%d cmd OK.\r\n",err_code);
  break;

  case  GSM_ERR_MALLOC_FAIL:
  log_error("GSM ERR CODE:%d malloc fail.\r\n",err_code);
  break;
    
  case  GSM_ERR_CMD_ERR:
  log_error("GSM ERR CODE:%d cmd execute err.\r\n",err_code);
  break;  
  
  case  GSM_ERR_CMD_TIMEOUT:
  log_error("GSM ERR CODE:%d cmd execute timeout.\r\n",err_code);   
  break;
  
  case  GSM_ERR_RSP_TIMEOUT:
  log_error("GSM ERR CODE:%d cmd rsp timeout.\r\n",err_code);   
  break;
  
  case  GSM_ERR_SERIAL_SEND:
  log_error("GSM ERR CODE:%d serial send err.\r\n",err_code);   
  break;
  
  case  GSM_ERR_SERIAL_RECV:
  log_error("GSM ERR CODE:%d serial recv err.\r\n",err_code);   
  break;
 
  case  GSM_ERR_RECV_NO_SPACE:
  log_error("GSM ERR CODE:%d serial recv no space.\r\n",err_code);   
  break;
  
  case  GSM_ERR_SOCKET_ALREADY_CONNECT:
  log_error("GSM ERR CODE:%d socket alread connect.\r\n",err_code);   
  break;
  
  case  GSM_ERR_CONNECT_FAIL:
  log_error("GSM ERR CODE:%d socket connect fail.\r\n",err_code);   
  break;
  
  case  GSM_ERR_SOCKET_SEND_FAIL:
  log_error("GSM ERR CODE:%d socket send fail.\r\n",err_code);   
  break;
  
  case  GSM_ERR_SOCKET_DISCONNECT:
  log_error("GSM ERR CODE:%d socket disconnect err .\r\n",err_code);   
  break;
  
  case  GSM_ERR_HAL_GPIO:
  log_error("GSM ERR CODE:%d hal gpio err.\r\n",err_code);   
  break;
  
  case  GSM_ERR_HAL_INIT:
  log_error("GSM ERR CODE:%d hal init err.\r\n",err_code);   
  break;
  
  case  GSM_ERR_UNKNOW:
  log_error("GSM ERR CODE:%d unknow err.\r\n",err_code);   
  break;
  
  default:
  log_error("GSM ERR CODE:%d invalid err code.\r\n",err_code);   
  }
}



static int gsm_m6312_at_cmd_send(const char *send,const uint16_t size,const uint32_t timeout)
{
 uint16_t write_total = 0;
 uint16_t remain_total = size;
 uint32_t write_timeout = timeout;
 int write_len;
 /*清空serial buffer*/
 serial_flush(gsm_m6312_serial_handle);
 do{
 write_timeout -- ;
 write_len = serial_write(gsm_m6312_serial_handle,(uint8_t *)send + write_total,remain_total);
 if(write_len == -1){
 log_error("at cmd write err.\r\n");
 return GSM_ERR_SERIAL_SEND;  
 }
 write_total += write_len;
 remain_total -= write_len;
 }while(remain_total > 0 && write_timeout > 0);  
  
 if(remain_total > 0){
 return GSM_ERR_SERIAL_SEND;
 }

 if( serial_complete(gsm_m6312_serial_handle,write_timeout) != 0){
 return GSM_ERR_SERIAL_SEND;  ;
 }
 return GSM_ERR_OK;
}

#define  GSM_M6312_SELECT_TIMEOUT          5
static int gsm_m6312_at_cmd_recv(char *recv,const uint16_t size,const uint32_t timeout)
{
 int select_size;
 uint16_t read_total = 0;
 int read_size;
 uint32_t read_timeout = timeout;
 /*清空serial buffer*/
 serial_flush(gsm_m6312_serial_handle); 
 select_size = serial_select(gsm_m6312_serial_handle,read_timeout);
 if(select_size < 0){
 return GSM_ERR_SERIAL_RECV;
 } 
 if(select_size == 0){
 return GSM_ERR_RSP_TIMEOUT;  
 }
 do{
 if(read_total + select_size > size){
 return GSM_ERR_RECV_NO_SPACE;
 }
 read_size = serial_read(gsm_m6312_serial_handle,(uint8_t *)recv + read_total,select_size); 
 if(read_size < 0){
 return GSM_ERR_SERIAL_RECV;  
 }
 read_total += read_size;
 select_size = serial_select(gsm_m6312_serial_handle,GSM_M6312_SELECT_TIMEOUT);
 if(select_size < 0){
 return GSM_ERR_SERIAL_RECV;
 } 
 }while(select_size != 0);
 
 return GSM_ERR_OK;
 }

  
#define  GSM_M6312_GET_SIM_ID_SEND_TIMEOUT         5
#define  GSM_M6312_GET_SIM_ID_RSP_TIMEOUT          1000

/* 函数名：gsm_m6312_get_sim_card_id
*  功能：  获取 sim card id
*  参数：  sim_id 指针 
*  返回：  0：ready 其他：失败
*/
int gsm_m6312_get_sim_card_id(char *sim_id)
{
 int rc =  GSM_ERR_UNKNOW;
 char rsp[40] = { 0 };
 const char *at_cmd = "AT+CCID?\r\n";
 char *ccid_str;
 osMutexWait(gsm_mutex,osWaitForever);
 
 rc = gsm_m6312_at_cmd_send(at_cmd,strlen(at_cmd),GSM_M6312_GET_SIM_ID_SEND_TIMEOUT);
 if(rc != GSM_ERR_OK){
 goto err_exit;
 }
 
 rc = gsm_m6312_at_cmd_recv(rsp,40,GSM_M6312_GET_SIM_ID_RSP_TIMEOUT);
 if(rc != GSM_ERR_OK){
 goto err_exit;
 }
 
 if(strstr(rsp,"ERROR")){ 
 rc = GSM_ERR_CMD_ERR;  
 goto err_exit;
 }
 
 if(strstr(rsp,"OK")){ 
 ccid_str = strstr(rsp,"+CCID: ");
 if(ccid_str){
 memcpy(sim_id,ccid_str + strlen("+CCID: "),20); 
 sim_id[20] = '\0';
 rc = GSM_ERR_OK;
 }
 }
 
err_exit:
 gsm_m6312_print_err_info(at_cmd,rc);
 return rc;  
 }
 
#define  GSM_M6312_SET_ECHO_SEND_TIMEOUT             5
#define  GSM_M6312_SET_ECHO_RSP_TIMEOUT              1000


/* 函数名：gsm_m6312_set_echo
*  功能：  设置是否回显输入的命令
*  参数：  echo 回显设置 
*  返回：  0：成功 其他：失败
*/
int gsm_m6312_set_echo(gsm_m6312_echo_t echo)
{
 int rc;
 char rsp[10] = { 0 };
 char *at_cmd;
 
 osMutexWait(gsm_mutex,osWaitForever);
 
 if(echo == GSM_M6312_ECHO_ON){
 at_cmd = "ATE1\r\n";
 }else{
 at_cmd = "ATE0\r\n";
 }
 rc = gsm_m6312_at_cmd_send(at_cmd,strlen(at_cmd),GSM_M6312_SET_ECHO_SEND_TIMEOUT);
 if(rc != GSM_ERR_OK){
 goto err_exit;
 }
 rc = gsm_m6312_at_cmd_recv(rsp,10,GSM_M6312_SET_ECHO_RSP_TIMEOUT);
 if(rc != GSM_ERR_OK){
 goto err_exit;
 }
 
 if(strstr(rsp,"ERROR")){
 rc = GSM_ERR_CMD_ERR;
 goto err_exit;
 }
 
 if(strstr(rsp,"OK") == NULL){
 rc = GSM_ERR_UNKNOW;
 }
 rc = GSM_ERR_OK;
 
err_exit:
 gsm_m6312_print_err_info(at_cmd,rc);
 osMutexRelease(gsm_mutex);
 return rc;   
} 


#define  GSM_M6312_GET_IMEI_SEND_TIMEOUT     5
#define  GSM_M6312_GET_IMEI_RSP_TIMEOUT      1000
/* 函数名：gsm_m6312_get_imei
*  功能：  获取设备IMEI串号
*  参数：  imei imei指针
*  返回：  0：ready 其他：失败
*/
int gsm_m6312_get_imei(char *imei)
{
 int rc;
 const char *at_cmd = "AT+CGSN\r\n";
 char rsp[30] = { 0 };
 char *p = NULL;
 
 osMutexWait(gsm_mutex,osWaitForever);
 
 rc = gsm_m6312_at_cmd_send(at_cmd,strlen(at_cmd),GSM_M6312_GET_IMEI_SEND_TIMEOUT);
 if(rc != GSM_ERR_OK){
 goto err_exit;
 }
 
 rc = gsm_m6312_at_cmd_recv(rsp,30,GSM_M6312_GET_IMEI_RSP_TIMEOUT);
 if(rc != GSM_ERR_OK){
 goto err_exit;
 }
 
 if(strstr(rsp,"ERROR")){
 rc = GSM_ERR_CMD_ERR;
 goto err_exit;
 }
 if(strstr(rsp,"OK") == NULL){
 rc = GSM_ERR_UNKNOW;
 goto err_exit;
 }
 /*找到开始标志*/
 p = strstr(rsp,"+CGSN: ");
 if(p == NULL){
 rc = GSM_ERR_UNKNOW;
 goto err_exit;
 }
 /*imei 15位*/
 memcpy(imei,p + strlen("+CGSN: "),15);
 imei[15] = '\0';
 rc = GSM_ERR_OK;
  
err_exit:
 gsm_m6312_print_err_info(at_cmd,rc);
 osMutexRelease(gsm_mutex);
 return rc;
}


#define  GSM_M6312_GET_SN_SEND_TIMEOUT      5
#define  GSM_M6312_GET_SN_RSP_TIMEOUT       1000
/* 函数名：gsm_m6312_get_imei
*  功能：  获取设备IMEI串号
*  参数：  sn sn指针
*  返回：  GSM_ERR_OK：成功 其他：失败
*/
int gsm_m6312_get_sn(char *sn)
{
 int rc;
 const char *at_cmd = "AT^SN\r\n";
 char rsp[40] = { 0 };
 char *sn_str;
 
 if(sn == NULL){
 return GSM_ERR_NULL_POINTER;
 }
 
 osMutexWait(gsm_mutex,osWaitForever);
 
 rc = gsm_m6312_at_cmd_send(at_cmd,strlen(at_cmd),GSM_M6312_GET_SN_SEND_TIMEOUT);
 if(rc != GSM_ERR_OK){
 goto err_exit; 
 }
 
 rc = gsm_m6312_at_cmd_recv(rsp,40,GSM_M6312_GET_IMEI_RSP_TIMEOUT);
 if(rc != GSM_ERR_OK){
 goto err_exit; 
 }
 
 if(strstr(rsp,"ERROR")){
 rc = GSM_ERR_CMD_ERR;
 goto err_exit; 
 }

 sn_str = strstr(rsp,"M6312");
 if(sn_str){
 memcpy(sn,sn_str,20);
 sn[20] = '\0';
 rc = GSM_ERR_OK;
 }else{
 rc = GSM_ERR_UNKNOW;   
 }

err_exit:
 gsm_m6312_print_err_info(at_cmd,rc);  
 osMutexRelease(gsm_mutex);
 return rc; 
}


#define  GSM_M6312_SET_APN_SEND_TIMEOUT           5
#define  GSM_M6312_SET_APN_RSP_TIMEOUT            5000
/* 函数名：gsm_m6312_gprs_set_apn
*  功能：  设置指定cid的gprs的APN
*  参数：  apn 网络APN 
*  返回：  0：成功 其他：失败
*/
int gsm_m6312_set_apn(gsm_m6312_apn_t apn)
{
 int rc;
 const char *at = "AT+CGDCONT=1,\"IP\",%s\r\n";
 char at_cmd[30] ={ 0 };
 char rsp[20] = { 0 };
 char *apn_str;
 
 osMutexWait(gsm_mutex,osWaitForever);
 
 if(apn == GSM_M6312_APN_CMNET){
 apn_str = "CMNET";
 }else{
 apn_str = "UNINET";
 }
 snprintf(at_cmd,30,at,apn_str);
 rc = gsm_m6312_at_cmd_send(at_cmd,strlen(at_cmd),10);
 if(rc != GSM_ERR_OK){
 goto err_exit; 
 }
 
 rc = gsm_m6312_at_cmd_recv(rsp,20,GSM_M6312_SET_APN_RSP_TIMEOUT);
 if(rc != GSM_ERR_OK){
 goto err_exit; 
 }
 
 if(strstr(rsp,"ERROR")){
 rc = GSM_ERR_CMD_ERR;
 goto err_exit; 
 }
 
 if(strstr(rsp,"OK") == NULL){
 rc = GSM_ERR_UNKNOW;
 goto err_exit; 
 }
 rc = GSM_ERR_OK;

err_exit:
 gsm_m6312_print_err_info(at_cmd,rc);
 osMutexRelease(gsm_mutex);
 return rc;  
}

#define  GSM_M6312_GET_APN_SEND_TIMEOUT          5
#define  GSM_M6312_GET_APN_RSP_TIMEOUT           5000

/* 函数名：gsm_m6312_gprs_get_apn
*  功能：  获取apn
*  参数：  apn 指针 
*  返回：  0：成功 其他：失败
*/
int gsm_m6312_get_apn(gsm_m6312_apn_t *apn)
{
 int rc;
 const char *at_cmd = "AT+CGDCONT?\r\n";
 char rsp[30] = { 0 };
 char *apn_str;
 
 osMutexWait(gsm_mutex,osWaitForever);
 
 rc = gsm_m6312_at_cmd_send(at_cmd,strlen(at_cmd),10);
 if(rc != GSM_ERR_OK){
 goto err_exit; 
 }
 
 rc = gsm_m6312_at_cmd_recv(rsp,30,GSM_M6312_GET_APN_RSP_TIMEOUT);
 if(rc != GSM_ERR_OK){
 goto err_exit; 
 }
 
 if(strstr(rsp,"ERROR")){
 rc = GSM_ERR_CMD_ERR;
 goto err_exit; 
 }
 
 if(strstr(rsp,"OK") == NULL){
 rc = GSM_ERR_UNKNOW;
 goto err_exit; 
 }
 
 apn_str = strstr(rsp,"CMNET");
 if(apn_str == NULL){
 apn_str = strstr(rsp,"UNINET");
 if(apn_str == NULL){   
 rc = GSM_ERR_UNKNOW;
 }else{
 *apn = GSM_M6312_APN_UNINET;
 }
 }else{
 *apn = GSM_M6312_APN_CMNET;
 }
 
 rc = GSM_ERR_OK;
 
err_exit:
 osMutexRelease(gsm_mutex);
 return rc;  
}

#define  GSM_M6312_SET_ACTIVE_SEND_TIMEOUT              5
#define  GSM_M6312_SET_ACTIVE_RSP_TIMEOUT               5000
/* 函数名：gsm_m6312_gprs_set_active_status
*  功能：  激活或者去激活GPRS功能
*  参数：  active 状态 
*  返回：  0：成功 其他：失败
*/
int gsm_m6312_set_active_status(gsm_m6312_active_status_t active)
{
 int rc;
 char *at_cmd;
 char rsp[20] = { 0 };
 
 osMutexWait(gsm_mutex,osWaitForever);
 if(active == GSM_M6312_ACTIVE){
 at_cmd = "AT+CGACT=1,1\r\n";
 }else{
 at_cmd = "AT+CGACT=0,1\r\n";
 }
 rc = gsm_m6312_at_cmd_send(at_cmd,strlen(at_cmd),GSM_M6312_SET_ACTIVE_SEND_TIMEOUT);
 if(rc != GSM_ERR_OK){
 goto err_exit; 
 }
 
 rc = gsm_m6312_at_cmd_recv(rsp,20,GSM_M6312_SET_ACTIVE_RSP_TIMEOUT);
 if(rc != GSM_ERR_OK){
 goto err_exit; 
 }
 
 if(strstr(rsp,"ERROR")){
 rc = GSM_ERR_CMD_ERR;
 goto err_exit; 
 }
 
 if(strstr(rsp,"OK") == NULL){
 rc = GSM_ERR_UNKNOW;
 goto err_exit; 
 }
 rc = GSM_ERR_OK;
 
err_exit:
 gsm_m6312_print_err_info(at_cmd,rc);
 osMutexRelease(gsm_mutex);
 return rc;  
}

#define  GSM_M6312_GET_ACTIVE_SEND_TIMEOUT           5
#define  GSM_M6312_GET_ACTIVE_RSP_TIMEOUT            1000

/* 函数名：gsm_m6312_get_active_status
*  功能：  获取激活状态
*  参数：  active 状态指针 
*  返回：  0：成功 其他：失败
*/
int gsm_m6312_get_active_status(gsm_m6312_active_status_t *active)
{
 int rc;
 char *at_cmd = "AT+CGACT?\r\n";
 char rsp[20] = { 0 };

 osMutexWait(gsm_mutex,osWaitForever);
 rc = gsm_m6312_at_cmd_send(at_cmd,strlen(at_cmd),GSM_M6312_GET_ACTIVE_SEND_TIMEOUT);
 
 if(rc != GSM_ERR_OK){
 goto err_exit; 
 }
 
 rc = gsm_m6312_at_cmd_recv(rsp,20,GSM_M6312_GET_ACTIVE_RSP_TIMEOUT);
 if(rc != GSM_ERR_OK){
 goto err_exit; 
 }
 
 if(strstr(rsp,"ERROR")){
 rc = GSM_ERR_CMD_ERR;
 goto err_exit; 
 }
 
 if(strstr(rsp,"OK") == NULL){
 rc = GSM_ERR_UNKNOW;
 goto err_exit; 
 }
 
 if(strstr(rsp,"+CGACT: 0,1") || strstr(rsp,"+CGACT: 0,0")){
 *active = GSM_M6312_INACTIVE;
 }else if(strstr(rsp,"+CGACT: 0,1")){
 *active = GSM_M6312_ACTIVE;
 }else{
 rc = GSM_ERR_UNKNOW;
 goto err_exit;
 }

 rc = GSM_ERR_OK;
err_exit:
 osMutexRelease(gsm_mutex);
 gsm_m6312_print_err_info(at_cmd,rc);
 return rc;   
} 
#define  GSM_M6312_GET_ATTACH_SEND_TIMEOUT        5
#define  GSM_M6312_GET_ATTACH_RSP_TIMEOUT         1000
/* 函数名：gsm_m6312_set_attach_status
*  功能：  设置GPRS网络附着状态
*  参数：  attach GPRS附着状态 
*  返回：  0：成功 其他：失败
*/
int gsm_m6312_set_attach_status(gsm_m6312_attach_status_t attach)
{
 int rc;
 char *at_cmd = "AT+CGATT=1\r\n";
 char rsp[20] = { 0 };

 osMutexWait(gsm_mutex,osWaitForever);
 if(attach == GSM_M6312_ATTACH){
 at_cmd = "AT+CGATT=1\r\n";
 }else{
 at_cmd = "AT+CGATT=0\r\n";  
 }
 rc = gsm_m6312_at_cmd_send(at_cmd,strlen(at_cmd),GSM_M6312_GET_ATTACH_SEND_TIMEOUT);
 
 if(rc != GSM_ERR_OK){
 goto err_exit; 
 }
 
 rc = gsm_m6312_at_cmd_recv(rsp,20,GSM_M6312_GET_ATTACH_RSP_TIMEOUT);
 if(rc != GSM_ERR_OK){
 goto err_exit; 
 }
 
 if(strstr(rsp,"ERROR")){
 rc = GSM_ERR_CMD_ERR;
 goto err_exit; 
 }
 
 if(strstr(rsp,"OK") == NULL){
 rc = GSM_ERR_UNKNOW;
 goto err_exit; 
 }
 rc = GSM_ERR_OK;
 
err_exit:
 gsm_m6312_print_err_info(at_cmd,rc);
 osMutexRelease(gsm_mutex);
 return rc; 
}

#define  GSM_M6312_GET_ATTACH_STATUS_SEND_TIMEOUT        5
#define  GSM_M6312_GET_ATTACH_STATUS_RSP_TIMEOUT         1000
/* 函数名：gsm_m6312_get_attach_status
*  功能：  获取附着状态
*  参数：  attach 附着状态指针
*  返回：  0：成功 其他：失败
*/
int gsm_m6312_get_attach_status(gsm_m6312_attach_status_t *attach)
{
 int rc;
 const char *at_cmd = "AT+CGATT?\r\n";
 char rsp[20] = { 0 };

 osMutexWait(gsm_mutex,osWaitForever);
 rc = gsm_m6312_at_cmd_send(at_cmd,strlen(at_cmd),GSM_M6312_GET_ATTACH_STATUS_SEND_TIMEOUT);
 
 if(rc != GSM_ERR_OK){
 goto err_exit; 
 }
 
 rc = gsm_m6312_at_cmd_recv(rsp,20,GSM_M6312_GET_ACTIVE_RSP_TIMEOUT);
 if(rc != GSM_ERR_OK){
 goto err_exit; 
 }
 
 if(strstr(rsp,"ERROR")){
 rc = GSM_ERR_CMD_ERR;
 goto err_exit; 
 }
 
 if(strstr(rsp,"OK") == NULL){
 rc = GSM_ERR_UNKNOW;
 goto err_exit; 
 }
 
 if(strstr(rsp,"+CGATT: 0")){
 *attach = GSM_M6312_ATTACH;
 rc = GSM_ERR_OK;
 }else if(strstr(rsp,"+CGATT: 1")){
 *attach = GSM_M6312_NOT_ATTACH;    
 rc = GSM_ERR_OK;
 }else{
 rc = GSM_ERR_UNKNOW;
 goto err_exit;
 }

err_exit:
 gsm_m6312_print_err_info(at_cmd,rc);
 osMutexRelease(gsm_mutex);
 return rc; 
}
  

#define  GSM_M6312_SET_CONNECT_MODE_SEND_TIMEOUT        5
#define  GSM_M6312_SET_CONNECT_MODE_RSP_TIMEOUT         2000
/* 函数名：gsm_m6312_set_connect_mode
*  功能：  设置连接模式指令
*  参数：  mode 单路或者多路 
*  返回：  0：成功 其他：失败
*/
int gsm_m6312_set_connect_mode(gsm_m6312_connect_mode_t mode)
{
 int rc;
 char *at_cmd;
 char rsp[20] = { 0 };

 osMutexWait(gsm_mutex,osWaitForever);
 
 if(mode == GSM_M6312_CONNECT_MODE_SINGLE){
 at_cmd = "AT+CMMUX=0\r\n";
 }else{
 at_cmd = "AT+CMMUX=1\r\n";
 }
 rc = gsm_m6312_at_cmd_send(at_cmd,strlen(at_cmd),GSM_M6312_SET_CONNECT_MODE_SEND_TIMEOUT);
 
 if(rc != GSM_ERR_OK){
 goto err_exit; 
 }
 
 rc = gsm_m6312_at_cmd_recv(rsp,20,GSM_M6312_GET_ACTIVE_RSP_TIMEOUT);
 if(rc != GSM_ERR_OK){
 goto err_exit; 
 }
 
 if(strstr(rsp,"ERROR")){
 rc = GSM_ERR_CMD_ERR;
 goto err_exit; 
 }
 
 if(strstr(rsp,"OK") == NULL){
 rc = GSM_ERR_UNKNOW;
 goto err_exit; 
 }
 rc = GSM_ERR_OK;
 
err_exit:
 gsm_m6312_print_err_info(at_cmd,rc);
 osMutexRelease(gsm_mutex);
 return rc; 
}
#define  GSM_M6312_GET_OPERATOR_SEND_TIMEOUT          5
#define  GSM_M6312_GET_OPERATOR_RSP_TIMEOUT           5000
/* 函数名：gsm_m6312_get_operator
*  功能：  查询运营商
*  参数：  operator_name 运营商指针
*  返回：  0：成功 其他：失败
*/
int gsm_m6312_get_operator(operator_name_t *operator_name)
{
 int rc;
 const char *at_cmd = "AT+COPS?\r\n";
 char rsp[20] = { 0 };

 osMutexWait(gsm_mutex,osWaitForever);
 rc = gsm_m6312_at_cmd_send(at_cmd,strlen(at_cmd),GSM_M6312_GET_OPERATOR_SEND_TIMEOUT);
 
 if(rc != GSM_ERR_OK){
 goto err_exit; 
 }
 
 rc = gsm_m6312_at_cmd_recv(rsp,20,GSM_M6312_GET_OPERATOR_RSP_TIMEOUT);
 if(rc != GSM_ERR_OK){
 goto err_exit; 
 }
 
 if(strstr(rsp,"ERROR")){
 rc = GSM_ERR_CMD_ERR;
 goto err_exit; 
 }
 
 if(strstr(rsp,"OK") == NULL){
 rc = GSM_ERR_UNKNOW;
 goto err_exit; 
 }
 
 if(strstr(rsp,"46000") ||strstr(rsp,"CMCC") || strstr(rsp,"ChinaMobile")){
 *operator_name = OPERATOR_CHINA_MOBILE;
 rc = GSM_ERR_OK;
 }else if(strstr(rsp,"46001") ||strstr(rsp,"UNICOM") || strstr(rsp,"ChinaUnicom")){
 *operator_name = OPERATOR_CHINA_UNICOM;
 rc = GSM_ERR_OK;
 }else{
 rc = GSM_ERR_UNKNOW;
 goto err_exit;
 }

err_exit:
 gsm_m6312_print_err_info(at_cmd,rc);
 osMutexRelease(gsm_mutex);
 return rc; 
}

#define  GSM_M6312_SET_AUTO_OPT_FORMAT_SEND_TIMEOUT        5
#define  GSM_M6312_SET_AUTO_OPT_FORMAT_RSP_TIMEOUT         5000
/* 函数名：gsm_m6312_set_auto_operator_format
*  功能：  设置自动运营商选择和运营商格式
*  参数：  operator_format 运营商格式
*  返回：  0：成功 其他：失败
*/
int gsm_m6312_set_auto_operator_format(gsm_m6312_operator_format_t operator_format)
{
 int rc;
 char *at_cmd;
 char rsp[20] = { 0 };

 osMutexWait(gsm_mutex,osWaitForever);
 
 if(operator_format == GSM_M6312_OPERATOR_FORMAT_NUMERIC_NAME){
 at_cmd = "AT+COPS=0,2\r\n";
 }else if(operator_format == GSM_M6312_OPERATOR_FORMAT_SHORT_NAME){
 at_cmd = "AT+COPS=0,1\r\n";
 }else{
 at_cmd = "AT+COPS=0,0\r\n";  
 }
 rc = gsm_m6312_at_cmd_send(at_cmd,strlen(at_cmd),GSM_M6312_SET_AUTO_OPT_FORMAT_SEND_TIMEOUT);
 
 if(rc != GSM_ERR_OK){
 goto err_exit; 
 }
 
 rc = gsm_m6312_at_cmd_recv(rsp,20,GSM_M6312_SET_AUTO_OPT_FORMAT_RSP_TIMEOUT);
 if(rc != GSM_ERR_OK){
 goto err_exit; 
 }
 
 if(strstr(rsp,"ERROR")){
 rc = GSM_ERR_CMD_ERR;
 goto err_exit; 
 }
 
 if(strstr(rsp,"OK") == NULL){
 rc = GSM_ERR_UNKNOW;
 goto err_exit; 
 }
 rc = GSM_ERR_OK;
 
err_exit:
 gsm_m6312_print_err_info(at_cmd,rc);
 osMutexRelease(gsm_mutex);
 return rc; 
}
 
#define  GSM_M6312_SET_SEND_PROMPT_SEND_TIMEOUT       5
#define  GSM_M6312_SET_SEND_PROMPT_RSP_TIMEOUT        1000
/* 函数名：gsm_m6312_set_set_prompt
*  功能：  设置发送提示
*  参数：  prompt 设置的发送提示
*  返回：  0：成功 其他：失败
*/
int gsm_m6312_set_send_prompt(gsm_m6312_send_prompt_t prompt)
{
 int rc;
 char *at_cmd;
 char rsp[20] = { 0 };

 osMutexWait(gsm_mutex,osWaitForever);
 
 if(prompt == GSM_M6312_SEND_PROMPT){
 at_cmd = "AT+PROMPT=1\r\n";
 }else {
 at_cmd = "AT+PROMPT=0\r\n";
 }
 rc = gsm_m6312_at_cmd_send(at_cmd,strlen(at_cmd),GSM_M6312_SET_SEND_PROMPT_SEND_TIMEOUT);
 
 if(rc != GSM_ERR_OK){
 goto err_exit; 
 }
 
 rc = gsm_m6312_at_cmd_recv(rsp,20,GSM_M6312_SET_SEND_PROMPT_RSP_TIMEOUT);
 if(rc != GSM_ERR_OK){
 goto err_exit; 
 }
 
 if(strstr(rsp,"ERROR")){
 rc = GSM_ERR_CMD_ERR;
 goto err_exit; 
 }
 
 if(strstr(rsp,"OK") == NULL){
 rc = GSM_ERR_UNKNOW;
 goto err_exit; 
 }
 rc = GSM_ERR_OK;
 
err_exit:
 gsm_m6312_print_err_info(at_cmd,rc);
 osMutexRelease(gsm_mutex);
 return rc;
}

#define  GSM_M6312_SET_TRANSPARENT_SEND_TIMEOUT         5
#define  GSM_M6312_SET_TRANSPARENT_RSP_TIMEOUT          2000
/* 函数名：gsm_m6312_set_transparent
*  功能：  设置透传模式
*  参数：  transparent 连接ID
*  返回：  0：成功 其他：失败
*/
int gsm_m6312_set_transparent(gsm_m6312_transparent_t transparent)
{
 int rc;
 char *at_cmd;
 char rsp[20] = { 0 };

 osMutexWait(gsm_mutex,osWaitForever);
 
 if(transparent == GSM_M6312_TRANPARENT){
 at_cmd = "AT+CMMODE=1\r\n";
 }else {
 at_cmd = "AT+CMMODE=0\r\n";
 }
 rc = gsm_m6312_at_cmd_send(at_cmd,strlen(at_cmd),GSM_M6312_SET_TRANSPARENT_SEND_TIMEOUT);
 
 if(rc != GSM_ERR_OK){
 goto err_exit; 
 }
 
 rc = gsm_m6312_at_cmd_recv(rsp,20,GSM_M6312_SET_TRANSPARENT_RSP_TIMEOUT);
 if(rc != GSM_ERR_OK){
 goto err_exit; 
 }
 
 if(strstr(rsp,"ERROR")){
 rc = GSM_ERR_CMD_ERR;
 goto err_exit; 
 }
 
 if(strstr(rsp,"OK") == NULL){
 rc = GSM_ERR_UNKNOW;
 goto err_exit; 
 }
 rc = GSM_ERR_OK;
 
err_exit:
 gsm_m6312_print_err_info(at_cmd,rc);
 osMutexRelease(gsm_mutex);
 return rc;
}
#define  GSM_M6312_SET_REPORT_SEND_TIMEOUT                5
#define  GSM_M6312_SET_REPORT_RSP_TIMEOUT                 1000
/* 函数名：gsm_m6312_set_report
*  功能：  关闭信息上报
*  参数：  无
*  返回：  0：成功 其他：失败
*/
int gsm_m6312_set_report(gsm_m6312_report_t report)
{
 int rc;
 char *at_cmd;
 char rsp[20] = { 0 };

 osMutexWait(gsm_mutex,osWaitForever);
 
 if(report == GSM_M6312_REPORT_ON){
 at_cmd = "AT^CURC=1\r\n";
 }else {
 at_cmd = "AT^CURC=0\r\n";
 }
 rc = gsm_m6312_at_cmd_send(at_cmd,strlen(at_cmd),GSM_M6312_SET_REPORT_SEND_TIMEOUT);
 
 if(rc != GSM_ERR_OK){
 goto err_exit; 
 }
 
 rc = gsm_m6312_at_cmd_recv(rsp,20,GSM_M6312_SET_REPORT_RSP_TIMEOUT);
 if(rc != GSM_ERR_OK){
 goto err_exit; 
 }
 
 if(strstr(rsp,"ERROR")){
 rc = GSM_ERR_CMD_ERR;
 goto err_exit; 
 }
 
 if(strstr(rsp,"OK") == NULL){
 rc = GSM_ERR_UNKNOW;
 goto err_exit; 
 }
 rc = GSM_ERR_OK;
 
err_exit:
 gsm_m6312_print_err_info(at_cmd,rc);
 osMutexRelease(gsm_mutex);
 return rc;
}
          
#define  GSM_M6312_OPEN_CLIENT_SEND_TIMEOUT                5
#define  GSM_M6312_OPEN_CLIENT_RSP_TIMEOUT                 20000
/* 函数名：gsm_m6312_open_client
*  功能：  打开连接
*  参数：  conn_id 连接ID
*  参数：  protocol协议类型<TCP或者UDP>
*  参数：  host 主机IP后者域名
*  参数：  port 主机端口号
*  返回：  0：成功 其他：失败
*/
int gsm_m6312_open_client(int conn_id,gsm_m6312_net_protocol_t protocol,const char *host,uint16_t port)
{
 int rc;
 char at_cmd[30] = { 0 };
 char rsp[20] = { 0 };
 char *protocol_str;

 if(host == NULL ){
 return GSM_ERR_NULL_POINTER;
 }
 osMutexWait(gsm_mutex,osWaitForever);
 if(protocol == GSM_M6312_NET_PROTOCOL_TCP){
 protocol_str = "\"TCP\"";
 }else{
 protocol_str = "\"UDP\"";  
 }
 snprintf(at_cmd,30,"AT+IPSTART=%1d,%s,%d\r\n",conn_id,protocol_str,port);
 
 rc = gsm_m6312_at_cmd_send(at_cmd,strlen(at_cmd),GSM_M6312_OPEN_CLIENT_SEND_TIMEOUT);
 
 if(rc != GSM_ERR_OK){
 goto err_exit; 
 }
 
 rc = gsm_m6312_at_cmd_recv(rsp,20,GSM_M6312_OPEN_CLIENT_RSP_TIMEOUT);
 if(rc != GSM_ERR_OK){
 goto err_exit; 
 }
 
 if(strstr(rsp,"ERROR")){
 rc = GSM_ERR_CMD_ERR;
 goto err_exit; 
 }
 if(strstr(rsp,"CONNECT FAIL\r\n")){
 rc = GSM_ERR_CONNECT_FAIL;
 goto err_exit; 
 }
 
 if(strstr(rsp,"COMMAND TIMEOUT\r\n")){
 rc = GSM_ERR_CMD_TIMEOUT;
 goto err_exit; 
 }
 
 if(strstr(rsp,"CONNECT OK\r\n") == NULL){
 rc = GSM_ERR_UNKNOW;
 goto err_exit; 
 }
 rc = GSM_ERR_OK;
 
err_exit:
 gsm_m6312_print_err_info(at_cmd,rc);
 osMutexRelease(gsm_mutex);
 return rc;
}
 
#define  GSM_M6312_CLOSE_CLIENT_SEND_TIMEOUT               5
#define  GSM_M6312_CLOSE_CLIENT_RSP_TIMEOUT                5000
/* 函数名：gsm_m6312_close_client
*  功能：  关闭连接
*  参数：  conn_id 连接ID
*  返回：  0：成功 其他：失败
*/
int gsm_m6312_close_client(int conn_id)
{
 int rc;
 char at_cmd[20] = { 0 };
 char rsp[20] = { 0 };

 osMutexWait(gsm_mutex,osWaitForever);

 snprintf(at_cmd,20,"AT+IPCLOSE=%1d\r\n",conn_id);
 rc = gsm_m6312_at_cmd_send(at_cmd,strlen(at_cmd),GSM_M6312_CLOSE_CLIENT_SEND_TIMEOUT);
 
 if(rc != GSM_ERR_OK){
 goto err_exit; 
 }
 
 rc = gsm_m6312_at_cmd_recv(rsp,20,GSM_M6312_GET_ACTIVE_RSP_TIMEOUT);
 if(rc != GSM_ERR_OK){
 goto err_exit; 
 }
 
 if(strstr(rsp,"ERROR")){
 rc = GSM_ERR_UNKNOW;
 goto err_exit; 
 }
 rc = GSM_ERR_OK;
 
err_exit:
 gsm_m6312_print_err_info(at_cmd,rc);
 osMutexRelease(gsm_mutex);
 return rc;
}
#define  GSM_M6312_GET_SOCKET_STATUS_SEND_TIMEOUT             5   
#define  GSM_M6312_GET_SOCKET_STATUS_RSP_TIMEOUT              5000
#define  GSM_M6312_CONNECT_STATUS_RSP_SIZE                    400

/* 函数名：gsm_m6312_get_connect_status
*  功能：  获取连接的状态
*  参数：  conn_id 连接ID
*  参数：  status连接状态指针
*  返回：  0：成功 其他：失败
*/
int gsm_m6312_get_connect_status(const int conn_id,gsm_m6312_socket_status_t *status)
{
 int rc;
 char *at_cmd;
 char conn_id_str[12];
 char *rsp;
 char *status_str;
 
 rsp = GSM_M6312_MALLOC(GSM_M6312_CONNECT_STATUS_RSP_SIZE);
 if(rsp == NULL){
 return GSM_ERR_MALLOC_FAIL; 
 }
 snprintf(conn_id_str,12,"+CMSTATE: %1d",conn_id);
 
 osMutexWait(gsm_mutex,osWaitForever);
 
 at_cmd = "AT+IPSTATE\r\n";
 rc = gsm_m6312_at_cmd_send(at_cmd,strlen(at_cmd),GSM_M6312_GET_SOCKET_STATUS_SEND_TIMEOUT);
 
 if(rc != GSM_ERR_OK){
 goto err_exit; 
 }
 
 rc = gsm_m6312_at_cmd_recv(rsp,GSM_M6312_CONNECT_STATUS_RSP_SIZE,GSM_M6312_GET_SOCKET_STATUS_RSP_TIMEOUT);
 if(rc != GSM_ERR_OK){
 goto err_exit; 
 }
 
 if(strstr(rsp,"ERROR")){
 rc = GSM_ERR_CMD_ERR;
 goto err_exit; 
 }
 
 if(strstr(rsp,"OK") == NULL){
 rc = GSM_ERR_UNKNOW;;
 goto err_exit; 
 }
 
 status_str = strstr(rsp,conn_id_str);
 if(status_str){
 if(strstr(status_str,"IP INITIAL")){
 *status = GSM_M6312_SOCKET_INIT;
 }else if(strstr(status_str,"IP CONNECT")){
 *status = GSM_M6312_SOCKET_CONNECT;  
 }else if(strstr(status_str,"IP CLOSE")){
 *status = GSM_M6312_SOCKET_CLOSE; 
 }else if(strstr(status_str,"BIND OK")){
 *status = GSM_M6312_SOCKET_CONNECT; 
 }else if(strstr(status_str,"IP CONNECTTING")){
 *status = GSM_M6312_SOCKET_CONNECTTING; 
 }else{
 rc = GSM_ERR_UNKNOW;  
 goto err_exit; 
 }
 }else{
 *status = GSM_M6312_SOCKET_INIT; 
 }
 rc = GSM_ERR_OK;
 
err_exit:
 gsm_m6312_print_err_info(at_cmd,rc);
 osMutexRelease(gsm_mutex);
 GSM_M6312_FREE(rsp);
 return rc;
}
#define  GSM_M6312_SEND_SEND_TIMEOUT               5      
#define  GSM_M6312_SEND_RSP_TIMEOUT                5000 
#define  GSM_M6312_SEND_DATA_SEND_TIMEOUT          1000
#define  GSM_M6312_SEND_DATA_RSP_TIMEOUT           20000

/* 函数名：gsm_m6312_send
*  功能：  发送数据
*  参数：  conn_id 连接ID
*  参数：  data 数据buffer
*  参数：  size 发送的数据数量
*  返回：  0：成功 其他：失败
*/
int gsm_m6312_send(int conn_id,const char *data,const int size)
{
 int rc;
 char at_cmd[30] = { 0 };
 char rsp[20] = { 0 };

 osMutexWait(gsm_mutex,osWaitForever);
 /*第1步 启动发送*/
 snprintf(at_cmd,30,"AT+IPSEND=%1d,%d\r\n",conn_id,size);
 rc = gsm_m6312_at_cmd_send(at_cmd,strlen(at_cmd),GSM_M6312_SEND_SEND_TIMEOUT);
 
 if(rc != GSM_ERR_OK){
 goto err_exit; 
 }
 
 rc = gsm_m6312_at_cmd_recv(rsp,20,GSM_M6312_SEND_RSP_TIMEOUT);
 if(rc != GSM_ERR_OK){
 goto err_exit; 
 }
 
 if(strstr(rsp,"ERROR")){
 rc = GSM_ERR_CMD_ERR;
 goto err_exit; 
 }
 if(strstr(rsp,">") == NULL){
 rc = GSM_ERR_UNKNOW;
 goto err_exit; 
 }
 /*第2步执行发送数据*/
 rc = gsm_m6312_at_cmd_send(data,size,GSM_M6312_SEND_DATA_SEND_TIMEOUT);
 
 if(rc != GSM_ERR_OK){
 goto err_exit; 
 }
 
 rc = gsm_m6312_at_cmd_recv(rsp,GSM_M6312_CONNECT_STATUS_RSP_SIZE,GSM_M6312_SEND_DATA_RSP_TIMEOUT);
 if(rc != GSM_ERR_OK){
 goto err_exit; 
 }
 
 if(strstr(rsp,"SEND FAIL")){
 rc = GSM_ERR_SOCKET_SEND_FAIL;
 goto err_exit; 
 }
 if(strstr(rsp,"COMMAND TIMEOUT")){
 rc = GSM_ERR_CMD_TIMEOUT;
 goto err_exit; 
 }
 
 if(strstr(rsp,"SEND OK") == NULL){
 rc = GSM_ERR_UNKNOW;
 goto err_exit; 
 }
 rc = GSM_ERR_OK;
 
err_exit:
 gsm_m6312_print_err_info(at_cmd,rc);
 osMutexRelease(gsm_mutex);
 return rc;
}
          
#define  GSM_M6312_CONFIG_RECV_BUFFER_SIZE_SEND_TIMEOUT           5
#define  GSM_M6312_CONFIG_RECV_BUFFER_SIZE_RSP_TIMEOUT            1000


/* 函数名：gsm_m6312_config_recv_buffer
*  功能：  配置数据接收缓存
*  参数：  buffer 缓存类型
*  返回：  0：成功 其他：失败
*/
int gsm_m6312_config_recv_buffer(gsm_m6312_recv_buffer_t buffer)
{
 int rc;
 char *at_cmd;
 char rsp[20] = { 0 };

 osMutexWait(gsm_mutex,osWaitForever);
 if(buffer == GSM_M6312_RECV_BUFFERE){
 at_cmd = "AT+CMNDI=1\r\n";
 }else{
 at_cmd = "AT+CMNDI=1\r\n";
 }
 rc = gsm_m6312_at_cmd_send(at_cmd,strlen(at_cmd),GSM_M6312_CONFIG_RECV_BUFFER_SIZE_SEND_TIMEOUT);
 
 if(rc != GSM_ERR_OK){
 goto err_exit; 
 }
 
 rc = gsm_m6312_at_cmd_recv(rsp,20,GSM_M6312_CONFIG_RECV_BUFFER_SIZE_RSP_TIMEOUT);
 if(rc != GSM_ERR_OK){
 goto err_exit; 
 }
 
 if(strstr(rsp,"ERROR")){
 rc = GSM_ERR_UNKNOW;
 goto err_exit; 
 }
 rc = GSM_ERR_OK;
 
err_exit:
 gsm_m6312_print_err_info(at_cmd,rc);
 osMutexRelease(gsm_mutex);
 return rc;  
}
#define  GSM_M6312_GET_RECV_BUFFER_SIZE_SEND_TIMEOUT                  5
#define  GSM_M6312_GET_RECV_BUFFER_SIZE_RSP_TIMEOUT                   1000
#define  GSM_M6312_GET_RECV_BUFFER_SEND_TIMEOUT                       5
#define  GSM_M6312_GET_RECV_BUFFER_RSP_TIMEOUT                        1000
#define  GSM_M6312_GET_RECV_BUFFER_RSP_SIZE                           2000
/* 函数名：gsm_m6312_recv
*  功能：  接收数据
*  参数：  conn_id 连接ID
*  参数：  data 数据buffer
*  参数：  size buffer的size
*  返回：  >=0：接收到的数据量 其他：失败
*/
int gsm_m6312_recv(int conn_id,char *buffer,const int size)
{
 int rc;
 char at_cmd[30] = { 0 };
 char *rsp;
 char conn_id_str[10] = {0};
 char *recv_buffer_size_str;
 uint16_t recv_buffer_size;
 char *tolon_str;
 
 if(size == 0 ){
 return 0;  
 }
 
 rsp = GSM_M6312_MALLOC(GSM_M6312_GET_RECV_BUFFER_RSP_SIZE);
 if(rsp == NULL){
 rc = GSM_ERR_MALLOC_FAIL;
 return rc;
 }
 memset(rsp,0,GSM_M6312_GET_RECV_BUFFER_RSP_SIZE);
 
 osMutexWait(gsm_mutex,osWaitForever);
 /*查询接收到的数量*/
 strcpy(at_cmd ,"AT+CMRD?\r\n");
 rc = gsm_m6312_at_cmd_send(at_cmd,strlen(at_cmd),GSM_M6312_GET_RECV_BUFFER_SIZE_SEND_TIMEOUT);
 
 if(rc != GSM_ERR_OK){
 goto err_exit; 
 }
 
 rc = gsm_m6312_at_cmd_recv(rsp,GSM_M6312_GET_RECV_BUFFER_RSP_SIZE,GSM_M6312_GET_RECV_BUFFER_SIZE_RSP_TIMEOUT);
 if(rc != GSM_ERR_OK){
 goto err_exit; 
 }
 snprintf(conn_id_str,20,"+CMRD: %1d",conn_id); 
 /*找到对应的conn id*/
 recv_buffer_size_str = strstr(rsp,conn_id_str);
 if(recv_buffer_size_str == NULL){
 rc = GSM_ERR_UNKNOW;
 goto err_exit;
 }
 
 for(uint8_t i = 0;i < 2; i ++){
 tolon_str = strstr(recv_buffer_size_str,",");
 if(tolon_str == NULL){
 rc = GSM_ERR_UNKNOW;
 goto err_exit; 
 }
 recv_buffer_size_str = tolon_str + 1; 
 }
 
 recv_buffer_size = atoi( recv_buffer_size_str);                            
 if(recv_buffer_size == 0){
 rc = 0;
 goto err_exit; 
 }
 memset(at_cmd,0,30);
 memset(rsp,0,GSM_M6312_GET_RECV_BUFFER_RSP_SIZE);
 
 recv_buffer_size = recv_buffer_size > size ? size : recv_buffer_size;
 /*读取对应的数据量*/
 snprintf(at_cmd,30,"AT+CMRD=%1d,%d\r\n",conn_id,recv_buffer_size);
 rc = gsm_m6312_at_cmd_send(at_cmd,strlen(at_cmd),GSM_M6312_GET_RECV_BUFFER_SEND_TIMEOUT);
 
 if(rc != GSM_ERR_OK){
 goto err_exit; 
 }
 
 rc = gsm_m6312_at_cmd_recv(rsp,size,GSM_M6312_GET_RECV_BUFFER_RSP_TIMEOUT);
 if(rc != GSM_ERR_OK){
 goto err_exit; 
 }
 
 if(strstr(rsp,"ERROR")){
 rc = GSM_ERR_CMD_ERR;
 goto err_exit; 
 }
 
 if(strstr(rsp,"OK") == NULL){
 rc = GSM_ERR_UNKNOW;
 goto err_exit; 
 }
 memcpy(buffer,rsp,recv_buffer_size);
 rc = recv_buffer_size;
 
err_exit:
 if(rc < 0){
 gsm_m6312_print_err_info(at_cmd,rc);
 }
 osMutexRelease(gsm_mutex);
 GSM_M6312_FREE(rsp);
 return rc;  
}