#ifndef  __AT_H__
#define  __AT_H__
#include "stdint.h"
#include "stdbool.h"

#define  AT_SEND_BUFFER_SIZE              2000
#define  AT_RECV_BUFFER_SIZE              2000

#define  AT_SELECT_TIMEOUT                5

typedef struct 
{
  char *str;
  int code;
  void *next;
}at_err_code_t;


typedef struct
{
  char send[AT_SEND_BUFFER_SIZE];
  char recv[AT_RECV_BUFFER_SIZE];
  uint16_t send_size;
  uint16_t recv_size;
  uint16_t send_timeout;
  uint32_t recv_timeout;
  at_err_code_t *err_head;
  bool complete;
}at_t;

enum
{
  AT_ERR_OK,
  AT_ERR_ERR,
  AT_ERR_SEND_TIMEOUT,
  AT_ERR_RECV_TIMEOUT,
  AT_ERR_SERIAL_RECV,
  AT_ERR_SERIAL_SEND,
  AT_ERR_RECV_NO_SPACE
};
/* 函数名：at_err_code_add
*  功能：  添加比对的错误码
*  参数：  err_head 错误头指针 
*  参数：  err_code 错误节点 
*  返回：  无
*/
void at_err_code_add(at_err_code_t **err_head,at_err_code_t *err_code);

/* 函数名：at_send
*  功能：  串口发送
*  参数：  send 发送的数据 
*  参数：  size 数据大小 
*  参数：  timeout 发送超时 
*  返回：  AT_ERR_OK：成功 其他：失败
*/
 int at_send(int handle,const char *send,const uint16_t size,const uint16_t timeout);
 
 /* 函数名：at_recv
*  功能：  串口接收
*  参数：  recv 数据buffer 
*  参数：  size 数据大小 
*  参数：  timeout 接收超时 
*  返回：  读到的一帧数据量 其他：失败
*/

int at_recv(int handle,char *recv,const uint16_t size,const uint32_t timeout);

/* 函数名：at_excute
*  功能：  at命令执行
*  参数：  cmd 命令指针 
*  返回：  返回： AT_ERR_OK：成功 其他：失败 
*/  
int at_excute(int handle,at_t *at);

#endif