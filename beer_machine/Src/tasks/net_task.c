#include "FreeRTOS.h"
#include "task.h"
#include "cmsis_os.h"
#include "device_config.h"
#include "net_task.h"
#include "report_task.h"
#include "display_task.h"
#include "tasks_init.h"
#include "utils.h"
#include "bootloader_if.h"
#include "log.h"


osThreadId  net_task_handle;
osMessageQId net_task_msg_q_id;

static net_t net;



static osTimerId    wifi_query_timer_id;
static osTimerId    gsm_query_timer_id;


static void wifi_query_timer_init(void);
static void wifi_query_timer_start(void);
//static void wifi_query_timer_stop(void);
static void wifi_query_timer_expired(void const *argument);

static void gsm_query_timer_init(void);
static void gsm_query_timer_start(void);
//static void gsm_query_timer_stop(void);
static void gsm_query_timer_expired(void const *argument);


static void wifi_query_timer_init(void)
{
 osTimerDef(wifi_query_timer,wifi_query_timer_expired);
 wifi_query_timer_id=osTimerCreate(osTimer(wifi_query_timer),osTimerOnce,0);
 log_assert(wifi_query_timer_id);
}

static void wifi_query_timer_start(void)
{
 osTimerStart(wifi_query_timer_id,NET_TASK_QUERY_WIFI_TIMEOUT);  
 log_debug("wifi net timer start.\r\n");
}
/*
static void wifi_query_timer_stop(void)
{
 osTimerStop(wifi_query_timer_id);  
 log_debug("wifi net timer stop.\r\n");  
}
*/
static void wifi_query_timer_expired(void const *argument)
{
  osStatus status;
  net_task_msg_t msg;
  msg.type = NET_TASK_MSG_QUERY_WIFI;
  
  status = osMessagePut(net_task_msg_q_id,*(uint32_t*)&msg,0);
  if(status != osOK){
     log_error("put query gsm msg err.\r\n");
  }
}



static void gsm_query_timer_init()
{
 osTimerDef(gsm_query_timer,gsm_query_timer_expired);
 gsm_query_timer_id=osTimerCreate(osTimer(gsm_query_timer),osTimerOnce,0);
 log_assert(gsm_query_timer_id);
}

static void gsm_query_timer_start(void)
{
 osTimerStart(gsm_query_timer_id,NET_TASK_QUERY_GSM_TIMEOUT);  
 log_debug("gsm net timer start.\r\n");
}
/*
static void gsm_query_timer_stop(void)
{
 osTimerStop(gsm_query_timer_id);  
 log_debug("wifi net timer stop.\r\n");  
}
*/
static void gsm_query_timer_expired(void const *argument)
{
  osStatus status;
  net_task_msg_t msg;
  msg.type = NET_TASK_MSG_QUERY_GSM;
  
  status = osMessagePut(net_task_msg_q_id,*(uint32_t*)&msg,0);
  if(status != osOK){
     log_error("put query gsm msg err.\r\n");
  }
}


static int net_task_init(void)
{
  net_init();

  memset(&net,0,sizeof(net)); 
  /*wifi相关*/
  net.wifi.status = NET_STATUS_OFFLINE;
  /*gsm相关*/
  net.gsm.status = NET_STATUS_OFFLINE;
  
  return 0;
}


/* 函数：net_task_wifi_init
*  功能：初始化wifi
*  参数：无
*  返回：0 成功 其他：失败
*/ 
static int net_task_wifi_init(uint32_t timeout)
{
  int rc;
  
  utils_timer_t timer;
  
  if(net.wifi.is_initial == true){
     log_error("wifi already init ok.\r\n");
     return 0;
  }
  utils_timer_init(&timer,timeout,false);
  
  /*复位*/
  rc = net_wifi_reset(utils_timer_value(&timer));
  if(rc != 0){
     return -1;
  }
        
  /*通信接口初始化*/
  rc = net_wifi_comm_if_init(utils_timer_value(&timer));
  if(rc != 0){
     return -1;
  }             
  /*获取mac地址*/
  rc = net_query_wifi_mac(net.wifi.mac);
  if(rc != 0){
     return -1;
  } 
  
  log_debug("net task wifi init ok.\r\n");
  return 0; 
}

/* 函数：net_task_gsm_init
*  功能：初始化gsm
*  参数：无
*  返回：0 成功 其他：失败
*/ 
static int net_task_gsm_init(uint32_t timeout)
{
  int rc;
  
  utils_timer_t timer;
  
  if(net.gsm.is_initial == true){
     log_error("gsm already init ok.\r\n");
     return 0;
  }
  utils_timer_init(&timer,timeout,false);
  /*复位*/
    if (net.gsm.is_power_on == false) {
        rc = net_gsm_reset(utils_timer_value(&timer));
        if (rc != 0) {
            return -1;
        }
    }

  /*通信接口初始化*/
  rc = net_gsm_comm_if_init(utils_timer_value(&timer));
  if (rc != 0) {
     return -1;
  }
  /*获取SIM卡ID*/           
  rc = net_query_sim_id(net.gsm.sim_id,utils_timer_value(&timer));
  if(rc != 0){
     return -1;
  }
  if (net.gsm.sim_id[0] != '\0'){
      net.gsm.is_sim_exsit = true;
        /*gprs初始化*/
      rc = net_gsm_gprs_init(utils_timer_value(&timer));  
  }else{
      net.gsm.is_sim_exsit = false;
      rc = -1;
  }
  
  log_debug("net task gsm init ok.\r\n");
  return rc; 
}

static void net_task_config_wifi(uint32_t timeout)
{
  int rc;
  net_wifi_config_t wifi_new_config;
  bootloader_env_t  env;
  
  rc = bootloader_get_env(&env);
  log_assert(rc == 0 && env.status == BOOTLOADER_ENV_STATUS_VALID);
  
  net.wifi.config = *(net_wifi_config_t *)&env.reserved[ENV_WIFI_CONFIG_OFFSET];
  /*启动配网router*/
  rc = net_wifi_config(wifi_new_config.ssid,wifi_new_config.passwd,timeout); 
  if(rc == 0){
    log_debug("wifi new config ok.ssid:%s passwd:%s.\r\n",wifi_new_config.ssid,wifi_new_config.passwd);
    if((strcmp(net.wifi.config.ssid,wifi_new_config.ssid) != 0      ||
        strcmp(net.wifi.config.passwd,wifi_new_config.passwd) != 0) &&
        strlen(wifi_new_config.ssid) > 0){
       log_debug("config is different and valid.save.\r\n");
       wifi_new_config.status = NET_WIFI_CONFIG_STATUS_VALID;
       *(net_wifi_config_t *)&env.reserved[ENV_WIFI_CONFIG_OFFSET] = wifi_new_config;
       bootloader_save_env(&env);
       net.wifi.config = wifi_new_config;
    }else{
       log_debug("config is same or invalid.skip.\r\n");
    }
  }

  if (net.wifi.config.status == NET_WIFI_CONFIG_STATUS_VALID) {
      log_info("wifi will connect ssid:%s passwd:%s.\r\n",net.wifi.config.ssid,net.wifi.config.passwd);
  }
   net.wifi.is_config = true;
}

/*发送注册用的sim id和wifi mac等信息*/
static void net_task_send_hal_info_to_report_task()
{
 osStatus status;
 
 static net_hal_information_t hal_info;
 report_task_msg_t report_msg;

 hal_info.mac = net.wifi.mac;
 hal_info.sim_id = net.gsm.sim_id;
 /*发送硬件消息*/
 status = osMessagePut(report_task_net_hal_info_msg_q_id,(uint32_t)&hal_info,NET_TASK_PUT_MSG_TIMEOUT);
 if(status !=osOK){
    log_error("put report hal info  msg error:%d\r\n",status);
 } 
 
 report_msg.type = REPORT_TASK_MSG_NET_HAL_INFO;
 status = osMessagePut(report_task_msg_q_id,*(uint32_t *)&report_msg,NET_TASK_PUT_MSG_TIMEOUT);
 if(status !=osOK){
    log_error("put report hal info notify msg error:%d\r\n",status);
 } 
 
}



static void net_task_send_location_msg_to_report_task()
{
  osStatus status;
              
  /*发送位置消息*/
  status = osMessagePut(report_task_location_msg_q_id,(uint32_t)&net.gsm.location.base_main,NET_TASK_PUT_MSG_TIMEOUT);
  if(status !=osOK){
     log_error("put loaction msg error:%d\r\n",status);
  }  
  
  /*发送位置通知消息*/
  report_task_msg_t report_msg;
  report_msg.type = REPORT_TASK_MSG_LOCATION;
  status = osMessagePut(report_task_msg_q_id,*(uint32_t *)&report_msg,NET_TASK_PUT_MSG_TIMEOUT);
  if(status !=osOK){
     log_error("put loaction notify msg error:%d\r\n",status);
  } 
 
}


/* 函数：get_wifi_net_status
*  功能：获取wifi网络状态
*  参数：无 
*  返回：0：成功 其他：失败
*/  
net_status_t get_wifi_net_status() 
{
  return net.wifi.status;
}

/* 函数：get_wifi_net_status
*  功能：获取wifi网络状态
*  参数：无 
*  返回：0：成功 其他：失败
*/  
net_status_t get_gsm_net_status() 
{
  return net.gsm.status;
}



void net_task(void const *argument)
{
    int rc;
    uint8_t init_err_cnt = 0;
    osEvent  os_event;
    osStatus status;

    net_task_msg_t msg;
    display_task_msg_t display_msg;
    char ssid_temp[33];


    /*wifi和gsm轮询定时器*/
    wifi_query_timer_init();
    gsm_query_timer_init();
 
    /*上电处理流程*/
    /*网络连接初始化*/
    net_task_init();

    /*gsm上电*/
    rc = gsm_m6312_pwr_on();
    if (rc == 0) {
        net.gsm.is_power_on = true;
    }
 
init:
    rc = net_task_wifi_init(NET_TASK_WIFI_INIT_TIMEOUT);
    if (rc != 0) {
        log_error("wifi module init fail.\r\n"); 
    } else {
        net.wifi.is_initial = true;
        if (net.wifi.is_config == false){
            net_task_config_wifi(DEFAULT_WIFI_CONFIG_TIMEOUT);
            /*重新初始化wifi*/
            net.wifi.is_initial = false;
            goto init;
        }
    }
 
    rc = net_task_gsm_init(NET_TASK_GSM_INIT_TIMEOUT);
    if (rc != 0){
        log_error("gsm module init fail.\r\n");  
    } else {
        net.gsm.is_initial = true;
        net.gsm.status = NET_STATUS_ONLINE;
    }


    if (init_err_cnt < NET_TASK_NET_INIT_ERR_CNT_MAX && net.gsm.is_initial == false && net.wifi.is_initial == false){
        log_error("all device is err.\r\n %d S later retry...\r\n",NET_TASK_INIT_RETRY_DELAY);
        osDelay(NET_TASK_INIT_RETRY_DELAY);
        init_err_cnt ++;
        goto init;
    }
    init_err_cnt = 0;
    /*向上报任务提供设备信息 sim id 和 wifi mac*/
    net_task_send_hal_info_to_report_task();
 
    /*等待任务同步*/
    /*
    xEventGroupSync(tasks_sync_evt_group_hdl,TASKS_SYNC_EVENT_NET_TASK_RDY,TASKS_SYNC_EVENT_ALL_TASKS_RDY,osWaitForever);
    log_debug("net task sync ok.\r\n");
    */

    gsm_query_timer_start();
    wifi_query_timer_start();
 
  while(1){
  os_event = osMessageGet(net_task_msg_q_id,osWaitForever);
  if (os_event.status == osEventMessage){
      msg = *(net_task_msg_t*)&os_event.value.v; 
 
 
  /*如果收到检查WIFI网络状态*/
  if(msg.type == NET_TASK_MSG_QUERY_WIFI){ 
     /*wifi初始化完成和配网后才轮询wifi状态*/
     if(net.wifi.is_initial == true && net.wifi.is_config == true && net.wifi.config.status == NET_WIFI_CONFIG_STATUS_VALID){
       /*扫描配置的ssid是否存在，同时获取信号强度*/
       rc = net_query_wifi_rssi_level(net.wifi.config.ssid,&net.wifi.rssi,&net.wifi.level);      
       if(rc != 0){
          net.wifi.err_cnt ++;
       }
      /*查询wifi当前的连接的ssid*/
       memset(ssid_temp,0,33);
       rc = net_query_wifi_ap_ssid(ssid_temp);
       if(rc != 0){
          net.wifi.err_cnt ++;
       }else{
          net.wifi.err_cnt = 0;
          /*如果wifi当前的连接的ssid是配置的ssid*/
          if(strcmp(ssid_temp,net.wifi.config.ssid) == 0){
             net.wifi.status = NET_STATUS_ONLINE;
          }else{
             net.wifi.status = NET_STATUS_OFFLINE;
             /*如果wifi当前的连接的ssid不是配置的或者不存在*/
             /*如果存在配置的ssid，尝试连接*/
             if(net.wifi.rssi != 0){
                log_debug("wifi start conenct to %s\r\n",net.wifi.config.ssid);
                rc = net_wifi_connect_ap(net.wifi.config.ssid,net.wifi.config.passwd);
                 /*连接结果处理*/
                 if(rc != 0){
                    net.wifi.err_cnt ++;                    
                 }else{
                    net.wifi.status = NET_STATUS_ONLINE;
                }
              }else{
                 log_info("wifi ssid:%s is not exsit.\r\n",net.wifi.config.ssid);
              }
          }
        }
      }
    
    display_msg.type = DISPLAY_TASK_MSG_WIFI;
    if(net.wifi.level == 0 || net.wifi.status == NET_STATUS_OFFLINE){
       display_msg.value = 3;
       display_msg.blink = true;    
    }else{
       display_msg.value = net.wifi.level;
       display_msg.blink = false;         
    }
    status = osMessagePut(display_task_msg_q_id,*(uint32_t*)&display_msg,NET_TASK_PUT_MSG_TIMEOUT);
    if(status !=osOK){
       log_error("put display msg error:%d\r\n",status);
    } 
    
   if(net.wifi.err_cnt >= NET_TASK_ERR_CNT_MAX){
      net.wifi.err_cnt = 0;
      net.wifi.status = NET_STATUS_OFFLINE;
      net.wifi.is_initial = false;
      /*需要重新初始化*/
      goto init;
   }
   /*重新启动wifi检查定时器*/
   wifi_query_timer_start(); 
 }
  
  
  /*如果收到检查GSM网络状态*/
  if(msg.type == NET_TASK_MSG_QUERY_GSM){ 
     /*存在sim卡才轮询gsm的基站信息*/
     if(net.gsm.is_sim_exsit == true){
       /*清空上次基站信息*/
        memset(&net.gsm.location,0,sizeof(net_location_t));
        rc = net_query_gsm_location(&net.gsm.location.base_main,4);
        if(rc != 0){
           net.gsm.err_cnt ++;
        }else{
           net.gsm.err_cnt = 0;
           net_task_send_location_msg_to_report_task();
        }
     }
     
   if(net.gsm.err_cnt >= NET_TASK_ERR_CNT_MAX){
      net.gsm.err_cnt = 0;
      net.gsm.status = NET_STATUS_OFFLINE;
      net.gsm.is_initial = false;
      /*需要重新初始化*/
      goto init;
   }
 /*重新启动GSM检查定时器*/
  gsm_query_timer_start(); 
 }
  
 
 
 }
 }
 }