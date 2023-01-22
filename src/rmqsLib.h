//---------------------------------------------------------------------------
#ifndef rmqsLibH
#define rmqsLibH
//---------------------------------------------------------------------------
#include <stdint.h>
//---------------------------------------------------------------------------
#include "rmqsGlobal.h"
//---------------------------------------------------------------------------
bool_t rmqsStringContainsSpace(const char_t *String);
bool_t rmqsStringContainsCtrlChar(const char_t *String);
void rmqsConvertToLower(char_t *String);
//---------------------------------------------------------------------------
#endif
