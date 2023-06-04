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
#include <ctype.h>
#include <stdint.h>
//---------------------------------------------------------------------------
#include "rmqsMessage.h"
#include "rmqsMemory.h"
//---------------------------------------------------------------------------
rmqsMessage_t * rmqsMessageCreate(uint64_t PublishingId, void *Data, uint32_t Size, bool_t CopyData, rqmsMessageEncoding_t Encoding)
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
    Message->Encoding = Encoding;

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
void rmqsBatchDestroy(rmqsMessage_t *MessageBatch, size_t Count)
{
    rmqsMessage_t *Message = MessageBatch;
    size_t i;

    for (i = 0; i < Count; i++)
    {
        if (Message->DeleteData)
        {
            rmqsFreeMemory((void *)Message->Data);
        }

        Message++;
    }

    rmqsFreeMemory((void *)MessageBatch);
}
//---------------------------------------------------------------------------

