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
#include "rmqsBroker.h"
#include "rmqsLib.h"
#include "rmqsError.h"
#include "rmqsMemory.h"
//---------------------------------------------------------------------------
rmqsBroker_t * rmqsBrokerCreate(const char_t *Hostname,
                                const uint16_t Port,
                                const char_t *Username,
                                const char_t *Password,
                                const char_t *DBSchema,
                                const char_t *VirtualHost,
                                char_t *ErrorString,
                                const size_t ErrorStringLength)
{
    rmqsBroker_t *Broker;

    memset(ErrorString, 0, ErrorStringLength);

    if (Hostname == 0)
    {
        rmqsSetError(RMQS_ERR_IDX_EMPTY_HOSTNAME, ErrorString, ErrorStringLength);
        return 0;
    }

    if (Port == 0)
    {
        rmqsSetError(RMQS_ERR_IDX_UNSPECIFIED_PORT, ErrorString, ErrorStringLength);
        return 0;
    }

    Broker = (rmqsBroker_t *)rmqsAllocateMemory(sizeof(rmqsBroker_t));

    memset(Broker, 0, sizeof(rmqsBroker_t));

    strncpy(Broker->Hostname, Hostname, RQMS_MAX_HOSTNAME_LENGTH);
    Broker->Port = Port;
    strncpy(Broker->Username, Username, RQMS_BROKER_MAX_USERNAME_LENGTH);
    strncpy(Broker->Password, Password, RQMS_BROKER_MAX_PASSWORD_LENGTH);
    strncpy(Broker->DBSchema, DBSchema, RQMS_BROKER_MAX_DB_SCHEMA_LENGTH);
    strncpy(Broker->VirtualHost, VirtualHost, RQMS_MAX_HOSTNAME_LENGTH);

    return Broker;
}
//---------------------------------------------------------------------------
void rmqsBrokerDestroy(rmqsBroker_t *Broker)
{
    if (Broker != 0)
    {
        rmqsFreeMemory((void *)Broker);
    }
}
//---------------------------------------------------------------------------
void rmqsBrokerSetDefault(rmqsBroker_t *Broker)
{
    memset(Broker, 0, sizeof(rmqsBroker_t));

    strncpy(Broker->Hostname, "localhost", RQMS_MAX_HOSTNAME_LENGTH);
    Broker->Port = 5552;
    strncpy(Broker->Username, "guest", RQMS_BROKER_MAX_USERNAME_LENGTH);
    strncpy(Broker->Password, "guest", RQMS_BROKER_MAX_PASSWORD_LENGTH);
    strncpy(Broker->DBSchema, "rabbit-mq", RQMS_BROKER_MAX_DB_SCHEMA_LENGTH);
    strncpy(Broker->VirtualHost, RMQS_BROKER_DEFAULT_VHOST, RQMS_MAX_HOSTNAME_LENGTH);
}
//---------------------------------------------------------------------------
bool_t rmqsBrokerParse(rmqsBroker_t *Broker, const char_t *BrokerString, char_t *ErrorString, const size_t ErrorStringLength)
{
    char_t PortString[5 + 1]; // Must contain from 0 to 65535
    char_t *Token, *BrokerStringEnd = (char_t *)BrokerString + strlen(BrokerString);

    memset(Broker, 0, sizeof(rmqsBroker_t));

    //
    // Read the schema
    //
    Token = strstr(BrokerString, RMQS_BROKER_DB_SCHEMA_SEPARATOR);

    if (! Token)
    {
        rmqsSetError(RMQS_ERR_IDX_WRONG_BROKER_DEF, ErrorString, ErrorStringLength);
        return false;
    }

    strncpy(Broker->DBSchema, BrokerString, minval(Token - BrokerString, RQMS_BROKER_MAX_DB_SCHEMA_LENGTH));
    rmqsConvertToLower(Broker->DBSchema);

    BrokerString = Token + strlen(RMQS_BROKER_DB_SCHEMA_SEPARATOR);

    if (BrokerString > BrokerStringEnd)
    {
        rmqsSetError(RMQS_ERR_IDX_WRONG_BROKER_DEF, ErrorString, ErrorStringLength);
        return false;
    }

    //
    // Read the username
    //
    memset(Broker->Username, 0, sizeof(Broker->Username));

    Token = strstr(BrokerString, RMQS_BROKER_USER_SEPARATOR);

    if (! Token)
    {
        rmqsSetError(RMQS_ERR_IDX_WRONG_BROKER_DEF, ErrorString, ErrorStringLength);
        return false;
    }

    strncpy(Broker->Username, BrokerString, minval(Token - BrokerString, RQMS_BROKER_MAX_USERNAME_LENGTH));
    rmqsConvertToLower(Broker->Username);

    BrokerString = Token + strlen(RMQS_BROKER_USER_SEPARATOR);

    if (BrokerString > BrokerStringEnd)
    {
        rmqsSetError(RMQS_ERR_IDX_WRONG_BROKER_DEF, ErrorString, ErrorStringLength);
        return false;
    }

    //
    // Read the password
    //
    Token = strstr(BrokerString, RMQS_BROKER_PASSWORD_SEPARATOR);

    if (! Token)
    {
        rmqsSetError(RMQS_ERR_IDX_WRONG_BROKER_DEF, ErrorString, ErrorStringLength);
        return false;
    }

    strncpy(Broker->Password, BrokerString, minval(Token - BrokerString, RQMS_BROKER_MAX_PASSWORD_LENGTH));
    rmqsConvertToLower(Broker->Password);

    BrokerString = Token + strlen(RMQS_BROKER_PASSWORD_SEPARATOR);

    if (BrokerString > BrokerStringEnd)
    {
        rmqsSetError(RMQS_ERR_IDX_WRONG_BROKER_DEF, ErrorString, ErrorStringLength);
        return false;
    }

    //
    // Read the hostname
    //
    Token = strstr(BrokerString, RMQS_BROKER_HOSTNAME_SEPARATOR);

    if (! Token)
    {
        rmqsSetError(RMQS_ERR_IDX_WRONG_BROKER_DEF, ErrorString, ErrorStringLength);
        return false;
    }

    strncpy(Broker->Hostname, BrokerString, minval(Token - BrokerString, RQMS_MAX_HOSTNAME_LENGTH));
    rmqsConvertToLower(Broker->Hostname);

    BrokerString = Token + strlen(RMQS_BROKER_HOSTNAME_SEPARATOR);

    if (BrokerString > BrokerStringEnd)
    {
        rmqsSetError(RMQS_ERR_IDX_WRONG_BROKER_DEF, ErrorString, ErrorStringLength);
        return false;
    }

    //
    // Read the port number
    //
    memset(PortString, 0, sizeof(PortString));

    Token = strstr(BrokerString, RMQS_BROKER_PORT_SEPARATOR);

    if (! Token)
    {
        strncpy(PortString, BrokerString, sizeof(PortString) - 1);
        Broker->Port = (uint16_t)atoi(PortString);

        strncpy(Broker->VirtualHost, RMQS_BROKER_DEFAULT_VHOST, RQMS_MAX_HOSTNAME_LENGTH);
    }
    else
    {
        strncpy(PortString, BrokerString, minval((size_t)(Token - BrokerString), sizeof(PortString) - 1));
        Broker->Port = (uint16_t)atoi(PortString);

        BrokerString = Token + strlen(RMQS_BROKER_PORT_SEPARATOR);

        if (BrokerString > BrokerStringEnd)
        {
            rmqsSetError(RMQS_ERR_IDX_WRONG_BROKER_DEF, ErrorString, ErrorStringLength);
            return false;
        }

        //
        // Read the virtual host
        //
        BrokerString--; // The host must include the slash character

        strncpy(Broker->VirtualHost, BrokerString, sizeof(Broker->VirtualHost) - 1);
        rmqsConvertToLower(Broker->VirtualHost);
    }

    Broker->UseTLS = (bool_t)(strstr(Broker->DBSchema, "+tls") != 0 ? true : false);

    return true;
}
//---------------------------------------------------------------------------

