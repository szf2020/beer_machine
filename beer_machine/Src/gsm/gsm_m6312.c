#include "beer_machine.h"
#include "serial.h"
#include "at_cmd.h"
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




#define  GSM_M6312_ASSERT_NULL_POINTER(x)            \
if((x) == NULL){                                     \
return -1;                                           \
}



#define  GSM_6312_SMALL_REQUEST_SIZE             200
#define  GSM_6312_SMALL_RESPONSE_SIZE            200

#define  GSM_6312_MIDDLE_REQUEST_SIZE            500
#define  GSM_6312_MIDDLE_RESPONSE_SIZE           500

#define  GSM_6312_LARGE_REQUEST_SIZE             1600
#define  GSM_6312_LARGE_RESPONSE_SIZE            1600

#define  CHINA_MOBILE_OPERATOR_NUMERIC_NAME_STR              "\"46000\""
#define  CHINA_UNICOM_OPERATOR_NUMERIC_NAME_STR              "\"46001\""

#define  CHINA_MOBILE_OPERATOR_SHORT_NAME_STR                "\"CMCC\""
#define  CHINA_UNICOM_OPERATOR_SHORT_NAME_STR                "\"UNICOM\""

#define  CHINA_MOBILE_OPERATOR_LONG_NAME_STR                 "\"ChinaMobile\""
#define  CHINA_UNICOM_OPERATOR_LONG_NAME_STR                 "\"ChinaUnicom\""

#define  GSM_M6312_SIM_CARD_READY_STR                        "READY"
#define  GSM_M6312_NO_SIM_CARD_STR                           "NO SIM"

#define  GSM_M6312_IS_READY_AT_STR                            "AT"
#define  GSM_M6312_IS_READY_RESPONSE_OK_AT_STR                "OK\r\n"
#define  GSM_M6312_IS_READY_RESPONSE_ERR_AT_STR               "ERROR"
#define  GSM_M6312_IS_READY_TIMEOUT                           1000

#define  GSM_M6312_SET_ECHO_ON_AT_STR                         "ATE1"
#define  GSM_M6312_SET_ECHO_OFF_AT_STR                        "ATE0"
#define  GSM_M6312_SET_ECHO_RESPONSE_OK_AT_STR                "OK\r\n"
#define  GSM_M6312_SET_ECHO_RESPONSE_ERR_AT_STR               "ERROR"
#define  GSM_M6312_SET_ECHO_TIMEOUT                           1000

#define  GSM_M6312_IS_SIM_READY_AT_STR                        "AT+CPIN?"
#define  GSM_M6312_IS_SIM_READY_RESPONSE_OK_AT_STR            "OK\r\n"
#define  GSM_M6312_IS_SIM_READY_RESPONSE_ERR_AT_STR           "ERROR"
#define  GSM_M6312_IS_SIM_READY_TIMEOUT                       1000

#define  GSM_M6312_GET_IMEI_AT_STR                            "AT+CGSN"
#define  GSM_M6312_GET_IMEI_RESPONSE_OK_AT_STR                "OK\r\n"
#define  GSM_M6312_GET_IMEI_RESPONSE_ERR_AT_STR               "ERROR"
#define  GSM_M6312_GET_IMEI_TIMEOUT                           1000

#define  GSM_M6312_GET_SN_AT_STR                              "AT^SN"
#define  GSM_M6312_GET_SN_RESPONSE_OK_AT_STR                  "OK\r\n"
#define  GSM_M6312_GET_SN_RESPONSE_ERR_AT_STR                 "ERROR"
#define  GSM_M6312_GET_SN_TIMEOUT                             1000

#define  GSM_M6312_GPRS_SET_APN_AT_STR                        "AT+CGDCONT="
#define  GSM_M6312_GPRS_SET_APN_RESPONSE_OK_AT_STR            "OK\r\n"
#define  GSM_M6312_GPRS_SET_APN_RESPONSE_ERR_AT_STR           "ERROR"
#define  GSM_M6312_GPRS_SET_APN_TIMEOUT                       2000

#define  GSM_M6312_GPRS_APN_IP_AT_STR                         "\"IP\""
#define  GSM_M6312_GPRS_APN_CMNET_AT_STR                      "\"cmnet\""
#define  GSM_M6312_GPRS_APN_UNINET_AT_STR                     "\"uninet\""


#define  GSM_M6312_GPRS_GET_APN_AT_STR                        "AT+CGDCONT?"
#define  GSM_M6312_GPRS_GET_APN_RESPONSE_OK_AT_STR            "OK\r\n"
#define  GSM_M6312_GPRS_GET_APN_RESPONSE_ERR_AT_STR           "ERROR"
#define  GSM_M6312_GPRS_GET_APN_TIMEOUT                       1000

#define  GSM_M6312_GPRS_SET_ACTIVE_AT_STR                     "AT+CGACT="
#define  GSM_M6312_GPRS_SET_ACTIVE_RESPONSE_OK_AT_STR         "OK\r\n"
#define  GSM_M6312_GPRS_SET_ACTIVE_RESPONSE_ERR_AT_STR        "ERROR"
#define  GSM_M6312_GPRS_SET_ACTIVE_TIMEOUT                    2000

#define  GSM_M6312_GPRS_ACTIVE_AT_STR                         "1"
#define  GSM_M6312_GPRS_INACTIVE_AT_STR                       "0"
#define  GSM_M6312_GPRS_GET_ACTIVE_AT_STR                     "AT+CGACT?"
#define  GSM_M6312_GPRS_GET_ACTIVE_RESPONSE_OK_AT_STR         "OK\r\n"
#define  GSM_M6312_GPRS_GET_ACTIVE_RESPONSE_ERR_AT_STR        "ERROR"
#define  GSM_M6312_GPRS_GET_ACTIVE_TIMEOUT                    2000
#define  GSM_M6312_GPRS_SET_ATTACH_AT_STR                     "AT+CGATT="
#define  GSM_M6312_GPRS_SET_ATTACH_RESPONSE_OK_AT_STR         "OK\r\n"
#define  GSM_M6312_GPRS_SET_ATTACH_RESPONSE_ERR_AT_STR        "ERROR"
#define  GSM_M6312_GPRS_SET_ATTACH_TIMEOUT                     5000
#define  GSM_M6312_GPRS_ATTACH_AT_STR                         "1"
#define  GSM_M6312_GPRS_NOT_ATTACH_AT_STR                     "0"

#define  GSM_M6312_GPRS_GET_ATTACH_AT_STR                      "AT+CGATT?"
#define  GSM_M6312_GPRS_GET_ATTACH_RESPONSE_OK_AT_STR          "OK\r\n"
#define  GSM_M6312_GPRS_GET_ATTACH_RESPONSE_ERR_AT_STR         "ERROR"
#define  GSM_M6312_GPRS_GET_ATTACH_TIMEOUT                     1000

#define  GSM_M6312_GPRS_SET_CONNECT_MODE_AT_STR                "AT+CMMUX="
#define  GSM_M6312_GPRS_SET_CONNECT_MODE_RESPONSE_OK_AT_STR    "OK\r\n"
#define  GSM_M6312_GPRS_SET_CONNECT_MODE_RESPONSE_ERR_AT_STR   "ERROR"
#define  GSM_M6312_GPRS_SET_CONNECT_MODE_TIMEOUT                5000
#define  GSM_M6312_GPRS_MULTIPLE_MODE_AT_STR                   "1"
#define  GSM_M6312_GPRS_SINGLE_MODE_AT_STR                     "0"

#define  GSM_M6312_GPRS_GET_OPERATOR_AT_STR                    "AT+COPS?"
#define  GSM_M6312_GPRS_GET_OPERATOR_RESPONSE_OK_AT_STR        "OK\r\n"
#define  GSM_M6312_GPRS_GET_OPERATOR_RESPONSE_ERR_AT_STR       "ERROR"
#define  GSM_M6312_GPRS_GET_OPERATOR_TIMEOUT                   1000

#define  GSM_M6312_GPRS_SET_OPERATOR_AT_STR                    "AT+COPS="
#define  GSM_M6312_GPRS_SET_OPERATOR_RESPONSE_OK_AT_STR        "OK\r\n"
#define  GSM_M6312_GPRS_SET_OPERATOR_RESPONSE_ERR_AT_STR       "ERROR"
#define  GSM_M6312_GPRS_SET_OPERATOR_TIMEOUT                   20000


#define  GSM_M6312_GPRS_OPERATOR_AUTO_MODE_AT_STR              "0"
#define  GSM_M6312_GPRS_OPERATOR_MANUAL_MODE_AT_STR            "1"
#define  GSM_M6312_GPRS_OPERATOR_LOGOUT_MODE_AT_STR            "2"

#define  GSM_M6312_OPERATOR_FORMAT_LONG_NAME_AT_STR            "0"
#define  GSM_M6312_OPERATOR_FORMAT_SHORT_NAME_AT_STR           "1"
#define  GSM_M6312_OPERATOR_FORMAT_NUMERIC_NAME_AT_STR         "2"

#define  GSM_M6312_SET_SEND_PROMPT_AT_STR                    "AT+PROMPT="
#define  GSM_M6312_SET_SEND_PROMPT_RESPONSE_OK_AT_STR        "OK\r\n"
#define  GSM_M6312_SET_SEND_PROMPT_RESPONSE_ERR_AT_STR       "ERROR"
#define  GSM_M6312_SET_SEND_PROMPT_TIMEOUT                    1000

#define  GSM_M6312_SEND_PROMPT_AT_STR                         "1"
#define  GSM_M6312_NO_SEND_PROMPT_AT_STR                      "0"

#define  GSM_M6312_SET_TRANSPARENT_AT_STR                     "AT+CMMODE="
#define  GSM_M6312_SET_TRANSPARENT_RESPONSE_OK_AT_STR         "OK\r\n"
#define  GSM_M6312_SET_TRANSPARENT_RESPONSE_ERR_AT_STR        "ERROR"
#define  GSM_M6312_SET_TRANSPARENT_TIMEOUT                    1000
#define  GSM_M6312_TRANSPARENT_AT_STR                         "1"
#define  GSM_M6312_NO_TRANSPARENT_AT_STR                      "0"

#define  GSM_M6312_GPRS_CANCLE_REPORT_AT_STR                  "AT^CURC="
#define  GSM_M6312_GPRS_CANCLE_REPORT_RESPONSE_OK_AT_STR      "OK\r\n"
#define  GSM_M6312_GPRS_CANCLE_REPORT_RESPONSE_ERR_AT_STR     "ERROR"
#define  GSM_M6312_REPORT_ON_AT_STR                           "1"
#define  GSM_M6312_REPORT_OFF_AT_STR                          "0"
#define  GSM_M6312_GPRS_CANCLE_REPORT_TIMEOUT                 1000


#define  GSM_M6312_GPRS_OPEN_CLIENT_AT_STR                    "AT+IPSTART="
#define  GSM_M6312_GPRS_OPEN_CLIENT_RESPONSE_OK_AT_STR        "CONNECT OK\r\n"
#define  GSM_M6312_GPRS_OPEN_CLIENT_RESPONSE_ERR_AT_STR       "CONNECT FAIL"AT_CMD_ERR_BREAK_STR"ERROR"AT_CMD_ERR_BREAK_STR"ALREADY CONNECT"AT_CMD_ERR_BREAK_STR"COMMAND TIMEOUT"
#define  GSM_M6312_GPRS_OPEN_CLIENT_TIMEOUT                   20000
#define  GSM_M6312_NET_PROTOCOL_TCP_AT_STR                    "\"TCP\""
#define  GSM_M6312_NET_PROTOCOL_UDP_AT_STR                    "\"UDP\""

#define  GSM_M6312_GPRS_CLOSE_CLIENT_AT_STR                   "AT+IPCLOSE="
#define  GSM_M6312_GPRS_CLOSE_CLIENT_RESPONSE_OK_AT_STR       "OK\r\n"
#define  GSM_M6312_GPRS_CLOSE_CLIENT_RESPONSE_ERR_AT_STR      "ERROR"
#define  GSM_M6312_GPRS_CLOSE_CLIENT_TIMEOUT                  1000

#define  GSM_M6312_GPRS_GET_CONNECT_STATUS_AT_STR                "AT+CMSTATE"
#define  GSM_M6312_GPRS_GET_CONNECT_STATUS_RESPONSE_OK_AT_STR    "OK\r\n"
#define  GSM_M6312_GPRS_GET_CONNECT_STATUS_RESPONSE_ERR_AT_STR   "ERROR"
#define  GSM_M6312_GPRS_GET_CONNECT_STATUS_TIMEOUT               1000

#define  GSM_M6312_TCP_CONNECT_STATUS_INIT_AT_STR                "IP INITIAL"
#define  GSM_M6312_TCP_CONNECT_STATUS_CONNECT_OK_AT_STR          "CONNECT OK"
#define  GSM_M6312_TCP_CONNECT_STATUS_CLOSE_AT_STR               "IP CLOSE"


#define  GSM_M6312_SEND_AT_STR                                "AT+IPSEND="
#define  GSM_M6312_SEND_RESPONSE_OK_AT_STR                    "> "
#define  GSM_M6312_SEND_RESPONSE_ERR_AT_STR                   "ERROR"
#define  GSM_M6312_SEND_TIMEOUT                               10000

#define  GSM_M6312_SEND_START_AT_STR                          ""                        
#define  GSM_M6312_SEND_START_RESPONSE_OK_AT_STR              "SEND OK\r\n"
#define  GSM_M6312_SEND_START_RESPONSE_ERR_AT_STR             "ERROR"
#define  GSM_M6312_SEND_START_TIMEOUT                          15000

#define  GSM_M6312_CONFIG_RECV_BUFFER_AT_STR                   "AT+CMNDI="
#define  GSM_M6312_CONFIG_RECV_BUFFER_RESPONSE_OK_AT_STR       "OK\r\n"
#define  GSM_M6312_CONFIG_RECV_BUFFER_RESPONSE_ERR_AT_STR      "ERROR"
#define  GSM_M6312_CONFIG_RECV_BUFFER_TIMEOUT                  1000

#define  GSM_M6312_RECV_BUFFER_AT_STR                          "1"
#define  GSM_M6312_RECV_NO_BUFFER_AT_STR                       "0"

#define  GSM_M6312_GET_RECV_BUFFER_SIZE_AT_STR                 "AT+CMRD?"
#define  GSM_M6312_GET_RECV_BUFFER_SIZE_RESPONSE_OK_AT_STR     "OK\r\n"
#define  GSM_M6312_GET_RECV_BUFFER_SIZE_RESPONSE_ERR_AT_STR    "ERROR"
#define  GSM_M6312_GET_RECV_BUFFER_SIZE_TIMEOUT                1000

#define  GSM_M6312_RECV_AT_STR                                 "AT+CMRD="
#define  GSM_M6312_RECV_RESPONSE_OK_AT_STR                     "OK\r\n"
#define  GSM_M6312_RECV_RESPONSE_ERR_AT_STR                    "ERROR"
#define  GSM_M6312_RECV_TIMEOUT                                1000



#define  GSM_M6312_SIM_CARD_STATUS_HEADER_STR                  "+CPIN: "
#define  GSM_M6312_IMEI_HEADER_STR                             "+CGSN: "
#define  GSM_M6312_SN_HEADER_STR                               "M6312"
#define  GSM_M6312_APN_HEADER_STR                              "+CGDCONT: "
#define  GSM_M6312_ACTIVE_HEADER_STR                           "+CGACT: "
#define  GSM_M6312_ATTACH_HEADER_STR                           "+CGATT: "
#define  GSM_M6312_OPERATOR_HEADER_STR                         "+COPS: "
#define  GSM_M6312_CONNECT_STATUS_HEADER_STR                   "+CMSTATE: "
#define  GSM_M6312_RECV_BUFFER_SIZE_HEADER_STR                 "+CMRD: "


#define  GSM_M6312_PWR_ON_DELAY                4000
#define  GSM_M6312_PWR_OFF_DELAY               10000


/* 函数名：gsm_m6312_pwr_on
*  功能：  m6312 2g模块开机
*  参数：  无 
*  返回：  0：成功 其他：失败
*/
int gsm_m6312_pwr_on(void)
{
int rc = -1;
uint32_t timeout = 0;
if(bsp_get_gsm_pwr_status() == BSP_GSM_STATUS_PWR_ON){
return 0;
}

bsp_gsm_pwr_key_press();
while(timeout < GSM_M6312_PWR_ON_DELAY){
osDelay(100);
timeout +=100;
if(bsp_get_gsm_pwr_status() == BSP_GSM_STATUS_PWR_ON){
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
*  返回：  0：成功 其他：失败
*/

int gsm_m6312_pwr_off(void)
{
int rc = -1;
uint32_t timeout = 0;

if(bsp_get_gsm_pwr_status() == BSP_GSM_STATUS_PWR_OFF){
return 0;
}
bsp_gsm_pwr_key_press();
while(timeout < GSM_M6312_PWR_OFF_DELAY){
osDelay(100);
timeout +=100;
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
*  返回：  0：成功 其他：失败
*/
int gsm_m6312_serial_hal_init(void)
{
int rc;
rc = serial_create(&gsm_m6312_serial_handle,GSM_M6312_BUFFER_SIZE,GSM_M6312_BUFFER_SIZE);
if(rc != 0){
log_error("m6312 create serial hal err.\r\n");
return -1;
}
log_debug("m6312 create serial hal ok.\r\n");
rc = serial_register_hal_driver(gsm_m6312_serial_handle,&gsm_m6312_serial_driver);
if(rc != 0){
log_error("m6312 register serial hal driver err.\r\n");
return -1;
}
log_debug("m6312 register serial hal driver ok.\r\n");

rc = serial_open(gsm_m6312_serial_handle,GSM_M6312_SERIAL_PORT,GSM_M6312_SERIAL_BAUDRATES,GSM_M6312_SERIAL_DATA_BITS,GSM_M6312_SERIAL_STOP_BITS);
if(rc != 0){
log_error("m6312 open serial hal err.\r\n");
return -1;
}
osMutexDef(gsm_mutex);
gsm_mutex = osMutexCreate(osMutex(gsm_mutex));
if(gsm_mutex == NULL){
log_error("create gsm mutex err.\r\n");
return -1;
}
osMutexRelease(gsm_mutex);
log_debug("create gsm mutex ok.\r\n");

log_debug("m6312 open serial hal ok.\r\n");
return 0; 
}


/* 函数名：gsm_m6312_is_ready
*  功能：  gsm_m6312模块是否就绪
*  参数：  无 
*  返回：  0：ready 其他：失败
*/
int gsm_m6312_is_ready()
{
 int at_rc;
 int ready_rc;
 at_cmd_t cmd;  
 
 osMutexWait(gsm_mutex,osWaitForever);
 cmd.request = (char *)GSM_M6312_MALLOC(GSM_6312_SMALL_REQUEST_SIZE);
 cmd.response = (char *)GSM_M6312_MALLOC(GSM_6312_SMALL_RESPONSE_SIZE);
 
 if(cmd.request == NULL ||cmd.response == NULL){
 log_debug("malloc fail.\r\n");
 ready_rc = -1;
 goto err_handler;
 }
 memset(cmd.request,0,GSM_6312_SMALL_REQUEST_SIZE);
 memset(cmd.response,0,GSM_6312_SMALL_RESPONSE_SIZE );
 
 cmd.response_status_pos = AT_CMD_RESPONSE_STATUS_POS_IN_TAIL;
 cmd.response_max = GSM_6312_SMALL_RESPONSE_SIZE - 1;
 
 strcpy(cmd.request,GSM_M6312_IS_READY_AT_STR);
 strcat(cmd.request,AT_CMD_EOF_STR);
 
 cmd.request_size = strlen(cmd.request);
 cmd.response_ok = GSM_M6312_IS_READY_RESPONSE_OK_AT_STR; 
 cmd.response_err = GSM_M6312_IS_READY_RESPONSE_ERR_AT_STR; 
 cmd.timeout = GSM_M6312_IS_READY_TIMEOUT; 

 at_rc = at_cmd_execute(gsm_m6312_serial_handle,&cmd);
 if(at_rc != 0 ){
 ready_rc = -1;
 goto err_handler;
 }else{
 ready_rc =0;
 }
 
err_handler:
 if(ready_rc == 0){
 log_debug("gsm m6312 is <rdy> ok.\r\n");
 }else{
 log_error("gsm m6312 is <not rdy> err.\r\n");  
 }
 GSM_M6312_FREE(cmd.request);
 GSM_M6312_FREE(cmd.response);
 osMutexRelease(gsm_mutex);
 return ready_rc;
} 


/* 函数名：gsm_m6312_set_echo
*  功能：  设置是否回显输入的命令
*  参数：  echo回显设置 
*  返回：  0：成功 其他：失败
*/
int gsm_m6312_set_echo(gsm_m6312_echo_t echo)
{
 int at_rc;
 int echo_rc;
 at_cmd_t cmd;  
 
 osMutexWait(gsm_mutex,osWaitForever);
 cmd.request = (char *)GSM_M6312_MALLOC(GSM_6312_SMALL_REQUEST_SIZE);
 cmd.response = (char *)GSM_M6312_MALLOC(GSM_6312_SMALL_RESPONSE_SIZE);
 
 if(cmd.request == NULL ||cmd.response == NULL){
 log_debug("malloc fail.\r\n");
 echo_rc = -1;
 goto err_handler;
 }
 memset(cmd.request,0,GSM_6312_SMALL_REQUEST_SIZE);
 memset(cmd.response,0,GSM_6312_SMALL_RESPONSE_SIZE );
 
 cmd.response_status_pos = AT_CMD_RESPONSE_STATUS_POS_IN_TAIL;
 cmd.response_max = GSM_6312_SMALL_RESPONSE_SIZE - 1;
 if(echo == GSM_M6312_ECHO_ON){
 strcpy(cmd.request,GSM_M6312_SET_ECHO_ON_AT_STR);
 }else{
 strcpy(cmd.request,GSM_M6312_SET_ECHO_OFF_AT_STR);
 } 
 strcat(cmd.request,AT_CMD_EOF_STR);
 
 cmd.request_size = strlen(cmd.request);
 cmd.response_ok = GSM_M6312_SET_ECHO_RESPONSE_OK_AT_STR; 
 cmd.response_err = GSM_M6312_SET_ECHO_RESPONSE_ERR_AT_STR; 
 cmd.timeout = GSM_M6312_SET_ECHO_TIMEOUT; 

 at_rc = at_cmd_execute(gsm_m6312_serial_handle,&cmd);
 if(at_rc != 0 ){
 echo_rc = -1;
 goto err_handler;
 }else{
 echo_rc =0;
 }
 
err_handler:
 if(echo_rc == 0){
 log_debug("gsm m6312 set echo ok.\r\n");
 }else{
 log_error("gsm m6312 set echo err.\r\n");  
 }
 GSM_M6312_FREE(cmd.request);
 GSM_M6312_FREE(cmd.response);
 osMutexRelease(gsm_mutex);
 return echo_rc;
} 

/* 函数名：gsm_m6312_dump_sim_card_status
*  功能：  dump sim card rdy status
*  参数：  buffer源数据指针
*  参数：  status状态指针
*  返回：  0：ready 其他：失败
*/
static int gsm_m6312_dump_sim_card_status(const char *buffer,sim_card_status_t *status)
{
 char *pos;
 int size;
 char status_str[8];
 
 GSM_M6312_ASSERT_NULL_POINTER(buffer);
 GSM_M6312_ASSERT_NULL_POINTER(status);
 
 /*找到header位置*/
 pos = strstr(buffer,GSM_M6312_SIM_CARD_STATUS_HEADER_STR);
 if(pos == NULL){
 log_error("cant find sim card header.r\n");
 return -1;
 }
 buffer = pos + strlen(GSM_M6312_SIM_CARD_STATUS_HEADER_STR);
 /*找到EOF位置*/
 pos = strstr(buffer,AT_CMD_EOF_STR);
 GSM_M6312_ASSERT_NULL_POINTER(pos);
 
 size = pos - buffer > 7 ? 7 : pos - buffer;
 memcpy(status_str,buffer,size);
 status_str[size] = '\0';
 
 if(strcmp(status_str,GSM_M6312_SIM_CARD_READY_STR) == 0){
 *status = SIM_CARD_STATUS_READY;
 log_debug("sim card is <ready>.\r\n");
 }else if(strcmp(status_str,GSM_M6312_NO_SIM_CARD_STR) == 0){
 *status = SIM_CARD_STATUS_NO_SIM_CARD; 
 log_debug("sim card is <no sim card>.\r\n");
 }else{
 log_error("sim card is <%s> unknowed.\r\n",status_str);
 return -1;
 }
  
 return 0; 
}

/* 函数名：gsm_m6312_is_sim_card_ready
*  功能：  查询sim卡是否解锁就绪
*  参数：  status状态指针 
*  返回：  0：成功 其他：失败
*/
int gsm_m6312_is_sim_card_ready(sim_card_status_t *status)
{ 
 int at_rc;
 int sim_rc;
 at_cmd_t cmd;  
 
 osMutexWait(gsm_mutex,osWaitForever);
 cmd.request = (char *)GSM_M6312_MALLOC(GSM_6312_SMALL_REQUEST_SIZE);
 cmd.response = (char *)GSM_M6312_MALLOC(GSM_6312_SMALL_RESPONSE_SIZE);
 
 if(cmd.request == NULL ||cmd.response == NULL){
 log_debug("malloc fail.\r\n");
 sim_rc = -1;
 goto err_handler;
 }
 memset(cmd.request,0,GSM_6312_SMALL_REQUEST_SIZE);
 memset(cmd.response,0,GSM_6312_SMALL_RESPONSE_SIZE );
 
 cmd.response_status_pos = AT_CMD_RESPONSE_STATUS_POS_IN_TAIL;
 cmd.response_max = GSM_6312_SMALL_RESPONSE_SIZE - 1;
 strcpy(cmd.request,GSM_M6312_IS_SIM_READY_AT_STR);
 strcat(cmd.request,AT_CMD_EOF_STR);
 
 cmd.request_size = strlen(cmd.request);
 cmd.response_ok = GSM_M6312_IS_SIM_READY_RESPONSE_OK_AT_STR; 
 cmd.response_err = GSM_M6312_IS_SIM_READY_RESPONSE_ERR_AT_STR; 
 cmd.timeout = GSM_M6312_IS_SIM_READY_TIMEOUT; 

 at_rc = at_cmd_execute(gsm_m6312_serial_handle,&cmd);
 if(at_rc != 0 ){
 sim_rc = -1;
 goto err_handler;
 }
 
 sim_rc = gsm_m6312_dump_sim_card_status(cmd.response,status);
   
err_handler:
 if(sim_rc == 0){
 log_debug("gsm m6312 sim is rdy ok.\r\n");
 }else{
 log_error("gsm m6312 sim is rdy err.\r\n");  
 }
 GSM_M6312_FREE(cmd.request);
 GSM_M6312_FREE(cmd.response);
 osMutexRelease(gsm_mutex);
 return sim_rc;
} 


/* 函数名：gsm_m6312_dump_imei
*  功能：  dump设备IMEI串号
*  参数：  buffer数据源 
*  参数：  imei指针
*  返回：  0：ready 其他：失败
*/
static int gsm_m6312_dump_imei(const char *buffer,char *imei)
{
 char *pos;
 int size;
 
 GSM_M6312_ASSERT_NULL_POINTER(buffer); 
 GSM_M6312_ASSERT_NULL_POINTER(imei); 
 /*找到header*/
 pos = strstr(buffer,GSM_M6312_IMEI_HEADER_STR);
 if(pos == NULL){
 log_error("cant find imei header.\r\n");
 return -1;
 }
 buffer = pos + strlen(GSM_M6312_IMEI_HEADER_STR);
 /*找到结束EOF*/
 pos = strstr(buffer,AT_CMD_EOF_STR);
 size = pos - buffer;
 /*IMEI必须为15位*/
 if(size != GSM_M6312_IMEI_LEN){
 log_error("imei len:%d is invalid.\r\n",size);
 return -1; 
 }
 memcpy(imei,buffer,GSM_M6312_IMEI_LEN);
 imei[GSM_M6312_IMEI_LEN]='\0';
 log_debug("imei:<%s>ok.\r\n",imei);
 
 return 0;
}

/* 函数名：gsm_m6312_get_imei
*  功能：  获取设备IMEI串号
*  参数：  imei imei指针
*  返回：  0：ready 其他：失败
*/
int gsm_m6312_get_imei(char *imei)
{
 int at_rc;
 int imei_rc;
 at_cmd_t cmd;  
 
 osMutexWait(gsm_mutex,osWaitForever);
 cmd.request = (char *)GSM_M6312_MALLOC(GSM_6312_SMALL_REQUEST_SIZE);
 cmd.response = (char *)GSM_M6312_MALLOC(GSM_6312_SMALL_RESPONSE_SIZE);
 
 if(cmd.request == NULL ||cmd.response == NULL){
 log_debug("malloc fail.\r\n");
 imei_rc = -1;
 goto err_handler;
 }
 memset(cmd.request,0,GSM_6312_SMALL_REQUEST_SIZE);
 memset(cmd.response,0,GSM_6312_SMALL_RESPONSE_SIZE );
 
 cmd.response_status_pos = AT_CMD_RESPONSE_STATUS_POS_IN_TAIL;
 cmd.response_max = GSM_6312_SMALL_RESPONSE_SIZE - 1;
 strcpy(cmd.request,GSM_M6312_GET_IMEI_AT_STR);
 strcat(cmd.request,AT_CMD_EOF_STR);
 
 cmd.request_size = strlen(cmd.request);
 cmd.response_ok = GSM_M6312_GET_IMEI_RESPONSE_OK_AT_STR; 
 cmd.response_err = GSM_M6312_GET_IMEI_RESPONSE_ERR_AT_STR; 
 cmd.timeout = GSM_M6312_GET_IMEI_TIMEOUT;

 at_rc = at_cmd_execute(gsm_m6312_serial_handle,&cmd);
 if(at_rc != 0 ){
 imei_rc = -1;
 goto err_handler;
 }

 imei_rc = gsm_m6312_dump_imei(cmd.response,imei);
err_handler:
 if(imei_rc == 0){
 log_debug("gsm m6312 get imei ok.\r\n");
 }else{
 log_error("gsm m6312 get imei err.\r\n");  
 }
 GSM_M6312_FREE(cmd.request);
 GSM_M6312_FREE(cmd.response);
 osMutexRelease(gsm_mutex);
 return imei_rc;
} 


/* 函数名：gsm_m6312_dump_sn
*  功能：  dump设备SN号
*  参数：  buffer 数据源 
*  参数：  sn 指针
*  返回：  0：ready 其他：失败
*/
static int gsm_m6312_dump_sn(const char *buffer,char *sn)
{
 char *pos;
 int size;
 
 GSM_M6312_ASSERT_NULL_POINTER(buffer); 
 GSM_M6312_ASSERT_NULL_POINTER(sn); 
 /*找到开始M6312*/
 pos = strstr(buffer,GSM_M6312_SN_HEADER_STR);
 if(pos == NULL){
 log_error("cant find sn start tag.\r\n");
 return -1;
 }
 buffer = pos;
 /*找到结束EOF*/
 pos = strstr(buffer,AT_CMD_EOF_STR);
 size = pos - buffer;
 /*SN必须为20位*/
 if(size != GSM_M6312_SN_LEN){
 log_error("sn len:%d is invalid.\r\n",size);
 return -1; 
 }
 memcpy(sn,buffer,GSM_M6312_SN_LEN);
 sn[GSM_M6312_SN_LEN]='\0';
 log_debug("sn:<%s> ok.\r\n",sn);
 
 return 0;
}

/* 函数名：gsm_m6312_get_sn
*  功能：  获取设备SN号
*  参数：  sn sn指针
*  返回：  0：ready 其他：失败
*/
int gsm_m6312_get_sn(char *sn)
{
 int at_rc;
 int sn_rc;
 at_cmd_t cmd;  
 
 osMutexWait(gsm_mutex,osWaitForever);
 cmd.request = (char *)GSM_M6312_MALLOC(GSM_6312_SMALL_REQUEST_SIZE);
 cmd.response = (char *)GSM_M6312_MALLOC(GSM_6312_SMALL_RESPONSE_SIZE);
 
 if(cmd.request == NULL ||cmd.response == NULL){
 log_debug("malloc fail.\r\n");
 sn_rc = -1;
 goto err_handler;
 }
 memset(cmd.request,0,GSM_6312_SMALL_REQUEST_SIZE);
 memset(cmd.response,0,GSM_6312_SMALL_RESPONSE_SIZE );
 
 cmd.response_status_pos = AT_CMD_RESPONSE_STATUS_POS_IN_TAIL;
 cmd.response_max = GSM_6312_SMALL_RESPONSE_SIZE - 1;
 strcpy(cmd.request,GSM_M6312_GET_SN_AT_STR);
 strcat(cmd.request,AT_CMD_EOF_STR);
 
 cmd.request_size = strlen(cmd.request);
 cmd.response_ok = GSM_M6312_GET_SN_RESPONSE_OK_AT_STR; 
 cmd.response_err = GSM_M6312_GET_SN_RESPONSE_ERR_AT_STR; 
 cmd.timeout = GSM_M6312_GET_SN_TIMEOUT;

 at_rc = at_cmd_execute(gsm_m6312_serial_handle,&cmd);
 if(at_rc != 0 ){
 sn_rc = -1;
 goto err_handler;
 }

 sn_rc = gsm_m6312_dump_sn(cmd.response,sn);
err_handler:
 if(sn_rc == 0){
 log_debug("gsm m6312 get sn ok.\r\n");
 }else{
 log_error("gsm m6312 get sn err.\r\n");  
 }
 GSM_M6312_FREE(cmd.request);
 GSM_M6312_FREE(cmd.response);
 osMutexRelease(gsm_mutex);
 return sn_rc;
}

/* 函数名：gsm_m6312_gprs_set_apn
*  功能：  设置指定cid的gprs的APN
*  参数：  cid 指定的cid
*  参数：  apn 网络APN 
*  返回：  0：成功 其他：失败
*/
int gsm_m6312_gprs_set_apn(int cid,gsm_gprs_apn_t apn)
{
 int at_rc;
 int apn_rc;
 at_cmd_t cmd;  
 
 osMutexWait(gsm_mutex,osWaitForever);
 cmd.request = (char *)GSM_M6312_MALLOC(GSM_6312_SMALL_REQUEST_SIZE);
 cmd.response = (char *)GSM_M6312_MALLOC(GSM_6312_SMALL_RESPONSE_SIZE);
 
 if(cmd.request == NULL ||cmd.response == NULL){
 log_debug("malloc fail.\r\n");
 apn_rc = -1;
 goto err_handler;
 }
 memset(cmd.request,0,GSM_6312_SMALL_REQUEST_SIZE);
 memset(cmd.response,0,GSM_6312_SMALL_RESPONSE_SIZE );
 
 cmd.response_status_pos = AT_CMD_RESPONSE_STATUS_POS_IN_TAIL;
 cmd.response_max = GSM_6312_SMALL_RESPONSE_SIZE - 1;
 strcpy(cmd.request,GSM_M6312_GPRS_SET_APN_AT_STR);
 
 if(cid < 1 || cid > 11){
 cid = 1;
 log_warning("cid is use default value:%d.\r\n",cid);
 }

 snprintf(cmd.request + strlen(cmd.request),3,"%d",cid);
 strcat(cmd.request,AT_CMD_BREAK_STR);
 strcat(cmd.request,GSM_M6312_GPRS_APN_IP_AT_STR);
 strcat(cmd.request,AT_CMD_BREAK_STR);
 
 if(apn == GSM_GPRS_APN_CMNET){
 strcat(cmd.request,GSM_M6312_GPRS_APN_CMNET_AT_STR);
 }else if(apn == GSM_GPRS_APN_UNINET) {
 strcat(cmd.request,GSM_M6312_GPRS_APN_UNINET_AT_STR); 
 }else{
 apn_rc = -1;
 log_error("gprs apn:%d is not support.\r\n",apn); 
 goto err_handler;
 }
 strcat(cmd.request,AT_CMD_EOF_STR);
 cmd.request_size = strlen(cmd.request);
 cmd.response_ok = GSM_M6312_GPRS_SET_APN_RESPONSE_OK_AT_STR; 
 cmd.response_err = GSM_M6312_GPRS_SET_APN_RESPONSE_ERR_AT_STR; 
 cmd.timeout = GSM_M6312_GPRS_SET_APN_TIMEOUT; 

 at_rc = at_cmd_execute(gsm_m6312_serial_handle,&cmd);
 if(at_rc != 0 ){
 apn_rc = -1;
 goto err_handler;
 }else{
 apn_rc = 0;
 }
 
err_handler:
 if(apn_rc == 0){
 log_debug("gsm m6312 set gprs apn ok.\r\n");
 }else{
 log_error("gsm m6312 set gprs apn err.\r\n");  
 }
 GSM_M6312_FREE(cmd.request);
 GSM_M6312_FREE(cmd.response);
 osMutexRelease(gsm_mutex);
 return apn_rc;
} 

/* 函数名：m6312_2g_dump_apn
*  功能：  m6312 2g dump cid 和 apn代码
*  参数：  buffer 数据源指针 
*  参数：  cid 网络cid指针 
*  参数：  apn 网络APN指针 
*  返回：  0：成功 其他：失败
*/
static int gsm_m6312_dump_apn(const char *buffer,int cid,gsm_gprs_apn_t *apn)
{
 char *pos;
 int  size;
 int  cid_temp;
 char apn_str[9];
 
 GSM_M6312_ASSERT_NULL_POINTER(buffer);
 GSM_M6312_ASSERT_NULL_POINTER(apn);
 
 while(1){
 /*找到header位置*/
 pos = strstr(buffer,GSM_M6312_APN_HEADER_STR);
 if(pos == NULL){
 log_error("not dfind apn header.\r\n");
 return -1;
 }
 buffer = pos + strlen(GSM_M6312_APN_HEADER_STR);
 cid_temp = atoi(buffer);
 if(cid_temp != cid){
 /*结束这行查询*/ 
 pos = strstr(buffer,AT_CMD_EOF_STR);
 buffer = pos + strlen(AT_CMD_EOF_STR);
 continue;
 }
 /*找到第1个break位置*/
 pos = strstr(buffer,AT_CMD_BREAK_STR);
 GSM_M6312_ASSERT_NULL_POINTER(pos);
 buffer = pos + strlen(AT_CMD_BREAK_STR);
 /*找到第2个break位置*/
 pos = strstr(buffer,AT_CMD_BREAK_STR);
 GSM_M6312_ASSERT_NULL_POINTER(pos);
 buffer = pos + strlen(AT_CMD_BREAK_STR);
 
 /*找到第3个break位置*/
 pos = strstr(buffer,AT_CMD_BREAK_STR);
 GSM_M6312_ASSERT_NULL_POINTER(pos);
 
 size = pos - buffer > 8 ? 8 : pos - buffer;
 memcpy(apn_str,buffer,size);
 apn_str[size] = '\0';
 
 if(strcmp(apn_str,GSM_M6312_GPRS_APN_CMNET_AT_STR) == 0){
 *apn = GSM_GPRS_APN_CMNET;
 log_debug("cid:%d apn is <cmnet>.\r\n",cid);
 }else if(strcmp(apn_str,GSM_M6312_GPRS_APN_UNINET_AT_STR) == 0){
 *apn = GSM_GPRS_APN_UNINET; 
 log_debug("cid:%d apn is <uninet>.\r\n",cid);
 }else{
 *apn = GSM_GPRS_APN_UNKNOW;
 log_error("cid:%d apn is <unknow>.\r\n");
 }
 break;
 }
 return 0;
}


/* 函数名：gsm_m6312_gprs_get_apn
*  功能：  获取指定cid的gprs的apn
*  参数：  cid 指定cid 
*  参数：  apn 指针 
*  返回：  0：成功 其他：失败
*/
int gsm_m6312_gprs_get_apn(int cid,gsm_gprs_apn_t *apn)
{
 int at_rc;
 int apn_rc;
 at_cmd_t cmd;  
 
 osMutexWait(gsm_mutex,osWaitForever);
 cmd.request = (char *)GSM_M6312_MALLOC(GSM_6312_SMALL_REQUEST_SIZE);
 cmd.response = (char *)GSM_M6312_MALLOC(GSM_6312_SMALL_RESPONSE_SIZE);
 
 if(cmd.request == NULL ||cmd.response == NULL){
 log_debug("malloc fail.\r\n");
 apn_rc = -1;
 goto err_handler;
 }
 memset(cmd.request,0,GSM_6312_SMALL_REQUEST_SIZE);
 memset(cmd.response,0,GSM_6312_SMALL_RESPONSE_SIZE );
 
 cmd.response_status_pos = AT_CMD_RESPONSE_STATUS_POS_IN_TAIL;
 cmd.response_max = GSM_6312_SMALL_RESPONSE_SIZE - 1;
 strcpy(cmd.request,GSM_M6312_GPRS_GET_APN_AT_STR);
 strcat(cmd.request,AT_CMD_EOF_STR);
 
 
 cmd.request_size = strlen(cmd.request);
 cmd.response_ok = GSM_M6312_GPRS_GET_APN_RESPONSE_OK_AT_STR; 
 cmd.response_err = GSM_M6312_GPRS_GET_APN_RESPONSE_ERR_AT_STR; 
 cmd.timeout = GSM_M6312_GPRS_GET_APN_TIMEOUT; 

 at_rc = at_cmd_execute(gsm_m6312_serial_handle,&cmd);
 if(at_rc != 0 ){
 apn_rc = -1;
 goto err_handler;
 }
 
apn_rc = gsm_m6312_dump_apn(cmd.response,cid,apn);

err_handler:
 if(apn_rc == 0){
 log_debug("gsm m6312 get gprs cid apn ok.\r\n");
 }else{
 log_error("gsm m6312 get gprs cid apn err.\r\n");  
 }
 GSM_M6312_FREE(cmd.request);
 GSM_M6312_FREE(cmd.response);
 osMutexRelease(gsm_mutex);
 return apn_rc;
} 

/* 函数名：gsm_m6312_gprs_set_active_status
*  功能：  激活或者去激活指定cid的GPRS功能
*  参数：  cid gprs cid 
*  参数：  active 状态 
*  返回：  0：成功 其他：失败
*/
int gsm_m6312_gprs_set_active_status(int cid,gsm_gprs_active_status_t active)
{
 int at_rc;
 int active_rc;
 at_cmd_t cmd;  
 
 osMutexWait(gsm_mutex,osWaitForever);
 cmd.request = (char *)GSM_M6312_MALLOC(GSM_6312_SMALL_REQUEST_SIZE);
 cmd.response = (char *)GSM_M6312_MALLOC(GSM_6312_SMALL_RESPONSE_SIZE);
 
 if(cmd.request == NULL ||cmd.response == NULL){
 log_debug("malloc fail.\r\n");
 active_rc = -1;
 goto err_handler;
 }
 memset(cmd.request,0,GSM_6312_SMALL_REQUEST_SIZE);
 memset(cmd.response,0,GSM_6312_SMALL_RESPONSE_SIZE );
 
 cmd.response_status_pos = AT_CMD_RESPONSE_STATUS_POS_IN_TAIL;
 cmd.response_max = GSM_6312_SMALL_RESPONSE_SIZE - 1;
 strcpy(cmd.request,GSM_M6312_GPRS_SET_ACTIVE_AT_STR);
 if(active == GSM_GPRS_ACTIVE){
 strcat(cmd.request,GSM_M6312_GPRS_ACTIVE_AT_STR);
 }else{
 strcat(cmd.request,GSM_M6312_GPRS_INACTIVE_AT_STR);  
 } 
 strcat(cmd.request,AT_CMD_BREAK_STR);
 if(cid < 1 || cid > 11){
 log_error("set act cid:%d is invalid.\r\n",cid);
 goto err_handler;
 }
 snprintf(cmd.request + strlen(cmd.request),3,"%d",cid);
 strcat(cmd.request,AT_CMD_EOF_STR);
 
 cmd.request_size = strlen(cmd.request);
 cmd.response_ok = GSM_M6312_GPRS_SET_ACTIVE_RESPONSE_OK_AT_STR; 
 cmd.response_err = GSM_M6312_GPRS_SET_ACTIVE_RESPONSE_ERR_AT_STR; 
 cmd.timeout = GSM_M6312_GPRS_SET_ACTIVE_TIMEOUT; 

 at_rc = at_cmd_execute(gsm_m6312_serial_handle,&cmd);
 if(at_rc != 0 ){
 active_rc = -1;
 goto err_handler;
 }else{
 active_rc = 0;
 }
 
err_handler:
 if(active_rc == 0){
 log_debug("gsm m6312 gprs active ok.\r\n");
 }else{
 log_error("gsm m6312 gprs active err.\r\n");  
 }
 GSM_M6312_FREE(cmd.request);
 GSM_M6312_FREE(cmd.response);
 osMutexRelease(gsm_mutex);
 return active_rc;
} 

/* 函数名：gsm_m6312_gprs_dump_active_status
*  功能：  获取对应cid的GPRS激活状态
*  参数：  buffer 数据源指针 
*  参数：  cid    gprs的id 
*  参数：  active gprs激活状态指针 
*  返回：  0：成功 其他：失败
*/
static int gsm_m6312_gprs_dump_active_status(const char *buffer,int cid,gsm_gprs_active_status_t *active)
{
 char *pos;
 int  cid_temp;
 char status_str[2];
 int size;
 int loop_cnt = 12;
 
 GSM_M6312_ASSERT_NULL_POINTER(buffer);
 GSM_M6312_ASSERT_NULL_POINTER(active);
 
 /*保证最多循环次数*/
 while(loop_cnt --){
 /*找到header位置*/
 pos = strstr(buffer,GSM_M6312_ACTIVE_HEADER_STR);
 if(pos == NULL){
 /*默认是未激活*/
 *active = GSM_GPRS_INACTIVE;
 log_debug("can not find act header .default is inactive.\r\n"); 
 return 0;
 }
 
 buffer = pos + strlen(GSM_M6312_ACTIVE_HEADER_STR);
 cid_temp = atoi(buffer);
 if(cid_temp != cid){
 /*跳过这一行的查找*/
 pos = strstr(buffer,AT_CMD_EOF_STR);
 GSM_M6312_ASSERT_NULL_POINTER(pos);  
 buffer = pos +  strlen(AT_CMD_EOF_STR);
 continue;
 }
 /*找到第1个brak位置*/
 pos = strstr(buffer,AT_CMD_BREAK_STR);
 GSM_M6312_ASSERT_NULL_POINTER(pos);
 buffer = pos + strlen(AT_CMD_BREAK_STR); 
 /*找到EOF位置*/
 pos = strstr(buffer,AT_CMD_EOF_STR);
 GSM_M6312_ASSERT_NULL_POINTER(pos);
 /*计算长度*/
 size = pos - buffer > 1 ? 1: pos - buffer;
 memcpy(status_str,buffer,size);
 status_str[size] = '\0';

 if(strcmp(status_str,GSM_M6312_GPRS_ACTIVE_AT_STR) == 0){
 *active = GSM_GPRS_ACTIVE;
 log_debug("cid:%d is <ACTIVE>.\r\n",cid);
 }else if(strcmp(status_str,GSM_M6312_GPRS_INACTIVE_AT_STR) == 0){
 *active = GSM_GPRS_INACTIVE; 
 log_debug("cid:%d is <INACTIVE>.\r\n",cid);
 }else{
 log_error("cid:%d active is <UNKNOW>.\r\n");
 *active = GSM_GPRS_ACTIVE_UNKNOW;
 } 
 return 0;
 }
 return  -1;   
}


/* 函数名：gsm_m6312_gprs_get_active_status
*  功能：  获取对应cid的GPRS激活状态
*  参数：  cid   gprs的id 
*  参数：  active gprs状态指针 
*  返回：  0：成功 其他：失败
*/
int gsm_m6312_gprs_get_active_status(int cid,gsm_gprs_active_status_t *active)
{
 int at_rc;
 int active_rc;
 at_cmd_t cmd;  
 
 GSM_M6312_ASSERT_NULL_POINTER(active);
 if(cid < 1 || cid > 11){
 log_error("set act cid:%d is invalid.\r\n",cid);
 return -1;
 }
 osMutexWait(gsm_mutex,osWaitForever);
 cmd.request = (char *)GSM_M6312_MALLOC(GSM_6312_SMALL_REQUEST_SIZE);
 cmd.response = (char *)GSM_M6312_MALLOC(GSM_6312_SMALL_RESPONSE_SIZE);
 
 if(cmd.request == NULL ||cmd.response == NULL){
 log_debug("malloc fail.\r\n");
 active_rc = -1;
 goto err_handler;
 }
 memset(cmd.request,0,GSM_6312_SMALL_REQUEST_SIZE);
 memset(cmd.response,0,GSM_6312_SMALL_RESPONSE_SIZE );
 
 cmd.response_status_pos = AT_CMD_RESPONSE_STATUS_POS_IN_TAIL;
 cmd.response_max = GSM_6312_SMALL_RESPONSE_SIZE - 1;
 strcpy(cmd.request,GSM_M6312_GPRS_GET_ACTIVE_AT_STR);
 strcat(cmd.request,AT_CMD_EOF_STR);
 
 cmd.request_size = strlen(cmd.request);
 cmd.response_ok = GSM_M6312_GPRS_GET_ACTIVE_RESPONSE_OK_AT_STR; 
 cmd.response_err = GSM_M6312_GPRS_GET_ACTIVE_RESPONSE_ERR_AT_STR; 
 cmd.timeout = GSM_M6312_GPRS_GET_ACTIVE_TIMEOUT; 

 at_rc = at_cmd_execute(gsm_m6312_serial_handle,&cmd);
 if(at_rc != 0 ){
 active_rc = -1;
 goto err_handler;
 }
 active_rc = gsm_m6312_gprs_dump_active_status(cmd.response,cid,active);
err_handler:
 if(active_rc == 0){
 log_debug("gsm m6312 gprs get active ok.\r\n");
 }else{
 log_error("gsm m6312 gprs get active err.\r\n");  
 }
 GSM_M6312_FREE(cmd.request);
 GSM_M6312_FREE(cmd.response);
 osMutexRelease(gsm_mutex);
 return active_rc;
} 


/* 函数名：gsm_m6312_gprs_set_attach_status
*  功能：  设置GPRS网络附着状态
*  参数：  attach GPRS附着状态 
*  返回：  0：成功 其他：失败
*/
int gsm_m6312_gprs_set_attach_status(gsm_gprs_attach_status_t attach)
{
 int at_rc;
 int attach_rc;
 at_cmd_t cmd;  
 
 osMutexWait(gsm_mutex,osWaitForever);
 cmd.request = (char *)GSM_M6312_MALLOC(GSM_6312_SMALL_REQUEST_SIZE);
 cmd.response = (char *)GSM_M6312_MALLOC(GSM_6312_SMALL_RESPONSE_SIZE);
 
 if(cmd.request == NULL ||cmd.response == NULL){
 log_debug("malloc fail.\r\n");
 attach_rc = -1;
 goto err_handler;
 }
 memset(cmd.request,0,GSM_6312_SMALL_REQUEST_SIZE);
 memset(cmd.response,0,GSM_6312_SMALL_RESPONSE_SIZE );
 
 cmd.response_status_pos = AT_CMD_RESPONSE_STATUS_POS_IN_TAIL;
 cmd.response_max = GSM_6312_SMALL_RESPONSE_SIZE - 1;
 strcpy(cmd.request,GSM_M6312_GPRS_SET_ATTACH_AT_STR);
 if(attach == GSM_GPRS_ATTACH){
 strcat(cmd.request,GSM_M6312_GPRS_ATTACH_AT_STR);
 }else{
 strcat(cmd.request,GSM_M6312_GPRS_NOT_ATTACH_AT_STR); 
 }
 strcat(cmd.request,AT_CMD_EOF_STR);
 
 cmd.request_size = strlen(cmd.request);
 cmd.response_ok = GSM_M6312_GPRS_SET_ATTACH_RESPONSE_OK_AT_STR; 
 cmd.response_err = GSM_M6312_GPRS_SET_ATTACH_RESPONSE_ERR_AT_STR; 
 cmd.timeout = GSM_M6312_GPRS_SET_ATTACH_TIMEOUT; 

 at_rc = at_cmd_execute(gsm_m6312_serial_handle,&cmd);
 if(at_rc != 0 ){
 attach_rc = -1;
 goto err_handler;
 }else{
 attach_rc = 0;
 }

err_handler:
 if(attach_rc == 0){
 log_debug("gsm m6312 gprs set attach ok.\r\n");
 }else{
 log_error("gsm m6312 gprs set attach err.\r\n");  
 }
 GSM_M6312_FREE(cmd.request);
 GSM_M6312_FREE(cmd.response);
 osMutexRelease(gsm_mutex);
 return attach_rc;
} 


/* 函数名：gsm_m6312_gprs_dump_attach_status
*  功能：  dump GPRS附着状态指令
*  参数：  buffer 数据源指针指针 
*  参数：  attach GPRS附着状态指针 
*  返回：  0：成功 其他：失败
*/
static int gsm_m6312_gprs_dump_attach_status(const char *buffer,gsm_gprs_attach_status_t *attach)
{
 char *pos;
 char attach_str[2];
 int size;
 
 GSM_M6312_ASSERT_NULL_POINTER(buffer);
 GSM_M6312_ASSERT_NULL_POINTER(attach);
 
 /*找到header位置*/
 pos = strstr(buffer,GSM_M6312_ATTACH_HEADER_STR);
 if(pos == NULL){
 log_debug("can not find attach header.\r\n"); 
 return -1;
 }
 buffer = pos + strlen(GSM_M6312_ATTACH_HEADER_STR);
 /*找到EOF位置*/
 pos = strstr(buffer,AT_CMD_EOF_STR);
 GSM_M6312_ASSERT_NULL_POINTER(pos);
 
 size = pos - buffer > 1 ? 1 : pos - buffer;
 memcpy(attach_str,buffer,size);
 attach_str[size] = '\0';
 
 if(strcmp(attach_str,GSM_M6312_GPRS_ATTACH_AT_STR) == 0){
 *attach = GSM_GPRS_ATTACH;
 log_debug("GPRS is <ATTACHED>.\r\n"); 
 }else if(strcmp(attach_str,GSM_M6312_GPRS_NOT_ATTACH_AT_STR) == 0){ 
 *attach = GSM_GPRS_NOT_ATTACH;
 log_debug("GPRS is <NOT ATTACHED>.\r\n"); 
 }else{
 *attach = GSM_GPRS_ATTACH_UNKNOW;   
 log_error("attach str:%s <UNKNOW>.\r\n",attach_str);
 }

 return 0;
}


/* 函数名：gsm_m6312_gprs_get_attach_status
*  功能：  获取GPRS的附着状态
*  参数：  attach GPRS附着状态指针
*  返回：  0：成功 其他：失败
*/
int gsm_m6312_gprs_get_attach_status(gsm_gprs_attach_status_t *attach)
{
 int at_rc;
 int attach_rc;
 at_cmd_t cmd;  
 
 osMutexWait(gsm_mutex,osWaitForever); 
 cmd.request = (char *)GSM_M6312_MALLOC(GSM_6312_SMALL_REQUEST_SIZE);
 cmd.response = (char *)GSM_M6312_MALLOC(GSM_6312_SMALL_RESPONSE_SIZE);
 
 if(cmd.request == NULL ||cmd.response == NULL){
 log_debug("malloc fail.\r\n");
 attach_rc = -1;
 goto err_handler;
 }
 memset(cmd.request,0,GSM_6312_SMALL_REQUEST_SIZE);
 memset(cmd.response,0,GSM_6312_SMALL_RESPONSE_SIZE );
 
 cmd.response_status_pos = AT_CMD_RESPONSE_STATUS_POS_IN_TAIL;
 cmd.response_max = GSM_6312_SMALL_RESPONSE_SIZE - 1;
 strcpy(cmd.request,GSM_M6312_GPRS_GET_ATTACH_AT_STR);
 strcat(cmd.request,AT_CMD_EOF_STR);
 
 cmd.request_size = strlen(cmd.request);
 cmd.response_ok = GSM_M6312_GPRS_GET_ATTACH_RESPONSE_OK_AT_STR; 
 cmd.response_err = GSM_M6312_GPRS_GET_ATTACH_RESPONSE_ERR_AT_STR; 
 cmd.timeout = GSM_M6312_GPRS_GET_ATTACH_TIMEOUT; 

 at_rc = at_cmd_execute(gsm_m6312_serial_handle,&cmd);
 if(at_rc != 0 ){
 attach_rc = -1;
 goto err_handler;
 }
attach_rc = gsm_m6312_gprs_dump_attach_status(cmd.response,attach);

err_handler:
 if(attach_rc == 0){
 log_debug("gsm m6312 gprs get attach ok.\r\n");
 }else{
 log_error("gsm m6312 gprs get attach err.\r\n");  
 }
 GSM_M6312_FREE(cmd.request);
 GSM_M6312_FREE(cmd.response);
 osMutexRelease(gsm_mutex);
 return attach_rc;
} 

/* 函数名：gsm_m6312_gprs_set_connect_mode
*  功能：  设置连接模式指令
*  参数：  mode 单路或者多路 
*  返回：  0：成功 其他：失败
*/
int gsm_m6312_gprs_set_connect_mode(gsm_m6312_gprs_connect_mode_t mode)
{
 int at_rc;
 int mode_rc;
 at_cmd_t cmd;  
 
 osMutexWait(gsm_mutex,osWaitForever);
 cmd.request = (char *)GSM_M6312_MALLOC(GSM_6312_SMALL_REQUEST_SIZE);
 cmd.response = (char *)GSM_M6312_MALLOC(GSM_6312_SMALL_RESPONSE_SIZE);
 
 if(cmd.request == NULL ||cmd.response == NULL){
 log_debug("malloc fail.\r\n");
 mode_rc = -1;
 goto err_handler;
 }
 memset(cmd.request,0,GSM_6312_SMALL_REQUEST_SIZE);
 memset(cmd.response,0,GSM_6312_SMALL_RESPONSE_SIZE );
 
 cmd.response_status_pos = AT_CMD_RESPONSE_STATUS_POS_IN_TAIL;
 cmd.response_max = GSM_6312_SMALL_RESPONSE_SIZE - 1;
 strcpy(cmd.request,GSM_M6312_GPRS_SET_CONNECT_MODE_AT_STR);
 if(mode == GSM_M6312_GPRS_CONNECT_MODE_SINGLE){
 strcat(cmd.request,GSM_M6312_GPRS_SINGLE_MODE_AT_STR);
 }else{
 strcat(cmd.request,GSM_M6312_GPRS_MULTIPLE_MODE_AT_STR);
 }
 strcat(cmd.request,AT_CMD_EOF_STR);
 
 cmd.request_size = strlen(cmd.request);
 cmd.response_ok = GSM_M6312_GPRS_SET_CONNECT_MODE_RESPONSE_OK_AT_STR; 
 cmd.response_err = GSM_M6312_GPRS_SET_CONNECT_MODE_RESPONSE_ERR_AT_STR; 
 cmd.timeout = GSM_M6312_GPRS_SET_CONNECT_MODE_TIMEOUT; 

 at_rc = at_cmd_execute(gsm_m6312_serial_handle,&cmd);
 if(at_rc != 0 ){
 mode_rc = -1;
 goto err_handler;
 }else{
 mode_rc = 0;
 }

err_handler:
 if(mode_rc == 0){
 log_debug("gsm m6312 gprs set connection mode ok.\r\n");
 }else{
 log_error("gsm m6312 gprs set connection mode err.\r\n");  
 }
 GSM_M6312_FREE(cmd.request);
 GSM_M6312_FREE(cmd.response);
 osMutexRelease(gsm_mutex);
 return mode_rc;
} 


/* 函数名：gsm_m6312_gprs_dump_operator
*  功能：  dump 运营商
*  参数：  buffer 数据源指针 
*  参数：  gsm_operator 运营商指针 
*  返回：  0：成功 其他：失败
*/
static int gsm_m6312_gprs_dump_operator(const char *buffer, operator_name_t *gsm_operator)
{
 char *pos;
 int mode;
 int format;
 char operator_str[17];
 int size;
 
 GSM_M6312_ASSERT_NULL_POINTER(buffer);
 GSM_M6312_ASSERT_NULL_POINTER(gsm_operator);
 
 /*找到header位置*/
 pos = strstr(buffer,GSM_M6312_OPERATOR_HEADER_STR);
 if(pos == NULL){
 log_debug("can not find operator header.\r\n"); 
 return -1;
 }
 buffer = pos + strlen(GSM_M6312_OPERATOR_HEADER_STR);
 mode = atoi(buffer);
 if(mode != OPERATOR_AUTO_MODE   &&
    mode != OPERATOR_MANUAL_MODE &&
    mode != OPERATOR_LOGOUT_MODE){  
  log_error("operator mode:%d is err.\r\n",mode);   
  return -1;
 }
 
 /*找到第1个BREAK位置*/
 pos = strstr(buffer,AT_CMD_BREAK_STR);
 if(pos == NULL){
 *gsm_operator = OPERATOR_UNKNOW;
 log_debug("operator is <UNKNOW>.\r\n");
 return 0; 
 }
 buffer = pos + strlen(AT_CMD_BREAK_STR);
 format = atoi(buffer); 
 /*找到第2个BREAK位置*/
 pos = strstr(buffer,AT_CMD_BREAK_STR);
 GSM_M6312_ASSERT_NULL_POINTER(pos);
 buffer = pos + strlen(AT_CMD_BREAK_STR);
 /*找到EOF位置*/
 pos = strstr(buffer,AT_CMD_EOF_STR);
 GSM_M6312_ASSERT_NULL_POINTER(pos);
 size = pos - buffer > 16 ? 16 : pos - buffer;
 memcpy(operator_str,buffer,size);
 operator_str[size] = '\0';
 
if(format == GSM_M6312_OPERATOR_FORMAT_LONG_NAME   ||
   format == GSM_M6312_OPERATOR_FORMAT_SHORT_NAME  ||
   format == GSM_M6312_OPERATOR_FORMAT_NUMERIC_NAME){

   if(strcmp(operator_str,CHINA_MOBILE_OPERATOR_LONG_NAME_STR)   == 0 ||
      strcmp(operator_str,CHINA_MOBILE_OPERATOR_SHORT_NAME_STR)  == 0 ||
      strcmp(operator_str,CHINA_MOBILE_OPERATOR_NUMERIC_NAME_STR)== 0 ){
   *gsm_operator = OPERATOR_CHINA_MOBILE;
   log_debug("operator is <%s>.\r\n",operator_str);
   return 0;  
   }
   
   if(strcmp(operator_str,CHINA_UNICOM_OPERATOR_LONG_NAME_STR)   == 0 ||
      strcmp(operator_str,CHINA_UNICOM_OPERATOR_SHORT_NAME_STR)  == 0 ||
      strcmp(operator_str,CHINA_UNICOM_OPERATOR_NUMERIC_NAME_STR)== 0 ){
   *gsm_operator = OPERATOR_CHINA_UNICOM;
   log_debug("operator is <%s>.\r\n",operator_str);
   return 0;  
   } 
 }else{
   log_error("operator format:%d is err.\r\n",format);
 return -1;   
 }
 
return -1;
}

/* 函数名：gsm_m6312_get_operator
*  功能：  查询运营商
*  参数：  operator_name 运营商指针
*  返回：  0：成功 其他：失败
*/
int gsm_m6312_get_operator(operator_name_t *operator_name)
{
 int at_rc;
 int operator_rc;
 at_cmd_t cmd; 
 
 osMutexWait(gsm_mutex,osWaitForever);
 cmd.request = (char *)GSM_M6312_MALLOC(GSM_6312_SMALL_REQUEST_SIZE);
 cmd.response = (char *)GSM_M6312_MALLOC(GSM_6312_SMALL_RESPONSE_SIZE);
 
 if(cmd.request == NULL ||cmd.response == NULL){
 log_debug("malloc fail.\r\n");
 operator_rc = -1;
 goto err_handler;
 }
 memset(cmd.request,0,GSM_6312_SMALL_REQUEST_SIZE);
 memset(cmd.response,0,GSM_6312_SMALL_RESPONSE_SIZE );
 
 cmd.response_status_pos = AT_CMD_RESPONSE_STATUS_POS_IN_TAIL;
 cmd.response_max = GSM_6312_SMALL_RESPONSE_SIZE - 1;
 strcpy(cmd.request,GSM_M6312_GPRS_GET_OPERATOR_AT_STR);
 strcat(cmd.request,AT_CMD_EOF_STR);
 
 cmd.request_size = strlen(cmd.request);
 cmd.response_ok = GSM_M6312_GPRS_GET_OPERATOR_RESPONSE_OK_AT_STR; 
 cmd.response_err = GSM_M6312_GPRS_GET_OPERATOR_RESPONSE_ERR_AT_STR; 
 cmd.timeout = GSM_M6312_GPRS_GET_OPERATOR_TIMEOUT; 

 at_rc = at_cmd_execute(gsm_m6312_serial_handle,&cmd);
 if(at_rc != 0 ){
 operator_rc = -1;
 goto err_handler;
 }

 operator_rc = gsm_m6312_gprs_dump_operator(cmd.response,operator_name);
 
err_handler:
 if(operator_rc == 0){
 log_debug("gsm m6312 get operator ok.\r\n");
 }else{
 log_error("gsm m6312 get operator err.\r\n");  
 }
 GSM_M6312_FREE(cmd.request);
 GSM_M6312_FREE(cmd.response);
 osMutexRelease(gsm_mutex);
 return operator_rc;
} 


/* 函数名：gsm_m6312_set_auto_operator_format
*  功能：  设置自动运营商选择和运营商格式
*  参数：  operator_format 运营商格式
*  返回：  0：成功 其他：失败
*/
int gsm_m6312_set_auto_operator_format(gsm_m6312_operator_format_t operator_format)
{
 int at_rc;
 int operator_rc;
 at_cmd_t cmd;  
 
 osMutexWait(gsm_mutex,osWaitForever);
 cmd.request = (char *)GSM_M6312_MALLOC(GSM_6312_SMALL_REQUEST_SIZE);
 cmd.response = (char *)GSM_M6312_MALLOC(GSM_6312_SMALL_RESPONSE_SIZE);
 
 if(cmd.request == NULL ||cmd.response == NULL){
 log_debug("malloc fail.\r\n");
 operator_rc = -1;
 goto err_handler;
 }
 memset(cmd.request,0,GSM_6312_SMALL_REQUEST_SIZE);
 memset(cmd.response,0,GSM_6312_SMALL_RESPONSE_SIZE );
 
 cmd.response_status_pos = AT_CMD_RESPONSE_STATUS_POS_IN_TAIL;
 cmd.response_max = GSM_6312_SMALL_RESPONSE_SIZE - 1;
 strcpy(cmd.request,GSM_M6312_GPRS_SET_OPERATOR_AT_STR);
 strcat(cmd.request,GSM_M6312_GPRS_OPERATOR_AUTO_MODE_AT_STR);
 strcat(cmd.request,AT_CMD_BREAK_STR);
 if(operator_format == GSM_M6312_OPERATOR_FORMAT_LONG_NAME){
 strcat(cmd.request,GSM_M6312_OPERATOR_FORMAT_LONG_NAME_AT_STR);
 }else if(operator_format == GSM_M6312_OPERATOR_FORMAT_SHORT_NAME){
 strcat(cmd.request,GSM_M6312_OPERATOR_FORMAT_SHORT_NAME_AT_STR);
 }else{
 strcat(cmd.request,GSM_M6312_OPERATOR_FORMAT_NUMERIC_NAME_AT_STR);
 }
 strcat(cmd.request,AT_CMD_EOF_STR);
 
 cmd.request_size = strlen(cmd.request);
 cmd.response_ok = GSM_M6312_GPRS_SET_OPERATOR_RESPONSE_OK_AT_STR; 
 cmd.response_err = GSM_M6312_GPRS_SET_OPERATOR_RESPONSE_ERR_AT_STR; 
 cmd.timeout = GSM_M6312_GPRS_SET_OPERATOR_TIMEOUT; 

 at_rc = at_cmd_execute(gsm_m6312_serial_handle,&cmd);
 if(at_rc != 0 ){
 operator_rc = -1;
 goto err_handler;
 }else{
 operator_rc = 0;
 }
 
err_handler:
 if(operator_rc == 0){
 log_debug("gsm m6312 gprs set operator format ok.\r\n");
 }else{
 log_error("gsm m6312 gprs set operator format err.\r\n");  
 }
 GSM_M6312_FREE(cmd.request);
 GSM_M6312_FREE(cmd.response);
 osMutexRelease(gsm_mutex);
 return operator_rc;
} 


/* 函数名：gsm_m6312_set_set_prompt
*  功能：  设置发送提示
*  参数：  prompt 设置的发送提示
*  返回：  0：成功 其他：失败
*/
int gsm_m6312_set_set_prompt(gsm_m6312_send_prompt_t prompt)
{
 int at_rc;
 int prompt_rc;
 at_cmd_t cmd;  
 
 osMutexWait(gsm_mutex,osWaitForever);
 cmd.request = (char *)GSM_M6312_MALLOC(GSM_6312_SMALL_REQUEST_SIZE);
 cmd.response = (char *)GSM_M6312_MALLOC(GSM_6312_LARGE_RESPONSE_SIZE);
 
 if(cmd.request == NULL ||cmd.response == NULL){
 log_debug("malloc fail.\r\n");
 prompt_rc = -1;
 goto err_handler;
 }

 memset(cmd.request,0,GSM_6312_SMALL_REQUEST_SIZE);
 memset(cmd.response,0,GSM_6312_LARGE_RESPONSE_SIZE );
 
 cmd.response_status_pos = AT_CMD_RESPONSE_STATUS_POS_IN_TAIL;
 cmd.response_max = GSM_6312_LARGE_RESPONSE_SIZE - 1;
 strcpy(cmd.request,GSM_M6312_SET_SEND_PROMPT_AT_STR);
 if(prompt == GSM_M6312_SEND_PROMPT){
 strcat(cmd.request,GSM_M6312_SEND_PROMPT_AT_STR);
 }else{
 strcat(cmd.request,GSM_M6312_NO_SEND_PROMPT_AT_STR);  
 }
 strcat(cmd.request,AT_CMD_EOF_STR);
 cmd.request_size = strlen(cmd.request);
 cmd.response_ok = GSM_M6312_SET_SEND_PROMPT_RESPONSE_OK_AT_STR; 
 cmd.response_err = GSM_M6312_SET_SEND_PROMPT_RESPONSE_ERR_AT_STR; 
 cmd.timeout = GSM_M6312_SET_SEND_PROMPT_TIMEOUT; 

 at_rc = at_cmd_execute(gsm_m6312_serial_handle,&cmd);
 if(at_rc != 0 ){
 prompt_rc = -1;
 goto err_handler;
 }else{
 prompt_rc = 0;
 }

err_handler:
 if(prompt_rc == 0){
 log_debug("gsm m6312 gprs set send prompt ok.\r\n");
 }else{
 log_error("gsm m6312 gprs  set send prompt err.\r\n");  
 }
 GSM_M6312_FREE(cmd.request);
 GSM_M6312_FREE(cmd.response);
 osMutexRelease(gsm_mutex);
 return prompt_rc;
}


/* 函数名：gsm_m6312_set_transparent
*  功能：  设置透传模式
*  参数：  transparent 连接ID
*  返回：  0：成功 其他：失败
*/
int gsm_m6312_set_transparent(gsm_m6312_transparent_t transparent)
{
 int at_rc;
 int transparent_rc;
 at_cmd_t cmd;  
 
 osMutexWait(gsm_mutex,osWaitForever);
 cmd.request = (char *)GSM_M6312_MALLOC(GSM_6312_SMALL_REQUEST_SIZE);
 cmd.response = (char *)GSM_M6312_MALLOC(GSM_6312_SMALL_RESPONSE_SIZE);
 
 if(cmd.request == NULL ||cmd.response == NULL){
 log_debug("malloc fail.\r\n");
 transparent_rc = -1;
 goto err_handler;
 }

 memset(cmd.request,0,GSM_6312_SMALL_REQUEST_SIZE);
 memset(cmd.response,0,GSM_6312_SMALL_RESPONSE_SIZE );
 
 cmd.response_status_pos = AT_CMD_RESPONSE_STATUS_POS_IN_TAIL;
 cmd.response_max = GSM_6312_SMALL_RESPONSE_SIZE - 1;
 strcpy(cmd.request,GSM_M6312_SET_TRANSPARENT_AT_STR);
 if(transparent == GSM_M6312_TRANPARENT){
 strcat(cmd.request,GSM_M6312_TRANSPARENT_AT_STR);
 }else{
 strcat(cmd.request,GSM_M6312_NO_TRANSPARENT_AT_STR);  
 }
 strcat(cmd.request,AT_CMD_EOF_STR);
 cmd.request_size = strlen(cmd.request);
 cmd.response_ok = GSM_M6312_SET_TRANSPARENT_RESPONSE_OK_AT_STR; 
 cmd.response_err = GSM_M6312_SET_TRANSPARENT_RESPONSE_ERR_AT_STR; 
 cmd.timeout = GSM_M6312_SET_TRANSPARENT_TIMEOUT; 

 at_rc = at_cmd_execute(gsm_m6312_serial_handle,&cmd);
 if(at_rc != 0 ){
 transparent_rc = -1;
 goto err_handler;
 }else{
 transparent_rc = 0;
 }

err_handler:
 if(transparent_rc == 0){
 log_debug("gsm m6312 gprs set transparent mode ok.\r\n");
 }else{
 log_error("gsm m6312 gprs set transparent mode err.\r\n");  
 }
 GSM_M6312_FREE(cmd.request);
 GSM_M6312_FREE(cmd.response);
 osMutexRelease(gsm_mutex);
 return transparent_rc;
}

/* 函数名：gsm_m6312_set_report
*  功能：  关闭信息上报
*  参数：  无
*  返回：  0：成功 其他：失败
*/
int gsm_m6312_set_report(gsm_m6312_report_t report)
{
 int at_rc;
 int cancle_rc;
 at_cmd_t cmd;  
 
 osMutexWait(gsm_mutex,osWaitForever);
 cmd.request = (char *)GSM_M6312_MALLOC(GSM_6312_SMALL_REQUEST_SIZE);
 cmd.response = (char *)GSM_M6312_MALLOC(GSM_6312_SMALL_RESPONSE_SIZE);
 
 if(cmd.request == NULL ||cmd.response == NULL){
 log_debug("malloc fail.\r\n");
 cancle_rc = -1;
 goto err_handler;
 }
 memset(cmd.request,0,GSM_6312_SMALL_REQUEST_SIZE);
 memset(cmd.response,0,GSM_6312_SMALL_RESPONSE_SIZE );
 
 cmd.response_status_pos = AT_CMD_RESPONSE_STATUS_POS_IN_TAIL;
 cmd.response_max = GSM_6312_SMALL_RESPONSE_SIZE - 1;
 strcpy(cmd.request,GSM_M6312_GPRS_CANCLE_REPORT_AT_STR);
 if(report == GSM_M6312_REPORT_ON){
 strcpy(cmd.request,GSM_M6312_REPORT_ON_AT_STR);
 }else{
 strcpy(cmd.request,GSM_M6312_REPORT_OFF_AT_STR);
 } 
 strcat(cmd.request,AT_CMD_EOF_STR);

 cmd.request_size = strlen(cmd.request);
 cmd.response_ok = GSM_M6312_GPRS_CANCLE_REPORT_RESPONSE_OK_AT_STR; 
 cmd.response_err = GSM_M6312_GPRS_CANCLE_REPORT_RESPONSE_ERR_AT_STR; 
 cmd.timeout = GSM_M6312_GPRS_CANCLE_REPORT_TIMEOUT; 

 at_rc = at_cmd_execute(gsm_m6312_serial_handle,&cmd);
 if(at_rc != 0 ){
 cancle_rc = -1;
 goto err_handler;
 }else{
 cancle_rc = 0;
 }
 
err_handler:
 if(cancle_rc == 0){
 log_debug("gsm m6312 gprs cancle report ok.\r\n");
 }else{
 log_error("gsm m6312 gprs cancle report err.\r\n");  
 }
 GSM_M6312_FREE(cmd.request);
 GSM_M6312_FREE(cmd.response);
 osMutexRelease(gsm_mutex);
 return cancle_rc;
}

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
 int at_rc;
 int open_rc;
 at_cmd_t cmd;  
 
 if(conn_id < 0 || conn_id > 4){
 log_error("conn id:%d is invalid.\r\n",conn_id);
 return -1;
 }
 GSM_M6312_ASSERT_NULL_POINTER(host);
 osMutexWait(gsm_mutex,osWaitForever);
 cmd.request = (char *)GSM_M6312_MALLOC(GSM_6312_SMALL_REQUEST_SIZE);
 cmd.response = (char *)GSM_M6312_MALLOC(GSM_6312_SMALL_RESPONSE_SIZE);
 
 if(cmd.request == NULL ||cmd.response == NULL){
 log_debug("malloc fail.\r\n");
 open_rc = -1;
 goto err_handler;
 }
 memset(cmd.request,0,GSM_6312_SMALL_REQUEST_SIZE);
 memset(cmd.response,0,GSM_6312_SMALL_RESPONSE_SIZE );
 
 cmd.response_status_pos = AT_CMD_RESPONSE_STATUS_POS_IN_TAIL;
 cmd.response_max = GSM_6312_SMALL_RESPONSE_SIZE - 1;
 strcpy(cmd.request,GSM_M6312_GPRS_OPEN_CLIENT_AT_STR);

 
 snprintf(cmd.request + strlen(cmd.request),2,"%d",conn_id);
 strcat(cmd.request,AT_CMD_BREAK_STR);
 if(protocol == GSM_M6312_NET_PROTOCOL_TCP){
 strcat(cmd.request,GSM_M6312_NET_PROTOCOL_TCP_AT_STR);
 }else{
 strcat(cmd.request,GSM_M6312_NET_PROTOCOL_UDP_AT_STR);
 }
 strcat(cmd.request,AT_CMD_BREAK_STR);
 strcat(cmd.request,host);
 strcat(cmd.request,AT_CMD_BREAK_STR);
 snprintf(cmd.request + strlen(cmd.request),6,"%d",port);
 strcat(cmd.request,AT_CMD_EOF_STR);

 cmd.request_size = strlen(cmd.request);
 cmd.response_ok = GSM_M6312_GPRS_OPEN_CLIENT_RESPONSE_OK_AT_STR; 
 cmd.response_err = GSM_M6312_GPRS_OPEN_CLIENT_RESPONSE_ERR_AT_STR; 
 cmd.timeout = GSM_M6312_GPRS_OPEN_CLIENT_TIMEOUT; 

 at_rc = at_cmd_execute(gsm_m6312_serial_handle,&cmd);
 if(at_rc != 0 ){
 open_rc = -1;
 goto err_handler;
 }else{
 open_rc = 0;
 }
err_handler:
 if(open_rc == 0){
 log_debug("gsm m6312 gprs open client ok.\r\n");
 }else{
 log_error("gsm m6312 gprs open client err.\r\n");  
 }
 GSM_M6312_FREE(cmd.request);
 GSM_M6312_FREE(cmd.response);
 osMutexRelease(gsm_mutex);
 return open_rc;
}


/* 函数名：gsm_m6312_close_client
*  功能：  关闭连接
*  参数：  conn_id 连接ID
*  返回：  0：成功 其他：失败
*/
int gsm_m6312_close_client(int conn_id)
{
 int at_rc;
 int close_rc;
 at_cmd_t cmd;  
 
 if(conn_id < 0 || conn_id > 4){
 log_error("conn id:%d is invalid.\r\n",conn_id);
 return -1;
 }
 osMutexWait(gsm_mutex,osWaitForever);
 cmd.request = (char *)GSM_M6312_MALLOC(GSM_6312_SMALL_REQUEST_SIZE);
 cmd.response = (char *)GSM_M6312_MALLOC(GSM_6312_SMALL_RESPONSE_SIZE);
 
 if(cmd.request == NULL ||cmd.response == NULL){
 log_debug("malloc fail.\r\n");
 close_rc = -1;
 goto err_handler;
 }
 memset(cmd.request,0,GSM_6312_SMALL_REQUEST_SIZE);
 memset(cmd.response,0,GSM_6312_SMALL_RESPONSE_SIZE );
 
 cmd.response_status_pos = AT_CMD_RESPONSE_STATUS_POS_IN_TAIL;
 cmd.response_max = GSM_6312_SMALL_RESPONSE_SIZE - 1;
 strcpy(cmd.request,GSM_M6312_GPRS_CLOSE_CLIENT_AT_STR);

 snprintf(cmd.request + strlen(cmd.request),2,"%d",conn_id);
 strcat(cmd.request,AT_CMD_EOF_STR);

 cmd.request_size = strlen(cmd.request);
 cmd.response_ok = GSM_M6312_GPRS_CLOSE_CLIENT_RESPONSE_OK_AT_STR; 
 cmd.response_err = GSM_M6312_GPRS_CLOSE_CLIENT_RESPONSE_ERR_AT_STR; 
 cmd.timeout = GSM_M6312_GPRS_CLOSE_CLIENT_TIMEOUT; 

 at_rc = at_cmd_execute(gsm_m6312_serial_handle,&cmd);
 if(at_rc != 0 ){
 close_rc = -1;
 goto err_handler;
 }else{
 close_rc = 0;
 }
 
err_handler:
 if(close_rc == 0){
 log_debug("gsm m6312 gprs close client ok.\r\n");
 }else{
 log_error("gsm m6312 gprs close client err.\r\n");  
 }
 GSM_M6312_FREE(cmd.request);
 GSM_M6312_FREE(cmd.response);
 osMutexRelease(gsm_mutex);
 return close_rc;
}

/* 函数名：gsm_m6312_dump_connect_status
*  功能：  获取连接的状态
*  参数：  buffer数据源
*  参数：  conn_id 连接ID
*  参数：  status连接状态指针
*  返回：  0：成功 其他：失败
*/
static int gsm_m6312_dump_connect_status(const char *buffer,const int conn_id,tcp_connect_status_t *status)
{
char *pos;
int conn_id_temp;
int size;
char status_str[15];
int loop_cnt = 5;
int break_str_len;

GSM_M6312_ASSERT_NULL_POINTER(buffer);
break_str_len = strlen(AT_CMD_BREAK_STR);

 while(loop_cnt -- ){
/*找到header位置*/
 pos = strstr(buffer,GSM_M6312_CONNECT_STATUS_HEADER_STR);
 if(pos == NULL){
 log_error("can not find conn status header.\r\n");
 return -1;
 }
 /*找到conn id位置*/
 buffer = pos + strlen(GSM_M6312_CONNECT_STATUS_HEADER_STR);
 conn_id_temp = atoi(buffer);
 if(conn_id_temp != conn_id){
 /*结束这一行的查找*/
 pos = strstr(buffer,AT_CMD_EOF_STR);
 buffer = pos + strlen(AT_CMD_EOF_STR);
 continue;
 }
 /*找到第1个BREAK位置*/
 pos = strstr(buffer,AT_CMD_BREAK_STR);
 GSM_M6312_ASSERT_NULL_POINTER(pos);
 /*找到协议位置*/
 buffer = pos + break_str_len;
 /*找到第2个BREAK位置*/
 pos = strstr(buffer,AT_CMD_BREAK_STR);
 GSM_M6312_ASSERT_NULL_POINTER(pos);
 /*找到IP位置*/
 buffer = pos + break_str_len;
 /*找到第3个BREAK位置*/
 pos = strstr(buffer,AT_CMD_BREAK_STR);
 GSM_M6312_ASSERT_NULL_POINTER(pos);
 /*找到端口位置*/
 buffer = pos + break_str_len;
 /*找到第4个BREAK位置*/
 pos = strstr(buffer,AT_CMD_BREAK_STR);
 GSM_M6312_ASSERT_NULL_POINTER(pos);
 /*找到连接状态位置*/
 buffer = pos + break_str_len;
 /*找到行结束位置*/
 pos =strstr(buffer,AT_CMD_EOF_STR);
 /*计算连接状态长度*/
 size = pos - buffer > 14 ? 14 :pos - buffer;
 memcpy(status_str,buffer,size);
 status_str[size] = '\0';
 
 if(strcmp(status_str,GSM_M6312_TCP_CONNECT_STATUS_INIT_AT_STR) == 0){
 *status = TCP_CONNECT_STATUS_INIT;
 log_debug("gsm tcp conn_id:%d status is <INIT>.\r\n",conn_id );
 }else if(strcmp(status_str,GSM_M6312_TCP_CONNECT_STATUS_CONNECT_OK_AT_STR) == 0){
 *status = TCP_CONNECT_STATUS_CONNECTED;
 log_debug("gsm tcp conn_id:%d status is <CONNECTED>.\r\n",conn_id);
 }else if(strcmp(status_str,GSM_M6312_TCP_CONNECT_STATUS_CLOSE_AT_STR) == 0){ 
 *status = TCP_CONNECT_STATUS_DISCONNECTED;
 log_debug("gsm tcp conn_id:%d status is <DISCONNECTED>.\r\n",conn_id);
 }else{
 log_error("gsm tcp conn_id:%d status:<%s> is unknow.\r\n",conn_id);
 return -1;
 }
 break;
}
 log_debug("dump gsm tcp connect status ok.\r\n" );
 return 0;
}
/* 函数名：gsm_m6312_get_connect_status
*  功能：  获取连接的状态
*  参数：  conn_id 连接ID
*  参数：  status连接状态指针
*  返回：  0：成功 其他：失败
*/
int gsm_m6312_get_connect_status(const int conn_id,tcp_connect_status_t *status)
{
 int at_rc;
 int status_rc;
 at_cmd_t cmd;  
 
 if(conn_id < 0 || conn_id > 4){
 log_error("conn id:%d is invalid.\r\n",conn_id);
 return -1;
 }
 osMutexWait(gsm_mutex,osWaitForever);
 cmd.request = (char *)GSM_M6312_MALLOC(GSM_6312_SMALL_REQUEST_SIZE);
 cmd.response = (char *)GSM_M6312_MALLOC(GSM_6312_MIDDLE_RESPONSE_SIZE);
 
 if(cmd.request == NULL ||cmd.response == NULL){
 log_debug("malloc fail.\r\n");
 status_rc = -1;
 goto err_handler;
 }
 memset(cmd.request,0,GSM_6312_SMALL_REQUEST_SIZE);
 memset(cmd.response,0,GSM_6312_MIDDLE_RESPONSE_SIZE );
 
 cmd.response_status_pos = AT_CMD_RESPONSE_STATUS_POS_IN_TAIL;
 cmd.response_max = GSM_6312_MIDDLE_RESPONSE_SIZE - 1;
 strcpy(cmd.request,GSM_M6312_GPRS_GET_CONNECT_STATUS_AT_STR);
 strcat(cmd.request,AT_CMD_EOF_STR);

 cmd.request_size = strlen(cmd.request);
 cmd.response_ok = GSM_M6312_GPRS_GET_CONNECT_STATUS_RESPONSE_OK_AT_STR; 
 cmd.response_err = GSM_M6312_GPRS_GET_CONNECT_STATUS_RESPONSE_ERR_AT_STR; 
 cmd.timeout = GSM_M6312_GPRS_GET_CONNECT_STATUS_TIMEOUT; 

 at_rc = at_cmd_execute(gsm_m6312_serial_handle,&cmd);
 if(at_rc != 0 ){
 status_rc = -1;
 goto err_handler;
 }
 
 status_rc = gsm_m6312_dump_connect_status(cmd.response,conn_id,status);
 
err_handler:
 if(status_rc == 0){
 log_debug("gsm m6312 gprs get conn status ok.\r\n");
 }else{
 log_error("gsm m6312 gprs get conn status err.\r\n");  
 }
 GSM_M6312_FREE(cmd.request);
 GSM_M6312_FREE(cmd.response);
 osMutexRelease(gsm_mutex);
 return status_rc;
}

/* 函数名：gsm_m6312_send
*  功能：  发送数据
*  参数：  conn_id 连接ID
*  参数：  data 数据buffer
*  参数：  size 发送的数据数量
*  返回：  0：成功 其他：失败
*/
int gsm_m6312_send(int conn_id,const char *data,const int size)
{
 int at_rc;
 int send_rc;
 at_cmd_t cmd;  
 
 if(conn_id < 0 || conn_id > 4){
 log_error("conn id:%d is invalid.\r\n",conn_id);
 return -1;
 }
 osMutexWait(gsm_mutex,osWaitForever);
 cmd.request = (char *)GSM_M6312_MALLOC(GSM_6312_LARGE_REQUEST_SIZE);
 cmd.response = (char *)GSM_M6312_MALLOC(GSM_6312_SMALL_RESPONSE_SIZE);
 
 if(cmd.request == NULL ||cmd.response == NULL){
 log_debug("malloc fail.\r\n");
 send_rc = -1;
 goto err_handler;
 } 
/*第一步启动发送*/
 memset(cmd.request,0,GSM_6312_LARGE_REQUEST_SIZE);
 memset(cmd.response,0,GSM_6312_SMALL_RESPONSE_SIZE );
 
 cmd.response_status_pos = AT_CMD_RESPONSE_STATUS_POS_IN_TAIL;
 cmd.response_max = GSM_6312_SMALL_RESPONSE_SIZE - 1;
 strcpy(cmd.request,GSM_M6312_SEND_AT_STR);

 snprintf(cmd.request + strlen(cmd.request),2,"%d",conn_id);
 strcat(cmd.request,AT_CMD_BREAK_STR);
 if(size > GSM_6312_LARGE_REQUEST_SIZE - 1 || size > 1460){
 log_error("send size:%d is too large.\r\n",size);
 goto err_handler;
 }
 snprintf(cmd.request + strlen(cmd.request),5,"%d",size);
 strcat(cmd.request,AT_CMD_EOF_STR);
 
 cmd.request_size = strlen(cmd.request);
 cmd.response_ok = GSM_M6312_SEND_RESPONSE_OK_AT_STR; 
 cmd.response_err = GSM_M6312_SEND_RESPONSE_ERR_AT_STR; 
 cmd.timeout = GSM_M6312_SEND_TIMEOUT; 

 at_rc = at_cmd_execute(gsm_m6312_serial_handle,&cmd);
 if(at_rc != 0 ){
 send_rc = -1;
 goto err_handler;
 }else{
 send_rc = 0;
 }
 /*第二步输入发送的数据*/
 memset(cmd.request,0,GSM_6312_LARGE_REQUEST_SIZE);
 memset(cmd.response,0,GSM_6312_SMALL_RESPONSE_SIZE );
 
 cmd.response_status_pos = AT_CMD_RESPONSE_STATUS_POS_IN_TAIL;
 cmd.response_max = GSM_6312_SMALL_RESPONSE_SIZE - 1;
 memcpy(cmd.request,data,size);
 cmd.request[size] = '\0';
 cmd.request_size = size;
 //cmd.request_size = strlen(cmd.request);
 cmd.response_ok = GSM_M6312_SEND_START_RESPONSE_OK_AT_STR; 
 cmd.response_err = GSM_M6312_SEND_START_RESPONSE_ERR_AT_STR; 
 cmd.timeout = GSM_M6312_SEND_START_TIMEOUT; 

 at_rc = at_cmd_execute(gsm_m6312_serial_handle,&cmd);
 if(at_rc != 0 ){
 send_rc = -1;
 goto err_handler;
 }else{
 send_rc = 0;
 }
 
err_handler:
 if(send_rc == 0){
 log_debug("gsm m6312 send ok.\r\n");
 }else{
 log_error("gsm m6312 send err.\r\n");  
 }
 GSM_M6312_FREE(cmd.request);
 GSM_M6312_FREE(cmd.response);
 osMutexRelease(gsm_mutex);
 return send_rc;
}


/* 函数名：gsm_m6312_config_recv_buffer
*  功能：  配置数据接收缓存
*  参数：  buffer 缓存类型
*  返回：  0：成功 其他：失败
*/
int gsm_m6312_config_recv_buffer(gsm_m6312_recv_buffer_t buffer)
{
 int at_rc;
 int buffer_rc;
 at_cmd_t cmd;  
 osMutexWait(gsm_mutex,osWaitForever); 
 cmd.request = (char *)GSM_M6312_MALLOC(GSM_6312_SMALL_REQUEST_SIZE);
 cmd.response = (char *)GSM_M6312_MALLOC(GSM_6312_SMALL_RESPONSE_SIZE);
 
 if(cmd.request == NULL ||cmd.response == NULL){
 log_debug("malloc fail.\r\n");
 buffer_rc = -1;
 goto err_handler;
 }

 memset(cmd.request,0,GSM_6312_SMALL_REQUEST_SIZE);
 memset(cmd.response,0,GSM_6312_SMALL_RESPONSE_SIZE );
 
 cmd.response_status_pos = AT_CMD_RESPONSE_STATUS_POS_IN_TAIL;
 cmd.response_max = GSM_6312_SMALL_RESPONSE_SIZE - 1;
 strcpy(cmd.request,GSM_M6312_CONFIG_RECV_BUFFER_AT_STR);
 if(buffer == GSM_M6312_RECV_BUFFERE){
 strcat(cmd.request,GSM_M6312_RECV_BUFFER_AT_STR);
 }else{
 strcat(cmd.request,GSM_M6312_RECV_NO_BUFFER_AT_STR);
 }
 strcat(cmd.request,AT_CMD_BREAK_STR);
 strcat(cmd.request,"0");
 strcat(cmd.request,AT_CMD_EOF_STR);
 
 cmd.request_size = strlen(cmd.request);
 cmd.response_ok = GSM_M6312_CONFIG_RECV_BUFFER_RESPONSE_OK_AT_STR; 
 cmd.response_err = GSM_M6312_CONFIG_RECV_BUFFER_RESPONSE_ERR_AT_STR; 
 cmd.timeout = GSM_M6312_CONFIG_RECV_BUFFER_TIMEOUT; 

 at_rc = at_cmd_execute(gsm_m6312_serial_handle,&cmd);
 if(at_rc != 0 ){
 buffer_rc = -1;
 goto err_handler;
 }else{
 buffer_rc = 0;
 }
 
err_handler:
 if(buffer_rc == 0){
 log_debug("gsm m6312 config recv buffer ok.\r\n");
 }else{
 log_error("gsm m6312 config recv buffer err.\r\n");  
 }
 GSM_M6312_FREE(cmd.request);
 GSM_M6312_FREE(cmd.response);
 osMutexRelease(gsm_mutex);
 return buffer_rc;
}


/* 函数名：gsm_m6312_dump_recv_buffer_size_value
*  功能：  dump数据缓存长度值
*  参数：  buffer源数据
*  返回：  >=0：缓存数据大小值 其他：失败
*/
static int gsm_m6312_dump_recv_buffer_size_value(const char *buffer,const int conn_id)
{
 char *pos;
 int conn_id_temp;
 int total_buffer_size;
 //int last_recv_size;
 int loop_cnt;
 
 GSM_M6312_ASSERT_NULL_POINTER(buffer);
 loop_cnt = 6;
 while(loop_cnt -- ){
 /*找到header位置*/
 pos = strstr(buffer,GSM_M6312_RECV_BUFFER_SIZE_HEADER_STR);
 if(pos == NULL){
 log_error("can not find recv buffer size header.\r\n");
 return -1;
 }
 /*conn id 位置*/
 buffer = pos + strlen(GSM_M6312_RECV_BUFFER_SIZE_HEADER_STR);
 conn_id_temp = atoi(buffer);
 if(conn_id != conn_id_temp){
 /*结束这一行查找*/
 pos = strstr(buffer,"\n");
 GSM_M6312_ASSERT_NULL_POINTER(pos);
 buffer = pos + 1;
 continue;  
 }
 /*第1个BREAK位置*/
 pos = strstr(buffer,AT_CMD_BREAK_STR);
 /*last recv size位置*/
 buffer = pos + strlen(AT_CMD_BREAK_STR);
 //last_recv_size = atoi(buffer);
/*第2个BREAK位置*/
 pos = strstr(buffer,AT_CMD_BREAK_STR);
 /*total size位置*/
 buffer = pos + strlen(AT_CMD_BREAK_STR);
 total_buffer_size = atoi(buffer);

 log_debug("recv buffer size:<%d >ok.\r\n",total_buffer_size);
 return total_buffer_size;
 }

 return -1;
}



/* 函数名：gsm_m6312_get_recv_buffer_size
*  功能：  获取数据缓存长度
*  参数：  conn_id 连接ID
*  返回：  >=0：缓存数据量 其他：失败
*/
int gsm_m6312_get_recv_buffer_size(int conn_id)
{
 int at_rc;
 int buffer_size_rc;
 at_cmd_t cmd;  
 
 if(conn_id < 0 || conn_id > 4){
 log_error("conn id:%d is invalid.\r\n",conn_id);
 return -1;
 }
 osMutexWait(gsm_mutex,osWaitForever);
 cmd.request = (char *)GSM_M6312_MALLOC(GSM_6312_SMALL_REQUEST_SIZE);
 cmd.response = (char *)GSM_M6312_MALLOC(GSM_6312_SMALL_RESPONSE_SIZE);
 
 if(cmd.request == NULL ||cmd.response == NULL){
 log_debug("malloc fail.\r\n");
 buffer_size_rc = -1;
 goto err_handler;
 }
 
 memset(cmd.request,0,GSM_6312_SMALL_REQUEST_SIZE);
 memset(cmd.response,0,GSM_6312_SMALL_RESPONSE_SIZE );
 
 cmd.response_status_pos = AT_CMD_RESPONSE_STATUS_POS_IN_TAIL;
 cmd.response_max = GSM_6312_SMALL_RESPONSE_SIZE - 1;
 strcpy(cmd.request,GSM_M6312_GET_RECV_BUFFER_SIZE_AT_STR);
 strcat(cmd.request,AT_CMD_EOF_STR);
 cmd.request_size = strlen(cmd.request);
 cmd.response_ok = GSM_M6312_GET_RECV_BUFFER_SIZE_RESPONSE_OK_AT_STR; 
 cmd.response_err = GSM_M6312_GET_RECV_BUFFER_SIZE_RESPONSE_ERR_AT_STR; 
 cmd.timeout = GSM_M6312_GET_RECV_BUFFER_SIZE_TIMEOUT; 

 at_rc = at_cmd_execute(gsm_m6312_serial_handle,&cmd);
 if(at_rc != 0 ){
 buffer_size_rc = -1;
 goto err_handler;
 }
 
 buffer_size_rc = gsm_m6312_dump_recv_buffer_size_value(cmd.response,conn_id);
err_handler:
 if(buffer_size_rc >= 0){
 log_debug("gsm m6312 get recv buffer size ok.\r\n");
 }else{
 log_error("gsm m6312 get recv buffer size err.\r\n");  
 }
 GSM_M6312_FREE(cmd.request);
 GSM_M6312_FREE(cmd.response);
 osMutexRelease(gsm_mutex);
 return buffer_size_rc;
}

/* 函数名：gsm_m6312_dump_recv_data
*  功能：  dump接收到的数据
*  参数：  dest 目的buffer
*  参数：  src 源buffer
*  参数：  size 接收到的大小
*  返回：  0：成功 其他：失败
*/
static int gsm_m6312_dump_recv_data(char *buffer_dest,const char *buffer_src,const int size)
{
 GSM_M6312_ASSERT_NULL_POINTER(buffer_dest);
 GSM_M6312_ASSERT_NULL_POINTER(buffer_src);

 memcpy(buffer_dest,buffer_src,size);
 buffer_dest[size] = '\0';
 return size;
}



/* 函数名：gsm_m6312_recv
*  功能：  接收数据
*  参数：  conn_id 连接ID
*  参数：  data 数据buffer
*  参数：  size buffer的size
*  返回：  >=0：接收到的数据量 其他：失败
*/
int gsm_m6312_recv(int conn_id,char *data,const int size)
{
 int at_rc;
 int recv_rc;
 at_cmd_t cmd;  
 
 if(conn_id < 0 || conn_id > 4){
 log_error("conn id:%d is invalid.\r\n",conn_id);
 return -1;
 }
 osMutexWait(gsm_mutex,osWaitForever);
 cmd.request = (char *)GSM_M6312_MALLOC(GSM_6312_SMALL_REQUEST_SIZE);
 cmd.response = (char *)GSM_M6312_MALLOC(GSM_6312_LARGE_RESPONSE_SIZE);
 
 if(cmd.request == NULL ||cmd.response == NULL){
 log_debug("malloc fail.\r\n");
 recv_rc = -1;
 goto err_handler;
 }

 memset(cmd.request,0,GSM_6312_SMALL_REQUEST_SIZE);
 memset(cmd.response,0,GSM_6312_LARGE_RESPONSE_SIZE );
 
 cmd.response_status_pos = AT_CMD_RESPONSE_STATUS_POS_IN_TAIL;
 cmd.response_max = GSM_6312_LARGE_RESPONSE_SIZE - 1;
 strcpy(cmd.request,GSM_M6312_RECV_AT_STR);

 snprintf(cmd.request + strlen(cmd.request),2,"%d",conn_id);
 strcat(cmd.request,AT_CMD_BREAK_STR);
 if(size <= 0 || size >  4096 || size > GSM_6312_LARGE_RESPONSE_SIZE - 20){
 log_warning("recv size:%d is too large.use smaller size.\r\n",size);
 goto err_handler;
 }
 snprintf(cmd.request + strlen(cmd.request),5,"%d",size);
 strcat(cmd.request,AT_CMD_EOF_STR);
 
 cmd.request_size = strlen(cmd.request);
 cmd.response_ok = GSM_M6312_RECV_RESPONSE_OK_AT_STR; 
 cmd.response_err = GSM_M6312_RECV_RESPONSE_ERR_AT_STR; 
 cmd.timeout = GSM_M6312_RECV_TIMEOUT; 

 at_rc = at_cmd_execute(gsm_m6312_serial_handle,&cmd);
 if(at_rc != 0 ){
 recv_rc = -1;
 goto err_handler;
 }
 recv_rc = gsm_m6312_dump_recv_data(data,cmd.response,size);
err_handler:
 if(recv_rc >= 0){
 log_debug("gsm m6312 recv ok.\r\n");
 }else{
 log_error("gsm m6312 recv err.\r\n");  
 }
 GSM_M6312_FREE(cmd.request);
 GSM_M6312_FREE(cmd.response);
 osMutexRelease(gsm_mutex);
 return recv_rc;
}
