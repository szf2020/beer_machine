#ifndef  __DEVICE_CONFIG_H__
#define  __DEVICE_CONFIG_H__


/*啤酒机默认平台通信参数表*/
#define  URL_LOG                                    "http://mh1597193030.uicp.cn:35787/device/log/submit"
#define  URL_ACTIVE                                 "http://mh1597193030.uicp.cn:35787/device/info/active"
#define  URL_FAULT                                  "http://mh1597193030.uicp.cn:35787/device/fault/submit"
#define  BOUNDARY                                   "##wkxboot##"
#define  KEY                                        "meiling-beer"
#define  SOURCE                                     "coolbeer"
#define  MODEL                                      "pijiuji"
#define  FW_VERSION                                 "v1.0.0"

/*啤酒机默认运行配置参数表*/
#define  DEFAULT_COMPRESSOR_LOW_TEMPERATURE         2
#define  DEFAULT_COMPRESSOR_HIGH_TEMPERATURE        5
#define  DEFAULT_LOW_PRESSURE                       1
#define  DEFAULT_HIGH_PRESSURE                      10
#define  DEFAULT_LOW_CAPACITY                       5
#define  DEFAULT_HIGH_CAPACITY                      20
#define  DEFAULT_REPORT_LOG_INTERVAL                (1 * 60 * 1000)

/*虽然压缩机温控可调，但也不能超出极限值，保证酒的品质*/
#define  DEFAULT_COMPRESSOR_LOW_TEMPERATURE_LIMIT   0
#define  DEFAULT_COMPRESSOR_HIGH_TEMPERATURE_LIMIT  8









#endif