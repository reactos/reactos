/*++

Copyright (c) 1990  Microsoft Corporation
Copyright (c) 1993  IBM Corporation

Module Name:

    initppc.c

Abstract:

    This module contains the machine dependent initialization for the
    memory management component.  It is specifically tailored to the
    PowerPC environment.

Author:

    Lou Perazzoli (loup) 3-Apr-1990

    Modified for PowerPC by Mark Mergen (mergen@watson.ibm.com) 8-Oct-1993

Revision History:

--*/

#include "mi.h"

//
// Local definitions
//

#define _16MB ((16*1024*1024)/PAGE_SIZE)


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

    ULONG i, j;
    ULONG HighPage;
    ULONG PdePageNumber;
    ULONG PdePage;
    ULONG PageFrameIndex;
    ULONG NextPhysicalPage;
    ULONG PfnAllocation;
    ULONG NumberOfPages;
    ULONG MaxPool;
    KIRQL OldIrql;
    PEPROCESS CurrentProcess;
    ULONG DirBase;
    ULONG MostFreePage = 0;
    ULONG MostFreeLowMem = 0;
    PLIST_ENTRY NextMd;
    PMEMORY_ALLOCATION_DESCRIPTOR FreeDescriptor = NULL;
    PMEMORY_ALLOCATION_DESCRIPTOR FreeDescriptorLowMem = NULL;
    PMEMORY_ALLOCATION_DESCRIPTOR MemoryDescriptor;
    MMPTE TempPte;
    PMMPTE PointerPde;
    PMMPTE PointerPte;
    PMMPTE LastPte;
    PMMPTE Pde;
    PMMPTE StartPde;
    PMMPTE EndPde;
    PMMPFN Pfn1;
    PMMPFN Pfn2;
    ULONG va;

    PointerPte = MiGetPdeAddress (PDE_BASE);

// N.B. this will cause first HPT miss fault, DSI in real0.s should fix it!
    PdePageNumber = PointerPte->u.Hard.PageFrameNumber;

    DirBase = PdePageNumber << PAGE_SHIFT;

    PsGetCurrentProcess()->Pcb.DirectoryTableBase[0] = DirBase;

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

        HighPage = MemoryDescriptor->BasePage + MemoryDescriptor->PageCount - 1;
        MmNumberOfPhysicalPages += MemoryDescriptor->PageCount;

        if (MemoryDescriptor->BasePage < MmLowestPhysicalPage) {
            MmLowestPhysicalPage = MemoryDescriptor->BasePage;
        }

        if (HighPage > MmHighestPhysicalPage) {
            MmHighestPhysicalPage = HighPage;
        }

	    //
        // Locate the largest free block and the largest free block below 16MB.
        //

        if ((MemoryDescriptor->MemoryType == LoaderFree) ||
            (MemoryDescriptor->MemoryType == LoaderLoadedProgram) ||
            (MemoryDescriptor->MemoryType == LoaderFirmwareTemporary) ||
            (MemoryDescriptor->MemoryType == LoaderOsloaderStack)) {

            if ((MemoryDescriptor->BasePage < _16MB) &&
                (MostFreeLowMem < MemoryDescriptor->PageCount) &&
                (MostFreeLowMem < ((ULONG)_16MB - MemoryDescriptor->BasePage))) {

                MostFreeLowMem = (ULONG)_16MB - MemoryDescriptor->BasePage;
                if (MemoryDescriptor->PageCount < MostFreeLowMem) {
                    MostFreeLowMem = MemoryDescriptor->PageCount;
                }
                FreeDescriptorLowMem = MemoryDescriptor;

            } else if (MemoryDescriptor->PageCount > MostFreePage) {

                MostFreePage = MemoryDescriptor->PageCount;
                FreeDescriptor = MemoryDescriptor;
            }
        }

        NextMd = MemoryDescriptor->ListEntry.Flink;
    }

    //
    // This printout must be updated when the HAL goes to unicode
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

    StartPde = MiGetPdeAddress (MmNonPagedSystemStart);

    EndPde = MiGetPdeAddress((PVOID)((PCHAR)MmNonPagedPoolEnd - 1));

    ASSERT ((ULONG)(EndPde - StartPde) < FreeDescriptorLowMem->PageCount);

    //
    // Start building the nonpaged pool with the largest free chunk of memory
    // below 16MB.
    //

    NextPhysicalPage = FreeDescriptorLowMem->BasePage;
    NumberOfPages = FreeDescriptorLowMem->PageCount;
    TempPte = ValidKernelPte;

    while (StartPde <= EndPde) {
        if (StartPde->u.Hard.Valid == 0) {

            //
            // Map in a page directory page.
            //

            TempPte.u.Hard.PageFrameNumber = NextPhysicalPage;
            NextPhysicalPage += 1;
            NumberOfPages -= 1;
            *StartPde = TempPte;

        }
        StartPde += 1;
    }

    ASSERT(NumberOfPages > 0);

    //
    // Zero the PTEs before nonpaged pool.
    //

    StartPde = MiGetPteAddress(MmNonPagedSystemStart);
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
        NumberOfPages -= 1;
        if (NumberOfPages == 0) {
            ASSERT (NextPhysicalPage != (FreeDescriptor->BasePage +
                                         FreeDescriptor->PageCount));
            NextPhysicalPage = FreeDescriptor->BasePage;
            NumberOfPages = FreeDescriptor->PageCount;
        }
        *PointerPte = TempPte;
        PointerPte++;
    }

    //
    // Zero the remaining PTEs (if any).
    //

    while (((ULONG)PointerPte & (PAGE_SIZE - 1)) != 0) {
        *PointerPte = ZeroKernelPte;
        PointerPte++;
    }

    MmPageAlignedPoolBase[NonPagedPool] = MmNonPagedPoolStart;

    //
    // Non-paged pages now exist, build the pool structures.
    //

    MmNonPagedPoolExpansionStart = (PVOID)((PCHAR)MmNonPagedPoolStart +
                    MmSizeOfNonPagedPoolInBytes);
    MiInitializeNonPagedPool (MmNonPagedPoolStart);

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
    // Get the number of secondary colors and add the arrary for tracking
    // secondary colors to the end of the PFN database.
    //

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
    // Calculate the start of the Pfn Database (it starts a physical
    // page zero, even if the Lowest physical page is not zero).
    //

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
                NumberOfPages -= 1;
                if (NumberOfPages == 0) {
                    ASSERT (NextPhysicalPage != (FreeDescriptor->BasePage +
                                                 FreeDescriptor->PageCount));
                    NextPhysicalPage = FreeDescriptor->BasePage;
                    NumberOfPages = FreeDescriptor->PageCount;
                }
                *PointerPte = TempPte;
                RtlZeroMemory (MiGetVirtualAddressMappedByPte (PointerPte),
                               PAGE_SIZE);
            }
            PointerPte++;
        }
        NextMd = MemoryDescriptor->ListEntry.Flink;
    }

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

#if MM_MAXIMUM_NUMBER_OF_COLORS > 1
    for (i = 0; i < MM_MAXIMUM_NUMBER_OF_COLORS; i++) {
        MmFreePagesByPrimaryColor[ZeroedPageList][i].ListName = ZeroedPageList;
        MmFreePagesByPrimaryColor[FreePageList][i].ListName = FreePageList;
        MmFreePagesByPrimaryColor[ZeroedPageList][i].Flink = MM_EMPTY_LIST;
        MmFreePagesByPrimaryColor[FreePageList][i].Flink = MM_EMPTY_LIST;
        MmFreePagesByPrimaryColor[ZeroedPageList][i].Blink = MM_EMPTY_LIST;
        MmFreePagesByPrimaryColor[FreePageList][i].Blink = MM_EMPTY_LIST;
    }
#endif

    //
    // Go through the page table entries and for any page which is
    // valid, update the corresponding PFN database element.
    //

    Pde = MiGetPdeAddress (NULL);
    PointerPde = MiGetPdeAddress (PTE_BASE);
    va = 0;

    for (i = 0; i < PDE_PER_PAGE; i++) {
        if (Pde->u.Hard.Valid == 1) {

            PdePage = Pde->u.Hard.PageFrameNumber;
            Pfn1 = MI_PFN_ELEMENT(PdePage);
            Pfn1->PteFrame = PointerPde->u.Hard.PageFrameNumber;
            Pfn1->PteAddress = Pde;
            Pfn1->u2.ShareCount += 1;
            Pfn1->u3.e2.ReferenceCount = 1;
            Pfn1->u3.e1.PageLocation = ActiveAndValid;
            Pfn1->u3.e1.PageColor = MI_GET_COLOR_FROM_SECONDARY(
                                        MI_GET_PAGE_COLOR_FROM_PTE (Pde));

            PointerPte = MiGetPteAddress (va);

            for (j = 0 ; j < PTE_PER_PAGE; j++) {
                if (PointerPte->u.Hard.Valid == 1) {

                    Pfn1->u2.ShareCount += 1;

                    PageFrameIndex = PointerPte->u.Hard.PageFrameNumber;

                    if (PageFrameIndex <= MmHighestPhysicalPage) {

                        Pfn2 = MI_PFN_ELEMENT(PageFrameIndex);

                        if (MmIsAddressValid(Pfn2) &&
                             MmIsAddressValid((PUCHAR)(Pfn2+1)-1)) {

                            Pfn2->PteFrame = PdePage;
                            Pfn2->PteAddress = PointerPte;
                            Pfn2->u2.ShareCount += 1;
                            Pfn2->u3.e2.ReferenceCount = 1;
                            Pfn2->u3.e1.PageLocation = ActiveAndValid;
                            Pfn2->u3.e1.PageColor = MI_GET_COLOR_FROM_SECONDARY(
                                                        MI_GET_PAGE_COLOR_FROM_PTE (
                                                            PointerPte));
                        }
                    }
                }
                va += PAGE_SIZE;
                PointerPte++;
            }
        } else {
            va += (ULONG)PDE_PER_PAGE * (ULONG)PAGE_SIZE;
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
        Pfn1->u3.e1.PageColor = MI_GET_COLOR_FROM_SECONDARY(
                                    MI_GET_PAGE_COLOR_FROM_PTE (Pde));
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

                        Pfn1->PteAddress =
                                        (PMMPTE)(NextPhysicalPage << PTE_SHIFT);

                        Pfn1->u3.e1.PageColor = MI_GET_COLOR_FROM_SECONDARY(
                                                    MI_GET_PAGE_COLOR_FROM_PTE (
                                                        Pfn1->PteAddress));
                        MiInsertPageInList (MmPageLocationList[FreePageList],
                                            NextPhysicalPage);
                    }
                    Pfn1++;
                    i -= 1;
                    NextPhysicalPage += 1;
                }
                break;

            default:

                PointerPte = MiGetPteAddress (KSEG0_BASE +
                                            (NextPhysicalPage << PAGE_SHIFT));
                Pfn1 = MI_PFN_ELEMENT (NextPhysicalPage);
                while (i != 0) {

                    //
                    // Set page as in use.
                    //

                    if (Pfn1->u3.e2.ReferenceCount == 0) {
                        Pfn1->PteFrame = PdePageNumber;
                        Pfn1->PteAddress = PointerPte;
                        Pfn1->u2.ShareCount += 1;
                        Pfn1->u3.e2.ReferenceCount = 1;
                        Pfn1->u3.e1.PageLocation = ActiveAndValid;
                        Pfn1->u3.e1.PageColor = MI_GET_COLOR_FROM_SECONDARY(
                                                    MI_GET_PAGE_COLOR_FROM_PTE (
                                                        PointerPte));
                    }
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
    // Indicate that the PFN database is allocated in NonPaged pool.
    //

    Pfn1 = MI_PFN_ELEMENT(MiGetPteAddress(&MmPfnDatabase[MmLowestPhysicalPage])->u.Hard.PageFrameNumber);
    Pfn1->u3.e1.StartOfAllocation = 1;
    Pfn1 = MI_PFN_ELEMENT(MiGetPteAddress(&MmPfnDatabase[MmHighestPhysicalPage])->u.Hard.PageFrameNumber);
    Pfn1->u3.e1.EndOfAllocation = 1;

    //
    // Indicate that nonpaged pool must succeed is allocated in
    // nonpaged pool.
    //

    i = MmSizeOfNonPagedMustSucceed;
    Pfn1 = MI_PFN_ELEMENT(MiGetPteAddress(MmNonPagedMustSucceed)->u.Hard.PageFrameNumber);

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

    PointerPte = MiGetPteAddress (MmNonPagedSystemStart);

    PointerPte = (PMMPTE)PAGE_ALIGN (PointerPte);

    MmNumberOfSystemPtes = MiGetPteAddress(MmNonPagedPoolStart) - PointerPte - 1;

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
    PointerPte->u.Hard.Write = 1;
    PageFrameIndex = PointerPte->u.Hard.PageFrameNumber;

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

    TempPte.u.Hard.PageFrameNumber = PageFrameIndex;
    PointerPde = MiGetPdeAddress (HYPER_SPACE) + 1;

    *PointerPde = TempPte;

    PointerPte = MiGetVirtualAddressMappedByPte (PointerPde);

    RtlZeroMemory ((PVOID)PointerPte, PAGE_SIZE);

    TempPte = *PointerPde;
    TempPte.u.Hard.Valid = 0;

    KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);
    KeFlushSingleTb (PointerPte,
                     TRUE,
                     FALSE,
                     (PHARDWARE_PTE)PointerPde,
                     TempPte.u.Hard);

    KeLowerIrql(OldIrql);

    CurrentProcess->Vm.MaximumWorkingSetSize = MmSystemProcessWorkingSetMax;
    CurrentProcess->Vm.MinimumWorkingSetSize = MmSystemProcessWorkingSetMin;

    MmInitializeProcessAddressSpace (CurrentProcess,
                                (PEPROCESS)NULL,
                                (PVOID)NULL);

    *PointerPde = ZeroKernelPte;

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

