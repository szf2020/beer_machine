#ifndef  __TOOL_H__
#define  __TOOL_H__



typedef struct
{
const char *name;
const char *value;
}form_data_t;

typedef struct
{
bool up;
uint32_t start;
uint32_t value;
}utils_timer_t;


#ifndef   IS_POWER_OF_TWO
#define   IS_POWER_OF_TWO(A)   (((A) != 0) && ((((A) - 1) & (A)) == 0))
#endif

#ifndef   MIN
#define   MIN(A,B)             ((A) > (B) ? (B) :(A))
#endif

#ifndef   MAX
#define   MAX(A,B)             ((A) > (B) ? (A) :(B))
#endif






/*字节转换成HEX字符串*/
 void bytes_to_hex_str(const char *src,char *dest,uint16_t src_len);


/* 函数：utils_build_sign
*  功能：  签名算法
*  参数：  sign 签名结果缓存 
*  返回：  0：成功 其他：失败 
*/ 
int utils_build_sign(char *sign,const int cnt,...);  



/* 函数：  utils_build_url
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
   
   
/* 函数：  utils_build_form_data
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
   
   
/* 函数：utils_timer_init
*  功能：自定义定时器初始化
*  参数：timer 定时器指针
*  参数：timeout 超时时间
*  参数：up 定时器方向-向上计数-向下计数
*  返回: 0：成功 其他：失败
*/ 
int utils_timer_init(utils_timer_t *timer,uint32_t timeout,bool up);

/* 函数：utils_timer_value
*  功能：定时器现在的值
*  返回：>=0：现在时间值 其他：失败
*/ 
int utils_timer_value(utils_timer_t *timer);

/* 函数：utils_get_str_addr
*  功能：获取字符串中第num个flag的地址
*  参数：buffer 字符串地址
*  参数：flag   标志
*  参数：num    第num个flag
*  参数：addr   第num个flag的地址
*  返回：>=0：成功 其他：失败
*/ 
int utils_get_str_addr(char *buffer,const char *flag,const uint8_t num,char **addr);

/* 函数：utils_get_str_value
*  功能：获取字符串中第num个flag和第num+1flag之间的字符串
*  参数：buffer 字符串地址
*  参数：dst    存储地址
*  参数：flag   标志
*  参数：num    第num个flag
*  返回：>=0：成功 其他：失败
*/
int utils_get_str_value(char *buffer,char *dst,const char *flag,const uint8_t num);


   
   
   
   
   
   
#endif