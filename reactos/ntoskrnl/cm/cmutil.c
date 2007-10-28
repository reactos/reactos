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
    PCM_DELAY_ALLOC Entry;
    PCM_ALLOC_PAGE AllocPage;
    ULONG i;
    PLIST_ENTRY NextEntry;
    PAGED_CODE();

    /* Lock the allocation buckets */
    KeAcquireGuardedMutex(&CmpDelayAllocBucketLock);
    
    /* Look for an item on the free list */
SearchList:
    if (!IsListEmpty(&CmpFreeDelayItemsListHead))
    {
        /* Get the current entry in the list */
        NextEntry = RemoveHeadList(&CmpFreeDelayItemsListHead);
        
        /* Grab the item */
        Entry = CONTAINING_RECORD(NextEntry, CM_DELAY_ALLOC, ListEntry);
        
        /* Clear the list */
        Entry->ListEntry.Flink = Entry->ListEntry.Blink = NULL;
        
        /* Grab the alloc page */
        AllocPage = (PCM_ALLOC_PAGE)((ULONG_PTR)Entry & 0xFFFFF000);
        
        /* Decrease free entries */
        ASSERT(AllocPage->FreeCount != 0);
        AllocPage->FreeCount--;
        
        /* Release the lock */
        KeReleaseGuardedMutex(&CmpDelayAllocBucketLock);
        return Entry;
    }
    
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
            Entry = (PVOID)((ULONG_PTR)AllocPage +
                            FIELD_OFFSET(CM_ALLOC_PAGE, AllocPage) +
                            i * sizeof(CM_DELAY_ALLOC));
            InsertTailList(&CmpFreeDelayItemsListHead,
                           &Entry->ListEntry);
            
            /* Clear the KCB pointer */
            Entry->Kcb = NULL;
        }
    }
    else
    {
        /* Release the lock */
        KeReleaseGuardedMutex(&CmpDelayAllocBucketLock);
        return NULL;
    }
    
    /* Do the search again */
    goto SearchList;
}

VOID
NTAPI
CmpFreeDelayItem(PVOID Entry)
{
    PCM_DELAY_ALLOC AllocEntry = (PCM_DELAY_ALLOC)Entry;
    PCM_ALLOC_PAGE AllocPage;
    ULONG i;
    PAGED_CODE();

    /* Lock the table */
    KeAcquireGuardedMutex(&CmpDelayAllocBucketLock);

    /* Add the entry at the end */
    InsertTailList(&CmpFreeDelayItemsListHead, &AllocEntry->ListEntry);

    /* Get the alloc page */
    AllocPage = (PCM_ALLOC_PAGE)((ULONG_PTR)Entry & 0xFFFFF000);
    ASSERT(AllocPage->FreeCount != CM_DELAYS_PER_PAGE);

    /* Increase the number of free items */
    if (++AllocPage->FreeCount == CM_DELAYS_PER_PAGE)
    {
        /* Page is totally free now, loop each entry */
        for (i = 0; i < CM_DELAYS_PER_PAGE; i++)
        {
            /* Get the entry and unlink it */
            AllocEntry = (PVOID)((ULONG_PTR)AllocPage +
                                 FIELD_OFFSET(CM_ALLOC_PAGE, AllocPage) +
                                 i * sizeof(CM_DELAY_ALLOC));
            RemoveEntryList(&AllocEntry->ListEntry);
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
