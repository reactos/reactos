/*++

Copyright (c) 1990  Microsoft Corporation
Copyright (c) 1992  Digital Equipment Corporation

Module Name:

    inialpha.c

Abstract:

    This module contains the machine dependent initialization for the
    memory management component.  It is specifically tailored to the
    ALPHA architecture.

Author:

    Lou Perazzoli (loup) 3-Apr-1990
    Joe Notarangelo  23-Apr-1992    ALPHA version

Revision History:

--*/

#include "mi.h"
#include <inbv.h>

//
// Local definitions
//

#define _1MB  (0x100000)
#define _16MB (0x1000000)
#define _24MB (0x1800000)
#define _32MB (0x2000000)

SIZE_T MmExpandedNonPagedPoolInBytes;


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
    ULONG LowMemoryReserved;
    ULONG i, j;
    ULONG HighPage;
    ULONG PagesLeft;
    ULONG PageNumber;
    ULONG PdePageNumber;
    ULONG PdePage;
    ULONG PageFrameIndex;
    ULONG NextPhysicalPage;
    ULONG PfnAllocation;
    ULONG NumberOfPages;
    PEPROCESS CurrentProcess;
    PVOID SpinLockPage;
    ULONG MostFreePage;
    ULONG MostFreeLowMem;
    PLIST_ENTRY NextMd;
    ULONG MaxPool;
    KIRQL OldIrql;
    PMEMORY_ALLOCATION_DESCRIPTOR FreeDescriptor;
    PMEMORY_ALLOCATION_DESCRIPTOR FreeDescriptorLowMem;
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
    CHAR Buffer[256];
    PMMFREE_POOL_ENTRY Entry;
    PVOID NonPagedPoolStartVirtual;
    ULONG Range;
    ULONG RemovedLowPage;
    ULONG RemovedLowCount;

    RemovedLowPage = 0;
    RemovedLowCount = 0;
    LowMemoryReserved = 0;
    MostFreePage = 0;
    MostFreeLowMem = 0;
    FreeDescriptor = NULL;
    FreeDescriptorLowMem = NULL;

    PointerPte = MiGetPdeAddress (PDE_BASE);

    PdePageNumber = PointerPte->u.Hard.PageFrameNumber;

    PsGetCurrentProcess()->Pcb.DirectoryTableBase[0] = PointerPte->u.Long;

    KeSweepDcache (FALSE);

    //
    // Get the lower bound of the free physical memory and the
    // number of physical pages by walking the memory descriptor lists.
    // In addition, find the memory descriptor with the most free pages
    // that begins at a physical address less than 16MB.  The 16 MB
    // boundary is necessary for allocating common buffers for use by
    // ISA devices that cannot address more than 24 bits.
    //

    NextMd = LoaderBlock->MemoryDescriptorListHead.Flink;

    //
    // When restoring a hibernation image, OS Loader needs to use "a few" extra
    // pages of LoaderFree memory.
    // This is not accounted for when reserving memory for hibernation below.
    // Start with a safety margin to allow for this plus modest future increase.
    //

    MmHiberPages = 96;

    while (NextMd != &LoaderBlock->MemoryDescriptorListHead) {

        MemoryDescriptor = CONTAINING_RECORD(NextMd,
                                             MEMORY_ALLOCATION_DESCRIPTOR,
                                             ListEntry);

        HighPage = MemoryDescriptor->BasePage + MemoryDescriptor->PageCount-1;

        //
        // This check results in /BURNMEMORY chunks not being counted.
        //
        
        if (MemoryDescriptor->MemoryType != LoaderBad) {
            MmNumberOfPhysicalPages += MemoryDescriptor->PageCount;
        }
        
        if (MemoryDescriptor->BasePage < MmLowestPhysicalPage) {
            MmLowestPhysicalPage = MemoryDescriptor->BasePage;
        }
        
        if (HighPage > MmHighestPhysicalPage) {
            MmHighestPhysicalPage = HighPage;
        }
        
        //
        // Locate the largest free block starting below 16 megs
        // and the largest free block.
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
                (MemoryDescriptor->BasePage < (_16MB >> PAGE_SHIFT)) &&
                (HighPage < MM_PAGES_IN_KSEG0)) {
            
                    MostFreeLowMem = MemoryDescriptor->PageCount;
                    FreeDescriptorLowMem = MemoryDescriptor;
            
            } else if (MemoryDescriptor->PageCount > MostFreePage) {
            
                    MostFreePage = MemoryDescriptor->PageCount;
                    FreeDescriptor = MemoryDescriptor;
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

    //
    // Perform sanity checks on the results of walking the memory
    // descriptors.
    //

    if (MmNumberOfPhysicalPages < 1024) {
        KeBugCheckEx (INSTALL_MORE_MEMORY,
                      MmNumberOfPhysicalPages,
                      MmLowestPhysicalPage,
                      MmHighestPhysicalPage,
                      0);
    }

    if (FreeDescriptorLowMem == NULL){
        InbvDisplayString("MmInit *** FATAL ERROR *** no free descriptors that begin below physical address 16MB\n");
        KeBugCheck (MEMORY_MANAGEMENT);
    }

    if (MmDynamicPfn == TRUE) {

        //
        // Since a ~128mb PFN database is required to span the 32GB supported
        // by Alpha, require 256mb of memory to be present to support
        // this option.
        //

        if (MmNumberOfPhysicalPages >= (256 * 1024 * 1024) / PAGE_SIZE) {
            MmHighestPossiblePhysicalPage = 0x400000 - 1;
        }
        else {
            MmDynamicPfn = FALSE;
        }
    }
    else {
        MmHighestPossiblePhysicalPage = MmHighestPhysicalPage;
    }

    //
    // Used later to build nonpaged pool.
    //

    NextPhysicalPage = FreeDescriptorLowMem->BasePage;
    NumberOfPages = FreeDescriptorLowMem->PageCount;

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
        // More than 7/8 of memory allocated to nonpagedpool, reset to 0.
        //

        MmSizeOfNonPagedPoolInBytes = 0;
    }

    if (MmSizeOfNonPagedPoolInBytes < MmMinimumNonPagedPoolSize) {

        //
        // Calculate the size of nonpaged pool.  Use the minimum size,
        // then for every MB above 8mb add extra pages.
        //

        MmSizeOfNonPagedPoolInBytes = MmMinimumNonPagedPoolSize;

        MmSizeOfNonPagedPoolInBytes +=
                         ((MmNumberOfPhysicalPages - 1024) /
                        (_1MB >> PAGE_SHIFT) ) *
                        MmMinAdditionNonPagedPoolPerMb;
    }

    //
    // Align to page size boundary.
    //

    MmSizeOfNonPagedPoolInBytes &= ~(PAGE_SIZE - 1);

    //
    // Limit initial nonpaged pool size to MM_MAX_INITIAL_NONPAGED_POOL
    //

    if (MmSizeOfNonPagedPoolInBytes > MM_MAX_INITIAL_NONPAGED_POOL) {
        MmSizeOfNonPagedPoolInBytes = MM_MAX_INITIAL_NONPAGED_POOL;
    }

    //
    // If the non-paged pool that we want to allocate will not fit in
    // the free memory descriptor that we have available then recompute
    // the size of non-paged pool to be the size of the free memory
    // descriptor.  If the free memory descriptor cannot fit the
    // minimum non-paged pool size (MmMinimumNonPagedPoolSize) then we
    // cannot boot the operating system.
    //

    if ((MmSizeOfNonPagedPoolInBytes >> PAGE_SHIFT) > NumberOfPages) {

         //
         // Reserve all of low memory for nonpaged pool.
         //

         MmSizeOfNonPagedPoolInBytes = NumberOfPages << PAGE_SHIFT;
         LowMemoryReserved = NextPhysicalPage;

         //
         // Switch to backup descriptor for all other allocations.
         //

         NextPhysicalPage = FreeDescriptor->BasePage;
         NumberOfPages = FreeDescriptor->PageCount;

         if (MmSizeOfNonPagedPoolInBytes < MmMinimumNonPagedPoolSize) {
            InbvDisplayString("MmInit *** FATAL ERROR *** cannot allocate non-paged pool\n");
            sprintf(Buffer,
                    "Largest description = %d pages, require %d pages\n",
                    NumberOfPages,
                    MmMinimumNonPagedPoolSize >> PAGE_SHIFT);
            InbvDisplayString (Buffer );
            KeBugCheck (MEMORY_MANAGEMENT);

         }
    }

    //
    // Calculate the maximum size of pool.
    //

    if (MmMaximumNonPagedPoolInBytes == 0) {

        //
        // Calculate the size of nonpaged pool.
        // For every MB above 8mb add extra pages.
        //

        MmMaximumNonPagedPoolInBytes = MmDefaultMaximumNonPagedPool;

        //
        // Make sure enough expansion for the PFN database exists.
        //

        MmMaximumNonPagedPoolInBytes += (ULONG)PAGE_ALIGN (
                                      MmHighestPhysicalPage * sizeof(MMPFN));

        MmMaximumNonPagedPoolInBytes +=
                         ((MmNumberOfPhysicalPages - 1024) /
                         (_1MB >> PAGE_SHIFT) ) *
                         MmMaxAdditionNonPagedPoolPerMb;
    }

    MaxPool = MmSizeOfNonPagedPoolInBytes + PAGE_SIZE * 16 +
                                   (ULONG)PAGE_ALIGN (
                                        MmHighestPhysicalPage * sizeof(MMPFN));

    if (MmMaximumNonPagedPoolInBytes < MaxPool) {
        MmMaximumNonPagedPoolInBytes = MaxPool;
    }

    //
    // If the system is configured for maximum system PTEs then limit maximum
    // nonpaged pool to 128mb so the rest of the virtual address space can
    // be used for the PTEs.  Also push as much nonpaged pool as possible
    // into kseg0 to free up more PTEs.
    //

    if (MmMaximumNonPagedPoolInBytes > MM_MAX_ADDITIONAL_NONPAGED_POOL) {

        ULONG InitialNonPagedPages;
        ULONG ExpansionPagesToMove;
        ULONG LowAvailPages;

        if ((MiRequestedSystemPtes == (ULONG)-1) || (MiHydra == TRUE)) {
            MmMaximumNonPagedPoolInBytes = MM_MAX_ADDITIONAL_NONPAGED_POOL;

            if (LowMemoryReserved == 0) {

                InitialNonPagedPages = (MmSizeOfNonPagedPoolInBytes >> PAGE_SHIFT);
    
                if (InitialNonPagedPages + 1024 < NumberOfPages) {
                    LowAvailPages = NumberOfPages - 1024 - InitialNonPagedPages;
        
                    ExpansionPagesToMove = (MmMaximumNonPagedPoolInBytes >> PAGE_SHIFT) - InitialNonPagedPages;
        
                    if (ExpansionPagesToMove > 32) {
                        ExpansionPagesToMove -= 32;
                        if (LowAvailPages > ExpansionPagesToMove) {
                            LowAvailPages = ExpansionPagesToMove;
                        }
            
                        MmSizeOfNonPagedPoolInBytes += (LowAvailPages << PAGE_SHIFT);
                    }
                }
            }

            if (MmSizeOfNonPagedPoolInBytes == MmMaximumNonPagedPoolInBytes) {
                ASSERT (MmSizeOfNonPagedPoolInBytes > (32 << PAGE_SHIFT));
                MmSizeOfNonPagedPoolInBytes -= (32 << PAGE_SHIFT);
            }
        }
    }

    //
    // Limit maximum nonpaged pool to MM_MAX_ADDITIONAL_NONPAGED_POOL.
    //

    if (MmMaximumNonPagedPoolInBytes > MM_MAX_ADDITIONAL_NONPAGED_POOL) {

        if (MmMaximumNonPagedPoolInBytes > MM_MAX_INITIAL_NONPAGED_POOL + MM_MAX_ADDITIONAL_NONPAGED_POOL) {
            MmMaximumNonPagedPoolInBytes = MM_MAX_INITIAL_NONPAGED_POOL + MM_MAX_ADDITIONAL_NONPAGED_POOL;
        }

        if (LowMemoryReserved != 0) {
            if (MmMaximumNonPagedPoolInBytes > MmSizeOfNonPagedPoolInBytes + MM_MAX_ADDITIONAL_NONPAGED_POOL) {
                MmMaximumNonPagedPoolInBytes = MmSizeOfNonPagedPoolInBytes + MM_MAX_ADDITIONAL_NONPAGED_POOL;
            }
            MmExpandedNonPagedPoolInBytes = MmMaximumNonPagedPoolInBytes - MmSizeOfNonPagedPoolInBytes;
        }
        else {

            if ((MM_MAX_INITIAL_NONPAGED_POOL >> PAGE_SHIFT) >= NumberOfPages) {

                //
                // Reserve all of low memory for nonpaged pool.
                //
                
                SIZE_T Diff;

                Diff = MmMaximumNonPagedPoolInBytes - MmSizeOfNonPagedPoolInBytes;
                if (Diff > MM_MAX_ADDITIONAL_NONPAGED_POOL) {
                    Diff = MM_MAX_ADDITIONAL_NONPAGED_POOL;
                }

                MmSizeOfNonPagedPoolInBytes = NumberOfPages << PAGE_SHIFT;
                MmMaximumNonPagedPoolInBytes = MmSizeOfNonPagedPoolInBytes + Diff;
                LowMemoryReserved = NextPhysicalPage;

                //
                // Switch to backup descriptor for all other allocations.
                //
                
                NextPhysicalPage = FreeDescriptor->BasePage;
                NumberOfPages = FreeDescriptor->PageCount;
            }
            else {

                MmSizeOfNonPagedPoolInBytes = MM_MAX_INITIAL_NONPAGED_POOL;

                //
                // The pages must be subtracted from the low descriptor so
                // they are not used for anything else or put on the freelist.
                // But they must be added back in later when initializing PFNs
                // for all the descriptor ranges.
                //

                RemovedLowCount = (MmSizeOfNonPagedPoolInBytes >> PAGE_SHIFT);
                FreeDescriptorLowMem->PageCount -= RemovedLowCount;
                RemovedLowPage = FreeDescriptorLowMem->BasePage + FreeDescriptorLowMem->PageCount;

                NumberOfPages = FreeDescriptorLowMem->PageCount;
            }

            MmExpandedNonPagedPoolInBytes = MmMaximumNonPagedPoolInBytes - MmSizeOfNonPagedPoolInBytes;

            if (MmExpandedNonPagedPoolInBytes > MM_MAX_ADDITIONAL_NONPAGED_POOL) {
                MmExpandedNonPagedPoolInBytes = MM_MAX_ADDITIONAL_NONPAGED_POOL;
            }
        }
    }

    if (MmExpandedNonPagedPoolInBytes) {
        MmNonPagedPoolStart = (PVOID)((ULONG)MmNonPagedPoolEnd
                                          - MmExpandedNonPagedPoolInBytes);
    }
    else {
        MmNonPagedPoolStart = (PVOID)((ULONG)MmNonPagedPoolEnd
                                          - (MmMaximumNonPagedPoolInBytes - 1));
    }

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
    }

    MmNumberOfSystemPtes = (((ULONG)MmNonPagedPoolStart -
                             (ULONG)MmNonPagedSystemStart) >> PAGE_SHIFT)-1;
    ASSERT (MmNumberOfSystemPtes > 1000);

    //
    // Set the global bit for all PDEs in system space.
    //

    StartPde = MiGetPdeAddress (MM_SYSTEM_SPACE_START);
    EndPde = MiGetPdeAddress (MM_SYSTEM_SPACE_END);

    while (StartPde <= EndPde) {

        if (StartPde->u.Hard.Global == 0) {
            TempPte = *StartPde;
            TempPte.u.Hard.Global = 1;
            *StartPde = TempPte;
        }

        StartPde += 1;
    }

    if (MiHydra == TRUE) {

        //
        // Clear the global bit for all session space addresses.
        //

        StartPde = MiGetPdeAddress (MmSessionBase);
        EndPde = MiGetPdeAddress (MI_SESSION_SPACE_END);

        while (StartPde < EndPde) {
    
            if (StartPde->u.Hard.Global == 1) {
                TempPte = *StartPde;
                TempPte.u.Hard.Global = 0;
                *StartPde = TempPte;
            }

            ASSERT (StartPde->u.Long == 0);
            StartPde += 1;
        }
    }

    StartPde = MiGetPdeAddress (MmNonPagedSystemStart);

    EndPde = MiGetPdeAddress (MmNonPagedPoolEnd);

    ASSERT ((EndPde - StartPde) < (LONG)NumberOfPages);

    TempPte = ValidKernelPte;

    while (StartPde <= EndPde) {
        if (StartPde->u.Hard.Valid == 0) {

        //
        // Map in a page directory page.
        //

        TempPte.u.Hard.PageFrameNumber = NextPhysicalPage;
        NumberOfPages -= 1;
        NextPhysicalPage += 1;

        if (NumberOfPages == 0) {
            ASSERT (NextPhysicalPage != (FreeDescriptor->BasePage +
                    FreeDescriptor->PageCount));
            NextPhysicalPage = FreeDescriptor->BasePage;
            NumberOfPages = FreeDescriptor->PageCount;
        }
        *StartPde = TempPte;
      }
      StartPde += 1;
    }

    //
    // Zero the PTEs before non-paged pool.
    //

    StartPde = MiGetPteAddress (MmNonPagedSystemStart);
    PointerPte = MiGetPteAddress (MmNonPagedPoolStart);

    RtlZeroMemory (StartPde, (ULONG)PointerPte - (ULONG)StartPde);

    //
    // Fill in the PTEs for non-paged pool.
    //

    PointerPte = MiGetPteAddress(MmNonPagedPoolStart);
    LastPte = MiGetPteAddress((ULONG)MmNonPagedPoolStart +
                                        MmSizeOfNonPagedPoolInBytes - 1);

    if (MmExpandedNonPagedPoolInBytes == 0) {
        if (!LowMemoryReserved) {
    
            if (NumberOfPages < (ULONG)(LastPte - PointerPte + 1)) {
    
                //
                // Can't just switch descriptors here - the initial nonpaged
                // pool is always mapped via KSEG0 and is thus required to be
                // virtually and physically contiguous.
                //
    
                KeBugCheckEx (INSTALL_MORE_MEMORY,
                              MmNumberOfPhysicalPages,
                              NumberOfPages,
                              LastPte - PointerPte + 1,
                              1);
            }
            
            while (PointerPte <= LastPte) {
                TempPte.u.Hard.PageFrameNumber = NextPhysicalPage;
                NextPhysicalPage += 1;
                NumberOfPages -= 1;
                ASSERT (NumberOfPages != 0);
                *PointerPte = TempPte;
                PointerPte += 1;
            }
    
        } else {
    
            ULONG ReservedPage = FreeDescriptorLowMem->BasePage;
    
            while (PointerPte <= LastPte) {
                TempPte.u.Hard.PageFrameNumber = ReservedPage;
                ReservedPage += 1;
                *PointerPte = TempPte;
                PointerPte += 1;
            }
        }
        LastPte = MiGetPteAddress ((ULONG)MmNonPagedPoolStart +
                                      MmMaximumNonPagedPoolInBytes - 1);
    }
    else {
        LastPte = MiGetPteAddress ((ULONG)MmNonPagedPoolStart +
                                      MmExpandedNonPagedPoolInBytes - 1);
    }

    //
    // Zero the remaining PTEs for non-paged pool maximum.
    //

    while (PointerPte <= LastPte) {
        *PointerPte = ZeroKernelPte;
        PointerPte += 1;
    }

    //
    // Zero the remaining PTEs (if any).
    //

    while (((ULONG)PointerPte & (PAGE_SIZE - 1)) != 0) {
        *PointerPte = ZeroKernelPte;
        PointerPte += 1;
    }

    if (MmExpandedNonPagedPoolInBytes) {

        if (LowMemoryReserved) {
            MmNonPagedPoolStart = (PVOID)((LowMemoryReserved << PAGE_SHIFT) |
                                  KSEG0_BASE);
        }
        else if (RemovedLowPage) {
            MmNonPagedPoolStart = (PVOID)((RemovedLowPage << PAGE_SHIFT) |
                                  KSEG0_BASE);
        }
        else {
            ASSERT (FALSE);
        }
    }
    else {
        PointerPte = MiGetPteAddress (MmNonPagedPoolStart);
        MmNonPagedPoolStart = (PVOID)((PointerPte->u.Hard.PageFrameNumber << PAGE_SHIFT) |
                              KSEG0_BASE);
    }

    MmPageAlignedPoolBase[NonPagedPool] = MmNonPagedPoolStart;

    MmSubsectionBase = (ULONG)MmNonPagedPoolStart;

    if (MmExpandedNonPagedPoolInBytes == 0) {
        if (NextPhysicalPage < (MM_SUBSECTION_MAP >> PAGE_SHIFT)) {
            MmSubsectionBase = KSEG0_BASE;
        }
    }

    MmSubsectionTopPage = (((MmSubsectionBase & ~KSEG0_BASE) + MM_SUBSECTION_MAP) >> PAGE_SHIFT);

    //
    // Non-paged pages now exist, build the pool structures.
    //

    if (MmExpandedNonPagedPoolInBytes) {
        MmNonPagedPoolExpansionStart = (PVOID)NonPagedPoolStartVirtual;
    }
    else {
        MmNonPagedPoolExpansionStart = (PVOID)((PCHAR)NonPagedPoolStartVirtual +
                        MmSizeOfNonPagedPoolInBytes);
    }

    MiInitializeNonPagedPool ();

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

    if (((MmSecondaryColors & (MmSecondaryColors -1)) != 0) ||
        (MmSecondaryColors < MM_SECONDARY_COLORS_MIN) ||
        (MmSecondaryColors > MM_SECONDARY_COLORS_MAX)) {

        MmSecondaryColors = MM_SECONDARY_COLORS_DEFAULT;
    }

    MmSecondaryColorMask = MmSecondaryColors - 1;

    PfnAllocation = 1 + ((((MmHighestPossiblePhysicalPage + 1) * sizeof(MMPFN)) +
                        (MmSecondaryColors * sizeof(MMCOLOR_TABLES)*2))
                            >> PAGE_SHIFT);

    //
    // If the number of pages remaining in the current descriptor is
    // greater than the number of pages needed for the PFN database,
    // and the descriptor is for memory below 1 gig, then allocate the
    // PFN database from the current free descriptor.
    // Note: FW creates a new memory descriptor for any memory above 1GB.
    // Thus we don't need to worry if the highest page will go beyond 1GB for
    // this memory descriptor.
    //

#ifndef PFN_CONSISTENCY
    if ((NumberOfPages >= PfnAllocation) &&
        (NextPhysicalPage + NumberOfPages <= MM_PAGES_IN_KSEG0)) {

        //
        // Allocate the PFN database in kseg0.
        //
        // Compute the address of the PFN by allocating the appropriate
        // number of pages from the end of the free descriptor.
        //

        PfnInKseg0 = TRUE;
        HighPage = NextPhysicalPage + NumberOfPages;
        MmPfnDatabase = (PMMPFN)(KSEG0_BASE |
                                 ((HighPage - PfnAllocation) << PAGE_SHIFT));
        RtlZeroMemory(MmPfnDatabase, PfnAllocation * PAGE_SIZE);

        //
        // Mark off the chunk of memory used for the PFN database.
        //

        NumberOfPages -= PfnAllocation;

        if (NextPhysicalPage >= FreeDescriptorLowMem->BasePage &&
            NextPhysicalPage < (FreeDescriptorLowMem->BasePage +
                                FreeDescriptorLowMem->PageCount)) {

            //
            // We haven't used the other descriptor.
            //
            
            FreeDescriptorLowMem->PageCount -= PfnAllocation;

        } else {

            FreeDescriptor->PageCount -= PfnAllocation;
        }

        //
        // Allocate one PTE at the very top of the Mm virtual address space.
        // This provides protection against the caller of the first real
        // nonpaged expansion allocation in case he accidentally overruns his
        // pool block.  (We'll trap instead of corrupting the crashdump PTEs).
        // This also allows us to freely increment in MiFreePoolPages
        // without having to worry about a valid PTE just after the end of
        // the highest nonpaged pool allocation.
        //

        MiReserveSystemPtes (1,
                             NonPagedPoolExpansion,
                             0,
                             0,
                             TRUE);

    } else {

#endif   // PFN_CONSISTENCY

        //
        // Calculate the start of the Pfn database (it starts at physical
        // page zero, even if the lowest physical page is not zero).
        //

        PfnInKseg0 = FALSE;
        PointerPte = MiReserveSystemPtes (PfnAllocation,
                                          NonPagedPoolExpansion,
                                          0,
                                          0,
                                          TRUE);

#if PFN_CONSISTENCY
        MiPfnStartPte = PointerPte;
        MiPfnPtes = PfnAllocation;
#endif

        MmPfnDatabase = (PMMPFN)(MiGetVirtualAddressMappedByPte (PointerPte));

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

        //
        // Go through the memory descriptors and for each physical page
        // make sure the PFN database has a valid PTE to map it.  This allows
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

            //
            // If memory was temporarily removed to create the initial non
            // paged pool, account for it now so PFN entries are created for it.
            //

            if (MemoryDescriptor == FreeDescriptorLowMem) {
                if (RemovedLowPage) {
                    ASSERT (MemoryDescriptor->BasePage + MemoryDescriptor->PageCount == RemovedLowPage);
                    LastPte = MiGetPteAddress (((PCHAR)(MI_PFN_ELEMENT(
                                                    MemoryDescriptor->BasePage +
                                                    RemovedLowCount +
                                                    MemoryDescriptor->PageCount))) - 1);
                }
            }

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

    MmFreePagesByColor[0] = (PMMCOLOR_TABLES)
                                &MmPfnDatabase[MmHighestPossiblePhysicalPage + 1];

    MmFreePagesByColor[1] = &MmFreePagesByColor[0][MmSecondaryColors];

    //
    // Make sure the PTEs are mapped.
    //

    if (!MI_IS_PHYSICAL_ADDRESS(MmFreePagesByColor[0])) {

        PointerPte = MiGetPteAddress (&MmFreePagesByColor[0][0]);

        LastPte = MiGetPteAddress (
                  (PVOID)((PCHAR)&MmFreePagesByColor[1][MmSecondaryColors]-1));

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
            PointerPte += 1;
        }
    }

    for (i = 0; i < MmSecondaryColors; i += 1) {
        MmFreePagesByColor[ZeroedPageList][i].Flink = MM_EMPTY_LIST;
        MmFreePagesByColor[FreePageList][i].Flink = MM_EMPTY_LIST;
    }

#if MM_MAXIMUM_NUMBER_OF_COLORS > 1
    for (i = 0; i < MM_MAXIMUM_NUMBER_OF_COLORS; i += 1) {
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

    PointerPde = MiGetPdeAddress (PTE_BASE);

    PdePage = PointerPde->u.Hard.PageFrameNumber;
    Pfn1 = MI_PFN_ELEMENT(PdePage);
    Pfn1->PteFrame = PdePage;
    Pfn1->PteAddress = PointerPde;
    Pfn1->u2.ShareCount += 1;
    Pfn1->u3.e2.ReferenceCount = 1;
    Pfn1->u3.e1.PageLocation = ActiveAndValid;
    Pfn1->u3.e1.PageColor =
        MI_GET_COLOR_FROM_SECONDARY(GET_PAGE_COLOR_FROM_PTE (PointerPde));

    //
    // Add the pages which were used to construct nonpaged pool to
    // the PFN database.
    //

    Pde = MiGetPdeAddress (MmNonPagedSystemStart);

    EndPde = MiGetPdeAddress(NON_PAGED_SYSTEM_END);

    while (Pde <= EndPde) {
        if (Pde->u.Hard.Valid == 1) {
            PdePage = Pde->u.Hard.PageFrameNumber;
            Pfn1 = MI_PFN_ELEMENT(PdePage);
            Pfn1->PteFrame = PointerPde->u.Hard.PageFrameNumber;
            Pfn1->PteAddress = Pde;
            Pfn1->u2.ShareCount += 1;
            Pfn1->u3.e2.ReferenceCount = 1;
            Pfn1->u3.e1.PageLocation = ActiveAndValid;
            Pfn1->u3.e1.PageColor =
                MI_GET_COLOR_FROM_SECONDARY(GET_PAGE_COLOR_FROM_PTE (Pde));

            PointerPte = MiGetVirtualAddressMappedByPte (Pde);
            for (j = 0 ; j < PTE_PER_PAGE; j += 1) {
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
                            MI_GET_COLOR_FROM_SECONDARY(GET_PAGE_COLOR_FROM_PTE (Pfn2->PteAddress));
                }
                PointerPte += 1;
            }
        }
        Pde += 1;
    }

    //
    // Handle the initial nonpaged pool on expanded systems.
    //

    if (MmExpandedNonPagedPoolInBytes) {
        PageFrameIndex = (((ULONG_PTR)MmNonPagedPoolStart & ~KSEG0_BASE) >> PAGE_SHIFT);
        Pfn1 = MI_PFN_ELEMENT(PageFrameIndex);
        j = PageFrameIndex + (MmSizeOfNonPagedPoolInBytes >> PAGE_SHIFT);
        while (PageFrameIndex < j) {
            Pfn1->PteFrame = PdePage;
            Pfn1->u2.ShareCount += 1;
            Pfn1->u3.e2.ReferenceCount = 1;
            Pfn1->u3.e1.PageLocation = ActiveAndValid;

            Pfn1->PteAddress =
                    (PMMPTE)(KSEG0_BASE | (PageFrameIndex << PTE_SHIFT));

            Pfn1->u3.e1.PageColor =
                    MI_GET_COLOR_FROM_SECONDARY(GET_PAGE_COLOR_FROM_PTE (Pfn1->PteAddress));
            PageFrameIndex += 1;
            Pfn1 += 1;
        }
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
        Pfn1->u3.e1.PageColor =
                MI_GET_COLOR_FROM_SECONDARY(GET_PAGE_COLOR_FROM_PTE (Pde));
    }

    // end of temporary set to physical page zero.

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
                        // Set the PTE address to the physical page for
                        // virtual address alignment checking.
                        //

                        Pfn1->PteAddress =
                                        (PMMPTE)(NextPhysicalPage << PTE_SHIFT);

                        Pfn1->u3.e1.PageColor =
                MI_GET_COLOR_FROM_SECONDARY(GET_PAGE_COLOR_FROM_PTE (Pfn1->PteAddress));
                        MiInsertPageInList (MmPageLocationList[FreePageList],
                                            NextPhysicalPage);
                    }
                    Pfn1 += 1;
                    i -= 1;
                    NextPhysicalPage += 1;
                }
                break;

            default:

                PointerPte = MiGetPteAddress (KSEG0_BASE |
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
                    Pfn1->u3.e1.PageColor =
                MI_GET_COLOR_FROM_SECONDARY(GET_PAGE_COLOR_FROM_PTE (PointerPte));

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
    // Indicate that the PFN database is allocated in NonPaged pool.
    //
    if (PfnInKseg0 == FALSE) {

        //
        // The PFN database is allocated in virtual memory
        //
        // Set the start and end of allocation.
        //

        Pfn1 = MI_PFN_ELEMENT(MiGetPteAddress(&MmPfnDatabase[MmLowestPhysicalPage])->u.Hard.PageFrameNumber);
        Pfn1->u3.e1.StartOfAllocation = 1;
        Pfn1 = MI_PFN_ELEMENT(MiGetPteAddress(&MmPfnDatabase[MmHighestPossiblePhysicalPage])->u.Hard.PageFrameNumber);
        Pfn1->u3.e1.EndOfAllocation = 1;

    } else {

        //
        // The PFN database is allocated in KSEG0.
        //
        // Mark all PFN entries for the PFN pages in use.
        //

        PageNumber = ((ULONG)MmPfnDatabase - KSEG0_BASE) >> PAGE_SHIFT;
        Pfn1 = MI_PFN_ELEMENT(PageNumber);
        do {
            Pfn1->PteAddress = (PMMPTE)(PageNumber << PTE_SHIFT);
            Pfn1->u3.e1.PageColor =
                MI_GET_COLOR_FROM_SECONDARY(GET_PAGE_COLOR_FROM_PTE (Pfn1->PteAddress));
            Pfn1 += 1;
            PfnAllocation -= 1;
        } while (PfnAllocation != 0);

        //
        // Scan the PFN database backward for pages that are completely zero.
        // These pages are unused and can be added to the free list
        //

        if (MmDynamicPfn == FALSE) {
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
                    // Set the PTE address to the physical page for
                    // virtual address alignment checking.
                    //
    
                    PageNumber = ((ULONG)BasePfn - KSEG0_BASE) >> PAGE_SHIFT;
                    Pfn1 = MI_PFN_ELEMENT(PageNumber);
    
                    ASSERT(Pfn1->u3.e2.ReferenceCount == 0);
    
                    PfnAllocation += 1;
    
                    Pfn1->PteAddress = (PMMPTE)(PageNumber << PTE_SHIFT);
                    Pfn1->u3.e1.PageColor =
                        MI_GET_COLOR_FROM_SECONDARY(GET_PAGE_COLOR_FROM_PTE (Pfn1->PteAddress));
    
                    MiInsertPageInList(MmPageLocationList[FreePageList],
                                       PageNumber);
                }

            } while (BottomPfn > MmPfnDatabase);
        }
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

    PointerPte = MiGetPteAddress (MmNonPagedSystemStart);

    //
    // Since the initial nonpaged pool must always reside in KSEG0 (many changes
    // would be needed in this routine otherwise), reallocate the PTEs for it
    // to the pagable system PTE pool now.
    //

    MmNumberOfSystemPtes = MiGetPteAddress(MmNonPagedPoolExpansionStart) - PointerPte - 1;

    MiInitializeSystemPtes (PointerPte, MmNumberOfSystemPtes, SystemPteSpace);

    //
    // Initialize the nonpaged pool.
    //

    InitializePool (NonPagedPool, 0);

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

    //
    // The PFN element for the PDE which maps hyperspace has already
    // been initialized, zero the reference count and the share count
    // so they won't be wrong.
    //

    Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);
    Pfn1->u2.ShareCount = 0;
    Pfn1->u3.e2.ReferenceCount = 0;

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

    //
    // Assert that the double mapped pages have the same alignment.
    //

    ASSERT ((PointerPte->u.Long & (0xF << PTE_SHIFT)) ==
            (PointerPde->u.Long & (0xF << PTE_SHIFT)));

    *PointerPde = TempPte;
    PointerPde->u.Hard.Global = 0;

    PointerPte = MiGetVirtualAddressMappedByPte (PointerPde);

    KeFillEntryTb ((PHARDWARE_PTE)PointerPde,
                    PointerPte,
                    TRUE);

    RtlZeroMemory ((PVOID)PointerPte, PAGE_SIZE);

    TempPte = *PointerPde;
    TempPte.u.Hard.Valid = 0;
    TempPte.u.Hard.Global = 0;

    KeFlushSingleTb (PointerPte,
                     TRUE,
                     FALSE,
                     (PHARDWARE_PTE)PointerPde,
                     TempPte.u.Hard);

    UNLOCK_PFN (OldIrql);

    //
    // Initialize hyperspace for this process.
    //

    PointerPte = MmFirstReservedMappingPte;
    PointerPte->u.Hard.PageFrameNumber = NUMBER_OF_MAPPING_PTES;

    CurrentProcess->Vm.MaximumWorkingSetSize = MmSystemProcessWorkingSetMax;
    CurrentProcess->Vm.MinimumWorkingSetSize = MmSystemProcessWorkingSetMin;

    MmInitializeProcessAddressSpace (CurrentProcess,
                                (PEPROCESS)NULL,
                                (PVOID)NULL,
                                (PVOID)NULL);

    *PointerPde = ZeroKernelPte;

    //
    // Check to see if moving the secondary page structures to the end
    // of the PFN database is a waste of memory.  And if so, copy it
    // to paged pool.
    //
    // If the PFN database ends on a page aligned boundary and the
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

        LOCK_PFN (OldIrql);

        Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);
        ASSERT ((Pfn1->u3.e2.ReferenceCount <= 1) && (Pfn1->u2.ShareCount <= 1));
        Pfn1->u2.ShareCount = 0;
        Pfn1->u3.e2.ReferenceCount = 0;
        MI_SET_PFN_DELETED (Pfn1);
#if DBG
        Pfn1->u3.e1.PageLocation = StandbyPageList;
#endif //DBG
        MiInsertPageInList (MmPageLocationList[FreePageList], PageFrameIndex);
        UNLOCK_PFN (OldIrql);
    }

    return;
}
