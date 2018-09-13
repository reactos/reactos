/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    init386.c

Abstract:

    This module contains the machine dependent initialization for the
    memory management component.  It is specifically tailored to the
    INTEL x86 and PAE machines.

Author:

    Lou Perazzoli (loup) 6-Jan-1990
    Landy Wang (landyw)  2-Jun-1997

Revision History:

--*/

#include "mi.h"

VOID
MiRemoveLowPages (
    VOID
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT,MiInitMachineDependent)
#pragma alloc_text(INIT,MiRemoveLowPages)
#endif

#define MM_BIOS_START (0xA0000 >> PAGE_SHIFT)
#define MM_BIOS_END  (0xFFFFF >> PAGE_SHIFT)

#define MM_LARGE_PAGE_MINIMUM  ((127*1024*1024) >> PAGE_SHIFT)

SIZE_T MmExpandedNonPagedPoolInBytes;

extern ULONG MmLargeSystemCache;
extern LOGICAL MmMakeLowMemory;
extern LOGICAL MmPagedPoolMaximumDesired;

#if defined(_X86PAE_)
LOGICAL MiUseGlobalBitInLargePdes;
LOGICAL MiNeedLowVirtualPfn;
LOGICAL MiNoLowMemory = FALSE;
PRTL_BITMAP MiLowMemoryBitMap;
#endif

#define MI_LOWMEM_MAGIC_BIT (0x80000000)


VOID
MiInitMachineDependent (
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
    )

/*++

Routine Description:

    This routine performs the necessary operations to enable virtual
    memory.  This includes building the page directory page, building
    page table pages to map the code section, the data section, the
    stack section and the trap handler.

    It also initializes the PFN database and populates the free list.

Arguments:

    LoaderBlock  - Supplies a pointer to the firmware setup loader block.

Return Value:

    None.

Environment:

    Kernel mode.

--*/

{
    PMMPFN BasePfn;
    PMMPFN BottomPfn;
    PMMPFN TopPfn;
    BOOLEAN PfnInKseg0;
    ULONG BasePage;
    ULONG HighPage;
    ULONG HighPageInKseg0;
    ULONG PagesLeft;
    ULONG Range;
    ULONG i, j;
    ULONG PdePageNumber;
    ULONG PdePage;
    ULONG PageFrameIndex;
    ULONG NextPhysicalPage;
    ULONG OldFreeDescriptorLowMemCount;
    ULONG OldFreeDescriptorLowMemBase;
    ULONG OldFreeDescriptorCount;
    ULONG OldFreeDescriptorBase;
    ULONG PfnAllocation;
    ULONG NumberOfPages;
    ULONG MaxPool;
    PEPROCESS CurrentProcess;
    ULONG DirBase;
    ULONG MostFreePage;
    ULONG MostFreeLowMem;
    ULONG MostFreeLowMem512;
    PLIST_ENTRY NextMd;
    PMEMORY_ALLOCATION_DESCRIPTOR FreeDescriptor;
    PMEMORY_ALLOCATION_DESCRIPTOR FreeDescriptor512;
    PMEMORY_ALLOCATION_DESCRIPTOR FreeDescriptorLowMem;
    PMEMORY_ALLOCATION_DESCRIPTOR MemoryDescriptor;
    PMEMORY_ALLOCATION_DESCRIPTOR UsableDescriptor;
    MMPTE TempPde;
    MMPTE TempPte;
    PMMPTE PointerPde;
    PMMPTE PointerPte;
    PMMPTE LastPte;
    PMMPTE Pde;
    PMMPTE StartPde;
    PMMPTE EndPde;
    PMMPFN Pfn1;
    PMMPFN Pfn2;
    ULONG PdeCount;
    ULONG va;
    ULONG SavedSize;
    KIRQL OldIrql;
    ULONG MapLargePages;
    PVOID NonPagedPoolStartVirtual;
    ULONG LargestFreePfnCount;
    ULONG LargestFreePfnStart;
    ULONG ExtraPtes;
    ULONG FreePfnCount;
    SIZE_T UsableDescriptorRemoved;
    LOGICAL SwitchedDescriptors;
    LOGICAL NeedLowVirtualPfn;
    ULONG NextUsablePhysicalPage;
    LOGICAL UsingHighMemory;
    PVOID LowVirtualNonPagedPoolStart;
    ULONG LowVirtualNonPagedPoolSizeInBytes;
    LOGICAL ExtraSystemCacheViews;

    ExtraSystemCacheViews = FALSE;
    SwitchedDescriptors = FALSE;
    PfnInKseg0 = FALSE;
    MostFreePage = 0;
    MostFreeLowMem = 0;
    MostFreeLowMem512 = 0;
    MapLargePages = 0;
    LargestFreePfnCount = 0;
    UsableDescriptor = NULL;
    UsableDescriptorRemoved = 0;

    if (InitializationPhase == 1) {

        //
        // If the kernel image has not been biased to allow for 3gb of user
        // space, the host processor supports large pages, and the number of
        // physical pages is greater than 127mb, then map the kernel image,
        // HAL, and boot drivers into a large page.
        //

        if ((MmVirtualBias == 0) &&
#if defined (_X86PAE_)
            (MiNeedLowVirtualPfn == FALSE) &&
#endif
            (KeFeatureBits & KF_LARGE_PAGE) &&
            (MmNumberOfPhysicalPages > MmLargePageMinimum)) {

            //
            // Map lower 512MB of physical memory as large pages starting
            // at address 0x80000000.
            //

            LOCK_PFN (OldIrql);

            PointerPde = MiGetPdeAddress (MM_KSEG0_BASE);
            LastPte = MiGetPdeAddress (MM_KSEG2_BASE);
            TempPte = ValidKernelPde;
            TempPte.u.Hard.PageFrameNumber = 0;
            TempPte.u.Hard.LargePage = 1;
#if defined(_X86PAE_)
            if (MiUseGlobalBitInLargePdes == TRUE) {
                TempPte.u.Hard.Global = 1;
            }
#endif

            do {
                if (PointerPde->u.Hard.Valid == 1) {
                    PageFrameIndex = MI_GET_PAGE_FRAME_FROM_PTE(PointerPde);
                    Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);
                    Pfn1->u2.ShareCount = 0;
                    Pfn1->u3.e2.ReferenceCount = 1;
                    Pfn1->u3.e1.PageLocation = StandbyPageList;
                    MI_SET_PFN_DELETED (Pfn1);
                    MiDecrementReferenceCount (PageFrameIndex);
                    KeFlushSingleTb (MiGetVirtualAddressMappedByPte (PointerPde),
                                     TRUE,
                                     TRUE,
                                     (PHARDWARE_PTE)PointerPde,
                                     TempPte.u.Flush);
                    KeFlushEntireTb (TRUE, TRUE);  //p6 errata...

                } else {
                    MI_WRITE_VALID_PTE (PointerPde, TempPte);
                }

                TempPte.u.Hard.PageFrameNumber += MM_VA_MAPPED_BY_PDE >> PAGE_SHIFT;
                PointerPde += 1;
            } while (PointerPde < LastPte);

            UNLOCK_PFN (OldIrql);

            MmKseg2Frame = (512*1024*1024) >> PAGE_SHIFT;
        }

        return;
    }

    ASSERT (InitializationPhase == 0);

    //
    // Enabling special IRQL automatically disables mapping the kernel with
    // large pages so we can catch kernel and HAL code.
    //

    if (KernelVerifier) {
        MmLargePageMinimum = (ULONG)-2;
    }
    else if (MmLargePageMinimum == 0) {
        MmLargePageMinimum = MM_LARGE_PAGE_MINIMUM;
    }

    if (MmProtectFreedNonPagedPool == TRUE) {
        MmLargePageMinimum = (ULONG)-2;
    }

    if (MmDynamicPfn == TRUE) {
        if (MmVirtualBias != 0) {
            MmDynamicPfn = FALSE;
        }
        MmLargePageMinimum = (ULONG)-2;
    }

    //
    // If the host processor supports global bits, then set the global
    // bit in the template kernel PTE and PDE entries.
    //

    if (KeFeatureBits & KF_GLOBAL_PAGE) {
        ValidKernelPte.u.Long |= MM_PTE_GLOBAL_MASK;
#if defined(_X86PAE_)
        //
        // Note that the PAE mode of the processor does not support the
        // global bit in PDEs which map 4K page table pages.
        //
        MiUseGlobalBitInLargePdes = TRUE;
#else
        ValidKernelPde.u.Long |= MM_PTE_GLOBAL_MASK;
#endif
        MmPteGlobal.u.Long = MM_PTE_GLOBAL_MASK;
    }

    TempPte = ValidKernelPte;
    TempPde = ValidKernelPde;

    //
    // Set the directory base for the system process.
    //

    PointerPte = MiGetPdeAddress (PDE_BASE);
    PdePageNumber = MI_GET_PAGE_FRAME_FROM_PTE(PointerPte);
#if defined(_X86PAE_)

    PrototypePte.u.Soft.PageFileHigh = MI_PTE_LOOKUP_NEEDED;

    PsGetCurrentProcess()->PaePageDirectoryPage = PdePageNumber;
    _asm {
        mov     eax, cr3
        mov     DirBase, eax
    }

    //
    // Note cr3 must be 32-byte aligned.
    //

    ASSERT ((DirBase & 0x1f) == 0);
#else
    DirBase = MI_GET_PAGE_FRAME_FROM_PTE(PointerPte) << PAGE_SHIFT;
#endif
    PsGetCurrentProcess()->Pcb.DirectoryTableBase[0] = DirBase;
    KeSweepDcache (FALSE);

    //
    // Unmap the low 2Gb of memory.
    //

    PointerPde = MiGetPdeAddress(0);
    LastPte = MiGetPdeAddress (KSEG0_BASE - 0x10000 - 1);
    while (PointerPde <= LastPte) {
        *PointerPde = ZeroKernelPte;
        PointerPde += 1;
    }

    //
    // Get the lower bound of the free physical memory and the
    // number of physical pages by walking the memory descriptor lists.
    //

    FreeDescriptor = NULL;
    FreeDescriptor512 = NULL;
    NextMd = LoaderBlock->MemoryDescriptorListHead.Flink;
    while (NextMd != &LoaderBlock->MemoryDescriptorListHead) {
        MemoryDescriptor = CONTAINING_RECORD(NextMd,
                                             MEMORY_ALLOCATION_DESCRIPTOR,
                                             ListEntry);

        if ((MemoryDescriptor->MemoryType != LoaderFirmwarePermanent) &&
            (MemoryDescriptor->MemoryType != LoaderBBTMemory) &&
            (MemoryDescriptor->MemoryType != LoaderSpecialMemory)) {

            //
            // This check results in /BURNMEMORY chunks not being counted.
            //

            if (MemoryDescriptor->MemoryType != LoaderBad) {
                MmNumberOfPhysicalPages += MemoryDescriptor->PageCount;
            }

            if (MemoryDescriptor->BasePage < MmLowestPhysicalPage) {
                MmLowestPhysicalPage = MemoryDescriptor->BasePage;
            }

            if ((MemoryDescriptor->BasePage + MemoryDescriptor->PageCount) >
                                                             MmHighestPhysicalPage) {
                MmHighestPhysicalPage =
                        MemoryDescriptor->BasePage + MemoryDescriptor->PageCount - 1;
            }

            //
            // Locate the largest free block and the largest free
            // block below 16mb.
            //

            if ((MemoryDescriptor->MemoryType == LoaderFree) ||
                (MemoryDescriptor->MemoryType == LoaderLoadedProgram) ||
                (MemoryDescriptor->MemoryType == LoaderFirmwareTemporary) ||
                (MemoryDescriptor->MemoryType == LoaderOsloaderStack)) {

                if (MemoryDescriptor->PageCount > MostFreePage) {
                    MostFreePage = MemoryDescriptor->PageCount;
                    FreeDescriptor = MemoryDescriptor;
                }

                if (MemoryDescriptor->BasePage < 0x20000) {

                    //
                    // This memory descriptor is below 512mb.
                    //

                    if ((MostFreeLowMem512 < MemoryDescriptor->PageCount) &&
                        (MostFreeLowMem512 < ((ULONG)0x20000 - MemoryDescriptor->BasePage))) {

                        MostFreeLowMem512 = (ULONG)0x20000 - MemoryDescriptor->BasePage;
                        if (MemoryDescriptor->PageCount < MostFreeLowMem512) {
                            MostFreeLowMem512 = MemoryDescriptor->PageCount;
                        }

                        FreeDescriptor512 = MemoryDescriptor;
                    }
                }

                if (MemoryDescriptor->BasePage < 0x1000) {

                    //
                    // This memory descriptor is below 16mb.
                    //

                    if ((MostFreeLowMem < MemoryDescriptor->PageCount) &&
                        (MostFreeLowMem < ((ULONG)0x1000 - MemoryDescriptor->BasePage))) {

                        MostFreeLowMem = (ULONG)0x1000 - MemoryDescriptor->BasePage;
                        if (MemoryDescriptor->PageCount < MostFreeLowMem) {
                            MostFreeLowMem = MemoryDescriptor->PageCount;
                        }

                        FreeDescriptorLowMem = MemoryDescriptor;
                    }
                }
            }
        }

        NextMd = MemoryDescriptor->ListEntry.Flink;
    }

    if (FreeDescriptorLowMem == FreeDescriptor) {
        FreeDescriptor = NULL;
    }

    if (MmLargeSystemCache != 0) {
        ExtraSystemCacheViews = TRUE;
    }

    NeedLowVirtualPfn = FALSE;

    if (MmDynamicPfn == TRUE) {
#if defined(_X86PAE_)
        if (ExVerifySuite(DataCenter) == TRUE) {
            MmHighestPossiblePhysicalPage = 0x1000000 - 1;
        }
        else if ((MmProductType != 0x00690057) &&
                 (ExVerifySuite(Enterprise) == TRUE)) {
            MmHighestPossiblePhysicalPage = 0x200000 - 1;
        }
        else {
            MmHighestPossiblePhysicalPage = 0x100000 - 1;
        }
#else
        MmHighestPossiblePhysicalPage = 0x100000 - 1;
#endif
        if (MmVirtualBias == 0) {
            NeedLowVirtualPfn = TRUE;
        }
    }
    else {
        MmHighestPossiblePhysicalPage = MmHighestPhysicalPage;
    }

    if (MmHighestPossiblePhysicalPage > 0x400000 - 1) {

        //
        // The PFN database is more than 112mb.  Force it to come from the
        // 2GB->3GB virtual address range.  Note the administrator cannot be
        // booting /3GB as when he does, the loader throws away memory
        // above the physical 16GB line.
        //

        ASSERT (MmVirtualBias == 0);

        //
        // The virtual space between 0xA4000000 and 0xC0000000 is best used
        // for system PTEs when this much physical memory is present.
        //

        ExtraSystemCacheViews = FALSE;
    }

    //
    // Two large descriptors have been saved.  If the one below 512mb
    // is sufficiently large, use that one as then large pages can be
    // applied to map it and the PFN database can be put in it.
    // Only do this if the PFN database size is small enough such that it
    // doesn't consume so much virtual space between 2gb and 2gb+512mb - ie:
    // there has to be enough virtual space to map initial nonpaged pool in
    // there.
    //

    if (MmHighestPossiblePhysicalPage > 0x100000) {

        //
        // The 0x700000 (== 28GB) is chosen carefully so that the start of
        // expanded nonpaged pool is always above the nonpaged system start.
        //

        if (MmHighestPossiblePhysicalPage < 0x700000) {

            if ((FreeDescriptor512 != NULL) &&
                (FreeDescriptor512 != FreeDescriptor) &&
                (FreeDescriptor512 != FreeDescriptorLowMem)) {
        
                if (MostFreeLowMem512 >= ((MmHighestPossiblePhysicalPage * sizeof (MMPFN) + MM_MAX_INITIAL_NONPAGED_POOL) / PAGE_SIZE)) {
    
                    FreeDescriptor = FreeDescriptor512;
                    MostFreePage = FreeDescriptor->PageCount;
                }
            }
    
            if (FreeDescriptor->BasePage >= 0x20000) {
                NeedLowVirtualPfn = TRUE;
            }
        }
        else {
            NeedLowVirtualPfn = TRUE;
        }
    }
    
#if defined(_X86PAE_)

    //
    // Only PAE machines with at least 5GB of physical memory get to use this
    // and then only if they are NOT booted /3GB.
    //

    if (strstr(LoaderBlock->LoadOptions, "NOLOWMEM")) {
        if ((MmVirtualBias == 0) &&
            (MmNumberOfPhysicalPages >= 5 * 1024 * 1024 / 4)) {
                MiNoLowMemory = TRUE;
                MmMakeLowMemory = TRUE;
                NeedLowVirtualPfn = TRUE;
        }
    }

    MiNeedLowVirtualPfn = NeedLowVirtualPfn;
#endif

    NextPhysicalPage = FreeDescriptorLowMem->BasePage;

    OldFreeDescriptorLowMemCount = FreeDescriptorLowMem->PageCount;
    OldFreeDescriptorLowMemBase = FreeDescriptorLowMem->BasePage;

    if (FreeDescriptor != NULL) {
        OldFreeDescriptorCount = FreeDescriptor->PageCount;
        OldFreeDescriptorBase = FreeDescriptor->BasePage;
    }

    NumberOfPages = FreeDescriptorLowMem->PageCount;
    if (MmNumberOfPhysicalPages < 1100) {
        KeBugCheckEx (INSTALL_MORE_MEMORY,
                      MmNumberOfPhysicalPages,
                      MmLowestPhysicalPage,
                      MmHighestPhysicalPage,
                      0);
    }

    //
    // Build non-paged pool using the physical pages following the
    // data page in which to build the pool from.  Non-paged pool grows
    // from the high range of the virtual address space and expands
    // downward.
    //
    // At this time non-paged pool is constructed so virtual addresses
    // are also physically contiguous.
    //

    if ((MmSizeOfNonPagedPoolInBytes >> PAGE_SHIFT) >
                        (7 * (MmNumberOfPhysicalPages >> 3))) {

        //
        // More than 7/8 of memory is allocated to nonpagedpool, reset to 0.
        //

        MmSizeOfNonPagedPoolInBytes = 0;
    }

    if (MmSizeOfNonPagedPoolInBytes < MmMinimumNonPagedPoolSize) {

        //
        // Calculate the size of nonpaged pool.
        // Use the minimum size, then for every MB above 4mb add extra
        // pages.
        //

        MmSizeOfNonPagedPoolInBytes = MmMinimumNonPagedPoolSize;

        MmSizeOfNonPagedPoolInBytes +=
                            ((MmNumberOfPhysicalPages - 1024)/256) *
                            MmMinAdditionNonPagedPoolPerMb;
    }

    if (MmSizeOfNonPagedPoolInBytes > MM_MAX_INITIAL_NONPAGED_POOL) {
        MmSizeOfNonPagedPoolInBytes = MM_MAX_INITIAL_NONPAGED_POOL;
    }

    //
    // Align to page size boundary.
    //

    MmSizeOfNonPagedPoolInBytes &= ~(PAGE_SIZE - 1);

    //
    // Calculate the maximum size of pool.
    //

    if (MmMaximumNonPagedPoolInBytes == 0) {

        //
        // Calculate the size of nonpaged pool.  If 4mb or less use
        // the minimum size, then for every MB above 4mb add extra
        // pages.
        //

        MmMaximumNonPagedPoolInBytes = MmDefaultMaximumNonPagedPool;

        //
        // Make sure enough expansion for the PFN database exists.
        //

        MmMaximumNonPagedPoolInBytes += (ULONG)PAGE_ALIGN (
                                      (MmHighestPossiblePhysicalPage + 1) * sizeof(MMPFN));

        //
        // Only use the new formula for autosizing nonpaged pool on machines
        // with at least 512MB.  The new formula allocates 1/2 as much nonpaged
        // pool per MB but scales much higher - machines with ~1.2GB or more
        // get 256MB of nonpaged pool.  Note that the old formula gave machines
        // with 512MB of RAM 128MB of nonpaged pool so this behavior is
        // preserved with the new formula as well.
        //

        if (MmNumberOfPhysicalPages >= 0x1f000) {
            MmMaximumNonPagedPoolInBytes +=
                            ((MmNumberOfPhysicalPages - 1024)/256) *
                            (MmMaxAdditionNonPagedPoolPerMb / 2);

            if (MmMaximumNonPagedPoolInBytes < MM_MAX_ADDITIONAL_NONPAGED_POOL) {
                MmMaximumNonPagedPoolInBytes = MM_MAX_ADDITIONAL_NONPAGED_POOL;
            }
        }
        else {
            MmMaximumNonPagedPoolInBytes +=
                            ((MmNumberOfPhysicalPages - 1024)/256) *
                            MmMaxAdditionNonPagedPoolPerMb;
        }
    }

    MaxPool = MmSizeOfNonPagedPoolInBytes + PAGE_SIZE * 16 +
                                   (ULONG)PAGE_ALIGN (
                                        (MmHighestPossiblePhysicalPage + 1) * sizeof(MMPFN));

    if (MmMaximumNonPagedPoolInBytes < MaxPool) {
        MmMaximumNonPagedPoolInBytes = MaxPool;
    }

    //
    // Systems that are booted /3GB have a 128MB nonpaged pool maximum,
    //
    // Systems that have a full 2GB system virtual address space, support
    // large pages, and have sufficient physical memory are allowed up to 256MB
    // of nonpaged pool.
    //

    MmExpandedNonPagedPoolInBytes = 0;

    if ((MmVirtualBias == 0) &&
        (KeFeatureBits & KF_LARGE_PAGE) &&
        (MmProtectFreedNonPagedPool == FALSE) &&
        (MmNumberOfPhysicalPages > MmLargePageMinimum)) {

        if (MmMaximumNonPagedPoolInBytes > MM_MAX_INITIAL_NONPAGED_POOL + MM_MAX_ADDITIONAL_NONPAGED_POOL) {
            MmMaximumNonPagedPoolInBytes = MM_MAX_INITIAL_NONPAGED_POOL + MM_MAX_ADDITIONAL_NONPAGED_POOL;
        }

        //
        // Initialize the expanded amount to the highest possible value as
        // there may not be enough contiguous pages in the FreeDescriptorLowMem
        // below 512MB to get the initial pool to its maximal size.
        //

        if (MmMaximumNonPagedPoolInBytes > MM_MAX_ADDITIONAL_NONPAGED_POOL) {

            MmExpandedNonPagedPoolInBytes = MM_MAX_ADDITIONAL_NONPAGED_POOL;
            MmSizeOfNonPagedPoolInBytes = MmMaximumNonPagedPoolInBytes -
                                                MmExpandedNonPagedPoolInBytes;
        }
    }
    else {
        if (MmMaximumNonPagedPoolInBytes > MM_MAX_ADDITIONAL_NONPAGED_POOL) {
            MmMaximumNonPagedPoolInBytes = MM_MAX_ADDITIONAL_NONPAGED_POOL;
        }
    }

    //
    // Get secondary color value from:
    //
    // (a) from the registry (already filled in) or
    // (b) from the PCR or
    // (c) default value.
    //

    if (MmSecondaryColors == 0) {
        MmSecondaryColors = KeGetPcr()->SecondLevelCacheSize;
    }

    MmSecondaryColors = MmSecondaryColors >> PAGE_SHIFT;

    if (MmSecondaryColors == 0) {
        MmSecondaryColors = MM_SECONDARY_COLORS_DEFAULT;

    } else {

        //
        // Make sure the value is a power of two and within limits.
        //

        if (((MmSecondaryColors & (MmSecondaryColors -1)) != 0) ||
            (MmSecondaryColors < MM_SECONDARY_COLORS_MIN) ||
            (MmSecondaryColors > MM_SECONDARY_COLORS_MAX)) {
            MmSecondaryColors = MM_SECONDARY_COLORS_DEFAULT;
        }
    }

    //
    // Add in the PFN database size (based on the number of pages required
    // from page zero to the highest page).
    //
    // Get the number of secondary colors and add the array for tracking
    // secondary colors to the end of the PFN database.
    //

    PfnAllocation = 1 + ((((MmHighestPossiblePhysicalPage + 1) * sizeof(MMPFN)) +
                        (MmSecondaryColors * sizeof(MMCOLOR_TABLES)*2))
                            >> PAGE_SHIFT);

    if (NeedLowVirtualPfn == FALSE) {
        MmMaximumNonPagedPoolInBytes += PfnAllocation << PAGE_SHIFT;
    }

    if (MmExpandedNonPagedPoolInBytes) {
        if (NeedLowVirtualPfn == FALSE) {
            MmExpandedNonPagedPoolInBytes += PfnAllocation << PAGE_SHIFT;
        }
        MmNonPagedPoolStart = (PVOID)((ULONG)MmNonPagedPoolEnd
                                          - MmExpandedNonPagedPoolInBytes);
    }
    else {
        MmNonPagedPoolStart = (PVOID)((ULONG)MmNonPagedPoolEnd
                                          - MmMaximumNonPagedPoolInBytes);
    }

    MmNonPagedPoolStart = (PVOID)PAGE_ALIGN(MmNonPagedPoolStart);

    MmPageAlignedPoolBase[NonPagedPool] = MmNonPagedPoolStart;

    //
    // Allocate additional paged pool provided it can fit and either the
    // user asked for it or we decide 460MB of PTE space is sufficient.
    //

    if ((MmVirtualBias == 0) &&
        ((MmSizeOfPagedPoolInBytes == (SIZE_T)-1) ||
         ((MmSizeOfPagedPoolInBytes == 0) &&
         (MmNumberOfPhysicalPages >= (1 * 1024 * 1024 * 1024 / PAGE_SIZE)) &&
         ((MiHydra == FALSE) || (ExpMultiUserTS == FALSE)) &&
         (MiRequestedSystemPtes != (ULONG)-1)))) {

        ExtraSystemCacheViews = FALSE;
        MmNumberOfSystemPtes = 3000;
        MmPagedPoolMaximumDesired = TRUE;

        //
        // Make sure we always allocate extra PTEs later as we have crimped
        // the initial allocation here.
        //

        if ((MiHydra == FALSE) || (ExpMultiUserTS == FALSE)) {
            if (MmNumberOfPhysicalPages <= 0x7F00) {
                MiRequestedSystemPtes = (ULONG)-1;
            }
        }
    }

    //
    // Calculate the starting PDE for the system PTE pool which is
    // right below the nonpaged pool.
    //

    MmNonPagedSystemStart = (PVOID)(((ULONG)MmNonPagedPoolStart -
                                ((MmNumberOfSystemPtes + 1) * PAGE_SIZE)) &
                                 (~PAGE_DIRECTORY_MASK));

    if (MmNonPagedSystemStart < MM_LOWEST_NONPAGED_SYSTEM_START) {
        MmNonPagedSystemStart = MM_LOWEST_NONPAGED_SYSTEM_START;
        MmNumberOfSystemPtes = (((ULONG)MmNonPagedPoolStart -
                                 (ULONG)MmNonPagedSystemStart) >> PAGE_SHIFT)-1;

        ASSERT (MmNumberOfSystemPtes > 1000);
    }

    //
    // Set up page table pages to map nonpaged pool and system PTEs.
    // If possible, use higher physical pages to preserve more low
    // memory for drivers.
    //

    StartPde = MiGetPdeAddress (MmNonPagedSystemStart);
    EndPde = MiGetPdeAddress ((PVOID)((PCHAR)MmNonPagedPoolEnd - 1));

    UsingHighMemory = FALSE;
    if (NextPhysicalPage < (FreeDescriptorLowMem->PageCount +
                            FreeDescriptorLowMem->BasePage)) {

        ULONG PagesNeeded;
    
        //
        // We haven't used the other descriptor yet, examine it now
        // to see if enough usable memory is available.
        //

        PagesNeeded = (EndPde - StartPde + 1);

        if ((FreeDescriptor) && (FreeDescriptor->PageCount >= PagesNeeded)) {

            UsableDescriptor = FreeDescriptor;
            NextUsablePhysicalPage = FreeDescriptor->BasePage;

            UsableDescriptorRemoved += PagesNeeded;

            //
            // Note this must be undone if the PFN database is created in
            // virtual memory because the memory descriptors are scanned
            // and only pages in the descriptors get mapped.
            //

            UsableDescriptor->BasePage += PagesNeeded;
            UsableDescriptor->PageCount -= PagesNeeded;
            UsingHighMemory = TRUE;
        }
    }

    while (StartPde <= EndPde) {

        ASSERT(StartPde->u.Hard.Valid == 0);

        //
        // Map in a page table page.
        //

        if (UsingHighMemory == TRUE) {
            TempPde.u.Hard.PageFrameNumber = NextUsablePhysicalPage;
            NextUsablePhysicalPage += 1;
        }
        else {
            TempPde.u.Hard.PageFrameNumber = NextPhysicalPage;
            NextPhysicalPage += 1;
            NumberOfPages -= 1;

            if (NumberOfPages == 0) {
                if (FreeDescriptor == NULL) {
                    KeBugCheckEx (INSTALL_MORE_MEMORY,
                                  MmNumberOfPhysicalPages,
                                  MmLowestPhysicalPage,
                                  MmHighestPhysicalPage,
                                  1);
                }
                ASSERT (NextPhysicalPage != (FreeDescriptor->BasePage +
                                             FreeDescriptor->PageCount));
                NextPhysicalPage = FreeDescriptor->BasePage;
                NumberOfPages = FreeDescriptor->PageCount;
                SwitchedDescriptors = TRUE;
            }
        }

        *StartPde = TempPde;
        PointerPte = MiGetVirtualAddressMappedByPte (StartPde);
        RtlZeroMemory (PointerPte, PAGE_SIZE);
        StartPde += 1;
    }

    ExtraPtes = 0;

    MiMaximumSystemCacheSizeExtra = 0;

    if (MmVirtualBias == 0) {

        if ((MiRequestedSystemPtes == (ULONG)-1) ||
            ((MiHydra == TRUE) && (ExpMultiUserTS == TRUE)) ||
            (MmSpecialPoolTag && MmNumberOfPhysicalPages > 0x7F00)) {

            ExtraPtes = BYTES_TO_PAGES(KSTACK_POOL_SIZE) - 1;
        }
        else if (ExtraSystemCacheViews == TRUE) {

            //
            // If the system is configured to favor large system caching, use
            // the remaining virtual address space for the system cache.  This
            // is possible since the 3GB user space option has not been
            // enabled, Hydra has not been chosen and extended special pool
            // is not enabled.
            //

            MiMaximumSystemCacheSizeExtra =
                (MM_SYSTEM_CACHE_END_EXTRA - MM_SYSTEM_CACHE_START_EXTRA) >> PAGE_SHIFT;
        }
        else if (MmNumberOfPhysicalPages > 0x7F00) {
            ExtraPtes = BYTES_TO_PAGES(KSTACK_POOL_SIZE) - 1;
        }

        if (ExtraPtes) {
            StartPde = MiGetPdeAddress (KSTACK_POOL_START);
            EndPde = MiGetPdeAddress ((PVOID)((PCHAR)KSTACK_POOL_START +
                            (ExtraPtes << PAGE_SHIFT) - 1));

            //
            // If possible, use higher physical pages to preserve more low
            // memory for drivers.
            //

            UsingHighMemory = FALSE;
            if (NextPhysicalPage < (FreeDescriptorLowMem->PageCount +
                                    FreeDescriptorLowMem->BasePage)) {
        
                ULONG PagesNeeded;
            
                //
                // We haven't used the other descriptor yet, examine it now
                // to see if enough usable memory is available.
                //
        
                PagesNeeded = (EndPde - StartPde + 1);
        
                if ((FreeDescriptor != NULL) && (FreeDescriptor->PageCount >= PagesNeeded)) {
        
                    UsableDescriptor = FreeDescriptor;
                    NextUsablePhysicalPage = FreeDescriptor->BasePage;
        
                    UsableDescriptorRemoved += PagesNeeded;
        
                    //
                    // Note this must be undone if the PFN database is
                    // created in virtual memory because the memory
                    // descriptors are scanned and only pages in the
                    // descriptors get mapped.
                    //
        
                    UsableDescriptor->BasePage += PagesNeeded;
                    UsableDescriptor->PageCount -= PagesNeeded;
                    UsingHighMemory = TRUE;
                }
            }

            while (StartPde <= EndPde) {

                ASSERT(StartPde->u.Hard.Valid == 0);

                //
                // Map in a page directory page.
                //

                if (UsingHighMemory == TRUE) {
                    TempPde.u.Hard.PageFrameNumber = NextUsablePhysicalPage;
                    NextUsablePhysicalPage += 1;
                }
                else {
                    TempPde.u.Hard.PageFrameNumber = NextPhysicalPage;
                    NumberOfPages -= 1;
                    NextPhysicalPage += 1;

                    if (NumberOfPages == 0) {
                        if (FreeDescriptor == NULL) {
                            KeBugCheckEx (INSTALL_MORE_MEMORY,
                                          MmNumberOfPhysicalPages,
                                          MmLowestPhysicalPage,
                                          MmHighestPhysicalPage,
                                              2);
                        }
                        ASSERT (NextPhysicalPage != (FreeDescriptor->BasePage +
                                                     FreeDescriptor->PageCount));
                        NextPhysicalPage = FreeDescriptor->BasePage;
                        NumberOfPages = FreeDescriptor->PageCount;
                        SwitchedDescriptors = TRUE;
                    }
                }

                *StartPde = TempPde;
                PointerPte = MiGetVirtualAddressMappedByPte (StartPde);
                RtlZeroMemory (PointerPte, PAGE_SIZE);
                StartPde += 1;
                MiNumberOfExtraSystemPdes += 1;
            }

            ExtraPtes = MiNumberOfExtraSystemPdes * PTE_PER_PAGE;
        }

        ASSERT (NumberOfPages > 0);

        //
        // If the kernel image has not been biased to allow for 3gb of user
        // space, the host processor supports large pages, and the number of
        // physical pages is greater than 127mb, then map the kernel image and
        // HAL into a large page.
        //

        if ((KeFeatureBits & KF_LARGE_PAGE) &&
            (NeedLowVirtualPfn == FALSE) &&
            (MmNumberOfPhysicalPages > MmLargePageMinimum)) {

            //
            // Map lower 512MB of physical memory as large pages starting
            // at address 0x80000000.
            //

            PointerPde = MiGetPdeAddress (MM_KSEG0_BASE);
            LastPte = MiGetPdeAddress (MM_KSEG2_BASE) - 1;
            if (MmHighestPhysicalPage < MM_PAGES_IN_KSEG0) {
                LastPte = MiGetPdeAddress (MM_KSEG0_BASE +
                                    (MmHighestPhysicalPage << PAGE_SHIFT));
            }

            PointerPte = MiGetPteAddress (MM_KSEG0_BASE);
            j = 0;

            do {
                PMMPTE PPte;

                Range = 0;
                if (PointerPde->u.Hard.Valid == 0) {
                    TempPde.u.Hard.PageFrameNumber = NextPhysicalPage;
                    NextPhysicalPage += 1;
                    NumberOfPages -= 1;
                    if (NumberOfPages == 0) {
                        if (FreeDescriptor == NULL) {
                            KeBugCheckEx (INSTALL_MORE_MEMORY,
                                          MmNumberOfPhysicalPages,
                                          MmLowestPhysicalPage,
                                          MmHighestPhysicalPage,
                                          3);
                        }
                        ASSERT (NextPhysicalPage != (FreeDescriptor->BasePage +
                                                     FreeDescriptor->PageCount));
                        NextPhysicalPage = FreeDescriptor->BasePage;
                        NumberOfPages = FreeDescriptor->PageCount;
                        SwitchedDescriptors = TRUE;
                    }
                    *PointerPde = TempPde;
                    Range = 1;
                }
                PPte = PointerPte;
                for (i = 0; i < PTE_PER_PAGE; i += 1) {
                    if (Range || (PPte->u.Hard.Valid == 0)) {
                        *PPte = ValidKernelPte;
                        PPte->u.Hard.PageFrameNumber = i + j;
                    }
                    PPte += 1;
                }
                PointerPde += 1;
                PointerPte += PTE_PER_PAGE;
                j += PTE_PER_PAGE;
            } while (PointerPde <= LastPte);

            MapLargePages = 1;
        }
        else if (NeedLowVirtualPfn == TRUE) {

            //
            // The PFN database is so large that it cannot be virtually
            // mapped in system PTEs and large pages cannot be used.
            // Select a low virtual address for it now and map it in.
            // 16mb is a good choice - just after the boot images.
            //

            MmPfnDatabase = (PMMPFN)(MM_KSEG0_BASE | MM_BOOT_IMAGE_SIZE);

            //
            // Ensure the maximum PFN database fits into the available virtual
            // address space.
            //

            if ((ULONG)MmPfnDatabase + (PfnAllocation << PAGE_SHIFT) > MM_KSEG2_BASE) {
                MmHighestPossiblePhysicalPage = (MM_KSEG2_BASE - (ULONG)MmPfnDatabase - (MmSecondaryColors * sizeof(MMCOLOR_TABLES)*2)) / sizeof (MMPFN) - 1;

                if (MmHighestPhysicalPage > MmHighestPossiblePhysicalPage) {
                    MmHighestPhysicalPage = MmHighestPossiblePhysicalPage;
                }
            }

            StartPde = MiGetPdeAddress (MmPfnDatabase);
            EndPde = MiGetPdeAddress ((PVOID)((PCHAR)MmPfnDatabase + (MmHighestPossiblePhysicalPage + 1) * sizeof(MMPFN) + (MmSecondaryColors * sizeof(MMCOLOR_TABLES)*2) - 1));

            UsingHighMemory = FALSE;

            if ((MmMakeLowMemory == TRUE) &&
                (NextPhysicalPage < (FreeDescriptorLowMem->PageCount +
                                    FreeDescriptorLowMem->BasePage))) {
        
                ULONG PagesNeeded;
            
                //
                // We haven't used the other descriptor yet, examine it now
                // to see if enough usable memory is available.
                //
        
                PagesNeeded = (EndPde - StartPde + 1);
        
                if ((FreeDescriptor != NULL) && (FreeDescriptor->PageCount >= PagesNeeded)) {
                    UsableDescriptor = FreeDescriptor;
                    NextUsablePhysicalPage = FreeDescriptor->BasePage;
                    UsingHighMemory = TRUE;
                }
            }

            while (StartPde <= EndPde) {
        
                if (StartPde->u.Hard.Valid == 0) {
        
                    //
                    // Map in a page directory page.
                    //
            
                    if (UsingHighMemory == TRUE) {

                        //
                        // Note is undone later as the PFN database is being
                        // created in virtual memory because the memory
                        // descriptors are scanned and only pages in the
                        // descriptors get mapped.
                        //
        
                        TempPde.u.Hard.PageFrameNumber = NextUsablePhysicalPage;
                        NextUsablePhysicalPage += 1;
                        UsableDescriptorRemoved += 1;
                        UsableDescriptor->BasePage += 1;
                        UsableDescriptor->PageCount -= 1;
                    }
                    else {
                        TempPde.u.Hard.PageFrameNumber = NextPhysicalPage;
                        NumberOfPages -= 1;
                        NextPhysicalPage += 1;

                        if (NumberOfPages == 0) {
                            if (FreeDescriptor == NULL) {
                                KeBugCheckEx (INSTALL_MORE_MEMORY,
                                              MmNumberOfPhysicalPages,
                                              MmLowestPhysicalPage,
                                              MmHighestPhysicalPage,
                                              4);
                            }
                            ASSERT (NextPhysicalPage != (FreeDescriptor->BasePage +
                                                         FreeDescriptor->PageCount));
                            NextPhysicalPage = FreeDescriptor->BasePage;
                            NumberOfPages = FreeDescriptor->PageCount;
                            SwitchedDescriptors = TRUE;
                        }
                    }
        
                    *StartPde = TempPde;
                    PointerPte = MiGetVirtualAddressMappedByPte (StartPde);
                    RtlZeroMemory (PointerPte, PAGE_SIZE);
                }
                StartPde += 1;
            }

            //
            // Leave some room for nonpaged pool expansion in order to share
            // common initialization code and to map physically contiguous
            // requests if needed.
            //

            if (MmSizeOfNonPagedPoolInBytes > MmMaximumNonPagedPoolInBytes * 2 / 3) {
                MmSizeOfNonPagedPoolInBytes = (SIZE_T)PAGE_ALIGN (MmMaximumNonPagedPoolInBytes * 2 / 3);
            }
        }
    }
    else {
        if ((PfnAllocation + 500) * PAGE_SIZE > MmMaximumNonPagedPoolInBytes - MmSizeOfNonPagedPoolInBytes) {

            //
            // Recarve portions of the initial and expansion nonpaged pools
            // so enough expansion PTEs will be available to map the PFN
            // database on large memory systems.
            //

            if ((PfnAllocation + 500) * PAGE_SIZE < MmSizeOfNonPagedPoolInBytes) {
                MmSizeOfNonPagedPoolInBytes -= ((PfnAllocation + 500) * PAGE_SIZE);
            }
        }
    }

    //
    // If a low virtual PFN database and initial nonpaged pool is desired,
    // construct the PDEs for the initial nonpaged pool now.
    //

    if (NeedLowVirtualPfn == TRUE) {

        ULONG PagesNeeded;
        
        LowVirtualNonPagedPoolStart = NULL;
        LowVirtualNonPagedPoolSizeInBytes = 0;

        PagesNeeded = 0;
        StartPde = MiGetPdeAddress ((PVOID)((PCHAR)MmPfnDatabase + (MmHighestPossiblePhysicalPage + 1) * sizeof(MMPFN) + (MmSecondaryColors * sizeof(MMCOLOR_TABLES)*2) - 1));
        StartPde += 1;
        EndPde = MiGetPdeAddress (MM_KSEG2_BASE) - 1;

        while (StartPde <= EndPde) {
            if (StartPde->u.Hard.Valid == 0) {
                if (LowVirtualNonPagedPoolStart == NULL) {
                    LowVirtualNonPagedPoolStart = MiGetVirtualAddressMappedByPde (StartPde);
                }
                PagesNeeded += 1;
            }
            else {
                if (LowVirtualNonPagedPoolStart != NULL) {
                    LowVirtualNonPagedPoolSizeInBytes = PagesNeeded * MM_VA_MAPPED_BY_PDE;
                }
            }

            StartPde += 1;
        }

        if (LowVirtualNonPagedPoolStart == NULL) {
            StartPde = MiGetPdeAddress ((PVOID)((PCHAR)MmPfnDatabase + (MmHighestPossiblePhysicalPage + 1) * sizeof(MMPFN) + (MmSecondaryColors * sizeof(MMCOLOR_TABLES)*2) - 1));
            KeBugCheckEx (MEMORY_MANAGEMENT,
                          0x7000,
                          (ULONG_PTR)StartPde,
                          (ULONG_PTR)EndPde,
                          PagesNeeded);
        }

        if (LowVirtualNonPagedPoolSizeInBytes == 0) {
            LowVirtualNonPagedPoolSizeInBytes = PagesNeeded * MM_VA_MAPPED_BY_PDE;
        }

        if (LowVirtualNonPagedPoolSizeInBytes < MmSizeOfNonPagedPoolInBytes) {
            MmSizeOfNonPagedPoolInBytes = LowVirtualNonPagedPoolSizeInBytes;
        }

        UsingHighMemory = FALSE;
        if ((MmMakeLowMemory == TRUE) &&
            (NextPhysicalPage < (FreeDescriptorLowMem->PageCount +
                                FreeDescriptorLowMem->BasePage)) &&
            (FreeDescriptor != NULL)) {
    
            //
            // We haven't used the other descriptor yet, examine it now
            // to see if enough usable memory is available.
            //
    
            if (FreeDescriptor->PageCount >= PagesNeeded) {
    
                UsableDescriptor = FreeDescriptor;
                NextUsablePhysicalPage = FreeDescriptor->BasePage;
    
                UsableDescriptorRemoved += PagesNeeded;
    
                //
                // Note this must be undone if the PFN database is created in
                // virtual memory because the memory descriptors are scanned
                // and only pages in the descriptors get mapped.
                //
    
                UsableDescriptor->BasePage += PagesNeeded;
                UsableDescriptor->PageCount -= PagesNeeded;
                UsingHighMemory = TRUE;
            }
        }
    
        StartPde = MiGetPdeAddress ((PVOID)((PCHAR)MmPfnDatabase + (MmHighestPossiblePhysicalPage + 1) * sizeof(MMPFN) + (MmSecondaryColors * sizeof(MMCOLOR_TABLES)*2) - 1));
        StartPde += 1;

        while (StartPde <= EndPde) {
            if (StartPde->u.Hard.Valid == 1) {
                StartPde += 1;
                continue;
            }
    
            if (UsingHighMemory == TRUE) {
                TempPde.u.Hard.PageFrameNumber = NextUsablePhysicalPage;
                NextUsablePhysicalPage += 1;
            }
            else {
                TempPde.u.Hard.PageFrameNumber = NextPhysicalPage;
                NextPhysicalPage += 1;
                NumberOfPages -= 1;
                if (NumberOfPages == 0) {
                    if (FreeDescriptor == NULL) {
                        KeBugCheckEx (INSTALL_MORE_MEMORY,
                                      MmNumberOfPhysicalPages,
                                      MmLowestPhysicalPage,
                                      MmHighestPhysicalPage,
                                      7);
                    }
                    ASSERT (NextPhysicalPage != (FreeDescriptor->BasePage +
                                                 FreeDescriptor->PageCount));
                    NextPhysicalPage = FreeDescriptor->BasePage;
                    NumberOfPages = FreeDescriptor->PageCount;
                    SwitchedDescriptors = TRUE;
                }
            }
            *StartPde = TempPde;
            PointerPte = MiGetVirtualAddressMappedByPte (StartPde);
            RtlZeroMemory (PointerPte, PAGE_SIZE);
            StartPde += 1;
        }
    }

    PointerPte = MiGetPteAddress(MmNonPagedPoolStart);
    NonPagedPoolStartVirtual = MmNonPagedPoolStart;

    //
    // Fill in the PTEs for the initial nonpaged pool using the
    // largest free chunk of memory below 16mb.
    //

    SavedSize = MmSizeOfNonPagedPoolInBytes;

    if (((MmProtectFreedNonPagedPool == FALSE) && (MapLargePages)) ||
        (NeedLowVirtualPfn == TRUE)) {

        ULONG NonPagedVirtualStartPfn;

        NonPagedVirtualStartPfn = 0;

        if (MmExpandedNonPagedPoolInBytes == 0) {

            UsingHighMemory = FALSE;
            if ((MmMakeLowMemory == TRUE) &&
                (NextPhysicalPage < (FreeDescriptorLowMem->PageCount +
                                    FreeDescriptorLowMem->BasePage)) &&
                (FreeDescriptor != NULL) &&
                ((FreeDescriptor->BasePage < MM_PAGES_IN_KSEG0) ||
                 (NeedLowVirtualPfn == TRUE))) {
        
                ULONG NumberOfUsablePages;
                ULONG PagesNeeded;
            
                PagesNeeded = MmSizeOfNonPagedPoolInBytes >> PAGE_SHIFT;
                MmSizeOfNonPagedPoolInBytes = PagesNeeded << PAGE_SHIFT;
        
                //
                // We haven't used the other descriptor yet, examine it now
                // to see if enough usable memory is available.
                //
        
                NumberOfUsablePages = FreeDescriptor->PageCount;

                if (NumberOfUsablePages > (MmSizeOfNonPagedPoolInBytes >> PAGE_SHIFT)) {
                    NumberOfUsablePages = MmSizeOfNonPagedPoolInBytes >> PAGE_SHIFT;
                }

                if (NeedLowVirtualPfn == FALSE) {
                    if (FreeDescriptor->BasePage + NumberOfUsablePages > MM_PAGES_IN_KSEG0) {
                        NumberOfUsablePages = MM_PAGES_IN_KSEG0 - FreeDescriptor->BasePage;
                    }
                }

                if (NumberOfUsablePages >= PagesNeeded) {
        
                    UsableDescriptor = FreeDescriptor;
                    NextUsablePhysicalPage = FreeDescriptor->BasePage;
        
                    UsableDescriptorRemoved += PagesNeeded;
        
                    //
                    // Note this must be undone if the PFN database is created
                    // in virtual memory because the memory descriptors are
                    // scanned and only pages in the descriptors get mapped.
                    //
        
                    UsableDescriptor->BasePage += PagesNeeded;
                    UsableDescriptor->PageCount -= PagesNeeded;
                    UsingHighMemory = TRUE;
                }
            }

            if (UsingHighMemory == FALSE) {
                if (MmSizeOfNonPagedPoolInBytes > (NumberOfPages << (PAGE_SHIFT))) {
                    MmSizeOfNonPagedPoolInBytes = NumberOfPages << PAGE_SHIFT;
                }
            }

            NonPagedPoolStartVirtual = (PVOID)((PCHAR)NonPagedPoolStartVirtual +
                                        MmSizeOfNonPagedPoolInBytes);

            //
            // No need to get page table pages for these as we can reference
            // them via large pages.  If we are not using large pages (ie:
            // for NeedLowVirtualPfn), the page tables are already allocated
            // and just the mappings need to be filled in.
            //

            if (UsingHighMemory == TRUE) {
                if (NeedLowVirtualPfn == TRUE) {
                    MmNonPagedPoolStart = LowVirtualNonPagedPoolStart;
                }
                else {
                    MmNonPagedPoolStart =
                        (PVOID)(MM_KSEG0_BASE | (NextUsablePhysicalPage << PAGE_SHIFT));
                }
                NonPagedVirtualStartPfn = NextUsablePhysicalPage;
            }
            else {
                if (NeedLowVirtualPfn == TRUE) {
                    MmNonPagedPoolStart = LowVirtualNonPagedPoolStart;
                }
                else {
                    MmNonPagedPoolStart =
                        (PVOID)(MM_KSEG0_BASE | (NextPhysicalPage << PAGE_SHIFT));
                }
    
                NonPagedVirtualStartPfn = NextPhysicalPage;
                NextPhysicalPage += MmSizeOfNonPagedPoolInBytes >> PAGE_SHIFT;
                NumberOfPages -= MmSizeOfNonPagedPoolInBytes >> PAGE_SHIFT;

                if (NumberOfPages == 0) {
                    if (FreeDescriptor == NULL) {
                        KeBugCheckEx (INSTALL_MORE_MEMORY,
                                      MmNumberOfPhysicalPages,
                                      MmLowestPhysicalPage,
                                      MmHighestPhysicalPage,
                                      5);
                    }
                    ASSERT (NextPhysicalPage != (FreeDescriptor->BasePage +
                                                 FreeDescriptor->PageCount));
                    NextPhysicalPage = FreeDescriptor->BasePage;
                    NumberOfPages = FreeDescriptor->PageCount;
                    SwitchedDescriptors = TRUE;
                }
            }

            MmSubsectionBase = (ULONG)MmNonPagedPoolStart;
            MmSubsectionTopPage = NonPagedVirtualStartPfn + (MmSizeOfNonPagedPoolInBytes >> PAGE_SHIFT);

            //
            // If the entire initial nonpaged pool is below 128mb, then
            // widen the subsection range so large pages can be used for any
            // nonpaged expansion pool single-page allocation below 128mb.
            //

            if (MmSubsectionTopPage < (MM_SUBSECTION_MAP >> PAGE_SHIFT)) {
                MmSubsectionBase = MM_KSEG0_BASE;
                MmSubsectionTopPage = MM_SUBSECTION_MAP >> PAGE_SHIFT;
            }
        }
        else {

            ULONG NumberOfUsablePages;
            PMEMORY_ALLOCATION_DESCRIPTOR UseFreeDescriptor;

            NumberOfUsablePages = 0;
            UseFreeDescriptor = NULL;

            if (NextPhysicalPage < (FreeDescriptorLowMem->PageCount +
                                    FreeDescriptorLowMem->BasePage)) {

                //
                // We haven't used the other descriptor yet, examine it now
                // to see if more usable (ie: below 512mb) memory is available
                // in it than the lowmem descriptor for building the initial
                // nonpaged pool.  The 512mb restriction is because we will be
                // using large pages to map it.
                //

                if (FreeDescriptor != NULL) {
                    if (NeedLowVirtualPfn == TRUE) {
                        NumberOfUsablePages = FreeDescriptor->PageCount;
    
                        if (NumberOfUsablePages > (MmSizeOfNonPagedPoolInBytes >> PAGE_SHIFT)) {
                            NumberOfUsablePages = MmSizeOfNonPagedPoolInBytes >> PAGE_SHIFT;
                        }
    
                        ASSERT (NumberOfUsablePages <= (MM_MAX_INITIAL_NONPAGED_POOL >> PAGE_SHIFT));
    
                    }
                    else if (FreeDescriptor->BasePage < MM_PAGES_IN_KSEG0) {

                        NumberOfUsablePages = FreeDescriptor->PageCount;
    
                        if (NumberOfUsablePages > (MmSizeOfNonPagedPoolInBytes >> PAGE_SHIFT)) {
                            NumberOfUsablePages = MmSizeOfNonPagedPoolInBytes >> PAGE_SHIFT;
                        }
    
                        ASSERT (NumberOfUsablePages <= (MM_MAX_INITIAL_NONPAGED_POOL >> PAGE_SHIFT));
    
                        if (FreeDescriptor->BasePage + NumberOfUsablePages > MM_PAGES_IN_KSEG0) {
                            NumberOfUsablePages = MM_PAGES_IN_KSEG0 - FreeDescriptor->BasePage;
                        }
                    }
                }

                //
                // If the free descriptor memory meets our needs, switch to it
                // just for the nonpaged pool creation.  Otherwise, just use
                // the low descriptor memory.
                //

                if (NumberOfUsablePages > NumberOfPages) {
                    UseFreeDescriptor = FreeDescriptor;
                    NextUsablePhysicalPage = FreeDescriptor->BasePage;
                }
            }

            if (UseFreeDescriptor == NULL) {
                NumberOfUsablePages = NumberOfPages;
                NextUsablePhysicalPage = NextPhysicalPage;
            }
            else {
                if (UsableDescriptor != NULL) {
                    ASSERT (UsableDescriptor == UseFreeDescriptor);
                }
                UsableDescriptor = UseFreeDescriptor;
            }

            if (MmSizeOfNonPagedPoolInBytes > (NumberOfUsablePages << (PAGE_SHIFT))) {
                MmSizeOfNonPagedPoolInBytes = NumberOfUsablePages << PAGE_SHIFT;
            }

            MmMaximumNonPagedPoolInBytes = MmSizeOfNonPagedPoolInBytes +
                                                MmExpandedNonPagedPoolInBytes;

            //
            // No need to get page table pages for these as we can reference
            // them via large pages.
            //

            if (NeedLowVirtualPfn == TRUE) {
                MmNonPagedPoolStart = LowVirtualNonPagedPoolStart;
            }
            else {
                MmNonPagedPoolStart =
                    (PVOID)(MM_KSEG0_BASE | (NextUsablePhysicalPage << PAGE_SHIFT));
            }

            NonPagedVirtualStartPfn = NextUsablePhysicalPage;

            if (UseFreeDescriptor != NULL) {

                UsableDescriptorRemoved += MmSizeOfNonPagedPoolInBytes >> PAGE_SHIFT;

                //
                // Note this must be undone if the PFN database is created in
                // virtual memory because the memory descriptors are scanned
                // and only pages in the descriptors get mapped.
                //

                UsableDescriptor->BasePage += (MmSizeOfNonPagedPoolInBytes >> PAGE_SHIFT);
                UsableDescriptor->PageCount -= (MmSizeOfNonPagedPoolInBytes >> PAGE_SHIFT);

                if (UsableDescriptor->BasePage <= (MM_SUBSECTION_MAP >> PAGE_SHIFT)) {
                    MmSubsectionBase = MM_KSEG0_BASE;
                    MmSubsectionTopPage = MM_SUBSECTION_MAP >> PAGE_SHIFT;
                }
                else {
                    MmSubsectionBase = (ULONG)MmNonPagedPoolStart;
                    MmSubsectionTopPage = UsableDescriptor->BasePage;
                }
            }
            else {
                NextPhysicalPage += MmSizeOfNonPagedPoolInBytes >> PAGE_SHIFT;
                NumberOfPages -= MmSizeOfNonPagedPoolInBytes >> PAGE_SHIFT;

                MmSubsectionBase = (ULONG)MmNonPagedPoolStart;
                MmSubsectionTopPage = NextPhysicalPage;

                if (NextPhysicalPage < (MM_SUBSECTION_MAP >> PAGE_SHIFT)) {
                    MmSubsectionBase = MM_KSEG0_BASE;
                    MmSubsectionTopPage = MM_SUBSECTION_MAP >> PAGE_SHIFT;
                }

                if (NumberOfPages == 0) {
                    if (FreeDescriptor == NULL) {
                        KeBugCheckEx (INSTALL_MORE_MEMORY,
                                      MmNumberOfPhysicalPages,
                                      MmLowestPhysicalPage,
                                      MmHighestPhysicalPage,
                                      6);
                    }
                    ASSERT (NextPhysicalPage != (FreeDescriptor->BasePage +
                                                 FreeDescriptor->PageCount));
                    NextPhysicalPage = FreeDescriptor->BasePage;
                    NumberOfPages = FreeDescriptor->PageCount;
                    SwitchedDescriptors = TRUE;
                }
            }
        }

        if (NeedLowVirtualPfn == TRUE) {

            ASSERT (NonPagedVirtualStartPfn != 0);
            PageFrameIndex = NonPagedVirtualStartPfn;

            PointerPte = MiGetPteAddress (MmNonPagedPoolStart);
            LastPte = MiGetPteAddress((ULONG)MmNonPagedPoolStart +
                                                MmSizeOfNonPagedPoolInBytes - 1);
            while (PointerPte <= LastPte) {
                ASSERT (PointerPte->u.Hard.Valid == 0);
                TempPte.u.Hard.PageFrameNumber = PageFrameIndex;
                *PointerPte = TempPte;
                PointerPte += 1;
                PageFrameIndex += 1;
            }
        }

        if (MmExpandedNonPagedPoolInBytes == 0) {
            MmNonPagedPoolExpansionStart = (PVOID)((PCHAR)NonPagedPoolStartVirtual +
                        (SavedSize - MmSizeOfNonPagedPoolInBytes));
        }
        else {
            MmNonPagedPoolExpansionStart = NonPagedPoolStartVirtual;
        }

#if defined(_X86PAE_)

        //
        // Always widen the subsection range for PAE so large pages can be used
        // for all nonpaged single-page expansion allocations below 512mb as
        // the widened PTE can always encode it properly.
        //

        MmSubsectionBase = MM_KSEG0_BASE;
        MmSubsectionTopPage = (512*1024*1024) >> PAGE_SHIFT;
#endif

    }
    else {

        ASSERT (MmExpandedNonPagedPoolInBytes == 0);
        LastPte = MiGetPteAddress((ULONG)MmNonPagedPoolStart +
                                            MmSizeOfNonPagedPoolInBytes - 1);

        UsingHighMemory = FALSE;
        if ((MmMakeLowMemory == TRUE) &&
            (NextPhysicalPage < (FreeDescriptorLowMem->PageCount +
                                FreeDescriptorLowMem->BasePage)) &&
            (FreeDescriptor != NULL)) {
    
            ULONG PagesNeeded;
        
            //
            // We haven't used the other descriptor yet, examine it now
            // to see if enough usable memory is available.
            //
    
            PagesNeeded = (LastPte - PointerPte + 1);
    
            if (FreeDescriptor->PageCount >= PagesNeeded) {
    
                UsableDescriptor = FreeDescriptor;
                NextUsablePhysicalPage = FreeDescriptor->BasePage;
    
                UsableDescriptorRemoved += PagesNeeded;
    
                //
                // Note this must be undone if the PFN database is created in
                // virtual memory because the memory descriptors are scanned
                // and only pages in the descriptors get mapped.
                //
    
                UsableDescriptor->BasePage += PagesNeeded;
                UsableDescriptor->PageCount -= PagesNeeded;
                UsingHighMemory = TRUE;
            }
        }

        while (PointerPte <= LastPte) {

            if (UsingHighMemory == TRUE) {
                TempPte.u.Hard.PageFrameNumber = NextUsablePhysicalPage;
                NextUsablePhysicalPage += 1;
            }
            else {
                TempPte.u.Hard.PageFrameNumber = NextPhysicalPage;
                NextPhysicalPage += 1;
                NumberOfPages -= 1;
                if (NumberOfPages == 0) {
                    if (FreeDescriptor == NULL) {
                        KeBugCheckEx (INSTALL_MORE_MEMORY,
                                      MmNumberOfPhysicalPages,
                                      MmLowestPhysicalPage,
                                      MmHighestPhysicalPage,
                                      9);
                    }
                    ASSERT (NextPhysicalPage != (FreeDescriptor->BasePage +
                                                 FreeDescriptor->PageCount));
                    NextPhysicalPage = FreeDescriptor->BasePage;
                    NumberOfPages = FreeDescriptor->PageCount;
                    SwitchedDescriptors = TRUE;
                    MmSizeOfNonPagedPoolInBytes = (PointerPte - MiGetPteAddress(MmNonPagedPoolStart)) << PAGE_SHIFT;
                    break;
                }
            }
            *PointerPte = TempPte;
            PointerPte += 1;
        }

        SavedSize = MmSizeOfNonPagedPoolInBytes;

        MmNonPagedPoolExpansionStart = (PVOID)((PCHAR)NonPagedPoolStartVirtual +
                    MmSizeOfNonPagedPoolInBytes);
    }

    //
    // There must be at least one page of system PTEs before the expanded
    // nonpaged pool.
    //

    ASSERT (MiGetPteAddress(MmNonPagedSystemStart) < MiGetPteAddress(MmNonPagedPoolExpansionStart));

    //
    // Non-paged pages now exist, build the pool structures.
    //

    MmPageAlignedPoolBase[NonPagedPool] = MmNonPagedPoolStart;

    if (MmExpandedNonPagedPoolInBytes == 0) {
        MmMaximumNonPagedPoolInBytes -= (SavedSize - MmSizeOfNonPagedPoolInBytes);
        MiInitializeNonPagedPool ();
        MmMaximumNonPagedPoolInBytes += (SavedSize - MmSizeOfNonPagedPoolInBytes);
    }
    else {
        MiInitializeNonPagedPool ();
    }

    //
    // Before Non-paged pool can be used, the PFN database must
    // be built.  This is due to the fact that the start and end of
    // allocation bits for nonpaged pool are maintained in the
    // PFN elements for the corresponding pages.
    //

    MmSecondaryColorMask = MmSecondaryColors - 1;

    //
    // The SwitchedDescriptors must be checked because if it is set, the
    // FreeDescriptor has already been allocated from without updating the
    // BasePage & PageCount (these were not changed because they are scanned
    // if the PFN database is virtually mapped to insert PTEs).
    //

    if ((SwitchedDescriptors == FALSE) && (FreeDescriptor != NULL)) {
        BasePage = FreeDescriptor->BasePage;
        HighPage = FreeDescriptor->BasePage + FreeDescriptor->PageCount;
    }
    else {
        BasePage = NextPhysicalPage;
        HighPage = NextPhysicalPage + NumberOfPages;
    }

    HighPageInKseg0 = HighPage;
    if (HighPageInKseg0 > MM_PAGES_IN_KSEG0) {
        HighPageInKseg0 = MM_PAGES_IN_KSEG0;
    }

    PagesLeft = HighPage - BasePage;

#if DBG
    if (FreeDescriptor == NULL) {
        ASSERT (MapLargePages == 0);
    }
#endif

#ifndef PFN_CONSISTENCY
    if ((MapLargePages) && (BasePage + PfnAllocation <= HighPageInKseg0)) {

        //
        // Allocate the PFN database in kseg0.
        //
        // Compute the address of the PFN by allocating the appropriate
        // number of pages from the end of the free descriptor.
        //

        PfnInKseg0 = TRUE;
        MmPfnDatabase = (PMMPFN)(MM_KSEG0_BASE | (BasePage << PAGE_SHIFT));

        //
        // Later we will walk the memory descriptors and add pages to the free
        // list in the PFN database.
        //
        // To do this correctly:
        //
        // The FreeDescriptor fields must be updated if we haven't already
        // switched descriptors so the PFN database consumption isn't
        // added to the freelist.
        //
        // If we have switched, then we must update NextPhysicalPage and the
        // FreeDescriptor need not be updated.
        //

        if (SwitchedDescriptors == TRUE) {
            ASSERT (NumberOfPages > PfnAllocation);
            NextPhysicalPage += PfnAllocation;
            NumberOfPages -= PfnAllocation;
        }
        else {
            FreeDescriptor->BasePage += PfnAllocation;
            FreeDescriptor->PageCount -= PfnAllocation;
        }

        RtlZeroMemory(MmPfnDatabase, PfnAllocation * PAGE_SIZE);

        //
        // The PFN database was allocated in kseg0.  Since space was left for
        // it virtually (in the nonpaged pool expansion PTEs), remove this
        // now unused space if it can cause PTE encoding to exceed the 27 bits.
        //

        if (MmTotalFreeSystemPtes[NonPagedPoolExpansion] >
                        (MM_MAX_ADDITIONAL_NONPAGED_POOL >> PAGE_SHIFT)) {
            //
            // Reserve the expanded pool PTEs so they cannot be used.
            //

            ULONG PfnDatabaseSpace;

            PfnDatabaseSpace = MmTotalFreeSystemPtes[NonPagedPoolExpansion] -
                        (MM_MAX_ADDITIONAL_NONPAGED_POOL >> PAGE_SHIFT);

            MiReserveSystemPtes (
                    PfnDatabaseSpace,
                    NonPagedPoolExpansion,
                    0,
                    0,
                    TRUE);

            //
            // Adjust the end of nonpaged pool to reflect this reservation.
            // This is so the entire nonpaged pool expansion space is available
            // not just for general purpose consumption, but also for subsection
            // encoding into protoptes when subsections are allocated from the
            // very end of the expansion range.
            //

            (PCHAR)MmNonPagedPoolEnd -= PfnDatabaseSpace * PAGE_SIZE;
        }
        else {

            //
            // Allocate one more PTE just below the PFN database.  This provides
            // protection against the caller of the first real nonpaged
            // expansion allocation in case he accidentally overruns his pool
            // block.  (We'll trap instead of corrupting the PFN database).
            // This also allows us to freely increment in MiFreePoolPages
            // without having to worry about a valid PTE just after the end of
            // the highest nonpaged pool allocation.
            //

            MiReserveSystemPtes (
                    1,
                    NonPagedPoolExpansion,
                    0,
                    0,
                    TRUE);
        }

    } else {

        ULONG FreeNextPhysicalPage;
        ULONG FreeNumberOfPages;

        //
        // The PFN database will be built from the FreeDescriptor.  This is
        // done to avoid using pages below 16mb to contain the PFN database
        // as these pages may be needed by drivers.  The FreeDescriptor will
        // always have enough pages to build the PFN database from.
        //

        FreeNextPhysicalPage = BasePage;
        FreeNumberOfPages = PagesLeft;

#endif  // PFN_CONSISTENCY

        //
        // Calculate the start of the PFN database (it starts at physical
        // page zero, even if the lowest physical page is not zero).
        //

        if (NeedLowVirtualPfn == TRUE) {
            ASSERT (MmPfnDatabase != NULL);
            PointerPte = MiGetPteAddress (MmPfnDatabase);
        }
        else {
            ASSERT (PagesLeft >= PfnAllocation);

            PointerPte = MiReserveSystemPtes (PfnAllocation,
                                              NonPagedPoolExpansion,
                                              0,
                                              0,
                                              TRUE);

            MmPfnDatabase = (PMMPFN)(MiGetVirtualAddressMappedByPte (PointerPte));

            //
            // Adjust the end of nonpaged pool to reflect the PFN database
            // allocation.  This is so the entire nonpaged pool expansion space
            // is available not just for general purpose consumption, but also
            // for subsection encoding into protoptes when subsections are
            // allocated from the very beginning of the initial nonpaged pool
            // range.
            //

            MmNonPagedPoolEnd = (PVOID)MmPfnDatabase;

            //
            // Allocate one more PTE just below the PFN database.  This provides
            // protection against the caller of the first real nonpaged
            // expansion allocation in case he accidentally overruns his pool
            // block.  (We'll trap instead of corrupting the PFN database).
            // This also allows us to freely increment in MiFreePoolPages
            // without having to worry about a valid PTE just after the end of
            // the highest nonpaged pool allocation.
            //
    
            MiReserveSystemPtes (1,
                                 NonPagedPoolExpansion,
                                 0,
                                 0,
                                 TRUE);
        }

#if PFN_CONSISTENCY
        MiPfnStartPte = PointerPte;
        MiPfnPtes = PfnAllocation;
#endif

        //
        // Go through the memory descriptors and for each physical page make
        // sure the PFN database has a valid PTE to map it. This allows machines
        // with sparse physical memory to have a minimal PFN database.
        //

        NextMd = LoaderBlock->MemoryDescriptorListHead.Flink;
        while (NextMd != &LoaderBlock->MemoryDescriptorListHead) {
            MemoryDescriptor = CONTAINING_RECORD(NextMd,
                                                 MEMORY_ALLOCATION_DESCRIPTOR,
                                                 ListEntry);

            if ((MemoryDescriptor->MemoryType == LoaderFirmwarePermanent) ||
                (MemoryDescriptor->MemoryType == LoaderBBTMemory) ||
                (MemoryDescriptor->MemoryType == LoaderSpecialMemory)) {

                //
                // If the descriptor lies within the highest PFN database entry
                // then create PFN pages for this range.  Note the PFN entries
                // must be created to support \Device\PhysicalMemory.
                //

                if (MemoryDescriptor->BasePage <= MmHighestPhysicalPage) {
                    if (MemoryDescriptor->BasePage + MemoryDescriptor->PageCount > MmHighestPhysicalPage + 1) {
                        MemoryDescriptor->PageCount = MmHighestPhysicalPage - MemoryDescriptor->BasePage + 1;
                    }
                }
                else {
                    NextMd = MemoryDescriptor->ListEntry.Flink;
                    continue;
                }

            }

            PointerPte = MiGetPteAddress (MI_PFN_ELEMENT(
                                            MemoryDescriptor->BasePage));

            LastPte = MiGetPteAddress (((PCHAR)(MI_PFN_ELEMENT(
                                            MemoryDescriptor->BasePage +
                                            MemoryDescriptor->PageCount))) - 1);

            if (MemoryDescriptor == UsableDescriptor) {

                //
                // Temporarily add back in the memory used to create the initial
                // nonpaged pool so the PFN entries for it will be mapped.
                //
                // This must be done carefully as memory from the descriptor
                // itself could get used to map the PFNs for the descriptor !
                //

                PointerPte = MiGetPteAddress (MI_PFN_ELEMENT(
                    MemoryDescriptor->BasePage - UsableDescriptorRemoved));
            }

            while (PointerPte <= LastPte) {
                if (PointerPte->u.Hard.Valid == 0) {
                    TempPte.u.Hard.PageFrameNumber = FreeNextPhysicalPage;
                    ASSERT (FreeNumberOfPages != 0);
                    FreeNextPhysicalPage += 1;
                    FreeNumberOfPages -= 1;
                    *PointerPte = TempPte;
                    RtlZeroMemory (MiGetVirtualAddressMappedByPte (PointerPte),
                                   PAGE_SIZE);
                }
                PointerPte += 1;
            }

            NextMd = MemoryDescriptor->ListEntry.Flink;
        }

        //
        // Handle the BIOS range here as some machines have big gaps in
        // their physical memory maps.  Big meaning > 3.5mb from page 0x37
        // up to page 0x350.
        //

        PointerPte = MiGetPteAddress (MI_PFN_ELEMENT(MM_BIOS_START));
        LastPte = MiGetPteAddress ((PCHAR)(MI_PFN_ELEMENT(MM_BIOS_END)));

        while (PointerPte <= LastPte) {
            if (PointerPte->u.Hard.Valid == 0) {
                TempPte.u.Hard.PageFrameNumber = FreeNextPhysicalPage;
                ASSERT (FreeNumberOfPages != 0);
                FreeNextPhysicalPage += 1;
                FreeNumberOfPages -= 1;
                *PointerPte = TempPte;
                RtlZeroMemory (MiGetVirtualAddressMappedByPte (PointerPte),
                               PAGE_SIZE);
            }
            PointerPte += 1;
        }

        //
        // Update the global counts - this would have been tricky to do while
        // removing pages from them as we looped above.
        //

        //
        // Later we will walk the memory descriptors and add pages to the free
        // list in the PFN database.
        //
        // To do this correctly:
        //
        // The FreeDescriptor fields must be updated if we haven't already
        // switched descriptors so the PFN database consumption isn't
        // added to the freelist.
        //
        // If we have switched, then we must update NextPhysicalPage and the
        // FreeDescriptor need not be updated.
        //

        if (SwitchedDescriptors == TRUE) {
            NextPhysicalPage = FreeNextPhysicalPage;
            NumberOfPages = FreeNumberOfPages;
        }
        else if (FreeDescriptor != NULL) {
            FreeDescriptor->BasePage = FreeNextPhysicalPage;
            FreeDescriptor->PageCount = FreeNumberOfPages;
        }
        else {
            FreeDescriptorLowMem->BasePage = FreeNextPhysicalPage;
            FreeDescriptorLowMem->PageCount = FreeNumberOfPages;
            NextPhysicalPage = FreeNextPhysicalPage;
            NumberOfPages = FreeNumberOfPages;
        }

#ifndef PFN_CONSISTENCY
    }
#endif // PFN_CONSISTENCY

    if (NeedLowVirtualPfn == FALSE) {
        MmAllocatedNonPagedPool += PfnAllocation;
    }

    //
    // Initialize support for colored pages.
    //

    MmFreePagesByColor[0] = (PMMCOLOR_TABLES)
                                &MmPfnDatabase[MmHighestPossiblePhysicalPage + 1];

    MmFreePagesByColor[1] = &MmFreePagesByColor[0][MmSecondaryColors];

    //
    // Make sure the PTEs are mapped.
    //

    if ((MmFreePagesByColor[0] > (PMMCOLOR_TABLES)MM_KSEG2_BASE) ||
        (NeedLowVirtualPfn == TRUE)) {

        PointerPte = MiGetPteAddress (&MmFreePagesByColor[0][0]);

        LastPte = MiGetPteAddress (
                  (PVOID)((PCHAR)&MmFreePagesByColor[1][MmSecondaryColors] - 1));

        while (PointerPte <= LastPte) {
            if (PointerPte->u.Hard.Valid == 0) {
                TempPte.u.Hard.PageFrameNumber = NextPhysicalPage;
                NextPhysicalPage += 1;
                NumberOfPages -= 1;
                if (NumberOfPages == 0) {
                    if (FreeDescriptor == NULL) {
                        KeBugCheckEx (INSTALL_MORE_MEMORY,
                                      MmNumberOfPhysicalPages,
                                      MmLowestPhysicalPage,
                                      MmHighestPhysicalPage,
                                      8);
                    }
                    ASSERT (NextPhysicalPage != (FreeDescriptor->BasePage +
                                                 FreeDescriptor->PageCount));
                    NextPhysicalPage = FreeDescriptor->BasePage;
                    NumberOfPages = FreeDescriptor->PageCount;
                    SwitchedDescriptors = TRUE;
                }

                *PointerPte = TempPte;
                RtlZeroMemory (MiGetVirtualAddressMappedByPte (PointerPte),
                               PAGE_SIZE);
            }

            PointerPte += 1;
        }
    }

    for (i = 0; i < MmSecondaryColors; i += 1) {
        MmFreePagesByColor[ZeroedPageList][i].Flink = MM_EMPTY_LIST;
        MmFreePagesByColor[FreePageList][i].Flink = MM_EMPTY_LIST;
    }

    //
    // Add nonpaged pool to PFN database if mapped via KSEG0.
    //

    PointerPde = MiGetPdeAddress (PTE_BASE);

    if ((MmNonPagedPoolStart < (PVOID)MM_KSEG2_BASE) && (MapLargePages != 0)) {
        j = MI_CONVERT_PHYSICAL_TO_PFN (MmNonPagedPoolStart);
        Pfn1 = MI_PFN_ELEMENT (j);
        i = MmSizeOfNonPagedPoolInBytes >> PAGE_SHIFT;

        PERFINFO_INIT_POOLRANGE(Pfn1 - MmPfnDatabase, i);

        do {
            PointerPde = MiGetPdeAddress (MM_KSEG0_BASE + (j << PAGE_SHIFT));
            Pfn1->PteFrame = MI_GET_PAGE_FRAME_FROM_PTE(PointerPde);
            Pfn1->PteAddress = (PMMPTE)(j << PAGE_SHIFT);
            Pfn1->u2.ShareCount += 1;
            Pfn1->u3.e2.ReferenceCount = 1;
            Pfn1->u3.e1.PageLocation = ActiveAndValid;
            Pfn1->u3.e1.PageColor = 0;
            j += 1;
            Pfn1 += 1;
            i -= 1;
        } while ( i );
    }

    //
    // Go through the page table entries and for any page which is valid,
    // update the corresponding PFN database element.
    //

    Pde = MiGetPdeAddress (NULL);
    va = 0;
    PdeCount = PDE_PER_PAGE;
#if defined(_X86PAE_)
    PdeCount *= PD_PER_SYSTEM;
#endif
    for (i = 0; i < PdeCount; i += 1) {

        //
        // If the kernel image has been biased to allow for 3gb of user
        // address space, then the first 16mb of memory is double mapped
        // to KSEG0_BASE and to ALTERNATE_BASE. Therefore, the KSEG0_BASE
        // entries must be skipped.
        //

        if (MmVirtualBias != 0) {
            if ((Pde >= MiGetPdeAddress(KSEG0_BASE)) &&
                (Pde < MiGetPdeAddress(KSEG0_BASE + 16 * 1024 * 1024))) {
                Pde += 1;
                va += (ULONG)PDE_PER_PAGE * (ULONG)PAGE_SIZE;
                continue;
            }
        }

        if ((Pde->u.Hard.Valid == 1) && (Pde->u.Hard.LargePage == 0)) {

            PdePage = MI_GET_PAGE_FRAME_FROM_PTE(Pde);
            Pfn1 = MI_PFN_ELEMENT(PdePage);
            Pfn1->PteFrame = MI_GET_PAGE_FRAME_FROM_PTE(PointerPde);
            Pfn1->PteAddress = Pde;
            Pfn1->u2.ShareCount += 1;
            Pfn1->u3.e2.ReferenceCount = 1;
            Pfn1->u3.e1.PageLocation = ActiveAndValid;
            Pfn1->u3.e1.PageColor = 0;

            PointerPte = MiGetPteAddress (va);

            //
            // Set global bit.
            //

            Pde->u.Long |= MiDetermineUserGlobalPteMask (PointerPte) &
                                                           ~MM_PTE_ACCESS_MASK;
            for (j = 0 ; j < PTE_PER_PAGE; j += 1) {
                if (PointerPte->u.Hard.Valid == 1) {

                    PointerPte->u.Long |= MiDetermineUserGlobalPteMask (PointerPte) &
                                                            ~MM_PTE_ACCESS_MASK;

                    Pfn1->u2.ShareCount += 1;

                    if ((PointerPte->u.Hard.PageFrameNumber <= MmHighestPhysicalPage) &&
                        ((va >= MM_KSEG2_BASE) &&
                         ((va < (KSEG0_BASE + MmVirtualBias)) ||
                          (va >= (KSEG0_BASE + MmVirtualBias + 16 * 1024 * 1024)))) ||
                        ((NeedLowVirtualPfn == TRUE) &&
                         (va >= (ULONG)MmNonPagedPoolStart) &&
                         (va < (ULONG)MmNonPagedPoolStart + MmSizeOfNonPagedPoolInBytes))) {

                        Pfn2 = MI_PFN_ELEMENT(PointerPte->u.Hard.PageFrameNumber);

                        if (MmIsAddressValid(Pfn2) &&
                             MmIsAddressValid((PUCHAR)(Pfn2+1)-1)) {

                            Pfn2->PteFrame = PdePage;
                            Pfn2->PteAddress = PointerPte;
                            Pfn2->u2.ShareCount += 1;
                            Pfn2->u3.e2.ReferenceCount = 1;
                            Pfn2->u3.e1.PageLocation = ActiveAndValid;
                            Pfn2->u3.e1.PageColor = 0;
                        }
                    }
                }

                va += PAGE_SIZE;
                PointerPte += 1;
            }

        } else {
            va += (ULONG)PDE_PER_PAGE * (ULONG)PAGE_SIZE;
        }

        Pde += 1;
    }

    KeRaiseIrql (DISPATCH_LEVEL, &OldIrql);
    KeFlushCurrentTb ();
    KeLowerIrql (OldIrql);

    //
    // If the lowest physical page is zero and the page is still unused, mark
    // it as in use. This is temporary as we want to find bugs where a physical
    // page is specified as zero.
    //

    Pfn1 = &MmPfnDatabase[MmLowestPhysicalPage];

    if ((MmLowestPhysicalPage == 0) && (Pfn1->u3.e2.ReferenceCount == 0)) {

        ASSERT (Pfn1->u3.e2.ReferenceCount == 0);

        //
        // Make the reference count non-zero and point it into a
        // page directory.
        //

        Pde = MiGetPdeAddress (0xffffffff);
        PdePage = MI_GET_PAGE_FRAME_FROM_PTE(Pde);
        Pfn1->PteFrame = PdePageNumber;
        Pfn1->PteAddress = Pde;
        Pfn1->u2.ShareCount += 1;
        Pfn1->u3.e2.ReferenceCount = 0xfff0;
        Pfn1->u3.e1.PageLocation = ActiveAndValid;
        Pfn1->u3.e1.PageColor = 0;
    }

    // end of temporary set to physical page zero.

    //
    // Walk through the memory descriptors and add pages to the
    // free list in the PFN database.
    //

    if (NextPhysicalPage < (FreeDescriptorLowMem->PageCount +
                            FreeDescriptorLowMem->BasePage)) {

        //
        // We haven't used the other descriptor.
        //

        FreeDescriptorLowMem->PageCount -= NextPhysicalPage -
            OldFreeDescriptorLowMemBase;
        FreeDescriptorLowMem->BasePage = NextPhysicalPage;

    } else {

        ASSERT (FreeDescriptor != NULL);
        FreeDescriptorLowMem->PageCount = 0;

        FreeDescriptor->PageCount = OldFreeDescriptorBase + OldFreeDescriptorCount - NextPhysicalPage;

        FreeDescriptor->BasePage = NextPhysicalPage;

    }

    //
    // Since the LoaderBlock memory descriptors are generally ordered
    // from low physical memory address to high, walk it backwards so the
    // high physical pages go to the front of the freelists.  The thinking
    // is that pages initially allocated by the system less likely to be
    // freed so don't waste memory below 16mb (or 4gb) that may be needed
    // by drivers later.
    //

    NextMd = LoaderBlock->MemoryDescriptorListHead.Blink;

    while (NextMd != &LoaderBlock->MemoryDescriptorListHead) {

        MemoryDescriptor = CONTAINING_RECORD(NextMd,
                                             MEMORY_ALLOCATION_DESCRIPTOR,
                                             ListEntry);

        i = MemoryDescriptor->PageCount;
        NextPhysicalPage = MemoryDescriptor->BasePage;

        switch (MemoryDescriptor->MemoryType) {
            case LoaderBad:
                while (i != 0) {
                    MiInsertPageInList (MmPageLocationList[BadPageList],
                                        NextPhysicalPage);
                    i -= 1;
                    NextPhysicalPage += 1;
                }
                break;

            case LoaderFree:
            case LoaderLoadedProgram:
            case LoaderFirmwareTemporary:
            case LoaderOsloaderStack:

                FreePfnCount = 0;
                Pfn1 = MI_PFN_ELEMENT (NextPhysicalPage);
                while (i != 0) {
                    if (Pfn1->u3.e2.ReferenceCount == 0) {

                        //
                        // Set the PTE address to the physical page for
                        // virtual address alignment checking.
                        //

                        Pfn1->PteAddress =
                                        (PMMPTE)(NextPhysicalPage << PTE_SHIFT);
                        MiInsertPageInList (MmPageLocationList[FreePageList],
                                            NextPhysicalPage);
                        FreePfnCount += 1;
                    }
                    else {
                        if (FreePfnCount > LargestFreePfnCount) {
                            LargestFreePfnCount = FreePfnCount;
                            LargestFreePfnStart = NextPhysicalPage - FreePfnCount;
                            FreePfnCount = 0;
                        }
                    }

                    Pfn1 += 1;
                    i -= 1;
                    NextPhysicalPage += 1;
                }

                if (FreePfnCount > LargestFreePfnCount) {
                    LargestFreePfnCount = FreePfnCount;
                    LargestFreePfnStart = NextPhysicalPage - FreePfnCount;
                }

                break;

            case LoaderFirmwarePermanent:
            case LoaderSpecialMemory:
            case LoaderBBTMemory:

                //
                // If the descriptor lies within the highest PFN database entry
                // then create PFN pages for this range.  Note the PFN entries
                // must be created to support \Device\PhysicalMemory.
                //

                if (MemoryDescriptor->BasePage <= MmHighestPhysicalPage) {

                    if (MemoryDescriptor->BasePage + MemoryDescriptor->PageCount > MmHighestPhysicalPage + 1) {
                        MemoryDescriptor->PageCount = MmHighestPhysicalPage - MemoryDescriptor->BasePage + 1;
                        i = MemoryDescriptor->PageCount;
                    }
                }
                else {
                    break;
                }

                //
                // Fall through as these pages must be marked in use as they
                // lie within the PFN limits and may be accessed through
                // \Device\PhysicalMemory.
                //

            default:

                PointerPte = MiGetPteAddress (KSEG0_BASE + MmVirtualBias +
                                            (NextPhysicalPage << PAGE_SHIFT));

                Pfn1 = MI_PFN_ELEMENT (NextPhysicalPage);
                while (i != 0) {

                    //
                    // Set page as in use.
                    //

                    PointerPde = MiGetPdeAddress (KSEG0_BASE + MmVirtualBias +
                                             (NextPhysicalPage << PAGE_SHIFT));

                    if (Pfn1->u3.e2.ReferenceCount == 0) {
                        Pfn1->PteFrame = MI_GET_PAGE_FRAME_FROM_PTE(PointerPde);
                        Pfn1->PteAddress = PointerPte;
                        Pfn1->u2.ShareCount += 1;
                        Pfn1->u3.e2.ReferenceCount = 1;
                        Pfn1->u3.e1.PageLocation = ActiveAndValid;
                        Pfn1->u3.e1.PageColor = 0;
                    }
                    Pfn1 += 1;
                    i -= 1;
                    NextPhysicalPage += 1;
                    PointerPte += 1;
                }
                break;
        }

        NextMd = MemoryDescriptor->ListEntry.Blink;
    }

    if (PfnInKseg0 == FALSE) {

        //
        // Indicate that the PFN database is allocated in NonPaged pool.
        //

        PointerPte = MiGetPteAddress (&MmPfnDatabase[MmLowestPhysicalPage]);
        Pfn1 = MI_PFN_ELEMENT(PointerPte->u.Hard.PageFrameNumber);
        Pfn1->u3.e1.StartOfAllocation = 1;

        if (NeedLowVirtualPfn == TRUE) {
            LastPte = MiGetPteAddress (&MmPfnDatabase[MmHighestPossiblePhysicalPage]);
            while (PointerPte <= LastPte) {
                Pfn1 = MI_PFN_ELEMENT(PointerPte->u.Hard.PageFrameNumber);
                Pfn1->u2.ShareCount = 1;
                Pfn1->u3.e2.ReferenceCount = 1;
                PointerPte += 1;
            }
        }

        //
        // Set the end of the allocation.
        //

        PointerPte = MiGetPteAddress (&MmPfnDatabase[MmHighestPossiblePhysicalPage]);
        Pfn1 = MI_PFN_ELEMENT(PointerPte->u.Hard.PageFrameNumber);
        Pfn1->u3.e1.EndOfAllocation = 1;

    }
    else {

        //
        // The PFN database is allocated in KSEG0.
        //
        // Mark all PFN entries for the PFN pages in use.
        //

        PageFrameIndex = MI_CONVERT_PHYSICAL_TO_PFN (MmPfnDatabase);
        Pfn1 = MI_PFN_ELEMENT(PageFrameIndex);
        do {
            Pfn1->PteAddress = (PMMPTE)(PageFrameIndex << PTE_SHIFT);
            Pfn1->u3.e1.PageColor = 0;
            Pfn1->u3.e2.ReferenceCount += 1;
            PageFrameIndex += 1;
            Pfn1 += 1;
            PfnAllocation -= 1;
        } while (PfnAllocation != 0);

        //
        // Scan the PFN database backward for pages that are completely zero.
        // These pages are unused and can be added to the free list.
        //

        BottomPfn = MI_PFN_ELEMENT(MmHighestPhysicalPage);
        do {

            //
            // Compute the address of the start of the page that is next
            // lower in memory and scan backwards until that page address
            // is reached or just crossed.
            //

            if (((ULONG)BottomPfn & (PAGE_SIZE - 1)) != 0) {
                BasePfn = (PMMPFN)((ULONG)BottomPfn & ~(PAGE_SIZE - 1));
                TopPfn = BottomPfn + 1;

            } else {
                BasePfn = (PMMPFN)((ULONG)BottomPfn - PAGE_SIZE);
                TopPfn = BottomPfn;
            }

            while (BottomPfn > BasePfn) {
                BottomPfn -= 1;
            }

            //
            // If the entire range over which the PFN entries span is
            // completely zero and the PFN entry that maps the page is
            // not in the range, then add the page to the appropriate
            // free list.
            //

            Range = (ULONG)TopPfn - (ULONG)BottomPfn;
            if (RtlCompareMemoryUlong((PVOID)BottomPfn, Range, 0) == Range) {

                //
                // Set the PTE address to the physical page for virtual
                // address alignment checking.
                //

                PageFrameIndex = MI_CONVERT_PHYSICAL_TO_PFN (BasePfn);
                Pfn1 = MI_PFN_ELEMENT(PageFrameIndex);

                ASSERT (Pfn1->u3.e2.ReferenceCount == 1);
                ASSERT (Pfn1->PteAddress == (PMMPTE)(PageFrameIndex << PTE_SHIFT));
                Pfn1->u3.e2.ReferenceCount = 0;
                PfnAllocation += 1;
                Pfn1->PteAddress = (PMMPTE)(PageFrameIndex << PTE_SHIFT);
                Pfn1->u3.e1.PageColor = 0;
                MiInsertPageInList(MmPageLocationList[FreePageList],
                                   PageFrameIndex);
            }
        } while (BottomPfn > MmPfnDatabase);
    }

    //
    // Indicate that nonpaged pool must succeed is allocated in
    // nonpaged pool.
    //

    PointerPte = MiGetPteAddress(MmNonPagedMustSucceed);
    i = MmSizeOfNonPagedMustSucceed;
    while ((LONG)i > 0) {
        Pfn1 = MI_PFN_ELEMENT (PointerPte->u.Hard.PageFrameNumber);
        Pfn1->u3.e1.StartOfAllocation = 1;
        Pfn1->u3.e1.EndOfAllocation = 1;
        i -= PAGE_SIZE;
        PointerPte += 1;
    }

    //
    // Adjust the memory descriptors to indicate that free pool has
    // been used for nonpaged pool creation.
    //

    FreeDescriptorLowMem->PageCount = OldFreeDescriptorLowMemCount;
    FreeDescriptorLowMem->BasePage = OldFreeDescriptorLowMemBase;

    if (FreeDescriptor != NULL) {
        FreeDescriptor->PageCount = OldFreeDescriptorCount;
        FreeDescriptor->BasePage = OldFreeDescriptorBase;
    }

    KeInitializeSpinLock (&MmSystemSpaceLock);

    KeInitializeSpinLock (&MmPfnLock);

    //
    // Initialize the nonpaged available PTEs for mapping I/O space
    // and kernel stacks.
    //

    PointerPte = MiGetPteAddress (MmNonPagedSystemStart);
    ASSERT (((ULONG)PointerPte & (PAGE_SIZE - 1)) == 0);

    MmNumberOfSystemPtes = MiGetPteAddress(NonPagedPoolStartVirtual) - PointerPte - 1;

    MiInitializeSystemPtes (PointerPte, MmNumberOfSystemPtes, SystemPteSpace);

    if (ExtraPtes != 0) {

        //
        // Add extra system PTEs to the pool.
        //

        PointerPte = MiGetPteAddress (KSTACK_POOL_START);
        MiAddSystemPtes (PointerPte, ExtraPtes, SystemPteSpace);
    }

    //
    // Add pages to nonpaged pool if we could not allocate enough physically
    // contiguous.
    //

    j = (SavedSize - MmSizeOfNonPagedPoolInBytes) >> PAGE_SHIFT;

    if ((j != 0) && (MmExpandedNonPagedPoolInBytes == 0)) {

        ULONG CountContiguous;

        CountContiguous = LargestFreePfnCount;
        PageFrameIndex = LargestFreePfnStart - 1;

        PointerPte = MiGetPteAddress (NonPagedPoolStartVirtual);

        while (j) {
            if (CountContiguous) {
                PageFrameIndex += 1;
                MiUnlinkFreeOrZeroedPage (PageFrameIndex);
                CountContiguous -= 1;
            } else {
                PageFrameIndex = MiRemoveAnyPage (
                                MI_GET_PAGE_COLOR_FROM_PTE (PointerPte));
            }
            Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);

            Pfn1->u3.e2.ReferenceCount = 1;
            Pfn1->u2.ShareCount = 1;
            Pfn1->PteAddress = PointerPte;
            Pfn1->OriginalPte.u.Long = MM_DEMAND_ZERO_WRITE_PTE;
            Pfn1->PteFrame = MI_GET_PAGE_FRAME_FROM_PTE(MiGetPteAddress(PointerPte));
            Pfn1->u3.e1.PageLocation = ActiveAndValid;

            TempPte.u.Hard.PageFrameNumber = PageFrameIndex;
            *PointerPte = TempPte;
            PointerPte += 1;

            j -= 1;
        }
        Pfn1->u3.e1.EndOfAllocation = 1;
        Pfn1 = MI_PFN_ELEMENT (MiGetPteAddress(NonPagedPoolStartVirtual)->u.Hard.PageFrameNumber);
        Pfn1->u3.e1.StartOfAllocation = 1;

        Range = MmAllocatedNonPagedPool;
        MiFreePoolPages (NonPagedPoolStartVirtual);
        MmAllocatedNonPagedPool = Range;
    }

    //
    // Initialize the nonpaged pool.
    //

    InitializePool (NonPagedPool, 0);

    //
    // Initialize memory management structures for this process.
    //

    //
    // Build working set list.  This requires the creation of a PDE
    // to map HYPER space and the page table page pointed to
    // by the PDE must be initialized.
    //
    // Note, we can't remove a zeroed page as hyper space does not
    // exist and we map non-zeroed pages into hyper space to zero.
    //

    TempPde = ValidKernelPdeLocal;

    PointerPde = MiGetPdeAddress(HYPER_SPACE);

    LOCK_PFN (OldIrql);

    PageFrameIndex = MiRemoveAnyPage (0);
    TempPde.u.Hard.PageFrameNumber = PageFrameIndex;
    *PointerPde = TempPde;

#if defined (_X86PAE_)
    PointerPde = MiGetPdeAddress((PVOID)((PCHAR)HYPER_SPACE + MM_VA_MAPPED_BY_PDE));

    PageFrameIndex = MiRemoveAnyPage (0);
    TempPde.u.Hard.PageFrameNumber = PageFrameIndex;
    *PointerPde = TempPde;

    //
    // Point to the page table page we just created and zero it.
    //

    PointerPte = MiGetVirtualAddressMappedByPte (PointerPde);
    RtlZeroMemory (PointerPte, PAGE_SIZE);
#endif

    KeFlushCurrentTb();

    UNLOCK_PFN (OldIrql);

    //
    // Point to the page table page we just created and zero it.
    //

    PointerPte = MiGetPteAddress(HYPER_SPACE);
    RtlZeroMemory ((PVOID)PointerPte, PAGE_SIZE);

    //
    // Hyper space now exists, set the necessary variables.
    //

    MmFirstReservedMappingPte = MiGetPteAddress (FIRST_MAPPING_PTE);
    MmLastReservedMappingPte = MiGetPteAddress (LAST_MAPPING_PTE);

    MmWorkingSetList = WORKING_SET_LIST;
    MmWsle = (PMMWSLE)((PUCHAR)WORKING_SET_LIST + sizeof(MMWSL));

    //
    // Initialize this process's memory management structures including
    // the working set list.
    //

    //
    // The PFN element for the page directory has already been initialized,
    // zero the reference count and the share count so they won't be
    // wrong.
    //

    Pfn1 = MI_PFN_ELEMENT (PdePageNumber);

    LOCK_PFN (OldIrql);

    Pfn1->u2.ShareCount = 0;
    Pfn1->u3.e2.ReferenceCount = 0;

#if defined (_X86PAE_)
    PointerPte = MiGetPteAddress (PDE_BASE);
    for (i = 0; i < PD_PER_SYSTEM; i += 1) {

        PdePageNumber = MI_GET_PAGE_FRAME_FROM_PTE(PointerPte);

        Pfn1 = MI_PFN_ELEMENT (PdePageNumber);
        Pfn1->u2.ShareCount = 0;
        Pfn1->u3.e2.ReferenceCount = 0;

        PointerPte += 1;
    }
#endif

    CurrentProcess = PsGetCurrentProcess ();

    //
    // Get a page for the working set list and zero it.
    //

    TempPte = ValidKernelPteLocal;
    PointerPte = MiGetPteAddress (HYPER_SPACE);
    PageFrameIndex = MiRemoveAnyPage (0);

    TempPte.u.Hard.PageFrameNumber = PageFrameIndex;
    PointerPte = MiGetPteAddress (HYPER_SPACE);
    *PointerPte = TempPte;
    RtlZeroMemory ((PVOID)HYPER_SPACE, PAGE_SIZE);
    *PointerPte = ZeroPte;

    CurrentProcess->WorkingSetPage = PageFrameIndex;

#if defined (_X86PAE_)
    MiPaeInitialize ();
#endif

    KeFlushCurrentTb();

    UNLOCK_PFN (OldIrql);

    CurrentProcess->Vm.MaximumWorkingSetSize = MmSystemProcessWorkingSetMax;
    CurrentProcess->Vm.MinimumWorkingSetSize = MmSystemProcessWorkingSetMin;

    MmInitializeProcessAddressSpace (CurrentProcess,
                                     (PEPROCESS)NULL,
                                     (PVOID)NULL,
                                     (PVOID)NULL);

    //
    // Check to see if moving the secondary page structures to the end
    // of the PFN database is a waste of memory.
    //
    // If the PFN database ends on a page aligned boundary and the
    // size of the two arrays is less than a page, free the page
    // and reallocate it from nonpagedpool.
    //

    if (NeedLowVirtualPfn == TRUE) {

        ASSERT (MmFreePagesByColor[0] < (PMMCOLOR_TABLES)MM_KSEG2_BASE);

        PointerPde = MiGetPdeAddress(MmFreePagesByColor[0]);
        ASSERT (PointerPde->u.Hard.Valid == 1);

        PointerPte = MiGetPteAddress(MmFreePagesByColor[0]);
        ASSERT (PointerPte->u.Hard.Valid == 1);

        PageFrameIndex = MI_GET_PAGE_FRAME_FROM_PTE(PointerPte);
        Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);

        LOCK_PFN (OldIrql);

        if (Pfn1->u3.e2.ReferenceCount == 0) {
            Pfn1->PteFrame = MI_GET_PAGE_FRAME_FROM_PTE(PointerPde);
            Pfn1->PteAddress = PointerPte;
            Pfn1->u2.ShareCount += 1;
            Pfn1->u3.e2.ReferenceCount = 1;
            Pfn1->u3.e1.PageLocation = ActiveAndValid;
            Pfn1->u3.e1.PageColor = 0;
        }
        UNLOCK_PFN (OldIrql);
    }
    else if ((((ULONG)MmFreePagesByColor[0] & (PAGE_SIZE - 1)) == 0) &&
        ((MmSecondaryColors * 2 * sizeof(MMCOLOR_TABLES)) < PAGE_SIZE)) {

        PMMCOLOR_TABLES c;

        c = MmFreePagesByColor[0];

        MmFreePagesByColor[0] = ExAllocatePoolWithTag (NonPagedPoolMustSucceed,
                               MmSecondaryColors * 2 * sizeof(MMCOLOR_TABLES),
                               '  mM');

        MmFreePagesByColor[1] = &MmFreePagesByColor[0][MmSecondaryColors];

        RtlMoveMemory (MmFreePagesByColor[0],
                       c,
                       MmSecondaryColors * 2 * sizeof(MMCOLOR_TABLES));

        //
        // Free the page.
        //

        if (c > (PMMCOLOR_TABLES)MM_KSEG2_BASE) {
            PointerPte = MiGetPteAddress(c);
            PageFrameIndex = MI_GET_PAGE_FRAME_FROM_PTE(PointerPte);
            *PointerPte = ZeroKernelPte;
        } else {
            PageFrameIndex = MI_CONVERT_PHYSICAL_TO_PFN (c);
        }

        LOCK_PFN (OldIrql);

        Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);
        ASSERT ((Pfn1->u2.ShareCount <= 1) && (Pfn1->u3.e2.ReferenceCount <= 1));
        Pfn1->u2.ShareCount = 0;
        Pfn1->u3.e2.ReferenceCount = 1;
        MI_SET_PFN_DELETED (Pfn1);
#if DBG
        Pfn1->u3.e1.PageLocation = StandbyPageList;
#endif //DBG
        MiDecrementReferenceCount (PageFrameIndex);

        UNLOCK_PFN (OldIrql);
    }

    //
    // Handle physical pages in BIOS memory range (640k to 1mb) by
    // explicitly initializing them in the PFN database so that they
    // can be handled properly when I/O is done to these pages (or virtual
    // reads across processes).
    //


    Pfn1 = MI_PFN_ELEMENT (MM_BIOS_START);
    Pfn2 = MI_PFN_ELEMENT (MM_BIOS_END);

    LOCK_PFN (OldIrql);

    do {
        if ((Pfn1->u2.ShareCount == 0) &&
            (Pfn1->u3.e2.ReferenceCount == 0) &&
            (Pfn1->PteAddress == 0)) {

            //
            // Set this as in use.
            //

            Pfn1->u3.e2.ReferenceCount = 1;
            Pfn1->PteAddress = (PMMPTE)0x7FFFFFFF;
            Pfn1->u3.e1.PageLocation = ActiveAndValid;
            Pfn1->u3.e1.PageColor = 0;
        }
        Pfn1 += 1;
    } while (Pfn1 <= Pfn2);

    UNLOCK_PFN (OldIrql);

#if defined (_X86PAE_)
    if (MiNoLowMemory == TRUE) {

        MiCreateBitMap (&MiLowMemoryBitMap, 1024 * 1024, NonPagedPool);

        if (MiLowMemoryBitMap != NULL) {
            RtlClearAllBits (MiLowMemoryBitMap);
            MiRemoveLowPages ();
            MmMakeLowMemory = TRUE;
        }
    }
#endif

    return;
}

#if defined (_X86PAE_)

#define MI_MAGIC_4GB_RECLAIM   0xffffedf0

ULONG MiRemoveCount;

//
// Normally there are approximately 15 runs.
//

#define MI_MAXPAERUNS   30

typedef struct _PAERUN {
    PFN_NUMBER StartFrame;
    PFN_NUMBER EndFrame;
} PAERUN, *PPAERUN;

PAERUN MiUnclaimed[MI_MAXPAERUNS];

VOID
MiRemoveLowPages (
    VOID
    )

/*++

Routine Description:

    This routine removes all pages below 4GB on a PAE system.  This lets
    us find problems with device drivers by putting all accesses high.

Arguments:

    None.

Return Value:

    None.

Environment:

    Kernel mode.

--*/

{
    ULONG i;
    KIRQL OldIrql;
    KIRQL OldIrqlHyper;
    PFN_COUNT PageCount;
    PMMPFN PfnNextColored;
    PMMPFN PfnNextFlink;
    PMMPFN PfnLastColored;
    PFN_NUMBER PageNextColored;
    PFN_NUMBER PageNextFlink;
    PFN_NUMBER PageLastColored;
    PFN_NUMBER Page;
    PFN_NUMBER HighPage;
    PMMPFN Pfn1;
    PMMPFNLIST ListHead;
    ULONG Color;
    PMMCOLOR_TABLES ColorHead;
    PFN_NUMBER MovedPage;
    PVOID TempVa;

    if (MiLowMemoryBitMap == NULL) {
        return;
    }

    ListHead = MmPageLocationList[FreePageList];
    PageCount = 0;

    LOCK_PFN (OldIrql);

    for (Color = 0; Color < MmSecondaryColors; Color += 1) {
        ColorHead = &MmFreePagesByColor[FreePageList][Color];

        MovedPage = MM_EMPTY_LIST;

        while (ColorHead->Flink != MM_EMPTY_LIST) {

            Page = ColorHead->Flink;

            Pfn1 = MI_PFN_ELEMENT(Page);

            ASSERT ((MMLISTS)Pfn1->u3.e1.PageLocation == FreePageList);

            // 
            // The Flink and Blink must be nonzero here for the page
            // to be on the listhead.  Only code that scans the
            // MmPhysicalMemoryBlock has to check for the zero case.
            //

            ASSERT (Pfn1->u1.Flink != 0);
            ASSERT (Pfn1->u2.Blink != 0);

            //
            // See if the page is below 4GB.
            //

            if (Page >= 0x100000) {

                //
                // Put page on end of list and if first time, save pfn.
                //

                if (MovedPage == MM_EMPTY_LIST) {
                    MovedPage = Page;
                }
                else if (Page == MovedPage) {

                    //
                    // No more pages available in this colored chain.
                    //

                    break;
                }

                //
                // If the colored chain has more than one entry then
                // put this page on the end.
                //

                PageNextColored = (PFN_NUMBER)Pfn1->OriginalPte.u.Long;

                if (PageNextColored == MM_EMPTY_LIST) {

                    //
                    // No more pages available in this colored chain.
                    //

                    break;
                }

                ASSERT (Pfn1->u1.Flink != 0);
                ASSERT (Pfn1->u1.Flink != MM_EMPTY_LIST);
                ASSERT (Pfn1->PteFrame != MI_MAGIC_4GB_RECLAIM);

                PfnNextColored = MI_PFN_ELEMENT(PageNextColored);
                ASSERT ((MMLISTS)PfnNextColored->u3.e1.PageLocation == FreePageList);
                ASSERT (PfnNextColored->PteFrame != MI_MAGIC_4GB_RECLAIM);

                //
                // Adjust the free page list so Page
                // follows PageNextFlink.
                //

                PageNextFlink = Pfn1->u1.Flink;
                PfnNextFlink = MI_PFN_ELEMENT(PageNextFlink);

                ASSERT ((MMLISTS)PfnNextFlink->u3.e1.PageLocation == FreePageList);
                ASSERT (PfnNextFlink->PteFrame != MI_MAGIC_4GB_RECLAIM);

                PfnLastColored = ColorHead->Blink;
                ASSERT (PfnLastColored != (PMMPFN)MM_EMPTY_LIST);
                ASSERT (PfnLastColored->OriginalPte.u.Long == MM_EMPTY_LIST);
                ASSERT (PfnLastColored->PteFrame != MI_MAGIC_4GB_RECLAIM);
                ASSERT (PfnLastColored->u2.Blink != MM_EMPTY_LIST);

                ASSERT ((MMLISTS)PfnLastColored->u3.e1.PageLocation == FreePageList);
                PageLastColored = PfnLastColored - MmPfnDatabase;

                if (ListHead->Flink == Page) {

                    ASSERT (Pfn1->u2.Blink == MM_EMPTY_LIST);
                    ASSERT (ListHead->Blink != Page);

                    ListHead->Flink = PageNextFlink;

                    PfnNextFlink->u2.Blink = MM_EMPTY_LIST;
                }
                else {

                    ASSERT (Pfn1->u2.Blink != MM_EMPTY_LIST);
                    ASSERT ((MMLISTS)(MI_PFN_ELEMENT((MI_PFN_ELEMENT(Pfn1->u2.Blink)->u1.Flink)))->PteFrame != MI_MAGIC_4GB_RECLAIM);
                    ASSERT ((MMLISTS)(MI_PFN_ELEMENT((MI_PFN_ELEMENT(Pfn1->u2.Blink)->u1.Flink)))->u3.e1.PageLocation == FreePageList);

                    MI_PFN_ELEMENT(Pfn1->u2.Blink)->u1.Flink = PageNextFlink;
                    PfnNextFlink->u2.Blink = Pfn1->u2.Blink;
                }

#if DBG
                if (PfnLastColored->u1.Flink == MM_EMPTY_LIST) {
                    ASSERT (ListHead->Blink == PageLastColored);
                }
#endif

                Pfn1->u1.Flink = PfnLastColored->u1.Flink;
                Pfn1->u2.Blink = PageLastColored;

                if (ListHead->Blink == PageLastColored) {
                    ListHead->Blink = Page;
                }

                //
                // Adjust the colored chains.
                //

                if (PfnLastColored->u1.Flink != MM_EMPTY_LIST) {
                    ASSERT (MI_PFN_ELEMENT(PfnLastColored->u1.Flink)->PteFrame != MI_MAGIC_4GB_RECLAIM);
                    ASSERT ((MMLISTS)(MI_PFN_ELEMENT(PfnLastColored->u1.Flink)->u3.e1.PageLocation) == FreePageList);
                    MI_PFN_ELEMENT(PfnLastColored->u1.Flink)->u2.Blink = Page;
                }

                PfnLastColored->u1.Flink = Page;

                ColorHead->Flink = PageNextColored;
                Pfn1->OriginalPte.u.Long = MM_EMPTY_LIST;

                ASSERT (PfnLastColored->OriginalPte.u.Long == MM_EMPTY_LIST);
                PfnLastColored->OriginalPte.u.Long = Page;
                ColorHead->Blink = Pfn1;

                continue;
            }

            //
            // Page is below 4GB so reclaim it.
            //

            ASSERT (Pfn1->u3.e1.ReadInProgress == 0);
            MiUnlinkFreeOrZeroedPage (Page);
            Pfn1->u3.e1.PageColor = 0;

            Pfn1->u3.e2.ReferenceCount = 1;
            Pfn1->u2.ShareCount = 1;
            MI_SET_PFN_DELETED(Pfn1);
            Pfn1->OriginalPte.u.Long = MM_DEMAND_ZERO_WRITE_PTE;
            Pfn1->PteFrame = MI_MAGIC_4GB_RECLAIM;
            Pfn1->u3.e1.PageLocation = ActiveAndValid;

            Pfn1->u3.e1.StartOfAllocation = 1;
            Pfn1->u3.e1.EndOfAllocation = 1;
            Pfn1->u3.e1.VerifierAllocation = 0;
            Pfn1->u3.e1.LargeSessionAllocation = 0;

            //
            // Fill the actual page with a recognizable data pattern.  No one
            // else should write to these pages unless they are allocated for
            // a contiguous memory request.
            //

            TempVa = MiMapPageInHyperSpace (Page, &OldIrqlHyper);

            RtlFillMemoryUlong (TempVa,
                                PAGE_SIZE,
                                Page | MI_LOWMEM_MAGIC_BIT);

            MiUnmapPageInHyperSpace (OldIrqlHyper);

            ASSERT (Page < MiLowMemoryBitMap->SizeOfBitMap);
            ASSERT (RtlCheckBit (MiLowMemoryBitMap, Page) == 0);
            RtlSetBits (MiLowMemoryBitMap, Page, 1L);
            PageCount += 1;
        }
    }

    MmNumberOfPhysicalPages -= PageCount;

    UNLOCK_PFN (OldIrql);

#if DBG
    DbgPrint ("Removed 0x%x pages from low memory for PAE testing\n", PageCount);
#endif

    MiRemoveCount += 1;

    if (MiRemoveCount == 2) {
        ULONG run;
        ULONG total;
        ULONG RunIndex;

        Pfn1 = MI_PFN_ELEMENT(0);
        run = 0;
        total = 0;
        RunIndex = 0;

#if DBG
        DbgPrint ("Unclaimable Pages below 4GB are:\n\n");
        DbgPrint ("StartPage EndPage  Length\n");
#endif

        for (i = 0; i < 0x100001; i += 1) {
            if ((!MmIsAddressValid(&Pfn1->PteFrame)) ||
                (Pfn1->PteFrame == MI_MAGIC_4GB_RECLAIM) ||
                (i == 0x100000)) {

                if (run != 0) {
#if DBG
                    DbgPrint ("%08lx  %08lx %08lx\n",
                                Pfn1 - run - MmPfnDatabase,
                                Pfn1 - MmPfnDatabase - 1,
                                run);
#endif
                    if (RunIndex < MI_MAXPAERUNS) {
                        MiUnclaimed[RunIndex].StartFrame = Pfn1 - run - MmPfnDatabase;
                        MiUnclaimed[RunIndex].EndFrame =  Pfn1 - MmPfnDatabase - 1;
                        RunIndex += 1;
                    }
                    run = 0;
                }
            }
            else {
                total += 1;
                run += 1;
            }
            Pfn1 += 1;
        }
#if DBG
        DbgPrint ("Total 0x%x Unclaimable Pages below 4GB\n\n", total);
#endif

        //
        // For every page below 4GB that could not be reclaimed, don't use the
        // high modulo-4GB equivalent page.  The motivation is to prevent
        // code bugs that drop the high bits from destroying critical
        // system data in the unclaimed pages (like the GDT, IDT, kernel code
        // and data, etc).
        //

        total = 0;

        LOCK_PFN (OldIrql);

        for (i = 0; i < RunIndex; i += 1) {
            Page = MiUnclaimed[i].StartFrame;
            PageCount = MiUnclaimed[i].EndFrame -
                            MiUnclaimed[i].StartFrame + 1;

            while (PageCount != 0) {
                HighPage = Page + 0x100000;
    
                while (HighPage <= MmHighestPhysicalPage) {

                    Pfn1 = MI_PFN_ELEMENT (HighPage);
            
                    if ((MmIsAddressValid(Pfn1)) &&
                        (MmIsAddressValid((PCHAR)Pfn1 + sizeof(MMPFN) - 1)) &&
                        ((ULONG)Pfn1->u3.e1.PageLocation <= (ULONG)StandbyPageList) &&
                        (Pfn1->u1.Flink != 0) &&
                        (Pfn1->u2.Blink != 0) &&
                        (Pfn1->u3.e2.ReferenceCount == 0)) {
        
                            //
                            // This page can be taken.
                            //
    
                            if (Pfn1->u3.e1.PageLocation == StandbyPageList) {
                                MiUnlinkPageFromList (Pfn1);
                                MiRestoreTransitionPte (HighPage);
                            } else {
                                MiUnlinkFreeOrZeroedPage (HighPage);
                            }
    
                            Pfn1->u3.e2.ShortFlags = 0;
                            Pfn1->u3.e1.PageColor = 0;
                            Pfn1->u3.e2.ReferenceCount = 1;
                            Pfn1->u2.ShareCount = 1;
                            Pfn1->PteAddress = (PMMPTE)-2;
                            Pfn1->OriginalPte.u.Long = MM_DEMAND_ZERO_WRITE_PTE;
                            Pfn1->PteFrame = MI_MAGIC_4GB_RECLAIM;
                            Pfn1->u3.e1.PageLocation = ActiveAndValid;
                            Pfn1->u3.e1.VerifierAllocation = 0;
                            Pfn1->u3.e1.LargeSessionAllocation = 0;
                            Pfn1->u3.e1.StartOfAllocation = 1;
                            Pfn1->u3.e1.EndOfAllocation = 1;

                            //
                            // Fill the actual page with a recognizable data
                            // pattern.  No one else should write to these
                            // pages unless they are allocated for
                            // a contiguous memory request.
                            //
                
                            TempVa = (PULONG)MiMapPageInHyperSpace (HighPage,
                                                                    &OldIrqlHyper);
                            RtlFillMemoryUlong (TempVa,
                                                PAGE_SIZE,
                                                HighPage | MI_LOWMEM_MAGIC_BIT);

                            MiUnmapPageInHyperSpace (OldIrqlHyper);
                
                            total += 1;
                    }
                    HighPage += 0x100000;
                }
                Page += 1;
                PageCount -= 1;
            }
        }

        MmNumberOfPhysicalPages -= total;

        UNLOCK_PFN (OldIrql);

#if DBG
        if (total != 0) {
            DbgPrint ("Total 0x%x Above-4GB Alias Pages also reclaimed\n\n", total);
        }
#endif
    }
}

extern POOL_DESCRIPTOR NonPagedPoolDescriptor;

PVOID
MiAllocateLowMemory (
    IN SIZE_T NumberOfBytes,
    IN PFN_NUMBER LowestAcceptablePfn,
    IN PFN_NUMBER HighestAcceptablePfn,
    IN PFN_NUMBER BoundaryPfn,
    IN PVOID CallingAddress,
    IN ULONG Tag
    )

/*++

Routine Description:

    This is a special routine for allocating contiguous physical memory below
    4GB on a PAE system that has been booted in test mode where all this memory
    has been made generally unavailable to all components.  This lets us find
    problems with device drivers.

Arguments:

    NumberOfBytes - Supplies the number of bytes to allocate.

    LowestAcceptablePfn - Supplies the lowest page frame number
                          which is valid for the allocation.

    HighestAcceptablePfn - Supplies the highest page frame number
                           which is valid for the allocation.

    BoundaryPfn - Supplies the page frame number multiple the allocation must
                  not cross.  0 indicates it can cross any boundary.

    CallingAddress - Supplies the calling address of the allocator.

    Tag - Supplies the tag to tie to this allocation.

Return Value:

    NULL - a contiguous range could not be found to satisfy the request.

    NON-NULL - Returns a pointer (virtual address in the nonpaged portion
               of the system) to the allocated physically contiguous
               memory.

Environment:

    Kernel mode, IRQL of APC_LEVEL or below.

--*/

{
    PFN_NUMBER Page;
    PFN_NUMBER BoundaryMask;
    PVOID BaseAddress;
    KIRQL OldIrql;
    KIRQL OldIrql2;
    PMMPFN Pfn1;
    ULONG BitMapHint;
    PFN_NUMBER SizeInPages;
    PFN_NUMBER SizeInPages2;
    PMMPTE PointerPte;
    PMMPTE StartPte;
    MMPTE TempPte;
    ULONG PageColor;

    PAGED_CODE();

    BitMapHint = LowestAcceptablePfn;
    SizeInPages = BYTES_TO_PAGES (NumberOfBytes);
    SizeInPages2 = SizeInPages;
    BoundaryMask = ~(BoundaryPfn - 1);

    OldIrql = ExLockPool (NonPagedPool);

    //
    // Try to find system PTES to expand the pool into.
    //

    PointerPte = MiReserveSystemPtes ((ULONG)SizeInPages,
                                      SystemPteSpace,
                                      0,
                                      0,
                                      FALSE);

    if (PointerPte == NULL) {
        goto Fail1;
    }

    StartPte = PointerPte;

    LOCK_PFN2 (OldIrql2);

    do {
        Page = RtlFindSetBits (MiLowMemoryBitMap, SizeInPages, BitMapHint);

        if (Page == (ULONG)-1) {
            goto Fail2;
        }

        if (BoundaryPfn == 0) {
            break;
        }

        if (((Page ^ (Page + SizeInPages - 1)) & BoundaryMask) == 0) {

            //
            // This portion of the range meets the alignment requirements.
            //

            break;
        }

        BitMapHint = (Page & BoundaryMask) + BoundaryPfn;

        if ((BitMapHint >= MiLowMemoryBitMap->SizeOfBitMap) ||
            (BitMapHint + SizeInPages > HighestAcceptablePfn)) {
            goto Fail2;
        }

    } while (TRUE);

    if (Page + SizeInPages > HighestAcceptablePfn) {
        goto Fail2;
    }

    RtlClearBits (MiLowMemoryBitMap, Page, SizeInPages);

    //
    // No need to update ResidentAvailable or commit as these pages were
    // never added to either.
    //

    BaseAddress = MiGetVirtualAddressMappedByPte (PointerPte);
    PageColor = MI_GET_PAGE_COLOR_FROM_VA(BaseAddress);
    TempPte = ValidKernelPte;
    MmAllocatedNonPagedPool += SizeInPages;
    NonPagedPoolDescriptor.TotalBigPages += (ULONG)SizeInPages;
    Pfn1 = MI_PFN_ELEMENT (Page);

    do {
        ASSERT (Pfn1->u3.e1.PageLocation == ActiveAndValid);
        ASSERT (Pfn1->u3.e2.ReferenceCount == 1);
        ASSERT (Pfn1->u2.ShareCount == 1);
        ASSERT (Pfn1->OriginalPte.u.Long == MM_DEMAND_ZERO_WRITE_PTE);
        ASSERT (Pfn1->u3.e1.VerifierAllocation == 0);
        ASSERT (Pfn1->u3.e1.LargeSessionAllocation == 0);

        MI_CHECK_PAGE_ALIGNMENT(Page, PageColor & MM_COLOR_MASK);
        Pfn1->u3.e1.PageColor = PageColor & MM_COLOR_MASK;
        PageColor += 1;
        TempPte.u.Hard.PageFrameNumber = Page;
        MI_WRITE_VALID_PTE (PointerPte, TempPte);

        Pfn1->PteAddress = PointerPte;
        Pfn1->PteFrame = MI_GET_PAGE_FRAME_FROM_PTE (MiGetPteAddress(PointerPte));
        if (StartPte == PointerPte) {
            Pfn1->u3.e1.StartOfAllocation = 1;
        }
        else {
            Pfn1->u3.e1.StartOfAllocation = 0;
        }
        Pfn1 += 1;
        PointerPte += 1;
        Page += 1;
        SizeInPages2 -= 1;
    } while (SizeInPages2 != 0);

    Pfn1 -= 1;
    Pfn1->u3.e1.EndOfAllocation = 1;
    UNLOCK_PFN2 (OldIrql2);
    ExUnlockPool (NonPagedPool, OldIrql);

    BaseAddress = MiGetVirtualAddressMappedByPte (StartPte);

#if 0
    MiInsertContiguousTag (BaseAddress,
                           SizeInPages << PAGE_SHIFT,
                           CallingAddress);
#endif

    ExInsertPoolTag (Tag,
                     BaseAddress,
                     SizeInPages << PAGE_SHIFT,
                     NonPagedPool);

    return BaseAddress;

Fail2:
    UNLOCK_PFN2 (OldIrql2);
    MiReleaseSystemPtes (PointerPte, (ULONG)SizeInPages, SystemPteSpace);

Fail1:
    ExUnlockPool (NonPagedPool, OldIrql);

    return NULL;
}

VOID
ExRemovePoolTag (
    ULONG Tag,
    PVOID Va,
    SIZE_T NumberOfBytes
    );

LOGICAL
MiFreeLowMemory (
    IN PVOID BaseAddress,
    IN ULONG Tag
    )

/*++

Routine Description:

    This is a special routine which returns allocated contiguous physical
    memory below 4GB on a PAE system that has been booted in test mode where
    all this memory has been made generally unavailable to all components.
    This lets us find problems with device drivers.

Arguments:

    BaseAddress - Supplies the base virtual address where the physical
                  address was previously mapped.

    Tag - Supplies the tag for this address.

Return Value:

    TRUE if the allocation was freed by this routine, FALSE if not.

Environment:

    Kernel mode, IRQL of APC_LEVEL or below.

--*/

{
    ULONG i;
    PFN_NUMBER Page;
    PFN_NUMBER StartPage;
    KIRQL OldIrql;
    KIRQL OldIrql2;
    KIRQL OldIrqlHyper;
    PMMPFN Pfn1;
    PFN_NUMBER SizeInPages;
    PMMPTE PointerPte;
    PMMPTE StartPte;
    PULONG TempVa;

    PAGED_CODE();

    PointerPte = MiGetPteAddress (BaseAddress);
    StartPte = PointerPte;

    ASSERT (PointerPte->u.Hard.Valid == 1);

    Page = MI_GET_PAGE_FRAME_FROM_PTE (PointerPte);

    //
    // Only free allocations here that really were obtained from the low pool.
    //

    if (Page >= 0x100000) {
        return FALSE;
    }

    StartPage = Page;
    Pfn1 = MI_PFN_ELEMENT (Page);

    SizeInPages = 0;

    OldIrql = ExLockPool (NonPagedPool);

    LOCK_PFN2 (OldIrql2);

    ASSERT (Pfn1->u3.e1.StartOfAllocation == 1);
    Pfn1->u3.e1.StartOfAllocation = 0;

    do {
        ASSERT (Pfn1->u3.e1.PageLocation == ActiveAndValid);
        ASSERT (Pfn1->u2.ShareCount == 1);
        ASSERT (Pfn1->OriginalPte.u.Long == MM_DEMAND_ZERO_WRITE_PTE);
        ASSERT (Pfn1->u3.e1.VerifierAllocation == 0);
        ASSERT (Pfn1->u3.e1.LargeSessionAllocation == 0);

        while (Pfn1->u3.e2.ReferenceCount != 1) {

            //
            // A driver is still transferring data even though the caller
            // is freeing the memory.   Wait a bit before restarting at
            // the beginning.
            //

            UNLOCK_PFN2 (OldIrql2);
            ExUnlockPool (NonPagedPool, OldIrql);

            KeDelayExecutionThread (KernelMode, FALSE, &MmShortTime);

            Pfn1 = MI_PFN_ELEMENT (StartPage);

#if DBG
            PointerPte = StartPte;
#endif
            Page = MI_GET_PAGE_FRAME_FROM_PTE (PointerPte);
            ASSERT (Page < 0x100000);

            SizeInPages = 0;
            OldIrql = ExLockPool (NonPagedPool);
            LOCK_PFN2 (OldIrql2);
        
            ASSERT (Pfn1->u3.e1.StartOfAllocation == 0);
            continue;
        }

        Pfn1->PteFrame = MI_MAGIC_4GB_RECLAIM;

        //
        // Fill the actual page with a recognizable data
        // pattern.  No one else should write to these
        // pages unless they are allocated for
        // a contiguous memory request.
        //

        TempVa = (PULONG)MiMapPageInHyperSpace (Page,
                                                &OldIrqlHyper);

        RtlFillMemoryUlong (TempVa,
                            PAGE_SIZE,
                            Page | MI_LOWMEM_MAGIC_BIT);

        MiUnmapPageInHyperSpace (OldIrqlHyper);

        SizeInPages += 1;

        if (Pfn1->u3.e1.EndOfAllocation == 1) {
            Pfn1->u3.e1.EndOfAllocation = 0;
            break;
        }

#if DBG
        PointerPte += 1;
        ASSERT (PointerPte->u.Hard.Valid == 1);
        ASSERT (MI_GET_PAGE_FRAME_FROM_PTE (PointerPte) == Page + 1);
#endif
        Page += 1;
        Pfn1 += 1;

    } while (TRUE);

    ASSERT (RtlAreBitsClear (MiLowMemoryBitMap, StartPage, SizeInPages) == TRUE);
    RtlSetBits (MiLowMemoryBitMap, StartPage, SizeInPages);

    //
    // No need to update ResidentAvailable or commit as these pages were
    // never added to either.
    //

    MmAllocatedNonPagedPool -= SizeInPages;
    NonPagedPoolDescriptor.TotalBigPages -= (ULONG)SizeInPages;

    UNLOCK_PFN2 (OldIrql2);

    MiReleaseSystemPtes (StartPte, (ULONG)SizeInPages, SystemPteSpace);

    ExUnlockPool (NonPagedPool, OldIrql);

    ExRemovePoolTag (Tag,
                     BaseAddress,
                     SizeInPages << PAGE_SHIFT);

    return TRUE;
}

LOGICAL
MiCheckPhysicalPagePattern (
    IN PFN_NUMBER PageFrameIndex,
    IN OUT PULONG CorruptionOffset OPTIONAL
    )

/*++

Routine Description:

    This is a special function called only from the kernel debugger
    to check that the physical memory below 4Gb removed with /NOLOWMEM
    contains the expected fill patterns.  If not, there is a high
    probability that a driver which cannot handle physical addresses greater
    than 32 bits corrupted the memory.

Arguments:

    PageFrameIndex - Supplies the physical page number to check.
    
    CorruptionOffset - If non-NULL and corruption is found, the byte offset
                       of the corruption start is returned here.

Return Value:

    TRUE if the page was removed and the fill pattern is correct, or
    if the page was never removed.  FALSE if corruption was detected
    in the page.

Environment:

    This routine is for use of the kernel debugger ONLY, specifically
    the !chklowmem command.

    The debugger's PTE will be repointed.
    
--*/

{
    PMMPFN Pfn;
    PULONG Va;
    ULONG Index;
    PHYSICAL_ADDRESS Pa;

    ASSERT (MiNoLowMemory);

    if (MiLowMemoryBitMap == NULL) {
        return TRUE;
    }

    //
    // Verify that the page to be verified is one of the reclaimed
    // pages.
    //

    if ((PageFrameIndex >= MiLowMemoryBitMap->SizeOfBitMap) ||
        (RtlCheckBit (MiLowMemoryBitMap, PageFrameIndex) == 0)) {

        return TRUE;
    }

    //
    // At this point we have a low page that is not in active use.
    // The fill pattern must match.
    //

#if DBG
    Pfn = MI_PFN_ELEMENT (PageFrameIndex);
    ASSERT (Pfn->PteFrame == MI_MAGIC_4GB_RECLAIM);
    ASSERT (Pfn->u3.e1.PageLocation == ActiveAndValid);
#endif

    //
    // Map the physical page using the debug PTE so the
    // fill pattern can be validated.
    //
    // The debugger cannot be using this virtual address on entry or exit.
    // This mapping really should have been done in the kernel debugger source
    // NOT in the memory management sources.
    //

    Pa.QuadPart = ((ULONGLONG)PageFrameIndex) << PAGE_SHIFT;

    Va = (PULONG) MmDbgTranslatePhysicalAddress64 (Pa);

    for (Index = 0; Index < PAGE_SIZE / sizeof(ULONG); Index += 1) {

        if (*Va != (PageFrameIndex | MI_LOWMEM_MAGIC_BIT)) {

            if (CorruptionOffset != NULL) {
                *CorruptionOffset = Index * sizeof(ULONG);
            }
        
            return FALSE;
        }

        Va += 1;
    }

    return TRUE;
}
#endif
