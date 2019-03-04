#ifndef  __NET_TASK_H__
#define  __NET_TASK_H__

#include "wifi_8710bx.h"
#include "gsm_m6312.h"
#include "net.h"

extern osThreadId   net_task_handle;
extern osMessageQId net_task_msg_q_id;

void net_task(void const *argument);


#define  NET_TASK_QUERY_WIFI_TIMEOUT              10000         /*查询wifi状态间隔 ms*/
#define  NET_TASK_QUERY_GSM_TIMEOUT               1 * 60 * 1000 /*查询基站间隔 ms*/


#define  NET_TASK_ERR_CNT_MAX                     20            /*硬件错误次数，到达后复位*/
#define  NET_TASK_WIFI_INIT_TIMEOUT               3000          /*WIFI初始化超时时间ms*/
#define  NET_TASK_GSM_INIT_TIMEOUT                30000         /*GSM初始化超时时间ms*/
#define  NET_TASK_INIT_RETRY_DELAY                60000         /*初始化超时后等待重试间隔时间ms*/
#define  NET_TASK_PUT_MSG_TIMEOUT                 5
#define  NET_TASK_NET_INIT_ERR_CNT_MAX            3             /*上电尝试的初始化错误次数*/

typedef struct
{
 gsm_base_t base_main;
 gsm_base_t base_assist1;
 gsm_base_t base_assist2;
 gsm_base_t base_assist3; 
}net_location_t;

typedef enum
{
  NET_TASK_MSG_QUERY_WIFI,
  NET_TASK_MSG_QUERY_GSM,
}net_task_msg_type_t;

typedef struct
{
  uint32_t type:8;
  uint32_t value:8;
  uint32_t reserved:16;
}net_task_msg_t;

typedef enum
{
  NET_WIFI_CONFIG_STATUS_VALID = 0xCC,
  NET_WIFI_CONFIG_STATUS_INVALID
}net_wifi_config_status_t;

typedef struct
{
  net_wifi_config_status_t status;
  char ssid[WIFI_8710BX_SSID_STR_LEN + 1];
  char passwd[WIFI_8710BX_PASSWD_STR_LEN + 1];
}net_wifi_config_t;


typedef struct
{
struct
{ 
  bool is_initial;
  bool is_config;
  int  rssi;
  int  level;
  net_wifi_config_t config;
  char mac[WIFI_8710BX_MAC_STR_LEN + 1];
  char ip[WIFI_8710BX_IP_STR_LEN + 1];
  uint16_t err_cnt;
  net_status_t status;
}wifi;

struct
{
  bool is_sim_exsit;
  bool is_initial;
  char sim_id[GSM_M6312_SIM_ID_STR_LEN + 1];
  net_location_t location;
  uint16_t err_cnt;
  net_status_t status;
}gsm;
}net_t;



typedef struct
{
  char *mac;
  char *sim_id;
}net_hal_information_t;







#endif