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
#ifndef rmqsBrokerH
#define rmqsBrokerH
//---------------------------------------------------------------------------
#include <stdint.h>
//---------------------------------------------------------------------------
#include "rmqsGlobal.h"
//---------------------------------------------------------------------------
#define RQMS_BROKER_MAX_USERNAME_LENGTH      128
#define RQMS_BROKER_MAX_PASSWORD_LENGTH      128
#define RQMS_BROKER_MAX_DB_SCHEMA_LENGTH      32
//---------------------------------------------------------------------------
#define RMQS_BROKER_DB_SCHEMA_SEPARATOR       "://"
#define RMQS_BROKER_USER_SEPARATOR            ":"
#define RMQS_BROKER_PASSWORD_SEPARATOR        "@"
#define RMQS_BROKER_HOSTNAME_SEPARATOR        ":"
#define RMQS_BROKER_PORT_SEPARATOR            "/"
#define RMQS_BROKER_SEPARATOR                 ';'
#define RMQS_BROKER_DEFAULT_VHOST             "/"
//---------------------------------------------------------------------------
//
// Broker syntax examples:
//
// rabbitmq-stream://user:pass@host:5522
// rabbitmq-stream://user:pass@host:5522/a-vhost
// rabbitmq-stream+tls://user:pass@host:5521/a-vhost
//
//---------------------------------------------------------------------------
typedef struct
{
    char_t Hostname[RQMS_MAX_HOSTNAME_LENGTH + 1];
    uint16_t Port;
    char_t Username[RQMS_BROKER_MAX_USERNAME_LENGTH + 1];
    char_t Password[RQMS_BROKER_MAX_PASSWORD_LENGTH + 1];
    char_t DBSchema[RQMS_BROKER_MAX_DB_SCHEMA_LENGTH + 1];
    char_t VirtualHost[RQMS_MAX_HOSTNAME_LENGTH + 1];
    bool_t UseTLS;
    char_t AdvicedHostname[RQMS_MAX_HOSTNAME_LENGTH + 1];
    uint16_t AdvicedPort;
}
rmqsBroker_t;
//---------------------------------------------------------------------------
rmqsBroker_t * rmqsBrokerCreate(char_t *Hostname,
                                uint16_t Port,
                                char_t *Username,
                                char_t *Password,
                                char_t *DBSchema,
                                char_t *VirtualHost,
                                char_t *ErrorString,
                                size_t ErrorStringLength);

void rmqsBrokerDestroy(rmqsBroker_t *Broker);
void rmqsBrokerSetDefault(rmqsBroker_t *Broker);
bool_t rmqsBrokerParse(rmqsBroker_t *Broker, char_t *BrokerString, char_t *ErrorString, size_t ErrorStringLength);
//---------------------------------------------------------------------------
#endif
//---------------------------------------------------------------------------
