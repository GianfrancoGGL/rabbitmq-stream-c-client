//---------------------------------------------------------------------------
#ifndef rmqsErrorH
#define rmqsErrorH
//---------------------------------------------------------------------------
#include <stdint.h>
//---------------------------------------------------------------------------
#include "rmqsGlobal.h"
//---------------------------------------------------------------------------
#define RMQS_ERR_IDX_BROKER_DEF_WITH_SPACE         0
#define RMQS_ERR_IDX_BROKER_DEF_WITH_CTRL_CHAR     1
#define RMQS_ERR_IDX_EMPTY_HOSTNAME                2
#define RMQS_ERR_IDX_UNSPECIFIED_PORT              3
#define RMQS_ERR_IDX_WRONG_BROKER_DEF              4
//---------------------------------------------------------------------------
#define RMQS_ERR_MAX_IDX                           RMQS_ERR_IDX_WRONG_BROKER_DEF
//---------------------------------------------------------------------------
#define RMQS_ERR_MAX_STRING_LENGTH               256
//---------------------------------------------------------------------------
void rmqsSetError(size_t ErrorIndex, char_t *ErrorString, const size_t ErrorStringLength);
//---------------------------------------------------------------------------
#endif
//---------------------------------------------------------------------------
