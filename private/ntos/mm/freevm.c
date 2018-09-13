/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

   freevm.c

Abstract:

    This module contains the routines which implement the
    NtFreeVirtualMemory service.

Author:

    Lou Perazzoli (loup) 22-May-1989
    Landy Wang (landyw) 02-June-1997

Revision History:

--*/

#include "mi.h"

#define MEM_CHECK_COMMIT_STATE 0x400000

#define MM_VALID_PTE_SIZE (256)


MMPTE MmDecommittedPte = {MM_DECOMMIT << MM_PROTECT_FIELD_SHIFT};

#if DBG
extern PEPROCESS MmWatchProcess;
VOID MmFooBar(VOID);
#endif // DBG
// #include "ntos.h"


#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,NtFreeVirtualMemory)
#pragma alloc_text(PAGE,MiIsEntireRangeCommitted)
#endif

VOID
MiProcessValidPteList (
    IN PMMPTE *PteList,
    IN ULONG Count
    );

ULONG
MiDecommitPages (
    IN PVOID StartingAddress,
    IN PMMPTE EndingPte,
    IN PEPROCESS Process,
    IN PMMVAD_SHORT Vad
    );

VOID
MiDeleteFreeVm (
    IN PVOID StartingAddress,
    IN PVOID EndingAddress
    );


NTSTATUS
NtFreeVirtualMemory(
    IN HANDLE ProcessHandle,
    IN OUT PVOID *BaseAddress,
    IN OUT PSIZE_T RegionSize,
    IN ULONG FreeType
     )

/*++

Routine Description:

    This function deletes a region of pages within the virtual address
    space of a subject process.

Arguments:

   ProcessHandle - An open handle to a process object.

   BaseAddress - The base address of the region of pages
                 to be freed. This value is rounded down to the
                 next host page address boundary.

   RegionSize - A pointer to a variable that will receive
                the actual size in bytes of the freed region of
                pages. The initial value of this argument is
                rounded up to the next host page size boundary.

   FreeType - A set of flags that describe the type of
              free that is to be performed for the specified
              region of pages.
        

       FreeType Flags


        MEM_DECOMMIT - The specified region of pages is to
             be decommitted.

        MEM_RELEASE - The specified region of pages is to
             be released.


Return Value:

    Returns the status

--*/

{
    PMMVAD_SHORT Vad;
    PMMVAD_SHORT NewVad;
    PMMVAD PreviousVad;
    PMMVAD NextVad;
    PEPROCESS Process;
    KPROCESSOR_MODE PreviousMode;
    PVOID StartingAddress;
    PVOID EndingAddress;
    NTSTATUS Status;
    ULONG Attached = FALSE;
    SIZE_T CapturedRegionSize;
    PVOID CapturedBase;
    PMMPTE StartingPte;
    PMMPTE EndingPte;
    SIZE_T OldQuota;
    SIZE_T QuotaCharge;
    SIZE_T CommitReduction;
    ULONG_PTR OldEnd;
    LOGICAL UserPhysicalPages;
#if defined(_MIALT4K_)
    PVOID OriginalStartingAddress;
    PVOID OriginalEndingAddress;
    BOOLEAN EmulationFor4kPage = FALSE;
#endif

    PAGED_CODE();

    //
    // Check to make sure FreeType is good.
    //

    if ((FreeType & ~(MEM_DECOMMIT | MEM_RELEASE)) != 0) {
        return STATUS_INVALID_PARAMETER_4;
    }

    //
    // One of MEM_DECOMMIT or MEM_RELEASE must be specified, but not both.
    //

    if (((FreeType & (MEM_DECOMMIT | MEM_RELEASE)) == 0) ||
        ((FreeType & (MEM_DECOMMIT | MEM_RELEASE)) ==
                            (MEM_DECOMMIT | MEM_RELEASE))) {
        return STATUS_INVALID_PARAMETER_4;
    }

    PreviousMode = KeGetPreviousMode();

    //
    // Establish an exception handler, probe the specified addresses
    // for write access and capture the initial values.
    //

    try {

        if (PreviousMode != KernelMode) {

            ProbeForWritePointer (BaseAddress);
            ProbeForWriteUlong_ptr (RegionSize);
        }

        //
        // Capture the base address.
        //

        CapturedBase = *BaseAddress;

        //
        // Capture the region size.
        //

        CapturedRegionSize = *RegionSize;

    } except (ExSystemExceptionFilter()) {

        //
        // If an exception occurs during the probe or capture
        // of the initial values, then handle the exception and
        // return the exception code as the status value.
        //

        return GetExceptionCode();
    }

#if DBG
    if (MmDebug & MM_DBG_SHOW_NT_CALLS) {
        if ( !MmWatchProcess ) {
            DbgPrint("freevm processhandle %lx base %lx size %lx type %lx\n",
                    ProcessHandle, CapturedBase, CapturedRegionSize, FreeType);
        }
    }
#endif

    //
    // Make sure the specified starting and ending addresses are
    // within the user part of the virtual address space.
    //

    if (CapturedBase > MM_HIGHEST_USER_ADDRESS) {

        //
        // Invalid base address.
        //

        return STATUS_INVALID_PARAMETER_2;
    }

    if ((ULONG_PTR)MM_HIGHEST_USER_ADDRESS - (ULONG_PTR)CapturedBase <
                                                        CapturedRegionSize) {

        //
        // Invalid region size;
        //

        return STATUS_INVALID_PARAMETER_3;

    }

    EndingAddress = (PVOID)(((LONG_PTR)CapturedBase + CapturedRegionSize - 1) |
                        (PAGE_SIZE - 1));

    StartingAddress = (PVOID)PAGE_ALIGN(CapturedBase);

    if ( ProcessHandle == NtCurrentProcess() ) {
        Process = PsGetCurrentProcess();
    } else {
        //
        // Reference the specified process handle for VM_OPERATION access.
        //

        Status = ObReferenceObjectByHandle ( ProcessHandle,
                                             PROCESS_VM_OPERATION,
                                             PsProcessType,
                                             PreviousMode,
                                             (PVOID *)&Process,
                                             NULL );

        if (!NT_SUCCESS(Status)) {
            return Status;
        }
    }

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
    // be inserted and walked.  Block APCs to prevent page faults while
    // we own the working set mutex.
    //

    LOCK_WS_AND_ADDRESS_SPACE (Process);

    //
    // Make sure the address space was not deleted.
    //

    if (Process->AddressSpaceDeleted != 0) {
        Status = STATUS_PROCESS_IS_TERMINATING;
        goto ErrorReturn;
    }

#if defined(_MIALT4K_)

    if (CapturedRegionSize != 0) {
        
        OriginalStartingAddress = (PVOID)PAGE_4K_ALIGN (CapturedBase);

        OriginalEndingAddress = (PVOID)(((LONG_PTR)CapturedBase + CapturedRegionSize - 1) |
                                        (PAGE_4K - 1));

        if (Process->Wow64Process != NULL) {

            EmulationFor4kPage = TRUE;

            //
            // adjust Starting/EndingAddress for the native page size 
            //

            StartingAddress = PAGE_NEXT_ALIGN(OriginalStartingAddress);

            EndingAddress = 
                (PVOID)((ULONG_PTR)PAGE_ALIGN ((ULONG_PTR)OriginalEndingAddress + PAGE_4K) - 1);

            if (StartingAddress > EndingAddress) {

                //
                // There is no need to free native pages
                //

                UNLOCK_WS_UNSAFE (Process);
        
                goto perform_free4kpages;

            }
        }
    }
        
#endif

    Vad = (PMMVAD_SHORT)MiLocateAddress (StartingAddress);

    if (Vad == NULL) {

        //
        // No Virtual Address Descriptor located for Base Address.
        //

        Status = STATUS_MEMORY_NOT_ALLOCATED;
        goto ErrorReturn;
    }

    //
    // Found the associated Virtual Address Descriptor.
    //

    if (Vad->EndingVpn < MI_VA_TO_VPN (EndingAddress)) {

        //
        // The entire range to delete is not contained within a single
        // virtual address descriptor.  Return an error.
        //

        Status = STATUS_UNABLE_TO_FREE_VM;
        goto ErrorReturn;
    }

    //
    // Check to ensure this Vad is deletable.  Delete is required
    // for both decommit and release.
    //

    if ((Vad->u.VadFlags.PrivateMemory == 0) ||
        (Vad->u.VadFlags.PhysicalMapping == 1)) {
        Status = STATUS_UNABLE_TO_DELETE_SECTION;
        goto ErrorReturn;
    }

    if (Vad->u.VadFlags.NoChange == 1) {

        //
        // An attempt is being made to delete a secured VAD, check
        // to see if this deletion is allowed.
        //

        if (FreeType & MEM_RELEASE) {

            //
            // Specify the whole range, this solves the problem with
            // splitting the VAD and trying to decide where the various
            // secure ranges need to go.
            //

            Status = MiCheckSecuredVad ((PMMVAD)Vad,
                                        MI_VPN_TO_VA (Vad->StartingVpn),
                        ((Vad->EndingVpn - Vad->StartingVpn) << PAGE_SHIFT) +
                                (PAGE_SIZE - 1),
                                        MM_SECURE_DELETE_CHECK);

        } else {
            Status = MiCheckSecuredVad ((PMMVAD)Vad,
                                        CapturedBase,
                                        CapturedRegionSize,
                                        MM_SECURE_DELETE_CHECK);
        }
        if (!NT_SUCCESS (Status)) {
            goto ErrorReturn;
        }
    }

    UserPhysicalPages = FALSE;

    PreviousVad = MiGetPreviousVad (Vad);
    NextVad = MiGetNextVad (Vad);
    if (FreeType & MEM_RELEASE) {

        //
        // *****************************************************************
        // MEM_RELEASE was specified.
        // *****************************************************************
        //

        //
        // The descriptor for the address range is deletable.  Remove or split
        // the descriptor.
        //

        //
        // If the region size is zero, remove the whole VAD.
        //

        if (CapturedRegionSize == 0) {

            //
            // If the region size is specified as 0, the base address
            // must be the starting address for the region.
            //

            if (MI_VA_TO_VPN (CapturedBase) != Vad->StartingVpn) {
                Status = STATUS_FREE_VM_NOT_AT_BASE;
                goto ErrorReturn;
            }

            //
            // This Virtual Address Descriptor has been deleted.
            //

            StartingAddress = MI_VPN_TO_VA (Vad->StartingVpn);
            EndingAddress = MI_VPN_TO_VA_ENDING (Vad->EndingVpn);

            //
            // Free all the physical pages that this VAD might be mapping.
            // Since only the AWE lock synchronizes the remap API, carefully
            // remove this VAD from the list first.
            //

            if (Vad->u.VadFlags.UserPhysicalPages == 1) {
                MiPhysicalViewRemover (Process, (PMMVAD)Vad);
                MiRemoveUserPhysicalPagesVad (Vad);
                UserPhysicalPages = TRUE;
            }
            else if (Vad->u.VadFlags.WriteWatch == 1) {
                MiPhysicalViewRemover (Process, (PMMVAD)Vad);
            }

            MiRemoveVad ((PMMVAD)Vad);
            ExFreePool (Vad);

#if defined(_MIALT4K_)

            OriginalStartingAddress = StartingAddress;
            OriginalEndingAddress  = EndingAddress;

            if (Process->Wow64Process != NULL) {

                EmulationFor4kPage = TRUE;

            } else {

                EmulationFor4kPage = FALSE;

            }

#endif

        } else {

            //
            // Region's size was not specified as zero, delete the
            // whole VAD or split the VAD.
            //

            if (MI_VA_TO_VPN (StartingAddress) == Vad->StartingVpn) {
                if (MI_VA_TO_VPN (EndingAddress) == Vad->EndingVpn) {

                    //
                    // This Virtual Address Descriptor has been deleted.
                    //

                    //
                    // Free all the physical pages that this VAD might be
                    // mapping.  Since only the AWE lock synchronizes the
                    // remap API, carefully remove this VAD from the list first.
                    //
        
                    if (Vad->u.VadFlags.UserPhysicalPages == 1) {
                        MiPhysicalViewRemover (Process, (PMMVAD)Vad);
                        MiRemoveUserPhysicalPagesVad (Vad);
                        UserPhysicalPages = TRUE;
                    }
                    else if (Vad->u.VadFlags.WriteWatch == 1) {
                        MiPhysicalViewRemover (Process, (PMMVAD)Vad);
                    }

                    MiRemoveVad ((PMMVAD)Vad);
                    ExFreePool (Vad);

                } else {

                    if ((Vad->u.VadFlags.UserPhysicalPages == 1) ||
                        (Vad->u.VadFlags.WriteWatch == 1)) {

                        //
                        // Splitting or chopping a physical VAD or a write-watch
                        // VAD is not allowed.
                        //

                        Status = STATUS_FREE_VM_NOT_AT_BASE;
                        goto ErrorReturn;
                    }

                    //
                    // This Virtual Address Descriptor has a new starting
                    // address.
                    //

                    CommitReduction = MiCalculatePageCommitment (
                                                            StartingAddress,
                                                            EndingAddress,
                                                            (PMMVAD)Vad,
                                                            Process );

                    Vad->StartingVpn = MI_VA_TO_VPN ((PCHAR)EndingAddress + 1);
                    Vad->u.VadFlags.CommitCharge -= CommitReduction;
                    ASSERT ((SSIZE_T)Vad->u.VadFlags.CommitCharge >= 0);
                    MiReturnPageFileQuota (CommitReduction, Process);
                    MiReturnCommitment (CommitReduction);
                    if (Process->JobStatus & PS_JOB_STATUS_REPORT_COMMIT_CHANGES) {
                        PsChangeJobMemoryUsage (-(SSIZE_T)CommitReduction);
                    }
                    MM_TRACK_COMMIT (MM_DBG_COMMIT_RETURN_NTFREEVM1,
                                     CommitReduction);
                    Process->CommitCharge -= CommitReduction;
                    NextVad = (PMMVAD)Vad;
                }

            } else {

                if ((Vad->u.VadFlags.UserPhysicalPages == 1) ||
                    (Vad->u.VadFlags.WriteWatch == 1)) {

                    //
                    // Splitting or chopping a physical VAD or a write-watch
                    // VAD is not allowed.
                    //

                    Status = STATUS_FREE_VM_NOT_AT_BASE;
                    goto ErrorReturn;
                }

                //
                // Starting address is greater than start of VAD.
                //

                if (MI_VA_TO_VPN (EndingAddress) == Vad->EndingVpn) {

                    //
                    // Change the ending address of the VAD.
                    //

                    CommitReduction = MiCalculatePageCommitment (
                                                            StartingAddress,
                                                            EndingAddress,
                                                            (PMMVAD)Vad,
                                                            Process );

                    Vad->u.VadFlags.CommitCharge -= CommitReduction;
                    MiReturnPageFileQuota (CommitReduction, Process);
                    MiReturnCommitment (CommitReduction);
                    if (Process->JobStatus & PS_JOB_STATUS_REPORT_COMMIT_CHANGES) {
                        PsChangeJobMemoryUsage (-(SSIZE_T)CommitReduction);
                    }
                    MM_TRACK_COMMIT (MM_DBG_COMMIT_RETURN_NTFREEVM2,
                                     CommitReduction);
                    Process->CommitCharge -= CommitReduction;

                    Vad->EndingVpn = MI_VA_TO_VPN ((PCHAR)StartingAddress - 1);
                    PreviousVad = (PMMVAD)Vad;

                } else {

                    //
                    // Split this VAD as the address range is within the VAD.
                    //

                    //
                    // Allocate an new VAD under an exception handler
                    // as there may not be enough quota.
                    //

                    NewVad = ExAllocatePoolWithTag (NonPagedPool,
                                                    sizeof(MMVAD_SHORT),
                                                    'SdaV');
                    if ( NewVad == NULL ) {
                        Status = STATUS_INSUFFICIENT_RESOURCES;
                        goto ErrorReturn;
                    }

                    CommitReduction = MiCalculatePageCommitment (
                                                            StartingAddress,
                                                            EndingAddress,
                                                            (PMMVAD)Vad,
                                                            Process );

                    OldQuota = Vad->u.VadFlags.CommitCharge - CommitReduction;
                    OldEnd = Vad->EndingVpn;

                    *NewVad = *Vad;

                    Vad->EndingVpn = MI_VA_TO_VPN ((PCHAR)StartingAddress - 1);
                    NewVad->StartingVpn = MI_VA_TO_VPN ((PCHAR)EndingAddress + 1);

                    //
                    // Set the commit charge to zero so MiInsertVad will
                    // not charge commitment for splitting the VAD.
                    //

                    NewVad->u.VadFlags.CommitCharge = 0;

                    try {

                        //
                        // Insert the VAD, this could get an exception
                        // on charging quota.
                        //

                        MiInsertVad ((PMMVAD)NewVad);

                    } except (EXCEPTION_EXECUTE_HANDLER) {

                        //
                        // Inserting the Vad failed, reset the original
                        // VAD, free new vad and return an error.
                        //

                        Vad->EndingVpn = OldEnd;

                        ExFreePool (NewVad);
                        Status = GetExceptionCode();
                        goto ErrorReturn;
                    }

                    Vad->u.VadFlags.CommitCharge -= CommitReduction;
                    MiReturnPageFileQuota (CommitReduction, Process);
                    MiReturnCommitment (CommitReduction);
                    if (Process->JobStatus & PS_JOB_STATUS_REPORT_COMMIT_CHANGES) {
                        PsChangeJobMemoryUsage (-(SSIZE_T)CommitReduction);
                    }
                    MM_TRACK_COMMIT (MM_DBG_COMMIT_RETURN_NTFREEVM3,
                                     CommitReduction);
                    Process->CommitCharge -= CommitReduction;

                    //
                    // As we have split the original VAD into 2 separate VADs
                    // there is know way of knowing what the commit charge
                    // is for each VAD.  Calculate the charge and reset
                    // each VAD.  Note that we also use the previous value
                    // to make sure the books stay balanced.
                    //

                    QuotaCharge = MiCalculatePageCommitment (MI_VPN_TO_VA (Vad->StartingVpn),
                                                             (PCHAR)StartingAddress - 1,
                                                             (PMMVAD)Vad,
                                                             Process );

                    Vad->u.VadFlags.CommitCharge = QuotaCharge;

                    //
                    // Give the remaining charge to the new VAD.
                    //

                    NewVad->u.VadFlags.CommitCharge = OldQuota - QuotaCharge;
                    PreviousVad = (PMMVAD)Vad;
                    NextVad = (PMMVAD)NewVad;
                }
            }
        }

        //
        // Return commitment for page table pages if possible.
        //

        MiReturnPageTablePageCommitment (StartingAddress,
                                         EndingAddress,
                                         Process,
                                         PreviousVad,
                                         NextVad);

        if (UserPhysicalPages == TRUE) {
            MiDeletePageTablesForPhysicalRange (StartingAddress, EndingAddress);
        }
        else {

            //
            // Get the PFN mutex so the MiDeleteVirtualAddresses can be called.
            //

            MiDeleteFreeVm (StartingAddress, EndingAddress);
        }

        UNLOCK_WS_UNSAFE (Process);

        CapturedRegionSize = 1 + (PCHAR)EndingAddress - (PCHAR)StartingAddress;

        //
        // Update the virtual size in the process header.
        //

        Process->VirtualSize -= CapturedRegionSize;

#if defined(_MIALT4K_)
        if (EmulationFor4kPage == TRUE) {

            goto perform_free4kpages;

        }
#endif

        UNLOCK_ADDRESS_SPACE (Process);

        if (Attached) {
            KeDetachProcess();
        }

        if ( ProcessHandle != NtCurrentProcess() ) {
            ObDereferenceObject (Process);
        }
        //
        // Establish an exception handler and write the size and base
        // address.
        //

        try {

            *RegionSize = CapturedRegionSize;
            *BaseAddress = StartingAddress;

        } except (EXCEPTION_EXECUTE_HANDLER) {

            //
            // An exception occurred, don't take any action (just handle
            // the exception and return success.

        }

#if DBG
    if (MmDebug & MM_DBG_SHOW_NT_CALLS) {
        if ( MmWatchProcess ) {
            if ( MmWatchProcess == PsGetCurrentProcess() ) {
                DbgPrint("\n--- FREE Type 0x%lx Base %lx Size %lx\n",
                        FreeType, StartingAddress, CapturedRegionSize);
                MmFooBar();
            }
        }
    }
#endif

        return STATUS_SUCCESS;
    }

    if (Vad->u.VadFlags.UserPhysicalPages == 1) {

        //
        // Pages from a physical VAD must be released via
        // NtFreeUserPhysicalPages, not this routine.
        //

        Status = STATUS_MEMORY_NOT_ALLOCATED;
        goto ErrorReturn;
    }

    //
    // **************************************************************
    //
    // MEM_DECOMMIT was specified.
    //
    // **************************************************************
    //

    //
    // Check to ensure the complete range of pages is already committed.
    //

    if (CapturedRegionSize == 0) {

        if (MI_VA_TO_VPN (CapturedBase) != Vad->StartingVpn) {
            Status = STATUS_FREE_VM_NOT_AT_BASE;
            goto ErrorReturn;
        }
        EndingAddress = MI_VPN_TO_VA_ENDING (Vad->EndingVpn);
    }

#if defined(_MIALT4K_)

        OriginalStartingAddress = StartingAddress;
        OriginalEndingAddress  = EndingAddress;

        if (Process->Wow64Process != NULL) {

            EmulationFor4kPage = TRUE;

        } else {

            EmulationFor4kPage = FALSE;

        }

#endif

#if 0
    if (FreeType & MEM_CHECK_COMMIT_STATE) {
        if ( !MiIsEntireRangeCommitted(StartingAddress,
                                       EndingAddress,
                                       Vad,
                                       Process)) {

            //
            // The entire range to be decommitted is not committed,
            // return an error.
            //

            Status = STATUS_UNABLE_TO_DECOMMIT_VM;
            goto ErrorReturn;
        }
    }
#endif //0

    //
    // The address range is entirely committed, decommit it now.
    //

    //
    // Calculate the initial quotas and commit charges for this VAD.
    //

    StartingPte = MiGetPteAddress (StartingAddress);
    EndingPte = MiGetPteAddress (EndingAddress);

    CommitReduction = 1 + EndingPte - StartingPte;

    //
    // Check to see if the entire range can be decommitted by
    // just updating the virtual address descriptor.
    //

    CommitReduction -= MiDecommitPages (StartingAddress,
                                        EndingPte,
                                        Process,
                                        Vad);

    //
    // Adjust the quota charges.
    //

    ASSERT ((LONG)CommitReduction >= 0);
    MiReturnPageFileQuota (CommitReduction, Process);
    MiReturnCommitment (CommitReduction);
    MM_TRACK_COMMIT (MM_DBG_COMMIT_RETURN_NTFREEVM4, CommitReduction);
    Vad->u.VadFlags.CommitCharge -= CommitReduction;
    if (Process->JobStatus & PS_JOB_STATUS_REPORT_COMMIT_CHANGES) {
        PsChangeJobMemoryUsage (-(SSIZE_T)CommitReduction);
    }
    Process->CommitCharge -= CommitReduction;
    ASSERT ((LONG)Vad->u.VadFlags.CommitCharge >= 0);


#if !(defined(_MIALT4K_))
    UNLOCK_WS_AND_ADDRESS_SPACE (Process);
#else
    UNLOCK_WS_UNSAFE(Process);

  perform_free4kpages:

    if (EmulationFor4kPage == TRUE) {

        if (FreeType & MEM_RELEASE) {

            MiReleaseFor4kPage(OriginalStartingAddress, 
                               OriginalEndingAddress, 
                               Process);

        } else {

            MiDecommitFor4kPage(OriginalStartingAddress, 
                                OriginalEndingAddress, 
                                Process);

        }

        StartingAddress = OriginalStartingAddress;
        EndingAddress = OriginalEndingAddress;
    }

    UNLOCK_ADDRESS_SPACE (Process);

#endif

    if (Attached) {
        KeDetachProcess();
    }
    if ( ProcessHandle != NtCurrentProcess() ) {
        ObDereferenceObject (Process);
    }

    //
    // Establish an exception handler and write the size and base
    // address.
    //

    try {

        *RegionSize = 1 + (PCHAR)EndingAddress - (PCHAR)StartingAddress;
        *BaseAddress = StartingAddress;

    } except (EXCEPTION_EXECUTE_HANDLER) {
        NOTHING;
    }

    return STATUS_SUCCESS;

ErrorReturn:
       UNLOCK_WS_AND_ADDRESS_SPACE (Process);

       if (Attached) {
           KeDetachProcess();
       }

       if ( ProcessHandle != NtCurrentProcess() ) {
           ObDereferenceObject (Process);
       }
       return Status;
}

ULONG
MiIsEntireRangeCommitted (
    IN PVOID StartingAddress,
    IN PVOID EndingAddress,
    IN PMMVAD Vad,
    IN PEPROCESS Process
    )

/*++

Routine Description:

    This routine examines the range of pages from the starting address
    up to and including the ending address and returns TRUE if every
    page in the range is committed, FALSE otherwise.

Arguments:

    StartingAddress - Supplies the starting address of the range.

    EndingAddress - Supplies the ending address of the range.

    Vad - Supplies the virtual address descriptor which describes the range.

    Process - Supplies the current process.

Return Value:

    TRUE if the entire range is committed.
    FALSE if any page within the range is not committed.

Environment:

    Kernel mode, APCs disable, WorkingSetMutex and AddressCreation mutexes
    held.

--*/

{
    PMMPTE PointerPte;
    PMMPTE LastPte;
    PMMPTE PointerPde;
    PMMPTE PointerPpe;
    ULONG FirstTime;
    ULONG Waited;
    PVOID Va;

    PAGED_CODE();

    FirstTime = TRUE;

    PointerPde = MiGetPdeAddress (StartingAddress);
    PointerPte = MiGetPteAddress (StartingAddress);
    LastPte = MiGetPteAddress (EndingAddress);

    //
    // Set the Va to the starting address + 8, this solves problems
    // associated with address 0 (NULL) being used as a valid virtual
    // address and NULL in the VAD commitment field indicating no pages
    // are committed.
    //

    Va = (PVOID)((PCHAR)StartingAddress + 8);

    while (PointerPte <= LastPte) {

        if (MiIsPteOnPdeBoundary(PointerPte) || (FirstTime)) {

            //
            // This may be a PPE/PDE boundary, check to see if both
            // PPE/PDE pages exist.
            //

            FirstTime = FALSE;
            PointerPde = MiGetPteAddress (PointerPte);
            PointerPpe = MiGetPteAddress (PointerPde);

            do {

                while (!MiDoesPpeExistAndMakeValid(PointerPpe, Process, FALSE, &Waited)) {

                    //
                    // No PDE exists for the starting address, check the VAD
                    // to see if the pages are committed.
                    //

                    PointerPpe += 1;

                    PointerPde = MiGetVirtualAddressMappedByPte (PointerPpe);
                    PointerPte = MiGetVirtualAddressMappedByPte (PointerPde);
                    Va = MiGetVirtualAddressMappedByPte (PointerPte);

                    if (PointerPte > LastPte) {

                        //
                        // Make sure the entire range is committed.
                        //

                        if (Vad->u.VadFlags.MemCommit == 0) {

                            //
                            // The entire range to be decommitted is not committed,
                            // return an error.
                            //

                            return FALSE;
                        } else {
                            return TRUE;
                        }
                    }

                    //
                    // Make sure the range thus far is committed.
                    //

                    if (Vad->u.VadFlags.MemCommit == 0) {

                        //
                        // The entire range to be decommitted is not committed,
                        // return an error.
                        //

                        return FALSE;
                    }
                }

                Waited = 0;

                while (!MiDoesPdeExistAndMakeValid(PointerPde, Process, FALSE, &Waited)) {

                    //
                    // No PDE exists for the starting address, check the VAD
                    // to see if the pages are committed.
                    //

                    PointerPde += 1;

                    PointerPpe = MiGetPteAddress (PointerPde);
                    PointerPte = MiGetVirtualAddressMappedByPte (PointerPde);
                    Va = MiGetVirtualAddressMappedByPte (PointerPte);

                    if (PointerPte > LastPte) {

                        //
                        // Make sure the entire range is committed.
                        //

                        if (Vad->u.VadFlags.MemCommit == 0) {

                            //
                            // The entire range to be decommitted is not committed,
                            // return an error.
                            //

                            return FALSE;
                        } else {
                            return TRUE;
                        }
                    }

                    //
                    // Make sure the range thus far is committed.
                    //

                    if (Vad->u.VadFlags.MemCommit == 0) {

                        //
                        // The entire range to be decommitted is not committed,
                        // return an error.
                        //

                        return FALSE;
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
        }

        //
        // The page table page exists, check each PTE for commitment.
        //

        if (PointerPte->u.Long == 0) {

            //
            // This page has not been committed, check the VAD.
            //

            if (Vad->u.VadFlags.MemCommit == 0) {

                //
                // The entire range to be decommitted is not committed,
                // return an error.
                //

                return FALSE;
            }
        } else {

            //
            // Has this page been explicitly decommitted?
            //

            if (MiIsPteDecommittedPage (PointerPte)) {

                //
                // This page has been explicitly decommitted, return an error.
                //

                return FALSE;
            }
        }
        PointerPte += 1;
        Va = (PVOID)((PCHAR)(Va) + PAGE_SIZE);
    }
    return TRUE;
}

ULONG
MiDecommitPages (
    IN PVOID StartingAddress,
    IN PMMPTE EndingPte,
    IN PEPROCESS Process,
    IN PMMVAD_SHORT Vad
    )

/*++

Routine Description:

    This routine decommits the specified range of pages.

Arguments:

    StartingAddress - Supplies the starting address of the range.

    EndingPte - Supplies the ending PTE of the range.

    Process - Supplies the current process.

    Vad - Supplies the virtual address descriptor which describes the range.

Return Value:

    Value to reduce commitment by for the VAD.

Environment:

    Kernel mode, APCs disabled, WorkingSetMutex and AddressCreation mutexes
    held.

--*/

{
    PMMPTE PointerPpe;
    PMMPTE PointerPde;
    PMMPTE PointerPte;
    PVOID Va;
    ULONG CommitReduction;
    PMMPTE CommitLimitPte;
    KIRQL OldIrql;
    PMMPTE ValidPteList[MM_VALID_PTE_SIZE];
    ULONG count;
    ULONG WorkingSetIndex;
    PMMPFN Pfn1;
    PMMPFN Pfn2;
    PVOID SwapVa;
    ULONG Entry;
    MMWSLENTRY Locked;
    MMPTE PteContents;
    PVOID UsedPageTableHandle;
    PVOID UsedPageDirectoryHandle;

    count = 0;
    CommitReduction = 0;

    if (Vad->u.VadFlags.MemCommit) {
        CommitLimitPte = MiGetPteAddress (MI_VPN_TO_VA (Vad->EndingVpn));
    } else {
        CommitLimitPte = NULL;
    }

    //
    // Decommit each page by setting the PTE to be explicitly
    // decommitted.  The PTEs cannot be deleted all at once as
    // this would set the PTEs to zero which would auto-evaluate
    // as committed if referenced by another thread when a page
    // table page is being in-paged.
    //

    PointerPpe = MiGetPpeAddress (StartingAddress);
    PointerPde = MiGetPdeAddress (StartingAddress);
    PointerPte = MiGetPteAddress (StartingAddress);
    Va = StartingAddress;

    //
    // Loop through all the PDEs which map this region and ensure that
    // they exist.  If they don't exist create them by touching a
    // PTE mapped by the PDE.
    //

#if defined (_WIN64)
    MiMakePpeExistAndMakeValid (PointerPpe, Process, FALSE);
    if (PointerPde->u.Long == 0) {
        UsedPageDirectoryHandle = MI_GET_USED_PTES_HANDLE (PointerPte);
        MI_INCREMENT_USED_PTES_BY_HANDLE (UsedPageDirectoryHandle);
    }
#endif

    MiMakePdeExistAndMakeValid(PointerPde, Process, FALSE);

    while (PointerPte <= EndingPte) {

        if (MiIsPteOnPdeBoundary (PointerPte)) {

            PointerPde = MiGetPdeAddress (Va);
            PointerPpe = MiGetPpeAddress (Va);
            if (count != 0) {
                MiProcessValidPteList (&ValidPteList[0], count);
                count = 0;
            }

#if defined (_WIN64)
            MiMakePpeExistAndMakeValid (PointerPpe, Process, FALSE);
            if (PointerPde->u.Long == 0) {
                UsedPageDirectoryHandle = MI_GET_USED_PTES_HANDLE (PointerPte);
                MI_INCREMENT_USED_PTES_BY_HANDLE (UsedPageDirectoryHandle);
            }
#endif

            MiMakePdeExistAndMakeValid(PointerPde, Process, FALSE);
        }

        //
        // The working set lock is held.  No PTEs can go from
        // invalid to valid or valid to invalid.  Transition
        // PTEs can go from transition to pagefile.
        //

        PteContents = *PointerPte;

        if (PteContents.u.Long != 0) {

            if (PointerPte->u.Long == MmDecommittedPte.u.Long) {

                //
                // This PTE is already decommitted.
                //

                CommitReduction += 1;

            } else {

                Process->NumberOfPrivatePages -= 1;

                if (PteContents.u.Hard.Valid == 1) {

                    //
                    // Make sure this is not a forked PTE.
                    //

                    Pfn1 = MI_PFN_ELEMENT (PteContents.u.Hard.PageFrameNumber);

                    if (Pfn1->u3.e1.PrototypePte) {

                        LOCK_PFN (OldIrql);
                        MiDeletePte (PointerPte,
                                     Va,
                                     FALSE,
                                     Process,
                                     NULL,
                                     NULL);
                        UNLOCK_PFN (OldIrql);
                        Process->NumberOfPrivatePages += 1;
                        MI_WRITE_INVALID_PTE (PointerPte, MmDecommittedPte);
                    } else {

                        //
                        // Pte is valid, process later when PFN lock is held.
                        //

                        if (count == MM_VALID_PTE_SIZE) {
                            MiProcessValidPteList (&ValidPteList[0], count);
                            count = 0;
                        }
                        ValidPteList[count] = PointerPte;
                        count += 1;

                        //
                        // Remove address from working set list.
                        //


                        WorkingSetIndex = Pfn1->u1.WsIndex;

                        ASSERT (PAGE_ALIGN(MmWsle[WorkingSetIndex].u1.Long) ==
                                                                           Va);
                        //
                        // Check to see if this entry is locked in the working set
                        // or locked in memory.
                        //

                        Locked = MmWsle[WorkingSetIndex].u1.e1;

                        MiRemoveWsle (WorkingSetIndex, MmWorkingSetList);

                        //
                        // Add this entry to the list of free working set entries
                        // and adjust the working set count.
                        //

                        MiReleaseWsle (WorkingSetIndex, &Process->Vm);

                        if ((Locked.LockedInWs == 1) || (Locked.LockedInMemory == 1)) {

                            //
                            // This entry is locked.
                            //

                            MmWorkingSetList->FirstDynamic -= 1;

                            if (WorkingSetIndex != MmWorkingSetList->FirstDynamic) {

                                SwapVa = MmWsle[MmWorkingSetList->FirstDynamic].u1.VirtualAddress;
                                SwapVa = PAGE_ALIGN (SwapVa);
                                Pfn2 = MI_PFN_ELEMENT (
                                          MiGetPteAddress (SwapVa)->u.Hard.PageFrameNumber);

                                Entry = MiLocateWsle (SwapVa,
                                                      MmWorkingSetList,
                                                      Pfn2->u1.WsIndex);

                                MiSwapWslEntries (Entry,
                                                  WorkingSetIndex,
                                                  &Process->Vm);
                            }
                        }
                        MI_SET_PTE_IN_WORKING_SET (PointerPte, 0);
                    }
                } else if (PteContents.u.Soft.Prototype) {

                    //
                    // This is a forked PTE, just delete it.
                    //

                    LOCK_PFN (OldIrql);
                    MiDeletePte (PointerPte,
                                 Va,
                                 FALSE,
                                 Process,
                                 NULL,
                                 NULL);
                    UNLOCK_PFN (OldIrql);
                    Process->NumberOfPrivatePages += 1;
                    MI_WRITE_INVALID_PTE (PointerPte, MmDecommittedPte);

                } else if (PteContents.u.Soft.Transition == 1) {

                    //
                    // Transition PTE, get the PFN database lock
                    // and reprocess this one.
                    //

                    LOCK_PFN (OldIrql);
                    PteContents = *PointerPte;

                    if (PteContents.u.Soft.Transition == 1) {

                        //
                        // PTE is still in transition, delete it.
                        //

                        Pfn1 = MI_PFN_ELEMENT (PteContents.u.Trans.PageFrameNumber);

                        MI_SET_PFN_DELETED (Pfn1);

                        MiDecrementShareCount (Pfn1->PteFrame);

                        //
                        // Check the reference count for the page, if the
                        // reference count is zero, move the page to the
                        // free list, if the reference count is not zero,
                        // ignore this page.  When the reference count
                        // goes to zero, it will be placed on the free list.
                        //

                        if (Pfn1->u3.e2.ReferenceCount == 0) {
                            MiUnlinkPageFromList (Pfn1);
                            MiReleasePageFileSpace (Pfn1->OriginalPte);
                            MiInsertPageInList (MmPageLocationList[FreePageList],
                                                MI_GET_PAGE_FRAME_FROM_TRANSITION_PTE(&PteContents));
                        }

                    } else {

                        //
                        // Page MUST be in page file format!
                        //

                        ASSERT (PteContents.u.Soft.Valid == 0);
                        ASSERT (PteContents.u.Soft.Prototype == 0);
                        ASSERT (PteContents.u.Soft.PageFileHigh != 0);
                        MiReleasePageFileSpace (PteContents);
                    }
                    MI_WRITE_INVALID_PTE (PointerPte, MmDecommittedPte);
                    UNLOCK_PFN (OldIrql);
                } else {

                    //
                    // Must be demand zero or paging file format.
                    //

                    if (PteContents.u.Soft.PageFileHigh != 0) {
                        LOCK_PFN (OldIrql);
                        MiReleasePageFileSpace (PteContents);
                        UNLOCK_PFN (OldIrql);
                    } else {

                        //
                        // Don't subtract out the private page count for
                        // a demand zero page.
                        //

                        Process->NumberOfPrivatePages += 1;
                    }

                    MI_WRITE_INVALID_PTE (PointerPte, MmDecommittedPte);
                }
            }

        } else {

            //
            // The PTE is already zero.
            //

            //
            // Increment the count of non-zero page table entries for this
            // page table and the number of private pages for the process.
            //

            UsedPageTableHandle = MI_GET_USED_PTES_HANDLE (Va);

            MI_INCREMENT_USED_PTES_BY_HANDLE (UsedPageTableHandle);

            if (PointerPte > CommitLimitPte) {

                //
                // Pte is not committed.
                //

                CommitReduction += 1;
            }
            MI_WRITE_INVALID_PTE (PointerPte, MmDecommittedPte);
        }

        PointerPte += 1;
        Va = (PVOID)((PCHAR)Va + PAGE_SIZE);
    }
    if (count != 0) {
        MiProcessValidPteList (&ValidPteList[0], count);
    }

    return CommitReduction;
}


VOID
MiProcessValidPteList (
    IN PMMPTE *ValidPteList,
    IN ULONG Count
    )

/*++

Routine Description:

    This routine flushes the specified range of valid PTEs.

Arguments:

    ValidPteList - Supplies a pointer to an array of PTEs to flush.

    Count - Supplies the count of the number of elements in the array.

Return Value:

    none.

Environment:

    Kernel mode, APCs disabled, WorkingSetMutex and AddressCreation mutexes
    held.

--*/

{
    ULONG i = 0;
    MMPTE_FLUSH_LIST PteFlushList;
    MMPTE PteContents;
    PMMPFN Pfn1;
    KIRQL OldIrql;

    PteFlushList.Count = Count;

    LOCK_PFN (OldIrql);

    do {
        PteContents = *ValidPteList[i];
        ASSERT (PteContents.u.Hard.Valid == 1);
        Pfn1 = MI_PFN_ELEMENT (PteContents.u.Hard.PageFrameNumber);

        //
        // Decrement the share and valid counts of the page table
        // page which maps this PTE.
        //

        MiDecrementShareAndValidCount (Pfn1->PteFrame);

        MI_SET_PFN_DELETED (Pfn1);

        //
        // Decrement the share count for the physical page.  As the page
        // is private it will be put on the free list.
        //

        MiDecrementShareCountOnly (MI_GET_PAGE_FRAME_FROM_PTE (&PteContents));

        if (Count < MM_MAXIMUM_FLUSH_COUNT) {
            PteFlushList.FlushPte[i] = ValidPteList[i];
            PteFlushList.FlushVa[i] =
                        MiGetVirtualAddressMappedByPte (ValidPteList[i]);
        }
        *ValidPteList[i] = MmDecommittedPte;
        i += 1;
    } while (i != Count);

    MiFlushPteList (&PteFlushList, FALSE, MmDecommittedPte);
    UNLOCK_PFN (OldIrql);
    return;
}


VOID
MiDeleteFreeVm (
    IN PVOID StartingAddress,
    IN PVOID EndingAddress
    )

/*++

Routine Description:

    Nonpagable routine to call acquire PFN lock and call
    MiDeleteVirtualAddresses.

Arguments:


Return Value:

    none.

Environment:

    Kernel mode, APCs disabled, WorkingSetMutex and AddressCreation mutexes
    held.

--*/

{
    KIRQL OldIrql;

    LOCK_PFN (OldIrql);

    //
    // Delete the address range.
    //

    MiDeleteVirtualAddresses (StartingAddress,
                              EndingAddress,
                              FALSE,
                              (PMMVAD)NULL);

    UNLOCK_PFN (OldIrql);

}

