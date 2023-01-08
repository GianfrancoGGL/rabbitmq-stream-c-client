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
rmqsListNode;
//---------------------------------------------------------------------------
typedef struct
{
    rmqsListNode *First;
    size_t Count;
    void (*ClearDataCB)(void *);
}
rmqsList;
//---------------------------------------------------------------------------
rmqsList * rmqsListCreate(void (*ClearDataCB)(void *));
void rmqsListDestroy(rmqsList *Stream);
//---------------------------------------------------------------------------
rmqsListNode * rmqsListAddBegin(rmqsList *List, void *Data);
rmqsListNode * rmqsListAddEnd(rmqsList *List, void *Data);
rmqsListNode * rmqsListAddPosition(rmqsList *List, size_t Position, void *Data);
//---------------------------------------------------------------------------
void rmqsListDeleteBegin(rmqsList *List);
void rmqsListDeleteEnd(rmqsList *List);
void rmqsListDeleteData(rmqsList *List, void *Data);
void rmqsListDeletePosition(rmqsList *List, size_t Position);
//---------------------------------------------------------------------------
rmqsListNode * rmqsListSearchByData(rmqsList *List, void *Data);
rmqsListNode * rmqsListSearchByPosition(rmqsList *List, size_t Position);
//---------------------------------------------------------------------------
#endif
//--------------------------------------------------------------------------
