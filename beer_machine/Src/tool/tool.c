#include "string.h"
#include "stdint.h"
#include "tool.h"
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