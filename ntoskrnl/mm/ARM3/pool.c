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

#define MODULE_INVOLVED_IN_ARM3
#include <mm/ARM3/miarm.h>

/* GLOBALS ********************************************************************/

LIST_ENTRY MmNonPagedPoolFreeListHead[MI_MAX_FREE_PAGE_LISTS];
PFN_COUNT MmNumberOfFreeNonPagedPool, MiExpansionPoolPagesInitialCharge;
PVOID MmNonPagedPoolEnd0;
PFN_NUMBER MiStartOfInitialPoolFrame, MiEndOfInitialPoolFrame;
KGUARDED_MUTEX MmPagedPoolMutex;
MM_PAGED_POOL_INFO MmPagedPoolInfo;
SIZE_T MmAllocatedNonPagedPool;
SIZE_T MmTotalNonPagedPoolQuota;
SIZE_T MmTotalPagedPoolQuota;
ULONG MmSpecialPoolTag;
ULONG MmConsumedPoolPercentage;
BOOLEAN MmProtectFreedNonPagedPool;
SLIST_HEADER MiNonPagedPoolSListHead;
ULONG MiNonPagedPoolSListMaximum = 4;
SLIST_HEADER MiPagedPoolSListHead;
ULONG MiPagedPoolSListMaximum = 8;

/* PRIVATE FUNCTIONS **********************************************************/

VOID
NTAPI
MiProtectFreeNonPagedPool(IN PVOID VirtualAddress,
                          IN ULONG PageCount)
{
    PMMPTE PointerPte, LastPte;
    MMPTE TempPte;

    /* If pool is physical, can't protect PTEs */
    if (MI_IS_PHYSICAL_ADDRESS(VirtualAddress)) return;

    /* Get PTE pointers and loop */
    PointerPte = MiAddressToPte(VirtualAddress);
    LastPte = PointerPte + PageCount;
    do
    {
        /* Capture the PTE for safety */
        TempPte = *PointerPte;

        /* Mark it as an invalid PTE, set proto bit to recognize it as pool */
        TempPte.u.Hard.Valid = 0;
        TempPte.u.Soft.Prototype = 1;
        MI_WRITE_INVALID_PTE(PointerPte, TempPte);
    } while (++PointerPte < LastPte);

    /* Flush the TLB */
    KeFlushEntireTb(TRUE, TRUE);
}

BOOLEAN
NTAPI
MiUnProtectFreeNonPagedPool(IN PVOID VirtualAddress,
                            IN ULONG PageCount)
{
    PMMPTE PointerPte;
    MMPTE TempPte;
    PFN_NUMBER UnprotectedPages = 0;

    /* If pool is physical, can't protect PTEs */
    if (MI_IS_PHYSICAL_ADDRESS(VirtualAddress)) return FALSE;

    /* Get, and capture the PTE */
    PointerPte = MiAddressToPte(VirtualAddress);
    TempPte = *PointerPte;

    /* Loop protected PTEs */
    while ((TempPte.u.Hard.Valid == 0) && (TempPte.u.Soft.Prototype == 1))
    {
        /* Unprotect the PTE */
        TempPte.u.Hard.Valid = 1;
        TempPte.u.Soft.Prototype = 0;
        MI_WRITE_VALID_PTE(PointerPte, TempPte);

        /* One more page */
        if (++UnprotectedPages == PageCount) break;

        /* Capture next PTE */
        TempPte = *(++PointerPte);
    }

    /* Return if any pages were unprotected */
    return UnprotectedPages ? TRUE : FALSE;
}

FORCEINLINE
VOID
MiProtectedPoolUnProtectLinks(IN PLIST_ENTRY Links,
                              OUT PVOID* PoolFlink,
                              OUT PVOID* PoolBlink)
{
    BOOLEAN Safe;
    PVOID PoolVa;

    /* Initialize variables */
    *PoolFlink = *PoolBlink = NULL;

    /* Check if the list has entries */
    if (IsListEmpty(Links) == FALSE)
    {
        /* We are going to need to forward link to do an insert */
        PoolVa = Links->Flink;

        /* So make it safe to access */
        Safe = MiUnProtectFreeNonPagedPool(PoolVa, 1);
        if (Safe) *PoolFlink = PoolVa;
    }

    /* Are we going to need a backward link too? */
    if (Links != Links->Blink)
    {
        /* Get the head's backward link for the insert */
        PoolVa = Links->Blink;

        /* Make it safe to access */
        Safe = MiUnProtectFreeNonPagedPool(PoolVa, 1);
        if (Safe) *PoolBlink = PoolVa;
    }
}

FORCEINLINE
VOID
MiProtectedPoolProtectLinks(IN PVOID PoolFlink,
                            IN PVOID PoolBlink)
{
    /* Reprotect the pages, if they got unprotected earlier */
    if (PoolFlink) MiProtectFreeNonPagedPool(PoolFlink, 1);
    if (PoolBlink) MiProtectFreeNonPagedPool(PoolBlink, 1);
}

VOID
NTAPI
MiProtectedPoolInsertList(IN PLIST_ENTRY ListHead,
                          IN PLIST_ENTRY Entry,
                          IN BOOLEAN Critical)
{
    PVOID PoolFlink, PoolBlink;

    /* Make the list accessible */
    MiProtectedPoolUnProtectLinks(ListHead, &PoolFlink, &PoolBlink);

    /* Now insert in the right position */
    Critical ? InsertHeadList(ListHead, Entry) : InsertTailList(ListHead, Entry);

    /* And reprotect the pages containing the free links */
    MiProtectedPoolProtectLinks(PoolFlink, PoolBlink);
}

VOID
NTAPI
MiProtectedPoolRemoveEntryList(IN PLIST_ENTRY Entry)
{
    PVOID PoolFlink, PoolBlink;

    /* Make the list accessible */
    MiProtectedPoolUnProtectLinks(Entry, &PoolFlink, &PoolBlink);

    /* Now remove */
    RemoveEntryList(Entry);

    /* And reprotect the pages containing the free links */
    if (PoolFlink) MiProtectFreeNonPagedPool(PoolFlink, 1);
    if (PoolBlink) MiProtectFreeNonPagedPool(PoolBlink, 1);
}

CODE_SEG("INIT")
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

CODE_SEG("INIT")
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

CODE_SEG("INIT")
VOID
NTAPI
MiInitializeNonPagedPool(VOID)
{
    ULONG i;
    PFN_COUNT PoolPages;
    PMMFREE_POOL_ENTRY FreeEntry, FirstEntry;
    PMMPTE PointerPte;
    PAGED_CODE();

    //
    // Initialize the pool S-LISTs as well as their maximum count. In general,
    // we'll allow 8 times the default on a 2GB system, and two times the default
    // on a 1GB system.
    //
    InitializeSListHead(&MiPagedPoolSListHead);
    InitializeSListHead(&MiNonPagedPoolSListHead);
    if (MmNumberOfPhysicalPages >= ((2 * _1GB) /PAGE_SIZE))
    {
        MiNonPagedPoolSListMaximum *= 8;
        MiPagedPoolSListMaximum *= 8;
    }
    else if (MmNumberOfPhysicalPages >= (_1GB /PAGE_SIZE))
    {
        MiNonPagedPoolSListMaximum *= 2;
        MiPagedPoolSListMaximum *= 2;
    }

    //
    // However if debugging options for the pool are enabled, turn off the S-LIST
    // to reduce the risk of messing things up even more
    //
    if (MmProtectFreedNonPagedPool)
    {
        MiNonPagedPoolSListMaximum = 0;
        MiPagedPoolSListMaximum = 0;
    }

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
    PoolPages = (PFN_COUNT)BYTES_TO_PAGES(MmSizeOfNonPagedPoolInBytes);
    MmNumberOfFreeNonPagedPool = PoolPages;

    //
    // Initialize the first free entry
    //
    FreeEntry = MmNonPagedPoolStart;
    FirstEntry = FreeEntry;
    FreeEntry->Size = PoolPages;
    FreeEntry->Signature = MM_FREE_POOL_SIGNATURE;
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
        FreeEntry->Signature = MM_FREE_POOL_SIGNATURE;
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
    MiExpansionPoolPagesInitialCharge = (PFN_COUNT)
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

POOL_TYPE
NTAPI
MmDeterminePoolType(IN PVOID PoolAddress)
{
    //
    // Use a simple bounds check
    //
    if (PoolAddress >= MmPagedPoolStart && PoolAddress <= MmPagedPoolEnd)
        return PagedPool;
    else if (PoolAddress >= MmNonPagedPoolStart && PoolAddress <= MmNonPagedPoolEnd)
        return NonPagedPool;
    KeBugCheckEx(BAD_POOL_CALLER, 0x42, (ULONG_PTR)PoolAddress, 0, 0);
}

PVOID
NTAPI
MiAllocatePoolPages(IN POOL_TYPE PoolType,
                    IN SIZE_T SizeInBytes)
{
    PFN_NUMBER PageFrameNumber;
    PFN_COUNT SizeInPages, PageTableCount;
    ULONG i;
    KIRQL OldIrql;
    PLIST_ENTRY NextEntry, NextHead, LastHead;
    PMMPTE PointerPte, StartPte;
    PMMPDE PointerPde;
    ULONG EndAllocation;
    MMPTE TempPte;
    MMPDE TempPde;
    PMMPFN Pfn1;
    PVOID BaseVa, BaseVaStart;
    PMMFREE_POOL_ENTRY FreeEntry;

    //
    // Figure out how big the allocation is in pages
    //
    SizeInPages = (PFN_COUNT)BYTES_TO_PAGES(SizeInBytes);

    //
    // Check for overflow
    //
    if (SizeInPages == 0)
    {
        //
        // Fail
        //
        return NULL;
    }

    //
    // Handle paged pool
    //
    if ((PoolType & BASE_POOL_TYPE_MASK) == PagedPool)
    {
        //
        // If only one page is being requested, try to grab it from the S-LIST
        //
        if ((SizeInPages == 1) && (ExQueryDepthSList(&MiPagedPoolSListHead)))
        {
            BaseVa = InterlockedPopEntrySList(&MiPagedPoolSListHead);
            if (BaseVa) return BaseVa;
        }

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
            i = ((SizeInPages - 1) / PTE_PER_PAGE) + 1;
            DPRINT("Paged pool expansion: %lu %x\n", i, SizeInPages);

            //
            // Check if there is enougn paged pool expansion space left
            //
            if (MmPagedPoolInfo.NextPdeForPagedPoolExpansion >
                (PMMPDE)MiAddressToPte(MmPagedPoolInfo.LastPteForPagedPool))
            {
                //
                // Out of memory!
                //
                DPRINT1("FAILED to allocate %Iu bytes from paged pool\n", SizeInBytes);
                KeReleaseGuardedMutex(&MmPagedPoolMutex);
                return NULL;
            }

            //
            // Check if we'll have to expand past the last PTE we have available
            //
            if (((i - 1) + MmPagedPoolInfo.NextPdeForPagedPoolExpansion) >
                 (PMMPDE)MiAddressToPte(MmPagedPoolInfo.LastPteForPagedPool))
            {
                //
                // We can only support this much then
                //
                PointerPde = MiPteToPde(MmPagedPoolInfo.LastPteForPagedPool);
                PageTableCount = (PFN_COUNT)(PointerPde + 1 -
                                 MmPagedPoolInfo.NextPdeForPagedPoolExpansion);
                ASSERT(PageTableCount < i);
                i = PageTableCount;
            }
            else
            {
                //
                // Otherwise, there is plenty of space left for this expansion
                //
                PageTableCount = i;
            }

            //
            // Get the template PDE we'll use to expand
            //
            TempPde = ValidKernelPde;

            //
            // Get the first PTE in expansion space
            //
            PointerPde = MmPagedPoolInfo.NextPdeForPagedPoolExpansion;
            BaseVa = MiPdeToPte(PointerPde);
            BaseVaStart = BaseVa;

            //
            // Lock the PFN database and loop pages
            //
            OldIrql = MiAcquirePfnLock();
            do
            {
                //
                // It should not already be valid
                //
                ASSERT(PointerPde->u.Hard.Valid == 0);

                /* Request a page */
                MI_SET_USAGE(MI_USAGE_PAGED_POOL);
                MI_SET_PROCESS2("Kernel");
                PageFrameNumber = MiRemoveAnyPage(MI_GET_NEXT_COLOR());
                TempPde.u.Hard.PageFrameNumber = PageFrameNumber;
#if (_MI_PAGING_LEVELS >= 3)
                /* On PAE/x64 systems, there's no double-buffering */
                /* Initialize the PFN entry for it */
                MiInitializePfnForOtherProcess(PageFrameNumber,
                                               (PMMPTE)PointerPde,
                                               PFN_FROM_PTE(MiAddressToPte(PointerPde)));

                /* Write the actual PDE now */
                MI_WRITE_VALID_PDE(PointerPde, TempPde);
#else
                //
                // Save it into our double-buffered system page directory
                //
                MmSystemPagePtes[((ULONG_PTR)PointerPde & (SYSTEM_PD_SIZE - 1)) / sizeof(MMPTE)] = TempPde;

                /* Initialize the PFN */
                MiInitializePfnForOtherProcess(PageFrameNumber,
                                               (PMMPTE)PointerPde,
                                               MmSystemPageDirectory[(PointerPde - MiAddressToPde(NULL)) / PDE_PER_PAGE]);
#endif

                //
                // Move on to the next expansion address
                //
                PointerPde++;
                BaseVa = (PVOID)((ULONG_PTR)BaseVa + PAGE_SIZE);
                i--;
            } while (i > 0);

            //
            // Release the PFN database lock
            //
            MiReleasePfnLock(OldIrql);

            //
            // These pages are now available, clear their availablity bits
            //
            EndAllocation = (ULONG)(MmPagedPoolInfo.NextPdeForPagedPoolExpansion -
                                    (PMMPDE)MiAddressToPte(MmPagedPoolInfo.FirstPteForPagedPool)) *
                            PTE_PER_PAGE;
            RtlClearBits(MmPagedPoolInfo.PagedPoolAllocationMap,
                         EndAllocation,
                         PageTableCount * PTE_PER_PAGE);

            //
            // Update the next expansion location
            //
            MmPagedPoolInfo.NextPdeForPagedPoolExpansion += PageTableCount;

            //
            // Zero out the newly available memory
            //
            RtlZeroMemory(BaseVaStart, PageTableCount * PAGE_SIZE);

            //
            // Now try consuming the pages again
            //
            i = RtlFindClearBitsAndSet(MmPagedPoolInfo.PagedPoolAllocationMap,
                                       SizeInPages,
                                       0);
            if (i == 0xFFFFFFFF)
            {
                //
                // Out of memory!
                //
                DPRINT1("FAILED to allocate %Iu bytes from paged pool\n", SizeInBytes);
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
        EndAllocation = i + SizeInPages - 1;
        RtlSetBit(MmPagedPoolInfo.EndOfPagedPoolBitmap, EndAllocation);

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

        /* Setup a demand-zero writable PTE */
        MI_MAKE_SOFTWARE_PTE(&TempPte, MM_READWRITE);

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
            MI_WRITE_INVALID_PTE(PointerPte, TempPte);
        } while (++PointerPte < StartPte);

        //
        // Return the allocation address to the caller
        //
        return BaseVa;
    }

    //
    // If only one page is being requested, try to grab it from the S-LIST
    //
    if ((SizeInPages == 1) && (ExQueryDepthSList(&MiNonPagedPoolSListHead)))
    {
        BaseVa = InterlockedPopEntrySList(&MiNonPagedPoolSListHead);
        if (BaseVa) return BaseVa;
    }

    //
    // Allocations of less than 4 pages go into their individual buckets
    //
    i = min(SizeInPages, MI_MAX_FREE_PAGE_LISTS) - 1;

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
            /* Is freed non paged pool enabled */
            if (MmProtectFreedNonPagedPool)
            {
                /* We need to be able to touch this page, unprotect it */
                MiUnProtectFreeNonPagedPool(NextEntry, 0);
            }

            //
            // Grab the entry and see if it can handle our allocation
            //
            FreeEntry = CONTAINING_RECORD(NextEntry, MMFREE_POOL_ENTRY, List);
            ASSERT(FreeEntry->Signature == MM_FREE_POOL_SIGNATURE);
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

                /* Remove the item from the list, depending if pool is protected */
                if (MmProtectFreedNonPagedPool)
                    MiProtectedPoolRemoveEntryList(&FreeEntry->List);
                else
                    RemoveEntryList(&FreeEntry->List);

                //
                // However, check if its' still got space left
                //
                if (FreeEntry->Size != 0)
                {
                    /* Check which list to insert this entry into */
                    i = min(FreeEntry->Size, MI_MAX_FREE_PAGE_LISTS) - 1;

                    /* Insert the entry into the free list head, check for prot. pool */
                    if (MmProtectFreedNonPagedPool)
                        MiProtectedPoolInsertList(&MmNonPagedPoolFreeListHead[i], &FreeEntry->List, TRUE);
                    else
                        InsertTailList(&MmNonPagedPoolFreeListHead[i], &FreeEntry->List);

                    /* Is freed non paged pool protected? */
                    if (MmProtectFreedNonPagedPool)
                    {
                        /* Protect the freed pool! */
                        MiProtectFreeNonPagedPool(FreeEntry, FreeEntry->Size);
                    }
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

                /* Mark it as special pool if needed */
                ASSERT(Pfn1->u4.VerifierAllocation == 0);
                if (PoolType & VERIFIER_POOL_MASK)
                {
                    Pfn1->u4.VerifierAllocation = 1;
                }

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

            /* Is freed non paged pool protected? */
            if (MmProtectFreedNonPagedPool)
            {
                /* Protect the freed pool! */
                MiProtectFreeNonPagedPool(FreeEntry, FreeEntry->Size);
            }
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
        DPRINT("Out of NP Expansion Pool\n");
        return NULL;
    }

    //
    // Acquire the pool lock now
    //
    OldIrql = KeAcquireQueuedSpinLock(LockQueueMmNonPagedPoolLock);

    //
    // Lock the PFN database too
    //
    MiAcquirePfnLockAtDpcLevel();

    /* Check that we have enough available pages for this request */
    if (MmAvailablePages < SizeInPages)
    {
        MiReleasePfnLockFromDpcLevel();
        KeReleaseQueuedSpinLock(LockQueueMmNonPagedPoolLock, OldIrql);

        MiReleaseSystemPtes(StartPte, SizeInPages, NonPagedPoolExpansion);

        DPRINT1("OUT OF AVAILABLE PAGES! Required %lu, Available %lu\n", SizeInPages, MmAvailablePages);

        return NULL;
    }

    //
    // Loop the pages
    //
    TempPte = ValidKernelPte;
    do
    {
        /* Allocate a page */
        MI_SET_USAGE(MI_USAGE_PAGED_POOL);
        MI_SET_PROCESS2("Kernel");
        PageFrameNumber = MiRemoveAnyPage(MI_GET_NEXT_COLOR());

        /* Get the PFN entry for it and fill it out */
        Pfn1 = MiGetPfnEntry(PageFrameNumber);
        Pfn1->u3.e2.ReferenceCount = 1;
        Pfn1->u2.ShareCount = 1;
        Pfn1->PteAddress = PointerPte;
        Pfn1->u3.e1.PageLocation = ActiveAndValid;
        Pfn1->u4.VerifierAllocation = 0;

        /* Write the PTE for it */
        TempPte.u.Hard.PageFrameNumber = PageFrameNumber;
        MI_WRITE_VALID_PTE(PointerPte++, TempPte);
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

    /* Mark it as a verifier allocation if needed */
    ASSERT(Pfn1->u4.VerifierAllocation == 0);
    if (PoolType & VERIFIER_POOL_MASK) Pfn1->u4.VerifierAllocation = 1;

    //
    // Release the PFN and nonpaged pool lock
    //
    MiReleasePfnLockFromDpcLevel();
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
    PFN_COUNT FreePages, NumberOfPages;
    KIRQL OldIrql;
    PMMFREE_POOL_ENTRY FreeEntry, NextEntry, LastEntry;
    ULONG i, End;
    ULONG_PTR Offset;

    //
    // Handle paged pool
    //
    if ((StartingVa >= MmPagedPoolStart) && (StartingVa <= MmPagedPoolEnd))
    {
        //
        // Calculate the offset from the beginning of paged pool, and convert it
        // into pages
        //
        Offset = (ULONG_PTR)StartingVa - (ULONG_PTR)MmPagedPoolStart;
        i = (ULONG)(Offset >> PAGE_SHIFT);
        End = i;

        //
        // Now use the end bitmap to scan until we find a set bit, meaning that
        // this allocation finishes here
        //
        while (!RtlTestBit(MmPagedPoolInfo.EndOfPagedPoolBitmap, End)) End++;

        //
        // Now calculate the total number of pages this allocation spans. If it's
        // only one page, add it to the S-LIST instead of freeing it
        //
        NumberOfPages = End - i + 1;
        if ((NumberOfPages == 1) &&
            (ExQueryDepthSList(&MiPagedPoolSListHead) < MiPagedPoolSListMaximum))
        {
            InterlockedPushEntrySList(&MiPagedPoolSListHead, StartingVa);
            return 1;
        }

        /* Delete the actual pages */
        PointerPte = MmPagedPoolInfo.FirstPteForPagedPool + i;
        FreePages = MiDeleteSystemPageableVm(PointerPte, NumberOfPages, 0, NULL);
        ASSERT(FreePages == NumberOfPages);

        //
        // Acquire the paged pool lock
        //
        KeAcquireGuardedMutex(&MmPagedPoolMutex);

        //
        // Clear the allocation and free bits
        //
        RtlClearBit(MmPagedPoolInfo.EndOfPagedPoolBitmap, End);
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
    // Get the first PTE and its corresponding PFN entry. If this is also the
    // last PTE, meaning that this allocation was only for one page, push it into
    // the S-LIST instead of freeing it
    //
    StartPte = PointerPte = MiAddressToPte(StartingVa);
    StartPfn = Pfn1 = MiGetPfnEntry(PointerPte->u.Hard.PageFrameNumber);
    if ((Pfn1->u3.e1.EndOfAllocation == 1) &&
        (ExQueryDepthSList(&MiNonPagedPoolSListHead) < MiNonPagedPoolSListMaximum))
    {
        InterlockedPushEntrySList(&MiNonPagedPoolSListHead, StartingVa);
        return 1;
    }

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
    NumberOfPages = (PFN_COUNT)(PointerPte - StartPte + 1);

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
        /* Sanity check */
        ASSERT((ULONG_PTR)StartingVa + NumberOfPages <= (ULONG_PTR)MmNonPagedPoolEnd);

        /* Check if protected pool is enabled */
        if (MmProtectFreedNonPagedPool)
        {
            /* The freed block will be merged, it must be made accessible */
            MiUnProtectFreeNonPagedPool(MiPteToAddress(PointerPte), 0);
        }

        //
        // Otherwise, our entire allocation must've fit within the initial non
        // paged pool, or the expansion nonpaged pool, so get the PFN entry of
        // the next allocation
        //
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
        ASSERT(FreeEntry->Signature == MM_FREE_POOL_SIGNATURE);
        ASSERT(FreeEntry->Owner == FreeEntry);

        /* Consume this entry's pages */
        FreePages += FreeEntry->Size;

        /* Remove the item from the list, depending if pool is protected */
        if (MmProtectFreedNonPagedPool)
            MiProtectedPoolRemoveEntryList(&FreeEntry->List);
        else
            RemoveEntryList(&FreeEntry->List);
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

        /* Check if protected pool is enabled */
        if (MmProtectFreedNonPagedPool)
        {
            /* The freed block will be merged, it must be made accessible */
            MiUnProtectFreeNonPagedPool(MiPteToAddress(PointerPte), 0);
        }

        /* Check if this is valid pool, or a guard page */
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
        ASSERT(FreeEntry->Signature == MM_FREE_POOL_SIGNATURE);
        FreeEntry = FreeEntry->Owner;

        /* Check if protected pool is enabled */
        if (MmProtectFreedNonPagedPool)
        {
            /* The freed block will be merged, it must be made accessible */
            MiUnProtectFreeNonPagedPool(FreeEntry, 0);
        }

        //
        // Check if the entry is small enough (1-3 pages) to be indexed on a free list
        // If it is, we'll want to re-insert it, since we're about to
        // collapse our pages on top of it, which will change its count
        //
        if (FreeEntry->Size < MI_MAX_FREE_PAGE_LISTS)
        {
            /* Remove the item from the list, depending if pool is protected */
            if (MmProtectFreedNonPagedPool)
                MiProtectedPoolRemoveEntryList(&FreeEntry->List);
            else
                RemoveEntryList(&FreeEntry->List);

            //
            // Update its size
            //
            FreeEntry->Size += FreePages;

            //
            // And now find the new appropriate list to place it in
            //
            i = min(FreeEntry->Size, MI_MAX_FREE_PAGE_LISTS) - 1;

            /* Insert the entry into the free list head, check for prot. pool */
            if (MmProtectFreedNonPagedPool)
                MiProtectedPoolInsertList(&MmNonPagedPoolFreeListHead[i], &FreeEntry->List, TRUE);
            else
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
        i = min(FreeEntry->Size, MI_MAX_FREE_PAGE_LISTS) - 1;

        /* Insert the entry into the free list head, check for prot. pool */
        if (MmProtectFreedNonPagedPool)
            MiProtectedPoolInsertList(&MmNonPagedPoolFreeListHead[i], &FreeEntry->List, TRUE);
        else
            InsertTailList(&MmNonPagedPoolFreeListHead[i], &FreeEntry->List);
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
        NextEntry->Signature = MM_FREE_POOL_SIGNATURE;
        NextEntry = (PMMFREE_POOL_ENTRY)((ULONG_PTR)NextEntry + PAGE_SIZE);
    } while (NextEntry != LastEntry);

    /* Is freed non paged pool protected? */
    if (MmProtectFreedNonPagedPool)
    {
        /* Protect the freed pool! */
        MiProtectFreeNonPagedPool(FreeEntry, FreeEntry->Size);
    }

    //
    // We're done, release the lock and let the caller know how much we freed
    //
    KeReleaseQueuedSpinLock(LockQueueMmNonPagedPoolLock, OldIrql);
    return NumberOfPages;
}

NTSTATUS
NTAPI
MiInitializeSessionPool(VOID)
{
    PMMPTE PointerPte, LastPte;
    PMMPDE PointerPde, LastPde;
    PFN_NUMBER PageFrameIndex, PdeCount;
    PPOOL_DESCRIPTOR PoolDescriptor;
    PMM_SESSION_SPACE SessionGlobal;
    PMM_PAGED_POOL_INFO PagedPoolInfo;
    NTSTATUS Status;
    ULONG Index, PoolSize, BitmapSize;
    PAGED_CODE();

    /* Lock session pool */
    SessionGlobal = MmSessionSpace->GlobalVirtualAddress;
    KeInitializeGuardedMutex(&SessionGlobal->PagedPoolMutex);

    /* Setup a valid pool descriptor */
    PoolDescriptor = &MmSessionSpace->PagedPool;
    ExInitializePoolDescriptor(PoolDescriptor,
                               PagedPoolSession,
                               0,
                               0,
                               &SessionGlobal->PagedPoolMutex);

    /* Setup the pool addresses */
    MmSessionSpace->PagedPoolStart = (PVOID)MiSessionPoolStart;
    MmSessionSpace->PagedPoolEnd = (PVOID)((ULONG_PTR)MiSessionPoolEnd - 1);
    DPRINT1("Session Pool Start: 0x%p End: 0x%p\n",
            MmSessionSpace->PagedPoolStart, MmSessionSpace->PagedPoolEnd);

    /* Reset all the counters */
    PagedPoolInfo = &MmSessionSpace->PagedPoolInfo;
    PagedPoolInfo->PagedPoolCommit = 0;
    PagedPoolInfo->PagedPoolHint = 0;
    PagedPoolInfo->AllocatedPagedPool = 0;

    /* Compute PDE and PTE addresses */
    PointerPde = MiAddressToPde(MmSessionSpace->PagedPoolStart);
    PointerPte = MiAddressToPte(MmSessionSpace->PagedPoolStart);
    LastPde = MiAddressToPde(MmSessionSpace->PagedPoolEnd);
    LastPte = MiAddressToPte(MmSessionSpace->PagedPoolEnd);

    /* Write them down */
    MmSessionSpace->PagedPoolBasePde = PointerPde;
    PagedPoolInfo->FirstPteForPagedPool = PointerPte;
    PagedPoolInfo->LastPteForPagedPool = LastPte;
    PagedPoolInfo->NextPdeForPagedPoolExpansion = PointerPde + 1;

    /* Zero the PDEs */
    PdeCount = LastPde - PointerPde;
    RtlZeroMemory(PointerPde, (PdeCount + 1) * sizeof(MMPTE));

    /* Initialize the PFN for the PDE */
    Status = MiInitializeAndChargePfn(&PageFrameIndex,
                                      PointerPde,
                                      MmSessionSpace->SessionPageDirectoryIndex,
                                      TRUE);
    ASSERT(NT_SUCCESS(Status) == TRUE);

    /* Initialize the first page table */
    Index = (ULONG_PTR)MmSessionSpace->PagedPoolStart - (ULONG_PTR)MmSessionBase;
    Index >>= 22;
#ifndef _M_AMD64 // FIXME
    ASSERT(MmSessionSpace->PageTables[Index].u.Long == 0);
    MmSessionSpace->PageTables[Index] = *PointerPde;
#endif

    /* Bump up counters */
    InterlockedIncrementSizeT(&MmSessionSpace->NonPageablePages);
    InterlockedIncrementSizeT(&MmSessionSpace->CommittedPages);

    /* Compute the size of the pool in pages, and of the bitmap for it */
    PoolSize = MmSessionPoolSize >> PAGE_SHIFT;
    BitmapSize = sizeof(RTL_BITMAP) + ((PoolSize + 31) / 32) * sizeof(ULONG);

    /* Allocate and initialize the bitmap to track allocations */
    PagedPoolInfo->PagedPoolAllocationMap = ExAllocatePoolWithTag(NonPagedPool,
                                                                  BitmapSize,
                                                                  TAG_MM);
    ASSERT(PagedPoolInfo->PagedPoolAllocationMap != NULL);
    RtlInitializeBitMap(PagedPoolInfo->PagedPoolAllocationMap,
                        (PULONG)(PagedPoolInfo->PagedPoolAllocationMap + 1),
                        PoolSize);

    /* Set all bits, but clear the first page table's worth */
    RtlSetAllBits(PagedPoolInfo->PagedPoolAllocationMap);
    RtlClearBits(PagedPoolInfo->PagedPoolAllocationMap, 0, PTE_PER_PAGE);

    /* Allocate and initialize the bitmap to track free space */
    PagedPoolInfo->EndOfPagedPoolBitmap = ExAllocatePoolWithTag(NonPagedPool,
                                                                BitmapSize,
                                                                TAG_MM);
    ASSERT(PagedPoolInfo->EndOfPagedPoolBitmap != NULL);
    RtlInitializeBitMap(PagedPoolInfo->EndOfPagedPoolBitmap,
                        (PULONG)(PagedPoolInfo->EndOfPagedPoolBitmap + 1),
                        PoolSize);

    /* Clear all the bits and return success */
    RtlClearAllBits(PagedPoolInfo->EndOfPagedPoolBitmap);
    return STATUS_SUCCESS;
}

/**
 * @brief
 * Raises the quota limit, depending on the given
 * pool type of the quota in question. The routine
 * is used exclusively by Process Manager for
 * quota handling.
 *
 * @param[in] PoolType
 * The type of quota pool which the quota in question
 * has to be raised.
 *
 * @param[in] CurrentMaxQuota
 * The current maximum limit of quota threshold.
 *
 * @param[out] NewMaxQuota
 * The newly raised maximum limit of quota threshold,
 * returned to the caller.
 *
 * @return
 * Returns TRUE if quota raising procedure has succeeded
 * without problems, FALSE otherwise.
 *
 * @remarks
 * A spin lock must be held when raising the pool quota
 * limit to avoid race occurences.
 */
_Requires_lock_held_(PspQuotaLock)
BOOLEAN
NTAPI
MmRaisePoolQuota(
    _In_ POOL_TYPE PoolType,
    _In_ SIZE_T CurrentMaxQuota,
    _Out_ PSIZE_T NewMaxQuota)
{
    /*
     * We must be in dispatch level interrupt here
     * as we should be under a spin lock at this point.
     */
    ASSERT_IRQL_EQUAL(DISPATCH_LEVEL);

    switch (PoolType)
    {
        case NonPagedPool:
        {
            /*
             * When concerning with a raise (charge) of quota
             * in a non paged pool scenario, make sure that
             * we've got at least 200 pages necessary to provide.
             */
            if (MmAvailablePages < MI_QUOTA_NON_PAGED_NEEDED_PAGES)
            {
                DPRINT1("MmRaisePoolQuota(): Not enough pages available (current pages -- %lu)\n", MmAvailablePages);
                return FALSE;
            }

            /*
             * Check if there's at least some space available
             * in the non paged pool area.
             */
            if (MmMaximumNonPagedPoolInPages < (MmAllocatedNonPagedPool >> PAGE_SHIFT))
            {
                /* There's too much allocated space, bail out */
                DPRINT1("MmRaisePoolQuota(): Failed to increase pool quota, not enough non paged pool space (current size -- %lu || allocated size -- %lu)\n",
                        MmMaximumNonPagedPoolInPages, MmAllocatedNonPagedPool);
                return FALSE;
            }

            /* Do we have enough resident pages to increase our quota? */
            if (MmResidentAvailablePages < MI_NON_PAGED_QUOTA_MIN_RESIDENT_PAGES)
            {
                DPRINT1("MmRaisePoolQuota(): Failed to increase pool quota, not enough resident pages available (current available pages -- %lu)\n",
                        MmResidentAvailablePages);
                return FALSE;
            }

            /*
             * Raise the non paged pool quota indicator and set
             * up new maximum limit of quota for the process.
             */
            MmTotalNonPagedPoolQuota += MI_CHARGE_NON_PAGED_POOL_QUOTA;
            *NewMaxQuota = CurrentMaxQuota + MI_CHARGE_NON_PAGED_POOL_QUOTA;
            DPRINT("MmRaisePoolQuota(): Non paged pool quota increased (before -- %lu || after -- %lu)\n", CurrentMaxQuota, NewMaxQuota);
            return TRUE;
        }

        case PagedPool:
        {
            /*
             * Before raising the quota limit of a paged quota
             * pool, make sure we've got enough space that is available.
             * On Windows it seems it wants to check for at least 1 MB of space
             * needed so that it would be possible to raise the paged pool quota.
             */
            if (MmSizeOfPagedPoolInPages < (MmPagedPoolInfo.AllocatedPagedPool >> PAGE_SHIFT))
            {
                /* We haven't gotten enough space, bail out */
                DPRINT1("MmRaisePoolQuota(): Failed to increase pool quota, not enough paged pool space (current size -- %lu || allocated size -- %lu)\n",
                        MmSizeOfPagedPoolInPages, MmPagedPoolInfo.AllocatedPagedPool >> PAGE_SHIFT);
                return FALSE;
            }

            /*
             * Raise the paged pool quota indicator and set
             * up new maximum limit of quota for the process.
             */
            MmTotalPagedPoolQuota += MI_CHARGE_PAGED_POOL_QUOTA;
            *NewMaxQuota = CurrentMaxQuota + MI_CHARGE_PAGED_POOL_QUOTA;
            DPRINT("MmRaisePoolQuota(): Paged pool quota increased (before -- %lu || after -- %lu)\n", CurrentMaxQuota, NewMaxQuota);
            return TRUE;
        }

        /* Only NonPagedPool and PagedPool are used */
        DEFAULT_UNREACHABLE;
    }
}

/**
 * @brief
 * Returns the quota, depending on the given
 * pool type of the quota in question. The routine
 * is used exclusively by Process Manager for quota
 * handling.
 *
 * @param[in] PoolType
 * The type of quota pool which the quota in question
 * has to be raised.
 *
 * @param[in] CurrentMaxQuota
 * The current maximum limit of quota threshold.
 *
 * @return
 * Nothing.
 *
 * @remarks
 * A spin lock must be held when raising the pool quota
 * limit to avoid race occurences.
 */
_Requires_lock_held_(PspQuotaLock)
VOID
NTAPI
MmReturnPoolQuota(
    _In_ POOL_TYPE PoolType,
    _In_ SIZE_T QuotaToReturn)
{
    /*
     * We must be in dispatch level interrupt here
     * as we should be under a spin lock at this point.
     */
    ASSERT_IRQL_EQUAL(DISPATCH_LEVEL);

    switch (PoolType)
    {
        case NonPagedPool:
        {
            /* This is a non paged pool type, decrease the non paged quota */
            ASSERT(MmTotalNonPagedPoolQuota >= QuotaToReturn);
            MmTotalNonPagedPoolQuota -= QuotaToReturn;
            DPRINT("MmReturnPoolQuota(): Non paged pool quota returned (current size -- %lu)\n", MmTotalNonPagedPoolQuota);
            break;
        }

        case PagedPool:
        {
            /* This is a paged pool type, decrease the paged quota */
            ASSERT(MmTotalPagedPoolQuota >= QuotaToReturn);
            MmTotalPagedPoolQuota -= QuotaToReturn;
            DPRINT("MmReturnPoolQuota(): Paged pool quota returned (current size -- %lu)\n", MmTotalPagedPoolQuota);
            break;
        }

        /* Only NonPagedPool and PagedPool are used */
        DEFAULT_UNREACHABLE;
    }
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
