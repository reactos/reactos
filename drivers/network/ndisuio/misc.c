/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS NDIS User I/O driver
 * FILE:        misc.c
 * PURPOSE:     Helper functions
 * PROGRAMMERS: Cameron Gutman (cameron.gutman@reactos.org)
 */

#include "ndisuio.h"

#define NDEBUG
#include <debug.h>

PNDISUIO_ADAPTER_CONTEXT
FindAdapterContextByName(PNDIS_STRING DeviceName)
{
    KIRQL OldIrql;
    PLIST_ENTRY CurrentEntry;
    PNDISUIO_ADAPTER_CONTEXT AdapterContext;

    KeAcquireSpinLock(&GlobalAdapterListLock, &OldIrql);
    CurrentEntry = GlobalAdapterList.Flink;
    while (CurrentEntry != &GlobalAdapterList)
    {
        AdapterContext = CONTAINING_RECORD(CurrentEntry, NDISUIO_ADAPTER_CONTEXT, ListEntry);
        
        /* Check if the device name matches */
        if (RtlEqualUnicodeString(&AdapterContext->DeviceName, DeviceName, TRUE))
        {
            KeAcquireSpinLockAtDpcLevel(&AdapterContext->Spinlock);

            /* Check that it's not being destroyed */
            if (AdapterContext->OpenCount > 0)
            {
                KeReleaseSpinLockFromDpcLevel(&AdapterContext->Spinlock);
                KeReleaseSpinLock(&GlobalAdapterListLock, OldIrql);
                return AdapterContext;
            }
            else
            {
                KeReleaseSpinLockFromDpcLevel(&Adaptercontext->Spinlock);
            }
        }
        
        CurrentEntry = CurrentEntry->Flink;
    }
    KeReleaseSpinLock(&GlobalAdapterListLock, OldIrql);
    
    return NULL;
}

VOID
ReferenceAdapterContext(PNDISUIO_ADAPTER_CONTEXT AdapterContext, BOOLEAN Locked)
{
    KIRQL OldIrql;

    /* Lock if needed */
    if (!Locked)
    {
        KeAcquireSpinLock(&AdapterContext->Spinlock, &OldIrql);
    }

    /* Increment the open count */
    AdapterContext->OpenCount++;
    
    /* Unlock if needed */
    if (!Locked)
    {
        KeReleaseSpinLock(&AdapterContext->Spinlock, OldIrql);
    }
}

VOID
DereferenceAdapterContextWithOpenEntry(PNDISUIO_ADAPTER_CONTEXT AdapterContext,
                                       PNDISUIO_OPEN_ENTRY OpenEntry)
{
    KIRQL OldIrql;

    /* Lock the adapter context */
    KeAcquireSpinLock(&AdapterContext->Spinlock, &OldIrql);
    
    /* Decrement the open count */
    AdapterContext->OpenCount--;

    /* Cleanup the open entry if we were given one */
    if (OpenEntry != NULL)
    {
        /* Remove the open entry */
        RemoveEntryList(&OpenEntry->ListEntry);

        /* Invalidate the FO */
        OpenEntry->FileObject->FsContext = NULL;
        OpenEntry->FileObject->FsContext2 = NULL;

        /* Free the open entry */
        ExFreePool(OpenEntry);
    }
    
    /* See if this binding can be destroyed */
    if (AdapterContext->OpenCount == 0)
    {
        /* Unlock the context */
        KeReleaseSpinLock(&AdapterContext->Spinlock, OldIrql);

        /* Destroy the adapter context */
        UnbindAdapterByContext(AdapterContext);
    }
    else
    {
        /* Still more references on it */
        KeReleaseSpinLock(&AdapterContext->Spinlock, OldIrql);
    }
}
