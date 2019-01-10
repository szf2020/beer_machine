#include "stdbool.h"
#include "stddef.h"
#include "stdint.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "at.h"
#include "serial.h"
#include "cmsis_os.h"
#include "utils.h"
#include "log.h"
#define  LOG_MODULE_LEVEL    LOG_LEVEL_DEBUG
#define  LOG_MODULE_NAME     "[at]"



/* 函数名：at_err_code_add
*  功能：  添加比对的错误码
*  参数：  err_head 错误头指针 
*  参数：  err_code 错误节点 
*  返回：  无
*/
void at_err_code_add(at_err_code_t **err_head,at_err_code_t *err_code)
{
    at_err_code_t *err_node;
 
    if (*err_head == NULL){
        *err_head = err_code;
        err_code->next = NULL;
        return;
    }
    err_node = *err_head;
    while(err_node->next){
          err_node = err_node->next;   
    }
    err_node->next = err_code;
}


/* 函数名：at_send
*  功能：  串口发送
*  参数：  send 发送的数据 
*  参数：  size 数据大小 
*  参数：  timeout 发送超时 
*  返回：  AT_ERR_OK：成功 其他：失败
*/
 int at_send(int handle,const char *send,const uint16_t size,const uint16_t timeout)
{
 int rc;
 uint16_t write_total = 0;
 uint16_t remain_total = size;
 int write_len;
 
 utils_timer_t timer;
 
 utils_timer_init(&timer,timeout,false);

 do{
   write_len = serial_write(handle,send + write_total,remain_total);
   if(write_len == -1){
      log_error("at cmd write err.\r\n");
      return AT_ERR_SERIAL_SEND;  
   }
   write_total += write_len;
   remain_total -= write_len;
   osDelay(1);
   }while(remain_total > 0 && utils_timer_value(&timer) > 0);  
  
 if(remain_total > 0){
    return AT_ERR_SEND_TIMEOUT;
 }

 rc = serial_complete(handle,utils_timer_value(&timer));
 if(rc < 0){
    return AT_ERR_SERIAL_SEND;  ;
 }
 
 if(rc > 0){
    return AT_ERR_SEND_TIMEOUT;  ;
 }
 
  return AT_ERR_OK;
}

/* 函数名：at_recv
*  功能：  串口接收
*  参数：  recv 数据buffer 
*  参数：  size 数据大小 
*  参数：  timeout 接收超时 
*  返回：  读到的一帧数据量 其他：失败
*/

int at_recv(int handle,char *recv,const uint16_t size,const uint32_t timeout)
{
 int select_size;
 uint16_t read_total = 0;
 int read_size;
 utils_timer_t timer;
 
 utils_timer_init(&timer,timeout,false);

 /*等待数据*/
 select_size = serial_select(handle,utils_timer_value(&timer));
 if(select_size < 0){
    return AT_ERR_SERIAL_RECV;
 } 
 if(select_size == 0){
    return AT_ERR_RECV_TIMEOUT;  
 }
 
 do{
   if(read_total + select_size > size){
      log_error("rsp too large.\r\n");
      return AT_ERR_RECV_NO_SPACE;
  }
  read_size = serial_read(handle,recv + read_total,select_size); 
  if(read_size < 0){
     return AT_ERR_SERIAL_RECV;  
  }
  read_total += read_size;
  select_size = serial_select(handle,AT_SELECT_TIMEOUT);
  if(select_size < 0){
     return AT_ERR_SERIAL_RECV;
  } 
  }while(select_size != 0);
 
  //recv[read_total] = '\0';
  
  return read_total;
  
 }

/* 函数名：at_check_response
*  功能：  检查回应
*  参数：  rsp 回应数组 
*  参数：  size 一帧数据的长度 
*  参数：  err_head 错误头 
*  参数：  complete 回应是否结束 
*  返回：  AT_ERR_OK：成功 其他：失败 
*/
static int at_check_response(const char *rsp,int size,at_err_code_t *err_head,bool *complete)
{
    at_err_code_t *err_node;
    
    err_node = err_head;
    while(err_node){   
         /*前部或者后面查找*/
         if(strstr(rsp,err_node->str) || strstr(rsp + size - 4,err_node->str)){
            *complete = true;
            return err_node->code;
         }
         err_node = err_node->next;
    }
    
    return AT_ERR_OK;     
}
  
/* 函数名：at_excute
*  功能：  at命令执行
*  参数：  cmd 命令指针 
*  返回：  返回： AT_ERR_OK：成功 其他：失败 
*/  
int at_excute(int handle,at_t *at)
{
  int rc;
  int recv_size;
  utils_timer_t timer;
 
  utils_timer_init(&timer,at->send_timeout,false);
    
  rc = at_send(handle,at->send,at->send_size,utils_timer_value(&timer));
  if(rc != AT_ERR_OK){
     goto exit;
  }
  utils_timer_init(&timer,at->recv_timeout,false);  
  /*发送完一帧数据的标志*/
  log_debug("at send:\r\n%s",at->send);
  /*清空数据*/
  serial_flush(handle);

  while (utils_timer_value(&timer)){
    recv_size = at_recv(handle,at->recv + at->recv_size,AT_RECV_BUFFER_SIZE - at->recv_size,utils_timer_value(&timer));
    if(recv_size < 0){
       rc = recv_size;
       goto exit; 
    }
    /*接收完一帧数据*/
    log_debug("at recv:\r\n%s",at->recv + at->recv_size);
    
    /*校验回应数据*/
    rc = at_check_response(at->recv + at->recv_size,recv_size,at->err_head,&at->complete);
    at->recv_size += recv_size;
    
    if(at->complete){
       goto exit;
    }
  }
  rc = AT_ERR_RECV_TIMEOUT;
exit:


  return rc;
}