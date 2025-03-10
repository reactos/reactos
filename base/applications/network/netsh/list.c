/*
 * PROJECT:    ReactOS NetSh
 * LICENSE:    GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:    List code
 * COPYRIGHT:  Copyright 2025 Curtis Wilson <LiquidFox1776@gmail.com>
 */
 
#include "precomp.h"
#define NDEBUG
#include <debug.h>


static BOOL _AddToList(PLIST self, PVOID data);
static BOOL _RemoveFromList(PLIST self, PVOID data);
static BOOL _RemoveItem(PLIST self, size_t index); // Changed to size_t
static size_t _CountOfList(PLIST self); // Changed to size_t
static PVOID _PopFromList(PLIST self);
static VOID _FreeList(PLIST self);
static PLIST_NODE _GetItem(PLIST self, size_t index); // Changed to size_t


// Function implementations
static 
BOOL 
_AddToList(
    PLIST self, 
    PVOID data) 
{
    if (!self || !data)
    {
        DPRINT1("%s self or data is NULL\n", __FUNCTION__); 
        return FALSE;
    }

    PLIST_NODE newNode = (PLIST_NODE)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(LIST_NODE));
    if (!newNode) 
    {
        DPRINT1("%s newNode is NULL!\n", __FUNCTION__);
        return FALSE;
    }
    
    newNode->Data = data;
    newNode->Next = self->Head;
    self->Head = newNode;
    self->NodeCount++;
    return TRUE;
}


static 
BOOL 
_RemoveFromList(
    PLIST self, 
    PVOID data) 
{
    if (!self || !data || !self->Head) 
        return FALSE;

    PLIST_NODE current = self->Head;
    PLIST_NODE previous = NULL;

    while (current) {
        if (current->Data == data) {
            if (previous) {
                previous->Next = current->Next;
            } else {
                self->Head = current->Next;
            }
            HeapFree(GetProcessHeap(), 0, current);
            self->NodeCount--;
            return TRUE;
        }
        previous = current;
        current = current->Next;
    }
    return FALSE;
}


static 
BOOL 
_RemoveItem(
    PLIST self, 
    size_t index) 
{
    if (!self || index >= self->NodeCount) 
        return FALSE;

    PLIST_NODE current = self->Head;
    PLIST_NODE previous = NULL;

    if (index == 0) {
        // Special case: Remove the head node
        self->Head = current->Next;
        HeapFree(GetProcessHeap(), 0, current);
        self->NodeCount--;
        return TRUE;
    }

    for (size_t i = 0; i < index; i++) {
        previous = current;
        current = current->Next;
    }

    if (current) {
        previous->Next = current->Next;
        HeapFree(GetProcessHeap(), 0, current);
        self->NodeCount--;
        return TRUE;
    }

    return FALSE; // If we reach here, the index was invalid
}


static 
size_t 
_CountOfList(
    PLIST self) 
{
    DPRINT1("%s() \n", __FUNCTION__);
    if (self == NULL)
    {
        DPRINT1("%s self is NULL!\n", __FUNCTION__); 
        return 0;
    }
    
    DPRINT1("%s self->NodeCount\n", __FUNCTION__);
    return self->NodeCount;
}


static 
PVOID 
_PopFromList(
    PLIST self) 
{
    if (!self || !self->Head) 
        return NULL;

    PLIST_NODE nodeToPop = self->Head;
    self->Head = nodeToPop->Next;
    self->NodeCount--;

    PVOID data = nodeToPop->Data;
    HeapFree(GetProcessHeap(), 0, nodeToPop);
    return data;
}


static 
VOID 
_FreeList(
    PLIST self) 
{
    if (!self) 
        return;

    PLIST_NODE current = self->Head;
    while (current) 
    {
        PLIST_NODE next = current->Next;
        HeapFree(GetProcessHeap(), 0, current);
        current = next;
    }
    HeapFree(GetProcessHeap(), 0, self);
}


static 
PLIST_NODE 
_GetItem(
    PLIST self, 
    size_t index) 
{
    
    if ((self == NULL) || (index >= self->NodeCount))
    {
        DPRINT1("%s self is NULL, or index >= NodeCount\n", __FUNCTION__);
        return NULL;
    }

    PLIST_NODE current = self->Head;
    size_t currentIndex = 0;
    
    while (current && currentIndex < index) 
    {
        current = current->Next;
        currentIndex++;
    }

    return current; // Returns NULL if the index is out of bounds
}


PLIST 
CreateList() 
{
    DPRINT("%s()\n", __FUNCTION__);
    PLIST list = (PLIST)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(LIST));
    if (list) 
    {
        list->Head = NULL;
        list->NodeCount = 0;

        list->Add = _AddToList;
        list->Remove = _RemoveFromList;
        list->RemoveItem = _RemoveItem;
        list->Count = _CountOfList;
        list->Pop = _PopFromList;
        list->Free = _FreeList;
        list->GetItem = _GetItem;
    }
    else
    {
        DPRINT1("%s List is NULL!\n", __FUNCTION__);
    }
    
    return list;
}
