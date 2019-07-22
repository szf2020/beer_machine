/*****************************************************************************
*  GSM模块HAL                                                          
*  Copyright (C) 2019 wkxboot 1131204425@qq.com.                             
*                                                                            
*                                                                            
*  This program is free software; you can redistribute it and/or modify      
*  it under the terms of the GNU General Public License version 3 as         
*  published by the Free Software Foundation.                                
*                                                                            
*  @file     gsm_m6312.c                                                   
*  @brief    GSM模块HAL                                                                                                                                                                                             
*  @author   wkxboot                                                      
*  @email    1131204425@qq.com                                              
*  @version  v1.0.0                                                  
*  @date     2019/1/11                                            
*  @license  GNU General Public License (GPL)                                
*                                                                            
*                                                                            
*****************************************************************************/

#include "beer_machine.h"
#include "serial.h"
#include "utils.h"
#include "at.h"
#include "gsm_m6312.h"
#include "cmsis_os.h"
#include "log.h"


int gsm_m6312_serial_handle;

static serial_hal_driver_t *gsm_m6312_serial_uart_driver = &st_serial_uart_hal_driver;


static osMutexId gsm_mutex;


#define  GSM_M6312_MALLOC(x)           pvPortMalloc((x))
#define  GSM_M6312_FREE(x)             vPortFree((x))

#define  GSM_M6312_PWR_ON_DELAY        5000
#define  GSM_M6312_PWR_OFF_DELAY       15000

static at_t gsm_at;

/* 函数名：gsm_m6312_pwr_on
*  功能：  m6312 2g模块开机
*  参数：  无 
*  返回：  GSM_ERR_OK：成功 其他：失败
*/
int gsm_m6312_pwr_on(void)
{
  int rc = -1;
  utils_timer_t timer;
  
  utils_timer_init(&timer,GSM_M6312_PWR_ON_DELAY,false);
  
  if(bsp_get_gsm_pwr_status() == BSP_GSM_STATUS_PWR_ON) {
	 return 0;
  }
  
  bsp_gsm_pwr_key_press();
  while(utils_timer_value(&timer)) {
	osDelay(100);
	if(bsp_get_gsm_pwr_status() == BSP_GSM_STATUS_PWR_ON) {
	   rc = 0;
	   break;
     }
   }
  bsp_gsm_pwr_key_release();  
  
  return rc;
}

/* 函数名：gsm_m6312_pwr_off
*  功能：  m6312 2g模块关机
*  参数：  无 
*  返回：  GSM_ERR_OK：成功 其他：失败
*/

int gsm_m6312_pwr_off(void)
{
  int rc = -1;
  utils_timer_t timer;
  
  utils_timer_init(&timer,GSM_M6312_PWR_ON_DELAY,false);

  if(bsp_get_gsm_pwr_status() == BSP_GSM_STATUS_PWR_OFF) {
	 return 0;
  }
  bsp_gsm_pwr_key_press();
	
  while(utils_timer_value(&timer)){
	osDelay(100);
	if(bsp_get_gsm_pwr_status() == BSP_GSM_STATUS_PWR_OFF){
	   rc = 0;
	   break;
	}
  }
  bsp_gsm_pwr_key_release();  
  return rc; 
}


/* 函数名：gsm_m6312_serial_hal_init
*  功能：  m6312 2g模块执行硬件和串口初始化
*  参数：  无 
*  返回：  GSM_ERR_OK：成功 其他：失败
*/
int gsm_m6312_serial_hal_init(void)
{
	int rc ;
    rc = serial_create(&gsm_m6312_serial_handle,GSM_M6312_BUFFER_SIZE,GSM_M6312_BUFFER_SIZE * 4);
    if (rc != 0) {
       log_error("m6312 create serial hal err.\r\n");
       return -1;
    }
    log_debug("m6312 create serial hal ok.\r\n");
   
    rc = serial_register_hal_driver(gsm_m6312_serial_handle,gsm_m6312_serial_uart_driver);
	if (rc != 0) {
       log_error("m6312 register serial hal driver err.\r\n");
       return -1;
    }
	log_debug("m6312 register serial hal driver ok.\r\n");
    
	rc = serial_open(gsm_m6312_serial_handle,GSM_M6312_SERIAL_PORT,GSM_M6312_SERIAL_BAUDRATES,GSM_M6312_SERIAL_DATA_BITS,GSM_M6312_SERIAL_STOP_BITS);
	if (rc != 0) {
       log_error("m6312 open serial hal err.\r\n");
       return -1;
  	}
	log_debug("m6312 open serial hal ok.\r\n");
	
	osMutexDef(gsm_mutex);
	gsm_mutex = osMutexCreate(osMutex(gsm_mutex));
	if (gsm_mutex == NULL) {
	   log_error("create gsm mutex err.\r\n");
	   return -1;
	}
	osMutexRelease(gsm_mutex);
	log_debug("create gsm mutex ok.\r\n");

	return 0; 
}

/* 函数名：gsm_m6312_get_sim_card_status
*  功能：  获取设备sim卡状态
*  参数：  sim_status sim状态指针
*  返回：  GSM_ERR_OK：成功 其他：失败
*/
int gsm_m6312_get_sim_card_status(sim_card_status_t *sim_status)
{
 int rc;
 at_err_code_t ok,err;
 
 osMutexWait(gsm_mutex,osWaitForever);
 
 memset(&gsm_at,0,sizeof(gsm_at));
 strcpy(gsm_at.send,"AT+CPIN?\r\n");
 gsm_at.send_size = strlen(gsm_at.send);
 gsm_at.send_timeout = 5;
 gsm_at.recv_timeout = 5000;
 ok.str = "OK\r\n";
 ok.code = GSM_ERR_OK;
 ok.next = NULL;
 err.str = "ERROR";
 err.code = GSM_ERR_CMD_ERR;
 err.next = NULL;
 
 at_err_code_add(&gsm_at.err_head,&ok);
 at_err_code_add(&gsm_at.err_head,&err);
 
 rc = at_excute(gsm_m6312_serial_handle,&gsm_at);

 if(rc == GSM_ERR_OK){
   
 if(strstr(gsm_at.recv,"READY")){
 *sim_status = SIM_CARD_STATUS_READY;
 }else if(strstr(gsm_at.recv,"NO SIM")){
 *sim_status = SIM_CARD_STATUS_NO_SIM_CARD;
 }else if(strstr(gsm_at.recv,"BLOCK")){
 *sim_status = SIM_CARD_STATUS_BLOCK;
 }else{
 rc = GSM_ERR_UNKNOW;
 } 
 }  
 
 osMutexRelease(gsm_mutex);
 return rc; 
}
 

/* 函数名：gsm_m6312_get_sim_card_id
*  功能：  获取 sim card id
*  参数：  sim_id 指针 
*  返回：  GSM_ERR_OK：成功 其他：失败
*/
int gsm_m6312_get_sim_card_id(char *sim_id)
{
 int rc;
 char *ccid_str;
 at_err_code_t ok,err;
 
 osMutexWait(gsm_mutex,osWaitForever);
 
 memset(&gsm_at,0,sizeof(gsm_at));
 strcpy(gsm_at.send,"AT+CCID?\r\n");
 gsm_at.send_size = strlen(gsm_at.send);
 gsm_at.send_timeout = 5;
 gsm_at.recv_timeout = 5000;
 ok.str = "OK\r\n";
 ok.code = GSM_ERR_OK;
 ok.next = NULL;
 err.str = "ERROR";
 err.code = GSM_ERR_CMD_ERR;
 err.next = NULL;
 
 at_err_code_add(&gsm_at.err_head,&ok);
 at_err_code_add(&gsm_at.err_head,&err);
 
 rc = at_excute(gsm_m6312_serial_handle,&gsm_at);

 if(rc == GSM_ERR_OK){ 
 ccid_str = strstr(gsm_at.recv,"+CCID: ");
 if(ccid_str){
 memcpy(sim_id,ccid_str + strlen("+CCID: "),GSM_M6312_SIM_ID_STR_LEN); 
 sim_id[GSM_M6312_SIM_ID_STR_LEN] = '\0';
 }else{
 rc = GSM_ERR_UNKNOW;   
 }
 }  
 
 osMutexRelease(gsm_mutex);
 return rc;
}

/* 函数名：gsm_m6312_get_rssi
*  功能：  获取当前基站信号强度
*  参数：  rssi指针 
*  返回：  GSM_ERR_OK：成功 其他：失败
*/
int gsm_m6312_get_rssi(char *rssi)
{
 int rc;
 char *rssi_str,*break_str;
 at_err_code_t ok,err;
 
 osMutexWait(gsm_mutex,osWaitForever);
 
 memset(&gsm_at,0,sizeof(gsm_at));
 strcpy(gsm_at.send,"AT+CSQ\r\n");
 gsm_at.send_size = strlen(gsm_at.send);
 gsm_at.send_timeout = 5;
 gsm_at.recv_timeout = 5000;
 ok.str = "OK\r\n";
 ok.code = GSM_ERR_OK;
 ok.next = NULL;
 err.str = "ERROR";
 err.code = GSM_ERR_CMD_ERR;
 err.next = NULL;
 
 at_err_code_add(&gsm_at.err_head,&ok);
 at_err_code_add(&gsm_at.err_head,&err);
 
 rc = at_excute(gsm_m6312_serial_handle,&gsm_at);

 if(rc == GSM_ERR_OK){ 
 rc = GSM_ERR_UNKNOW;   
 rssi_str = strstr(gsm_at.recv,"+CSQ: ");
 if(rssi_str){
 rssi_str +=strlen("+CSQ: ");
 break_str = strstr(rssi_str ,",");
 if(break_str){
 memcpy(rssi,rssi_str,break_str - rssi_str);
 rssi[break_str - rssi_str] = '\0';
 rc = GSM_ERR_OK;
 }
 }
 }
 
 
 osMutexRelease(gsm_mutex);
 return rc;
}

/* 函数名：gsm_m6312_get_assist_base_info
*  功能：  获取辅助基站信息
*  参数：  assist_base辅助基站指针 
*  返回：  GSM_ERR_OK：成功 其他：失败
*/
int gsm_m6312_get_assist_base_info(gsm_m6312_assist_base_t *assist_base)
{
 int rc;
 int rc_search;
 char *base_str;
 uint8_t i;
 at_err_code_t ok,err;
 
 osMutexWait(gsm_mutex,osWaitForever);
 
 memset(&gsm_at,0,sizeof(gsm_at));
 strcpy(gsm_at.send,"AT+CCED=0,2\r\n");
 gsm_at.send_size = strlen(gsm_at.send);
 gsm_at.send_timeout = 5;
 gsm_at.recv_timeout = 5000;
 ok.str = "OK\r\n";
 ok.code = GSM_ERR_OK;
 ok.next = NULL;
 err.str = "ERROR";
 err.code = GSM_ERR_CMD_ERR;
 err.next = NULL;
 
 at_err_code_add(&gsm_at.err_head,&ok);
 at_err_code_add(&gsm_at.err_head,&err);
 
 rc = at_excute(gsm_m6312_serial_handle,&gsm_at);

 if(rc == GSM_ERR_OK){ 
 rc_search = utils_get_str_addr_by_num(gsm_at.recv,"+CCED:",1,&base_str);
 if(rc_search != 0){
   rc = GSM_ERR_UNKNOW;  
   goto err_exit;
 } 
 
 i = 0;
 do{
 rc_search = utils_get_str_value_by_num(gsm_at.recv,assist_base->base[i].lac,",",2 + i * 6);
 if(rc_search != 0){
   goto err_exit;
 } 
 rc_search = utils_get_str_value_by_num(gsm_at.recv,assist_base->base[i].ci,",",3 + i * 6);
 if(rc_search != 0){
   rc = GSM_ERR_UNKNOW;  
   goto err_exit;
 }
 
 rc_search = utils_get_str_value_by_num(gsm_at.recv,assist_base->base[i].rssi,",",5 + i * 6);
 if(rc_search != 0){
   rc = GSM_ERR_UNKNOW;  
   goto err_exit;
 } 
 }while( ++ i < 3);
 
 }
 

err_exit: 
 
 osMutexRelease(gsm_mutex);
 return rc;
}

/* 函数名：gsm_m6312_set_echo
*  功能：  设置是否回显输入的命令
*  参数：  echo 回显设置 
*  返回：  GSM_ERR_OK：成功 其他：失败
*/
int gsm_m6312_set_echo(gsm_m6312_echo_t echo)
{
 int rc;
 at_err_code_t ok,err;
 
 osMutexWait(gsm_mutex,osWaitForever);
 
 memset(&gsm_at,0,sizeof(gsm_at));
 if(echo == GSM_M6312_ECHO_ON){
 strcpy(gsm_at.send,"ATE1\r\n");
 }else{
 strcpy(gsm_at.send,"ATE0\r\n");
 }
 gsm_at.send_size = strlen(gsm_at.send);
 gsm_at.send_timeout = 5;
 gsm_at.recv_timeout = 5000;
 ok.str = "OK\r\n";
 ok.code = GSM_ERR_OK;
 ok.next = NULL;
 err.str = "ERROR";
 err.code = GSM_ERR_CMD_ERR;
 err.next = NULL;
 
 at_err_code_add(&gsm_at.err_head,&ok);
 at_err_code_add(&gsm_at.err_head,&err);
 
 rc = at_excute(gsm_m6312_serial_handle,&gsm_at);

 
 osMutexRelease(gsm_mutex);
 return rc;
}

 
/* 函数名：gsm_m6312_get_imei
*  功能：  获取设备IMEI串号
*  参数：  imei imei指针
*  返回：  0：ready 其他：失败
*/
int gsm_m6312_get_imei(char *imei)
{
 int rc;
 char *temp;
 at_err_code_t ok,err;
 
 osMutexWait(gsm_mutex,osWaitForever);
 
 memset(&gsm_at,0,sizeof(gsm_at));
 strcpy(gsm_at.send,"AT+CGSN\r\n");
 gsm_at.send_size = strlen(gsm_at.send);
 gsm_at.send_timeout = 5;
 gsm_at.recv_timeout = 5000;
 ok.str = "OK\r\n";
 ok.code = GSM_ERR_OK;
 ok.next = NULL;
 err.str = "ERROR";
 err.code = GSM_ERR_CMD_ERR;
 err.next = NULL;
 
 at_err_code_add(&gsm_at.err_head,&ok);
 at_err_code_add(&gsm_at.err_head,&err);
 
 rc = at_excute(gsm_m6312_serial_handle,&gsm_at);
 if(rc == GSM_ERR_OK){
 /*找到开始标志*/
 temp = strstr(gsm_at.recv,"+CGSN: ");
 if(temp){
 /*imei 15位*/
 memcpy(imei,temp + strlen("+CGSN: "),15);
 imei[15] = '\0';
 }else{
 rc = GSM_ERR_UNKNOW;
 }
 }
 
 
 osMutexRelease(gsm_mutex);
 return rc;
}

/* 函数名：gsm_m6312_get_imei
*  功能：  获取设备IMEI串号
*  参数：  sn sn指针
*  返回：  GSM_ERR_OK：成功 其他：失败
*/
int gsm_m6312_get_sn(char *sn)
{
 int rc;
 char *temp;
 at_err_code_t ok,err;
 
 osMutexWait(gsm_mutex,osWaitForever);
 
 memset(&gsm_at,0,sizeof(gsm_at));
 strcpy(gsm_at.send,"AT^SN\r\n");
 gsm_at.send_size = strlen(gsm_at.send);
 gsm_at.send_timeout = 5;
 gsm_at.recv_timeout = 5000;
 ok.str = "OK\r\n";
 ok.code = GSM_ERR_OK;
 ok.next = NULL;
 err.str = "ERROR";
 err.code = GSM_ERR_CMD_ERR;
 err.next = NULL;
 
 at_err_code_add(&gsm_at.err_head,&ok);
 at_err_code_add(&gsm_at.err_head,&err);
 
 rc = at_excute(gsm_m6312_serial_handle,&gsm_at);
 if(rc == GSM_ERR_OK){
 /*找到开始标志*/
 temp = strstr(gsm_at.recv,"M6312");
 if(temp){
 /*sn 20位*/
 memcpy(sn,temp,GSM_M6312_SN_LEN);
 sn[GSM_M6312_SN_LEN] = '\0';
 }else{
 rc = GSM_ERR_UNKNOW;
 }
 }
 
 
 osMutexRelease(gsm_mutex);
 return rc;
}

/* 函数名：gsm_m6312_gprs_set_apn
*  功能：  设置指定cid的gprs的APN
*  参数：  apn 网络APN 
*  返回：  GSM_ERR_OK：成功 其他：失败
*/
int gsm_m6312_set_apn(gsm_m6312_apn_t apn)
{
 int rc;
 at_err_code_t ok,err;
 
 osMutexWait(gsm_mutex,osWaitForever);
 
 memset(&gsm_at,0,sizeof(gsm_at));
 snprintf(gsm_at.send,AT_SEND_BUFFER_SIZE,"AT+CGDCONT=1,\"IP\",%s\r\n",apn == GSM_M6312_APN_CMNET ? "\"CMNET\"" : "\"UNINET\"");
 gsm_at.send_size = strlen(gsm_at.send);
 gsm_at.send_timeout = 5;
 gsm_at.recv_timeout = 5000;
 ok.str = "OK\r\n";
 ok.code = GSM_ERR_OK;
 ok.next = NULL;
 err.str = "ERROR";
 err.code = GSM_ERR_CMD_ERR;
 err.next = NULL;
 
 at_err_code_add(&gsm_at.err_head,&ok);
 at_err_code_add(&gsm_at.err_head,&err);
 
 rc = at_excute(gsm_m6312_serial_handle,&gsm_at);
 
 
 osMutexRelease(gsm_mutex);
 return rc;
}


/* 函数名：gsm_m6312_gprs_get_apn
*  功能：  获取apn
*  参数：  apn 指针 
*  返回：  GSM_ERR_OK：成功 其他：失败
*/
int gsm_m6312_get_apn(gsm_m6312_apn_t *apn)
{
 int rc;
 char *apn_str;
 at_err_code_t ok,err;
 
 osMutexWait(gsm_mutex,osWaitForever);
 
 memset(&gsm_at,0,sizeof(gsm_at));
 strcpy(gsm_at.send,"AT+CGDCONT?\r\n");
 gsm_at.send_size = strlen(gsm_at.send);
 gsm_at.send_timeout = 5;
 gsm_at.recv_timeout = 2000;
 ok.str = "OK\r\n";
 ok.code = GSM_ERR_OK;
 ok.next = NULL;
 err.str = "ERROR";
 err.code = GSM_ERR_CMD_ERR;
 err.next = NULL;
 
 at_err_code_add(&gsm_at.err_head,&ok);
 at_err_code_add(&gsm_at.err_head,&err);
 
 rc = at_excute(gsm_m6312_serial_handle,&gsm_at);
 if(rc == GSM_ERR_OK){
 apn_str = strstr(gsm_at.recv,"CMNET");
 if(apn_str == NULL){
 apn_str = strstr(gsm_at.recv,"UNINET");
 if(apn_str == NULL){   
 rc = GSM_ERR_UNKNOW;
 }else{
 *apn = GSM_M6312_APN_UNINET;
 }
 }else{
 *apn = GSM_M6312_APN_CMNET;
 }  
 }
 
 
 osMutexRelease(gsm_mutex);
 return rc;
}

 
/* 函数名：gsm_m6312_gprs_set_active_status
*  功能：  激活或者去激活GPRS功能
*  参数：  active 状态 
*  返回：  GSM_ERR_OK：成功 其他：失败
*/
int gsm_m6312_set_active_status(gsm_m6312_active_status_t active)
{
 int rc;
 at_err_code_t ok,err;
 
 osMutexWait(gsm_mutex,osWaitForever);
 
 memset(&gsm_at,0,sizeof(gsm_at));
 if(active == GSM_M6312_ACTIVE){
 strcpy(gsm_at.send,"AT+CGACT=1,1\r\n");
 }else{
 strcpy(gsm_at.send,"AT+CGACT=0,1\r\n");  
 }
 gsm_at.send_size = strlen(gsm_at.send);
 gsm_at.send_timeout = 5;
 gsm_at.recv_timeout = 20000;
 ok.str = "OK\r\n";
 ok.code = GSM_ERR_OK;
 ok.next = NULL;
 err.str = "ERROR";
 err.code = GSM_ERR_CMD_ERR;
 err.next = NULL;
 
 at_err_code_add(&gsm_at.err_head,&ok);
 at_err_code_add(&gsm_at.err_head,&err);
 
 rc = at_excute(gsm_m6312_serial_handle,&gsm_at);
 
 
 osMutexRelease(gsm_mutex);
 return rc;
}

/* 函数名：gsm_m6312_get_active_status
*  功能：  获取激活状态
*  参数：  active 状态指针 
*  返回：  GSM_ERR_OK：成功 其他：失败
*/
int gsm_m6312_get_active_status(gsm_m6312_active_status_t *active)
{
 int rc;
 at_err_code_t ok,err;
 
 osMutexWait(gsm_mutex,osWaitForever);
 
 memset(&gsm_at,0,sizeof(gsm_at));
 strcpy(gsm_at.send,"AT+CGACT?\r\n");
 gsm_at.send_size = strlen(gsm_at.send);
 gsm_at.send_timeout = 5;
 gsm_at.recv_timeout = 5000;
 ok.str = "OK\r\n";
 ok.code = GSM_ERR_OK;
 ok.next = NULL;
 err.str = "ERROR";
 err.code = GSM_ERR_CMD_ERR;
 err.next = NULL;
 
 at_err_code_add(&gsm_at.err_head,&ok);
 at_err_code_add(&gsm_at.err_head,&err);
 
 rc = at_excute(gsm_m6312_serial_handle,&gsm_at);
 if(rc == GSM_ERR_OK){
 if(strstr(gsm_at.recv,"+CGACT: 0,1") || strstr(gsm_at.recv,"+CGACT: 0,0")){
 *active = GSM_M6312_INACTIVE;
 }else if(strstr(gsm_at.recv,"+CGACT: 1,1")){
 *active = GSM_M6312_ACTIVE;
 }else{
 rc = GSM_ERR_UNKNOW;
 }
 }
 
 
 osMutexRelease(gsm_mutex);
 return rc;
}

/* 函数名：gsm_m6312_set_attach_status
*  功能：  设置GPRS网络附着状态
*  参数：  attach GPRS附着状态 
*  返回：  GSM_ERR_OK：成功 其他：失败
*/
int gsm_m6312_set_attach_status(gsm_m6312_attach_status_t attach)
{
 int rc;
 at_err_code_t ok,err;
 
 osMutexWait(gsm_mutex,osWaitForever);
 
 memset(&gsm_at,0,sizeof(gsm_at));
 if(attach == GSM_M6312_ATTACH){
 strcpy(gsm_at.send,"AT+CGATT=1\r\n");
 }else{
 strcpy(gsm_at.send,"AT+CGATT=0\r\n");
 }
 gsm_at.send_size = strlen(gsm_at.send);
 gsm_at.send_timeout = 5;
 gsm_at.recv_timeout = 20000;
 ok.str = "OK\r\n";
 ok.code = GSM_ERR_OK;
 ok.next = NULL;
 err.str = "ERROR";
 err.code = GSM_ERR_CMD_ERR;
 err.next = NULL;
 
 at_err_code_add(&gsm_at.err_head,&ok);
 at_err_code_add(&gsm_at.err_head,&err);
 
 rc = at_excute(gsm_m6312_serial_handle,&gsm_at);
 
 
 osMutexRelease(gsm_mutex);
 return rc;
}

/* 函数名：gsm_m6312_get_attach_status
*  功能：  获取附着状态
*  参数：  attach 附着状态指针
*  返回：  GSM_ERR_OK：成功 其他：失败
*/
int gsm_m6312_get_attach_status(gsm_m6312_attach_status_t *attach)
{
 int rc;
 at_err_code_t ok,err;
 
 osMutexWait(gsm_mutex,osWaitForever);
 
 memset(&gsm_at,0,sizeof(gsm_at));
 strcpy(gsm_at.send,"AT+CGATT?\r\n");
 gsm_at.send_size = strlen(gsm_at.send);
 gsm_at.send_timeout = 5;
 gsm_at.recv_timeout = 5000;
 ok.str = "OK\r\n";
 ok.code = GSM_ERR_OK;
 ok.next = NULL;
 err.str = "ERROR";
 err.code = GSM_ERR_CMD_ERR;
 err.next = NULL;
 
 at_err_code_add(&gsm_at.err_head,&ok);
 at_err_code_add(&gsm_at.err_head,&err);
 
 rc = at_excute(gsm_m6312_serial_handle,&gsm_at);
 if(rc == GSM_ERR_OK){
 if(strstr(gsm_at.recv,"+CGATT: 1")){
 *attach = GSM_M6312_ATTACH;
 }else if(strstr(gsm_at.recv,"+CGACT: 0")){
 *attach = GSM_M6312_NOT_ATTACH;
 }else{
 rc = GSM_ERR_UNKNOW;
 }
 }
 
 
 osMutexRelease(gsm_mutex);
 return rc;
}

/* 函数名：gsm_m6312_set_connect_mode
*  功能：  设置连接模式指令
*  参数：  mode 单路或者多路 
*  返回：  GSM_ERR_OK：成功 其他：失败
*/
int gsm_m6312_set_connect_mode(gsm_m6312_connect_mode_t mode)
{
 int rc;
 at_err_code_t ok,err;
 
 osMutexWait(gsm_mutex,osWaitForever);
 
 memset(&gsm_at,0,sizeof(gsm_at));
 if(mode == GSM_M6312_CONNECT_MODE_SINGLE){ 
 strcpy(gsm_at.send,"AT+CMMUX=0\r\n");
 }else{
 strcpy(gsm_at.send,"AT+CMMUX=1\r\n");
 }
 gsm_at.send_size = strlen(gsm_at.send);
 gsm_at.send_timeout = 5;
 gsm_at.recv_timeout = 5000;
 ok.str = "OK\r\n";
 ok.code = GSM_ERR_OK;
 ok.next = NULL;
 err.str = "ERROR";
 err.code = GSM_ERR_CMD_ERR;
 err.next = NULL;
 
 at_err_code_add(&gsm_at.err_head,&ok);
 at_err_code_add(&gsm_at.err_head,&err);
 
 rc = at_excute(gsm_m6312_serial_handle,&gsm_at);
 
 
 osMutexRelease(gsm_mutex);
 return rc;
}

/* 函数名：gsm_m6312_get_operator
*  功能：  查询运营商
*  参数：  operator_name 运营商指针
*  返回：  GSM_ERR_OK：成功 其他：失败
*/
int gsm_m6312_get_operator(operator_name_t *operator_name)
{
 int rc;
 at_err_code_t ok,err;
 
 osMutexWait(gsm_mutex,osWaitForever);
 
 memset(&gsm_at,0,sizeof(gsm_at));
 strcpy(gsm_at.send,"AT+CIMI\r\n");
 gsm_at.send_size = strlen(gsm_at.send);
 gsm_at.send_timeout = 5;
 gsm_at.recv_timeout = 5000;
 ok.str = "OK\r\n";
 ok.code = GSM_ERR_OK;
 ok.next = NULL;
 err.str = "ERROR";
 err.code = GSM_ERR_CMD_ERR;
 err.next = NULL;
 
 at_err_code_add(&gsm_at.err_head,&ok);
 at_err_code_add(&gsm_at.err_head,&err);
 
 rc = at_excute(gsm_m6312_serial_handle,&gsm_at);
 if(rc == GSM_ERR_OK){
 /*IMSI 移动GSM 46000 46002 46007*/
 if(strstr(gsm_at.recv,"46000") || strstr(gsm_at.recv,"46002") || strstr(gsm_at.recv,"46004") || strstr(gsm_at.recv,"46007")){
 *operator_name = OPERATOR_CHINA_MOBILE;
 }else if(strstr(gsm_at.recv,"46001") || strstr(gsm_at.recv,"46006")){
  /*IMSI 联通GSM 46001*/
 *operator_name = OPERATOR_CHINA_UNICOM;
 }else{
 rc = GSM_ERR_UNKNOW;
 }
 }
 
 
 osMutexRelease(gsm_mutex);
 return rc;
}

/* 函数名：gsm_m6312_set_auto_operator_format
*  功能：  设置自动运营商选择和运营商格式
*  参数：  operator_format 运营商格式
*  返回：  GSM_ERR_OK：成功 其他：失败
*/
int gsm_m6312_set_auto_operator_format(gsm_m6312_operator_format_t operator_format)
{
 int rc;
 at_err_code_t ok,err;
 
 osMutexWait(gsm_mutex,osWaitForever);
 
 memset(&gsm_at,0,sizeof(gsm_at));
 if(operator_format == GSM_M6312_OPERATOR_FORMAT_NUMERIC_NAME){ 
 strcpy(gsm_at.send,"AT+COPS=0,2\r\n");
 }else if(operator_format == GSM_M6312_OPERATOR_FORMAT_SHORT_NAME){
 strcpy(gsm_at.send,"AT+COPS=0,1\r\n");
 }else{
 strcpy(gsm_at.send,"AT+COPS=0,0\r\n");
 }
 gsm_at.send_size = strlen(gsm_at.send);
 gsm_at.send_timeout = 5;
 gsm_at.recv_timeout = 20000;
 ok.str = "OK\r\n";
 ok.code = GSM_ERR_OK;
 ok.next = NULL;
 err.str = "ERROR";
 err.code = GSM_ERR_CMD_ERR;
 err.next = NULL;
 
 at_err_code_add(&gsm_at.err_head,&ok);
 at_err_code_add(&gsm_at.err_head,&err);
 
 rc = at_excute(gsm_m6312_serial_handle,&gsm_at);
 
 
 osMutexRelease(gsm_mutex);
 return rc;
}

/* 函数名：gsm_m6312_set_set_prompt
*  功能：  设置发送提示
*  参数：  prompt 设置的发送提示
*  返回：  GSM_ERR_OK：成功 其他：失败
*/
int gsm_m6312_set_send_prompt(gsm_m6312_send_prompt_t prompt)
{
 int rc;
 at_err_code_t ok,err;
 
 osMutexWait(gsm_mutex,osWaitForever);
 
 memset(&gsm_at,0,sizeof(gsm_at));
 if(prompt == GSM_M6312_SEND_PROMPT){ 
 strcpy(gsm_at.send,"AT+PROMPT=1\r\n");
 }else {
 strcpy(gsm_at.send,"AT+PROMPT=0\r\n");
 }
 gsm_at.send_size = strlen(gsm_at.send);
 gsm_at.send_timeout = 5;
 gsm_at.recv_timeout = 5000;
 ok.str = "OK\r\n";
 ok.code = GSM_ERR_OK;
 ok.next = NULL;
 err.str = "ERROR";
 err.code = GSM_ERR_CMD_ERR;
 err.next = NULL;
 
 at_err_code_add(&gsm_at.err_head,&ok);
 at_err_code_add(&gsm_at.err_head,&err);
 
 rc = at_excute(gsm_m6312_serial_handle,&gsm_at);
 
 
 osMutexRelease(gsm_mutex);
 return rc;
}

/* 函数名：gsm_m6312_set_transparent
*  功能：  设置透传模式
*  参数：  transparent 连接ID
*  返回：  GSM_ERR_OK：成功 其他：失败
*/
int gsm_m6312_set_transparent(gsm_m6312_transparent_t transparent)
{
 int rc;
 at_err_code_t ok,err;
 
 osMutexWait(gsm_mutex,osWaitForever);
 
 memset(&gsm_at,0,sizeof(gsm_at));
 if(transparent == GSM_M6312_TRANPARENT){
 strcpy(gsm_at.send,"AT+CMMODE=1\r\n");
 }else {
 strcpy(gsm_at.send,"AT+CMMODE=0\r\n");
 }
 gsm_at.send_size = strlen(gsm_at.send);
 gsm_at.send_timeout = 5;
 gsm_at.recv_timeout = 5000;
 ok.str = "OK\r\n";
 ok.code = GSM_ERR_OK;
 ok.next = NULL;
 err.str = "ERROR";
 err.code = GSM_ERR_CMD_ERR;
 err.next = NULL;
 
 at_err_code_add(&gsm_at.err_head,&ok);
 at_err_code_add(&gsm_at.err_head,&err);
 
 rc = at_excute(gsm_m6312_serial_handle,&gsm_at);
 
 
 osMutexRelease(gsm_mutex);
 return rc;
}

/* 函数名：gsm_m6312_set_report
*  功能：  关闭信息上报
*  参数：  无
*  返回：  GSM_ERR_OK：成功 其他：失败
*/
int gsm_m6312_set_report(gsm_m6312_report_t report)
{
 int rc;
 at_err_code_t ok,err;
 
 osMutexWait(gsm_mutex,osWaitForever);
 
 memset(&gsm_at,0,sizeof(gsm_at));
 if(report == GSM_M6312_REPORT_ON){
 strcpy(gsm_at.send,"AT^CURC=1\r\n");
 }else {
 strcpy(gsm_at.send,"AT^CURC=0\r\n");
 }
 gsm_at.send_size = strlen(gsm_at.send);
 gsm_at.send_timeout = 5;
 gsm_at.recv_timeout = 5000;
 ok.str = "OK\r\n";
 ok.code = GSM_ERR_OK;
 ok.next = NULL;
 err.str = "ERROR";
 err.code = GSM_ERR_CMD_ERR;
 err.next = NULL;
 
 at_err_code_add(&gsm_at.err_head,&ok);
 at_err_code_add(&gsm_at.err_head,&err);
 
 rc = at_excute(gsm_m6312_serial_handle,&gsm_at);
 
 
 osMutexRelease(gsm_mutex);
 return rc;
}

/* 函数名：gsm_m6312_set_reg_echo
*  功能：  设置网络和基站位置主动回显
*  参数：  reg_echo
*  返回：  GSM_ERR_OK：成功 其他：失败
*/
int gsm_m6312_set_reg_echo(gsm_m6312_reg_echo_t reg_echo)
{
 int rc;
 at_err_code_t ok,err;
 
 osMutexWait(gsm_mutex,osWaitForever);
 
 memset(&gsm_at,0,sizeof(gsm_at));
 if(reg_echo == GSM_M6312_REG_ECHO_ON){
 strcpy(gsm_at.send,"AT+CGREG=2\r\n");
 }else {
 strcpy(gsm_at.send,"AT+CGREG=0\r\n");
 }
 gsm_at.send_size = strlen(gsm_at.send);
 gsm_at.send_timeout = 5;
 gsm_at.recv_timeout = 5000;
 ok.str = "OK\r\n";
 ok.code = GSM_ERR_OK;
 ok.next = NULL;
 err.str = "ERROR";
 err.code = GSM_ERR_CMD_ERR;
 err.next = NULL;
 
 at_err_code_add(&gsm_at.err_head,&ok);
 at_err_code_add(&gsm_at.err_head,&err);
 
 rc = at_excute(gsm_m6312_serial_handle,&gsm_at);
 
 
 osMutexRelease(gsm_mutex);
 return rc;
}

/* 函数名：gsm_m6312_get_reg_location
*  功能：  获取注册和基站位置信息
*  参数：  reg 信息指针
*  返回：  GSM_ERR_OK：成功 其他：失败
*/
int gsm_m6312_get_reg_location(gsm_m6312_register_t *reg)
{
 int rc;
 int str_rc;
 char *temp;
 at_err_code_t ok,err;
 
 osMutexWait(gsm_mutex,osWaitForever);
 
 memset(&gsm_at,0,sizeof(gsm_at));
 strcpy(gsm_at.send,"AT+CGREG?\r\n");

 gsm_at.send_size = strlen(gsm_at.send);
 gsm_at.send_timeout = 5;
 gsm_at.recv_timeout = 5000;
 ok.str = "OK\r\n";
 ok.code = GSM_ERR_OK;
 ok.next = NULL;
 err.str = "ERROR";
 err.code = GSM_ERR_CMD_ERR;
 err.next = NULL;
 
 at_err_code_add(&gsm_at.err_head,&ok);
 at_err_code_add(&gsm_at.err_head,&err);
 
 rc = at_excute(gsm_m6312_serial_handle,&gsm_at);
 if(rc == GSM_ERR_OK){
 rc = GSM_ERR_UNKNOW;
 temp = strstr(gsm_at.recv,"+CGREG: ");
 if(temp == NULL){
 goto err_exit;  
 }
 temp +=strlen("+CGREG: ");
 
 /*找到第1个，*/
 str_rc = utils_get_str_addr_by_num(temp ,",",1,&temp);
 if(str_rc != 0){
 goto err_exit; 
 }

 if(strncmp(temp + 1,"1",1) == 0){
 reg->status = GSM_M6312_STATUS_REGISTER;  
 }else {
 reg->status = GSM_M6312_STATUS_NO_REGISTER; 
 /*没有注册就没有位置信息*/ 
 rc = GSM_ERR_OK;
 goto err_exit;
 }
 
 /*获取第1个和第2个 " 之间的字符串*/
 str_rc = utils_get_str_value_by_num(temp,reg->base.lac,"\"",1);
 if(str_rc != 0){  
 goto err_exit;    
 }  
 /*获取第3个和第4个 " 之间的字符串*/
 str_rc = utils_get_str_value_by_num(temp,reg->base.ci,"\"",3);
 if(str_rc != 0){  
 goto err_exit;    
 }  
 rc = GSM_ERR_OK;
 }

err_exit: 
 
 osMutexRelease(gsm_mutex);
 return rc;
}

/* 函数名：gsm_m6312_open_client
*  功能：  打开连接
*  参数：  conn_id 连接ID
*  参数：  protocol协议类型<TCP或者UDP>
*  参数：  host 主机IP后者域名
*  参数：  port 主机端口号
*  返回：  GSM_ERR_OK：成功 其他：失败
*/
int gsm_m6312_open_client(int conn_id,gsm_m6312_net_protocol_t protocol,const char *host,uint16_t port)
{
 int rc;
 at_err_code_t conn_ok,bind_ok,already,conn_fail,bind_fail,timeout,err;
 
 osMutexWait(gsm_mutex,osWaitForever);
 
 memset(&gsm_at,0,sizeof(gsm_at));
 snprintf(gsm_at.send,GSM_M6312_BUFFER_SIZE,"AT+IPSTART=%1d,%s,%s,%d\r\n",conn_id,protocol ==  GSM_M6312_NET_PROTOCOL_TCP ? "\"TCP\"" : "\"UDP\"",host,port);
 gsm_at.send_size = strlen(gsm_at.send);
 gsm_at.send_timeout = 10;
 gsm_at.recv_timeout = 20000;
 
 conn_ok.str = "CONNECT OK\r\n";
 conn_ok.code = GSM_ERR_OK;
 conn_ok.next = NULL;
 
 already.str = "ALREADY CONNECT\r\n";
 already.code = GSM_ERR_OK;
 already.next = NULL;
 
 conn_fail.str = "CONNECT FAIL\r\n";
 conn_fail.code = GSM_ERR_SOCKET_CONNECT_FAIL;
 conn_fail.next = NULL;
 
 bind_ok.str = "BIND OK\r\n\r\nOK\r\n";
 bind_ok.code = GSM_ERR_OK;
 bind_ok.next = NULL;
 
 bind_fail.str = "BIND FAIL\r\n";
 bind_fail.code = GSM_ERR_SOCKET_CONNECT_FAIL;
 bind_fail.next = NULL;
 
 timeout.str = "COMMAND TIMEOUT\r\n";
 timeout.code = GSM_ERR_CMD_TIMEOUT;
 timeout.next = NULL;
 
 err.str = "ERROR";
 err.code = GSM_ERR_CMD_ERR;
 err.next = NULL;
 
 at_err_code_add(&gsm_at.err_head,&conn_ok);
 at_err_code_add(&gsm_at.err_head,&bind_ok);
 at_err_code_add(&gsm_at.err_head,&already);
 at_err_code_add(&gsm_at.err_head,&conn_fail);
 at_err_code_add(&gsm_at.err_head,&bind_fail);
 at_err_code_add(&gsm_at.err_head,&timeout);
 at_err_code_add(&gsm_at.err_head,&err);

 rc = at_excute(gsm_m6312_serial_handle,&gsm_at);
 if(rc == GSM_ERR_OK){
 rc = conn_id;
 }
 
 osMutexRelease(gsm_mutex);
 return rc;
}

/* 函数名：gsm_m6312_close_client
*  功能：  关闭连接
*  参数：  conn_id 连接ID
*  返回：  GSM_ERR_OK：成功 其他：失败
*/
int gsm_m6312_close_client(int conn_id)
{
 int rc;
 at_err_code_t ok,err;
 
 osMutexWait(gsm_mutex,osWaitForever);
 
 memset(&gsm_at,0,sizeof(gsm_at));
 snprintf(gsm_at.send,AT_SEND_BUFFER_SIZE,"AT+IPCLOSE=%1d\r\n",conn_id);
 gsm_at.send_size = strlen(gsm_at.send);
 gsm_at.send_timeout = 10;
 gsm_at.recv_timeout = 5000;
 ok.str = "OK\r\n";
 ok.code = GSM_ERR_OK;
 ok.next = NULL;
 err.str = "ERROR";
 err.code = GSM_ERR_CMD_ERR;
 err.next = NULL;
 
 at_err_code_add(&gsm_at.err_head,&ok);
 at_err_code_add(&gsm_at.err_head,&err);
 
 rc = at_excute(gsm_m6312_serial_handle,&gsm_at);
 
 
 osMutexRelease(gsm_mutex);
 return rc;
}

/* 函数名：gsm_m6312_get_connect_status
*  功能：  获取连接的状态
*  参数：  conn_id 连接ID
*  参数：  status连接状态指针
*  返回：  GSM_ERR_OK：成功 其他：失败
*/
int gsm_m6312_get_connect_status(const int conn_id,gsm_m6312_socket_status_t *status)
{
 int rc;
 char conn_id_str[10] = { 0 };
 char *status_str;
 at_err_code_t ok,err;
 
 osMutexWait(gsm_mutex,osWaitForever);
 snprintf(conn_id_str,10,"con_id:%1d",conn_id);
 memset(&gsm_at,0,sizeof(gsm_at));
 strcpy(gsm_at.send,"AT+IPSTATE\r\n");
 gsm_at.send_size = strlen(gsm_at.send);
 gsm_at.send_timeout = 5;
 gsm_at.recv_timeout = 20000;
 ok.str = "OK\r\n";
 ok.code = GSM_ERR_OK;
 ok.next = NULL;
 err.str = "ERROR";
 err.code = GSM_ERR_CMD_ERR;
 err.next = NULL;
 
 at_err_code_add(&gsm_at.err_head,&ok);
 at_err_code_add(&gsm_at.err_head,&err);
 
 rc = at_excute(gsm_m6312_serial_handle,&gsm_at);
 if(rc == GSM_ERR_OK){
 status_str = strstr(gsm_at.recv,conn_id_str);
 if(status_str){
 if(strstr(status_str,"IP INITIAL")){
 *status = GSM_M6312_SOCKET_INIT;
 }else if(strstr(status_str,"IP CONNECT")){
 *status = GSM_M6312_SOCKET_CONNECT;  
 }else if(strstr(status_str,"IP CLOSE")){
 *status = GSM_M6312_SOCKET_CLOSE; 
 }else if(strstr(status_str,"BIND OK\r\n")){
 *status = GSM_M6312_SOCKET_CONNECT; 
 }else if(strstr(status_str,"IP CONNECTTING")){
 *status = GSM_M6312_SOCKET_CONNECTTING; 
 }else{
 rc = GSM_ERR_UNKNOW;  
 }
 }else{
 *status = GSM_M6312_SOCKET_INIT; 
 }  
 }
 
 
 osMutexRelease(gsm_mutex);
 return rc;
}

/* 函数名：gsm_m6312_send
*  功能：  发送数据
*  参数：  conn_id 连接ID
*  参数：  data 数据buffer
*  参数：  size 发送的数据数量
*  返回：  >=0：成功发送的数据 其他：失败
*/
int gsm_m6312_send(int conn_id,const char *data,const int size)
{
 int rc;
 int send_size;
 at_err_code_t ok,fail,timeout,err;
 
 osMutexWait(gsm_mutex,osWaitForever);
 /*第1步 启动发送*/
 memset(&gsm_at,0,sizeof(gsm_at));
 snprintf(gsm_at.send,AT_SEND_BUFFER_SIZE,"AT+IPSEND=%1d,%d\r\n",conn_id,size);
 gsm_at.send_size = strlen(gsm_at.send);
 gsm_at.send_timeout = 10;
 gsm_at.recv_timeout = 5000;
 ok.str = "> ";
 ok.code = GSM_ERR_OK;
 ok.next = NULL;
 err.str = "ERROR";
 err.code = GSM_ERR_CMD_ERR;
 err.next = NULL;
 
 at_err_code_add(&gsm_at.err_head,&ok);
 at_err_code_add(&gsm_at.err_head,&err);
 
 rc = at_excute(gsm_m6312_serial_handle,&gsm_at);
 /*第2步执行发送数据*/
 if( rc == GSM_ERR_OK){
 memset(&gsm_at,0,sizeof(gsm_at));
 send_size = AT_SEND_BUFFER_SIZE > size ? size : AT_SEND_BUFFER_SIZE;
 memcpy(gsm_at.send,data,send_size);
 gsm_at.send_size = send_size;
 gsm_at.send_timeout = send_size / 8 + 10;
 gsm_at.recv_timeout = 20000;
 ok.str = "SEND OK\r\n";
 ok.code = GSM_ERR_OK;
 ok.next = NULL;
 fail.str = "SEND FAIL\r\n";
 fail.code = GSM_ERR_SOCKET_SEND_FAIL;
 fail.next = NULL;
 timeout.str = "COMMAND TIMEOUT\r\n";
 timeout.code = GSM_ERR_CMD_TIMEOUT;
 timeout.next = NULL;
 err.str = "ERROR";
 err.code = GSM_ERR_CMD_ERR;
 err.next = NULL;
 
 at_err_code_add(&gsm_at.err_head,&ok);
 at_err_code_add(&gsm_at.err_head,&fail);
 at_err_code_add(&gsm_at.err_head,&timeout);
 at_err_code_add(&gsm_at.err_head,&err);
 
 rc = at_excute(gsm_m6312_serial_handle,&gsm_at);  
 if(rc == GSM_ERR_OK){
 rc = send_size;  
 }
 }
 
 
 osMutexRelease(gsm_mutex);
 return rc;
}

/* 函数名：gsm_m6312_config_recv_buffer
*  功能：  配置数据接收缓存
*  参数：  buffer 缓存类型
*  返回：  GSM_ERR_OK：成功 其他：失败
*/
int gsm_m6312_config_recv_buffer(gsm_m6312_recv_buffer_t buffer)
{
 int rc;
 at_err_code_t ok,err;
 
 osMutexWait(gsm_mutex,osWaitForever);
 
 memset(&gsm_at,0,sizeof(gsm_at));
 if(buffer == GSM_M6312_RECV_BUFFERE){
 strcpy(gsm_at.send,"AT+CMNDI=1,0\r\n");
 }else{
 strcpy(gsm_at.send,"AT+CMNDI=0,0\r\n");
 }
 gsm_at.send_size = strlen(gsm_at.send);
 gsm_at.send_timeout = 5;
 gsm_at.recv_timeout = 5000;
 ok.str = "OK\r\n";
 ok.code = GSM_ERR_OK;
 ok.next = NULL;
 err.str = "ERROR";
 err.code = GSM_ERR_CMD_ERR;
 err.next = NULL;
 
 at_err_code_add(&gsm_at.err_head,&ok);
 at_err_code_add(&gsm_at.err_head,&err);
 
 rc = at_excute(gsm_m6312_serial_handle,&gsm_at);
 
 
 osMutexRelease(gsm_mutex);
 return rc;
}

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
 int str_rc;
 
 char conn_id_str[10] = {0};
 char *buffer_size_str,*break_str;
 int buffer_size;
 at_err_code_t ok,err;
  
 if(size == 0 ){
 return 0;  
 }
 
 osMutexWait(gsm_mutex,osWaitForever);
 snprintf(conn_id_str,10,"+CMRD: %1d",conn_id); 
 /*第一步 查询缓存数据*/
 memset(&gsm_at,0,sizeof(gsm_at));
 strcpy(gsm_at.send,"AT+CMRD?\r\n");
 gsm_at.send_size = strlen(gsm_at.send);
 gsm_at.send_timeout = 5;
 gsm_at.recv_timeout = 5000;
 ok.str = "OK\r\n";
 ok.code = GSM_ERR_OK;
 ok.next = NULL;
 err.str = "ERROR";
 err.code = GSM_ERR_CMD_ERR;
 err.next = NULL;
 
 at_err_code_add(&gsm_at.err_head,&ok);
 at_err_code_add(&gsm_at.err_head,&err);
 
 rc = at_excute(gsm_m6312_serial_handle,&gsm_at);
 if(rc == GSM_ERR_OK){
 buffer_size_str = strstr(gsm_at.recv,conn_id_str);
 if(buffer_size_str){
    str_rc = utils_get_str_addr_by_num((char *)gsm_at.recv,",",2,&break_str);
     if(str_rc != 0){
        rc = GSM_ERR_UNKNOW;
        goto err_exit;
     }
    buffer_size_str = break_str + 1;

    buffer_size = atoi(buffer_size_str);                            
    if(buffer_size == 0){
       rc = 0;
       goto err_exit;
    }
 }else{
   rc = GSM_ERR_UNKNOW;    
   goto err_exit;
 }
 
  /*读取对应的数据*/
  memset(&gsm_at,0,sizeof(gsm_at));
  buffer_size = buffer_size > size ? size : buffer_size;
  snprintf(gsm_at.send,AT_SEND_BUFFER_SIZE,"AT+CMRD=%1d,%d\r\n",conn_id,buffer_size); 
  gsm_at.send_size = strlen(gsm_at.send);
  gsm_at.send_timeout = 10;
  gsm_at.recv_timeout = 5000;
 
  ok.str = "OK\r\n";
  ok.code = GSM_ERR_OK;
  err.str = "ERROR";
  err.code = GSM_ERR_CMD_ERR;
  at_err_code_add(&gsm_at.err_head,&ok);
  at_err_code_add(&gsm_at.err_head,&err);
 
  rc = at_excute(gsm_m6312_serial_handle,&gsm_at);
  if(rc == GSM_ERR_OK){
     memcpy(buffer,gsm_at.recv,buffer_size);
     rc = buffer_size;  
  }
 }
 
err_exit: 
 
 osMutexRelease(gsm_mutex);
 return rc;
}
