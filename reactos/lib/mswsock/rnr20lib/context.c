/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS Winsock 2 SPI
 * FILE:        lib/mswsock/lib/init.c
 * PURPOSE:     DLL Initialization
 */

/* INCLUDES ******************************************************************/
#include "msafd.h"

/* DATA **********************************************************************/

LIST_ENTRY ListAnchor;
BOOLEAN g_fRnrLockInit;
CRITICAL_SECTION g_RnrLock;

#define AcquireRnR2Lock() EnterCriticalSection(&g_RnrLock);
#define ReleaseRnR2Lock() LeaveCriticalSection(&g_RnrLock);

/* FUNCTIONS *****************************************************************/

PRNR_CONTEXT
WSPAPI
RnrCtx_Create(IN HANDLE LookupHandle,
              IN LPWSTR ServiceName)
{
    PRNR_CONTEXT RnrContext;
    SIZE_T StringSize = 0;

    /* Get the size of the string */
    if (ServiceName) StringSize = wcslen(ServiceName);

    /* Allocate the Context */
    RnrContext = Temp_AllocZero(sizeof(RNR_CONTEXT) + (DWORD)StringSize);
    
    /* Check that we got one */
    if (RnrContext)
    {
        /* Set it up */
        RnrContext->RefCount = 2;
        RnrContext->Handle = (LookupHandle ? LookupHandle : (HANDLE)RnrContext);
        RnrContext->Instance = -1;
        RnrContext->Signature = 0xaabbccdd;
        wcscpy(RnrContext->ServiceName, ServiceName);

        /* Insert it into the list */
        AcquireRnR2Lock();
        InsertHeadList(&ListAnchor, &RnrContext->ListEntry);
        ReleaseRnR2Lock();
    }

    /* Return it */
    return RnrContext;
}

VOID
WSPAPI
RnrCtx_Release(PRNR_CONTEXT RnrContext)
{
    /* Acquire the lock */
    AcquireRnR2Lock();

    /* Decrease reference count and check if it's still in use */
    if(!(--RnrContext->RefCount))
    {
        /* Remove it from the List */
        RemoveEntryList(&RnrContext->ListEntry);

        /* Release the lock */
        ReleaseRnR2Lock();

        /* Deallocated any cached Hostent */
        if(RnrContext->CachedSaBlob) SaBlob_Free(RnrContext->CachedSaBlob);

        /* Deallocate the Blob */
        if(RnrContext->CachedBlob.pBlobData)
        {
            DnsApiFree(RnrContext->CachedBlob.pBlobData);
        }

        /* Deallocate the actual context itself */
        DnsApiFree(RnrContext);
    }
    else
    {
        /* Release the lock */
        ReleaseRnR2Lock();
    }
}

PRNR_CONTEXT
WSPAPI
RnrCtx_Get(HANDLE LookupHandle,
           DWORD dwControlFlags,
           PLONG Instance)
{
    PLIST_ENTRY Entry;
    PRNR_CONTEXT RnRContext = NULL;

    /* Acquire the lock */
    AcquireRnR2Lock();

    /* Loop the RNR Context List */
    for(Entry = ListAnchor.Flink; Entry != &ListAnchor; Entry = Entry->Flink)
    {
        /* Get the Current RNR Context */
        RnRContext = CONTAINING_RECORD(Entry, RNR_CONTEXT, ListEntry);

        /* Check if it matches the one we got */
        if(RnRContext == (PRNR_CONTEXT)LookupHandle) break;
    }

    /* If we found it, mark it in use */
    if(RnRContext) RnRContext->RefCount++;

    /* Increase the Instance and return it */
    *Instance = ++RnRContext->Instance;

    /* If we're flushing the previous one, then bias the Instance by one */
    if(dwControlFlags & LUP_FLUSHPREVIOUS) *Instance = ++RnRContext->Instance;

    /* Release the lock */
    ReleaseRnR2Lock();

    /* Return the Context */
    return RnRContext;
}

VOID
WSPAPI
RnrCtx_DecInstance(IN PRNR_CONTEXT RnrContext)
{
    /* Acquire the lock */
    AcquireRnR2Lock();

    /* Decrease instance count */
    RnrContext->Instance--;

    /* Release the lock */
    ReleaseRnR2Lock();
}

VOID
WSPAPI
RnrCtx_ListCleanup(VOID)
{
    PLIST_ENTRY Entry;

    /* Acquire RnR Lock */
    AcquireRnR2Lock();

    /* Loop the contexts */
    while ((Entry = ListAnchor.Flink) != &ListAnchor)
    {
        /* Release this context */
        RnrCtx_Release(CONTAINING_RECORD(Entry, RNR_CONTEXT, ListEntry));
    }

    /* Release lock */
    ReleaseRnR2Lock();
}
