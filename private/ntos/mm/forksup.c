/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

   forksup.c

Abstract:

    This module contains the routines which support the POSIX fork operation.

Author:

    Lou Perazzoli (loup) 22-Jul-1989
    Landy Wang (landyw) 02-June-1997

Revision History:

--*/

#include "mi.h"

VOID
MiUpPfnReferenceCount (
    IN PFN_NUMBER Page,
    IN USHORT Count
    );

VOID
MiDownPfnReferenceCount (
    IN PFN_NUMBER Page,
    IN USHORT Count
    );

VOID
MiUpControlAreaRefs (
    IN PCONTROL_AREA ControlArea
    );

ULONG
MiDoneWithThisPageGetAnother (
    IN PPFN_NUMBER PageFrameIndex,
    IN PMMPTE PointerPde,
    IN PEPROCESS CurrentProcess
    );

VOID
MiUpForkPageShareCount(
    IN PMMPFN PfnForkPtePage
    );

VOID
MiUpCloneProcessRefCount (
    IN PMMCLONE_DESCRIPTOR Clone
    );

VOID
MiUpCloneProtoRefCount (
    IN PMMCLONE_BLOCK CloneProto,
    IN PEPROCESS CurrentProcess
    );

ULONG
MiHandleForkTransitionPte (
    IN PMMPTE PointerPte,
    IN PMMPTE PointerNewPte,
    IN PMMCLONE_BLOCK ForkProtoPte
    );

VOID
MiDownShareCountFlushEntireTb (
    IN PFN_NUMBER PageFrameIndex
    );

VOID
MiBuildForkPageTable(
    IN PFN_NUMBER PageFrameIndex,
    IN PMMPTE PointerPde,
    IN PMMPTE PointerNewPde,
    IN PFN_NUMBER PdePhysicalPage,
    IN PMMPFN PfnPdPage
    );

VOID
MiRetrievePageDirectoryFrames(
    IN PFN_NUMBER RootPhysicalPage,
    OUT PPFN_NUMBER PageDirectoryFrames
    );

#define MM_FORK_SUCCEEDED 0
#define MM_FORK_FAILED 1
    
#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,MiCloneProcessAddressSpace)
#endif


NTSTATUS
MiCloneProcessAddressSpace (
    IN PEPROCESS ProcessToClone,
    IN PEPROCESS ProcessToInitialize,
    IN PFN_NUMBER RootPhysicalPage,
    IN PFN_NUMBER HyperPhysicalPage
    )

/*++

Routine Description:

    This routine stands on its head to produce a copy of the specified
    process's address space in the process to initialize.  This
    is done by examining each virtual address descriptor's inherit
    attributes.  If the pages described by the VAD should be inherited,
    each PTE is examined and copied into the new address space.

    For private pages, fork prototype PTEs are constructed and the pages
    become shared, copy-on-write, between the two processes.


Arguments:

    ProcessToClone - Supplies the process whose address space should be
                     cloned.

    ProcessToInitialize - Supplies the process whose address space is to
                          be created.

    RootPhysicalPage - Supplies the physical page number of the top level
                       page (parent on 64-bit systems) directory
                       of the process to initialize.

    HyperPhysicalPage - Supplies the physical page number of the page table
                        page which maps hyperspace for the process to
                        initialize.  This is only needed for 32-bit systems.

Return Value:

    None.

Environment:

    Kernel mode, APCs disabled.

--*/

{
    PFN_NUMBER PpePhysicalPage;
    PFN_NUMBER PdePhysicalPage;
    PEPROCESS CurrentProcess;
    PMMPTE PdeBase;
    PMMCLONE_HEADER CloneHeader;
    PMMCLONE_BLOCK CloneProtos;
    PMMCLONE_DESCRIPTOR CloneDescriptor;
    PMMVAD NewVad;
    PMMVAD Vad;
    PMMVAD NextVad;
    PMMVAD *VadList;
    PMMVAD FirstNewVad;
    PMMCLONE_DESCRIPTOR *CloneList;
    PMMCLONE_DESCRIPTOR FirstNewClone;
    PMMCLONE_DESCRIPTOR Clone;
    PMMCLONE_DESCRIPTOR NextClone;
    PMMCLONE_DESCRIPTOR NewClone;
    ULONG Attached;
    ULONG CloneFailed;
    ULONG VadInsertFailed;
    WSLE_NUMBER WorkingSetIndex;
    PVOID VirtualAddress;
    NTSTATUS status;
    PMMPFN Pfn2;
    PMMPFN PfnPdPage;
    MMPTE TempPte;
    MMPTE PteContents;
#if defined (_X86PAE_)
    PMDL MdlPageDirectory;
    PPFN_NUMBER MdlPageFrames;
    PFN_NUMBER PageDirectoryFrames[PD_PER_SYSTEM];
    PFN_NUMBER MdlHackPageDirectory[(sizeof(MDL)/sizeof(PFN_NUMBER)) + PD_PER_SYSTEM];
#endif
    PFN_NUMBER MdlPage;
    PFN_NUMBER MdlDirPage;
    PMMPTE PointerPte;
    PMMPTE PointerPde;
    PMMPTE PointerPpe;
    PMMPTE LastPte;
    PMMPTE PointerNewPte;
    PMMPTE NewPteMappedAddress;
    PMMPTE PointerNewPde;
    PLIST_ENTRY NextEntry;
    PMI_PHYSICAL_VIEW PhysicalView;
    PFN_NUMBER PageFrameIndex;
    PFN_NUMBER PageDirFrameIndex;
    PMMCLONE_BLOCK ForkProtoPte;
    PMMCLONE_BLOCK CloneProto;
    PMMCLONE_BLOCK LockedForkPte;
    PMMPTE ContainingPte;
    ULONG NumberOfForkPtes;
    PFN_NUMBER NumberOfPrivatePages;
    PFN_NUMBER PageTablePage;
    SIZE_T TotalPagedPoolCharge;
    SIZE_T TotalNonPagedPoolCharge;
    PMMPFN PfnForkPtePage;
    PVOID UsedPageTableEntries;
    ULONG ReleasedWorkingSetMutex;
    ULONG FirstTime;
    ULONG Waited;
    ULONG i;
    ULONG PpePdeOffset;
#if defined (_WIN64)
    PVOID UsedPageDirectoryEntries;
    PMMPTE PointerNewPpe;
    PMMPTE PpeBase;
    PMMPFN PfnPpPage;
#else
    PMMWSL HyperBase;
#endif

    PageTablePage = 2;
    NumberOfForkPtes = 0;
    Attached = FALSE;
    PageFrameIndex = (PFN_NUMBER)-1;
    PageDirFrameIndex = (PFN_NUMBER)-1;

#if DBG
    if (MmDebug & MM_DBG_FORK) {
        DbgPrint("beginning clone operation process to clone = %lx\n",
            ProcessToClone);
    }
#endif //DBG

    PAGED_CODE();

    if (ProcessToClone != PsGetCurrentProcess()) {
        Attached = TRUE;
        KeAttachProcess (&ProcessToClone->Pcb);
    }

#if defined (_X86PAE_)
    MiRetrievePageDirectoryFrames (RootPhysicalPage, PageDirectoryFrames);
#endif

    CurrentProcess = ProcessToClone;

    //
    // Get the working set mutex and the address creation mutex
    // of the process to clone.  This prevents page faults while we
    // are examining the address map and prevents virtual address space
    // from being created or deleted.
    //

    LOCK_ADDRESS_SPACE (CurrentProcess);

    //
    // Write-watch VAD bitmaps are not currently duplicated
    // so fork is not allowed.
    //

    if (CurrentProcess->Vm.u.Flags.WriteWatch == 1) {
        status = STATUS_INVALID_PAGE_PROTECTION;
        goto ErrorReturn1;
    }

    //
    // Check for AWE regions as they are not duplicated so fork is not allowed.
    // Note that since this is a readonly list walk, the address space mutex
    // is sufficient to synchronize properly.
    //

    NextEntry = CurrentProcess->PhysicalVadList.Flink;
    while (NextEntry != &CurrentProcess->PhysicalVadList) {

        PhysicalView = CONTAINING_RECORD(NextEntry,
                                         MI_PHYSICAL_VIEW,
                                         ListEntry);

        if (PhysicalView->Vad->u.VadFlags.UserPhysicalPages == 1) {
            status = STATUS_INVALID_PAGE_PROTECTION;
            goto ErrorReturn1;
        }

        NextEntry = NextEntry->Flink;
    }

    //
    // Make sure the address space was not deleted, if so, return an error.
    //

    if (CurrentProcess->AddressSpaceDeleted != 0) {
        status = STATUS_PROCESS_IS_TERMINATING;
        goto ErrorReturn1;
    }

    //
    // Attempt to acquire the needed pool before starting the
    // clone operation, this allows an easier failure path in
    // the case of insufficient system resources.
    //

    NumberOfPrivatePages = CurrentProcess->NumberOfPrivatePages;

    CloneProtos = ExAllocatePoolWithTag (PagedPool, sizeof(MMCLONE_BLOCK) *
                                                NumberOfPrivatePages,
                                                'lCmM');
    if (CloneProtos == NULL) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto ErrorReturn1;
    }

    CloneHeader = ExAllocatePoolWithTag (NonPagedPool,
                                         sizeof(MMCLONE_HEADER),
                                         '  mM');
    if (CloneHeader == NULL) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto ErrorReturn2;
    }

    CloneDescriptor = ExAllocatePoolWithTag (NonPagedPool,
                                             sizeof(MMCLONE_DESCRIPTOR),
                                             '  mM');
    if (CloneDescriptor == NULL) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto ErrorReturn3;
    }

    Vad = MiGetFirstVad (CurrentProcess);
    VadList = &FirstNewVad;

    while (Vad != (PMMVAD)NULL) {

        //
        // If the VAD does not go to the child, ignore it.
        //

        if ((Vad->u.VadFlags.UserPhysicalPages == 0) &&

            ((Vad->u.VadFlags.PrivateMemory == 1) ||
            (Vad->u2.VadFlags2.Inherit == MM_VIEW_SHARE))) {

            NewVad = ExAllocatePoolWithTag (NonPagedPool, sizeof(MMVAD), ' daV');

            if (NewVad == NULL) {

                //
                // Unable to allocate pool for all the VADs.  Deallocate
                // all VADs and other pool obtained so far.
                //

                *VadList = (PMMVAD)NULL;
                status = STATUS_INSUFFICIENT_RESOURCES;
                goto ErrorReturn4;
            }
            *VadList = NewVad;
            VadList = &NewVad->Parent;
        }
        Vad = MiGetNextVad (Vad);
    }

    //
    // Terminate list of VADs for new process.
    //

    *VadList = (PMMVAD)NULL;

    //
    // Charge the current process the quota for the paged and nonpaged
    // global structures.  This consists of the array of clone blocks
    // in paged pool and the clone header in non-paged pool.
    //

    try {
        PsChargePoolQuota (CurrentProcess, PagedPool, sizeof(MMCLONE_BLOCK) *
                                                NumberOfPrivatePages);
        PageTablePage = 1;
        PsChargePoolQuota (CurrentProcess, NonPagedPool, sizeof(MMCLONE_HEADER));
        PageTablePage = 0;

    } except (EXCEPTION_EXECUTE_HANDLER) {

        //
        // Unable to charge quota for the clone blocks.
        //

        status = GetExceptionCode();
        goto ErrorReturn4;
    }

    LOCK_WS (CurrentProcess);

    ASSERT (CurrentProcess->ForkInProgress == NULL);

    //
    // Indicate to the pager that the current process is being
    // forked.  This blocks other threads in that process from
    // modifying clone blocks counts and contents.
    //

    CurrentProcess->ForkInProgress = PsGetCurrentThread();

#if defined (_WIN64)

    //
    // Increment the reference count for the pages which are being "locked"
    // in MDLs.  This prevents the page from being reused while it is
    // being double mapped.  Note we have a count of 3 below because in addition
    // to the PPE, we also set up a initial dummy PDE and PTE.
    //

    MiUpPfnReferenceCount (RootPhysicalPage, 3);

    //
    // Map the page directory parent page into the system address
    // space.  This is accomplished by building an MDL to describe the
    // Page directory parent page.
    //

    PpeBase = (PMMPTE)MiMapSinglePage (NULL,
                                       RootPhysicalPage,
                                       MmCached,
                                       HighPagePriority);

    if (PpeBase == NULL) {
        MiDownPfnReferenceCount (RootPhysicalPage, 3);
        CurrentProcess->ForkInProgress = NULL;
        UNLOCK_WS (CurrentProcess);
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto ErrorReturn4;
    }

    PfnPpPage = MI_PFN_ELEMENT (RootPhysicalPage);

#else

#if !defined (_X86PAE_)
    MiUpPfnReferenceCount (RootPhysicalPage, 1);
#endif

#endif

    //
    // Initialize a page directory map so it can be
    // unlocked in the loop and the end of the loop without
    // any testing to see if has a valid value the first time through.
    // Note this is a dummy map for 64-bit systems and a real one for 32-bit.
    //

#if !defined (_X86PAE_)

    MdlDirPage = RootPhysicalPage;

    PdePhysicalPage = RootPhysicalPage;

    PdeBase = (PMMPTE)MiMapSinglePage (NULL,
                                       MdlDirPage,
                                       MmCached,
                                       HighPagePriority);

    if (PdeBase == NULL) {
#if defined (_WIN64)
        MiDownPfnReferenceCount (RootPhysicalPage, 3);
        MiUnmapSinglePage (PpeBase);
#else
        MiDownPfnReferenceCount (RootPhysicalPage, 1);
#endif
        CurrentProcess->ForkInProgress = NULL;
        UNLOCK_WS (CurrentProcess);
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto ErrorReturn4;
    }

#else

    //
    // All 4 page directory pages need to be mapped for PAE so the heavyweight
    // mapping must be used.
    //

    MdlPageDirectory = (PMDL)&MdlHackPageDirectory[0];

    MmInitializeMdl(MdlPageDirectory, (PVOID)PDE_BASE, PD_PER_SYSTEM * PAGE_SIZE);
    MdlPageDirectory->MdlFlags |= MDL_PAGES_LOCKED;

    MdlPageFrames = (PPFN_NUMBER)(MdlPageDirectory + 1);

    for (i = 0; i < PD_PER_SYSTEM; i += 1) {
        *(MdlPageFrames + i) = PageDirectoryFrames[i];
        MiUpPfnReferenceCount (PageDirectoryFrames[i], 1);
    }

    PdePhysicalPage = RootPhysicalPage;

    PdeBase = (PMMPTE)MmMapLockedPagesSpecifyCache (MdlPageDirectory,
                                                    KernelMode,
                                                    MmCached,
                                                    NULL,
                                                    FALSE,
                                                    HighPagePriority);

    if (PdeBase == NULL) {
        for (i = 0; i < PD_PER_SYSTEM; i += 1) {
            MiDownPfnReferenceCount (PageDirectoryFrames[i], 1);
        }
        CurrentProcess->ForkInProgress = NULL;
        UNLOCK_WS (CurrentProcess);
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto ErrorReturn4;
    }

#endif

    PfnPdPage = MI_PFN_ELEMENT (RootPhysicalPage);

#if !defined (_WIN64)

    //
    // Map hyperspace so target UsedPageTable entries can be incremented.
    //

    MiUpPfnReferenceCount (HyperPhysicalPage, 2);

    HyperBase = (PMMWSL)MiMapSinglePage (NULL,
                                         HyperPhysicalPage,
                                         MmCached,
                                         HighPagePriority);

    if (HyperBase == NULL) {
        MiDownPfnReferenceCount (HyperPhysicalPage, 2);
#if !defined (_X86PAE_)
        MiDownPfnReferenceCount (RootPhysicalPage, 1);
        MiUnmapSinglePage (PdeBase);
#else
        for (i = 0; i < PD_PER_SYSTEM; i += 1) {
            MiDownPfnReferenceCount (PageDirectoryFrames[i], 1);
        }
        MmUnmapLockedPages (PdeBase, MdlPageDirectory);
#endif
        CurrentProcess->ForkInProgress = NULL;
        UNLOCK_WS (CurrentProcess);
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto ErrorReturn4;
    }
#endif

    //
    // Initialize a page table MDL to lock and map the hyperspace page so it
    // can be unlocked in the loop and the end of the loop without
    // any testing to see if has a valid value the first time through.
    //

#if defined (_WIN64)
    MdlPage = RootPhysicalPage;
#else
    MdlPage = HyperPhysicalPage;
#endif

    NewPteMappedAddress = (PMMPTE)MiMapSinglePage (NULL,
                                                   MdlPage,
                                                   MmCached,
                                                   HighPagePriority);

    if (NewPteMappedAddress == NULL) {

#if defined (_WIN64)

        MiDownPfnReferenceCount (RootPhysicalPage, 3);
        MiUnmapSinglePage (PpeBase);
        MiUnmapSinglePage (PdeBase);

#else
        MiDownPfnReferenceCount (HyperPhysicalPage, 2);
        MiUnmapSinglePage (HyperBase);
#if !defined (_X86PAE_)
        MiDownPfnReferenceCount (RootPhysicalPage, 1);
        MiUnmapSinglePage (PdeBase);
#else
        for (i = 0; i < PD_PER_SYSTEM; i += 1) {
            MiDownPfnReferenceCount (PageDirectoryFrames[i], 1);
        }
        MmUnmapLockedPages (PdeBase, MdlPageDirectory);
#endif

#endif

        CurrentProcess->ForkInProgress = NULL;
        UNLOCK_WS (CurrentProcess);
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto ErrorReturn4;
    }

    PointerNewPte = NewPteMappedAddress;

    //
    // Build new clone prototype PTE block and descriptor, note that
    // each prototype PTE has a reference count following it.
    //

    ForkProtoPte = CloneProtos;

    LockedForkPte = ForkProtoPte;
    MiLockPagedAddress (LockedForkPte, FALSE);

    CloneHeader->NumberOfPtes = (ULONG)NumberOfPrivatePages;
    CloneHeader->NumberOfProcessReferences = 1;
    CloneHeader->ClonePtes = CloneProtos;



    CloneDescriptor->StartingVpn = (ULONG_PTR)CloneProtos;
    CloneDescriptor->EndingVpn = (ULONG_PTR)((ULONG_PTR)CloneProtos +
                            NumberOfPrivatePages *
                              sizeof(MMCLONE_BLOCK));
    CloneDescriptor->NumberOfReferences = 0;
    CloneDescriptor->NumberOfPtes = (ULONG)NumberOfPrivatePages;
    CloneDescriptor->CloneHeader = CloneHeader;
    CloneDescriptor->PagedPoolQuotaCharge = sizeof(MMCLONE_BLOCK) *
                                NumberOfPrivatePages;

    //
    // Insert the clone descriptor for this fork operation into the
    // process which was cloned.
    //

    MiInsertClone (CloneDescriptor);

    //
    // Examine each virtual address descriptor and create the
    // proper structures for the new process.
    //

    Vad = MiGetFirstVad (CurrentProcess);
    NewVad = FirstNewVad;

    while (Vad != (PMMVAD)NULL) {

        //
        // Examine the VAD to determine its type and inheritance
        // attribute.
        //

        if ((Vad->u.VadFlags.UserPhysicalPages == 0) &&

            ((Vad->u.VadFlags.PrivateMemory == 1) ||
            (Vad->u2.VadFlags2.Inherit == MM_VIEW_SHARE))) {

            //
            // The virtual address descriptor should be shared in the
            // forked process.
            //

            //
            // Make a copy of the VAD for the new process, the new VADs
            // are preallocated and linked together through the parent
            // field.
            //

            NextVad = NewVad->Parent;


            if (Vad->u.VadFlags.PrivateMemory == 1) {
                *(PMMVAD_SHORT)NewVad = *(PMMVAD_SHORT)Vad;
                NewVad->u.VadFlags.NoChange = 0;
            } else {
                *NewVad = *Vad;
            }

            if (NewVad->u.VadFlags.NoChange) {
                if ((NewVad->u2.VadFlags2.OneSecured) ||
                    (NewVad->u2.VadFlags2.MultipleSecured)) {

                    //
                    // Eliminate these as the memory was secured
                    // only in this process, not in the new one.
                    //

                    NewVad->u2.VadFlags2.OneSecured = 0;
                    NewVad->u2.VadFlags2.MultipleSecured = 0;
                    NewVad->u2.VadFlags2.StoredInVad = 0;
                    NewVad->u3.List.Flink = NULL;
                    NewVad->u3.List.Blink = NULL;
                }
                if (NewVad->u2.VadFlags2.SecNoChange == 0) {
                    NewVad->u.VadFlags.NoChange = 0;
                }
            }
            NewVad->Parent = NextVad;

            //
            // If the VAD refers to a section, up the view count for that
            // section.  This requires the PFN mutex to be held.
            //

            if ((Vad->u.VadFlags.PrivateMemory == 0) &&
                (Vad->ControlArea != (PCONTROL_AREA)NULL)) {

                //
                // Increment the count of the number of views for the
                // section object.  This requires the PFN mutex to be held.
                //

                MiUpControlAreaRefs (Vad->ControlArea);
            }

            //
            // Examine each PTE and create the appropriate PTE for the
            // new process.
            //

            PointerPde = MiGetPdeAddress (MI_VPN_TO_VA (Vad->StartingVpn));
            PointerPte = (volatile PMMPTE) MiGetPteAddress (
                                          MI_VPN_TO_VA (Vad->StartingVpn));
            LastPte = MiGetPteAddress (MI_VPN_TO_VA (Vad->EndingVpn));
            FirstTime = TRUE;

            while ((PMMPTE)PointerPte <= LastPte) {

                //
                // For each PTE contained in the VAD check the page table
                // page, and if non-zero, make the appropriate modifications
                // to copy the PTE to the new process.
                //

                if ((FirstTime) || MiIsPteOnPdeBoundary (PointerPte)) {

                    PointerPpe = MiGetPdeAddress (PointerPte);
                    PointerPde = MiGetPteAddress (PointerPte);

                    do {

                        while (!MiDoesPpeExistAndMakeValid (PointerPpe,
                                                            CurrentProcess,
                                                            FALSE,
                                                            &Waited)) {
    
                            //
                            // Page directory parent is empty, go to the next one.
                            //
    
                            PointerPpe += 1;
                            PointerPde = MiGetVirtualAddressMappedByPte (PointerPpe);
                            PointerPte = MiGetVirtualAddressMappedByPte (PointerPde);
    
                            if ((PMMPTE)PointerPte > LastPte) {
    
                                //
                                // All done with this VAD, exit loop.
                                //
    
                                goto AllDone;
                            }
                        }
    
                        Waited = 0;
    
                        while (!MiDoesPdeExistAndMakeValid (PointerPde,
                                                            CurrentProcess,
                                                            FALSE,
                                                            &Waited)) {
    
                            //
                            // This page directory is empty, go to the next one.
                            //
    
                            PointerPde += 1;
                            PointerPte = MiGetVirtualAddressMappedByPte (PointerPde);
    
                            if ((PMMPTE)PointerPte > LastPte) {
    
                                //
                                // All done with this VAD, exit loop.
                                //
    
                                goto AllDone;
                            }
#if defined (_WIN64)
                            if (MiIsPteOnPdeBoundary (PointerPde)) {
                                PointerPpe = MiGetPteAddress (PointerPde);
                                Waited = 1;
                                break;
                            }
#endif
                        }
    
                    } while (Waited != 0);

                    FirstTime = FALSE;

#if defined (_WIN64)
                    //
                    // Calculate the address of the PPE in the new process's
                    // page directory parent page.
                    //

                    PointerNewPpe = &PpeBase[MiGetPdeOffset(PointerPte)];

                    if (PointerNewPpe->u.Long == 0) {

                        //
                        // No physical page has been allocated yet, get a page
                        // and map it in as a transition page.  This will
                        // become a page table page for the new process.
                        //

                        ReleasedWorkingSetMutex =
                                MiDoneWithThisPageGetAnother (&PageDirFrameIndex,
                                                              PointerPpe,
                                                              CurrentProcess);

                        MI_ZERO_USED_PAGETABLE_ENTRIES (MI_PFN_ELEMENT(PageDirFrameIndex));

                        if (ReleasedWorkingSetMutex) {

                            do {

                                MiDoesPpeExistAndMakeValid (PointerPpe,
                                                            CurrentProcess,
                                                            FALSE,
                                                            &Waited);
    
                                Waited = 0;
    
                                MiDoesPdeExistAndMakeValid (PointerPde,
                                                            CurrentProcess,
                                                            FALSE,
                                                            &Waited);
                            } while (Waited != 0);
                        }

                        //
                        // Hand initialize this PFN as normal initialization
                        // would do it for the process whose context we are
                        // attached to.
                        //
                        // The PFN lock must be held while initializing the
                        // frame to prevent those scanning the database for
                        // free frames from taking it after we fill in the
                        // u2 field.
                        //

                        MiBuildForkPageTable (PageDirFrameIndex,
                                              PointerPpe,
                                              PointerNewPpe,
                                              RootPhysicalPage,
                                              PfnPpPage);

                        //
                        // Map the new page directory page into the system
                        // portion of the address space.  Note that hyperspace
                        // cannot be used as other operations (allocating
                        // nonpaged pool at DPC level) could cause the
                        // hyperspace page being used to be reused.
                        //

                        MiDownPfnReferenceCount (MdlDirPage, 1);

                        MdlDirPage = PageDirFrameIndex;

                        ASSERT (PdeBase != NULL);

                        PdeBase = (PMMPTE)MiMapSinglePage (PdeBase,
                                                           MdlDirPage,
                                                           MmCached,
                                                           HighPagePriority);

                        MiUpPfnReferenceCount (MdlDirPage, 1);

                        PointerNewPde = PdeBase;
                    }
                    else {
                        ASSERT (PointerNewPpe->u.Hard.Valid == 1 ||
                                PointerNewPpe->u.Soft.Transition == 1);

                        if (PointerNewPpe->u.Hard.Valid == 1) {
                            PageDirFrameIndex = MI_GET_PAGE_FRAME_FROM_PTE (PointerNewPpe);
                        }
                        else {
                            PageDirFrameIndex = MI_GET_PAGE_FRAME_FROM_TRANSITION_PTE (PointerNewPpe);
                        }
                    }

                    //
                    // Calculate the address of the new PDE to build.
                    // Note that FirstTime could be true, yet the page
                    // directory page might already be built.
                    //

                    PointerNewPde = (PMMPTE)((ULONG_PTR)PAGE_ALIGN(PointerNewPde) |
                                            BYTE_OFFSET (PointerPde));

                    PdePhysicalPage = PageDirFrameIndex;
                
                    PfnPdPage = MI_PFN_ELEMENT (PdePhysicalPage);

                    UsedPageDirectoryEntries = (PVOID)PfnPdPage;
#endif

                    //
                    // Calculate the address of the PDE in the new process's
                    // page directory page.
                    //

                    PpePdeOffset = MiGetPpePdeOffset(MiGetVirtualAddressMappedByPte(PointerPte));
                    PointerNewPde = &PdeBase[PpePdeOffset];

                    if (PointerNewPde->u.Long == 0) {

                        //
                        // No physical page has been allocated yet, get a page
                        // and map it in as a transition page.  This will
                        // become a page table page for the new process.
                        //

                        ReleasedWorkingSetMutex =
                                MiDoneWithThisPageGetAnother (&PageFrameIndex,
                                                              PointerPde,
                                                              CurrentProcess);

                        if (ReleasedWorkingSetMutex) {

                            do {

                                MiDoesPpeExistAndMakeValid (PointerPpe,
                                                            CurrentProcess,
                                                            FALSE,
                                                            &Waited);
    
                                Waited = 0;
    
                                MiDoesPdeExistAndMakeValid (PointerPde,
                                                            CurrentProcess,
                                                            FALSE,
                                                            &Waited);
                            } while (Waited != 0);
                        }

                        //
                        // Hand initialize this PFN as normal initialization
                        // would do it for the process whose context we are
                        // attached to.
                        //
                        // The PFN lock must be held while initializing the
                        // frame to prevent those scanning the database for
                        // free frames from taking it after we fill in the
                        // u2 field.
                        //

#if defined (_X86PAE_)
                        PdePhysicalPage = PageDirectoryFrames[MiGetPdPteOffset(MiGetVirtualAddressMappedByPte(PointerPte))];
                        PfnPdPage = MI_PFN_ELEMENT (PdePhysicalPage);
#endif

                        MiBuildForkPageTable (PageFrameIndex,
                                              PointerPde,
                                              PointerNewPde,
                                              PdePhysicalPage,
                                              PfnPdPage);

#if defined (_WIN64)
                        MI_INCREMENT_USED_PTES_BY_HANDLE (UsedPageDirectoryEntries);
#endif

                        //
                        // Map the new page table page into the system portion
                        // of the address space.  Note that hyperspace
                        // cannot be used as other operations (allocating
                        // nonpaged pool at DPC level) could cause the
                        // hyperspace page being used to be reused.
                        //

                        ASSERT (NewPteMappedAddress != NULL);

                        MiDownPfnReferenceCount (MdlPage, 1);

                        MdlPage = PageFrameIndex;

                        PointerNewPte = (PMMPTE)MiMapSinglePage (NewPteMappedAddress,
                                                                 MdlPage,
                                                                 MmCached,
                                                                 HighPagePriority);
                    
                        ASSERT (PointerNewPte != NULL);

                        MiUpPfnReferenceCount (MdlPage, 1);
                    }

                    //
                    // Calculate the address of the new PTE to build.
                    // Note that FirstTime could be true, yet the page
                    // table page already built.
                    //

                    PointerNewPte = (PMMPTE)((ULONG_PTR)PAGE_ALIGN(PointerNewPte) |
                                            BYTE_OFFSET (PointerPte));

#ifdef _WIN64
                    UsedPageTableEntries = (PVOID)MI_PFN_ELEMENT((PFN_NUMBER)PointerNewPde->u.Hard.PageFrameNumber);
#else
#if !defined (_X86PAE_)
                    UsedPageTableEntries = (PVOID)&HyperBase->UsedPageTableEntries
                                                [MiGetPteOffset( PointerPte )];
#else
                    UsedPageTableEntries = (PVOID)&HyperBase->UsedPageTableEntries
                                                [MiGetPpePdeOffset(MiGetVirtualAddressMappedByPte(PointerPte))];
#endif
#endif

                }

                //
                // Make the fork prototype PTE location resident.
                //

                if (PAGE_ALIGN (ForkProtoPte) != PAGE_ALIGN (LockedForkPte)) {
                    MiUnlockPagedAddress (LockedForkPte, FALSE);
                    LockedForkPte = ForkProtoPte;
                    MiLockPagedAddress (LockedForkPte, FALSE);
                }

                MiMakeSystemAddressValid (PointerPte, CurrentProcess);

                PteContents = *PointerPte;

                //
                // Check each PTE.
                //

                if (PteContents.u.Long == 0) {
                    NOTHING;

                } else if (PteContents.u.Hard.Valid == 1) {

                    //
                    // Valid.
                    //

                    Pfn2 = MI_PFN_ELEMENT (PteContents.u.Hard.PageFrameNumber);
                    VirtualAddress = MiGetVirtualAddressMappedByPte (PointerPte);
                    WorkingSetIndex = MiLocateWsle (VirtualAddress,
                                                    MmWorkingSetList,
                                                    Pfn2->u1.WsIndex);

                    ASSERT (WorkingSetIndex != WSLE_NULL_INDEX);

                    if (Pfn2->u3.e1.PrototypePte == 1) {

                        //
                        // This PTE is already in prototype PTE format.
                        //

                        //
                        // This is a prototype PTE.  The PFN database does
                        // not contain the contents of this PTE it contains
                        // the contents of the prototype PTE.  This PTE must
                        // be reconstructed to contain a pointer to the
                        // prototype PTE.
                        //
                        // The working set list entry contains information about
                        // how to reconstruct the PTE.
                        //

                        if (MmWsle[WorkingSetIndex].u1.e1.SameProtectAsProto
                                                                        == 0) {

                            //
                            // The protection for the prototype PTE is in the
                            // WSLE.
                            //

                            TempPte.u.Long = 0;
                            TempPte.u.Soft.Protection =
                                MI_GET_PROTECTION_FROM_WSLE(&MmWsle[WorkingSetIndex]);
                            TempPte.u.Soft.PageFileHigh = MI_PTE_LOOKUP_NEEDED;

                        } else {

                            //
                            // The protection is in the prototype PTE.
                            //

                            TempPte.u.Long = MiProtoAddressForPte (
                                                            Pfn2->PteAddress);
 //                            TempPte.u.Proto.ForkType =
 //                                        MmWsle[WorkingSetIndex].u1.e1.ForkType;
                        }

                        TempPte.u.Proto.Prototype = 1;
                        MI_WRITE_INVALID_PTE (PointerNewPte, TempPte);

                        //
                        // A PTE is now non-zero, increment the used page
                        // table entries counter.
                        //

                        MI_INCREMENT_USED_PTES_BY_HANDLE (UsedPageTableEntries);

                        //
                        // Check to see if this is a fork prototype PTE,
                        // and if it is increment the reference count
                        // which is in the longword following the PTE.
                        //

                        if (MiLocateCloneAddress ((PVOID)Pfn2->PteAddress) !=
                                    (PMMCLONE_DESCRIPTOR)NULL) {

                            //
                            // The reference count field, or the prototype PTE
                            // for that matter may not be in the working set.
                            //

                            CloneProto = (PMMCLONE_BLOCK)Pfn2->PteAddress;

                            MiUpCloneProtoRefCount (CloneProto,
                                                    CurrentProcess);

                            if (PAGE_ALIGN (ForkProtoPte) !=
                                                    PAGE_ALIGN (LockedForkPte)) {
                                MiUnlockPagedAddress (LockedForkPte, FALSE);
                                LockedForkPte = ForkProtoPte;
                                MiLockPagedAddress (LockedForkPte, FALSE);
                            }

                            MiMakeSystemAddressValid (PointerPte,
                                                      CurrentProcess);
                        }

                    } else {

                        //
                        // This is a private page, create a fork prototype PTE
                        // which becomes the "prototype" PTE for this page.
                        // The protection is the same as that in the prototype'
                        // PTE so the WSLE does not need to be updated.
                        //

                        MI_MAKE_VALID_PTE_WRITE_COPY (PointerPte);

                        ForkProtoPte->ProtoPte = *PointerPte;
                        ForkProtoPte->CloneRefCount = 2;

                        //
                        // Transform the PFN element to reference this new fork
                        // prototype PTE.
                        //

                        Pfn2->PteAddress = &ForkProtoPte->ProtoPte;
                        Pfn2->u3.e1.PrototypePte = 1;

                        ContainingPte = MiGetPteAddress(&ForkProtoPte->ProtoPte);
                        if (ContainingPte->u.Hard.Valid == 0) {
#if !defined (_WIN64)
                            if (!NT_SUCCESS(MiCheckPdeForPagedPool (&ForkProtoPte->ProtoPte))) {
#endif
                                KeBugCheckEx (MEMORY_MANAGEMENT,
                                              0x61940, 
                                              (ULONG_PTR)&ForkProtoPte->ProtoPte,
                                              (ULONG_PTR)ContainingPte->u.Long,
                                              (ULONG_PTR)MiGetVirtualAddressMappedByPte(&ForkProtoPte->ProtoPte));
#if !defined (_WIN64)
                            }
#endif
                        }
                        Pfn2->PteFrame = MI_GET_PAGE_FRAME_FROM_PTE (ContainingPte);


                        //
                        // Increment the share count for the page containing the
                        // fork prototype PTEs as we have just placed a valid
                        // PTE into the page.
                        //

                        PfnForkPtePage = MI_PFN_ELEMENT (
                                            ContainingPte->u.Hard.PageFrameNumber );

                        MiUpForkPageShareCount (PfnForkPtePage);

                        //
                        // Change the protection in the PFN database to COPY
                        // on write, if writable.
                        //

                        MI_MAKE_PROTECT_WRITE_COPY (Pfn2->OriginalPte);

                        //
                        // Put the protection into the WSLE and mark the WSLE
                        // to indicate that the protection field for the PTE
                        // is the same as the prototype PTE.
                        //

                        MmWsle[WorkingSetIndex].u1.e1.Protection =
                            MI_GET_PROTECTION_FROM_SOFT_PTE(&Pfn2->OriginalPte);

                        MmWsle[WorkingSetIndex].u1.e1.SameProtectAsProto = 1;

                        TempPte.u.Long = MiProtoAddressForPte (Pfn2->PteAddress);
                        TempPte.u.Proto.Prototype = 1;
                        MI_WRITE_INVALID_PTE (PointerNewPte, TempPte);

                        //
                        // A PTE is now non-zero, increment the used page
                        // table entries counter.
                        //

                        MI_INCREMENT_USED_PTES_BY_HANDLE (UsedPageTableEntries);

                        //
                        // One less private page (it's now shared).
                        //

                        CurrentProcess->NumberOfPrivatePages -= 1;

                        ForkProtoPte += 1;
                        NumberOfForkPtes += 1;

                    }

                } else if (PteContents.u.Soft.Prototype == 1) {

                    //
                    // Prototype PTE, check to see if this is a fork
                    // prototype PTE already.  Note that if COW is set,
                    // the PTE can just be copied (fork compatible format).
                    //

                    MI_WRITE_INVALID_PTE (PointerNewPte, PteContents);

                    //
                    // A PTE is now non-zero, increment the used page
                    // table entries counter.
                    //

                    MI_INCREMENT_USED_PTES_BY_HANDLE (UsedPageTableEntries);

                    //
                    // Check to see if this is a fork prototype PTE,
                    // and if it is increment the reference count
                    // which is in the longword following the PTE.
                    //

                    CloneProto = (PMMCLONE_BLOCK)(MiPteToProto(PointerPte));

                    if (MiLocateCloneAddress ((PVOID)CloneProto) !=
                                (PMMCLONE_DESCRIPTOR)NULL) {

                        //
                        // The reference count field, or the prototype PTE
                        // for that matter may not be in the working set.
                        //

                        MiUpCloneProtoRefCount (CloneProto,
                                                CurrentProcess);

                        if (PAGE_ALIGN (ForkProtoPte) !=
                                                PAGE_ALIGN (LockedForkPte)) {
                            MiUnlockPagedAddress (LockedForkPte, FALSE);
                            LockedForkPte = ForkProtoPte;
                            MiLockPagedAddress (LockedForkPte, FALSE);
                        }

                        MiMakeSystemAddressValid (PointerPte,
                                                    CurrentProcess);
                    }

                } else if (PteContents.u.Soft.Transition == 1) {

                    //
                    // Transition.
                    //

                    if (MiHandleForkTransitionPte (PointerPte,
                                                   PointerNewPte,
                                                   ForkProtoPte)) {
                        //
                        // PTE is no longer transition, try again.
                        //

                        continue;
                    }

                    //
                    // A PTE is now non-zero, increment the used page
                    // table entries counter.
                    //

                    MI_INCREMENT_USED_PTES_BY_HANDLE (UsedPageTableEntries);

                    //
                    // One less private page (it's now shared).
                    //

                    CurrentProcess->NumberOfPrivatePages -= 1;

                    ForkProtoPte += 1;
                    NumberOfForkPtes += 1;

                } else {

                    //
                    // Page file format (may be demand zero).
                    //

                    if (IS_PTE_NOT_DEMAND_ZERO (PteContents)) {

                        if (PteContents.u.Soft.Protection == MM_DECOMMIT) {

                            //
                            // This is a decommitted PTE, just move it
                            // over to the new process.  Don't increment
                            // the count of private pages.
                            //

                            MI_WRITE_INVALID_PTE (PointerNewPte, PteContents);
                        } else {

                            //
                            // The PTE is not demand zero, move the PTE to
                            // a fork prototype PTE and make this PTE and
                            // the new processes PTE refer to the fork
                            // prototype PTE.
                            //

                            ForkProtoPte->ProtoPte = PteContents;

                            //
                            // Make the protection write-copy if writable.
                            //

                            MI_MAKE_PROTECT_WRITE_COPY (ForkProtoPte->ProtoPte);

                            ForkProtoPte->CloneRefCount = 2;

                            TempPte.u.Long =
                                 MiProtoAddressForPte (&ForkProtoPte->ProtoPte);

                            TempPte.u.Proto.Prototype = 1;

                            MI_WRITE_INVALID_PTE (PointerPte, TempPte);
                            MI_WRITE_INVALID_PTE (PointerNewPte, TempPte);

                            //
                            // One less private page (it's now shared).
                            //

                            CurrentProcess->NumberOfPrivatePages -= 1;

                            ForkProtoPte += 1;
                            NumberOfForkPtes += 1;
                        }
                    } else {

                        //
                        // The page is demand zero, make the new process's
                        // page demand zero.
                        //

                        MI_WRITE_INVALID_PTE (PointerNewPte, PteContents);
                    }

                    //
                    // A PTE is now non-zero, increment the used page
                    // table entries counter.
                    //

                    MI_INCREMENT_USED_PTES_BY_HANDLE (UsedPageTableEntries);
                }

                PointerPte += 1;
                PointerNewPte += 1;

            }  // end while for PTEs
AllDone:
            NewVad = NewVad->Parent;
        }
        Vad = MiGetNextVad (Vad);

    } // end while for VADs

    //
    // Unlock paged pool page.
    //

    MiUnlockPagedAddress (LockedForkPte, FALSE);

    //
    // Unmap the PD Page and hyper space page.
    //

#if defined (_WIN64)
    MiUnmapSinglePage (PpeBase);
#endif

#if !defined (_X86PAE_)
    MiUnmapSinglePage (PdeBase);
#else
    MmUnmapLockedPages (PdeBase, MdlPageDirectory);
#endif

#if !defined (_WIN64)
    MiUnmapSinglePage (HyperBase);
#endif

    MiUnmapSinglePage (NewPteMappedAddress);

#if defined (_WIN64)
    MiDownPfnReferenceCount (RootPhysicalPage, 1);
#endif

#if defined (_X86PAE_)
    for (i = 0; i < PD_PER_SYSTEM; i += 1) {
        MiDownPfnReferenceCount (PageDirectoryFrames[i], 1);
    }
#else
    MiDownPfnReferenceCount (MdlDirPage, 1);
#endif

#if !defined (_WIN64)
    MiDownPfnReferenceCount (HyperPhysicalPage, 1);
#endif

    MiDownPfnReferenceCount (MdlPage, 1);

    //
    // Make the count of private pages match between the two processes.
    //

    ASSERT ((SPFN_NUMBER)CurrentProcess->NumberOfPrivatePages >= 0);

    ProcessToInitialize->NumberOfPrivatePages =
                                          CurrentProcess->NumberOfPrivatePages;

    ASSERT (NumberOfForkPtes <= CloneDescriptor->NumberOfPtes);

    if (NumberOfForkPtes != 0) {

        //
        // The number of fork PTEs is non-zero, set the values
        // into the structures.
        //

        CloneHeader->NumberOfPtes = NumberOfForkPtes;
        CloneDescriptor->NumberOfReferences = NumberOfForkPtes;
        CloneDescriptor->NumberOfPtes = NumberOfForkPtes;

    } else {

        //
        // There were no fork PTEs created.  Remove the clone descriptor
        // from this process and clean up the related structures.
        // Note - must be holding the working set mutex and not holding
        // the PFN lock.
        //

        MiRemoveClone (CloneDescriptor);

        UNLOCK_WS (CurrentProcess);

        ExFreePool (CloneDescriptor->CloneHeader->ClonePtes);

        ExFreePool (CloneDescriptor->CloneHeader);

        //
        // Return the pool for the global structures referenced by the
        // clone descriptor.
        //

        PsReturnPoolQuota (CurrentProcess,
                           PagedPool,
                           CloneDescriptor->PagedPoolQuotaCharge);

        PsReturnPoolQuota (CurrentProcess, NonPagedPool, sizeof(MMCLONE_HEADER));

        ExFreePool (CloneDescriptor);

        LOCK_WS (CurrentProcess);
    }

    MiDownShareCountFlushEntireTb (PageFrameIndex);

#if defined (_WIN64)
    MiDownShareCountFlushEntireTb (PageDirFrameIndex);
#endif

    PageFrameIndex = (PFN_NUMBER)-1;
    PageDirFrameIndex = (PFN_NUMBER)-1;

    //
    // Copy the clone descriptors from this process to the new process.
    //

    Clone = MiGetFirstClone ();
    CloneList = &FirstNewClone;
    CloneFailed = FALSE;

    while (Clone != (PMMCLONE_DESCRIPTOR)NULL) {

        //
        // Increment the count of processes referencing this clone block.
        //

        MiUpCloneProcessRefCount (Clone);

        do {
            NewClone = ExAllocatePoolWithTag (NonPagedPool,
                                              sizeof( MMCLONE_DESCRIPTOR),
                                              '  mM');

            if (NewClone != NULL) {
                break;
            }

            //
            // There are insufficient resources to continue this operation,
            // however, to properly clean up at this point, all the
            // clone headers must be allocated, so when the cloned process
            // is deleted, the clone headers will be found.  if the pool
            // is not readily available, loop periodically trying for it.
            // Force the clone operation to fail so the pool will soon be
            // released.
            //

            CloneFailed = TRUE;
            status = STATUS_INSUFFICIENT_RESOURCES;

            KeDelayExecutionThread (KernelMode,
                                    FALSE,
                                    (PLARGE_INTEGER)&MmShortTime);
            continue;
        } while (TRUE);

        *NewClone = *Clone;

        *CloneList = NewClone;
        CloneList = &NewClone->Parent;
        Clone = MiGetNextClone (Clone);
    }

    *CloneList = (PMMCLONE_DESCRIPTOR)NULL;

    //
    // Release the working set mutex and the address creation mutex from
    // the current process as all the necessary information is now
    // captured.
    //

    UNLOCK_WS (CurrentProcess);

    CurrentProcess->ForkInProgress = NULL;

    UNLOCK_ADDRESS_SPACE (CurrentProcess);

    //
    // As we have updated many PTEs to clear dirty bits, flush the
    // TB cache.  Note that this was not done every time we changed
    // a valid PTE so other threads could be modifying the address
    // space without causing copy on writes. (Too bad).
    //


    //
    // Attach to the process to initialize and insert the vad and clone
    // descriptors into the tree.
    //

    if (Attached) {
        KeDetachProcess ();
        Attached = FALSE;
    }

    if (PsGetCurrentProcess() != ProcessToInitialize) {
        Attached = TRUE;
        KeAttachProcess (&ProcessToInitialize->Pcb);
    }

    CurrentProcess = ProcessToInitialize;

    //
    // We are now in the context of the new process, build the
    // VAD list and the clone list.
    //

    Vad = FirstNewVad;
    VadInsertFailed = FALSE;

    LOCK_WS (CurrentProcess);

    while (Vad != (PMMVAD)NULL) {

        NextVad = Vad->Parent;

        try {

            if (VadInsertFailed) {
                Vad->u.VadFlags.CommitCharge = MM_MAX_COMMIT;
            }

            MiInsertVad (Vad);

        } except (EXCEPTION_EXECUTE_HANDLER) {

            //
            // Charging quota for the VAD failed, set the
            // remaining quota fields in this VAD and all
            // subsequent VADs to zero so the VADs can be
            // inserted and later deleted.
            //

            VadInsertFailed = TRUE;
            status = GetExceptionCode();

            //
            // Do the loop again for this VAD.
            //

            continue;
        }

        //
        // Update the current virtual size.
        //

        CurrentProcess->VirtualSize += PAGE_SIZE +
                            ((Vad->EndingVpn - Vad->StartingVpn) >> PAGE_SHIFT);

        Vad = NextVad;
    }

    UNLOCK_WS (CurrentProcess);
    //MmUnlockCode (MiCloneProcessAddressSpace, 5000);

    //
    // Update the peak virtual size.
    //

    CurrentProcess->PeakVirtualSize = CurrentProcess->VirtualSize;

    Clone = FirstNewClone;
    TotalPagedPoolCharge = 0;
    TotalNonPagedPoolCharge = 0;

    while (Clone != (PMMCLONE_DESCRIPTOR)NULL) {

        NextClone = Clone->Parent;
        MiInsertClone (Clone);

        //
        // Calculate the page pool and non-paged pool to charge for these
        // operations.
        //

        TotalPagedPoolCharge += Clone->PagedPoolQuotaCharge;
        TotalNonPagedPoolCharge += sizeof(MMCLONE_HEADER);

        Clone = NextClone;
    }

    if (CloneFailed || VadInsertFailed) {

        CurrentProcess->ForkWasSuccessful = MM_FORK_FAILED;

        if (Attached) {
            KeDetachProcess ();
        }
        KdPrint(("MMFORK: vad insert failed\n"));

        return status;
    }

    try {

        PageTablePage = 1;
        PsChargePoolQuota (CurrentProcess, PagedPool, TotalPagedPoolCharge);
        PageTablePage = 0;
        PsChargePoolQuota (CurrentProcess, NonPagedPool, TotalNonPagedPoolCharge);

    } except (EXCEPTION_EXECUTE_HANDLER) {

        if (PageTablePage == 0) {
            PsReturnPoolQuota (CurrentProcess, PagedPool, TotalPagedPoolCharge);
        }
        KdPrint(("MMFORK: pool quota failed\n"));

        CurrentProcess->ForkWasSuccessful = MM_FORK_FAILED;

        if (Attached) {
            KeDetachProcess ();
        }
        return GetExceptionCode();
    }

    ASSERT (ProcessToClone->ForkWasSuccessful == MM_FORK_SUCCEEDED);
    ASSERT (CurrentProcess->ForkWasSuccessful == MM_FORK_SUCCEEDED);

    if (Attached) {
        KeDetachProcess ();
    }

#if DBG
    if (MmDebug & MM_DBG_FORK) {
        DbgPrint("ending clone operation process to clone = %lx\n",
            ProcessToClone);
    }
#endif //DBG

    return STATUS_SUCCESS;

    //
    // Error returns.
    //

ErrorReturn4:
        if (PageTablePage == 2) {
            NOTHING;
        }
        else if (PageTablePage == 1) {
            PsReturnPoolQuota (CurrentProcess, PagedPool, sizeof(MMCLONE_BLOCK) *
                                NumberOfPrivatePages);
        }
        else {
            ASSERT (PageTablePage == 0);
            PsReturnPoolQuota (CurrentProcess, PagedPool, sizeof(MMCLONE_BLOCK) *
                                NumberOfPrivatePages);
            PsReturnPoolQuota (CurrentProcess, NonPagedPool, sizeof(MMCLONE_HEADER));
        }

        NewVad = FirstNewVad;
        while (NewVad != NULL) {
            Vad = NewVad->Parent;
            ExFreePool (NewVad);
            NewVad = Vad;
        }

        ExFreePool (CloneDescriptor);
ErrorReturn3:
        ExFreePool (CloneHeader);
ErrorReturn2:
        ExFreePool (CloneProtos);
ErrorReturn1:
        UNLOCK_ADDRESS_SPACE (CurrentProcess);
        ASSERT (CurrentProcess->ForkWasSuccessful == MM_FORK_SUCCEEDED);
        if (Attached) {
            KeDetachProcess ();
        }
        return status;
}

ULONG
MiDecrementCloneBlockReference (
    IN PMMCLONE_DESCRIPTOR CloneDescriptor,
    IN PMMCLONE_BLOCK CloneBlock,
    IN PEPROCESS CurrentProcess
    )

/*++

Routine Description:

    This routine decrements the reference count field of a "fork prototype
    PTE" (clone-block).  If the reference count becomes zero, the reference
    count for the clone-descriptor is decremented and if that becomes zero,
    it is deallocated and the number of process count for the clone header is
    decremented.  If the number of process count becomes zero, the clone
    header is deallocated.

Arguments:

    CloneDescriptor - Supplies the clone descriptor which describes the
                      clone block.

    CloneBlock - Supplies the clone block to decrement the reference count of.

    CurrentProcess - Supplies the current process.

Return Value:

    TRUE if the working set mutex was released, FALSE if it was not.

Environment:

    Kernel mode, APCs disabled, working set mutex and PFN mutex held.

--*/

{

    ULONG MutexReleased;
    MMPTE CloneContents;
    PMMPFN Pfn3;
    KIRQL OldIrql;
    LONG NewCount;
    LOGICAL WsHeldSafe;

    MutexReleased = FALSE;
    OldIrql = APC_LEVEL;

    MutexReleased = MiMakeSystemAddressValidPfnWs (CloneBlock, CurrentProcess);

    while (CurrentProcess->ForkInProgress) {
        MiWaitForForkToComplete (CurrentProcess);
        MiMakeSystemAddressValidPfnWs (CloneBlock, CurrentProcess);
        MutexReleased = TRUE;
    }

    CloneBlock->CloneRefCount -= 1;
    NewCount = CloneBlock->CloneRefCount;

    ASSERT (NewCount >= 0);

    if (NewCount == 0) {
        CloneContents = CloneBlock->ProtoPte;
    } else {
        CloneContents = ZeroPte;
    }

    if ((NewCount == 0) && (CloneContents.u.Long != 0)) {

        //
        // The last reference to a fork prototype PTE
        // has been removed.  Deallocate any page file
        // space and the transition page, if any.
        //


        //
        // Assert that the page is no longer valid.
        //

        ASSERT (CloneContents.u.Hard.Valid == 0);

        //
        // Assert that the PTE is not in subsection format (doesn't point
        // to a file).
        //

        ASSERT (CloneContents.u.Soft.Prototype == 0);

        if (CloneContents.u.Soft.Transition == 1) {

            //
            // Prototype PTE in transition, put the page
            // on the free list.
            //

            Pfn3 = MI_PFN_ELEMENT (CloneContents.u.Trans.PageFrameNumber);
            MI_SET_PFN_DELETED (Pfn3);

            MiDecrementShareCount (Pfn3->PteFrame);

            //
            // Check the reference count for the page, if the reference
            // count is zero and the page is not on the freelist,
            // move the page to the free list, if the reference
            // count is not zero, ignore this page.
            // When the reference count goes to zero, it will be placed on the
            // free list.
            //

            if ((Pfn3->u3.e2.ReferenceCount == 0) &&
                (Pfn3->u3.e1.PageLocation != FreePageList)) {

                MiUnlinkPageFromList (Pfn3);
                MiReleasePageFileSpace (Pfn3->OriginalPte);
                MiInsertPageInList (MmPageLocationList[FreePageList],
                             MI_GET_PAGE_FRAME_FROM_TRANSITION_PTE(&CloneContents));
            }
        } else {

            if (IS_PTE_NOT_DEMAND_ZERO (CloneContents)) {
                MiReleasePageFileSpace (CloneContents);
            }
        }
    }

    //
    // Decrement the number of references to the
    // clone descriptor.
    //

    CloneDescriptor->NumberOfReferences -= 1;

    if (CloneDescriptor->NumberOfReferences == 0) {

        //
        // There are no longer any PTEs in this process which refer
        // to the fork prototype PTEs for this clone descriptor.
        // Remove the CloneDescriptor and decrement the CloneHeader
        // number of process's reference count.
        //

        CloneDescriptor->CloneHeader->NumberOfProcessReferences -= 1;

        MiRemoveClone (CloneDescriptor);

        if (CloneDescriptor->CloneHeader->NumberOfProcessReferences == 0) {

            //
            // There are no more processes pointing to this fork header
            // blow it away.
            //

            UNLOCK_PFN (OldIrql);

            //
            // The working set lock may have been acquired safely or unsafely
            // by our caller.  Handle both cases here and below.
            //

            UNLOCK_WS_REGARDLESS (CurrentProcess, WsHeldSafe);

            MutexReleased = TRUE;


#if DBG
            {

            ULONG i;
            PMMCLONE_BLOCK OldCloneBlock;

            OldCloneBlock = CloneDescriptor->CloneHeader->ClonePtes;
            for (i = 0; i < CloneDescriptor->CloneHeader->NumberOfPtes; i += 1) {
                if (OldCloneBlock->CloneRefCount != 0) {
                    DbgPrint("fork block with non zero ref count %lx %lx %lx\n",
                        OldCloneBlock, CloneDescriptor,
                        CloneDescriptor->CloneHeader);
                    KeBugCheckEx (MEMORY_MANAGEMENT, 1, 0, 0, 0);
                }
            }

            if (MmDebug & MM_DBG_FORK) {
                DbgPrint("removing clone header at address %lx\n",
                        CloneDescriptor->CloneHeader);
            }

            }
#endif //DBG

            ExFreePool (CloneDescriptor->CloneHeader->ClonePtes);

            ExFreePool (CloneDescriptor->CloneHeader);

            //
            // The working set lock may have been acquired safely or unsafely
            // by our caller.  Reacquire it in the same manner our caller did.
            //

            LOCK_WS_REGARDLESS (CurrentProcess, WsHeldSafe);

            LOCK_PFN (OldIrql);
        }

#if DBG
        if (MmDebug & MM_DBG_FORK) {
          DbgPrint("removing clone descriptor at address %lx\n",CloneDescriptor);
        }
#endif //DBG

        //
        // Return the pool for the global structures referenced by the
        // clone descriptor.
        //

        UNLOCK_PFN (OldIrql);

        if (CurrentProcess->ForkWasSuccessful == MM_FORK_SUCCEEDED) {

            PsReturnPoolQuota (CurrentProcess,
                               PagedPool,
                               CloneDescriptor->PagedPoolQuotaCharge);
    
            PsReturnPoolQuota (CurrentProcess,
                               NonPagedPool,
                               sizeof(MMCLONE_HEADER));
        }

        ExFreePool (CloneDescriptor);
        LOCK_PFN (OldIrql);
    }

    return MutexReleased;
}

VOID
MiWaitForForkToComplete (
    IN PEPROCESS CurrentProcess
    )

/*++

Routine Description:

    This routine waits for the current process to complete a fork
    operation.

Arguments:

    CurrentProcess - Supplies the current process value.

Return Value:

    None.

Environment:

    Kernel mode, APCs disabled, working set mutex and PFN mutex held.

--*/

{
    ULONG OldIrql;
    LOGICAL WsHeldSafe;

    //
    // A fork operation is in progress and the count of clone-blocks
    // and other structures may not be changed.  Release the mutexes
    // and wait for the address creation mutex which governs the
    // fork operation.
    //

    UNLOCK_PFN (APC_LEVEL);

    //
    // The working set mutex may have been acquired safely or unsafely
    // by our caller.  Handle both cases here and below, carefully making sure
    // that the OldIrql left in the WS mutex on return is the same as on entry.
    //

    OldIrql = CurrentProcess->WorkingSetLock.OldIrql;

    UNLOCK_WS_REGARDLESS (CurrentProcess, WsHeldSafe);

    //
    // Explicitly acquire the working set lock safely as it may be released and
    // reacquired by our caller.
    //

    LOCK_ADDRESS_SPACE (CurrentProcess);

    //
    // The working set lock may have been acquired safely or unsafely
    // by our caller.  Reacquire it in the same manner our caller did.
    //

    LOCK_WS_REGARDLESS (CurrentProcess, WsHeldSafe);

    //
    // Release the address creation mutex, the working set mutex
    // must be held to set the ForkInProgress field.
    //

    UNLOCK_ADDRESS_SPACE (CurrentProcess);

    //
    // Ensure the previous IRQL is correctly set to its entry value.
    //

    CurrentProcess->WorkingSetLock.OldIrql = OldIrql;

    //
    // Get the PFN lock again.
    //

    LOCK_PFN (OldIrql);
    return;
}
#if DBG
VOID
CloneTreeWalk (
    PMMCLONE_DESCRIPTOR Start
    )

{
    Start;
    NodeTreeWalk ( (PMMADDRESS_NODE)(PsGetCurrentProcess()->CloneRoot));
    return;
}
#endif //DBG

VOID
MiUpPfnReferenceCount (
    IN PFN_NUMBER Page,
    IN USHORT Count
    )

    // non paged helper routine.

{
    KIRQL OldIrql;
    PMMPFN Pfn1;

    Pfn1 = MI_PFN_ELEMENT (Page);
    LOCK_PFN (OldIrql);
    Pfn1->u3.e2.ReferenceCount += Count;
    UNLOCK_PFN (OldIrql);
    return;
}

VOID
MiDownPfnReferenceCount (
    IN PFN_NUMBER Page,
    IN USHORT Count
    )

    // non paged helper routine.

{
    USHORT i;
    KIRQL OldIrql;

    LOCK_PFN (OldIrql);
    for (i = 0; i < Count; i += 1) {
        MiDecrementReferenceCount (Page);
    }
    UNLOCK_PFN (OldIrql);
    return;
}

VOID
MiUpControlAreaRefs (
    IN PCONTROL_AREA ControlArea
    )

{
    KIRQL OldIrql;

    LOCK_PFN (OldIrql);

    ControlArea->NumberOfMappedViews += 1;
    ControlArea->NumberOfUserReferences += 1;

    UNLOCK_PFN (OldIrql);
    return;
}


ULONG
MiDoneWithThisPageGetAnother (
    IN PPFN_NUMBER PageFrameIndex,
    IN PMMPTE PointerPde,
    IN PEPROCESS CurrentProcess
    )

{
    KIRQL OldIrql;
    ULONG ReleasedMutex;

    LOCK_PFN (OldIrql);

    if (*PageFrameIndex != (PFN_NUMBER)-1) {

        //
        // Decrement the share count of the last page which
        // we operated on.
        //

        MiDecrementShareCountOnly (*PageFrameIndex);
    }

    ReleasedMutex =
                  MiEnsureAvailablePageOrWait (
                                        CurrentProcess,
                                        NULL);

    *PageFrameIndex = MiRemoveZeroPage (
                   MI_PAGE_COLOR_PTE_PROCESS (PointerPde,
                                              &CurrentProcess->NextPageColor));

    UNLOCK_PFN (OldIrql);
    return ReleasedMutex;
}

VOID
MiUpCloneProcessRefCount (
    IN PMMCLONE_DESCRIPTOR Clone
    )
{
    KIRQL OldIrql;

    LOCK_PFN (OldIrql);

    Clone->CloneHeader->NumberOfProcessReferences += 1;

    UNLOCK_PFN (OldIrql);
    return;
}

VOID
MiUpCloneProtoRefCount (
    IN PMMCLONE_BLOCK CloneProto,
    IN PEPROCESS CurrentProcess
    )

{
    KIRQL OldIrql;

    LOCK_PFN (OldIrql);

    MiMakeSystemAddressValidPfnWs (CloneProto,
                                   CurrentProcess);

    CloneProto->CloneRefCount += 1;

    UNLOCK_PFN (OldIrql);
    return;
}

ULONG
MiHandleForkTransitionPte (
    IN PMMPTE PointerPte,
    IN PMMPTE PointerNewPte,
    IN PMMCLONE_BLOCK ForkProtoPte
    )

{
    KIRQL OldIrql;
    PMMPFN Pfn2;
    MMPTE PteContents;
    PMMPTE ContainingPte;
    PFN_NUMBER PageTablePage;
    MMPTE TempPte;
    PMMPFN PfnForkPtePage;


    LOCK_PFN (OldIrql);

    //
    // Now that we have the PFN mutex which prevents pages from
    // leaving the transition state, examine the PTE again to
    // ensure that it is still transition.
    //

    PteContents = *(volatile PMMPTE)PointerPte;

    if ((PteContents.u.Soft.Transition == 0) ||
        (PteContents.u.Soft.Prototype == 1)) {

        //
        // The PTE is no longer in transition... do this
        // loop again.
        //

        UNLOCK_PFN (OldIrql);
        return TRUE;

    } else {

        //
        // The PTE is still in transition, handle like a
        // valid PTE.
        //

        Pfn2 = MI_PFN_ELEMENT (PteContents.u.Trans.PageFrameNumber);

        //
        // Assertion that PTE is not in prototype PTE format.
        //

        ASSERT (Pfn2->u3.e1.PrototypePte != 1);

        //
        // This is a private page in transition state,
        // create a fork prototype PTE
        // which becomes the "prototype" PTE for this page.
        //

        ForkProtoPte->ProtoPte = PteContents;

        //
        // Make the protection write-copy if writable.
        //

        MI_MAKE_PROTECT_WRITE_COPY (ForkProtoPte->ProtoPte);

        ForkProtoPte->CloneRefCount = 2;

        //
        // Transform the PFN element to reference this new fork
        // prototype PTE.
        //

        //
        // Decrement the share count for the page table
        // page which contains the PTE as it is no longer
        // valid or in transition.
        //
        Pfn2->PteAddress = &ForkProtoPte->ProtoPte;
        Pfn2->u3.e1.PrototypePte = 1;

        //
        // Make original PTE copy on write.
        //

        MI_MAKE_PROTECT_WRITE_COPY (Pfn2->OriginalPte);

        ContainingPte = MiGetPteAddress(&ForkProtoPte->ProtoPte);

        if (ContainingPte->u.Hard.Valid == 0) {
#if !defined (_WIN64)
            if (!NT_SUCCESS(MiCheckPdeForPagedPool (&ForkProtoPte->ProtoPte))) {
#endif
                KeBugCheckEx (MEMORY_MANAGEMENT,
                              0x61940, 
                              (ULONG_PTR)&ForkProtoPte->ProtoPte,
                              (ULONG_PTR)ContainingPte->u.Long,
                              (ULONG_PTR)MiGetVirtualAddressMappedByPte(&ForkProtoPte->ProtoPte));
#if !defined (_WIN64)
            }
#endif
        }

        PageTablePage = Pfn2->PteFrame;

        Pfn2->PteFrame = MI_GET_PAGE_FRAME_FROM_PTE (ContainingPte);

        //
        // Increment the share count for the page containing
        // the fork prototype PTEs as we have just placed
        // a transition PTE into the page.
        //

        PfnForkPtePage = MI_PFN_ELEMENT (
                        ContainingPte->u.Hard.PageFrameNumber );

        PfnForkPtePage->u2.ShareCount += 1;

        TempPte.u.Long =
                    MiProtoAddressForPte (Pfn2->PteAddress);
        TempPte.u.Proto.Prototype = 1;
        MI_WRITE_INVALID_PTE (PointerPte, TempPte);
        MI_WRITE_INVALID_PTE (PointerNewPte, TempPte);

        //
        // Decrement the share count for the page table
        // page which contains the PTE as it is no longer
        // valid or in transition.
        //

        MiDecrementShareCount (PageTablePage);
    }
    UNLOCK_PFN (OldIrql);
    return FALSE;
}

VOID
MiDownShareCountFlushEntireTb (
    IN PFN_NUMBER PageFrameIndex
    )

{
    KIRQL OldIrql;

    LOCK_PFN (OldIrql);

    if (PageFrameIndex != (PFN_NUMBER)-1) {

        //
        // Decrement the share count of the last page which
        // we operated on.
        //

        MiDecrementShareCountOnly (PageFrameIndex);
    }

    KeFlushEntireTb (FALSE, FALSE);
    UNLOCK_PFN (OldIrql);
    return;
}

VOID
MiUpForkPageShareCount(
    IN PMMPFN PfnForkPtePage
    )
{
    KIRQL OldIrql;

    LOCK_PFN (OldIrql);
    PfnForkPtePage->u2.ShareCount += 1;

    UNLOCK_PFN (OldIrql);
    return;
}

VOID
MiBuildForkPageTable(
    IN PFN_NUMBER PageFrameIndex,
    IN PMMPTE PointerPde,
    IN PMMPTE PointerNewPde,
    IN PFN_NUMBER PdePhysicalPage,
    IN PMMPFN PfnPdPage
    )
{
    KIRQL OldIrql;
    PMMPFN Pfn1;

    Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);

    //
    // The PFN lock must be held while initializing the
    // frame to prevent those scanning the database for
    // free frames from taking it after we fill in the
    // u2 field.
    //

    LOCK_PFN (OldIrql);

    Pfn1->OriginalPte = DemandZeroPde;
    Pfn1->u2.ShareCount = 1;
    Pfn1->u3.e2.ReferenceCount = 1;
    Pfn1->PteAddress = PointerPde;
    Pfn1->u3.e1.Modified = 1;
    Pfn1->u3.e1.PageLocation = ActiveAndValid;
    Pfn1->PteFrame = PdePhysicalPage;

    //
    // Increment the share count for the page containing
    // this PTE as the PTE is in transition.
    //

    PfnPdPage->u2.ShareCount += 1;

    //
    // Put the PDE into the transition state as it is not
    // really mapped and decrement share count does not
    // put private pages into transition, only prototypes.
    //

    MI_WRITE_INVALID_PTE (PointerNewPde, TransitionPde);

    //
    // Make the PTE owned by user mode.
    //

#ifndef _ALPHA_
    MI_SET_OWNER_IN_PTE (PointerNewPde, UserMode);
#endif //_ALPHA_
    PointerNewPde->u.Trans.PageFrameNumber = PageFrameIndex;

    UNLOCK_PFN (OldIrql);
}

#if defined (_X86PAE_)
VOID
MiRetrievePageDirectoryFrames(
    IN PFN_NUMBER RootPhysicalPage,
    OUT PPFN_NUMBER PageDirectoryFrames
    )
{
    ULONG i;
    KIRQL OldIrql;
    PMMPTE PointerPte;

    PointerPte = (PMMPTE)MiMapPageInHyperSpace (RootPhysicalPage, &OldIrql);

    for (i = 0; i < PD_PER_SYSTEM; i += 1) {
        PageDirectoryFrames[i] = MI_GET_PAGE_FRAME_FROM_PTE (PointerPte);
        PointerPte += 1;
    }

    MiUnmapPageInHyperSpace (OldIrql);

    return;
}
#endif
