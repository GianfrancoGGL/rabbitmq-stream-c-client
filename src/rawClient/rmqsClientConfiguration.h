//---------------------------------------------------------------------------
#ifndef rmqsClientConfigurationH
#define rmqsClientConfigurationH
//---------------------------------------------------------------------------
#include <stdint.h>
//---------------------------------------------------------------------------
#include "rmqsList.h"
#include "rmqsLogger.h"
//---------------------------------------------------------------------------
typedef struct
{
    bool_t IsLittleEndianMachine;
    rmqsList_t *BrokerList;
    bool_t UseTLS;
    rmqsLogger_t *Logger;
}
rmqsClientConfiguration_t;
//---------------------------------------------------------------------------
rmqsClientConfiguration_t * rmqsClientConfigurationCreate(const char_t *BrokersString,
                                                          const bool_t EnableLogging,
                                                          const char_t *LogFileName,
                                                          char_t *ErrorString,
                                                          const size_t ErrorStringLength);
                                                          
void rmqsClientConfigurationDestroy(rmqsClientConfiguration_t *ClientConfiguration);
//---------------------------------------------------------------------------
#endif
//--------------------------------------------------------------------------
