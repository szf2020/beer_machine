#include "stdbool.h"
#include "stddef.h"
#include "stdint.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "at_cmd.h"
#include "serial.h"
#include "cmsis_os.h"
#include "log.h"
#define  LOG_MODULE_LEVEL    LOG_LEVEL_DEBUG
#define  LOG_MODULE_NAME     "[at]"


/*校验at回应是否成功*/              
static int at_cmd_check_response(at_cmd_t *cmd)
{
 char err_str[AT_CMD_ERR_STR_LEN + 1];
 const char *err_break;
 const char *err_str_start;
 uint8_t err_str_len;
 
 const char *err_pos;
 const char *err_tolon_pos;
 const char *err_code_pos;
 const char *ok_pos;
 int ok_len;
 
 if(cmd == NULL){  
 return -1;
 }

 ok_len = strlen(cmd->response_ok);
 if(cmd->response_size >= ok_len){
 /*回应的位置在尾部*/
 if(cmd->response_status_pos == AT_CMD_RESPONSE_STATUS_POS_IN_TAIL){ 
 ok_pos = cmd->response + (cmd->response_size - ok_len);
 if(strcmp(ok_pos,cmd->response_ok) == 0){
 cmd->err_code = 0;
 return 0;
 }  
 }else{/*回应的位置在头部*/
 if(strstr(cmd->response,cmd->response_ok)){
 cmd->err_code = 0;
 return 0;     
 }
 }
 }
 /*错误回应处理 最多4个错误类型*/
 err_str_start = cmd->response_err;
 for(uint8_t cnt= 0; cnt < 4; cnt++){
 err_break = strstr(err_str_start,AT_CMD_ERR_BREAK_STR);
 if(err_break == NULL){
 strcpy(err_str,err_str_start);
 /*循环结束*/
 cnt = 4;
 }else{
 err_str_len = err_break - err_str_start > AT_CMD_ERR_STR_LEN ? AT_CMD_ERR_STR_LEN :err_break - err_str_start;
 memcpy(err_str,err_str_start,err_str_len);
 err_str[err_str_len] = '\0';
 err_str_start = err_break + strlen(AT_CMD_ERR_BREAK_STR);
 }
 /*对比错误字符*/
 err_pos = strstr(cmd->response,err_str);
 if(err_pos){
 err_tolon_pos = strstr(err_pos,AT_CMD_TOLON_STR);
 if(err_tolon_pos == NULL){
 cmd->err_code = -1;
 }else{
 err_code_pos = err_tolon_pos + strlen(AT_CMD_TOLON_STR);
 cmd->err_code = atoi(err_code_pos);
 }
 return 0;
 }
 }
 return -1; 
}


/*执行at指令*/
int at_cmd_execute(int handle,at_cmd_t *cmd)
{
 int write_len;
 int write_remain;
 int write_len_total;
 int send_remain;
 int select_len;
 int read_len;
 int rc;
 uint32_t start_time,cur_time;
 uint32_t write_timeout = AT_CMD_WEITE_TIMEOUT;
 uint32_t response_timeout;
 
 cmd->err_code = 0;
 write_len_total = 0;
 write_remain = cmd->request_size;

 
 do{
 osDelay(1);
 write_timeout -- ;
 write_len = serial_write(handle,(const uint8_t*)cmd->request + write_len_total,write_remain);
 if(write_len == -1){
 log_error("at cmd write err.\r\n");
 goto err_handler;  
 }
 write_len_total +=write_len;
 write_remain = cmd->request_size - write_len_total;
 }while(write_remain > 0 && write_timeout > 0);
 
 if(cmd->request_size !=  write_len_total){
 log_error("at cmd no space to send.\r\n");
 goto err_handler;
 }
 send_remain = serial_complete(handle,AT_CMD_SEND_TIMEOUT); 
 if(send_remain != 0){
 log_error("at cmd send timeout.\r\n");
 goto err_handler;  ;
 }
 
 cmd->response_size = 0;
 response_timeout = cmd->timeout;
 cmd->response_len_changed = false;
 do{
 start_time = osKernelSysTick();
 select_len = serial_select(handle,AT_CMD_SELECT_TIMEOUT);
 if(select_len < 0){
 goto err_handler; 
 }

 /*接收完一帧数据*/
 if(select_len == 0 && cmd->response_len_changed == true){
 cmd->response[cmd->response_size] = '\0';
 cmd->response_len_changed = false;
 rc = at_cmd_check_response(cmd);
 /*得到了期望的回应ok or err*/
 if(rc == 0){
 /*执行成功*/
 if(cmd->err_code == 0){
 log_debug("%s excute ok.\r\n",cmd->request);
 return 0;
 }else{
 /*执行失败*/ 
 goto err_handler;
 }
 }   
 }else{
 if(cmd->response_size + select_len >= cmd->response_max){
 log_error("at cmd response too long.\r\n");
 goto err_handler; 
 }
 read_len = serial_read(handle,(uint8_t *)cmd->response + cmd->response_size,select_len);
 cmd->response_size += read_len;
 cmd->response_len_changed = true;
 }

 cur_time = osKernelSysTick();
 if(response_timeout > (cur_time - start_time)){
 response_timeout -=(cur_time - start_time);
 }else{
 response_timeout = 0;
 }
 }while(response_timeout > 0);  
 
err_handler:
 log_error("%s excute err.code:%d.\r\n",cmd->request,cmd->err_code);
 return -1;
}