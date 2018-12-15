/*
 * Copyright (C) 2015-2018 Alibaba Group Holding Limited
 */





#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "cmsis_os.h"
#include "SEGGER_RTT.h"

#include "iot_import.h"
#include "iotx_hal_internal.h"
//#include "kv.h"

#define __DEMO__

#ifdef __DEMO__
    char _product_key[PRODUCT_KEY_LEN + 1];
    char _product_secret[PRODUCT_SECRET_LEN + 1];
    char _device_name[DEVICE_NAME_LEN + 1];
    char _device_secret[DEVICE_SECRET_LEN + 1];
#endif

void *HAL_MutexCreate(void)
{
 osMutexId mutex_id;
 osMutexDef(mutex);
 mutex_id = osMutexCreate(osMutex(mutex));
 if(mutex_id == NULL){
 hal_err("create mutex err.\r\n");
 return NULL;
 }
 osMutexRelease(mutex_id);
  
 return mutex_id;
}

void HAL_MutexDestroy(_IN_ void *mutex)
{  
  if (!mutex) {
  hal_warning("mutex want to destroy is NULL!");
  return;
  }
  if(osOK != osMutexDelete((osMutexId)mutex)) {
  hal_err("destroy mutex failed");
  }
}

void HAL_MutexLock(_IN_ void *mutex)
{
 osMutexWait((osMutexId )mutex,osWaitForever);
}

void HAL_MutexUnlock(_IN_ void *mutex)
{
 osMutexRelease((osMutexId)mutex);
}

void *HAL_Malloc(_IN_ uint32_t size)
{
 return  pvPortMalloc(size);
}

void *HAL_Realloc(_IN_ void *ptr, _IN_ uint32_t size)
{
 return NULL;
}

void *HAL_Calloc(_IN_ uint32_t nmemb, _IN_ uint32_t size)
{
  char *temp;
  uint32_t malloc_size = nmemb * size;
  temp = pvPortMalloc(malloc_size);
  if(temp ){
  memset(temp,0,malloc_size);  
  return temp; 
  }
  
  return NULL;
}

void HAL_Free(_IN_ void *ptr)
{
    vPortFree(ptr);
}

#ifdef __APPLE__
uint64_t HAL_UptimeMs(void)
{
    struct timeval tv = { 0 };
    uint64_t time_ms;

    gettimeofday(&tv, NULL);

    time_ms = tv.tv_sec * 1000 + tv.tv_usec / 1000;

    return time_ms;
}
#else
uint64_t HAL_UptimeMs(void)
{
 return osKernelSysTick();
}

char *HAL_GetTimeStr(_IN_ char *buf, _IN_ int len)
{
 return NULL;
}
#endif

void HAL_SleepMs(_IN_ uint32_t ms)
{
 osDelay(ms);
}

void HAL_Srandom(uint32_t seed)
{
// srandom(seed);
}

uint32_t HAL_Random(uint32_t region)
{
  return 0;//(region > 0) ? (random() % region) : 0;
}

int HAL_Snprintf(_IN_ char *str, const int len, const char *fmt, ...)
{
    va_list args;
    int     rc;

    va_start(args, fmt);
    rc = vsnprintf(str, len, fmt, args);
    va_end(args);

    return rc;
}

int HAL_Vsnprintf(_IN_ char *str, _IN_ const int len, _IN_ const char *format, va_list ap)
{
 return vsnprintf(str, len, format, ap);
}

void HAL_Printf(_IN_ const char *fmt, ...)
{
int SEGGER_RTT_vprintf(unsigned BufferIndex, const char * sFormat, va_list * pParamList);
    va_list args;

    va_start(args, fmt);
    SEGGER_RTT_vprintf(0, fmt, &args);
    va_end(args);
}
/*
int HAL_GetPartnerID(char *pid_str)
{
  memset(pid_str, 0x0, PID_STRLEN_MAX);
  strcpy(pid_str, "meiling");
  return strlen(pid_str);
}

int HAL_GetModuleID(char *mid_str)
{
 memset(mid_str, 0x0, MID_STRLEN_MAX);
 strcpy(mid_str, "beer_machine");
 return strlen(mid_str);
}


char *HAL_GetChipID(_OU_ char *cid_str)
{
    memset(cid_str, 0x0, HAL_CID_LEN);
    strncpy(cid_str, "stm32f405rgt6", HAL_CID_LEN);
    cid_str[HAL_CID_LEN - 1] = '\0';
    return cid_str;
}


int HAL_GetDeviceID(_OU_ char *device_id)
{
    memset(device_id, 0x0, DEVICE_ID_LEN);
#ifdef __DEMO__
    HAL_Snprintf(device_id, DEVICE_ID_LEN, "%s.%s", _product_key, _device_name);
    device_id[DEVICE_ID_LEN - 1] = '\0';
#endif

    return strlen(device_id);
}

int HAL_SetProductKey(_IN_ char *product_key)
{
    int len = strlen(product_key);
#ifdef __DEMO__
    if (len > PRODUCT_KEY_LEN) {
        return -1;
    }
    memset(_product_key, 0x0, PRODUCT_KEY_LEN + 1);
    strncpy(_product_key, product_key, len);
#endif
    return len;
}


int HAL_SetDeviceName(_IN_ char *device_name)
{
    int len = strlen(device_name);
#ifdef __DEMO__
    if (len > DEVICE_NAME_LEN) {
        return -1;
    }
    memset(_device_name, 0x0, DEVICE_NAME_LEN + 1);
    strncpy(_device_name, device_name, len);
#endif
    return len;
}


int HAL_SetDeviceSecret(_IN_ char *device_secret)
{
    int len = strlen(device_secret);
#ifdef __DEMO__
    if (len > DEVICE_SECRET_LEN) {
        return -1;
    }
    memset(_device_secret, 0x0, DEVICE_SECRET_LEN + 1);
    strncpy(_device_secret, device_secret, len);
#endif
    return len;
}


int HAL_SetProductSecret(_IN_ char *product_secret)
{
    int len = strlen(product_secret);
#ifdef __DEMO__
    if (len > PRODUCT_SECRET_LEN) {
        return -1;
    }
    memset(_product_secret, 0x0, PRODUCT_SECRET_LEN + 1);
    strncpy(_product_secret, product_secret, len);
#endif
    return len;
}

int HAL_GetProductKey(_OU_ char *product_key)
{
    int len = strlen(_product_key);
    memset(product_key, 0x0, PRODUCT_KEY_LEN);

#ifdef __DEMO__
    strncpy(product_key, _product_key, len);
#endif

    return len;
}

int HAL_GetProductSecret(_OU_ char *product_secret)
{
    int len = strlen(_product_secret);
    memset(product_secret, 0x0, PRODUCT_SECRET_LEN);

#ifdef __DEMO__
    strncpy(product_secret, _product_secret, len);
#endif

    return len;
}

int HAL_GetDeviceName(_OU_ char *device_name)
{
    int len = strlen(_device_name);
    memset(device_name, 0x0, DEVICE_NAME_LEN);

#ifdef __DEMO__
    strncpy(device_name, _device_name, len);
#endif

    return strlen(device_name);
}

int HAL_GetDeviceSecret(_OU_ char *device_secret)
{
    int len = strlen(_device_secret);
    memset(device_secret, 0x0, DEVICE_SECRET_LEN);

#ifdef __DEMO__
    strncpy(device_secret, _device_secret, len);
#endif

    return len;
}
*/

