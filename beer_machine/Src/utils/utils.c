#include "stdio.h"
#include "string.h"
#include "stdint.h"
#include "stdarg.h"
#include "stdbool.h"
#include "md5.h"
#include "utils.h"
#include "cmsis_os.h"
#include "log.h"
#define  LOG_MODULE_LEVEL    LOG_LEVEL_DEBUG
#define  LOG_MODULE_NAME     "[utils]"

/*字节转换成HEX字符串*/
 void bytes_to_hex_str(const char *src,char *dest,uint16_t src_len)
 {
 char temp;
 char hex_digital[] = {'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};
 for (uint16_t i = 0; i < src_len; i++){  
 temp = src[i];  
 dest[2 * i] = hex_digital[(temp >> 4) & 0x0f ];  
 dest[2 * i + 1] = hex_digital[temp & 0x0f];  
 }
 dest[src_len * 2] = '\0';
}


/* 函数名：utils_build_sign
*  功能：  签名算法
*  参数：  sign 签名结果缓存 
*  返回：  0：成功 其他：失败 
*/ 
int utils_build_sign(char *sign,const int cnt,...)
{ 
#define  SIGN_SRC_BUFFER_SIZE_MAX               300
 int size = 0;
 char sign_src[SIGN_SRC_BUFFER_SIZE_MAX] = { 0 };
 char md5_hex[16];
 char md5_str[33];
 va_list ap;
 char *temp;
 
 va_start(ap, cnt);
 /*组合MD5的源数据,根据输入数据*/  
 for(uint8_t i=0; i < cnt; i++){
 temp = va_arg(ap,char*);
 /*保证数据不溢出*/
 if(size + strlen(temp) >= SIGN_SRC_BUFFER_SIZE_MAX){
 return -1;
 }
 size += strlen(temp);
 strcat(sign_src,temp);
 if(i < cnt - 1){
 strcat(sign_src,"&");
 }
 }
 
 /*第1次MD5*/
 md5(sign_src,strlen(sign_src),md5_hex);
 /*把字节装换成HEX字符串*/
 bytes_to_hex_str(md5_hex,md5_str,16);
 /*第2次MD5*/
 md5(md5_str,32,md5_hex);
 bytes_to_hex_str(md5_hex,md5_str,16);
 strcpy(sign,md5_str);
 
 return 0;
}

/* 函数名：utils_build_url
*  功能：  构造新的啤酒机需要的url格式
*  参数：  url 新url缓存 
*  参数：  size 缓存大小 
*  参数：  origin_url 原始url 
*  参数：  sn 串号 
*  参数：  sign 签名 
*  参数：  source 源 
*  参数：  timestamp 时间戳 
*  返回：  0：成功 其他：失败 
*/ 
int utils_build_url(char *url,const int size,const char *origin_url,const char *sn,const char *sign,const char *source,const char *timestamp)
{
snprintf(url,size,"%s?sn=%s&sign=%s&source=%s&timestamp=%s",origin_url,sn,sign,source,timestamp);
if(strlen(url) == size - 1){
log_error("url size:%d too large.\r\n",size - 1); 
return -1;
}
return 0;
}
 

/* 函数：utils_build_form_data
*  功能：构造form-data格式数据
*  参数：form_data form-data缓存 
*  参数：size 缓存大小 
*  参数：boundary 分界标志 
*  参数：cnt form-data数据个数 
*  参数：... form-data参数列表 
*  参数：source 源 
*  参数：timestamp 时间戳 
*  返回：0：成功 其他：失败 
*/ 
int utils_build_form_data(char *form_data,const int size,const char *boundary,const int cnt,...)
{
 int put_size;
 int temp_size;  
 va_list ap;
 form_data_t *temp;

 put_size = 0;
 va_start(ap, cnt);
 
 /*组合输入数据*/  
 for(uint8_t i=0; i < cnt; i++){
 
 temp = va_arg(ap,form_data_t*);
 /*添加boundary 和 name 和 value*/
 snprintf(form_data + put_size ,size - put_size,"--%s\r\nContent-Disposition: form-data; name=%s\r\n\r\n%s\r\n",boundary,temp->name,temp->value);
 temp_size = strlen(form_data);
 /*保证数据完整*/
 if(temp_size >= size - 1){
 log_error("form data size :%d is too large.\r\n",temp_size);
 return -1;
 } 
 put_size = strlen(form_data);
 }
 /*添加结束标志*/
 snprintf(form_data + put_size,size - put_size,"--%s--\r\n",boundary);
 put_size = strlen(form_data);
 if(put_size >= size - 1){
 log_error("form data size :%d is too large.\r\n",temp_size);
 return -1;  
 }
 
 return 0; 
}


/* 函数：utils_timer_init
*  功能：自定义定时器初始化
*  参数：timer 定时器指针
*  参数：timeout 超时时间
*  参数：up 定时器方向-向上计数-向下计数
*  返回: 0：成功 其他：失败
*/ 
int utils_timer_init(utils_timer_t *timer,uint32_t timeout,bool up)
{
if(timer == NULL){
return -1;
}

timer->up = up;
timer->start = osKernelSysTick();  
timer->value = timeout;  

return 0;
}

/* 函数：utils_timer_value
*  功能：定时器现在的值
*  返回：>=0：现在时间值 其他：失败
*/ 
int utils_timer_value(utils_timer_t *timer)
{
uint32_t time_elapse;

if(timer == NULL){
   return -1;
}  
time_elapse = osKernelSysTick() - timer->start; 

if(timer->up == true){
   return  timer->value > time_elapse ? time_elapse : timer->value;
}

return  timer->value > time_elapse ? timer->value - time_elapse : 0; 
}

/* 函数：utils_get_str_addr_by_num
*  功能：获取字符串中第num个str的地址
*  参数：buffer 字符串地址
*  参数：str    查找的字符串
*  参数：num    第num个str
*  参数：addr   第num个str的地址
*  返回：0：成功 其他：失败
*/ 
int utils_get_str_addr_by_num(char *buffer,const char *str,const uint8_t num,char **addr)
{
 uint8_t cnt = 0;
 uint16_t str_size;
 
 char *search,*temp;
 
 temp = buffer;
 str_size = strlen(str);
 
 while(cnt < num ){
       search = strstr(temp,str);
       if(search == NULL){
          return -1;
 }
 temp = search + str_size;
 cnt ++;
 }
 *addr = search;
 
 return 0; 
}

/* 函数：utils_get_str_value_by_num
*  功能：获取字符串中第num和第num+1个str之间的字符串
*  参数：buffer 字符串地址
*  参数：dst    存储地址
*  参数：str    字符串
*  参数：num    第num个字符串
*  返回：0：成功 其他：失败
*/
int utils_get_str_value_by_num(char *buffer,char *dst,const char *str,uint8_t num)
{
 int rc;
 uint16_t str_size,str_start_size;
 
 char *addr_start,*addr_end;
 
 rc = utils_get_str_addr_by_num(buffer,str,num,&addr_start);
 if(rc != 0){
    return -1;
 }
 /*先查找原来的字符串，没有就查询结束符*/
 rc = utils_get_str_addr_by_num(buffer,str,num + 1,&addr_end);
 if(rc != 0){
    rc = utils_get_str_addr_by_num(addr_start,"\r\n",1,&addr_end);
    if(rc != 0){
       return -1;
    }
 }
 
 str_start_size = strlen(str);
 if(addr_end < addr_start + str_start_size){
    return -1;
 }
 
 str_size = addr_end - addr_start - str_start_size;
 
 memcpy(dst,addr_start + str_start_size,str_size);
 dst[str_size] = '\0';
 
 return 0; 
}