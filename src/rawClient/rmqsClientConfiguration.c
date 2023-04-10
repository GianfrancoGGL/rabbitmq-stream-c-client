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
#include "rmqsClientConfiguration.h"
#include "rmqsBroker.h"
#include "rmqsList.h"
#include "rmqsMemory.h"
#include "rmqsProtocol.h"
#include "rmqsLib.h"
#include "rmqsError.h"
//---------------------------------------------------------------------------
rmqsClientConfiguration_t * rmqsClientConfigurationCreate(char_t *BrokersString,
                                                          bool_t EnableLogging,
                                                          char_t *LogFileName,
                                                          char_t *ErrorString,
                                                          size_t ErrorStringLength)
{
    rmqsClientConfiguration_t *ClientConfiguration = 0;
    size_t BrokersStringLen = strlen(BrokersString);
    rmqsList_t *BrokersStringList;
    char_t *pBrokersString, *pNewString, *pNewStringVal;
    rmqsBroker_t *Broker;
    size_t i;

    //
    // Windows machine, initialize sockets
    //
    #if _WIN32 || _WIN64
    rmqsInitWinsock();
    #endif

    if (rmqsStringContainsSpace(BrokersString))
    {
        rmqsSetError(RMQS_ERR_IDX_BROKER_DEF_WITH_SPACE, ErrorString, ErrorStringLength);
        return ClientConfiguration;
    }

    if (rmqsStringContainsCtrlChar(BrokersString))
    {
        rmqsSetError(RMQS_ERR_IDX_BROKER_DEF_WITH_CTRL_CHAR, ErrorString, ErrorStringLength);
        return ClientConfiguration;
    }

    ClientConfiguration = (rmqsClientConfiguration_t *)rmqsAllocateMemory(sizeof(rmqsClientConfiguration_t));

    memset(ClientConfiguration, 0, sizeof(rmqsClientConfiguration_t));

    //
    // Check whether the machine is big or little endian
    //
    ClientConfiguration->IsLittleEndianMachine = rmqsIsLittleEndianMachine();

    //
    // Create the brokers list
    //
    ClientConfiguration->BrokerList = rmqsListGenericCreate();
    ClientConfiguration->WaitReplyTimer = rmqsTimerCreate();

    if (EnableLogging && LogFileName && *LogFileName != 0)
    {
        ClientConfiguration->Logger = rmqsLoggerCreate(LogFileName, 0);
    }

    if (BrokersString == 0)
    {
        Broker = (rmqsBroker_t *)rmqsAllocateMemory(sizeof(rmqsBroker_t));
        rmqsBrokerSetDefault(Broker);
        rmqsListAddEnd(ClientConfiguration->BrokerList, Broker);
        return ClientConfiguration;
    }

    //---------------------------------------------------------------------------
    //
    // Split the brokers string that may contain multiple definitions
    // into an array of strings
    //
    BrokersStringList = rmqsListGenericCreate();

    pBrokersString = (char_t *)BrokersString;
    pNewString = pNewStringVal = 0;

    while (*pBrokersString)
    {
        if (! pNewString)
        {
            pNewString = pNewStringVal = rmqsAllocateMemory((size_t)BrokersStringLen + 1);
            memset(pNewStringVal, 0, (size_t)BrokersStringLen + 1);
        }

        if (*(pBrokersString + 1) == 0)
        {
            *pNewStringVal = *pBrokersString;
            rmqsListAddEnd(BrokersStringList, pNewString);
        }
        else if (*pBrokersString == RMQS_BROKER_SEPARATOR)
        {
            rmqsListAddEnd(BrokersStringList, pNewString);
            pNewString = pNewStringVal = 0; // Once the string has been added to the list, force the creation of a new one
        }
        else
        {
            *pNewStringVal = *pBrokersString;
            pNewStringVal++;
        }

        pBrokersString++;
    }
    //---------------------------------------------------------------------------

    //---------------------------------------------------------------------------
    //
    // Parse the n broker strings
    //
    for (i = 0; i < BrokersStringList->Count; i++)
    {
        Broker = (rmqsBroker_t *)rmqsAllocateMemory(sizeof(rmqsBroker_t));
        pBrokersString = (char_t *)rmqsListGetDataByPosition(BrokersStringList, i);

        if (rmqsBrokerParse(Broker, pBrokersString, ErrorString, ErrorStringLength))
        {
            rmqsListAddEnd(ClientConfiguration->BrokerList, Broker);
        }
        else
        {
            rmqsFreeMemory(Broker);
        }
    }

    if (ClientConfiguration->BrokerList->Count == 0)
    {
        Broker = (rmqsBroker_t *)rmqsAllocateMemory(sizeof(rmqsBroker_t));
        rmqsBrokerSetDefault(Broker);
        rmqsListAddEnd(ClientConfiguration->BrokerList, Broker);
    }

    rmqsListDestroy(BrokersStringList);

    return ClientConfiguration;
}
//---------------------------------------------------------------------------
void rmqsClientConfigurationDestroy(rmqsClientConfiguration_t *ClientConfiguration)
{
    if (ClientConfiguration == 0)
    {
        return;
    }

    rmqsListDestroy(ClientConfiguration->BrokerList);

    rmqsTimerDestroy(ClientConfiguration->WaitReplyTimer);

    if (ClientConfiguration->Logger != 0)
    {
        rmqsLoggerDestroy(ClientConfiguration->Logger);
    }

    rmqsFreeMemory((void *)ClientConfiguration);

    //
    // Windows machine, shutdown sockets
    //
    #if _WIN32 || _WIN64
    rmqsShutdownWinsock();
    #endif
}
//---------------------------------------------------------------------------
