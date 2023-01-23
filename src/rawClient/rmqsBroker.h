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
rmqsBroker_t * rmqsBrokerCreate(const char_t *Hostname,
                                const uint16_t Port,
                                const char_t *Username,
                                const char_t *Password,
                                const char_t *DBSchema,
                                const char_t *VirtualHost,
                                char_t *ErrorString,
                                const size_t ErrorStringLength);

void rmqsBrokerDestroy(rmqsBroker_t *Broker);
void rmqsBrokerSetDefault(rmqsBroker_t *Broker);
bool_t rmqsBrokerParse(rmqsBroker_t *Broker, const char_t *BrokerString, char_t *ErrorString, const size_t ErrorStringLength);
//---------------------------------------------------------------------------
#endif
//---------------------------------------------------------------------------
