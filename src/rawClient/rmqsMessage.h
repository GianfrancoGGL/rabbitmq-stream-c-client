//---------------------------------------------------------------------------
#ifndef rmqsMessageH
#define rmqsMessageH
//---------------------------------------------------------------------------
#include <stdint.h>
//---------------------------------------------------------------------------
#include "rmqsGlobal.h"
//---------------------------------------------------------------------------
typedef struct
{
    uint64_t PublishingId;
    void *Data;
    uint32_t Size;
    bool_t DeleteData;
}
rmqsMessage_t;
//---------------------------------------------------------------------------
rmqsMessage_t * rmqsMessageCreate(uint64_t PublishingId, void *Data, uint32_t Size, bool_t CopyData);
void rmqsMessageDestroy(rmqsMessage_t *Message);
//---------------------------------------------------------------------------
#endif
//--------------------------------------------------------------------------
