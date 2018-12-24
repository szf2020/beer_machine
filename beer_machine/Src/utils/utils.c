#include "stdio.h"
#include "string.h"
#include "stdint.h"
#include "stdarg.h"
#include "md5.h"
#include "utils.h"
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
 

/* 函数名：utils_build_form_data
*  功能：  构造form-data格式数据
*  参数：  form_data form-data缓存 
*  参数：  size 缓存大小 
*  参数：  boundary 分界标志 
*  参数：  cnt form-data数据个数 
*  参数：  ... form-data参数列表 
*  参数：  source 源 
*  参数：  timestamp 时间戳 
*  返回：  0：成功 其他：失败 
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