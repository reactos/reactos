/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/config/cmhook.c
 * PURPOSE:         Configuration Manager - Registry Notifications/Callbacks
 * PROGRAMMERS:     Thomas Weidenmueller (w3seek@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include "ntoskrnl.h"
#include "cm.h"
#define NDEBUG
#include "debug.h"

/* GLOBALS *******************************************************************/

ULONG CmpCallBackCount = 0;
EX_CALLBACK CmpCallBackVector[100];

LIST_ENTRY CmiCallbackHead;
FAST_MUTEX CmiCallbackLock;

typedef struct _REGISTRY_CALLBACK
{
    LIST_ENTRY ListEntry;
    EX_RUNDOWN_REF RundownRef;
    PEX_CALLBACK_FUNCTION Function;
    PVOID Context;
    LARGE_INTEGER Cookie;
    BOOLEAN PendingDelete;
} REGISTRY_CALLBACK, *PREGISTRY_CALLBACK;

/* PRIVATE FUNCTIONS *********************************************************/

VOID
NTAPI
CmpInitCallback(VOID)
{
    ULONG i;
    PAGED_CODE();

    /* Reset counter */
    CmpCallBackCount = 0;

    /* Loop all the callbacks */
    for (i = 0; i < CMP_MAX_CALLBACKS; i++)
    {
        /* Initialize this one */
        ExInitializeCallBack(&CmpCallBackVector[i]);
    }

    /* ROS: Initialize old-style callbacks for now */
    InitializeListHead(&CmiCallbackHead);
    ExInitializeFastMutex(&CmiCallbackLock);
}

NTSTATUS
CmiCallRegisteredCallbacks(IN REG_NOTIFY_CLASS Argument1,
                           IN PVOID Argument2)
{
    PLIST_ENTRY CurrentEntry;
    NTSTATUS Status = STATUS_SUCCESS;
    PREGISTRY_CALLBACK CurrentCallback;
    PAGED_CODE();

    ExAcquireFastMutex(&CmiCallbackLock);

    for (CurrentEntry = CmiCallbackHead.Flink;
         CurrentEntry != &CmiCallbackHead;
         CurrentEntry = CurrentEntry->Flink)
    {
        CurrentCallback = CONTAINING_RECORD(CurrentEntry, REGISTRY_CALLBACK, ListEntry);
        if (!CurrentCallback->PendingDelete &&
            ExAcquireRundownProtection(&CurrentCallback->RundownRef))
        {
            /* don't hold locks during the callbacks! */
            ExReleaseFastMutex(&CmiCallbackLock);

            Status = CurrentCallback->Function(CurrentCallback->Context,
                                         (PVOID)Argument1,
                                         Argument2);

            ExAcquireFastMutex(&CmiCallbackLock);

            /* don't release the rundown protection before holding the callback lock
            so the pointer to the next callback isn't cleared in case this callback
            get's deleted */
            ExReleaseRundownProtection(&CurrentCallback->RundownRef);
            if(!NT_SUCCESS(Status))
            {
                /* one callback returned failure, don't call any more callbacks */
                break;
            }
        }
    }

    ExReleaseFastMutex(&CmiCallbackLock);

    return Status;
}

/* PUBLIC FUNCTIONS **********************************************************/

/*
 * @implemented
 */
NTSTATUS
NTAPI
CmRegisterCallback(IN PEX_CALLBACK_FUNCTION Function,
                   IN PVOID Context,
                   IN OUT PLARGE_INTEGER Cookie)
{
    PREGISTRY_CALLBACK Callback;
    PAGED_CODE();
    ASSERT(Function && Cookie);

    Callback = ExAllocatePoolWithTag(PagedPool,
                                   sizeof(REGISTRY_CALLBACK),
                                   TAG('C', 'M', 'c', 'b'));
    if (Callback != NULL)
    {
        /* initialize the callback */
        ExInitializeRundownProtection(&Callback->RundownRef);
        Callback->Function = Function;
        Callback->Context = Context;
        Callback->PendingDelete = FALSE;

        /* add it to the callback list and receive a cookie for the callback */
        ExAcquireFastMutex(&CmiCallbackLock);

        /* FIXME - to receive a unique cookie we'll just return the pointer to the
           callback object */
        Callback->Cookie.QuadPart = (ULONG_PTR)Callback;
        InsertTailList(&CmiCallbackHead, &Callback->ListEntry);

        ExReleaseFastMutex(&CmiCallbackLock);

        *Cookie = Callback->Cookie;
        return STATUS_SUCCESS;
    }

    return STATUS_INSUFFICIENT_RESOURCES;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
CmUnRegisterCallback(IN LARGE_INTEGER Cookie)
{
    PLIST_ENTRY CurrentEntry;
    PREGISTRY_CALLBACK CurrentCallback;
    PAGED_CODE();

    ExAcquireFastMutex(&CmiCallbackLock);

    for (CurrentEntry = CmiCallbackHead.Flink;
         CurrentEntry != &CmiCallbackHead;
         CurrentEntry = CurrentEntry->Flink)
    {
        CurrentCallback = CONTAINING_RECORD(CurrentEntry, REGISTRY_CALLBACK, ListEntry);
        if (CurrentCallback->Cookie.QuadPart == Cookie.QuadPart)
        {
            if (!CurrentCallback->PendingDelete)
            {
                /* found the callback, don't unlink it from the list yet so we don't screw
                the calling loop */
                CurrentCallback->PendingDelete = TRUE;
                ExReleaseFastMutex(&CmiCallbackLock);

                /* if the callback is currently executing, wait until it finished */
                ExWaitForRundownProtectionRelease(&CurrentCallback->RundownRef);

                /* time to unlink it. It's now safe because every attempt to acquire a
                runtime protection on this callback will fail */
                ExAcquireFastMutex(&CmiCallbackLock);
                RemoveEntryList(&CurrentCallback->ListEntry);
                ExReleaseFastMutex(&CmiCallbackLock);

                /* free the callback */
                ExFreePool(CurrentCallback);
                return STATUS_SUCCESS;
            }
            else
            {
                /* pending delete, pretend like it already is deleted */
                ExReleaseFastMutex(&CmiCallbackLock);
                return STATUS_UNSUCCESSFUL;
            }
        }
    }

    ExReleaseFastMutex(&CmiCallbackLock);

    return STATUS_UNSUCCESSFUL;
}
