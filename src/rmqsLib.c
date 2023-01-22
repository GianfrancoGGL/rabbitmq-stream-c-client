//---------------------------------------------------------------------------
#include <time.h>
#include <stdio.h>
#include <limits.h>
//---------------------------------------------------------------------------
#include <ctype.h>
//---------------------------------------------------------------------------
#include "rmqsLib.h"
//---------------------------------------------------------------------------
bool_t rmqsStringContainsSpace(const char_t *String)
{
    while (*String)
    {
        if (*String == ' ')
        {
            return true;
        }

        String++;
    }

    return false;
}
//---------------------------------------------------------------------------
bool_t rmqsStringContainsCtrlChar(const char_t *String)
{
    while (*String)
    {
        if (iscntrl(*String))
        {
            return true;
        }

        String++;
    }

    return false;
}
//---------------------------------------------------------------------------
void rmqsConvertToLower(char_t *String)
{
    while (*String)
    {
        *String = (char_t)tolower(*String);
        String++;
    }
}
//---------------------------------------------------------------------------


