//---------------------------------------------------------------------------
#include <memory.h>
//---------------------------------------------------------------------------
#include "rmqsList.h"
#include "rmqsMemory.h"
//---------------------------------------------------------------------------
rmqsList * rmqsListCreate(void (*ClearDataCB)(void *))
{
    rmqsList *List = (rmqsList *)rmqsAllocateMemory(sizeof(rmqsList));

    List->First = 0;
    List->Count = 0;
    List->ClearDataCB = ClearDataCB;

    return List;
}
//---------------------------------------------------------------------------
void rmqsListDestroy(rmqsList *List)
{
    while (List->Count > 0)
    {
        rmqsListDeleteBegin(List);
    }

    rmqsFreeMemory((void *)List);
}
//---------------------------------------------------------------------------
rmqsListNode * rmqsListAddBegin(rmqsList *List, void *Data)
{
    rmqsListNode *NewNode = (rmqsListNode *)rmqsAllocateMemory(sizeof(rmqsListNode));

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
rmqsListNode * rmqsListAddEnd(rmqsList *List, void *Data)
{
    rmqsListNode *NewNode = (rmqsListNode *)rmqsAllocateMemory(sizeof(rmqsListNode));
    rmqsListNode *Node;

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
            Node = (rmqsListNode *)Node->Next;
        }

        Node->Next = NewNode;
    }

    List->Count++;

    return NewNode;
}
//---------------------------------------------------------------------------
rmqsListNode * rmqsListAddPosition(rmqsList *List, size_t Position, void *Data)
{
    rmqsListNode *NewNode = (rmqsListNode *)rmqsAllocateMemory(sizeof(rmqsListNode));

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
        rmqsListNode *Node, *PrevNode;
        size_t i;

        Node = PrevNode = List->First;

        for (i = 0; i < Position; i++)
        {
            PrevNode = Node;
            Node = (rmqsListNode *)Node->Next;

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
void rmqsListDeleteBegin(rmqsList *List)
{
    rmqsListNode *Node;

    if (List->First == 0)
    {
        return;
    }

    Node = List->First;

    List->First = (rmqsListNode *)List->First->Next;

    if (List->ClearDataCB != 0)
    {
        List->ClearDataCB(Node->Data);
    }

    rmqsFreeMemory(Node);

    List->Count--;
}
//---------------------------------------------------------------------------
void rmqsListDeleteEnd(rmqsList *List)
{
    if (List->First == 0)
    {
        return;
    }

    if (List->First->Next == 0)
    {
        rmqsListNode *Node = List->First;

        List->First = (rmqsListNode *)Node->Next;

        if (List->ClearDataCB != 0)
        {
            List->ClearDataCB(Node->Data);
        }

        rmqsFreeMemory(Node);
    }
    else
    {
        rmqsListNode *Node, *PrevNode;

        Node = List->First;
        PrevNode = 0;

        while (Node->Next != 0)
        {
            PrevNode = Node;
            Node = (rmqsListNode *)Node->Next;
        }

        PrevNode->Next = 0;

        if (List->ClearDataCB != 0)
        {
            List->ClearDataCB(Node->Data);
        }

        rmqsFreeMemory(Node);
    }

    List->Count--;
}
//---------------------------------------------------------------------------
void rmqsListDeleteData(rmqsList *List, void *Data)
{
    rmqsListNode *Node;
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

        Node = (rmqsListNode *)Node->Next;
    }
}
//---------------------------------------------------------------------------
void rmqsListDeletePosition(rmqsList *List, size_t Position)
{
    rmqsListNode *Node;

    if (List->First == 0)
    {
        return;
    }

    Node = List->First;

    if (Position == 0)
    {
        Node = List->First;

        List->First = (rmqsListNode *)Node->Next;

        if (List->ClearDataCB != 0)
        {
            List->ClearDataCB(Node->Data);
        }

        rmqsFreeMemory(Node);

        List->Count--;
    }
    else
    {
        rmqsListNode *PrevNode;
        size_t i;

        for (i = 0; i < Position; i++)
        {
            PrevNode = Node;
            Node = (rmqsListNode *)Node->Next;

            if (Node == 0)
            {
                return; // Wroing index
            }
        }

        PrevNode->Next = Node->Next;

        if (List->ClearDataCB != 0)
        {
            List->ClearDataCB(Node->Data);
        }

        rmqsFreeMemory(Node);

        List->Count--;
    }
}
//---------------------------------------------------------------------------
rmqsListNode * rmqsListSearchByData(rmqsList *List, void *Data)
{
    rmqsListNode *Node = List->First;

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

        Node = (rmqsListNode *)Node->Next;
    }
    while (Node != 0);

    return 0;
}
//---------------------------------------------------------------------------
rmqsListNode * rmqsListSearchByPosition(rmqsList *List, size_t Position)
{
    rmqsListNode *Node = List->First;
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

        Node = (rmqsListNode *)Node->Next;
    }
    while (Node != 0);

    return 0;
}
//---------------------------------------------------------------------------

