#ifndef  __FIRMWARE_VERSION_H__
#define  __FIRMWARE_VERSION_H__

#define  MAKE_STR(a)                        #a
#define  MAKE_VERSION_STR(a,b,c)            MAKE_STR(a.b.c)
/*版本号*/
#define  FIRMWARE_VERSION_MAJOR_CODE        1
#define  FIRMWARE_VERSION_MINOR_CODE        0
#define  FIRMWARE_VERSION_REVISION_CODE     121

#define  FIRMWARE_VERSION_HEX               (FIRMWARE_VERSION_MAJOR_CODE << 16 | FIRMWARE_VERSION_MINOR_CODE << 8 | FIRMWARE_VERSION_REVISION_CODE)
#define  FIRMWARE_VERSION_STR               MAKE_VERSION_STR(FIRMWARE_VERSION_MAJOR_CODE,FIRMWARE_VERSION_MINOR_CODE,FIRMWARE_VERSION_REVISION_CODE)





#endif