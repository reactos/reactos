/*++

Copyright (c) 1990  Microsoft Corporation
Copyright (c) 1992  Digital Equipment Corporation

Module Name:

    initalpha.c

Abstract:

    This module contains the machine dependent initialization for the
    memory management component. It is specifically tailored to the
    ALPHA architecture.

Author:

    Lou Perazzoli (loup) 3-Apr-1990
    Joe Notarangelo  23-Apr-1992    ALPHA version

Revision History:

    Landy Wang (landyw) 02-June-1998 : Modifications for full 3-level 64-bit NT.

--*/

#include "mi.h"
#include <inbv.h>

//
// Local definitions.
//

#define _1mbInPages  (0x100000 >> PAGE_SHIFT)
#define _4gbInPages (0x100000000 >> PAGE_SHIFT)

//
// Local data.
//

PMEMORY_ALLOCATION_DESCRIPTOR MxFreeDescriptor;
PFN_NUMBER MxNextPhysicalPage;
PFN_NUMBER MxNumberOfPages;

PFN_NUMBER
MxGetNextPage (
    VOID
    )

/*++

Routine Description:

    This function returns the next physical page number from either the
    largest low memory descritor or the largest free descriptor. If there
    are no physical pages left, then a bugcheck is executed since the
    system cannot be initialized.

Arguments:

    LoaderBlock - Supplies the address of the loader block.

Return Value:

    None.

Environment:

    Kernel mode.

--*/

{

    //
    // If there are free pages left in the current descriptor, then
    // return the next physical page. Otherwise, attempt to switch
    // descriptors.
    //

    if (MxNumberOfPages != 0) {
        MxNumberOfPages -= 1;
        return MxNextPhysicalPage++;

    } else {

        //
        // If the current descriptor is not the largest free descriptor,
        // then switch to the largest free descriptor. Otherwise, bugcheck.
        //

        if (MxNextPhysicalPage ==
                (MxFreeDescriptor->BasePage + MxFreeDescriptor->PageCount)) {
            KeBugCheckEx(INSTALL_MORE_MEMORY,
                         MmNumberOfPhysicalPages,
                         MmLowestPhysicalPage,
                         MmHighestPhysicalPage,
                         0);

            return 0;

        } else {
            MxNumberOfPages = MxFreeDescriptor->PageCount - 1;
            MxNextPhysicalPage = MxFreeDescriptor->BasePage;
            return MxNextPhysicalPage++;
        }
    }
}

VOID
MiInitMachineDependent (
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
    )

/*++

Routine Description:

    This routine performs the necessary operations to enable virtual
    memory. This includes building the page directory parent pages and
    the page directories for the system, building page table pages to map
    the code section, the data section, the stack section and the trap handler.
    It also initializes the PFN database and populates the free list.

Arguments:

    LoaderBlock - Supplies the address of the loader block.

Return Value:

    None.

Environment:

    Kernel mode.

--*/

{
    LOGICAL First;
    CHAR Buffer[256];
    PMMPFN BasePfn;
    PMMPFN BottomPfn;
    PMMPFN TopPfn;
    PFN_NUMBER i;
    ULONG j;
    PFN_NUMBER HighPage;
    PFN_NUMBER PagesLeft;
    PFN_NUMBER PageNumber;
    PFN_NUMBER PtePage;
    PFN_NUMBER PdePage;
    PFN_NUMBER PpePage;
    PFN_NUMBER FrameNumber;
    PFN_NUMBER PfnAllocation;
    PEPROCESS CurrentProcess;
    PVOID SpinLockPage;
    PFN_NUMBER MostFreePage;
    PFN_NUMBER MostFreeLowMem;
    PLIST_ENTRY NextMd;
    SIZE_T MaxPool;
    PFN_NUMBER NextPhysicalPage;
    KIRQL OldIrql;
    PMEMORY_ALLOCATION_DESCRIPTOR FreeDescriptorLowMem;
    PMEMORY_ALLOCATION_DESCRIPTOR MemoryDescriptor;
    MMPTE TempPte;
    PMMPTE PointerPde;
    PMMPTE PointerPte;
    PMMPTE LastPte;
    PMMPTE CacheStackPage;
    PMMPTE Pde;
    PMMPTE StartPpe;
    PMMPTE StartPde;
    PMMPTE StartPte;
    PMMPTE EndPpe;
    PMMPTE EndPde;
    PMMPTE EndPte;
    PMMPFN Pfn1;
    PMMPFN Pfn2;
    PULONG PointerLong;
    PMMFREE_POOL_ENTRY Entry;
    PVOID NonPagedPoolStartVirtual;
    ULONG Range;

    MostFreePage = 0;
    MostFreeLowMem = 0;
    FreeDescriptorLowMem = NULL;

    //
    // Get the lower bound of the free physical memory and the number of
    // physical pages by walking the memory descriptor lists. In addition,
    // find the memory descriptor with the most free pages that is within
    // the first 4gb of physical memory. This memory can be used to allocate
    // common buffers for use by PCI devices that cannot address more than
    // 32 bits. Also find the largest free memory descriptor.
    //

    //
    // When restoring a hibernation image, OS Loader needs to use "a few" extra
    // pages of LoaderFree memory.
    // This is not accounted for when reserving memory for hibernation below.
    // Start with a safety margin to allow for this plus modest future increase.
    //

    MmHiberPages = 96;

    NextMd = LoaderBlock->MemoryDescriptorListHead.Flink;
    while (NextMd != &LoaderBlock->MemoryDescriptorListHead) {
        MemoryDescriptor = CONTAINING_RECORD(NextMd,
                                             MEMORY_ALLOCATION_DESCRIPTOR,
                                             ListEntry);

        HighPage = MemoryDescriptor->BasePage + MemoryDescriptor->PageCount - 1;

        //
        // This check results in /BURNMEMORY chunks not being counted.
        //

        if (MemoryDescriptor->MemoryType != LoaderBad) {
            MmNumberOfPhysicalPages += (PFN_COUNT)MemoryDescriptor->PageCount;
        }

        //
        // If the lowest page is lower than the lowest page encountered
        // so far, then set the new low page number.
        //

        if (MemoryDescriptor->BasePage < MmLowestPhysicalPage) {
            MmLowestPhysicalPage = MemoryDescriptor->BasePage;
        }

        //
        // If the highest page is greater than the highest page encountered
        // so far, then set the new high page number.
        //

        if (HighPage > MmHighestPhysicalPage) {
            MmHighestPhysicalPage = HighPage;
        }

        //
        // Locate the largest free block starting below 4GB and the largest
        // free block.
        //

        if ((MemoryDescriptor->MemoryType == LoaderFree) ||
            (MemoryDescriptor->MemoryType == LoaderLoadedProgram) ||
            (MemoryDescriptor->MemoryType == LoaderFirmwareTemporary) ||
            (MemoryDescriptor->MemoryType == LoaderOsloaderStack)) {

            //
            // Every page that will be used as free memory that is not already
            // marked as LoaderFree must be counted so a hibernate can reserve
            // the proper amount.
            //

            if (MemoryDescriptor->MemoryType != LoaderFree) {
                MmHiberPages += MemoryDescriptor->PageCount;
            }

            if ((MemoryDescriptor->PageCount > MostFreeLowMem) &&
                (MemoryDescriptor->BasePage < _4gbInPages) &&
                (HighPage < _4gbInPages)) {
                MostFreeLowMem = MemoryDescriptor->PageCount;
                FreeDescriptorLowMem = MemoryDescriptor;

            } else if (MemoryDescriptor->PageCount > MostFreePage) {
                MostFreePage = MemoryDescriptor->PageCount;
                MxFreeDescriptor = MemoryDescriptor;
            }
        } else if (MemoryDescriptor->MemoryType == LoaderOsloaderHeap) {
            //
            // We do not want to use this memory yet as it still has important
            // data structures in it. But we still want to account for this in
            // the hibernation pages
            //
            MmHiberPages += MemoryDescriptor->PageCount;
        }

        NextMd = MemoryDescriptor->ListEntry.Flink;
    }

    MmHighestPossiblePhysicalPage = MmHighestPhysicalPage;

    //
    // Perform sanity checks on the results of walking the memory
    // descriptors.
    //
    // If the number of physical pages is less that 1024 (i.e., 8mb), then
    // bugcheck. There is not enough memory to run the system.
    //

    if (MmNumberOfPhysicalPages < 1024) {
        KeBugCheckEx(INSTALL_MORE_MEMORY,
                     MmNumberOfPhysicalPages,
                     MmLowestPhysicalPage,
                     MmHighestPhysicalPage,
                     0);
    }

    //
    // If there is no free descriptor below 4gb, then it is not possible to
    // run devices that only support 32 address bits. It is also highly
    // unlikely that the configuration data is correct so bugcheck.
    //

    if (FreeDescriptorLowMem == NULL) {
        InbvDisplayString("MmInit *** FATAL ERROR *** no free memory below 4gb\n");
        KeBugCheck(MEMORY_MANAGEMENT);
    }

    //
    // Set the initial nonpaged frame allocation parameters.
    //

    MxNextPhysicalPage = FreeDescriptorLowMem->BasePage;
    MxNumberOfPages = FreeDescriptorLowMem->PageCount;

    //
    // Compute the initial and maximum size of nonpaged pool. The initial
    // allocation of nonpaged pool is such that it is both virtually and
    // physically contiguous.
    //
    // If the size of the initial nonpaged pool was initialized from the
    // registry and is greater than 7/8 of physical memory, then force the
    // size of the initial nonpaged pool to be computed.
    //

    if ((MmSizeOfNonPagedPoolInBytes >> PAGE_SHIFT) >
                                    (7 * (MmNumberOfPhysicalPages >> 3))) {
        MmSizeOfNonPagedPoolInBytes = 0;
    }

    //
    // If the size of the initial nonpaged pool is less than the minimum
    // amount, then compute the size of initial nonpaged pool as the minimum
    // size up to 8mb and a computed amount for every 1mb thereafter.
    //

    if (MmSizeOfNonPagedPoolInBytes < MmMinimumNonPagedPoolSize) {
        MmSizeOfNonPagedPoolInBytes = MmMinimumNonPagedPoolSize;
        if (MmNumberOfPhysicalPages > 1024) {
            MmSizeOfNonPagedPoolInBytes +=
                ((MmNumberOfPhysicalPages - 1024) / _1mbInPages) *
                                                MmMinAdditionNonPagedPoolPerMb;
        }
    }

    //
    // Align the size of the initial nonpaged pool to page size boundary.
    //

    MmSizeOfNonPagedPoolInBytes &= ~(PAGE_SIZE - 1);

    //
    // Limit initial nonpaged pool size to the maximum allowable size.
    //

    if (MmSizeOfNonPagedPoolInBytes > MM_MAX_INITIAL_NONPAGED_POOL) {
        MmSizeOfNonPagedPoolInBytes = MM_MAX_INITIAL_NONPAGED_POOL;
    }

    //
    // If the computed size of the initial nonpaged pool will not fit in the
    // largest low memory descriptor, then recompute the size of nonpaged pool
    // to be the size of the largest low memory descriptor. If the largest
    // low memory descriptor does not contain the minimum initial nonpaged
    // pool size, then the system cannot be booted.
    //

    if ((MmSizeOfNonPagedPoolInBytes >> PAGE_SHIFT) > MxNumberOfPages) {

        //
        // Reserve all of low memory for nonpaged pool.
        //

        MmSizeOfNonPagedPoolInBytes = MxNumberOfPages << PAGE_SHIFT;
        if(MmSizeOfNonPagedPoolInBytes < MmMinimumNonPagedPoolSize) {
           InbvDisplayString("MmInit *** FATAL ERROR *** cannot allocate nonpaged pool\n");
           sprintf(Buffer,
                   "Largest description = %d pages, require %d pages\n",
                   MxNumberOfPages,
                   MmMinimumNonPagedPoolSize >> PAGE_SHIFT);

           InbvDisplayString(Buffer);
           KeBugCheck(MEMORY_MANAGEMENT);
        }
    }

    //
    // Reserve the physically and virtually contiguous memory that maps
    // the initial nonpaged pool and set page frame allocation parameters.
    //

    MxNextPhysicalPage += (PFN_NUMBER)(MmSizeOfNonPagedPoolInBytes >> PAGE_SHIFT);
    MxNumberOfPages -= (PFN_NUMBER)(MmSizeOfNonPagedPoolInBytes >> PAGE_SHIFT);

    //
    // Calculate the maximum size of nonpaged pool.
    //

    if (MmMaximumNonPagedPoolInBytes == 0) {

        //
        // Calculate the size of nonpaged pool. If 8mb or less use the
        // minimum size, then for every MB above 8mb add extra pages.
        //

        MmMaximumNonPagedPoolInBytes = MmDefaultMaximumNonPagedPool;

        //
        // Make sure enough expansion for PFN database exists.
        //

        MmMaximumNonPagedPoolInBytes +=
            ((ULONG_PTR)PAGE_ALIGN((MmHighestPhysicalPage + 1) * sizeof(MMPFN)));

        //
        // If the number of physical pages is greater than 8mb, then compute
        // an additional amount for every 1mb thereafter.
        //

        if (MmNumberOfPhysicalPages > 1024) {
            MmMaximumNonPagedPoolInBytes +=
                ((MmNumberOfPhysicalPages - 1024) / _1mbInPages) *
                                                MmMaxAdditionNonPagedPoolPerMb;
        }

        //
        // If the maximum size of nonpaged pool is greater than the maximum
        // default size of nonpaged pool, then limit the maximum size of
        // onopaged pool to the maximum default size.
        //

        if (MmMaximumNonPagedPoolInBytes > MM_MAX_DEFAULT_NONPAGED_POOL) {
            MmMaximumNonPagedPoolInBytes = MM_MAX_DEFAULT_NONPAGED_POOL;
        }
    }

    //
    // Align the maximum size of nonpaged pool to page size boundary.
    //

    MmMaximumNonPagedPoolInBytes &= ~(PAGE_SIZE - 1);

    //
    // Compute the maximum size of nonpaged pool to include 16 additional
    // pages and enough space to map the PFN database.
    //

    MaxPool = MmSizeOfNonPagedPoolInBytes + (PAGE_SIZE * 16) +
            ((ULONG_PTR)PAGE_ALIGN((MmHighestPhysicalPage + 1) * sizeof(MMPFN)));

    //
    // If the maximum size of nonpaged pool is less than the computed
    // maximum size of nonpaged pool, then set the maximum size of nonpaged
    // pool to the computed maximum size.
    //

    if (MmMaximumNonPagedPoolInBytes < MaxPool) {
        MmMaximumNonPagedPoolInBytes = MaxPool;
    }

    //
    // Limit maximum nonpaged pool to MM_MAX_ADDITIONAL_NONPAGED_POOL.
    //

    if (MmMaximumNonPagedPoolInBytes > MM_MAX_ADDITIONAL_NONPAGED_POOL) {
        MmMaximumNonPagedPoolInBytes = MM_MAX_ADDITIONAL_NONPAGED_POOL;
    }

    //
    // Compute the starting address of nonpaged pool.
    //

    MmNonPagedPoolStart = (PCHAR)MmNonPagedPoolEnd - MmMaximumNonPagedPoolInBytes;
    NonPagedPoolStartVirtual = MmNonPagedPoolStart;

    //
    // Calculate the starting address for nonpaged system space rounded
    // down to a second level PDE mapping boundary.
    //

    MmNonPagedSystemStart = (PVOID)(((ULONG_PTR)MmNonPagedPoolStart -
                                (((ULONG_PTR)MmNumberOfSystemPtes + 1) * PAGE_SIZE)) &
                                                        (~PAGE_DIRECTORY2_MASK));

    //
    // Limit the starting address of system space to the lowest allowable
    // address for nonpaged system space.
    //

    if (MmNonPagedSystemStart < MM_LOWEST_NONPAGED_SYSTEM_START) {
        MmNonPagedSystemStart = MM_LOWEST_NONPAGED_SYSTEM_START;
    }

    //
    // Recompute the actual number of system PTEs.
    //

    MmNumberOfSystemPtes = (ULONG)(((ULONG_PTR)MmNonPagedPoolStart -
                            (ULONG_PTR)MmNonPagedSystemStart) >> PAGE_SHIFT) - 1;

    ASSERT(MmNumberOfSystemPtes > 1000);

    //
    // Set the global bit for all PPEs and PDEs in system space.
    //

    StartPde = MiGetPdeAddress(MM_SYSTEM_SPACE_START);
    EndPde = MiGetPdeAddress(MM_SYSTEM_SPACE_END);
    First = TRUE;

    while (StartPde <= EndPde) {

        if (First == TRUE || MiIsPteOnPdeBoundary(StartPde)) {
            First = FALSE;
            StartPpe = MiGetPteAddress(StartPde);
            if (StartPpe->u.Hard.Valid == 0) {
                StartPpe += 1;
                StartPde = MiGetVirtualAddressMappedByPte (StartPpe);
                continue;
            }
            TempPte = *StartPpe;
            TempPte.u.Hard.Global = 1;
            *StartPpe = TempPte;
        }

        TempPte = *StartPde;
        TempPte.u.Hard.Global = 1;
        *StartPde = TempPte;
        StartPde += 1;
    }

    //
    // If HYDRA, then reset the global bit for all PPE & PDEs in session space.
    //

    if (MiHydra == TRUE) {

        StartPde = MiGetPdeAddress(MmSessionBase);
        EndPde = MiGetPdeAddress(MI_SESSION_SPACE_END);
        First = TRUE;

        while (StartPde < EndPde) {

            if (First == TRUE || MiIsPteOnPdeBoundary(StartPde)) {
                First = FALSE;
                StartPpe = MiGetPteAddress(StartPde);
                if (StartPpe->u.Hard.Valid == 0) {
                    StartPpe += 1;
                    StartPde = MiGetVirtualAddressMappedByPte (StartPpe);
                    continue;
                }
                TempPte = *StartPpe;
                TempPte.u.Hard.Global = 0;
                *StartPpe = TempPte;
            }

            TempPte = *StartPde;
            TempPte.u.Hard.Global = 0;
            *StartPde = TempPte;

            ASSERT(StartPde->u.Long == 0);

            StartPde += 1;
        }
    }

    //
    // Allocate a page directory and a pair of page table pages.
    // Map the hyper space page directory page into the top level parent
    // directory & the hyper space page table page into the page directory
    // and map an additional page that will eventually be used for the
    // working set list.  Page tables after the first two are set up later
    // on during individual process working set initialization.
    //
    // The working set list page will eventually be a part of hyper space.
    // It is mapped into the second level page directory page so it can be
    // zeroed and so it will be accounted for in the PFN database. Later
    // the page will be unmapped, and its page frame number captured in the
    // system process object.
    //

    TempPte = ValidKernelPte;
    TempPte.u.Hard.Global = 0;

    StartPde = MiGetPdeAddress(HYPER_SPACE);
    StartPpe = MiGetPteAddress(StartPde);

    if (StartPpe->u.Hard.Valid == 0) {
        ASSERT (StartPpe->u.Long == 0);
        TempPte.u.Hard.PageFrameNumber = MxGetNextPage();
        *StartPpe = TempPte;
        RtlZeroMemory (MiGetVirtualAddressMappedByPte (StartPpe),
                       PAGE_SIZE);
    }

    TempPte.u.Hard.PageFrameNumber = MxGetNextPage();
    *StartPde = TempPte;

    //
    // Zero the hyper space page table page.
    //

    StartPte = MiGetPteAddress(HYPER_SPACE);
    RtlZeroMemory(StartPte, PAGE_SIZE);

    //
    // Allocate page directory and page table pages for
    // system PTEs and nonpaged pool.
    //

    TempPte = ValidKernelPte;
    StartPde = MiGetPdeAddress(MmNonPagedSystemStart);
    EndPde = MiGetPdeAddress(MmNonPagedPoolEnd);
    First = TRUE;

    while (StartPde <= EndPde) {

        if (First == TRUE || MiIsPteOnPdeBoundary(StartPde)) {
            First = FALSE;
            StartPpe = MiGetPteAddress(StartPde);
            if (StartPpe->u.Hard.Valid == 0) {
                TempPte.u.Hard.PageFrameNumber = MxGetNextPage();
                *StartPpe = TempPte;
                RtlZeroMemory (MiGetVirtualAddressMappedByPte (StartPpe),
                               PAGE_SIZE);
            }
        }

        if (StartPde->u.Hard.Valid == 0) {
            TempPte.u.Hard.PageFrameNumber = MxGetNextPage();
            *StartPde = TempPte;
        }
        StartPde += 1;
    }

    //
    // Zero the PTEs that map the nonpaged region just before nonpaged pool.
    //

    StartPte = MiGetPteAddress(MmNonPagedSystemStart);
    EndPte = MiGetPteAddress(MmNonPagedPoolEnd);

    if (!MiIsPteOnPdeBoundary (EndPte)) {
        EndPte = (PMMPTE)((ULONG_PTR)PAGE_ALIGN (EndPte) + PAGE_SIZE);
    }

    RtlZeroMemory(StartPte, (ULONG_PTR)EndPte - (ULONG_PTR)StartPte);

    //
    // Fill in the PTEs to cover the initial nonpaged pool. The physical
    // page frames to cover this range were reserved earlier from the
    // largest low memory free descriptor. The initial allocation is both
    // physically and virtually contiguous.
    //

    StartPte = MiGetPteAddress(MmNonPagedPoolStart);
    EndPte = MiGetPteAddress((PCHAR)MmNonPagedPoolStart +
                                                MmSizeOfNonPagedPoolInBytes);

    PageNumber = FreeDescriptorLowMem->BasePage;

#if 0
    ASSERT (MxFreeDescriptor == FreeDescriptorLowMem);
    MxNumberOfPages -= (EndPte - StartPte);
    MxNextPhysicalPage += (EndPte - StartPte);
#endif

    while (StartPte < EndPte) {
        TempPte.u.Hard.PageFrameNumber = PageNumber;
        PageNumber += 1;
        *StartPte = TempPte;
        StartPte += 1;
    }

    //
    // Zero the remaining PTEs (if any) for the initial nonpaged pool up to
    // the end of the current page table page.
    //

    while (!MiIsPteOnPdeBoundary (StartPte)) {
        *StartPte = ZeroKernelPte;
        StartPte += 1;
    }

    //
    // Convert the starting nonpaged pool address to a 43-bit superpage
    // address and set the address of the initial nonpaged pool allocation.
    //

    PointerPte = MiGetPteAddress(MmNonPagedPoolStart);
    MmNonPagedPoolStart = KSEG_ADDRESS(PointerPte->u.Hard.PageFrameNumber);
    MmPageAlignedPoolBase[NonPagedPool] = MmNonPagedPoolStart;

    //
    // Set subsection base to the address to zero (the PTE format allows the
    // complete address space to be spanned) and the top subsection page.
    //

    MmSubsectionBase = 0;
    MmSubsectionTopPage = (KSEG2_BASE - KSEG0_BASE) >> PAGE_SHIFT;

    //
    // Initialize the pool structures in the nonpaged memory just mapped.
    //

    MmNonPagedPoolExpansionStart =
                (PCHAR)NonPagedPoolStartVirtual + MmSizeOfNonPagedPoolInBytes;

    MiInitializeNonPagedPool ();

    //
    // Before Nonpaged pool can be used, the PFN database must be built.
    // This is due to the fact that the start and ending allocation bits
    // for nonpaged pool are stored in the PFN elements for the corresponding
    // pages.
    //
    // Calculate the number of pages required from page zero to the highest
    // page.
    //
    // Get the number of secondary colors and add the array for tracking
    // secondary colors to the end of the PFN database.
    //

    if (MmSecondaryColors == 0) {
        MmSecondaryColors = PCR->SecondLevelCacheSize;
    }

    MmSecondaryColors = MmSecondaryColors >> PAGE_SHIFT;

    //
    // Make sure value is power of two and within limits.
    //

    if (((MmSecondaryColors & (MmSecondaryColors - 1)) != 0) ||
        (MmSecondaryColors < MM_SECONDARY_COLORS_MIN) ||
        (MmSecondaryColors > MM_SECONDARY_COLORS_MAX)) {
        MmSecondaryColors = MM_SECONDARY_COLORS_DEFAULT;
    }

    MmSecondaryColorMask = MmSecondaryColors - 1;
    PfnAllocation =
        1 + ((((MmHighestPhysicalPage + 1) * sizeof(MMPFN)) +
            ((PFN_NUMBER)MmSecondaryColors * sizeof(MMCOLOR_TABLES) * 2)) >> PAGE_SHIFT);

    //
    // If the number of pages remaining in the current descriptor is
    // greater than the number of pages needed for the PFN database,
    // then allocate the PFN database from the current free descriptor.
    // Otherwise, allocate the PFN database virtually.
    //

#ifndef PFN_CONSISTENCY

    if (MxNumberOfPages >= PfnAllocation) {

        //
        // Allocate the PFN database in the 43-bit superpage.
        //
        // Compute the address of the PFN by allocating the appropriate
        // number of pages from the end of the free descriptor.
        //

        HighPage = MxNextPhysicalPage + MxNumberOfPages;
        MmPfnDatabase = KSEG_ADDRESS(HighPage - PfnAllocation);
        RtlZeroMemory(MmPfnDatabase, PfnAllocation * PAGE_SIZE);

        //
        // Mark off the chunk of memory used for the PFN database from
        // either the largest low free memory descriptor or the largest
        // free memory descriptor.
        //
        // N.B. The PFN database size is subtracted from the appropriate
        //      memory descriptor so it will not appear to be free when
        //      the memory descriptors are scanned to initialize the PFN
        //      database.
        //

        MxNumberOfPages -= PfnAllocation;
        if ((MxNextPhysicalPage >= FreeDescriptorLowMem->BasePage) &&
            (MxNextPhysicalPage < (FreeDescriptorLowMem->BasePage +
                                          FreeDescriptorLowMem->PageCount))) {
            FreeDescriptorLowMem->PageCount -= (PFN_COUNT)PfnAllocation;

        } else {
            MxFreeDescriptor->PageCount -= (PFN_COUNT)PfnAllocation;
        }

        //
        // Allocate one PTE at the very top of nonpaged pool. This provides
        // protection against the caller of the first real nonpaged expansion allocation in case he accidentally overruns his
        // pool block.  (We'll trap instead of corrupting the PFN database).
        // This also allows us to freely increment in MiFreePoolPages without
        // having to worry about a valid PTE after the end of the highest
        // nonpaged pool allocation.
        //

        MiReserveSystemPtes(1, NonPagedPoolExpansion, 0, 0, TRUE);

    } else {

#endif // PFN_CONSISTENCY

        //
        // Calculate the start of the PFN database (it starts at physical
        // page zero, even if the lowest physical page is not zero).
        //

        PointerPte = MiReserveSystemPtes((ULONG)PfnAllocation,
                                         NonPagedPoolExpansion,
                                         0,
                                         0,
                                         TRUE);

#if PFN_CONSISTENCY

        MiPfnStartPte = PointerPte;
        MiPfnPtes = PfnAllocation;

#endif

        MmPfnDatabase = (PMMPFN)(MiGetVirtualAddressMappedByPte(PointerPte));

        //
        // Allocate one more PTE just below the PFN database. This provides
        // protection against the caller of the first real nonpaged
        // expansion allocation in case he accidentally overruns his pool
        // block. (We'll trap instead of corrupting the PFN database).
        // This also allows us to freely increment in MiFreePoolPages
        // without having to worry about a valid PTE just after the end of
        // the highest nonpaged pool allocation.
        //

        MiReserveSystemPtes(1, NonPagedPoolExpansion, 0, 0, TRUE);

        //
        // Go through the memory descriptors and for each physical page
        // make the PFN database have a valid PTE to map it. This allows
        // machines with sparse physical memory to have a minimal PFN
        // database.
        //

        NextMd = LoaderBlock->MemoryDescriptorListHead.Flink;
        while (NextMd != &LoaderBlock->MemoryDescriptorListHead) {
            MemoryDescriptor = CONTAINING_RECORD(NextMd,
                                                 MEMORY_ALLOCATION_DESCRIPTOR,
                                                 ListEntry);

            PointerPte = MiGetPteAddress(MI_PFN_ELEMENT(
                                         MemoryDescriptor->BasePage));

            HighPage = MemoryDescriptor->BasePage + MemoryDescriptor->PageCount;
            LastPte = MiGetPteAddress((PCHAR)MI_PFN_ELEMENT(HighPage) - 1);
            while (PointerPte <= LastPte) {
                if (PointerPte->u.Hard.Valid == 0) {
                    TempPte.u.Hard.PageFrameNumber = MxGetNextPage();
                    *PointerPte = TempPte;
                    RtlZeroMemory(MiGetVirtualAddressMappedByPte(PointerPte),
                                  PAGE_SIZE);
                }

                PointerPte += 1;
            }

            NextMd = MemoryDescriptor->ListEntry.Flink;
        }

#ifndef PFN_CONSISTENCY

    }

#endif // PFN_CONSISTENCY

    //
    // Initialize support for colored pages.
    //

    MmFreePagesByColor[0] =
                (PMMCOLOR_TABLES)&MmPfnDatabase[MmHighestPhysicalPage + 1];

    MmFreePagesByColor[1] = &MmFreePagesByColor[0][MmSecondaryColors];

    //
    // Make sure the color table are mapped if they are not physically
    // allocated.
    //

    if (MI_IS_PHYSICAL_ADDRESS(MmFreePagesByColor[0]) == FALSE) {
        PointerPte = MiGetPteAddress(&MmFreePagesByColor[0][0]);
        LastPte =
            MiGetPteAddress((PCHAR)&MmFreePagesByColor[1][MmSecondaryColors] - 1);

        while (PointerPte <= LastPte) {
            if (PointerPte->u.Hard.Valid == 0) {
                TempPte.u.Hard.PageFrameNumber = MxGetNextPage();
                *PointerPte = TempPte;
                RtlZeroMemory(MiGetVirtualAddressMappedByPte(PointerPte),
                              PAGE_SIZE);
            }

            PointerPte += 1;
        }
    }

    //
    // Initialize the secondary color free page listheads.
    //

    for (i = 0; i < MmSecondaryColors; i += 1) {
        MmFreePagesByColor[ZeroedPageList][i].Flink = MM_EMPTY_LIST;
        MmFreePagesByColor[FreePageList][i].Flink = MM_EMPTY_LIST;
    }

    //
    // Go through the page table entries and for any page which is valid,
    // update the corresponding PFN database element.
    //
    // Add the level one page directory parent page to the PFN database.
    //

    PointerPde = (PMMPTE)PDE_SELFMAP;
    PpePage = MI_GET_PAGE_FRAME_FROM_PTE(PointerPde);
    Pfn1 = MI_PFN_ELEMENT(PpePage);
    Pfn1->PteFrame = PpePage;
    Pfn1->PteAddress = PointerPde;
    Pfn1->u2.ShareCount += 1;
    Pfn1->u3.e2.ReferenceCount = 1;
    Pfn1->u3.e1.PageLocation = ActiveAndValid;
    Pfn1->u3.e1.PageColor =
            MI_GET_COLOR_FROM_SECONDARY(GET_PAGE_COLOR_FROM_PTE(PointerPde));

    //
    // Add the pages which were used to construct the nonpaged part of the
    // system, hyper space, and the system process working set list to the
    // PFN database.
    //
    // The scan begins at the start of hyper space so the hyper space page
    // table page and the working set list page will be accounted for in
    // the PFN database.
    //

    StartPde = MiGetPdeAddress(HYPER_SPACE);
    EndPde = MiGetPdeAddress(NON_PAGED_SYSTEM_END);
    First = TRUE;

    while (StartPde <= EndPde) {

        if (First == TRUE || MiIsPteOnPdeBoundary(StartPde)) {
            First = FALSE;
            StartPpe = MiGetPteAddress(StartPde);
            if (StartPpe->u.Hard.Valid == 0) {
                StartPpe += 1;
                StartPde = MiGetVirtualAddressMappedByPte (StartPpe);
                continue;
            }

            PdePage = MI_GET_PAGE_FRAME_FROM_PTE(StartPpe);

            Pfn1 = MI_PFN_ELEMENT(PdePage);
            Pfn1->PteFrame = PpePage;
            Pfn1->PteAddress = StartPde;
            Pfn1->u2.ShareCount += 1;
            Pfn1->u3.e2.ReferenceCount = 1;
            Pfn1->u3.e1.PageLocation = ActiveAndValid;
            Pfn1->u3.e1.PageColor =
                MI_GET_COLOR_FROM_SECONDARY(GET_PAGE_COLOR_FROM_PTE(StartPpe));
        }

        //
        // If the second level PDE entry is valid, then add the page to the
        // PFN database.
        //

        if (StartPde->u.Hard.Valid == 1) {

            PtePage = MI_GET_PAGE_FRAME_FROM_PTE(StartPde);
            Pfn1 = MI_PFN_ELEMENT(PtePage);
            Pfn1->PteFrame = PdePage;
            Pfn1->PteAddress = StartPde;
            Pfn1->u2.ShareCount += 1;
            Pfn1->u3.e2.ReferenceCount = 1;
            Pfn1->u3.e1.PageLocation = ActiveAndValid;
            Pfn1->u3.e1.PageColor =
                MI_GET_COLOR_FROM_SECONDARY(GET_PAGE_COLOR_FROM_PTE(StartPde));

            //
            // Scan the page table page for valid PTEs.
            //

            PointerPte = MiGetVirtualAddressMappedByPte(StartPde);

            if ((PointerPte < MiGetPteAddress (KSEG0_BASE)) ||
                (PointerPte >= MiGetPteAddress (KSEG2_BASE))) {

                for (j = 0 ; j < PTE_PER_PAGE; j += 1) {

                    //
                    // If the page table page is valid, then add the page
                    // to the PFN database.
                    //

                    if (PointerPte->u.Hard.Valid == 1) {
                        FrameNumber = MI_GET_PAGE_FRAME_FROM_PTE(PointerPte);
                        Pfn2 = MI_PFN_ELEMENT(FrameNumber);
                        Pfn2->PteFrame = PtePage;
                        Pfn2->PteAddress = (PMMPTE)KSEG_ADDRESS(PtePage) + j;
                        Pfn2->u2.ShareCount += 1;
                        Pfn2->u3.e2.ReferenceCount = 1;
                        Pfn2->u3.e1.PageLocation = ActiveAndValid;
                        Pfn2->u3.e1.PageColor =
                            MI_GET_COLOR_FROM_SECONDARY(GET_PAGE_COLOR_FROM_PTE(Pfn2->PteAddress));
                    }

                    PointerPte += 1;
                }
            }
        }

        StartPde += 1;
    }

    //
    // If the lowest physical page is still unused add it to the PFN
    // database by making its reference count nonzero and pointing
    // it to a second level page directory entry.
    //

    Pfn1 = &MmPfnDatabase[MmLowestPhysicalPage];
    if (Pfn1->u3.e2.ReferenceCount == 0) {
        Pde = MiGetPdeAddress(0xFFFFFFFFB0000000);
        PdePage = MI_GET_PAGE_FRAME_FROM_PTE(Pde);
        Pfn1->PteFrame = PdePage;
        Pfn1->PteAddress = Pde;
        Pfn1->u2.ShareCount += 1;
        Pfn1->u3.e2.ReferenceCount = 1;
        Pfn1->u3.e1.PageLocation = ActiveAndValid;
        Pfn1->u3.e1.PageColor =
                    MI_GET_COLOR_FROM_SECONDARY(GET_PAGE_COLOR_FROM_PTE(Pde));
    }

    //
    // Walk through the memory descriptors and add pages to the free list
    // in the PFN database as appropriate.
    //

    NextMd = LoaderBlock->MemoryDescriptorListHead.Flink;
    while (NextMd != &LoaderBlock->MemoryDescriptorListHead) {
        MemoryDescriptor = CONTAINING_RECORD(NextMd,
                                             MEMORY_ALLOCATION_DESCRIPTOR,
                                             ListEntry);

        //
        // Set the base page number and the number of pages and switch
        // on the memory type.
        //

        i = MemoryDescriptor->PageCount;
        NextPhysicalPage = MemoryDescriptor->BasePage;
        switch (MemoryDescriptor->MemoryType) {

            //
            // Bad pages are not usable and are placed on the bad
            // page list.
            //

        case LoaderBad:
            while (i != 0) {
                MiInsertPageInList(MmPageLocationList[BadPageList],
                                   NextPhysicalPage);

                i -= 1;
                NextPhysicalPage += 1;
            }

            break;

            //
            // Pages from descriptor types free, loaded program, firmware
            // temporary, and OS Loader stack are potentially free.
            //

        case LoaderFree:
        case LoaderLoadedProgram:
        case LoaderFirmwareTemporary:
        case LoaderOsloaderStack:
            Pfn1 = MI_PFN_ELEMENT(NextPhysicalPage);
            while (i != 0) {

                //
                // If the PFN database entry for the respective page
                // is not referenced, then insert the page in the free
                // page list.
                //

                if (Pfn1->u3.e2.ReferenceCount == 0) {
                    Pfn1->PteAddress = KSEG_ADDRESS(NextPhysicalPage);
                    Pfn1->u3.e1.PageColor =
                        MI_GET_COLOR_FROM_SECONDARY(GET_PAGE_COLOR_FROM_PTE(Pfn1->PteAddress));

                    MiInsertPageInList(MmPageLocationList[FreePageList],
                                        NextPhysicalPage);
                }

                Pfn1 += 1;
                i -= 1;
                NextPhysicalPage += 1;
            }

            break;

            //
            // All the remaining memory descriptor types represent memory
            // that has been already allocated and is not available.
            //

        default:
            PointerPte = KSEG_ADDRESS(NextPhysicalPage);
            Pfn1 = MI_PFN_ELEMENT(NextPhysicalPage);
            while (i != 0) {

                //
                // Set the page in use.
                //

                Pfn1->PteFrame = PpePage;
                Pfn1->PteAddress = PointerPte;
                Pfn1->u2.ShareCount += 1;
                Pfn1->u3.e2.ReferenceCount = 1;
                Pfn1->u3.e1.PageLocation = ActiveAndValid;
                Pfn1->u3.e1.PageColor =
                    MI_GET_COLOR_FROM_SECONDARY(GET_PAGE_COLOR_FROM_PTE(PointerPte));

                Pfn1 += 1;
                i -= 1;
                NextPhysicalPage += 1;
                PointerPte += 1;
            }

            break;
        }

        NextMd = MemoryDescriptor->ListEntry.Flink;
    }

    //
    // Everything has been accounted for except the PFN database.
    //

    if (MI_IS_PHYSICAL_ADDRESS(MmPfnDatabase) == FALSE) {

        //
        // The PFN database is allocated in virtual memory.
        //
        // Set the start and end of allocation in the PFN database.
        //

        Pfn1 = MI_PFN_ELEMENT(MiGetPteAddress(&MmPfnDatabase[MmLowestPhysicalPage])->u.Hard.PageFrameNumber);
        Pfn1->u3.e1.StartOfAllocation = 1;
        Pfn1 = MI_PFN_ELEMENT(MiGetPteAddress(&MmPfnDatabase[MmHighestPhysicalPage])->u.Hard.PageFrameNumber);
        Pfn1->u3.e1.EndOfAllocation = 1;

    } else {

        //
        // The PFN database is allocated in KSEG43.
        //
        // Mark all PFN entries for the PFN pages in use.
        //

        PageNumber = MI_CONVERT_PHYSICAL_TO_PFN(MmPfnDatabase);
        Pfn1 = MI_PFN_ELEMENT(PageNumber);
        do {
            Pfn1->PteAddress = KSEG_ADDRESS(PageNumber);
            Pfn1->u3.e1.PageColor =
                MI_GET_COLOR_FROM_SECONDARY(GET_PAGE_COLOR_FROM_PTE(Pfn1->PteAddress));

            Pfn1->u3.e2.ReferenceCount += 1;
            PageNumber += 1;
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

            if (((ULONG_PTR)BottomPfn & (PAGE_SIZE - 1)) != 0) {
                BasePfn = (PMMPFN)((ULONG_PTR)BottomPfn & ~(PAGE_SIZE - 1));
                TopPfn = BottomPfn + 1;

            } else {
                BasePfn = (PMMPFN)((ULONG_PTR)BottomPfn - PAGE_SIZE);
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

            Range = (ULONG)((ULONG_PTR)TopPfn - (ULONG_PTR)BottomPfn);
            if (RtlCompareMemoryUlong((PVOID)BottomPfn, Range, 0) == Range) {

                //
                // Set the PTE address to the physical page for virtual
                // address alignment checking.
                //

                PageNumber = (PFN_NUMBER)(((ULONG_PTR)BasePfn - KSEG43_BASE) >> PAGE_SHIFT);
                Pfn1 = MI_PFN_ELEMENT(PageNumber);

                ASSERT(Pfn1->u3.e2.ReferenceCount == 1);

                Pfn1->u3.e2.ReferenceCount = 0;
                PfnAllocation += 1;
                Pfn1->PteAddress = KSEG_ADDRESS(PageNumber);
                Pfn1->u3.e1.PageColor =
                    MI_GET_COLOR_FROM_SECONDARY(GET_PAGE_COLOR_FROM_PTE(Pfn1->PteAddress));

                MiInsertPageInList(MmPageLocationList[FreePageList],
                                   PageNumber);
            }

        } while (BottomPfn > MmPfnDatabase);
    }

    //
    // Indicate that nonpaged pool must succeed is allocated in nonpaged pool.
    //

    i = MmSizeOfNonPagedMustSucceed;
    Pfn1 = MI_PFN_ELEMENT(MI_CONVERT_PHYSICAL_TO_PFN(MmNonPagedMustSucceed));
    while (i != 0) {
        Pfn1->u3.e1.StartOfAllocation = 1;
        Pfn1->u3.e1.EndOfAllocation = 1;
        i -= PAGE_SIZE;
        Pfn1 += 1;
    }

    //
    // Recompute the number of system PTEs to include the virtual space
    // occupied by the initialize nonpaged pool allocation in KSEG43, and
    // initialize the nonpaged available PTEs for mapping I/O space and
    // kernel stacks.
    //

    PointerPte = MiGetPteAddress(MmNonPagedSystemStart);
    MmNumberOfSystemPtes = (ULONG)(MiGetPteAddress(MmNonPagedPoolExpansionStart) - PointerPte - 1);
    KeInitializeSpinLock(&MmSystemSpaceLock);
    KeInitializeSpinLock(&MmPfnLock);
    MiInitializeSystemPtes(PointerPte, MmNumberOfSystemPtes, SystemPteSpace);

    //
    // Initialize the nonpaged pool.
    //

    InitializePool(NonPagedPool, 0);

    //
    // Initialize memory management structures for the system process.
    //
    // Set the address of the first and last reserved PTE in hyper space.
    //

    MmFirstReservedMappingPte = MiGetPteAddress(FIRST_MAPPING_PTE);
    MmLastReservedMappingPte = MiGetPteAddress(LAST_MAPPING_PTE);

    //
    // Set the address of the start of the working set list and header.
    //

    MmWorkingSetList = WORKING_SET_LIST;
    MmWsle = (PMMWSLE)((PUCHAR)WORKING_SET_LIST + sizeof(MMWSL));

    //
    // The PFN element for the page directory parent will be initialized
    // a second time when the process address space is initialized. Therefore,
    // the share count and the reference count must be set to zero.
    //

    Pfn1 = MI_PFN_ELEMENT(MI_GET_PAGE_FRAME_FROM_PTE((PMMPTE)PDE_SELFMAP));
    Pfn1->u2.ShareCount = 0;
    Pfn1->u3.e2.ReferenceCount = 0;

    //
    // The PFN element for the hyper space page directory page will be
    // initialized a second time when the process address space is initialized.
    // Therefore, the share count and the reference count must be set to zero.
    //

    PointerPte = MiGetPpeAddress(HYPER_SPACE);
    Pfn1 = MI_PFN_ELEMENT(MI_GET_PAGE_FRAME_FROM_PTE(PointerPte));
    Pfn1->u2.ShareCount = 0;
    Pfn1->u3.e2.ReferenceCount = 0;

    //
    // The PFN elements for the hyper space page table page and working set list
    // page will be initialized a second time when the process address space
    // is initialized. Therefore, the share count and the reference must be
    // set to zero.
    //

    StartPde = MiGetPdeAddress(HYPER_SPACE);

    Pfn1 = MI_PFN_ELEMENT(MI_GET_PAGE_FRAME_FROM_PTE(StartPde));
    Pfn1->u2.ShareCount = 0;
    Pfn1->u3.e2.ReferenceCount = 0;

    //
    // Save the page frame number of the working set page in the system
    // process object and unmap the working set page from the second level
    // page directory page.
    //

    LOCK_PFN(OldIrql);

    FrameNumber = MiRemoveZeroPageIfAny (0);
    if (FrameNumber == 0) {
        FrameNumber = MiRemoveAnyPage (0);
        UNLOCK_PFN (OldIrql);
        MiZeroPhysicalPage (FrameNumber, 0);
        LOCK_PFN (OldIrql);

        Pfn1 = MI_PFN_ELEMENT(FrameNumber);
        Pfn1->u2.ShareCount = 0;
        Pfn1->u3.e2.ReferenceCount = 0;
    }

    CurrentProcess = PsGetCurrentProcess();
    CurrentProcess->WorkingSetPage = FrameNumber;
    PointerPte = MiGetVirtualAddressMappedByPte(EndPde);

    UNLOCK_PFN(OldIrql);

    //
    // Initialize the system process memory management structures including
    // the working set list.
    //

    PointerPte = MmFirstReservedMappingPte;
    PointerPte->u.Hard.PageFrameNumber = NUMBER_OF_MAPPING_PTES;
    CurrentProcess->Vm.MaximumWorkingSetSize = (ULONG)MmSystemProcessWorkingSetMax;
    CurrentProcess->Vm.MinimumWorkingSetSize = (ULONG)MmSystemProcessWorkingSetMin;

    MmInitializeProcessAddressSpace(CurrentProcess, NULL, NULL, NULL);

    //
    // Check to see if moving the secondary page structures to the end
    // of the PFN database is a waste of memory.  And if so, copy it
    // to paged pool.
    //
    // If the PFN database ends on a page aligned boundary and the
    // size of the two arrays is less than a page, free the page
    // and allocate nonpagedpool for this.
    //

    if ((((ULONG_PTR)MmFreePagesByColor[0] & (PAGE_SIZE - 1)) == 0) &&
       ((MmSecondaryColors * 2 * sizeof(MMCOLOR_TABLES)) < PAGE_SIZE)) {

        PMMCOLOR_TABLES c;

        c = MmFreePagesByColor[0];
        MmFreePagesByColor[0] =
            ExAllocatePoolWithTag(NonPagedPoolMustSucceed,
                                  MmSecondaryColors * 2 * sizeof(MMCOLOR_TABLES),
                                  '  mM');

        MmFreePagesByColor[1] = &MmFreePagesByColor[0][MmSecondaryColors];

        RtlMoveMemory (MmFreePagesByColor[0],
                       c,
                       MmSecondaryColors * 2 * sizeof(MMCOLOR_TABLES));

        //
        // Free the page.
        //

        if (!MI_IS_PHYSICAL_ADDRESS(c)) {
            PointerPte = MiGetPteAddress(c);
            FrameNumber = MI_GET_PAGE_FRAME_FROM_PTE(PointerPte);
            *PointerPte = ZeroKernelPte;

        } else {
            FrameNumber = MI_CONVERT_PHYSICAL_TO_PFN(c);
        }

        LOCK_PFN (OldIrql);

        Pfn1 = MI_PFN_ELEMENT (FrameNumber);

        ASSERT ((Pfn1->u3.e2.ReferenceCount <= 1) && (Pfn1->u2.ShareCount <= 1));

        Pfn1->u2.ShareCount = 0;
        Pfn1->u3.e2.ReferenceCount = 0;
        MI_SET_PFN_DELETED(Pfn1);

#if DBG

        Pfn1->u3.e1.PageLocation = StandbyPageList;

#endif //DBG

        MiInsertPageInList (MmPageLocationList[FreePageList], FrameNumber);

        UNLOCK_PFN (OldIrql);
    }

    return;
}
