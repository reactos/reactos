/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/cm/cmapi.c
 * PURPOSE:         Internal routines that implement Nt* API functionality
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>
#include "cm.h"

ERESOURCE CmpRegistryLock;
PVOID CmpRegistryLockCallerCaller, CmpRegistryLockCaller;
BOOLEAN CmpFlushStarveWriters;

KGUARDED_MUTEX CmpDelayAllocBucketLock;
LIST_ENTRY CmpFreeDelayItemsListHead;

/* FUNCTIONS *****************************************************************/

VOID
NTAPI
CmpLockRegistryExclusive(VOID)
{
    /* Enter a critical region and lock the registry */
    KeEnterCriticalRegion();
    ExAcquireResourceExclusiveLite(&CmpRegistryLock, TRUE);

    /* Sanity check */
    ASSERT(CmpFlushStarveWriters == 0);
    RtlGetCallersAddress(&CmpRegistryLockCaller, &CmpRegistryLockCallerCaller);
}

BOOLEAN
NTAPI
CmpTestRegistryLock(VOID)
{
    /* Test the lock */
    return (BOOLEAN)ExIsResourceAcquiredSharedLite(&CmpRegistryLock);
}

BOOLEAN
NTAPI
CmpTestRegistryLockExclusive(VOID)
{
    /* Test the lock */
    return ExIsResourceAcquiredExclusiveLite(&CmpRegistryLock);
}

PVOID
NTAPI
CmpAllocateDelayItem(VOID)
{
    PCM_DELAYED_CLOSE_ENTRY Entry;
    PCM_ALLOC_PAGE AllocPage;
    ULONG i;
    PLIST_ENTRY NextEntry;
    PAGED_CODE();

    /* Lock the allocation buckets */
    KeAcquireGuardedMutex(&CmpDelayAllocBucketLock);
    while (TRUE)
    {
        /* Get the current entry in the list */
        NextEntry = CmpFreeDelayItemsListHead.Flink;

        /* Check if we need to allocate an entry */
        if (((NextEntry) && (CmpFreeDelayItemsListHead.Blink) &&
             IsListEmpty(&CmpFreeDelayItemsListHead)) ||
            (!(NextEntry) && !(CmpFreeDelayItemsListHead.Blink)))
        {
            /* Allocate an allocation page */
            AllocPage = ExAllocatePoolWithTag(PagedPool, PAGE_SIZE, TAG_CM);
            if (AllocPage)
            {
                /* Set default entries */
                AllocPage->FreeCount = CM_DELAYS_PER_PAGE;

                /* Loop each entry */
                for (i = 0; i < CM_DELAYS_PER_PAGE; i++)
                {
                    /* Get this entry and link it */
                    Entry = (PCM_DELAYED_CLOSE_ENTRY)(&AllocPage[i]);
                    InsertHeadList(&Entry->DelayedLRUList,
                                   &CmpFreeDelayItemsListHead);

                    /* Clear the KCB pointer */
                    Entry->KeyControlBlock = NULL;
                }
            }
            else
            {
                /* Release the lock */
                KeReleaseGuardedMutex(&CmpDelayAllocBucketLock);
                return NULL;
            }
        }
    }

    /* Set the next item in the list */
    CmpFreeDelayItemsListHead.Flink = NextEntry->Flink;
    NextEntry->Flink->Blink = &CmpFreeDelayItemsListHead;
    NextEntry->Blink = NULL;

    /* Get the entry and the alloc page */
    Entry = CONTAINING_RECORD(NextEntry,
                              CM_DELAYED_CLOSE_ENTRY,
                              DelayedLRUList);
    AllocPage = (PCM_ALLOC_PAGE)((ULONG_PTR)Entry & 0xFFFFF000);

    /* Decrease free entries */
    ASSERT(AllocPage->FreeCount != 0);
    AllocPage->FreeCount--;

    /* Release the lock */
    KeReleaseGuardedMutex(&CmpDelayAllocBucketLock);
    return Entry;
}

VOID
NTAPI
CmpFreeDelayItem(PVOID Entry)
{
    PCM_DELAYED_CLOSE_ENTRY AllocEntry = (PCM_DELAYED_CLOSE_ENTRY)Entry;
    PCM_DELAYED_CLOSE_ENTRY *AllocTable;
    PCM_ALLOC_PAGE AllocPage;
    ULONG i;
    PAGED_CODE();

    /* Lock the table */
    KeAcquireGuardedMutex(&CmpDelayAllocBucketLock);

    /* Add the entry at the end */
    InsertTailList(&CmpFreeDelayItemsListHead, &AllocEntry->DelayedLRUList);

    /* Get the alloc page */
    AllocPage = (PCM_ALLOC_PAGE)((ULONG_PTR)Entry & 0xFFFFF000);
    ASSERT(AllocPage->FreeCount != CM_DELAYS_PER_PAGE);

    /* Increase the number of free items */
    if (++AllocPage->FreeCount == CM_DELAYS_PER_PAGE)
    {
        /* Page is totally free now, loop each entry */
        AllocTable = (PCM_DELAYED_CLOSE_ENTRY*)&AllocPage->AllocPage;
        for (i = CM_DELAYS_PER_PAGE; i; i--)
        {
            /* Get the entry and unlink it */
            AllocEntry = AllocTable[i];
            RemoveEntryList(&AllocEntry->DelayedLRUList);
        }

        /* Now free the page */
        ExFreePool(AllocPage);
    }

    /* Release the lock */
    KeReleaseGuardedMutex(&CmpDelayAllocBucketLock);
}

VOID
NTAPI
CmpUnlockRegistry(VOID)
{
    /* Sanity check */
    CMP_ASSERT_REGISTRY_LOCK();

    /* Check if we should flush the registry */
    if (CmpFlushOnLockRelease)
    {
        /* The registry should be exclusively locked for this */
        CMP_ASSERT_EXCLUSIVE_REGISTRY_LOCK();

        /* Flush the registry */
        CmpFlushEntireRegistry(TRUE);
        CmpFlushOnLockRelease = FALSE;
    }

    /* Release the lock and leave the critical region */
    ExReleaseResourceLite(&CmpRegistryLock);
    KeLeaveCriticalRegion();
}
