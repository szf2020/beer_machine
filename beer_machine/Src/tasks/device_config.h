#ifndef  __DEVICE_CONFIG_H__
#define  __DEVICE_CONFIG_H__


/*啤酒机默认平台通信参数表*/
#define  URL_LOG                                    "http://mh1597193030.uicp.cn:35787/device/log/submit"
#define  URL_ACTIVE                                 "http://mh1597193030.uicp.cn:35787/device/info/active"
#define  URL_FAULT                                  "http://mh1597193030.uicp.cn:35787/device/fault/submit"
#define  URL_UPGRADE                                "http://mh1597193030.uicp.cn:35787/device/info/getUpgradeInfo"
#define  BOUNDARY                                   "##wkxboot##"
#define  KEY                                        "meiling-beer"
#define  SOURCE                                     "coolbeer"
#define  MODEL                                      "pijiuji"


#define  SN_LEN                                     18        /*SN在flash中的地址*/
#define  SN_ADDR                                    0x803FF00 /*SN字节长度*/
/*啤酒机默认运行配置参数表*/
#define  DEFAULT_COMPRESSOR_LOW_TEMPERATURE         2  /*默认压缩机最低温度*/
#define  DEFAULT_COMPRESSOR_HIGH_TEMPERATURE        5  /*默认压缩机最高温度*/
#define  DEFAULT_LOW_PRESSURE                       1  /*默认压力报警最低值kg/cm2（放大10倍）*/
#define  DEFAULT_HIGH_PRESSURE                      10 /*默认压力报警最高值kg/cm2（放大10倍）*/
#define  DEFAULT_LOW_CAPACITY                       5  /*容量报警低值（暂不可配）*/
#define  DEFAULT_HIGH_CAPACITY                      20 /*容量报警高值（暂不可配）*/
#define  DEFAULT_REPORT_LOG_INTERVAL                (1 * 60 * 1000) /*默认日志上报间隔时间ms*/

/*虽然压缩机温控可调，但也不能超出极限值，保证酒的品质*/
#define  DEFAULT_COMPRESSOR_LOW_TEMPERATURE_LIMIT   0 /*SN字节长度*/
#define  DEFAULT_COMPRESSOR_HIGH_TEMPERATURE_LIMIT  8 /*SN字节长度*/









#endif