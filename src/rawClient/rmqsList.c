//---------------------------------------------------------------------------
#include <memory.h>
//---------------------------------------------------------------------------
#include "rmqsList.h"
#include "rmqsMemory.h"
//---------------------------------------------------------------------------
rmqsList_t * rmqsListCreate(void (*ClearDataCallback)(void *))
{
    rmqsList_t *List = (rmqsList_t *)rmqsAllocateMemory(sizeof(rmqsList_t));

    List->First = 0;
    List->Count = 0;
    List->ClearDataCallback = ClearDataCallback;

    return List;
}
//---------------------------------------------------------------------------
rmqsList_t * rmqsListGenericCreate(void)
{
    return rmqsListCreate(rmqListGenericDestroyCallcack);
}
//---------------------------------------------------------------------------
void rmqsListDestroy(rmqsList_t *List)
{
    while (List->Count > 0)
    {
        rmqsListDeleteBegin(List);
    }

    rmqsFreeMemory((void *)List);
}
//---------------------------------------------------------------------------
rmqsListNode_t * rmqsListAddBegin(rmqsList_t *List, void *Data)
{
    rmqsListNode_t *NewNode = (rmqsListNode_t *)rmqsAllocateMemory(sizeof(rmqsListNode_t));

    NewNode->Data = Data;

    if (List->First == 0)
    {
        NewNode->Next = 0;
        List->First = NewNode;
    }
    else
    {
        NewNode->Next = List->First;
        List->First = NewNode;
    }

    List->Count++;

    return NewNode;
}
//---------------------------------------------------------------------------
rmqsListNode_t * rmqsListAddEnd(rmqsList_t *List, void *Data)
{
    rmqsListNode_t *NewNode = (rmqsListNode_t *)rmqsAllocateMemory(sizeof(rmqsListNode_t));
    rmqsListNode_t *Node;

    NewNode->Data = Data,
    NewNode->Next = 0;

    if (List->First == 0)
    {
        List->First = NewNode;
    }
    else
    {
        Node = List->First;

        while (Node->Next != 0)
        {
            Node = (rmqsListNode_t *)Node->Next;
        }

        Node->Next = NewNode;
    }

    List->Count++;

    return NewNode;
}
//---------------------------------------------------------------------------
rmqsListNode_t * rmqsListAddPosition(rmqsList_t *List, size_t Position, void *Data)
{
    rmqsListNode_t *NewNode = (rmqsListNode_t *)rmqsAllocateMemory(sizeof(rmqsListNode_t));

    NewNode->Data = Data,
    NewNode->Next = 0;

    if (List->First == 0)
    {
        List->First = NewNode;
    }
    else if (Position == 0)
    {
        NewNode->Next = List->First;
        List->First = NewNode;
    }
    else
    {
        rmqsListNode_t *Node, *PrevNode;
        size_t i;

        Node = PrevNode = List->First;

        for (i = 0; i < Position; i++)
        {
            PrevNode = Node;
            Node = (rmqsListNode_t *)Node->Next;

            if (Node == 0)
            {
                break; // Wrong index, end reached
            }
        }

        NewNode->Next = Node;
        PrevNode->Next = NewNode;
    }

    List->Count++;

    return NewNode;
}
//---------------------------------------------------------------------------
void rmqsListDeleteBegin(rmqsList_t *List)
{
    rmqsListNode_t *Node;

    if (List->First == 0)
    {
        return;
    }

    Node = List->First;

    List->First = (rmqsListNode_t *)List->First->Next;

    if (List->ClearDataCallback != 0)
    {
        List->ClearDataCallback(Node->Data);
    }

    rmqsFreeMemory(Node);

    List->Count--;
}
//---------------------------------------------------------------------------
void rmqsListDeleteEnd(rmqsList_t *List)
{
    if (List->First == 0)
    {
        return;
    }

    if (List->First->Next == 0)
    {
        rmqsListNode_t *Node = List->First;

        List->First = (rmqsListNode_t *)Node->Next;

        if (List->ClearDataCallback != 0)
        {
            List->ClearDataCallback(Node->Data);
        }

        rmqsFreeMemory(Node);
    }
    else
    {
        rmqsListNode_t *Node, *PrevNode;

        Node = List->First;
        PrevNode = 0;

        while (Node->Next != 0)
        {
            PrevNode = Node;
            Node = (rmqsListNode_t *)Node->Next;
        }

        PrevNode->Next = 0;

        if (List->ClearDataCallback != 0)
        {
            List->ClearDataCallback(Node->Data);
        }

        rmqsFreeMemory(Node);
    }

    List->Count--;
}
//---------------------------------------------------------------------------
void rmqsListDeleteData(rmqsList_t *List, void *Data)
{
    rmqsListNode_t *Node;
    size_t Position = 0;

    Node = List->First;

    if (Node != 0 && Node->Data == Data)
    {
        rmqsListDeletePosition(List, Position);

        return;
    }

    while (Node->Next != 0)
    {
        Position++;

        if (Node->Data == Data)
        {
            rmqsListDeletePosition(List, Position);

            return;
        }

        Node = (rmqsListNode_t *)Node->Next;
    }
}
//---------------------------------------------------------------------------
void rmqsListDeletePosition(rmqsList_t *List, size_t Position)
{
    rmqsListNode_t *Node;

    if (List->First == 0)
    {
        return;
    }

    Node = List->First;

    if (Position == 0)
    {
        Node = List->First;

        List->First = (rmqsListNode_t *)Node->Next;

        if (List->ClearDataCallback != 0)
        {
            List->ClearDataCallback(Node->Data);
        }

        rmqsFreeMemory(Node);

        List->Count--;
    }
    else
    {
        rmqsListNode_t *PrevNode;
        size_t i;

        for (i = 0; i < Position; i++)
        {
            PrevNode = Node;
            Node = (rmqsListNode_t *)Node->Next;

            if (Node == 0)
            {
                return; // Wroing index
            }
        }

        PrevNode->Next = Node->Next;

        if (List->ClearDataCallback != 0)
        {
            List->ClearDataCallback(Node->Data);
        }

        rmqsFreeMemory(Node);

        List->Count--;
    }
}
//---------------------------------------------------------------------------
rmqsListNode_t * rmqsListSearchByData(rmqsList_t *List, void *Data)
{
    rmqsListNode_t *Node = List->First;

    if (Node == 0)
    {
        return 0;
    }

    do
    {
        if (Node->Data == Data)
        {
            return Node;
        }

        Node = (rmqsListNode_t *)Node->Next;
    }
    while (Node != 0);

    return 0;
}
//---------------------------------------------------------------------------
rmqsListNode_t * rmqsListSearchByPosition(rmqsList_t *List, size_t Position)
{
    rmqsListNode_t *Node = List->First;
    size_t Count = 0;

    if (Node == 0)
    {
        return 0;
    }

    do
    {
        if (Count++ == Position)
        {
            return Node;
        }

        Node = (rmqsListNode_t *)Node->Next;
    }
    while (Node != 0);

    return 0;
}
//---------------------------------------------------------------------------
void * rmqsListGetDataByPosition(rmqsList_t *List, size_t Position)
{
    rmqsListNode_t *Node = rmqsListSearchByPosition(List, Position);

    if (Node)
    {
        return Node->Data;
    }
    else
    {
        return 0;
    }
}
//---------------------------------------------------------------------------
void rmqListGenericDestroyCallcack(void *Data)
{
    rmqsFreeMemory(Data);
}
//---------------------------------------------------------------------------

