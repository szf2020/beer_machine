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
#define  GSM_M6312_PWR_OFF_DELAY       12000

static gsm_m6312_at_cmd_t gsm_cmd;

/* 函数名：gsm_m6312_pwr_on
*  功能：  m6312 2g模块开机
*  参数：  无 
*  返回：  GSM_ERR_OK：成功 其他：失败
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
*  返回：  GSM_ERR_OK：成功 其他：失败
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
*  返回：  GSM_ERR_OK：成功 其他：失败
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
static void gsm_m6312_print_err_info(const char *send,const char *recv,const int err_code)
{
  if(send){
  log_debug("send:\r\n%s\r\n",send);
  }
  
  if(recv){
  log_debug("recv:\r\n%s\r\n",recv);
  }
  
  if(err_code > 0){
  log_debug("size:%d\r\n",err_code);
  return;   
  }
  
  switch(err_code)
  {
  case  GSM_ERR_OK:
  log_debug("GSM ERR CODE:%d cmd OK\r\n.\r\n",err_code);
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
  
  case  GSM_ERR_SEND_TIMEOUT:
  log_error("GSM ERR CODE:%d cmd send timeout.\r\n",err_code);   
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
  
  case  GSM_ERR_SEND_NO_SPACE:
  log_error("GSM ERR CODE:%d serial recv no space.\r\n",err_code);   
  break;
  
  case  GSM_ERR_SOCKET_ALREADY_CONNECT:
  log_error("GSM ERR CODE:%d socket alread connect.\r\n",err_code);   
  break;
  
  case  GSM_ERR_SOCKET_CONNECT_FAIL:
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



/* 函数名：gsm_m6312_err_code_add
*  功能：  添加比对的错误码
*  参数：  err_head 错误头指针 
*  参数：  err_code 错误节点 
*  返回：  无
*/
static void gsm_m6312_err_code_add(gsm_m6312_err_code_t **err_head,gsm_m6312_err_code_t *err_code)
{
    gsm_m6312_err_code_t **err_node;
 
    if (*err_head == NULL){
        *err_head = err_code;
        return;
    }
    err_node = err_head;
    while((*err_node)->next){
    err_node = (gsm_m6312_err_code_t **)(&(*err_node)->next);   
    }
    (*err_node)->next = err_code;
}


/* 函数名：gsm_m6312_at_cmd_send
*  功能：  串口发送
*  参数：  send 发送的数据 
*  参数：  size 数据大小 
*  参数：  timeout 发送超时 
*  返回：  GSM_ERR_OK：成功 其他：失败
*/
static int gsm_m6312_at_cmd_send(const char *send,const uint16_t size,const uint16_t timeout)
{
 int rc;
 uint16_t write_total = 0;
 uint16_t remain_total = size;
 uint16_t write_timeout = timeout;
 int write_len;

 do{
 write_len = serial_write(gsm_m6312_serial_handle,(uint8_t *)send + write_total,remain_total);
 if(write_len == -1){
 log_error("at cmd write err.\r\n");
 return GSM_ERR_SERIAL_SEND;  
 }
 write_total += write_len;
 remain_total -= write_len;
 write_timeout -- ;
 osDelay(1);
 }while(remain_total > 0 && write_timeout > 0);  
  
 if(remain_total > 0){
 return GSM_ERR_SEND_TIMEOUT;
 }

 rc = serial_complete(gsm_m6312_serial_handle,write_timeout);
 if( rc < 0){
 return GSM_ERR_SERIAL_SEND;  ;
 }
 
 if (rc > 0){
 return GSM_ERR_SEND_TIMEOUT;  ;
 }
 
 return GSM_ERR_OK;
}

/* 函数名：gsm_m6312_at_cmd_recv
*  功能：  串口接收
*  参数：  recv 数据buffer 
*  参数：  size 数据大小 
*  参数：  timeout 接收超时 
*  返回：  读到的一帧数据量 其他：失败
*/
#define  GSM_M6312_SELECT_TIMEOUT               5
static int gsm_m6312_at_cmd_recv(char *recv,const uint16_t size,const uint32_t timeout)
{
 int select_size;
 uint16_t read_total = 0;
 int read_size;
 uint32_t read_timeout = timeout;

 /*等待数据*/
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
 recv[read_total] = '\0';
 return read_total;
 }

/* 函数名：gsm_m6312_cmd_check_response
*  功能：  检查回应
*  参数：  rsp 回应数组 
*  参数：  err_head 错误头 
*  参数：  complete 回应是否结束 
*  返回：  GSM_ERR_OK：成功 其他：失败 
*/
static int gsm_m6312_cmd_check_response(const char *rsp,uint16_t size,gsm_m6312_err_code_t *err_head,bool *complete)
{
    gsm_m6312_err_code_t *err_node;
    char *check_pos;
    
    /*接受完一帧数据标志*/
    //log_warning("**\r\n");

    err_node = err_head;
    while(err_node){
    /*错误从头检测*/
    if(strcmp(err_node->str,"ERROR") == 0){
    check_pos = (char *)rsp;
    }else{
   /*其他的从尾部检测*/
    check_pos = (char *)rsp + size - strlen(err_node->str);  
    }
    if(strstr(check_pos,err_node->str)){
    *complete = true;
    return err_node->code;
    }
    err_node = err_node->next;
    }
    
    return GSM_ERR_OK;     
}
  
/* 函数名：gsm_m6312_at_cmd_excute
*  功能：  wifi at命令执行
*  参数：  cmd 命令指针 
*  返回：  返回：  GSM_ERR_OK：成功 其他：失败 
*/  
static int gsm_m6312_at_cmd_excute(gsm_m6312_at_cmd_t *at_cmd)
{
    int rc;
    int recv_size;
    uint32_t start_time,cur_time,time_left;
    
    rc = gsm_m6312_at_cmd_send(at_cmd->send,at_cmd->send_size,at_cmd->send_timeout);
    if (rc != GSM_ERR_OK){
    return rc;
    }
    
    /*发送完一帧数据的标志*/
    //log_warning("++\r\n");
    /*清空数据*/
    serial_flush(gsm_m6312_serial_handle);
 
    start_time = osKernelSysTick();
    time_left = at_cmd->recv_timeout;
    while (time_left){
    recv_size = gsm_m6312_at_cmd_recv(at_cmd->recv + at_cmd->recv_size,GSM_M6312_RECV_BUFFER_SIZE - at_cmd->recv_size,time_left);
    if (recv_size < 0){
    return recv_size; 
    }
    /*判断这帧数据是否是系统输出的无用数据*/
    if(strstr(at_cmd->recv + at_cmd->recv_size,"CONNECTION CLOSED:")){
    log_warning("recv sys prompt:%s\r\ndiscard!\r\n",at_cmd->recv + at_cmd->recv_size);  
    }else{
    at_cmd->recv_size += recv_size;
    /*校验回应数据*/
    rc = gsm_m6312_cmd_check_response(at_cmd->recv,at_cmd->recv_size,at_cmd->err_head,&at_cmd->complete);
    if(at_cmd->complete){
    return rc;
    }
    }
    cur_time = osKernelSysTick();
    time_left = at_cmd->recv_timeout > cur_time - start_time ? at_cmd->recv_timeout - (cur_time - start_time) : 0;
    }
    
    return GSM_ERR_UNKNOW;
}

/* 函数名：gsm_m6312_get_sim_card_status
*  功能：  获取设备sim卡状态
*  参数：  sim_status sim状态指针
*  返回：  GSM_ERR_OK：成功 其他：失败
*/
int gsm_m6312_get_sim_card_status(sim_card_status_t *sim_status)
{
 int rc;
 gsm_m6312_err_code_t ok,err;
 
 osMutexWait(gsm_mutex,osWaitForever);
 
 memset(&gsm_cmd,0,sizeof(gsm_cmd));
 strcpy(gsm_cmd.send,"AT+CPIN?\r\n");
 gsm_cmd.send_size = strlen(gsm_cmd.send);
 gsm_cmd.send_timeout = 5;
 gsm_cmd.recv_timeout = 1000;
 ok.str = "OK\r\n";
 ok.code = GSM_ERR_OK;
 ok.next = NULL;
 err.str = "ERROR";
 err.code = GSM_ERR_CMD_ERR;
 err.next = NULL;
 
 gsm_m6312_err_code_add(&gsm_cmd.err_head,&ok);
 gsm_m6312_err_code_add(&gsm_cmd.err_head,&err);
 
 rc = gsm_m6312_at_cmd_excute(&gsm_cmd);

 if(rc == GSM_ERR_OK){
   
 if(strstr(gsm_cmd.recv,"READY")){
 *sim_status = SIM_CARD_STATUS_READY;
 }else if(strstr(gsm_cmd.recv,"NO SIM")){
 *sim_status = SIM_CARD_STATUS_NO_SIM_CARD;
 }else if(strstr(gsm_cmd.recv,"BLOCK")){
 *sim_status = SIM_CARD_STATUS_BLOCK;
 }else{
 rc = GSM_ERR_UNKNOW;
 } 
 }  
 gsm_m6312_print_err_info(gsm_cmd.send,gsm_cmd.recv,rc);
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
 gsm_m6312_err_code_t ok,err;
 
 osMutexWait(gsm_mutex,osWaitForever);
 
 memset(&gsm_cmd,0,sizeof(gsm_cmd));
 strcpy(gsm_cmd.send,"AT+CCID?\r\n");
 gsm_cmd.send_size = strlen(gsm_cmd.send);
 gsm_cmd.send_timeout = 5;
 gsm_cmd.recv_timeout = 1000;
 ok.str = "OK\r\n";
 ok.code = GSM_ERR_OK;
 ok.next = NULL;
 err.str = "ERROR";
 err.code = GSM_ERR_CMD_ERR;
 err.next = NULL;
 
 gsm_m6312_err_code_add(&gsm_cmd.err_head,&ok);
 gsm_m6312_err_code_add(&gsm_cmd.err_head,&err);
 
 rc = gsm_m6312_at_cmd_excute(&gsm_cmd);

 if(rc == GSM_ERR_OK){ 
 ccid_str = strstr(gsm_cmd.recv,"+CCID: ");
 if(ccid_str){
 memcpy(sim_id,ccid_str + strlen("+CCID: "),20); 
 sim_id[20] = '\0';
 }else{
 rc = GSM_ERR_UNKNOW;   
 }
 }  
 gsm_m6312_print_err_info(gsm_cmd.send,gsm_cmd.recv,rc);
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
 gsm_m6312_err_code_t ok,err;
 
 osMutexWait(gsm_mutex,osWaitForever);
 
 memset(&gsm_cmd,0,sizeof(gsm_cmd));
 strcpy(gsm_cmd.send,"AT+CSQ\r\n");
 gsm_cmd.send_size = strlen(gsm_cmd.send);
 gsm_cmd.send_timeout = 5;
 gsm_cmd.recv_timeout = 1000;
 ok.str = "OK\r\n";
 ok.code = GSM_ERR_OK;
 ok.next = NULL;
 err.str = "ERROR";
 err.code = GSM_ERR_CMD_ERR;
 err.next = NULL;
 
 gsm_m6312_err_code_add(&gsm_cmd.err_head,&ok);
 gsm_m6312_err_code_add(&gsm_cmd.err_head,&err);
 
 rc = gsm_m6312_at_cmd_excute(&gsm_cmd);

 if(rc == GSM_ERR_OK){ 
 rc = GSM_ERR_UNKNOW;   
 rssi_str = strstr(gsm_cmd.recv,"+CSQ: ");
 if(rssi_str){
 rssi_str +=strlen("+CSQ: ");
 break_str = strstr(rssi_str ,",");
 if(break_str){
 memcpy(rssi,rssi_str,break_str - rssi_str);
 rssi[break_str - rssi_str] = '\0';
 }
 }
 }
 
 gsm_m6312_print_err_info(gsm_cmd.send,gsm_cmd.recv,rc);
 osMutexRelease(gsm_mutex);
 return rc;
}

/*查找指定位置和指定字符后的地址*/
static int gsm_m6312_get_str_addr(char *buffer,const char *flag,const uint8_t num,char **addr)
{
 uint8_t cnt = 0;
 uint16_t flag_size;
 
 char *search,*temp;
 
 temp = buffer;
 flag_size = strlen(flag);
 
 while(cnt < num ){
 search = strstr(temp,flag);
 if(search == NULL){
   return -1;
 }
 temp = search + flag_size;
 cnt ++;
 }
 *addr = search;
 return 0; 
}

/*查找指定位置和指定字符串后面的字符串*/
static int gsm_m6312_get_str(char *buffer,char *dst,const char *flag,const uint8_t num)
{
 int rc;
 uint16_t flag_size;
 
 char *addr_start,*addr_end;
 flag_size = strlen(flag);
 
 rc = gsm_m6312_get_str_addr(buffer,flag,num,&addr_start);
 if(rc != 0){
    return -1;
 }
 addr_start += flag_size;
 rc = gsm_m6312_get_str_addr(buffer,flag,num + 1,&addr_end);
 if(rc != 0){
    return -1;
 }
 memcpy(dst,addr_start,addr_end - addr_start);
 dst[addr_end - addr_start] = '\0';
 
 return 0; 
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
 gsm_m6312_err_code_t ok,err;
 
 osMutexWait(gsm_mutex,osWaitForever);
 
 memset(&gsm_cmd,0,sizeof(gsm_cmd));
 strcpy(gsm_cmd.send,"AT+CCED=0,2\r\n");
 gsm_cmd.send_size = strlen(gsm_cmd.send);
 gsm_cmd.send_timeout = 5;
 gsm_cmd.recv_timeout = 1000;
 ok.str = "OK\r\n";
 ok.code = GSM_ERR_OK;
 ok.next = NULL;
 err.str = "ERROR";
 err.code = GSM_ERR_CMD_ERR;
 err.next = NULL;
 
 gsm_m6312_err_code_add(&gsm_cmd.err_head,&ok);
 gsm_m6312_err_code_add(&gsm_cmd.err_head,&err);
 
 rc = gsm_m6312_at_cmd_excute(&gsm_cmd);

 if(rc == GSM_ERR_OK){ 
 rc_search = gsm_m6312_get_str_addr(gsm_cmd.recv,"+CCED:",1,&base_str);
 if(rc_search != 0){
   rc = GSM_ERR_UNKNOW;  
   goto err_exit;
 } 
 
 for(uint8_t i =0 ;i < 3;i++){
 rc_search = gsm_m6312_get_str(gsm_cmd.recv,assist_base->base[i].lac,",",2 + i * 6);
 if(rc_search != 0){
   rc = GSM_ERR_UNKNOW;  
   goto err_exit;
 } 
 rc_search = gsm_m6312_get_str(gsm_cmd.recv,assist_base->base[i].ci,",",3 + i * 6);
 if(rc_search != 0){
   rc = GSM_ERR_UNKNOW;  
   goto err_exit;
 }
 
 rc_search = gsm_m6312_get_str(gsm_cmd.recv,assist_base->base[i].rssi,",",5 + i * 6);
 if(rc_search != 0){
   rc = GSM_ERR_UNKNOW;  
   goto err_exit;
 } 
 }
 }
 

err_exit: 
 gsm_m6312_print_err_info(gsm_cmd.send,gsm_cmd.recv,rc);
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
 gsm_m6312_err_code_t ok,err;
 
 osMutexWait(gsm_mutex,osWaitForever);
 
 memset(&gsm_cmd,0,sizeof(gsm_cmd));
 if(echo == GSM_M6312_ECHO_ON){
 strcpy(gsm_cmd.send,"ATE1\r\n");
 }else{
 strcpy(gsm_cmd.send,"ATE0\r\n");
 }
 gsm_cmd.send_size = strlen(gsm_cmd.send);
 gsm_cmd.send_timeout = 5;
 gsm_cmd.recv_timeout = 1000;
 ok.str = "OK\r\n";
 ok.code = GSM_ERR_OK;
 ok.next = NULL;
 err.str = "ERROR";
 err.code = GSM_ERR_CMD_ERR;
 err.next = NULL;
 
 gsm_m6312_err_code_add(&gsm_cmd.err_head,&ok);
 gsm_m6312_err_code_add(&gsm_cmd.err_head,&err);
 
 rc = gsm_m6312_at_cmd_excute(&gsm_cmd);

 gsm_m6312_print_err_info(gsm_cmd.send,gsm_cmd.recv,rc);
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
 gsm_m6312_err_code_t ok,err;
 
 osMutexWait(gsm_mutex,osWaitForever);
 
 memset(&gsm_cmd,0,sizeof(gsm_cmd));
 strcpy(gsm_cmd.send,"AT+CGSN\r\n");
 gsm_cmd.send_size = strlen(gsm_cmd.send);
 gsm_cmd.send_timeout = 5;
 gsm_cmd.recv_timeout = 1000;
 ok.str = "OK\r\n";
 ok.code = GSM_ERR_OK;
 ok.next = NULL;
 err.str = "ERROR";
 err.code = GSM_ERR_CMD_ERR;
 err.next = NULL;
 
 gsm_m6312_err_code_add(&gsm_cmd.err_head,&ok);
 gsm_m6312_err_code_add(&gsm_cmd.err_head,&err);
 
 rc = gsm_m6312_at_cmd_excute(&gsm_cmd);
 if(rc == GSM_ERR_OK){
 /*找到开始标志*/
 temp = strstr(gsm_cmd.recv,"+CGSN: ");
 if(temp){
 /*imei 15位*/
 memcpy(imei,temp + strlen("+CGSN: "),15);
 imei[15] = '\0';
 }else{
 rc = GSM_ERR_UNKNOW;
 }
 }
 
 gsm_m6312_print_err_info(gsm_cmd.send,gsm_cmd.recv,rc);
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
 gsm_m6312_err_code_t ok,err;
 
 osMutexWait(gsm_mutex,osWaitForever);
 
 memset(&gsm_cmd,0,sizeof(gsm_cmd));
 strcpy(gsm_cmd.send,"AT^SN\r\n");
 gsm_cmd.send_size = strlen(gsm_cmd.send);
 gsm_cmd.send_timeout = 5;
 gsm_cmd.recv_timeout = 1000;
 ok.str = "OK\r\n";
 ok.code = GSM_ERR_OK;
 ok.next = NULL;
 err.str = "ERROR";
 err.code = GSM_ERR_CMD_ERR;
 err.next = NULL;
 
 gsm_m6312_err_code_add(&gsm_cmd.err_head,&ok);
 gsm_m6312_err_code_add(&gsm_cmd.err_head,&err);
 
 rc = gsm_m6312_at_cmd_excute(&gsm_cmd);
 if(rc == GSM_ERR_OK){
 /*找到开始标志*/
 temp = strstr(gsm_cmd.recv,"M6312");
 if(temp){
 /*sn 20位*/
 memcpy(sn,temp,GSM_M6312_SN_LEN);
 sn[GSM_M6312_SN_LEN] = '\0';
 }else{
 rc = GSM_ERR_UNKNOW;
 }
 }
 
 gsm_m6312_print_err_info(gsm_cmd.send,gsm_cmd.recv,rc);
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
 gsm_m6312_err_code_t ok,err;
 
 osMutexWait(gsm_mutex,osWaitForever);
 
 memset(&gsm_cmd,0,sizeof(gsm_cmd));
 snprintf(gsm_cmd.send,GSM_M6312_SEND_BUFFER_SIZE,"AT+CGDCONT=1,\"IP\",%s\r\n",apn == GSM_M6312_APN_CMNET ? "\"CMNET\"" : "\"UNINET\"");
 gsm_cmd.send_size = strlen(gsm_cmd.send);
 gsm_cmd.send_timeout = 5;
 gsm_cmd.recv_timeout = 1000;
 ok.str = "OK\r\n";
 ok.code = GSM_ERR_OK;
 ok.next = NULL;
 err.str = "ERROR";
 err.code = GSM_ERR_CMD_ERR;
 err.next = NULL;
 
 gsm_m6312_err_code_add(&gsm_cmd.err_head,&ok);
 gsm_m6312_err_code_add(&gsm_cmd.err_head,&err);
 
 rc = gsm_m6312_at_cmd_excute(&gsm_cmd);
 
 gsm_m6312_print_err_info(gsm_cmd.send,gsm_cmd.recv,rc);
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
 gsm_m6312_err_code_t ok,err;
 
 osMutexWait(gsm_mutex,osWaitForever);
 
 memset(&gsm_cmd,0,sizeof(gsm_cmd));
 strcpy(gsm_cmd.send,"AT+CGDCONT?\r\n");
 gsm_cmd.send_size = strlen(gsm_cmd.send);
 gsm_cmd.send_timeout = 5;
 gsm_cmd.recv_timeout = 2000;
 ok.str = "OK\r\n";
 ok.code = GSM_ERR_OK;
 ok.next = NULL;
 err.str = "ERROR";
 err.code = GSM_ERR_CMD_ERR;
 err.next = NULL;
 
 gsm_m6312_err_code_add(&gsm_cmd.err_head,&ok);
 gsm_m6312_err_code_add(&gsm_cmd.err_head,&err);
 
 rc = gsm_m6312_at_cmd_excute(&gsm_cmd);
 if(rc == GSM_ERR_OK){
 apn_str = strstr(gsm_cmd.recv,"CMNET");
 if(apn_str == NULL){
 apn_str = strstr(gsm_cmd.recv,"UNINET");
 if(apn_str == NULL){   
 rc = GSM_ERR_UNKNOW;
 }else{
 *apn = GSM_M6312_APN_UNINET;
 }
 }else{
 *apn = GSM_M6312_APN_CMNET;
 }  
 }
 
 gsm_m6312_print_err_info(gsm_cmd.send,gsm_cmd.recv,rc);
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
 gsm_m6312_err_code_t ok,err;
 
 osMutexWait(gsm_mutex,osWaitForever);
 
 memset(&gsm_cmd,0,sizeof(gsm_cmd));
 if(active == GSM_M6312_ACTIVE){
 strcpy(gsm_cmd.send,"AT+CGACT=1,1\r\n");
 }else{
 strcpy(gsm_cmd.send,"AT+CGACT=0,1\r\n");  
 }
 gsm_cmd.send_size = strlen(gsm_cmd.send);
 gsm_cmd.send_timeout = 5;
 gsm_cmd.recv_timeout = 5000;
 ok.str = "OK\r\n";
 ok.code = GSM_ERR_OK;
 ok.next = NULL;
 err.str = "ERROR";
 err.code = GSM_ERR_CMD_ERR;
 err.next = NULL;
 
 gsm_m6312_err_code_add(&gsm_cmd.err_head,&ok);
 gsm_m6312_err_code_add(&gsm_cmd.err_head,&err);
 
 rc = gsm_m6312_at_cmd_excute(&gsm_cmd);
 
 gsm_m6312_print_err_info(gsm_cmd.send,gsm_cmd.recv,rc);
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
 gsm_m6312_err_code_t ok,err;
 
 osMutexWait(gsm_mutex,osWaitForever);
 
 memset(&gsm_cmd,0,sizeof(gsm_cmd));
 strcpy(gsm_cmd.send,"AT+CGACT?\r\n");
 gsm_cmd.send_size = strlen(gsm_cmd.send);
 gsm_cmd.send_timeout = 5;
 gsm_cmd.recv_timeout = 1000;
 ok.str = "OK\r\n";
 ok.code = GSM_ERR_OK;
 ok.next = NULL;
 err.str = "ERROR";
 err.code = GSM_ERR_CMD_ERR;
 err.next = NULL;
 
 gsm_m6312_err_code_add(&gsm_cmd.err_head,&ok);
 gsm_m6312_err_code_add(&gsm_cmd.err_head,&err);
 
 rc = gsm_m6312_at_cmd_excute(&gsm_cmd);
 if(rc == GSM_ERR_OK){
 if(strstr(gsm_cmd.recv,"+CGACT: 0,1") || strstr(gsm_cmd.recv,"+CGACT: 0,0")){
 *active = GSM_M6312_INACTIVE;
 }else if(strstr(gsm_cmd.recv,"+CGACT: 1,1")){
 *active = GSM_M6312_ACTIVE;
 }else{
 rc = GSM_ERR_UNKNOW;
 }
 }
 
 gsm_m6312_print_err_info(gsm_cmd.send,gsm_cmd.recv,rc);
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
 gsm_m6312_err_code_t ok,err;
 
 osMutexWait(gsm_mutex,osWaitForever);
 
 memset(&gsm_cmd,0,sizeof(gsm_cmd));
 if(attach == GSM_M6312_ATTACH){
 strcpy(gsm_cmd.send,"AT+CGATT=1\r\n");
 }else{
 strcpy(gsm_cmd.send,"AT+CGATT=0\r\n");
 }
 gsm_cmd.send_size = strlen(gsm_cmd.send);
 gsm_cmd.send_timeout = 5;
 gsm_cmd.recv_timeout = 10000;
 ok.str = "OK\r\n";
 ok.code = GSM_ERR_OK;
 ok.next = NULL;
 err.str = "ERROR";
 err.code = GSM_ERR_CMD_ERR;
 err.next = NULL;
 
 gsm_m6312_err_code_add(&gsm_cmd.err_head,&ok);
 gsm_m6312_err_code_add(&gsm_cmd.err_head,&err);
 
 rc = gsm_m6312_at_cmd_excute(&gsm_cmd);
 
 gsm_m6312_print_err_info(gsm_cmd.send,gsm_cmd.recv,rc);
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
 gsm_m6312_err_code_t ok,err;
 
 osMutexWait(gsm_mutex,osWaitForever);
 
 memset(&gsm_cmd,0,sizeof(gsm_cmd));
 strcpy(gsm_cmd.send,"AT+CGATT?\r\n");
 gsm_cmd.send_size = strlen(gsm_cmd.send);
 gsm_cmd.send_timeout = 5;
 gsm_cmd.recv_timeout = 1000;
 ok.str = "OK\r\n";
 ok.code = GSM_ERR_OK;
 ok.next = NULL;
 err.str = "ERROR";
 err.code = GSM_ERR_CMD_ERR;
 err.next = NULL;
 
 gsm_m6312_err_code_add(&gsm_cmd.err_head,&ok);
 gsm_m6312_err_code_add(&gsm_cmd.err_head,&err);
 
 rc = gsm_m6312_at_cmd_excute(&gsm_cmd);
 if(rc == GSM_ERR_OK){
 if(strstr(gsm_cmd.recv,"+CGATT: 1")){
 *attach = GSM_M6312_ATTACH;
 }else if(strstr(gsm_cmd.recv,"+CGACT: 0")){
 *attach = GSM_M6312_NOT_ATTACH;
 }else{
 rc = GSM_ERR_UNKNOW;
 }
 }
 
 gsm_m6312_print_err_info(gsm_cmd.send,gsm_cmd.recv,rc);
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
 gsm_m6312_err_code_t ok,err;
 
 osMutexWait(gsm_mutex,osWaitForever);
 
 memset(&gsm_cmd,0,sizeof(gsm_cmd));
 if(mode == GSM_M6312_CONNECT_MODE_SINGLE){ 
 strcpy(gsm_cmd.send,"AT+CMMUX=0\r\n");
 }else{
 strcpy(gsm_cmd.send,"AT+CMMUX=1\r\n");
 }
 gsm_cmd.send_size = strlen(gsm_cmd.send);
 gsm_cmd.send_timeout = 5;
 gsm_cmd.recv_timeout = 1000;
 ok.str = "OK\r\n";
 ok.code = GSM_ERR_OK;
 ok.next = NULL;
 err.str = "ERROR";
 err.code = GSM_ERR_CMD_ERR;
 err.next = NULL;
 
 gsm_m6312_err_code_add(&gsm_cmd.err_head,&ok);
 gsm_m6312_err_code_add(&gsm_cmd.err_head,&err);
 
 rc = gsm_m6312_at_cmd_excute(&gsm_cmd);
 
 gsm_m6312_print_err_info(gsm_cmd.send,gsm_cmd.recv,rc);
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
 gsm_m6312_err_code_t ok,err;
 
 osMutexWait(gsm_mutex,osWaitForever);
 
 memset(&gsm_cmd,0,sizeof(gsm_cmd));
 strcpy(gsm_cmd.send,"AT+CIMI\r\n");
 gsm_cmd.send_size = strlen(gsm_cmd.send);
 gsm_cmd.send_timeout = 5;
 gsm_cmd.recv_timeout = 1000;
 ok.str = "OK\r\n";
 ok.code = GSM_ERR_OK;
 ok.next = NULL;
 err.str = "ERROR";
 err.code = GSM_ERR_CMD_ERR;
 err.next = NULL;
 
 gsm_m6312_err_code_add(&gsm_cmd.err_head,&ok);
 gsm_m6312_err_code_add(&gsm_cmd.err_head,&err);
 
 rc = gsm_m6312_at_cmd_excute(&gsm_cmd);
 if(rc == GSM_ERR_OK){
 if(strstr(gsm_cmd.recv,"46000")){
 *operator_name = OPERATOR_CHINA_MOBILE;
 }else if(strstr(gsm_cmd.recv,"46001")){
 *operator_name = OPERATOR_CHINA_UNICOM;
 }else{
 rc = GSM_ERR_UNKNOW;
 }
 }
 
 gsm_m6312_print_err_info(gsm_cmd.send,gsm_cmd.recv,rc);
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
 gsm_m6312_err_code_t ok,err;
 
 osMutexWait(gsm_mutex,osWaitForever);
 
 memset(&gsm_cmd,0,sizeof(gsm_cmd));
 if(operator_format == GSM_M6312_OPERATOR_FORMAT_NUMERIC_NAME){ 
 strcpy(gsm_cmd.send,"AT+COPS=0,2\r\n");
 }else if(operator_format == GSM_M6312_OPERATOR_FORMAT_SHORT_NAME){
 strcpy(gsm_cmd.send,"AT+COPS=0,1\r\n");
 }else{
 strcpy(gsm_cmd.send,"AT+COPS=0,0\r\n");
 }
 gsm_cmd.send_size = strlen(gsm_cmd.send);
 gsm_cmd.send_timeout = 5;
 gsm_cmd.recv_timeout = 20000;
 ok.str = "OK\r\n";
 ok.code = GSM_ERR_OK;
 ok.next = NULL;
 err.str = "ERROR";
 err.code = GSM_ERR_CMD_ERR;
 err.next = NULL;
 
 gsm_m6312_err_code_add(&gsm_cmd.err_head,&ok);
 gsm_m6312_err_code_add(&gsm_cmd.err_head,&err);
 
 rc = gsm_m6312_at_cmd_excute(&gsm_cmd);
 
 gsm_m6312_print_err_info(gsm_cmd.send,gsm_cmd.recv,rc);
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
 gsm_m6312_err_code_t ok,err;
 
 osMutexWait(gsm_mutex,osWaitForever);
 
 memset(&gsm_cmd,0,sizeof(gsm_cmd));
 if(prompt == GSM_M6312_SEND_PROMPT){ 
 strcpy(gsm_cmd.send,"AT+PROMPT=1\r\n");
 }else {
 strcpy(gsm_cmd.send,"AT+PROMPT=0\r\n");
 }
 gsm_cmd.send_size = strlen(gsm_cmd.send);
 gsm_cmd.send_timeout = 5;
 gsm_cmd.recv_timeout = 1000;
 ok.str = "OK\r\n";
 ok.code = GSM_ERR_OK;
 ok.next = NULL;
 err.str = "ERROR";
 err.code = GSM_ERR_CMD_ERR;
 err.next = NULL;
 
 gsm_m6312_err_code_add(&gsm_cmd.err_head,&ok);
 gsm_m6312_err_code_add(&gsm_cmd.err_head,&err);
 
 rc = gsm_m6312_at_cmd_excute(&gsm_cmd);
 
 gsm_m6312_print_err_info(gsm_cmd.send,gsm_cmd.recv,rc);
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
 gsm_m6312_err_code_t ok,err;
 
 osMutexWait(gsm_mutex,osWaitForever);
 
 memset(&gsm_cmd,0,sizeof(gsm_cmd));
 if(transparent == GSM_M6312_TRANPARENT){
 strcpy(gsm_cmd.send,"AT+CMMODE=1\r\n");
 }else {
 strcpy(gsm_cmd.send,"AT+CMMODE=0\r\n");
 }
 gsm_cmd.send_size = strlen(gsm_cmd.send);
 gsm_cmd.send_timeout = 5;
 gsm_cmd.recv_timeout = 1000;
 ok.str = "OK\r\n";
 ok.code = GSM_ERR_OK;
 ok.next = NULL;
 err.str = "ERROR";
 err.code = GSM_ERR_CMD_ERR;
 err.next = NULL;
 
 gsm_m6312_err_code_add(&gsm_cmd.err_head,&ok);
 gsm_m6312_err_code_add(&gsm_cmd.err_head,&err);
 
 rc = gsm_m6312_at_cmd_excute(&gsm_cmd);
 
 gsm_m6312_print_err_info(gsm_cmd.send,gsm_cmd.recv,rc);
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
 gsm_m6312_err_code_t ok,err;
 
 osMutexWait(gsm_mutex,osWaitForever);
 
 memset(&gsm_cmd,0,sizeof(gsm_cmd));
 if(report == GSM_M6312_REPORT_ON){
 strcpy(gsm_cmd.send,"AT^CURC=1\r\n");
 }else {
 strcpy(gsm_cmd.send,"AT^CURC=0\r\n");
 }
 gsm_cmd.send_size = strlen(gsm_cmd.send);
 gsm_cmd.send_timeout = 5;
 gsm_cmd.recv_timeout = 1000;
 ok.str = "OK\r\n";
 ok.code = GSM_ERR_OK;
 ok.next = NULL;
 err.str = "ERROR";
 err.code = GSM_ERR_CMD_ERR;
 err.next = NULL;
 
 gsm_m6312_err_code_add(&gsm_cmd.err_head,&ok);
 gsm_m6312_err_code_add(&gsm_cmd.err_head,&err);
 
 rc = gsm_m6312_at_cmd_excute(&gsm_cmd);
 
 gsm_m6312_print_err_info(gsm_cmd.send,gsm_cmd.recv,rc);
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
 gsm_m6312_err_code_t ok,err;
 
 osMutexWait(gsm_mutex,osWaitForever);
 
 memset(&gsm_cmd,0,sizeof(gsm_cmd));
 if(reg_echo == GSM_M6312_REG_ECHO_ON){
 strcpy(gsm_cmd.send,"AT+CGREG=2\r\n");
 }else {
 strcpy(gsm_cmd.send,"AT+CGREG=0\r\n");
 }
 gsm_cmd.send_size = strlen(gsm_cmd.send);
 gsm_cmd.send_timeout = 5;
 gsm_cmd.recv_timeout = 1000;
 ok.str = "OK\r\n";
 ok.code = GSM_ERR_OK;
 ok.next = NULL;
 err.str = "ERROR";
 err.code = GSM_ERR_CMD_ERR;
 err.next = NULL;
 
 gsm_m6312_err_code_add(&gsm_cmd.err_head,&ok);
 gsm_m6312_err_code_add(&gsm_cmd.err_head,&err);
 
 rc = gsm_m6312_at_cmd_excute(&gsm_cmd);
 
 gsm_m6312_print_err_info(gsm_cmd.send,gsm_cmd.recv,rc);
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
 char *temp;
 char *break_str;
 gsm_m6312_err_code_t ok,err;
 
 osMutexWait(gsm_mutex,osWaitForever);
 
 memset(&gsm_cmd,0,sizeof(gsm_cmd));
 strcpy(gsm_cmd.send,"AT+CGREG?\r\n");

 gsm_cmd.send_size = strlen(gsm_cmd.send);
 gsm_cmd.send_timeout = 5;
 gsm_cmd.recv_timeout = 2000;
 ok.str = "OK\r\n";
 ok.code = GSM_ERR_OK;
 ok.next = NULL;
 err.str = "ERROR";
 err.code = GSM_ERR_CMD_ERR;
 err.next = NULL;
 
 gsm_m6312_err_code_add(&gsm_cmd.err_head,&ok);
 gsm_m6312_err_code_add(&gsm_cmd.err_head,&err);
 
 rc = gsm_m6312_at_cmd_excute(&gsm_cmd);
 if(rc == GSM_ERR_OK){
 rc = GSM_ERR_UNKNOW;
 temp = strstr(gsm_cmd.recv,"+CGREG: ");
 if(temp == NULL){
 goto err_exit;  
 }
 temp +=strlen("+CGREG: ");
 
 /*找到第1个，*/
 break_str = strstr(temp ,",");
 if(break_str == NULL){
 goto err_exit; 
 }
 temp = break_str + 1;
 
 if(strncmp(temp,"1",1) == 0){
 reg->status = GSM_M6312_STATUS_REGISTER;  
 }else {
 reg->status = GSM_M6312_STATUS_NO_REGISTER; 
 /*没有注册就没有位置信息*/ 
 rc = GSM_ERR_OK;
 goto err_exit;
 }
 
 /*找到第2个，*/
 break_str = strstr(temp,",");
 if(break_str == NULL){  
 goto err_exit;    
 }  
 temp = break_str + 1;
 /*找到第3个，*/
 break_str = strstr(temp,",");
 if(break_str == NULL || break_str - temp != 6){
 log_error("gsm location format err.\r\n");
 goto err_exit;
 }
 memcpy(reg->base.lac,temp,6);
 reg->base.lac[6] = '\0';
 /*找到第4个，*/
 temp = break_str + 1;
 break_str = strstr(temp,",");
 if(break_str == NULL || break_str - temp != 6){
 log_error("gsm location format err.\r\n");
 goto err_exit;
 }
 memcpy(reg->base.ci,temp,6);
 reg->base.ci[6] = '\0';
 rc = GSM_ERR_OK;
 }

err_exit: 
 gsm_m6312_print_err_info(gsm_cmd.send,gsm_cmd.recv,rc);
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
 gsm_m6312_err_code_t conn_ok,bind_ok,already,conn_fail,bind_fail,timeout,err;
 
 osMutexWait(gsm_mutex,osWaitForever);
 
 memset(&gsm_cmd,0,sizeof(gsm_cmd));
 snprintf(gsm_cmd.send,GSM_M6312_BUFFER_SIZE,"AT+IPSTART=%1d,%s,%s,%d\r\n",conn_id,protocol ==  GSM_M6312_NET_PROTOCOL_TCP ? "\"TCP\"" : "\"UDP\"",host,port);
 gsm_cmd.send_size = strlen(gsm_cmd.send);
 gsm_cmd.send_timeout = 10;
 gsm_cmd.recv_timeout = 20000;
 
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
 
 gsm_m6312_err_code_add(&gsm_cmd.err_head,&conn_ok);
 gsm_m6312_err_code_add(&gsm_cmd.err_head,&bind_ok);
 gsm_m6312_err_code_add(&gsm_cmd.err_head,&already);
 gsm_m6312_err_code_add(&gsm_cmd.err_head,&conn_fail);
 gsm_m6312_err_code_add(&gsm_cmd.err_head,&bind_fail);
 gsm_m6312_err_code_add(&gsm_cmd.err_head,&timeout);
 gsm_m6312_err_code_add(&gsm_cmd.err_head,&err);

 rc = gsm_m6312_at_cmd_excute(&gsm_cmd);
 if(rc == GSM_ERR_OK){
 rc = conn_id;
 }
 gsm_m6312_print_err_info(gsm_cmd.send,gsm_cmd.recv,rc);
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
 gsm_m6312_err_code_t ok,err;
 
 osMutexWait(gsm_mutex,osWaitForever);
 
 memset(&gsm_cmd,0,sizeof(gsm_cmd));
 snprintf(gsm_cmd.send,GSM_M6312_SEND_BUFFER_SIZE,"AT+IPCLOSE=%1d\r\n",conn_id);
 gsm_cmd.send_size = strlen(gsm_cmd.send);
 gsm_cmd.send_timeout = 10;
 gsm_cmd.recv_timeout = 5000;
 ok.str = "OK\r\n";
 ok.code = GSM_ERR_OK;
 ok.next = NULL;
 err.str = "ERROR";
 err.code = GSM_ERR_CMD_ERR;
 err.next = NULL;
 
 gsm_m6312_err_code_add(&gsm_cmd.err_head,&ok);
 gsm_m6312_err_code_add(&gsm_cmd.err_head,&err);
 
 rc = gsm_m6312_at_cmd_excute(&gsm_cmd);
 
 gsm_m6312_print_err_info(gsm_cmd.send,gsm_cmd.recv,rc);
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
 gsm_m6312_err_code_t ok,err;
 
 osMutexWait(gsm_mutex,osWaitForever);
 snprintf(conn_id_str,10,"con_id:%1d",conn_id);
 memset(&gsm_cmd,0,sizeof(gsm_cmd));
 strcpy(gsm_cmd.send,"AT+IPSTATE\r\n");
 gsm_cmd.send_size = strlen(gsm_cmd.send);
 gsm_cmd.send_timeout = 5;
 gsm_cmd.recv_timeout = 20000;
 ok.str = "OK\r\n";
 ok.code = GSM_ERR_OK;
 ok.next = NULL;
 err.str = "ERROR";
 err.code = GSM_ERR_CMD_ERR;
 err.next = NULL;
 
 gsm_m6312_err_code_add(&gsm_cmd.err_head,&ok);
 gsm_m6312_err_code_add(&gsm_cmd.err_head,&err);
 
 rc = gsm_m6312_at_cmd_excute(&gsm_cmd);
 if(rc == GSM_ERR_OK){
 status_str = strstr(gsm_cmd.recv,conn_id_str);
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
 
 gsm_m6312_print_err_info(gsm_cmd.send,gsm_cmd.recv,rc);
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
 gsm_m6312_err_code_t ok,fail,timeout,err;
 
 osMutexWait(gsm_mutex,osWaitForever);
 /*第1步 启动发送*/
 memset(&gsm_cmd,0,sizeof(gsm_cmd));
 snprintf(gsm_cmd.send,GSM_M6312_SEND_BUFFER_SIZE,"AT+IPSEND=%1d,%d\r\n",conn_id,size);
 gsm_cmd.send_size = strlen(gsm_cmd.send);
 gsm_cmd.send_timeout = 10;
 gsm_cmd.recv_timeout = 5000;
 ok.str = "> ";
 ok.code = GSM_ERR_OK;
 ok.next = NULL;
 err.str = "ERROR";
 err.code = GSM_ERR_CMD_ERR;
 err.next = NULL;
 
 gsm_m6312_err_code_add(&gsm_cmd.err_head,&ok);
 gsm_m6312_err_code_add(&gsm_cmd.err_head,&err);
 
 rc = gsm_m6312_at_cmd_excute(&gsm_cmd);
 /*第2步执行发送数据*/
 if( rc == GSM_ERR_OK){
 memset(&gsm_cmd,0,sizeof(gsm_cmd));
 send_size = GSM_M6312_SEND_BUFFER_SIZE > size ? size : GSM_M6312_SEND_BUFFER_SIZE;
 memcpy(gsm_cmd.send,data,send_size);
 gsm_cmd.send_size = send_size;
 gsm_cmd.send_timeout = send_size / 8 + 10;
 gsm_cmd.recv_timeout = 20000;
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
 
 gsm_m6312_err_code_add(&gsm_cmd.err_head,&ok);
 gsm_m6312_err_code_add(&gsm_cmd.err_head,&fail);
 gsm_m6312_err_code_add(&gsm_cmd.err_head,&timeout);
 gsm_m6312_err_code_add(&gsm_cmd.err_head,&err);
 
 rc = gsm_m6312_at_cmd_excute(&gsm_cmd);  
 if(rc == GSM_ERR_OK){
 rc = send_size;  
 }
 }
 
 gsm_m6312_print_err_info(gsm_cmd.send,gsm_cmd.recv,rc);
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
 gsm_m6312_err_code_t ok,err;
 
 osMutexWait(gsm_mutex,osWaitForever);
 
 memset(&gsm_cmd,0,sizeof(gsm_cmd));
 if(buffer == GSM_M6312_RECV_BUFFERE){
 strcpy(gsm_cmd.send,"AT+CMNDI=1,0\r\n");
 }else{
 strcpy(gsm_cmd.send,"AT+CMNDI=0,0\r\n");
 }
 gsm_cmd.send_size = strlen(gsm_cmd.send);
 gsm_cmd.send_timeout = 5;
 gsm_cmd.recv_timeout = 1000;
 ok.str = "OK\r\n";
 ok.code = GSM_ERR_OK;
 ok.next = NULL;
 err.str = "ERROR";
 err.code = GSM_ERR_CMD_ERR;
 err.next = NULL;
 
 gsm_m6312_err_code_add(&gsm_cmd.err_head,&ok);
 gsm_m6312_err_code_add(&gsm_cmd.err_head,&err);
 
 rc = gsm_m6312_at_cmd_excute(&gsm_cmd);
 
 gsm_m6312_print_err_info(gsm_cmd.send,gsm_cmd.recv,rc);
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
 char conn_id_str[10] = {0};
 char *buffer_size_str,*break_str;
 int buffer_size;
 gsm_m6312_err_code_t ok,err;
  
 if(size == 0 ){
 return 0;  
 }
 
 osMutexWait(gsm_mutex,osWaitForever);
 snprintf(conn_id_str,10,"+CMRD: %1d",conn_id); 
 /*第一步 查询缓存数据*/
 memset(&gsm_cmd,0,sizeof(gsm_cmd));
 strcpy(gsm_cmd.send,"AT+CMRD?\r\n");
 gsm_cmd.send_size = strlen(gsm_cmd.send);
 gsm_cmd.send_timeout = 5;
 gsm_cmd.recv_timeout = 1000;
 ok.str = "OK\r\n";
 ok.code = GSM_ERR_OK;
 ok.next = NULL;
 err.str = "ERROR";
 err.code = GSM_ERR_CMD_ERR;
 err.next = NULL;
 
 gsm_m6312_err_code_add(&gsm_cmd.err_head,&ok);
 gsm_m6312_err_code_add(&gsm_cmd.err_head,&err);
 
 rc = gsm_m6312_at_cmd_excute(&gsm_cmd);
 if(rc == GSM_ERR_OK){
 buffer_size_str = strstr(gsm_cmd.recv,conn_id_str);
 if(buffer_size_str){
 for(uint8_t i = 0;i < 2; i ++){
 break_str = strstr(buffer_size_str,",");
 if(break_str == NULL){
 rc = GSM_ERR_UNKNOW;
 goto err_exit;
 }
 buffer_size_str = break_str + 1;
 }
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
 memset(&gsm_cmd,0,sizeof(gsm_cmd));
 buffer_size = buffer_size > size ? size : buffer_size;
 snprintf(gsm_cmd.send,GSM_M6312_SEND_BUFFER_SIZE,"AT+CMRD=%1d,%d\r\n",conn_id,buffer_size); 
 gsm_cmd.send_size = strlen(gsm_cmd.send);
 gsm_cmd.send_timeout = 10;
 gsm_cmd.recv_timeout = 2000;
 ok.str = "OK\r\n";
 ok.code = GSM_ERR_OK;
 ok.next = NULL;
 err.str = "ERROR";
 err.code = GSM_ERR_CMD_ERR;
 err.next = NULL;
 
 gsm_m6312_err_code_add(&gsm_cmd.err_head,&ok);
 gsm_m6312_err_code_add(&gsm_cmd.err_head,&err);
 
 rc = gsm_m6312_at_cmd_excute(&gsm_cmd);
 if(rc == GSM_ERR_OK){
 memcpy(buffer,gsm_cmd.recv,buffer_size);
 rc = buffer_size;  
 }
 }
 
err_exit: 
 gsm_m6312_print_err_info(gsm_cmd.send,gsm_cmd.recv,rc);
 osMutexRelease(gsm_mutex);
 return rc;
}
