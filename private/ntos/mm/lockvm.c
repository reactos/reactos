/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

   lockvm.c

Abstract:

    This module contains the routines which implement the
    NtLockVirtualMemory service.

Author:

    Lou Perazzoli (loup) 20-August-1989

Revision History:

    Landy Wang (landyw) 08-April-1998 : Modifications for 3-level 64-bit NT.

--*/

#include "mi.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,NtLockVirtualMemory)
#pragma alloc_text(PAGE,NtUnlockVirtualMemory)
#endif


NTSTATUS
NtLockVirtualMemory (
    IN HANDLE ProcessHandle,
    IN OUT PVOID *BaseAddress,
    IN OUT PSIZE_T RegionSize,
    IN ULONG MapType
     )

/*++

Routine Description:

    This function locks a region of pages within the working set list
    of a subject process.

    The caller of this function must have PROCESS_VM_OPERATION access
    to the target process.  The caller must also have SeLockMemoryPrivilege.

Arguments:

   ProcessHandle - Supplies an open handle to a process object.

   BaseAddress - The base address of the region of pages
        to be locked. This value is rounded down to the
        next host page address boundary.

   RegionSize - A pointer to a variable that will receive
        the actual size in bytes of the locked region of
        pages. The initial value of this argument is
        rounded up to the next host page size boundary.

   MapType - A set of flags that describe the type of locking to
            perform.  One of MAP_PROCESS or MAP_SYSTEM.

Return Value:

    Returns the status

    STATUS_PRIVILEGE_NOT_HELD - The caller did not have sufficient
        privilege to perform the requested operation.

    TBS


--*/

{
    PVOID Va;
    PVOID EndingAddress;
    PMMPTE PointerPte;
    PMMPTE PointerPte1;
    PMMPFN Pfn1;
    PMMPTE PointerPde;
    PMMPTE PointerPpe;
    ULONG_PTR CapturedRegionSize;
    PVOID CapturedBase;
    PEPROCESS TargetProcess;
    NTSTATUS Status;
    BOOLEAN WasLocked;
    KPROCESSOR_MODE PreviousMode;
    ULONG Entry;
    ULONG SwapEntry;
    SIZE_T NumberOfAlreadyLocked;
    SIZE_T NumberToLock;
    ULONG WorkingSetIndex;
    PMMVAD Vad;
    PVOID LastVa;
    MMLOCK_CONFLICT Conflict;
    ULONG Waited;
    LOGICAL Attached;
#if defined(_MIALT4K_)
    BOOLEAN IsWow64Process = FALSE;
#endif

    PAGED_CODE();

    WasLocked = FALSE;
    LastVa = NULL;

    //
    // Validate the flags in MapType.
    //

    if ((MapType & ~(MAP_PROCESS | MAP_SYSTEM)) != 0) {
        return STATUS_INVALID_PARAMETER;
    }

    if ((MapType & (MAP_PROCESS | MAP_SYSTEM)) == 0) {
        return STATUS_INVALID_PARAMETER;
    }

    PreviousMode = KeGetPreviousMode();

    try {

        if (PreviousMode != KernelMode) {

            ProbeForWritePointer ((PULONG)BaseAddress);
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

    //
    // Make sure the specified starting and ending addresses are
    // within the user part of the virtual address space.
    //

    if (CapturedBase > MM_HIGHEST_USER_ADDRESS) {

        //
        // Invalid base address.
        //

        return STATUS_INVALID_PARAMETER;
    }

    if ((ULONG_PTR)MM_HIGHEST_USER_ADDRESS - (ULONG_PTR)CapturedBase <
                                                        CapturedRegionSize) {

        //
        // Invalid region size;
        //

        return STATUS_INVALID_PARAMETER;

    }

    if (CapturedRegionSize == 0) {
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Reference the specified process.
    //

    Status = ObReferenceObjectByHandle ( ProcessHandle,
                                         PROCESS_VM_OPERATION,
                                         PsProcessType,
                                         PreviousMode,
                                         (PVOID *)&TargetProcess,
                                         NULL );

    if (!NT_SUCCESS(Status)) {
        return Status;
    }

    if ((MapType & MAP_SYSTEM) != 0) {

        //
        // In addition to PROCESS_VM_OPERATION access to the target
        // process, the caller must have SE_LOCK_MEMORY_PRIVILEGE.
        //

        if (!SeSinglePrivilegeCheck(
                           SeLockMemoryPrivilege,
                           PreviousMode
                           )) {

            ObDereferenceObject( TargetProcess );
            return( STATUS_PRIVILEGE_NOT_HELD );
        }
    }

    //
    // Attach to the specified process.
    //

    if (ProcessHandle != NtCurrentProcess()) {
        KeAttachProcess (&TargetProcess->Pcb);
        Attached = TRUE;
    }
    else {
        Attached = FALSE;
    }

    //
    // Get address creation mutex, this prevents the
    // address range from being modified while it is examined.  Raise
    // to APC level to prevent an APC routine from acquiring the
    // address creation mutex.  Get the working set mutex so the
    // number of already locked pages in the request can be determined.
    //

#if defined(_MIALT4K_)

    //
    // changing to 4k aligned should not change the correctness.
    //

    EndingAddress = PAGE_4K_ALIGN((PCHAR)CapturedBase + CapturedRegionSize - 1);
#else
    EndingAddress = PAGE_ALIGN((PCHAR)CapturedBase + CapturedRegionSize - 1);
#endif

    Va = PAGE_ALIGN (CapturedBase);
    NumberOfAlreadyLocked = 0;
    NumberToLock = ((ULONG_PTR)EndingAddress - (ULONG_PTR)Va) >> PAGE_SHIFT;

    LOCK_WS_AND_ADDRESS_SPACE (TargetProcess);

    //
    // Make sure the address space was not deleted, if so, return an error.
    //

    if (TargetProcess->AddressSpaceDeleted != 0) {
        Status = STATUS_PROCESS_IS_TERMINATING;
        goto ErrorReturn;
    }

    if (NumberToLock + MM_FLUID_WORKING_SET >
                                    TargetProcess->Vm.MinimumWorkingSetSize) {
        Status = STATUS_WORKING_SET_QUOTA;
        goto ErrorReturn;
    }

    while (Va <= EndingAddress) {

        if (Va > LastVa) {

            //
            // Don't lock physically mapped views.
            //

            Vad = MiLocateAddress (Va);
            if (Vad == NULL) {
                Status = STATUS_ACCESS_VIOLATION;
                goto ErrorReturn;
            }

            if ((Vad->u.VadFlags.PhysicalMapping == 1) ||
                (Vad->u.VadFlags.UserPhysicalPages == 1)) {
                Status = STATUS_INCOMPATIBLE_FILE_MAP;
                goto ErrorReturn;
            }
            LastVa = MI_VPN_TO_VA (Vad->EndingVpn);
        }

        if (MmIsAddressValid (Va)) {

            //
            // The page is valid, therefore it is in the working set.
            // Locate the WSLE for the page and see if it is locked.
            //

            PointerPte1 = MiGetPteAddress (Va);
            Pfn1 = MI_PFN_ELEMENT (PointerPte1->u.Hard.PageFrameNumber);

            WorkingSetIndex = MiLocateWsle (Va,
                                            MmWorkingSetList,
                                            Pfn1->u1.WsIndex);

            ASSERT (WorkingSetIndex != WSLE_NULL_INDEX);

            if (WorkingSetIndex < MmWorkingSetList->FirstDynamic) {

                //
                // This page is locked in the working set.
                //

                NumberOfAlreadyLocked += 1;

                //
                // Check to see if the WAS_LOCKED status should be returned.
                //

                if ((MapType & MAP_PROCESS) &&
                        (MmWsle[WorkingSetIndex].u1.e1.LockedInWs == 1)) {
                    WasLocked = TRUE;
                }

                if ((MapType & MAP_SYSTEM) &&
                        (MmWsle[WorkingSetIndex].u1.e1.LockedInMemory == 1)) {
                    WasLocked = TRUE;
                }
            }
        }
        Va = (PVOID)((PCHAR)Va + PAGE_SIZE);
    }

    UNLOCK_WS_UNSAFE (TargetProcess);

    //
    // Check to ensure the working set list is still fluid after
    // the requested number of pages are locked.
    //

    if (TargetProcess->Vm.MinimumWorkingSetSize <
          ((MmWorkingSetList->FirstDynamic + NumberToLock +
                      MM_FLUID_WORKING_SET) - NumberOfAlreadyLocked)) {

        Status = STATUS_WORKING_SET_QUOTA;
        UNLOCK_ADDRESS_SPACE (TargetProcess);
        goto ErrorReturn1;
    }

    Va = PAGE_ALIGN (CapturedBase);

#if defined(_MIALT4K_)

    if (TargetProcess->Wow64Process != NULL) {

        IsWow64Process = TRUE;
        Va = PAGE_4K_ALIGN (CapturedBase);

    }

#endif

    //
    // Set up an exception handler and touch each page in the specified
    // range.
    //

    MiInsertConflictInList (&Conflict);

    try {

        while (Va <= EndingAddress) {
            *(volatile ULONG *)Va;
            Va = (PVOID)((PCHAR)Va + PAGE_SIZE);
        }

    } except (EXCEPTION_EXECUTE_HANDLER) {
        Status = GetExceptionCode();
        MiRemoveConflictFromList (&Conflict);
        UNLOCK_ADDRESS_SPACE (TargetProcess);
        goto ErrorReturn1;
    }

    MiRemoveConflictFromList (&Conflict);

    //
    // The complete address range is accessible, lock the pages into
    // the working set.
    //

    PointerPte = MiGetPteAddress (CapturedBase);
    Va = PAGE_ALIGN (CapturedBase);

#if defined(_MIALT4K_)

    if (IsWow64Process) {

        Va = PAGE_4K_ALIGN (CapturedBase);

    }

#endif

    //
    // Acquire the working set mutex, no page faults are allowed.
    //

    LOCK_WS_UNSAFE (TargetProcess);

    while (Va <= EndingAddress) {

        //
        // Make sure the PDE is valid.
        //

        PointerPde = MiGetPdeAddress (Va);
        PointerPpe = MiGetPteAddress (PointerPde);

        do {

            (VOID)MiDoesPpeExistAndMakeValid (PointerPpe,
                                              TargetProcess,
                                              FALSE,
                                              &Waited);

            Waited = 0;

            (VOID)MiDoesPdeExistAndMakeValid (PointerPde,
                                              TargetProcess,
                                              FALSE,
                                              &Waited);

        } while (Waited != 0);

        //
        // Make sure the page is in the working set.
        //

        while (PointerPte->u.Hard.Valid == 0) {

            //
            // Release the working set mutex and fault in the page.
            //

            UNLOCK_WS_UNSAFE (TargetProcess);

            //
            // Page in the PDE and make the PTE valid.
            //

            *(volatile ULONG *)Va;

            //
            // Reacquire the working set mutex.
            //

            LOCK_WS_UNSAFE (TargetProcess);

            //
            // Make sure the page directory & table pages are still valid.
            // Trimming could occur if either of the pages that were just
            // made valid were removed from the working set before the
            // working set lock was acquired.
            //

            do {

                (VOID)MiDoesPpeExistAndMakeValid (PointerPpe,
                                                  TargetProcess,
                                                  FALSE,
                                                  &Waited);

                Waited = 0;

                (VOID)MiDoesPdeExistAndMakeValid (PointerPde,
                                                  TargetProcess,
                                                  FALSE,
                                                  &Waited);
            } while (Waited != 0);
        }

        //
        // The page is now in the working set, lock the page into
        // the working set.
        //

        PointerPte1 = MiGetPteAddress (Va);
        Pfn1 = MI_PFN_ELEMENT (PointerPte1->u.Hard.PageFrameNumber);

        Entry = MiLocateWsle (Va, MmWorkingSetList, Pfn1->u1.WsIndex);

        if (Entry >= MmWorkingSetList->FirstDynamic) {

            SwapEntry = MmWorkingSetList->FirstDynamic;

            if (Entry != MmWorkingSetList->FirstDynamic) {

                //
                // Swap this entry with the one at first dynamic.
                //

                MiSwapWslEntries (Entry, SwapEntry, &TargetProcess->Vm);
            }

            MmWorkingSetList->FirstDynamic += 1;
        } else {
            SwapEntry = Entry;
        }

        //
        // Indicate that the page is locked.
        //

        if (MapType & MAP_PROCESS) {
            MmWsle[SwapEntry].u1.e1.LockedInWs = 1;
        }

        if (MapType & MAP_SYSTEM) {
            MmWsle[SwapEntry].u1.e1.LockedInMemory = 1;
        }

        //
        // Increment to the next va and PTE.
        //

        PointerPte += 1;
        Va = (PVOID)((PCHAR)Va + PAGE_SIZE);
    }

#if !(defined(_MIALT4K_))

    UNLOCK_WS_AND_ADDRESS_SPACE (TargetProcess);

#else

    UNLOCK_WS_UNSAFE (TargetProcess);

    if (IsWow64Process) {

        MiLockFor4kPage(CapturedBase, CapturedRegionSize, TargetProcess);

    }

    UNLOCK_ADDRESS_SPACE (TargetProcess);

#endif

    if (Attached == TRUE) {
        KeDetachProcess();
    }
    ObDereferenceObject (TargetProcess);

    //
    // Update return arguments.
    //

    //
    // Establish an exception handler and write the size and base
    // address.
    //

    try {

#if defined(_MIALT4K_)

        if (IsWow64Process) { 

            *RegionSize = ((PCHAR)EndingAddress -
                        (PCHAR)PAGE_4K_ALIGN(CapturedBase)) + PAGE_4K;

            *BaseAddress = PAGE_4K_ALIGN(CapturedBase);


        } else {    

#endif
        *RegionSize = ((PCHAR)EndingAddress - (PCHAR)PAGE_ALIGN(CapturedBase)) +
                                                                    PAGE_SIZE;
        *BaseAddress = PAGE_ALIGN(CapturedBase);

#if defined(_MIALT4K_)
        }
#endif

    } except (EXCEPTION_EXECUTE_HANDLER) {
        return GetExceptionCode();
    }

    if (WasLocked) {
        return STATUS_WAS_LOCKED;
    }

    return STATUS_SUCCESS;

ErrorReturn:
        UNLOCK_WS_AND_ADDRESS_SPACE (TargetProcess);
ErrorReturn1:
        if (Attached == TRUE) {
            KeDetachProcess();
        }
        ObDereferenceObject (TargetProcess);
        return Status;
}

NTSTATUS
NtUnlockVirtualMemory (
    IN HANDLE ProcessHandle,
    IN OUT PVOID *BaseAddress,
    IN OUT PSIZE_T RegionSize,
    IN ULONG MapType
    )

/*++

Routine Description:

    This function unlocks a region of pages within the working set list
    of a subject process.

    As a side effect, any pages which are not locked and are in the
    process's working set are removed from the process's working set.
    This allows NtUnlockVirtualMemory to remove a range of pages
    from the working set.

    The caller of this function must have PROCESS_VM_OPERATION access
    to the target process.

    The caller must also have SeLockMemoryPrivilege for MAP_SYSTEM.

Arguments:

   ProcessHandle - Supplies an open handle to a process object.

   BaseAddress - The base address of the region of pages
        to be unlocked. This value is rounded down to the
        next host page address boundary.

   RegionSize - A pointer to a variable that will receive
        the actual size in bytes of the unlocked region of
        pages. The initial value of this argument is
        rounded up to the next host page size boundary.

   MapType - A set of flags that describe the type of unlocking to
            perform.  One of MAP_PROCESS or MAP_SYSTEM.

Return Value:

    Returns the status

    TBS


--*/

{
    PVOID Va;
    PVOID EndingAddress;
    SIZE_T CapturedRegionSize;
    PVOID CapturedBase;
    PEPROCESS TargetProcess;
    NTSTATUS Status;
    KPROCESSOR_MODE PreviousMode;
    ULONG Entry;
    PMMPTE PointerPte;
    PMMPFN Pfn1;
    PMMVAD Vad;
    PVOID LastVa;
    LOGICAL Attached;
#if defined(_MIALT4K_)
    BOOLEAN IsWow64Process = FALSE;
#endif

    PAGED_CODE();

    LastVa = NULL;

    //
    // Validate the flags in MapType.
    //

    if ((MapType & ~(MAP_PROCESS | MAP_SYSTEM)) != 0) {
        return STATUS_INVALID_PARAMETER;
    }

    if ((MapType & (MAP_PROCESS | MAP_SYSTEM)) == 0) {
        return STATUS_INVALID_PARAMETER;
    }

    PreviousMode = KeGetPreviousMode();

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

    //
    // Make sure the specified starting and ending addresses are
    // within the user part of the virtual address space.
    //

    if (CapturedBase > MM_HIGHEST_USER_ADDRESS) {

        //
        // Invalid base address.
        //

        return STATUS_INVALID_PARAMETER;
    }

    if ((ULONG_PTR)MM_HIGHEST_USER_ADDRESS - (ULONG_PTR)CapturedBase <
                                                        CapturedRegionSize) {

        //
        // Invalid region size;
        //

        return STATUS_INVALID_PARAMETER;

    }

    if (CapturedRegionSize == 0) {
        return STATUS_INVALID_PARAMETER;
    }

    Status = ObReferenceObjectByHandle ( ProcessHandle,
                                         PROCESS_VM_OPERATION,
                                         PsProcessType,
                                         PreviousMode,
                                         (PVOID *)&TargetProcess,
                                         NULL );

    if (!NT_SUCCESS(Status)) {
        return Status;
    }

    if ((MapType & MAP_SYSTEM) != 0) {

        //
        // In addition to PROCESS_VM_OPERATION access to the target
        // process, the caller must have SE_LOCK_MEMORY_PRIVILEGE.
        //

        if (!SeSinglePrivilegeCheck(
                           SeLockMemoryPrivilege,
                           PreviousMode
                           )) {

            ObDereferenceObject( TargetProcess );
            return STATUS_PRIVILEGE_NOT_HELD;
        }
    }

    //
    // Attach to the specified process.
    //

    if (ProcessHandle != NtCurrentProcess()) {
        KeAttachProcess (&TargetProcess->Pcb);
        Attached = TRUE;
    }
    else {
        Attached = FALSE;
    }

    //
    // Get address creation mutex, this prevents the
    // address range from being modified while it is examined.
    // Block APCs so an APC routine can't get a page fault and
    // corrupt the working set list, etc.
    //

    LOCK_WS_AND_ADDRESS_SPACE (TargetProcess);

    //
    // Make sure the address space was not deleted, if so, return an error.
    //

    if (TargetProcess->AddressSpaceDeleted != 0) {
        Status = STATUS_PROCESS_IS_TERMINATING;
        goto ErrorReturn;
    }

    EndingAddress = PAGE_ALIGN((PCHAR)CapturedBase + CapturedRegionSize - 1);

    Va = PAGE_ALIGN (CapturedBase);

    while (Va <= EndingAddress) {

        //
        // Check to ensure all the specified pages are locked.
        //

        //
        // Don't unlock physically mapped views.
        //

        if (Va > LastVa) {
            Vad = MiLocateAddress (Va);
            if (Vad == NULL) {
                Va = (PVOID)((PCHAR)Va + PAGE_SIZE);
                Status = STATUS_NOT_LOCKED;
                break;
            }

            if ((Vad->u.VadFlags.PhysicalMapping == 1) ||
                (Vad->u.VadFlags.UserPhysicalPages == 1)) {
                Va = MI_VPN_TO_VA (Vad->EndingVpn);
                break;
            }
            LastVa = MI_VPN_TO_VA (Vad->EndingVpn);
        }

        if (!MmIsAddressValid (Va)) {

            //
            // This page is not valid, therefore not in working set.
            //

            Status = STATUS_NOT_LOCKED;
        } else {

            PointerPte = MiGetPteAddress (Va);
            ASSERT (PointerPte->u.Hard.Valid != 0);
            Pfn1 = MI_PFN_ELEMENT (PointerPte->u.Hard.PageFrameNumber);
            Entry = MiLocateWsle (Va, MmWorkingSetList, Pfn1->u1.WsIndex);
            ASSERT (Entry != WSLE_NULL_INDEX);

            if ((MmWsle[Entry].u1.e1.LockedInWs == 0) &&
                (MmWsle[Entry].u1.e1.LockedInMemory == 0)) {

                //
                // Not locked in memory or system, remove from working
                // set.
                //

                PERFINFO_PAGE_INFO_DECL();

                PERFINFO_GET_PAGE_INFO(PointerPte);

                if (MiFreeWsle (Entry, &TargetProcess->Vm, PointerPte)) {
                    PERFINFO_LOG_WS_REMOVAL(PERFINFO_LOG_TYPE_OUTWS_EMPTYQ, &TargetProcess->Vm);
                }

                Status = STATUS_NOT_LOCKED;

            } else if (MapType & MAP_PROCESS) {
                if (MmWsle[Entry].u1.e1.LockedInWs == 0)  {

                    //
                    // This page is not locked.
                    //

                    Status = STATUS_NOT_LOCKED;
                }
            } else {
                if (MmWsle[Entry].u1.e1.LockedInMemory == 0)  {

                    //
                    // This page is not locked.
                    //

                    Status = STATUS_NOT_LOCKED;
                }
            }
        }
        Va = (PVOID)((PCHAR)Va + PAGE_SIZE);
    } // end while

#if defined(_MIALT4K_)

    if (TargetProcess->Wow64Process != NULL) {
        
        IsWow64Process = TRUE;
        Status = MiUnlockFor4kPage(CapturedBase, CapturedRegionSize, TargetProcess);

    }

#endif

    if (Status == STATUS_NOT_LOCKED) {
        goto ErrorReturn;
    }

    //
    // The complete address range is locked, unlock them.
    //

    Va = PAGE_ALIGN (CapturedBase);
    LastVa = NULL;


    while (Va <= EndingAddress) {

#if defined(_MIALT4K_)

    if (IsWow64Process) {

        if (!MiShouldBeUnlockedFor4kPage(Va, TargetProcess)) {

            //
            // The other 4k pages in the native page still hold the page lock.
            // Should skip unlocking.

            Va = (PVOID)((PCHAR)Va + PAGE_SIZE);
            continue;
        }
    }

#endif
        //
        // Don't unlock physically mapped views.
        //

        if (Va > LastVa) {
            Vad = MiLocateAddress (Va);
            ASSERT (Vad != NULL);

            if ((Vad->u.VadFlags.PhysicalMapping == 1) ||
                (Vad->u.VadFlags.UserPhysicalPages == 1)) {
                Va = MI_VPN_TO_VA (Vad->EndingVpn);
                break;
            }
            LastVa = MI_VPN_TO_VA (Vad->EndingVpn);
        }

        PointerPte = MiGetPteAddress (Va);
        ASSERT (PointerPte->u.Hard.Valid == 1);
        Pfn1 = MI_PFN_ELEMENT (PointerPte->u.Hard.PageFrameNumber);
        Entry = MiLocateWsle (Va, MmWorkingSetList, Pfn1->u1.WsIndex);

        if (MapType & MAP_PROCESS) {
            MmWsle[Entry].u1.e1.LockedInWs = 0;
        }

        if (MapType & MAP_SYSTEM) {
            MmWsle[Entry].u1.e1.LockedInMemory = 0;
        }

        if ((MmWsle[Entry].u1.e1.LockedInMemory == 0) &&
             MmWsle[Entry].u1.e1.LockedInWs == 0) {

            //
            // The page is no longer should be locked, move
            // it to the dynamic part of the working set.
            //

            MmWorkingSetList->FirstDynamic -= 1;

            if (Entry != MmWorkingSetList->FirstDynamic) {

                //
                // Swap this element with the last locked page, making
                // this element the new first dynamic entry.
                //

                MiSwapWslEntries (Entry,
                                  MmWorkingSetList->FirstDynamic,
                                  &TargetProcess->Vm);
            }
        }

        Va = (PVOID)((PCHAR)Va + PAGE_SIZE);
    }

    UNLOCK_WS_AND_ADDRESS_SPACE (TargetProcess);
    if (Attached == TRUE) {
        KeDetachProcess();
    }
    ObDereferenceObject (TargetProcess);

    //
    // Update return arguments.
    //

    //
    // Establish an exception handler and write the size and base
    // address.
    //

    try {

#if defined(_MIALT4K_)

        if (IsWow64Process) { 

            *RegionSize = ((PCHAR)EndingAddress -
                        (PCHAR)PAGE_4K_ALIGN(CapturedBase)) + PAGE_4K;

            *BaseAddress = PAGE_4K_ALIGN(CapturedBase);


        } else {    

#endif
        *RegionSize = ((PCHAR)EndingAddress -
                        (PCHAR)PAGE_ALIGN(CapturedBase)) + PAGE_SIZE;

        *BaseAddress = PAGE_ALIGN(CapturedBase);

#if defined(_MIALT4K_)
        }
#endif

    } except (EXCEPTION_EXECUTE_HANDLER) {
        return GetExceptionCode();
    }

    return STATUS_SUCCESS;

ErrorReturn:

        UNLOCK_WS_AND_ADDRESS_SPACE (TargetProcess);
        if (Attached == TRUE) {
            KeDetachProcess();
        }
        ObDereferenceObject (TargetProcess);
        return Status;
}


//
// Nonpagable routines.
//

VOID
MiInsertConflictInList (
    PMMLOCK_CONFLICT Conflict
    )

{
    KIRQL OldIrql;
    Conflict->Thread = PsGetCurrentThread();
    ExAcquireSpinLock (&MmChargeCommitmentLock, &OldIrql);
    InsertHeadList ( &MmLockConflictList,
                     &Conflict->List);
    ExReleaseSpinLock (&MmChargeCommitmentLock, OldIrql);
    return;
}


VOID
MiRemoveConflictFromList (
    PMMLOCK_CONFLICT Conflict
    )
{
    KIRQL OldIrql;

    ExAcquireSpinLock (&MmChargeCommitmentLock, &OldIrql);
    RemoveEntryList (&Conflict->List);
    ExReleaseSpinLock (&MmChargeCommitmentLock, OldIrql);
    return;
}
