#include <stdio.h>
#include <string.h>
#include "connection.h"

#include "iot_import.h"
#include "iotx_hal_internal.h"

static uint64_t _freertos_get_time_ms(void)
{
 return osKernelSysTick();
}

static uint64_t _freertos_time_left(uint64_t t_end, uint64_t t_now)
{
    uint64_t t_left;

    if (t_end > t_now) {
        t_left = t_end - t_now;
    } else {
        t_left = 0;
    }

    return t_left;
}

uintptr_t HAL_TCP_Establish(const char *host, uint16_t port)
{
   int rc;
   hal_info("establish tcp connection with server(host='%s', port=[%u])", host, port);
   rc = connection_connect(host,port,CONNECTION_PROTOCOL_TCP);
   if(rc < 0){
   hal_err("establish tcp connection err.\r\n"); 
   return -1;
   }
   return (uintptr_t)rc;
}


int HAL_TCP_Destroy(uintptr_t fd)
{
    int rc;
    rc = connection_disconnect((int)fd);
    if (0 != rc) {
    hal_err("close socket error");
    return -1;
    }
    return 0;
}


int32_t HAL_TCP_Write(uintptr_t fd, const char *buf, uint32_t len, uint32_t timeout_ms)
{
    int ret;
    ret = connection_send((int)fd,buf,len,timeout_ms);
    if(ret < 0){
    hal_err("socket send error");   
    }
    return ret;
}


int32_t HAL_TCP_Read(uintptr_t fd, char *buf, uint32_t len, uint32_t timeout_ms)
{
    int ret;
    ret = connection_recv((int)fd,buf,len,timeout_ms);
    if(ret < 0){
    hal_err("socket recv error");   
    }
    return ret;
}