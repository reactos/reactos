/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/config/cmdelay.c
 * PURPOSE:         Routines for handling delay close and allocate.
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

WORK_QUEUE_ITEM CmpDelayDerefKCBWorkItem;
LIST_ENTRY CmpFreeDelayItemsListHead;

ULONG CmpDelayedCloseSize = 2048;
ULONG CmpDelayedCloseElements;
KGUARDED_MUTEX CmpDelayedCloseTableLock;
BOOLEAN CmpDelayCloseWorkItemActive;
WORK_QUEUE_ITEM CmpDelayCloseWorkItem;
LIST_ENTRY CmpDelayedLRUListHead;
ULONG CmpDelayCloseIntervalInSeconds = 5;
KDPC CmpDelayCloseDpc;
KTIMER CmpDelayCloseTimer;

KGUARDED_MUTEX CmpDelayDerefKCBLock;
BOOLEAN CmpDelayDerefKCBWorkItemActive;
LIST_ENTRY CmpDelayDerefKCBListHead;
ULONG CmpDelayDerefKCBIntervalInSeconds = 5;
KDPC CmpDelayDerefKCBDpc;
KTIMER CmpDelayDerefKCBTimer;

/* FUNCTIONS *****************************************************************/

VOID
NTAPI
CmpDelayCloseDpcRoutine(IN PKDPC Dpc,
                        IN PVOID DeferredContext,
                        IN PVOID SystemArgument1,
                        IN PVOID SystemArgument2)
{
    /* Sanity check */
    ASSERT(CmpDelayCloseWorkItemActive);

    /* Queue the work item */
    ExQueueWorkItem(&CmpDelayCloseWorkItem, DelayedWorkQueue);
}

VOID
NTAPI
CmpDelayCloseWorker(IN PVOID Context)
{
    PCM_DELAYED_CLOSE_ENTRY ListEntry;
    ULONG i, ConvKey;
    PAGED_CODE();

    /* Sanity check */
    ASSERT(CmpDelayCloseWorkItemActive);

    /* Lock the registry */
    CmpLockRegistry();

    /* Acquire the delayed close table lock */
    KeAcquireGuardedMutex(&CmpDelayedCloseTableLock);

    /* Iterate */
    for (i = 0; i < (CmpDelayedCloseSize >> 2); i++)
    {
        /* Break out of the loop if there is nothing to process */
        if (CmpDelayedCloseElements <= CmpDelayedCloseSize) break;

        /* Sanity check */
        ASSERT(!IsListEmpty(&CmpDelayedLRUListHead));

        /* Get the entry */
        ListEntry = CONTAINING_RECORD(CmpDelayedLRUListHead.Blink,
                                      CM_DELAYED_CLOSE_ENTRY,
                                      DelayedLRUList);

        /* Save the ConvKey value of the KCB */
        ConvKey = ListEntry->KeyControlBlock->ConvKey;

        /* Release the delayed close table lock */
        KeReleaseGuardedMutex(&CmpDelayedCloseTableLock);

        /* Acquire the KCB lock */
        CmpAcquireKcbLockExclusiveByKey(ConvKey);

        /* Reacquire the delayed close table lock */
        KeAcquireGuardedMutex(&CmpDelayedCloseTableLock);

        /* Get the entry */
        ListEntry = CONTAINING_RECORD(CmpDelayedLRUListHead.Blink,
                                      CM_DELAYED_CLOSE_ENTRY,
                                      DelayedLRUList);

        /* Is the entry we have still the first one? */
        if (CmpDelayedCloseElements <= CmpDelayedCloseSize)
        {
            /* No, someone already inserted an entry there */
            CmpReleaseKcbLockByKey(ConvKey);
            break;
        }

        /* Is it a different entry? */
        if (ConvKey != ListEntry->KeyControlBlock->ConvKey)
        {
            /* Release the delayed close table lock */
            KeReleaseGuardedMutex(&CmpDelayedCloseTableLock);

            /* Release the KCB lock */
            CmpReleaseKcbLockByKey(ConvKey);

            /* Reacquire the delayed close table lock */
            KeAcquireGuardedMutex(&CmpDelayedCloseTableLock);

            /* Iterate again */
            continue;
        }

        /* Remove it from the end of the list */
        ListEntry =
            (PCM_DELAYED_CLOSE_ENTRY)RemoveTailList(&CmpDelayedLRUListHead);

        /* Get the containing entry */
        ListEntry = CONTAINING_RECORD(ListEntry,
                                      CM_DELAYED_CLOSE_ENTRY,
                                      DelayedLRUList);

        /* Process the entry */
        if ((ListEntry->KeyControlBlock->RefCount) ||
            (ListEntry->KeyControlBlock->DelayedCloseIndex))
        {
            /* Add it to the beginning of the list */
            InsertHeadList(&CmpDelayedLRUListHead, &ListEntry->DelayedLRUList);

            /* Release the delayed close table lock */
            KeReleaseGuardedMutex(&CmpDelayedCloseTableLock);
        }
        else
        {
            /* Release the delayed close table lock */
            KeReleaseGuardedMutex(&CmpDelayedCloseTableLock);

            /* Zero out the DelayCloseEntry pointer */
            ListEntry->KeyControlBlock->DelayCloseEntry = NULL;

            /* Cleanup the KCB cache */
            CmpCleanUpKcbCacheWithLock(ListEntry->KeyControlBlock, FALSE);

            /* Free the delay item */
            CmpFreeDelayItem(ListEntry);

            /* Decrement delayed close elements count */
            InterlockedDecrement((PLONG)&CmpDelayedCloseElements);
        }

        /* Release the KCB lock */
        CmpReleaseKcbLockByKey(ConvKey);

        /* Reacquire the delayed close table lock */
        KeAcquireGuardedMutex(&CmpDelayedCloseTableLock);
    }

    if (CmpDelayedCloseElements <= CmpDelayedCloseSize)
    {
        /* We're not active anymore */
        CmpDelayCloseWorkItemActive = FALSE;
    }
    else
    {
        /* We didn't process all things, so reschedule for the next time */
        CmpArmDelayedCloseTimer();
    }

    /* Release the delayed close table lock */
    KeReleaseGuardedMutex(&CmpDelayedCloseTableLock);

    /* Unlock the registry */
    CmpUnlockRegistry();
}

VOID
NTAPI
CmpInitializeDelayedCloseTable(VOID)
{
    
    /* Setup the delayed close lock */
    KeInitializeGuardedMutex(&CmpDelayedCloseTableLock);
    
    /* Setup the work item */
    ExInitializeWorkItem(&CmpDelayCloseWorkItem, CmpDelayCloseWorker, NULL);
    
    /* Setup the list head */
    InitializeListHead(&CmpDelayedLRUListHead);
    
    /* Setup the DPC and its timer */
    KeInitializeDpc(&CmpDelayCloseDpc, CmpDelayCloseDpcRoutine, NULL);
    KeInitializeTimer(&CmpDelayCloseTimer);
}

VOID
NTAPI
CmpDelayDerefKCBDpcRoutine(IN PKDPC Dpc,
                           IN PVOID DeferredContext,
                           IN PVOID SystemArgument1,
                           IN PVOID SystemArgument2)
{
    /* Sanity check */
    ASSERT(CmpDelayDerefKCBWorkItemActive);

    /* Queue the work item */
    ExQueueWorkItem(&CmpDelayDerefKCBWorkItem, DelayedWorkQueue);
}

VOID
NTAPI
CmpDelayDerefKCBWorker(IN PVOID Context)
{
    PCM_DELAY_DEREF_KCB_ITEM Entry;
    PAGED_CODE();

    /* Sanity check */
    ASSERT(CmpDelayDerefKCBWorkItemActive);

    /* Lock the registry and and list lock */
    CmpLockRegistry();
    KeAcquireGuardedMutex(&CmpDelayDerefKCBLock);

    /* Check if the list is empty */
    while (!IsListEmpty(&CmpDelayDerefKCBListHead))
    {
        /* Grab an entry */
        Entry = (PVOID)RemoveHeadList(&CmpDelayDerefKCBListHead);
        
        /* We can release the lock now */
        KeReleaseGuardedMutex(&CmpDelayDerefKCBLock);
        
        /* Now grab the actual entry */
        Entry = CONTAINING_RECORD(Entry, CM_DELAY_DEREF_KCB_ITEM, ListEntry);
        Entry->ListEntry.Flink = Entry->ListEntry.Blink = NULL;
        
        /* Dereference and free */
        CmpDereferenceKeyControlBlock(Entry->Kcb);
        CmpFreeDelayItem(Entry);
        
        /* Lock the list again */
        KeAcquireGuardedMutex(&CmpDelayDerefKCBLock);
    }
    
    /* We're done */
    CmpDelayDerefKCBWorkItemActive = FALSE;
    KeReleaseGuardedMutex(&CmpDelayDerefKCBLock);
    CmpUnlockRegistry();
}

VOID
NTAPI
CmpInitDelayDerefKCBEngine(VOID)
{
    /* Initialize lock and list */
    KeInitializeGuardedMutex(&CmpDelayDerefKCBLock);
    InitializeListHead(&CmpDelayDerefKCBListHead);

    /* Setup the work item */
    ExInitializeWorkItem(&CmpDelayDerefKCBWorkItem,
                         CmpDelayDerefKCBWorker,
                         NULL);

    /* Setup the DPC and timer for it */
    KeInitializeDpc(&CmpDelayDerefKCBDpc, CmpDelayDerefKCBDpcRoutine, NULL);
    KeInitializeTimer(&CmpDelayDerefKCBTimer);
}

VOID
NTAPI
CmpDelayDerefKeyControlBlock(IN PCM_KEY_CONTROL_BLOCK Kcb)
{
    LONG OldRefCount, NewRefCount;
    LARGE_INTEGER Timeout;
    PCM_DELAY_DEREF_KCB_ITEM Entry;
    PAGED_CODE();

    /* Get the previous reference count */
    OldRefCount = *(PLONG)&Kcb->RefCount;
    NewRefCount = OldRefCount - 1;
    if (((NewRefCount & 0xFFFF) > 0) &&
        (InterlockedCompareExchange((PLONG)&Kcb->RefCount,
                                    NewRefCount,
                                    OldRefCount) == OldRefCount))
    {
        /* KCB still had references, so we're done */
        return;
    }

    /* Allocate a delay item */
    Entry = CmpAllocateDelayItem();
    if (!Entry) return;

    /* Set the KCB */
    Entry->Kcb = Kcb;

    /* Acquire the delayed deref table lock */
    KeAcquireGuardedMutex(&CmpDelayDerefKCBLock);

    /* Insert the entry into the list */
    InsertTailList(&CmpDelayDerefKCBListHead, &Entry->ListEntry);

    /* Check if we need to enable anything */
    if (!CmpDelayDerefKCBWorkItemActive)
    {
        /* Yes, we have no work item, setup the interval */
        CmpDelayDerefKCBWorkItemActive = TRUE;
        Timeout.QuadPart = CmpDelayDerefKCBIntervalInSeconds * -10000000;
        KeSetTimer(&CmpDelayDerefKCBTimer, Timeout, &CmpDelayDerefKCBDpc);
    }

    /* Release the table lock */
    KeReleaseGuardedMutex(&CmpDelayDerefKCBLock);
}

VOID
NTAPI
CmpArmDelayedCloseTimer(VOID)
{
    LARGE_INTEGER Timeout;
    PAGED_CODE();
    
    /* Set the worker active */
    CmpDelayCloseWorkItemActive = TRUE;

    /* Setup the interval */
    Timeout.QuadPart = CmpDelayCloseIntervalInSeconds * -10000000;
    KeSetTimer(&CmpDelayCloseTimer, Timeout, &CmpDelayCloseDpc);
}

VOID
NTAPI
CmpAddToDelayedClose(IN PCM_KEY_CONTROL_BLOCK Kcb,
                     IN BOOLEAN LockHeldExclusively)
{
    ULONG i;
    ULONG OldRefCount, NewRefCount;
    PCM_DELAYED_CLOSE_ENTRY Entry;
    PAGED_CODE();

    /* Sanity check */
    ASSERT((CmpIsKcbLockedExclusive(Kcb) == TRUE) ||
           (CmpTestRegistryLockExclusive() == TRUE));

    /* Make sure it's valid */
    if (Kcb->DelayedCloseIndex != CmpDelayedCloseSize) ASSERT(FALSE);

    /* Sanity checks */
    ASSERT(Kcb->RefCount == 0);
    ASSERT(IsListEmpty(&Kcb->KeyBodyListHead) == TRUE);
    for (i = 0; i < 4; i++) ASSERT(Kcb->KeyBodyArray[i] == NULL);

    /* Allocate a delay item */
    Entry = CmpAllocateDelayItem();
    if (!Entry)
    {
        /* Cleanup immediately */
        CmpCleanUpKcbCacheWithLock(Kcb, LockHeldExclusively);
        return;
    }

    /* Sanity check */
    if (Kcb->InDelayClose) ASSERT(FALSE);

    /* Get the previous reference count */
    OldRefCount = *(PLONG)&Kcb->InDelayClose;
    ASSERT(OldRefCount == 0);

    /* Write the new one */
    NewRefCount = 1;
    if (InterlockedCompareExchange((PLONG)&Kcb->InDelayClose,
                                   NewRefCount,
                                   OldRefCount) != OldRefCount)
    {
        /* Sanity check */
        ASSERT(FALSE);
    }

    /* Reset the delayed close index */
    Kcb->DelayedCloseIndex = 0;

    /* Set up the close entry */
    Kcb->DelayCloseEntry = Entry;
    Entry->KeyControlBlock = Kcb;

    /* Increase the number of elements */
    InterlockedIncrement((PLONG)&CmpDelayedCloseElements);

    /* Acquire the delayed close table lock */
    KeAcquireGuardedMutex(&CmpDelayedCloseTableLock);

    /* Insert the entry into the list */
    InsertHeadList(&CmpDelayedLRUListHead, &Entry->DelayedLRUList);

    /* Check if we need to enable anything */
    if ((CmpDelayedCloseElements > CmpDelayedCloseSize) &&
        !(CmpDelayCloseWorkItemActive))
    {
        /* Yes, we have too many elements to close, and no work item */
        CmpArmDelayedCloseTimer();
    }

    /* Release the table lock */
    KeReleaseGuardedMutex(&CmpDelayedCloseTableLock);
}

VOID
NTAPI
CmpRemoveFromDelayedClose(IN PCM_KEY_CONTROL_BLOCK Kcb)
{
    PCM_DELAYED_CLOSE_ENTRY Entry;
    ULONG NewRefCount, OldRefCount;
    PAGED_CODE();
    
    /* Sanity checks */
    ASSERT((CmpIsKcbLockedExclusive(Kcb) == TRUE) ||
           (CmpTestRegistryLockExclusive() == TRUE));
    if (Kcb->DelayedCloseIndex == CmpDelayedCloseSize) ASSERT(FALSE);
    
    /* Get the entry and lock the table */
    Entry = Kcb->DelayCloseEntry;
    ASSERT(Entry);
    KeAcquireGuardedMutex(&CmpDelayedCloseTableLock);
    
    /* Remove the entry */
    RemoveEntryList(&Entry->DelayedLRUList);
    
    /* Release the lock */
    KeReleaseGuardedMutex(&CmpDelayedCloseTableLock);
    
    /* Free the entry */
    CmpFreeDelayItem(Entry);
    
    /* Reduce the number of elements */
    InterlockedDecrement((PLONG)&CmpDelayedCloseElements);
    
    /* Sanity check */
    if (!Kcb->InDelayClose) ASSERT(FALSE);
    
    /* Get the previous reference count */
    OldRefCount = *(PLONG)&Kcb->InDelayClose;
    ASSERT(OldRefCount == 1);
    
    /* Write the new one */
    NewRefCount = 0;
    if (InterlockedCompareExchange((PLONG)&Kcb->InDelayClose,
                                   NewRefCount,
                                   OldRefCount) != OldRefCount)
    {
        /* Sanity check */
        ASSERT(FALSE);
    }
    
    /* Remove the link to the entry */
    Kcb->DelayCloseEntry = NULL;
    
    /* Set new delay size and remove the delete flag */
    Kcb->DelayedCloseIndex = CmpDelayedCloseSize;
}
