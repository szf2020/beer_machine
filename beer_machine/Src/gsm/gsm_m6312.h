#ifndef  __GSM_M6312_H__
#define  __GSM_M6312_H__

#include "stdbool.h"
#include "stddef.h"
#include "stdint.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"


#define  GSM_M6312_BUFFER_SIZE                   256
#define  GSM_M6312_SERIAL_PORT                   1
#define  GSM_M6312_SERIAL_BAUDRATES              115200
#define  GSM_M6312_SERIAL_DATA_BITS              8
#define  GSM_M6312_SERIAL_STOP_BITS              1

#define  GSM_M6312_IMEI_LEN                      15
#define  GSM_M6312_SN_LEN                        20

enum
{
 GSM_ERR_OK,
 GSM_ERR_MALLOC_FAIL = -1,
 GSM_ERR_CMD_ERR = -2,
 GSM_ERR_CMD_TIMEOUT = -3,
 GSM_ERR_RSP_TIMEOUT = -4,
 GSM_ERR_SERIAL_SEND = -5,
 GSM_ERR_SERIAL_RECV = -6,
 GSM_ERR_RECV_NO_SPACE = -7,
 GSM_ERR_SOCKET_ALREADY_CONNECT = -8,
 GSM_ERR_CONNECT_FAIL = -9,
 GSM_ERR_SOCKET_SEND_FAIL = -10,
 GSM_ERR_SOCKET_DISCONNECT = -11,
 GSM_ERR_NULL_POINTER = -12,
 GSM_ERR_HAL_GPIO = -13,
 GSM_ERR_HAL_INIT = -14,
 GSM_ERR_UNKNOW = -200
};



typedef enum
{
GSM_M6312_SOCKET_INIT,
GSM_M6312_SOCKET_CONNECT,
GSM_M6312_SOCKET_CONNECTTING,
GSM_M6312_SOCKET_CLOSE,
GSM_M6312_SOCKET_BIND
}gsm_m6312_socket_status_t;

typedef enum
{
GSM_M6312_ECHO_ON,
GSM_M6312_ECHO_OFF
}gsm_m6312_echo_t;


typedef enum
{
GSM_M6312_REPORT_ON,
GSM_M6312_REPORT_OFF
}gsm_m6312_report_t;


typedef enum
{
GSM_M6312_APN_CMNET,
GSM_M6312_APN_UNINET,
GSM_M6312_APN_UNKNOW
}gsm_m6312_apn_t;

typedef enum
{
GSM_M6312_ACTIVE ,
GSM_M6312_INACTIVE
}gsm_m6312_active_status_t;

typedef enum
{
GSM_M6312_ATTACH,
GSM_M6312_NOT_ATTACH
}gsm_m6312_attach_status_t;

typedef enum
{
SIM_CARD_STATUS_READY,
SIM_CARD_STATUS_NO_SIM_CARD,
SIM_CARD_STATUS_LOCKED
}sim_card_status_t;

typedef enum
{
OPERATOR_CHINA_MOBILE,
OPERATOR_CHINA_UNICOM,
OPERATOR_CHINA_TELECOM
}operator_name_t;

typedef enum 
{
OPERATOR_AUTO_MODE,
OPERATOR_MANUAL_MODE,
OPERATOR_LOGOUT_MODE
}operator_mode_t;

typedef enum
{
GSM_M6312_CONNECT_MODE_SINGLE,
GSM_M6312_CONNECT_MODE_MULTIPLE
}gsm_m6312_connect_mode_t;

typedef enum
{
GSM_M6312_OPERATOR_FORMAT_LONG_NAME,
GSM_M6312_OPERATOR_FORMAT_SHORT_NAME,
GSM_M6312_OPERATOR_FORMAT_NUMERIC_NAME
}gsm_m6312_operator_format_t;

typedef enum
{
GSM_M6312_RECV_BUFFERE,
GSM_M6312_RECV_NO_BUFFERE,
}gsm_m6312_recv_buffer_t;

typedef enum
{
GSM_M6312_NET_PROTOCOL_TCP,
GSM_M6312_NET_PROTOCOL_UDP
}gsm_m6312_net_protocol_t;

typedef enum
{
GSM_M6312_SEND_PROMPT,
GSM_M6312_NO_SEND_PROMPT
}gsm_m6312_send_prompt_t;

typedef enum
{
GSM_M6312_TRANPARENT,
GSM_M6312_NO_TRANPARENT
}gsm_m6312_transparent_t;

/* 函数名：gsm_m6312_pwr_on
*  功能：  m6312 2g模块开机
*  参数：  无 
*  返回：  0：成功 其他：失败
*/
int gsm_m6312_pwr_on(void);
/* 函数名：gsm_m6312_pwr_off
*  功能：  m6312 2g模块关机
*  参数：  无 
*  返回：  0：成功 其他：失败
*/
int gsm_m6312_pwr_off(void);
/* 函数名：gsm_m6312_serial_hal_init
*  功能：  m6312 2g模块执行硬件和串口初始化
*  参数：  无 
*  返回：  0：成功 其他：失败
*/
int gsm_m6312_serial_hal_init(void);

/* 函数名：gsm_m6312_set_echo
*  功能：  设置是否回显输入的命令
*  参数：  echo回显设置 
*  返回：  0：成功 其他：失败
*/
int gsm_m6312_set_echo(gsm_m6312_echo_t echo);
/* 函数名：gsm_m6312_is_sim_card_ready
*  功能：  查询sim卡是否解锁就绪
*  参数：  status状态指针 
*  返回：  0：成功 其他：失败
*/
int gsm_m6312_is_sim_card_ready(sim_card_status_t *status);
/* 函数名：gsm_m6312_get_imei
*  功能：  获取设备IMEI串号
*  参数：  imei imei指针
*  返回：  0：ready 其他：失败
*/
int gsm_m6312_get_imei(char *imei);
/* 函数名：gsm_m6312_get_sn
*  功能：  获取设备SN号
*  参数：  sn sn指针
*  返回：  0：ready 其他：失败
*/
int gsm_m6312_get_sn(char *sn);
/* 函数名：gsm_m6312_gprs_set_apn
*  功能：  设置指定cid的gprs的APN
*  参数：  cid 指定的cid
*  参数：  apn 网络APN 
*  返回：  0：成功 其他：失败
*/
int gsm_m6312_gprs_set_apn(gsm_m6312_apn_t apn);
/* 函数名：gsm_m6312_gprs_get_apn
*  功能：  获取指定cid的gprs的apn
*  参数：  cid 指定cid 
*  参数：  apn 指针 
*  返回：  0：成功 其他：失败
*/
int gsm_m6312_gprs_get_apn(gsm_m6312_apn_t *apn);
/* 函数名：gsm_m6312_gprs_set_active_status
*  功能：  激活或者去激活指定cid的GPRS功能
*  参数：  cid gprs cid 
*  参数：  active 状态 
*  返回：  0：成功 其他：失败
*/
int gsm_m6312_gprs_set_active_status(gsm_m6312_active_status_t active);
/* 函数名：gsm_m6312_gprs_get_active_status
*  功能：  获取对应cid的GPRS激活状态
*  参数：  cid   gprs的id 
*  参数：  active gprs状态指针 
*  返回：  0：成功 其他：失败
*/
int gsm_m6312_gprs_get_active_status(gsm_m6312_active_status_t *active);
/* 函数名：gsm_m6312_gprs_set_attach_status
*  功能：  设置GPRS网络附着状态
*  参数：  attach GPRS附着状态 
*  返回：  0：成功 其他：失败
*/
int gsm_m6312_gprs_set_attach_status(gsm_m6312_attach_status_t attach);
/* 函数名：gsm_m6312_gprs_get_attach_status
*  功能：  获取GPRS的附着状态
*  参数：  attach GPRS附着状态指针
*  返回：  0：成功 其他：失败
*/
int gsm_m6312_gprs_get_attach_status(gsm_m6312_attach_status_t *attach);
/* 函数名：gsm_m6312_gprs_set_connect_mode
*  功能：  设置连接模式指令
*  参数：  mode 单路或者多路 
*  返回：  0：成功 其他：失败
*/
int gsm_m6312_gprs_set_connect_mode(gsm_m6312_connect_mode_t mode);
/* 函数名：gsm_m6312_get_operator
*  功能：  查询运营商
*  参数：  operator_name 运营商指针
*  返回：  0：成功 其他：失败
*/
int gsm_m6312_get_operator(operator_name_t *operator_name);
/* 函数名：gsm_m6312_set_auto_operator_format
*  功能：  设置自动运营商选择和运营商格式
*  参数：  operator_format 运营商格式
*  返回：  0：成功 其他：失败
*/
int gsm_m6312_set_auto_operator_format(gsm_m6312_operator_format_t operator_format);
/* 函数名：gsm_m6312_set_set_prompt
*  功能：  设置发送提示
*  参数：  prompt 设置的发送提示
*  返回：  0：成功 其他：失败
*/
int gsm_m6312_set_set_prompt(gsm_m6312_send_prompt_t prompt);

/* 函数名：gsm_m6312_set_transparent
*  功能：  设置透传模式
*  参数：  transparent 连接ID
*  返回：  0：成功 其他：失败
*/
int gsm_m6312_set_transparent(gsm_m6312_transparent_t transparent);
/* 函数名：gsm_m6312_open_client
*  功能：  打开连接
*  参数：  conn_id 连接ID
*  参数：  protocol协议类型<TCP或者UDP>
*  参数：  host 主机IP后者域名
*  参数：  port 主机端口号
*  返回：  0：成功 其他：失败
*/
int gsm_m6312_open_client(int conn_id,gsm_m6312_net_protocol_t protocol,const char *host,uint16_t port);
/* 函数名：gsm_m6312_close_client
*  功能：  关闭连接
*  参数：  conn_id 连接ID
*  返回：  0：成功 其他：失败
*/
int gsm_m6312_close_client(int conn_id);
/* 函数名：gsm_m6312_get_connect_status
*  功能：  获取连接的状态
*  参数：  conn_id 连接ID
*  参数：  status连接状态指针
*  返回：  0：成功 其他：失败
*/
int gsm_m6312_get_connect_status(const int conn_id,gsm_m6312_socket_status_t *status);
/* 函数名：gsm_m6312_send
*  功能：  发送数据
*  参数：  conn_id 连接ID
*  参数：  data 数据buffer
*  参数：  size 发送的数据数量
*  返回：  0：成功 其他：失败
*/
int gsm_m6312_send(int conn_id,const char *data,const int size);
/* 函数名：gsm_m6312_config_recv_buffer
*  功能：  配置数据接收缓存
*  参数：  buffer 缓存类型
*  返回：  0：成功 其他：失败
*/
int gsm_m6312_config_recv_buffer(gsm_m6312_recv_buffer_t buffer);
/* 函数名：gsm_m6312_get_recv_buffer_size
*  功能：  获取数据缓存长度
*  参数：  conn_id 连接ID
*  返回：  >=0：缓存数据量 其他：失败
*/
int gsm_m6312_get_recv_buffer_size(int conn_id);
/* 函数名：gsm_m6312_recv
*  功能：  接收数据
*  参数：  conn_id 连接ID
*  参数：  data 数据buffer
*  参数：  size buffer的size
*  返回：  >=0：接收到的数据量 其他：失败
*/
int gsm_m6312_recv(int conn_id,char *data,const int size);


#endif