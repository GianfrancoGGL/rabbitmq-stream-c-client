/****************************************************************************
MIT License

Copyright (c) 2023 Gianfranco Giugliano

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sub-license, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
****************************************************************************/
//---------------------------------------------------------------------------
#include <string.h>
//---------------------------------------------------------------------------
#include "rmqsError.h"
//---------------------------------------------------------------------------
static char_t *rmqsError[] =
{
"Brokers string is null",
"Brokers string cannot contain spaces",
"Brokers contains control characters",
"Hostname cannot be empty",
"Port cannot be zero",
"Wrong broker syntax, examples: 'rabbitmq-stream://user:pass@host:5522/a-vhost"
};
//---------------------------------------------------------------------------
void rmqsSetError(size_t ErrorIndex, char_t *ErrorString, size_t ErrorStringLength)
{
    memset(ErrorString, 0, ErrorIndex);

    if (ErrorIndex > RMQS_ERR_MAX_IDX)
    {
        return;
    }

    strncpy(ErrorString, rmqsError[ErrorIndex], ErrorStringLength - 1);
}
//---------------------------------------------------------------------------

