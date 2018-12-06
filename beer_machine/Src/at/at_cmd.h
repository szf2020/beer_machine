#ifndef  __AT_CMD_H__
#define  __AT_CMD_H__

#define  AT_CMD_REQUEST_BUFFER_SIZE         1024
#define  AT_CMD_RESPONSE_BUFFER_SIZE        1600

#define  AT_CMD_SELECT_TIMEOUT              5
#define  AT_CMD_SEND_TIMEOUT                200
#define  AT_CMD_WEITE_TIMEOUT               300 

#define  AT_CMD_ERR_STR_LEN                 20
#define  AT_CMD_ERR_BREAK_STR               "@"

#define  AT_CMD_EOF_STR                     "\r\n"
#define  AT_CMD_BREAK_STR                   ","
#define  AT_CMD_TOLON_STR                   ":"
#define  AT_CMD_EQUAL_STR                   "="
#define  AT_CMD_SPACE_STR                   " "

#include "stdint.h"
#include "stdbool.h"

typedef enum
{
AT_CMD_RESPONSE_STATUS_POS_IN_HEAD,
AT_CMD_RESPONSE_STATUS_POS_IN_TAIL
}at_cmd_response_status_pos_t;

typedef struct
{
at_cmd_response_status_pos_t response_status_pos;
char *request;
char *response;
const char *response_ok;
const char *response_err;
uint16_t  request_size;
uint16_t  response_size;
uint16_t  request_max;
uint16_t  response_max;
uint16_t  timeout;
int32_t   err_code;
bool      response_len_changed;
}at_cmd_t;


int at_cmd_execute(int handle,at_cmd_t *cmd);



#endif