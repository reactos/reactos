/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            ntoskrnl/mm/ARM3/pool.c
 * PURPOSE:         ARM Memory Manager Pool Allocator
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

#line 15 "ARM³::POOL"
#define MODULE_INVOLVED_IN_ARM3
#include "../ARM3/miarm.h"

/* GLOBALS ********************************************************************/

LIST_ENTRY MmNonPagedPoolFreeListHead[MI_MAX_FREE_PAGE_LISTS];
PFN_NUMBER MmNumberOfFreeNonPagedPool, MiExpansionPoolPagesInitialCharge;
PVOID MmNonPagedPoolEnd0;
PFN_NUMBER MiStartOfInitialPoolFrame, MiEndOfInitialPoolFrame;

/* PRIVATE FUNCTIONS **********************************************************/

VOID
NTAPI
MiInitializeArmPool(VOID)
{
    ULONG i;
    PFN_NUMBER PoolPages;
    PMMFREE_POOL_ENTRY FreeEntry, FirstEntry;
    PMMPTE PointerPte;
    PAGED_CODE();

    //
    // We keep 4 lists of free pages (4 lists help avoid contention)
    //
    for (i = 0; i < MI_MAX_FREE_PAGE_LISTS; i++)
    {
        //
        // Initialize each of them
        //
        InitializeListHead(&MmNonPagedPoolFreeListHead[i]);
    }

    //
    // Calculate how many pages the initial nonpaged pool has
    //
    PoolPages = BYTES_TO_PAGES(MmSizeOfNonPagedPoolInBytes);
    MmNumberOfFreeNonPagedPool = PoolPages;
       
    //
    // Initialize the first free entry
    //
    FreeEntry = MmNonPagedPoolStart;
    FirstEntry = FreeEntry;
    FreeEntry->Size = PoolPages;
    FreeEntry->Owner = FirstEntry;

    //
    // Insert it into the last list
    //
    InsertHeadList(&MmNonPagedPoolFreeListHead[MI_MAX_FREE_PAGE_LISTS - 1],
                   &FreeEntry->List);
    
    //
    // Now create free entries for every single other page
    //
    while (PoolPages-- > 1)
    {
        //
        // Link them all back to the original entry
        //
        FreeEntry = (PMMFREE_POOL_ENTRY)((ULONG_PTR)FreeEntry + PAGE_SIZE);
        FreeEntry->Owner = FirstEntry;
    }

    //
    // Validate and remember first allocated pool page
    //
    PointerPte = MiAddressToPte(MmNonPagedPoolStart);
    ASSERT(PointerPte->u.Hard.Valid == 1);
    MiStartOfInitialPoolFrame = PFN_FROM_PTE(PointerPte);
    
    //
    // Keep track of where initial nonpaged pool ends
    //
    MmNonPagedPoolEnd0 = (PVOID)((ULONG_PTR)MmNonPagedPoolStart +
                                 MmSizeOfNonPagedPoolInBytes);
    
    //
    // Validate and remember last allocated pool page
    //
    PointerPte = MiAddressToPte((PVOID)((ULONG_PTR)MmNonPagedPoolEnd0 - 1));
    ASSERT(PointerPte->u.Hard.Valid == 1);
    MiEndOfInitialPoolFrame = PFN_FROM_PTE(PointerPte);
    
    //
    // Validate the first nonpaged pool expansion page (which is a guard page)
    //
    PointerPte = MiAddressToPte(MmNonPagedPoolExpansionStart);
    ASSERT(PointerPte->u.Hard.Valid == 0);
    
    //
    // Calculate the size of the expansion region alone
    //
    MiExpansionPoolPagesInitialCharge =
    BYTES_TO_PAGES(MmMaximumNonPagedPoolInBytes - MmSizeOfNonPagedPoolInBytes);
    
    //
    // Remove 2 pages, since there's a guard page on top and on the bottom
    //
    MiExpansionPoolPagesInitialCharge -= 2;
    
    //
    // Now initialize the nonpaged pool expansion PTE space. Remember there's a
    // guard page on top so make sure to skip it. The bottom guard page will be
    // guaranteed by the fact our size is off by one.
    //
    MiInitializeSystemPtes(PointerPte + 1,
                           MiExpansionPoolPagesInitialCharge,
                           NonPagedPoolExpansion);
}

PVOID
NTAPI
MiAllocatePoolPages(IN POOL_TYPE PoolType,
                    IN SIZE_T SizeInBytes)
{
    PFN_NUMBER SizeInPages;
    ULONG i;
    KIRQL OldIrql;
    PLIST_ENTRY NextEntry, NextHead, LastHead;
    PMMPTE PointerPte;
    PMMPFN Pfn1;
    PVOID BaseVa;
    PMMFREE_POOL_ENTRY FreeEntry;
    
    //
    // Figure out how big the allocation is in pages
    //
    SizeInPages = BYTES_TO_PAGES(SizeInBytes);
    
    //
    // Allocations of less than 4 pages go into their individual buckets
    //
    i = SizeInPages - 1;
    if (i >= MI_MAX_FREE_PAGE_LISTS) i = MI_MAX_FREE_PAGE_LISTS - 1;
   
    //
    // Loop through all the free page lists based on the page index
    //
    NextHead = &MmNonPagedPoolFreeListHead[i];
    LastHead = &MmNonPagedPoolFreeListHead[MI_MAX_FREE_PAGE_LISTS];
    
    //
    // Acquire the nonpaged pool lock
    //
    OldIrql = KeAcquireQueuedSpinLock(LockQueueMmNonPagedPoolLock);
    do
    {
        //
        // Now loop through all the free page entries in this given list
        //
        NextEntry = NextHead->Flink;
        while (NextEntry != NextHead)
        {
            //
            // Grab the entry and see if it can handle our allocation
            //
            FreeEntry = CONTAINING_RECORD(NextEntry, MMFREE_POOL_ENTRY, List);
            if (FreeEntry->Size >= SizeInPages)
            {
                //
                // It does, so consume the pages from here
                //
                FreeEntry->Size -= SizeInPages;

                //
                // The allocation will begin in this free page area
                //
                BaseVa = (PVOID)((ULONG_PTR)FreeEntry +
                                 (FreeEntry->Size  << PAGE_SHIFT));
                
                //
                // This is not a free page segment anymore
                //
                RemoveEntryList(&FreeEntry->List);
                
                //
                // However, check if its' still got space left
                //
                if (FreeEntry->Size != 0)
                {
                    //
                    // Insert it back into a different list, based on its pages
                    //
                    i = FreeEntry->Size - 1;
                    if (i >= MI_MAX_FREE_PAGE_LISTS) i = MI_MAX_FREE_PAGE_LISTS - 1;
                    InsertTailList (&MmNonPagedPoolFreeListHead[i],
                                    &FreeEntry->List);
                }
                
                //
                // Grab the PTE for this allocation
                //
                PointerPte = MiAddressToPte(BaseVa);
                ASSERT(PointerPte->u.Hard.Valid == 1);
                
                //
                // Grab the PFN NextEntry and index
                //
                Pfn1 = MiGetPfnEntry(PFN_FROM_PTE(PointerPte));
                
                //
                // Now mark it as the beginning of an allocation
                //
                ASSERT(Pfn1->u3.e1.StartOfAllocation == 0);
                Pfn1->u3.e1.StartOfAllocation = 1;
                
                //
                // Check if the allocation is larger than one page
                //
                if (SizeInPages != 1)
                {
                    //
                    // Navigate to the last PFN entry and PTE
                    //
                    PointerPte += SizeInPages - 1;
                    ASSERT(PointerPte->u.Hard.Valid == 1);
                    Pfn1 = MiGetPfnEntry(PointerPte->u.Hard.PageFrameNumber);
                }
                
                //
                // Mark this PFN as the last (might be the same as the first)
                //
                ASSERT(Pfn1->u3.e1.EndOfAllocation == 0);
                Pfn1->u3.e1.EndOfAllocation = 1;
                
                //
                // Release the nonpaged pool lock, and return the allocation
                //
                KeReleaseQueuedSpinLock(LockQueueMmNonPagedPoolLock, OldIrql);
                return BaseVa;
            }
            
            //
            // Try the next free page entry
            //
            NextEntry = FreeEntry->List.Flink;
        }
    } while (++NextHead < LastHead);
    
    //
    // If we got here, we're out of space.
    // Start by releasing the lock
    //
    KeReleaseQueuedSpinLock (LockQueueMmNonPagedPoolLock, OldIrql);
    
    //
    // We should now go into expansion nonpaged pool
    //
    DPRINT1("Out of NP Pool\n");
    return NULL;
}

ULONG
NTAPI
MiFreePoolPages(IN PVOID StartingVa)
{
    PMMPTE PointerPte, StartPte;
    PMMPFN Pfn1, StartPfn;
    PFN_NUMBER FreePages, NumberOfPages;
    KIRQL OldIrql;
    PMMFREE_POOL_ENTRY FreeEntry, NextEntry, LastEntry;
    ULONG i;
    
    //
    // Get the first PTE and its corresponding PFN entry
    //
    StartPte = PointerPte = MiAddressToPte(StartingVa);
    StartPfn = Pfn1 = MiGetPfnEntry(PointerPte->u.Hard.PageFrameNumber);
    
    //
    // Loop until we find the last PTE
    //
    while (Pfn1->u3.e1.EndOfAllocation == 0)
    {
        //
        // Keep going
        //
        PointerPte++;
        Pfn1 = MiGetPfnEntry(PointerPte->u.Hard.PageFrameNumber);
    }
    
    //
    // Now we know how many pages we have
    //
    NumberOfPages = PointerPte - StartPte + 1;
    
    //
    // Acquire the nonpaged pool lock
    //
    OldIrql = KeAcquireQueuedSpinLock(LockQueueMmNonPagedPoolLock);
    
    //
    // Mark the first and last PTEs as not part of an allocation anymore
    //
    StartPfn->u3.e1.StartOfAllocation = 0;    
    Pfn1->u3.e1.EndOfAllocation = 0;
    
    //
    // Assume we will free as many pages as the allocation was
    //
    FreePages = NumberOfPages;
    
    //
    // Peek one page past the end of the allocation
    //
    PointerPte++;
    
    //
    // Guard against going past initial nonpaged pool
    //
    if (MiGetPfnEntryIndex(Pfn1) == MiEndOfInitialPoolFrame)
    {
        //
        // This page is on the outskirts of initial nonpaged pool, so ignore it
        //
        DPRINT1("End of initial frame\n");
        Pfn1 = NULL;
    }
    else
    {
        //
        // Otherwise, our entire allocation must've fit within the initial non 
        // paged pool, or the expansion nonpaged pool, so get the PFN entry of
        // the next allocation
        //
        ASSERT((ULONG_PTR)StartingVa + NumberOfPages <= (ULONG_PTR)MmNonPagedPoolEnd);
        if (PointerPte->u.Hard.Valid == 1)
        {
            //
            // It's either expansion or initial: get the PFN entry
            //
            Pfn1 = MiGetPfnEntry(PointerPte->u.Hard.PageFrameNumber);            
        }
        else
        {
            //
            // This means we've reached the guard page that protects the end of
            // the expansion nonpaged pool
            //
            Pfn1 = NULL;
        }
        
    }
    
    //
    // Check if this allocation actually exists
    //
    if ((Pfn1) && (Pfn1->u3.e1.StartOfAllocation == 0))
    {
        //
        // It doesn't, so we should actually locate a free entry descriptor
        //
        FreeEntry = (PMMFREE_POOL_ENTRY)((ULONG_PTR)StartingVa +
                                         (NumberOfPages << PAGE_SHIFT));
        ASSERT(FreeEntry->Owner == FreeEntry);
        
        //
        // Consume this entry's pages, and remove it from its free list
        //
        FreePages += FreeEntry->Size;
        RemoveEntryList (&FreeEntry->List);
    }
    
    //
    // Now get the official free entry we'll create for the caller's allocation
    //
    FreeEntry = StartingVa;
    
    //
    // Check if the our allocation is the very first page
    //
    if (MiGetPfnEntryIndex(StartPfn) == MiStartOfInitialPoolFrame)
    {
        //
        // Then we can't do anything or we'll risk underflowing
        //
        DPRINT1("Start of of initial frame\n");
        Pfn1 = NULL;
    }
    else
    {
        //
        // Otherwise, get the PTE for the page right before our allocation
        //
        PointerPte -= NumberOfPages + 1;
        if (PointerPte->u.Hard.Valid == 1)
        {
            //
            // It's either expansion or initial nonpaged pool, get the PFN entry
            //
            Pfn1 = MiGetPfnEntry(PointerPte->u.Hard.PageFrameNumber);
        }
        else
        {
            //
            // We must've reached the guard page, so don't risk touching it
            //
            Pfn1 = NULL;
        }
    }
    
    //
    // Check if there is a valid PFN entry for the page before the allocation
    // and then check if this page was actually the end of an allocation.
    // If it wasn't, then we know for sure it's a free page
    //
    if ((Pfn1) && (Pfn1->u3.e1.EndOfAllocation == 0))
    {
        //
        // Get the free entry descriptor for that given page range
        //
        FreeEntry = (PMMFREE_POOL_ENTRY)((ULONG_PTR)StartingVa - PAGE_SIZE);
        FreeEntry = FreeEntry->Owner;
        
        //
        // Check if the entry is small enough to be indexed on a free list
        // If it is, we'll want to re-insert it, since we're about to
        // collapse our pages on top of it, which will change its count
        //
        if (FreeEntry->Size < (MI_MAX_FREE_PAGE_LISTS - 1))
        {
            //
            // Remove the list from where it is now
            //
            RemoveEntryList(&FreeEntry->List);
            
            //
            // Update its size
            //
            FreeEntry->Size += FreePages;
            
            //
            // And now find the new appropriate list to place it in
            //
            i = (ULONG)(FreeEntry->Size - 1);
            if (i >= MI_MAX_FREE_PAGE_LISTS) i = MI_MAX_FREE_PAGE_LISTS - 1;
            
            //
            // Do it
            //
            InsertTailList(&MmNonPagedPoolFreeListHead[i], &FreeEntry->List);
        }
        else
        {
            //
            // Otherwise, just combine our free pages into this entry
            //
            FreeEntry->Size += FreePages;
        }
    }
    
    //
    // Check if we were unable to do any compaction, and we'll stick with this
    //
    if (FreeEntry == StartingVa)
    {
        //
        // Well, now we are a free entry. At worse we just have our newly freed
        // pages, at best we have our pages plus whatever entry came after us
        //
        FreeEntry->Size = FreePages;
        
        //
        // Find the appropriate list we should be on
        //
        i = FreeEntry->Size - 1;
        if (i >= MI_MAX_FREE_PAGE_LISTS) i = MI_MAX_FREE_PAGE_LISTS - 1;
        
        //
        // And insert us
        //
        InsertTailList (&MmNonPagedPoolFreeListHead[i], &FreeEntry->List);
    }
    
    //
    // Just a sanity check
    //
    ASSERT(FreePages != 0);
    
    //
    // Get all the pages between our allocation and its end. These will all now
    // become free page chunks.
    //
    NextEntry = StartingVa;
    LastEntry = (PMMFREE_POOL_ENTRY)((ULONG_PTR)NextEntry + (FreePages << PAGE_SHIFT));    
    do
    {
        //
        // Link back to the parent free entry, and keep going
        //
        NextEntry->Owner = FreeEntry;    
        NextEntry = (PMMFREE_POOL_ENTRY)((ULONG_PTR)NextEntry + PAGE_SIZE);
    } while (NextEntry != LastEntry);
    
    //
    // We're done, release the lock and let the caller know how much we freed
    //
    KeReleaseQueuedSpinLock(LockQueueMmNonPagedPoolLock, OldIrql);
    return NumberOfPages;
}

/* EOF */
