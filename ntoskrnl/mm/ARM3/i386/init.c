/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            ntoskrnl/mm/ARM3/i386/init.c
 * PURPOSE:         ARM Memory Manager Initialization for x86
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

#define MODULE_INVOLVED_IN_ARM3
#include <mm/ARM3/miarm.h>

/* GLOBALS ********************************************************************/

/* Template PTE and PDE for a kernel page */
 /* FIXME: These should be PTE_GLOBAL */
MMPTE ValidKernelPde = {{PTE_VALID|PTE_READWRITE|PTE_DIRTY|PTE_ACCESSED}};
MMPTE ValidKernelPte = {{PTE_VALID|PTE_READWRITE|PTE_DIRTY|PTE_ACCESSED}};

/* The same, but for local pages */
MMPTE ValidKernelPdeLocal = {{PTE_VALID|PTE_READWRITE|PTE_DIRTY|PTE_ACCESSED}};
MMPTE ValidKernelPteLocal = {{PTE_VALID|PTE_READWRITE|PTE_DIRTY|PTE_ACCESSED}};

/* Template PDE for a demand-zero page */
MMPDE DemandZeroPde  = {{MM_READWRITE << MM_PTE_SOFTWARE_PROTECTION_BITS}};
MMPTE DemandZeroPte  = {{MM_READWRITE << MM_PTE_SOFTWARE_PROTECTION_BITS}};

/* Template PTE for prototype page */
MMPTE PrototypePte = {{(MM_READWRITE << MM_PTE_SOFTWARE_PROTECTION_BITS) |
                      PTE_PROTOTYPE | (MI_PTE_LOOKUP_NEEDED << PAGE_SHIFT)}};

/* Template PTE for decommited page */
MMPTE MmDecommittedPte = {{MM_DECOMMIT << MM_PTE_SOFTWARE_PROTECTION_BITS}};

/* PRIVATE FUNCTIONS **********************************************************/

CODE_SEG("INIT")
VOID
NTAPI
MiInitializeSessionSpaceLayout(VOID)
{
    //
    // Set the size of session view, pool, and image
    //
    MmSessionSize = MI_SESSION_SIZE;
    MmSessionViewSize = MI_SESSION_VIEW_SIZE;
    MmSessionPoolSize = MI_SESSION_POOL_SIZE;
    MmSessionImageSize = MI_SESSION_IMAGE_SIZE;

    //
    // Set the size of system view
    //
    MmSystemViewSize = MI_SYSTEM_VIEW_SIZE;

    //
    // This is where it all ends
    //
    MiSessionImageEnd = (PVOID)PTE_BASE;

    //
    // This is where we will load Win32k.sys and the video driver
    //
    MiSessionImageStart = (PVOID)((ULONG_PTR)MiSessionImageEnd -
                                  MmSessionImageSize);

    //
    // So the view starts right below the session working set (itself below
    // the image area)
    //
    MiSessionViewStart = (PVOID)((ULONG_PTR)MiSessionImageEnd -
                                 MmSessionImageSize -
                                 MI_SESSION_WORKING_SET_SIZE -
                                 MmSessionViewSize);

    //
    // Session pool follows
    //
    MiSessionPoolEnd = MiSessionViewStart;
    MiSessionPoolStart = (PVOID)((ULONG_PTR)MiSessionPoolEnd -
                                 MmSessionPoolSize);

    //
    // And it all begins here
    //
    MmSessionBase = MiSessionPoolStart;

    //
    // Sanity check that our math is correct
    //
    ASSERT((ULONG_PTR)MmSessionBase + MmSessionSize == PTE_BASE);

    //
    // Session space ends wherever image session space ends
    //
    MiSessionSpaceEnd = MiSessionImageEnd;

    //
    // System view space ends at session space, so now that we know where
    // this is, we can compute the base address of system view space itself.
    //
    MiSystemViewStart = (PVOID)((ULONG_PTR)MmSessionBase -
                                MmSystemViewSize);

    /* Compute the PTE addresses for all the addresses we carved out */
    MiSessionImagePteStart = MiAddressToPte(MiSessionImageStart);
    MiSessionImagePteEnd = MiAddressToPte(MiSessionImageEnd);
    MiSessionBasePte = MiAddressToPte(MmSessionBase);
    MiSessionSpaceWs = (PVOID)((ULONG_PTR)MiSessionViewStart + MmSessionViewSize);
    MiSessionLastPte = MiAddressToPte(MiSessionSpaceEnd);

    /* Initialize session space */
    MmSessionSpace = (PMM_SESSION_SPACE)((ULONG_PTR)MmSessionBase +
                                         MmSessionSize -
                                         MmSessionImageSize -
                                         MM_ALLOCATION_GRANULARITY);
}

CODE_SEG("INIT")
VOID
NTAPI
MiComputeNonPagedPoolVa(IN ULONG FreePages)
{
    IN PFN_NUMBER PoolPages;

    /* Check if this is a machine with less than 256MB of RAM, and no overide */
    if ((MmNumberOfPhysicalPages <= MI_MIN_PAGES_FOR_NONPAGED_POOL_TUNING) &&
        !(MmSizeOfNonPagedPoolInBytes))
    {
        /* Force the non paged pool to be 2MB so we can reduce RAM usage */
        MmSizeOfNonPagedPoolInBytes = 2 * _1MB;
    }

    /* Hyperspace ends here */
    MmHyperSpaceEnd = (PVOID)((ULONG_PTR)MmSystemCacheWorkingSetList - 1);

    /* Check if the user gave a ridicuously large nonpaged pool RAM size */
    if ((MmSizeOfNonPagedPoolInBytes >> PAGE_SHIFT) > (FreePages * 7 / 8))
    {
        /* More than 7/8ths of RAM was dedicated to nonpaged pool, ignore! */
        MmSizeOfNonPagedPoolInBytes = 0;
    }

    /* Check if no registry setting was set, or if the setting was too low */
    if (MmSizeOfNonPagedPoolInBytes < MmMinimumNonPagedPoolSize)
    {
        /* Start with the minimum (256 KB) and add 32 KB for each MB above 4 */
        MmSizeOfNonPagedPoolInBytes = MmMinimumNonPagedPoolSize;
        MmSizeOfNonPagedPoolInBytes += (FreePages - 1024) / 256 * MmMinAdditionNonPagedPoolPerMb;
    }

    /* Check if the registy setting or our dynamic calculation was too high */
    if (MmSizeOfNonPagedPoolInBytes > MI_MAX_INIT_NONPAGED_POOL_SIZE)
    {
        /* Set it to the maximum */
        MmSizeOfNonPagedPoolInBytes = MI_MAX_INIT_NONPAGED_POOL_SIZE;
    }

    /* Check if a percentage cap was set through the registry */
    if (MmMaximumNonPagedPoolPercent) UNIMPLEMENTED;

    /* Page-align the nonpaged pool size */
    MmSizeOfNonPagedPoolInBytes &= ~(PAGE_SIZE - 1);

    /* Now, check if there was a registry size for the maximum size */
    if (!MmMaximumNonPagedPoolInBytes)
    {
        /* Start with the default (1MB) */
        MmMaximumNonPagedPoolInBytes = MmDefaultMaximumNonPagedPool;

        /* Add space for PFN database */
        MmMaximumNonPagedPoolInBytes += (ULONG)
            PAGE_ALIGN((MmHighestPhysicalPage +  1) * sizeof(MMPFN));

        /* Check if the machine has more than 512MB of free RAM */
        if (FreePages >= 0x1F000)
        {
            /* Add 200KB for each MB above 4 */
            MmMaximumNonPagedPoolInBytes += (FreePages - 1024) / 256 *
                                            (MmMaxAdditionNonPagedPoolPerMb / 2);
            if (MmMaximumNonPagedPoolInBytes < MI_MAX_NONPAGED_POOL_SIZE)
            {
                /* Make it at least 128MB since this machine has a lot of RAM */
                MmMaximumNonPagedPoolInBytes = MI_MAX_NONPAGED_POOL_SIZE;
            }
        }
        else
        {
            /* Add 400KB for each MB above 4 */
            MmMaximumNonPagedPoolInBytes += (FreePages - 1024) / 256 *
                                            MmMaxAdditionNonPagedPoolPerMb;
        }
    }

    /* Make sure there's at least 16 pages + the PFN available for expansion */
    PoolPages = MmSizeOfNonPagedPoolInBytes + (PAGE_SIZE * 16) +
                ((ULONG)PAGE_ALIGN(MmHighestPhysicalPage + 1) * sizeof(MMPFN));
    if (MmMaximumNonPagedPoolInBytes < PoolPages)
    {
        /* The maximum should be at least high enough to cover all the above */
        MmMaximumNonPagedPoolInBytes = PoolPages;
    }

    /* Systems with 2GB of kernel address space get double the size */
    PoolPages = MI_MAX_NONPAGED_POOL_SIZE * 2;

    /* On the other hand, make sure that PFN + nonpaged pool doesn't get too big */
    if (MmMaximumNonPagedPoolInBytes > PoolPages)
    {
        /* Trim it down to the maximum architectural limit (256MB) */
        MmMaximumNonPagedPoolInBytes = PoolPages;
    }

    /* Check if this is a system with > 128MB of non paged pool */
    if (MmMaximumNonPagedPoolInBytes > MI_MAX_NONPAGED_POOL_SIZE)
    {
        /* Check if the initial size is less than the extra 128MB boost */
        if (MmSizeOfNonPagedPoolInBytes < (MmMaximumNonPagedPoolInBytes -
                                           MI_MAX_NONPAGED_POOL_SIZE))
        {
            /* FIXME: Should check if the initial pool can be expanded */

            /* Assume no expansion possible, check ift he maximum is too large */
            if (MmMaximumNonPagedPoolInBytes > (MmSizeOfNonPagedPoolInBytes +
                                                MI_MAX_NONPAGED_POOL_SIZE))
            {
                /* Set it to the initial value plus the boost */
                MmMaximumNonPagedPoolInBytes = MmSizeOfNonPagedPoolInBytes +
                                               MI_MAX_NONPAGED_POOL_SIZE;
            }
        }
    }
}

CODE_SEG("INIT")
NTSTATUS
NTAPI
MiInitMachineDependent(IN PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    PFN_NUMBER PageFrameIndex;
    PMMPTE StartPde, EndPde, PointerPte, LastPte;
    MMPTE TempPde, TempPte;
    PVOID NonPagedPoolExpansionVa;
    SIZE_T NonPagedSystemSize;
    KIRQL OldIrql;
    PMMPFN Pfn1;
    ULONG Flags;

#if defined(_GLOBAL_PAGES_ARE_AWESOME_)

    /* Check for global bit */
    if (KeFeatureBits & KF_GLOBAL_PAGE)
    {
        /* Set it on the template PTE and PDE */
        ValidKernelPte.u.Hard.Global = TRUE;
        ValidKernelPde.u.Hard.Global = TRUE;
    }

#endif

    /* Now templates are ready */
    TempPte = ValidKernelPte;
    TempPde = ValidKernelPde;

    //
    // Set CR3 for the system process
    //
    PointerPte = MiAddressToPde(PDE_BASE);
    PageFrameIndex = PFN_FROM_PTE(PointerPte) << PAGE_SHIFT;
    PsGetCurrentProcess()->Pcb.DirectoryTableBase[0] = PageFrameIndex;

    //
    // Blow away user-mode
    //
    StartPde = MiAddressToPde(0);
    EndPde = MiAddressToPde(KSEG0_BASE);
    RtlZeroMemory(StartPde, (EndPde - StartPde) * sizeof(MMPTE));

    /* Compute non paged pool limits and size */
    MiComputeNonPagedPoolVa(MiNumberOfFreePages);

    //
    // Now calculate the nonpaged pool expansion VA region
    //
    MmNonPagedPoolStart = (PVOID)((ULONG_PTR)MmNonPagedPoolEnd -
                                  MmMaximumNonPagedPoolInBytes +
                                  MmSizeOfNonPagedPoolInBytes);
    MmNonPagedPoolStart = (PVOID)PAGE_ALIGN(MmNonPagedPoolStart);
    NonPagedPoolExpansionVa = MmNonPagedPoolStart;
    DPRINT("NP Pool has been tuned to: %lu bytes and %lu bytes\n",
           MmSizeOfNonPagedPoolInBytes, MmMaximumNonPagedPoolInBytes);

    //
    // Now calculate the nonpaged system VA region, which includes the
    // nonpaged pool expansion (above) and the system PTEs. Note that it is
    // then aligned to a PDE boundary (4MB).
    //
    NonPagedSystemSize = (MmNumberOfSystemPtes + 1) * PAGE_SIZE;
    MmNonPagedSystemStart = (PVOID)((ULONG_PTR)MmNonPagedPoolStart -
                                    NonPagedSystemSize);
    MmNonPagedSystemStart = (PVOID)((ULONG_PTR)MmNonPagedSystemStart &
                                    ~(PDE_MAPPED_VA - 1));

    //
    // Don't let it go below the minimum
    //
    if (MmNonPagedSystemStart < (PVOID)0xEB000000)
    {
        //
        // This is a hard-coded limit in the Windows NT address space
        //
        MmNonPagedSystemStart = (PVOID)0xEB000000;

        //
        // Reduce the amount of system PTEs to reach this point
        //
        MmNumberOfSystemPtes = ((ULONG_PTR)MmNonPagedPoolStart -
                                (ULONG_PTR)MmNonPagedSystemStart) >>
                                PAGE_SHIFT;
        MmNumberOfSystemPtes--;
        ASSERT(MmNumberOfSystemPtes > 1000);
    }

    //
    // Check if we are in a situation where the size of the paged pool
    // is so large that it overflows into nonpaged pool
    //
    if (MmSizeOfPagedPoolInBytes >
        ((ULONG_PTR)MmNonPagedSystemStart - (ULONG_PTR)MmPagedPoolStart))
    {
        //
        // We need some recalculations here
        //
        DPRINT1("Paged pool is too big!\n");
    }

    //
    // Normally, the PFN database should start after the loader images.
    // This is already the case in ReactOS, but for now we want to co-exist
    // with the old memory manager, so we'll create a "Shadow PFN Database"
    // instead, and arbitrarly start it at 0xB0000000.
    //
    MmPfnDatabase = (PVOID)0xB0000000;
    ASSERT(((ULONG_PTR)MmPfnDatabase & (PDE_MAPPED_VA - 1)) == 0);

    //
    // Non paged pool comes after the PFN database
    //
    MmNonPagedPoolStart = (PVOID)((ULONG_PTR)MmPfnDatabase +
                                  (MxPfnAllocation << PAGE_SHIFT));

    //
    // Now we actually need to get these many physical pages. Nonpaged pool
    // is actually also physically contiguous (but not the expansion)
    //
    PageFrameIndex = MxGetNextPage(MxPfnAllocation +
                                   (MmSizeOfNonPagedPoolInBytes >> PAGE_SHIFT));
    ASSERT(PageFrameIndex != 0);
    DPRINT("PFN DB PA PFN begins at: %lx\n", PageFrameIndex);
    DPRINT("NP PA PFN begins at: %lx\n", PageFrameIndex + MxPfnAllocation);

    /* Convert nonpaged pool size from bytes to pages */
    MmMaximumNonPagedPoolInPages = MmMaximumNonPagedPoolInBytes >> PAGE_SHIFT;

    //
    // Now we need some pages to create the page tables for the NP system VA
    // which includes system PTEs and expansion NP
    //
    StartPde = MiAddressToPde(MmNonPagedSystemStart);
    EndPde = MiAddressToPde((PVOID)((ULONG_PTR)MmNonPagedPoolEnd - 1));
    while (StartPde <= EndPde)
    {
        //
        // Get a page
        //
        TempPde.u.Hard.PageFrameNumber = MxGetNextPage(1);
        MI_WRITE_VALID_PTE(StartPde, TempPde);

        //
        // Zero out the page table
        //
        PointerPte = MiPteToAddress(StartPde);
        RtlZeroMemory(PointerPte, PAGE_SIZE);

        //
        // Next
        //
        StartPde++;
    }

    //
    // Now we need pages for the page tables which will map initial NP
    //
    StartPde = MiAddressToPde(MmPfnDatabase);
    EndPde = MiAddressToPde((PVOID)((ULONG_PTR)MmNonPagedPoolStart +
                                    MmSizeOfNonPagedPoolInBytes - 1));
    while (StartPde <= EndPde)
    {
        //
        // Get a page
        //
        TempPde.u.Hard.PageFrameNumber = MxGetNextPage(1);
        MI_WRITE_VALID_PTE(StartPde, TempPde);

        //
        // Zero out the page table
        //
        PointerPte = MiPteToAddress(StartPde);
        RtlZeroMemory(PointerPte, PAGE_SIZE);

        //
        // Next
        //
        StartPde++;
    }

    MmSubsectionBase = (ULONG_PTR)MmNonPagedPoolStart;

    //
    // Now remember where the expansion starts
    //
    MmNonPagedPoolExpansionStart = NonPagedPoolExpansionVa;

    //
    // Last step is to actually map the nonpaged pool
    //
    PointerPte = MiAddressToPte(MmNonPagedPoolStart);
    LastPte = MiAddressToPte((PVOID)((ULONG_PTR)MmNonPagedPoolStart +
                                     MmSizeOfNonPagedPoolInBytes - 1));
    while (PointerPte <= LastPte)
    {
        //
        // Use one of our contigous pages
        //
        TempPte.u.Hard.PageFrameNumber = PageFrameIndex++;
        MI_WRITE_VALID_PTE(PointerPte++, TempPte);
    }

    //
    // Sanity check: make sure we have properly defined the system PTE space
    //
    ASSERT(MiAddressToPte(MmNonPagedSystemStart) <
           MiAddressToPte(MmNonPagedPoolExpansionStart));

    /* Now go ahead and initialize the nonpaged pool */
    MiInitializeNonPagedPool();
    MiInitializeNonPagedPoolThresholds();

    /* Map the PFN database pages */
    MiMapPfnDatabase(LoaderBlock);

    /* Initialize the color tables */
    MiInitializeColorTables();

    /* Build the PFN Database */
    MiInitializePfnDatabase(LoaderBlock);
    MmInitializeBalancer(MmAvailablePages, 0);

    //
    // Reset the descriptor back so we can create the correct memory blocks
    //
    *MxFreeDescriptor = MxOldFreeDescriptor;

    //
    // Initialize the nonpaged pool
    //
    InitializePool(NonPagedPool, 0);

    //
    // We PDE-aligned the nonpaged system start VA, so haul some extra PTEs!
    //
    PointerPte = MiAddressToPte(MmNonPagedSystemStart);
    MmNumberOfSystemPtes = MiAddressToPte(MmNonPagedPoolExpansionStart) -
                           PointerPte;
    MmNumberOfSystemPtes--;
    DPRINT("Final System PTE count: %lu (%lu bytes)\n",
           MmNumberOfSystemPtes, MmNumberOfSystemPtes * PAGE_SIZE);

    //
    // Create the system PTE space
    //
    MiInitializeSystemPtes(PointerPte, MmNumberOfSystemPtes, SystemPteSpace);

    /* Get the PDE For hyperspace */
    StartPde = MiAddressToPde(HYPER_SPACE);

    /* Lock PFN database */
    OldIrql = MiAcquirePfnLock();

    /* Allocate a page for hyperspace and create it */
    MI_SET_USAGE(MI_USAGE_PAGE_TABLE);
    MI_SET_PROCESS2("Kernel");
    PageFrameIndex = MiRemoveAnyPage(0);
    TempPde = ValidKernelPdeLocal;
    TempPde.u.Hard.PageFrameNumber = PageFrameIndex;
    MI_WRITE_VALID_PTE(StartPde, TempPde);
    PsGetCurrentProcess()->Pcb.DirectoryTableBase[1] = PageFrameIndex << PAGE_SHIFT;

    /* Flush the TLB */
    KeFlushCurrentTb();

    /* Release the lock */
    MiReleasePfnLock(OldIrql);

    //
    // Zero out the page table now
    //
    PointerPte = MiAddressToPte(HYPER_SPACE);
    RtlZeroMemory(PointerPte, PAGE_SIZE);

    //
    // Setup the mapping PTEs
    //
    MmFirstReservedMappingPte = MiAddressToPte(MI_MAPPING_RANGE_START);
    MmLastReservedMappingPte = MiAddressToPte(MI_MAPPING_RANGE_END);
    MmFirstReservedMappingPte->u.Hard.PageFrameNumber = MI_HYPERSPACE_PTES;

    /* Set the working set address */
    MmWorkingSetList = (PVOID)MI_WORKING_SET_LIST;

    //
    // Reserve system PTEs for zeroing PTEs and clear them
    //
    MiFirstReservedZeroingPte = MiReserveSystemPtes(MI_ZERO_PTES + 1,
                                                    SystemPteSpace);
    RtlZeroMemory(MiFirstReservedZeroingPte, (MI_ZERO_PTES + 1) * sizeof(MMPTE));

    //
    // Set the counter to maximum to boot with
    //
    MiFirstReservedZeroingPte->u.Hard.PageFrameNumber = MI_ZERO_PTES;

    /* Lock PFN database */
    OldIrql = MiAcquirePfnLock();

    /* Reset the ref/share count so that MmInitializeProcessAddressSpace works */
    Pfn1 = MiGetPfnEntry(PFN_FROM_PTE(MiAddressToPde(PDE_BASE)));
    Pfn1->u2.ShareCount = 0;
    Pfn1->u3.e2.ReferenceCount = 0;

    /* Get a page for the working set list */
    MI_SET_USAGE(MI_USAGE_PAGE_TABLE);
    MI_SET_PROCESS2("Kernel WS List");
    PageFrameIndex = MiRemoveAnyPage(0);
    TempPte = ValidKernelPteLocal;
    TempPte.u.Hard.PageFrameNumber = PageFrameIndex;

    /* Map the working set list */
    PointerPte = MiAddressToPte(MmWorkingSetList);
    MI_WRITE_VALID_PTE(PointerPte, TempPte);

    /* Zero it out, and save the frame index */
    RtlZeroMemory(MiPteToAddress(PointerPte), PAGE_SIZE);
    PsGetCurrentProcess()->WorkingSetPage = PageFrameIndex;

    /* Check for Pentium LOCK errata */
    if (KiI386PentiumLockErrataPresent)
    {
        /* Mark the 1st IDT page as Write-Through to prevent a lockup
           on a F00F instruction.
           See https://www.rcollins.org/Errata/Dec97/F00FBug.html */
        PointerPte = MiAddressToPte(KeGetPcr()->IDT);
        PointerPte->u.Hard.WriteThrough = 1;
    }

    /* Release the lock */
    MiReleasePfnLock(OldIrql);

    /* Initialize the bogus address space */
    Flags = 0;
    MmInitializeProcessAddressSpace(PsGetCurrentProcess(), NULL, NULL, &Flags, NULL);

    /* Make sure the color lists are valid */
    ASSERT(MmFreePagesByColor[0] < (PMMCOLOR_TABLES)PTE_BASE);
    StartPde = MiAddressToPde(MmFreePagesByColor[0]);
    ASSERT(StartPde->u.Hard.Valid == 1);
    PointerPte = MiAddressToPte(MmFreePagesByColor[0]);
    ASSERT(PointerPte->u.Hard.Valid == 1);
    LastPte = MiAddressToPte((ULONG_PTR)&MmFreePagesByColor[1][MmSecondaryColors] - 1);
    ASSERT(LastPte->u.Hard.Valid == 1);

    /* Loop the color list PTEs */
    while (PointerPte <= LastPte)
    {
        /* Get the PFN entry */
        Pfn1 = MiGetPfnEntry(PFN_FROM_PTE(PointerPte));
        if (!Pfn1->u3.e2.ReferenceCount)
        {
            /* Fill it out */
            Pfn1->u4.PteFrame = PFN_FROM_PTE(StartPde);
            Pfn1->PteAddress = PointerPte;
            Pfn1->u2.ShareCount++;
            Pfn1->u3.e2.ReferenceCount = 1;
            Pfn1->u3.e1.PageLocation = ActiveAndValid;
            Pfn1->u3.e1.CacheAttribute = MiCached;
        }

        /* Keep going */
        PointerPte++;
    }

    /* All done */
    return STATUS_SUCCESS;
}

/* EOF */
