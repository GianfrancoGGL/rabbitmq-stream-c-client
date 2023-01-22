//---------------------------------------------------------------------------
#include <string.h>
//---------------------------------------------------------------------------
#include "rmqsError.h"
//---------------------------------------------------------------------------
static char_t *rmqsError[] =
{
"Brokers string cannot contain spaces",
"Brokers contains control characters",
"Hostname cannot be empty",
"Port cannot be zero",
"Wrong broker syntax, examples: 'rabbitmq-stream://user:pass@host:5522/a-vhost"
};
//---------------------------------------------------------------------------
void rmqsSetError(size_t ErrorIndex, char_t *ErrorString, const size_t ErrorStringLength)
{
    memset(ErrorString, 0, ErrorIndex);

    if (ErrorIndex > RMQS_ERR_MAX_IDX)
    {
        return;
    }

    strncpy(ErrorString, rmqsError[ErrorIndex], ErrorStringLength - 1);
}
//---------------------------------------------------------------------------

