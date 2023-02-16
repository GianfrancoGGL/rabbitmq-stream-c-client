//---------------------------------------------------------------------------
#ifndef rmqsGlobalH
#define rmqsGlobalH
//---------------------------------------------------------------------------
typedef char char_t;
typedef unsigned char uchar_t;
typedef long long_t;
typedef unsigned long ulong_t;
typedef unsigned char bool_t;
typedef double double_t;
//---------------------------------------------------------------------------
#ifndef __cplusplus
#ifndef true
#define true   1
#endif
#ifndef false
#define false  0
#endif
#endif
#ifndef min
#define min(x, y) (x < y ? x : y)
#endif
//---------------------------------------------------------------------------
#define RQMS_MAX_HOSTNAME_LENGTH      256
//---------------------------------------------------------------------------
#define rmqsFieldSize(Type, Field)    sizeof(((Type *)0)->Field)
//---------------------------------------------------------------------------
#endif
