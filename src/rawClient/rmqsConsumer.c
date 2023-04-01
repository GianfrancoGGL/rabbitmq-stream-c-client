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
#include <stdio.h>
//---------------------------------------------------------------------------
#include "rmqsConsumer.h"
#include "rmqsClientConfiguration.h"
#include "rmqsMemory.h"
//---------------------------------------------------------------------------
rmqsConsumer_t * rmqsConsumerCreate(rmqsClientConfiguration_t *ClientConfiguration, DeliverResultCallback_t DeliverResultCallback)
{
    rmqsConsumer_t *Consumer = (rmqsConsumer_t *)rmqsAllocateMemory(sizeof(rmqsConsumer_t));

    memset(Consumer, 0, sizeof(rmqsConsumer_t));

    Consumer->Client = rmqsClientCreate(ClientConfiguration, rmqsctConsumer, Consumer);
    Consumer->DeliverResultCallback = DeliverResultCallback;

    return Consumer;
}
//---------------------------------------------------------------------------
void rmqsConsumerDestroy(rmqsConsumer_t *Consumer)
{
    rmqsClientDestroy(Consumer->Client);

    rmqsFreeMemory((void *)Consumer);
}
//---------------------------------------------------------------------------
void rmqsConsumerPoll(rmqsConsumer_t *Consumer, const rmqsSocket Socket, uint32_t Timeout, bool_t *ConnectionLost)
{
    uint32_t WaitMessageTimeout = Timeout;
    uint32_t Time;

    *ConnectionLost = false;

    rmqsTimerStart(Consumer->Client->ClientConfiguration->WaitReplyTimer);

    while (rmqsTimerGetTime(Consumer->Client->ClientConfiguration->WaitReplyTimer) < Timeout)
    {
        rmqsWaitMessage(Consumer->Client, Socket, WaitMessageTimeout, ConnectionLost);

        if (*ConnectionLost)
        {
            return;
        }

        Time = rmqsTimerGetTime(Consumer->Client->ClientConfiguration->WaitReplyTimer);

        WaitMessageTimeout = Timeout - Time;
    }
}
//---------------------------------------------------------------------------
