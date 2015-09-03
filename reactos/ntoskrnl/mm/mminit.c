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

HANDLE MpwThreadHandle;
KEVENT MpwThreadEvent;

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
VOID
INIT_FUNCTION
NTAPI
MiCreateArm3StaticMemoryArea(PVOID BaseAddress, ULONG Size, BOOLEAN Executable)
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

VOID
INIT_FUNCTION
NTAPI
MiInitSystemMemoryAreas(VOID)
{
    //
    // Create all the static memory areas.
    //

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

    // System NP
    MiCreateArm3StaticMemoryArea(MmNonPagedSystemStart, MiNonPagedSystemSize, FALSE);

    // System view space
    MiCreateArm3StaticMemoryArea(MiSystemViewStart, MmSystemViewSize, FALSE);

    // Session space
    MiCreateArm3StaticMemoryArea(MmSessionBase, (ULONG_PTR)MiSessionSpaceEnd - (ULONG_PTR)MmSessionBase, FALSE);

    // Paged pool
    MiCreateArm3StaticMemoryArea(MmPagedPoolStart, MmSizeOfPagedPoolInBytes, FALSE);

#ifndef _M_AMD64
    // KPCR, one page per CPU. Only for 32-bit kernel.
    MiCreateArm3StaticMemoryArea(PCR, PAGE_SIZE * KeNumberProcessors, FALSE);
#endif

    // KUSER_SHARED_DATA
    MiCreateArm3StaticMemoryArea((PVOID)KI_USER_SHARED_DATA, PAGE_SIZE, FALSE);

    // Debugger mapping
    MiCreateArm3StaticMemoryArea(MI_DEBUG_MAPPING, PAGE_SIZE, FALSE);

#if defined(_X86_)
    // Reserve the 2 pages we currently make use of for HAL mappings.
    // TODO: Remove hard-coded constant and replace with a define.
    MiCreateArm3StaticMemoryArea((PVOID)0xFFC00000, PAGE_SIZE * 2, FALSE);
#endif
}

VOID
NTAPI
INIT_FUNCTION
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

VOID
NTAPI
MmMpwThreadMain(PVOID Parameter)
{
    NTSTATUS Status;
#ifndef NEWCC
    ULONG PagesWritten;
#endif
    LARGE_INTEGER Timeout;

    UNREFERENCED_PARAMETER(Parameter);

    Timeout.QuadPart = -50000000;

    for(;;)
    {
        Status = KeWaitForSingleObject(&MpwThreadEvent,
                                       0,
                                       KernelMode,
                                       FALSE,
                                       &Timeout);
        if (!NT_SUCCESS(Status))
        {
            DbgPrint("MpwThread: Wait failed\n");
            KeBugCheck(MEMORY_MANAGEMENT);
            return;
        }

#ifndef NEWCC
        PagesWritten = 0;

        // XXX arty -- we flush when evicting pages or destorying cache
        // sections.
        CcRosFlushDirtyPages(128, &PagesWritten, FALSE);
#endif
    }
}

NTSTATUS
NTAPI
INIT_FUNCTION
MmInitMpwThread(VOID)
{
    KPRIORITY Priority;
    NTSTATUS Status;
    CLIENT_ID MpwThreadId;

    KeInitializeEvent(&MpwThreadEvent, SynchronizationEvent, FALSE);

    Status = PsCreateSystemThread(&MpwThreadHandle,
                                  THREAD_ALL_ACCESS,
                                  NULL,
                                  NULL,
                                  &MpwThreadId,
                                  MmMpwThreadMain,
                                  NULL);
    if (!NT_SUCCESS(Status))
    {
        return(Status);
    }

    Priority = 27;
    NtSetInformationThread(MpwThreadHandle,
                           ThreadPriority,
                           &Priority,
                           sizeof(Priority));

    return(STATUS_SUCCESS);
}

NTSTATUS
NTAPI
INIT_FUNCTION
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

BOOLEAN
NTAPI
INIT_FUNCTION
MmInitSystem(IN ULONG Phase,
             IN PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    extern MMPTE ValidKernelPte;
    PMMPTE PointerPte;
    MMPTE TempPte = ValidKernelPte;
    PFN_NUMBER PageFrameNumber;

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
                          '  mM');
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

    /*
     * Initialise the modified page writer.
     */
    MmInitMpwThread();

    /* Initialize the balance set manager */
    MmInitBsmThread();

    return TRUE;
}

