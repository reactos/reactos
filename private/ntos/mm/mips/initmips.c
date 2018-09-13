/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    initmips.c

Abstract:

    This module contains the machine dependent initialization for the
    memory management component.  It is specifically tailored to the
    MIPS environment (both r3000 and r4000).

Author:

    Lou Perazzoli (loup) 3-Apr-1990

Revision History:

--*/

#include "mi.h"


VOID
MiInitMachineDependent (
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
    )

/*++

Routine Description:

    This routine performs the necessary operations to enable virtual
    memory.  This includes building the page directory page, building
    page table pages to map the code section, the data section, the'
    stack section and the trap handler.

    It also initializes the PFN database and populates the free list.


Arguments:

    None.

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
    ULONG i, j;
    ULONG HighPage;
    ULONG PagesLeft;
    ULONG PageNumber;
    ULONG PdePageNumber;
    ULONG PdePage;
    ULONG PageFrameIndex;
    ULONG NextPhysicalPage;
    ULONG PfnAllocation;
    ULONG MaxPool;
    KIRQL OldIrql;
    PEPROCESS CurrentProcess;
    ULONG DirBase;
    PVOID SpinLockPage;
    ULONG MostFreePage = 0;
    PLIST_ENTRY NextMd;
    PMEMORY_ALLOCATION_DESCRIPTOR FreeDescriptor;
    PMEMORY_ALLOCATION_DESCRIPTOR MemoryDescriptor;
    MMPTE TempPte;
    PMMPTE PointerPde;
    PMMPTE PointerPte;
    PMMPTE LastPte;
    PMMPTE CacheStackPage;
    PMMPTE Pde;
    PMMPTE StartPde;
    PMMPTE EndPde;
    PMMPFN Pfn1;
    PMMPFN Pfn2;
    PULONG PointerLong;
    PVOID NonPagedPoolStartVirtual;
    ULONG Range;

    //
    // Initialize color tables and cache policy fields of static data if R4000.
    //

    ValidKernelPte.u.Hard.CachePolicy = PCR->CachePolicy;
    ValidUserPte.u.Hard.CachePolicy = PCR->CachePolicy;
    ValidPtePte.u.Hard.CachePolicy = PCR->CachePolicy;
    ValidPdePde.u.Hard.CachePolicy = PCR->CachePolicy;
    ValidKernelPde.u.Hard.CachePolicy = PCR->CachePolicy ;

    MmProtectToPteMask[MM_READONLY] |= PCR->AlignedCachePolicy;
    MmProtectToPteMask[MM_EXECUTE] |= PCR->AlignedCachePolicy;
    MmProtectToPteMask[MM_EXECUTE_READ] |= PCR->AlignedCachePolicy;
    MmProtectToPteMask[MM_READWRITE] |= PCR->AlignedCachePolicy;
    MmProtectToPteMask[MM_WRITECOPY] |= PCR->AlignedCachePolicy;
    MmProtectToPteMask[MM_EXECUTE_READWRITE] |= PCR->AlignedCachePolicy;
    MmProtectToPteMask[MM_EXECUTE_WRITECOPY] |= PCR->AlignedCachePolicy;

    MmProtectToPteMask[MM_GUARD_PAGE | MM_READONLY] |= PCR->AlignedCachePolicy;
    MmProtectToPteMask[MM_GUARD_PAGE | MM_EXECUTE] |= PCR->AlignedCachePolicy;
    MmProtectToPteMask[MM_GUARD_PAGE | MM_EXECUTE_READ] |= PCR->AlignedCachePolicy;
    MmProtectToPteMask[MM_GUARD_PAGE | MM_READWRITE] |= PCR->AlignedCachePolicy;
    MmProtectToPteMask[MM_GUARD_PAGE | MM_WRITECOPY] |= PCR->AlignedCachePolicy;
    MmProtectToPteMask[MM_GUARD_PAGE | MM_EXECUTE_READWRITE] |= PCR->AlignedCachePolicy;
    MmProtectToPteMask[MM_GUARD_PAGE | MM_EXECUTE_WRITECOPY] |= PCR->AlignedCachePolicy;

    PointerPte = MiGetPdeAddress (PDE_BASE);

    PointerPte->u.Hard.Dirty = 1;
    PointerPte->u.Hard.Valid = 1;
    PointerPte->u.Hard.Global = 1;
    PointerPte->u.Hard.Write = 0;

    PdePageNumber = PointerPte->u.Hard.PageFrameNumber;

    PsGetCurrentProcess()->Pcb.DirectoryTableBase[0] = PointerPte->u.Long;

    KeSweepDcache (FALSE);

    //
    // Get the lower bound of the free physical memory and the
    // number of physical pages by walking the memory descriptor lists.
    //

    NextMd = LoaderBlock->MemoryDescriptorListHead.Flink;
    while (NextMd != &LoaderBlock->MemoryDescriptorListHead) {
        MemoryDescriptor = CONTAINING_RECORD(NextMd,
                                             MEMORY_ALLOCATION_DESCRIPTOR,
                                             ListEntry);

        MmNumberOfPhysicalPages += MemoryDescriptor->PageCount;
        if (MemoryDescriptor->BasePage < MmLowestPhysicalPage) {
            MmLowestPhysicalPage = MemoryDescriptor->BasePage;
        }

        //
        // If the memory range described by the descriptor is larger
        // than the previous largest range and the descriptor describes
        // memory that is in KSEG0, then record the address of the
        // descriptor.
        //

        HighPage = MemoryDescriptor->BasePage + MemoryDescriptor->PageCount - 1;
        if (MemoryDescriptor->MemoryType == LoaderFree) {
            if ((MemoryDescriptor->PageCount > MostFreePage) &&
                (HighPage < MM_PAGES_IN_KSEG0)) {
                MostFreePage = MemoryDescriptor->PageCount;
                FreeDescriptor = MemoryDescriptor;
            }
        }

        if (HighPage > MmHighestPhysicalPage) {
            MmHighestPhysicalPage = HighPage;
        }

        NextMd = MemoryDescriptor->ListEntry.Flink;
    }

    //
    // If the number of physical pages is less than 1024, then bug check.
    //

    if (MmNumberOfPhysicalPages < 1024) {
        KeBugCheckEx (INSTALL_MORE_MEMORY,
                      MmNumberOfPhysicalPages,
                      MmLowestPhysicalPage,
                      MmHighestPhysicalPage,
                      0);
    }

    //
    // Build non-paged pool using the physical pages following the
    // data page in which to build the pool from.  Non-page pool grows
    // from the high range of the virtual address space and expands
    // downward.
    //
    // At this time non-paged pool is constructed so virtual addresses
    // are also physically contiguous.
    //

    if ((MmSizeOfNonPagedPoolInBytes >> PAGE_SHIFT) >
                        (7 * (MmNumberOfPhysicalPages << 3))) {

        //
        // More than 7/8 of memory allocated to nonpagedpool, reset to 0.
        //

        MmSizeOfNonPagedPoolInBytes = 0;
    }

    if (MmSizeOfNonPagedPoolInBytes < MmMinimumNonPagedPoolSize) {

        //
        // Calculate the size of nonpaged pool.
        // Use the minimum size, then for every MB about 4mb add extra
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
        // Calculate the size of nonpaged pool.  If 4mb of less use
        // the minimum size, then for every MB about 4mb add extra
        // pages.
        //

        MmMaximumNonPagedPoolInBytes = MmDefaultMaximumNonPagedPool;

        //
        // Make sure enough expansion for pfn database exists.
        //

        MmMaximumNonPagedPoolInBytes += (ULONG)PAGE_ALIGN (
                                      MmHighestPhysicalPage * sizeof(MMPFN));

        MmMaximumNonPagedPoolInBytes +=
                        ((MmNumberOfPhysicalPages - 1024)/256) *
                        MmMaxAdditionNonPagedPoolPerMb;
    }

    MaxPool = MmSizeOfNonPagedPoolInBytes + PAGE_SIZE * 16 +
                                   (ULONG)PAGE_ALIGN (
                                        MmHighestPhysicalPage * sizeof(MMPFN));

    if (MmMaximumNonPagedPoolInBytes < MaxPool) {
        MmMaximumNonPagedPoolInBytes = MaxPool;
    }

    if (MmMaximumNonPagedPoolInBytes > MM_MAX_ADDITIONAL_NONPAGED_POOL) {
        MmMaximumNonPagedPoolInBytes = MM_MAX_ADDITIONAL_NONPAGED_POOL;
    }

    MmNonPagedPoolStart = (PVOID)((ULONG)MmNonPagedPoolEnd
                                      - (MmMaximumNonPagedPoolInBytes - 1));

    MmNonPagedPoolStart = (PVOID)PAGE_ALIGN(MmNonPagedPoolStart);
    NonPagedPoolStartVirtual = MmNonPagedPoolStart;


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
    // Set the global bit in all PDE's for system space.
    //

    StartPde = MiGetPdeAddress (MM_SYSTEM_SPACE_START);
    EndPde = MiGetPdeAddress (MM_SYSTEM_SPACE_END);

    while (StartPde <= EndPde) {
        if (StartPde->u.Hard.Global == 0) {

            //
            // Set the Global bit.
            //

            TempPte = *StartPde;
            TempPte.u.Hard.Global = 1;
            *StartPde = TempPte;
        }
        StartPde += 1;
    }

    StartPde = MiGetPdeAddress (MmNonPagedSystemStart);

    EndPde = MiGetPdeAddress((PVOID)((PCHAR)MmNonPagedPoolEnd - 1));

    ASSERT ((EndPde - StartPde) < (LONG)FreeDescriptor->PageCount);

    NextPhysicalPage = FreeDescriptor->BasePage;
    TempPte = ValidKernelPte;

    while (StartPde <= EndPde) {
        if (StartPde->u.Hard.Valid == 0) {

            //
            // Map in a page directory page.
            //

            TempPte.u.Hard.PageFrameNumber = NextPhysicalPage;
            NextPhysicalPage += 1;
            *StartPde = TempPte;

        }
        StartPde += 1;
    }

    //
    // Zero the PTEs before nonpaged pool.
    //

    StartPde = MiGetPteAddress (MmNonPagedSystemStart);
    PointerPte = MiGetPteAddress(MmNonPagedPoolStart);

    RtlZeroMemory (StartPde, ((ULONG)PointerPte - (ULONG)StartPde));

    //
    // Fill in the PTEs for non-paged pool.
    //

    LastPte = MiGetPteAddress((ULONG)MmNonPagedPoolStart +
                                        MmSizeOfNonPagedPoolInBytes - 1);
    while (PointerPte <= LastPte) {
        TempPte.u.Hard.PageFrameNumber = NextPhysicalPage;
        NextPhysicalPage += 1;
        *PointerPte = TempPte;
        PointerPte++;
    }

    ASSERT (NextPhysicalPage <
                       (FreeDescriptor->BasePage + FreeDescriptor->PageCount));

    //
    // Zero the remaining PTEs (if any).
    //

    StartPde = (PMMPTE)((ULONG)MiGetPteAddress (MmNonPagedPoolEnd) | (PAGE_SIZE - sizeof(MMPTE)));
    while (PointerPte <= StartPde) {
        *PointerPte = ZeroKernelPte;
        PointerPte++;
    }

    PointerPte = MiGetPteAddress (MmNonPagedPoolStart);
    MmNonPagedPoolStart =
        (PVOID)(KSEG0_BASE | (PointerPte->u.Hard.PageFrameNumber << PAGE_SHIFT));

    MmPageAlignedPoolBase[NonPagedPool] = MmNonPagedPoolStart;

    MmSubsectionBase = (ULONG)MmNonPagedPoolStart;
    if (NextPhysicalPage < (MM_SUBSECTION_MAP >> PAGE_SHIFT)) {
        MmSubsectionBase = KSEG0_BASE;
        MmSubsectionTopPage = MM_SUBSECTION_MAP >> PAGE_SHIFT;
    }

    //
    // Non-paged pages now exist, build the pool structures.
    //

    MmNonPagedPoolExpansionStart = (PVOID)((PCHAR)NonPagedPoolStartVirtual +
                    MmSizeOfNonPagedPoolInBytes);
    MiInitializeNonPagedPool (NonPagedPoolStartVirtual);

    //
    // Before Non-paged pool can be used, the PFN database must
    // be built.  This is due to the fact that the start and end of
    // allocation bits for nonpaged pool are maintained in the
    // PFN elements for the corresponding pages.
    //

    //
    // Calculate the number of pages required from page zero to
    // the highest page.
    //

    //
    // Get the number of secondary colors and add the arrary for tracking
    // secondary colors to the end of the PFN database.
    //
    // Get secondary color value from registry.
    //

    if (MmSecondaryColors == 0) {
        MmSecondaryColors = PCR->SecondLevelDcacheSize;
    }

    MmSecondaryColors = MmSecondaryColors >> PAGE_SHIFT;

    //
    // Make sure value is power of two and within limits.
    //

    if (((MmSecondaryColors & (MmSecondaryColors -1)) != 0) ||
        (MmSecondaryColors < MM_SECONDARY_COLORS_MIN) ||
        (MmSecondaryColors > MM_SECONDARY_COLORS_MAX)) {
        MmSecondaryColors = MM_SECONDARY_COLORS_DEFAULT;
    }

    MmSecondaryColorMask = (MmSecondaryColors - 1) & ~MM_COLOR_MASK;

    PfnAllocation = 1 + ((((MmHighestPhysicalPage + 1) * sizeof(MMPFN)) +
                        (MmSecondaryColors * sizeof(MMCOLOR_TABLES)*2))
                            >> PAGE_SHIFT);

    //
    // If the number of pages remaining in the current descriptor is
    // greater than the number of pages needed for the PFN database,
    // then allocate the PFN database from the current free descriptor.
    //

    HighPage = FreeDescriptor->BasePage + FreeDescriptor->PageCount;
    PagesLeft = HighPage - NextPhysicalPage;
    if (PagesLeft >= PfnAllocation) {

        //
        // Allocate the PFN database in kseg0.
        //
        // Compute the address of the PFN by allocating the appropriate
        // number of pages from the end of the free descriptor.
        //

        PfnInKseg0 = TRUE;
        MmPfnDatabase = (PMMPFN)(KSEG0_BASE |
                                    ((HighPage - PfnAllocation) << PAGE_SHIFT));

        RtlZeroMemory(MmPfnDatabase, PfnAllocation * PAGE_SIZE);
        FreeDescriptor->PageCount -= PfnAllocation;

    } else {

        //
        // Allocate the PFN database in virtual memory.
        //
        // Calculate the start of the Pfn Database (it starts a physical
        // page zero, even if the Lowest physical page is not zero).
        //

        PfnInKseg0 = FALSE;
        PointerPte = MiReserveSystemPtes (PfnAllocation,
                                          NonPagedPoolExpansion,
                                          0,
                                          0,
                                          TRUE);

        MmPfnDatabase = (PMMPFN)(MiGetVirtualAddressMappedByPte (PointerPte));

        //
        // Go through the memory descriptors and for each physical page
        // make the PFN database has a valid PTE to map it.  This allows
        // machines with sparse physical memory to have a minimal PFN
        // database.
        //

        NextMd = LoaderBlock->MemoryDescriptorListHead.Flink;
        while (NextMd != &LoaderBlock->MemoryDescriptorListHead) {

            MemoryDescriptor = CONTAINING_RECORD(NextMd,
                                                 MEMORY_ALLOCATION_DESCRIPTOR,
                                                 ListEntry);

            PointerPte = MiGetPteAddress (MI_PFN_ELEMENT(
                                            MemoryDescriptor->BasePage));

            LastPte = MiGetPteAddress (((PCHAR)(MI_PFN_ELEMENT(
                                            MemoryDescriptor->BasePage +
                                            MemoryDescriptor->PageCount))) - 1);

            while (PointerPte <= LastPte) {
                if (PointerPte->u.Hard.Valid == 0) {
                    TempPte.u.Hard.PageFrameNumber = NextPhysicalPage;
                    NextPhysicalPage += 1;
                    *PointerPte = TempPte;
                    RtlZeroMemory (MiGetVirtualAddressMappedByPte (PointerPte),
                                   PAGE_SIZE);
                }
                PointerPte++;
            }

            NextMd = MemoryDescriptor->ListEntry.Flink;
        }
    }

    //
    // Initialize support for colored pages.
    //

    MmFreePagesByColor[0] = (PMMCOLOR_TABLES)
                                &MmPfnDatabase[MmHighestPhysicalPage + 1];

    MmFreePagesByColor[1] = &MmFreePagesByColor[0][MmSecondaryColors];

    //
    // Make sure the PTEs are mapped.
    //

    if (!MI_IS_PHYSICAL_ADDRESS(MmFreePagesByColor[0])) {
        PointerPte = MiGetPteAddress (&MmFreePagesByColor[0][0]);

        LastPte = MiGetPteAddress (
              (PVOID)((PCHAR)&MmFreePagesByColor[1][MmSecondaryColors] - 1));

        while (PointerPte <= LastPte) {
            if (PointerPte->u.Hard.Valid == 0) {
                TempPte.u.Hard.PageFrameNumber = NextPhysicalPage;
                NextPhysicalPage += 1;
                *PointerPte = TempPte;
                RtlZeroMemory (MiGetVirtualAddressMappedByPte (PointerPte),
                               PAGE_SIZE);
            }
            PointerPte++;
        }
    }

    for (i = 0; i < MmSecondaryColors; i++) {
        MmFreePagesByColor[ZeroedPageList][i].Flink = MM_EMPTY_LIST;
        MmFreePagesByColor[FreePageList][i].Flink = MM_EMPTY_LIST;
    }

    for (i = 0; i < MM_MAXIMUM_NUMBER_OF_COLORS; i++) {
        MmFreePagesByPrimaryColor[ZeroedPageList][i].ListName = ZeroedPageList;
        MmFreePagesByPrimaryColor[FreePageList][i].ListName = FreePageList;
        MmFreePagesByPrimaryColor[ZeroedPageList][i].Flink = MM_EMPTY_LIST;
        MmFreePagesByPrimaryColor[FreePageList][i].Flink = MM_EMPTY_LIST;
        MmFreePagesByPrimaryColor[ZeroedPageList][i].Blink = MM_EMPTY_LIST;
        MmFreePagesByPrimaryColor[FreePageList][i].Blink = MM_EMPTY_LIST;
    }

    //
    // Go through the page table entries and for any page which is
    // valid, update the corresponding PFN database element.
    //

    PointerPde = MiGetPdeAddress (PTE_BASE);

    PdePage = PointerPde->u.Hard.PageFrameNumber;
    Pfn1 = MI_PFN_ELEMENT(PdePage);
    Pfn1->PteFrame = PdePage;
    Pfn1->PteAddress = PointerPde;
    Pfn1->u2.ShareCount += 1;
    Pfn1->u3.e2.ReferenceCount = 1;
    Pfn1->u3.e1.PageLocation = ActiveAndValid;
    Pfn1->u3.e1.PageColor = MI_GET_PAGE_COLOR_FROM_PTE (PointerPde);

    //
    // Add the pages which were used to construct nonpaged pool to
    // the pfn database.
    //

    Pde = MiGetPdeAddress ((ULONG)NonPagedPoolStartVirtual -
                                ((MmNumberOfSystemPtes + 1) * PAGE_SIZE));

    EndPde = MiGetPdeAddress(MmNonPagedPoolEnd);

    while (Pde <= EndPde) {
        if (Pde->u.Hard.Valid == 1) {
            PdePage = Pde->u.Hard.PageFrameNumber;
            Pfn1 = MI_PFN_ELEMENT(PdePage);
            Pfn1->PteFrame = PointerPde->u.Hard.PageFrameNumber;
            Pfn1->PteAddress = Pde;
            Pfn1->u2.ShareCount += 1;
            Pfn1->u3.e2.ReferenceCount = 1;
            Pfn1->u3.e1.PageLocation = ActiveAndValid;
            Pfn1->u3.e1.PageColor = MI_GET_PAGE_COLOR_FROM_PTE (Pde);

            PointerPte = MiGetVirtualAddressMappedByPte (Pde);
            for (j = 0 ; j < PTE_PER_PAGE; j++) {
                if (PointerPte->u.Hard.Valid == 1) {

                    PageFrameIndex = PointerPte->u.Hard.PageFrameNumber;
                    Pfn2 = MI_PFN_ELEMENT(PageFrameIndex);
                    Pfn2->PteFrame = PdePage;
                    Pfn2->u2.ShareCount += 1;
                    Pfn2->u3.e2.ReferenceCount = 1;
                    Pfn2->u3.e1.PageLocation = ActiveAndValid;
                    Pfn2->PteAddress =
                            (PMMPTE)(KSEG0_BASE | (PageFrameIndex << PTE_SHIFT));

                    Pfn2->u3.e1.PageColor =
                             MI_GET_PAGE_COLOR_FROM_PTE (Pfn2->PteAddress);
                }
                PointerPte++;
            }
        }
        Pde++;
    }

    //
    // If page zero is still unused, mark it as in use. This is
    // temporary as we want to find bugs where a physical page
    // is specified as zero.
    //

    Pfn1 = &MmPfnDatabase[MmLowestPhysicalPage];
    if (Pfn1->u3.e2.ReferenceCount == 0) {

        //
        // Make the reference count non-zero and point it into a
        // page directory.
        //

        Pde = MiGetPdeAddress (0xb0000000);
        PdePage = Pde->u.Hard.PageFrameNumber;
        Pfn1->PteFrame = PdePageNumber;
        Pfn1->PteAddress = Pde;
        Pfn1->u2.ShareCount += 1;
        Pfn1->u3.e2.ReferenceCount = 1;
        Pfn1->u3.e1.PageLocation = ActiveAndValid;
        Pfn1->u3.e1.PageColor = MI_GET_PAGE_COLOR_FROM_PTE (Pde);
    }

    // end of temporary set to physical page zero.


    //
    //
    // Walk through the memory descriptors and add pages to the
    // free list in the PFN database.
    //

    NextMd = LoaderBlock->MemoryDescriptorListHead.Flink;

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

                Pfn1 = MI_PFN_ELEMENT (NextPhysicalPage);
                while (i != 0) {
                    if (Pfn1->u3.e2.ReferenceCount == 0) {

                        //
                        // Set the PTE address to the phyiscal page for
                        // virtual address alignment checking.
                        //

                        Pfn1->PteAddress = (PMMPTE)(NextPhysicalPage << PTE_SHIFT);
                        Pfn1->u3.e1.PageColor = MI_GET_PAGE_COLOR_FROM_PTE (
                                                    Pfn1->PteAddress);

                        MiInsertPageInList (MmPageLocationList[FreePageList],
                                            NextPhysicalPage);
                    }
                    Pfn1++;
                    i -= 1;
                    NextPhysicalPage += 1;
                }
                break;

            default:
                PointerPte = MiGetPteAddress(KSEG0_BASE |
                                            (NextPhysicalPage << PAGE_SHIFT));

                Pfn1 = MI_PFN_ELEMENT (NextPhysicalPage);
                while (i != 0) {

                    //
                    // Set page as in use.
                    //

                    Pfn1->PteFrame = PdePageNumber;
                    Pfn1->PteAddress = PointerPte;
                    Pfn1->u2.ShareCount += 1;
                    Pfn1->u3.e2.ReferenceCount = 1;
                    Pfn1->u3.e1.PageLocation = ActiveAndValid;
                    Pfn1->u3.e1.PageColor = MI_GET_PAGE_COLOR_FROM_PTE (
                                                    PointerPte);

                    Pfn1++;
                    i -= 1;
                    NextPhysicalPage += 1;
                    PointerPte += 1;
                }

                break;
        }

        NextMd = MemoryDescriptor->ListEntry.Flink;
    }

    //
    // If the PFN database is allocated in virtual memory, then indicate that
    // the PFN database is allocated in NonPaged pool. Otherwise, scan the PFN
    // database for holes and insert the respective pages in the free page list.
    //

    if (PfnInKseg0 == FALSE) {

        //
        // The PFN database is allocated in virtual memory.
        //
        // Set the start and end of allocation.
        //

        Pfn1 = MI_PFN_ELEMENT(MiGetPteAddress(&MmPfnDatabase[MmLowestPhysicalPage])->u.Hard.PageFrameNumber);
        Pfn1->u3.e1.StartOfAllocation = 1;
        Pfn1 = MI_PFN_ELEMENT(MiGetPteAddress(&MmPfnDatabase[MmHighestPhysicalPage])->u.Hard.PageFrameNumber);
        Pfn1->u3.e1.EndOfAllocation = 1;

    } else {

        //
        // The PFN database is allocated in KSEG0.
        //
        // Mark all pfn entries for the pfn pages in use.
        //

        PageNumber = ((ULONG)MmPfnDatabase - KSEG0_BASE) >> PAGE_SHIFT;
        Pfn1 = MI_PFN_ELEMENT(PageNumber);
        do {
            Pfn1->PteAddress = (PMMPTE)(PageNumber << PTE_SHIFT);
            Pfn1->u3.e1.PageColor = MI_GET_PAGE_COLOR_FROM_PTE(Pfn1->PteAddress);
            Pfn1 += 1;
            PfnAllocation -= 1;
        } while (PfnAllocation != 0);

        // Scan the PFN database backward for pages that are completely zero.
        // These pages are unused and can be added to the free list
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

                PageNumber = ((ULONG)BasePfn - KSEG0_BASE) >> PAGE_SHIFT;
                Pfn1 = MI_PFN_ELEMENT(PageNumber);

                ASSERT(Pfn1->u3.e2.ReferenceCount == 0);

                PfnAllocation += 1;

                Pfn1->PteAddress = (PMMPTE)(PageNumber << PTE_SHIFT);
                Pfn1->u3.e1.PageColor = MI_GET_PAGE_COLOR_FROM_PTE(Pfn1->PteAddress);
                MiInsertPageInList(MmPageLocationList[FreePageList],
                                   PageNumber);
            }

        } while (BottomPfn > MmPfnDatabase);
    }

    //
    // Indicate that nonpaged pool must succeed is allocated in
    // nonpaged pool.
    //

    i = MmSizeOfNonPagedMustSucceed;
    Pfn1 = MI_PFN_ELEMENT(MI_CONVERT_PHYSICAL_TO_PFN (MmNonPagedMustSucceed));
    while ((LONG)i > 0) {
        Pfn1->u3.e1.StartOfAllocation = 1;
        Pfn1->u3.e1.EndOfAllocation = 1;
        i -= PAGE_SIZE;
        Pfn1 += 1;
    }

    KeInitializeSpinLock (&MmSystemSpaceLock);
    KeInitializeSpinLock (&MmPfnLock);

    //
    // Initialize the nonpaged available PTEs for mapping I/O space
    // and kernel stacks.
    //

    PointerPte = MiGetPteAddress ((ULONG)NonPagedPoolStartVirtual -
                                ((MmNumberOfSystemPtes + 1) * PAGE_SIZE));

    PointerPte = (PMMPTE)PAGE_ALIGN (PointerPte);
    if (PfnInKseg0) {
        MmNumberOfSystemPtes = MiGetPteAddress(MmNonPagedPoolExpansionStart) - PointerPte - 1;
    } else {
        MmNumberOfSystemPtes = MiGetPteAddress(NonPagedPoolStartVirtual) - PointerPte - 1;
    }

    MiInitializeSystemPtes (PointerPte, MmNumberOfSystemPtes, SystemPteSpace);

    //
    // Initialize the nonpaged pool.
    //

    InitializePool(NonPagedPool,0);

    //
    // Initialize memory management structures for this process.
    //

    //
    // Build working set list.  System initialization has created
    // a PTE for hyperspace.
    //
    // Note, we can't remove a zeroed page as hyper space does not
    // exist and we map non-zeroed pages into hyper space to zero.
    //

    PointerPte = MiGetPdeAddress(HYPER_SPACE);

    ASSERT (PointerPte->u.Hard.Valid == 1);
    PointerPte->u.Hard.Global = 0;
    PointerPte->u.Hard.Write = 1;
    PageFrameIndex = PointerPte->u.Hard.PageFrameNumber;

    //
    // Point to the page table page we just created and zero it.
    //


//    KeFillEntryTb ((PHARDWARE_PTE)PointerPte,
//                    MiGetPteAddress(HYPER_SPACE),
//                    TRUE);

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
    // The pfn element for the page directory has already been initialized,
    // zero the reference count and the share count so they won't be
    // wrong.
    //

    Pfn1 = MI_PFN_ELEMENT (PdePageNumber);
    Pfn1->u2.ShareCount = 0;
    Pfn1->u3.e2.ReferenceCount = 0;
    Pfn1->u3.e1.PageColor = 0;

    //
    // The pfn element for the PDE which maps hyperspace has already
    // been initialized, zero the reference count and the share count
    // so they won't be wrong.
    //

    Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);
    Pfn1->u2.ShareCount = 0;
    Pfn1->u3.e2.ReferenceCount = 0;
    Pfn1->u3.e1.PageColor = 1;


    CurrentProcess = PsGetCurrentProcess ();

    //
    // Get a page for the working set list and map it into the Page
    // directory at the page after hyperspace.
    //

    PointerPte = MiGetPteAddress (HYPER_SPACE);
    PageFrameIndex = MiRemoveAnyPage (MI_GET_PAGE_COLOR_FROM_PTE(PointerPte));
    CurrentProcess->WorkingSetPage = PageFrameIndex;

    TempPte.u.Hard.Global = 0;
    TempPte.u.Hard.PageFrameNumber = PageFrameIndex;
    PointerPde = MiGetPdeAddress (HYPER_SPACE) + 1;

    //
    // Assert that the double mapped pages have the same alignment.
    //

    ASSERT ((PointerPte->u.Long & (0xF << PTE_SHIFT)) ==
            (PointerPde->u.Long & (0xF << PTE_SHIFT)));

    *PointerPde = TempPte;

    PointerPte = MiGetVirtualAddressMappedByPte (PointerPde);

    KeFillEntryTb ((PHARDWARE_PTE)PointerPde,
                    PointerPte,
                    TRUE);

    RtlZeroMemory ((PVOID)PointerPte, PAGE_SIZE);

    TempPte = *PointerPde;
    TempPte.u.Hard.Valid = 0;
    TempPte.u.Hard.Global = 0;

    KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);
    KeFlushSingleTb (PointerPte,
                     TRUE,
                     FALSE,
                     (PHARDWARE_PTE)PointerPde,
                     TempPte.u.Hard);

    KeLowerIrql(OldIrql);

#ifdef R4000

    //
    // Initialize hyperspace for this process.
    //

    i = NUMBER_OF_MAPPING_PTES - MM_COLOR_MASK;
    PointerPte = MmFirstReservedMappingPte;
    while (PointerPte <= (MmFirstReservedMappingPte + MM_COLOR_MASK)) {
        PointerPte->u.Hard.PageFrameNumber = i;
        PointerPte += 1;
    }

#endif

    CurrentProcess->Vm.MaximumWorkingSetSize = MmSystemProcessWorkingSetMax;
    CurrentProcess->Vm.MinimumWorkingSetSize = MmSystemProcessWorkingSetMin;

    MmInitializeProcessAddressSpace (CurrentProcess,
                                (PEPROCESS)NULL,
                                (PVOID)NULL);

    *PointerPde = ZeroPte;
    KeFlushEntireTb(TRUE, TRUE);

    //
    // Check to see if moving the secondary page structures to the end
    // of the PFN database is a waste of memory.  And if so, copy it
    // to paged pool.
    //
    // If the PFN datbase ends on a page aligned boundary and the
    // size of the two arrays is less than a page, free the page
    // and allocate nonpagedpool for this.
    //

    if ((((ULONG)MmFreePagesByColor[0] & (PAGE_SIZE - 1)) == 0) &&
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

        if (!MI_IS_PHYSICAL_ADDRESS(c)) {
            PointerPte = MiGetPteAddress(c);
            PageFrameIndex = PointerPte->u.Hard.PageFrameNumber;
            *PointerPte = ZeroKernelPte;
        } else {
            PageFrameIndex = MI_CONVERT_PHYSICAL_TO_PFN (c);
        }

        Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);
        ASSERT ((Pfn1->u3.e2.ReferenceCount <= 1) && (Pfn1->u2.ShareCount <= 1));
        Pfn1->u2.ShareCount = 0;
        Pfn1->u3.e2.ReferenceCount = 0;
        MI_SET_PFN_DELETED (Pfn1);
#if DBG
        Pfn1->u3.e1.PageLocation = StandbyPageList;
#endif //DBG
        MiInsertPageInList (MmPageLocationList[FreePageList], PageFrameIndex);
    }
    return;
}
