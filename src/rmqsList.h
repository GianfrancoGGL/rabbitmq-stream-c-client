//---------------------------------------------------------------------------
#ifndef rmqsListH
#define rmqsListH
//---------------------------------------------------------------------------
#include <stdint.h>
#include <stddef.h>
//---------------------------------------------------------------------------
typedef struct
{
    void *Data;
    void *Next;
}
rmqsListNode_t;
//---------------------------------------------------------------------------
typedef struct
{
    rmqsListNode_t *First;
    size_t Count;
    void (*ClearDataCallback)(void *);
}
rmqsList_t;
//---------------------------------------------------------------------------
rmqsList_t * rmqsListCreate(void (*ClearDataCallback)(void *));
rmqsList_t * rmqsListGenericCreate(void);
void rmqsListDestroy(rmqsList_t *Stream);
//---------------------------------------------------------------------------
rmqsListNode_t * rmqsListAddBegin(rmqsList_t *List, void *Data);
rmqsListNode_t * rmqsListAddEnd(rmqsList_t *List, void *Data);
rmqsListNode_t * rmqsListAddPosition(rmqsList_t *List, size_t Position, void *Data);
//---------------------------------------------------------------------------
void rmqsListDeleteBegin(rmqsList_t *List);
void rmqsListDeleteEnd(rmqsList_t *List);
void rmqsListDeleteData(rmqsList_t *List, void *Data);
void rmqsListDeletePosition(rmqsList_t *List, size_t Position);
//---------------------------------------------------------------------------
rmqsListNode_t * rmqsListSearchByData(rmqsList_t *List, void *Data);
rmqsListNode_t * rmqsListSearchByPosition(rmqsList_t *List, size_t Position);
//---------------------------------------------------------------------------
void * rmqsListGetDataByPosition(rmqsList_t *List, size_t Position);
//---------------------------------------------------------------------------
void rmqListGenericDestroyCallcack(void *Data);
//---------------------------------------------------------------------------
#endif
//--------------------------------------------------------------------------
