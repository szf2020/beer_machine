#ifndef  __REPORT_TASK_H__
#define  __REPORT_TASK_H__



extern osThreadId  report_task_handle;
extern osMessageQId report_task_msg_q_id;
extern osMessageQId report_task_location_msg_q_id;
extern osMessageQId report_task_net_hal_info_msg_q_id;

void report_task(void const * argument);


#define  REPORT_TASK_PUT_MSG_TIMEOUT             5
#define  REPORT_TASK_GET_MSG_INTERVAL            500

#define  REPORT_TASK_REPORT_LOG_INTERVAL         (60 * 1000) /*默认日志数据上报间隔*/
#define  REPORT_TASK_RETRY_INTERVAL              10000       /*重试间隔*/
#define  REPORT_TASK_DEVICE_ACTIVE_RETRY         10

#define  URL_LOG                                 "http://mh1597193030.uicp.cn:35787/device/log/submit"
#define  URL_ACTIVE                              "http://mh1597193030.uicp.cn:35787/device/info/active"
#define  URL_FAULT                               "http://mh1597193030.uicp.cn:35787/device/info/active"
#define  BOUNDARY                                "##wkxboot##"
#define  KEY                                     "meiling-beer"
#define  SOURCE                                  "coolbeer"
#define  MODEL                                   "pijiuji"
#define  FW_VERSION                              "v1.0.0"


typedef enum
{
REPORT_TASK_MSG_CONFIG_DEVICE,
REPORT_TASK_MSG_CAPACITY,
REPORT_TASK_MSG_PRESSURE,
REPORT_TASK_MSG_TEMPERATURE,
REPORT_TASK_MSG_REPORT_LOG,
REPORT_TASK_MSG_REPORT_FAULT,
REPORT_TASK_MSG_TEMPERATURE_SENSOR_FAULT,
REPORT_TASK_MSG_PRESSURE_SENSOR_FAULT,
REPORT_TASK_MSG_CAPACITY_SENSOR_FAULT
}report_task_msg_type_t;

typedef struct
{
uint8_t type;
union {
int16_t temperature;
uint8_t capacity;
uint8_t pressure;
};
}report_task_msg_t;



















#endif