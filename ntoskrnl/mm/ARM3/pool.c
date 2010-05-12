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
KGUARDED_MUTEX MmPagedPoolMutex;
MM_PAGED_POOL_INFO MmPagedPoolInfo;
SIZE_T MmAllocatedNonPagedPool;
ULONG MmSpecialPoolTag;
ULONG MmConsumedPoolPercentage;
BOOLEAN MmProtectFreedNonPagedPool;

/* PRIVATE FUNCTIONS **********************************************************/

VOID
NTAPI
MiInitializeNonPagedPoolThresholds(VOID)
{
    PFN_NUMBER Size = MmMaximumNonPagedPoolInPages;

    /* Default low threshold of 8MB or one third of nonpaged pool */
    MiLowNonPagedPoolThreshold = (8 * _1MB) >> PAGE_SHIFT;
    MiLowNonPagedPoolThreshold = min(MiLowNonPagedPoolThreshold, Size / 3);

    /* Default high threshold of 20MB or 50% */
    MiHighNonPagedPoolThreshold = (20 * _1MB) >> PAGE_SHIFT;
    MiHighNonPagedPoolThreshold = min(MiHighNonPagedPoolThreshold, Size / 2);
    ASSERT(MiLowNonPagedPoolThreshold < MiHighNonPagedPoolThreshold);
}

VOID
NTAPI
MiInitializePoolEvents(VOID)
{
    KIRQL OldIrql;
    PFN_NUMBER FreePoolInPages;

    /* Lock paged pool */
    KeAcquireGuardedMutex(&MmPagedPoolMutex);

    /* Total size of the paged pool minus the allocated size, is free */
    FreePoolInPages = MmSizeOfPagedPoolInPages - MmPagedPoolInfo.AllocatedPagedPool;

    /* Check the initial state high state */
    if (FreePoolInPages >= MiHighPagedPoolThreshold)
    {
        /* We have plenty of pool */
        KeSetEvent(MiHighPagedPoolEvent, 0, FALSE);
    }
    else
    {
        /* We don't */
        KeClearEvent(MiHighPagedPoolEvent);
    }

    /* Check the initial low state */
    if (FreePoolInPages <= MiLowPagedPoolThreshold)
    {
        /* We're very low in free pool memory */
        KeSetEvent(MiLowPagedPoolEvent, 0, FALSE);
    }
    else
    {
        /* We're not */
        KeClearEvent(MiLowPagedPoolEvent);
    }

    /* Release the paged pool lock */
    KeReleaseGuardedMutex(&MmPagedPoolMutex);

    /* Now it's time for the nonpaged pool lock */
    OldIrql = KeAcquireQueuedSpinLock(LockQueueMmNonPagedPoolLock);

    /* Free pages are the maximum minus what's been allocated */
    FreePoolInPages = MmMaximumNonPagedPoolInPages - MmAllocatedNonPagedPool;

    /* Check if we have plenty */
    if (FreePoolInPages >= MiHighNonPagedPoolThreshold)
    {
        /* We do, set the event */
        KeSetEvent(MiHighNonPagedPoolEvent, 0, FALSE);
    }
    else
    {
        /* We don't, clear the event */
        KeClearEvent(MiHighNonPagedPoolEvent);
    }

    /* Check if we have very little */
    if (FreePoolInPages <= MiLowNonPagedPoolThreshold)
    {
        /* We do, set the event */
        KeSetEvent(MiLowNonPagedPoolEvent, 0, FALSE);
    }
    else
    {
        /* We don't, clear it */
        KeClearEvent(MiLowNonPagedPoolEvent);
    }

    /* We're done, release the nonpaged pool lock */
    KeReleaseQueuedSpinLock(LockQueueMmNonPagedPoolLock, OldIrql);
}

VOID
NTAPI
MiInitializeNonPagedPool(VOID)
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
    PFN_NUMBER SizeInPages, PageFrameNumber;
    ULONG i;
    KIRQL OldIrql;
    PLIST_ENTRY NextEntry, NextHead, LastHead;
    PMMPTE PointerPte, StartPte;
    MMPTE TempPte;
    PMMPFN Pfn1;
    PVOID BaseVa, BaseVaStart;
    PMMFREE_POOL_ENTRY FreeEntry;
    PKSPIN_LOCK_QUEUE LockQueue;
    
    //
    // Figure out how big the allocation is in pages
    //
    SizeInPages = BYTES_TO_PAGES(SizeInBytes);
    
    //
    // Handle paged pool
    //
    if (PoolType == PagedPool)
    {
        //
        // Lock the paged pool mutex
        //
        KeAcquireGuardedMutex(&MmPagedPoolMutex);
        
        //
        // Find some empty allocation space
        //
        i = RtlFindClearBitsAndSet(MmPagedPoolInfo.PagedPoolAllocationMap,
                                   SizeInPages,
                                   MmPagedPoolInfo.PagedPoolHint);
        if (i == 0xFFFFFFFF)
        {
            //
            // Get the page bit count
            //
            i = ((SizeInPages - 1) / 1024) + 1;
            
            //
            // Check if there is enougn paged pool expansion space left
            //
            if (MmPagedPoolInfo.NextPdeForPagedPoolExpansion >
                MiAddressToPte(MmPagedPoolInfo.LastPteForPagedPool))
            {
                //
                // Out of memory!
                //
                DPRINT1("OUT OF PAGED POOL!!!\n");
                KeReleaseGuardedMutex(&MmPagedPoolMutex);
                return NULL;
            }
            
            //
            // Check if we'll have to expand past the last PTE we have available
            //            
            if (((i - 1) + MmPagedPoolInfo.NextPdeForPagedPoolExpansion) >
                 MiAddressToPte(MmPagedPoolInfo.LastPteForPagedPool))
            {
                //
                // We can only support this much then
                //
                SizeInPages = MiAddressToPte(MmPagedPoolInfo.LastPteForPagedPool) - 
                              MmPagedPoolInfo.NextPdeForPagedPoolExpansion +
                              1;
                ASSERT(SizeInPages < i);
                i = SizeInPages;
            }
            else
            {
                //
                // Otherwise, there is plenty of space left for this expansion
                //
                SizeInPages = i;
            }
            
            //
            // Get the template PTE we'll use to expand
            //
            TempPte = ValidKernelPte;
            
            //
            // Get the first PTE in expansion space
            //
            PointerPte = MmPagedPoolInfo.NextPdeForPagedPoolExpansion;
            BaseVa = MiPteToAddress(PointerPte);
            BaseVaStart = BaseVa;
            
            //
            // Lock the PFN database and loop pages
            //            
            OldIrql = KeAcquireQueuedSpinLock(LockQueuePfnLock);          
            do
            {
                //
                // It should not already be valid
                //
                ASSERT(PointerPte->u.Hard.Valid == 0);
                
                //
                // Request a paged pool page and write the PFN for it
                //
                PageFrameNumber = MmAllocPage(MC_PPOOL);
                TempPte.u.Hard.PageFrameNumber = PageFrameNumber;
                
                //
                // Save it into our double-buffered system page directory
                //
                MmSystemPagePtes[(ULONG_PTR)PointerPte & (PAGE_SIZE - 1) /
                                 sizeof(MMPTE)] = TempPte;
                            
                //
                // Write the actual PTE now
                //
                *PointerPte++ = TempPte;
                
                //
                // Move on to the next expansion address
                //
                BaseVa = (PVOID)((ULONG_PTR)BaseVa + PAGE_SIZE);
            } while (--i > 0);
            
            //
            // Release the PFN database lock
            //            
            KeReleaseQueuedSpinLock(LockQueuePfnLock, OldIrql);
            
            //
            // These pages are now available, clear their availablity bits
            //
            RtlClearBits(MmPagedPoolInfo.PagedPoolAllocationMap,
                         (MmPagedPoolInfo.NextPdeForPagedPoolExpansion -
                          MiAddressToPte(MmPagedPoolInfo.FirstPteForPagedPool)) *
                         1024,
                         SizeInPages * 1024);
                        
            //
            // Update the next expansion location
            //
            MmPagedPoolInfo.NextPdeForPagedPoolExpansion += SizeInPages;
            
            //
            // Zero out the newly available memory
            //
            RtlZeroMemory(BaseVaStart, SizeInPages * PAGE_SIZE);
            
            //
            // Now try consuming the pages again
            //
            SizeInPages = BYTES_TO_PAGES(SizeInBytes);
            i = RtlFindClearBitsAndSet(MmPagedPoolInfo.PagedPoolAllocationMap,
                                       SizeInPages,
                                       0);
            if (i == 0xFFFFFFFF) 
            {
                //
                // Out of memory!
                //
                DPRINT1("OUT OF PAGED POOL!!!\n");
                KeReleaseGuardedMutex(&MmPagedPoolMutex);
                return NULL;
            }
        }
        
        //
        // Update the pool hint if the request was just one page
        //
        if (SizeInPages == 1) MmPagedPoolInfo.PagedPoolHint = i + 1;
        
        //
        // Update the end bitmap so we know the bounds of this allocation when
        // the time comes to free it
        //
        RtlSetBit(MmPagedPoolInfo.EndOfPagedPoolBitmap, i + SizeInPages - 1);
        
        //
        // Now we can release the lock (it mainly protects the bitmap)
        //
        KeReleaseGuardedMutex(&MmPagedPoolMutex);
        
        //
        // Now figure out where this allocation starts
        //
        BaseVa = (PVOID)((ULONG_PTR)MmPagedPoolStart + (i << PAGE_SHIFT));
        
        //
        // Flush the TLB
        //
        KeFlushEntireTb(TRUE, TRUE);
        
        //
        // Setup a demand-zero writable PTE
        //
        TempPte.u.Long = 0;
        MI_MAKE_WRITE_PAGE(&TempPte);
        
        //
        // Find the first and last PTE, then loop them all
        //
        PointerPte = MiAddressToPte(BaseVa);
        StartPte = PointerPte + SizeInPages;
        do
        {
            //
            // Write the demand zero PTE and keep going
            //
            *PointerPte++ = TempPte;
        } while (PointerPte < StartPte);
        
        //
        // Return the allocation address to the caller
        //
        return BaseVa;
    }    
    
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
    KeReleaseQueuedSpinLock(LockQueueMmNonPagedPoolLock, OldIrql);

    //
    // Allocate some system PTEs
    //
    StartPte = MiReserveSystemPtes(SizeInPages, NonPagedPoolExpansion);
    PointerPte = StartPte;
    if (StartPte == NULL)
    {
        //
        // Ran out of memory
        //
        DPRINT1("Out of NP Expansion Pool\n");
        return NULL;
    }
    
    //
    // Acquire the pool lock now
    //
    OldIrql = KeAcquireQueuedSpinLock(LockQueueMmNonPagedPoolLock);
    
    //
    // Lock the PFN database too
    //
    LockQueue = &KeGetCurrentPrcb()->LockQueue[LockQueuePfnLock];
    KeAcquireQueuedSpinLockAtDpcLevel(LockQueue);
    
    //
    // Loop the pages
    //
    TempPte = ValidKernelPte;
    do
    {
        //
        // Allocate a page
        //
        PageFrameNumber = MmAllocPage(MC_NPPOOL);
        
        //
        // Get the PFN entry for it
        //
        Pfn1 = MiGetPfnEntry(PageFrameNumber);
        
        //
        // Write the PTE for it
        //
        TempPte.u.Hard.PageFrameNumber = PageFrameNumber;
        ASSERT(PointerPte->u.Hard.Valid == 0);
        ASSERT(TempPte.u.Hard.Valid == 1);
        *PointerPte++ = TempPte;
    } while (--SizeInPages > 0);
    
    //
    // This is the last page
    //
    Pfn1->u3.e1.EndOfAllocation = 1;
    
    //
    // Get the first page and mark it as such
    //
    Pfn1 = MiGetPfnEntry(StartPte->u.Hard.PageFrameNumber);
    Pfn1->u3.e1.StartOfAllocation = 1;
    
    //
    // Release the PFN and nonpaged pool lock
    //
    KeReleaseQueuedSpinLockFromDpcLevel(LockQueue);
    KeReleaseQueuedSpinLock(LockQueueMmNonPagedPoolLock, OldIrql);
    
    //
    // Return the address
    //
    return MiPteToAddress(StartPte);
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
    ULONG i, End;
    
    //
    // Handle paged pool
    //
    if ((StartingVa >= MmPagedPoolStart) && (StartingVa <= MmPagedPoolEnd))
    {
        //
        // Calculate the offset from the beginning of paged pool, and convert it
        // into pages
        //
        i = ((ULONG_PTR)StartingVa - (ULONG_PTR)MmPagedPoolStart) >> PAGE_SHIFT;
        End = i;
        
        //
        // Now use the end bitmap to scan until we find a set bit, meaning that
        // this allocation finishes here
        //
        while (!RtlTestBit(MmPagedPoolInfo.EndOfPagedPoolBitmap, End)) End++;
        
        //
        // Now calculate the total number of pages this allocation spans
        //
        NumberOfPages = End - i + 1;
        
        //
        // Acquire the paged pool lock
        //
        KeAcquireGuardedMutex(&MmPagedPoolMutex);
        
        //
        // Clear the allocation and free bits
        //
        RtlClearBit(MmPagedPoolInfo.EndOfPagedPoolBitmap, i);
        RtlClearBits(MmPagedPoolInfo.PagedPoolAllocationMap, i, NumberOfPages);
        
        //
        // Update the hint if we need to
        //
        if (i < MmPagedPoolInfo.PagedPoolHint) MmPagedPoolInfo.PagedPoolHint = i;
        
        //
        // Release the lock protecting the bitmaps
        //
        KeReleaseGuardedMutex(&MmPagedPoolMutex);
        
        //
        // And finally return the number of pages freed
        //
        return NumberOfPages;
    }
    
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


BOOLEAN
NTAPI
MiRaisePoolQuota(IN POOL_TYPE PoolType,
                 IN ULONG CurrentMaxQuota,
                 OUT PULONG NewMaxQuota)
{
    //
    // Not implemented
    //
    UNIMPLEMENTED;
    *NewMaxQuota = CurrentMaxQuota + 65536;
    return TRUE;
}

/* PUBLIC FUNCTIONS ***********************************************************/

/*
 * @unimplemented
 */
PVOID
NTAPI
MmAllocateMappingAddress(IN SIZE_T NumberOfBytes,
                         IN ULONG PoolTag)
{
	UNIMPLEMENTED;
	return NULL;
}

/*
 * @unimplemented
 */
VOID
NTAPI
MmFreeMappingAddress(IN PVOID BaseAddress,
                     IN ULONG PoolTag)
{
	UNIMPLEMENTED;
}

/* EOF */
