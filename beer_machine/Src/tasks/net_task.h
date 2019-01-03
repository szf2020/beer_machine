#ifndef  __NET_TASK_H__
#define  __NET_TASK_H__

#include "wifi_8710bx.h"
#include "gsm_m6312.h"
#include "net.h"

extern osThreadId   net_task_handle;
extern osMessageQId net_task_msg_q_id;

void net_task(void const *argument);

/*wifi配置参数在env reserved中的偏移位置*/
#define  NET_WIFI_CONFIG_ENV_OFFSET               4

#define  NET_TASK_WIFI_INIT_TIMEOUT               3000
#define  NET_TASK_GSM_INIT_TIMEOUT                20000
#define  NET_TASK_INIT_RETRY_DELAY                10000
#define  NET_WIFI_CONFIG_TIMEOUT                  (0)/*wifi配网时间2分钟*/
#define  NET_TASK_PUT_MSG_TIMEOUT                 5

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