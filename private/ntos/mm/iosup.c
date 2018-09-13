/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

   iosup.c

Abstract:

    This module contains routines which provide support for the I/O system.

Author:

    Lou Perazzoli (loup) 25-Apr-1989
    Landy Wang (landyw) 02-June-1997

Revision History:

--*/

#include "mi.h"

#undef MmIsRecursiveIoFault

PFN_NUMBER MmSystemLockPagesCount;

extern ULONG MmTotalSystemDriverPages;

BOOLEAN
MmIsRecursiveIoFault(
    VOID
    );

PVOID
MiAllocateContiguousMemory (
    IN SIZE_T NumberOfBytes,
    IN PFN_NUMBER LowestAcceptablePfn,
    IN PFN_NUMBER HighestAcceptablePfn,
    IN PFN_NUMBER BoundaryPfn,
    PVOID CallingAddress
    );

PVOID
MiMapLockedPagesInUserSpace (
     IN PMDL MemoryDescriptorList,
     IN PVOID StartingVa,
     IN MEMORY_CACHING_TYPE CacheType,
     IN PVOID BaseVa
     );

VOID
MiUnmapLockedPagesInUserSpace (
     IN PVOID BaseAddress,
     IN PMDL MemoryDescriptorList
     );

LOGICAL
MiGetSystemPteAvailability (
    IN ULONG NumberOfPtes,
    IN MM_PAGE_PRIORITY Priority
    );

VOID
MiAddMdlTracker (
    IN PMDL MemoryDescriptorList,
    IN PVOID CallingAddress,
    IN PVOID CallersCaller,
    IN PFN_NUMBER NumberOfPagesToLock,
    IN ULONG Who
    );

typedef struct _PTE_TRACKER {
    LIST_ENTRY ListEntry;
    PMDL Mdl;
    PFN_NUMBER Count;
    PVOID SystemVa;
    PVOID StartVa;
    ULONG Offset;
    ULONG Length;
    ULONG_PTR Page;
    PVOID CallingAddress;
    PVOID CallersCaller;
    PVOID PteAddress;
} PTE_TRACKER, *PPTE_TRACKER;

typedef struct _SYSPTES_HEADER {
    LIST_ENTRY ListHead;
    PFN_NUMBER Count;
} SYSPTES_HEADER, *PSYSPTES_HEADER;

LOGICAL MmTrackPtes = FALSE;
BOOLEAN MiTrackPtesAborted = FALSE;
SYSPTES_HEADER MiPteHeader;
LIST_ENTRY MiDeadPteTrackerListHead;
KSPIN_LOCK MiPteTrackerLock;

LOCK_HEADER MmLockedPagesHead;
BOOLEAN MiTrackingAborted = FALSE;

VOID
MiInsertPteTracker (
     IN PVOID PoolBlock,
     IN PMDL MemoryDescriptorList,
     IN PFN_NUMBER NumberOfPtes,
     IN PVOID MyCaller,
     IN PVOID MyCallersCaller
     );

PVOID
MiRemovePteTracker (
     IN PMDL MemoryDescriptorList,
     IN PVOID PteAddress,
     IN PFN_NUMBER NumberOfPtes
     );

VOID
MiReleaseDeadPteTrackers (
    VOID
    );

VOID
MiInsertDeadPteTrackingBlock (
    IN PVOID PoolBlock
    );



VOID
MiProtectFreeNonPagedPool (
    IN PVOID VirtualAddress,
    IN ULONG SizeInPages
    );

LOGICAL
MiUnProtectFreeNonPagedPool (
    IN PVOID VirtualAddress,
    IN ULONG SizeInPages
    );

extern LOGICAL MiNoLowMemory;

PVOID
MiAllocateLowMemory (
    IN SIZE_T NumberOfBytes,
    IN PFN_NUMBER LowestAcceptablePfn,
    IN PFN_NUMBER HighestAcceptablePfn,
    IN PFN_NUMBER BoundaryPfn,
    IN PVOID CallingAddress,
    IN ULONG Tag
    );

LOGICAL
MiFreeLowMemory (
    IN PVOID BaseAddress,
    IN ULONG Tag
    );

#if DBG
ULONG MiPrintLockedPages;

VOID
MiVerifyLockedPageCharges (
    VOID
    );
#endif

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, MmAllocateIndependentPages)
#pragma alloc_text(INIT, MmSetPageProtection)
#pragma alloc_text(INIT, MiInitializeIoTrackers)

#pragma alloc_text(PAGE, MmLockPagableDataSection)
#pragma alloc_text(PAGE, MiLookupDataTableEntry)
#pragma alloc_text(PAGE, MiMapLockedPagesInUserSpace)
#pragma alloc_text(PAGE, MmSetBankedSection)
#pragma alloc_text(PAGE, MmProbeAndLockProcessPages)
#pragma alloc_text(PAGE, MmProbeAndLockSelectedPages)
#pragma alloc_text(PAGE, MmMapVideoDisplay)
#pragma alloc_text(PAGE, MmUnmapVideoDisplay)
#pragma alloc_text(PAGE, MmGetSectionRange)
#pragma alloc_text(PAGE, MiMapSinglePage)
#pragma alloc_text(PAGE, MiUnmapSinglePage)

#pragma alloc_text(PAGELK, MiUnmapLockedPagesInUserSpace)
#pragma alloc_text(PAGELK, MmAllocateNonCachedMemory)
#pragma alloc_text(PAGELK, MmFreeNonCachedMemory)
#pragma alloc_text(PAGELK, MmAllocatePagesForMdl)
#pragma alloc_text(PAGELK, MmFreePagesFromMdl)
#pragma alloc_text(PAGELK, MmLockPagedPool)
#pragma alloc_text(PAGELK, MmUnlockPagedPool)
#pragma alloc_text(PAGELK, MmGatherMemoryForHibernate)
#pragma alloc_text(PAGELK, MmReturnMemoryForHibernate)
#pragma alloc_text(PAGELK, MmReleaseDumpAddresses)
#pragma alloc_text(PAGELK, MmEnablePAT)

#pragma alloc_text(PAGEVRFY, MmIsSystemAddressLocked)

#pragma alloc_text(PAGEHYDRA, MmDispatchWin32Callout)
#endif

extern POOL_DESCRIPTOR NonPagedPoolDescriptor;

PFN_NUMBER MmMdlPagesAllocated;

KEVENT MmCollidedLockEvent;
ULONG MmCollidedLockWait;

SIZE_T MmLockedCode;

BOOLEAN MiWriteCombiningPtes = FALSE;

#ifdef LARGE_PAGES
ULONG MmLargeVideoMapped;
#endif

#if DBG
ULONG MiPrintAwe;
#endif

#define MI_PROBE_RAISE_SIZE 10

ULONG MiProbeRaises[MI_PROBE_RAISE_SIZE];

#define MI_INSTRUMENT_PROBE_RAISES(i)       \
        ASSERT (i < MI_PROBE_RAISE_SIZE);   \
        MiProbeRaises[i] += 1;

//
//  Note: this should be > 2041 to account for the cache manager's
//  aggressive zeroing logic.
//

ULONG MmReferenceCountCheck = 2500;


VOID
MmProbeAndLockPages (
     IN OUT PMDL MemoryDescriptorList,
     IN KPROCESSOR_MODE AccessMode,
     IN LOCK_OPERATION Operation
     )

/*++

Routine Description:

    This routine probes the specified pages, makes the pages resident and
    locks the physical pages mapped by the virtual pages in memory.  The
    Memory descriptor list is updated to describe the physical pages.

Arguments:

    MemoryDescriptorList - Supplies a pointer to a Memory Descriptor List
                            (MDL). The supplied MDL must supply a virtual
                            address, byte offset and length field.  The
                            physical page portion of the MDL is updated when
                            the pages are locked in memory.

    AccessMode - Supplies the access mode in which to probe the arguments.
                 One of KernelMode or UserMode.

    Operation - Supplies the operation type.  One of IoReadAccess, IoWriteAccess
                or IoModifyAccess.

Return Value:

    None - exceptions are raised.

Environment:

    Kernel mode.  APC_LEVEL and below for pagable addresses,
                  DISPATCH_LEVEL and below for non-pagable addresses.

--*/

{
    PPFN_NUMBER Page;
    MMPTE PteContents;
    PMMPTE PointerPte;
    PMMPTE PointerPde;
    PMMPTE PointerPpe;
    PVOID Va;
    PVOID EndVa;
    PVOID AlignedVa;
    PMMPFN Pfn1;
    PFN_NUMBER PageFrameIndex;
    PEPROCESS CurrentProcess;
    KIRQL OldIrql;
    PFN_NUMBER NumberOfPagesToLock;
    PFN_NUMBER NumberOfPagesSpanned;
    NTSTATUS status;
    NTSTATUS ProbeStatus;
    PETHREAD Thread;
    ULONG SavedState;
    LOGICAL AddressIsPhysical;
    PLIST_ENTRY NextEntry;
    PMI_PHYSICAL_VIEW PhysicalView;
    PCHAR StartVa;
    PVOID CallingAddress;
    PVOID CallersCaller;

#if !defined (_X86_)
    CallingAddress = (PVOID)_ReturnAddress();
    CallersCaller = (PVOID)0;
#endif

#if DBG
    if (MiPrintLockedPages != 0) {
        MiVerifyLockedPageCharges ();
    }
#endif

    ASSERT (MemoryDescriptorList->ByteCount != 0);
    ASSERT (((ULONG)MemoryDescriptorList->ByteOffset & ~(PAGE_SIZE - 1)) == 0);

    Page = (PPFN_NUMBER)(MemoryDescriptorList + 1);

    ASSERT (((ULONG_PTR)MemoryDescriptorList->StartVa & (PAGE_SIZE - 1)) == 0);
    AlignedVa = (PVOID)MemoryDescriptorList->StartVa;

    ASSERT ((MemoryDescriptorList->MdlFlags & (
                    MDL_PAGES_LOCKED |
                    MDL_MAPPED_TO_SYSTEM_VA |
                    MDL_SOURCE_IS_NONPAGED_POOL |
                    MDL_PARTIAL |
                    MDL_IO_SPACE)) == 0);

    Va = (PCHAR)AlignedVa + MemoryDescriptorList->ByteOffset;
    StartVa = Va;

    PointerPte = MiGetPteAddress (Va);

    //
    // Endva is one byte past the end of the buffer, if ACCESS_MODE is not
    // kernel, make sure the EndVa is in user space AND the byte count
    // does not cause it to wrap.
    //

    EndVa = (PVOID)((PCHAR)Va + MemoryDescriptorList->ByteCount);

    if ((AccessMode != KernelMode) &&
        ((EndVa > (PVOID)MM_USER_PROBE_ADDRESS) || (Va >= EndVa))) {
        *Page = MM_EMPTY_LIST;
        MI_INSTRUMENT_PROBE_RAISES(0);
        ExRaiseStatus (STATUS_ACCESS_VIOLATION);
        return;
    }

    //
    // There is an optimization which could be performed here.  If
    // the operation is for WriteAccess and the complete page is
    // being modified, we can remove the current page, if it is not
    // resident, and substitute a demand zero page.
    // Note, that after analysis by marking the thread and then
    // noting if a page read was done, this rarely occurs.
    //

    MemoryDescriptorList->Process = (PEPROCESS)NULL;

    Thread = PsGetCurrentThread ();

    if (!MI_IS_PHYSICAL_ADDRESS(Va)) {

        AddressIsPhysical = FALSE;
        ProbeStatus = STATUS_SUCCESS;

        NumberOfPagesToLock = COMPUTE_PAGES_SPANNED (Va,
                                       MemoryDescriptorList->ByteCount);

        ASSERT (NumberOfPagesToLock != 0);

        NumberOfPagesSpanned = NumberOfPagesToLock;

        PointerPpe = MiGetPpeAddress (Va);
        PointerPde = MiGetPdeAddress (Va);

        MmSavePageFaultReadAhead (Thread, &SavedState);
        MmSetPageFaultReadAhead (Thread, (ULONG)(NumberOfPagesToLock - 1));

        try {

            do {

                *Page = MM_EMPTY_LIST;

                //
                // Make sure the page is resident.
                //

                *(volatile CHAR *)Va;

                if ((Operation != IoReadAccess) &&
                    (Va <= MM_HIGHEST_USER_ADDRESS)) {

                    //
                    // Probe for write access as well.
                    //

                    ProbeForWriteChar ((PCHAR)Va);
                }

                NumberOfPagesToLock -= 1;

                MmSetPageFaultReadAhead (Thread, (ULONG)(NumberOfPagesToLock - 1));
                Va = (PVOID)(((ULONG_PTR)(PCHAR)Va + PAGE_SIZE) & ~(PAGE_SIZE - 1));
                Page += 1;
            } while (Va < EndVa);

            ASSERT (NumberOfPagesToLock == 0);

        } except (EXCEPTION_EXECUTE_HANDLER) {
            ProbeStatus = GetExceptionCode();
        }

        //
        // We may still fault again below but it's generally rare.
        // Restore this thread's normal fault behavior now.
        //

        MmResetPageFaultReadAhead (Thread, SavedState);

        if (ProbeStatus != STATUS_SUCCESS) {
            MI_INSTRUMENT_PROBE_RAISES(1);
            ExRaiseStatus (ProbeStatus);
            return;
        }
    }
    else {
        AddressIsPhysical = TRUE;
        *Page = MM_EMPTY_LIST;
    }

    Va = AlignedVa;
    Page = (PPFN_NUMBER)(MemoryDescriptorList + 1);

    //
    // Indicate that this is a write operation.
    //

    if (Operation != IoReadAccess) {
        MemoryDescriptorList->MdlFlags |= MDL_WRITE_OPERATION;
    } else {
        MemoryDescriptorList->MdlFlags &= ~(MDL_WRITE_OPERATION);
    }

    //
    // Acquire the PFN database lock.
    //

    LOCK_PFN2 (OldIrql);

    if (Va <= MM_HIGHEST_USER_ADDRESS) {

        //
        // These are addresses with user space, check to see if the
        // working set size will allow these pages to be locked.
        //

        ASSERT (NumberOfPagesSpanned != 0);

        CurrentProcess = PsGetCurrentProcess ();

        //
        // Check for a transfer to/from a physical VAD - no reference counts
        // may be modified for these pages.
        //

        NextEntry = CurrentProcess->PhysicalVadList.Flink;
        while (NextEntry != &CurrentProcess->PhysicalVadList) {

            PhysicalView = CONTAINING_RECORD(NextEntry,
                                             MI_PHYSICAL_VIEW,
                                             ListEntry);

            if ((PhysicalView->Vad->u.VadFlags.UserPhysicalPages == 0) &&
                (PhysicalView->Vad->u.VadFlags.PhysicalMapping == 0)) {
                NextEntry = NextEntry->Flink;
                continue;
            }

            if (StartVa < PhysicalView->StartVa) {

                if ((PCHAR)EndVa - 1 >= PhysicalView->StartVa) {

                    //
                    // The range encompasses a physical VAD.  This is not
                    // allowed.
                    //

                    UNLOCK_PFN2 (OldIrql);
                    MI_INSTRUMENT_PROBE_RAISES(2);
                    ExRaiseStatus (STATUS_ACCESS_VIOLATION);
                    return;
                }

                NextEntry = NextEntry->Flink;
                continue;
            }

            if (StartVa <= PhysicalView->EndVa) {

                //
                // Ensure that the entire range lies within the VAD.
                //

                if ((PCHAR)EndVa - 1 > PhysicalView->EndVa) {

                    //
                    // The range goes past the end of the VAD - not allowed.
                    //

                    UNLOCK_PFN2 (OldIrql);
                    MI_INSTRUMENT_PROBE_RAISES(3);
                    ExRaiseStatus (STATUS_ACCESS_VIOLATION);
                    return;
                }

                if (PhysicalView->Vad->u.VadFlags.UserPhysicalPages == 1) {

                    //
                    // All the PTEs must still be checked and reference
                    // counts bumped on the pages.  Just don't charge
                    // against the working set.
                    //

                    NextEntry = NextEntry->Flink;
                    continue;
                }

                //
                // The range lies within a physical VAD.
                //

                if (Operation != IoReadAccess) {

                    //
                    // Ensure the VAD is writable.  Changing individual PTE
                    // protections in a physical VAD is not allowed.
                    //

                    if ((PhysicalView->Vad->u.VadFlags.Protection & MM_READWRITE) == 0) {
                        UNLOCK_PFN2 (OldIrql);
                        MI_INSTRUMENT_PROBE_RAISES(4);
                        ExRaiseStatus (STATUS_ACCESS_VIOLATION);
                        return;
                    }
                }

                //
                // Don't charge page locking for this transfer as it is all
                // physical, just initialize the MDL.  Note the pages do not
                // have to be physically contiguous, so the frames must be
                // extracted from the PTEs.
                //

                MemoryDescriptorList->MdlFlags |= (MDL_PHYSICAL_VIEW | MDL_PAGES_LOCKED);
                MemoryDescriptorList->Process = CurrentProcess;

                do {
                    PageFrameIndex = MI_GET_PAGE_FRAME_FROM_PTE (PointerPte);
                    *Page = PageFrameIndex;
                    Page += 1;
                    PointerPte += 1;
                    Va = (PVOID)((PCHAR)Va + PAGE_SIZE);
                } while (Va < EndVa);

                UNLOCK_PFN2 (OldIrql);
                return;
            }
            NextEntry = NextEntry->Flink;
        }

        CurrentProcess->NumberOfLockedPages += NumberOfPagesSpanned;

        MemoryDescriptorList->Process = CurrentProcess;
    }

    MemoryDescriptorList->MdlFlags |= MDL_PAGES_LOCKED;

    do {

        if (AddressIsPhysical == TRUE) {

            //
            // On certain architectures, virtual addresses
            // may be physical and hence have no corresponding PTE.
            //

            PageFrameIndex = MI_CONVERT_PHYSICAL_TO_PFN (Va);

        } else {

#if defined (_WIN64)
            while ((PointerPpe->u.Hard.Valid == 0) ||
                   (PointerPde->u.Hard.Valid == 0) ||
                   (PointerPte->u.Hard.Valid == 0))
#else
            while ((PointerPde->u.Hard.Valid == 0) ||
                   (PointerPte->u.Hard.Valid == 0))
#endif
            {

                //
                // PDE is not resident, release PFN lock touch the page and make
                // it appear.
                //

                UNLOCK_PFN2 (OldIrql);

                MmSetPageFaultReadAhead (Thread, 0);

                status = MmAccessFault (FALSE, Va, KernelMode, (PVOID)0);

                MmResetPageFaultReadAhead (Thread, SavedState);

                if (!NT_SUCCESS(status)) {

                    //
                    // An exception occurred.  Unlock the pages locked
                    // so far.
                    //

failure:
                    if (MmTrackLockedPages == TRUE) {

                        //
                        // Adjust the MDL length so that MmUnlockPages only
                        // processes the part that was completed.
                        //

                        ULONG PagesLocked;
            
                        PagesLocked = ADDRESS_AND_SIZE_TO_SPAN_PAGES(StartVa,
                                              MemoryDescriptorList->ByteCount);

#if defined (_X86_)
                        RtlGetCallersAddress(&CallingAddress, &CallersCaller);
#endif
                        MiAddMdlTracker (MemoryDescriptorList,
                                         CallingAddress,
                                         CallersCaller,
                                         PagesLocked,
                                         0);
                    }

                    MmUnlockPages (MemoryDescriptorList);

                    //
                    // Raise an exception of access violation to the caller.
                    //

                    MI_INSTRUMENT_PROBE_RAISES(7);
                    ExRaiseStatus (status);
                    return;
                }

                LOCK_PFN2 (OldIrql);
            }

            PteContents = *PointerPte;
            ASSERT (PteContents.u.Hard.Valid == 1);

            if (Va <= MM_HIGHEST_USER_ADDRESS) {
                if (Operation != IoReadAccess) {

                    if ((PteContents.u.Long & MM_PTE_WRITE_MASK) == 0) {

                        //
                        // The caller has made the page protection more
                        // restrictive, this should never be done once the
                        // request has been issued !  Rather than wading
                        // through the PFN database entry to see if it
                        // could possibly work out, give the caller an
                        // access violation.
                        //

#if DBG
                        DbgPrint ("MmProbeAndLockPages: PTE %p %p changed\n",
                            PointerPte,
                            PteContents.u.Long);
                        ASSERT (FALSE);
#endif

                        UNLOCK_PFN2 (OldIrql);
                        status = STATUS_ACCESS_VIOLATION;
                        goto failure;
                    }
                }
            }

            PageFrameIndex = MI_GET_PAGE_FRAME_FROM_PTE (&PteContents);
        }

        if (PageFrameIndex > MmHighestPhysicalPage) {

            //
            // This is an I/O space address don't allow operations
            // on addresses not in the PFN database.
            //

            MemoryDescriptorList->MdlFlags |= MDL_IO_SPACE;

        } else {
            ASSERT ((MemoryDescriptorList->MdlFlags & MDL_IO_SPACE) == 0);

            Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);

#if PFN_CONSISTENCY
            ASSERT(Pfn1->u3.e1.PageTablePage == 0);
#endif

            //
            // Check to make sure this page is not locked down an unusually
            // high number of times.
            //

            if (Pfn1->u3.e2.ReferenceCount >= MmReferenceCountCheck) {
                UNLOCK_PFN2 (OldIrql);
                ASSERT (FALSE);
                status = STATUS_WORKING_SET_QUOTA;
                goto failure;
            }

            //
            // Check to make sure the systemwide locked pages count is fluid.
            //

            if (MI_NONPAGABLE_MEMORY_AVAILABLE() <= 0) {

                //
                // If this page is for paged pool or privileged code/data,
                // then force it in.
                //

                if ((Va > MM_HIGHEST_USER_ADDRESS) &&
                    (!MI_IS_SYSTEM_CACHE_ADDRESS(Va))) {
                    MI_INSTRUMENT_PROBE_RAISES(8);
                    goto ok;
                }

                MI_INSTRUMENT_PROBE_RAISES(5);
                UNLOCK_PFN2 (OldIrql);
                status = STATUS_WORKING_SET_QUOTA;
                goto failure;
            }

            //
            // Check to make sure any administrator-desired limit is obeyed.
            //

            if (MmSystemLockPagesCount + 1 >= MmLockPagesLimit) {

                //
                // If this page is for paged pool or privileged code/data,
                // then force it in.
                //

                if ((Va > MM_HIGHEST_USER_ADDRESS) &&
                    (!MI_IS_SYSTEM_CACHE_ADDRESS(Va))) {
                    MI_INSTRUMENT_PROBE_RAISES(9);
                    goto ok;
                }

                MI_INSTRUMENT_PROBE_RAISES(6);
                UNLOCK_PFN2 (OldIrql);
                status = STATUS_WORKING_SET_QUOTA;
                goto failure;
            }

ok:
            MI_ADD_LOCKED_PAGE_CHARGE(Pfn1, 0);

            Pfn1->u3.e2.ReferenceCount += 1;
        }

        *Page = PageFrameIndex;

        Page += 1;
        PointerPte += 1;
        if (MiIsPteOnPdeBoundary(PointerPte)) {
            PointerPde += 1;
            if (MiIsPteOnPpeBoundary(PointerPte)) {
                PointerPpe += 1;
            }
        }

        Va = (PVOID)((PCHAR)Va + PAGE_SIZE);
    } while (Va < EndVa);

    UNLOCK_PFN2 (OldIrql);

    if ((MmTrackLockedPages == TRUE) && (AlignedVa <= MM_HIGHEST_USER_ADDRESS)) {

        ASSERT (NumberOfPagesSpanned != 0);

#if defined (_X86_)
        RtlGetCallersAddress(&CallingAddress, &CallersCaller);
#endif

        MiAddMdlTracker (MemoryDescriptorList,
                         CallingAddress,
                         CallersCaller,
                         NumberOfPagesSpanned,
                         1);
    }

    return;
}

NTKERNELAPI
VOID
MmProbeAndLockProcessPages (
    IN OUT PMDL MemoryDescriptorList,
    IN PEPROCESS Process,
    IN KPROCESSOR_MODE AccessMode,
    IN LOCK_OPERATION Operation
    )

/*++

Routine Description:

    This routine probes and locks the address range specified by
    the MemoryDescriptorList in the specified Process for the AccessMode
    and Operation.

Arguments:

    MemoryDescriptorList - Supplies a pre-initialized MDL that describes the
                           address range to be probed and locked.

    Process - Specifies the address of the process whose address range is
              to be locked.

    AccessMode - The mode for which the probe should check access to the range.

    Operation - Supplies the type of access which for which to check the range.

Return Value:

    None.

--*/

{
    LOGICAL Attached;
    NTSTATUS Status;

    Attached = FALSE;
    Status = STATUS_SUCCESS;

    if (Process != PsGetCurrentProcess ()) {
        KeAttachProcess (&Process->Pcb);
        Attached = TRUE;
    }

    try {

        MmProbeAndLockPages (MemoryDescriptorList,
                             AccessMode,
                             Operation);

    } except (EXCEPTION_EXECUTE_HANDLER) {
        Status = GetExceptionCode();
    }

    if (Attached) {
        KeDetachProcess();
    }

    if (Status != STATUS_SUCCESS) {
        ExRaiseStatus (Status);
    }
    return;
}

VOID
MiAddMdlTracker (
    IN PMDL MemoryDescriptorList,
    IN PVOID CallingAddress,
    IN PVOID CallersCaller,
    IN PFN_NUMBER NumberOfPagesToLock,
    IN ULONG Who
    )

/*++

Routine Description:

    This routine adds an MDL to the specified process' chain.

Arguments:

    MemoryDescriptorList - Supplies a pointer to a Memory Descriptor List
                           (MDL). The MDL must supply the length. The
                           physical page portion of the MDL is updated when
                           the pages are locked in memory.

    CallingAddress - Supplies the address of the caller of our caller.

    CallersCaller - Supplies the address of the caller of CallingAddress.

    NumberOfPagesToLock - Specifies the number of pages to lock.

    Who - Specifies which routine is adding the entry.

Return Value:

    None - exceptions are raised.

Environment:

    Kernel mode.  APC_LEVEL and below.

--*/

{
    KIRQL OldIrql;
    PEPROCESS Process;
    PLOCK_HEADER LockedPagesHeader;
    PLOCK_TRACKER Tracker;
    PLOCK_TRACKER P;
    PLIST_ENTRY NextEntry;

    ASSERT (MmTrackLockedPages == TRUE);

    Process = MemoryDescriptorList->Process;

    if (Process == NULL) {
        return;
    }

    LockedPagesHeader = Process->LockedPagesList;

    if (LockedPagesHeader == NULL) {
        return;
    }

    //
    // It's ok to check unsynchronized for aborted tracking as the worst case
    // is just that one more entry gets added which will be freed later anyway.
    // The main purpose behind aborted tracking is that frees and exits don't
    // mistakenly bugcheck when an entry cannot be found.
    //

    if (MiTrackingAborted == TRUE) {
        return;
    }

    Tracker = ExAllocatePoolWithTag (NonPagedPool,
                                     sizeof (LOCK_TRACKER),
                                     'kLmM');

    if (Tracker == NULL) {

        //
        // It's ok to set this without synchronization as the worst case
        // is just that a few more entries gets added which will be freed
        // later anyway.  The main purpose behind aborted tracking is that
        // frees and exits don't mistakenly bugcheck when an entry cannot
        // be found.
        //
    
        MiTrackingAborted = TRUE;

        return;
    }

    Tracker->Mdl = MemoryDescriptorList;
    Tracker->Count = NumberOfPagesToLock;
    Tracker->StartVa = MemoryDescriptorList->StartVa;
    Tracker->Offset = MemoryDescriptorList->ByteOffset;
    Tracker->Length = MemoryDescriptorList->ByteCount;
    Tracker->Page = *(PPFN_NUMBER)(MemoryDescriptorList + 1);

    Tracker->CallingAddress = CallingAddress;
    Tracker->CallersCaller = CallersCaller;

    Tracker->Who = Who;
    Tracker->Process = Process;

    LOCK_PFN2 (OldIrql);

    //
    // Update the list for this process.  First make sure it's not already
    // inserted.
    //

    NextEntry = LockedPagesHeader->ListHead.Flink;
    while (NextEntry != &LockedPagesHeader->ListHead) {

        P = CONTAINING_RECORD (NextEntry,
                               LOCK_TRACKER,
                               ListEntry);

        if (P->Mdl == MemoryDescriptorList) {
            KeBugCheckEx (LOCKED_PAGES_TRACKER_CORRUPTION,
                          0x1,
                          (ULONG_PTR)P,
                          (ULONG_PTR)MemoryDescriptorList,
                          (ULONG_PTR)MmLockedPagesHead.Count);
        }
        NextEntry = NextEntry->Flink;
    }

    InsertHeadList (&LockedPagesHeader->ListHead, &Tracker->ListEntry);
    LockedPagesHeader->Count += NumberOfPagesToLock;

    //
    // Update the systemwide global list.  First make sure it's not
    // already inserted.
    //

    NextEntry = MmLockedPagesHead.ListHead.Flink;
    while (NextEntry != &MmLockedPagesHead.ListHead) {

        P = CONTAINING_RECORD(NextEntry,
                              LOCK_TRACKER,
                              GlobalListEntry);

        if (P->Mdl == MemoryDescriptorList) {
            KeBugCheckEx (LOCKED_PAGES_TRACKER_CORRUPTION,
                          0x2,
                          (ULONG_PTR)P,
                          (ULONG_PTR)MemoryDescriptorList,
                          (ULONG_PTR)MmLockedPagesHead.Count);
        }

        NextEntry = NextEntry->Flink;
    }

    InsertHeadList (&MmLockedPagesHead.ListHead,
                    &Tracker->GlobalListEntry);
    MmLockedPagesHead.Count += NumberOfPagesToLock;

    UNLOCK_PFN2 (OldIrql);
}

LOGICAL
MiFreeMdlTracker (
    IN OUT PMDL MemoryDescriptorList,
    IN PFN_NUMBER NumberOfPages
    )

/*++

Routine Description:

    This deletes an MDL from the specified process' chain.  Used specifically
    by MmProbeAndLockSelectedPages () because it builds an MDL in its local
    stack and then copies the requested pages into the real MDL.  this lets
    us track these pages.

Arguments:

    MemoryDescriptorList - Supplies a pointer to a Memory Descriptor List
                           (MDL). The MDL must supply the length.

    NumberOfPages - Supplies the number of pages to be freed.

Return Value:

    TRUE.

Environment:

    Kernel mode.  APC_LEVEL and below.

--*/
{
    KIRQL OldIrql;
    PLOCK_TRACKER Tracker;
    PLIST_ENTRY NextEntry;
    PLOCK_HEADER LockedPagesHeader;
    PPFN_NUMBER Page;
    PLOCK_TRACKER Found;
    PVOID PoolToFree;

    ASSERT (MemoryDescriptorList->Process != NULL);

    LockedPagesHeader = (PLOCK_HEADER)MemoryDescriptorList->Process->LockedPagesList;

    if (LockedPagesHeader == NULL) {
        return TRUE;
    }

    Found = NULL;
    Page = (PPFN_NUMBER) (MemoryDescriptorList + 1);

    LOCK_PFN2 (OldIrql);

    NextEntry = LockedPagesHeader->ListHead.Flink;
    while (NextEntry != &LockedPagesHeader->ListHead) {

        Tracker = CONTAINING_RECORD (NextEntry,
                                     LOCK_TRACKER,
                                     ListEntry);

        if (MemoryDescriptorList == Tracker->Mdl) {

            if (Found != NULL) {
                KeBugCheckEx (LOCKED_PAGES_TRACKER_CORRUPTION,
                              0x3,
                              (ULONG_PTR)Found,
                              (ULONG_PTR)Tracker,
                              (ULONG_PTR)MemoryDescriptorList);
            }

            ASSERT (Tracker->Page == *Page);
            ASSERT (NumberOfPages == Tracker->Count);
            Tracker->Count = (PFN_NUMBER)-1;
            RemoveEntryList (NextEntry);
            LockedPagesHeader->Count -= NumberOfPages;

            RemoveEntryList (&Tracker->GlobalListEntry);
            MmLockedPagesHead.Count -= NumberOfPages;

            Found = Tracker;
            PoolToFree = (PVOID)NextEntry;
        }
        NextEntry = Tracker->ListEntry.Flink;
    }

    UNLOCK_PFN2 (OldIrql);

    if (Found == NULL) {

        //
        // A driver is trying to unlock pages that aren't locked.
        //

        if (MiTrackingAborted == TRUE) {
            return TRUE;
        }

        KeBugCheckEx (PROCESS_HAS_LOCKED_PAGES,
                      1,
                      (ULONG_PTR)MemoryDescriptorList,
                      MemoryDescriptorList->Process->NumberOfLockedPages,
                      (ULONG_PTR)MemoryDescriptorList->Process->LockedPagesList);
    }

    ExFreePool (PoolToFree);

    return TRUE;
}


NTKERNELAPI
VOID
MmProbeAndLockSelectedPages (
    IN OUT PMDL MemoryDescriptorList,
    IN PFILE_SEGMENT_ELEMENT SegmentArray,
    IN KPROCESSOR_MODE AccessMode,
    IN LOCK_OPERATION Operation
    )

/*++

Routine Description:

    This routine probes the specified pages, makes the pages resident and
    locks the physical pages mapped by the virtual pages in memory.  The
    Memory descriptor list is updated to describe the physical pages.

Arguments:

    MemoryDescriptorList - Supplies a pointer to a Memory Descriptor List
                           (MDL). The MDL must supply the length. The
                           physical page portion of the MDL is updated when
                           the pages are locked in memory.

    SegmentArray - Supplies a pointer to a list of buffer segments to be
                   probed and locked.

    AccessMode - Supplies the access mode in which to probe the arguments.
                 One of KernelMode or UserMode.

    Operation - Supplies the operation type.  One of IoReadAccess, IoWriteAccess
                or IoModifyAccess.

Return Value:

    None - exceptions are raised.

Environment:

    Kernel mode.  APC_LEVEL and below.

--*/

{
    PMDL TempMdl;
    PFN_NUMBER MdlHack[(sizeof(MDL)/sizeof(PFN_NUMBER)) + 1];
    PPFN_NUMBER Page;
    PFILE_SEGMENT_ELEMENT LastSegment;
    PVOID CallingAddress;
    PVOID CallersCaller;
    ULONG NumberOfPagesToLock;

    PAGED_CODE();

#if !defined (_X86_)
    CallingAddress = (PVOID)_ReturnAddress();
    CallersCaller = (PVOID)0;
#endif

    NumberOfPagesToLock = 0;

    ASSERT (MemoryDescriptorList->ByteCount != 0);
    ASSERT (((ULONG_PTR)MemoryDescriptorList->ByteOffset & ~(PAGE_SIZE - 1)) == 0);

    ASSERT ((MemoryDescriptorList->MdlFlags & (
                    MDL_PAGES_LOCKED |
                    MDL_MAPPED_TO_SYSTEM_VA |
                    MDL_SOURCE_IS_NONPAGED_POOL |
                    MDL_PARTIAL |
                    MDL_IO_SPACE)) == 0);

    //
    // Initialize TempMdl.
    //

    TempMdl = (PMDL) &MdlHack;

    MmInitializeMdl( TempMdl, SegmentArray->Buffer, PAGE_SIZE );

    Page = (PPFN_NUMBER) (MemoryDescriptorList + 1);

    //
    // Calculate the end of the segment list.
    //

    LastSegment = SegmentArray +
                  BYTES_TO_PAGES(MemoryDescriptorList->ByteCount);

    ASSERT(SegmentArray < LastSegment);

    //
    // Build a small Mdl for each segment and call probe and lock pages.
    // Then copy the PFNs to the real mdl.  The first page is processed
    // outside of the try/finally to ensure that the flags and process
    // field are correctly set in case MmUnlockPages needs to be called.
    //

    //
    // Even systems without 64 bit pointers are required to zero the
    // upper 32 bits of the segment address so use alignment rather
    // than the buffer pointer.
    //

    SegmentArray += 1;
    MmProbeAndLockPages( TempMdl, AccessMode, Operation );

    if (MmTrackLockedPages == TRUE) {

        //
        // Since we move the page from the temp MDL to the real one below
        // and never free the temp one, fixup our accounting now.
        //

        if (MiFreeMdlTracker (TempMdl, 1) == TRUE) {
            NumberOfPagesToLock += 1;
        }
    }

    *Page++ = *((PPFN_NUMBER) (TempMdl + 1));

    //
    // Copy the flags and process fields.
    //

    MemoryDescriptorList->MdlFlags |= TempMdl->MdlFlags;
    MemoryDescriptorList->Process = TempMdl->Process;

    try {

        while (SegmentArray < LastSegment) {

            //
            // Even systems without 64 bit pointers are required to zero the
            // upper 32 bits of the segment address so use alignment rather
            // than the buffer pointer.
            //

            TempMdl->StartVa = (PVOID)(ULONG_PTR)SegmentArray->Buffer;
            TempMdl->MdlFlags = 0;

            SegmentArray += 1;
            MmProbeAndLockPages( TempMdl, AccessMode, Operation );


            if (MmTrackLockedPages == TRUE) {

                //
                // Since we move the page from the temp MDL to the real one
                // below and never free the temp one, fixup our accounting now.
                //

                if (MiFreeMdlTracker (TempMdl, 1) == TRUE) {
                    NumberOfPagesToLock += 1;
                }
            }

            *Page++ = *((PPFN_NUMBER) (TempMdl + 1));
        }
    } finally {

        if (abnormal_termination()) {

            //
            // Adjust the MDL length so that MmUnlockPages only processes
            // the part that was completed.
            //

            MemoryDescriptorList->ByteCount =
                (ULONG) (Page - (PPFN_NUMBER) (MemoryDescriptorList + 1)) << PAGE_SHIFT;

            if (MmTrackLockedPages == TRUE) {
#if defined (_X86_)
                RtlGetCallersAddress(&CallingAddress, &CallersCaller);
#endif
                MiAddMdlTracker (MemoryDescriptorList,
                                 CallingAddress,
                                 CallersCaller,
                                 NumberOfPagesToLock,
                                 2);
            }

            MmUnlockPages( MemoryDescriptorList );
        }
        else if (MmTrackLockedPages == TRUE) {
#if defined (_X86_)
            RtlGetCallersAddress(&CallingAddress, &CallersCaller);
#endif
            MiAddMdlTracker (MemoryDescriptorList,
                             CallingAddress,
                             CallersCaller,
                             NumberOfPagesToLock,
                             3);
        }
    }
}

VOID
MmUnlockPages (
     IN OUT PMDL MemoryDescriptorList
     )

/*++

Routine Description:

    This routine unlocks physical pages which are described by a Memory
    Descriptor List.

Arguments:

    MemoryDescriptorList - Supplies a pointer to a memory descriptor list
                            (MDL). The supplied MDL must have been supplied
                            to MmLockPages to lock the pages down.  As the
                            pages are unlocked, the MDL is updated.

Return Value:

    None.

Environment:

    Kernel mode, IRQL of DISPATCH_LEVEL or below.

--*/

{
    PFN_NUMBER NumberOfPages;
    PPFN_NUMBER Page;
    PVOID StartingVa;
    KIRQL OldIrql;
    PMMPFN Pfn1;
    LOGICAL Unlock;

    ASSERT ((MemoryDescriptorList->MdlFlags & MDL_PAGES_LOCKED) != 0);
    ASSERT ((MemoryDescriptorList->MdlFlags & MDL_SOURCE_IS_NONPAGED_POOL) == 0);
    ASSERT ((MemoryDescriptorList->MdlFlags & MDL_PARTIAL) == 0);
    ASSERT (MemoryDescriptorList->ByteCount != 0);

    if (MemoryDescriptorList->MdlFlags & MDL_MAPPED_TO_SYSTEM_VA) {

        //
        // This MDL has been mapped into system space, unmap now.
        //

        MmUnmapLockedPages (MemoryDescriptorList->MappedSystemVa,
                            MemoryDescriptorList);
    }

    Page = (PPFN_NUMBER)(MemoryDescriptorList + 1);
    Unlock = TRUE;
    StartingVa = (PVOID)((PCHAR)MemoryDescriptorList->StartVa +
                    MemoryDescriptorList->ByteOffset);

    NumberOfPages = ADDRESS_AND_SIZE_TO_SPAN_PAGES(StartingVa,
                                              MemoryDescriptorList->ByteCount);

    if (MmTrackLockedPages == TRUE) {
        if ((MemoryDescriptorList->Process != NULL) &&
            (Unlock == TRUE) &&
            ((MemoryDescriptorList->MdlFlags & MDL_PHYSICAL_VIEW) == 0)) {
                MiFreeMdlTracker (MemoryDescriptorList, NumberOfPages);
        }
    }

    ASSERT (NumberOfPages != 0);

    LOCK_PFN2 (OldIrql);

    if (MmLockedPagesHead.ListHead.Flink != 0) {

        PLOCK_TRACKER P;
        PLIST_ENTRY NextEntry;

        NextEntry = MmLockedPagesHead.ListHead.Flink;
        while (NextEntry != &MmLockedPagesHead.ListHead) {

            P = CONTAINING_RECORD(NextEntry,
                                  LOCK_TRACKER,
                                  GlobalListEntry);

            if (P->Mdl == MemoryDescriptorList) {
                KeBugCheckEx (LOCKED_PAGES_TRACKER_CORRUPTION,
                              0x4,
                              (ULONG_PTR)P,
                              (ULONG_PTR)MemoryDescriptorList,
                              0);
            }

            NextEntry = NextEntry->Flink;
        }
    }

    if ((MemoryDescriptorList->Process != NULL) &&
        (Unlock == TRUE) &&
        ((MemoryDescriptorList->MdlFlags & MDL_PHYSICAL_VIEW) == 0)) {

        MemoryDescriptorList->Process->NumberOfLockedPages -= NumberOfPages;
        ASSERT ((SPFN_NUMBER)MemoryDescriptorList->Process->NumberOfLockedPages >= 0);
    }

    if ((MemoryDescriptorList->MdlFlags & (MDL_IO_SPACE | MDL_PHYSICAL_VIEW)) == 0) {

        //
        // Only unlock if not I/O or physical space.
        //

        do {

            if (*Page == MM_EMPTY_LIST) {

                //
                // There are no more locked pages.
                //

                break;
            }
            ASSERT (*Page <= MmHighestPhysicalPage);

            //
            // If this was a write operation set the modified bit in the
            // PFN database.
            //

            Pfn1 = MI_PFN_ELEMENT (*Page);
            if (MemoryDescriptorList->MdlFlags & MDL_WRITE_OPERATION) {
                Pfn1->u3.e1.Modified = 1;
                if ((Pfn1->OriginalPte.u.Soft.Prototype == 0) &&
                             (Pfn1->u3.e1.WriteInProgress == 0)) {
                    MiReleasePageFileSpace (Pfn1->OriginalPte);
                    Pfn1->OriginalPte.u.Soft.PageFileHigh = 0;
                }
            }

            MI_REMOVE_LOCKED_PAGE_CHARGE(Pfn1, 1);

            MiDecrementReferenceCount (*Page);

            *Page = MM_EMPTY_LIST;
            Page += 1;
            NumberOfPages -= 1;
        } while (NumberOfPages != 0);
    }

    MemoryDescriptorList->MdlFlags &= ~MDL_PAGES_LOCKED;
    UNLOCK_PFN2 (OldIrql);

    return;
}

VOID
MmBuildMdlForNonPagedPool (
    IN OUT PMDL MemoryDescriptorList
    )

/*++

Routine Description:

    This routine fills in the "pages" portion of the MDL using the PFN
    numbers corresponding the buffers which resides in non-paged pool.

    Unlike MmProbeAndLockPages, there is no corresponding unlock as no
    reference counts are incremented as the buffers being in nonpaged
    pool are always resident.

Arguments:

    MemoryDescriptorList - Supplies a pointer to a Memory Descriptor List
                            (MDL). The supplied MDL must supply a virtual
                            address, byte offset and length field.  The
                            physical page portion of the MDL is updated when
                            the pages are locked in memory.  The virtual
                            address must be within the non-paged portion
                            of the system space.

Return Value:

    None.

Environment:

    Kernel mode, IRQL of DISPATCH_LEVEL or below.

--*/

{
    PPFN_NUMBER Page;
    PMMPTE PointerPte;
    PMMPTE LastPte;
    PVOID EndVa;
    PFN_NUMBER PageFrameIndex;

    Page = (PPFN_NUMBER)(MemoryDescriptorList + 1);

    ASSERT (MemoryDescriptorList->ByteCount != 0);
    ASSERT ((MemoryDescriptorList->MdlFlags & (
                    MDL_PAGES_LOCKED |
                    MDL_MAPPED_TO_SYSTEM_VA |
                    MDL_SOURCE_IS_NONPAGED_POOL |
                    MDL_PARTIAL)) == 0);

    MemoryDescriptorList->Process = (PEPROCESS)NULL;

    //
    // Endva is last byte of the buffer.
    //

    MemoryDescriptorList->MdlFlags |= MDL_SOURCE_IS_NONPAGED_POOL;

    MemoryDescriptorList->MappedSystemVa =
            (PVOID)((PCHAR)MemoryDescriptorList->StartVa +
                                           MemoryDescriptorList->ByteOffset);

    EndVa = (PVOID)(((PCHAR)MemoryDescriptorList->MappedSystemVa +
                            MemoryDescriptorList->ByteCount - 1));

    LastPte = MiGetPteAddress (EndVa);

    ASSERT (MmIsNonPagedSystemAddressValid (MemoryDescriptorList->StartVa));

    PointerPte = MiGetPteAddress (MemoryDescriptorList->StartVa);

    if (MI_IS_PHYSICAL_ADDRESS(EndVa)) {
        PageFrameIndex = MI_CONVERT_PHYSICAL_TO_PFN (
                                MemoryDescriptorList->StartVa);

        do {
            *Page = PageFrameIndex;
            Page += 1;
            PageFrameIndex += 1;
            PointerPte += 1;
        } while (PointerPte <= LastPte);
    } else {
        do {
            PageFrameIndex = MI_GET_PAGE_FRAME_FROM_PTE (PointerPte);
            *Page = PageFrameIndex;
            Page += 1;
            PointerPte += 1;
        } while (PointerPte <= LastPte);
    }

    return;
}

VOID
MiInitializeIoTrackers (
    VOID
    )
{
    if (MmTrackPtes != 0) {
        InitializeListHead (&MiDeadPteTrackerListHead);
        KeInitializeSpinLock (&MiPteTrackerLock);
        InitializeListHead (&MiPteHeader.ListHead);
    }

    if (MmTrackLockedPages == TRUE) {
        InitializeListHead (&MmLockedPagesHead.ListHead);
    }
}

VOID
MiInsertPteTracker (
     IN PVOID PoolBlock,
     IN PMDL MemoryDescriptorList,
     IN PFN_NUMBER NumberOfPtes,
     IN PVOID MyCaller,
     IN PVOID MyCallersCaller
     )
/*++

Routine Description:

    This function inserts a PTE tracking block as the caller has just
    consumed system PTEs.

Arguments:

    PoolBlock - Supplies a tracker pool block.  This is supplied by the caller
                since the MmSystemSpaceLock is held on entry hence pool
                allocations may not be done here.

    MemoryDescriptorList - Supplies a valid Memory Descriptor List.

    NumberOfPtes - Supplies the number of system PTEs allocated.

    MyCaller - Supplies the return address of the caller who consumed the
               system PTEs to map this MDL.

    MyCallersCaller - Supplies the return address of the caller of the caller
                      who consumed the system PTEs to map this MDL.

Return Value:

    None.

Environment:

    Kernel mode, protected by MmSystemSpaceLock at DISPATCH_LEVEL.

--*/

{
    PPTE_TRACKER Tracker;

    Tracker = (PPTE_TRACKER)PoolBlock;

    Tracker->Mdl = MemoryDescriptorList;
    Tracker->SystemVa = MemoryDescriptorList->MappedSystemVa;
    Tracker->Count = NumberOfPtes;

    Tracker->StartVa = MemoryDescriptorList->StartVa;
    Tracker->Offset = MemoryDescriptorList->ByteOffset;
    Tracker->Length = MemoryDescriptorList->ByteCount;
    Tracker->Page = *(PPFN_NUMBER)(MemoryDescriptorList + 1);

    Tracker->CallingAddress = MyCaller;
    Tracker->CallersCaller = MyCallersCaller;
    Tracker->PteAddress = MiGetPteAddress (Tracker->SystemVa);

    MiPteHeader.Count += NumberOfPtes;

    InsertHeadList (&MiPteHeader.ListHead, &Tracker->ListEntry);
}

PVOID
MiRemovePteTracker (
     IN PMDL MemoryDescriptorList OPTIONAL,
     IN PVOID PteAddress,
     IN PFN_NUMBER NumberOfPtes
     )

/*++

Routine Description:

    This function removes a PTE tracking block from the lists as the PTEs
    are being freed.

Arguments:

    MemoryDescriptorList - Supplies a valid Memory Descriptor List.

    PteAddress - Supplies the address the system PTEs were mapped to.

    NumberOfPtes - Supplies the number of system PTEs allocated.

Return Value:

    The pool block that held the tracking info that must be freed by our
    caller _AFTER_ our caller releases MmSystemSpaceLock (to prevent deadlock).

Environment:

    Kernel mode, protected by MmSystemSpaceLock at DISPATCH_LEVEL.

--*/

{
    PPTE_TRACKER Tracker;
    PFN_NUMBER Page;
    PVOID BaseAddress;
    PLIST_ENTRY LastFound;
    PLIST_ENTRY NextEntry;

    BaseAddress = MiGetVirtualAddressMappedByPte (PteAddress);

    if (ARGUMENT_PRESENT (MemoryDescriptorList)) {
        Page = *(PPFN_NUMBER)(MemoryDescriptorList + 1);
    }

    LastFound = NULL;
    NextEntry = MiPteHeader.ListHead.Flink;
    while (NextEntry != &MiPteHeader.ListHead) {

        Tracker = (PPTE_TRACKER) CONTAINING_RECORD (NextEntry,
                                                    PTE_TRACKER,
                                                    ListEntry.Flink);

        if (PteAddress == Tracker->PteAddress) {

            if (LastFound != NULL) {

                //
                // Duplicate map entry.
                //

                KeBugCheckEx (SYSTEM_PTE_MISUSE,
                              0x1,
                              (ULONG_PTR)Tracker,
                              (ULONG_PTR)MemoryDescriptorList,
                              (ULONG_PTR)LastFound);
            }

            if (Tracker->Count != NumberOfPtes) {

                //
                // Not unmapping the same of number of PTEs that were mapped.
                //

                KeBugCheckEx (SYSTEM_PTE_MISUSE,
                              0x2,
                              (ULONG_PTR)Tracker,
                              Tracker->Count,
                              NumberOfPtes);
            }

            if (ARGUMENT_PRESENT (MemoryDescriptorList)) {

                if (Tracker->SystemVa != MemoryDescriptorList->MappedSystemVa) {

                    //
                    // Not unmapping the same address that was mapped.
                    //

                    KeBugCheckEx (SYSTEM_PTE_MISUSE,
                                  0x3,
                                  (ULONG_PTR)Tracker,
                                  (ULONG_PTR)Tracker->SystemVa,
                                  (ULONG_PTR)MemoryDescriptorList->MappedSystemVa);
                }

                if (Tracker->Page != Page) {

                    //
                    // The first page in the MDL has changed since it was mapped.
                    //

                    KeBugCheckEx (SYSTEM_PTE_MISUSE,
                                  0x4,
                                  (ULONG_PTR)Tracker,
                                  (ULONG_PTR)Tracker->Page,
                                  (ULONG_PTR)Page);
                }

                if (Tracker->StartVa != MemoryDescriptorList->StartVa) {

                    //
                    // Map and unmap don't match up.
                    //

                    KeBugCheckEx (SYSTEM_PTE_MISUSE,
                                  0x5,
                                  (ULONG_PTR)Tracker,
                                  (ULONG_PTR)Tracker->StartVa,
                                  (ULONG_PTR)MemoryDescriptorList->StartVa);
                }
            }

            RemoveEntryList (NextEntry);
            LastFound = NextEntry;
        }
        NextEntry = Tracker->ListEntry.Flink;
    }

    if ((LastFound == NULL) && (MiTrackPtesAborted == FALSE)) {

        //
        // Can't unmap something that was never (or isn't currently) mapped.
        //

        KeBugCheckEx (SYSTEM_PTE_MISUSE,
                      0x6,
                      (ULONG_PTR)MemoryDescriptorList,
                      (ULONG_PTR)BaseAddress,
                      (ULONG_PTR)NumberOfPtes);
    }

    MiPteHeader.Count -= NumberOfPtes;

    return (PVOID)LastFound;
}

VOID
MiInsertDeadPteTrackingBlock (
    IN PVOID PoolBlock
    )

/*++

Routine Description:

    This routine inserts a tracking block into the dead PTE list for later
    release.  Locks (including the PFN lock) may be held on entry, thus the
    block cannot be directly freed to pool at this time.

Arguments:

    PoolBlock - Supplies the base pool address to free.

Return Value:

    None.

Environment:

    Kernel mode.  DISPATCH_LEVEL or below, locks may be held.

--*/
{
    KIRQL OldIrql;

    ExAcquireSpinLock (&MiPteTrackerLock, &OldIrql);

    InsertTailList (&MiDeadPteTrackerListHead, (PLIST_ENTRY)PoolBlock);

    ExReleaseSpinLock (&MiPteTrackerLock, OldIrql);
}

VOID
MiReleaseDeadPteTrackers (
    VOID
    )
/*++

Routine Description:

    This routine removes tracking blocks from the dead PTE list and frees
    them to pool.

Arguments:

    None.

Return Value:

    None.

Environment:

    Kernel mode.  No locks held.

--*/
{
    KIRQL OldIrql;
    PVOID PoolBlock;

    ASSERT (KeGetCurrentIrql() <= DISPATCH_LEVEL);

    ExAcquireSpinLock (&MiPteTrackerLock, &OldIrql);

    while (IsListEmpty(&MiDeadPteTrackerListHead) == 0) {
        PoolBlock = (PVOID)RemoveHeadList(&MiDeadPteTrackerListHead);
        ExReleaseSpinLock (&MiPteTrackerLock, OldIrql);
        ExFreePool (PoolBlock);
        ExAcquireSpinLock (&MiPteTrackerLock, &OldIrql);
    }

    ExReleaseSpinLock (&MiPteTrackerLock, OldIrql);
}

PVOID
MiGetHighestPteConsumer (
    OUT PULONG_PTR NumberOfPtes
    )

/*++

Routine Description:

    This function examines the PTE tracking blocks and returns the biggest
    consumer.

Arguments:

    None.

Return Value:

    The loaded module entry of the biggest consumer.

Environment:

    Kernel mode, called during bugcheck only.  Many locks may be held.

--*/

{
    PPTE_TRACKER Tracker;
    PVOID BaseAddress;
    PFN_NUMBER NumberOfPages;
    PLIST_ENTRY NextEntry;
    PLIST_ENTRY NextEntry2;
    PLDR_DATA_TABLE_ENTRY DataTableEntry;
    ULONG_PTR Highest;
    ULONG_PTR PagesByThisModule;
    PLDR_DATA_TABLE_ENTRY HighDataTableEntry;

    *NumberOfPtes = 0;

    //
    // No locks are acquired as this is only called during a bugcheck.
    //

    if (MmTrackPtes == FALSE) {
        return NULL;
    }

    if (MiTrackPtesAborted == TRUE) {
        return NULL;
    }

    if (IsListEmpty(&MiPteHeader.ListHead)) {
        return NULL;
    }

    if (PsLoadedModuleList.Flink == NULL) {
        return NULL;
    }

    Highest = 0;
    HighDataTableEntry = NULL;

    NextEntry = PsLoadedModuleList.Flink;
    while (NextEntry != &PsLoadedModuleList) {

        DataTableEntry = CONTAINING_RECORD(NextEntry,
                                           LDR_DATA_TABLE_ENTRY,
                                           InLoadOrderLinks);

        PagesByThisModule = 0;

        //
        // Walk the PTE mapping list and update each driver's counts.
        //
    
        NextEntry2 = MiPteHeader.ListHead.Flink;
        while (NextEntry2 != &MiPteHeader.ListHead) {
    
            Tracker = (PPTE_TRACKER) CONTAINING_RECORD (NextEntry2,
                                                        PTE_TRACKER,
                                                        ListEntry.Flink);
    
            BaseAddress = Tracker->CallingAddress;
            NumberOfPages = Tracker->Count;
    
            if ((BaseAddress >= DataTableEntry->DllBase) &&
                (BaseAddress < (PVOID)((ULONG_PTR)(DataTableEntry->DllBase) + DataTableEntry->SizeOfImage))) {

                PagesByThisModule += NumberOfPages;
            }
        
            NextEntry2 = NextEntry2->Flink;
    
        }
    
        if (PagesByThisModule > Highest) {
            Highest = PagesByThisModule;
            HighDataTableEntry = DataTableEntry;
        }

        NextEntry = NextEntry->Flink;
    }

    *NumberOfPtes = Highest;

    return (PVOID)HighDataTableEntry;
}

PVOID
MmMapLockedPages (
     IN PMDL MemoryDescriptorList,
     IN KPROCESSOR_MODE AccessMode
     )

/*++

Routine Description:

    This function maps physical pages described by a memory descriptor
    list into the system virtual address space or the user portion of
    the virtual address space.

Arguments:

    MemoryDescriptorList - Supplies a valid Memory Descriptor List which has
                            been updated by MmProbeAndLockPages.


    AccessMode - Supplies an indicator of where to map the pages;
                 KernelMode indicates that the pages should be mapped in the
                 system part of the address space, UserMode indicates the
                 pages should be mapped in the user part of the address space.

Return Value:

    Returns the base address where the pages are mapped.  The base address
    has the same offset as the virtual address in the MDL.

    This routine will raise an exception if the processor mode is USER_MODE
    and quota limits or VM limits are exceeded.

Environment:

    Kernel mode.  DISPATCH_LEVEL or below if access mode is KernelMode,
                  APC_LEVEL or below if access mode is UserMode.

--*/

{
    return MmMapLockedPagesSpecifyCache (MemoryDescriptorList,
                                         AccessMode,
                                         MmCached,
                                         NULL,
                                         TRUE,
                                         HighPagePriority);
}

PVOID
MmMapLockedPagesSpecifyCache (
     IN PMDL MemoryDescriptorList,
     IN KPROCESSOR_MODE AccessMode,
     IN MEMORY_CACHING_TYPE CacheType,
     IN PVOID RequestedAddress,
     IN ULONG BugCheckOnFailure,
     IN MM_PAGE_PRIORITY Priority
     )

/*++

Routine Description:

    This function maps physical pages described by a memory descriptor
    list into the system virtual address space or the user portion of
    the virtual address space.

Arguments:

    MemoryDescriptorList - Supplies a valid Memory Descriptor List which has
                           been updated by MmProbeAndLockPages.

    AccessMode - Supplies an indicator of where to map the pages;
                 KernelMode indicates that the pages should be mapped in the
                 system part of the address space, UserMode indicates the
                 pages should be mapped in the user part of the address space.

    CacheType - Supplies the type of cache mapping to use for the MDL.
                MmCached indicates "normal" kernel or user mappings.

    RequestedAddress - Supplies the base user address of the view. This is only
                       used if the AccessMode is UserMode.  If the initial
                       value of this argument is not null, then the view will
                       be allocated starting at the specified virtual
                       address rounded down to the next 64kb address
                       boundary. If the initial value of this argument is
                       null, then the operating system will determine
                       where to allocate the view.

    BugCheckOnFailure - Supplies whether to bugcheck if the mapping cannot be
                        obtained.  This flag is only checked if the MDL's
                        MDL_MAPPING_CAN_FAIL is zero, which implies that the
                        default MDL behavior is to bugcheck.  This flag then
                        provides an additional avenue to avoid the bugcheck.
                        Done this way in order to provide WDM compatibility.

    Priority - Supplies an indication as to how important it is that this
               request succeed under low available PTE conditions.

Return Value:

    Returns the base address where the pages are mapped.  The base address
    has the same offset as the virtual address in the MDL.

    This routine will raise an exception if the processor mode is USER_MODE
    and quota limits or VM limits are exceeded.

Environment:

    Kernel mode.  DISPATCH_LEVEL or below if access mode is KernelMode,
                  APC_LEVEL or below if access mode is UserMode.

--*/

{
    PFN_NUMBER NumberOfPages;
    PFN_NUMBER SavedPageCount;
    PPFN_NUMBER Page;
    PMMPTE PointerPte;
    PVOID BaseVa;
    MMPTE TempPte;
    PVOID StartingVa;
    PMMPFN Pfn2;
    KIRQL OldIrql;
    PFN_NUMBER NumberOfPtes;
    PVOID CallingAddress;
    PVOID CallersCaller;
    PVOID Tracker;

#if !defined (_X86_)
    CallingAddress = (PVOID)_ReturnAddress();
    CallersCaller = (PVOID)0;
#endif

    StartingVa = (PVOID)((PCHAR)MemoryDescriptorList->StartVa +
                    MemoryDescriptorList->ByteOffset);

    ASSERT (MemoryDescriptorList->ByteCount != 0);

    if (AccessMode == KernelMode) {

        Page = (PPFN_NUMBER)(MemoryDescriptorList + 1);
        NumberOfPages = COMPUTE_PAGES_SPANNED (StartingVa,
                                               MemoryDescriptorList->ByteCount);
        SavedPageCount = NumberOfPages;

        //
        // Map the pages into the system part of the address space as
        // kernel read/write.
        //

        ASSERT ((MemoryDescriptorList->MdlFlags & (
                        MDL_MAPPED_TO_SYSTEM_VA |
                        MDL_SOURCE_IS_NONPAGED_POOL |
                        MDL_PARTIAL_HAS_BEEN_MAPPED)) == 0);
        ASSERT ((MemoryDescriptorList->MdlFlags & (
                        MDL_PAGES_LOCKED |
                        MDL_PARTIAL)) != 0);

        //
        // Map this with KSEG0 if possible.
        //

#if defined(_ALPHA_)
#define KSEG0_MAXPAGE ((PFN_NUMBER)((KSEG2_BASE - KSEG0_BASE) >> PAGE_SHIFT))
#endif

#if defined(_X86_) || defined(_IA64_)
#define KSEG0_MAXPAGE MmKseg2Frame
#endif

#if defined(_IA64_)
#define MM_KSEG0_BASE KSEG0_BASE
#endif

        if ((NumberOfPages == 1) && (CacheType == MmCached) &&
            (*Page < KSEG0_MAXPAGE)) {
            BaseVa = (PVOID)(MM_KSEG0_BASE + (*Page << PAGE_SHIFT) +
                            MemoryDescriptorList->ByteOffset);
            MemoryDescriptorList->MappedSystemVa = BaseVa;
            MemoryDescriptorList->MdlFlags |= MDL_MAPPED_TO_SYSTEM_VA;

            goto Update;
        }

        //
        // Make sure there are enough PTEs of the requested size.
        //

        if ((Priority != HighPagePriority) &&
            (MiGetSystemPteAvailability ((ULONG)NumberOfPages, Priority) == FALSE)) {
                return NULL;
        }

        PointerPte = MiReserveSystemPtes (
                                    (ULONG)NumberOfPages,
                                    SystemPteSpace,
                                    MM_COLOR_ALIGNMENT,
                                    (PtrToUlong(StartingVa) &
                                                       MM_COLOR_MASK_VIRTUAL),
                        MemoryDescriptorList->MdlFlags & MDL_MAPPING_CAN_FAIL ? 0 : BugCheckOnFailure);

        if (PointerPte == NULL) {

            //
            // Not enough system PTES are available.
            //

            return NULL;
        }
        BaseVa = (PVOID)((PCHAR)MiGetVirtualAddressMappedByPte (PointerPte) +
                                MemoryDescriptorList->ByteOffset);

        NumberOfPtes = NumberOfPages;

        TempPte = ValidKernelPte;

        switch (CacheType) {

            case MmNonCached:
                MI_DISABLE_CACHING (TempPte);
                break;

            case MmCached:
                break;

            case MmWriteCombined:
                MI_SET_PTE_WRITE_COMBINE (TempPte);
                break;

            case MmHardwareCoherentCached:
                break;

#if 0
            case MmNonCachedUnordered:
                break;
#endif

            default:
                break;
        }

#if defined(_IA64_)
        if (CacheType != MmCached) {
            KeFlushEntireTb(FALSE, TRUE);
        }
#endif

#if DBG
        LOCK_PFN2 (OldIrql);
#endif //DBG

        do {

            if (*Page == MM_EMPTY_LIST) {
                break;
            }
            TempPte.u.Hard.PageFrameNumber = *Page;
            ASSERT (PointerPte->u.Hard.Valid == 0);

#if DBG
            if ((MemoryDescriptorList->MdlFlags & (MDL_IO_SPACE | MDL_PHYSICAL_VIEW)) == 0) {
                Pfn2 = MI_PFN_ELEMENT (*Page);
                ASSERT (Pfn2->u3.e2.ReferenceCount != 0);
                ASSERT ((((ULONG_PTR)PointerPte >> PTE_SHIFT) & MM_COLOR_MASK) ==
                     (((ULONG)Pfn2->u3.e1.PageColor)));
            }
#endif //DBG

            MI_WRITE_VALID_PTE (PointerPte, TempPte);
            Page += 1;
            PointerPte += 1;
            NumberOfPages -= 1;
        } while (NumberOfPages != 0);

#if DBG
        UNLOCK_PFN2 (OldIrql);
#endif //DBG

#if defined(i386)
        //
        // If write combined was specified then flush all caches and TBs.
        //

        if (CacheType == MmWriteCombined && MiWriteCombiningPtes == TRUE) {
            KeFlushEntireTb (FALSE, TRUE);
            KeInvalidateAllCaches (TRUE);
        }
#endif

#if defined(_IA64_)
        if (CacheType != MmCached) {
            MiSweepCacheMachineDependent(BaseVa, SavedPageCount * PAGE_SIZE, CacheType);
        }
#endif
        if (MmTrackPtes != 0) {

            //
            // First free any zombie blocks as no locks are being held.
            //

            MiReleaseDeadPteTrackers ();

            Tracker = ExAllocatePoolWithTag (NonPagedPool,
                                             sizeof (PTE_TRACKER),
                                             'ySmM');
            if (Tracker == NULL) {
                MiTrackPtesAborted = TRUE;
            }
        }

        MiLockSystemSpace(OldIrql);
        if (MemoryDescriptorList->MdlFlags & MDL_MAPPED_TO_SYSTEM_VA) {

            //
            // Another thread must have already mapped this.
            // Clean up the system PTES and release them.
            //

            MiUnlockSystemSpace(OldIrql);

            if (MmTrackPtes != 0) {
                if (Tracker != NULL) {
                    ExFreePool(Tracker);
                }
            }

#if DBG
            if ((MemoryDescriptorList->MdlFlags & (MDL_IO_SPACE | MDL_PHYSICAL_VIEW)) == 0) {
                PMMPFN Pfn3;
                PFN_NUMBER j;
                PPFN_NUMBER Page1;
    
                Page1 = (PPFN_NUMBER)(MemoryDescriptorList + 1);
                for (j = 0; j < SavedPageCount ;j += 1) {
                    if (*Page == MM_EMPTY_LIST) {
                        break;
                    }
                    Pfn3 = MI_PFN_ELEMENT (*Page1);
                    ASSERT (Pfn3->u3.e2.ReferenceCount != 0);
                    Page1 += 1;
                }
            }
#endif //DBG
            PointerPte = MiGetPteAddress (BaseVa);

            MiReleaseSystemPtes (PointerPte,
                                 (ULONG)SavedPageCount,
                                 SystemPteSpace);

            return MemoryDescriptorList->MappedSystemVa;
        }

        MemoryDescriptorList->MappedSystemVa = BaseVa;
        *(volatile ULONG *)&MmSystemLockPagesCount;  //need to force order.
        MemoryDescriptorList->MdlFlags |= MDL_MAPPED_TO_SYSTEM_VA;

        if ((MmTrackPtes != 0) && (Tracker != NULL)) {
#if defined (_X86_)
            RtlGetCallersAddress(&CallingAddress, &CallersCaller);
#endif
            MiInsertPteTracker (Tracker,
                                MemoryDescriptorList,
                                NumberOfPtes,
                                CallingAddress,
                                CallersCaller);
        }

        MiUnlockSystemSpace(OldIrql);

Update:
        if ((MemoryDescriptorList->MdlFlags & MDL_PARTIAL) != 0) {
            MemoryDescriptorList->MdlFlags |= MDL_PARTIAL_HAS_BEEN_MAPPED;
        }

        return BaseVa;

    } else {

        return MiMapLockedPagesInUserSpace (MemoryDescriptorList,
                                            StartingVa,
                                            CacheType,
                                            RequestedAddress);
    }
}

PVOID
MiMapSinglePage (
     IN PVOID VirtualAddress OPTIONAL,
     IN PFN_NUMBER PageFrameIndex,
     IN MEMORY_CACHING_TYPE CacheType,
     IN MM_PAGE_PRIORITY Priority
     )

/*++

Routine Description:

    This function (re)maps a single system PTE to the specified physical page.

Arguments:

    VirtualAddress - Supplies the virtual address to map the page frame at.
                     NULL indicates a system PTE is needed.  Non-NULL supplies
                     the virtual address returned by an earlier
                     MiMapSinglePage call.

    PageFrameIndex - Supplies the page frame index to map.

    CacheType - Supplies the type of cache mapping to use for the MDL.
                MmCached indicates "normal" kernel or user mappings.

    Priority - Supplies an indication as to how important it is that this
               request succeed under low available PTE conditions.

Return Value:

    Returns the base address where the page is mapped, or NULL if it the
    mapping failed.

Environment:

    Kernel mode.  APC_LEVEL or below.

--*/

{
    PMMPTE PointerPte;
    MMPTE TempPte;

    PAGED_CODE ();

    if (VirtualAddress == NULL) {

        //
        // Make sure there are enough PTEs of the requested size.
        //
    
        if ((Priority != HighPagePriority) &&
            (MiGetSystemPteAvailability (1, Priority) == FALSE)) {
                return NULL;
        }

        PointerPte = MiReserveSystemPtes (1,
                                          SystemPteSpace,
                                          MM_COLOR_ALIGNMENT,
                                          0,
                                          0);

        if (PointerPte == NULL) {
    
            //
            // Not enough system PTES are available.
            //
    
            return NULL;
        }

        ASSERT (PointerPte->u.Hard.Valid == 0);
        VirtualAddress = MiGetVirtualAddressMappedByPte (PointerPte);
    }
    else {
        ASSERT (MI_IS_PHYSICAL_ADDRESS (VirtualAddress) == 0);
        ASSERT (VirtualAddress >= MM_SYSTEM_RANGE_START);

        PointerPte = MiGetPteAddress (VirtualAddress);
        ASSERT (PointerPte->u.Hard.Valid == 1);

        MI_WRITE_INVALID_PTE (PointerPte, ZeroPte);

        KeFlushSingleTb (VirtualAddress,
                         TRUE,
                         TRUE,
                         (PHARDWARE_PTE)PointerPte,
                         ZeroPte.u.Flush);
    }

    TempPte = ValidKernelPte;

    switch (CacheType) {

        case MmNonCached:
            MI_DISABLE_CACHING (TempPte);
            break;

        case MmCached:
            break;

        case MmWriteCombined:
            MI_SET_PTE_WRITE_COMBINE (TempPte);
            break;

        case MmHardwareCoherentCached:
            break;

#if 0
        case MmNonCachedUnordered:
            break;
#endif

        default:
            break;
    }

#if defined(_IA64_)
    if (CacheType != MmCached) {
        KeFlushEntireTb(FALSE, TRUE);
    }
#endif

    TempPte.u.Hard.PageFrameNumber = PageFrameIndex;

    MI_WRITE_VALID_PTE (PointerPte, TempPte);

#if defined(i386)
    //
    // If write combined was specified then flush all caches and TBs.
    //

    if (CacheType == MmWriteCombined && MiWriteCombiningPtes == TRUE) {
        KeFlushEntireTb (FALSE, TRUE);
        KeInvalidateAllCaches (TRUE);
    }
#endif

#if defined(_IA64_)
    if (CacheType != MmCached) {
        MiSweepCacheMachineDependent(VirtualAddress, PAGE_SIZE, CacheType);
    }
#endif

    return VirtualAddress;
}

VOID
MiUnmapSinglePage (
     IN PVOID VirtualAddress
     )

/*++

Routine Description:

    This routine unmaps a single locked pages which was previously mapped via
    an MiMapSinglePage call.

Arguments:

    VirtualAddress - Supplies the virtual address used to map the page.

Return Value:

    None.

Environment:

    Kernel mode.  APC_LEVEL or below, base address is within system space.

--*/

{
    PMMPTE PointerPte;

    PAGED_CODE ();

    ASSERT (MI_IS_PHYSICAL_ADDRESS (VirtualAddress) == 0);
    ASSERT (VirtualAddress >= MM_SYSTEM_RANGE_START);

    PointerPte = MiGetPteAddress (VirtualAddress);

    MiReleaseSystemPtes (PointerPte, 1, SystemPteSpace);
    return;
}

VOID
MiPhysicalViewInserter (
    IN PEPROCESS Process,
    IN PMI_PHYSICAL_VIEW PhysicalView
    )

/*++

Routine Description:

    This function is a nonpaged wrapper which acquires the PFN lock to insert
    a physical VAD into the process chain.

Arguments:

    Process - Supplies the process to add the physical VAD to.

    PhysicalView - Supplies the physical view data to link in.

Return Value:

    None.

Environment:

    Kernel mode.  APC_LEVEL, working set and address space mutexes held.

--*/
{
    KIRQL OldIrql;
    KIRQL OldIrql2;

    LOCK_AWE (Process, OldIrql);

    LOCK_PFN2 (OldIrql2);

    InsertHeadList (&Process->PhysicalVadList, &PhysicalView->ListEntry);

    if (PhysicalView->Vad->u.VadFlags.WriteWatch == 1) {
        MiActiveWriteWatch += 1;
    }

    UNLOCK_PFN2 (OldIrql2);

    UNLOCK_AWE (Process, OldIrql);

    if (PhysicalView->Vad->u.VadFlags.WriteWatch == 1) {

        //
        // Mark this process as forever containing write-watch
        // address space(s).

        if (Process->Vm.u.Flags.WriteWatch == 0) {
            MiMarkProcessAsWriteWatch (Process);
        }
    }
}

VOID
MiPhysicalViewRemover (
    IN PEPROCESS Process,
    IN PMMVAD Vad
    )

/*++

Routine Description:

    This function is a nonpaged wrapper which acquires the PFN lock to remove
    a physical VAD from the process chain.

Arguments:

    Process - Supplies the process to remove the physical VAD from.

    Vad - Supplies the Vad to remove.

Return Value:

    None.

Environment:

    Kernel mode, APC_LEVEL, working set and address space mutexes held.

--*/
{
    KIRQL OldIrql;
    KIRQL OldIrql2;
    PRTL_BITMAP BitMap;
    PLIST_ENTRY NextEntry;
    PMI_PHYSICAL_VIEW PhysicalView;
    ULONG BitMapSize;

    BitMap = NULL;

    LOCK_AWE (Process, OldIrql);

    LOCK_PFN2 (OldIrql2);

    NextEntry = Process->PhysicalVadList.Flink;
    while (NextEntry != &Process->PhysicalVadList) {

        PhysicalView = CONTAINING_RECORD(NextEntry,
                                         MI_PHYSICAL_VIEW,
                                         ListEntry);

        if (PhysicalView->Vad == Vad) {
            RemoveEntryList (NextEntry);

            if (Vad->u.VadFlags.WriteWatch == 1) {
                MiActiveWriteWatch -= 1;
                BitMap = PhysicalView->BitMap;
                ASSERT (BitMap != NULL);
            }

            UNLOCK_PFN2 (OldIrql2);
            UNLOCK_AWE (Process, OldIrql);
            ExFreePool (PhysicalView);

            if (BitMap != NULL) {
                BitMapSize = sizeof(RTL_BITMAP) + (ULONG)(((BitMap->SizeOfBitMap + 31) / 32) * 4);
                PsReturnPoolQuota (Process, NonPagedPool, BitMapSize);
                ExFreePool (BitMap);
            }

            return;
        }

        NextEntry = NextEntry->Flink;
    }

    ASSERT (FALSE);

    UNLOCK_PFN2 (OldIrql2);
    UNLOCK_AWE (Process, OldIrql);
}

VOID
MiPhysicalViewAdjuster (
    IN PEPROCESS Process,
    IN PMMVAD OldVad,
    IN PMMVAD NewVad
    )

/*++

Routine Description:

    This function is a nonpaged wrapper which acquires the PFN lock to repoint
    a physical VAD in the process chain.

Arguments:

    Process - Supplies the process in which to adjust the physical VAD.

    Vad - Supplies the old Vad to replace.

    NewVad - Supplies the newVad to substitute.

Return Value:

    None.

Environment:

    Kernel mode, called with APCs disabled, working set mutex held.

--*/
{
    KIRQL OldIrql;
    KIRQL OldIrql2;
    PLIST_ENTRY NextEntry;
    PMI_PHYSICAL_VIEW PhysicalView;

    LOCK_AWE (Process, OldIrql);

    LOCK_PFN2 (OldIrql2);

    NextEntry = Process->PhysicalVadList.Flink;
    while (NextEntry != &Process->PhysicalVadList) {

        PhysicalView = CONTAINING_RECORD(NextEntry,
                                         MI_PHYSICAL_VIEW,
                                         ListEntry);

        if (PhysicalView->Vad == OldVad) {
            PhysicalView->Vad = NewVad;
            UNLOCK_PFN2 (OldIrql2);
            UNLOCK_AWE (Process, OldIrql);
            return;
        }

        NextEntry = NextEntry->Flink;
    }

    ASSERT (FALSE);

    UNLOCK_PFN2 (OldIrql2);
    UNLOCK_AWE (Process, OldIrql);
}

PVOID
MiMapLockedPagesInUserSpace (
     IN PMDL MemoryDescriptorList,
     IN PVOID StartingVa,
     IN MEMORY_CACHING_TYPE CacheType,
     IN PVOID BaseVa
     )

/*++

Routine Description:

    This function maps physical pages described by a memory descriptor
    list into the user portion of the virtual address space.

Arguments:

    MemoryDescriptorList - Supplies a valid Memory Descriptor List which has
                           been updated by MmProbeAndLockPages.


    StartingVa - Supplies the starting address.

    CacheType - Supplies the type of cache mapping to use for the MDL.
                MmCached indicates "normal" user mappings.

    BaseVa - Supplies the base address of the view. If the initial
             value of this argument is not null, then the view will
             be allocated starting at the specified virtual
             address rounded down to the next 64kb address
             boundary. If the initial value of this argument is
             null, then the operating system will determine
             where to allocate the view.

Return Value:

    Returns the base address where the pages are mapped.  The base address
    has the same offset as the virtual address in the MDL.

    This routine will raise an exception if the processor mode is USER_MODE
    and quota limits or VM limits are exceeded.

Environment:

    Kernel mode.  APC_LEVEL or below.

--*/

{
    PFN_NUMBER NumberOfPages;
    PPFN_NUMBER Page;
    PMMPTE PointerPte;
    PMMPTE PointerPde;
    PMMPTE PointerPpe;
    PCHAR Va;
    MMPTE TempPte;
    PVOID EndingAddress;
    PMMVAD Vad;
    PEPROCESS Process;
    PMMPFN Pfn2;
    PVOID UsedPageTableHandle;
    PMI_PHYSICAL_VIEW PhysicalView;
#if defined (_WIN64)
    PVOID UsedPageDirectoryHandle;
#endif

    PAGED_CODE ();
    Page = (PPFN_NUMBER)(MemoryDescriptorList + 1);
    NumberOfPages = COMPUTE_PAGES_SPANNED (StartingVa,
                                           MemoryDescriptorList->ByteCount);

    if (MemoryDescriptorList->MdlFlags & MDL_IO_SPACE) {
        ExRaiseStatus (STATUS_INVALID_ADDRESS);
        return NULL;
    }

    //
    // Map the pages into the user part of the address as user
    // read/write no-delete.
    //

    TempPte = ValidUserPte;

    switch (CacheType) {

        case MmNonCached:
            MI_DISABLE_CACHING (TempPte);
            break;

        case MmCached:
            break;

        case MmWriteCombined:
            MI_SET_PTE_WRITE_COMBINE (TempPte);
            break;

        case MmHardwareCoherentCached:
            break;

#if 0
        case MmNonCachedUnordered:
            break;
#endif

        default:
            break;
    }

    Process = PsGetCurrentProcess ();

    //
    // Make sure the specified starting and ending addresses are
    // within the user part of the virtual address space.
    //

    if (BaseVa != NULL) {

        if ((ULONG_PTR)BaseVa & (PAGE_SIZE - 1)) {

            //
            // Invalid base address.
            //

            ExRaiseStatus (STATUS_INVALID_ADDRESS);
            return NULL;
        }

        EndingAddress = (PVOID)((PCHAR)BaseVa + ((ULONG_PTR)NumberOfPages * PAGE_SIZE) - 1);

        if (EndingAddress <= BaseVa) {

            //
            // Invalid region size.
            //

            ExRaiseStatus (STATUS_INVALID_ADDRESS);
            return NULL;
        }

        if (EndingAddress > MM_HIGHEST_VAD_ADDRESS) {

            //
            // Invalid region size.
            //

            ExRaiseStatus (STATUS_INVALID_ADDRESS);
            return NULL;
        }

        LOCK_WS_AND_ADDRESS_SPACE (Process);

        //
        // Make sure the address space was not deleted, if so, return an error.
        //

        if (Process->AddressSpaceDeleted != 0) {
            UNLOCK_WS_AND_ADDRESS_SPACE (Process);
            ExRaiseStatus (STATUS_PROCESS_IS_TERMINATING);
            return NULL;
        }

        Vad = MiCheckForConflictingVad (BaseVa, EndingAddress);

        //
        // Make sure the address space is not already in use.
        //

        if (Vad != (PMMVAD)NULL) {
            UNLOCK_WS_AND_ADDRESS_SPACE (Process);
            ExRaiseStatus (STATUS_CONFLICTING_ADDRESSES);
            return NULL;
        }
    }
    else {

        //
        // Get the working set mutex and address creation mutex.
        //

        LOCK_WS_AND_ADDRESS_SPACE (Process);

        //
        // Make sure the address space was not deleted, if so, return an error.
        //

        if (Process->AddressSpaceDeleted != 0) {
            UNLOCK_WS_AND_ADDRESS_SPACE (Process);
            ExRaiseStatus (STATUS_PROCESS_IS_TERMINATING);
            return NULL;
        }

        try {

            BaseVa = MiFindEmptyAddressRange ( (ULONG_PTR)NumberOfPages * PAGE_SIZE,
                                                X64K,
                                                0 );

            EndingAddress = (PVOID)((PCHAR)BaseVa + ((ULONG_PTR)NumberOfPages * PAGE_SIZE) - 1);

        } except (EXCEPTION_EXECUTE_HANDLER) {
            BaseVa = NULL;
            goto Done;
        }
    }

    PhysicalView = (PMI_PHYSICAL_VIEW)ExAllocatePoolWithTag (NonPagedPool,
                                                             sizeof(MI_PHYSICAL_VIEW),
                                                             MI_PHYSICAL_VIEW_KEY);
    if (PhysicalView == NULL) {
        BaseVa = NULL;
        goto Done;
    }

    Vad = ExAllocatePoolWithTag (NonPagedPool, sizeof(MMVAD), ' daV');

    if (Vad == NULL) {
        ExFreePool (PhysicalView);
        BaseVa = NULL;
        goto Done;
    }

    PhysicalView->Vad = Vad;
    PhysicalView->StartVa = BaseVa;
    PhysicalView->EndVa = EndingAddress;

    Vad->StartingVpn = MI_VA_TO_VPN (BaseVa);
    Vad->EndingVpn = MI_VA_TO_VPN (EndingAddress);
    Vad->ControlArea = NULL;
    Vad->FirstPrototypePte = NULL;
    Vad->u.LongFlags = 0;
    Vad->u.VadFlags.Protection = MM_READWRITE;
    Vad->u.VadFlags.PhysicalMapping = 1;
    Vad->u.VadFlags.PrivateMemory = 1;
    Vad->u4.Banked = NULL;

    try {

        MiInsertVad (Vad);

    } except (EXCEPTION_EXECUTE_HANDLER) {
        ExFreePool (PhysicalView);
        ExFreePool (Vad);
        BaseVa = NULL;
        goto Done;
    }

    MiPhysicalViewInserter (Process, PhysicalView);

#if defined(_IA64_)
    if (CacheType != MmCached) {
        KeFlushEntireTb(FALSE, TRUE);
    }
#endif

    //
    // Create a page table and fill in the mappings for the Vad.
    //

    Va = BaseVa;
    PointerPte = MiGetPteAddress (BaseVa);

    do {

        if (*Page == MM_EMPTY_LIST) {
            break;
        }

        ASSERT (*Page <= MmHighestPhysicalPage);

        PointerPde = MiGetPteAddress (PointerPte);
        PointerPpe = MiGetPdeAddress (PointerPte);

#if defined (_WIN64)
        MiMakePpeExistAndMakeValid (PointerPpe, Process, FALSE);
        if (PointerPde->u.Long == 0) {
            UsedPageDirectoryHandle = MI_GET_USED_PTES_HANDLE (PointerPte);
            MI_INCREMENT_USED_PTES_BY_HANDLE (UsedPageDirectoryHandle);
        }
#endif

        MiMakePdeExistAndMakeValid(PointerPde, Process, FALSE);

        ASSERT (PointerPte->u.Hard.Valid == 0);
        TempPte.u.Hard.PageFrameNumber = *Page;
        MI_WRITE_VALID_PTE (PointerPte, TempPte);

        //
        // A PTE just went from not present, not transition to
        // present.  The share count and valid count must be
        // updated in the page table page which contains this
        // PTE.
        //

        Pfn2 = MI_PFN_ELEMENT(PointerPde->u.Hard.PageFrameNumber);
        Pfn2->u2.ShareCount += 1;

        //
        // Another zeroed PTE has become non-zero.
        //

        UsedPageTableHandle = MI_GET_USED_PTES_HANDLE (Va);

        MI_INCREMENT_USED_PTES_BY_HANDLE (UsedPageTableHandle);

        Page += 1;
        PointerPte += 1;
        NumberOfPages -= 1;
        Va += PAGE_SIZE;
    } while (NumberOfPages != 0);

#if defined(_IA64_)
    if (CacheType != MmCached) {
        MiSweepCacheMachineDependent (BaseVa, MemoryDescriptorList->ByteCount, CacheType);
    }
#endif

Done:
    UNLOCK_WS_AND_ADDRESS_SPACE (Process);
    if (BaseVa == NULL) {
        ExRaiseStatus (STATUS_INSUFFICIENT_RESOURCES);
        return NULL;
    }

#if defined(i386)
    //
    // If write combined was specified then flush all caches and TBs.
    //

    if (CacheType == MmWriteCombined && MiWriteCombiningPtes == TRUE) {
        KeFlushEntireTb (FALSE, TRUE);
        KeInvalidateAllCaches (TRUE);
    }
#endif

    BaseVa = (PVOID)((PCHAR)BaseVa + MemoryDescriptorList->ByteOffset);

    return BaseVa;
}

VOID
MmUnmapLockedPages (
     IN PVOID BaseAddress,
     IN PMDL MemoryDescriptorList
     )

/*++

Routine Description:

    This routine unmaps locked pages which were previously mapped via
    a MmMapLockedPages call.

Arguments:

    BaseAddress - Supplies the base address where the pages were previously
                  mapped.

    MemoryDescriptorList - Supplies a valid Memory Descriptor List which has
                            been updated by MmProbeAndLockPages.

Return Value:

    None.

Environment:

    Kernel mode.  DISPATCH_LEVEL or below if base address is within
    system space; APC_LEVEL or below if base address is user space.

--*/

{
    PFN_NUMBER NumberOfPages;
    PFN_NUMBER i;
    PPFN_NUMBER Page;
    PMMPTE PointerPte;
    PMMPTE PointerBase;
    PVOID StartingVa;
    KIRQL OldIrql;
    PVOID PoolBlock;

    ASSERT (MemoryDescriptorList->ByteCount != 0);
    ASSERT ((MemoryDescriptorList->MdlFlags & MDL_PARENT_MAPPED_SYSTEM_VA) == 0);

    if (MI_IS_PHYSICAL_ADDRESS (BaseAddress)) {

        //
        // MDL is not mapped into virtual space, just clear the fields
        // and return.
        //

        MemoryDescriptorList->MdlFlags &= ~(MDL_MAPPED_TO_SYSTEM_VA |
                                            MDL_PARTIAL_HAS_BEEN_MAPPED);
        return;
    }

    if (BaseAddress > MM_HIGHEST_USER_ADDRESS) {

        StartingVa = (PVOID)((PCHAR)MemoryDescriptorList->StartVa +
                        MemoryDescriptorList->ByteOffset);

        NumberOfPages = COMPUTE_PAGES_SPANNED (StartingVa,
                                               MemoryDescriptorList->ByteCount);

        PointerBase = MiGetPteAddress (BaseAddress);


        ASSERT ((MemoryDescriptorList->MdlFlags & MDL_MAPPED_TO_SYSTEM_VA) != 0);


#if DBG
        PointerPte = PointerBase;
        i = NumberOfPages;
        Page = (PPFN_NUMBER)(MemoryDescriptorList + 1);
        if ((MemoryDescriptorList->MdlFlags & MDL_LOCK_HELD) == 0) {
            LOCK_PFN2 (OldIrql);
        }

        while (i != 0) {
            ASSERT (PointerPte->u.Hard.Valid == 1);
            ASSERT (*Page == MI_GET_PAGE_FRAME_FROM_PTE (PointerPte));
            if ((MemoryDescriptorList->MdlFlags & (MDL_IO_SPACE | MDL_PHYSICAL_VIEW)) == 0) {
                PMMPFN Pfn3;
                Pfn3 = MI_PFN_ELEMENT (*Page);
                ASSERT (Pfn3->u3.e2.ReferenceCount != 0);
            }

            Page += 1;
            PointerPte += 1;
            i -= 1;
        }

        if ((MemoryDescriptorList->MdlFlags & MDL_LOCK_HELD) == 0) {
            UNLOCK_PFN2 (OldIrql);
        }
#endif //DBG

        MemoryDescriptorList->MdlFlags &= ~(MDL_MAPPED_TO_SYSTEM_VA |
                                            MDL_PARTIAL_HAS_BEEN_MAPPED);

        if (MmTrackPtes != 0) {
            MiLockSystemSpace(OldIrql);
            PoolBlock = MiRemovePteTracker (MemoryDescriptorList,
                                            PointerBase,
                                            NumberOfPages);
            MiUnlockSystemSpace(OldIrql);

            //
            // Can't free the pool block here because we may be getting called
            // from the fault path in MiWaitForInPageComplete holding the PFN
            // lock.  Queue the block for later release.
            //

            if (PoolBlock) {
                MiInsertDeadPteTrackingBlock (PoolBlock);
            }
        }

        MiReleaseSystemPtes (PointerBase, (ULONG)NumberOfPages, SystemPteSpace);
        return;

    } else {

        MiUnmapLockedPagesInUserSpace (BaseAddress,
                                       MemoryDescriptorList);
    }
}

VOID
MiUnmapLockedPagesInUserSpace (
     IN PVOID BaseAddress,
     IN PMDL MemoryDescriptorList
     )

/*++

Routine Description:

    This routine unmaps locked pages which were previously mapped via
    a MmMapLockedPages function.

Arguments:

    BaseAddress - Supplies the base address where the pages were previously
                  mapped.

    MemoryDescriptorList - Supplies a valid Memory Descriptor List which has
                           been updated by MmProbeAndLockPages.

Return Value:

    None.

Environment:

    Kernel mode.  DISPATCH_LEVEL or below if base address is within system space;
                APC_LEVEL or below if base address is user space.

--*/

{
    PFN_NUMBER NumberOfPages;
    PPFN_NUMBER Page;
    PMMPTE PointerPte;
    PMMPTE PointerBase;
    PMMPTE PointerPde;
    PVOID StartingVa;
    KIRQL OldIrql;
    PMMVAD Vad;
    PVOID TempVa;
    PEPROCESS Process;
    PVOID UsedPageTableHandle;
#if defined (_WIN64)
    PVOID UsedPageDirectoryHandle;
#endif

    MmLockPagableSectionByHandle (ExPageLockHandle);

    StartingVa = (PVOID)((PCHAR)MemoryDescriptorList->StartVa +
                    MemoryDescriptorList->ByteOffset);

    Page = (PPFN_NUMBER)(MemoryDescriptorList + 1);
    NumberOfPages = COMPUTE_PAGES_SPANNED (StartingVa,
                                           MemoryDescriptorList->ByteCount);

    PointerPte = MiGetPteAddress (BaseAddress);
    PointerBase = PointerPte;

    //
    // This was mapped into the user portion of the address space and
    // the corresponding virtual address descriptor must be deleted.
    //

    //
    // Get the working set mutex and address creation mutex.
    //

    Process = PsGetCurrentProcess ();

    LOCK_WS_AND_ADDRESS_SPACE (Process);

    Vad = MiLocateAddress (BaseAddress);
    ASSERT (Vad != NULL);

    MiPhysicalViewRemover (Process, Vad);

    MiRemoveVad (Vad);

    //
    // Get the PFN mutex so we can safely decrement share and valid
    // counts on page table pages.
    //

    LOCK_PFN (OldIrql);

    do {

        if (*Page == MM_EMPTY_LIST) {
            break;
        }

        ASSERT64 (MiGetPdeAddress(PointerPte)->u.Hard.Valid == 1);
        ASSERT (MiGetPteAddress(PointerPte)->u.Hard.Valid == 1);
        ASSERT (PointerPte->u.Hard.Valid == 1);

        (VOID)KeFlushSingleTb (BaseAddress,
                               TRUE,
                               FALSE,
                               (PHARDWARE_PTE)PointerPte,
                               ZeroPte.u.Flush);

        PointerPde = MiGetPteAddress(PointerPte);
        MiDecrementShareAndValidCount (MI_GET_PAGE_FRAME_FROM_PTE (PointerPde));

        //
        // Another PTE has become zero.
        //

        UsedPageTableHandle = MI_GET_USED_PTES_HANDLE (BaseAddress);

        MI_DECREMENT_USED_PTES_BY_HANDLE (UsedPageTableHandle);

        //
        // If all the entries have been eliminated from the previous
        // page table page, delete the page table page itself.  Likewise
        // with the page directory page.
        //

        if (MI_GET_USED_PTES_FROM_HANDLE (UsedPageTableHandle) == 0) {

            TempVa = MiGetVirtualAddressMappedByPte (PointerPde);
            MiDeletePte (PointerPde,
                         TempVa,
                         FALSE,
                         Process,
                         (PMMPTE)NULL,
                         NULL);

#if defined (_WIN64)
            UsedPageDirectoryHandle = MI_GET_USED_PTES_HANDLE (PointerPte);

            MI_DECREMENT_USED_PTES_BY_HANDLE (UsedPageDirectoryHandle);

            if (MI_GET_USED_PTES_FROM_HANDLE (UsedPageDirectoryHandle) == 0) {

                TempVa = MiGetVirtualAddressMappedByPte(MiGetPteAddress(PointerPde));
                MiDeletePte (MiGetPteAddress(PointerPde),
                             TempVa,
                             FALSE,
                             Process,
                             NULL,
                             NULL);
            }
#endif
        }

        Page += 1;
        PointerPte += 1;
        NumberOfPages -= 1;
        BaseAddress = (PVOID)((PCHAR)BaseAddress + PAGE_SIZE);
    } while (NumberOfPages != 0);

    UNLOCK_PFN (OldIrql);
    UNLOCK_WS_AND_ADDRESS_SPACE (Process);
    ExFreePool (Vad);
    MmUnlockPagableImageSection(ExPageLockHandle);
    return;
}


PVOID
MmMapIoSpace (
     IN PHYSICAL_ADDRESS PhysicalAddress,
     IN SIZE_T NumberOfBytes,
     IN MEMORY_CACHING_TYPE CacheType
     )

/*++

Routine Description:

    This function maps the specified physical address into the non-pagable
    portion of the system address space.

Arguments:

    PhysicalAddress - Supplies the starting physical address to map.

    NumberOfBytes - Supplies the number of bytes to map.

    CacheType - Supplies MmNonCached if the physical address is to be mapped
                as non-cached, MmCached if the address should be cached, and
                MmWriteCombined if the address should be cached and
                write-combined as a frame buffer which is to be used only by
                the video port driver.  All other callers should use
                MmUSWCCached.  MmUSWCCached is available only if the PAT
                feature is present and available.

                For I/O device registers, this is usually specified
                as MmNonCached.

Return Value:

    Returns the virtual address which maps the specified physical addresses.
    The value NULL is returned if sufficient virtual address space for
    the mapping could not be found.

Environment:

    Kernel mode, Should be IRQL of APC_LEVEL or below, but unfortunately
    callers are coming in at DISPATCH_LEVEL and it's too late to change the
    rules now.  This means you can never make this routine pagable.

--*/

{
    PFN_NUMBER NumberOfPages;
    PFN_NUMBER PageFrameIndex;
    PMMPTE PointerPte;
    PVOID BaseVa;
    MMPTE TempPte;
    KIRQL OldIrql;
    PMDL TempMdl;
    PFN_NUMBER MdlHack[(sizeof(MDL)/sizeof(PFN_NUMBER)) + 1];
    PPFN_NUMBER Page;
    PLOCK_TRACKER Tracker;
    PVOID CallingAddress;
    PVOID CallersCaller;
#ifdef i386
    NTSTATUS Status;
#endif

#if !defined (_X86_)
    CallingAddress = (PVOID)_ReturnAddress();
    CallersCaller = (PVOID)0;
#endif

    //
    // For compatibility for when CacheType used to be passed as a BOOLEAN
    // mask off the upper bits (TRUE == MmCached, FALSE == MmNonCached).
    //

    CacheType &= 0xFF;

    if (CacheType >= MmMaximumCacheType) {
        return (NULL);
    }

#if defined (i386) && !defined (_X86PAE_)
    ASSERT (PhysicalAddress.HighPart == 0);
#endif

    ASSERT (NumberOfBytes != 0);
    NumberOfPages = COMPUTE_PAGES_SPANNED (PhysicalAddress.LowPart,
                                           NumberOfBytes);

    PointerPte = MiReserveSystemPtes((ULONG)NumberOfPages,
                                     SystemPteSpace,
                                     MM_COLOR_ALIGNMENT,
                                     (PhysicalAddress.LowPart &
                                                       MM_COLOR_MASK_VIRTUAL),
                                     FALSE);
    if (PointerPte == NULL) {
        return(NULL);
    }

    BaseVa = (PVOID)MiGetVirtualAddressMappedByPte (PointerPte);
    BaseVa = (PVOID)((PCHAR)BaseVa + BYTE_OFFSET(PhysicalAddress.LowPart));

    TempPte = ValidKernelPte;

#ifdef i386
    //
    // Set the physical range to proper caching type.  If the PAT feature
    // is supported, then set the caching type in the PTE, otherwise modify
    // the MTRRs if applicable.  If the cache type is MmUSWCCached and the
    // PAT is not supported then fail the call.
    //

    if (KeFeatureBits & KF_PAT) {
        if ((CacheType == MmWriteCombined) || (CacheType == MmUSWCCached)) {
            if (MiWriteCombiningPtes == TRUE) {
                MI_SET_PTE_WRITE_COMBINE(TempPte);
                Status = STATUS_SUCCESS;
            } else {
                Status = STATUS_UNSUCCESSFUL;
            }
        } else {

            //
            // For Non-MmFrameBufferCaching type use existing mm macros.
            //

            Status = STATUS_SUCCESS;
        }
    } else {

        // Set the MTRRs if possible.

        Status = KeSetPhysicalCacheTypeRange(
                    PhysicalAddress,
                    NumberOfBytes,
                    CacheType
                    );
    }

    //
    // If range could not be set, determine what to do
    //

    if (!NT_SUCCESS(Status)) {

        if ((Status == STATUS_NOT_SUPPORTED) &&
            ((CacheType == MmNonCached) || (CacheType == MmCached))) {

            //
            // The range may not have been set into the proper cache
            // type.  If the range is either MmNonCached or MmCached just
            // continue as the PTE will be marked properly.
            //

            NOTHING;

        } else if (Status == STATUS_UNSUCCESSFUL  &&  CacheType == MmCached) {

            //
            // If setting a range to Cached was unsuccessful things are not
            // optimal, but not fatal.  The range can be returned to the
            // caller and it will have whatever caching type it has - possibly
            // something below fully cached.
            //

            NOTHING;

        } else {

            //
            // If there's still a problem, fail the request.
            //

            MiReleaseSystemPtes(PointerPte, NumberOfPages, SystemPteSpace);

            return(NULL);
         }
    }
#endif

    if (CacheType == MmNonCached) {
        MI_DISABLE_CACHING (TempPte);
    }

#if defined(_IA64_)
    if (CacheType != MmCached) { 
        KeFlushEntireTb(FALSE, TRUE);
    }
#endif

    PageFrameIndex = (PFN_NUMBER)(PhysicalAddress.QuadPart >> PAGE_SHIFT);

    do {
        ASSERT (PointerPte->u.Hard.Valid == 0);
        TempPte.u.Hard.PageFrameNumber = PageFrameIndex;
        MI_WRITE_VALID_PTE (PointerPte, TempPte);
        PointerPte += 1;
        PageFrameIndex += 1;
        NumberOfPages -= 1;
    } while (NumberOfPages != 0);

#if defined(i386)
    //
    // WriteCombined is a non self-snooping memory type.  This memory type
    // requires a writeback invalidation of all the caches on all processors
    // and each accompanying TB flush if the PAT is supported.
    //

    if ((KeFeatureBits & KF_PAT) && ((CacheType == MmWriteCombined)
        || (CacheType == MmUSWCCached)) && (MiWriteCombiningPtes == TRUE)) {
            KeFlushEntireTb (FALSE, TRUE);
            KeInvalidateAllCaches (TRUE);
    }
#endif

#if defined(_IA64_)
    if (CacheType != MmCached) {
        MiSweepCacheMachineDependent(BaseVa, NumberOfBytes, CacheType);
    }
#endif

    if (MmTrackPtes != 0) {

        //
        // First free any zombie blocks as no locks are being held.
        //

        MiReleaseDeadPteTrackers ();

        Tracker = ExAllocatePoolWithTag (NonPagedPool,
                                         sizeof (PTE_TRACKER),
                                         'ySmM');

        if (Tracker != NULL) {
#if defined (_X86_)
            RtlGetCallersAddress(&CallingAddress, &CallersCaller);
#endif

            TempMdl = (PMDL) &MdlHack;
            TempMdl->MappedSystemVa = BaseVa;
            TempMdl->StartVa = (PVOID)(ULONG_PTR)PhysicalAddress.QuadPart;
            TempMdl->ByteOffset = BYTE_OFFSET(PhysicalAddress.LowPart);
            TempMdl->ByteCount = (ULONG)NumberOfBytes;
    
            Page = (PPFN_NUMBER) (TempMdl + 1);
            Page = (PPFN_NUMBER)-1;
    
            MiLockSystemSpace(OldIrql);
    
            MiInsertPteTracker (Tracker,
                                TempMdl,
                                COMPUTE_PAGES_SPANNED (PhysicalAddress.LowPart,
                                               NumberOfBytes),
                                CallingAddress,
                                CallersCaller);
    
            MiUnlockSystemSpace(OldIrql);
        }
        else {
            MiTrackPtesAborted = TRUE;
        }
    }
    
    return BaseVa;
}

VOID
MmUnmapIoSpace (
     IN PVOID BaseAddress,
     IN SIZE_T NumberOfBytes
     )

/*++

Routine Description:

    This function unmaps a range of physical address which were previously
    mapped via an MmMapIoSpace function call.

Arguments:

    BaseAddress - Supplies the base virtual address where the physical
                  address was previously mapped.

    NumberOfBytes - Supplies the number of bytes which were mapped.

Return Value:

    None.

Environment:

    Kernel mode, Should be IRQL of APC_LEVEL or below, but unfortunately
    callers are coming in at DISPATCH_LEVEL and it's too late to change the
    rules now.  This means you can never make this routine pagable.

--*/

{
    PFN_NUMBER NumberOfPages;
    PMMPTE FirstPte;
    KIRQL OldIrql;
    PVOID PoolBlock;

    PAGED_CODE();
    ASSERT (NumberOfBytes != 0);
    NumberOfPages = COMPUTE_PAGES_SPANNED (BaseAddress, NumberOfBytes);
    FirstPte = MiGetPteAddress (BaseAddress);
    MiReleaseSystemPtes(FirstPte, (ULONG)NumberOfPages, SystemPteSpace);

    if (MmTrackPtes != 0) {
        MiLockSystemSpace(OldIrql);

        PoolBlock = MiRemovePteTracker (NULL,
                                        FirstPte,
                                        NumberOfPages);
        MiUnlockSystemSpace(OldIrql);

        //
        // Can't free the pool block here because we may be getting called
        // from the fault path in MiWaitForInPageComplete holding the PFN
        // lock.  Queue the block for later release.
        //

        if (PoolBlock) {
            MiInsertDeadPteTrackingBlock (PoolBlock);
        }
    }

    return;
}

PVOID
MmAllocateContiguousMemorySpecifyCache (
    IN SIZE_T NumberOfBytes,
    IN PHYSICAL_ADDRESS LowestAcceptableAddress,
    IN PHYSICAL_ADDRESS HighestAcceptableAddress,
    IN PHYSICAL_ADDRESS BoundaryAddressMultiple OPTIONAL,
    IN MEMORY_CACHING_TYPE CacheType
    )

/*++

Routine Description:

    This function allocates a range of physically contiguous non-cached,
    non-paged memory.  This is accomplished by using MmAllocateContiguousMemory
    which uses nonpaged pool virtual addresses to map the found memory chunk.

    Then this function establishes another map to the same physical addresses,
    but this alternate map is initialized as non-cached.  All references by
    our caller will be done through this alternate map.

    This routine is designed to be used by a driver's initialization
    routine to allocate a contiguous block of noncached physical memory for
    things like the AGP GART.

Arguments:

    NumberOfBytes - Supplies the number of bytes to allocate.

    LowestAcceptableAddress - Supplies the lowest physical address
                              which is valid for the allocation.  For
                              example, if the device can only reference
                              physical memory in the 8M to 16MB range, this
                              value would be set to 0x800000 (8Mb).

    HighestAcceptableAddress - Supplies the highest physical address
                               which is valid for the allocation.  For
                               example, if the device can only reference
                               physical memory below 16MB, this
                               value would be set to 0xFFFFFF (16Mb - 1).

    BoundaryAddressMultiple - Supplies the physical address multiple this
                              allocation must not cross.

Return Value:

    NULL - a contiguous range could not be found to satisfy the request.

    NON-NULL - Returns a pointer (virtual address in the nonpaged portion
               of the system) to the allocated physically contiguous
               memory.

Environment:

    Kernel mode, IRQL of DISPATCH_LEVEL or below.

--*/

{
    PVOID BaseAddress;
    PVOID NewVa;
    PFN_NUMBER LowestPfn;
    PFN_NUMBER HighestPfn;
    PFN_NUMBER BoundaryPfn;
    PMMPTE PointerPte;
    PHYSICAL_ADDRESS PhysicalAddress;
    PVOID CallingAddress;

#if defined (_X86_)
    PVOID CallersCaller;

    RtlGetCallersAddress(&CallingAddress, &CallersCaller);
#else
    CallingAddress = (PVOID)_ReturnAddress();
#endif

    ASSERT (NumberOfBytes != 0);

    LowestPfn = (PFN_NUMBER)(LowestAcceptableAddress.QuadPart >> PAGE_SHIFT);
    if (BYTE_OFFSET(LowestAcceptableAddress.LowPart)) {
        LowestPfn += 1;
    }

    if (BYTE_OFFSET(BoundaryAddressMultiple.LowPart)) {
        return NULL;
    }

    BoundaryPfn = (PFN_NUMBER)(BoundaryAddressMultiple.QuadPart >> PAGE_SHIFT);

    HighestPfn = (PFN_NUMBER)(HighestAcceptableAddress.QuadPart >> PAGE_SHIFT);

    BaseAddress = MiAllocateContiguousMemory(NumberOfBytes,
                                             LowestPfn,
                                             HighestPfn,
                                             BoundaryPfn,
                                             CallingAddress);

    if (BaseAddress) {

        if (CacheType != MmCached) {

            //
            // We have an address range but it's cached.  Create an uncached
            // alternate mapping now.  Stash the original virtual address at the
            // end of the mapped range so we can unmap the nonpaged pool VAs and
            // the actual pages when the caller frees the memory.
            //

            PhysicalAddress = MmGetPhysicalAddress (BaseAddress);

            NewVa = MmMapIoSpace (PhysicalAddress,
                                  NumberOfBytes + (2 * PAGE_SIZE),
                                  CacheType);

            if (NewVa) {

                PointerPte = MiGetPteAddress(NewVa);

                PointerPte += ((NumberOfBytes + PAGE_SIZE - 1) >> PAGE_SHIFT);
                PointerPte->u.Long = (ULONG_PTR)BaseAddress;

                PointerPte += 1;
                PointerPte->u.Long = NumberOfBytes;

                KeSweepDcache (TRUE);
                BaseAddress = NewVa;
            }
            else {
                MmFreeContiguousMemory (BaseAddress);
                BaseAddress = NULL;
            }
        }
    }

    return BaseAddress;
}

PVOID
MmAllocateContiguousMemory (
    IN SIZE_T NumberOfBytes,
    IN PHYSICAL_ADDRESS HighestAcceptableAddress
    )

/*++

Routine Description:

    This function allocates a range of physically contiguous non-paged pool.

    This routine is designed to be used by a driver's initialization
    routine to allocate a contiguous block of physical memory for
    issuing DMA requests from.

Arguments:

    NumberOfBytes - Supplies the number of bytes to allocate.

    HighestAcceptableAddress - Supplies the highest physical address
                               which is valid for the allocation.  For
                               example, if the device can only reference
                               physical memory in the lower 16MB this
                               value would be set to 0xFFFFFF (16Mb - 1).

Return Value:

    NULL - a contiguous range could not be found to satisfy the request.

    NON-NULL - Returns a pointer (virtual address in the nonpaged portion
               of the system) to the allocated physically contiguous
               memory.

Environment:

    Kernel mode, IRQL of DISPATCH_LEVEL or below.

--*/

{
    PFN_NUMBER HighestPfn;
    PVOID CallingAddress;

#if defined (_X86_)
    PVOID CallersCaller;

    RtlGetCallersAddress(&CallingAddress, &CallersCaller);
#else
    CallingAddress = (PVOID)_ReturnAddress();
#endif

    HighestPfn = (PFN_NUMBER)(HighestAcceptableAddress.QuadPart >> PAGE_SHIFT);

    return MiAllocateContiguousMemory(NumberOfBytes,
                                      0,
                                      HighestPfn,
                                      0,
                                      CallingAddress);
}

PVOID
MmAllocateIndependentPages(
    IN SIZE_T NumberOfBytes
    )

/*++

Routine Description:

    This function allocates a range of virtually contiguous nonpaged pages that
    can have independent page protections applied to each page.

Arguments:

    NumberOfBytes - Supplies the number of bytes to allocate.

Return Value:

    The virtual address of the memory or NULL if none could be allocated.

Environment:

    Kernel mode, IRQL of APC_LEVEL or below.

--*/

{
    PFN_NUMBER NumberOfPages;
    PMMPTE PointerPte;
    MMPTE TempPte;
    PFN_NUMBER PageFrameIndex;
    PVOID BaseAddress;
    KIRQL OldIrql;

    NumberOfPages = BYTES_TO_PAGES (NumberOfBytes);

    PointerPte = MiReserveSystemPtes ((ULONG)NumberOfPages,
                                      SystemPteSpace,
                                      0,
                                      0,
                                      FALSE);
    if (PointerPte == NULL) {
        return NULL;
    }

    BaseAddress = (PVOID)MiGetVirtualAddressMappedByPte (PointerPte);

    LOCK_PFN (OldIrql);

    if ((SPFN_NUMBER)NumberOfPages > MI_NONPAGABLE_MEMORY_AVAILABLE()) {
        UNLOCK_PFN (OldIrql);
        MiReleaseSystemPtes (PointerPte, (ULONG)NumberOfPages, SystemPteSpace);
        return NULL;
    }

    MmResidentAvailablePages -= NumberOfPages;
    MM_BUMP_COUNTER(28, NumberOfPages);

    do {
        ASSERT (PointerPte->u.Hard.Valid == 0);
        MiEnsureAvailablePageOrWait (NULL, NULL);
        PageFrameIndex = MiRemoveAnyPage (MI_GET_PAGE_COLOR_FROM_PTE (PointerPte));

        MI_MAKE_VALID_PTE (TempPte,
                           PageFrameIndex,
                           MM_READWRITE,
                           PointerPte);

        MI_SET_PTE_DIRTY (TempPte);
        MI_WRITE_VALID_PTE (PointerPte, TempPte);
        MiInitializePfn (PageFrameIndex, PointerPte, 1);

        PointerPte += 1;
        NumberOfPages -= 1;
    } while (NumberOfPages != 0);

    UNLOCK_PFN (OldIrql);

    MiChargeCommitmentCantExpand (NumberOfPages, TRUE);

    MM_TRACK_COMMIT (MM_DBG_COMMIT_INDEPENDENT_PAGES, NumberOfPages);

    return BaseAddress;
}

BOOLEAN
MmSetPageProtection(
    IN PVOID VirtualAddress,
    IN SIZE_T NumberOfBytes,
    IN ULONG NewProtect
    )

/*++

Routine Description:

    This function sets the specified virtual address range to the desired
    protection.  This assumes that the virtual addresses are backed by PTEs
    which can be set (ie: not in kseg0 or large pages).

Arguments:

    VirtualAddress - Supplies the start address to protect.

    NumberOfBytes - Supplies the number of bytes to set.

    NewProtect - Supplies the protection to set the pages to (PAGE_XX).

Return Value:

    TRUE if the protection was applied, FALSE if not.

Environment:

    Kernel mode, IRQL of APC_LEVEL or below.

--*/

{
    PFN_NUMBER i;
    PFN_NUMBER NumberOfPages;
    PMMPTE PointerPte;
    MMPTE TempPte;
    MMPTE NewPteContents;
    KIRQL OldIrql;
    ULONG ProtectionMask;

    if (MI_IS_PHYSICAL_ADDRESS(VirtualAddress)) {
        return FALSE;
    }

    try {
        ProtectionMask = MiMakeProtectionMask (NewProtect);
    } except (EXCEPTION_EXECUTE_HANDLER) {
        return FALSE;
    }

    PointerPte = MiGetPteAddress (VirtualAddress);
    NumberOfPages = BYTES_TO_PAGES (NumberOfBytes);

    LOCK_PFN (OldIrql);

    for (i = 0; i < NumberOfPages; i += 1) {
        TempPte.u.Long = PointerPte->u.Long;

        MI_MAKE_VALID_PTE (NewPteContents,
                           TempPte.u.Hard.PageFrameNumber,
                           ProtectionMask,
                           PointerPte);

        KeFlushSingleTb ((PVOID)((PUCHAR)VirtualAddress + (i << PAGE_SHIFT)),
                         TRUE,
                         TRUE,
                         (PHARDWARE_PTE)PointerPte,
                         NewPteContents.u.Flush);

        PointerPte += 1;
    }

    UNLOCK_PFN (OldIrql);

    return TRUE;
}


PVOID
MiAllocateContiguousMemory (
    IN SIZE_T NumberOfBytes,
    IN PFN_NUMBER LowestAcceptablePfn,
    IN PFN_NUMBER HighestAcceptablePfn,
    IN PFN_NUMBER BoundaryPfn,
    PVOID CallingAddress
    )

/*++

Routine Description:

    This function allocates a range of physically contiguous non-paged
    pool.  It relies on the fact that non-paged pool is built at
    system initialization time from a contiguous range of physical
    memory.  It allocates the specified size of non-paged pool and
    then checks to ensure it is contiguous as pool expansion does
    not maintain the contiguous nature of non-paged pool.

    This routine is designed to be used by a driver's initialization
    routine to allocate a contiguous block of physical memory for
    issuing DMA requests from.

Arguments:

    NumberOfBytes - Supplies the number of bytes to allocate.

    LowestAcceptablePfn - Supplies the lowest page frame number
                          which is valid for the allocation.

    HighestAcceptablePfn - Supplies the highest page frame number
                           which is valid for the allocation.

    BoundaryPfn - Supplies the page frame number multiple the allocation must
                  not cross.  0 indicates it can cross any boundary.

    CallingAddress - Supplies the calling address of the allocator.

Return Value:

    NULL - a contiguous range could not be found to satisfy the request.

    NON-NULL - Returns a pointer (virtual address in the nonpaged portion
               of the system) to the allocated physically contiguous
               memory.

Environment:

    Kernel mode, IRQL of DISPATCH_LEVEL or below.

--*/

{
    PVOID BaseAddress;
    PFN_NUMBER SizeInPages;
    PFN_NUMBER LowestPfn;
    PFN_NUMBER HighestPfn;
    PFN_NUMBER i;

    ASSERT (NumberOfBytes != 0);

#if defined (_X86PAE_)
    if (MiNoLowMemory == TRUE) {
        if (HighestAcceptablePfn <= 0xFFFFF) {
            return MiAllocateLowMemory (NumberOfBytes,
                                        LowestAcceptablePfn,
                                        HighestAcceptablePfn,
                                        BoundaryPfn,
                                        CallingAddress,
                                        'tnoC');
        }
        LowestPfn = 0x100000;
    }
#endif

    BaseAddress = ExAllocatePoolWithTag (NonPagedPoolCacheAligned,
                                         NumberOfBytes,
                                         'mCmM');

    //
    // N.B. This setting of SizeInPages to exactly the request size means the
    // non-NULL return value from MiCheckForContiguousMemory is guaranteed to
    // be the BaseAddress.  If this size is ever changed, then the non-NULL
    // return value must be checked and split/returned accordingly.
    //

    SizeInPages = BYTES_TO_PAGES (NumberOfBytes);

    LowestPfn = LowestAcceptablePfn;
    HighestPfn = HighestAcceptablePfn;

    if (BaseAddress != NULL) {
        if (MiCheckForContiguousMemory( BaseAddress,
                                        SizeInPages,
                                        SizeInPages,
                                        LowestPfn,
                                        HighestPfn,
                                        BoundaryPfn)) {

            return BaseAddress;
        }

        //
        // The allocation from pool does not meet the contiguous
        // requirements. Free the page and see if any of the free
        // pool pages meet the requirement.
        //

        ExFreePool (BaseAddress);

    } else {

        //
        // No pool was available, return NULL.
        //

        return NULL;
    }

    if (KeGetCurrentIrql() > APC_LEVEL) {
        return NULL;
    }

    BaseAddress = NULL;

    i = 3;

    InterlockedIncrement (&MiDelayPageFaults);

    for (; ; ) {
        BaseAddress = MiFindContiguousMemory (LowestPfn,
                                              HighestPfn,
                                              BoundaryPfn,
                                              SizeInPages,
                                              CallingAddress);

        if ((BaseAddress != NULL) || (i == 0)) {
            break;
        }

        //
        // Attempt to move pages to the standby list.
        //

        MiEmptyAllWorkingSets ();
        MiFlushAllPages();

        KeDelayExecutionThread (KernelMode,
                                FALSE,
                                (PLARGE_INTEGER)&MmHalfSecond);

        i -= 1;
    }
    InterlockedDecrement (&MiDelayPageFaults);
    return BaseAddress;
}

PFN_NUMBER MiLastCallLowPage;
PFN_NUMBER MiLastCallHighPage;
ULONG MiLastCallColor;


PMDL
MmAllocatePagesForMdl (
    IN PHYSICAL_ADDRESS LowAddress,
    IN PHYSICAL_ADDRESS HighAddress,
    IN PHYSICAL_ADDRESS SkipBytes,
    IN SIZE_T TotalBytes
    )

/*++

Routine Description:

    This routine searches the PFN database for free, zeroed or standby pages
    to satisfy the request.  This does not map the pages - it just allocates
    them and puts them into an MDL.  It is expected that our caller will
    map the MDL as needed.

    NOTE: this routine may return an MDL mapping a smaller number of bytes
    than the amount requested.  It is the caller's responsibility to check the
    MDL upon return for the size actually allocated.

    These pages comprise physical non-paged memory and are zero-filled.

    This routine is designed to be used by an AGP driver to obtain physical
    memory in a specified range since hardware may provide substantial
    performance wins depending on where the backing memory is allocated.

Arguments:

    LowAddress - Supplies the low physical address of the first range that
                 the allocated pages can come from.

    HighAddress - Supplies the high physical address of the first range that
                  the allocated pages can come from.

    SkipBytes - Number of bytes to skip (from the Low Address) to get to the
                next physical address range that allocated pages can come from.

    TotalBytes - Supplies the number of bytes to allocate.

Return Value:

    MDL - An MDL mapping a range of pages in the specified range.
          This may map less memory than the caller requested if the full amount
          is not currently available.

    NULL - No pages in the specified range OR not enough virtually contiguous
           nonpaged pool for the MDL is available at this time.

Environment:

    Kernel mode, IRQL of APC_LEVEL or below.

--*/

{
    PMDL MemoryDescriptorList;
    PMDL MemoryDescriptorList2;
    PMMPFN Pfn1;
    PMMPFN PfnNextColored;
    PMMPFN PfnNextFlink;
    PMMPFN PfnLastColored;
    KIRQL OldIrql;
    PFN_NUMBER start;
    PFN_NUMBER count;
    PFN_NUMBER Page;
    PFN_NUMBER LastPage;
    PFN_NUMBER found;
    PFN_NUMBER BasePage;
    PFN_NUMBER LowPage;
    PFN_NUMBER HighPage;
    PFN_NUMBER SizeInPages;
    PFN_NUMBER MdlPageSpan;
    PFN_NUMBER SkipPages;
    PFN_NUMBER MaxPages;
    PPFN_NUMBER MdlPage;
    PPFN_NUMBER LastMdlPage;
    ULONG Color;
    PMMCOLOR_TABLES ColorHead;
    MMLISTS MemoryList;
    PPFN_NUMBER FirstMdlPageToZero;
    PFN_NUMBER LowPage1;
    PFN_NUMBER HighPage1;
    LOGICAL PagePlacementOk;
    PFN_NUMBER PageNextColored;
    PFN_NUMBER PageNextFlink;
    PFN_NUMBER PageLastColored;
    PMMPFNLIST ListHead;
    PPFN_NUMBER ColorAnchorsHead;
    PPFN_NUMBER ColorAnchor;
    ULONG FullAnchorCount;
#if DBG
    ULONG FinishedCount;
#endif

    ASSERT (KeGetCurrentIrql() <= APC_LEVEL);

    //
    // The skip increment must be a page-size multiple.
    //

    if (BYTE_OFFSET(SkipBytes.LowPart)) {
        return (PMDL)0;
    }

    MmLockPagableSectionByHandle (ExPageLockHandle);

    LowPage = (PFN_NUMBER)(LowAddress.QuadPart >> PAGE_SHIFT);
    HighPage = (PFN_NUMBER)(HighAddress.QuadPart >> PAGE_SHIFT);

    //
    // Maximum allocation size is constrained by the MDL ByteCount field.
    //

    if (TotalBytes > (SIZE_T)((ULONG)(MAXULONG - PAGE_SIZE))) {
        TotalBytes = (SIZE_T)((ULONG)(MAXULONG - PAGE_SIZE));
    }

    SizeInPages = (PFN_NUMBER)ADDRESS_AND_SIZE_TO_SPAN_PAGES(0, TotalBytes);

    SkipPages = (PFN_NUMBER)(SkipBytes.QuadPart >> PAGE_SHIFT);

    BasePage = LowPage;

    LOCK_PFN (OldIrql);

    MaxPages = MI_NONPAGABLE_MEMORY_AVAILABLE() - 1024;

    if ((SPFN_NUMBER)MaxPages <= 0) {
        SizeInPages = 0;
    }
    else if (SizeInPages > MaxPages) {
        SizeInPages = MaxPages;
    }

    if (SizeInPages == 0) {
        UNLOCK_PFN (OldIrql);
        MmUnlockPagableImageSection (ExPageLockHandle);
        return (PMDL)0;
    }

    UNLOCK_PFN (OldIrql);

#if DBG
    if (SizeInPages < (PFN_NUMBER)ADDRESS_AND_SIZE_TO_SPAN_PAGES(0, TotalBytes)) {
        if (MiPrintAwe != 0) {
            DbgPrint("MmAllocatePagesForMdl1: unable to get %p pages, trying for %p instead\n",
                ADDRESS_AND_SIZE_TO_SPAN_PAGES(0, TotalBytes),
                SizeInPages);
        }
    }
#endif

    //
    // Allocate an MDL to return the pages in.
    //

    do {
        MemoryDescriptorList = MmCreateMdl ((PMDL)0,
                                            (PVOID)0,
                                            SizeInPages << PAGE_SHIFT);
    
        if (MemoryDescriptorList != (PMDL)0) {
            break;
        }
        SizeInPages -= (SizeInPages >> 4);
    } while (SizeInPages != 0);

    if (MemoryDescriptorList == (PMDL)0) {
        MmUnlockPagableImageSection (ExPageLockHandle);
        return (PMDL)0;
    }

    //
    // Allocate a list of colored anchors.
    //

    ColorAnchorsHead = (PPFN_NUMBER) ExAllocatePoolWithTag (NonPagedPool,
                                                            MmSecondaryColors * sizeof (PFN_NUMBER),
                                                            'ldmM');

    if (ColorAnchorsHead == NULL) {
        MmUnlockPagableImageSection (ExPageLockHandle);
        ExFreePool (MemoryDescriptorList);
        return (PMDL)0;
    }

    MdlPageSpan = SizeInPages;

    //
    // Recalculate as the PFN lock was dropped.
    //

    start = 0;
    found = 0;

    MdlPage = (PPFN_NUMBER)(MemoryDescriptorList + 1);

    ExAcquireFastMutex (&MmDynamicMemoryMutex);

    LOCK_PFN (OldIrql);

    MaxPages = MI_NONPAGABLE_MEMORY_AVAILABLE() - 1024;

    if ((SPFN_NUMBER)MaxPages <= 0) {
        SizeInPages = 0;
    }
    else if (SizeInPages > MaxPages) {
        SizeInPages = MaxPages;
    }

    if (SizeInPages == 0) {
        UNLOCK_PFN (OldIrql);
        ExReleaseFastMutex (&MmDynamicMemoryMutex);
        MmUnlockPagableImageSection (ExPageLockHandle);
        ExFreePool (MemoryDescriptorList);
        ExFreePool (ColorAnchorsHead);
        return (PMDL)0;
    }

    //
    // Ensure there is enough commit prior to allocating the pages as this
    // is not a nonpaged pool allocation but rather a dynamic MDL allocation.
    //

    if (MiChargeCommitmentCantExpand (SizeInPages, FALSE) == FALSE) {
        UNLOCK_PFN (OldIrql);
        ExReleaseFastMutex (&MmDynamicMemoryMutex);
        MmUnlockPagableImageSection (ExPageLockHandle);
        ExFreePool (MemoryDescriptorList);
        ExFreePool (ColorAnchorsHead);
        return (PMDL)0;
    }

    MM_TRACK_COMMIT (MM_DBG_COMMIT_MDL_PAGES, SizeInPages);

    if ((MiLastCallLowPage != LowPage) || (MiLastCallHighPage != HighPage)) {
        MiLastCallColor = 0;
    }

    MiLastCallLowPage = LowPage;
    MiLastCallHighPage = HighPage;

    FirstMdlPageToZero = MdlPage;

    do {
        //
        // Grab all zeroed (and then free) pages first directly from the
        // colored lists to avoid multiple walks down these singly linked lists.
        // Then snatch transition pages as needed.  In addition to optimizing
        // the speed of the removals this also avoids cannibalizing the page
        // cache unless it's absolutely needed.
        //

        for (MemoryList = ZeroedPageList; MemoryList <= FreePageList; MemoryList += 1) {

            ListHead = MmPageLocationList[MemoryList];

            FullAnchorCount = 0;

            for (Color = 0; Color < MmSecondaryColors; Color += 1) {
                ColorAnchorsHead[Color] = MM_EMPTY_LIST;
            }

            Color = MiLastCallColor;
            ASSERT (Color < MmSecondaryColors);

            do {

                ColorHead = &MmFreePagesByColor[MemoryList][Color];
                ColorAnchor = &ColorAnchorsHead[Color];
    
                Color += 1;
                if (Color >= MmSecondaryColors) {
                    Color = 0;
                }

                if (*ColorAnchor == (MM_EMPTY_LIST - 1)) {

                    //
                    // This colored list has already been completely searched.
                    //

                    continue;
                }

                if (ColorHead->Flink == MM_EMPTY_LIST) {

                    //
                    // This colored list is empty.
                    //

                    FullAnchorCount += 1;
                    *ColorAnchor = (MM_EMPTY_LIST - 1);
                    continue;
                }

                while (ColorHead->Flink != MM_EMPTY_LIST) {
    
                    Page = ColorHead->Flink;
        
                    Pfn1 = MI_PFN_ELEMENT(Page);

                    ASSERT ((MMLISTS)Pfn1->u3.e1.PageLocation == MemoryList);
    
                    //
                    // See if the page is within the caller's page constraints.
                    //

                    PagePlacementOk = FALSE;

                    LowPage1 = LowPage;
                    HighPage1 = HighPage;

                    do {
                        if ((Page >= LowPage1) && (Page <= HighPage1)) {
                            PagePlacementOk = TRUE;
                            break;
                        }

                        if (SkipPages == 0) {
                            break;
                        }

                        LowPage1 += SkipPages;
                        HighPage1 += SkipPages;

                        if (LowPage1 > MmHighestPhysicalPage) {
                            break;
                        }
                        if (HighPage1 > MmHighestPhysicalPage) {
                            HighPage1 = MmHighestPhysicalPage;
                        }
                    } while (TRUE);
                
                    // 
                    // The Flink and Blink must be nonzero here for the page
                    // to be on the listhead.  Only code that scans the
                    // MmPhysicalMemoryBlock has to check for the zero case.
                    //

                    ASSERT (Pfn1->u1.Flink != 0);
                    ASSERT (Pfn1->u2.Blink != 0);

                    if (PagePlacementOk == FALSE) {

                        //
                        // Put page on end of list and if first time, save pfn.
                        //
    
                        if (*ColorAnchor == MM_EMPTY_LIST) {
                            *ColorAnchor = Page;
                        }
                        else if (Page == *ColorAnchor) {

                            //
                            // No more pages available in this colored chain.
                            //

                            FullAnchorCount += 1;
                            *ColorAnchor = (MM_EMPTY_LIST - 1);
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

                            FullAnchorCount += 1;
                            *ColorAnchor = (MM_EMPTY_LIST - 1);
                            break;
                        }

                        ASSERT (Pfn1->u1.Flink != 0);
                        ASSERT (Pfn1->u1.Flink != MM_EMPTY_LIST);
                        ASSERT (Pfn1->PteFrame != MI_MAGIC_AWE_PTEFRAME);

                        PfnNextColored = MI_PFN_ELEMENT(PageNextColored);
                        ASSERT ((MMLISTS)PfnNextColored->u3.e1.PageLocation == MemoryList);
                        ASSERT (PfnNextColored->PteFrame != MI_MAGIC_AWE_PTEFRAME);

                        //
                        // Adjust the free page list so Page
                        // follows PageNextFlink.
                        //

                        PageNextFlink = Pfn1->u1.Flink;
                        PfnNextFlink = MI_PFN_ELEMENT(PageNextFlink);

                        ASSERT ((MMLISTS)PfnNextFlink->u3.e1.PageLocation == MemoryList);
                        ASSERT (PfnNextFlink->PteFrame != MI_MAGIC_AWE_PTEFRAME);

                        PfnLastColored = ColorHead->Blink;
                        ASSERT (PfnLastColored != (PMMPFN)MM_EMPTY_LIST);
                        ASSERT (PfnLastColored->OriginalPte.u.Long == MM_EMPTY_LIST);
                        ASSERT (PfnLastColored->PteFrame != MI_MAGIC_AWE_PTEFRAME);
                        ASSERT (PfnLastColored->u2.Blink != MM_EMPTY_LIST);

                        ASSERT ((MMLISTS)PfnLastColored->u3.e1.PageLocation == MemoryList);
                        PageLastColored = PfnLastColored - MmPfnDatabase;

                        if (ListHead->Flink == Page) {

                            ASSERT (Pfn1->u2.Blink == MM_EMPTY_LIST);
                            ASSERT (ListHead->Blink != Page);

                            ListHead->Flink = PageNextFlink;

                            PfnNextFlink->u2.Blink = MM_EMPTY_LIST;
                        }
                        else {

                            ASSERT (Pfn1->u2.Blink != MM_EMPTY_LIST);
                            ASSERT ((MMLISTS)(MI_PFN_ELEMENT((MI_PFN_ELEMENT(Pfn1->u2.Blink)->u1.Flink)))->PteFrame != MI_MAGIC_AWE_PTEFRAME);
                            ASSERT ((MMLISTS)(MI_PFN_ELEMENT((MI_PFN_ELEMENT(Pfn1->u2.Blink)->u1.Flink)))->u3.e1.PageLocation == MemoryList);

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
                            ASSERT (MI_PFN_ELEMENT(PfnLastColored->u1.Flink)->PteFrame != MI_MAGIC_AWE_PTEFRAME);
                            ASSERT ((MMLISTS)(MI_PFN_ELEMENT(PfnLastColored->u1.Flink)->u3.e1.PageLocation) == MemoryList);
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
    
                    found += 1;
                    ASSERT (Pfn1->u3.e1.ReadInProgress == 0);
                    MiUnlinkFreeOrZeroedPage (Page);
                    Pfn1->u3.e1.PageColor = 0;
    
                    Pfn1->u3.e2.ReferenceCount = 1;
                    Pfn1->u2.ShareCount = 1;
                    MI_SET_PFN_DELETED(Pfn1);
                    Pfn1->OriginalPte.u.Long = MM_DEMAND_ZERO_WRITE_PTE;
#if DBG
                    Pfn1->PteFrame = MI_MAGIC_AWE_PTEFRAME;
#endif
                    Pfn1->u3.e1.PageLocation = ActiveAndValid;
    
                    Pfn1->u3.e1.StartOfAllocation = 1;
                    Pfn1->u3.e1.EndOfAllocation = 1;
                    Pfn1->u3.e1.VerifierAllocation = 0;
                    Pfn1->u3.e1.LargeSessionAllocation = 0;
    
                    *MdlPage = Page;
                    MdlPage += 1;
    
                    if (found == SizeInPages) {
    
                        //
                        // All the pages requested are available.
                        //

                        if (MemoryList == ZeroedPageList) {
                            FirstMdlPageToZero = MdlPage;
                            MiLastCallColor = Color;
                        }
    
#if DBG
                        FinishedCount = 0;
                        for (Color = 0; Color < MmSecondaryColors; Color += 1) {
                            if (ColorAnchorsHead[Color] == (MM_EMPTY_LIST - 1)) {
                                FinishedCount += 1;
                            }
                        }
                        ASSERT (FinishedCount == FullAnchorCount);
#endif

                        goto pass2_done;
                    }
    
                    //
                    // March on to the next colored chain so the overall
                    // allocation round-robins the page colors.
                    //

                    break;
                }

            } while (FullAnchorCount != MmSecondaryColors);

#if DBG
            FinishedCount = 0;
            for (Color = 0; Color < MmSecondaryColors; Color += 1) {
                if (ColorAnchorsHead[Color] == (MM_EMPTY_LIST - 1)) {
                    FinishedCount += 1;
                }
            }
            ASSERT (FinishedCount == FullAnchorCount);
#endif

            if (MemoryList == ZeroedPageList) {
                FirstMdlPageToZero = MdlPage;
            }

            MiLastCallColor = 0;
        }

        start = 0;

        do {

            count = MmPhysicalMemoryBlock->Run[start].PageCount;
            Page = MmPhysicalMemoryBlock->Run[start].BasePage;

            if (count != 0) {

                //
                // Close the gaps, then examine the range for a fit.
                //

                LastPage = Page + count;

                if (LastPage - 1 > HighPage) {
                    LastPage = HighPage + 1;
                }
            
                if (Page < LowPage) {
                    Page = LowPage;
                }

                if ((Page < LastPage) &&
                    (Page >= MmPhysicalMemoryBlock->Run[start].BasePage) &&
                    (LastPage <= MmPhysicalMemoryBlock->Run[start].BasePage +
                        MmPhysicalMemoryBlock->Run[start].PageCount)) {

                    Pfn1 = MI_PFN_ELEMENT (Page);
                    do {
    
                        if (Pfn1->u3.e1.PageLocation == StandbyPageList) {
    
                            if ((Pfn1->u1.Flink != 0) &&
                                (Pfn1->u2.Blink != 0) &&
                                (Pfn1->u3.e2.ReferenceCount == 0)) {
    
                                ASSERT (Pfn1->u3.e1.ReadInProgress == 0);

                                found += 1;
    
                                //
                                // This page is in the desired range - grab it.
                                //
    
                                MiUnlinkPageFromList (Pfn1);
                                MiRestoreTransitionPte (Page);
    
                                Pfn1->u3.e1.PageColor = 0;
    
                                Pfn1->u3.e2.ReferenceCount = 1;
                                Pfn1->u2.ShareCount = 1;
                                MI_SET_PFN_DELETED(Pfn1);
                                Pfn1->OriginalPte.u.Long = MM_DEMAND_ZERO_WRITE_PTE;
#if DBG
                                Pfn1->PteFrame = MI_MAGIC_AWE_PTEFRAME;
#endif
                                Pfn1->u3.e1.PageLocation = ActiveAndValid;
    
                                Pfn1->u3.e1.StartOfAllocation = 1;
                                Pfn1->u3.e1.EndOfAllocation = 1;
                                Pfn1->u3.e1.VerifierAllocation = 0;
                                Pfn1->u3.e1.LargeSessionAllocation = 0;
    
                                *MdlPage = Page;
                                MdlPage += 1;
    
                                if (found == SizeInPages) {

                                    //
                                    // All the pages requested are available.
                                    //
    
                                    goto pass2_done;
                                }
                            }
                        }
                        Page += 1;
                        Pfn1 += 1;
    
                    } while (Page < LastPage);
                }
            }
            start += 1;
        } while (start != MmPhysicalMemoryBlock->NumberOfRuns);

        if (SkipPages == 0) {
            break;
        }
        LowPage += SkipPages;
        HighPage += SkipPages;
        if (LowPage > MmHighestPhysicalPage) {
            break;
        }
        if (HighPage > MmHighestPhysicalPage) {
            HighPage = MmHighestPhysicalPage;
        }
    } while (1);

pass2_done:

    MmMdlPagesAllocated += found;

    MmResidentAvailablePages -= found;
    MM_BUMP_COUNTER(34, found);

    UNLOCK_PFN (OldIrql);

    ExReleaseFastMutex (&MmDynamicMemoryMutex);
    MmUnlockPagableImageSection (ExPageLockHandle);

    ExFreePool (ColorAnchorsHead);

    if (found != SizeInPages) {
        ASSERT (found < SizeInPages);
        MiReturnCommitment (SizeInPages - found);
        MM_TRACK_COMMIT (MM_DBG_COMMIT_MDL_PAGES, 0 - (SizeInPages - found));
    }

    if (found == 0) {
        ExFreePool (MemoryDescriptorList);
        return (PMDL)0;
    }

    MemoryDescriptorList->ByteCount = (ULONG)(found << PAGE_SHIFT);

    if (found != SizeInPages) {
        *MdlPage = MM_EMPTY_LIST;
    }

    //
    // If the number of pages allocated was substantially less than the
    // initial request amount, attempt to allocate a smaller MDL to save
    // pool.
    //

    if ((MdlPageSpan - found) > ((4 * PAGE_SIZE) / sizeof (PFN_NUMBER))) {
        MemoryDescriptorList2 = MmCreateMdl ((PMDL)0,
                                             (PVOID)0,
                                             found << PAGE_SHIFT);
    
        if (MemoryDescriptorList2 != (PMDL)0) {
            RtlMoveMemory ((PVOID)(MemoryDescriptorList2 + 1),
                           (PVOID)(MemoryDescriptorList + 1),
                           found * sizeof (PFN_NUMBER));
            FirstMdlPageToZero = (PPFN_NUMBER)(MemoryDescriptorList2 + 1) +
                                 (FirstMdlPageToZero -
                                    (PPFN_NUMBER)(MemoryDescriptorList + 1));
            ExFreePool (MemoryDescriptorList);
            MemoryDescriptorList = MemoryDescriptorList2;
        }
    }

    MdlPage = (PPFN_NUMBER)(MemoryDescriptorList + 1);
    LastMdlPage = MdlPage + found;

#if DBG
    //
    // Ensure all pages are within the caller's page constraints.
    //

    LowPage = (PFN_NUMBER)(LowAddress.QuadPart >> PAGE_SHIFT);
    HighPage = (PFN_NUMBER)(HighAddress.QuadPart >> PAGE_SHIFT);

    while (MdlPage < FirstMdlPageToZero) {
        Page = *MdlPage;
        PagePlacementOk = FALSE;
        LowPage1 = LowPage;
        HighPage1 = HighPage;

        do {
            if ((Page >= LowPage1) && (Page <= HighPage1)) {
                PagePlacementOk = TRUE;
                break;
            }

            if (SkipPages == 0) {
                break;
            }

            LowPage1 += SkipPages;
            HighPage1 += SkipPages;

            if (LowPage1 > MmHighestPhysicalPage) {
                break;
            }
            if (HighPage1 > MmHighestPhysicalPage) {
                HighPage1 = MmHighestPhysicalPage;
            }
        } while (TRUE);

        ASSERT (PagePlacementOk == TRUE);
        Pfn1 = MI_PFN_ELEMENT(*MdlPage);
        ASSERT (Pfn1->PteFrame == MI_MAGIC_AWE_PTEFRAME);
        MdlPage += 1;
    }
#endif

    while (FirstMdlPageToZero < LastMdlPage) {

#if DBG
        //
        // Ensure all pages are within the caller's page constraints.
        //

        Page = *FirstMdlPageToZero;

        PagePlacementOk = FALSE;
        LowPage1 = LowPage;
        HighPage1 = HighPage;

        do {
            if ((Page >= LowPage1) && (Page <= HighPage1)) {
                PagePlacementOk = TRUE;
                break;
            }

            if (SkipPages == 0) {
                break;
            }

            LowPage1 += SkipPages;
            HighPage1 += SkipPages;

            if (LowPage1 > MmHighestPhysicalPage) {
                break;
            }
            if (HighPage1 > MmHighestPhysicalPage) {
                HighPage1 = MmHighestPhysicalPage;
            }
        } while (TRUE);

        ASSERT (PagePlacementOk == TRUE);
        Pfn1 = MI_PFN_ELEMENT(*FirstMdlPageToZero);
        ASSERT (Pfn1->PteFrame == MI_MAGIC_AWE_PTEFRAME);
#endif
        MiZeroPhysicalPage (*FirstMdlPageToZero, 0);
        FirstMdlPageToZero += 1;
    }

    return MemoryDescriptorList;
}


VOID
MmFreePagesFromMdl (
    IN PMDL MemoryDescriptorList
    )

/*++

Routine Description:

    This routine walks the argument MDL freeing each physical page back to
    the PFN database.  This is designed to free pages acquired via
    MmAllocatePagesForMdl only.

Arguments:

    MemoryDescriptorList - Supplies an MDL which contains the pages to be freed.

Return Value:

    None.

Environment:

    Kernel mode, IRQL of APC_LEVEL or below.

--*/
{
    PMMPFN Pfn1;
    KIRQL OldIrql;
    PVOID StartingAddress;
    PVOID AlignedVa;
    PPFN_NUMBER Page;
    PFN_NUMBER NumberOfPages;
    PFN_NUMBER PagesFreed;

    ASSERT (KeGetCurrentIrql() <= APC_LEVEL);

    PagesFreed = 0;

    MmLockPagableSectionByHandle (ExPageLockHandle);

    Page = (PPFN_NUMBER)(MemoryDescriptorList + 1);

    ASSERT ((MemoryDescriptorList->MdlFlags & (MDL_IO_SPACE | MDL_PHYSICAL_VIEW)) == 0);

    ASSERT (((ULONG_PTR)MemoryDescriptorList->StartVa & (PAGE_SIZE - 1)) == 0);
    AlignedVa = (PVOID)MemoryDescriptorList->StartVa;

    StartingAddress = (PVOID)((PCHAR)AlignedVa +
                    MemoryDescriptorList->ByteOffset);

    NumberOfPages = ADDRESS_AND_SIZE_TO_SPAN_PAGES(StartingAddress,
                                              MemoryDescriptorList->ByteCount);

    MI_MAKING_MULTIPLE_PTES_INVALID (TRUE);

    LOCK_PFN (OldIrql);

    do {

        if (*Page == MM_EMPTY_LIST) {

            //
            // There are no more locked pages.
            //

            break;
        }

        ASSERT (*Page <= MmHighestPhysicalPage);

        Pfn1 = MI_PFN_ELEMENT (*Page);
        ASSERT (Pfn1->u2.ShareCount == 1);
        ASSERT (MI_IS_PFN_DELETED (Pfn1) == TRUE);
        ASSERT (MI_PFN_IS_AWE (Pfn1) == TRUE);
        ASSERT (Pfn1->PteFrame == MI_MAGIC_AWE_PTEFRAME);

        Pfn1->u3.e1.StartOfAllocation = 0;
        Pfn1->u3.e1.EndOfAllocation = 0;
        Pfn1->u2.ShareCount = 0;
#if DBG
        Pfn1->PteFrame -= 1;
        Pfn1->u3.e1.PageLocation = StandbyPageList;
#endif

        MiDecrementReferenceCount (*Page);

        PagesFreed += 1;

        StartingAddress = (PVOID)((PCHAR)StartingAddress + PAGE_SIZE);

        *Page++ = MM_EMPTY_LIST;
        NumberOfPages -= 1;

    } while (NumberOfPages != 0);

    MmMdlPagesAllocated -= PagesFreed;

    MmResidentAvailablePages += PagesFreed;
    MM_BUMP_COUNTER(35, PagesFreed);

    UNLOCK_PFN (OldIrql);

    MmUnlockPagableImageSection (ExPageLockHandle);

    MiReturnCommitment (PagesFreed);
    MM_TRACK_COMMIT (MM_DBG_COMMIT_RETURN_MDL_PAGES, PagesFreed);
}


NTSTATUS
MmMapUserAddressesToPage (
    IN PVOID BaseAddress,
    IN SIZE_T NumberOfBytes,
    IN PVOID PageAddress
    )

/*++

Routine Description:

    This function maps a range of addresses in a physical memory VAD to the
    specified page address.  This is typically used by a driver to nicely
    remove an application's access to things like video memory when the
    application is not responding to requests to relinquish it.

    Note the entire range must be currently mapped (ie, all the PTEs must
    be valid) by the caller.

Arguments:

    BaseAddress - Supplies the base virtual address where the physical
                  address is mapped.

    NumberOfBytes - Supplies the number of bytes to remap to the new address.

    PageAddress - Supplies the virtual address of the page this is remapped to.
                  This must be nonpaged memory.

Return Value:

    Various NTSTATUS codes.

Environment:

    Kernel mode, IRQL of APC_LEVEL or below.

--*/

{
    PMMVAD Vad;
    PMMPTE PointerPte;
    MMPTE PteContents;
    PMMPTE LastPte;
    PEPROCESS Process;
    NTSTATUS Status;
    PVOID EndingAddress;
    PFN_NUMBER PageFrameNumber;
    SIZE_T NumberOfPtes;
    PHYSICAL_ADDRESS PhysicalAddress;
    KIRQL OldIrql;

    PAGED_CODE();

    if (BaseAddress > MM_HIGHEST_USER_ADDRESS) {
        return STATUS_INVALID_PARAMETER_1;
    }

    if ((ULONG_PTR)BaseAddress + NumberOfBytes > (ULONG64)MM_HIGHEST_USER_ADDRESS) {
        return STATUS_INVALID_PARAMETER_2;
    }

    Process = PsGetCurrentProcess();

    EndingAddress = (PVOID)((PCHAR)BaseAddress + NumberOfBytes - 1);

    LOCK_WS_AND_ADDRESS_SPACE (Process);

    //
    // Make sure the address space was not deleted.
    //

    if (Process->AddressSpaceDeleted != 0) {
        Status = STATUS_PROCESS_IS_TERMINATING;
        goto ErrorReturn;
    }

    Vad = (PMMVAD)MiLocateAddress (BaseAddress);

    if (Vad == NULL) {

        //
        // No virtual address descriptor located.
        //

        Status = STATUS_MEMORY_NOT_ALLOCATED;
        goto ErrorReturn;
    }

    if (NumberOfBytes == 0) {

        //
        // If the region size is specified as 0, the base address
        // must be the starting address for the region.  The entire VAD
        // will then be repointed.
        //

        if (MI_VA_TO_VPN (BaseAddress) != Vad->StartingVpn) {
            Status = STATUS_FREE_VM_NOT_AT_BASE;
            goto ErrorReturn;
        }

        BaseAddress = MI_VPN_TO_VA (Vad->StartingVpn);
        EndingAddress = MI_VPN_TO_VA_ENDING (Vad->EndingVpn);
        NumberOfBytes = (PCHAR)EndingAddress - (PCHAR)BaseAddress + 1;
    }

    //
    // Found the associated virtual address descriptor.
    //

    if (Vad->EndingVpn < MI_VA_TO_VPN (EndingAddress)) {

        //
        // The entire range to remap is not contained within a single
        // virtual address descriptor.  Return an error.
        //

        Status = STATUS_INVALID_PARAMETER_2;
        goto ErrorReturn;
    }

    if (Vad->u.VadFlags.PhysicalMapping == 0) {

        //
        // The virtual address descriptor is not a physical mapping.
        //

        Status = STATUS_INVALID_ADDRESS;
        goto ErrorReturn;
    }

    PointerPte = MiGetPteAddress (BaseAddress);
    LastPte = MiGetPteAddress (EndingAddress);
    NumberOfPtes = LastPte - PointerPte + 1;

    PhysicalAddress = MmGetPhysicalAddress (PageAddress);
    PageFrameNumber = (PFN_NUMBER)(PhysicalAddress.QuadPart >> PAGE_SHIFT);

    PteContents = *PointerPte;
    PteContents.u.Hard.PageFrameNumber = PageFrameNumber;

#if DBG

    //
    // All the PTEs must be valid or the filling will corrupt the
    // UsedPageTableCounts.
    //

    do {
        ASSERT (PointerPte->u.Hard.Valid == 1);
        PointerPte += 1;
    } while (PointerPte < LastPte);
    PointerPte = MiGetPteAddress (BaseAddress);
#endif

    //
    // Fill the PTEs and flush at the end - no race here because it doesn't
    // matter whether the user app sees the old or the new data until we
    // return (writes going to either page is acceptable prior to return
    // from this function).  There is no race with I/O and ProbeAndLockPages
    // because the PFN lock is acquired here.
    //

    LOCK_PFN (OldIrql);

#if !defined (_X86PAE_)
    MiFillMemoryPte (PointerPte,
                     NumberOfPtes * sizeof (MMPTE),
                     PteContents.u.Long);
#else

    //
    // Note that the PAE architecture must very carefully fill these PTEs.
    //

    do {
        ASSERT (PointerPte->u.Hard.Valid == 1);
        PointerPte += 1;
        (VOID)KeInterlockedSwapPte ((PHARDWARE_PTE)PointerPte,
                                    (PHARDWARE_PTE)&PteContents);
    } while (PointerPte < LastPte);
    PointerPte = MiGetPteAddress (BaseAddress);

#endif

    if (NumberOfPtes == 1) {

        (VOID)KeFlushSingleTb (BaseAddress,
                               TRUE,
                               TRUE,
                               (PHARDWARE_PTE)PointerPte,
                               PteContents.u.Flush);
    }
    else {
        KeFlushEntireTb (TRUE, TRUE);
    }

    UNLOCK_PFN (OldIrql);

    Status = STATUS_SUCCESS;

ErrorReturn:

    UNLOCK_WS_AND_ADDRESS_SPACE (Process);

    return Status;
}



VOID
MmFreeContiguousMemory (
    IN PVOID BaseAddress
    )

/*++

Routine Description:

    This function deallocates a range of physically contiguous non-paged
    pool which was allocated with the MmAllocateContiguousMemory function.

Arguments:

    BaseAddress - Supplies the base virtual address where the physical
                  address was previously mapped.

Return Value:

    None.

Environment:

    Kernel mode, IRQL of APC_LEVEL or below.

--*/

{
    PAGED_CODE();

#if defined (_X86PAE_)
    if (MiNoLowMemory == TRUE) {
        if (MiFreeLowMemory (BaseAddress, 'tnoC') == TRUE) {
            return;
        }
    }
#endif

    ExFreePool (BaseAddress);
}


VOID
MmFreeContiguousMemorySpecifyCache (
    IN PVOID BaseAddress,
    IN SIZE_T NumberOfBytes,
    IN MEMORY_CACHING_TYPE CacheType
    )

/*++

Routine Description:

    This function deallocates a range of noncached memory in
    the non-paged portion of the system address space.

Arguments:

    BaseAddress - Supplies the base virtual address where the noncached

    NumberOfBytes - Supplies the number of bytes allocated to the request.
                    This must be the same number that was obtained with
                    the MmAllocateContiguousMemorySpecifyCache call.

    CacheType - Supplies the cachetype used when the caller made the
                MmAllocateContiguousMemorySpecifyCache call.

Return Value:

    None.

Environment:

    Kernel mode, IRQL of APC_LEVEL or below.

--*/

{
    PVOID PoolAddress;
    PMMPTE PointerPte;

    PAGED_CODE();

    if (CacheType != MmCached) {

        //
        // The caller was using an alternate mapping - free these PTEs too.
        //

        PointerPte = MiGetPteAddress(BaseAddress);

        PointerPte += ((NumberOfBytes + PAGE_SIZE - 1) >> PAGE_SHIFT);
        PoolAddress = (PVOID)(ULONG_PTR)PointerPte->u.Long;

        PointerPte += 1;
        ASSERT (NumberOfBytes == PointerPte->u.Long);

        NumberOfBytes += (2 * PAGE_SIZE);
        MmUnmapIoSpace (BaseAddress, NumberOfBytes);
        BaseAddress = PoolAddress;
    }
    else {
        ASSERT (BaseAddress < MmNonPagedSystemStart ||
                BaseAddress >= (PVOID)((PCHAR)MmNonPagedSystemStart + (MmNumberOfSystemPtes << PAGE_SHIFT)));
    }

#if defined (_X86PAE_)
    if (MiNoLowMemory == TRUE) {
        if (MiFreeLowMemory (BaseAddress, 'tnoC') == TRUE) {
            return;
        }
    }
#endif

    ExFreePool (BaseAddress);
}


PHYSICAL_ADDRESS
MmGetPhysicalAddress (
     IN PVOID BaseAddress
     )

/*++

Routine Description:

    This function returns the corresponding physical address for a
    valid virtual address.

Arguments:

    BaseAddress - Supplies the virtual address for which to return the
                  physical address.

Return Value:

    Returns the corresponding physical address.

Environment:

    Kernel mode.  Any IRQL level.

--*/

{
    PMMPTE PointerPte;
    PHYSICAL_ADDRESS PhysicalAddress;

    if (MI_IS_PHYSICAL_ADDRESS(BaseAddress)) {
        PhysicalAddress.QuadPart = MI_CONVERT_PHYSICAL_TO_PFN (BaseAddress);
    } else {

        PointerPte = MiGetPteAddress(BaseAddress);

        if (PointerPte->u.Hard.Valid == 0) {
            KdPrint(("MM:MmGetPhysicalAddressFailed base address was %lx",
                      BaseAddress));
            ZERO_LARGE (PhysicalAddress);
            return PhysicalAddress;
        }
        PhysicalAddress.QuadPart = MI_GET_PAGE_FRAME_FROM_PTE (PointerPte);
    }

    PhysicalAddress.QuadPart = PhysicalAddress.QuadPart << PAGE_SHIFT;
    PhysicalAddress.LowPart += BYTE_OFFSET(BaseAddress);

    return PhysicalAddress;
}

PVOID
MmGetVirtualForPhysical (
    IN PHYSICAL_ADDRESS PhysicalAddress
     )

/*++

Routine Description:

    This function returns the corresponding virtual address for a physical
    address whose primary virtual address is in system space.

Arguments:

    PhysicalAddress - Supplies the physical address for which to return the
                  virtual address.

Return Value:

    Returns the corresponding virtual address.

Environment:

    Kernel mode.  Any IRQL level.

--*/

{
    PFN_NUMBER PageFrameIndex;
    PMMPFN Pfn;

    PageFrameIndex = (PFN_NUMBER)(PhysicalAddress.QuadPart >> PAGE_SHIFT);

    Pfn = MI_PFN_ELEMENT (PageFrameIndex);

    return (PVOID)((PCHAR)MiGetVirtualAddressMappedByPte (Pfn->PteAddress) +
                    BYTE_OFFSET (PhysicalAddress.LowPart));
}

PVOID
MmAllocateNonCachedMemory (
    IN SIZE_T NumberOfBytes
    )

/*++

Routine Description:

    This function allocates a range of noncached memory in
    the non-paged portion of the system address space.

    This routine is designed to be used by a driver's initialization
    routine to allocate a noncached block of virtual memory for
    various device specific buffers.

Arguments:

    NumberOfBytes - Supplies the number of bytes to allocate.

Return Value:

    NON-NULL - Returns a pointer (virtual address in the nonpaged portion
               of the system) to the allocated physically contiguous
               memory.

    NULL - The specified request could not be satisfied.

Environment:

    Kernel mode, IRQL of APC_LEVEL or below.

--*/

{
    PMMPTE PointerPte;
    MMPTE TempPte;
    PFN_NUMBER NumberOfPages;
    PFN_NUMBER PageFrameIndex;
    PVOID BaseAddress;
    KIRQL OldIrql;

    ASSERT (NumberOfBytes != 0);

    NumberOfPages = BYTES_TO_PAGES(NumberOfBytes);

    //
    // Obtain enough virtual space to map the pages.
    //

    PointerPte = MiReserveSystemPtes ((ULONG)NumberOfPages,
                                      SystemPteSpace,
                                      0,
                                      0,
                                      FALSE);

    if (PointerPte == NULL) {
        return NULL;
    }

    //
    // Obtain backing commitment for the pages.
    //

    if (MiChargeCommitmentCantExpand (NumberOfPages, FALSE) == FALSE) {
        MiReleaseSystemPtes (PointerPte, (ULONG)NumberOfPages, SystemPteSpace);
        return NULL;
    }

    MM_TRACK_COMMIT (MM_DBG_COMMIT_NONCACHED_PAGES, NumberOfPages);

    MmLockPagableSectionByHandle (ExPageLockHandle);

    //
    // Acquire the PFN mutex to synchronize access to the PFN database.
    //

    LOCK_PFN (OldIrql);

    //
    // Obtain enough pages to contain the allocation.
    // Check to make sure the physical pages are available.
    //

    if ((SPFN_NUMBER)NumberOfPages > MI_NONPAGABLE_MEMORY_AVAILABLE()) {
        UNLOCK_PFN (OldIrql);
        MmUnlockPagableImageSection (ExPageLockHandle);
        MiReleaseSystemPtes (PointerPte, (ULONG)NumberOfPages, SystemPteSpace);
        MiReturnCommitment (NumberOfPages);
        return NULL;
    }

#if defined(_IA64_)
    KeFlushEntireTb(FALSE, TRUE);
#endif

    MmResidentAvailablePages -= NumberOfPages;
    MM_BUMP_COUNTER(4, NumberOfPages);

    BaseAddress = (PVOID)MiGetVirtualAddressMappedByPte (PointerPte);

    do {
        ASSERT (PointerPte->u.Hard.Valid == 0);
        MiEnsureAvailablePageOrWait (NULL, NULL);
        PageFrameIndex = MiRemoveAnyPage (MI_GET_PAGE_COLOR_FROM_PTE (PointerPte));

        MI_MAKE_VALID_PTE (TempPte,
                           PageFrameIndex,
                           MM_READWRITE,
                           PointerPte);

        MI_SET_PTE_DIRTY (TempPte);
        MI_DISABLE_CACHING (TempPte);
        MI_WRITE_VALID_PTE (PointerPte, TempPte);
        MiInitializePfn (PageFrameIndex, PointerPte, 1);

        PointerPte += 1;
        NumberOfPages -= 1;
    } while (NumberOfPages != 0);

    //
    // Flush any data for this page out of the dcaches.
    //

#if !defined(_IA64_)
    //
    // Flush any data for this page out of the dcaches.
    //

    KeSweepDcache (TRUE);
#else
    MiSweepCacheMachineDependent(BaseAddress, NumberOfBytes, MmNonCached);
#endif

    UNLOCK_PFN (OldIrql);
    MmUnlockPagableImageSection (ExPageLockHandle);

    return BaseAddress;
}

VOID
MmFreeNonCachedMemory (
    IN PVOID BaseAddress,
    IN SIZE_T NumberOfBytes
    )

/*++

Routine Description:

    This function deallocates a range of noncached memory in
    the non-paged portion of the system address space.

Arguments:

    BaseAddress - Supplies the base virtual address where the noncached
                  memory resides.

    NumberOfBytes - Supplies the number of bytes allocated to the request.
                    This must be the same number that was obtained with
                    the MmAllocateNonCachedMemory call.

Return Value:

    None.

Environment:

    Kernel mode, IRQL of APC_LEVEL or below.

--*/

{

    PMMPTE PointerPte;
    PMMPFN Pfn1;
    PFN_NUMBER NumberOfPages;
    PFN_NUMBER i;
    PFN_NUMBER PageFrameIndex;
    KIRQL OldIrql;

    ASSERT (NumberOfBytes != 0);
    ASSERT (PAGE_ALIGN (BaseAddress) == BaseAddress);

    MI_MAKING_MULTIPLE_PTES_INVALID (TRUE);

    NumberOfPages = BYTES_TO_PAGES(NumberOfBytes);

    PointerPte = MiGetPteAddress (BaseAddress);

    i = NumberOfPages;

    MmLockPagableSectionByHandle (ExPageLockHandle);

    LOCK_PFN (OldIrql);

    do {

        PageFrameIndex = MI_GET_PAGE_FRAME_FROM_PTE (PointerPte);

        //
        // Mark the page for deletion when the reference count goes to zero.
        //

        Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);
        ASSERT (Pfn1->u2.ShareCount == 1);
        MiDecrementShareAndValidCount (Pfn1->PteFrame);
        MI_SET_PFN_DELETED (Pfn1);
        MiDecrementShareCountOnly (PageFrameIndex);
        PointerPte += 1;
        i -= 1;
    } while (i != 0);

    PointerPte -= NumberOfPages;

    //
    // Update the count of available resident pages.
    //

    MmResidentAvailablePages += NumberOfPages;
    MM_BUMP_COUNTER(5, NumberOfPages);

    UNLOCK_PFN (OldIrql);

    MmUnlockPagableImageSection (ExPageLockHandle);

    MiReleaseSystemPtes (PointerPte, (ULONG)NumberOfPages, SystemPteSpace);

    MiReturnCommitment (NumberOfPages);
    MM_TRACK_COMMIT (MM_DBG_COMMIT_RETURN_NONCACHED_PAGES, NumberOfPages);

    return;
}

SIZE_T
MmSizeOfMdl (
    IN PVOID Base,
    IN SIZE_T Length
    )

/*++

Routine Description:

    This function returns the number of bytes required for an MDL for a
    given buffer and size.

Arguments:

    Base - Supplies the base virtual address for the buffer.

    Length - Supplies the size of the buffer in bytes.

Return Value:

    Returns the number of bytes required to contain the MDL.

Environment:

    Kernel mode.  Any IRQL level.

--*/

{
    return( sizeof( MDL ) +
                (ADDRESS_AND_SIZE_TO_SPAN_PAGES( Base, Length ) *
                 sizeof( PFN_NUMBER ))
          );
}


PMDL
MmCreateMdl (
    IN PMDL MemoryDescriptorList OPTIONAL,
    IN PVOID Base,
    IN SIZE_T Length
    )

/*++

Routine Description:

    This function optionally allocates and initializes an MDL.

Arguments:

    MemoryDescriptorList - Optionally supplies the address of the MDL
                           to initialize.  If this address is supplied as NULL
                           an MDL is allocated from non-paged pool and
                           initialized.

    Base - Supplies the base virtual address for the buffer.

    Length - Supplies the size of the buffer in bytes.

Return Value:

    Returns the address of the initialized MDL.

Environment:

    Kernel mode, IRQL of DISPATCH_LEVEL or below.

--*/

{
    SIZE_T MdlSize;

    MdlSize = MmSizeOfMdl( Base, Length );

    if (!ARGUMENT_PRESENT( MemoryDescriptorList )) {

        //
        // The pool manager doesn't like being called with large requests
        // marked MustSucceed, so try the normal nonpaged if the
        // request is large.
        //

        if (MdlSize > POOL_BUDDY_MAX) {
            MemoryDescriptorList = (PMDL)ExAllocatePoolWithTag (
                                                     NonPagedPool,
                                                     MdlSize,
                                                     'ldmM');
            if (MemoryDescriptorList == (PMDL)0) {
                return (PMDL)0;
            }
        }
        else {
            MemoryDescriptorList = (PMDL)ExAllocatePoolWithTag (
                                                     NonPagedPoolMustSucceed,
                                                     MdlSize,
                                                     'ldmM');
        }
    }

    MmInitializeMdl (MemoryDescriptorList, Base, Length);
    return MemoryDescriptorList;
}

BOOLEAN
MmSetAddressRangeModified (
    IN PVOID Address,
    IN SIZE_T Length
    )

/*++

Routine Description:

    This routine sets the modified bit in the PFN database for the
    pages that correspond to the specified address range.

    Note that the dirty bit in the PTE is cleared by this operation.

Arguments:

    Address - Supplies the address of the start of the range.  This
              range must reside within the system cache.

    Length - Supplies the length of the range.

Return Value:

    TRUE if at least one PTE was dirty in the range, FALSE otherwise.

Environment:

    Kernel mode.  APC_LEVEL and below for pagable addresses,
                  DISPATCH_LEVEL and below for non-pagable addresses.

--*/

{
    PMMPTE PointerPte;
    PMMPTE LastPte;
    PMMPFN Pfn1;
    PMMPTE FlushPte;
    MMPTE PteContents;
    MMPTE FlushContents;
    KIRQL OldIrql;
    PVOID VaFlushList[MM_MAXIMUM_FLUSH_COUNT];
    ULONG Count;
    BOOLEAN Result;

    Count = 0;
    Result = FALSE;

    //
    // Loop on the copy on write case until the page is only
    // writable.
    //

    PointerPte = MiGetPteAddress (Address);
    LastPte = MiGetPteAddress ((PVOID)((PCHAR)Address + Length - 1));

    LOCK_PFN2 (OldIrql);

    do {

        PteContents = *PointerPte;

        if (PteContents.u.Hard.Valid == 1) {

            Pfn1 = MI_PFN_ELEMENT (PteContents.u.Hard.PageFrameNumber);
            Pfn1->u3.e1.Modified = 1;

            if ((Pfn1->OriginalPte.u.Soft.Prototype == 0) &&
                         (Pfn1->u3.e1.WriteInProgress == 0)) {
                MiReleasePageFileSpace (Pfn1->OriginalPte);
                Pfn1->OriginalPte.u.Soft.PageFileHigh = 0;
            }

#ifdef NT_UP
            //
            // On uniprocessor systems no need to flush if this processor
            // doesn't think the PTE is dirty.
            //

            if (MI_IS_PTE_DIRTY (PteContents)) {
                Result = TRUE;
#else  //NT_UP
                Result |= (BOOLEAN)(MI_IS_PTE_DIRTY (PteContents));
#endif //NT_UP
                MI_SET_PTE_CLEAN (PteContents);
                MI_WRITE_VALID_PTE_NEW_PROTECTION (PointerPte, PteContents);
                FlushContents = PteContents;
                FlushPte = PointerPte;

                //
                // Clear the write bit in the PTE so new writes can be tracked.
                //

                if (Count != MM_MAXIMUM_FLUSH_COUNT) {
                    VaFlushList[Count] = Address;
                    Count += 1;
                }
#ifdef NT_UP
            }
#endif //NT_UP
        }
        PointerPte += 1;
        Address = (PVOID)((PCHAR)Address + PAGE_SIZE);
    } while (PointerPte <= LastPte);

    if (Count != 0) {
        if (Count == 1) {

            (VOID)KeFlushSingleTb (VaFlushList[0],
                                   FALSE,
                                   TRUE,
                                   (PHARDWARE_PTE)FlushPte,
                                   FlushContents.u.Flush);

        } else if (Count != MM_MAXIMUM_FLUSH_COUNT) {

            KeFlushMultipleTb (Count,
                               &VaFlushList[0],
                               FALSE,
                               TRUE,
                               NULL,
                               *(PHARDWARE_PTE)&ZeroPte.u.Flush);

        } else {
            KeFlushEntireTb (FALSE, TRUE);
        }
    }
    UNLOCK_PFN2 (OldIrql);
    return Result;
}


PVOID
MiCheckForContiguousMemory (
    IN PVOID BaseAddress,
    IN PFN_NUMBER BaseAddressPages,
    IN PFN_NUMBER SizeInPages,
    IN PFN_NUMBER LowestPfn,
    IN PFN_NUMBER HighestPfn,
    IN PFN_NUMBER BoundaryPfn
    )

/*++

Routine Description:

    This routine checks to see if the physical memory mapped
    by the specified BaseAddress for the specified size is
    contiguous and that the first page is greater than or equal to
    the specified LowestPfn and that the last page of the physical memory is
    less than or equal to the specified HighestPfn.

Arguments:

    BaseAddress - Supplies the base address to start checking at.

    BaseAddressPages - Supplies the number of pages to scan from the
                       BaseAddress.

    SizeInPages - Supplies the number of pages in the range.

    LowestPfn - Supplies lowest PFN acceptable as a physical page.

    HighestPfn - Supplies the highest PFN acceptable as a physical page.

    BoundaryPfn - Supplies the PFN multiple the allocation must
                  not cross.  0 indicates it can cross any boundary.

Return Value:

    Returns the usable virtual address within the argument range that the
    caller should return to his caller.  NULL if there is no usable address.

Environment:

    Kernel mode, memory management internal.

--*/

{
    PMMPTE PointerPte;
    PMMPTE LastPte;
    PFN_NUMBER PreviousPage;
    PFN_NUMBER Page;
    PFN_NUMBER HighestStartPage;
    PFN_NUMBER LastPage;
    PFN_NUMBER OriginalPage;
    PFN_NUMBER OriginalLastPage;
    PVOID BoundaryAllocation;
    PFN_NUMBER BoundaryMask;
    ULONG PageCount;
    MMPTE PteContents;

    BoundaryMask = ~(BoundaryPfn - 1);

    if (LowestPfn > HighestPfn) {
        return NULL;
    }

    if (LowestPfn + SizeInPages <= LowestPfn) {
        return NULL;
    }

    if (LowestPfn + SizeInPages > HighestPfn + 1) {
        return NULL;
    }

    if (BaseAddressPages < SizeInPages) {
        return NULL;
    }

    if (MI_IS_PHYSICAL_ADDRESS (BaseAddress)) {

        OriginalPage = MI_CONVERT_PHYSICAL_TO_PFN(BaseAddress);
        OriginalLastPage = OriginalPage + BaseAddressPages;

        Page = OriginalPage;
        LastPage = OriginalLastPage;

        //
        // Close the gaps, then examine the range for a fit.
        //

        if (Page < LowestPfn) {
            Page = LowestPfn;
        }

        if (LastPage > HighestPfn + 1) {
            LastPage = HighestPfn + 1;
        }

        HighestStartPage = LastPage - SizeInPages;

        if (Page > HighestStartPage) {
            return NULL;
        }

        if (BoundaryPfn != 0) {
            do {
                if (((Page ^ (Page + SizeInPages - 1)) & BoundaryMask) == 0) {

                    //
                    // This portion of the range meets the alignment
                    // requirements.
                    //

                    break;
                }
                Page |= (BoundaryPfn - 1);
                Page += 1;
            } while (Page <= HighestStartPage);

            if (Page > HighestStartPage) {
                return NULL;
            }
            BoundaryAllocation = (PVOID)((PCHAR)BaseAddress + ((Page - OriginalPage) << PAGE_SHIFT));

            //
            // The request can be satisfied.  Since specific alignment was
            // requested, return the fit now without getting fancy.
            //

            return BoundaryAllocation;
        }

        //
        // If possible return a chunk on the end to reduce fragmentation.
        //
    
        if (LastPage == OriginalLastPage) {
            return (PVOID)((PCHAR)BaseAddress + ((BaseAddressPages - SizeInPages) << PAGE_SHIFT));
        }
    
        //
        // The end chunk did not satisfy the requirements.  The next best option
        // is to return a chunk from the beginning.  Since that's where the search
        // began, just return the current chunk.
        //

        return (PVOID)((PCHAR)BaseAddress + ((Page - OriginalPage) << PAGE_SHIFT));
    }

    //
    // Check the virtual addresses for physical contiguity.
    //

    PointerPte = MiGetPteAddress (BaseAddress);
    LastPte = PointerPte + BaseAddressPages;

    HighestStartPage = HighestPfn + 1 - SizeInPages;
    PageCount = 0;

    while (PointerPte < LastPte) {

        PteContents = *PointerPte;
        ASSERT (PteContents.u.Hard.Valid == 1);
        Page = MI_GET_PAGE_FRAME_FROM_PTE (&PteContents);

        //
        // Before starting a new run, ensure that it
        // can satisfy the location & boundary requirements (if any).
        //

        if (PageCount == 0) {

            if ((Page >= LowestPfn) && (Page <= HighestStartPage)) {

                if (BoundaryPfn == 0) {
                    PageCount += 1;
                }
                else if (((Page ^ (Page + SizeInPages - 1)) & BoundaryMask) == 0) {
                    //
                    // This run's physical address meets the alignment
                    // requirement.
                    //

                    PageCount += 1;
                }
            }

            if (PageCount == SizeInPages) {

                //
                // Success - found a single page satifying the requirements.
                //

                BaseAddress = MiGetVirtualAddressMappedByPte (PointerPte);
                return BaseAddress;
            }

            PreviousPage = Page;
            PointerPte += 1;
            continue;
        }

        if (Page != PreviousPage + 1) {

            //
            // This page is not physically contiguous.  Start over.
            //

            PageCount = 0;
            continue;
        }

        PageCount += 1;

        if (PageCount == SizeInPages) {

            //
            // Success - found a page range satifying the requirements.
            //

            BaseAddress = MiGetVirtualAddressMappedByPte (PointerPte - PageCount + 1);
            return BaseAddress;
        }

        PointerPte += 1;
    }

    return NULL;
}


VOID
MmLockPagableSectionByHandle (
    IN PVOID ImageSectionHandle
    )


/*++

Routine Description:

    This routine checks to see if the specified pages are resident in
    the process's working set and if so the reference count for the
    page is incremented.  The allows the virtual address to be accessed
    without getting a hard page fault (have to go to the disk... except
    for extremely rare case when the page table page is removed from the
    working set and migrates to the disk.

    If the virtual address is that of the system wide global "cache" the
    virtual address of the "locked" pages is always guaranteed to
    be valid.

    NOTE: This routine is not to be used for general locking of user
    addresses - use MmProbeAndLockPages.  This routine is intended for
    well behaved system code like the file system caches which allocates
    virtual addresses for mapping files AND guarantees that the mapping
    will not be modified (deleted or changed) while the pages are locked.

Arguments:

    ImageSectionHandle - Supplies the value returned by a previous call
        to MmLockPagableDataSection.  This is a pointer to the Section
        header for the image.

Return Value:

    None.

Environment:

    Kernel mode, IRQL of DISPATCH_LEVEL or below.

--*/

{
    PIMAGE_SECTION_HEADER NtSection;
    PVOID BaseAddress;
    ULONG SizeToLock;
    PMMPTE PointerPte;
    PMMPTE LastPte;
    KIRQL OldIrql;
    KIRQL OldIrqlWs;
    ULONG Collision;

    if (MI_IS_PHYSICAL_ADDRESS(ImageSectionHandle)) {

        //
        // No need to lock physical addresses.
        //

        return;
    }

    NtSection = (PIMAGE_SECTION_HEADER)ImageSectionHandle;

    BaseAddress = SECTION_BASE_ADDRESS(NtSection);

    ASSERT (!MI_IS_SYSTEM_CACHE_ADDRESS(BaseAddress));

    ASSERT (BaseAddress >= MmSystemRangeStart);

    SizeToLock = NtSection->SizeOfRawData;
    PointerPte = MiGetPteAddress(BaseAddress);
    LastPte = MiGetPteAddress((PCHAR)BaseAddress + SizeToLock - 1);

    ASSERT (SizeToLock != 0);

    //
    // The address must be within the system space.
    //

RetryLock:

    LOCK_SYSTEM_WS (OldIrqlWs);
    LOCK_PFN2 (OldIrql);

    MiMakeSystemAddressValidPfnSystemWs (&NtSection->NumberOfLinenumbers);

    //
    // The NumberOfLinenumbers field is used to store the
    // lock count.
    //
    //  Value of 0 means unlocked,
    //  Value of 1 means lock in progress by another thread.
    //  Value of 2 or more means locked.
    //
    //  If the value is 1, this thread must block until the other thread's
    //  lock operation is complete.
    //

    NtSection->NumberOfLinenumbers += 1;

    if (NtSection->NumberOfLinenumbers >= 3) {

        //
        // Already locked, increment counter and return.
        //

        UNLOCK_PFN2 (OldIrql);
        UNLOCK_SYSTEM_WS (OldIrqlWs);
        return;
    }

    if (NtSection->NumberOfLinenumbers == 2) {

        //
        // A lock is in progress.
        // Reset back to 1 and wait.
        //

        NtSection->NumberOfLinenumbers = 1;
        MmCollidedLockWait = TRUE;

        KeEnterCriticalRegion();

        //
        // The unlock IRQLs are deliberately reversed as the lock and mutex
        // are being released in reverse order.
        //

        UNLOCK_SYSTEM_WS_NO_IRQL ();
        UNLOCK_PFN_AND_THEN_WAIT (OldIrqlWs);

        KeWaitForSingleObject(&MmCollidedLockEvent,
                              WrVirtualMemory,
                              KernelMode,
                              FALSE,
                              (PLARGE_INTEGER)NULL);
        KeLeaveCriticalRegion();
        goto RetryLock;
    }

    //
    // Value was 0 when the lock was obtained.  It is now 1 indicating
    // a lock is in progress.
    //

    MiLockCode (PointerPte, LastPte, MM_LOCK_BY_REFCOUNT);

    //
    // Set lock count to 2 (it was 1 when this started) and check
    // to see if any other threads tried to lock while this was happening.
    //

    MiMakeSystemAddressValidPfnSystemWs (&NtSection->NumberOfLinenumbers);
    NtSection->NumberOfLinenumbers += 1;

    ASSERT (NtSection->NumberOfLinenumbers == 2);

    Collision = MmCollidedLockWait;
    MmCollidedLockWait = FALSE;

    UNLOCK_PFN2 (OldIrql);
    UNLOCK_SYSTEM_WS (OldIrqlWs);

    if (Collision) {

        //
        // Wake up all waiters.
        //

        KePulseEvent (&MmCollidedLockEvent, 0, FALSE);
    }

    return;
}


VOID
MiLockCode (
    IN PMMPTE FirstPte,
    IN PMMPTE LastPte,
    IN ULONG LockType
    )

/*++

Routine Description:

    This routine checks to see if the specified pages are resident in
    the process's working set and if so the reference count for the
    page is incremented.  This allows the virtual address to be accessed
    without getting a hard page fault (have to go to the disk...) except
    for the extremely rare case when the page table page is removed from the
    working set and migrates to the disk.

    If the virtual address is that of the system wide global "cache", the
    virtual address of the "locked" pages is always guaranteed to
    be valid.

    NOTE: This routine is not to be used for general locking of user
    addresses - use MmProbeAndLockPages.  This routine is intended for
    well behaved system code like the file system caches which allocates
    virtual addresses for mapping files AND guarantees that the mapping
    will not be modified (deleted or changed) while the pages are locked.

Arguments:

    FirstPte - Supplies the base address to begin locking.

    LastPte - The last PTE to lock.

    LockType - Supplies either MM_LOCK_BY_REFCOUNT or MM_LOCK_NONPAGE.
               LOCK_BY_REFCOUNT increments the reference count to keep
               the page in memory, LOCK_NONPAGE removes the page from
               the working set so it's locked just like nonpaged pool.

Return Value:

    None.

Environment:

    Kernel mode, System working set mutex and PFN LOCK held.

--*/

{
    PMMPFN Pfn1;
    PMMPTE PointerPte;
    MMPTE TempPte;
    MMPTE PteContents;
    WSLE_NUMBER WorkingSetIndex;
    WSLE_NUMBER SwapEntry;
    PFN_NUMBER PageFrameIndex;
    KIRQL OldIrql;
    LOGICAL SessionSpace;
    PMMWSL WorkingSetList;
    PMMSUPPORT Vm;
#if PFN_CONSISTENCY
    KIRQL PfnIrql;
#endif

    MM_PFN_LOCK_ASSERT();

    SessionSpace = MI_IS_SESSION_IMAGE_ADDRESS (MiGetVirtualAddressMappedByPte(FirstPte));

    if (SessionSpace == TRUE) {
        Vm = &MmSessionSpace->Vm;
        WorkingSetList = MmSessionSpace->Vm.VmWorkingSetList;
    }

    //
    // Session space is never locked by refcount.
    //

    ASSERT ((SessionSpace == FALSE) || (LockType != MM_LOCK_BY_REFCOUNT));

    ASSERT (!MI_IS_PHYSICAL_ADDRESS(MiGetVirtualAddressMappedByPte(FirstPte)));
    PointerPte = FirstPte;

    MmLockedCode += 1 + LastPte - FirstPte;

    do {

        PteContents = *PointerPte;
        ASSERT (PteContents.u.Long != ZeroKernelPte.u.Long);
        if (PteContents.u.Hard.Valid == 0) {

            if (PteContents.u.Soft.Prototype == 1) {

                //
                // Page is not in memory and it is a prototype.
                //

                MiMakeSystemAddressValidPfnSystemWs (
                        MiGetVirtualAddressMappedByPte(PointerPte));

                continue;
            }
            else if (PteContents.u.Soft.Transition == 1) {

                PageFrameIndex = MI_GET_PAGE_FRAME_FROM_TRANSITION_PTE (&PteContents);

                Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);
                if ((Pfn1->u3.e1.ReadInProgress) ||
                    (Pfn1->u3.e1.InPageError)) {

                    //
                    // Page read is ongoing, force a collided fault.
                    //

                    MiMakeSystemAddressValidPfnSystemWs (
                            MiGetVirtualAddressMappedByPte(PointerPte));

                    continue;
                }

                //
                // Paged pool is trimmed without regard to sharecounts.
                // This means a paged pool PTE can be in transition while
                // the page is still marked active.
                //

                if (Pfn1->u3.e1.PageLocation == ActiveAndValid) {

                    ASSERT (((Pfn1->PteAddress >= MiGetPteAddress(MmPagedPoolStart)) &&
                            (Pfn1->PteAddress <= MiGetPteAddress(MmPagedPoolEnd))) ||
                            ((Pfn1->PteAddress >= MiGetPteAddress(MmSpecialPoolStart)) &&
                            (Pfn1->PteAddress <= MiGetPteAddress(MmSpecialPoolEnd))));

                    //
                    // Don't increment the valid PTE count for the
                    // paged pool page.
                    //

                    ASSERT (Pfn1->u2.ShareCount != 0);
                    ASSERT (Pfn1->u3.e2.ReferenceCount != 0);
                    Pfn1->u2.ShareCount += 1;
                }
                else {

                    MiUnlinkPageFromList (Pfn1);

                    //
                    // Set the reference count and share counts to 1.  Note the
                    // reference count may be 1 already if a modified page
                    // write is underway.  The systemwide locked page charges
                    // are correct in either case and nothing needs to be done
                    // just yet.
                    //

                    Pfn1->u3.e2.ReferenceCount += 1;
                    Pfn1->u2.ShareCount = 1;
                }

                Pfn1->u3.e1.PageLocation = ActiveAndValid;

                MI_MAKE_VALID_PTE (TempPte,
                                   PageFrameIndex,
                                   Pfn1->OriginalPte.u.Soft.Protection,
                                   PointerPte);

                MI_WRITE_VALID_PTE (PointerPte, TempPte);

                //
                // Increment the reference count one for putting it the
                // working set list and one for locking it for I/O.
                //

                if (LockType == MM_LOCK_BY_REFCOUNT) {

                    //
                    // Lock the page in the working set by upping the
                    // reference count.
                    //

                    MI_ADD_LOCKED_PAGE_CHARGE (Pfn1, 34);
                    Pfn1->u3.e2.ReferenceCount += 1;
                    Pfn1->u1.Event = (PVOID)PsGetCurrentThread();

                    UNLOCK_PFN (APC_LEVEL);
                    WorkingSetIndex = MiLocateAndReserveWsle (&MmSystemCacheWs);

                    MiUpdateWsle (&WorkingSetIndex,
                                  MiGetVirtualAddressMappedByPte (PointerPte),
                                  MmSystemCacheWorkingSetList,
                                  Pfn1);

                    MI_SET_PTE_IN_WORKING_SET (PointerPte, WorkingSetIndex);

                    LOCK_PFN (OldIrql);

                } else {

                    //
                    // The wsindex field must be zero because the
                    // page is not in the system (or session) working set.
                    //

                    ASSERT (Pfn1->u1.WsIndex == 0);

                    //
                    // Adjust available pages as this page is now not in any
                    // working set, just like a non-paged pool page.  On entry
                    // this page was in transition so it was part of the
                    // available pages by definition.
                    //
    
                    MmResidentAvailablePages -= 1;
                    if (Pfn1->u3.e1.PrototypePte == 0) {
                        MmTotalSystemDriverPages -= 1;
                    }
                    MM_BUMP_COUNTER(29, 1);
                }
            } else {

                //
                // Page is not in memory.
                //

                MiMakeSystemAddressValidPfnSystemWs (
                        MiGetVirtualAddressMappedByPte(PointerPte));

                continue;
            }

        }
        else {

            //
            // This address is already in the system (or session) working set.
            //

            Pfn1 = MI_PFN_ELEMENT (PteContents.u.Hard.PageFrameNumber);

            //
            // Up the reference count so the page cannot be released.
            //

            MI_ADD_LOCKED_PAGE_CHARGE (Pfn1, 36);
            Pfn1->u3.e2.ReferenceCount += 1;

            if (LockType != MM_LOCK_BY_REFCOUNT) {

                //
                // If the page is in the system working set, remove it.
                // The system working set lock MUST be owned to check to
                // see if this page is in the working set or not.  This
                // is because the pager may have just released the PFN lock,
                // acquired the system lock and is now trying to add the
                // page to the system working set.
                //
                // If the page is in the SESSION working set, it cannot be
                // removed as all these pages are carefully accounted for.
                // Instead move it to the locked portion of the working set
                // if it is not there already.
                //

                if (Pfn1->u1.WsIndex != 0) {

                    UNLOCK_PFN (APC_LEVEL);

                    if (SessionSpace == TRUE) {

                        WorkingSetIndex = MiLocateWsle (
                                    MiGetVirtualAddressMappedByPte(PointerPte),
                                    WorkingSetList,
                                    Pfn1->u1.WsIndex);

                        if (WorkingSetIndex >= WorkingSetList->FirstDynamic) {
                
                            SwapEntry = WorkingSetList->FirstDynamic;
                
                            if (WorkingSetIndex != WorkingSetList->FirstDynamic) {
                
                                //
                                // Swap this entry with the one at first
                                // dynamic.  Note that the working set index
                                // in the PTE is updated here as well.
                                //
                
                                MiSwapWslEntries (WorkingSetIndex,
                                                  SwapEntry,
                                                  Vm);
                            }
                
                            WorkingSetList->FirstDynamic += 1;
                        }
                        else {
                            SwapEntry = WorkingSetIndex;
                        }

                        //
                        // Indicate that the page is locked.
                        //
            
                        MmSessionSpace->Wsle[SwapEntry].u1.e1.LockedInWs = 1;
                    }
                    else {
                        MiRemoveWsle (Pfn1->u1.WsIndex, MmSystemCacheWorkingSetList);
                        MiReleaseWsle (Pfn1->u1.WsIndex, &MmSystemCacheWs);

                        MI_SET_PTE_IN_WORKING_SET (PointerPte, 0);
                    }

                    LOCK_PFN (OldIrql);

                    MI_ZERO_WSINDEX (Pfn1);

                    //
                    // Adjust available pages as this page is now not in any
                    // working set, just like a non-paged pool page.
                    //
    
                    MmResidentAvailablePages -= 1;
                    MM_BUMP_COUNTER(29, 1);
                    if (Pfn1->u3.e1.PrototypePte == 0) {
                        MmTotalSystemDriverPages -= 1;
                    }
                }
                ASSERT (Pfn1->u3.e2.ReferenceCount > 1);
                MI_REMOVE_LOCKED_PAGE_CHARGE (Pfn1, 37);
                Pfn1->u3.e2.ReferenceCount -= 1;
            }
        }

        PointerPte += 1;
    } while (PointerPte <= LastPte);

    return;
}


NTSTATUS
MmGetSectionRange(
    IN PVOID AddressWithinSection,
    OUT PVOID *StartingSectionAddress,
    OUT PULONG SizeofSection
    )
{
    PLDR_DATA_TABLE_ENTRY DataTableEntry;
    ULONG i;
    PIMAGE_NT_HEADERS NtHeaders;
    PIMAGE_SECTION_HEADER NtSection;
    NTSTATUS Status;
    ULONG_PTR Rva;

    PAGED_CODE();

    //
    // Search the loaded module list for the data table entry that describes
    // the DLL that was just unloaded. It is possible that an entry is not in
    // the list if a failure occurred at a point in loading the DLL just before
    // the data table entry was generated.
    //

    Status = STATUS_NOT_FOUND;

    KeEnterCriticalRegion();
    ExAcquireResourceShared (&PsLoadedModuleResource, TRUE);

    DataTableEntry = MiLookupDataTableEntry (AddressWithinSection, TRUE);
    if (DataTableEntry) {

        Rva = (ULONG_PTR)((PUCHAR)AddressWithinSection - (ULONG_PTR)DataTableEntry->DllBase);

        NtHeaders = (PIMAGE_NT_HEADERS)RtlImageNtHeader(DataTableEntry->DllBase);

        NtSection = (PIMAGE_SECTION_HEADER)((PCHAR)NtHeaders +
                            sizeof(ULONG) +
                            sizeof(IMAGE_FILE_HEADER) +
                            NtHeaders->FileHeader.SizeOfOptionalHeader
                            );

        for (i = 0; i < NtHeaders->FileHeader.NumberOfSections; i += 1) {

            if ( Rva >= NtSection->VirtualAddress &&
                 Rva < NtSection->VirtualAddress + NtSection->SizeOfRawData ) {

                //
                // Found it
                //

                *StartingSectionAddress = (PVOID)
                    ((PCHAR) DataTableEntry->DllBase + NtSection->VirtualAddress);
                *SizeofSection = NtSection->SizeOfRawData;
                Status = STATUS_SUCCESS;
                break;
            }

            NtSection += 1;
        }
    }

    ExReleaseResource (&PsLoadedModuleResource);
    KeLeaveCriticalRegion();
    return Status;
}


PVOID
MmLockPagableDataSection(
    IN PVOID AddressWithinSection
    )

/*++

Routine Description:

    This functions locks the entire section that contains the specified
    section in memory.  This allows pagable code to be brought into
    memory and to be used as if the code was not really pagable.  This
    should not be done with a high degree of frequency.

Arguments:

    AddressWithinSection - Supplies the address of a function
        contained within a section that should be brought in and locked
        in memory.

Return Value:

    This function returns a value to be used in a subsequent call to
    MmUnlockPagableImageSection.

--*/

{
    PLDR_DATA_TABLE_ENTRY DataTableEntry;
    ULONG i;
    PIMAGE_NT_HEADERS NtHeaders;
    PIMAGE_SECTION_HEADER NtSection;
    PIMAGE_SECTION_HEADER FoundSection;
    ULONG_PTR Rva;

    PAGED_CODE();

    if (MI_IS_PHYSICAL_ADDRESS(AddressWithinSection)) {

        //
        // Physical address, just return that as the handle.
        //

        return AddressWithinSection;
    }

    //
    // Search the loaded module list for the data table entry that describes
    // the DLL that was just unloaded. It is possible that an entry is not in
    // the list if a failure occurred at a point in loading the DLL just before
    // the data table entry was generated.
    //

    FoundSection = NULL;

    KeEnterCriticalRegion();
    ExAcquireResourceShared (&PsLoadedModuleResource, TRUE);

    DataTableEntry = MiLookupDataTableEntry (AddressWithinSection, TRUE);

    Rva = (ULONG_PTR)((PUCHAR)AddressWithinSection - (ULONG_PTR)DataTableEntry->DllBase);

    NtHeaders = (PIMAGE_NT_HEADERS)RtlImageNtHeader(DataTableEntry->DllBase);

    NtSection = (PIMAGE_SECTION_HEADER)((ULONG_PTR)NtHeaders +
                        sizeof(ULONG) +
                        sizeof(IMAGE_FILE_HEADER) +
                        NtHeaders->FileHeader.SizeOfOptionalHeader
                        );

    for (i = 0; i < NtHeaders->FileHeader.NumberOfSections; i += 1) {

        if ( Rva >= NtSection->VirtualAddress &&
             Rva < NtSection->VirtualAddress + NtSection->SizeOfRawData ) {
            FoundSection = NtSection;

            if (SECTION_BASE_ADDRESS(NtSection) != ((PUCHAR)DataTableEntry->DllBase +
                            NtSection->VirtualAddress)) {

                //
                // Overwrite the PointerToRelocations field (and on Win64, the
                // PointerToLinenumbers field also) so that it contains
                // the Va of this section and NumberOfLinenumbers so it contains
                // the Lock Count for the section.
                //

                SECTION_BASE_ADDRESS(NtSection) = ((PUCHAR)DataTableEntry->DllBase +
                                        NtSection->VirtualAddress);
                NtSection->NumberOfLinenumbers = 0;
            }

            //
            // Now lock in the code
            //

#if DBG
            if (MmDebug & MM_DBG_LOCK_CODE) {
                DbgPrint("MM Lock %wZ %8s %p -> %p : %p %3ld.\n",
                        &DataTableEntry->BaseDllName,
                        NtSection->Name,
                        AddressWithinSection,
                        NtSection,
                        SECTION_BASE_ADDRESS(NtSection),
                        NtSection->NumberOfLinenumbers);
            }
#endif //DBG

            MmLockPagableSectionByHandle ((PVOID)NtSection);

            break;
        }
        NtSection += 1;
    }

    ExReleaseResource (&PsLoadedModuleResource);
    KeLeaveCriticalRegion();
    if (!FoundSection) {
        KeBugCheckEx (MEMORY_MANAGEMENT,
                      0x1234,
                      (ULONG_PTR)AddressWithinSection,
                      0,
                      0);
    }
    return (PVOID)FoundSection;
}


PLDR_DATA_TABLE_ENTRY
MiLookupDataTableEntry (
    IN PVOID AddressWithinSection,
    IN ULONG ResourceHeld
    )

/*++

Routine Description:

    This functions locates the data table entry that maps the specified address.

Arguments:

    AddressWithinSection - Supplies the address of a function contained
                           within the desired module.

    ResourceHeld - Supplies TRUE if the loaded module resource is already held,
                   FALSE if not.

Return Value:

    The address of the loaded module list data table entry that maps the
    argument address.

--*/

{
    PLDR_DATA_TABLE_ENTRY DataTableEntry;
    PLDR_DATA_TABLE_ENTRY FoundEntry = NULL;
    PLIST_ENTRY NextEntry;

    PAGED_CODE();

    //
    // Search the loaded module list for the data table entry that describes
    // the DLL that was just unloaded. It is possible that an entry is not in
    // the list if a failure occurred at a point in loading the DLL just before
    // the data table entry was generated.
    //

    if (!ResourceHeld) {
        KeEnterCriticalRegion();
        ExAcquireResourceShared (&PsLoadedModuleResource, TRUE);
    }

    NextEntry = PsLoadedModuleList.Flink;
    do {

        DataTableEntry = CONTAINING_RECORD(NextEntry,
                                           LDR_DATA_TABLE_ENTRY,
                                           InLoadOrderLinks);

        //
        // Locate the loaded module that contains this address.
        //

        if ( AddressWithinSection >= DataTableEntry->DllBase &&
             AddressWithinSection < (PVOID)((PUCHAR)DataTableEntry->DllBase+DataTableEntry->SizeOfImage) ) {

            FoundEntry = DataTableEntry;
            break;
        }

        NextEntry = NextEntry->Flink;
    } while (NextEntry != &PsLoadedModuleList);

    if (!ResourceHeld) {
        ExReleaseResource (&PsLoadedModuleResource);
        KeLeaveCriticalRegion();
    }
    return FoundEntry;
}

VOID
MmUnlockPagableImageSection(
    IN PVOID ImageSectionHandle
    )

/*++

Routine Description:

    This function unlocks from memory, the pages locked by a preceding call to
    MmLockPagableDataSection.

Arguments:

    ImageSectionHandle - Supplies the value returned by a previous call
        to MmLockPagableDataSection.

Return Value:

    None.

--*/

{
    PIMAGE_SECTION_HEADER NtSection;
    PMMPTE PointerPte;
    PMMPTE LastPte;
    PFN_NUMBER PageFrameIndex;
    PMMPFN Pfn1;
    KIRQL OldIrql;
    PVOID BaseAddress;
    ULONG SizeToUnlock;
    ULONG Collision;

    if (MI_IS_PHYSICAL_ADDRESS(ImageSectionHandle)) {

        //
        // No need to lock physical addresses.
        //

        return;
    }

    NtSection = (PIMAGE_SECTION_HEADER)ImageSectionHandle;

    BaseAddress = SECTION_BASE_ADDRESS(NtSection);
    SizeToUnlock = NtSection->SizeOfRawData;

    PointerPte = MiGetPteAddress(BaseAddress);
    LastPte = MiGetPteAddress((PCHAR)BaseAddress + SizeToUnlock - 1);

    //
    // Address must be within the system cache.
    //

    LOCK_PFN2 (OldIrql);

    //
    // The NumberOfLinenumbers field is used to store the
    // lock count.
    //

    ASSERT (NtSection->NumberOfLinenumbers >= 2);
    NtSection->NumberOfLinenumbers -= 1;

    if (NtSection->NumberOfLinenumbers != 1) {
        UNLOCK_PFN2 (OldIrql);
        return;
    }

    do {
        ASSERT (PointerPte->u.Hard.Valid == 1);

        PageFrameIndex = MI_GET_PAGE_FRAME_FROM_PTE (PointerPte);
        Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);

        ASSERT (Pfn1->u3.e2.ReferenceCount > 1);

        MI_REMOVE_LOCKED_PAGE_CHARGE (Pfn1, 37);

        MiDecrementReferenceCount (PageFrameIndex);

        PointerPte += 1;

    } while (PointerPte <= LastPte);

    NtSection->NumberOfLinenumbers -= 1;
    ASSERT (NtSection->NumberOfLinenumbers == 0);
    Collision = MmCollidedLockWait;
    MmCollidedLockWait = FALSE;
    MmLockedCode -= SizeToUnlock;

    UNLOCK_PFN2 (OldIrql);

    if (Collision) {
        KePulseEvent (&MmCollidedLockEvent, 0, FALSE);
    }

    return;
}


BOOLEAN
MmIsRecursiveIoFault(
    VOID
    )

/*++

Routine Description:

    This function examines the thread's page fault clustering information
    and determines if the current page fault is occurring during an I/O
    operation.

Arguments:

    None.

Return Value:

    Returns TRUE if the fault is occurring during an I/O operation,
    FALSE otherwise.

--*/

{
    return (BOOLEAN)(PsGetCurrentThread()->DisablePageFaultClustering |
                     PsGetCurrentThread()->ForwardClusterOnly);
}


VOID
MmMapMemoryDumpMdl(
    IN OUT PMDL MemoryDumpMdl
    )

/*++

Routine Description:

    For use by crash dump routine ONLY.  Maps an MDL into a fixed
    portion of the address space.  Only 1 MDL can be mapped at a
    time.

Arguments:

    MemoryDumpMdl - Supplies the MDL to map.

Return Value:

    None, fields in MDL updated.

--*/

{
    PFN_NUMBER NumberOfPages;
    PMMPTE PointerPte;
    PCHAR BaseVa;
    MMPTE TempPte;
    PPFN_NUMBER Page;

    NumberOfPages = BYTES_TO_PAGES (MemoryDumpMdl->ByteCount + MemoryDumpMdl->ByteOffset);

    ASSERT (NumberOfPages <= 16);

    PointerPte = MmCrashDumpPte;
    BaseVa = (PCHAR)MiGetVirtualAddressMappedByPte(PointerPte);
    MemoryDumpMdl->MappedSystemVa = (PCHAR)BaseVa + MemoryDumpMdl->ByteOffset;
    TempPte = ValidKernelPte;
    Page = (PPFN_NUMBER)(MemoryDumpMdl + 1);

    //
    // If the pages don't span the entire dump virtual address range,
    // build a barrier.  Otherwise use the default barrier provided at the
    // end of the dump virtual address range.
    //

    if (NumberOfPages < 16) {
        KiFlushSingleTb (TRUE, BaseVa + (NumberOfPages << PAGE_SHIFT));
        (PointerPte + NumberOfPages)->u.Long = MM_KERNEL_DEMAND_ZERO_PTE;
    }

    do {

        KiFlushSingleTb (TRUE, BaseVa);

        TempPte.u.Hard.PageFrameNumber = *Page;

        //
        // Note this PTE may be valid or invalid prior to the overwriting here.
        //

        *PointerPte = TempPte;

        Page += 1;
        PointerPte += 1;
        BaseVa += PAGE_SIZE;
        NumberOfPages -= 1;
    } while (NumberOfPages != 0);

    return;
}


VOID
MmReleaseDumpAddresses (
    IN PFN_NUMBER Pages
    )

/*++

Routine Description:

    For use by hibernate routine ONLY.  Puts zeros back into the
    used dump PTEs.

Arguments:

    None

Return Value:

    None

--*/

{
    PMMPTE PointerPte;
    PCHAR BaseVa;

    PointerPte = MmCrashDumpPte;
    BaseVa = (PCHAR)MiGetVirtualAddressMappedByPte(PointerPte);

    while (Pages) {

        KiFlushSingleTb (TRUE, BaseVa);

        PointerPte->u.Long = MM_ZERO_PTE;
        PointerPte += 1;
        BaseVa += PAGE_SIZE;
        Pages -= 1;
    }
}


NTSTATUS
MmSetBankedSection (
    IN HANDLE ProcessHandle,
    IN PVOID VirtualAddress,
    IN ULONG BankLength,
    IN BOOLEAN ReadWriteBank,
    IN PBANKED_SECTION_ROUTINE BankRoutine,
    IN PVOID Context
    )

/*++

Routine Description:

    This function declares a mapped video buffer as a banked
    section.  This allows banked video devices (i.e., even
    though the video controller has a megabyte or so of memory,
    only a small bank (like 64k) can be mapped at any one time.

    In order to overcome this problem, the pager handles faults
    to this memory, unmaps the current bank, calls off to the
    video driver and then maps in the new bank.

    This function creates the necessary structures to allow the
    video driver to be called from the pager.

 ********************* NOTE NOTE NOTE *************************
    At this time only read/write banks are supported!

Arguments:

    ProcessHandle - Supplies a handle to the process in which to
                    support the banked video function.

    VirtualAddress - Supplies the virtual address where the video
                     buffer is mapped in the specified process.

    BankLength - Supplies the size of the bank.

    ReadWriteBank - Supplies TRUE if the bank is read and write.

    BankRoutine - Supplies a pointer to the routine that should be
                  called by the pager.

    Context - Supplies a context to be passed by the pager to the
              BankRoutine.

Return Value:

    Returns the status of the function.

Environment:

    Kernel mode, APC_LEVEL or below.

--*/

{
    NTSTATUS Status;
    PEPROCESS Process;
    PMMVAD Vad;
    PMMPTE PointerPte;
    PMMPTE LastPte;
    MMPTE TempPte;
    ULONG_PTR size;
    LONG count;
    ULONG NumberOfPtes;
    PMMBANKED_SECTION Bank;

    PAGED_CODE ();

    UNREFERENCED_PARAMETER (ReadWriteBank);

    //
    // Reference the specified process handle for VM_OPERATION access.
    //

    Status = ObReferenceObjectByHandle ( ProcessHandle,
                                         PROCESS_VM_OPERATION,
                                         PsProcessType,
                                         KernelMode,
                                         (PVOID *)&Process,
                                         NULL );

    if (!NT_SUCCESS(Status)) {
        return Status;
    }

    KeAttachProcess (&Process->Pcb);

    //
    // Get the address creation mutex to block multiple threads from
    // creating or deleting address space at the same time and
    // get the working set mutex so virtual address descriptors can
    // be inserted and walked.  Block APCs so an APC which takes a page
    // fault does not corrupt various structures.
    //

    LOCK_WS_AND_ADDRESS_SPACE (Process);

    //
    // Make sure the address space was not deleted, if so, return an error.
    //

    if (Process->AddressSpaceDeleted != 0) {
        Status = STATUS_PROCESS_IS_TERMINATING;
        goto ErrorReturn;
    }

    Vad = MiLocateAddress (VirtualAddress);

    if ((Vad == NULL) ||
        (Vad->StartingVpn != MI_VA_TO_VPN (VirtualAddress)) ||
        (Vad->u.VadFlags.PhysicalMapping == 0)) {
        Status = STATUS_NOT_MAPPED_DATA;
        goto ErrorReturn;
    }

    size = PAGE_SIZE + ((Vad->EndingVpn - Vad->StartingVpn) << PAGE_SHIFT);
    if ((size % BankLength) != 0) {
        Status = STATUS_INVALID_VIEW_SIZE;
        goto ErrorReturn;
    }

    count = -1;
    NumberOfPtes = BankLength;

    do {
        NumberOfPtes = NumberOfPtes >> 1;
        count += 1;
    } while (NumberOfPtes != 0);

    //
    // Turn VAD into Banked VAD
    //

    NumberOfPtes = BankLength >> PAGE_SHIFT;

    Bank = ExAllocatePoolWithTag (NonPagedPool,
                                    sizeof (MMBANKED_SECTION) +
                                       (NumberOfPtes - 1) * sizeof(MMPTE),
                                    '  mM');
    if (Bank == NULL) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto ErrorReturn;
    }

    Bank->BankShift = PTE_SHIFT + count - PAGE_SHIFT;

    PointerPte = MiGetPteAddress(MI_VPN_TO_VA (Vad->StartingVpn));
    ASSERT (PointerPte->u.Hard.Valid == 1);

    Vad->u4.Banked = Bank;
    Bank->BasePhysicalPage = MI_GET_PAGE_FRAME_FROM_PTE (PointerPte);
    Bank->BasedPte = PointerPte;
    Bank->BankSize = BankLength;
    Bank->BankedRoutine = BankRoutine;
    Bank->Context = Context;
    Bank->CurrentMappedPte = PointerPte;

    //
    // Build the template PTEs structure.
    //

    count = 0;
    TempPte = ZeroPte;

    MI_MAKE_VALID_PTE (TempPte,
                       Bank->BasePhysicalPage,
                       MM_READWRITE,
                       PointerPte);

    if (TempPte.u.Hard.Write) {
        MI_SET_PTE_DIRTY (TempPte);
    }

    do {
        Bank->BankTemplate[count] = TempPte;
        TempPte.u.Hard.PageFrameNumber += 1;
        count += 1;
    } while ((ULONG)count < NumberOfPtes );

    LastPte = MiGetPteAddress (MI_VPN_TO_VA (Vad->EndingVpn));

    //
    // Set all PTEs within this range to zero.  Any faults within
    // this range will call the banked routine before making the
    // page valid.
    //

    RtlFillMemory (PointerPte,
                   (size >> (PAGE_SHIFT - PTE_SHIFT)),
                   (UCHAR)ZeroPte.u.Long);

    KeFlushEntireTb (TRUE, TRUE);

    Status = STATUS_SUCCESS;
ErrorReturn:

    UNLOCK_WS_AND_ADDRESS_SPACE (Process);
    KeDetachProcess();
    ObDereferenceObject (Process);
    return Status;
}

PVOID
MmMapVideoDisplay (
     IN PHYSICAL_ADDRESS PhysicalAddress,
     IN SIZE_T NumberOfBytes,
     IN MEMORY_CACHING_TYPE CacheType
     )

/*++

Routine Description:

    This function maps the specified physical address into the non-pagable
    portion of the system address space.

Arguments:

    PhysicalAddress - Supplies the starting physical address to map.

    NumberOfBytes - Supplies the number of bytes to map.

    CacheType - Supplies MmNonCached if the physical address is to be mapped
                as non-cached, MmCached if the address should be cached, and
                MmWriteCombined if the address should be cached and
                write-combined as a frame buffer. For I/O device registers,
                this is usually specified as MmNonCached.

Return Value:

    Returns the virtual address which maps the specified physical addresses.
    The value NULL is returned if sufficient virtual address space for
    the mapping could not be found.

Environment:

    Kernel mode, IRQL of APC_LEVEL or below.

--*/

{
    PMMPTE PointerPte;
    PVOID BaseVa;
#ifdef LARGE_PAGES
    MMPTE TempPte;
    PFN_NUMBER PageFrameIndex;
    PFN_NUMBER NumberOfPages;
    ULONG size;
    PMMPTE protoPte;
    PMMPTE largePte;
    ULONG pageSize;
    PSUBSECTION Subsection;
    ULONG Alignment;
    ULONG EmPageSize;
#endif LARGE_PAGES
    ULONG LargePages;

    LargePages = FALSE;
    PointerPte = NULL;

#if defined (i386) && !defined (_X86PAE_)
    ASSERT (PhysicalAddress.HighPart == 0);
#endif

    PAGED_CODE();

    ASSERT (NumberOfBytes != 0);

#ifdef LARGE_PAGES
    NumberOfPages = COMPUTE_PAGES_SPANNED (PhysicalAddress.LowPart,
                                           NumberOfBytes);

    TempPte = ValidKernelPte;
    MI_DISABLE_CACHING (TempPte);
    PageFrameIndex = (PFN_NUMBER)(PhysicalAddress.QuadPart >> PAGE_SHIFT);
    TempPte.u.Hard.PageFrameNumber = PageFrameIndex;

    if ((NumberOfBytes > X64K) && (!MmLargeVideoMapped)) {
        size = (NumberOfBytes - 1) >> (PAGE_SHIFT + 1);
        pageSize = PAGE_SIZE;

        while (size != 0) {
            size = size >> 2;
            pageSize = pageSize << 2;
        }

        Alignment = pageSize << 1;
        if (Alignment < MM_VA_MAPPED_BY_PDE) {
            Alignment = MM_VA_MAPPED_BY_PDE;
        }

#if defined(_IA64_)

        //
        // Convert pageSize to the EM specific page-size field format
        //

        EmPageSize = 0;
        size = pageSize - 1 ;

        while (size) {
            size = size >> 1;
            EmPageSize += 1;
        }

        if (NumberOfBytes > pageSize) {

            if (MmPageSizeInfo & (pageSize << 1)) {

                //
                // if larger page size is supported in the implementation
                //

                pageSize = pageSize << 1;
                EmPageSize += 1;

            }
            else {

                EmPageSize = EmPageSize | pageSize;

            }
        }

        pageSize = EmPageSize;
#endif

        NumberOfPages = Alignment >> PAGE_SHIFT;

        PointerPte = MiReserveSystemPtes(NumberOfPages,
                                         SystemPteSpace,
                                         Alignment,
                                         0,
                                         FALSE);

        if (PointerPte == NULL) {
            goto MapWithSmallPages;
        }

        protoPte = ExAllocatePoolWithTag (PagedPool,
                                           sizeof (MMPTE),
                                           'bSmM');

        if (protoPte == NULL) {
            MiReleaseSystemPtes(PointerPte, NumberOfPages, SystemPteSpace);
            goto MapWithSmallPages;
        }

        Subsection = ExAllocatePoolWithTag (NonPagedPool,
                                     sizeof(SUBSECTION) + (4 * sizeof(MMPTE)),
                                     'bSmM');

        if (Subsection == NULL) {
            ExFreePool (protoPte);
            MiReleaseSystemPtes(PointerPte, NumberOfPages, SystemPteSpace);
            goto MapWithSmallPages;
        }

        MiFillMemoryPte (PointerPte,
                         Alignment >> (PAGE_SHIFT - PTE_SHIFT),
                         MM_ZERO_KERNEL_PTE);

        //
        // Build large page descriptor and fill in all the PTEs.
        //

        Subsection->StartingSector = pageSize;
        Subsection->EndingSector = (ULONG)NumberOfPages;
        Subsection->u.LongFlags = 0;
        Subsection->u.SubsectionFlags.LargePages = 1;
        Subsection->u.SubsectionFlags.Protection = MM_READWRITE | MM_NOCACHE;
        Subsection->PtesInSubsection = Alignment;
        Subsection->SubsectionBase = PointerPte;

        largePte = (PMMPTE)(Subsection + 1);

        //
        // Build the first 2 PTEs as entries for the TLB to
        // map the specified physical address.
        //

        *largePte = TempPte;
        largePte += 1;

        if (NumberOfBytes > pageSize) {
            *largePte = TempPte;
            largePte->u.Hard.PageFrameNumber += (pageSize >> PAGE_SHIFT);
        } else {
            *largePte = ZeroKernelPte;
        }

        //
        // Build the first prototype PTE as a paging file format PTE
        // referring to the subsection.
        //

        protoPte->u.Long = MiGetSubsectionAddressForPte(Subsection);
        protoPte->u.Soft.Prototype = 1;
        protoPte->u.Soft.Protection = MM_READWRITE | MM_NOCACHE;

        //
        // Set the PTE up for all the user's PTE entries, proto pte
        // format pointing to the 3rd prototype PTE.
        //

        TempPte.u.Long = MiProtoAddressForPte (protoPte);
        MI_SET_GLOBAL_STATE (TempPte, 1);
        LargePages = TRUE;
        MmLargeVideoMapped = TRUE;
    }

    if (PointerPte != NULL) {
        BaseVa = (PVOID)MiGetVirtualAddressMappedByPte (PointerPte);
        BaseVa = (PVOID)((PCHAR)BaseVa + BYTE_OFFSET(PhysicalAddress.LowPart));

        do {
            ASSERT (PointerPte->u.Hard.Valid == 0);
            MI_WRITE_VALID_PTE (PointerPte, TempPte);
            PointerPte += 1;
            NumberOfPages -= 1;
        } while (NumberOfPages != 0);
    } else {

MapWithSmallPages:

#endif //LARGE_PAGES

        BaseVa = MmMapIoSpace (PhysicalAddress,
                               NumberOfBytes,
                               CacheType);
#ifdef LARGE_PAGES
    }
#endif //LARGE_PAGES

    return BaseVa;
}

VOID
MmUnmapVideoDisplay (
     IN PVOID BaseAddress,
     IN SIZE_T NumberOfBytes
     )

/*++

Routine Description:

    This function unmaps a range of physical address which were previously
    mapped via an MmMapVideoDisplay function call.

Arguments:

    BaseAddress - Supplies the base virtual address where the physical
                  address was previously mapped.

    NumberOfBytes - Supplies the number of bytes which were mapped.

Return Value:

    None.

Environment:

    Kernel mode, IRQL of APC_LEVEL or below.

--*/

{

#ifdef LARGE_PAGES
    PFN_NUMBER NumberOfPages;
    ULONG i;
    PMMPTE FirstPte;
    KIRQL OldIrql;
    PMMPTE LargePte;
    PSUBSECTION Subsection;

    PAGED_CODE();

    ASSERT (NumberOfBytes != 0);
    NumberOfPages = COMPUTE_PAGES_SPANNED (BaseAddress, NumberOfBytes);
    FirstPte = MiGetPteAddress (BaseAddress);

    if ((NumberOfBytes > X64K) && (FirstPte->u.Hard.Valid == 0)) {

        ASSERT (MmLargeVideoMapped);
        LargePte = MiPteToProto (FirstPte);
        Subsection = MiGetSubsectionAddress (LargePte);
        ASSERT (Subsection->SubsectionBase == FirstPte);

        NumberOfPages = Subsection->EndingSector;
        ExFreePool (Subsection);
        ExFreePool (LargePte);
        MmLargeVideoMapped = FALSE;
        KeFillFixedEntryTb ((PHARDWARE_PTE)FirstPte, (PVOID)KSEG0_BASE, LARGE_ENTRY);
    }
    MiReleaseSystemPtes(FirstPte, NumberOfPages, SystemPteSpace);
    return;

#else // LARGE_PAGES

    MmUnmapIoSpace (BaseAddress, NumberOfBytes);
    return;
#endif //LARGE_PAGES
}


VOID
MmLockPagedPool (
    IN PVOID Address,
    IN SIZE_T SizeInBytes
    )

/*++

Routine Description:

    Locks the specified address (which MUST reside in paged pool) into
    memory until MmUnlockPagedPool is called.

Arguments:

    Address - Supplies the address in paged pool to lock.

    SizeInBytes - Supplies the size in bytes to lock.

Return Value:

    None.

Environment:

    Kernel mode, IRQL of APC_LEVEL or below.

--*/

{
    PMMPTE PointerPte;
    PMMPTE LastPte;
    KIRQL OldIrql;
    KIRQL OldIrqlWs;

    MmLockPagableSectionByHandle(ExPageLockHandle);
    PointerPte = MiGetPteAddress (Address);
    LastPte = MiGetPteAddress ((PVOID)((PCHAR)Address + (SizeInBytes - 1)));
    LOCK_SYSTEM_WS (OldIrqlWs);
    LOCK_PFN (OldIrql);
    MiLockCode (PointerPte, LastPte, MM_LOCK_BY_REFCOUNT);
    UNLOCK_PFN (OldIrql);
    UNLOCK_SYSTEM_WS (OldIrqlWs);
    MmUnlockPagableImageSection(ExPageLockHandle);
    return;
}

NTKERNELAPI
VOID
MmUnlockPagedPool (
    IN PVOID Address,
    IN SIZE_T SizeInBytes
    )

/*++

Routine Description:

    Unlocks paged pool that was locked with MmLockPagedPool.

Arguments:

    Address - Supplies the address in paged pool to unlock.

    Size - Supplies the size to unlock.

Return Value:

    None.

Environment:

    Kernel mode, IRQL of APC_LEVEL or below.

--*/

{
    PMMPTE PointerPte;
    PMMPTE LastPte;
    KIRQL OldIrql;
    PFN_NUMBER PageFrameIndex;
    PMMPFN Pfn1;

    MmLockPagableSectionByHandle(ExPageLockHandle);
    PointerPte = MiGetPteAddress (Address);
    LastPte = MiGetPteAddress ((PVOID)((PCHAR)Address + (SizeInBytes - 1)));
    LOCK_PFN2 (OldIrql);

    do {
        ASSERT (PointerPte->u.Hard.Valid == 1);

        PageFrameIndex = MI_GET_PAGE_FRAME_FROM_PTE (PointerPte);
        Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);

        ASSERT (Pfn1->u3.e2.ReferenceCount > 1);

        MI_REMOVE_LOCKED_PAGE_CHARGE (Pfn1, 35);

        MiDecrementReferenceCount (PageFrameIndex);

        PointerPte += 1;
    } while (PointerPte <= LastPte);

    UNLOCK_PFN2 (OldIrql);
    MmUnlockPagableImageSection(ExPageLockHandle);
    return;
}

NTKERNELAPI
ULONG
MmGatherMemoryForHibernate (
    IN PMDL Mdl,
    IN BOOLEAN Wait
    )

/*++

Routine Description:

    Finds enough memory to fill in the pages of the MDL for power management
    hibernate function.

Arguments:

    Mdl - Supplies an MDL, the start VA field should be NULL.  The length
          field indicates how many pages to obtain.

    Wait - FALSE to fail immediately if the pages aren't available.

Return Value:

    TRUE if the MDL could be filled in, FALSE otherwise.

Environment:

    Kernel mode, IRQL of APC_LEVEL or below.

--*/

{
    KIRQL OldIrql;
    PFN_NUMBER PagesNeeded;
    PPFN_NUMBER Pages;
    PFN_NUMBER i;
    PFN_NUMBER PageFrameIndex;
    PMMPFN Pfn1;
    ULONG status;

    status = FALSE;

    PagesNeeded = Mdl->ByteCount >> PAGE_SHIFT;
    Pages = (PPFN_NUMBER)(Mdl + 1);

    i = Wait ? 100 : 1;

    InterlockedIncrement (&MiDelayPageFaults);

    do {

        LOCK_PFN2 (OldIrql);
        if (MmAvailablePages > PagesNeeded) {

            //
            // Fill in the MDL.
            //

            do {
                PageFrameIndex = MiRemoveAnyPage (MI_GET_PAGE_COLOR_FROM_PTE (NULL));
                Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);
                MI_SET_PFN_DELETED (Pfn1);
                Pfn1->u3.e2.ReferenceCount += 1;
                Pfn1->OriginalPte.u.Long = MM_DEMAND_ZERO_WRITE_PTE;
                *Pages = PageFrameIndex;
                Pages += 1;
                PagesNeeded -= 1;
            } while (PagesNeeded);
            UNLOCK_PFN2 (OldIrql);
            Mdl->MdlFlags |= MDL_PAGES_LOCKED;
            status = TRUE;
            break;
        }

        UNLOCK_PFN2 (OldIrql);

        //
        // If we're being called at DISPATCH_LEVEL we cannot move pages to
        // the standby list because mutexes must be acquired to do so.
        //

        if (OldIrql > APC_LEVEL) {
            break;
        }

        if (!i) {
            break;
        }

        //
        // Attempt to move pages to the standby list.
        //

        MiEmptyAllWorkingSets ();
        MiFlushAllPages();

        KeDelayExecutionThread (KernelMode,
                                FALSE,
                                (PLARGE_INTEGER)&Mm30Milliseconds);
        i -= 1;

    } while (TRUE);

    InterlockedDecrement (&MiDelayPageFaults);

    return status;
}

NTKERNELAPI
VOID
MmReturnMemoryForHibernate (
    IN PMDL Mdl
    )

/*++

Routine Description:

    Returns memory from MmGatherMemoryForHibername.

Arguments:

    Mdl - Supplies an MDL, the start VA field should be NULL.  The length
          field indicates how many pages to obtain.

Return Value:

    None.

Environment:

    Kernel mode, IRQL of APC_LEVEL or below.

--*/

{
    KIRQL OldIrql;
    PFN_NUMBER PagesNeeded;
    PPFN_NUMBER Pages;

    PagesNeeded = (Mdl->ByteCount >> PAGE_SHIFT);
    Pages = (PPFN_NUMBER)(Mdl + 1);

    LOCK_PFN2 (OldIrql);
    do {
        MiDecrementReferenceCount (*Pages);
        Pages += 1;
        PagesNeeded -= 1;
    } while (PagesNeeded);
    UNLOCK_PFN2 (OldIrql);
    return;
}


VOID
MmSetKernelDumpRange(
    IN OUT PVOID pDumpContext
    )

/*++

Routine Description:

    For use by crash dump routine ONLY.

    Specifies the range of memory (mostly kernel) to include in the dump.

Arguments:

    pDumpContext - Opaque dump context

Return Value:

    None.

Environment:

    Kernel mode, post-bugcheck.

--*/

{
    PLIST_ENTRY NextEntry;
    PLDR_DATA_TABLE_ENTRY DataTableEntry;
    PCHAR Kseg0Addr;
    PCHAR Kseg2Addr;
    PFN_NUMBER Pages;
    PCHAR Va;
    PCHAR HighVa;
    LOGICAL IsPhysicalAddress;
    ULONG Index;
#if defined(_X86_)
    PHYSICAL_ADDRESS DirBase;
    PKDESCRIPTOR Descriptor;
    PKPROCESSOR_STATE ProcessorState;
#endif

    //
    // Initialize memory boundary values.
    //

    Kseg0Addr = (PCHAR)MmSystemRangeStart;
    Kseg2Addr = Kseg0Addr + MM_PAGES_IN_KSEG0 * PAGE_SIZE;

    //
    // If unbiased, copy only valid portions of KSEG0.
    //

    if (!MmVirtualBias) {

        //
        // Include loaded modules in KSEG0.
        //

        NextEntry = PsLoadedModuleList.Flink;

        while (NextEntry != &PsLoadedModuleList) {

            DataTableEntry = CONTAINING_RECORD (NextEntry,
                                                LDR_DATA_TABLE_ENTRY,
                                                InLoadOrderLinks);
            Va = DataTableEntry->DllBase;

            if (Va >= Kseg0Addr && Va < Kseg2Addr) {

                Pages = COMPUTE_PAGES_SPANNED(Va, DataTableEntry->SizeOfImage);

                IoSetDumpRange (pDumpContext,
                                Va,
                                (ULONG)Pages,
                                (BOOLEAN) MI_IS_PHYSICAL_ADDRESS (Va));
            }
            NextEntry = NextEntry->Flink;
        }
    }

    //
    // Add the processor block array. That is, the array of pointers to
    // KPRCB objects. This is necessary if we are going to support the
    // !prcb extension.
    //

    Va = (PCHAR) KiProcessorBlock;

    Pages = ADDRESS_AND_SIZE_TO_SPAN_PAGES (
                            Va,
                            sizeof (PKPRCB) * KeNumberProcessors);


    IoSetDumpRange (pDumpContext,
                    Va,
                    (ULONG)Pages,
                    (BOOLEAN) MI_IS_PHYSICAL_ADDRESS (Va));

    //
    // Add the contents of the processor blocks.
    //
    
    for (Index = 0; (CCHAR)Index < KeNumberProcessors; Index += 1) {

        Va = (PCHAR) KiProcessorBlock[Index];

        Pages = ADDRESS_AND_SIZE_TO_SPAN_PAGES (Va,
                                                sizeof (KPRCB));

        IoSetDumpRange (pDumpContext,
                        Va,
                        (ULONG)Pages,
                        (BOOLEAN) MI_IS_PHYSICAL_ADDRESS (Va));


#if defined (_X86_)

        //
        // Add the global descriptor table.
        //
        
        ProcessorState = &KiProcessorBlock[Index]->ProcessorState;

        Descriptor = &ProcessorState->SpecialRegisters.Gdtr;

        Va = (PCHAR) Descriptor->Base;

        if (Va != NULL) {

            Pages = ADDRESS_AND_SIZE_TO_SPAN_PAGES (Va, Descriptor->Limit);

            IoSetDumpRange (pDumpContext,
                            Va,
                            (ULONG)Pages,
                            (BOOLEAN) MI_IS_PHYSICAL_ADDRESS (Va));
        }

        //
        // Add the intrrupt descriptor table.
        //
        
        Descriptor = &ProcessorState->SpecialRegisters.Idtr;

        Va = (PCHAR) Descriptor->Base;

        if (Va != NULL) {

            Pages = ADDRESS_AND_SIZE_TO_SPAN_PAGES (Va, Descriptor->Limit);

            IoSetDumpRange (pDumpContext,
                            Va,
                            (ULONG)Pages,
                            (BOOLEAN) MI_IS_PHYSICAL_ADDRESS (Va));
        }
    }

    //
    // Add the current page directory table page - don't use the directory
    // table base for the crashing process as we have switched cr3 on
    // stack overflow crashes, etc.
    //

    _asm {
        mov     eax, cr3
        mov     DirBase.LowPart, eax
    }

    //
    // cr3 is always located below 4gb physical.
    //

    DirBase.HighPart = 0;

    Va = MmGetVirtualForPhysical (DirBase);

    IoSetDumpRange (pDumpContext,
                    Va,
                    1,
                    (BOOLEAN) MI_IS_PHYSICAL_ADDRESS (Va));
#endif

    //
    // Check to see if nonpaged pool starts before Kseg2 and if so add it.
    //

    if (((PCHAR)MmNonPagedPoolStart >= Kseg0Addr) &&
         ((PCHAR)MmNonPagedPoolStart <= Kseg2Addr)) {

        Va = MmNonPagedPoolStart;

        Pages = (ULONG) COMPUTE_PAGES_SPANNED(Va, MmSizeOfNonPagedPoolInBytes);

        IoSetDumpRange (pDumpContext,
                        Va,
                        (ULONG)Pages,
                        (BOOLEAN) MI_IS_PHYSICAL_ADDRESS (Va));
    }

    //
    // Include all valid kernel memory above KSEG2 (or KSEG0 if biased).
    //

    if (!MmVirtualBias) {
        Va = Kseg2Addr;
    }
    else {
        Va = Kseg0Addr;
    }

    do {

        if (MmIsAddressValid(Va)) {

            IoSetDumpRange(pDumpContext,
                           Va,
                           1,
                           FALSE);
        }

        Va += PAGE_SIZE;

    } while ((LONG_PTR)Va < (MM_SYSTEM_SPACE_END & ~(PAGE_SIZE - 1)));

    //
    // Include the PFN database if it was not already done above.
    //

    if ((PCHAR)MmPfnDatabase < Kseg2Addr) {

        IsPhysicalAddress = MI_IS_PHYSICAL_ADDRESS(MmPfnDatabase);

        IoSetDumpRange (pDumpContext,
                        MmPfnDatabase,
                        ADDRESS_AND_SIZE_TO_SPAN_PAGES (MmPfnDatabase,
                          (MmHighestPossiblePhysicalPage + 1) * sizeof (MMPFN)),
                        (BOOLEAN) IsPhysicalAddress);

        if (IsPhysicalAddress == FALSE) {

            //
            // The PFN database may be sparse.  Exclude any nonexistent
            // ranges now.
            //

            Va = (PCHAR)MmPfnDatabase;
            HighVa = Va + (MmHighestPossiblePhysicalPage + 1) * sizeof (MMPFN);

            do {
                if (!MmIsAddressValid(Va)) {
    
                    IoFreeDumpRange (pDumpContext,
                                     Va,
                                     1,
                                     FALSE);
                }

                Va += PAGE_SIZE;

            } while (Va < HighVa);
        }
    }

    //
    // Exclude all free non-paged pool.
    //

    for (Index = 0; Index < MI_MAX_FREE_LIST_HEADS; Index += 1) {

        NextEntry = MmNonPagedPoolFreeListHead[Index].Flink;

        while (NextEntry != &MmNonPagedPoolFreeListHead[Index]) {

            PMMFREE_POOL_ENTRY FreePageInfo;

            //
            // The list is not empty, remove free ones
            //

            if (MmProtectFreedNonPagedPool == TRUE) {
                MiUnProtectFreeNonPagedPool ((PVOID)NextEntry, 0);
            }

            FreePageInfo = CONTAINING_RECORD(NextEntry,
                                             MMFREE_POOL_ENTRY,
                                             List);

            ASSERT (FreePageInfo->Signature == MM_FREE_POOL_SIGNATURE);

            if (FreePageInfo->Size) {

                Va = (PCHAR)FreePageInfo;

                Pages = COMPUTE_PAGES_SPANNED(FreePageInfo, FreePageInfo->Size);

                IoFreeDumpRange(pDumpContext,
                                Va,
                                (ULONG)Pages,
                                (BOOLEAN) MI_IS_PHYSICAL_ADDRESS (Va));

            }

            NextEntry = FreePageInfo->List.Flink;

            if (MmProtectFreedNonPagedPool == TRUE) {
                MiProtectFreeNonPagedPool ((PVOID)FreePageInfo,
                                           (ULONG)FreePageInfo->Size);
            }
        }
    }

    //
    // Exclude all valid system cache addresses.
    //

    for (Va = (PCHAR)MmSystemCacheStart; Va < (PCHAR)MmSystemCacheEnd; Va += PAGE_SIZE) {

        if (MmIsAddressValid(Va)) {

            ASSERT (MI_IS_PHYSICAL_ADDRESS(Va) == FALSE);

            IoFreeDumpRange (pDumpContext,
                             Va,
                             1,
                             FALSE);
        }
    }

#if defined(_X86_)
    if (MiSystemCacheEndExtra != MmSystemCacheEnd) {
        for (Va = (PCHAR)MiSystemCacheStartExtra; Va < (PCHAR)MiSystemCacheEndExtra; Va += PAGE_SIZE) {

            if (MmIsAddressValid(Va)) {

                ASSERT (MI_IS_PHYSICAL_ADDRESS(Va) == FALSE);

                IoFreeDumpRange(pDumpContext,
                                Va,
                                1,
                                FALSE);
            }
        }
    }
#endif
}


VOID
MmEnablePAT (
     VOID
     )

/*++

Routine Description:

    This routine enables the page attribute capability for individual PTE
    mappings.

Arguments:

    None.

Return Value:

    None.

Environment:

    Kernel mode.

--*/
{
    MiWriteCombiningPtes = TRUE;
}

NTSTATUS
MmDispatchWin32Callout(
    IN PKWIN32_CALLOUT CallbackRoutine,
    IN PKWIN32_CALLOUT WorkerCallback OPTIONAL,
    IN PVOID Parameter,
    IN PULONG SessionId OPTIONAL
    )

/*++

Routine Description:

    This routine dispatches callbacks to the WIN32K.SYS module.

Arguments:

    CallbackRoutine - Supplies the function in WIN32K.SYS to callback.

    WorkerCallback - If present indicates the routine to be called to do
                     additional processing if required. This routine MUST
                     then call the callback.  The CallbackRoutine will NOT be
                     called, but only used to check for existence in session
                     space.

    Parameter - Supplies the parameter to pass to the called function.

    SessionId - Supplies an optional pointer to the session ID in
                which to dispatch the callback.  NULL indicates all sessions.

Return Value:

    NTSTATUS code.

Environment:

    Kernel mode, system process context.

--*/

{
    ULONG i;
    KIRQL OldIrql;
    PLIST_ENTRY NextEntry;
    PMM_SESSION_SPACE Session;
    PEPROCESS *Processes;
    NTSTATUS Status;
    ULONG SessionCount;
    BOOLEAN Signal;
    PEPROCESS SingleProcess;
    PLIST_ENTRY NextProcessEntry;
    PEPROCESS Process;

    if (WorkerCallback == NULL) {
        WorkerCallback = CallbackRoutine;
    }

    if (MiHydra == FALSE) {
        return (*WorkerCallback)(Parameter);
    }

    if (MI_IS_SESSION_ADDRESS((ULONG_PTR)*CallbackRoutine) == FALSE) {
        return (*WorkerCallback)(Parameter);
    }

    //
    // If the call is from a user mode process, we are in
    // a session space.  In this case the power event is
    // only of interest to the current session.
    //

    if (PsGetCurrentProcess()->Vm.u.Flags.ProcessInSession == 1) {
        ASSERT (MmIsAddressValid(MmSessionSpace) == TRUE);
        return (*WorkerCallback)(Parameter);
    }

    if (ARGUMENT_PRESENT(SessionId)) {

        //
        // If the SessionId is passed in as an argument, just attach to that
        // session and deliver the callback.
        //

        SessionCount = 1;

        Processes = &SingleProcess;

        LOCK_EXPANSION (OldIrql);
    }
    else {

        //
        // We must call all the WIN32 sessions on a Hydra system.  Each is in
        // a separate address space and is pagable.  Build a list of one
        // process in each session and attach to each in order to deliver
        // the callback.
        //

ReAllocate:

        SessionCount = MiSessionCount + 10;

        Processes = (PEPROCESS *) ExAllocatePoolWithTag(
                                  NonPagedPool,
                                  SessionCount * sizeof(PEPROCESS),
                                  '23WD'
                                  );

        if (Processes == NULL) {
            return STATUS_NO_MEMORY;
        }

        LOCK_EXPANSION (OldIrql);

        if (SessionCount < MiSessionCount) {
            UNLOCK_EXPANSION (OldIrql);
            ExFreePool (Processes);
            goto ReAllocate;
        }
    }

    SessionCount = 0;

    NextEntry = MiSessionWsList.Flink;

    while (NextEntry != &MiSessionWsList) {

        Session = CONTAINING_RECORD(NextEntry, MM_SESSION_SPACE, WsListEntry);

        NextProcessEntry = Session->ProcessList.Flink;

        if ((Session->u.Flags.DeletePending == 0) &&
            (NextProcessEntry != &Session->ProcessList)) {

            if (ARGUMENT_PRESENT(SessionId) && Session->SessionId != *SessionId)
            {
                //
                // Not the one we're looking for.
                //

                NextEntry = NextEntry->Flink;
                continue;
            }

            Process = CONTAINING_RECORD (NextProcessEntry,
                                         EPROCESS,
                                         SessionProcessLinks);

            if (Process->Vm.u.Flags.SessionLeader == 1) {

                //
                // If session manager is still the first process (ie: smss
                // hasn't detached yet), then don't bother delivering to this
                // session this early in its lifetime.
                //

                NextEntry = NextEntry->Flink;
                continue;
            }

            //
            // Attach to the oldest process in the session (presumably csrss)
            // in hopes that it will exit the session last so we don't have
            // to retry.
            //

            Processes[SessionCount] = CONTAINING_RECORD (NextProcessEntry,
                                                         EPROCESS,
                                                         SessionProcessLinks);

            ObReferenceObject (Processes[SessionCount]);
            SessionCount += 1;
            Session->AttachCount += 1;

            if (ARGUMENT_PRESENT(SessionId) && Session->SessionId != *SessionId)
            {
                break;
            }
        }
        NextEntry = NextEntry->Flink;
    }

    UNLOCK_EXPANSION (OldIrql);

    //
    // Now callback the referenced process objects.
    //

    Status = STATUS_NOT_FOUND;

    for (i = 0; i < SessionCount; i += 1) {

        KeAttachProcess (&Processes[i]->Pcb);

        if (Processes[i]->SessionId == 0) {

            //
            // Return status for the console session.
            //

            Status = (*WorkerCallback)(Parameter);
        }
        else {

            (VOID)(*WorkerCallback)(Parameter);
        }

        Signal = FALSE;

        LOCK_EXPANSION (OldIrql);

        MmSessionSpace->AttachCount -= 1;

        if (MmSessionSpace->u.Flags.DeletePending == 1 && MmSessionSpace->AttachCount == 0) {
            Signal = TRUE;
        }

        UNLOCK_EXPANSION (OldIrql);

        if (Signal == TRUE) {
            KeSetEvent (&MmSessionSpace->AttachEvent, 0, FALSE);
        }

        KeDetachProcess();

        ObDereferenceObject (Processes[i]);
    }

    if (ARGUMENT_PRESENT(SessionId) == 0) {
        ExFreePool (Processes);
    }

    return Status;
}

LOGICAL
MmIsSystemAddressLocked(
    IN PVOID VirtualAddress
    )

/*++

Routine Description:

    This routine determines whether the specified system address is currently
    locked.

    This routine should only be called for debugging purposes, as it is not
    guaranteed upon return to the caller that the address is still locked.
    (The address could easily have been trimmed prior to return).

Arguments:

    VirtualAddress - Supplies the virtual address to check.

Return Value:

    TRUE if the address is locked.  FALSE if not.

Environment:

    DISPATCH LEVEL or below.  No memory management locks may be held.

--*/
{
    PMMPFN Pfn1;
    KIRQL OldIrql;
    PMMPTE PointerPte;
    PFN_NUMBER PageFrameIndex;

    if (IS_SYSTEM_ADDRESS (VirtualAddress) == FALSE) {
        return FALSE;
    }

    if (MI_IS_PHYSICAL_ADDRESS (VirtualAddress)) {
        return TRUE;
    }

    //
    // Hyperspace and page maps are not treated as locked down.
    //

    if (MI_IS_PROCESS_SPACE_ADDRESS (VirtualAddress) == TRUE) {
        return FALSE;
    }

#if defined (_IA64_)
    if (MI_IS_KERNEL_PTE_ADDRESS (VirtualAddress) == TRUE) {
        return FALSE;
    }
#endif

    LOCK_PFN2 (OldIrql);

    if (MmIsAddressValid (VirtualAddress) == FALSE) {
        UNLOCK_PFN2 (OldIrql);
        return FALSE;
    }

    PointerPte = MiGetPteAddress (VirtualAddress);

    PageFrameIndex = MI_GET_PAGE_FRAME_FROM_PTE (PointerPte);

    //
    // Note that the mapped page may not be in the PFN database.  Treat
    // this as locked.  There is no way to detect if the PFN database is
    // sparse without walking the loader blocks.  Don't bother doing this
    // as few machines are still sparse today.
    //

    if (PageFrameIndex > MmHighestPhysicalPage) {
        UNLOCK_PFN2 (OldIrql);
        return FALSE;
    }

    Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);

    //
    // Check for the page being locked by reference.
    //

    if (Pfn1->u3.e2.ReferenceCount > 1) {
        UNLOCK_PFN2 (OldIrql);
        return TRUE;
    }

    if (Pfn1->u3.e2.ReferenceCount > Pfn1->u2.ShareCount) {
        UNLOCK_PFN2 (OldIrql);
        return TRUE;
    }

    //
    // Check whether the page is locked into the working set.
    //

    if (Pfn1->u1.Event == NULL) {
        UNLOCK_PFN2 (OldIrql);
        return TRUE;
    }

    UNLOCK_PFN2 (OldIrql);

    return FALSE;
}

#if DBG

VOID
MiVerifyLockedPageCharges (
    VOID
    )
{
    PMMPFN Pfn1;
    KIRQL OldIrql;
    PFN_NUMBER start;
    PFN_NUMBER count;
    PFN_NUMBER Page;
    PFN_NUMBER LockCharged;

    if (MiPrintLockedPages == 0) {
        return;
    }

    if (KeGetCurrentIrql() > APC_LEVEL) {
        return;
    }

    start = 0;
    LockCharged = 0;

    ExAcquireFastMutex (&MmDynamicMemoryMutex);

    LOCK_PFN2 (OldIrql);

    do {

        count = MmPhysicalMemoryBlock->Run[start].PageCount;
        Page = MmPhysicalMemoryBlock->Run[start].BasePage;

        if (count != 0) {
            Pfn1 = MI_PFN_ELEMENT (Page);
            do {
                if (Pfn1->u3.e1.LockCharged == 1) {
                    if (MiPrintLockedPages & 0x4) {
                        DbgPrint ("%x ", Pfn1 - MmPfnDatabase);
                    }
                    LockCharged += 1;
                }
                count -= 1;
                Pfn1 += 1;
            } while (count != 0);
        }

        start += 1;
    } while (start != MmPhysicalMemoryBlock->NumberOfRuns);

    if (LockCharged != MmSystemLockPagesCount) {
        if (MiPrintLockedPages & 0x1) {
            DbgPrint ("MM: Locked pages MISMATCH %u %u\n",
                LockCharged, MmSystemLockPagesCount);
        }
    }
    else {
        if (MiPrintLockedPages & 0x2) {
            DbgPrint ("MM: Locked pages ok %u\n",
                LockCharged);
        }
    }

    UNLOCK_PFN2 (OldIrql);

    ExReleaseFastMutex (&MmDynamicMemoryMutex);

    return;
}
#endif
