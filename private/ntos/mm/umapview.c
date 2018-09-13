/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

   umapview.c

Abstract:

    This module contains the routines which implement the
    NtUnmapViewOfSection service.

Author:

    Lou Perazzoli (loup) 22-May-1989

Revision History:

    Landy Wang (landyw) 08-April-1998 : Modifications for 3-level 64-bit NT.

--*/

#include "mi.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,NtUnmapViewOfSection)
#pragma alloc_text(PAGE,MmUnmapViewOfSection)
#endif


NTSTATUS
NtUnmapViewOfSection(
    IN HANDLE ProcessHandle,
    IN PVOID BaseAddress
     )

/*++

Routine Description:

    This function unmaps a previously created view to a section.

Arguments:

    ProcessHandle - Supplies an open handle to a process object.

    BaseAddress - Supplies the base address of the view.

Return Value:

    Returns the status

    TBS


--*/

{
    PEPROCESS Process;
    KPROCESSOR_MODE PreviousMode;
    NTSTATUS Status;

    PAGED_CODE();

    PreviousMode = KeGetPreviousMode();

    if ((PreviousMode == UserMode) && (BaseAddress > MM_HIGHEST_USER_ADDRESS)) {
        return STATUS_NOT_MAPPED_VIEW;
    }

    Status = ObReferenceObjectByHandle ( ProcessHandle,
                                         PROCESS_VM_OPERATION,
                                         PsProcessType,
                                         PreviousMode,
                                         (PVOID *)&Process,
                                         NULL );

    if (!NT_SUCCESS(Status)) {
        return Status;
    }

    Status = MmUnmapViewOfSection ( Process, BaseAddress );
    ObDereferenceObject (Process);

    return Status;
}

NTSTATUS
MmUnmapViewOfSection(
    IN PEPROCESS Process,
    IN PVOID BaseAddress
     )

/*++

Routine Description:

    This function unmaps a previously created view to a section.

Arguments:

    Process - Supplies a referenced pointer to a process object.

    BaseAddress - Supplies the base address of the view.

Return Value:

    Returns the status

    TBS


--*/

{
    PMMVAD Vad;
    PMMVAD PreviousVad;
    PMMVAD NextVad;
    SIZE_T RegionSize;
    PVOID UnMapImageBase;
    NTSTATUS status;
    BOOLEAN Attached = FALSE;

    PAGED_CODE();

    UnMapImageBase = NULL;

    //
    // If the specified process is not the current process, attach
    // to the specified process.
    //

    if (PsGetCurrentProcess() != Process) {
        KeAttachProcess (&Process->Pcb);
        Attached = TRUE;
    }

    //
    // Get the address creation mutex to block multiple threads from
    // creating or deleting address space at the same time and
    // get the working set mutex so virtual address descriptors can
    // be inserted and walked.
    // Raise IRQL to block APCs.
    //
    // Get the working set mutex, no page faults allowed for now until
    // working set mutex released.
    //


    LOCK_WS_AND_ADDRESS_SPACE (Process);

    //
    // Make sure the address space was not deleted, if so, return an error.
    //

    if (Process->AddressSpaceDeleted != 0) {
        status = STATUS_PROCESS_IS_TERMINATING;
        goto ErrorReturn;
    }

    //
    // Find the associated vad.
    //

    Vad = MiLocateAddress (BaseAddress);

    if ((Vad == (PMMVAD)NULL) || (Vad->u.VadFlags.PrivateMemory)) {

        //
        // No Virtual Address Descriptor located for Base Address.
        //

        status = STATUS_NOT_MAPPED_VIEW;
        goto ErrorReturn;
    }

    if (Vad->u.VadFlags.NoChange == 1) {

        //
        // An attempt is being made to delete a secured VAD, check
        // the whole VAD to see if this deletion is allowed.
        //

        status = MiCheckSecuredVad ((PMMVAD)Vad,
                                    MI_VPN_TO_VA (Vad->StartingVpn),
                                    ((Vad->EndingVpn - Vad->StartingVpn) << PAGE_SHIFT) +
                                            (PAGE_SIZE - 1),
                                    MM_SECURE_DELETE_CHECK);

        if (!NT_SUCCESS (status)) {
            goto ErrorReturn;
        }
    }

    //
    // If this Vad is for an image section, then
    // get the base address of the section
    //

    if ((Vad->u.VadFlags.ImageMap == 1) && (Process == PsGetCurrentProcess())) {
        UnMapImageBase = MI_VPN_TO_VA (Vad->StartingVpn);
    }

    RegionSize = PAGE_SIZE + ((Vad->EndingVpn - Vad->StartingVpn) << PAGE_SHIFT);

    PreviousVad = MiGetPreviousVad (Vad);
    NextVad = MiGetNextVad (Vad);


    MiRemoveVad (Vad);

    //
    // Return commitment for page table pages if possible.
    //

    MiReturnPageTablePageCommitment (MI_VPN_TO_VA (Vad->StartingVpn),
                                     MI_VPN_TO_VA_ENDING (Vad->EndingVpn),
                                     Process,
                                     PreviousVad,
                                     NextVad);

    MiRemoveMappedView (Process, Vad);

#if defined(_MIALT4K_)

    if (Process->Wow64Process != NULL) {

        UNLOCK_WS_UNSAFE (Process);

        MiDeleteFor4kPage(MI_VPN_TO_VA (Vad->StartingVpn),
                          MI_VPN_TO_VA_ENDING (Vad->EndingVpn),
                          Process);

        LOCK_WS_UNSAFE (Process);

    }

#endif

    ExFreePool (Vad);

    //
    // Update the current virtual size in the process header.
    //

    Process->VirtualSize -= RegionSize;
    status = STATUS_SUCCESS;

ErrorReturn:

    UNLOCK_WS_AND_ADDRESS_SPACE (Process);

    if ( UnMapImageBase ) {
        DbgkUnMapViewOfSection(UnMapImageBase);
    }
    if (Attached == TRUE) {
        KeDetachProcess();
    }

    return status;
}

VOID
MiRemoveMappedView (
    IN PEPROCESS CurrentProcess,
    IN PMMVAD Vad
    )

/*++

Routine Description:

    This function removes the mapping from the current process's
    address space.  The physical VAD may be a normal mapping (backed by
    a control area) or it may have no control area (it was mapped by a driver).

Arguments:

    Process - Supplies a referenced pointer to the current process object.

    Vad - Supplies the VAD which maps the view.

Return Value:

    None.

Environment:

    APC level, working set mutex and address creation mutex held.

    NOTE:  THE WORKING SET MUTEXES MAY BE RELEASED THEN REACQUIRED!!!!

           SINCE MiCheckControlArea releases unsafe, the WS mutex must be
           acquired UNSAFE.

--*/

{
    KIRQL OldIrql;
    BOOLEAN DereferenceSegment;
    PCONTROL_AREA ControlArea;
    PMMPTE PointerPte;
    PMMPTE PointerPde;
    PMMPTE LastPte;
    PFN_NUMBER PdePage;
    PKEVENT PurgeEvent;
    PVOID TempVa;
    BOOLEAN DeleteOnClose;
    MMPTE_FLUSH_LIST PteFlushList;
    PVOID UsedPageTableHandle;
    PLIST_ENTRY NextEntry;
    PMI_PHYSICAL_VIEW PhysicalView;
    PVOID PhysicalPool;
#if defined (_WIN64)
    PMMPTE PointerPpe;
    PVOID UsedPageDirectoryHandle;
#endif

    DereferenceSegment = FALSE;
    PurgeEvent = NULL;
    DeleteOnClose = FALSE;
    PhysicalPool = NULL;

    ControlArea = Vad->ControlArea;

    if (Vad->u.VadFlags.PhysicalMapping == 1) {

        if (Vad->u4.Banked) {
            ExFreePool (Vad->u4.Banked);
        }

#ifdef LARGE_PAGES
        if (Vad->u.VadFlags.LargePages == 1) {

            //
            // Delete the subsection allocated to hold the large pages.
            //

            ExFreePool (Vad->FirstPrototypePte);
            Vad->FirstPrototypePte = NULL;
            KeFlushEntireTb (TRUE, FALSE);
            LOCK_PFN (OldIrql);
        } else {

#endif //LARGE_PAGES

            //
            // This is a physical memory view.  The pages map physical memory
            // and are not accounted for in the working set list or in the PFN
            // database.
            //

            LOCK_AWE (CurrentProcess, OldIrql);

            NextEntry = CurrentProcess->PhysicalVadList.Flink;
            while (NextEntry != &CurrentProcess->PhysicalVadList) {
    
                PhysicalView = CONTAINING_RECORD(NextEntry,
                                                 MI_PHYSICAL_VIEW,
                                                 ListEntry);
    
                if (Vad == PhysicalView->Vad) {
                    RemoveEntryList (NextEntry);
                    PhysicalPool = (PVOID)PhysicalView;
                    break;
                }
                NextEntry = NextEntry->Flink;
            }

            UNLOCK_AWE (CurrentProcess, OldIrql);

            //
            // Set count so only flush entire TB operations are performed.
            //

            PteFlushList.Count = MM_MAXIMUM_FLUSH_COUNT;

            LOCK_PFN (OldIrql);

            //
            // Remove the PTES from the address space.
            //

            PointerPde = MiGetPdeAddress (MI_VPN_TO_VA (Vad->StartingVpn));
            PdePage = MI_GET_PAGE_FRAME_FROM_PTE (PointerPde);
            PointerPte = MiGetPteAddress (MI_VPN_TO_VA (Vad->StartingVpn));
            LastPte = MiGetPteAddress (MI_VPN_TO_VA (Vad->EndingVpn));

            UsedPageTableHandle = MI_GET_USED_PTES_HANDLE (MI_VPN_TO_VA (Vad->StartingVpn));

            while (PointerPte <= LastPte) {

                if (MiIsPteOnPdeBoundary (PointerPte)) {

                    PointerPde = MiGetPteAddress (PointerPte);
                    PdePage = MI_GET_PAGE_FRAME_FROM_PTE (PointerPde);

                    UsedPageTableHandle = MI_GET_USED_PTES_HANDLE (MiGetVirtualAddressMappedByPte (PointerPte));
                }

                MI_WRITE_INVALID_PTE (PointerPte, ZeroPte);
                MiDecrementShareAndValidCount (PdePage);

                //
                // Decrement the count of non-zero page table entries for this
                // page table.
                //

                MI_DECREMENT_USED_PTES_BY_HANDLE (UsedPageTableHandle);

                //
                // If all the entries have been eliminated from the previous
                // page table page, delete the page table page itself.  And if
                // this results in an empty page directory page, then delete
                // that too.
                //

                if (MI_GET_USED_PTES_FROM_HANDLE(UsedPageTableHandle) == 0) {

                    TempVa = MiGetVirtualAddressMappedByPte(PointerPde);

                    PteFlushList.Count = MM_MAXIMUM_FLUSH_COUNT;

                    MiDeletePte (PointerPde,
                                 TempVa,
                                 FALSE,
                                 CurrentProcess,
                                 (PMMPTE)NULL,
                                 &PteFlushList);

                    //
                    // Add back in the private page MiDeletePte subtracted.
                    //
    
                    CurrentProcess->NumberOfPrivatePages += 1;

#if defined (_WIN64)
                    UsedPageDirectoryHandle = MI_GET_USED_PTES_HANDLE (PointerPte);
    
                    MI_DECREMENT_USED_PTES_BY_HANDLE (UsedPageDirectoryHandle);

                    if (MI_GET_USED_PTES_FROM_HANDLE(UsedPageDirectoryHandle) == 0) {
    
                        PointerPpe = MiGetPdeAddress(PointerPte);
                        TempVa = MiGetVirtualAddressMappedByPte(PointerPpe);
    
                        PteFlushList.Count = MM_MAXIMUM_FLUSH_COUNT;
    
                        MiDeletePte (PointerPpe,
                                     TempVa,
                                     FALSE,
                                     CurrentProcess,
                                     (PMMPTE)NULL,
                                     &PteFlushList);

                        //
                        // Add back in the private page MiDeletePte subtracted.
                        //
        
                        CurrentProcess->NumberOfPrivatePages += 1;
                    }
#endif
                }
                PointerPte += 1;
            }
            KeFlushEntireTb (TRUE, FALSE);

#ifdef LARGE_PAGES
        }
#endif //LARGE_PAGES
    } else {

        if (Vad->u2.VadFlags2.ExtendableFile) {
            PMMEXTEND_INFO exinfo = NULL;
            ExAcquireFastMutexUnsafe (&MmSectionBasedMutex);
            ASSERT (Vad->ControlArea->Segment->ExtendInfo == Vad->u4.ExtendedInfo);
            Vad->u4.ExtendedInfo->ReferenceCount -= 1;
            if (Vad->u4.ExtendedInfo->ReferenceCount == 0) {
                exinfo = Vad->u4.ExtendedInfo;
                Vad->ControlArea->Segment->ExtendInfo = NULL;
            }
            ExReleaseFastMutexUnsafe (&MmSectionBasedMutex);
            if (exinfo) {
                ExFreePool (exinfo);
            }
        }

        LOCK_PFN (OldIrql);
        MiDeleteVirtualAddresses (MI_VPN_TO_VA (Vad->StartingVpn),
                                  MI_VPN_TO_VA_ENDING (Vad->EndingVpn),
                                  FALSE,
                                  Vad);
    }

    //
    // Only physical VADs mapped by drivers don't have control areas.
    // If this view has a control area, the view count must be decremented now.
    //

    if (ControlArea) {

        //
        // Decrement the count of the number of views for the
        // Segment object.  This requires the PFN mutex to be held (it is
        // already).
        //
    
        ControlArea->NumberOfMappedViews -= 1;
        ControlArea->NumberOfUserReferences -= 1;
    
        //
        // Check to see if the control area (segment) should be deleted.
        // This routine releases the PFN lock.
        //
    
        MiCheckControlArea (ControlArea, CurrentProcess, OldIrql);
    }
    else {

        UNLOCK_PFN (OldIrql);

        //
        // Even though it says short VAD in VadFlags, it better be a long VAD.
        //

        ASSERT (Vad->u.VadFlags.PhysicalMapping == 1);
        ASSERT (Vad->u4.Banked == NULL);
        ASSERT (Vad->ControlArea == NULL);
        ASSERT (Vad->FirstPrototypePte == NULL);
    }
    
    if (PhysicalPool != NULL) {
        ExFreePool (PhysicalPool);
    }

    return;
}

VOID
MiPurgeImageSection (
    IN PCONTROL_AREA ControlArea,
    IN PEPROCESS Process OPTIONAL
    )

/*++

Routine Description:

    This function locates subsections within an image section that
    contain global memory and resets the global memory back to
    the initial subsection contents.

    Note, that for this routine to be called the section is not
    referenced nor is it mapped in any process.

Arguments:

    ControlArea - Supplies a pointer to the control area for the section.

    Process - Supplies a pointer to the process IFF the working set mutex
              is held, else NULL is supplied.  Note that IFF the working set
              mutex is held, it must always be acquired unsafe.

Return Value:

    None.

Environment:

    PFN LOCK held.

--*/

{
    PMMPTE PointerPte;
    PMMPTE LastPte;
    PMMPFN Pfn1;
    MMPTE PteContents;
    MMPTE NewContents;
    MMPTE NewContentsDemandZero;
    KIRQL OldIrql = APC_LEVEL;
    ULONG i;
    ULONG SizeOfRawData;
    ULONG OffsetIntoSubsection;
    PSUBSECTION Subsection;
#if DBG
    ULONG DelayCount = 0;
#endif //DBG

    ASSERT (ControlArea->u.Flags.Image != 0);

    i = ControlArea->NumberOfSubsections;

    if (ControlArea->u.Flags.GlobalOnlyPerSession == 0) {
        Subsection = (PSUBSECTION)(ControlArea + 1);
    }
    else {
        Subsection = (PSUBSECTION)((PLARGE_CONTROL_AREA)ControlArea + 1);
    }

    //
    // Loop through all the subsections

    while (i > 0) {

        if (Subsection->u.SubsectionFlags.GlobalMemory == 1) {

            NewContents.u.Long = 0;
            NewContentsDemandZero.u.Long = 0;
            SizeOfRawData = 0;
            OffsetIntoSubsection = 0;

            //
            // Purge this section.
            //

            if (Subsection->StartingSector != 0) {

                //
                // This is not a demand zero section.
                //

                NewContents.u.Long = MiGetSubsectionAddressForPte(Subsection);
                NewContents.u.Soft.Prototype = 1;

                SizeOfRawData = (Subsection->NumberOfFullSectors << MMSECTOR_SHIFT) |
                               Subsection->u.SubsectionFlags.SectorEndOffset;
            }

            NewContents.u.Soft.Protection =
                                       Subsection->u.SubsectionFlags.Protection;
            NewContentsDemandZero.u.Soft.Protection =
                                        NewContents.u.Soft.Protection;

            PointerPte = Subsection->SubsectionBase;
            LastPte = &Subsection->SubsectionBase[Subsection->PtesInSubsection];
            ControlArea = Subsection->ControlArea;

            //
            // The WS lock may be released and reacquired and our callers
            // always acquire it unsafe.
            //

            MiMakeSystemAddressValidPfnWs (PointerPte, Process);

            while (PointerPte < LastPte) {

                if (MiIsPteOnPdeBoundary(PointerPte)) {

                    //
                    // We are on a page boundary, make sure this PTE is resident.
                    //

                    MiMakeSystemAddressValidPfnWs (PointerPte, Process);
                }

                PteContents = *PointerPte;
                if (PteContents.u.Long == 0) {

                    //
                    // No more valid PTEs to deal with.
                    //

                    break;
                }

                ASSERT (PteContents.u.Hard.Valid == 0);

                if ((PteContents.u.Soft.Prototype == 0) &&
                         (PteContents.u.Soft.Transition == 1)) {

                    //
                    // The prototype PTE is in transition format.
                    //

                    Pfn1 = MI_PFN_ELEMENT (PteContents.u.Trans.PageFrameNumber);

                    //
                    // If the prototype PTE is no longer pointing to
                    // the original image page (not in protopte format),
                    // or has been modified, remove it from memory.
                    //

                    if ((Pfn1->u3.e1.Modified == 1) ||
                        (Pfn1->OriginalPte.u.Soft.Prototype == 0)) {
                        ASSERT (Pfn1->OriginalPte.u.Hard.Valid == 0);

                        //
                        // This is a transition PTE which has been
                        // modified or is no longer in protopte format.
                        //

                        if (Pfn1->u3.e2.ReferenceCount != 0) {

                            //
                            // There must be an I/O in progress on this
                            // page.  Wait for the I/O operation to complete.
                            //

                            UNLOCK_PFN (OldIrql);

                            KeDelayExecutionThread (KernelMode, FALSE, &MmShortTime);

                            //
                            // Redo the loop.
                            //
#if DBG
                            if ((DelayCount % 1024) == 0) {
                                DbgPrint("MMFLUSHSEC: waiting for i/o to complete PFN %lx\n",
                                    Pfn1);
                            }
                            DelayCount += 1;
#endif //DBG

                            LOCK_PFN (OldIrql);

                            MiMakeSystemAddressValidPfnWs (PointerPte, Process);
                            continue;
                        }

                        ASSERT (!((Pfn1->OriginalPte.u.Soft.Prototype == 0) &&
                           (Pfn1->OriginalPte.u.Soft.Transition == 1)));

                        MI_WRITE_INVALID_PTE (PointerPte, Pfn1->OriginalPte);
                        ASSERT (Pfn1->OriginalPte.u.Hard.Valid == 0);

                        //
                        // Only reduce the number of PFN references if
                        // the original PTE is still in prototype PTE
                        // format.
                        //

                        if (Pfn1->OriginalPte.u.Soft.Prototype == 1) {
                            ControlArea->NumberOfPfnReferences -= 1;
                            ASSERT ((LONG)ControlArea->NumberOfPfnReferences >= 0);
                        }
                        MiUnlinkPageFromList (Pfn1);

                        MI_SET_PFN_DELETED (Pfn1);

                        MiDecrementShareCount (Pfn1->PteFrame);

                        //
                        // If the reference count for the page is zero, insert
                        // it into the free page list, otherwise leave it alone
                        // and when the reference count is decremented to zero
                        // the page will go to the free list.
                        //

                        if (Pfn1->u3.e2.ReferenceCount == 0) {
                            MiReleasePageFileSpace (Pfn1->OriginalPte);
                            MiInsertPageInList (MmPageLocationList[FreePageList],
                                                MI_GET_PAGE_FRAME_FROM_TRANSITION_PTE (&PteContents));
                        }

                        MI_WRITE_INVALID_PTE (PointerPte, NewContents);
                    }
                } else {

                    //
                    // Prototype PTE is not in transition format.
                    //

                    if (PteContents.u.Soft.Prototype == 0) {

                        //
                        // This refers to a page in the paging file,
                        // as it no longer references the image,
                        // restore the PTE contents to what they were
                        // at the initial image creation.
                        //

                        if (PteContents.u.Long != NoAccessPte.u.Long) {
                            MiReleasePageFileSpace (PteContents);
                            MI_WRITE_INVALID_PTE (PointerPte, NewContents);
                        }
                    }
                }
                PointerPte += 1;
                OffsetIntoSubsection += PAGE_SIZE;

                if (OffsetIntoSubsection >= SizeOfRawData) {

                    //
                    // There are trailing demand zero pages in this
                    // subsection, set the PTE contents to be demand
                    // zero for the remainder of the PTEs in this
                    // subsection.
                    //

                    NewContents = NewContentsDemandZero;
                }

#if DBG
                DelayCount = 0;
#endif //DBG

            } //end while
        }

        i -=1;
        Subsection += 1;
     }

    return;
}
