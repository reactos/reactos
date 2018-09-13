/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    initia64.c

Abstract:

    This module contains the machine dependent initialization for the
    memory management component.  It is specifically tailored to the
    INTEL IA64 machine.

Author:

    Lou Perazzoli (loup) 6-Jan-1990

Revision History:

--*/

#include "mi.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT,MiInitMachineDependent)
#endif

extern VOID 
KeInitializeVhpt (
    IN PVOID Virtual,
    IN ULONG Size
    );

extern VOID
KeFillLargePdePinned (
    IN PMMPTE Pte,
    IN PVOID Virtual
    );

VOID
MiConvertToSuperPages(
    PVOID StartVirtual,
    PVOID EndVirtual,
    SIZE_T PageSize,
    ULONG PageShift
    );

VOID
MiConvertBackToStandardPages(
    IN PVOID StartVirtual,
    IN PVOID EndVirtual
    );

VOID
MiBuildPageTableForDrivers(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
    );

VOID
MiBuildPageTableForLoaderMemory(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
    );

VOID
MiRemoveLoaderSuperPages(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
    );

VOID
MiMakeKernelPagesPermanent(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
    );

VOID
MiCheckMemoryDescriptorList(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
    );

ULONG_PTR MmKseg3VhptBase;
ULONG MmKseg3VhptPs;

PFN_NUMBER MmSystemParentTablePage;
MMPTE MiSystemSelfMappedPte;
MMPTE MiUserSelfMappedPte;
PVOID MiKseg0Start = 0;
PVOID MiKseg0End = 0;
BOOLEAN MiKseg0Mapping = TRUE;
BOOLEAN MiPrintMemoryDescriptors = FALSE;
PFN_NUMBER MiNtoskrnlPhysicalBase;
ULONG_PTR MiNtoskrnlVirtualBase;
ULONG MiNtoskrnlPageShift;
PFN_NUMBER MiWasteStart = 0;
PFN_NUMBER MiWasteEnd = 0;

#define MiGetKSegPteAddress(Va) ((PMMPTE)__thash((ULONG_PTR)(Va)))

#define _x1mb (1024*1024)
#define _x1mbnp ((1024*1024) >> PAGE_SHIFT)
#define _x4mb (1024*1024*4)
#define _x4mbnp ((1024*1024*4) >> PAGE_SHIFT)
#define _x16mb (1024*1024*16)
#define _x16mbnp ((1024*1024*16) >> PAGE_SHIFT)
#define _x32mb (1024*1024*32)
#define _x32mbnp ((1024*1024*32) >> PAGE_SHIFT)
#define _x64mb (1024*1024*64)
#define _x64mbnp ((1024*1024*64) >> PAGE_SHIFT)
#define _x256mb (1024*1024*256)
#define _x256mbnp ((1024*1024*256) >> PAGE_SHIFT)
#define _x512mb (1024*1024*512)
#define _x512mbnp ((1024*1024*512) >> PAGE_SHIFT)
#define _x4gb (0x100000000UI64)
#define _x4gbnp (0x100000000UI64 >> PAGE_SHIFT)

#define ADD_HIGH_MEMORY 0

#if ADD_HIGH_MEMORY
MEMORY_ALLOCATION_DESCRIPTOR MiHighMemoryDescriptor;
#endif

PMEMORY_ALLOCATION_DESCRIPTOR MiFreeDescriptor;
PMEMORY_ALLOCATION_DESCRIPTOR MiFreeDescriptorLargest = NULL;
PMEMORY_ALLOCATION_DESCRIPTOR MiFreeDescriptorLowMem = NULL;
PMEMORY_ALLOCATION_DESCRIPTOR MiFreeDescriptorNonPaged = NULL;

PFN_NUMBER MiNextPhysicalPage;
PFN_NUMBER MiNumberOfPages;
PFN_NUMBER MiOldFreeDescriptorBase;
PFN_NUMBER MiOldFreeDescriptorCount;

//
// Processor Specific VM information
//

ULONG MiImplVirtualMsb = 50;


PFN_NUMBER
MiGetNextPhysicalPage (
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

    if (MiNumberOfPages != 0) {
        MiNumberOfPages -= 1;
        return MiNextPhysicalPage++;

    } else {

        //
        // If the current descriptor is not the largest free descriptor,
        // then switch to the largest free descriptor. Otherwise, bugcheck.
        //

        if (MiFreeDescriptor == MiFreeDescriptorLargest) {

            KeBugCheckEx(INSTALL_MORE_MEMORY,
                         MmNumberOfPhysicalPages,
                         MmLowestPhysicalPage,
                         MmHighestPhysicalPage,
                         0);

            return 0;

        } else if (MiFreeDescriptor == MiFreeDescriptorLowMem) {
            
            MiOldFreeDescriptorCount = MiFreeDescriptorLargest->PageCount;
            MiOldFreeDescriptorBase = MiFreeDescriptorLargest->BasePage;
        
            MiFreeDescriptor = MiFreeDescriptorLargest;
            MiNumberOfPages = MiFreeDescriptorLargest->PageCount - 1;
            MiNextPhysicalPage = MiFreeDescriptorLargest->BasePage;

            MiFreeDescriptorLowMem->PageCount = 0;

            return MiNextPhysicalPage++;

        } else {

            MiOldFreeDescriptorCount = MiFreeDescriptorLowMem->PageCount;
            MiOldFreeDescriptorBase = MiFreeDescriptorLowMem->BasePage;
        
            MiFreeDescriptor = MiFreeDescriptorLowMem;
            MiNumberOfPages = MiFreeDescriptorLowMem->PageCount - 1;
            MiNextPhysicalPage = MiFreeDescriptorLowMem->BasePage;

            MiFreeDescriptorNonPaged->PageCount = 0;

            return MiNextPhysicalPage++;
        }
    }
}

BOOLEAN
MiEnsureAvailablePagesInFreeDescriptor(
    IN PFN_NUMBER Pages,
    IN PFN_NUMBER MaxPage
    )
{
    if ((MiNumberOfPages < Pages) &&
        (MiFreeDescriptor == MiFreeDescriptorNonPaged)) {

        MiFreeDescriptor->BasePage = (ULONG)MiNextPhysicalPage;
        MiFreeDescriptor->PageCount = (PFN_COUNT)MiNumberOfPages;

        if (MiFreeDescriptor != MiFreeDescriptorLowMem) {

            MiOldFreeDescriptorCount = MiFreeDescriptorLowMem->PageCount;
            MiOldFreeDescriptorBase = MiFreeDescriptorLowMem->BasePage;
        
            MiFreeDescriptor = MiFreeDescriptorLowMem;
            MiNumberOfPages = MiFreeDescriptorLowMem->PageCount;
            MiNextPhysicalPage = MiFreeDescriptorLowMem->BasePage;

        } else if (MiFreeDescriptor != MiFreeDescriptorLargest) {

            MiOldFreeDescriptorCount = MiFreeDescriptorLargest->PageCount;
            MiOldFreeDescriptorBase = MiFreeDescriptorLargest->BasePage;
        
            MiFreeDescriptor = MiFreeDescriptorLargest;
            MiNumberOfPages = MiFreeDescriptorLargest->PageCount;
            MiNextPhysicalPage = MiFreeDescriptorLargest->BasePage;

        } else {

            KeBugCheckEx(INSTALL_MORE_MEMORY,
                         MmNumberOfPhysicalPages,
                         MmLowestPhysicalPage,
                         MmHighestPhysicalPage,
                         1);
        }
    }

    if (MaxPage < (MiFreeDescriptor->BasePage + Pages)) {

        return FALSE;

    } else {

        return TRUE;

    }
}


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
    SIZE_T Range;
    PFN_NUMBER i;
    ULONG j;
    PFN_NUMBER PdePageNumber;
    PFN_NUMBER PdePage;
    PFN_NUMBER PageFrameIndex;
    PFN_NUMBER NextPhysicalPage;
    SPFN_NUMBER PfnAllocation;
    SPFN_NUMBER NumberOfPages;
    SIZE_T MaxPool;
    PEPROCESS CurrentProcess;
    PFN_NUMBER MostFreePage = 0;
    PFN_NUMBER MostFreeLowMem = 0;
    PFN_NUMBER MostFreeNonPaged = 0;
    PLIST_ENTRY NextMd;
    PMEMORY_ALLOCATION_DESCRIPTOR MemoryDescriptor;
    MMPTE TempPte;
    PMMPTE PointerPde;
    PMMPTE PointerPte;
    PMMPTE StartPte;
    PMMPTE LastPte;
    PMMPTE EndPte;
    PMMPTE Pde;
    PMMPTE StartPde;
    PMMPTE StartPpe;
    PMMPTE EndPde;
    PMMPTE FirstPpe;
    PMMPFN Pfn1;
    PMMPFN Pfn2;
    ULONG First;
    KIRQL OldIrql;
    ULONG_PTR va;
    SIZE_T Kseg0Size = 0;
    PVOID NonPagedPoolStartVirtual;
    ULONG i1;
    ULONG i2;
    PFN_NUMBER PhysicalPages;
    ULONG_PTR HighestPageAddress;
    ULONG_PTR Kseg3VhptSize;
    PFN_NUMBER Kseg3VhptPn;
    ULONG_PTR size;
    ULONG_PTR PageSize;
    PFN_NUMBER KernelStart;
    PFN_NUMBER KernelEnd;

    if (InitializationPhase == 1) {

        MiMakeKernelPagesPermanent(LoaderBlock);

        return;

    }

    //
    // Initialize the kernel mapping info
    //

    MiNtoskrnlPhysicalBase = LoaderBlock->u.Ia64.ItrInfo[ITR_KERNEL_INDEX].PhysicalAddress;
    MiNtoskrnlVirtualBase = LoaderBlock->u.Ia64.ItrInfo[ITR_KERNEL_INDEX].VirtualAddress;
    MiNtoskrnlPageShift = LoaderBlock->u.Ia64.ItrInfo[ITR_KERNEL_INDEX].PageSize;

    KernelStart = MiNtoskrnlPhysicalBase >> PAGE_SHIFT;
    PageSize = (ULONG_PTR)1 << MiNtoskrnlPageShift;
    KernelEnd = KernelStart + (PageSize >> PAGE_SHIFT);

    //
    // Initialize MmDebugPte and MmCrashDumpPte
    //

    MmDebugPte = MiGetPteAddress(MM_DEBUG_VA);

    MmCrashDumpPte = MiGetPteAddress(MM_CRASH_DUMP_VA);

    //
    // Set TempPte to ValidKernelPte for the future use
    //

    TempPte = ValidKernelPte;

    //
    // Check the memory descriptor list from NT loader
    //

    MiCheckMemoryDescriptorList(LoaderBlock);


    //
    // Get the lower bound of the free physical memory and the
    // number of physical pages by walking the memory descriptor lists.
    //

    NextMd = LoaderBlock->MemoryDescriptorListHead.Flink;

#if ADD_HIGH_MEMORY
    LoaderBlock->MemoryDescriptorListHead.Flink = &MiHighMemoryDescriptor.ListEntry;
    MiHighMemoryDescriptor.ListEntry.Flink = NextMd;
    MiHighMemoryDescriptor.MemoryType = 5;
    MiHighMemoryDescriptor.BasePage = 0x80000;
    MiHighMemoryDescriptor.PageCount = 0x20000;
    NextMd = &MiHighMemoryDescriptor.ListEntry;
#endif 

    while (NextMd != &LoaderBlock->MemoryDescriptorListHead) {

        MemoryDescriptor = CONTAINING_RECORD(NextMd,
                                             MEMORY_ALLOCATION_DESCRIPTOR,
                                             ListEntry);

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
                    MemoryDescriptor->BasePage + MemoryDescriptor->PageCount -1;
        }

        //
        // We assume at this stage (Gambit Port) that the minimum memory requirement 
        // for the iA EM is 32mb. There will be no memory hole right below the 16m 
        // boundary on EM machines unlike some x86 of PC platforms. 
        //

        if ((MemoryDescriptor->MemoryType == LoaderFree) ||
            (MemoryDescriptor->MemoryType == LoaderLoadedProgram) ||
            (MemoryDescriptor->MemoryType == LoaderFirmwareTemporary) ||
            (MemoryDescriptor->MemoryType == LoaderOsloaderStack)) {

            if (MemoryDescriptor->PageCount > MostFreePage) {
                MostFreePage = MemoryDescriptor->PageCount;
                MiFreeDescriptorLargest = MemoryDescriptor;
             }

            if (MemoryDescriptor->BasePage < _x256mbnp) {
                
                //
                // This memory descriptor is below 16mb.
                //

                if ((MostFreeLowMem < MemoryDescriptor->PageCount) &&
                    (MostFreeLowMem < _x256mbnp - MemoryDescriptor->BasePage)) {
                    
                    MostFreeLowMem = _x256mbnp - MemoryDescriptor->BasePage;
                    if (MemoryDescriptor->PageCount < MostFreeLowMem) {
                        MostFreeLowMem = MemoryDescriptor->PageCount;
                    }

                    MiFreeDescriptorLowMem = MemoryDescriptor;
                }
            }

            if ((MemoryDescriptor->BasePage >= KernelStart) && 
                (MemoryDescriptor->PageCount > MostFreeNonPaged)) {
                
                MostFreeNonPaged = MemoryDescriptor->PageCount;
                MiFreeDescriptorNonPaged = MemoryDescriptor;

            }

        }

#if DBG
        if (MiPrintMemoryDescriptors) {
            DbgPrint("MemoryType = %x\n", MemoryDescriptor->MemoryType);
            DbgPrint("BasePage = %p\n", (PFN_NUMBER)MemoryDescriptor->BasePage << PAGE_SHIFT);
            DbgPrint("PageCount = %x\n\n", MemoryDescriptor->PageCount << PAGE_SHIFT);
        }
#endif

        NextMd = MemoryDescriptor->ListEntry.Flink;
    }

    MmHighestPossiblePhysicalPage = MmHighestPhysicalPage;

    //
    // If the number of physical pages is less than  16mb, then
    // bugcheck. There is not enough memory to run the system.
    //

    if (MmNumberOfPhysicalPages < _x16mbnp) {
        KeBugCheckEx (INSTALL_MORE_MEMORY,
                      MmNumberOfPhysicalPages,
                      MmLowestPhysicalPage,
                      MmHighestPhysicalPage,
                      2);
    }

    //
    // initialize the next page allocation variable & structures
    //


    if (MiFreeDescriptorNonPaged != NULL) {

        MiFreeDescriptor = MiFreeDescriptorNonPaged;

    } else if (MiFreeDescriptorLowMem != NULL) {

        MiFreeDescriptor = MiFreeDescriptorLowMem;

    } else {

        MiFreeDescriptor = MiFreeDescriptorLargest;
    }

    if (MiFreeDescriptorLowMem == NULL) {

        MiKseg0Mapping = FALSE;

    }

    MiNextPhysicalPage = MiFreeDescriptor->BasePage;
    MiNumberOfPages = MiFreeDescriptor->PageCount;
    MiOldFreeDescriptorCount = MiFreeDescriptor->PageCount;
    MiOldFreeDescriptorBase = MiFreeDescriptor->BasePage;

    //
    // Produce the size of the Kseg 0 space
    // 

    if (MiKseg0Mapping == TRUE) {


        MiKseg0Start = KSEG0_ADDRESS(MiNextPhysicalPage);

        Kseg0Size = MiNextPhysicalPage + MiNumberOfPages;
    
        if (Kseg0Size > MM_PAGES_IN_KSEG0) {
            Kseg0Size = MM_PAGES_IN_KSEG0 - (_x16mbnp * 3); 
        }

        Kseg0Size = Kseg0Size << PAGE_SHIFT;

    } else {
        
        MiKseg0Mapping = FALSE;
        Kseg0Size = 0;

    }

    //
    // Initialize VHPT registers
    //

    KeInitializeVhpt ((PVOID)PTA_BASE, MiImplVirtualMsb - PAGE_SHIFT + PTE_SHIFT);

    //
    // Build 1 to 1 virtual to physical mapping space
    //

#if defined(KSEG_VHPT)
    HighestPageAddress = ((ULONG_PTR)MmHighestPhysicalPage << PAGE_SHIFT);
    
    HighestPageAddress = (HighestPageAddress + (ULONG_PTR)X64K - 1) & ~((ULONG_PTR)X64K - 1);
        
    Kseg3VhptSize = HighestPageAddress >> 16 << PTE_SHIFT;

    size = (Kseg3VhptSize-1) >> 12;  // make the base page size 4KB
    PageSize = 12;

    while (size != 0) {
        size = size >> 2;
        PageSize += 2;
    }

    Kseg3VhptSize = (ULONG_PTR)1 << PageSize;

    Kseg3VhptPages = (PFN_NUMBER)(Kseg3VhptSize >> PAGE_SHIFT);

    if ((MiFreeDescriptor->BasePage & (_x16mbnp - 1)) != 0) {
        KeBugCheckEx (INSTALL_MORE_MEMORY,
                      MmNumberOfPhysicalPages,
                      MmLowestPhysicalPage,
                      MmHighestPhysicalPage,
                      3);
    }

    MmKseg3VhptBase = MiNextPhysicalPage;
    MiNextPhysicalPage += Kseg3VhptPages;
    MiNumberOfPages -= Kseg3VhptPages;

    if (MiNumberOfPages < 0) {

        KeBugCheckEx(INSTALL_MORE_MEMORY,
                     MmNumberOfPhysicalPages,
                     MmLowestPhysicalPage,
                     MmHighestPhysicalPage,
                     4);
    }

#endif

    //
    // Build a parent directory page table for the kernel space
    //

    PdePageNumber = (PFN_NUMBER)LoaderBlock->u.Ia64.PdrPage;
    
    MmSystemParentTablePage = MiGetNextPhysicalPage();

    RtlZeroMemory(KSEG_ADDRESS(MmSystemParentTablePage), PAGE_SIZE);

    TempPte.u.Hard.PageFrameNumber = MmSystemParentTablePage;
    
    MiSystemSelfMappedPte = TempPte;

    KeFillFixedEntryTb((PHARDWARE_PTE)&TempPte, (PVOID)PDE_KTBASE, DTR_KTBASE_INDEX_TMP);

    //
    // Install the self-mapped Ppe entry into the kernel parent directory table
    //

    PointerPte = MiGetPteAddress(PDE_KTBASE);

    MI_WRITE_VALID_PTE(PointerPte, TempPte);

    //
    // Install the kernel image Ppe entry into the parent directory table
    //

    PointerPte = MiGetPteAddress(PDE_KBASE);

    TempPte.u.Hard.PageFrameNumber = PdePageNumber;

    MI_WRITE_VALID_PTE(PointerPte, TempPte);

    //
    // Build a parent directory page table for the user space
    //

    NextPhysicalPage = MiGetNextPhysicalPage();

    RtlZeroMemory (KSEG_ADDRESS(NextPhysicalPage), PAGE_SIZE);
    
    TempPte.u.Hard.PageFrameNumber = NextPhysicalPage;

    INITIALIZE_DIRECTORY_TABLE_BASE 
      (&(PsGetCurrentProcess()->Pcb.DirectoryTableBase[0]), NextPhysicalPage);

    MiUserSelfMappedPte = TempPte;

    KeFillFixedEntryTb((PHARDWARE_PTE)&TempPte, (PVOID)PDE_UTBASE, DTR_UTBASE_INDEX_TMP);

    //
    // Install the self-mapped entry into the user parent directory table
    //

    PointerPte = MiGetPteAddress(PDE_UTBASE);

    MI_WRITE_VALID_PTE(PointerPte, TempPte);

    //
    // Build a parent directory page table for the hydra space
    //

    NextPhysicalPage = MiGetNextPhysicalPage();

    RtlZeroMemory (KSEG_ADDRESS(NextPhysicalPage), PAGE_SIZE);
    
    TempPte.u.Hard.PageFrameNumber = NextPhysicalPage;

    INITIALIZE_DIRECTORY_TABLE_BASE 
      (&(PsGetCurrentProcess()->Pcb.SessionParentBase), NextPhysicalPage);

    KeFillFixedEntryTb((PHARDWARE_PTE)&TempPte, (PVOID)PDE_STBASE, DTR_STBASE_INDEX);

    //
    // Install the self-mapped entry into the hydra parent directory table
    //

    PointerPte = MiGetPteAddress(PDE_STBASE);

    MI_WRITE_VALID_PTE(PointerPte, TempPte);

    //
    // Build Vhpt table for KSEG3 
    //

#if defined(KSEG_VHPT)
    va = KSEG0_ADDRESS(MmKseg3VhptBase);
    
    RtlZeroMemory((PVOID)va, Kseg3VhptSize);

    TempPte.u.Hard.PageFrameNumber = MmKseg3VhptBase;

    PointerPte = MiGetKSegPteAddress(KSEG3_BASE);

    KeFillFixedLargeEntryTb((PHARDWARE_PTE)&TempPte, 
                            (PVOID)PointerPte,
                            PageSize,
                            DTR_KSEG3_INDEX);

    MmKseg3VhptPs = PageSize;
#endif 

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
            ((MmNumberOfPhysicalPages - _x16mbnp)/_x1mbnp) *
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

        MmMaximumNonPagedPoolInBytes += (MmHighestPhysicalPage * sizeof(MMPFN)) & ~(PAGE_SIZE-1);

        MmMaximumNonPagedPoolInBytes +=
            ((MmNumberOfPhysicalPages - _x16mbnp)/_x1mbnp) *
            MmMaxAdditionNonPagedPoolPerMb;

    }

    MaxPool = MmSizeOfNonPagedPoolInBytes + PAGE_SIZE * 16 +
                ((MmHighestPhysicalPage * sizeof(MMPFN)) & ~(PAGE_SIZE -1));

    if (MmMaximumNonPagedPoolInBytes < MaxPool) {
        MmMaximumNonPagedPoolInBytes = MaxPool;
    }

    if (MmMaximumNonPagedPoolInBytes > MM_MAX_ADDITIONAL_NONPAGED_POOL) {
        MmMaximumNonPagedPoolInBytes = MM_MAX_ADDITIONAL_NONPAGED_POOL;
    }

    MmNonPagedPoolStart = (PVOID)((ULONG_PTR)MmNonPagedPoolEnd
                                      - MmMaximumNonPagedPoolInBytes);

    MmNonPagedPoolStart = (PVOID)PAGE_ALIGN(MmNonPagedPoolStart);

    MmPageAlignedPoolBase[NonPagedPool] = MmNonPagedPoolStart;

    //
    // Calculate the starting PDE for the system PTE pool which is
    // right below the nonpaged pool.
    //

    MmNonPagedSystemStart = (PVOID)(((ULONG_PTR)MmNonPagedPoolStart -
                                ((MmNumberOfSystemPtes + 1) * PAGE_SIZE)) &
                                 (~PAGE_DIRECTORY2_MASK));

    if (MmNonPagedSystemStart < MM_LOWEST_NONPAGED_SYSTEM_START) {
        MmNonPagedSystemStart = MM_LOWEST_NONPAGED_SYSTEM_START;
        MmNumberOfSystemPtes = (ULONG)(((ULONG_PTR)MmNonPagedPoolStart -
                                (ULONG_PTR)MmNonPagedSystemStart) >> PAGE_SHIFT)-1;
        ASSERT (MmNumberOfSystemPtes > 1000);
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

    TempPte = ValidPdePde;
    StartPde = MiGetPdeAddress(HYPER_SPACE);
    StartPpe = MiGetPpeAddress(HYPER_SPACE); 
    EndPde = StartPde;
    First = (StartPpe->u.Hard.Valid == 0) ? TRUE : FALSE;

    while (StartPde <= EndPde) {

        if (First == TRUE || MiIsPteOnPdeBoundary(StartPde)) {
            First = FALSE;
            StartPpe = MiGetPteAddress(StartPde);
            if (StartPpe->u.Hard.Valid == 0) {
                ASSERT (StartPpe->u.Long == 0);
                NextPhysicalPage = MiGetNextPhysicalPage();
                RtlZeroMemory(KSEG_ADDRESS(NextPhysicalPage), PAGE_SIZE);
                TempPte.u.Hard.PageFrameNumber = NextPhysicalPage;
                MI_WRITE_VALID_PTE(StartPpe, TempPte);
            }
        }
        NextPhysicalPage = MiGetNextPhysicalPage();
        RtlZeroMemory(KSEG_ADDRESS(NextPhysicalPage), PAGE_SIZE);
        TempPte.u.Hard.PageFrameNumber = NextPhysicalPage;
        MI_WRITE_VALID_PTE(StartPde, TempPte);
        StartPde += 1;
    }

    //
    // Allocate page directory and page table pages for
    // system PTEs and nonpaged pool.
    //

    TempPte = ValidKernelPte;
    StartPde = MiGetPdeAddress(MmNonPagedSystemStart);
    StartPpe = MiGetPpeAddress(MmNonPagedSystemStart);
    EndPde = MiGetPdeAddress((ULONG_PTR)MmNonPagedPoolEnd - 1);
    First = (StartPpe->u.Hard.Valid == 0) ? TRUE : FALSE;

    while (StartPde <= EndPde) {

        if (First == TRUE || MiIsPteOnPdeBoundary(StartPde)) {
            First = FALSE;
            StartPpe = MiGetPteAddress(StartPde);
            if (StartPpe->u.Hard.Valid == 0) {
                NextPhysicalPage = MiGetNextPhysicalPage();
                RtlZeroMemory(KSEG_ADDRESS(NextPhysicalPage), PAGE_SIZE);
                TempPte.u.Hard.PageFrameNumber = NextPhysicalPage;
                MI_WRITE_VALID_PTE(StartPpe, TempPte);
            }
        }

        if (StartPde->u.Hard.Valid == 0) {
            NextPhysicalPage = MiGetNextPhysicalPage();
            RtlZeroMemory(KSEG_ADDRESS(NextPhysicalPage), PAGE_SIZE);
            TempPte.u.Hard.PageFrameNumber = NextPhysicalPage;
            MI_WRITE_VALID_PTE(StartPde, TempPte);
        }
        StartPde += 1;
    }

    NonPagedPoolStartVirtual = MmNonPagedPoolStart;

//    MiBuildPageTableForDrivers(LoaderBlock);


    MiBuildPageTableForLoaderMemory(LoaderBlock);

    MiRemoveLoaderSuperPages(LoaderBlock);

    //
    // remove the temporary super pages for the root page table pages,
    // and remap again with DTR_KTBASE_INDEX and DTR_UTBASE_INDEX. 
    //

    KiFlushFixedDataTb(FALSE, (PVOID)PDE_KTBASE);
    KiFlushFixedDataTb(FALSE, (PVOID)PDE_UTBASE);
    KeFillFixedEntryTb((PHARDWARE_PTE)&MiSystemSelfMappedPte, (PVOID)PDE_KTBASE, DTR_KTBASE_INDEX);
    KeFillFixedEntryTb((PHARDWARE_PTE)&MiUserSelfMappedPte, (PVOID)PDE_UTBASE, DTR_UTBASE_INDEX);

    if (MiKseg0Mapping == TRUE) {

        //
        // Fill the PDEs for KSEG0 space
        //

        StartPde = MiGetPdeAddress(MiKseg0Start);
        EndPde = MiGetPdeAddress((PCHAR)KSEG0_BASE + Kseg0Size);

        while (StartPde <= EndPde) {

            if (StartPde->u.Hard.Valid == 0) {
                NextPhysicalPage = MiGetNextPhysicalPage();
                RtlZeroMemory(KSEG_ADDRESS(NextPhysicalPage), PAGE_SIZE);
                TempPte.u.Hard.PageFrameNumber = NextPhysicalPage;
                MI_WRITE_VALID_PTE(StartPde, TempPte);
            }
            StartPde++;
        }
    }

    //
    // Initial NonPagedPool allocation priorities
    //
    // 1. allocate pages from 16mb kernel image maping space and use the 16mb super page 
    //    addresses
    // 2. allocate pages from low memory space ( < 256mb) and use the KSEG0 addresses
    // 3. allocate pages from the largest free memory descriptor list and use
    //    MM_NON_PAGED_POOL addresses.
    //   

    if (MiKseg0Mapping == TRUE) {

        //
        // When KiKseg0Mapping is enabled, check to see if 
        //

        MiKseg0Mapping = 
            MiEnsureAvailablePagesInFreeDescriptor(BYTES_TO_PAGES(MmSizeOfNonPagedPoolInBytes),
                                                   MM_PAGES_IN_KSEG0);

    }

    //
    // Fill in the PTEs to cover the initial nonpaged pool. The physical
    // page frames to cover this range were reserved earlier from the
    // largest low memory free descriptor. The initial allocation is both
    // physically and virtually contiguous.
    //

    PointerPte = MiGetPteAddress(MmNonPagedPoolStart);
    LastPte = MiGetPteAddress((PCHAR)MmNonPagedPoolStart +
                                                MmSizeOfNonPagedPoolInBytes - 1);

    while (PointerPte <= LastPte) {
        NextPhysicalPage = MiGetNextPhysicalPage();
        TempPte.u.Hard.PageFrameNumber = NextPhysicalPage;
        MI_WRITE_VALID_PTE(PointerPte, TempPte);
        PointerPte++;
    }

    //
    // Zero the remaining PTEs (if any) for the initial nonpaged pool up to
    // the end of the current page table page.
    //

    while (!MiIsPteOnPdeBoundary (PointerPte)) {
        MI_WRITE_INVALID_PTE(PointerPte, ZeroKernelPte);
        PointerPte += 1;
    }

    //
    // Convert the starting nonpaged pool address to KSEG0 space
    // address and set the address of the initial nonpaged pool allocation.
    //

    if (MiKseg0Mapping == TRUE) {


        //
        // No need to get page table pages for these as we can reference
        // them via large pages.
        //

        PointerPte = MiGetPteAddress (MmNonPagedPoolStart);
        MmNonPagedPoolStart = KSEG0_ADDRESS(PointerPte->u.Hard.PageFrameNumber);
        MmSubsectionBase = KSEG0_BASE;

        StartPte = MiGetPteAddress(MmNonPagedPoolStart); 
        LastPte = MiGetPteAddress((PCHAR)MmNonPagedPoolStart +
                                  MmSizeOfNonPagedPoolInBytes - 1);

        MiKseg0Start = MiGetVirtualAddressMappedByPte(StartPte);

        while (StartPte <= LastPte) {

            //
            // duplicating PTEs to map non initial non paged pool 
            // in the KSEG0 space.
            //

            MI_WRITE_VALID_PTE(StartPte, *PointerPte);
            StartPte++;
            PointerPte++;
        }

        MiKseg0End = MiGetVirtualAddressMappedByPte(LastPte);

    } else {
        
        MiKseg0Mapping = FALSE;
        MmSubsectionBase = 0;

    }

    //
    // As only the initial non paged pool is mapped through superpages, 
    // MmSubsectionToPage is always set to zero.  
    //

    MmSubsectionTopPage = 0;


    MmNonPagedPoolExpansionStart = (PVOID)((PCHAR)NonPagedPoolStartVirtual +
                                           MmSizeOfNonPagedPoolInBytes);

    MmPageAlignedPoolBase[NonPagedPool] = MmNonPagedPoolStart;


    //
    // Non-paged pages now exist, build the pool structures.
    //

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
    // Get secondary color value from registry.
    //

    MmSecondaryColors = MmSecondaryColors >> PAGE_SHIFT;

    if (MmSecondaryColors == 0) {
        MmSecondaryColors = MM_SECONDARY_COLORS_DEFAULT;
    } else {

        //
        // Make sure value is power of two and within limits.
        //

        if (((MmSecondaryColors & (MmSecondaryColors -1)) != 0) ||
            (MmSecondaryColors < MM_SECONDARY_COLORS_MIN) ||
            (MmSecondaryColors > MM_SECONDARY_COLORS_MAX)) {
            MmSecondaryColors = MM_SECONDARY_COLORS_DEFAULT;
        }
    }

    MmSecondaryColorMask = MmSecondaryColors - 1;

    //
    // Get the number of secondary colors and add the arrary for tracking
    // secondary colors to the end of the PFN database.
    //


    PfnAllocation = 1 + ((((MmHighestPhysicalPage + 1) * sizeof(MMPFN)) +
                        (MmSecondaryColors * sizeof(MMCOLOR_TABLES)*2))
                            >> PAGE_SHIFT);


    if ((MmHighestPhysicalPage < _x4gbnp) && 
        (MiKseg0Mapping == TRUE) &&
        (MiEnsureAvailablePagesInFreeDescriptor(PfnAllocation,
                                                MM_PAGES_IN_KSEG0))) {
        //
        // Allocate the PFN database in the superpage space
        //
        // Compute the address of the PFN by allocating the appropriate
        // number of pages from the end of the free descriptor.
        //

        MmPfnDatabase = (PMMPFN)KSEG0_ADDRESS(MiNextPhysicalPage);

        StartPte = MiGetPteAddress(MmPfnDatabase); 
        LastPte = MiGetPteAddress((PCHAR)MmPfnDatabase + (PfnAllocation << PAGE_SHIFT) - 1);

        while (StartPte <= LastPte) {
            TempPte.u.Hard.PageFrameNumber = MiGetNextPhysicalPage();
            MI_WRITE_VALID_PTE(StartPte, TempPte);
            StartPte++;
        }

        RtlZeroMemory(MmPfnDatabase, PfnAllocation * PAGE_SIZE);
        MiKseg0End = MiGetVirtualAddressMappedByPte(LastPte);

    } else {

        //
        // Calculate the start of the Pfn Database (it starts a physical
        // page zero, even if the Lowest physical page is not zero).
        //

        MmPfnDatabase = (PMMPFN)MM_PFN_DATABASE_START;

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
            
            First = TRUE;

            while (PointerPte <= LastPte) {

                if (First == TRUE || MiIsPteOnPpeBoundary(PointerPte)) {
                    StartPpe = MiGetPdeAddress(PointerPte);
                    if (StartPpe->u.Hard.Valid == 0) {
                        ASSERT (StartPpe->u.Long == 0);
                        NextPhysicalPage = MiGetNextPhysicalPage();
                        RtlZeroMemory(KSEG_ADDRESS(NextPhysicalPage), PAGE_SIZE);
                        TempPte.u.Hard.PageFrameNumber = NextPhysicalPage;
                        MI_WRITE_VALID_PTE(StartPpe, TempPte);
                    }
                }

                if ((First == TRUE) || MiIsPteOnPdeBoundary(PointerPte)) {
                    First = FALSE;
                    StartPde = MiGetPteAddress(PointerPte);
                    if (StartPde->u.Hard.Valid == 0) {
                        NextPhysicalPage = MiGetNextPhysicalPage();
                        RtlZeroMemory(KSEG_ADDRESS(NextPhysicalPage), PAGE_SIZE);
                        TempPte.u.Hard.PageFrameNumber = NextPhysicalPage;
                        MI_WRITE_VALID_PTE(StartPde, TempPte);
                    }
                }

                if (PointerPte->u.Hard.Valid == 0) {
                    NextPhysicalPage = MiGetNextPhysicalPage();
                    RtlZeroMemory(KSEG_ADDRESS(NextPhysicalPage), PAGE_SIZE);
                    TempPte.u.Hard.PageFrameNumber = NextPhysicalPage;
                    MI_WRITE_VALID_PTE(PointerPte, TempPte);
                }

                PointerPte++;
            }
            NextMd = MemoryDescriptor->ListEntry.Flink;
        }
    }

    if (MiKseg0Mapping == TRUE) {

        //
        // Try to convert to superpages
        //

        MiConvertToSuperPages(MiKseg0Start, 
                              MiKseg0End,
                              _x1mb,
                              MM_PTE_1MB_PAGE);

        MiConvertToSuperPages(MiKseg0Start,
                              MiKseg0End,
                              _x4mb,
                              MM_PTE_4MB_PAGE);

        MiConvertToSuperPages(MiKseg0Start, 
                              MiKseg0End,
                              _x16mb,
                              MM_PTE_16MB_PAGE);

        MiConvertToSuperPages(MiKseg0Start, 
                              MiKseg0End,
                              _x64mb,
                              MM_PTE_64MB_PAGE);

        MiConvertToSuperPages(MiKseg0Start, 
                              MiKseg0End,
                              _x256mb,
                              MM_PTE_256MB_PAGE);

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
                NextPhysicalPage = MiGetNextPhysicalPage();
                RtlZeroMemory(KSEG_ADDRESS(NextPhysicalPage), PAGE_SIZE);
                TempPte.u.Hard.PageFrameNumber = NextPhysicalPage;
                MI_WRITE_VALID_PTE(PointerPte, TempPte);
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

    StartPde = MiGetPdeAddress (HYPER_SPACE);
    StartPpe = MiGetPpeAddress (HYPER_SPACE);
    EndPde = MiGetPdeAddress(HYPER_SPACE_END);
    First = (StartPpe->u.Hard.Valid == 0) ? TRUE : FALSE;

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
            Pfn1->PteFrame = MmSystemParentTablePage;
            Pfn1->PteAddress = StartPde;
            Pfn1->u2.ShareCount += 1;
            Pfn1->u3.e2.ReferenceCount = 1;
            Pfn1->u3.e1.PageLocation = ActiveAndValid;
            Pfn1->u3.e1.PageColor =
                MI_GET_COLOR_FROM_SECONDARY(GET_PAGE_COLOR_FROM_PTE(StartPpe));
        }


        if (StartPde->u.Hard.Valid == 1) {
            PdePage = MI_GET_PAGE_FRAME_FROM_PTE(StartPde);
            Pfn1 = MI_PFN_ELEMENT(PdePage);
            PointerPde = MiGetPteAddress(StartPde);
            Pfn1->PteFrame = MI_GET_PAGE_FRAME_FROM_PTE(PointerPde);
            Pfn1->PteAddress = StartPde;
            Pfn1->u2.ShareCount += 1;
            Pfn1->u3.e2.ReferenceCount = 1;
            Pfn1->u3.e1.PageLocation = ActiveAndValid;
            Pfn1->u3.e1.PageColor =
                MI_GET_COLOR_FROM_SECONDARY(GET_PAGE_COLOR_FROM_PTE (StartPde));

            PointerPte = MiGetVirtualAddressMappedByPte(StartPde);
            for (j = 0 ; j < PTE_PER_PAGE; j++) {
                if (PointerPte->u.Hard.Valid == 1) {

                    Pfn1->u2.ShareCount += 1;

                    if (PointerPte->u.Hard.PageFrameNumber <=
                                            MmHighestPhysicalPage) {
                        Pfn2 = MI_PFN_ELEMENT(PointerPte->u.Hard.PageFrameNumber);
                        Pfn2->PteFrame = PdePage;
                        Pfn2->PteAddress = PointerPte;
                        Pfn2->u2.ShareCount += 1;
                        Pfn2->u3.e2.ReferenceCount = 1;
                        Pfn2->u3.e1.PageLocation = ActiveAndValid;
                        Pfn2->u3.e1.PageColor =
                            MI_GET_COLOR_FROM_SECONDARY(
                                                  MI_GET_PAGE_COLOR_FROM_PTE (
                                                        PointerPte));
                    }
                }
                PointerPte += 1;
            }
        }

        StartPde++;
    }

    //
    // do it for the kernel space
    //

    StartPde = MiGetPdeAddress (KADDRESS_BASE);
    StartPpe = MiGetPpeAddress (KADDRESS_BASE);
    EndPde = MiGetPdeAddress(MM_SYSTEM_SPACE_END);
    First = (StartPpe->u.Hard.Valid == 0) ? TRUE : FALSE;

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
            Pfn1->PteFrame = MmSystemParentTablePage;
            Pfn1->PteAddress = StartPde;
            Pfn1->u2.ShareCount += 1;
            Pfn1->u3.e2.ReferenceCount = 1;
            Pfn1->u3.e1.PageLocation = ActiveAndValid;
            Pfn1->u3.e1.PageColor =
                MI_GET_COLOR_FROM_SECONDARY(GET_PAGE_COLOR_FROM_PTE(StartPpe));
        }

        if (StartPde->u.Hard.Valid == 1) {
            PdePage = MI_GET_PAGE_FRAME_FROM_PTE(StartPde);
            Pfn1 = MI_PFN_ELEMENT(PdePage);
            PointerPde = MiGetPteAddress(StartPde);
            Pfn1->PteFrame = MI_GET_PAGE_FRAME_FROM_PTE(PointerPde);
            Pfn1->PteAddress = StartPde;
            Pfn1->u2.ShareCount += 1;
            Pfn1->u3.e2.ReferenceCount = 1;
            Pfn1->u3.e1.PageLocation = ActiveAndValid;
            Pfn1->u3.e1.PageColor =
                MI_GET_COLOR_FROM_SECONDARY(GET_PAGE_COLOR_FROM_PTE (StartPde));

            PointerPte = MiGetVirtualAddressMappedByPte(StartPde);
            for (j = 0 ; j < PTE_PER_PAGE; j++) {
                if (PointerPte->u.Hard.Valid == 1) {

                    Pfn1->u2.ShareCount += 1;

                    if (PointerPte->u.Hard.PageFrameNumber <=
                                            MmHighestPhysicalPage) {
                        Pfn2 = MI_PFN_ELEMENT(PointerPte->u.Hard.PageFrameNumber);
                        Pfn2->PteFrame = PdePage;
                        Pfn2->PteAddress = PointerPte;
                        Pfn2->u2.ShareCount += 1;
                        Pfn2->u3.e2.ReferenceCount = 1;
                        Pfn2->u3.e1.PageLocation = ActiveAndValid;
                        Pfn2->u3.e1.PageColor =
                            MI_GET_COLOR_FROM_SECONDARY(
                                                  MI_GET_PAGE_COLOR_FROM_PTE (
                                                        PointerPte));
                    }
                }
                PointerPte += 1;
            }
        }

        StartPde++;
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

        Pde = MiGetPdeAddress (KADDRESS_BASE + 0xb0000000);
        PdePage = MI_GET_PAGE_FRAME_FROM_PTE(Pde);
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

    MiFreeDescriptor->PageCount -= 
        (PFN_COUNT)(MiNextPhysicalPage - MiOldFreeDescriptorBase);

    //
    // Until BasePage (arc.h) is changed to PFN_NUMBER, NextPhysicalPage
    // needs (ULONG) cast.   
    //

    MiFreeDescriptor->BasePage = (ULONG)MiNextPhysicalPage;

    //
    // making unused pages inside the kernel super page mapping unusable
    // so that no one is going to reclaim it for uncached pages.
    //

    if (MiFreeDescriptorNonPaged != NULL) {

        if (MiFreeDescriptorNonPaged->BasePage > KernelEnd) {
            
            MiWasteStart = KernelEnd;
            MiWasteEnd = KernelEnd;

            
        } else if ((MiFreeDescriptorNonPaged->BasePage + 
                    MiFreeDescriptorNonPaged->PageCount) > KernelEnd) {
        
            MiWasteStart = MiFreeDescriptorNonPaged->BasePage;
            MiWasteEnd = KernelEnd;

            MiFreeDescriptorNonPaged->PageCount -=  
                (PFN_COUNT) (KernelEnd - MiFreeDescriptorNonPaged->BasePage);

            MiFreeDescriptorNonPaged->BasePage = (ULONG) KernelEnd;

        } else if (MiFreeDescriptorNonPaged->PageCount != 0) {

            MiWasteStart = MiFreeDescriptorNonPaged->BasePage;
            MiWasteEnd = MiWasteStart + MiFreeDescriptorNonPaged->PageCount;
            MiFreeDescriptorNonPaged->PageCount = 0;
            
        }
    }

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

                        Pfn1->PteAddress = KSEG_ADDRESS (NextPhysicalPage);
                        MiInsertPageInList (MmPageLocationList[FreePageList],
                                            NextPhysicalPage);
                    }
                    Pfn1++;
                    i -= 1;
                    NextPhysicalPage += 1;
                }
                break;

            default:

                PointerPte = KSEG_ADDRESS(NextPhysicalPage);
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

    if (MI_IS_PHYSICAL_ADDRESS(MmPfnDatabase) == FALSE) {

        //
        // Indicate that the PFN database is allocated in NonPaged pool.
        //

        PointerPte = MiGetPteAddress (&MmPfnDatabase[MmLowestPhysicalPage]);
        Pfn1 = MI_PFN_ELEMENT(PointerPte->u.Hard.PageFrameNumber);
        Pfn1->u3.e1.StartOfAllocation = 1;

        //
        // Set the end of the allocation.
        //

        PointerPte = MiGetPteAddress (&MmPfnDatabase[MmHighestPhysicalPage]);
        Pfn1 = MI_PFN_ELEMENT(PointerPte->u.Hard.PageFrameNumber);
        Pfn1->u3.e1.EndOfAllocation = 1;

    } else {
        //
        // The PFN database is allocated in the superpage space
        //
        // Mark all pfn entries for the pfn pages in use.
        //

        PageFrameIndex = MI_CONVERT_PHYSICAL_TO_PFN (MmPfnDatabase);
        Pfn1 = MI_PFN_ELEMENT(PageFrameIndex);
        do {
            Pfn1->PteAddress = KSEG_ADDRESS(PageFrameIndex);
            Pfn1->u3.e1.PageColor = 0;
            Pfn1->u3.e2.ReferenceCount += 1;
            Pfn1->u3.e1.PageLocation = ActiveAndValid;
            PageFrameIndex += 1;
            Pfn1 += 1;
            PfnAllocation -= 1;
        } while (PfnAllocation != 0);

        //
        // To avoid creating WB/UC/WC aliasing problem, we should not scan 
        // and add free pages to the free list. 
        //
#if 0
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

            Range = (ULONG_PTR)TopPfn - (ULONG_PTR)BottomPfn;
            if (RtlCompareMemoryUlong((PVOID)BottomPfn, Range, 0) == Range) {

                //
                // Set the PTE address to the physical page for virtual
                // address alignment checking.
                //

                PageFrameIndex = MI_CONVERT_PHYSICAL_TO_PFN (BasePfn);
                Pfn1 = MI_PFN_ELEMENT(PageFrameIndex);

                ASSERT (Pfn1->u3.e2.ReferenceCount == 1);
                ASSERT (Pfn1->PteAddress == KSEG_ADDRESS(PageFrameIndex));
                Pfn1->u3.e2.ReferenceCount = 0;
                PfnAllocation += 1;
                Pfn1->PteAddress = (PMMPTE)((ULONG_PTR)PageFrameIndex << PTE_SHIFT);
                Pfn1->u3.e1.PageColor = 0;
                MiInsertPageInList(MmPageLocationList[FreePageList],
                                   PageFrameIndex);
            }

        } while (BottomPfn > MmPfnDatabase);
#endif

    }


    //
    // Indicate that nonpaged pool must succeed is allocated in
    // nonpaged pool.
    //

    if (MI_IS_PHYSICAL_ADDRESS(MmNonPagedMustSucceed)) {
        Pfn1 = MI_PFN_ELEMENT(MI_CONVERT_PHYSICAL_TO_PFN (MmNonPagedMustSucceed));
    } else {
        PointerPte = MiGetPteAddress(MmNonPagedMustSucceed);
        Pfn1 = MI_PFN_ELEMENT(PointerPte->u.Hard.PageFrameNumber);
    }

    i = MmSizeOfNonPagedMustSucceed;
    while ((LONG)i > 0) {
        Pfn1->u3.e1.StartOfAllocation = 1;
        Pfn1->u3.e1.EndOfAllocation = 1;
        i -= PAGE_SIZE;
        Pfn1 += 1;
    }

#if 0
    //
    // Adjust the memory descriptors to indicate that free pool has
    // been used for nonpaged pool creation.
    //

    MiFreeDescriptor->PageCount = (ULONG)MiOldFreeDescriptorCount;

    //
    // Until PagePage is defined to PFN_NUMBER, we need (ULONG) cast to 
    // remove warning. 
    //

    MiFreeDescriptor->BasePage = (ULONG)MiOldFreeDescriptorBase;

#endif

// moved from above for pool hack routines...
    KeInitializeSpinLock (&MmSystemSpaceLock);

    KeInitializeSpinLock (&MmPfnLock);

    //
    // Initialize the nonpaged available PTEs for mapping I/O space
    // and kernel stacks.
    //

    PointerPte = MiGetPteAddress (MmNonPagedSystemStart);
    ASSERT (((ULONG_PTR)PointerPte & (PAGE_SIZE - 1)) == 0);

    MmNumberOfSystemPtes = (ULONG)(MiGetPteAddress(NonPagedPoolStartVirtual) - PointerPte - 1);

    MiInitializeSystemPtes (PointerPte, MmNumberOfSystemPtes, SystemPteSpace);

    //
    // Initialize the nonpaged pool.
    //

    InitializePool(NonPagedPool,0);

    //
    // Initialize memory management structures for the system process.
    //
    // Set the address of the first and last reserved PTE in hyper space.
    //

    MmFirstReservedMappingPte = MiGetPteAddress (FIRST_MAPPING_PTE);
    MmLastReservedMappingPte = MiGetPteAddress (LAST_MAPPING_PTE);

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

    CurrentProcess = PsGetCurrentProcess ();

    //
    // Get a page for the working set list and map it into the Page
    // directory at the page after hyperspace.
    //

    PageFrameIndex = MiRemoveAnyPage (0);

    CurrentProcess->WorkingSetPage = PageFrameIndex;
    TempPte.u.Hard.PageFrameNumber = PageFrameIndex;

    RtlZeroMemory (KSEG_ADDRESS(PageFrameIndex), PAGE_SIZE);

    CurrentProcess->Vm.MaximumWorkingSetSize = (ULONG)MmSystemProcessWorkingSetMax;
    CurrentProcess->Vm.MinimumWorkingSetSize = (ULONG)MmSystemProcessWorkingSetMin;

    MmInitializeProcessAddressSpace (CurrentProcess,
                                (PEPROCESS)NULL,
                                (PVOID)NULL,
                                (PVOID)NULL);

    
    //    *PointerPde = ZeroPte;

    //
    // page after hyperspace should be reclaimed for system cache structure
    //

    KeFlushCurrentTb();

    //
    // Check to see if moving the secondary page structures to the end
    // of the PFN database is a waste of memory.  And if so, copy it
    // to paged pool.
    //
    // If the PFN datbase ends on a page aligned boundary and the
    // size of the two arrays is less than a page, free the page
    // and allocate nonpagedpool for this.
    //

    if ((((ULONG_PTR)MmFreePagesByColor[0] & (PAGE_SIZE - 1)) == 0) &&
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
            PageFrameIndex = MI_GET_PAGE_FRAME_FROM_PTE(PointerPte);
            MI_WRITE_INVALID_PTE(PointerPte, ZeroKernelPte);
        } else {
            PageFrameIndex = MI_CONVERT_PHYSICAL_TO_PFN (c);
        }

        Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);
        ASSERT ((Pfn1->u2.ShareCount <= 1) && (Pfn1->u3.e2.ReferenceCount <= 1));
        Pfn1->u2.ShareCount = 0;
        Pfn1->u3.e2.ReferenceCount = 1;
        MI_SET_PFN_DELETED (Pfn1);
#if DBG
        Pfn1->u3.e1.PageLocation = StandbyPageList;
#endif //DBG
        MiDecrementReferenceCount (PageFrameIndex);
    }

    return;
}

PVOID
MiGetKSegAddress (
    PFN_NUMBER FrameNumber
    )
/*++

Routine Description:

    This function returns the KSEG3 address which maps the given physical page.

Arguments:

   FrameNumber - Supplies the physical page number to get the KSEG3 address for 

Return Value:

    Virtual address mapped in KSEG3 space

    TBS

--*/
{
    PVOID Virtual;
    PMMPTE PointerPte;
    MMPTE TempPte;

    ASSERT (FrameNumber <= MmHighestPhysicalPage);
 
    Virtual = ((PVOID)(KSEG3_BASE | ((ULONG_PTR)(FrameNumber) << PAGE_SHIFT)));
    
#if defined(KSEG_VHPT)
    PointerPte = MiGetKSegPteAddress (Virtual);

    if (PointerPte->u.Long == 0) {

        //
        // if the VHPT entry for the KSEG3 address is still zero, build it here.
        //

        TempPte = ValidKernelPte;
        TempPte.u.Hard.PageFrameNumber = FrameNumber;
        MI_WRITE_VALID_PTE(PointerPte, TempPte);

        __mf();
    }
#endif

    return (Virtual);
}

VOID
MiConvertToSuperPages(
    IN PVOID StartVirtual,
    IN PVOID EndVirtual,
    IN SIZE_T PageSize,
    IN ULONG PageShift
    )
/*++

Routine Description:

    This function makes contiguous non-paged memory use super pages rather than 
    using page tables. 

Arguments:

    StartVirtual - the start address of the region of pages to be mapped by 
          super pages.
 
    EndVirtual - the end address of the region of pages to be mapped by super
          pages.

    Page Size - the page size to be used by the super page.

    Page Shift - the page shift count to be used by the super page.

Return Value:

    None.

--*/
{
    ULONG_PTR VirtualAddress;
    ULONG_PTR i;
    ULONG_PTR NumberOfPte;
    PMMPTE StartPte;
    
    VirtualAddress = (ULONG_PTR) PAGE_ALIGN(StartVirtual);
    i = VirtualAddress & (PageSize - 1);

    if (i != 0) {

        VirtualAddress = (VirtualAddress + PageSize - 1) & ~(PageSize - 1); 

    }

    StartPte = MiGetPteAddress(VirtualAddress);
    NumberOfPte = PageSize >> PAGE_SHIFT;

    i = 0;

    while (VirtualAddress <= (ULONG_PTR)EndVirtual) {

        if (i == NumberOfPte) {
            
            StartPte -= NumberOfPte;

            for (i = 0; i < NumberOfPte; i++) {
                
                StartPte->u.Hard.Valid = 0;
                StartPte->u.Large.LargePage = 1;
                StartPte->u.Large.PageSize = PageShift;
                StartPte += 1;
            }

            i = 0;
        }

        i += 1;
        StartPte += 1;
        VirtualAddress += PAGE_SIZE;
    }
}

VOID
MiConvertBackToStandardPages(
    IN PVOID StartVirtual,
    IN PVOID EndVirtual
    )
/*++

Routine Description:

    This function disables the use of the super pages.

Arguments:

    StartVirtual - the start address of the region of pages to disable super pages.
          super pages.
 
    EndVirtual - the end address of the region of pages to disable super pages.

Return Value:

    None.

--*/
{
    ULONG_PTR VirtualAddress;
    ULONG_PTR i;
    ULONG_PTR NumberOfPte;
    PMMPTE StartPte;
    MMPTE TempPte;
    
    VirtualAddress = (ULONG_PTR) PAGE_ALIGN(StartVirtual);

    StartPte = MiGetPteAddress(VirtualAddress);

    while (VirtualAddress <= (ULONG_PTR)EndVirtual) {

        TempPte = *StartPte;
        TempPte.u.Large.PageSize = 0;
        TempPte.u.Large.PageSize = 0;
        TempPte.u.Hard.Valid = 1;
        MI_WRITE_VALID_PTE (StartPte, TempPte);

        StartPte += 1;
        VirtualAddress += PAGE_SIZE;
    }
}

VOID
MiSweepCacheMachineDependent(
    IN PVOID VirtualAddress,
    IN SIZE_T Size,
    IN MEMORY_CACHING_TYPE CacheType
    ) 
/*++

Routine Description:

    This function checks and perform appropriate cache flushing operations.

Arguments:

    StartVirtual - the start address of the region of pages to be examined.
 
    Size - the size of the region of pages

    Cache - the new cache type 

Return Value:

    None.

--*/
{
    PFN_NUMBER j;
    PFN_NUMBER NumberOfPages;
    KIRQL OldIrql;
    PMMPTE PointerPte;
    MMPTE TempPte;
            
    NumberOfPages = COMPUTE_PAGES_SPANNED (VirtualAddress, Size);
    VirtualAddress = PAGE_ALIGN(VirtualAddress);
    Size = NumberOfPages * PAGE_SIZE;

    KeSweepCacheRangeWithDrain(TRUE, VirtualAddress, (ULONG)Size);

    if (CacheType == MmWriteCombined) {
        PointerPte = MiGetPteAddress(VirtualAddress);
        for (j = 0; j < NumberOfPages; j += 1) {
            TempPte = *PointerPte;
            MI_SET_PTE_WRITE_COMBINE (TempPte);
            MI_WRITE_VALID_PTE (PointerPte, TempPte);
            PointerPte += 1;
        }
    }
}

VOID
MiBuildPageTableForDrivers(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
    )
/*++

Routine Description:

    This function builds page ables for loader loaded drivers.

Arguments:

    LoaderBlock - Supplies the address of the loader block.
 
Return Value:

    None.

--*/
{
    PMMPTE StartPte;
    PMMPTE EndPte;
    PMMPTE StartPde;
    PMMPTE StartPpe;
    MMPTE TempPte;
    ULONG First;
    ULONG_PTR i;
    PLIST_ENTRY NextEntry;
    ULONG NumberOfLoaderPtes;
    PFN_NUMBER NextPhysicalPage;
    PVOID Va;
    PLDR_DATA_TABLE_ENTRY DataTableEntry;

    TempPte = ValidKernelPte;
    TempPte.u.Long |= MM_PTE_EXECUTE;

    i = 0;
    NextEntry = LoaderBlock->LoadOrderListHead.Flink;

    for ( ; NextEntry != &LoaderBlock->LoadOrderListHead; NextEntry = NextEntry->Flink) {

        //
        // As it is mapped through the translation registers, skip the kernel.
        //

        i += 1;
        if (i <= 1) {
            continue;
        }

        DataTableEntry = CONTAINING_RECORD(NextEntry,
                                           LDR_DATA_TABLE_ENTRY,
                                           InLoadOrderLinks);

        NumberOfLoaderPtes = (ULONG)((ROUND_TO_PAGES(DataTableEntry->SizeOfImage)) >> PAGE_SHIFT);

        Va = DataTableEntry->DllBase;
        StartPte = MiGetPteAddress(Va);
        EndPte = StartPte + NumberOfLoaderPtes;

        First = TRUE;

        while (StartPte <= EndPte) {

            if (First == TRUE || MiIsPteOnPpeBoundary(StartPte)) {
                StartPpe = MiGetPdeAddress(StartPte);
                if (StartPpe->u.Hard.Valid == 0) {
                    ASSERT (StartPpe->u.Long == 0);
                    NextPhysicalPage = MiGetNextPhysicalPage();
                    RtlZeroMemory(KSEG_ADDRESS(NextPhysicalPage), PAGE_SIZE);
                    TempPte.u.Hard.PageFrameNumber = NextPhysicalPage;
                    MI_WRITE_VALID_PTE(StartPpe, TempPte);
                }
            }

            if ((First == TRUE) || MiIsPteOnPdeBoundary(StartPte)) {
                First = FALSE;
                StartPde = MiGetPteAddress(StartPte);
                if (StartPde->u.Hard.Valid == 0) {
                    NextPhysicalPage = MiGetNextPhysicalPage();
                    RtlZeroMemory(KSEG_ADDRESS(NextPhysicalPage), PAGE_SIZE);
                    TempPte.u.Hard.PageFrameNumber = NextPhysicalPage;
                    MI_WRITE_VALID_PTE(StartPde, TempPte);
                }
            }

            TempPte.u.Hard.PageFrameNumber = MI_CONVERT_PHYSICAL_TO_PFN(Va); 
            MI_WRITE_VALID_PTE (StartPte, TempPte);
            StartPte += 1;
            Va = (PVOID)((ULONG_PTR)Va + PAGE_SIZE);
        }
    }
}

PVOID
MiConvertToLoaderVirtual(
    IN PFN_NUMBER Page,
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
    )
{
    ULONG_PTR PageAddress = Page << PAGE_SHIFT;
    PTR_INFO ItrInfo = &LoaderBlock->u.Ia64.ItrInfo[0];
    

    if ((PageAddress >= ItrInfo[ITR_KERNEL_INDEX].PhysicalAddress) &&
        (PageAddress <= ItrInfo[ITR_KERNEL_INDEX].PhysicalAddress + 
         ((ULONG_PTR)1 << ItrInfo[ITR_KERNEL_INDEX].PageSize))) {

        return (PVOID)(ItrInfo[ITR_KERNEL_INDEX].VirtualAddress + 
                       (PageAddress - ItrInfo[ITR_KERNEL_INDEX].PhysicalAddress));

    } else if ((PageAddress >= ItrInfo[ITR_DRIVER0_INDEX].PhysicalAddress) &&
        (PageAddress <= ItrInfo[ITR_DRIVER0_INDEX].PhysicalAddress + 
         ((ULONG_PTR)1 << ItrInfo[ITR_DRIVER0_INDEX].PageSize))) {

        return (PVOID)(ItrInfo[ITR_DRIVER0_INDEX].VirtualAddress + 
                       (PageAddress - ItrInfo[ITR_DRIVER0_INDEX].PhysicalAddress));

    } else if ((PageAddress >= ItrInfo[ITR_DRIVER1_INDEX].PhysicalAddress) &&
        (PageAddress <= ItrInfo[ITR_DRIVER1_INDEX].PhysicalAddress + 
         ((ULONG_PTR)1 << ItrInfo[ITR_DRIVER1_INDEX].PageSize))) {

        return (PVOID)(ItrInfo[ITR_DRIVER1_INDEX].VirtualAddress + 
                       (PageAddress - ItrInfo[ITR_DRIVER1_INDEX].PhysicalAddress));

    } else {

        KeBugCheckEx (MEMORY_MANAGEMENT,
                      0x01010101,
                      PageAddress,
                      (ULONG_PTR)&ItrInfo[0],
                      (ULONG_PTR)LoaderBlock);

        return 0;
    }
}


VOID
MiBuildPageTableForLoaderMemory(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
    )
/*++

Routine Description:

    This function builds page ables for loader loaded drivers and loader 
    allocated memory.

Arguments:

    LoaderBlock - Supplies the address of the loader block.
 
Return Value:

    None.

--*/
{
    PMMPTE StartPte;
    PMMPTE EndPte;
    PMMPTE StartPde;
    PMMPTE StartPpe;
    MMPTE TempPte;
    ULONG First;
    PLIST_ENTRY NextEntry;
    PFN_NUMBER NextPhysicalPage;
    PVOID Va;
    PFN_NUMBER PfnNumber;
    PMEMORY_ALLOCATION_DESCRIPTOR MemoryDescriptor;

    TempPte = ValidKernelPte;
    NextEntry = LoaderBlock->MemoryDescriptorListHead.Flink;

    for ( ; NextEntry != &LoaderBlock->MemoryDescriptorListHead; NextEntry = NextEntry->Flink) {

        MemoryDescriptor = CONTAINING_RECORD(NextEntry,
                                             MEMORY_ALLOCATION_DESCRIPTOR,
                                             ListEntry);

        if ((MemoryDescriptor->MemoryType == LoaderOsloaderHeap) ||
            (MemoryDescriptor->MemoryType == LoaderRegistryData) ||
            (MemoryDescriptor->MemoryType == LoaderNlsData) ||
            (MemoryDescriptor->MemoryType == LoaderStartupDpcStack) || 
            (MemoryDescriptor->MemoryType == LoaderStartupKernelStack) ||
            (MemoryDescriptor->MemoryType == LoaderStartupPanicStack) ||
            (MemoryDescriptor->MemoryType == LoaderStartupPdrPage) ||
            (MemoryDescriptor->MemoryType == LoaderMemoryData)) {
            
            TempPte.u.Hard.Execute = 0;

        } else if ((MemoryDescriptor->MemoryType == LoaderSystemCode) ||
                   (MemoryDescriptor->MemoryType == LoaderHalCode) ||
                   (MemoryDescriptor->MemoryType == LoaderBootDriver) ||
                   (MemoryDescriptor->MemoryType == LoaderStartupDpcStack)) {
            
            TempPte.u.Hard.Execute = 1;

        } else {

            continue;

        }

        PfnNumber = MemoryDescriptor->BasePage;
        Va = MiConvertToLoaderVirtual(MemoryDescriptor->BasePage, LoaderBlock);

        StartPte = MiGetPteAddress(Va);
        EndPte = StartPte + MemoryDescriptor->PageCount;

        First = TRUE;

        while (StartPte <= EndPte) {

            if (First == TRUE || MiIsPteOnPpeBoundary(StartPte)) {
                StartPpe = MiGetPdeAddress(StartPte);
                if (StartPpe->u.Hard.Valid == 0) {
                    ASSERT (StartPpe->u.Long == 0);
                    NextPhysicalPage = MiGetNextPhysicalPage();
                    RtlZeroMemory(KSEG_ADDRESS(NextPhysicalPage), PAGE_SIZE);
                    TempPte.u.Hard.PageFrameNumber = NextPhysicalPage;
                    MI_WRITE_VALID_PTE(StartPpe, TempPte);
                }
            }

            if ((First == TRUE) || MiIsPteOnPdeBoundary(StartPte)) {
                First = FALSE;
                StartPde = MiGetPteAddress(StartPte);
                if (StartPde->u.Hard.Valid == 0) {
                    NextPhysicalPage = MiGetNextPhysicalPage();
                    RtlZeroMemory(KSEG_ADDRESS(NextPhysicalPage), PAGE_SIZE);
                    TempPte.u.Hard.PageFrameNumber = NextPhysicalPage;
                    MI_WRITE_VALID_PTE(StartPde, TempPte);
                }
            }

            TempPte.u.Hard.PageFrameNumber = PfnNumber;
            MI_WRITE_VALID_PTE (StartPte, TempPte);
            StartPte += 1;
            PfnNumber += 1;
            Va = (PVOID)((ULONG_PTR)Va + PAGE_SIZE);
        }
    }
}


VOID
MiRemoveLoaderSuperPages(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
    )
{

    //
    //  remove the super pages for the boot drivers
    //

    KiFlushFixedInstTb(FALSE, LoaderBlock->u.Ia64.ItrInfo[ITR_DRIVER0_INDEX].VirtualAddress);
    KiFlushFixedInstTb(FALSE, LoaderBlock->u.Ia64.ItrInfo[ITR_DRIVER1_INDEX].VirtualAddress);
    KiFlushFixedDataTb(FALSE, LoaderBlock->u.Ia64.DtrInfo[DTR_DRIVER0_INDEX].VirtualAddress);
    KiFlushFixedDataTb(FALSE, LoaderBlock->u.Ia64.DtrInfo[DTR_DRIVER1_INDEX].VirtualAddress);

}

VOID
MiMakeKernelPagesPermanent(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
    )
{
    PFN_NUMBER KernelStart;
    PFN_NUMBER KernelEnd;
    ULONG_PTR PageSize;
    PLIST_ENTRY NextEntry;
    PMEMORY_ALLOCATION_DESCRIPTOR MemoryDescriptor;

    KernelStart = MiNtoskrnlPhysicalBase >> PAGE_SHIFT;
    PageSize = (ULONG_PTR)1 << MiNtoskrnlPageShift;
    KernelEnd = KernelStart + (PageSize >> PAGE_SHIFT);

    NextEntry = LoaderBlock->MemoryDescriptorListHead.Flink;

    for ( ; NextEntry != &LoaderBlock->MemoryDescriptorListHead; NextEntry = NextEntry->Flink) {

        MemoryDescriptor = CONTAINING_RECORD(NextEntry,
                                             MEMORY_ALLOCATION_DESCRIPTOR,
                                             ListEntry);

        if (((MemoryDescriptor->BasePage >= KernelStart) && 
             (MemoryDescriptor->BasePage < KernelEnd)) && 
            ((MemoryDescriptor->MemoryType == LoaderOsloaderHeap) ||
             (MemoryDescriptor->MemoryType == LoaderRegistryData) ||
             (MemoryDescriptor->MemoryType == LoaderNlsData))) {

            //
            // prevent these pages from being reclaimed later
            //

            MemoryDescriptor->MemoryType = LoaderSpecialMemory;

        }

    }
}

VOID
MiCheckMemoryDescriptorList(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
    )
{
    PFN_NUMBER KernelStart;
    PFN_NUMBER KernelEnd;
    ULONG_PTR PageSize;
    PLIST_ENTRY NextEntry;
    PLIST_ENTRY PreviousEntry;
    PMEMORY_ALLOCATION_DESCRIPTOR MemoryDescriptor;
    PMEMORY_ALLOCATION_DESCRIPTOR PreviousMemoryDescriptor;
    

    KernelStart = MiNtoskrnlPhysicalBase >> PAGE_SHIFT;
    PageSize = (ULONG_PTR)1 << MiNtoskrnlPageShift;
    KernelEnd = KernelStart + (PageSize >> PAGE_SHIFT);

    PreviousMemoryDescriptor = NULL;
    PreviousEntry = NULL;

    NextEntry = LoaderBlock->MemoryDescriptorListHead.Flink;

    for ( ; NextEntry != &LoaderBlock->MemoryDescriptorListHead; NextEntry = NextEntry->Flink) {

        MemoryDescriptor = CONTAINING_RECORD(NextEntry,
                                             MEMORY_ALLOCATION_DESCRIPTOR,
                                             ListEntry);

#if DBG
        if (MiPrintMemoryDescriptors) {
            DbgPrint("MemoryType = %x\n", MemoryDescriptor->MemoryType);
            DbgPrint("BasePage = %p\n", (PFN_NUMBER)MemoryDescriptor->BasePage << PAGE_SHIFT);
            DbgPrint("PageCount = %x\n\n", MemoryDescriptor->PageCount << PAGE_SHIFT);
        }
#endif
        if ((MemoryDescriptor->BasePage >= KernelStart) && 
            (MemoryDescriptor->BasePage + MemoryDescriptor->PageCount <= KernelEnd)) {

            if (MemoryDescriptor->MemoryType == LoaderSystemBlock) {

                MemoryDescriptor->MemoryType = LoaderFirmwareTemporary;

            } else if (MemoryDescriptor->MemoryType == LoaderSpecialMemory) {
            
                MemoryDescriptor->MemoryType = LoaderFirmwareTemporary;

            }
        }


        if ((PreviousMemoryDescriptor != NULL) &&
            (MemoryDescriptor->MemoryType == PreviousMemoryDescriptor->MemoryType) &&
            (MemoryDescriptor->BasePage == 
             (PreviousMemoryDescriptor->BasePage + PreviousMemoryDescriptor->PageCount))) {

            PreviousMemoryDescriptor->PageCount += MemoryDescriptor->PageCount;
            PreviousEntry->Flink = NextEntry->Flink;

        } else { 

            PreviousMemoryDescriptor = MemoryDescriptor;    
            PreviousEntry = NextEntry;

        }

    }

}
