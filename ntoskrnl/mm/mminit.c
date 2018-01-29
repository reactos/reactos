/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/mm/mminit.c
 * PURPOSE:         Memory Manager Initialization
 * PROGRAMMERS:
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

#define MODULE_INVOLVED_IN_ARM3
#include "ARM3/miarm.h"

/* GLOBALS *******************************************************************/

VOID NTAPI MiInitializeUserPfnBitmap(VOID);

BOOLEAN Mm64BitPhysicalAddress = FALSE;
ULONG MmReadClusterSize;
//
// 0 | 1 is on/off paging, 2 is undocumented
//
UCHAR MmDisablePagingExecutive = 1; // Forced to off
PMMPTE MmSharedUserDataPte;
PMMSUPPORT MmKernelAddressSpace;

extern KEVENT MmWaitPageEvent;
extern FAST_MUTEX MiGlobalPageOperation;
extern LIST_ENTRY MiSegmentList;
extern NTSTATUS MiRosTrimCache(ULONG Target, ULONG Priority, PULONG NrFreed);

/* PRIVATE FUNCTIONS *********************************************************/

//
// Helper function to create initial memory areas.
// The created area is always read/write.
//
INIT_FUNCTION
VOID
NTAPI
MiCreateArm3StaticMemoryArea(PVOID BaseAddress, SIZE_T Size, BOOLEAN Executable)
{
    const ULONG Protection = Executable ? PAGE_EXECUTE_READWRITE : PAGE_READWRITE;
    PVOID pBaseAddress = BaseAddress;
    PMEMORY_AREA MArea;
    NTSTATUS Status;

    Status = MmCreateMemoryArea(MmGetKernelAddressSpace(),
                                MEMORY_AREA_OWNED_BY_ARM3 | MEMORY_AREA_STATIC,
                                &pBaseAddress,
                                Size,
                                Protection,
                                &MArea,
                                0,
                                PAGE_SIZE);
    ASSERT(Status == STATUS_SUCCESS);
    // TODO: Perhaps it would be  prudent to bugcheck here, not only assert?
}

INIT_FUNCTION
VOID
NTAPI
MiInitSystemMemoryAreas(VOID)
{
    //
    // Create all the static memory areas.
    //

#ifdef _M_AMD64
    // Reserved range FFFF800000000000 - FFFFF68000000000
    MiCreateArm3StaticMemoryArea((PVOID)MI_REAL_SYSTEM_RANGE_START, PTE_BASE - MI_REAL_SYSTEM_RANGE_START, FALSE);
#endif /* _M_AMD64 */

    // The loader mappings. The only Executable area.
    MiCreateArm3StaticMemoryArea((PVOID)KSEG0_BASE, MmBootImageSize, TRUE);

    // The PTE base
    MiCreateArm3StaticMemoryArea((PVOID)PTE_BASE, PTE_TOP - PTE_BASE + 1, FALSE);

    // Hyperspace
    MiCreateArm3StaticMemoryArea((PVOID)HYPER_SPACE, HYPER_SPACE_END - HYPER_SPACE + 1, FALSE);

    // Protect the PFN database
    MiCreateArm3StaticMemoryArea(MmPfnDatabase, (MxPfnAllocation << PAGE_SHIFT), FALSE);

    // ReactOS requires a memory area to keep the initial NP area off-bounds
    MiCreateArm3StaticMemoryArea(MmNonPagedPoolStart, MmSizeOfNonPagedPoolInBytes, FALSE);

    // System PTE space
    MiCreateArm3StaticMemoryArea(MmNonPagedSystemStart, (MmNumberOfSystemPtes + 1) * PAGE_SIZE, FALSE);

    // Nonpaged pool expansion space
    MiCreateArm3StaticMemoryArea(MmNonPagedPoolExpansionStart, (ULONG_PTR)MmNonPagedPoolEnd - (ULONG_PTR)MmNonPagedPoolExpansionStart, FALSE);

    // System view space
    MiCreateArm3StaticMemoryArea(MiSystemViewStart, MmSystemViewSize, FALSE);

    // Session space
    MiCreateArm3StaticMemoryArea(MmSessionBase, (ULONG_PTR)MiSessionSpaceEnd - (ULONG_PTR)MmSessionBase, FALSE);

    // Paged pool
    MiCreateArm3StaticMemoryArea(MmPagedPoolStart, MmSizeOfPagedPoolInBytes, FALSE);

    // Debugger mapping
    MiCreateArm3StaticMemoryArea(MI_DEBUG_MAPPING, PAGE_SIZE, FALSE);

#if defined(_X86_)
    // Reserved HAL area (includes KUSER_SHARED_DATA and KPCR)
    MiCreateArm3StaticMemoryArea((PVOID)MM_HAL_VA_START, MM_HAL_VA_END - MM_HAL_VA_START + 1, FALSE);
#else /* _X86_ */
#ifndef _M_AMD64
    // KPCR, one page per CPU. Only for 32-bit kernel.
    MiCreateArm3StaticMemoryArea(PCR, PAGE_SIZE * KeNumberProcessors, FALSE);
#endif /* _M_AMD64 */

    // KUSER_SHARED_DATA
    MiCreateArm3StaticMemoryArea((PVOID)KI_USER_SHARED_DATA, PAGE_SIZE, FALSE);
#endif /* _X86_ */
}

INIT_FUNCTION
VOID
NTAPI
MiDbgDumpAddressSpace(VOID)
{
    //
    // Print the memory layout
    //
    DPRINT1("          0x%p - 0x%p\t%s\n",
            KSEG0_BASE,
            (ULONG_PTR)KSEG0_BASE + MmBootImageSize,
            "Boot Loaded Image");
    DPRINT1("          0x%p - 0x%p\t%s\n",
            MmPfnDatabase,
            (ULONG_PTR)MmPfnDatabase + (MxPfnAllocation << PAGE_SHIFT),
            "PFN Database");
    DPRINT1("          0x%p - 0x%p\t%s\n",
            MmNonPagedPoolStart,
            (ULONG_PTR)MmNonPagedPoolStart + MmSizeOfNonPagedPoolInBytes,
            "ARM3 Non Paged Pool");
    DPRINT1("          0x%p - 0x%p\t%s\n",
            MiSystemViewStart,
            (ULONG_PTR)MiSystemViewStart + MmSystemViewSize,
            "System View Space");
    DPRINT1("          0x%p - 0x%p\t%s\n",
            MmSessionBase,
            MiSessionSpaceEnd,
            "Session Space");
    DPRINT1("          0x%p - 0x%p\t%s\n",
            PTE_BASE, PTE_TOP,
            "Page Tables");
    DPRINT1("          0x%p - 0x%p\t%s\n",
            PDE_BASE, PDE_TOP,
            "Page Directories");
    DPRINT1("          0x%p - 0x%p\t%s\n",
            HYPER_SPACE, HYPER_SPACE_END,
            "Hyperspace");
    DPRINT1("          0x%p - 0x%p\t%s\n",
            MmSystemCacheStart, MmSystemCacheEnd,
            "System Cache");
    DPRINT1("          0x%p - 0x%p\t%s\n",
            MmPagedPoolStart,
            (ULONG_PTR)MmPagedPoolStart + MmSizeOfPagedPoolInBytes,
            "ARM3 Paged Pool");
    DPRINT1("          0x%p - 0x%p\t%s\n",
            MmNonPagedSystemStart, MmNonPagedPoolExpansionStart,
            "System PTE Space");
    DPRINT1("          0x%p - 0x%p\t%s\n",
            MmNonPagedPoolExpansionStart, MmNonPagedPoolEnd,
            "Non Paged Pool Expansion PTE Space");
}

INIT_FUNCTION
NTSTATUS
NTAPI
MmInitBsmThread(VOID)
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE ThreadHandle;

    /* Create the thread */
    InitializeObjectAttributes(&ObjectAttributes, NULL, 0, NULL, NULL);
    Status = PsCreateSystemThread(&ThreadHandle,
                                  THREAD_ALL_ACCESS,
                                  &ObjectAttributes,
                                  NULL,
                                  NULL,
                                  KeBalanceSetManager,
                                  NULL);

    /* Close the handle and return status */
    ZwClose(ThreadHandle);
    return Status;
}

INIT_FUNCTION
BOOLEAN
NTAPI
MmInitSystem(IN ULONG Phase,
             IN PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    extern MMPTE ValidKernelPte;
    PMMPTE PointerPte;
    MMPTE TempPte = ValidKernelPte;
    PFN_NUMBER PageFrameNumber;
    PLIST_ENTRY ListEntry;
    PLDR_DATA_TABLE_ENTRY DataTableEntry;

    /* Initialize the kernel address space */
    ASSERT(Phase == 1);

    InitializeListHead(&MiSegmentList);
    ExInitializeFastMutex(&MiGlobalPageOperation);
    KeInitializeEvent(&MmWaitPageEvent, SynchronizationEvent, FALSE);
    // Until we're fully demand paged, we can do things the old way through
    // the balance manager
    MmInitializeMemoryConsumer(MC_CACHE, MiRosTrimCache);

    MmKernelAddressSpace = &PsIdleProcess->Vm;

    /* Intialize system memory areas */
    MiInitSystemMemoryAreas();

    /* Dump the address space */
    MiDbgDumpAddressSpace();

    MmInitGlobalKernelPageDirectory();
    MiInitializeUserPfnBitmap();
    MmInitializeMemoryConsumer(MC_USER, MmTrimUserMemory);
    MmInitializeRmapList();
    MmInitSectionImplementation();
    MmInitPagingFile();

    //
    // Create a PTE to double-map the shared data section. We allocate it
    // from paged pool so that we can't fault when trying to touch the PTE
    // itself (to map it), since paged pool addresses will already be mapped
    // by the fault handler.
    //
    MmSharedUserDataPte = ExAllocatePoolWithTag(PagedPool,
                          sizeof(MMPTE),
                          TAG_MM);
    if (!MmSharedUserDataPte) return FALSE;

    //
    // Now get the PTE for shared data, and read the PFN that holds it
    //
    PointerPte = MiAddressToPte((PVOID)KI_USER_SHARED_DATA);
    ASSERT(PointerPte->u.Hard.Valid == 1);
    PageFrameNumber = PFN_FROM_PTE(PointerPte);

    /* Build the PTE and write it */
    MI_MAKE_HARDWARE_PTE_KERNEL(&TempPte,
                                PointerPte,
                                MM_READONLY,
                                PageFrameNumber);
    *MmSharedUserDataPte = TempPte;

    /* Initialize session working set support */
    MiInitializeSessionWsSupport();

    /* Setup session IDs */
    MiInitializeSessionIds();

    /* Setup the memory threshold events */
    if (!MiInitializeMemoryEvents()) return FALSE;

    /*
     * Unmap low memory
     */
    MiInitBalancerThread();

    /* Initialize the balance set manager */
    MmInitBsmThread();

    /* Loop the boot loaded images */
    for (ListEntry = PsLoadedModuleList.Flink;
         ListEntry != &PsLoadedModuleList;
         ListEntry = ListEntry->Flink)
    {
        /* Get the data table entry */
        DataTableEntry = CONTAINING_RECORD(ListEntry, LDR_DATA_TABLE_ENTRY, InLoadOrderLinks);

        /* Set up the image protection */
        MiWriteProtectSystemImage(DataTableEntry->DllBase);
    }

    return TRUE;
}

