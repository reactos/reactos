/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

   session.c

Abstract:

    This module contains the routines which implement the creation and
    deletion of session spaces along with associated support routines.

Author:

    Landy Wang (landyw) 05-Dec-1997

Revision History:

--*/

#include "mi.h"

ULONG MiSessionCount;

#define MM_MAXIMUM_CONCURRENT_SESSIONS  16384

FAST_MUTEX  MiSessionIdMutex;

PRTL_BITMAP MiSessionIdBitmap;

#if defined (_WIN64)
#define MI_SESSION_COMMIT_CHARGE 3
#else
#define MI_SESSION_COMMIT_CHARGE 2
#endif

VOID
MiSessionAddProcess(
    PEPROCESS NewProcess
    );

VOID
MiSessionRemoveProcess (
    VOID
    );

VOID
MiInitializeSessionIds (
    VOID
    );

NTSTATUS
MiSessionCreateInternal(
    OUT PULONG SessionId
    );

NTSTATUS
MiSessionCommitPageTables(
    IN PVOID StartVa,
    IN PVOID EndVa
    );

BOOLEAN
MiDereferenceSession(
    VOID
    );

VOID
MiSessionDeletePde(
    IN PMMPTE Pde,
    IN BOOLEAN WorkingSetInitialized,
    IN PMMPTE SelfMapPde
    );

#if DBG
VOID
MiCheckSessionVirtualSpace(
    IN PVOID VirtualAddress,
    IN ULONG NumberOfBytes
    );
#endif

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT,MiInitializeSessionIds)

#pragma alloc_text(PAGE, MmSessionLeader)
#pragma alloc_text(PAGE, MmSessionSetUnloadAddress)

#pragma alloc_text(PAGEHYDRA, MiSessionAddProcess)
#pragma alloc_text(PAGEHYDRA, MiSessionRemoveProcess)
#pragma alloc_text(PAGEHYDRA, MiSessionCreateInternal)
#pragma alloc_text(PAGEHYDRA, MmSessionCreate)
#pragma alloc_text(PAGEHYDRA, MmSessionDelete)
#pragma alloc_text(PAGEHYDRA, MiAttachSession)
#pragma alloc_text(PAGEHYDRA, MiDetachSession)
#pragma alloc_text(PAGEHYDRA, MiSessionCommitImagePages)
#pragma alloc_text(PAGEHYDRA, MiSessionCommitPageTables)
#pragma alloc_text(PAGEHYDRA, MiSessionOutSwapProcess)
#pragma alloc_text(PAGEHYDRA, MiSessionInSwapProcess)
#pragma alloc_text(PAGEHYDRA, MiSessionDeletePde)

#if DBG
#pragma alloc_text(PAGEHYDRA, MiCheckSessionVirtualSpace)
#endif
#endif

VOID
MmSessionLeader(
    IN PEPROCESS Process
    )

/*++

Routine Description:

    Mark the argument process as having the ability to create or delete session
    spaces.  This is only granted to the session manager process.

Arguments:

    Process - Supplies a pointer to the privileged process.

Return Value:

    None.

Environment:

    Kernel mode.

--*/

{
    Process->Vm.u.Flags.SessionLeader = 1;
}


VOID
MmSessionSetUnloadAddress (
    IN PDRIVER_OBJECT pWin32KDevice
    )

/*++

Routine Description:

    Copy the win32k.sys driver object to the session structure for use during
    unload.

Arguments:

    NewProcess - Supplies a pointer to the win32k driver object.

Return Value:

    None.

Environment:

    Kernel mode.

--*/

{
    if (MiHydra == TRUE && PsGetCurrentProcess()->Vm.u.Flags.ProcessInSession == 1) {

        ASSERT (MmIsAddressValid(MmSessionSpace) == TRUE);

        RtlMoveMemory (&MmSessionSpace->Win32KDriverObject,
                       pWin32KDevice,
                       sizeof(DRIVER_OBJECT));
	}
}


VOID
MiSessionAddProcess(
    PEPROCESS NewProcess
    )

/*++

Routine Description:

    Add the new process to the current session space.

Arguments:

    NewProcess - Supplies a pointer to the process being created.

Return Value:

    None.

Environment:

    Kernel mode, APCs disabled.

--*/

{
    KIRQL OldIrql;
    PMM_SESSION_SPACE SessionGlobal;

    //
    // If the calling process has no session, then the new process won't get
    // one either.
    //

    if (PsGetCurrentProcess()->Vm.u.Flags.ProcessInSession == 0) {
        return;
    }

    ASSERT (MmIsAddressValid (MmSessionSpace) == TRUE);

    SessionGlobal = SESSION_GLOBAL(MmSessionSpace);

    LOCK_SESSION (OldIrql);

    MmSessionSpace->ReferenceCount += 1;

    UNLOCK_SESSION (OldIrql);

#if defined(_IA64_)
    KeAddSessionSpace(&NewProcess->Pcb, &SessionGlobal->SessionMapInfo);
#endif

    //
    // Link the process entry into the session space and WSL structures.
    //

    LOCK_EXPANSION (OldIrql);

    if (IsListEmpty(&SessionGlobal->ProcessList)) {

        if (MmSessionSpace->Vm.AllowWorkingSetAdjustment == FALSE) {

            ASSERT (MmSessionSpace->u.Flags.WorkingSetInserted == 0);

            MmSessionSpace->Vm.AllowWorkingSetAdjustment = TRUE;

            InsertTailList (&MmWorkingSetExpansionHead.ListHead,
                            &SessionGlobal->Vm.WorkingSetExpansionLinks);

            MmSessionSpace->u.Flags.WorkingSetInserted = 1;
        }
    }

    InsertTailList (&SessionGlobal->ProcessList, &NewProcess->SessionProcessLinks);

    NewProcess->Vm.u.Flags.ProcessInSession = 1;

    UNLOCK_EXPANSION (OldIrql);
}


VOID
MiSessionRemoveProcess (
    VOID
    )

/*++

Routine Description:

    This routine removes the current process from the current session space.
    This may trigger a substantial round of dereferencing and resource freeing
    if it is also the last process in the session, (holding the last image
    in the group, etc).

Arguments:

    None.

Return Value:

    None.

Environment:

    Kernel mode, APC_LEVEL and below, but queueing of APCs to this thread has
    been permanently disabled.  This is the last thread in the process
    being deleted.  The caller has ensured that this process is not
    on the expansion list and therefore there can be no races in regards to
    trimming.

--*/

{
    KIRQL OldIrql;
    PEPROCESS CurrentProcess;
#if DBG
    ULONG Found;
    PEPROCESS Process;
    PLIST_ENTRY NextEntry;
    PMM_SESSION_SPACE SessionGlobal;
#endif

    CurrentProcess = PsGetCurrentProcess();

    if (CurrentProcess->Vm.u.Flags.ProcessInSession == 0) {
        return;
    }

    ASSERT (MmIsAddressValid (MmSessionSpace) == TRUE);

    //
    // Remove this process from the list of processes in the current session.
    //

    LOCK_EXPANSION (OldIrql);

#if DBG

    SessionGlobal = SESSION_GLOBAL(MmSessionSpace);

    Found = 0;
    NextEntry = SessionGlobal->ProcessList.Flink;

    while (NextEntry != &SessionGlobal->ProcessList) {
        Process = CONTAINING_RECORD (NextEntry, EPROCESS, SessionProcessLinks);

        if (Process == CurrentProcess) {
            Found = 1;
        }

        NextEntry = NextEntry->Flink;
    }

    ASSERT (Found == 1);

#endif

    RemoveEntryList (&CurrentProcess->SessionProcessLinks);

    UNLOCK_EXPANSION (OldIrql);

    //
    // Decrement this process' reference count to the session.  If this
    // is the last reference, then the entire session will be destroyed
    // upon return.  This includes unloading drivers, unmapping pools,
    // freeing page tables, etc.
    //

    MiDereferenceSession ();
}

VOID
MiInitializeSessionIds (
    VOID
    )

/*++

Routine Description:

    This routine creates and initializes session ID allocation/deallocation.

Arguments:

    None.

Return Value:

    None.

--*/

{
    //
    // If this ever grows beyond the size of a page, both the allocation and
    // deletion code will need to be updated.
    //

    ASSERT (sizeof(MM_SESSION_SPACE) <= PAGE_SIZE);

    ExInitializeFastMutex (&MiSessionIdMutex);

    MiCreateBitMap (&MiSessionIdBitmap,
                    MM_MAXIMUM_CONCURRENT_SESSIONS,
                    PagedPool);

    if (MiSessionIdBitmap == NULL) {
        MiCreateBitMap (&MiSessionIdBitmap,
                        MM_MAXIMUM_CONCURRENT_SESSIONS,
                        NonPagedPoolMustSucceed);
    }

    RtlClearAllBits (MiSessionIdBitmap);
}

NTSTATUS
MiSessionCreateInternal(
    OUT PULONG SessionId
    )

/*++

Routine Description:

    This routine creates the data structure that describes and maintains
    the session space.  It resides at the beginning of the session space.
    Carefully construct the first page mapping to bootstrap the fault
    handler which relies on the session space data structure being
    present and valid.

    In 32-bit NT, this initial mapping for the portion of session space
    mapped by the first PDE will automatically be inherited by all child
    processes when the system copies the system portion of the page
    directory for new address spaces.  Additional entries are faulted
    in by the session space fault handler, which references this structure.
    For 64-bit NT, everything is automatically inherited.

    This routine commits virtual memory within the current session space with
    backing pages.  The virtual addresses within session space are
    allocated with a separate facility in the image management facility.
    This is because images must be at a unique system wide virtual address.

Arguments:

    SessionId - Supplies a pointer to place the new session ID into.

Return Value:

    STATUS_SUCCESS if all went well, various failure status codes
    if the session was not created.

Environment:

    Kernel mode, no mutexes held.

--*/

{
    KIRQL  OldIrql;
    PMMPTE PointerPpe;
    PMMPTE PointerPde;
    PMMPTE PointerPte;
    NTSTATUS Status;
    PMM_SESSION_SPACE SessionSpace;
    PMM_SESSION_SPACE SessionGlobal;
    PFN_NUMBER ResidentPages;
    BOOLEAN GotCommit;
    BOOLEAN GotPages;
    PMMPFN Pfn1;
    MMPTE TempPte;
    MMPTE AliasPte;
    MMPTE PreviousPte;
    ULONG_PTR Va;
    PFN_NUMBER DataPage;
    PFN_NUMBER PageTablePage;
    PFN_NUMBER PageDirectoryPage;
    ULONG PageColor;

    SessionSpace = NULL;
    GotCommit = FALSE;
    GotPages = FALSE;

    ASSERT (MmIsAddressValid(SessionSpace) == FALSE);

    ExAcquireFastMutex (&MiSessionIdMutex);

    *SessionId = RtlFindClearBitsAndSet (MiSessionIdBitmap, 1, 0);

    ExReleaseFastMutex (&MiSessionIdMutex);

    if (*SessionId == 0xFFFFFFFF) {
#if DBG
        DbgPrint("MiSessionCreateInternal: No session IDs available\n");
#endif
        MM_BUMP_SESSION_FAILURES (MM_SESSION_FAILURE_NO_IDS);
        return STATUS_NO_MEMORY;
    }

    ResidentPages = MI_SESSION_COMMIT_CHARGE;

    if (MiChargeCommitment (ResidentPages, NULL) == FALSE) {
#if DBG
        if (MmDebug & MM_DBG_SESSIONS) {
            DbgPrint("MiSessionCreateInternal: No commit for %d pages\n",
                ResidentPages);
        }
#endif
        MM_BUMP_SESSION_FAILURES (MM_SESSION_FAILURE_NO_COMMIT);
        goto Failure;
    }

    GotCommit = TRUE;

    MM_TRACK_COMMIT (MM_DBG_COMMIT_SESSION_CREATE, ResidentPages);

    LOCK_PFN (OldIrql);

    //
    // Check to make sure the physical pages are available.
    //

    if ((SPFN_NUMBER)(ResidentPages + MI_SESSION_SPACE_WORKING_SET_MINIMUM) > MI_NONPAGABLE_MEMORY_AVAILABLE()) {
#if DBG
        if (MmDebug & MM_DBG_SESSIONS) {
            DbgPrint("MiSessionCreateInternal: No Resident Pages %d, Need %d\n",
                MmResidentAvailablePages,
                ResidentPages + MI_SESSION_SPACE_WORKING_SET_MINIMUM);
        }
#endif
        UNLOCK_PFN (OldIrql);

        MM_BUMP_SESSION_FAILURES (MM_SESSION_FAILURE_NO_RESIDENT);
        goto Failure;
    }

    GotPages = TRUE;

    MmResidentAvailablePages -= (ResidentPages + MI_SESSION_SPACE_WORKING_SET_MINIMUM);

    MM_BUMP_COUNTER(40, ResidentPages + MI_SESSION_SPACE_WORKING_SET_MINIMUM);

#if defined (_WIN64)

    //
    // Initialize the page directory page.
    //

    MiEnsureAvailablePageOrWait (NULL, NULL);

    PageColor = MI_GET_PAGE_COLOR_FROM_VA (NULL);

    PageDirectoryPage = MiRemoveZeroPageIfAny (PageColor);
    if (PageDirectoryPage == 0) {
        PageDirectoryPage = MiRemoveAnyPage (PageColor);
        UNLOCK_PFN (OldIrql);
        MiZeroPhysicalPage (PageDirectoryPage, PageColor);
        LOCK_PFN (OldIrql);
    }

    //
    // The global bit is masked off since we need to make sure the TB entry
    // is flushed when we switch to a process in a different session space.
    //

    TempPte.u.Long = ValidKernelPdeLocal.u.Long;
    TempPte.u.Hard.PageFrameNumber = PageDirectoryPage;

    PointerPpe = MiGetPpeAddress ((PVOID)MmSessionSpace);

    //
    // Another thread may have gotten here before us, check for that now.
    //

    if (PointerPpe->u.Long != 0) {

#if DBG
        ASSERT (PointerPpe->u.Hard.Valid == 1);
        DbgPrint("MiSessionCreateInternal: Detected PPE %p race\n",
            PointerPpe->u.Long);
        DbgBreakPoint ();
#endif //DBG

        Pfn1 = MI_PFN_ELEMENT (PageDirectoryPage);
        ASSERT (Pfn1->u2.ShareCount == 0);
        ASSERT (Pfn1->u3.e2.ReferenceCount == 0);
        Pfn1->u3.e2.ReferenceCount = 1;
        Pfn1->OriginalPte.u.Long = MM_DEMAND_ZERO_WRITE_PTE;
        MI_SET_PFN_DELETED (Pfn1);
#if DBG
        Pfn1->u3.e1.PageLocation = StandbyPageList;
#endif

        MiDecrementReferenceCount (PageDirectoryPage);

        MmResidentAvailablePages += (ResidentPages + MI_SESSION_SPACE_WORKING_SET_MINIMUM);
        MM_BUMP_COUNTER(46, ResidentPages + MI_SESSION_SPACE_WORKING_SET_MINIMUM);

        MM_BUMP_SESS_COUNTER (MM_DBG_SESSION_INITIAL_PAGETABLE_FREE_RACE, 1);

        GotPages = FALSE;
        UNLOCK_PFN (OldIrql);

        MM_BUMP_SESSION_FAILURES (MM_SESSION_FAILURE_RACE_DETECTED);

        goto Failure;
    }

    *PointerPpe = TempPte;

    MiInitializePfnForOtherProcess (PageDirectoryPage, PointerPpe, 0);
    Pfn1 = MI_PFN_ELEMENT (PageDirectoryPage);
    Pfn1->PteFrame = 0;

    ASSERT (MI_PFN_ELEMENT(PageDirectoryPage)->u1.WsIndex == 0);

    KeFillEntryTb ((PHARDWARE_PTE) PointerPpe,
                   MiGetVirtualAddressMappedByPte (PointerPpe),
                   FALSE);
#endif

    //
    // Initialize the page table page.
    //

    MiEnsureAvailablePageOrWait (NULL, NULL);

    PageColor = MI_GET_PAGE_COLOR_FROM_VA (NULL);

    PageTablePage = MiRemoveZeroPageIfAny (PageColor);
    if (PageTablePage == 0) {
        PageTablePage = MiRemoveAnyPage (PageColor);
        UNLOCK_PFN (OldIrql);
        MiZeroPhysicalPage (PageTablePage, PageColor);
        LOCK_PFN (OldIrql);
    }

    //
    // The global bit is masked off since we need to make sure the TB entry
    // is flushed when we switch to a process in a different session space.
    //

    TempPte.u.Long = ValidKernelPdeLocal.u.Long;
    TempPte.u.Hard.PageFrameNumber = PageTablePage;

    PointerPde = MiGetPdeAddress ((PVOID)MmSessionSpace);

#if !defined (_WIN64)

    //
    // Another thread may have gotten here before us, check for that now.
    //

    if (PointerPde->u.Long != 0) {
#if DBG
        ASSERT (PointerPde->u.Hard.Valid == 1);
        DbgPrint("MiSessionCreateInternal: Detected PDE %p race\n",
            PointerPde->u.Long);
        DbgBreakPoint ();
#endif //DBG

        Pfn1 = MI_PFN_ELEMENT (PageTablePage);
        ASSERT (Pfn1->u2.ShareCount == 0);
        ASSERT (Pfn1->u3.e2.ReferenceCount == 0);
        Pfn1->u3.e2.ReferenceCount = 1;
        Pfn1->OriginalPte.u.Long = MM_DEMAND_ZERO_WRITE_PTE;
        MI_SET_PFN_DELETED (Pfn1);
#if DBG
        Pfn1->u3.e1.PageLocation = StandbyPageList;
#endif

        MiDecrementReferenceCount (PageTablePage);

        MmResidentAvailablePages += (ResidentPages + MI_SESSION_SPACE_WORKING_SET_MINIMUM);
        MM_BUMP_COUNTER(46, ResidentPages + MI_SESSION_SPACE_WORKING_SET_MINIMUM);

        MM_BUMP_SESS_COUNTER (MM_DBG_SESSION_INITIAL_PAGETABLE_FREE_RACE, 1);

        GotPages = FALSE;
        UNLOCK_PFN (OldIrql);

        MM_BUMP_SESSION_FAILURES (MM_SESSION_FAILURE_RACE_DETECTED);

        goto Failure;
    }

#endif

    MI_WRITE_VALID_PTE (PointerPde, TempPte);

    //
    // This page frame references itself instead of the current (SMSS.EXE)
    // page directory as its PteFrame.  This allows the current process to
    // appear more normal (at least on 32-bit NT).  It just means we have
    // to treat this page specially during teardown.
    //

    MiInitializePfnForOtherProcess (PageTablePage, PointerPde, PageTablePage);

    //
    // This page is never paged, ensure that its WsIndex stays clear so the
    // release of the page is handled correctly.
    //

    ASSERT (MI_PFN_ELEMENT(PageTablePage)->u1.WsIndex == 0);

    Va = (ULONG_PTR)MiGetPteAddress (MmSessionSpace);

    KeFillEntryTb ((PHARDWARE_PTE) PointerPde, (PMMPTE)Va, FALSE);

    MiEnsureAvailablePageOrWait (NULL, NULL);

    PageColor = MI_GET_PAGE_COLOR_FROM_VA (NULL);

    DataPage = MiRemoveZeroPageIfAny (PageColor);
    if (DataPage == 0) {
        DataPage = MiRemoveAnyPage (PageColor);
        UNLOCK_PFN (OldIrql);
        MiZeroPhysicalPage (DataPage, PageColor);
        LOCK_PFN (OldIrql);
    }

    //
    // The global bit is masked off since we need to make sure the TB entry
    // is flushed when we switch to a process in a different session space.
    //

    TempPte.u.Long = ValidKernelPteLocal.u.Long;
    TempPte.u.Hard.PageFrameNumber = DataPage;

    PointerPte = MiGetPteAddress (MmSessionSpace);

    MI_WRITE_VALID_PTE (PointerPte, TempPte);

    MiInitializePfn (DataPage, PointerPte, 1);

    ASSERT (MI_PFN_ELEMENT(DataPage)->u1.WsIndex == 0);

    UNLOCK_PFN (OldIrql);

    KeFillEntryTb ((PHARDWARE_PTE) PointerPte, (PMMPTE)MmSessionSpace, FALSE);

    AliasPte = *PointerPte;

    //
    // Initialize the new session space data structure.
    //

    SessionSpace = MmSessionSpace;

    MM_BUMP_SESS_COUNTER (MM_DBG_SESSION_INITIAL_PAGETABLE_ALLOC, 1);
    MM_BUMP_SESS_COUNTER (MM_DBG_SESSION_INITIAL_PAGE_ALLOC, 1);

    SessionSpace->ReferenceCount = 1;
    SessionSpace->u.LongFlags = 0;
    SessionSpace->SessionId = *SessionId;
    SessionSpace->SessionPageDirectoryIndex = PageTablePage;

    SessionSpace->Color = PageColor;

    //
    // Track the page table page and the data page.
    //

    SessionSpace->NonPagablePages = ResidentPages;
    SessionSpace->CommittedPages = ResidentPages;

#if defined (_WIN64)

    //
    // Initialize the session data page directory entry so trimmers can attach.
    //

    PointerPpe = MiGetPpeAddress ((PVOID)MmSessionSpace);
    SessionSpace->PageDirectory = *PointerPpe;

#else

    //
    // Load the session data page table entry so that other processes
    // can fault in the mapping.
    //

    SessionSpace->PageTables[PointerPde - MiGetPdeAddress (MmSessionBase)] = *PointerPde;

#endif

    //
    // Reserve a global system PTE and map the data page into it.
    //

    SessionSpace->GlobalPteEntry = MiReserveSystemPtes (1,
                                                        SystemPteSpace,
                                                        0,
                                                        0,
                                                        FALSE
                                                        );

    if (SessionSpace->GlobalPteEntry == NULL) {
#if DBG
        if (MmDebug & MM_DBG_SESSIONS) {
            DbgPrint("MiSessionCreateInternal: No memory for session self-map\n");
        }
#endif
        MM_BUMP_SESSION_FAILURES (MM_SESSION_FAILURE_NO_SYSPTES);
        goto Failure;
    }

    *(SessionSpace->GlobalPteEntry) = AliasPte;

    SessionSpace->GlobalVirtualAddress = MiGetVirtualAddressMappedByPte (SessionSpace->GlobalPteEntry);

    SessionGlobal = SESSION_GLOBAL(SessionSpace);

    //
    // This list entry is only referenced while within the
    // session space and has session space (not global) addresses.
    //

    InitializeListHead (&SessionSpace->ImageList);

    //
    // Initialize the session space pool.
    //

    Status = MiInitializeSessionPool ();

    if (!NT_SUCCESS(Status)) {
#if DBG
        if (MmDebug & MM_DBG_SESSIONS) {
            DbgPrint("MiSessionCreateInternal: No memory for session pool\n");
        }
#endif
        goto Failure;
    }

    //
    // Initialize the view mapping support - note this must happen after
    // initializing session pool.
    //

    if (MiInitializeSystemSpaceMap (&SessionGlobal->Session) == FALSE) {
#if DBG
        if (MmDebug & MM_DBG_SESSIONS) {
            DbgPrint("MiSessionCreateInternal: No memory for view mapping\n");
        }
#endif
        goto Failure;
    }

    //
    // Use the global virtual address rather than the session space virtual
    // address to set up fields that need to be globally accessible.
    //

    InitializeListHead (&SessionGlobal->WsListEntry);

    InitializeListHead (&SessionGlobal->ProcessList);

    KeInitializeSpinLock (&SessionGlobal->SpinLock);

#if defined(_IA64_)
    KeEnableSessionSharing(&SessionGlobal->SessionMapInfo);
#endif

    return STATUS_SUCCESS;

Failure:

    if (GotCommit == TRUE) {
        MiReturnCommitment (ResidentPages);
        MM_TRACK_COMMIT (MM_DBG_COMMIT_RETURN_SESSION_CREATE_FAILURE1,
                         ResidentPages);
    }

    if (GotPages == TRUE) {

#if defined (_WIN64)

        PointerPpe = MiGetPpeAddress (MI_SESSION_POOL);

        ASSERT (PointerPpe->u.Hard.Valid != 0);

#endif

        PointerPde = MiGetPdeAddress (MI_SESSION_POOL);

        ASSERT (PointerPde->u.Hard.Valid != 0);

        //
        // Note that this call will acquire the session space lock, so the
        // data page better be mapped.
        //

        MiFreeSessionSpaceMap ();

        //
        // Free the initial page table page that was allocated for the
        // paged pool range (if it has been allocated at this point).
        //

        MiFreeSessionPoolBitMaps ();

        if (SessionSpace->GlobalPteEntry) {
            MiReleaseSystemPtes (SessionSpace->GlobalPteEntry,
                                 1,
                                 SystemPteSpace);
            SessionSpace->GlobalPteEntry = (PMMPTE)0;
        }

        LOCK_PFN (OldIrql);

        //
        // Free the session data structure page.
        //

        Pfn1 = MI_PFN_ELEMENT (DataPage);
        MiDecrementShareAndValidCount (Pfn1->PteFrame);
        MI_SET_PFN_DELETED (Pfn1);
        MiDecrementShareCountOnly (DataPage);

        MI_FLUSH_SINGLE_SESSION_TB (MmSessionSpace,
                                    TRUE,
                                    TRUE,
                                    (PHARDWARE_PTE)PointerPte,
                                    ZeroKernelPte.u.Flush,
                                    PreviousPte);

        MM_BUMP_SESS_COUNTER (MM_DBG_SESSION_INITIAL_PAGE_FREE_FAIL1, 1);

        //
        // Free the page table page.
        //

        Pfn1 = MI_PFN_ELEMENT (PageTablePage);
        ASSERT (PageTablePage == Pfn1->PteFrame);
        ASSERT (Pfn1->u2.ShareCount == 2);
        Pfn1->u2.ShareCount -= 1;
        MI_SET_PFN_DELETED (Pfn1);
        MiDecrementShareCountOnly (PageTablePage);

        MI_FLUSH_SINGLE_SESSION_TB (MiGetVirtualAddressMappedByPte(PointerPde),
                                    TRUE,
                                    TRUE,
                                    (PHARDWARE_PTE)PointerPde,
                                    ZeroKernelPte.u.Flush,
                                    PreviousPte);

#if defined (_WIN64)

        //
        // Free the page directory page.
        //

        Pfn1 = MI_PFN_ELEMENT (PageDirectoryPage);
        ASSERT (PageDirectoryPage == Pfn1->PteFrame);
        ASSERT (Pfn1->u2.ShareCount == 1);
        ASSERT (Pfn1->u3.e2.ReferenceCount == 1);
        MI_SET_PFN_DELETED (Pfn1);
        MiDecrementShareCountOnly (PageDirectoryPage);

        MI_FLUSH_SINGLE_SESSION_TB (MiGetVirtualAddressMappedByPte(PointerPpe),
                                    TRUE,
                                    TRUE,
                                    (PHARDWARE_PTE)PointerPpe,
                                    ZeroKernelPte.u.Flush,
                                    PreviousPte);

#endif

        //
        // Now that the PDE has been zeroed, another thread in this process
        // could attempt a session create so be careful not to count on any
        // anything in this particular session.
        //

        MmResidentAvailablePages += (ResidentPages + MI_SESSION_SPACE_WORKING_SET_MINIMUM);

        MM_BUMP_COUNTER (49, ResidentPages + MI_SESSION_SPACE_WORKING_SET_MINIMUM);

        MM_BUMP_SESS_COUNTER (MM_DBG_SESSION_INITIAL_PAGETABLE_FREE_FAIL1, 1);

        UNLOCK_PFN (OldIrql);
    }

    ExAcquireFastMutex (&MiSessionIdMutex);

    ASSERT (RtlCheckBit (MiSessionIdBitmap, *SessionId));
    RtlClearBits (MiSessionIdBitmap, *SessionId, 1);

    ExReleaseFastMutex (&MiSessionIdMutex);

    return STATUS_NO_MEMORY;
}


NTSTATUS
MmSessionCreate(
    OUT PULONG SessionId
    )

/*++

Routine Description:

    Called from NtSetSystemInformation() to create a session space
    in the calling process with the specified SessionId.  An error is returned
    if the calling process already has a session Space.

Arguments:

    SessionId - Supplies a pointer to place the resulting session id in.

Return Value:

    Various NTSTATUS error codes.

Environment:

    Kernel mode, no mutexes held.

--*/

{
    KIRQL OldIrql;
    NTSTATUS Status;
    PEPROCESS CurrentProcess;
#if DBG
    PMMPTE StartPde;
    PMMPTE EndPde;
#endif

    if (MiHydra == FALSE) {
        return STATUS_INVALID_SYSTEM_SERVICE;
    }

    CurrentProcess = PsGetCurrentProcess();

    //
    // A simple check to see if the calling process already has a session space.
    // No need to go through all this if it does.  Creation races are caught
    // below and recovered from regardless.
    //

    if (CurrentProcess->Vm.u.Flags.ProcessInSession == 1) {
        return STATUS_ALREADY_COMMITTED;
    }

    if (CurrentProcess->Vm.u.Flags.SessionLeader == 0) {

        //
        // Only the session manager can create a session.
        //

        return STATUS_INVALID_SYSTEM_SERVICE;
    }

#if DBG
    ASSERT (MmIsAddressValid(MmSessionSpace) == FALSE);

#if defined (_WIN64)
    ASSERT ((MiGetPpeAddress(MmSessionBase))->u.Long == ZeroKernelPte.u.Long);
#else
    StartPde = MiGetPdeAddress (MmSessionBase);
    EndPde = MiGetPdeAddress (MI_SESSION_SPACE_END);

    while (StartPde < EndPde) {
        ASSERT (StartPde->u.Long == ZeroKernelPte.u.Long);
        StartPde += 1;
    }
#endif

#endif

    KeEnterCriticalRegion();

    Status = MiSessionCreateInternal (SessionId);

    if (!NT_SUCCESS(Status)) {
        KeLeaveCriticalRegion();
        return Status;
    }

    LOCK_EXPANSION (OldIrql);

    MiSessionCount += 1;

    UNLOCK_EXPANSION (OldIrql);

    //
    // Add the session space to the working set list.
    //

    Status = MiSessionInitializeWorkingSetList ();

    if (!NT_SUCCESS(Status)) {
        MiDereferenceSession ();
        KeLeaveCriticalRegion();
        return Status;
    }

    KeLeaveCriticalRegion();

    MmSessionSpace->u.Flags.Initialized = 1;

    LOCK_EXPANSION (OldIrql);

    CurrentProcess->Vm.u.Flags.ProcessInSession = 1;

    UNLOCK_EXPANSION (OldIrql);

    return Status;
}


NTSTATUS
MmSessionDelete(
    ULONG SessionId
    )

/*++

Routine Description:

    Called from NtSetSystemInformation() to detach from an existing
    session space in the calling process.  An error is returned
    if the calling process has no session Space.

Arguments:

    SessionId - Supplies the session id to delete.

Return Value:

    STATUS_SUCCESS on success, STATUS_UNABLE_TO_FREE_VM on failure.

    This process will not be able to access session space anymore upon
    a successful return.  If this is the last process in the session then
    the entire session is torn down.

Environment:

    Kernel mode, no mutexes held.

--*/

{
    PEPROCESS CurrentProcess;

    if (MiHydra == FALSE) {
        return STATUS_INVALID_SYSTEM_SERVICE;
    }

    CurrentProcess = PsGetCurrentProcess();

    //
    // See if the calling process has a session space - this must be
    // checked since we can be called via a system service.
    //

    if (CurrentProcess->Vm.u.Flags.ProcessInSession == 0) {
#if DBG
        DbgPrint ("MmSessionDelete: Process %p not in a session\n",
            PsGetCurrentProcess());
        DbgBreakPoint();
#endif
        return STATUS_UNABLE_TO_FREE_VM;
    }

    if (CurrentProcess->Vm.u.Flags.SessionLeader == 0) {

        //
        // Only the session manager can delete a session.  This is because
        // it affects the virtual mappings for all threads within the process
        // when this address space is deleted.  This is different from normal
        // VAD clearing because win32k and other drivers rely on this space.
        //

        return STATUS_UNABLE_TO_FREE_VM;
    }

    ASSERT (MmIsAddressValid(MmSessionSpace) == TRUE);

    if (MmSessionSpace->SessionId != SessionId) {
#if DBG
        DbgPrint("MmSessionDelete: Wrong SessionId! Own %d, Ask %d\n",
            MmSessionSpace->SessionId,
            SessionId);
        DbgBreakPoint();
#endif
        return STATUS_UNABLE_TO_FREE_VM;
    }

    KeEnterCriticalRegion ();

    MiDereferenceSession ();

    KeLeaveCriticalRegion ();

    return STATUS_SUCCESS;
}


VOID
MiAttachSession(
    IN PMM_SESSION_SPACE SessionGlobal
    )

/*++

Routine Description:

    Attaches to the specified session space.

Arguments:

    SessionGlobal - Supplies a pointer to the session to attach to.

Return Value:

    None.

Environment:

    Kernel mode.  No locks held.  Current process must not have a session
    space - ie: the caller should be the system process or smss.exe.

--*/

{
    PMMPTE PointerPde;
#if DBG
    PMMPTE EndPde;

    ASSERT (MiHydra == TRUE);

    ASSERT (PsGetCurrentProcess()->Vm.u.Flags.ProcessInSession == 0);

#if defined (_WIN64)

    PointerPde = MiGetPpeAddress (MmSessionBase);
    ASSERT (PointerPde->u.Long == ZeroKernelPte.u.Long);

#else
    PointerPde = MiGetPdeAddress (MmSessionBase);
    EndPde = MiGetPdeAddress (MI_SESSION_SPACE_END);

    while (PointerPde < EndPde) {
        ASSERT (PointerPde->u.Long == ZeroKernelPte.u.Long);
        PointerPde += 1;
    }
#endif

#endif

#if defined (_WIN64)

    PointerPde = MiGetPpeAddress (MmSessionBase);
    MI_WRITE_VALID_PTE (PointerPde, SessionGlobal->PageDirectory);

#else

    PointerPde = MiGetPdeAddress (MmSessionBase);

    RtlCopyMemory (PointerPde,
                   &SessionGlobal->PageTables[0],
                   MI_SESSION_SPACE_PAGE_TABLES * sizeof (MMPTE));
#endif

#if defined(_IA64_)
    KeAttachSessionSpace(&SessionGlobal->SessionMapInfo);
#endif
}


VOID
MiDetachSession(
    VOID
    )

/*++

Routine Description:

    Detaches from the specified session space.

Arguments:

    None.

Return Value:

    None.

Environment:

    Kernel mode.  No locks held.  Current process must not have a session
    space to return to - ie: this should be the system process.

--*/

{
    PMMPTE PointerPde;
    PMMPTE EndPde;
    KIRQL OldIrql;

    ASSERT (MiHydra == TRUE);

    ASSERT (PsGetCurrentProcess()->Vm.u.Flags.ProcessInSession == 0);
    ASSERT (MmIsAddressValid(MmSessionSpace) == TRUE);

#if defined (_WIN64)
    PointerPde = MiGetPpeAddress (MmSessionBase);
    PointerPde->u.Long = ZeroKernelPte.u.Long;
#else
    PointerPde = MiGetPdeAddress (MmSessionBase);

    EndPde = MiGetPdeAddress (MI_SESSION_SPACE_END);

    RtlZeroMemory (PointerPde, MI_SESSION_SPACE_PAGE_TABLES * sizeof (MMPTE));
#endif

    MI_FLUSH_SESSION_TB (OldIrql);

#if defined(_IA64_)
    KeDetachSessionSpace();
#endif
}

#if DBG

VOID
MiCheckSessionVirtualSpace(
    IN PVOID VirtualAddress,
    IN ULONG NumberOfBytes
    )

/*++

Routine Description:

    Used to verify that no drivers fail to clean up their session allocations.

Arguments:

    VirtualAddress - Supplies the starting virtual address to check.

    NumberOfBytes - Supplies the number of bytes to check.

Return Value:

    TRUE if all the PTEs have been freed, FALSE if not.

Environment:

    Kernel mode.  APCs disabled.

--*/

{
    PMMPTE StartPde;
    PMMPTE EndPde;
    PMMPTE StartPte;
    PMMPTE EndPte;
    ULONG Index;

    //
    // Check the specified region.  Everything should have been cleaned up
    // already.
    //

#if defined (_WIN64)
    ASSERT (MiGetPpeAddress (VirtualAddress)->u.Hard.Valid == 1);
#endif

    StartPde = MiGetPdeAddress (VirtualAddress);
    EndPde = MiGetPdeAddress ((PVOID)((PCHAR)VirtualAddress + NumberOfBytes - 1));

    StartPte = MiGetPteAddress (VirtualAddress);
    EndPte = MiGetPteAddress ((PVOID)((PCHAR)VirtualAddress + NumberOfBytes - 1));

    Index = (ULONG)(StartPde - MiGetPdeAddress (MmSessionBase));

#if defined (_WIN64)
    while (StartPde <= EndPde && StartPde->u.Long == 0)
#else
    while (StartPde <= EndPde && MmSessionSpace->PageTables[Index].u.Long == 0)
#endif
    {
        StartPde += 1;
        Index += 1;
        StartPte = MiGetVirtualAddressMappedByPte (StartPde);
    }

    while (StartPte <= EndPte) {

        if (MiIsPteOnPdeBoundary(StartPte)) {

            StartPde = MiGetPteAddress (StartPte);
            Index = (ULONG)(StartPde - MiGetPdeAddress (MmSessionBase));

#if defined (_WIN64)
            while (StartPde <= EndPde && StartPde->u.Long == 0)
#else
            while (StartPde <= EndPde && MmSessionSpace->PageTables[Index].u.Long == 0)
#endif
            {
                Index += 1;
                StartPde += 1;
                StartPte = MiGetVirtualAddressMappedByPte (StartPde);
            }
            if (StartPde > EndPde) {
                break;
            }
        }

        if (StartPte->u.Long != 0 && StartPte->u.Long != MM_KERNEL_NOACCESS_PTE) {
            DbgPrint("MiCheckSessionVirtualSpace: StartPte 0x%p is still valid! 0x%p, VA 0x%p\n",
                StartPte,
                StartPte->u.Long,
                MiGetVirtualAddressMappedByPte(StartPte));

            DbgBreakPoint();
        }
        StartPte += 1;
    }
}
#endif


VOID
MiSessionDeletePde(
    IN PMMPTE Pde,
    IN BOOLEAN WorkingSetInitialized,
    IN PMMPTE SelfMapPde
    )

/*++

Routine Description:

    Used to delete a page directory entry from a session space.

Arguments:

    Pde - Supplies the page directory entry to delete.

    WorkingSetInitialized - Supplies TRUE if the working set has been
                            initialized.

    SelfMapPde - Supplies the page directory entry that contains the self map
                 session page.


Return Value:

    None.

Environment:

    Kernel mode.  PFN lock held.

--*/

{
    PMMPFN Pfn1;
    PMMPFN Pfn2;
    KIRQL OldIrql2;
    PFN_NUMBER PageFrameIndex;
    BOOLEAN SelfMapPage;
#if DBG
    ULONG i;
    PMMPTE PointerPte;
#endif

    if (Pde->u.Long == ZeroKernelPte.u.Long) {
        return;
    }

    SelfMapPage = (Pde == SelfMapPde ? TRUE : FALSE);

    ASSERT (Pde->u.Hard.Valid == 1);

    PageFrameIndex = MI_GET_PAGE_FRAME_FROM_PTE (Pde);
    Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);

#if DBG

    ASSERT (PageFrameIndex <= MmHighestPhysicalPage);

    if (WorkingSetInitialized == TRUE) {
        ASSERT (Pfn1->u1.WsIndex);
    }

    ASSERT (Pfn1->u3.e1.PrototypePte == 0);
    ASSERT (Pfn1->u3.e2.ReferenceCount == 1);
    ASSERT (Pfn1->PteFrame <= MmHighestPhysicalPage);

    Pfn2 = MI_PFN_ELEMENT (Pfn1->PteFrame);

    //
    // Verify the containing page table is still the page
    // table page mapping the session data structure.
    //

    if (SelfMapPage == FALSE) {

        //
        // Note these ASSERTs will fail if win32k leaks pool.
        //

        ASSERT (Pfn1->u2.ShareCount == 1);
#if !defined (_WIN64)

        //
        // 32-bit NT points the additional page tables at the master.
        // 64-bit NT doesn't need to use this trick as there is always
        // an additional hierarchy level.
        //

        ASSERT (Pfn1->PteFrame == MI_GET_PAGE_FRAME_FROM_PTE (SelfMapPde));
#endif
        ASSERT (Pfn2->u2.ShareCount > 2);
    }
    else {
        ASSERT (Pfn1 == Pfn2);
        ASSERT (Pfn1->u2.ShareCount == 2);
    }

    //
    // Make sure the page table page is zeroed.  It should always be zero
    // unless win32k leaks pool or random bits get set.
    //

    PointerPte = (PMMPTE)MiMapPageInHyperSpace (PageFrameIndex, &OldIrql2);

    i = 0;
    while (i < PTE_PER_PAGE) {
        if (PointerPte->u.Long != 0 && PointerPte->u.Long != MM_KERNEL_NOACCESS_PTE) {
            DbgPrint("MM: Deleting session page table: Index %d PTE %p is not zero! %p\n",
                i,
                PointerPte,
                PointerPte->u.Long);
            DbgBreakPoint();
        }
        PointerPte += 1;
        i += 1;
    }

    MiUnmapPageInHyperSpace (OldIrql2);

#endif // DBG

    if (SelfMapPage == FALSE) {
        MiDecrementShareAndValidCount (Pfn1->PteFrame);
    }
    else {
        ASSERT (Pfn1 == Pfn2);
        ASSERT (Pfn1->u2.ShareCount == 2);
        Pfn1->u2.ShareCount -= 1;
    }

    MI_SET_PFN_DELETED (Pfn1);
    MiDecrementShareCountOnly (PageFrameIndex);
}

BOOLEAN
MiDereferenceSession(
    VOID
    )

/*++

Routine Description:

    Decrement this process' reference count to the session, unmapping access
    to the session for the current process.  If this is the last process
    reference to this session, then the entire session will be destroyed upon
    return.  This includes unloading drivers, unmapping pools, freeing
    page tables, etc.

Arguments:

    None.

Return Value:

    TRUE if the session was deleted, FALSE if only this process' access to
    the session was deleted.

Environment:

    Kernel mode, no mutexes held, APCs disabled.

--*/

{
    KIRQL OldIrql;
    ULONG Index;
    ULONG CountReleased;
    ULONG SessionId;
    PFN_NUMBER PageFrameIndex;
    ULONG SessionDataPdeIndex;
    KEVENT Event;
    MMPTE PreviousPte;
    PMMPFN Pfn1;
    PMMPTE PointerPpe;
    PMMPTE PointerPte;
    PMMPTE EndPte;
    PMMPTE GlobalPteEntrySave;
    PMMPTE StartPde;
    PMMPTE EndPde;
    PMM_SESSION_SPACE SessionGlobal;
    MMPTE SavePageTables[MI_SESSION_SPACE_PAGE_TABLES];
    BOOLEAN WorkingSetWasInitialized;
    ULONG AttachCount;
    PEPROCESS Process;

    ASSERT ((PsGetCurrentProcess()->Vm.u.Flags.ProcessInSession == 1) ||
            ((MmSessionSpace->u.Flags.Initialized == 0) && (PsGetCurrentProcess()->Vm.u.Flags.SessionLeader == 1) && (MmSessionSpace->ReferenceCount == 1)));

    SessionId = MmSessionSpace->SessionId;

    ASSERT (RtlCheckBit (MiSessionIdBitmap, SessionId));

    LOCK_SESSION (OldIrql);

    if (MmSessionSpace->ReferenceCount > 1) {

        MmSessionSpace->ReferenceCount -= 1;

        UNLOCK_SESSION (OldIrql);

        Process = PsGetCurrentProcess();

        LOCK_EXPANSION (OldIrql);

        Process->Vm.u.Flags.ProcessInSession = 0;

        UNLOCK_EXPANSION (OldIrql);

        //
        // Don't delete any non-smss session space mappings here.  Let them
        // live on through process death.  This handles the case where
        // MmDispatchWin32Callout picks csrss - csrss has exited as it's not
        // the last process (smss is).  smss is simultaneously detaching from
        // the session and since it is the last process, it's waiting on
        // the AttachCount below.  The dispatch callout ends up in csrss but
        // has no way to synchronize against csrss exiting through this path
        // as the object reference count doesn't stop it.  So leave the
        // session space mappings alive so the callout can execute through
        // the remains of csrss.
        //
        // Note that when smss detaches, the address space must get cleared
        // here so that subsequent session creations by smss will succeed.
        //

        if (Process->Vm.u.Flags.SessionLeader == 1) {

#if defined (_WIN64)
            StartPde = MiGetPpeAddress (MmSessionBase);
            StartPde->u.Long = ZeroKernelPte.u.Long;
#else
            StartPde = MiGetPdeAddress (MmSessionBase);
            EndPde = MiGetPdeAddress (MI_SESSION_SPACE_END);
            RtlZeroMemory (StartPde, (EndPde - StartPde) * sizeof(MMPTE));
#endif

            //
            // Flush the session space TB entries.
            //
    
            MI_FLUSH_SESSION_TB (OldIrql);
        }

#if defined(_IA64_)
        KeDetachSessionSpace();
#endif

        return FALSE;
    }

    //
    // Mark it as being deleted.
    //

    MmSessionSpace->u.Flags.BeingDeleted = 1;

    //
    // This is the final dereference.  We could be any process
    // including SMSS when a session space load fails.  Note also that
    // processes can terminate in any order as well.
    //

    UNLOCK_SESSION (OldIrql);

    SessionGlobal = SESSION_GLOBAL (MmSessionSpace);

    LOCK_EXPANSION (OldIrql);

    //
    // Wait for any cross-session process attaches to detach.  Refuse
    // subsequent attempts to cross-session attach so the address invalidation
    // code doesn't surprise an ongoing or subsequent attachee.
    //

    ASSERT (MmSessionSpace->u.Flags.DeletePending == 0);

    MmSessionSpace->u.Flags.DeletePending = 1;

    AttachCount = MmSessionSpace->AttachCount;

    if (AttachCount) {

        KeInitializeEvent (&MmSessionSpace->AttachEvent,
                           NotificationEvent,
                           FALSE);

        UNLOCK_EXPANSION (OldIrql);

        KeWaitForSingleObject( &MmSessionSpace->AttachEvent,
                               WrVirtualMemory,
                               KernelMode,
                               FALSE,
                               (PLARGE_INTEGER)NULL);

        LOCK_EXPANSION (OldIrql);

        ASSERT (MmSessionSpace->u.Flags.DeletePending == 1);
        ASSERT (MmSessionSpace->AttachCount == 0);
    }

    if (MmSessionSpace->Vm.u.Flags.BeingTrimmed) {

        //
        // Initialize an event and put the event address
        // in the VmSupport.  When the trimming is complete,
        // this event will be set.
        //

        KeInitializeEvent(&Event, NotificationEvent, FALSE);

        MmSessionSpace->Vm.WorkingSetExpansionLinks.Blink = (PLIST_ENTRY)&Event;

        //
        // Release the mutex and wait for the event.
        //

        KeEnterCriticalRegion();
        UNLOCK_EXPANSION_AND_THEN_WAIT (OldIrql);

        KeWaitForSingleObject(&Event,
                              WrVirtualMemory,
                              KernelMode,
                              FALSE,
                              (PLARGE_INTEGER)NULL);
        KeLeaveCriticalRegion();

        LOCK_EXPANSION (OldIrql);
    }
    else if (MmSessionSpace->u.Flags.WorkingSetInserted == 1) {

        //
        // Remove this session from the session list and the working
        // set list.
        //

        RemoveEntryList (&SessionGlobal->Vm.WorkingSetExpansionLinks);

        MmSessionSpace->u.Flags.WorkingSetInserted = 0;
    }

    if (MmSessionSpace->u.Flags.SessionListInserted == 1) {

        RemoveEntryList (&SessionGlobal->WsListEntry);

        MmSessionSpace->u.Flags.SessionListInserted = 0;
    }

    MiSessionCount -= 1;

    UNLOCK_EXPANSION (OldIrql);

#if DBG
    if (PsGetCurrentProcess()->Vm.u.Flags.SessionLeader == 0) {
        ASSERT (MmSessionSpace->ProcessOutSwapCount == 0);
        ASSERT (MmSessionSpace->ReferenceCount == 1);
    }
#endif

    MM_SNAP_SESS_MEMORY_COUNTERS(0);

    //
    // If an unload function has been registered for WIN32K.SYS,
    // call it now before we force an unload on any modules.  WIN32K.SYS
    // is responsible for calling any other loaded modules that have
    // unload routines to be run.  Another option is to have the other
    // session drivers register a DLL initialize/uninitialize pair on load.
    //

    if (MmSessionSpace->Win32KDriverObject.DriverUnload ) {
        MmSessionSpace->Win32KDriverObject.DriverUnload (&MmSessionSpace->Win32KDriverObject);
    }

    //
    // Now that all modules have had their unload routine(s)
    // called, check for pool leaks before unloading the images.
    //

    MiCheckSessionPoolAllocations ();

    ASSERT (MmSessionSpace->ReferenceCount == 1);

    MM_SNAP_SESS_MEMORY_COUNTERS(1);

    //
    // Destroy the view mapping structures.
    //

    MiFreeSessionSpaceMap ();

    MM_SNAP_SESS_MEMORY_COUNTERS(2);

    //
    // Walk down the list of modules we have loaded dereferencing them.
    //
    // This allows us to force an unload of any kernel images loaded by
    // the session so we do not have any virtual space and paging
    // file leaks.
    //

    MiSessionUnloadAllImages ();

    MM_SNAP_SESS_MEMORY_COUNTERS(3);

    //
    // Destroy the session space bitmap structure
    //

    MiFreeSessionPoolBitMaps ();

    MM_SNAP_SESS_MEMORY_COUNTERS(4);

    //
    // Reference the session space structure using its global
    // kernel PTE based address. This is to avoid deleting it out
    // from underneath ourselves.
    //

    GlobalPteEntrySave = MmSessionSpace->GlobalPteEntry;

    //
    // Sweep the individual regions in their proper order.
    //

#if DBG

    //
    // Check the executable image region. All images
    // should have been unloaded by the image handler.
    //

    MiCheckSessionVirtualSpace ((PVOID)MI_SESSION_IMAGE_START,
                                MI_SESSION_IMAGE_SIZE);
#endif

    MM_SNAP_SESS_MEMORY_COUNTERS(5);

#if DBG

    //
    // Check the view region. All views should have been cleaned up already.
    //

    MiCheckSessionVirtualSpace ((PVOID)MI_SESSION_VIEW_START,
                                MI_SESSION_VIEW_SIZE);
#endif

    //
    // Save the page tables in the session space structure since
    // we are going to tear it down.
    //

#if defined (_WIN64)
    RtlCopyMemory (SavePageTables,
                   MiGetPdeAddress (MmSessionBase),
                   MI_SESSION_SPACE_PAGE_TABLES * sizeof (MMPTE));
#else
    RtlCopyMemory (SavePageTables,
                   MmSessionSpace->PageTables,
                   MI_SESSION_SPACE_PAGE_TABLES * sizeof (MMPTE));
#endif

    MM_SNAP_SESS_MEMORY_COUNTERS(6);

#if DBG
    //
    // Check everything possible before the remaining virtual address space
    // is torn down.  In this way if anything is amiss, the data can be
    // more easily examined.
    //

    Pfn1 = MI_PFN_ELEMENT (MmSessionSpace->SessionPageDirectoryIndex);

    //
    // This should be greater than 1 because working set page tables are
    // using this as their parent as well.
    //

    ASSERT (Pfn1->u2.ShareCount > 1);
#endif

    CountReleased = 0;

    if (MmSessionSpace->u.Flags.HasWsLock == 1) {

        PointerPte = MiGetPteAddress (MI_SESSION_SPACE_WS);
        EndPte = MiGetPteAddress (MmSessionSpace->Vm.VmWorkingSetList->HighestPermittedHashAddress);

        for ( ; PointerPte < EndPte; PointerPte += 1) {

            if (PointerPte->u.Long) {

                ASSERT (PointerPte->u.Hard.Valid == 1);
                MM_BUMP_SESS_COUNTER (MM_DBG_SESSION_WS_PAGE_FREE, 1);

                MmSessionSpace->CommittedPages -= 1;
                MmSessionSpace->NonPagablePages -= 1;
                CountReleased += 1;
            }
        }
        WorkingSetWasInitialized = TRUE;
        MmSessionSpace->u.Flags.HasWsLock = 0;
    }
    else {
        WorkingSetWasInitialized = FALSE;
    }

    //
    // Account for the session data structure data page.  For NT64, the page
    // directory page is also accounted for here.
    //

#if defined (_WIN64)
    MM_BUMP_SESS_COUNTER (MM_DBG_SESSION_INITIAL_PAGE_FREE, 2);
    MmSessionSpace->CommittedPages -= 2;
    MmSessionSpace->NonPagablePages -= 2;
    CountReleased += 2;
#else
    MM_BUMP_SESS_COUNTER (MM_DBG_SESSION_INITIAL_PAGE_FREE, 1);
    MmSessionSpace->CommittedPages -= 1;
    MmSessionSpace->NonPagablePages -= 1;
    CountReleased += 1;
#endif

    //
    // Account for any needed session space page tables.
    //

    for (Index = 0; Index < MI_SESSION_SPACE_PAGE_TABLES; Index += 1) {

        StartPde = &SavePageTables[Index];

        if (StartPde->u.Long != ZeroKernelPte.u.Long) {
            MM_BUMP_SESS_COUNTER (MM_DBG_SESSION_PAGETABLE_FREE, 1);
            MmSessionSpace->CommittedPages -= 1;
            MmSessionSpace->NonPagablePages -= 1;
            CountReleased += 1;
        }
    }

    ASSERT (MmSessionSpace->NonPagablePages == 0);

    //
    // Note that whenever win32k or drivers loaded by it leak pool, the
    // ASSERT below will be triggered.
    //

    ASSERT (MmSessionSpace->CommittedPages == 0);

    MiReturnCommitment (CountReleased);

    MM_TRACK_COMMIT (MM_DBG_COMMIT_RETURN_SESSION_DEREFERENCE, CountReleased);

    //
    // Sweep the working set entries.
    // No more accesses to the working set or its lock are allowed.
    //

    if (WorkingSetWasInitialized == TRUE) {
        ExDeleteResource (&SessionGlobal->WsLock);

        PointerPte = MiGetPteAddress (MI_SESSION_SPACE_WS);
        EndPte = MiGetPteAddress (MmSessionSpace->Vm.VmWorkingSetList->HighestPermittedHashAddress);

        for ( ; PointerPte < EndPte; PointerPte += 1) {

            if (PointerPte->u.Long) {

                ASSERT (PointerPte->u.Hard.Valid == 1);

                //
                // Delete the page.
                //

                PageFrameIndex = MI_GET_PAGE_FRAME_FROM_PTE (PointerPte);

                Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);

                //
                // Each page should still be locked in the session working set.
                //

                LOCK_PFN (OldIrql);

                ASSERT (Pfn1->u3.e2.ReferenceCount == 1);

                MiDecrementShareAndValidCount (Pfn1->PteFrame);
                MI_SET_PFN_DELETED (Pfn1);
                MiDecrementShareCountOnly (PageFrameIndex);
                MI_WRITE_INVALID_PTE (PointerPte, ZeroKernelPte);

                MmResidentAvailablePages += 1;
                MM_BUMP_COUNTER(52, 1);

                UNLOCK_PFN (OldIrql);
            }
        }
    }

#if defined(_IA64_)
    KeDisableSessionSharing(&SessionGlobal->SessionMapInfo);
#endif

    //
    // Now delete the session space structure itself.
    // No more accesses to MmSessionSpace after this.
    //

    PointerPte = MiGetPteAddress (MmSessionSpace);

    //
    // Delete the page.
    //

    PageFrameIndex = MI_GET_PAGE_FRAME_FROM_PTE (PointerPte);

    Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);

    //
    // Make sure this page is still locked.
    //

    ASSERT (Pfn1->u1.WsIndex == 0);

    LOCK_PFN (OldIrql);

    ASSERT (Pfn1->u3.e2.ReferenceCount == 1);

    ASSERT (Pfn1->PteFrame == MmSessionSpace->SessionPageDirectoryIndex);

    MiDecrementShareAndValidCount (Pfn1->PteFrame);
    MI_SET_PFN_DELETED (Pfn1);
    MiDecrementShareCountOnly (PageFrameIndex);
    MI_WRITE_INVALID_PTE (PointerPte, ZeroKernelPte);

    MmResidentAvailablePages += CountReleased;

    MM_BUMP_COUNTER(53, CountReleased);

    MmResidentAvailablePages += MI_SESSION_SPACE_WORKING_SET_MINIMUM;

    MM_BUMP_COUNTER(56, MI_SESSION_SPACE_WORKING_SET_MINIMUM);

    UNLOCK_PFN (OldIrql);

    StartPde = MiGetPdeAddress (MmSessionBase);

    EndPde = MiGetPdeAddress (MI_SESSION_SPACE_END);

    RtlZeroMemory (StartPde, (EndPde - StartPde) * sizeof(MMPTE));

    //
    // Flush the session space TB entries.
    //

    MI_FLUSH_SESSION_TB (OldIrql);

    //
    // Free the self-map PTE.
    //

    MiReleaseSystemPtes (GlobalPteEntrySave, 1, SystemPteSpace);

    //
    // Delete page table pages.  Note that the page table page mapping the
    // session space data structure is done last so that we can apply
    // various ASSERTs in the DeletePde routine.
    //

    SessionDataPdeIndex = MiGetPdeSessionIndex (MmSessionSpace);

    LOCK_PFN (OldIrql);

    for (Index = 0; Index < MI_SESSION_SPACE_PAGE_TABLES; Index += 1) {

        if (Index == SessionDataPdeIndex) {

            //
            // The self map entry must be done last.
            //

            continue;
        }

        MiSessionDeletePde (&SavePageTables[Index],
                            WorkingSetWasInitialized,
                            &SavePageTables[SessionDataPdeIndex]);
    }

    MiSessionDeletePde (&SavePageTables[SessionDataPdeIndex],
                        WorkingSetWasInitialized,
                        &SavePageTables[SessionDataPdeIndex]);

#if defined (_WIN64)

    //
    // Delete the session page directory page.
    //

    PointerPpe = MiGetPpeAddress (MmSessionSpace);
    PageFrameIndex = MI_GET_PAGE_FRAME_FROM_PTE (PointerPpe);
    Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);
    MI_SET_PFN_DELETED (Pfn1);
    MiDecrementShareCountOnly (PageFrameIndex);

    MI_FLUSH_SINGLE_SESSION_TB (MiGetPdeAddress (MmSessionSpace),
                                TRUE,
                                TRUE,
                                (PHARDWARE_PTE)PointerPpe,
                                ZeroKernelPte.u.Flush,
                                PreviousPte);
#endif

    UNLOCK_PFN (OldIrql);

    ExAcquireFastMutex (&MiSessionIdMutex);

    ASSERT (RtlCheckBit (MiSessionIdBitmap, SessionId));
    RtlClearBits (MiSessionIdBitmap, SessionId, 1);

    ExReleaseFastMutex (&MiSessionIdMutex);

    LOCK_EXPANSION (OldIrql);

    PsGetCurrentProcess()->Vm.u.Flags.ProcessInSession = 0;

    UNLOCK_EXPANSION (OldIrql);

    //
    // The session space has been deleted and all TB flushing is complete.
    //

    return TRUE;
}

NTSTATUS
MiSessionCommitImagePages(
    IN PVOID VirtualAddress,
    IN SIZE_T NumberOfBytes
    )

/*++

Routine Description:

    This routine commits virtual memory within the current session space with
    backing pages.  The virtual addresses within session space are
    allocated with a separate facility in the image management facility.
    This is because images must be at a unique system wide virtual address.

Arguments:

    VirtualAddress - Supplies the first virtual address to commit.

    NumberOfBytes - Supplies the number of bytes to commit.

Return Value:

    STATUS_SUCCESS if all went well, STATUS_NO_MEMORY if the current process
    has no session.

Environment:

    Kernel mode, MmSystemLoadLock held.

--*/

{
    KIRQL WsIrql;
    KIRQL OldIrql;
    ULONG Color;
    PFN_NUMBER SizeInPages;
    PMMPFN Pfn1;
    ULONG_PTR AllocationStart;
    PFN_NUMBER PageFrameIndex;
    NTSTATUS Status;
    PMMPTE StartPte, EndPte;
    MMPTE TempPte;

    SYSLOAD_LOCK_OWNED_BY_ME ();

    if (NumberOfBytes == 0) {
        return STATUS_SUCCESS;
    }

    if (MmIsAddressValid(MmSessionSpace) == FALSE) {
#if DBG
        DbgPrint ("MiSessionCommitImagePages: No session space!\n");
#endif
        return STATUS_NO_MEMORY;
    }

    ASSERT (((ULONG_PTR)VirtualAddress % PAGE_SIZE) == 0);
    ASSERT ((NumberOfBytes % PAGE_SIZE) == 0);

    SizeInPages = (PFN_NUMBER)(NumberOfBytes >> PAGE_SHIFT);

    //
    // Calculate pages needed.
    //

    AllocationStart = (ULONG_PTR)VirtualAddress;

    //
    // Lock the session space working set.
    //

    LOCK_SESSION_SPACE_WS(WsIrql);

    //
    // Make sure we have page tables for the PTE
    // entries we must fill in the session space structure.
    //

    Status = MiSessionCommitPageTables ((PVOID)AllocationStart,
                                        (PVOID)(AllocationStart + NumberOfBytes));

    if (!NT_SUCCESS(Status)) {
#if DBG
        if (MmDebug & MM_DBG_SESSIONS) {
            DbgPrint("MiSessionCommitImagePages: Could not commit pagetables, Not enough memory! MmResidentAvailablePages %d, SizeInPages %d\n",
                MmResidentAvailablePages,
                SizeInPages);
        }
#endif
        UNLOCK_SESSION_SPACE_WS(WsIrql);
        return STATUS_NO_MEMORY;
    }

    //
    // go into loop allocating them and placing them into the page
    // tables.
    //

    StartPte = MiGetPteAddress (AllocationStart);
    EndPte = MiGetPteAddress (AllocationStart + NumberOfBytes);

    if (MiChargeCommitment (SizeInPages, NULL) == FALSE) {
        MM_BUMP_SESSION_FAILURES (MM_SESSION_FAILURE_NO_COMMIT);
        UNLOCK_SESSION_SPACE_WS(WsIrql);
        return STATUS_NO_MEMORY;
    }

    MM_TRACK_COMMIT (MM_DBG_COMMIT_SESSION_IMAGE_PAGES, SizeInPages);

    TempPte = ValidKernelPteLocal;

    LOCK_PFN (OldIrql);

    //
    // Check to make sure the physical pages are available.
    //

    if ((SPFN_NUMBER)SizeInPages > MI_NONPAGABLE_MEMORY_AVAILABLE() - 20) {
        UNLOCK_PFN (OldIrql);
        MiReturnCommitment (SizeInPages);
        MM_TRACK_COMMIT (MM_DBG_COMMIT_RETURN_SESSION_IMAGE_FAILURE1, SizeInPages);
        MM_BUMP_SESSION_FAILURES (MM_SESSION_FAILURE_NO_RESIDENT);
        UNLOCK_SESSION_SPACE_WS(WsIrql);
        return STATUS_NO_MEMORY;
    }

    MmResidentAvailablePages -= SizeInPages;

    MM_BUMP_COUNTER(45, SizeInPages);

    while (StartPte < EndPte) {

        ASSERT (StartPte->u.Long == ZeroKernelPte.u.Long);

        MiEnsureAvailablePageOrWait (NULL, NULL);

        Color = MI_GET_PAGE_COLOR_FROM_SESSION (MmSessionSpace);

        PageFrameIndex = MiRemoveZeroPageIfAny (Color);
        if (PageFrameIndex == 0) {
            PageFrameIndex = MiRemoveAnyPage (Color);
            UNLOCK_PFN (OldIrql);
            MiZeroPhysicalPage (PageFrameIndex, Color);
            LOCK_PFN (OldIrql);
        }

        TempPte.u.Hard.PageFrameNumber = PageFrameIndex;
        MI_WRITE_VALID_PTE (StartPte, TempPte);

        Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);

        ASSERT (Pfn1->u1.WsIndex == 0);

        MiInitializePfn (PageFrameIndex, StartPte, 1);

        KeFillEntryTb ((PHARDWARE_PTE) StartPte, (PMMPTE)AllocationStart, FALSE);

        StartPte += 1;
        AllocationStart += PAGE_SIZE;
    }

    UNLOCK_PFN (OldIrql);

    MmSessionSpace->CommittedPages += SizeInPages;
    MmSessionSpace->NonPagablePages += SizeInPages;

    MM_BUMP_SESS_COUNTER (MM_DBG_SESSION_DRIVER_PAGES_LOCKED, SizeInPages);

    UNLOCK_SESSION_SPACE_WS(WsIrql);

    return STATUS_SUCCESS;
}

NTSTATUS
MiSessionCommitPageTables(
    IN PVOID StartVa,
    IN PVOID EndVa
    )

/*++

Routine Description:

    Fill in page tables covering the specified virtual address range.

Arguments:

    StartVa - Supplies a starting virtual address.

    EndVa - Supplies an ending virtual address.

Return Value:

    STATUS_SUCCESS on success, STATUS_NO_MEMORY on failure.

Environment:

    Kernel mode.  Session space working set mutex held.

--*/

{
    KIRQL OldIrql;
    ULONG Color;
    ULONG Index;
    PMMPTE StartPde, EndPde;
    MMPTE TempPte;
    PMMPFN Pfn1;
    ULONG Entry;
    ULONG SwapEntry;
    PFN_NUMBER SizeInPages;
    PFN_NUMBER PageTablePage;
    PVOID SessionPte;
    PMMWSL WorkingSetList;
    CHAR SavePageTables[MI_SESSION_SPACE_PAGE_TABLES];

    ASSERT (MmIsAddressValid(MmSessionSpace) == TRUE);

    MM_SESSION_SPACE_WS_LOCK_ASSERT();

    ASSERT (StartVa >= (PVOID)MmSessionBase);
    ASSERT (EndVa < (PVOID)MI_SESSION_SPACE_END);

    //
    // Allocate the page table pages, loading them
    // into the current process's page directory.
    //

    StartPde = MiGetPdeAddress (StartVa);
    EndPde = MiGetPdeAddress (EndVa);
    Index = MiGetPdeSessionIndex (StartVa);

    SizeInPages = 0;

    while (StartPde <= EndPde) {
#if defined (_WIN64)
        if (StartPde->u.Long == ZeroKernelPte.u.Long)
#else
        if (MmSessionSpace->PageTables[Index].u.Long == ZeroKernelPte.u.Long)
#endif
        {
            SizeInPages += 1;
        }
        StartPde += 1;
        Index += 1;
    }

    if (SizeInPages == 0) {
        return STATUS_SUCCESS;
    }

    if (MiChargeCommitment (SizeInPages, NULL) == FALSE) {
        MM_BUMP_SESSION_FAILURES (MM_SESSION_FAILURE_NO_COMMIT);
        return STATUS_NO_MEMORY;
    }

    MM_TRACK_COMMIT (MM_DBG_COMMIT_SESSION_PAGETABLE_PAGES, SizeInPages);

    StartPde = MiGetPdeAddress (StartVa);
    Index = MiGetPdeSessionIndex (StartVa);

    TempPte = ValidKernelPdeLocal;

    LOCK_PFN (OldIrql);

    //
    // Check to make sure the physical pages are available.
    //

    if ((SPFN_NUMBER)SizeInPages > MI_NONPAGABLE_MEMORY_AVAILABLE() - 20) {
        UNLOCK_PFN (OldIrql);
        MiReturnCommitment (SizeInPages);
        MM_TRACK_COMMIT (MM_DBG_COMMIT_RETURN_SESSION_PAGETABLE_PAGES, SizeInPages);
        MM_BUMP_SESSION_FAILURES (MM_SESSION_FAILURE_NO_RESIDENT);
        return STATUS_NO_MEMORY;
    }

    MmResidentAvailablePages -= SizeInPages;

    MM_BUMP_SESS_COUNTER (MM_DBG_SESSION_PAGETABLE_ALLOC, SizeInPages);

    MM_BUMP_COUNTER(44, SizeInPages);

    WorkingSetList = MmSessionSpace->Vm.VmWorkingSetList;

    RtlZeroMemory (SavePageTables, sizeof (SavePageTables));

    while (StartPde <= EndPde) {

#if defined (_WIN64)
        if (StartPde->u.Long == ZeroKernelPte.u.Long)
#else
        if (MmSessionSpace->PageTables[Index].u.Long == ZeroKernelPte.u.Long)
#endif
        {

            ASSERT (StartPde->u.Hard.Valid == 0);

            SavePageTables[Index] = 1;

            MiEnsureAvailablePageOrWait (NULL, NULL);

            Color = MI_GET_PAGE_COLOR_FROM_SESSION (MmSessionSpace);

            PageTablePage = MiRemoveZeroPageIfAny (Color);
            if (PageTablePage == 0) {
                PageTablePage = MiRemoveAnyPage (Color);
                UNLOCK_PFN (OldIrql);
                MiZeroPhysicalPage (PageTablePage, Color);
                LOCK_PFN (OldIrql);
            }

            TempPte.u.Hard.PageFrameNumber = PageTablePage;
            MI_WRITE_VALID_PTE (StartPde, TempPte);

#if !defined (_WIN64)
            MmSessionSpace->PageTables[Index] = TempPte;
#endif
            MmSessionSpace->NonPagablePages += 1;
            MmSessionSpace->CommittedPages += 1;

            MiInitializePfnForOtherProcess (PageTablePage,
                                            StartPde,
                                            MmSessionSpace->SessionPageDirectoryIndex);
        }

        StartPde += 1;
        Index += 1;
    }

    UNLOCK_PFN (OldIrql);

    StartPde = MiGetPdeAddress (StartVa);
    Index = MiGetPdeSessionIndex (StartVa);

    while (StartPde <= EndPde) {

        if (SavePageTables[Index] == 1) {

            ASSERT (StartPde->u.Hard.Valid == 1);

            PageTablePage = MI_GET_PAGE_FRAME_FROM_PTE (StartPde);

            Pfn1 = MI_PFN_ELEMENT (PageTablePage);

            ASSERT (Pfn1->u1.Event == NULL);
            Pfn1->u1.Event = (PVOID)PsGetCurrentThread ();

            SessionPte = MiGetVirtualAddressMappedByPte (StartPde);

            MiAddValidPageToWorkingSet (SessionPte,
                                        StartPde,
                                        Pfn1,
                                        0);

            Entry = MiLocateWsle (SessionPte,
                                  MmSessionSpace->Vm.VmWorkingSetList,
                                  Pfn1->u1.WsIndex);

            if (Entry >= WorkingSetList->FirstDynamic) {

                SwapEntry = WorkingSetList->FirstDynamic;

                if (Entry != WorkingSetList->FirstDynamic) {

                    //
                    // Swap this entry with the one at first dynamic.
                    //

                    MiSwapWslEntries (Entry, SwapEntry, &MmSessionSpace->Vm);
                }

                WorkingSetList->FirstDynamic += 1;
            }
            else {
                SwapEntry = Entry;
            }

            //
            // Indicate that the page is locked.
            //

            MmSessionSpace->Wsle[SwapEntry].u1.e1.LockedInWs = 1;
        }

        StartPde += 1;
        Index += 1;
    }

    return STATUS_SUCCESS;
}

#if DBG
typedef struct _MISWAP {
    ULONG Flag;
    PEPROCESS Process;
    PMM_SESSION_SPACE Session;
    ULONG OutSwapCount;
} MISWAP, *PMISWAP;

ULONG MiSessionInfo[4];
MISWAP MiSessionSwap[0x100];
ULONG  MiSwapIndex;
#endif


VOID
MiSessionOutSwapProcess (
    IN PEPROCESS Process
    )

/*++

Routine Description:

    This routine notifies the containing session that the specified process is
    being outswapped.  When all the processes within a session have been
    outswapped, the containing session undergoes a heavy trim.

Arguments:

    Process - Supplies a pointer to the process that is swapped out of memory.

Return Value:

    None.

Environment:

    Kernel mode.  This routine must not enter a wait state for memory resources
    or the system will deadlock.

--*/

{
    MMPTE TempPte;
    PFN_NUMBER PdePage;
    PMMPTE PageDirectoryMap;
    KIRQL OldIrql;
    PFN_NUMBER SessionPage;
    PMM_SESSION_SPACE SessionGlobal;
#if DBG
    ULONG InCount;
    ULONG OutCount;
    PLIST_ENTRY NextEntry;
#endif
#if defined (_X86PAE_)
    ULONG i;
    PPAE_ENTRY PaeVa;
#endif

    ASSERT (MiHydra == TRUE && Process->Vm.u.Flags.ProcessInSession == 1);

    //
    // smss doesn't count when we swap it before it has detached from the
    // session it is currently creating.
    //

    if (Process->Vm.u.Flags.SessionLeader == 1) {
        return;
    }

#if defined (_X86PAE_)
    PaeVa = Process->PaeTop;

#if DBG
    for (i = 0; i < PD_PER_SYSTEM; i += 1) {
        ASSERT (PaeVa->PteEntry[i].u.Hard.Valid == 1);
    }
#endif

    PdePage = MI_GET_PAGE_FRAME_FROM_PTE (&PaeVa->PteEntry[MiGetPdPteOffset(MmSessionSpace)]);

#else

    PdePage = MI_GET_DIRECTORY_FRAME_FROM_PROCESS(Process);

#endif

    PageDirectoryMap = MiMapPageInHyperSpace (PdePage, &OldIrql);

#if defined (_WIN64)
    TempPte = PageDirectoryMap[MiGetPpeOffset(MmSessionSpace)];

    MiUnmapPageInHyperSpace (OldIrql);

    PdePage = MI_GET_PAGE_FRAME_FROM_PTE (&TempPte);

    PageDirectoryMap = MiMapPageInHyperSpace (PdePage, &OldIrql);
#endif

    TempPte = PageDirectoryMap[MiGetPdeOffset(MmSessionSpace)];

    MiUnmapPageInHyperSpace (OldIrql);

    PdePage = MI_GET_PAGE_FRAME_FROM_PTE (&TempPte);

    PageDirectoryMap = MiMapPageInHyperSpace (PdePage, &OldIrql);

    TempPte = PageDirectoryMap[MiGetPteOffset(MmSessionSpace)];

    MiUnmapPageInHyperSpace (OldIrql);

    SessionPage = MI_GET_PAGE_FRAME_FROM_PTE (&TempPte);

    SessionGlobal = (PMM_SESSION_SPACE) MiMapPageInHyperSpace ( SessionPage,
                                                                &OldIrql);

    ASSERT (MI_GET_PAGE_FRAME_FROM_PTE (MiGetPteAddress(SessionGlobal->GlobalVirtualAddress)) == SessionPage);

    SessionGlobal = SessionGlobal->GlobalVirtualAddress;

    MiUnmapPageInHyperSpace (OldIrql);

    LOCK_EXPANSION (OldIrql);

    SessionGlobal->ProcessOutSwapCount += 1;

#if DBG
    ASSERT ((LONG)SessionGlobal->ProcessOutSwapCount > 0);

    InCount = 0;
    OutCount = 0;
    NextEntry = SessionGlobal->ProcessList.Flink;

    while (NextEntry != &SessionGlobal->ProcessList) {
        Process = CONTAINING_RECORD (NextEntry, EPROCESS, SessionProcessLinks);

        if (Process->ProcessOutswapEnabled == TRUE) {
            OutCount += 1;
        }
        else {
            InCount += 1;
        }

        NextEntry = NextEntry->Flink;
    }

    if (InCount + OutCount > SessionGlobal->ReferenceCount) {
        DbgPrint ("MiSessionOutSwapProcess : process count mismatch %p %x %x %x\n",
            SessionGlobal,
            SessionGlobal->ReferenceCount,
            InCount,
            OutCount);
        DbgBreakPoint ();
    }

    if (SessionGlobal->ProcessOutSwapCount != OutCount) {
        DbgPrint ("MiSessionOutSwapProcess : out count mismatch %p %x %x %x %x\n",
            SessionGlobal,
            SessionGlobal->ReferenceCount,
            SessionGlobal->ProcessOutSwapCount,
            InCount,
            OutCount);
        DbgBreakPoint ();
    }

    ASSERT (SessionGlobal->ProcessOutSwapCount <= SessionGlobal->ReferenceCount);

    MiSessionSwap[MiSwapIndex].Flag = 1;
    MiSessionSwap[MiSwapIndex].Process = Process;
    MiSessionSwap[MiSwapIndex].Session = SessionGlobal;
    MiSessionSwap[MiSwapIndex].OutSwapCount = SessionGlobal->ProcessOutSwapCount;
    MiSwapIndex += 1;
    if (MiSwapIndex == 0x100) {
        MiSwapIndex = 0;
    }
#endif

    if (SessionGlobal->ProcessOutSwapCount == SessionGlobal->ReferenceCount) {
        SessionGlobal->Vm.u.Flags.TrimHard = 1;
#if DBG
        if (MmDebug & MM_DBG_SESSIONS) {
            DbgPrint ("Mm: Last process (%d total) just swapped out for session %d, %d pages\n",
                SessionGlobal->ProcessOutSwapCount,
                SessionGlobal->SessionId,
                SessionGlobal->Vm.WorkingSetSize);
        }
        MiSessionInfo[0] += 1;
#endif
        KeQuerySystemTime (&SessionGlobal->LastProcessSwappedOutTime);
    }
#if DBG
    else {
        MiSessionInfo[1] += 1;
    }
#endif

    UNLOCK_EXPANSION (OldIrql);
}


VOID
MiSessionInSwapProcess (
    IN PEPROCESS Process
    )

/*++

Routine Description:

    This routine in swaps the specified process.

Arguments:

    Process - Supplies a pointer to the process that is to be swapped
        into memory.

Return Value:

    None.

Environment:

    Kernel mode.  This routine must not enter a wait state for memory resources
    or the system will deadlock.

--*/

{
    MMPTE TempPte;
    PFN_NUMBER PdePage;
    PMMPTE PageDirectoryMap;
    KIRQL OldIrql;
    PFN_NUMBER SessionPage;
    PMM_SESSION_SPACE SessionGlobal;
#if DBG
    ULONG InCount;
    ULONG OutCount;
    PLIST_ENTRY NextEntry;
#endif
#if defined (_X86PAE_)
    ULONG i;
    PPAE_ENTRY PaeVa;
#endif

    ASSERT (MiHydra == TRUE && Process->Vm.u.Flags.ProcessInSession == 1);

    //
    // smss doesn't count when we catch it before it has detached from the
    // session it is currently creating.
    //

    if (Process->Vm.u.Flags.SessionLeader == 1) {
        return;
    }

    ASSERT (MiHydra == TRUE && Process->Vm.u.Flags.ProcessInSession == 1);

    //
    // smss doesn't count when we swap it before it has detached from the
    // session it is currently creating.
    //

    if (Process->Vm.u.Flags.SessionLeader == 1) {
        return;
    }

#if defined (_X86PAE_)
    PaeVa = Process->PaeTop;

#if DBG
    for (i = 0; i < PD_PER_SYSTEM; i += 1) {
        ASSERT (PaeVa->PteEntry[i].u.Hard.Valid == 1);
    }
#endif

    PdePage = MI_GET_PAGE_FRAME_FROM_PTE (&PaeVa->PteEntry[MiGetPdPteOffset(MmSessionSpace)]);

#else
    PdePage = MI_GET_DIRECTORY_FRAME_FROM_PROCESS(Process);
#endif

    PageDirectoryMap = MiMapPageInHyperSpace (PdePage, &OldIrql);

#if defined (_WIN64)
    TempPte = PageDirectoryMap[MiGetPpeOffset(MmSessionSpace)];

    MiUnmapPageInHyperSpace (OldIrql);

    PdePage = MI_GET_PAGE_FRAME_FROM_PTE (&TempPte);

    PageDirectoryMap = MiMapPageInHyperSpace (PdePage, &OldIrql);
#endif

    TempPte = PageDirectoryMap[MiGetPdeOffset(MmSessionSpace)];

    MiUnmapPageInHyperSpace (OldIrql);

    PdePage = MI_GET_PAGE_FRAME_FROM_PTE (&TempPte);

    PageDirectoryMap = MiMapPageInHyperSpace (PdePage, &OldIrql);

    TempPte = PageDirectoryMap[MiGetPteOffset(MmSessionSpace)];

    MiUnmapPageInHyperSpace (OldIrql);

    SessionPage = MI_GET_PAGE_FRAME_FROM_PTE (&TempPte);

    SessionGlobal = (PMM_SESSION_SPACE) MiMapPageInHyperSpace (SessionPage,
                                                               &OldIrql);

    ASSERT (MI_GET_PAGE_FRAME_FROM_PTE (MiGetPteAddress(SessionGlobal->GlobalVirtualAddress)) == SessionPage);

    SessionGlobal = SessionGlobal->GlobalVirtualAddress;

    MiUnmapPageInHyperSpace (OldIrql);

    LOCK_EXPANSION (OldIrql);

#if DBG
    ASSERT ((LONG)SessionGlobal->ProcessOutSwapCount > 0);

    InCount = 0;
    OutCount = 0;
    NextEntry = SessionGlobal->ProcessList.Flink;

    while (NextEntry != &SessionGlobal->ProcessList) {
        Process = CONTAINING_RECORD (NextEntry, EPROCESS, SessionProcessLinks);

        if (Process->ProcessOutswapEnabled == TRUE) {
            OutCount += 1;
        }
        else {
            InCount += 1;
        }

        NextEntry = NextEntry->Flink;
    }

    if (InCount + OutCount > SessionGlobal->ReferenceCount) {
        DbgPrint ("MiSessionInSwapProcess : count mismatch %p %x %x %x\n",
            SessionGlobal,
            SessionGlobal->ReferenceCount,
            InCount,
            OutCount);
        DbgBreakPoint ();
    }

    if (SessionGlobal->ProcessOutSwapCount != OutCount) {
        DbgPrint ("MiSessionInSwapProcess : out count mismatch %p %x %x %x %x\n",
            SessionGlobal,
            SessionGlobal->ReferenceCount,
            SessionGlobal->ProcessOutSwapCount,
            InCount,
            OutCount);
        DbgBreakPoint ();
    }

    ASSERT (SessionGlobal->ProcessOutSwapCount <= SessionGlobal->ReferenceCount);

    MiSessionSwap[MiSwapIndex].Flag = 2;
    MiSessionSwap[MiSwapIndex].Process = Process;
    MiSessionSwap[MiSwapIndex].Session = SessionGlobal;
    MiSessionSwap[MiSwapIndex].OutSwapCount = SessionGlobal->ProcessOutSwapCount;
    MiSwapIndex += 1;
    if (MiSwapIndex == 0x100) {
        MiSwapIndex = 0;
    }
#endif

    if (SessionGlobal->ProcessOutSwapCount == SessionGlobal->ReferenceCount) {
#if DBG
        MiSessionInfo[2] += 1;
        if (MmDebug & MM_DBG_SESSIONS) {
            DbgPrint ("Mm: First process (%d total) just swapped back in for session %d, %d pages\n",
                SessionGlobal->ProcessOutSwapCount,
                SessionGlobal->SessionId,
                SessionGlobal->Vm.WorkingSetSize);
        }
#endif
        SessionGlobal->Vm.u.Flags.TrimHard = 0;
    }
#if DBG
    else {
        MiSessionInfo[3] += 1;
    }
#endif

    SessionGlobal->ProcessOutSwapCount -= 1;

    ASSERT ((LONG)SessionGlobal->ProcessOutSwapCount >= 0);

    UNLOCK_EXPANSION (OldIrql);
}
