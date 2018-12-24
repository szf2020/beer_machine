#ifndef  __TOOL_H__
#define  __TOOL_H__



typedef struct
{
const char *name;
const char *value;
}form_data_t;



/*字节转换成HEX字符串*/
 void bytes_to_hex_str(const char *src,char *dest,uint16_t src_len);


 /* 函数名：utils_build_sign
*  功能：  签名算法
*  参数：  sign 签名结果缓存 
*  返回：  0：成功 其他：失败 
*/ 
int utils_build_sign(char *sign,const int cnt,...);  



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
int utils_build_url(char *url,const int size,const char *origin_url,const char *sn,const char *sign,const char *source,const char *timestamp);   
   
   
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
int utils_build_form_data(char *form_data,const int size,const char *boundary,const int cnt,...);  
   
   
   
   
   
   
   
   
   
#endif