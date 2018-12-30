#include "stdbool.h"
#include "stddef.h"
#include "stdint.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "utils.h"
#include "cmsis_os.h"
#include "gsm_m6312.h"
#include "wifi_8710bx.h"
#include "socket.h"
#include "net.h"
#include "log.h"
#define  LOG_MODULE_LEVEL    LOG_LEVEL_DEBUG
#define  LOG_MODULE_NAME     "[net]"

/* 函数：net_init
*  功能：网络初始化
*  参数：无
*  返回：0：成功 其他：失败
*/
int net_init(void)
{
 if(wifi_8710bx_serial_hal_init() !=0 || gsm_m6312_serial_hal_init() != 0){
 log_assert(0);
 }
 /*socket*/
 socket_init(); 
 return 0;
}

/* 函数：net_wifi_comm_if_init
*  功能：初始化wifi通信接口
*  参数：无
*  返回：0 成功 其他：失败
*/ 
int net_wifi_comm_if_init(uint32_t timeout)
{
 int rc;
 utils_timer_t timer;
 
 utils_timer_init(&timer,timeout,false);
 
 while(utils_timer_value(&timer) > 0){
       rc = wifi_8710bx_set_echo(WIFI_8710BX_ECHO_OFF);
       if(rc == 0){
          break;
       }
 }
 while(utils_timer_value(&timer) > 0){
       rc = wifi_8710bx_set_mode(WIFI_8710BX_STATION_MODE);
       if(rc == 0){
          break;
       }
 }
 
 if(utils_timer_value(&timer) == 0){
    return -1;
 }

 return 0;
}


/* 函数：net_wifi_reset
*  功能：复位WiFi
*  返回：0  成功 其他：失败
*/ 
int net_wifi_reset(uint32_t timeout)
{
  int rc;
  utils_timer_t timer;
  
  utils_timer_init(&timer,timeout,false);
  
  while(utils_timer_value(&timer) > 0){
        rc = wifi_8710bx_reset();
        if(rc == 0){
           break;
        }
  }
  if(utils_timer_value(&timer) == 0){
     return -1;
  }
  return 0;
}

/* 函数：net_wifi_switch_to_station
*  功能：wifi切换到station模式
*  参数：无
*  返回：0 成功 其他：失败
*/ 
int net_wifi_switch_to_station(void)
{
 return wifi_8710bx_set_mode(WIFI_8710BX_STATION_MODE);

}

/* 函数：net_wifi_switch_to_ap
*  功能：wifi切换到AP模式
*  参数：无
*  返回：0 成功 其他：失败
*/ 
int net_wifi_switch_to_ap(void)
{
 return wifi_8710bx_set_mode(WIFI_8710BX_AP_MODE);
}

/* 函数：net_wifi_switch_to_ap
*  功能：wifi切换到AP模式
*  参数：无
*  返回：0 成功 其他：失败
*/ 
int net_wifi_connect_ap(char *ssid,char *passwd)
{
 return wifi_8710bx_connect_ap(ssid,passwd);
}

/* 函数：net_query_wifi_mac
*  功能：询问WiFi mac 地址
*  参数：mac wifi mac
*  返回：0：成功 其他：失败
*/ 
int net_query_wifi_mac(char *mac)
{
 int rc;
 wifi_8710bx_device_t wifi;
 
 rc = wifi_8710bx_get_wifi_device(&wifi);
 if(rc != 0){
    return -1;
 }
 
 strcpy(mac,wifi.device.mac);
 return 0;
}

/* 函数：net_query_wifi_ap_ssid
*  功能：询问WiFi目前连接的ap
*  参数：mac wifi mac
*  返回：0：成功 其他：失败
*/ 
int net_query_wifi_ap_ssid(char *ssid)
{
 int rc;
 wifi_8710bx_device_t wifi;
 
 rc = wifi_8710bx_get_wifi_device(&wifi);
 if(rc != 0){
    return -1;
 }
 
 strcpy(ssid,wifi.ap.ssid);
 return 0;
}



/* 函数：net_query_wifi_rssi_level
*  功能：询问WiFi的level值
*  参数：rssi值指针
*  返回：0  成功 其他：失败
*/ 
int net_query_wifi_rssi_level(char *ssid,int *rssi,int *level)
{
 int rc;
 /*尝试扫描ssid*/
 rc = wifi_8710bx_get_ap_rssi(ssid,rssi);
 /*执行失败*/
 if(rc != 0){
 return -1;
 }
 /*没有发现ssid*/  
 if(*rssi == 0){
    *level = 0;
 return 0;
 }
 /*设置wifi信号等级*/
 if(*rssi >= NET_WIFI_RSSI_LEVEL_3){
 *level = 3; 
 }else if(*rssi >= NET_WIFI_RSSI_LEVEL_2){
 *level = 2; 
 }else {
 *level = 1;  
 }
 
 return 0; 
}



/* 函数：net_gsm_comm_if_init
*  功能：gsm模块通信接口初始化
*  参数：无
*  返回：0 成功 其他：失败
*/ 
int net_gsm_comm_if_init(uint32_t timeout)
{
 int rc;
 utils_timer_t timer;
 
 utils_timer_init(&timer,timeout,false);
 /*关闭回显*/
 while(utils_timer_value(&timer) > 0){
       rc = gsm_m6312_set_echo(GSM_M6312_ECHO_OFF); 
       if(rc == 0){
          break;
       }
 }
 
 /*关闭信息主动上报*/
 while(utils_timer_value(&timer) > 0){
       rc = gsm_m6312_set_report(GSM_M6312_REPORT_OFF);
       if(rc == 0){
          break;
       }
 }  
  
 /*设置SIM卡注册主动回应位置信息*/
 while(utils_timer_value(&timer) > 0){
       rc = gsm_m6312_set_reg_echo(GSM_M6312_REG_ECHO_ON);
       if(rc == 0){
          break; 
       } 
 }
 if(utils_timer_value(&timer) == 0){
    log_error("gsm comm if init error.\r\n");
    return -1;
 }
 
  log_debug("gsm comm if init ok.\r\n");
  return 0;
}

/* 函数：net_gsm_gprs_init
*  功能：gprs网络初始化
*  参数：无
*  返回：0 成功 其他：失败
*/ 
int net_gsm_gprs_init(uint32_t timeout)
{
 int rc;
 utils_timer_t timer;
 operator_name_t operator_name;
 gsm_m6312_apn_t apn;
 
 utils_timer_init(&timer,timeout,false);

 /*去除附着网络*/
 while(utils_timer_value(&timer) > 0){
      rc = gsm_m6312_set_attach_status(GSM_M6312_NOT_ATTACH); 
      if(rc == 0){
         break;
      }
 }

 /*设置多连接*/
 while(utils_timer_value(&timer) > 0){
       rc = gsm_m6312_set_connect_mode(GSM_M6312_CONNECT_MODE_MULTIPLE); 
      if(rc == 0){
         break;
      }
 }
 /*设置运营商格式*/
 /*
 rc = gsm_m6312_set_auto_operator_format(GSM_M6312_OPERATOR_FORMAT_SHORT_NAME);
 if(rc != 0){
 goto err_exit;
 }
*/

 /*设置接收缓存*/
 while(utils_timer_value(&timer) > 0){
       rc = gsm_m6312_config_recv_buffer(GSM_M6312_RECV_BUFFERE); 
       if(rc == 0){
          break;
      }
 }
 
 /*获取运营商*/
 while(utils_timer_value(&timer) > 0){
       rc = gsm_m6312_get_operator(&operator_name);
       if(rc == 0){
       break;
       }
 }

 /*赋值apn值*/
 if(operator_name == OPERATOR_CHINA_MOBILE){
    apn =  GSM_M6312_APN_CMNET;
 }else{
    apn = GSM_M6312_APN_UNINET;
 }

 /*设置apn*/
 while(utils_timer_value(&timer) > 0){
       rc = gsm_m6312_set_apn(apn); 
       if(rc == 0){
          break;
       }
 }

 /*附着网络*/
 while(utils_timer_value(&timer) > 0){
       rc = gsm_m6312_set_attach_status(GSM_M6312_ATTACH); 
       if(rc == 0){
       break;
       }
 }
 
 /*激活网络*/
 while(utils_timer_value(&timer) > 0){
       rc = gsm_m6312_set_active_status(GSM_M6312_ACTIVE); 
       if(rc == 0){
          break;
       }
 }
 
 /*超时判断*/
 if(utils_timer_value(&timer) == 0){
    log_error("net gsm gprs init err.\r\n");
    return -1;
 }
 
 log_debug("net gsm gprs init ok.\r\n");
 return 0;
}


/* 函数：net_gsm_reset
*  功能：复位gsm
*  返回：0  成功 其他：失败
*/ 
int net_gsm_reset(uint32_t timeout)
{
  int rc;
  utils_timer_t timer;
 
  utils_timer_init(&timer,timeout,false);

  while(utils_timer_value(&timer) > 0){
        rc = gsm_m6312_pwr_off();
        if(rc == 0){
           break;
        }
  }
  while(utils_timer_value(&timer) > 0){
        rc = gsm_m6312_pwr_on();
        if(rc == 0){
           break;
        }
  } 

  if(utils_timer_value(&timer) == 0){
     return -1;
  }
  return 0; 
}


/* 函数：net_module_query_sim_id
*  功能：查询sim id
*  参数：sim_id sim id指针
*  参数：timeout 超时时间
*  返回：0 成功 其他：失败
*/ 
int net_query_sim_id(char *sim_id,uint32_t timeout)
{
 int rc;
 sim_card_status_t sim_card_status;
 utils_timer_t timer;
 
 utils_timer_init(&timer,timeout,false);
 
 /*默认为空*/
 sim_id[0]= '\0';
 
 while(utils_timer_value(&timer) > 0){
 /*获取sim卡状态*/
 rc = gsm_m6312_get_sim_card_status(&sim_card_status);
 if(rc != 0){
    continue; 
 }
 
 /*sim卡是否就位*/
 if(sim_card_status != SIM_CARD_STATUS_READY){
    break;  
 }
 rc = gsm_m6312_get_sim_card_id(sim_id);
 if(rc != 0){
    continue;
 }
 break;
 }
 
 if(utils_timer_value(&timer) == 0){
    return -1;
 }
 
 return 0;
}
 
/* 函数：net_query_gsm_location
*  功能：询问gsm基站位置信息
*  参数：location位置信息指针
*  返回：0 成功 其他：失败
*/ 
int net_query_gsm_location(gsm_base_t *base,uint8_t cnt)
{
 int rc;
 gsm_m6312_register_t register_info;
 gsm_m6312_assist_base_t assist_base;
 
  /*SIM卡是否注册*/  
 rc = gsm_m6312_get_reg_location(&register_info);
 if(rc != 0){
    goto err_exit; 
 }
/*没有注册直接返回*/ 
 if(register_info.status == GSM_M6312_STATUS_NO_REGISTER){
    return 0;
 }
 /*注册后就有位置信息*/
 strcpy(base[0].lac,register_info.base.lac);
 strcpy(base[0].ci,register_info.base.ci);
 /*获取主机站信号强度信息*/
 rc = gsm_m6312_get_rssi(base[0].rssi);
 if(rc != 0){
    goto err_exit; 
 }
 /*获取辅助基站信息*/
 memset(&assist_base,0,sizeof(assist_base));
 rc = gsm_m6312_get_assist_base_info(&assist_base);
 if(rc != 0){
 goto err_exit; 
 }
 for(uint8_t i = 0; i < 3; i++){
 strcpy(base[i + 1].lac,assist_base.base[i].lac);
 strcpy(base[i + 1].ci,assist_base.base[i].ci);
 strcpy(base[i + 1].rssi,assist_base.base[i].rssi);
 }
 return 0;
 
err_exit:
  return -1;
  
}
