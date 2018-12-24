#ifndef  __REPORT_TASK_H__
#define  __REPORT_TASK_H__



extern osThreadId  report_task_handle;
extern osMessageQId report_task_msg_q_id;


typedef enum
{
REPORT_TASK_MSG_CAPACITY,
REPORT_TASK_MSG_PRESSURE,
REPORT_TASK_MSG_TEMPERATURE,
REPORT_TASK_MSG_LOCATION,
REPORT_TASK_MSG_REPORT_LOG,
REPORT_TASK_MSG_CONFIG_DEVICE
}report_task_msg_type_t;

typedef struct
{
char lac[7];
char cid[7];
}location_t;

typedef struct
{
location_t value[3];
}report_location_t;

typedef struct
{
uint8_t type;
union {
int16_t temperature;
uint8_t capacity;
uint8_t pressure;
report_location_t location;
};
}report_task_msg_t;

void report_task(void const * argument);

















#endif