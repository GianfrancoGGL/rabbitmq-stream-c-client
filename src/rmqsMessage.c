//---------------------------------------------------------------------------
#include <ctype.h>
#include <stdint.h>
//---------------------------------------------------------------------------
#include "rmqsMessage.h"
#include "rmqsMemory.h"
//---------------------------------------------------------------------------
rmqsMessage_t * rmqsMessageCreate(uint64_t PublishingId, void *Data, uint32_t Size, bool_t CopyData)
{
    rmqsMessage_t *Message = (rmqsMessage_t *)rmqsAllocateMemory(sizeof(rmqsMessage_t));

    Message->PublishingId = PublishingId;

    if (! CopyData)
    {
        Message->Data = Data;
    }
    else
    {
        Message->Data = rmqsAllocateMemory(Size);
        memcpy(Message->Data, Data, Size);
    }

    Message->Size = Size;
    Message->DeleteData = CopyData;

    return Message;
}
//---------------------------------------------------------------------------
void rmqsMessageDestroy(rmqsMessage_t *Message)
{
    if (Message->DeleteData)
    {
        rmqsFreeMemory((void *)Message->Data);
    }

    rmqsFreeMemory((void *)Message);
}
//---------------------------------------------------------------------------

