/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/config/cmalloc.c
 * PURPOSE:         Routines for allocating and freeing registry structures
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>
#include "cm.h"

/* GLOBALS *******************************************************************/

BOOLEAN CmpAllocInited;
KGUARDED_MUTEX CmpAllocBucketLock, CmpDelayAllocBucketLock;

LIST_ENTRY CmpFreeKCBListHead;
KGUARDED_MUTEX CmpDelayAllocBucketLock;
LIST_ENTRY CmpFreeDelayItemsListHead;

/* FUNCTIONS *****************************************************************/

VOID
NTAPI
CmpInitCmPrivateAlloc(VOID)
{
    /* Make sure we didn't already do this */
    if (!CmpAllocInited)
    {
        /* Setup the lock and list */
        KeInitializeGuardedMutex(&CmpAllocBucketLock);
        InitializeListHead(&CmpFreeKCBListHead);
        CmpAllocInited = TRUE;
    }
}

VOID
NTAPI
CmpInitCmPrivateDelayAlloc(VOID)
{
    /* Initialize the delay allocation list and lock */
    KeInitializeGuardedMutex(&CmpDelayAllocBucketLock);
    InitializeListHead(&CmpFreeDelayItemsListHead);
}

VOID
NTAPI
CmpFreeKeyControlBlock(IN PCM_KEY_CONTROL_BLOCK Kcb)
{
    ULONG i;
    PCM_ALLOC_PAGE AllocPage;
    PAGED_CODE();
    
    /* Sanity checks */
    ASSERT(IsListEmpty(&(Kcb->KeyBodyListHead)) == TRUE);
    for (i = 0; i < 4; i++) ASSERT(Kcb->KeyBodyArray[i] == NULL);
    
    /* Check if it wasn't privately allocated */
    if (!Kcb->PrivateAlloc)
    {
        /* Free it from the pool */
        ExFreePool(Kcb);
        return;
    }
    
    /* Acquire the private allocation lock */
    KeAcquireGuardedMutex(&CmpAllocBucketLock);
    
    /* Sanity check on lock ownership */
    ASSERT((GET_HASH_ENTRY(CmpCacheTable, Kcb->ConvKey).Owner ==
            KeGetCurrentThread()) ||
           (CmpTestRegistryLockExclusive() == TRUE));
    
    /* Add us to the free list */
    InsertTailList(&CmpFreeKCBListHead, &Kcb->FreeListEntry);
    
    /* Get the allocation page */
    AllocPage = (PCM_ALLOC_PAGE)((ULONG_PTR)Kcb & 0xFFFFF000);
    
    /* Sanity check */
    ASSERT(AllocPage->FreeCount != CM_KCBS_PER_PAGE);
    
    /* Increase free count */
    if (++AllocPage->FreeCount == CM_KCBS_PER_PAGE)
    {
        /* Loop all the entries */
        for (i = CM_KCBS_PER_PAGE - 1; i; i--)
        {
            /* Get the KCB */
            Kcb = (PVOID)((ULONG_PTR)AllocPage +
                          FIELD_OFFSET(CM_ALLOC_PAGE, AllocPage) +
                          i * sizeof(CM_KEY_CONTROL_BLOCK));
            
            /* Remove the entry */ 
            RemoveEntryList(&Kcb->FreeListEntry);
        }
        
        /* Free the page */
        ExFreePool(AllocPage);
    }
    
    /* Release the lock */
    KeReleaseGuardedMutex(&CmpAllocBucketLock);
}

PCM_KEY_CONTROL_BLOCK
NTAPI
CmpAllocateKeyControlBlock(VOID)
{
    PLIST_ENTRY NextEntry;
    PCM_KEY_CONTROL_BLOCK CurrentKcb;
    PCM_ALLOC_PAGE AllocPage;
    ULONG i;
    PAGED_CODE();
    
    /* Check if private allocations are initialized */
    if (CmpAllocInited)
    {
        /* They are, acquire the bucket lock */
        KeAcquireGuardedMutex(&CmpAllocBucketLock);
        
        /* See if there's something on the free KCB list */
SearchKcbList:
        if (!IsListEmpty(&CmpFreeKCBListHead))
        {
            /* Remove the entry */
            NextEntry = RemoveHeadList(&CmpFreeKCBListHead);
            
            /* Get the KCB */
            CurrentKcb = CONTAINING_RECORD(NextEntry,
                                           CM_KEY_CONTROL_BLOCK,
                                           FreeListEntry);
            
            /* Get the allocation page */
            AllocPage = (PCM_ALLOC_PAGE)((ULONG_PTR)CurrentKcb & 0xFFFFF000);
            
            /* Decrease the free count */
            ASSERT(AllocPage->FreeCount != 0);
            AllocPage->FreeCount--;
            
            /* Make sure this KCB is privately allocated */
            ASSERT(CurrentKcb->PrivateAlloc == 1);
            
            /* Release the allocation lock */
            KeReleaseGuardedMutex(&CmpAllocBucketLock);
            
            /* Return the KCB */
            return CurrentKcb;
        }
        
        /* Allocate an allocation page */
        AllocPage = ExAllocatePoolWithTag(PagedPool, PAGE_SIZE, TAG_CM);
        if (AllocPage)
        {
            /* Set default entries */
            AllocPage->FreeCount = CM_KCBS_PER_PAGE;
            
            /* Loop each entry */
            for (i = 0; i < CM_KCBS_PER_PAGE; i++)
            {
                /* Get this entry */
                CurrentKcb = (PVOID)((ULONG_PTR)AllocPage +
                                     FIELD_OFFSET(CM_ALLOC_PAGE, AllocPage) +
                                     i * sizeof(CM_KEY_CONTROL_BLOCK));
                
                /* Set it up */
                CurrentKcb->PrivateAlloc = TRUE;
                CurrentKcb->DelayCloseEntry = NULL;
                InsertHeadList(&CmpFreeKCBListHead,
                               &CurrentKcb->FreeListEntry);
            }
            
            /* Now go back and search the list */
            goto SearchKcbList;
        }
    }
    
    /* Allocate a KCB only */
    CurrentKcb = ExAllocatePoolWithTag(PagedPool,
                                       sizeof(CM_KEY_CONTROL_BLOCK),
                                       TAG_CM);
    if (CurrentKcb)
    {
        /* Set it up */
        CurrentKcb->PrivateAlloc = 0;
        CurrentKcb->DelayCloseEntry = NULL;
    }
    
    /* Return it */
    return CurrentKcb;
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
