/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

   protect.c

Abstract:

    This module contains the routines which implement the
    NtProtectVirtualMemory service.

Author:

    Lou Perazzoli (loup) 18-Aug-1989
    Landy Wang (landyw) 02-June-1997

Revision History:

--*/

#include "mi.h"

#if DBG
PEPROCESS MmWatchProcess;
VOID MmFooBar(VOID);
#endif // DBG

HARDWARE_PTE
MiFlushTbAndCapture(
    IN PMMPTE PtePointer,
    IN HARDWARE_PTE TempPte,
    IN PMMPFN Pfn1
    );

ULONG
MiSetProtectionOnTransitionPte (
    IN PMMPTE PointerPte,
    IN ULONG ProtectionMask
    );

MMPTE
MiCaptureSystemPte (
    IN PMMPTE PointerProtoPte,
    IN PEPROCESS Process
    );

#if PFN_CONSISTENCY
VOID
MiSetOriginalPteProtection (
    IN PMMPFN Pfn1,
    IN ULONG ProtectionMask
    )
#endif // PFN_CONSISTENCY

extern CCHAR MmReadWrite[32];

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,NtProtectVirtualMemory)
#pragma alloc_text(PAGE,MiProtectVirtualMemory)
#pragma alloc_text(PAGE,MiSetProtectionOnSection)
#pragma alloc_text(PAGE,MiGetPageProtection)
#pragma alloc_text(PAGE,MiChangeNoAccessForkPte)
#endif


NTSTATUS
NtProtectVirtualMemory(
     IN HANDLE ProcessHandle,
     IN OUT PVOID *BaseAddress,
     IN OUT PSIZE_T RegionSize,
     IN ULONG NewProtect,
     OUT PULONG OldProtect
     )

/*++

Routine Description:

    This routine changes the protection on a region of committed pages
    within the virtual address space of the subject process.  Setting
    the protection on a range of pages causes the old protection to be
    replaced by the specified protection value.

Arguments:

     ProcessHandle - An open handle to a process object.

     BaseAddress - The base address of the region of pages
          whose protection is to be changed. This value is
          rounded down to the next host page address
          boundary.

     RegionSize - A pointer to a variable that will receive
          the actual size in bytes of the protected region
          of pages. The initial value of this argument is
          rounded up to the next host page size boundary.

     NewProtect - The new protection desired for the
          specified region of pages.

     Protect Values

          PAGE_NOACCESS - No access to the specified region
               of pages is allowed. An attempt to read,
               write, or execute the specified region
               results in an access violation (i.e. a GP
               fault).

          PAGE_EXECUTE - Execute access to the specified
               region of pages is allowed. An attempt to
               read or write the specified region results in
               an access violation.

          PAGE_READONLY - Read only and execute access to the
               specified region of pages is allowed. An
               attempt to write the specified region results
               in an access violation.

          PAGE_READWRITE - Read, write, and execute access to
               the specified region of pages is allowed. If
               write access to the underlying section is
               allowed, then a single copy of the pages are
               shared. Otherwise the pages are shared read
               only/copy on write.

          PAGE_GUARD - Read, write, and execute access to the
               specified region of pages is allowed,
               however, access to the region causes a "guard
               region entered" condition to be raised in the
               subject process. If write access to the
               underlying section is allowed, then a single
               copy of the pages are shared. Otherwise the
               pages are shared read only/copy on write.

          PAGE_NOCACHE - The page should be treated as uncached.
               This is only valid for non-shared pages.


     OldProtect - A pointer to a variable that will receive
          the old protection of the first page within the
          specified region of pages.

Return Value:

    Returns the status

    TBS


Environment:

    Kernel mode.

--*/


{
    //
    // note - special treatment for the following cases...
    //
    // if a page is locked in the working set (memory?) and the
    // protection is changed to no access, the page should be
    // removed from the working set... valid pages can't be no access.
    //
    // if page is going to be read only or no access? and is demand
    // zero, make sure it is changed to a page of zeroes.
    //
    // update the vm spec to explain locked pages are unlocked when
    // freed or protection is changed to no-access (this may be a nasty
    // problem if we don't want to do this!!
    //

    PEPROCESS Process;
    KPROCESSOR_MODE PreviousMode;
    NTSTATUS Status;
    ULONG Attached = FALSE;
    PVOID CapturedBase;
    SIZE_T CapturedRegionSize;
    ULONG ProtectionMask;

    ULONG LastProtect;

    PAGED_CODE();

    //
    // Check the protection field.  This could raise an exception.
    //

    try {
        ProtectionMask = MiMakeProtectionMask (NewProtect);
    } except (EXCEPTION_EXECUTE_HANDLER) {
        return GetExceptionCode();
    }

    PreviousMode = KeGetPreviousMode();

    if (PreviousMode != KernelMode) {

        //
        // Capture the region size and base address under an exception handler.
        //

        try {

            ProbeForWritePointer (BaseAddress);
            ProbeForWriteUlong_ptr (RegionSize);
            ProbeForWriteUlong (OldProtect);

            //
            // Capture the region size and base address.
            //

            CapturedBase = *BaseAddress;
            CapturedRegionSize = *RegionSize;

        } except (EXCEPTION_EXECUTE_HANDLER) {

            //
            // If an exception occurs during the probe or capture
            // of the initial values, then handle the exception and
            // return the exception code as the status value.
            //

            return GetExceptionCode();
        }

    } else {

        //
        // Capture the region size and base address.
        //

        CapturedRegionSize = *RegionSize;
        CapturedBase = *BaseAddress;
    }

#if DBG
    if (MmDebug & MM_DBG_SHOW_NT_CALLS) {
        if ( !MmWatchProcess ) {
            DbgPrint("protectvm process handle %lx base address %lx size %lx protect %lx\n",
                ProcessHandle, CapturedBase, CapturedRegionSize, NewProtect);
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

    if (CapturedRegionSize == 0) {
        return STATUS_INVALID_PARAMETER_3;
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

    //
    // If the specified process is not the current process, attach
    // to the specified process.
    //

    if (PsGetCurrentProcess() != Process) {
        KeAttachProcess (&Process->Pcb);
        Attached = TRUE;
    }

    Status = MiProtectVirtualMemory (Process,
                                     &CapturedBase,
                                     &CapturedRegionSize,
                                     NewProtect,
                                     &LastProtect);


    if (Attached) {
        KeDetachProcess();
    }

    ObDereferenceObject (Process);

    //
    // Establish an exception handler and write the size and base
    // address.
    //

    try {

        //
        // Reprobe the addresses as certain architectures (intel 386 for one)
        // do not trap kernel writes.  This is the one service which allows
        // the protection of the page to change between the initial probe
        // and the final argument update.
        //

        if (PreviousMode != KernelMode) {

            ProbeForWritePointer (BaseAddress);
            ProbeForWriteUlong_ptr (RegionSize);
            ProbeForWriteUlong (OldProtect);
        }

        *RegionSize = CapturedRegionSize;
        *BaseAddress = CapturedBase;
        *OldProtect = LastProtect;

    } except (EXCEPTION_EXECUTE_HANDLER) {
        NOTHING;
    }

    return Status;
}


NTSTATUS
MiProtectVirtualMemory (
    IN PEPROCESS Process,
    IN PVOID *BaseAddress,
    IN PSIZE_T RegionSize,
    IN ULONG NewProtect,
    IN PULONG LastProtect
    )

/*++

Routine Description:

    This routine changes the protection on a region of committed pages
    within the virtual address space of the subject process.  Setting
    the protection on a range of pages causes the old protection to be
    replaced by the specified protection value.

Arguments:

    Process - Supplies a pointer to the current process.

    BaseAddress - Supplies the starting address to protect.

    RegionsSize - Supplies the size of the region to protect.

    NewProtect - Supplies the new protection to set.

    LastProtect - Supplies the address of a kernel owned pointer to
                store (without probing) the old protection into.


Return Value:

    the status of the protect operation.

Environment:

    Kernel mode

--*/

{
    PMMVAD FoundVad;
    PVOID StartingAddress;
    PVOID EndingAddress;
    PVOID CapturedBase;
    SIZE_T CapturedRegionSize;
    NTSTATUS Status;
    ULONG Attached;
    PMMPTE PointerPte;
    PMMPTE LastPte;
    PMMPTE PointerPde;
    PMMPTE PointerPpe;
    PMMPTE PointerProtoPte;
    PMMPTE LastProtoPte;
    PMMPFN Pfn1;
    ULONG CapturedOldProtect;
    ULONG ProtectionMask;
    MMPTE TempPte;
    MMPTE PteContents;
    MMPTE PreviousPte;
    ULONG Locked;
    PVOID Va;
    ULONG DoAgain;
    ULONG Waited;
    PVOID UsedPageTableHandle;
    PVOID UsedPageDirectoryHandle;
    ULONG WorkingSetIndex;
#if defined(_MIALT4K_)
    PVOID OriginalBase;
    SIZE_T OriginalRegionSize;
    ULONG OriginalProtectionMask;
    PVOID StartingAddressFor4k;
    PVOID EndingAddressFor4k;
    SIZE_T CapturedRegionSizeFor4k;
    ULONG CapturedOldProtectFor4k;
    BOOLEAN EmulationFor4kPage = FALSE; 
#endif

    Attached = FALSE;
    Locked = FALSE;

    //
    // Get the address creation mutex to block multiple threads from
    // creating or deleting address space at the same time.
    // Get the working set mutex so PTEs can be modified.
    // Block APCs so an APC which takes a page
    // fault does not corrupt various structures.
    //

    CapturedBase = *BaseAddress;
    CapturedRegionSize = *RegionSize;

#if defined(_MIALT4K_)
    OriginalBase = CapturedBase;
    OriginalRegionSize = CapturedRegionSize;

    if (Process->Wow64Process != NULL) {

        StartingAddressFor4k = (PVOID)PAGE_4K_ALIGN(OriginalBase);

        EndingAddressFor4k = (PVOID)(((ULONG_PTR)OriginalBase +
                                      OriginalRegionSize - 1) | (PAGE_4K - 1));
            
        CapturedRegionSizeFor4k = (ULONG_PTR)EndingAddressFor4k - 
            (ULONG_PTR)StartingAddressFor4k + 1L;

        OriginalProtectionMask = MiMakeProtectionMask(NewProtect);

        EmulationFor4kPage = TRUE;

    }
#endif

    try {
        ProtectionMask = MiMakeProtectionMask (NewProtect);
    } except (EXCEPTION_EXECUTE_HANDLER) {
        return GetExceptionCode();
    }

    LOCK_WS_AND_ADDRESS_SPACE (Process);

    //
    // Make sure the address space was not deleted, if so, return an error.
    //

    if (Process->AddressSpaceDeleted != 0) {
        Status = STATUS_PROCESS_IS_TERMINATING;
        goto ErrorFound;
    }

    EndingAddress = (PVOID)(((ULONG_PTR)CapturedBase +
                                CapturedRegionSize - 1L) | (PAGE_SIZE - 1L));
    StartingAddress = (PVOID)PAGE_ALIGN(CapturedBase);
    FoundVad = MiCheckForConflictingVad (StartingAddress, EndingAddress);

    if (FoundVad == (PMMVAD)NULL) {

        //
        // No virtual address is reserved at the specified base address,
        // return an error.
        //

        Status = STATUS_CONFLICTING_ADDRESSES;
        goto ErrorFound;
    }

    //
    // Ensure that the starting and ending addresses are all within
    // the same virtual address descriptor.
    //

    if ((MI_VA_TO_VPN (StartingAddress) < FoundVad->StartingVpn) ||
        (MI_VA_TO_VPN (EndingAddress) > FoundVad->EndingVpn)) {

        //
        // Not within the section virtual address descriptor,
        // return an error.
        //

        Status = STATUS_CONFLICTING_ADDRESSES;
        goto ErrorFound;
    }

    if (FoundVad->u.VadFlags.UserPhysicalPages == 1) {

        //
        // These must always be readwrite.
        //

        Status = STATUS_CONFLICTING_ADDRESSES;
        goto ErrorFound;
    }

    if (FoundVad->u.VadFlags.PhysicalMapping == 1) {

        //
        // Setting the protection of a physically mapped section is
        // not allowed as there is no corresponding PFN database element.
        //

        Status = STATUS_CONFLICTING_ADDRESSES;
        goto ErrorFound;
    }

    if (FoundVad->u.VadFlags.NoChange == 1) {

        //
        // An attempt is made at changing the protection
        // of a secured VAD, check to see if the address range
        // to change allows the change.
        //

        Status = MiCheckSecuredVad (FoundVad,
                                    CapturedBase,
                                    CapturedRegionSize,
                                    ProtectionMask);

        if (!NT_SUCCESS (Status)) {
            goto ErrorFound;
        }
    }
#if defined(_MIALT4K_)
    else if (EmulationFor4kPage == TRUE) {
        //
        // if not secured, relax the protection
        //

        NewProtect = MiMakeProtectForNativePage(StartingAddressFor4k, 
                                                NewProtect, 
                                                Process);

        ProtectionMask = MiMakeProtectionMask (NewProtect);
    }
#endif

    if (FoundVad->u.VadFlags.PrivateMemory == 0) {


        //
        // For mapped sections, the NO_CACHE attribute is not allowed.
        //

        if (NewProtect & PAGE_NOCACHE) {

            //
            // Not allowed.
            //

            Status = STATUS_INVALID_PARAMETER_4;
            goto ErrorFound;
        }

        //
        // If this is a file mapping, then all pages must be
        // committed as there can be no sparse file maps. Images
        // can have non-committed pages if the alignment is greater
        // than the page size.
        //

        if ((FoundVad->ControlArea->u.Flags.File == 0) ||
            (FoundVad->ControlArea->u.Flags.Image == 1)) {

            PointerProtoPte = MiGetProtoPteAddress (FoundVad,
                                        MI_VA_TO_VPN (StartingAddress));
            LastProtoPte = MiGetProtoPteAddress (FoundVad,
                                        MI_VA_TO_VPN (EndingAddress));

            //
            // Release the working set mutex and acquire the section
            // commit mutex.  Check all the prototype PTEs described by
            // the virtual address range to ensure they are committed.
            //

            UNLOCK_WS_UNSAFE (Process);
            ExAcquireFastMutexUnsafe (&MmSectionCommitMutex);

            while (PointerProtoPte <= LastProtoPte) {

                //
                // Check to see if the prototype PTE is committed, if
                // not return an error.
                //

                if (PointerProtoPte->u.Long == 0) {

                    //
                    // Error, this prototype PTE is not committed.
                    //

                    ExReleaseFastMutexUnsafe (&MmSectionCommitMutex);
                    Status = STATUS_NOT_COMMITTED;
                    UNLOCK_ADDRESS_SPACE (Process);
                    goto ErrorFoundNoWs;
                }
                PointerProtoPte += 1;
            }

            //
            // The range is committed, release the section commitment
            // mutex, acquire the working set mutex and update the local PTEs.
            //

            ExReleaseFastMutexUnsafe (&MmSectionCommitMutex);

            //
            // Set the protection on the section pages.  This could
            // get a quota exceeded exception.
            //

            LOCK_WS_UNSAFE (Process);
        }

#if defined(_MIALT4K_)

        //
        // We need to change the alternate table before PTEs are created for the protection 
        // change.
        //

        if (EmulationFor4kPage == TRUE) {

            //
            // Before accessing Alternate Table, unlock the working set mutex
            //

            UNLOCK_WS_UNSAFE (Process);

            //
            // Get the old protection
            //

            CapturedOldProtectFor4k = 
                MiQueryProtectionFor4kPage(StartingAddressFor4k, Process);
            
            if (CapturedOldProtectFor4k != 0) {
 
                CapturedOldProtectFor4k = 
                    MI_CONVERT_FROM_PTE_PROTECTION(CapturedOldProtectFor4k);

            }

            //
            // Set the alternate permission table
            //

            if ((FoundVad->u.VadFlags.ImageMap == 1) ||
                (FoundVad->u2.VadFlags2.CopyOnWrite == 1)) {

                OriginalProtectionMask |= MM_PROTECTION_COPY_MASK;

            }

            MiProtectFor4kPage (StartingAddressFor4k, 
                                CapturedRegionSizeFor4k, 
                                OriginalProtectionMask, 
                                ALT_CHANGE,
                                Process);

            LOCK_WS_UNSAFE (Process);
        }
#endif

        try {
            Locked = MiSetProtectionOnSection ( Process,
                                                FoundVad,
                                                StartingAddress,
                                                EndingAddress,
                                                NewProtect,
                                                &CapturedOldProtect,
                                                FALSE );

        } except (EXCEPTION_EXECUTE_HANDLER) {

            Status = GetExceptionCode();
            goto ErrorFound;
        }
    } else {

        //
        // Not a section, private.
        // For private pages, the WRITECOPY attribute is not allowed.
        //

        if ((NewProtect & PAGE_WRITECOPY) ||
            (NewProtect & PAGE_EXECUTE_WRITECOPY)) {

            //
            // Not allowed.
            //

            Status = STATUS_INVALID_PARAMETER_4;
            goto ErrorFound;
        }

        //
        // Ensure all of the pages are already committed as described
        // in the virtual address descriptor.
        //

        if ( !MiIsEntireRangeCommitted(StartingAddress,
                                         EndingAddress,
                                         FoundVad,
                                         Process)) {

            //
            // Previously reserved pages have been decommitted, or an error
            // occurred, release mutex and return status.
            //

            Status = STATUS_NOT_COMMITTED;
            goto ErrorFound;
        }

#if defined(_MIALT4K_)

        //
        // We need to change the alternate table before PTEs are created for the protection 
        // change.
        //

        if (EmulationFor4kPage == TRUE) {

            //
            // Before accessing Alternate Table, unlock the working set mutex
            //

            UNLOCK_WS_UNSAFE (Process);

            //
            // Get the old protection
            //

            CapturedOldProtectFor4k = 
                MiQueryProtectionFor4kPage(StartingAddressFor4k, Process);
            
            if (CapturedOldProtectFor4k != 0) {
 
                CapturedOldProtectFor4k = 
                    MI_CONVERT_FROM_PTE_PROTECTION(CapturedOldProtectFor4k);

            }

            //
            // Set the alternate permission table
            //

            MiProtectFor4kPage (StartingAddressFor4k, 
                                CapturedRegionSizeFor4k, 
                                OriginalProtectionMask, 
                                ALT_CHANGE,
                                Process);

            LOCK_WS_UNSAFE (Process);
        }
#endif

        //
        // The address range is committed, change the protection.
        //

        PointerPpe = MiGetPpeAddress (StartingAddress);
        PointerPde = MiGetPdeAddress (StartingAddress);
        PointerPte = MiGetPteAddress (StartingAddress);
        LastPte = MiGetPteAddress (EndingAddress);

#if defined (_WIN64)
        MiMakePpeExistAndMakeValid (PointerPpe, Process, FALSE);
        if (PointerPde->u.Long == 0) {
            UsedPageDirectoryHandle = MI_GET_USED_PTES_HANDLE (PointerPte);
            MI_INCREMENT_USED_PTES_BY_HANDLE (UsedPageDirectoryHandle);
        }
#endif

        MiMakePdeExistAndMakeValid(PointerPde, Process, FALSE);

        //
        // Capture the protection for the first page.
        //

        if (PointerPte->u.Long != 0) {

            CapturedOldProtect = MiGetPageProtection (PointerPte, Process);

            //
            // Make sure the page directory & table pages are still resident.
            //

#if defined (_WIN64)
            MiMakePpeExistAndMakeValid (PointerPpe, Process, FALSE);
            if (PointerPde->u.Long == 0) {
                UsedPageDirectoryHandle = MI_GET_USED_PTES_HANDLE (PointerPte);
                MI_INCREMENT_USED_PTES_BY_HANDLE (UsedPageDirectoryHandle);
            }
#endif

            (VOID) MiMakePdeExistAndMakeValid(PointerPde, Process, FALSE);

        } else {

            //
            // Get the protection from the VAD.
            //

            CapturedOldProtect =
               MI_CONVERT_FROM_PTE_PROTECTION(FoundVad->u.VadFlags.Protection);
        }

        //
        // For all the PTEs in the specified address range, set the
        // protection depending on the state of the PTE.
        //

        while (PointerPte <= LastPte) {

            if (MiIsPteOnPdeBoundary (PointerPte)) {

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
            }

            PteContents = *PointerPte;

            if (PteContents.u.Long == 0) {

                //
                // Increment the count of non-zero page table entries
                // for this page table and the number of private pages
                // for the process.  The protection will be set as
                // if the PTE was demand zero.
                //

                UsedPageTableHandle = MI_GET_USED_PTES_HANDLE (MiGetVirtualAddressMappedByPte (PointerPte));

                MI_INCREMENT_USED_PTES_BY_HANDLE (UsedPageTableHandle);
            }

            if (PteContents.u.Hard.Valid == 1) {

                //
                // Set the protection into both the PTE and the original PTE
                // in the PFN database.
                //

                Pfn1 = MI_PFN_ELEMENT (PteContents.u.Hard.PageFrameNumber);

                if (Pfn1->u3.e1.PrototypePte == 1) {

                    //
                    // This PTE refers to a fork prototype PTE, make it
                    // private.
                    //

                    MiCopyOnWrite (MiGetVirtualAddressMappedByPte (PointerPte),
                                    PointerPte);

                    //
                    // This may have released the working set mutex and
                    // the page directory and table pages may no longer be
                    // in memory.
                    //

                    do {

                        (VOID)MiDoesPpeExistAndMakeValid (MiGetPteAddress (PointerPde),
                                                          Process,
                                                          FALSE,
                                                          &Waited);

                        Waited = 0;

                        (VOID)MiDoesPdeExistAndMakeValid (PointerPde,
                                                          Process,
                                                          FALSE,
                                                          &Waited);

                    } while (Waited != 0);

                    //
                    // Do the loop again for the same PTE.
                    //

                    continue;
                } else {

                    //
                    // The PTE is a private page which is valid, if the
                    // specified protection is no-access or guard page
                    // remove the PTE from the working set.
                    //

                    if ((NewProtect & PAGE_NOACCESS) ||
                        (NewProtect & PAGE_GUARD)) {

                        //
                        // Remove the page from the working set.
                        //

                        Locked = MiRemovePageFromWorkingSet (PointerPte,
                                                             Pfn1,
                                                             &Process->Vm);


                        continue;
                    } else {

#if PFN_CONSISTENCY
                        MiSetOriginalPteProtection (Pfn1, ProtectionMask);
#else
                        Pfn1->OriginalPte.u.Soft.Protection = ProtectionMask;
#endif
                        MI_MAKE_VALID_PTE (TempPte,
                                           PointerPte->u.Hard.PageFrameNumber,
                                           ProtectionMask,
                                           PointerPte);

                        WorkingSetIndex = MI_GET_WORKING_SET_FROM_PTE (&PteContents);
                        MI_SET_PTE_IN_WORKING_SET (&TempPte, WorkingSetIndex);

                        //
                        // Flush the TB as we have changed the protection
                        // of a valid PTE.
                        //

                        PreviousPte.u.Flush = MiFlushTbAndCapture (PointerPte,
                                                               TempPte.u.Flush,
                                                               Pfn1);
                    }
                }
            } else {

                if (PteContents.u.Soft.Prototype == 1) {

                    //
                    // This PTE refers to a fork prototype PTE, make the
                    // page private.  This is accomplished by releasing
                    // the working set mutex, reading the page thereby
                    // causing a fault, and re-executing the loop, hopefully,
                    // this time, we'll find the page present and will
                    // turn it into a private page.
                    //
                    // Note, that a TRY is used to catch guard
                    // page exceptions and no-access exceptions.
                    //

                    Va = MiGetVirtualAddressMappedByPte (PointerPte);

                    DoAgain = TRUE;

                    while (PteContents.u.Hard.Valid == 0) {

                        UNLOCK_WS_UNSAFE (Process);

                        try {

                            *(volatile ULONG *)Va;
                        } except (EXCEPTION_EXECUTE_HANDLER) {

                            if (GetExceptionCode() ==
                                                STATUS_ACCESS_VIOLATION) {

                                //
                                // The prototype PTE must be noaccess.
                                //

                                DoAgain = MiChangeNoAccessForkPte (PointerPte,
                                                                ProtectionMask);
                            } else if (GetExceptionCode() ==
                                                    STATUS_IN_PAGE_ERROR) {
                                //
                                // Ignore this page and go onto the next one.
                                //

                                PointerPte += 1;
                                DoAgain = TRUE;

                                if (PointerPte > LastPte) {
                                    LOCK_WS_UNSAFE (Process);
                                    break;
                                }
                            }
                        }

                        LOCK_WS_UNSAFE (Process);

                        do {

                            (VOID)MiDoesPpeExistAndMakeValid (MiGetPteAddress (PointerPde),
                                                              Process,
                                                              FALSE,
                                                              &Waited);

                            Waited = 0;

                            (VOID)MiDoesPdeExistAndMakeValid (PointerPde,
                                                              Process,
                                                              FALSE,
                                                              &Waited);

                        } while (Waited != 0);

                        PteContents = *(volatile MMPTE *)PointerPte;
                    }

                    if (DoAgain) {
                        continue;
                    }

                } else {

                    if (PteContents.u.Soft.Transition == 1) {

                        if (MiSetProtectionOnTransitionPte (
                                                    PointerPte,
                                                    ProtectionMask)) {
                            continue;
                        }
                    } else {

                        //
                        // Must be page file space or demand zero.
                        //

                        PointerPte->u.Soft.Protection = ProtectionMask;
                        ASSERT (PointerPte->u.Long != 0);
                    }
                }
            }
            PointerPte += 1;
        } //end while
    }

    //
    // Common completion code.
    //

#if defined(_MIALT4K_)

    if (EmulationFor4kPage == TRUE) {

        StartingAddress = StartingAddressFor4k;

        EndingAddress = EndingAddressFor4k;
            
        if (CapturedOldProtectFor4k != 0) {

            //
            // change CapturedOldProtect when CapturedOldProtectFor4k
            // contains the true protection for the 4k page
            //

            CapturedOldProtect = CapturedOldProtectFor4k;

        }
    }
#endif

    *RegionSize = (PCHAR)EndingAddress - (PCHAR)StartingAddress + 1L;
    *BaseAddress = StartingAddress;
    *LastProtect = CapturedOldProtect;

    if (Locked) {
        Status = STATUS_WAS_UNLOCKED;
    } else {
        Status = STATUS_SUCCESS;
    }

ErrorFound:
    UNLOCK_WS_AND_ADDRESS_SPACE (Process);

ErrorFoundNoWs:
    return Status;
}


ULONG
MiSetProtectionOnSection (
    IN PEPROCESS Process,
    IN PMMVAD FoundVad,
    IN PVOID StartingAddress,
    IN PVOID EndingAddress,
    IN ULONG NewProtect,
    OUT PULONG CapturedOldProtect,
    IN ULONG DontCharge
    )

/*++

Routine Description:

    This routine changes the protection on a region of committed pages
    within the virtual address space of the subject process.  Setting
    the protection on a range of pages causes the old protection to be
    replaced by the specified protection value.

Arguments:

    Process - Supplies a pointer to the current process.

    FoundVad - Supplies a pointer to the VAD containing the range to protect.

    StartingAddress - Supplies the starting address to protect.

    EndingAddress - Supplies the ending address to protect.

    NewProtect - Supplies the new protection to set.

    CapturedOldProtect - Supplies the address of a kernel owned pointer to
                store (without probing) the old protection into.

    DontCharge - Supplies TRUE if no quota or commitment should be charged.

Return Value:

    Returns TRUE if a locked page was removed from the working set (protection
    was guard page or no-access, FALSE otherwise.

Exceptions raised for page file quota or commitment violations.

Environment:

    Kernel mode, working set mutex held, address creation mutex held
    APCs disabled.

--*/

{
    PMMPTE PointerPte;
    PMMPTE LastPte;
    PMMPTE PointerPde;
    PMMPTE PointerPpe;
    PMMPTE PointerProtoPte;
    PMMPFN Pfn1;
    MMPTE TempPte;
    MMPTE PreviousPte;
    ULONG Locked;
    ULONG ProtectionMask;
    ULONG ProtectionMaskNotCopy;
    ULONG NewProtectionMask;
    MMPTE PteContents;
    ULONG Index;
    PULONG Va;
    ULONG WriteCopy;
    ULONG DoAgain;
    ULONG Waited;
    SIZE_T QuotaCharge;
    PVOID UsedPageTableHandle;
    PVOID UsedPageDirectoryHandle;
    ULONG WorkingSetIndex;

    PAGED_CODE();

    Locked = FALSE;
    WriteCopy = FALSE;
    QuotaCharge = 0;

    //
    // Make the protection field.
    //

    ASSERT (FoundVad->u.VadFlags.PrivateMemory == 0);

    if ((FoundVad->u.VadFlags.ImageMap == 1) ||
        (FoundVad->u2.VadFlags2.CopyOnWrite == 1)) {

        if (NewProtect & PAGE_READWRITE) {
            NewProtect &= ~PAGE_READWRITE;
            NewProtect |= PAGE_WRITECOPY;
        }

        if (NewProtect & PAGE_EXECUTE_READWRITE) {
            NewProtect &= ~PAGE_EXECUTE_READWRITE;
            NewProtect |= PAGE_EXECUTE_WRITECOPY;
        }
    }

    ProtectionMask = MiMakeProtectionMask (NewProtect);

    //
    // Determine if copy on write is being set.
    //

    ProtectionMaskNotCopy = ProtectionMask;
    if ((ProtectionMask & MM_COPY_ON_WRITE_MASK) == MM_COPY_ON_WRITE_MASK) {
        WriteCopy = TRUE;
        ProtectionMaskNotCopy &= ~MM_PROTECTION_COPY_MASK;
    }

#if defined(_MIALT4K_)

    if ((Process->Wow64Process != NULL) && 
        (FoundVad->u.VadFlags.ImageMap == 0) &&
        (FoundVad->u2.VadFlags2.CopyOnWrite == 0) && 
        (WriteCopy)) {
        
        NTSTATUS status;

        status = MiSetCopyPagesFor4kPage(Process,
                                         &FoundVad,
                                         &StartingAddress,
                                         &EndingAddress,
                                         ProtectionMask);

        if (status != STATUS_SUCCESS) {
            ExRaiseStatus (status);
        }

    }
        
#endif

    PointerPpe = MiGetPpeAddress (StartingAddress);
    PointerPde = MiGetPdeAddress (StartingAddress);
    PointerPte = MiGetPteAddress (StartingAddress);
    LastPte = MiGetPteAddress (EndingAddress);

#if defined (_WIN64)
    MiMakePpeExistAndMakeValid (PointerPpe, Process, FALSE);
    if (PointerPde->u.Long == 0) {
        UsedPageDirectoryHandle = MI_GET_USED_PTES_HANDLE (PointerPte);
        MI_INCREMENT_USED_PTES_BY_HANDLE (UsedPageDirectoryHandle);
    }
#endif

    MiMakePdeExistAndMakeValid(PointerPde, Process, FALSE);

    //
    // Capture the protection for the first page.
    //

    if (PointerPte->u.Long != 0) {

        *CapturedOldProtect = MiGetPageProtection (PointerPte, Process);

        //
        // Make sure the Page table page is still resident.
        //

        PointerPpe = MiGetPteAddress (PointerPde);

        do {

            (VOID)MiDoesPpeExistAndMakeValid (PointerPpe, Process, FALSE, &Waited);
            Waited = 0;

            (VOID)MiDoesPdeExistAndMakeValid (PointerPde, Process, FALSE, &Waited);
        } while (Waited != 0);

    } else {

        //
        // Get the protection from the VAD, unless image file.
        //

        if (FoundVad->u.VadFlags.ImageMap == 0) {

            //
            // This is not an image file, the protection is in the VAD.
            //

            *CapturedOldProtect =
                MI_CONVERT_FROM_PTE_PROTECTION(FoundVad->u.VadFlags.Protection);
        } else {

            //
            // This is an image file, the protection is in the
            // prototype PTE.
            //

            PointerProtoPte = MiGetProtoPteAddress (FoundVad,
                                    MI_VA_TO_VPN (
                                    MiGetVirtualAddressMappedByPte (PointerPte)));

            TempPte = MiCaptureSystemPte (PointerProtoPte, Process);

            *CapturedOldProtect = MiGetPageProtection (&TempPte,
                                                       Process);

            //
            // Make sure the Page directory and table pages are still resident.
            //

            PointerPpe = MiGetPteAddress (PointerPde);

            do {

                (VOID)MiDoesPpeExistAndMakeValid(PointerPpe, Process, FALSE, &Waited);
                Waited = 0;

                (VOID)MiDoesPdeExistAndMakeValid(PointerPde, Process, FALSE, &Waited);
            } while (Waited != 0);
        }
    }

    //
    // If the page protection is being change to be copy-on-write, the
    // commitment and page file quota for the potentially dirty private pages
    // must be calculated and charged.  This must be done before any
    // protections are changed as the changes cannot be undone.
    //

    if (WriteCopy) {

        //
        // Calculate the charges.  If the page is shared and not write copy
        // it is counted as a charged page.
        //

        while (PointerPte <= LastPte) {

            if (MiIsPteOnPdeBoundary (PointerPte)) {

                PointerPde = MiGetPteAddress (PointerPte);
                PointerPpe = MiGetPteAddress (PointerPde);

                do {

                    while (!MiDoesPpeExistAndMakeValid(PointerPpe, Process, FALSE, &Waited)) {

                        //
                        // No PPE exists for this address.  Therefore
                        // all the PTEs are shared and not copy on write.
                        // go to the next PPE.
                        //

                        PointerPpe += 1;
                        PointerPde = MiGetVirtualAddressMappedByPte (PointerPpe);
                        PointerProtoPte = PointerPte;
                        PointerPte = MiGetVirtualAddressMappedByPte (PointerPde);

                        if (PointerPte > LastPte) {
                            QuotaCharge += 1 + LastPte - PointerProtoPte;
                            goto Done;
                        }
                        QuotaCharge += PointerPte - PointerProtoPte;
                    }

                    Waited = 0;

                    while (!MiDoesPdeExistAndMakeValid(PointerPde, Process, FALSE, &Waited)) {

                        //
                        // No PDE exists for this address.  Therefore
                        // all the PTEs are shared and not copy on write.
                        // go to the next PDE.
                        //

                        PointerPde += 1;
                        PointerProtoPte = PointerPte;
                        PointerPpe = MiGetPteAddress (PointerPde);
                        PointerPte = MiGetVirtualAddressMappedByPte (PointerPde);

                        if (PointerPte > LastPte) {
                            QuotaCharge += 1 + LastPte - PointerProtoPte;
                            goto Done;
                        }
                        QuotaCharge += PointerPte - PointerProtoPte;
#if defined (_WIN64)
                        if (MiIsPteOnPdeBoundary (PointerPde)) {
                            Waited = 1;
                            break;
                        }
#endif
                    }
                } while (Waited != 0);
            }

            PteContents = *PointerPte;

            if (PteContents.u.Long == 0) {

                //
                // The PTE has not been evaluated, assume copy on write.
                //

                QuotaCharge += 1;

            } else if ((PteContents.u.Hard.Valid == 1) &&
                (PteContents.u.Hard.CopyOnWrite == 0)) {

                //
                // See if this is a prototype PTE, if so charge it.
                //

                Pfn1 = MI_PFN_ELEMENT (PteContents.u.Hard.PageFrameNumber);

                if (Pfn1->u3.e1.PrototypePte == 1) {
                    QuotaCharge += 1;
                }
            } else {

                if (PteContents.u.Soft.Prototype == 1) {

                    //
                    // This is a prototype PTE.  Charge if it is not
                    // in copy on write format.
                    //

                    if (PteContents.u.Soft.PageFileHigh == MI_PTE_LOOKUP_NEEDED) {

                        //
                        // Page protection is within the PTE.
                        //

                        if (!MI_IS_PTE_PROTECTION_COPY_WRITE(PteContents.u.Soft.Protection)) {
                            QuotaCharge += 1;
                        }
                    } else {

                        //
                        // The PTE references the prototype directly, therefore
                        // it can't be copy on write.  Charge.
                        //

                        QuotaCharge += 1;
                    }
                }
            }
            PointerPte += 1;
        }

Done:
        NOTHING;

        //
        // Charge for the quota.
        //

        if (!DontCharge) {
            MiChargePageFileQuota (QuotaCharge, Process);

            if (Process->CommitChargeLimit) {
                if (Process->CommitCharge + QuotaCharge > Process->CommitChargeLimit) {
                    MiReturnPageFileQuota (QuotaCharge, Process);
                    if (Process->Job) {
                        PsReportProcessMemoryLimitViolation ();
                    }
                    ExRaiseStatus (STATUS_COMMITMENT_LIMIT);
                }
            }
            if (Process->JobStatus & PS_JOB_STATUS_REPORT_COMMIT_CHANGES) {
                if (PsChangeJobMemoryUsage(QuotaCharge) == FALSE) {
                    MiReturnPageFileQuota (QuotaCharge, Process);
                    ExRaiseStatus (STATUS_COMMITMENT_LIMIT);
                }
            }

            if (MiChargeCommitment (QuotaCharge, Process) == FALSE) {
                if (Process->JobStatus & PS_JOB_STATUS_REPORT_COMMIT_CHANGES) {

                    //
                    // Temporarily up the process commit charge as the
                    // job code will be referencing it as though everything
                    // has succeeded.
                    //

                    Process->CommitCharge += QuotaCharge;
                    PsChangeJobMemoryUsage(-(SSIZE_T)QuotaCharge);
                    Process->CommitCharge -= QuotaCharge;
                }
                MiReturnPageFileQuota (QuotaCharge, Process);
                ExRaiseStatus STATUS_COMMITMENT_LIMIT;
            }

            //
            // Add the quota into the charge to the VAD.
            //

            MM_TRACK_COMMIT (MM_DBG_COMMIT_SET_PROTECTION, QuotaCharge);
            FoundVad->u.VadFlags.CommitCharge += QuotaCharge;
            Process->CommitCharge += QuotaCharge;
            if (Process->CommitCharge > Process->CommitChargePeak) {
                Process->CommitChargePeak = Process->CommitCharge;
            }
        }
    }

    //
    // For all the PTEs in the specified address range, set the
    // protection depending on the state of the PTE.
    //

    //
    // If the PTE was copy on write (but not written) and the
    // new protection is NOT copy-on-write, return page file quota
    // and commitment.
    //

    PointerPpe = MiGetPpeAddress (StartingAddress);
    PointerPde = MiGetPdeAddress (StartingAddress);
    PointerPte = MiGetPteAddress (StartingAddress);

    do {

        MiDoesPpeExistAndMakeValid (PointerPpe, Process, FALSE, &Waited);

        Waited = 0;

        MiDoesPdeExistAndMakeValid (PointerPde, Process, FALSE, &Waited);

    } while (Waited != 0);

    QuotaCharge = 0;

    while (PointerPte <= LastPte) {

        if (MiIsPteOnPdeBoundary (PointerPte)) {
            PointerPde = MiGetPteAddress (PointerPte);
            PointerPpe = MiGetPdeAddress (PointerPte);

#if defined (_WIN64)
            MiMakePpeExistAndMakeValid (PointerPpe, Process, FALSE);
            if (PointerPde->u.Long == 0) {
                UsedPageDirectoryHandle = MI_GET_USED_PTES_HANDLE (PointerPte);
                MI_INCREMENT_USED_PTES_BY_HANDLE (UsedPageDirectoryHandle);
            }
#endif

            MiMakePdeExistAndMakeValid (PointerPde, Process, FALSE);
        }

        PteContents = *PointerPte;

        if (PteContents.u.Long == 0) {

            //
            // The PTE is zero, set it into prototype PTE format
            // with the protection in the prototype PTE.
            //

            TempPte = PrototypePte;
            TempPte.u.Soft.Protection = ProtectionMask;
            MI_WRITE_INVALID_PTE (PointerPte, TempPte);

            //
            // Increment the count of non-zero page table entries
            // for this page table and the number of private pages
            // for the process.
            //

            UsedPageTableHandle = MI_GET_USED_PTES_HANDLE (MiGetVirtualAddressMappedByPte (PointerPte));

            MI_INCREMENT_USED_PTES_BY_HANDLE (UsedPageTableHandle);

        } else if (PteContents.u.Hard.Valid == 1) {

            //
            // Set the protection into both the PTE and the original PTE
            // in the PFN database for private pages only.
            //

            NewProtectionMask = ProtectionMask;

            Pfn1 = MI_PFN_ELEMENT (PteContents.u.Hard.PageFrameNumber);

            if ((NewProtect & PAGE_NOACCESS) ||
                (NewProtect & PAGE_GUARD)) {

                Locked = MiRemovePageFromWorkingSet (PointerPte,
                                                     Pfn1,
                                                     &Process->Vm );
                continue;

            } else {

                if (Pfn1->u3.e1.PrototypePte == 1) {

                    //
                    // The true protection may be in the WSLE, locate
                    // the WSLE.
                    //

                    Va = (PULONG)MiGetVirtualAddressMappedByPte (PointerPte);
                    Index = MiLocateWsle ((PVOID)Va, MmWorkingSetList,
                                            Pfn1->u1.WsIndex);

                    //
                    // Check to see if this is a prototype PTE.  This
                    // is done by comparing the PTE address in the
                    // PFN database to the PTE address indicated by the
                    // VAD.  If they are not equal, this is a prototype
                    // PTE.
                    //

                    if (Pfn1->PteAddress !=
                                  MiGetProtoPteAddress (FoundVad,
                                              MI_VA_TO_VPN ((PVOID)Va))) {

                        //
                        // This PTE refers to a fork prototype PTE, make it
                        // private.
                        //

                        MiCopyOnWrite ((PVOID)Va, PointerPte);

                        if (WriteCopy) {
                            QuotaCharge += 1;
                        }

                        //
                        // This may have released the working set mutex and
                        // the page table page may no longer be in memory.
                        //

                        PointerPpe = MiGetPteAddress (PointerPde);

                        do {

                            (VOID)MiDoesPpeExistAndMakeValid (PointerPpe,
                                                              Process,
                                                              FALSE,
                                                              &Waited);

                            Waited = 0;

                            (VOID)MiDoesPdeExistAndMakeValid (PointerPde,
                                                              Process,
                                                              FALSE,
                                                              &Waited);

                        } while (Waited != 0);

                        //
                        // Do the loop again.
                        //

                        continue;

                    } else {

                        //
                        // Update the protection field in the WSLE and
                        // the PTE.
                        //
                        //
                        // If the PTE is copy on write uncharge the
                        // previously charged quota.
                        //

                        if ((!WriteCopy) && (PteContents.u.Hard.CopyOnWrite == 1)) {
                            QuotaCharge += 1;
                        }

                        MmWsle[Index].u1.e1.Protection = ProtectionMask;
                        MmWsle[Index].u1.e1.SameProtectAsProto = 0;
                    }

                } else {

                    //
                    // Page is private (copy on written), protection mask
                    // is stored in the original pte field.
                    //

#if PFN_CONSISTENCY
                    MiSetOriginalPteProtection (Pfn1, ProtectionMaskNotCopy);
#else
                    Pfn1->OriginalPte.u.Soft.Protection = ProtectionMaskNotCopy;
#endif
                    NewProtectionMask = ProtectionMaskNotCopy;
                }

                MI_MAKE_VALID_PTE (TempPte,
                                   PteContents.u.Hard.PageFrameNumber,
                                   NewProtectionMask,
                                   PointerPte);

                WorkingSetIndex = MI_GET_WORKING_SET_FROM_PTE (&PteContents);

                MI_SET_PTE_IN_WORKING_SET (&TempPte, WorkingSetIndex);
            }

            //
            // Flush the TB as we have changed the protection
            // of a valid PTE.
            //

            PreviousPte.u.Flush = MiFlushTbAndCapture (PointerPte,
                                                       TempPte.u.Flush,
                                                       Pfn1);
        } else {

            if (PteContents.u.Soft.Prototype == 1) {

                //
                // The PTE is in prototype PTE format.
                //

                //
                // Is it a fork prototype PTE?
                //

                Va = (PULONG)MiGetVirtualAddressMappedByPte (PointerPte);

                if ((PteContents.u.Soft.PageFileHigh != MI_PTE_LOOKUP_NEEDED) &&
                   (MiPteToProto (PointerPte) !=
                                     MiGetProtoPteAddress (FoundVad,
                                         MI_VA_TO_VPN ((PVOID)Va)))) {

                    //
                    // This PTE refers to a fork prototype PTE, make the
                    // page private.  This is accomplished by releasing
                    // the working set mutex, reading the page thereby
                    // causing a fault, and re-executing the loop, hopefully,
                    // this time, we'll find the page present and will
                    // turn it into a private page.
                    //
                    // Note, that page with prototype = 1 cannot be
                    // no-access.
                    //

                    DoAgain = TRUE;

                    while (PteContents.u.Hard.Valid == 0) {

                        UNLOCK_WS_UNSAFE (Process);

                        try {

                            *(volatile ULONG *)Va;
                        } except (EXCEPTION_EXECUTE_HANDLER) {

                            if (GetExceptionCode() !=
                                                STATUS_GUARD_PAGE_VIOLATION) {

                                //
                                // The prototype PTE must be noaccess.
                                //

                                DoAgain = MiChangeNoAccessForkPte (PointerPte,
                                                                ProtectionMask);
                            }
                        }

                        PointerPpe = MiGetPteAddress (PointerPde);

                        LOCK_WS_UNSAFE (Process);

                        do {

                            (VOID)MiDoesPpeExistAndMakeValid (PointerPpe,
                                                              Process,
                                                              FALSE,
                                                              &Waited);

                            Waited = 0;

                            (VOID)MiDoesPdeExistAndMakeValid (PointerPde,
                                                              Process,
                                                              FALSE,
                                                              &Waited);

                        } while (Waited != 0);

                        PteContents = *(volatile MMPTE *)PointerPte;
                    }

                    if (DoAgain) {
                        continue;
                    }

                } else {

                    //
                    // If the new protection is not write-copy, the PTE
                    // protection is not in the prototype PTE (can't be
                    // write copy for sections), and the protection in
                    // the PTE is write-copy, release the page file
                    // quota and commitment for this page.
                    //

                    if ((!WriteCopy) &&
                        (PteContents.u.Soft.PageFileHigh == MI_PTE_LOOKUP_NEEDED)) {
                        if (MI_IS_PTE_PROTECTION_COPY_WRITE(PteContents.u.Soft.Protection)) {
                            QuotaCharge += 1;
                        }

                    }

                    //
                    // The PTE is a prototype PTE.  Make the high part
                    // of the PTE indicate that the protection field
                    // is in the PTE itself.
                    //

                    MI_WRITE_INVALID_PTE (PointerPte, PrototypePte);
                    PointerPte->u.Soft.Protection = ProtectionMask;
                }

            } else {

                if (PteContents.u.Soft.Transition == 1) {

                    //
                    // This is a transition PTE. (Page is private)
                    //

                    if (MiSetProtectionOnTransitionPte (
                                                PointerPte,
                                                ProtectionMaskNotCopy)) {
                        continue;
                    }

                } else {

                    //
                    // Must be page file space or demand zero.
                    //

                    PointerPte->u.Soft.Protection = ProtectionMaskNotCopy;
                }
            }
        }

        PointerPte += 1;
    }

    //
    // Return the quota charge and the commitment, if any.
    //

    if ((QuotaCharge > 0) && (!DontCharge)) {

        MiReturnCommitment (QuotaCharge);
        MM_TRACK_COMMIT (MM_DBG_COMMIT_RETURN_PROTECTION, QuotaCharge);
        MiReturnPageFileQuota (QuotaCharge, Process);

        ASSERT (QuotaCharge <= FoundVad->u.VadFlags.CommitCharge);

        FoundVad->u.VadFlags.CommitCharge -= QuotaCharge;
        if (Process->JobStatus & PS_JOB_STATUS_REPORT_COMMIT_CHANGES) {
            PsChangeJobMemoryUsage(-(SSIZE_T)QuotaCharge);
        }
        Process->CommitCharge -= QuotaCharge;
    }

    return Locked;
}

ULONG
MiGetPageProtection (
    IN PMMPTE PointerPte,
    IN PEPROCESS Process
    )

/*++

Routine Description:

    This routine returns the page protection of a non-zero PTE.
    It may release and reacquire the working set mutex.

Arguments:

    PointerPte - Supplies a pointer to a non-zero PTE.

Return Value:

    Returns the protection code.

Environment:

    Kernel mode, working set and address creation mutex held.
    Note, that the address creation mutex does not need to be held
    if the working set mutex does not need to be released in the
    case of a prototype PTE.

--*/

{

    MMPTE PteContents;
    MMPTE ProtoPteContents;
    PMMPFN Pfn1;
    PMMPTE ProtoPteAddress;
    PVOID Va;
    ULONG Index;

    PAGED_CODE();

    PteContents = *PointerPte;

    if ((PteContents.u.Soft.Valid == 0) && (PteContents.u.Soft.Prototype == 1)) {

        //
        // This pte is in prototype format, the protection is
        // stored in the prototype PTE.
        //

        if (MI_IS_PTE_PROTOTYPE(PointerPte) ||
#ifdef _X86_
            (MiNumberOfExtraSystemPdes && (PointerPte >= (PMMPTE)KSTACK_POOL_START && PointerPte < (PMMPTE)(KSTACK_POOL_START + MiNumberOfExtraSystemPdes * MM_VA_MAPPED_BY_PDE))) ||
#endif
            (PteContents.u.Soft.PageFileHigh == MI_PTE_LOOKUP_NEEDED)) {

            //
            // The protection is within this PTE.
            //

            return MI_CONVERT_FROM_PTE_PROTECTION (
                                            PteContents.u.Soft.Protection);
        }

        ProtoPteAddress = MiPteToProto (PointerPte);

        //
        // Capture protopte PTE contents.
        //

        ProtoPteContents = MiCaptureSystemPte (ProtoPteAddress, Process);

        //
        // The working set mutex may have been released and the
        // page may no longer be in prototype format, get the
        // new contents of the PTE and obtain the protection mask.
        //

        PteContents = MiCaptureSystemPte (PointerPte, Process);
    }

    if ((PteContents.u.Soft.Valid == 0) && (PteContents.u.Soft.Prototype == 1)) {

        //
        // Pte is still prototype, return the protection captured
        // from the prototype PTE.
        //

        if (ProtoPteContents.u.Hard.Valid == 1) {

            //
            // The prototype PTE is valid, get the protection from
            // the PFN database.
            //

            Pfn1 = MI_PFN_ELEMENT (ProtoPteContents.u.Hard.PageFrameNumber);
            return MI_CONVERT_FROM_PTE_PROTECTION(
                                      Pfn1->OriginalPte.u.Soft.Protection);

        } else {

            //
            // The prototype PTE is not valid, return the protection from the
            // PTE.
            //

            return MI_CONVERT_FROM_PTE_PROTECTION (
                                     ProtoPteContents.u.Soft.Protection);
        }
    }

    if (PteContents.u.Hard.Valid == 1) {

        //
        // The page is valid, the protection field is either in the
        // PFN database original PTE element or the WSLE.  If
        // the page is private, get it from the PFN original PTE
        // element, else use the WSLE.
        //

        Pfn1 = MI_PFN_ELEMENT (PteContents.u.Hard.PageFrameNumber);

        if ((Pfn1->u3.e1.PrototypePte == 0) ||
#ifdef _X86_
            (MiNumberOfExtraSystemPdes && (PointerPte >= (PMMPTE)KSTACK_POOL_START && PointerPte < (PMMPTE)(KSTACK_POOL_START + MiNumberOfExtraSystemPdes * MM_VA_MAPPED_BY_PDE))) ||
#endif
            (MI_IS_PTE_PROTOTYPE(PointerPte))) {

            //
            // This is a private PTE or the PTE address is that of a
            // prototype PTE, hence the protection is in
            // the original PTE.
            //

            return MI_CONVERT_FROM_PTE_PROTECTION(
                                      Pfn1->OriginalPte.u.Soft.Protection);
        }

        //
        // The PTE was a hardware PTE, get the protection
        // from the WSLE.

        Va = (PULONG)MiGetVirtualAddressMappedByPte (PointerPte);
        Index = MiLocateWsle ((PVOID)Va, MmWorkingSetList,
                                            Pfn1->u1.WsIndex);

        return MI_CONVERT_FROM_PTE_PROTECTION (MmWsle[Index].u1.e1.Protection);
    }

    //
    // PTE is either demand zero or transition, in either
    // case protection is in PTE.
    //

    return MI_CONVERT_FROM_PTE_PROTECTION (PteContents.u.Soft.Protection);

}

ULONG
MiChangeNoAccessForkPte (
    IN PMMPTE PointerPte,
    IN ULONG ProtectionMask
    )

/*++

Routine Description:


Arguments:

    PointerPte - Supplies a pointer to the current PTE.

    ProtectionMask - Supplies the protection mask to set.

Return Value:

    FALSE if the loop should be repeated for this PTE, TRUE
    if protection has been set.


Environment:

    Kernel mode, address creation mutex held, APCs disabled.

--*/

{
    PAGED_CODE();

    if (ProtectionMask == MM_NOACCESS) {

        //
        // No need to change the page protection.
        //

        return TRUE;
    }

    PointerPte->u.Proto.ReadOnly = 1;

    return FALSE;
}


HARDWARE_PTE
MiFlushTbAndCapture(
    IN PMMPTE PointerPte,
    IN HARDWARE_PTE TempPte,
    IN PMMPFN Pfn1
    )

// non pagable helper routine.

{
    MMPTE PreviousPte;
    KIRQL OldIrql;
    PEPROCESS Process;
    PVOID VirtualAddress;

    //
    // Flush the TB as we have changed the protection
    // of a valid PTE.
    //

    LOCK_PFN (OldIrql);

    PreviousPte.u.Flush = KeFlushSingleTb (
                            MiGetVirtualAddressMappedByPte (PointerPte),
                            FALSE,
                            FALSE,
                            (PHARDWARE_PTE)PointerPte,
                            TempPte);

    ASSERT (PreviousPte.u.Hard.Valid == 1);

    //
    // A page protection is being changed, on certain
    // hardware the dirty bit should be ORed into the
    // modify bit in the PFN element.
    //

    MI_CAPTURE_DIRTY_BIT_TO_PFN (&PreviousPte, Pfn1);

    //
    // If the PTE indicates the page has been modified (this is different
    // from the PFN indicating this), then ripple it back to the write watch
    // bitmap now since we are still in the correct process context.
    //

    if (MiActiveWriteWatch != 0) {
        if ((Pfn1->u3.e1.PrototypePte == 0) &&
            (MI_IS_PTE_DIRTY(PreviousPte))) {

            Process = PsGetCurrentProcess();

            if (Process->Vm.u.Flags.WriteWatch == 1) {

                //
                // This process has (or had) write watch VADs.  Search now
                // for a write watch region encapsulating the PTE being
                // invalidated.
                //

                VirtualAddress = MiGetVirtualAddressMappedByPte (PointerPte);
                MiCaptureWriteWatchDirtyBit (Process, VirtualAddress);
            }
        }
    }
#if DBG
    else {
        Process = PsGetCurrentProcess();
        ASSERT (Process->Vm.u.Flags.WriteWatch == 0);
    }
#endif

    UNLOCK_PFN (OldIrql);
    return PreviousPte.u.Flush;
}

ULONG
MiSetProtectionOnTransitionPte (
    IN PMMPTE PointerPte,
    IN ULONG ProtectionMask
    )

    // nonpaged helper routine.

{
    KIRQL OldIrql;
    MMPTE PteContents;
    PMMPFN Pfn1;

    //
    // This is a transition PTE. (Page is private)
    //

    //
    // Need pfn mutex to ensure page doesn't become
    // non-transition.
    //

    LOCK_PFN (OldIrql);

    //
    // Make sure the page is still a transition page.
    //

    PteContents = *(volatile MMPTE *)PointerPte;

    if ((PteContents.u.Soft.Prototype == 0) &&
                     (PointerPte->u.Soft.Transition == 1)) {

        Pfn1 = MI_PFN_ELEMENT (
                      PteContents.u.Trans.PageFrameNumber);

        Pfn1->OriginalPte.u.Soft.Protection = ProtectionMask;
        PointerPte->u.Soft.Protection = ProtectionMask;
        UNLOCK_PFN (OldIrql);
        return FALSE;
    }

    //
    // Do this loop again for the same PTE.
    //

    UNLOCK_PFN (OldIrql);
    return TRUE;
}

MMPTE
MiCaptureSystemPte (
    IN PMMPTE PointerProtoPte,
    IN PEPROCESS Process
    )

/*++

Routine Description:

    Nonpagable helper routine to capture the contents of a pagable PTE.

Arguments:

    PointerProtoPte - Supplies a pointer to the prototype PTE.

    Process - Supplies the relevant process.

Return Value:

    PTE contents.

Environment:

    Kernel mode.  Caller holds address space and working set mutexes if
    Process is set.  Working set mutex was acquired unsafe.

--*/

{
    MMPTE TempPte;
    KIRQL OldIrql;

    LOCK_PFN (OldIrql);
    MiMakeSystemAddressValidPfnWs (PointerProtoPte, Process);
    TempPte = *PointerProtoPte;
    UNLOCK_PFN (OldIrql);
    return TempPte;
}

NTSTATUS
MiCheckSecuredVad (
    IN PMMVAD Vad,
    IN PVOID Base,
    IN SIZE_T Size,
    IN ULONG ProtectionMask
    )

/*++

Routine Description:

    This routine checks to see if the specified VAD is secured in such
    a way as to conflict with the address range and protection mask
    specified.

Arguments:

    Vad - Supplies a pointer to the VAD containing the address range.

    Base - Supplies the base of the range the protection starts at.

    Size - Supplies the size of the range.

    ProtectionMask - Supplies the protection mask being set.

Return Value:

    Status value.

Environment:

    Kernel mode.

--*/

{
    PVOID End;
    PLIST_ENTRY Next;
    PMMSECURE_ENTRY Entry;
    NTSTATUS Status = STATUS_SUCCESS;

    End = (PVOID)((PCHAR)Base + Size);

    if (ProtectionMask < MM_SECURE_DELETE_CHECK) {
        if ((Vad->u.VadFlags.NoChange == 1) &&
            (Vad->u2.VadFlags2.SecNoChange == 1) &&
            (Vad->u.VadFlags.Protection != ProtectionMask)) {

            //
            // An attempt is made at changing the protection
            // of a SEC_NO_CHANGE section - return an error.
            //

            Status = STATUS_INVALID_PAGE_PROTECTION;
            goto done;
        }
    } else {

        //
        // Deletion - set to no-access for check.  SEC_NOCHANGE allows
        // deletion, but does not allow page protection changes.
        //

        ProtectionMask = 0;
    }

    if (Vad->u2.VadFlags2.OneSecured) {

        if (((ULONG_PTR)Base <= Vad->u3.Secured.EndVpn) &&
             ((ULONG_PTR)End >= Vad->u3.Secured.StartVpn)) {

            //
            // This region conflicts, check the protections.
            //

            if (ProtectionMask & MM_GUARD_PAGE) {
                Status = STATUS_INVALID_PAGE_PROTECTION;
                goto done;
            }

            if (Vad->u2.VadFlags2.ReadOnly) {
                if (MmReadWrite[ProtectionMask] < 10) {
                    Status = STATUS_INVALID_PAGE_PROTECTION;
                    goto done;
                }
            } else {
                if (MmReadWrite[ProtectionMask] < 11) {
                    Status = STATUS_INVALID_PAGE_PROTECTION;
                    goto done;
                }
            }
        }

    } else if (Vad->u2.VadFlags2.MultipleSecured) {

        Next = Vad->u3.List.Flink;
        do {
            Entry = CONTAINING_RECORD( Next,
                                       MMSECURE_ENTRY,
                                       List);

            if (((ULONG_PTR)Base <= Entry->EndVpn) &&
                ((ULONG_PTR)End >= Entry->StartVpn)) {

                //
                // This region conflicts, check the protections.
                //

                if (ProtectionMask & MM_GUARD_PAGE) {
                    Status = STATUS_INVALID_PAGE_PROTECTION;
                    goto done;
                }
    
                if (Entry->u2.VadFlags2.ReadOnly) {
                    if (MmReadWrite[ProtectionMask] < 10) {
                        Status = STATUS_INVALID_PAGE_PROTECTION;
                        goto done;
                    }
                } else {
                    if (MmReadWrite[ProtectionMask] < 11) {
                        Status = STATUS_INVALID_PAGE_PROTECTION;
                        goto done;
                    }
                }
            }
            Next = Entry->List.Flink;
        } while (Entry->List.Flink != &Vad->u3.List);
    }

done:
    return Status;
}

#ifdef PFN_CONSISTENCY

BOOLEAN MiCheckPfnState = FALSE;

VOID
MiMapInPfnDatabase (
    VOID
    )

/*++

Routine Description:

    This routine allows PFN database access (removes write-protection).

Arguments:

    None.

Return Value:

    None.

Environment:

    Kernel mode.  The PFN database must be locked at DISPATCH level on entry.

--*/

{
    PFN_NUMBER i;
    PMMPTE PointerPte;

    MM_PFN_LOCK_ASSERT();

    ASSERT (MiPfnStartPte);

    PointerPte = MiPfnStartPte;

    for (i = 0; i < MiPfnPtes; i += 1) {

        //
        // Account for a sparse database
        //

        if (PointerPte->u.Hard.Valid == 1) {
#if defined (_ALPHA_)
            if (MiCheckPfnState == TRUE) {
                ASSERT (PointerPte->u.Hard.Dirty == 0);
            }
            PointerPte->u.Hard.Dirty = 1;
#elif defined (NT_UP)
            if (MiCheckPfnState == TRUE) {
                ASSERT (PointerPte->u.Hard.Write == 0);
            }
            PointerPte->u.Hard.Write = 1;
#else
            if (MiCheckPfnState == TRUE) {
                ASSERT (PointerPte->u.Hard.Writable == 0);
            }
            PointerPte->u.Hard.Writable = 1;
#endif
        }
        PointerPte += 1;
    }
    KeFlushEntireTb (TRUE, TRUE);
}


VOID
MiUnMapPfnDatabase (
    VOID
    )

/*++

Routine Description:

    This routine write protects the PFN database to catch errant pointers.

Arguments:

    None.

Return Value:

    None.

Environment:

    Kernel mode.  The PFN database must be locked at DISPATCH level on entry.

--*/

{
    PFN_NUMBER i;
    PMMPTE PointerPte;

    MM_PFN_LOCK_ASSERT();

    ASSERT (MiPfnStartPte);

    if (MiPfnProtectionEnabled == FALSE) {
        //
        // We're bugchecking, so don't unmap this anymore.
        //
        return;
    }

    PointerPte = MiPfnStartPte;

    for (i = 0; i < MiPfnPtes; i += 1) {

        //
        // Account for a sparse database
        //

        if (PointerPte->u.Hard.Valid == 1) {
#if defined (_ALPHA_)
            if (MiCheckPfnState == TRUE) {
                ASSERT (PointerPte->u.Hard.Dirty == 1);
            }
            PointerPte->u.Hard.Dirty = 0;
#elif defined (NT_UP)
            if (MiCheckPfnState == TRUE) {
                ASSERT (PointerPte->u.Hard.Write == 1);
            }
            PointerPte->u.Hard.Write = 0;
#else
            if (MiCheckPfnState == TRUE) {
                ASSERT (PointerPte->u.Hard.Writable == 1);
            }
            PointerPte->u.Hard.Writable = 0;
#endif
            PointerPte += 1;
        }
    }
    KeFlushEntireTb (TRUE, TRUE);
}

VOID
MiSetOriginalPteProtection (
    IN PMMPFN Pfn1,
    IN ULONG ProtectionMask
    )

/*++

Routine Description:

    This routine updates the OriginalPte's protection in the
    specified PFN database entry.

Arguments:

    Pfn1 - Supplies a pointer to the PFN database entry.

    OriginalPteProtection - Supplies the requested original PTE protection.

Return Value:

    None.

Environment:

    Kernel mode.

--*/

{
    KIRQL OldIrql;

    LOCK_PFN (OldIrql);
    Pfn1->OriginalPte.u.Soft.Protection = ProtectionMask;
    UNLOCK_PFN (OldIrql);
}

VOID
MiSetModified (
    IN PMMPFN Pfn1,
    IN ULONG Dirty
    )

/*++

Routine Description:

    This routine marks the specified PFN database entry as modified or clean.

Arguments:

    Pfn1 - Supplies a pointer to the PFN database entry.

    Dirty - Supplies TRUE if the PFN should be marked modified, FALSE if clean.

Return Value:

    None.

Environment:

    Kernel mode.

--*/

{
    KIRQL OldIrql;

    LOCK_PFN (OldIrql);
    Pfn1->u3.e1.Modified = Dirty;
    UNLOCK_PFN (OldIrql);
}

#endif // PFN_CONSISTENCY
