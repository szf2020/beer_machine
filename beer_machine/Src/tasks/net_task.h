#ifndef  __NET_TASK_H__
#define  __NET_TASK_H__

#include "wifi_8710bx.h"
#include "gsm_m6312.h"
#include "net.h"

extern osThreadId   net_task_handle;
extern osMessageQId net_task_msg_q_id;

void net_task(void const *argument);



#define  NET_TASK_WIFI_INIT_TIMEOUT               3000
#define  NET_TASK_GSM_INIT_TIMEOUT                20000
#define  NET_TASK_INIT_RETRY_DELAY                10000
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
}net_task_msg_t;


typedef struct
{
struct
{ 
  bool is_initial;
  bool is_config;
  int  rssi;
  int  level;
  char ssid[WIFI_8710BX_SSID_STR_LEN];
  char passwd[WIFI_8710BX_PASSWD_STR_LEN];
  char mac[WIFI_8710BX_MAC_STR_LEN];
  char ip[WIFI_8710BX_IP_STR_LEN];
  uint16_t err_cnt;
  net_status_t status;
}wifi;

struct
{
  bool is_sim_exsit;
  bool is_initial;
  char sim_id[GSM_M6312_SIM_ID_STR_LEN];
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