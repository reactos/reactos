/*
 * PROJECT:    ReactOS NetSh
 * LICENSE:    GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:    Stack code
 * COPYRIGHT:  Copyright 2025 Curtis Wilson <LiquidFox1776@gmail.com>
 */

/* INCLUDES *******************************************************************/

#include "precomp.h"

#define NDEBUG
#include <debug.h>
STACK* 
CreateStack() 
{
    DPRINT1("%s()\n",  __FUNCTION__);
    STACK* pStack = (STACK*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(STACK));
    if (pStack == NULL) {
        DPRINT1("%s Memory allocation failed\n",  __FUNCTION__);
        return NULL;
    }
    pStack->pTop = NULL;
    return pStack;
}


void 
StackPush(STACK *pStack, void *pData) 
{
    DPRINT1("%s()\n",  __FUNCTION__);
    if (pStack == NULL)
    {
        DPRINT1("%s Stack is NULL\n",  __FUNCTION__);
        return;
    }
    
    STATCK_NODE *pNewNode = (STATCK_NODE*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(STATCK_NODE));
    if (pNewNode == NULL) 
    {
        DPRINT1("%s Memory allocation failed\n",  __FUNCTION__);
        return;
    }
    
    pNewNode->pData = pData;
    pNewNode->pNext = pStack->pTop;
    pStack->pTop = pNewNode;
}


void* 
StackPop(STACK *pStack) 
{
    DPRINT1("%s()\n",  __FUNCTION__);
    
    if (pStack == NULL)
    {
        DPRINT1("%s Stack is NULL\n",  __FUNCTION__);
        return NULL;
    }
    
    if (pStack->pTop == NULL) 
    {
        DPRINT1("%s Stack is empty\n",  __FUNCTION__);
        return NULL;
    }
    
    STATCK_NODE *pTempNode = pStack->pTop;
    void *pPoppedEntry = pTempNode->pData;
    pStack->pTop = pStack->pTop->pNext;
    HeapFree(GetProcessHeap(), 0, pTempNode); // Free the stack node
    
    return pPoppedEntry;
}


void* 
StackPeek(STACK* pStack) 
{
    DPRINT1("%s()\n",  __FUNCTION__);
    
    if (pStack->pTop == NULL) 
    {
        DPRINT1("%s Stack is empty\n", __FUNCTION__);
        return NULL;
    }
    return pStack->pTop->pData;
}


void 
StackFree(STACK *pStack, BOOL bFreeData) 
{
    DPRINT1("%s()\n",  __FUNCTION__);
    
    STATCK_NODE* pCurrent = pStack->pTop;
    STATCK_NODE* pNextNode;
    
    while (pCurrent != NULL) 
    {
        pNextNode = pCurrent->pNext;
        
        if (bFreeData == TRUE)
            HeapFree(GetProcessHeap(), 0, pCurrent->pData);
            
        HeapFree(GetProcessHeap(), 0, pCurrent);
        pCurrent = pNextNode;
    }
    
    HeapFree(GetProcessHeap(), 0, pStack);
}

BOOL IsStackEmpty(STACK *pStack)
{
    if (pStack != NULL)
        return (pStack->pTop == NULL) ? TRUE : FALSE;
        
    return TRUE;
}

