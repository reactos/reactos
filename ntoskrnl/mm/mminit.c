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

VOID
INIT_FUNCTION
NTAPI
MiInitSystemMemoryAreas()
{
    PVOID BaseAddress;
    PMEMORY_AREA MArea;
    NTSTATUS Status;

    //
    // Create the memory area to define the loader mappings
    //
    BaseAddress = (PVOID)KSEG0_BASE;
    Status = MmCreateMemoryArea(MmGetKernelAddressSpace(),
                                MEMORY_AREA_OWNED_BY_ARM3 | MEMORY_AREA_STATIC,
                                &BaseAddress,
                                MmBootImageSize,
                                PAGE_EXECUTE_READWRITE,
                                &MArea,
                                TRUE,
                                0,
                                PAGE_SIZE);
    ASSERT(Status == STATUS_SUCCESS);

    //
    // Create the memory area to define the PTE base
    //
    BaseAddress = (PVOID)PTE_BASE;
    Status = MmCreateMemoryArea(MmGetKernelAddressSpace(),
                                MEMORY_AREA_OWNED_BY_ARM3 | MEMORY_AREA_STATIC,
                                &BaseAddress,
                                PTE_TOP - PTE_BASE + 1,
                                PAGE_READWRITE,
                                &MArea,
                                TRUE,
                                0,
                                PAGE_SIZE);
    ASSERT(Status == STATUS_SUCCESS);

    //
    // Create the memory area to define Hyperspace
    //
    BaseAddress = (PVOID)HYPER_SPACE;
    Status = MmCreateMemoryArea(MmGetKernelAddressSpace(),
                                MEMORY_AREA_OWNED_BY_ARM3 | MEMORY_AREA_STATIC,
                                &BaseAddress,
                                HYPER_SPACE_END - HYPER_SPACE + 1,
                                PAGE_READWRITE,
                                &MArea,
                                TRUE,
                                0,
                                PAGE_SIZE);
    ASSERT(Status == STATUS_SUCCESS);

    //
    // Protect the PFN database
    //
    BaseAddress = MmPfnDatabase;
    Status = MmCreateMemoryArea(MmGetKernelAddressSpace(),
                                MEMORY_AREA_OWNED_BY_ARM3 | MEMORY_AREA_STATIC,
                                &BaseAddress,
                                (MxPfnAllocation << PAGE_SHIFT),
                                PAGE_READWRITE,
                                &MArea,
                                TRUE,
                                0,
                                PAGE_SIZE);
    ASSERT(Status == STATUS_SUCCESS);

    //
    // ReactOS requires a memory area to keep the initial NP area off-bounds
    //
    BaseAddress = MmNonPagedPoolStart;
    Status = MmCreateMemoryArea(MmGetKernelAddressSpace(),
                                MEMORY_AREA_OWNED_BY_ARM3 | MEMORY_AREA_STATIC,
                                &BaseAddress,
                                MmSizeOfNonPagedPoolInBytes,
                                PAGE_READWRITE,
                                &MArea,
                                TRUE,
                                0,
                                PAGE_SIZE);
    ASSERT(Status == STATUS_SUCCESS);

    //
    // And we need one more for the system NP
    //
    BaseAddress = MmNonPagedSystemStart;
    Status = MmCreateMemoryArea(MmGetKernelAddressSpace(),
                                MEMORY_AREA_OWNED_BY_ARM3 | MEMORY_AREA_STATIC,
                                &BaseAddress,
                                MiNonPagedSystemSize,
                                PAGE_READWRITE,
                                &MArea,
                                TRUE,
                                0,
                                PAGE_SIZE);
    ASSERT(Status == STATUS_SUCCESS);

    //
    // We also need one for system view space
    //
    BaseAddress = MiSystemViewStart;
    Status = MmCreateMemoryArea(MmGetKernelAddressSpace(),
                                MEMORY_AREA_OWNED_BY_ARM3 | MEMORY_AREA_STATIC,
                                &BaseAddress,
                                MmSystemViewSize,
                                PAGE_READWRITE,
                                &MArea,
                                TRUE,
                                0,
                                PAGE_SIZE);
    ASSERT(Status == STATUS_SUCCESS);

    //
    // And another for session space
    //
    BaseAddress = MmSessionBase;
    Status = MmCreateMemoryArea(MmGetKernelAddressSpace(),
                                MEMORY_AREA_OWNED_BY_ARM3 | MEMORY_AREA_STATIC,
                                &BaseAddress,
                                (ULONG_PTR)MiSessionSpaceEnd -
                                (ULONG_PTR)MmSessionBase,
                                PAGE_READWRITE,
                                &MArea,
                                TRUE,
                                0,
                                PAGE_SIZE);
    ASSERT(Status == STATUS_SUCCESS);

    //
    // One more for ARM paged pool
    //
    BaseAddress = MmPagedPoolStart;
    Status = MmCreateMemoryArea(MmGetKernelAddressSpace(),
                                MEMORY_AREA_OWNED_BY_ARM3 | MEMORY_AREA_STATIC,
                                &BaseAddress,
                                MmSizeOfPagedPoolInBytes,
                                PAGE_READWRITE,
                                &MArea,
                                TRUE,
                                0,
                                PAGE_SIZE);
    ASSERT(Status == STATUS_SUCCESS);
#ifndef _M_AMD64
    //
    // Next, the KPCR
    //
    BaseAddress = (PVOID)PCR;
    Status = MmCreateMemoryArea(MmGetKernelAddressSpace(),
                                MEMORY_AREA_OWNED_BY_ARM3 | MEMORY_AREA_STATIC,
                                &BaseAddress,
                                PAGE_SIZE * KeNumberProcessors,
                                PAGE_READWRITE,
                                &MArea,
                                TRUE,
                                0,
                                PAGE_SIZE);
    ASSERT(Status == STATUS_SUCCESS);
#endif
    //
    // Now the KUSER_SHARED_DATA
    //
    BaseAddress = (PVOID)KI_USER_SHARED_DATA;
    Status = MmCreateMemoryArea(MmGetKernelAddressSpace(),
                                MEMORY_AREA_OWNED_BY_ARM3 | MEMORY_AREA_STATIC,
                                &BaseAddress,
                                PAGE_SIZE,
                                PAGE_READWRITE,
                                &MArea,
                                TRUE,
                                0,
                                PAGE_SIZE);
    ASSERT(Status == STATUS_SUCCESS);

    //
    // And the debugger mapping
    //
    BaseAddress = MI_DEBUG_MAPPING;
    Status = MmCreateMemoryArea(MmGetKernelAddressSpace(),
                                MEMORY_AREA_OWNED_BY_ARM3 | MEMORY_AREA_STATIC,
                                &BaseAddress,
                                PAGE_SIZE,
                                PAGE_READWRITE,
                                &MArea,
                                TRUE,
                                0,
                                PAGE_SIZE);
    ASSERT(Status == STATUS_SUCCESS);

#if defined(_X86_)
    //
    // Finally, reserve the 2 pages we currently make use of for HAL mappings
    //
    BaseAddress = (PVOID)0xFFC00000;
    Status = MmCreateMemoryArea(MmGetKernelAddressSpace(),
                                MEMORY_AREA_OWNED_BY_ARM3 | MEMORY_AREA_STATIC,
                                &BaseAddress,
                                PAGE_SIZE * 2,
                                PAGE_READWRITE,
                                &MArea,
                                TRUE,
                                0,
                                PAGE_SIZE);
    ASSERT(Status == STATUS_SUCCESS);
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
    PMMPTE SysPte, PteTable;

    UNREFERENCED_PARAMETER(Parameter);

    /* Reserve a PTE for the page table */
    SysPte = MiReserveSystemPtes(1, SystemPteSpace);
    ASSERT(SysPte != NULL);
    PteTable = MiPteToAddress(SysPte);

    for(;;)
    {
        KIRQL OldIrql;
        PFN_NUMBER PageFrameIndex;
        PMMPFN Pfn;
        MMPTE TempPte, PdePte;
        PMMPTE PointerPte;
        BOOLEAN PageFileEntryFromPage = FALSE;
        /*
         * To start working, we wait for two conditions:
         *  - there are pages to be paged out.
         *  - We are in a low memory situation.
         */
        Status = KeWaitForSingleObject(
            &MpwThreadEvent,
            WrPageOut,
            KernelMode,
            FALSE,
            NULL);

        DPRINT("THE KRAKEN WAS RELEASED AND WILL PAGE YOUR ASS OUT!\n");

        if (!NT_SUCCESS(Status))
        {
            DPRINT1("MpwThread: Wait failed\n");
            KeBugCheck(MEMORY_MANAGEMENT);
            return;
        }

        /* Lock the PFN database */
        OldIrql = KeAcquireQueuedSpinLock(LockQueuePfnLock);

        /* The true main loop */
        while ((MmTotalPagesForPagingFile != 0) && (MiFreeSwapPages != 0))
        {
            /* Get the first page from the list */
            PageFrameIndex = MmModifiedPageListByColor[0].Flink;

            /* Get The Pfn */
            Pfn = MI_PFN_ELEMENT(PageFrameIndex);

            /* Some things which must always hold */
            ASSERT(Pfn->OriginalPte.u.Soft.Transition == 0);
            ASSERT(Pfn->u3.ReferenceCount == 0);

            /* And some that are not yet supported */
            ASSERT(Pfn->OriginalPte.u.Soft.Prototype == 0);

            /* Maybe this is not the first time */
            if ((Pfn->OriginalPte.u.Soft.Prototype == 0) &&
                    (Pfn->OriginalPte.u.Soft.PageFileHigh != 0))
            {
                /* Use that */
                TempPte = Pfn->OriginalPte;
                PageFileEntryFromPage = TRUE;
            }
            else
            {
                MiReservePageFileEntry(&TempPte);
            }

            /* Get it out of the list and reference it */
            MiUnlinkPageFromList(Pfn);
            MiReferenceUnusedPageAndBumpLockCount(Pfn);

            ASSERT(Pfn->u3.e1.PageLocation == ModifiedPageList);

            /* Mark it as write in progress */
            Pfn->u3.e1.WriteInProgress = 1;
            Pfn->u1.Event = NULL;

            /* Release the PFN lock while we are writing */
            KeReleaseQueuedSpinLock(LockQueuePfnLock, OldIrql);

            /* Do the actual write */
            Status = MiWritePageFile(PageFrameIndex, &TempPte);

            /* Get the lock again */
            OldIrql = KeAcquireQueuedSpinLock(LockQueuePfnLock);

            /* Get a mapping to the page table */
            MI_MAKE_HARDWARE_PTE_KERNEL(&PdePte,
                                        SysPte,
                                        MM_READWRITE,
                                        Pfn->u4.PteFrame);
            MI_WRITE_VALID_PTE(SysPte, PdePte);

            /* Finally get a pointer to the PTE this page represents */
            PointerPte = &PteTable[MiAddressToPteOffset(MiPteToAddress(Pfn->PteAddress))];

            /* Get relevant values from the original PTE */
            TempPte.u.Soft.Protection = PointerPte->u.Soft.Protection;

            /* Maybe someone aborted the operation */
            if (Pfn->u3.e1.WriteInProgress == 0)
            {
                DPRINT1("Someone aborted the page-out operation!\n");
                /* Just set the event and let the waiter go along */
                ASSERT(Pfn->u1.Event != NULL);
                KeSetEvent(Pfn->u1.Event, IO_NO_INCREMENT, FALSE);
                Pfn->u1.Event = NULL;
                /* This is now useless */
                if (!PageFileEntryFromPage)
                    MiFreePageFileEntry(&TempPte);
            }
            /* Maybe someone tried to access the page while we were not looking */
            else if (Pfn->u1.Event != NULL)
            {
                DPRINT1("Page fault occured while we were paging out!\n");

                /* So the page fault handler marked the page as unmodified */
                ASSERT(Pfn->u3.e1.Modified == 0);

                /* Save the pagefile entry for this page */
                Pfn->OriginalPte = TempPte;
                KeSetEvent(Pfn->u1.Event, IO_NO_INCREMENT, FALSE);
                Pfn->u1.Event = NULL;
            }
            else if (!NT_SUCCESS(Status))
            {
                DPRINT1("Failed to write page to pagefile!\n");
                /* This is now useless */
                if (!PageFileEntryFromPage)
                    MiFreePageFileEntry(&TempPte);
                /* MiDereferencePfnAndDropLockCount will put it back on the tail of the list */
            }
            else
            {
                DPRINT1("Page %x successfully paged out. PTE pointer %p (-> %x)\n",
                    PageFrameIndex, Pfn->PteAddress, TempPte.u.Long);

                /* Of course it must already be invalid */
                ASSERT(PointerPte->u.Hard.Valid == 0);

                /* And be in transition */
                ASSERT(PointerPte->u.Soft.Transition == 1);

                /* So the PTE is now officially paged out */
                MI_WRITE_INVALID_PTE(PointerPte, TempPte);

                /* And the pagefile entry belongs to the PTE, not to the page! */
                Pfn->OriginalPte.u.Long = 0;

                /* And dereference the page table frame */
                MiDecrementShareCount(MI_PFN_ELEMENT(Pfn->u4.PteFrame), Pfn->u4.PteFrame);
                /* We can finally make it available for real */
                MI_SET_PFN_DELETED(Pfn);
            }

            /* We're done with this */
            Pfn->u3.e1.WriteInProgress = 0;

            /* This will put it back in the free list */
            MiDereferencePfnAndDropLockCount(Pfn);

            /* Unmap the Page Table */
            MI_ERASE_PTE(SysPte);
            KeInvalidateTlbEntry(PteTable);
        }

        /* We're done for this run */
        KeReleaseQueuedSpinLock(LockQueuePfnLock, OldIrql);
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

