/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

   VLM.C

Abstract:

    This module contains the routines which implement the 64-bit Very Large
    Memory support.

Author:

    Lou Perazzoli (loup) 3-Sep-1996

Revision History:

--*/

#include "mi.h"

extern ULONG MMVADKEY;

#ifdef VLM_SUPPORT

#define ROUND_TO_PAGES64(Size)  ((ULONGLONG)(Size) + (PAGE_SIZE - 1) & ~((LONGLONG)PAGE_SIZE - 1))

#define MM_VALID_PTE_SIZE (256)

#define MM_LARGEST_VLM_RANGE \
   ((ULONGLONG)MM_HIGHEST_USER_ADDRESS64 - (ULONGLONG)MM_LOWEST_USER_ADDRESS64)

#define DELETE_TYPE_PRIVATE 0
#define DELETE_TYPE_SHARED 1

MMPTE
MiCaptureSystemPte (
    IN PMMPTE PointerProtoPte,
    IN PEPROCESS Process
    );

LOGICAL
MiCommitPages64 (
    IN PMMPTE StartPte,
    IN PMMPTE LastPte,
    IN ULONG ProtectionMask,
    IN PEPROCESS Process,
    OUT PSIZE_T ValidPtes
    );

ULONG
MiDecommitOrDeletePages64 (
    IN PMMPTE StartingPte,
    IN PMMPTE EndingPte,
    IN PEPROCESS Process,
    IN ULONG Type,
    IN LOGICAL FlushTb
    );

VOID
MiFlushPteList64 (
    IN PMMPTE_FLUSH_LIST64 PteFlushList,
    IN ULONG AllProcessors,
    IN MMPTE FillPte
    );

VOID
MiReturnAvailablePages (
    IN PFN_NUMBER Amount
    );

LOGICAL
MiCheckPdeForDeletion (
    IN PMMPTE PointerPde,
    IN LOGICAL FlushTb
    );

VOID
MiProcessValidPteList64 (
    IN PMMPTE *ValidPteList,
    IN ULONG Count,
    IN LOGICAL FlushTb
    );

ULONG
MiDoesPdeExist64 (
    IN PMMPTE PointerPde
    );

LOGICAL
MiMakePdeExistAndMakeValid64 (
    IN PMMPTE PointerPde,
    IN PEPROCESS TargetProcess,
    IN LOGICAL PfnLockHeld,
    IN LOGICAL MakePageTablePage
    );

HARDWARE_PTE
MiFlushTbAndCapture(
    IN PMMPTE PtePointer,
    IN HARDWARE_PTE TempPte,
    IN PMMPFN Pfn1
    );

NTSTATUS
MiMapViewOfVlmDataSection (
    IN PCONTROL_AREA ControlArea,
    IN PEPROCESS Process,
    IN PVOID64 *CapturedBase,
    IN PULONGLONG SectionOffset,
    IN PULONGLONG CapturedViewSize,
    IN PSECTION Section,
    IN ULONG ProtectionMask,
    IN ULONG AllocationType,
    IN PULONG ReleasedWsMutex
    );

ULONG
MiIsEntireRangeCommitted64 (
    IN PVOID64 StartingAddress,
    IN PVOID64 EndingAddress,
    IN PMMVAD Vad,
    IN PEPROCESS Process
    );

VOID
MiMakeValidPageNoAccess64 (
    IN PMMPTE PointePte
    );

VOID
MiMakeNoAccessPageValid64 (
    IN PMMPTE PointerPte,
    IN ULONG Protect
    );

ULONG
MiSetProtectionOnTransitionPte (
    IN PMMPTE PointerPte,
    IN ULONG ProtectionMask
    );

ULONG
MiQueryAddressState64 (
    IN PVOID64 Va,
    IN PMMVAD Vad,
    IN PEPROCESS TargetProcess,
    OUT PULONG ReturnedProtect
    );

#define MI_NONPAGABLE_MEMORY_AVAILABLE(_SizeInPages) \
    (MmResidentAvailablePages > ((SPFN_NUMBER)_SizeInPages + (SPFN_NUMBER)MmTotalFreeSystemPtes[NonPagedPoolExpansion] + ((SPFN_NUMBER)MiSpecialPagesNonPagedMaximum - (SPFN_NUMBER)MiSpecialPagesNonPaged) + (SPFN_NUMBER)MM_VLM_FLUID_PAGES))

#define MI_GET_USED_PDE64_HANDLE(PDE64) \
        (&(((PMI_PROCESS_VLM_INFO) (((PMMPTE)PDE_BASE64)->u.Long))->UsedPageTableEntries[PDE64 - MiGetPdeAddress64 (MM_LOWEST_USER_ADDRESS64)]))

ULONG MmSharedCommitVlm;            // Protected by the PFN lock.

KSPIN_LOCK MiVlmStatisticsLock;     // Protects both fields below.

ULONG MiVlmCommitChargeInPages;
ULONG MiVlmCommitChargeInPagesPeak;


NTSTATUS
NtAllocateVirtualMemory64 (
    IN HANDLE ProcessHandle,
    IN OUT PVOID64 *BaseAddress,
    IN ULONG ZeroBits,
    IN OUT PULONGLONG RegionSize,
    IN ULONG AllocationType,
    IN ULONG Protect
    )

/*++

Routine Description:

    This function creates a region of pages within the virtual address
    space of a subject process.

Arguments:

    ProcessHandle - Supplies an open handle to a process object.

    BaseAddress - Supplies a pointer to a variable that will receive
         the base address of the allocated region of pages.
         If the initial value of this argument is not null,
         then the region will be allocated starting at the
         specified virtual address rounded down to the next
         host page size address boundary. If the initial
         value of this argument is null, then the operating
         system will determine where to allocate the
         region.

    ZeroBits - IGNORED

    RegionSize - Supplies a pointer to a variable that will receive
         the actual size in bytes of the allocated region
         of pages. The initial value of this argument
         specifies the size in bytes of the region and is
         rounded up to the next host page size boundary.

    AllocationType - Supplies a set of flags that describe the type
         of allocation that is to be performed for the
         specified region of pages. Flags are:


         MEM_COMMIT - The specified region of pages is to
              be committed.

         MEM_RESERVE - The specified region of pages is to
              be reserved.

         MEM_TOP_DOWN - NOT ALLOWED.

         MEM_RESET - IGNORED.

    Protect - Supplies the protection desired for the committed
         region of pages.

        Protect Values:


         PAGE_NOACCESS - No access to the committed region
              of pages is allowed. An attempt to read,
              write, or execute the committed region
              results in an access violation (i.e. a GP
              fault).

         PAGE_EXECUTE - Not allowed

         PAGE_READONLY - Read only and execute access to the
              committed region of pages is allowed. An
              attempt to write the committed region results
              in an access violation.

         PAGE_READWRITE - Read, write, and execute access to
              the committed region of pages is allowed. If
              write access to the underlying section is
              allowed, then a single copy of the pages are
              shared. Otherwise the pages are shared read
              only/copy on write.

         PAGE_NOCACHE - not allowed.

Return Value:

    Returns the status

    TBS


--*/

{
    PMMVAD Vad;
    PMMVAD FoundVad;
    PEPROCESS Process;
    KPROCESSOR_MODE PreviousMode;
    PVOID64 StartingAddress;
    PVOID64 EndingAddress;
    NTSTATUS Status;
    PVOID64 CapturedBase;
    ULONGLONG CapturedRegionSize;
    PMMPTE CommitLimitPte;
    ULONG ProtectionMask;
    PMMPTE LastPte;
    PMMPTE PointerPde;
    PMMPTE StartingPte;
    PMMPTE PointerPte;
    MMPTE TempPte;
    ULONG OldProtect;
    SSIZE_T QuotaCharge;
    SIZE_T QuotaFree;
    ULONG CopyOnWriteCharge;
    BOOLEAN PageFileChargeSucceeded;
    LOGICAL Attached;
    KIRQL OldIrql;
    ULONG_PTR NewPages;
    LOGICAL ChargedExactQuota;
    LOGICAL CommitSucceeded;
    LOGICAL LargeVad;
    LOGICAL GotResidentAvail;
    PMI_PROCESS_VLM_INFO VlmInfo;
    ULONG CommitCharge;

    Attached = FALSE;

    if (MI_VLM_ENABLED() == FALSE) {
        return STATUS_NOT_IMPLEMENTED;
    }

    //
    // Check the AllocationType for correctness.
    //

    if ((AllocationType & ~(MEM_COMMIT | MEM_RESERVE |
                            MEM_RESET)) != 0) {
        return STATUS_INVALID_PARAMETER_5;
    }

    //
    // One of MEM_COMMIT, MEM_RESET or MEM_RESERVE must be set.
    //

    if ((AllocationType & (MEM_COMMIT | MEM_RESERVE | MEM_RESET)) == 0) {
        return STATUS_INVALID_PARAMETER_5;
    }

    if ((AllocationType & MEM_RESET) && (AllocationType != MEM_RESET)) {

        //
        // MEM_RESET may not be used with any other flag.
        //

        return STATUS_INVALID_PARAMETER_5;
    }

    //
    // Check the protection field.  This could raise an exception.
    //

    try {
        ProtectionMask = MiMakeProtectionMask (Protect);
    } except (EXCEPTION_EXECUTE_HANDLER) {
        return GetExceptionCode();
    }

    if ((ProtectionMask == MM_NOACCESS) &&
        (AllocationType & MEM_COMMIT)) {

        return STATUS_INVALID_PAGE_PROTECTION;
    }

    PreviousMode = KeGetPreviousMode();

    //
    // Establish an exception handler, probe the specified addresses
    // for write access and capture the initial values.
    //

    try {

        if (PreviousMode != KernelMode) {

            ProbeForWrite (BaseAddress, sizeof(PVOID64), sizeof(PVOID64));
            ProbeForWrite (RegionSize, sizeof(ULONGLONG), sizeof(ULONGLONG));
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

#if 0
    if (MmDebug & MM_DBG_SHOW_NT_CALLS) {
        if ( MmWatchProcess ) {
            ;
        } else {
            DbgPrint("allocvm process handle %lx base address %p zero bits %lx\n",
                ProcessHandle, CapturedBase, ZeroBits);
            DbgPrint("    region size %p alloc type %lx protect %lx\n",
                CapturedRegionSize, AllocationType, Protect);
        }
    }
#endif

    //
    // Make sure the specified starting and ending addresses are
    // within the user part of the virtual address space.
    //

    if ((CapturedBase != NULL ) &&
        ((CapturedBase > (PVOID64)MM_HIGHEST_USER_ADDRESS64) ||
        (CapturedBase < (PVOID64)MM_LOWEST_USER_ADDRESS64))) {

        //
        // Invalid base address.
        //

        return STATUS_INVALID_PARAMETER_2;
    }

    if (CapturedBase != NULL) {

        if ((ULONGLONG)CapturedBase + CapturedRegionSize < (ULONGLONG)CapturedBase) {

            //
            // the requested region wraps
            //

            return STATUS_INVALID_PARAMETER_4;
        }


        if ((ULONGLONG)CapturedBase + CapturedRegionSize > (ULONGLONG)MM_HIGHEST_USER_ADDRESS64 - X64K) {

            //
            // the requested region goes beyond the end of user memory - flag it
            //

            return STATUS_INVALID_PARAMETER_4;
        }
    }
    else if (CapturedRegionSize > (ULONGLONG)MM_LARGEST_VLM_RANGE - X64K) {

        //
        // Invalid region size;
        //

        return STATUS_INVALID_PARAMETER_4;
    }

    if (CapturedRegionSize == 0) {

        //
        // Region size cannot be 0.
        //

        return STATUS_INVALID_PARAMETER_4;
    }

    //
    // Reference the specified process handle for VM_OPERATION access.
    //

    if ( ProcessHandle == NtCurrentProcess() ) {
        Process = PsGetCurrentProcess();
    } else {
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

    //
    // Don't allow POSIX apps to do VLM operations.
    //

    if ((Process->CloneRoot != NULL) &&
         (Process->ForkWasSuccessful != MM_NO_FORK_ALLOWED)) {
        Status = STATUS_INVALID_VLM_OPERATION;
        goto ErrorReturn;
    }
    Process->ForkWasSuccessful = MM_NO_FORK_ALLOWED;

    if ((CapturedBase == NULL) || (AllocationType & MEM_RESERVE)) {

        //
        // PAGE_WRITECOPY is not valid for private pages.
        //

        if ((Protect & PAGE_WRITECOPY) ||
            (Protect & PAGE_EXECUTE_WRITECOPY)) {
            Status = STATUS_INVALID_PAGE_PROTECTION;
            goto ErrorReturn;
        }

        //
        // Reserve the address space.
        //

        if (CapturedBase == NULL) {

            //
            // No base address was specified.  This MUST be a reserve or
            // reserve and commit.
            //

            CapturedRegionSize = ROUND_TO_PAGES64 (CapturedRegionSize);

            //
            // Establish exception handler as MiFindEmptyAddressRange
            // will raise an exception if it fails.
            //

            try {


                StartingAddress = MiFindEmptyAddressRangeInTree64 (
                                   CapturedRegionSize,
                                   X64K,
                                   (PMMADDRESS_NODE)(Process->CloneRoot));

            } except (EXCEPTION_EXECUTE_HANDLER) {
                Status = GetExceptionCode();
                goto ErrorReturn;
            }

            //
            // Calculate the ending address based on the top address.
            //

            EndingAddress = (PVOID64)(((ULONGLONG)StartingAddress +
                                  CapturedRegionSize - 1L) | (PAGE_SIZE - 1L));

        } else {

            //
            // A non-NULL base address was specified. Check to make sure
            // the specified base address to ending address is currently
            // unused.
            //

            EndingAddress = (PVOID64)(((ULONGLONG)CapturedBase +
                                  CapturedRegionSize - 1L) | (PAGE_SIZE - 1L));

            //
            // Align the starting address on a 64k boundary.
            //

            StartingAddress = MI_64K_ALIGN64(CapturedBase);

            //
            // See if a VAD overlaps with this starting/ending address pair.
            //

            if (MiCheckForConflictingVad64 (StartingAddress, EndingAddress) !=
                    (PMMVAD)NULL) {

                Status = STATUS_CONFLICTING_ADDRESSES;
                goto ErrorReturn;
            }
        }

        if (AllocationType & MEM_COMMIT) {
            StartingPte = MiGetPteAddress64 (StartingAddress);
            ASSERT (MiGetPdeAddress64(StartingAddress) == MiGetPteAddress (StartingPte));
            LastPte = MiGetPteAddress64 (EndingAddress);
            QuotaCharge = 1 + LastPte - StartingPte;
        }
        else {
            QuotaCharge = 0;
        }

        //
        // An unoccupied address range has been found, build the virtual
        // address descriptor to describe this range.
        //

        //
        // Establish an exception handler and attempt to allocate
        // the pool and charge quota.  Note that the InsertVad routine
        // will also charge quota which could raise an exception.
        //

        try  {

            if ((AllocationType & MEM_COMMIT) && (QuotaCharge + QuotaCharge / PTE_PER_PAGE + 2 >= MM_MAX_COMMIT)) {
                LargeVad = TRUE;
                Vad = (PMMVAD)ExAllocatePoolWithTag (NonPagedPool,
                                                     sizeof(MMVAD),
                                                     MMVADKEY);
            }
            else {
                LargeVad = FALSE;
                Vad = (PMMVAD)ExAllocatePoolWithTag (NonPagedPool,
                                                     sizeof(MMVAD_SHORT),
                                                     'SdaV');
            }

            //
            // Faults are not allowed here since we are already holding the WS
            // lock.  This means we must check pool allocations explicitly.
            //

            if (Vad == (PMMVAD)0) {
                ExRaiseStatus (STATUS_INSUFFICIENT_RESOURCES);
            }

            Vad->StartingVpn = MI_VA_TO_VPN64 (StartingAddress);
            Vad->EndingVpn = MI_VA_TO_VPN64 (EndingAddress);

            Vad->u.LongFlags = 0;
            if (AllocationType & MEM_COMMIT) {
                Vad->u.VadFlags.MemCommit = 1;

                if (LargeVad == TRUE) {
                    Vad->u.VadFlags.ImageMap = 1;       // Signify large commit
                    Vad->u.VadFlags.CommitCharge = 0;
                    Vad->u3.List.Flink = (PLIST_ENTRY)QuotaCharge;
                }
                else {
                    Vad->u.VadFlags.CommitCharge = QuotaCharge;
                }
            }

            Vad->u.VadFlags.Protection = ProtectionMask;
            Vad->u.VadFlags.PrivateMemory = 1;

            MiInsertVad64 ((PVOID)Vad);

        } except (EXCEPTION_EXECUTE_HANDLER) {

            if (Vad != (PMMVAD)NULL) {

                //
                // The pool allocation succeeded, but the quota charge
                // in InsertVad failed, deallocate the pool and return
                // an error.
                //

                ExFreePool (Vad);
                Status = GetExceptionCode();
            } else {
                Status = STATUS_INSUFFICIENT_RESOURCES;
            }

            goto ErrorReturn;
        }

        if (AllocationType & MEM_COMMIT) {

            //
            // Check to ensure these pages can be committed.
            // Charge quota and commitment for the range.  Establish an
            // exception handler as this could raise an exception.
            //

            QuotaFree = 1;
            Status = STATUS_SUCCESS;

            LOCK_PFN (OldIrql);

            if (MI_NONPAGABLE_MEMORY_AVAILABLE (QuotaCharge)) {
                MmResidentAvailablePages -= (PFN_NUMBER)QuotaCharge;
                MM_BUMP_COUNTER(22, QuotaCharge);
                QuotaFree = 0;
            }
            UNLOCK_PFN (OldIrql);
            if (QuotaFree == 1) {
                MiRemoveVad64 ((PMMVAD)Vad);
                ExFreePool (Vad);
                Status = STATUS_COMMITMENT_LIMIT;
                goto ErrorReturn;
            }

            CommitSucceeded = MiCommitPages64 (StartingPte,
                                               LastPte,
                                               ProtectionMask,
                                               Process,
                                               &QuotaFree);

            if (CommitSucceeded == FALSE) {

                //
                // Nothing has been committed so just adjust counts and bail.
                //

                LOCK_PFN (OldIrql);
                MmResidentAvailablePages += (PFN_NUMBER) QuotaCharge;
                MM_BUMP_COUNTER(22, -QuotaCharge);
                UNLOCK_PFN (OldIrql);
                MiRemoveVad64 ((PMMVAD)Vad);
                ExFreePool (Vad);
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto ErrorReturn;
            }

            if (QuotaFree != 0) {
                LOCK_PFN (OldIrql);
                MmResidentAvailablePages += (PFN_NUMBER) QuotaFree;
                MM_BUMP_COUNTER(23, QuotaFree);
                UNLOCK_PFN (OldIrql);
            }

            VlmInfo = (PMI_PROCESS_VLM_INFO) (((PMMPTE)PDE_BASE64)->u.Long);
            VlmInfo->CommitCharge += (QuotaCharge - QuotaFree);
            if (VlmInfo->CommitCharge > VlmInfo->CommitChargePeak) {
                VlmInfo->CommitChargePeak = VlmInfo->CommitCharge;
            }
        }
        else {

            //
            // The top level PDE map page is allocated here (it it is not
            // already) even though this is only a reserve.  Because data like
            // the virtual size is saved in a pool allocation whose pointer is
            // in the top level PDE map page.
            //

            CommitSucceeded = MiMakePdeExistAndMakeValid64 (
                                        MiGetPdeAddress64 (StartingAddress),
                                        Process,
                                        FALSE,
                                        FALSE);

            if (CommitSucceeded == FALSE) {
                MiRemoveVad64 ((PMMVAD)Vad);
                ExFreePool (Vad);
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto ErrorReturn;
            }
            QuotaCharge = 0;
        }

        //
        // Unlock the working set lock, page faults can now be taken.
        //

        UNLOCK_WS (Process);

        //
        // Update the current virtual size in the process header, the
        // address space lock protects this operation.
        //

        CapturedRegionSize = (ULONGLONG)EndingAddress - (ULONGLONG)StartingAddress + 1;

        VlmInfo = (PMI_PROCESS_VLM_INFO) (((PMMPTE)PDE_BASE64)->u.Long);
        VlmInfo->VirtualSize += CapturedRegionSize;
        if (VlmInfo->VirtualSize > VlmInfo->VirtualSizePeak) {
            VlmInfo->VirtualSizePeak = VlmInfo->VirtualSize;
        }

        ExAcquireFastLock (&MiVlmStatisticsLock, &OldIrql);

        if (AllocationType & MEM_COMMIT) {
            MiVlmCommitChargeInPages += (QuotaCharge - QuotaFree);
            if (MiVlmCommitChargeInPages > MiVlmCommitChargeInPagesPeak) {
                MiVlmCommitChargeInPagesPeak = MiVlmCommitChargeInPages;
            }
        }

        ExReleaseFastLock (&MiVlmStatisticsLock, OldIrql);

        //
        // Release the address space lock, lower IRQL, detach, and dereference
        // the process object.
        //

        UNLOCK_ADDRESS_SPACE(Process);
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
            // Return success at this point even if the results
            // cannot be written.
            //

            NOTHING;
        }

#if 0
        if (MmDebug & MM_DBG_SHOW_NT_CALLS) {
            if ( MmWatchProcess ) {
                if ( MmWatchProcess == PsGetCurrentProcess() ) {
                    DbgPrint("\n+++ ALLOC Type %lx Base %p Size %p\n",
                        AllocationType,StartingAddress, CapturedRegionSize);
                    MmFooBar();
                }
            } else {
                DbgPrint("return allocvm status %lx baseaddr %p size %p\n",
                    Status, StartingAddress, CapturedRegionSize);
            }
        }
#endif

        return STATUS_SUCCESS;

    } else {

        //
        // Commit previously reserved pages.  Note that these pages could
        // be either private or a section.
        //

        if (AllocationType == MEM_RESET) {

            //
            // Don't do anything for the mem reset case as the pages are
            // all valid.
            //

            Status = STATUS_SUCCESS;
            goto ErrorReturn;

        } else {
            EndingAddress = (PVOID64)(((ULONGLONG)CapturedBase +
                                    CapturedRegionSize - 1) | (PAGE_SIZE - 1));
            StartingAddress = (PVOID64)PAGE_ALIGN64(CapturedBase);
        }

        CapturedRegionSize = (ULONGLONG)EndingAddress - (ULONGLONG)StartingAddress + 1;

        FoundVad = MiCheckForConflictingVad64 (StartingAddress, EndingAddress);

        if (FoundVad == (PMMVAD)NULL) {

            //
            // No virtual address is reserved at the specified base address,
            // return an error.
            //

            Status = STATUS_CONFLICTING_ADDRESSES;
            goto ErrorReturn;
        }

        //
        // Ensure that the starting and ending addresses are all within
        // the same virtual address descriptor.
        //

        if ((MI_VA_TO_VPN64 (StartingAddress) < FoundVad->StartingVpn) ||
            (MI_VA_TO_VPN64 (EndingAddress) > FoundVad->EndingVpn)) {

            //
            // Not within the section virtual address descriptor,
            // return an error.
            //

            Status = STATUS_CONFLICTING_ADDRESSES;
            goto ErrorReturn;
        }


        if (FoundVad->u.VadFlags.PrivateMemory == 1) {

            //
            // PAGE_WRITECOPY is not valid for private pages.
            //

            if ((Protect & PAGE_WRITECOPY) ||
                (Protect & PAGE_EXECUTE_WRITECOPY)) {
                Status = STATUS_INVALID_PAGE_PROTECTION;
                goto ErrorReturn;
            }

            //
            // Ensure none of the pages are already committed as described
            // in the virtual address descriptor.
            //
#if 0
            if (AllocationType & MEM_CHECK_COMMIT_STATE) {
                if ( !MiIsEntireRangeDecommitted(StartingAddress,
                                                 EndingAddress,
                                                 FoundVad,
                                                 Process)) {

                    //
                    // Previously reserved pages have been committed, or
                    // an error occurred, release mutex and return status.
                    //

                    Status = STATUS_ALREADY_COMMITTED;
                    goto ErrorReturn;
                }
            }
#endif //0

            //
            // The address range has not been committed, commit it now.
            // Note, that for private pages, commitment is handled by
            // explicitly updating PTEs to contain Demand Zero entries.
            //

            StartingPte = MiGetPteAddress64 (StartingAddress);
            ASSERT (MiGetPdeAddress64(StartingAddress) == MiGetPteAddress (StartingPte));
            LastPte = MiGetPteAddress64 (EndingAddress);

            //
            // Check to ensure these pages can be committed.
            //

            QuotaCharge = 1 + LastPte - StartingPte;

            //
            // Charge quota and commitment for the range.  Establish an
            // exception handler as this could raise an exception.
            //

            QuotaFree = 1;
            Status = STATUS_SUCCESS;

            LOCK_PFN (OldIrql);
            if (MI_NONPAGABLE_MEMORY_AVAILABLE (QuotaCharge)) {
                MmResidentAvailablePages -= (PFN_NUMBER) QuotaCharge;
                MM_BUMP_COUNTER(24, QuotaCharge);
                QuotaFree = 0;
            }
            UNLOCK_PFN (OldIrql);
            if (QuotaFree == 1) {
                Status = STATUS_COMMITMENT_LIMIT;
                goto ErrorReturn;
            }

            if (Process->CommitChargeLimit) {
                if (Process->CommitCharge + QuotaCharge > Process->CommitChargeLimit) {
                    Status = STATUS_COMMITMENT_LIMIT;
                    LOCK_PFN (OldIrql);
                    MmResidentAvailablePages += (PFN_NUMBER) QuotaCharge;
                    MM_BUMP_COUNTER(24, -QuotaCharge);
                    UNLOCK_PFN (OldIrql);
                    if (Process->Job) {
                        PsReportProcessMemoryLimitViolation ();
                    }
                    goto ErrorReturn;
                }
            }
            if (Process->JobStatus & PS_JOB_STATUS_REPORT_COMMIT_CHANGES) {
                if (PsChangeJobMemoryUsage(QuotaCharge) == FALSE) {
                    Status = STATUS_COMMITMENT_LIMIT;
                    LOCK_PFN (OldIrql);
                    MmResidentAvailablePages += (PFN_NUMBER) QuotaCharge;
                    MM_BUMP_COUNTER(24, -QuotaCharge);
                    UNLOCK_PFN (OldIrql);
                    goto ErrorReturn;
                }
            }

            try {
                MiChargeCommitment (QuotaCharge, Process);
            } except (EXCEPTION_EXECUTE_HANDLER) {

                Status = GetExceptionCode();
                LOCK_PFN (OldIrql);
                MmResidentAvailablePages += (PFN_NUMBER) QuotaCharge;
                MM_BUMP_COUNTER(24, -QuotaCharge);
                UNLOCK_PFN (OldIrql);
                if (Process->JobStatus & PS_JOB_STATUS_REPORT_COMMIT_CHANGES) {

                    //
                    // Temporarily up the process commit charge as the
                    // job code will be referencing it as though everything
                    // has succeeded.
                    //

                    Process->CommitCharge += QuotaCharge;
                    PsChangeJobMemoryUsage (-(SSIZE_T)QuotaCharge);
                    Process->CommitCharge -= QuotaCharge;
                }
                goto ErrorReturn;
            }

            MM_TRACK_COMMIT (MM_DBG_COMMIT_ALLOCVM1_VLM, QuotaCharge);

            CommitSucceeded = MiCommitPages64 (StartingPte,
                                               LastPte,
                                               ProtectionMask,
                                               Process,
                                               &QuotaFree);

            if (CommitSucceeded == FALSE) {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                LOCK_PFN (OldIrql);
                MmResidentAvailablePages += (PFN_NUMBER) QuotaCharge;
                MM_BUMP_COUNTER(24, -QuotaCharge);
                UNLOCK_PFN (OldIrql);
                MiReturnCommitment (QuotaCharge);
                if (Process->JobStatus & PS_JOB_STATUS_REPORT_COMMIT_CHANGES) {

                    //
                    // Temporarily up the process commit charge as the
                    // job code will be referencing it as though everything
                    // has succeeded.
                    //

                    Process->CommitCharge += QuotaCharge;
                    PsChangeJobMemoryUsage (-(SSIZE_T)QuotaCharge);
                    Process->CommitCharge -= QuotaCharge;
                }
                goto ErrorReturn;
            }

            NewPages = QuotaCharge - QuotaFree;

            //
            // Excess charging of commit is returned below.
            //

            if (FoundVad->u.VadFlags.ImageMap == 1) {
                ASSERT (FoundVad->u.VadFlags.CommitCharge == 0);
                CommitCharge = (ULONG)FoundVad->u3.List.Flink;
            }
            else {
                CommitCharge = FoundVad->u.VadFlags.CommitCharge;
            }

            ASSERT (CommitCharge + NewPages >= CommitCharge);
            CommitCharge += NewPages;

            if (QuotaFree) {
                MiReturnCommitment (QuotaFree);
                MM_TRACK_COMMIT (MM_DBG_COMMIT_ALLOCVM1_VLM, -QuotaFree);
                if (Process->JobStatus & PS_JOB_STATUS_REPORT_COMMIT_CHANGES) {

                    //
                    // Temporarily up the process commit charge as the
                    // job code will be referencing it as though everything
                    // has succeeded.
                    //

                    Process->CommitCharge += QuotaFree;
                    PsChangeJobMemoryUsage (-(SSIZE_T)QuotaFree);
                    Process->CommitCharge -= QuotaFree;
                }
            }

            if (FoundVad->u.VadFlags.ImageMap == 1) {
                ASSERT (FoundVad->u.VadFlags.CommitCharge == 0);
                FoundVad->u3.List.Flink = (PLIST_ENTRY)CommitCharge;
            }
            else {
                FoundVad->u.VadFlags.CommitCharge = CommitCharge;
            }

            Process->CommitCharge += NewPages;
            if (Process->CommitCharge > Process->CommitChargePeak) {
                Process->CommitChargePeak = Process->CommitCharge;
            }

            VlmInfo = (PMI_PROCESS_VLM_INFO) (((PMMPTE)PDE_BASE64)->u.Long);
            VlmInfo->CommitCharge += NewPages;
            if (VlmInfo->CommitCharge > VlmInfo->CommitChargePeak) {
                VlmInfo->CommitChargePeak = VlmInfo->CommitCharge;
            }

            ExAcquireFastLock (&MiVlmStatisticsLock, &OldIrql);

            MiVlmCommitChargeInPages += NewPages;
            if (MiVlmCommitChargeInPages > MiVlmCommitChargeInPagesPeak) {
                MiVlmCommitChargeInPagesPeak = MiVlmCommitChargeInPages;
            }

            ExReleaseFastLock (&MiVlmStatisticsLock, OldIrql);

        } else {

            //
            // The no cache option is not allowed for sections.
            //

            if (Protect & PAGE_NOCACHE) {
                Status = STATUS_INVALID_PAGE_PROTECTION;
                goto ErrorReturn;
            }

            ASSERT (FoundVad->u.VadFlags.NoChange == 0);

            ASSERT (FoundVad->ControlArea->FilePointer == NULL);

            StartingPte = MiGetProtoPteAddress (FoundVad,
                                    MI_VA_TO_VPN64 (StartingAddress));
            PointerPte = StartingPte;
            LastPte = MiGetProtoPteAddress (FoundVad,
                                    MI_VA_TO_VPN64 (EndingAddress));

            UNLOCK_WS (Process);

            ExAcquireFastMutex (&MmSectionCommitMutex);

            PointerPte = StartingPte;

            //
            // Check to ensure these pages can be committed if this
            // is a page file backed segment.  Note that page file quota
            // has already been charged for this.
            //

            QuotaCharge = 1 + LastPte - StartingPte;

            //
            // Charge commitment for the range.  Establish an
            // exception handler as this could raise an exception.
            //

            QuotaFree = 0;

            Status = STATUS_INVALID_VLM_OPERATION;
            ChargedExactQuota = FALSE;

            for (; ; ) {
                GotResidentAvail = FALSE;
                LOCK_PFN (OldIrql);
                if (MI_NONPAGABLE_MEMORY_AVAILABLE (QuotaCharge)) {
                    MmResidentAvailablePages -= (PFN_NUMBER) QuotaCharge;
                    MM_BUMP_COUNTER(36, QuotaCharge);
                    Status = STATUS_SUCCESS;
                    GotResidentAvail = TRUE;
                }
                UNLOCK_PFN (OldIrql);

                if (Status == STATUS_SUCCESS) {

                    try {
                        MiChargeCommitment (QuotaCharge, NULL);
                    } except (EXCEPTION_EXECUTE_HANDLER) {
                        Status = GetExceptionCode();
                    }
                    if (Status == STATUS_SUCCESS) {
                        LOCK_PFN (OldIrql);
                        MmSharedCommitVlm += QuotaCharge;
                        UNLOCK_PFN (OldIrql);
                        break;
                    }
                }

                //
                // Not enough resident pages or commmit for the commitment,
                // If we've already backed out the previously committed
                // pages from the charge, just return an error.
                //

                if (GotResidentAvail == TRUE) {
                    LOCK_PFN (OldIrql);
                    MmResidentAvailablePages += (PFN_NUMBER) QuotaCharge;
                    MM_BUMP_COUNTER(36, -QuotaCharge);
                    UNLOCK_PFN (OldIrql);
                }

                if (Status != STATUS_INVALID_VLM_OPERATION) {

                    //
                    // We have already tried for the precise charge,
                    // return an error.
                    //

                    ExReleaseFastMutex (&MmSectionCommitMutex);
                    goto ErrorReturn1;
                }

                //
                // Quota charge failed, calculate the exact quota
                // taking into account pages that may already be
                // committed and retry the operation.
                //

                while (PointerPte <= LastPte) {

                    //
                    // Check to see if the prototype PTE is committed.
                    // Note that prototype PTEs cannot be decommitted so
                    // PTE only needs to be checked for zeroes.
                    //
                    //

                    if (PointerPte->u.Long != 0) {
                        QuotaFree += 1;
                    }
                    PointerPte += 1;
                }

                if (QuotaFree) {
                    PointerPte = StartingPte;
                    QuotaCharge -= QuotaFree;
                    Status = STATUS_COMMITMENT_LIMIT;
                    ChargedExactQuota = TRUE;
                }
                else {
                    ExReleaseFastMutex (&MmSectionCommitMutex);
                    goto ErrorReturn1;
                }
            }

            //
            // Commit all the pages.
            //

            QuotaFree = 0;
            TempPte = FoundVad->ControlArea->Segment->SegmentPteTemplate;

            FoundVad->ControlArea->Segment->NumberOfCommittedPages +=
                                                        QuotaCharge;

            while (PointerPte <= LastPte) {

                if (PointerPte->u.Long != 0) {

                    //
                    // Page is already committed, back out commitment.
                    //

                    QuotaFree += 1;
                } else {
                    *PointerPte = TempPte;
                }
                PointerPte += 1;
            }

            if (ChargedExactQuota == TRUE) {
                QuotaFree = 0;
            }
            else {
                LOCK_PFN (OldIrql);
                MmSharedCommitVlm -= QuotaFree;
                UNLOCK_PFN (OldIrql);
                FoundVad->ControlArea->Segment->NumberOfCommittedPages -=
                                                        QuotaFree;
            }

            ExReleaseFastMutex (&MmSectionCommitMutex);
            ASSERT ((LONG)QuotaFree >= 0);

            //
            // Change all the protections to be protected as specified.
            //

            LOCK_WS (Process);

#if 0 //fixfix charge and update protection.
            MiSetProtectionOnSection (Process,
                                      FoundVad,
                                      StartingAddress,
                                      EndingAddress,
                                      Protect,
                                      &OldProtect,
                                      TRUE);

#endif //0
        }

        UNLOCK_WS (Process);

        if (QuotaFree != 0) {
            LOCK_PFN (OldIrql);
            MmResidentAvailablePages += (PFN_NUMBER) QuotaFree;
            MM_BUMP_COUNTER(25, QuotaFree);
            UNLOCK_PFN (OldIrql);
        }

        //
        // Previously reserved pages have been committed, or an error occurred,
        // release working set lock, address creation lock, detach,
        // dereference process and return status.
        //

        UNLOCK_ADDRESS_SPACE(Process);

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
            return GetExceptionCode();
        }

#if 0
        if (MmDebug & MM_DBG_SHOW_NT_CALLS) {
            if ( MmWatchProcess ) {
                if ( MmWatchProcess == PsGetCurrentProcess() ) {
                    DbgPrint("\n+++ ALLOC Type %lx Base %p Size %p\n",
                        AllocationType,StartingAddress, CapturedRegionSize);
                    MmFooBar();
                }
            } else {
                DbgPrint("return allocvm status %lx baseaddr %p size %p\n",
                    Status, CapturedRegionSize, StartingAddress);
            }
        }
#endif

        return STATUS_SUCCESS;
    }

ErrorReturn:
    UNLOCK_WS (Process);

ErrorReturn1:

    UNLOCK_ADDRESS_SPACE (Process);
    if (Attached) {
        KeDetachProcess();
    }
    if (ProcessHandle != NtCurrentProcess()) {
        ObDereferenceObject (Process);
    }
    return Status;
}

NTSTATUS
NtFreeVirtualMemory64(
    IN HANDLE ProcessHandle,
    IN OUT PVOID64 *BaseAddress,
    IN OUT PULONGLONG RegionSize,
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

   FreeType - A set of flags that describe the type of free that is to
              be performed for the specified region of pages.


       FreeType Flags


        MEM_DECOMMIT - The specified region of pages is to
                       be decommitted.

        MEM_RELEASE - The specified region of pages is to
                      be released.


Return Value:

    Returns the status

    TBS


--*/

{
    PMMVAD_SHORT Vad;
    PMMVAD_SHORT NewVad;
    PMMVAD PreviousVad;
    PMMVAD NextVad;
    PEPROCESS Process;
    KPROCESSOR_MODE PreviousMode;
    PVOID64 StartingAddress;
    PVOID64 EndingAddress;
    NTSTATUS Status;
    LOGICAL Attached;
    ULONGLONG CapturedRegionSize;
    PVOID64 CapturedBase;
    PMMPTE StartingPte;
    PMMPTE EndingPte;
    SIZE_T OldQuota;
    SIZE_T QuotaCharge;
    SIZE_T CommitReduction;
    ULONG OldEnd;
    KIRQL OldIrql;
    PMI_PROCESS_VLM_INFO VlmInfo;

    PAGED_CODE();

    Attached = FALSE;

    if (MI_VLM_ENABLED() == FALSE) {
        return STATUS_NOT_IMPLEMENTED;
    }

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

            ProbeForWrite (BaseAddress, sizeof(PVOID64), sizeof(PVOID64));
            ProbeForWrite (RegionSize, sizeof(ULONGLONG), sizeof(ULONGLONG));
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

#if 0
    if (MmDebug & MM_DBG_SHOW_NT_CALLS) {
        if ( !MmWatchProcess ) {
            DbgPrint("freevm processhandle %lx base %p size %p type %lx\n",
                    ProcessHandle, CapturedBase, CapturedRegionSize, FreeType);
        }
    }
#endif

    if ((CapturedRegionSize != 0) && (FreeType & MEM_RELEASE)) {
        return STATUS_FREE_VM_NOT_AT_BASE;
    }

    //
    // Make sure the specified starting and ending addresses are
    // within the user part of the virtual address space.
    //

    if (CapturedBase > (PVOID64)MM_HIGHEST_USER_ADDRESS64) {

        //
        // Invalid base address.
        //

        return STATUS_INVALID_PARAMETER_2;
    }

    if ((ULONGLONG)MM_HIGHEST_USER_ADDRESS64 - (ULONGLONG)CapturedBase <
                                                        CapturedRegionSize) {

        //
        // Invalid region size;
        //

        return STATUS_INVALID_PARAMETER_3;

    }

    EndingAddress = (PVOID64)(((ULONGLONG)CapturedBase + CapturedRegionSize - 1) |
                        (PAGE_SIZE - 1));

    StartingAddress = (PVOID64)PAGE_ALIGN64(CapturedBase);

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

    if (Process->ForkWasSuccessful != MM_NO_FORK_ALLOWED) {
        Status = STATUS_MEMORY_NOT_ALLOCATED;
        goto ErrorReturn;
    }

    Vad = (PMMVAD_SHORT)MiLocateAddress64 (MI_VA_TO_VPN64 (StartingAddress));

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

    if (Vad->EndingVpn < MI_VA_TO_VPN64 (EndingAddress)) {

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

    ASSERT (Vad->u.VadFlags.NoChange == 0);

    if (FreeType & MEM_RELEASE) {

        //
        // *****************************************************************
        // MEM_RELEASE was specified.
        // *****************************************************************
        //

        //
        // The descriptor for the address range is deletable.  Remove the
        // the descriptor.
        //

        ASSERT (CapturedRegionSize == 0);

        //
        // If the region size is specified as 0, the base address
        // must be the starting address for the region.
        //

        if (MI_VA_TO_VPN64 (CapturedBase) != Vad->StartingVpn) {
            Status = STATUS_FREE_VM_NOT_AT_BASE;
            goto ErrorReturn;
        }

        //
        // This Virtual Address Descriptor has been deleted.
        //

        StartingAddress = MI_VPN_TO_VA64 (Vad->StartingVpn);
        StartingPte = MiGetPteAddress64 (StartingAddress);
        EndingAddress = MI_VPN_TO_VA64 (Vad->EndingVpn);
        EndingPte = MiGetPteAddress64 (EndingAddress);
        CapturedRegionSize = ((EndingPte - StartingPte + 1) << PAGE_SHIFT);
        MiRemoveVad64 ((PMMVAD)Vad);
        ExFreePool (Vad);

        //
        // Get the PFN mutex so that MiDeleteVirtualAddresses can be called.
        //

        CommitReduction = MiDecommitOrDeletePages64 (StartingPte,
                                                     EndingPte,
                                                     Process,
                                                     DELETE_TYPE_PRIVATE,
                                                     TRUE);
        UNLOCK_WS (Process);

        //
        // Update the virtual size in the process header.
        //

        VlmInfo = (PMI_PROCESS_VLM_INFO) (((PMMPTE)PDE_BASE64)->u.Long);
        VlmInfo->VirtualSize -= CapturedRegionSize;
        VlmInfo->CommitCharge -= CommitReduction;

        ExAcquireFastLock (&MiVlmStatisticsLock, &OldIrql);
        MiVlmCommitChargeInPages -= CommitReduction;
        ExReleaseFastLock (&MiVlmStatisticsLock, OldIrql);

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

#if 0
        if (MmDebug & MM_DBG_SHOW_NT_CALLS) {
            if ( MmWatchProcess ) {
                if ( MmWatchProcess == PsGetCurrentProcess() ) {
                    DbgPrint("\n--- FREE Type 0x%lx Base %p Size %p\n",
                            FreeType, StartingAddress, CapturedRegionSize);
                    MmFooBar();
                }
            }
        }
#endif

        return STATUS_SUCCESS;
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

        if (MI_VA_TO_VPN64 (CapturedBase) != Vad->StartingVpn) {
            Status = STATUS_FREE_VM_NOT_AT_BASE;
            goto ErrorReturn;
        }
        EndingAddress = MI_VPN_TO_VA_ENDING64 (Vad->EndingVpn);
    }

    //
    // Calculate the initial quotas and commit charges for this VAD.
    //

    StartingPte = MiGetPteAddress64 (StartingAddress);
    EndingPte = MiGetPteAddress64 (EndingAddress);

    CapturedRegionSize = 1 + (ULONGLONG)EndingAddress - (ULONGLONG)StartingAddress;

    CommitReduction = MiDecommitOrDeletePages64 (StartingPte,
                                                 EndingPte,
                                                 Process,
                                                 DELETE_TYPE_PRIVATE,
                                                 TRUE);

    VlmInfo = (PMI_PROCESS_VLM_INFO) (((PMMPTE)PDE_BASE64)->u.Long);
    VlmInfo->CommitCharge -= CommitReduction;

    ExAcquireFastLock (&MiVlmStatisticsLock, &OldIrql);
    MiVlmCommitChargeInPages -= CommitReduction;
    ExReleaseFastLock (&MiVlmStatisticsLock, OldIrql);

    UNLOCK_WS (Process);
    UNLOCK_ADDRESS_SPACE (Process);

    if (Attached) {
        KeDetachProcess();
    }
    if (ProcessHandle != NtCurrentProcess()) {
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
        NOTHING;
    }

    return STATUS_SUCCESS;

ErrorReturn:

    UNLOCK_WS (Process);
    UNLOCK_ADDRESS_SPACE (Process);

    if (Attached) {
        KeDetachProcess();
    }

    if (ProcessHandle != NtCurrentProcess()) {
       ObDereferenceObject (Process);
    }

    return Status;
}


PVOID64
MiFindEmptyAddressRangeInTree64 (
    IN ULONGLONG SizeOfRange64,
    IN ULONG_PTR Alignment,
    IN PMMADDRESS_NODE Root
    )

/*++

Routine Description:

    The function examines the virtual address descriptors to locate
    an unused range of the specified size and returns the starting
    address of the range.

Arguments:

    SizeOfRange - Supplies the size in bytes of the range to locate.

    Alignment - Supplies the alignment for the address.  Must be
                 a power of 2 and greater than the page_size.

    Root - Supplies the root of the tree to search through.


Return Value:

    Returns the starting address of a suitable range.

--*/

{
    PMMADDRESS_NODE Node;
    PMMADDRESS_NODE NextNode;
    ULONG_PTR AlignmentVpn;
    ULONG SizeOfRange;
    PVOID64 Start;

    AlignmentVpn = Alignment >> PAGE_SHIFT;

    //
    // Locate the Node with the lowest starting address.
    //

    SizeOfRange = (ULONG)((SizeOfRange64 + (PAGE_SIZE - 1)) >> PAGE_SHIFT);
    ASSERT (SizeOfRange != 0);

    Node = Root;

    if (Node == (PMMADDRESS_NODE)NULL) {
        return (PVOID64)MM_LOWEST_USER_ADDRESS64;
    }
    while (Node->LeftChild != (PMMADDRESS_NODE)NULL) {
        Node = Node->LeftChild;
    }

    //
    // Check to see if a range exists between the lowest address VAD
    // and lowest user address.
    //

    if (Node->StartingVpn > MI_VA_TO_VPN64 (MM_LOWEST_USER_ADDRESS64)) {
        if ( SizeOfRange <
            (Node->StartingVpn - MI_VA_TO_VPN64 (MM_LOWEST_USER_ADDRESS64))) {

            return (PVOID64)MM_LOWEST_USER_ADDRESS64;
        }
    }

    for (;;) {

        NextNode = MiGetNextNode (Node);

        if (NextNode != (PMMADDRESS_NODE)NULL) {

            if (SizeOfRange <=
                ((ULONG_PTR)NextNode->StartingVpn -
                                MI_ROUND_TO_SIZE(1 + Node->EndingVpn,
                                                 AlignmentVpn))) {

                //
                // Check to ensure that the ending address aligned upwards
                // is not greater than the starting address.
                //

                if ((ULONG_PTR)NextNode->StartingVpn >
                        MI_ROUND_TO_SIZE(1 + Node->EndingVpn,
                                         AlignmentVpn)) {

                    return (PVOID64)MI_ROUND_TO_SIZE(
                            (ULONGLONG)(MI_VPN_TO_VA_ENDING64(Node->EndingVpn)),
                            (ULONGLONG)Alignment);
                }
            }

        } else {

            //
            // No more descriptors, check to see if this fits into the remainder
            // of the address space.
            //

            if ((((ULONG_PTR)Node->EndingVpn + MI_VA_TO_VPN(X64K)) <
                    MI_VA_TO_VPN64 (MM_HIGHEST_VAD_ADDRESS64))
                        &&
                (SizeOfRange64 <=
                    ((ULONGLONG)MM_HIGHEST_VAD_ADDRESS64 - X64K -
                            MI_ROUND_TO_SIZE(
                             (ULONGLONG)(MI_VPN_TO_VA64(Node->EndingVpn)),
                             (ULONGLONG)Alignment)))) {

                Start = MI_VPN_TO_VA_ENDING64 (Node->EndingVpn);
                ASSERT (MI_VA_TO_VPN64 (Start) == Node->EndingVpn);

                Start = (PVOID64)MI_ROUND_TO_SIZE((ULONGLONG)Start,
                                                  (ULONGLONG)Alignment);
                ASSERT (MI_VA_TO_VPN64 (Start) > Node->EndingVpn);
                return Start;
            } else {
                ExRaiseStatus (STATUS_NO_MEMORY);
            }
        }
        Node = NextNode;
    }
}

LOGICAL
MiCommitPages64 (
    IN PMMPTE StartPte,
    IN PMMPTE LastPte,
    IN ULONG ProtectionMask,
    IN PEPROCESS Process,
    OUT PSIZE_T PtesCommitted
    )

/*++

Routine Description:

    This function commits by making each page valid in the specified range of
    PTEs.  It also changes the protection of any existing valid PTEs in
    the specified range.

Arguments:

    StartPte - First PTE in the range.

    LastPte - Last PTE in the range.

    ProtectionMask - Protection mask to set in the valid PTEs.

    Process - Pointer to the process object of the subject process.

    PtesCommitted - Supplies a pointer to receive the number of valid PTEs
                    encountered in the range.

Return Value:

    TRUE if the pages were committed, FALSE if not.

Environment:

    Process Working Set Lock held.

--*/

{
    PMMPTE PointerPte;
    PMMPTE PointerPde;
    MMPTE TempPte;
    PFN_NUMBER PageFrameIndex;
    PMMPFN Pfn1;
    ULONG PageColor;
    KIRQL OldIrql;
    ULONG BarrierStamp;
    LOGICAL TopPdeBuilt;
    LOGICAL PdeBuilt;
    PUSHORT UsedPde64Handle;

    *PtesCommitted = 0;
    PointerPde = MiGetPteAddress (StartPte);

    //
    // Fill in all the page table pages with the valid PTEs.
    //

    TopPdeBuilt = MiMakePdeExistAndMakeValid64 (PointerPde, Process, FALSE, TRUE);
    if (TopPdeBuilt == FALSE) {
        return FALSE;
    }

    UsedPde64Handle = MI_GET_USED_PDE64_HANDLE (PointerPde);

    PointerPte = StartPte;
    while (PointerPte <= LastPte) {

        if (MiIsPteOnPdeBoundary(PointerPte)) {

            //
            // Pointing to the next page table page, make
            // a page table page exist and make it valid.
            //

            PointerPde = MiGetPteAddress (PointerPte);
            UsedPde64Handle = MI_GET_USED_PDE64_HANDLE (PointerPde);

            //
            // This call will always succeed since the top level PDE already
            // exists.
            //

            PdeBuilt = MiMakePdeExistAndMakeValid64 (PointerPde, Process, FALSE, TRUE);
            ASSERT (PdeBuilt == TRUE);
        }

        if (PointerPte->u.Long == 0) {

            PageColor = MI_PAGE_COLOR_PTE_PROCESS (PointerPte,
                                                   &Process->NextPageColor);
            PointerPte->u.Soft.Protection = ProtectionMask;
            LOCK_PFN (OldIrql);

            //
            // There can be no races on private PTEs (just PDEs) so even
            // if a wait state occurs, nothing needs to be rechecked.
            //

            MiEnsureAvailablePageOrWait (Process, NULL);

            PageFrameIndex = MiRemoveZeroPage (PageColor);

            BarrierStamp = MI_PFN_ELEMENT(PageFrameIndex)->PteFrame;

            MiInitializePfn (PageFrameIndex, PointerPte, 1);
            UNLOCK_PFN (OldIrql);

            MI_MAKE_VALID_PTE (TempPte,
                               PageFrameIndex,
                               ProtectionMask,
                               NULL);

            if (TempPte.u.Hard.Write) {
                MI_SET_PTE_DIRTY (TempPte);
            }

            MI_BARRIER_SYNCHRONIZE (BarrierStamp);

            *PointerPte = TempPte;
            (*UsedPde64Handle) += 1;

        } else {
            ASSERT (PointerPte->u.Hard.Valid == 1);
            (*PtesCommitted) += 1;

            Pfn1 = MI_PFN_ELEMENT (PointerPte->u.Hard.PageFrameNumber);

            if (Pfn1->OriginalPte.u.Soft.Protection != ProtectionMask) {

                //
                // Change protection to match specified protection.
                //

                Pfn1->OriginalPte.u.Soft.Protection = ProtectionMask;
                MI_MAKE_VALID_PTE (TempPte,
                                   PointerPte->u.Hard.PageFrameNumber,
                                   ProtectionMask,
                                   NULL);
                if (TempPte.u.Hard.Write) {
                    MI_SET_PTE_DIRTY (TempPte);
                }

                //
                // Flush the TB as we have changed the protection
                // of a valid PTE.
                //

                MiFlushTbAndCapture (PointerPte,
                                     TempPte.u.Flush,
                                     Pfn1);
            }
        }
        PointerPte += 1;
    }
    return TRUE;
}

LOGICAL
MiMakePdeExistAndMakeValid64 (
    IN PMMPTE PointerPde,
    IN PEPROCESS TargetProcess,
    IN LOGICAL PfnLockHeld,
    IN LOGICAL MakePageTablePage
    )

/*++

Routine Description:

    This routine examines the specified Page Directory Entry to determine
    if the page table page mapped by the PDE exists.

    If the page table page exists and is not currently in memory, the
    working set mutex and, if held, the PFN mutex are released and the
    page table page is faulted into the working set.  The mutexes are
    required.

    If the PDE exists, the function returns TRUE.

    If the PDE does not exist, a zero filled PTE is created and it
    too is brought into the working set.  In this case the return
    value is FALSE.

Arguments:

    PointerPde - Supplies a pointer to the PDE to examine and bring
                 into the working set.

    TargetProcess - Supplies a pointer to the current process.

    PfnLockHeld - Supplies the value TRUE if the PFN mutex is held, FALSE
                  otherwise.

    MakePageTablePage - Supplies the value TRUE if the PDE is to be created.
                        FALSE if not - meaning just the top level Base is
                        to be created.

Return Value:

    TRUE if the PDE exists, FALSE if the PDE was created.

Environment:

    Kernel mode, APCs disabled, WorkingSetLock and AddressSpace locks held.

--*/

{
    PMMPTE PointerPte;
    KIRQL OldIrql;
    MMPTE TempPte;
    ULONG PageColor;
    PFN_NUMBER PageFrameIndex;
    ULONG BarrierStamp;
    PMI_PROCESS_VLM_INFO VlmInfo;
    PMMPTE PdeBase;

    VlmInfo = NULL;

    //
    // Make sure the Page Directory Page for this 32GB region exists.
    //

    PointerPte = MiGetPteAddress (PointerPde);

redo:

    if (PointerPte->u.Long == 0) {

        //
        // Make a page directory page.
        //

        if (VlmInfo == NULL) {

            if (PfnLockHeld) {
                UNLOCK_PFN (OldIrql);
            }

            UNLOCK_WS (TargetProcess);

            VlmInfo = (PMI_PROCESS_VLM_INFO) ExAllocatePoolWithTag (
                                                NonPagedPool,
                                                sizeof (MI_PROCESS_VLM_INFO),
                                                'lVmM');

            if (VlmInfo) {
                RtlZeroMemory (VlmInfo,
                               sizeof (MI_PROCESS_VLM_INFO));
            }

            LOCK_WS (TargetProcess);

            if (PfnLockHeld) {
                LOCK_PFN (OldIrql);
            }

            if (VlmInfo == NULL) {
                return FALSE;
            }

            //
            // Since locks were released, everything must be rechecked.
            // Note the VAD cannot disappear as the address space lock was
            // held all the way through.
            //

            goto redo;
        }

        PageColor = MI_PAGE_COLOR_PTE_PROCESS (PointerPte,
                                               &TargetProcess->NextPageColor);
        if (!PfnLockHeld) {
            LOCK_PFN (OldIrql);
        }

        if (MiEnsureAvailablePageOrWait (TargetProcess, NULL) == TRUE) {

            //
            // A wait state occurred, everything must be rechecked.
            //

            goto redo;
        }

        PageFrameIndex = MiRemoveZeroPage (PageColor);
        BarrierStamp = MI_PFN_ELEMENT(PageFrameIndex)->PteFrame;
        MiInitializePfn (PageFrameIndex, PointerPte, 1);

        if (!PfnLockHeld) {
            UNLOCK_PFN (OldIrql);
        }

        MI_MAKE_VALID_PTE (TempPte,
                           PageFrameIndex,
                           MM_READWRITE,
                           NULL);

        MI_SET_PTE_DIRTY (TempPte);

        MI_BARRIER_SYNCHRONIZE (BarrierStamp);

        *PointerPte = TempPte;

        //
        // Stash the used page table page counts pointer in the PDE_BASE64.
        //

        PdeBase = MiGetVirtualAddressMappedByPte (PointerPte);

        //
        // This will always be true unless VLM is expanded past 32GB.
        //

        ASSERT (PdeBase == (PMMPTE)PDE_BASE64);

        PdeBase->u.Long = (ULONG)VlmInfo;
    }
    else {
        if (VlmInfo) {
            ExFreePool (VlmInfo);
        }
    }

    ASSERT (PointerPte->u.Hard.Valid == 1);

    if (MakePageTablePage == FALSE) {
        return TRUE;
    }

redo2:

    if (PointerPde->u.Hard.Valid == 1) {

        //
        // Already valid.
        //

        return TRUE;
    }

    //
    // Page directory entry not valid, make it valid.
    //

    PageColor = MI_PAGE_COLOR_PTE_PROCESS (PointerPde,
                                           &TargetProcess->NextPageColor);
    if (!PfnLockHeld) {
        LOCK_PFN (OldIrql);
    }

    if (MiEnsureAvailablePageOrWait (TargetProcess, NULL) == TRUE) {

        //
        // A wait state occurred, everything from the PDE down must
        // be rechecked.
        //

        if (!PfnLockHeld) {
            UNLOCK_PFN (OldIrql);
        }
        goto redo2;
    }

    PageFrameIndex = MiRemoveZeroPage (PageColor);
    BarrierStamp = MI_PFN_ELEMENT(PageFrameIndex)->PteFrame;
    MiInitializePfn (PageFrameIndex, PointerPde, 1);

    if (!PfnLockHeld) {
        UNLOCK_PFN (OldIrql);
    }

    MI_MAKE_VALID_PTE (TempPte,
                       PageFrameIndex,
                       MM_READWRITE,
                       NULL);

    MI_SET_PTE_DIRTY (TempPte);

    MI_BARRIER_SYNCHRONIZE (BarrierStamp);

    *PointerPde = TempPte;

    return TRUE;
}

ULONG
MiDoesPdeExist64 (
    IN PMMPTE PointerPde
    )

/*++

Routine Description:

    This routine examines the specified Page Directory Entry to determine
    if the page table page mapped by the PDE exists.

    If the page table page exists and is not currently in memory, the
    working set mutex and, if held, the PFN mutex are released and the
    page table page is faulted into the working set.  The mutexes are
    required.

    If the PDE exists, the function returns true.

Arguments:

    PointerPde - Supplies a pointer to the PDE to examine and potentially
                 bring into the working set.

    TargetProcess - Supplies a pointer to the current process.

    PfnMutexHeld - Supplies the value TRUE if the PFN mutex is held, FALSE
                   otherwise.

Return Value:

    TRUE if the PDE exists, FALSE if the PDE is zero.

Environment:

    Kernel mode, APCs disabled, WorkingSetLock held.

--*/

{
    PMMPTE PointerPte;
    PMI_PROCESS_VLM_INFO VlmInfo;

    PointerPte = MiGetPteAddress(PointerPde);
    if (PointerPte->u.Long == 0) {

        //
        // This page directory entry doesn't exist, return FALSE.
        //

        return FALSE;
    }

    if (PointerPde->u.Long == 0) {

        //
        // This page directory entry doesn't exist, return FALSE.
        //

        return FALSE;
    }

    ASSERT (PointerPde->u.Hard.Valid == 1);

    VlmInfo = (PMI_PROCESS_VLM_INFO) (((PMMPTE)PDE_BASE64)->u.Long);
    ASSERT (VlmInfo != NULL);

    return TRUE;
}

ULONG
MiDecommitOrDeletePages64 (
    IN PMMPTE StartingPte,
    IN PMMPTE EndingPte,
    IN PEPROCESS Process,
    IN ULONG Type,
    IN LOGICAL FlushTb
    )

/*++

Routine Description:

    This routine decommits the specified range of pages.

Arguments:

    StartingAddress - Supplies the starting address of the range.

    EndingPte - Supplies the ending PTE of the range.

    Process - Supplies the current process.

    Type - One of: DELETE_TYPE_PRIVATE for private memory or DELETE_TYPE_SHARED
           for shared memory.

    FlushTb - Supplies TRUE if the TB needs to be flushed, FALSE if not.

Return Value:

    Value to reduce commitment by for the VAD.

Environment:

    Kernel mode, APCs disabled, WorkingSetMutex and AddressCreation mutexes
    held.

--*/

{
    PMMPTE PointerPde;
    PMMPTE PointerPte;
    ULONG PdeOffset;
    PFN_NUMBER CommitReduction;
    PMMPTE CommitLimitPte;
    KIRQL OldIrql;
    PMMPTE ValidPteList[MM_VALID_PTE_SIZE];
    ULONG count;
    ULONG WorkingSetIndex;
    PMMPFN Pfn1;
    PMMPFN Pfn2;
    PVOID SwapVa;
    ULONG Entry;
    MMPTE PteContents;
    MMPTE NewPte;
    ULONG PfnLockHeld;
    PMMPTE LowestPde;
    PMI_PROCESS_VLM_INFO VlmInfo;

    count = 0;
    PfnLockHeld = FALSE;
    CommitReduction = 0;

    //
    // Decommit each page by setting the PTE to be explicitly
    // decommitted.  The PTEs cannot be deleted all at once as
    // this would set the PTEs to zero which would auto-evaluate
    // as committed if referenced by another thread when a page
    // table page is being in-paged.
    //

    PointerPte = StartingPte;
    PointerPde = MiGetPteAddress (StartingPte);

    VlmInfo = (PMI_PROCESS_VLM_INFO) (((PMMPTE)PDE_BASE64)->u.Long);
    LowestPde = MiGetPdeAddress64 (MM_LOWEST_USER_ADDRESS64);

    //
    // Loop through all the PDEs which map this region and ensure that
    // they exist.  If they don't exist create them by touching a
    // PTE mapped by the PDE.
    //

    if (!MiDoesPdeExist64 (PointerPde)) {
        PointerPte = (PMMPTE)PAGE_ALIGN (PointerPte);
        PointerPte += PTE_PER_PAGE;
    }

    while (PointerPte <= EndingPte) {

        if (MiIsPteOnPdeBoundary(PointerPte)) {

            if (count != 0) {
                MiProcessValidPteList64 (&ValidPteList[0], count, FlushTb);
                count = 0;
            }

            if (PfnLockHeld) {
                UNLOCK_PFN (OldIrql);
            }

            MiCheckPdeForDeletion (PointerPde, FlushTb);
            PointerPde = MiGetPteAddress (PointerPte);

            if (!MiDoesPdeExist64 (PointerPde)) {
                PointerPte += PTE_PER_PAGE;
                continue;
            }
            if (PfnLockHeld) {
                LOCK_PFN (OldIrql);
            }
        }

        //
        // The working set lock is held.  No PTEs can go from
        // invalid to valid or valid to invalid.  Transition
        // PTEs can go from transition to pagefile.
        //

        PteContents = *PointerPte;

        if (PteContents.u.Long != 0) {

            VlmInfo->UsedPageTableEntries[PointerPde - LowestPde] -= 1;

            if (PteContents.u.Hard.Valid == 1) {
                if (Type == DELETE_TYPE_PRIVATE) {
                    CommitReduction += 1;
                }

                //
                // Pte is valid, process later when PFN lock is held.
                //

                if (count == MM_VALID_PTE_SIZE) {
                    MiProcessValidPteList64 (&ValidPteList[0], count, FlushTb);
                    count = 0;
                }
                ValidPteList[count] = PointerPte;
                count += 1;
            } else if (PteContents.u.Soft.Prototype == 1) {
                NOTHING;

            } else if (PteContents.u.Soft.Transition == 1) {

                //
                // This is a transition PTE. (Page is private)
                //
                // Get the PFN mutex so that MiDeletePte can be called.
                //

                if (!PfnLockHeld) {
                    LOCK_PFN (OldIrql);
                    PfnLockHeld = TRUE;
                    continue;
                }
                Pfn1 = MI_PFN_ELEMENT (PteContents.u.Trans.PageFrameNumber);

                MI_SET_PFN_DELETED (Pfn1);

                MiDecrementShareCount (Pfn1->PteFrame);

                //
                // Check the reference count for the page, if the reference
                // count is zero, move the page to the free list, if the
                // reference count is not zero, ignore this page.  When the
                // reference count goes to zero, it will be placed on the
                // free list.
                //

                if (Pfn1->u3.e2.ReferenceCount == 0) {
                    MiUnlinkPageFromList (Pfn1);
                    MiReleasePageFileSpace (Pfn1->OriginalPte);
                    MiInsertPageInList (MmPageLocationList[FreePageList],
                                        MI_GET_PAGE_FRAME_FROM_TRANSITION_PTE(&PteContents));
                }
            } else {
                ASSERT (PteContents.u.Soft.PageFileHigh == 0);
            }
        }

        if (PfnLockHeld) {
            UNLOCK_PFN (OldIrql);
            PfnLockHeld = FALSE;
        }
        PointerPte += 1;
    }
    if (count != 0) {
        MiProcessValidPteList64 (&ValidPteList[0], count, FlushTb);
    }

    MiCheckPdeForDeletion (PointerPde, FlushTb);

    MiReturnAvailablePages (CommitReduction);

    return CommitReduction;
}
VOID
MiProcessValidPteList64 (
    IN PMMPTE *ValidPteList,
    IN ULONG Count,
    IN LOGICAL FlushTb
    )

/*++

Routine Description:

    This routine flushes the specified range of valid PTEs.

Arguments:

    ValidPteList - Supplies a pointer to an array of PTEs to flush.

    Count - Supplies the count of the number of elements in the array.

    FlushTb - Supplies TRUE if the TB needs to be flushed, FALSE if not.

Return Value:

    none.

Environment:

    Kernel mode, APCs disabled, WorkingSetMutex and AddressCreation mutexes
    held.

--*/

{
    ULONG i = 0;
    MMPTE_FLUSH_LIST64 PteFlushList;
    MMPTE PteContents;
    PMMPFN Pfn1;
    KIRQL OldIrql;
    PMMPTE PointerPde;

    PteFlushList.Count = Count;

    LOCK_PFN (OldIrql);

    do {
        PteContents = *ValidPteList[i];
        ASSERT (PteContents.u.Hard.Valid == 1);
        Pfn1 = MI_PFN_ELEMENT (PteContents.u.Hard.PageFrameNumber);

        if (Pfn1->u3.e1.PrototypePte == 1) {

            //
            // Capture the state of the modified bit for this
            // pte.
            //

            MI_CAPTURE_DIRTY_BIT_TO_PFN (&PteContents, Pfn1);

            //
            // Decrement the share and valid counts of the page table
            // page which maps this PTE.
            //

            PointerPde = MiGetPteAddress (ValidPteList[i]);
            MiDecrementShareAndValidCount (MI_GET_PAGE_FRAME_FROM_PTE(PointerPde));

            //
            // Decrement the share count for the physical page.
            //

            MiDecrementShareCount (MI_GET_PAGE_FRAME_FROM_PTE(&PteContents));

        } else {

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

            MiDecrementShareCountOnly (MI_GET_PAGE_FRAME_FROM_PTE(&PteContents));
        }

        if (Count < MM_MAXIMUM_FLUSH_COUNT) {
            PteFlushList.FlushPte[i] = ValidPteList[i];
            PteFlushList.FlushVpn[i] =
                 MiGetVirtualPageNumberMappedByPte64 (ValidPteList[i]) >> PAGE_SHIFT;
        }
        *ValidPteList[i] = ZeroPte;
        i += 1;
    } while (i != Count);

    if (FlushTb) {
        MiFlushPteList64 (&PteFlushList, FALSE, ZeroPte);
    }

    UNLOCK_PFN (OldIrql);
    return;
}

VOID
MiReturnAvailablePages (
    IN PFN_NUMBER Amount
    )

/*++

Routine Description:

    This routine returns the specified amount to the system resident pages list.

Arguments:

    Amount - Supplies the amount to return.

Return Value:

    none.

Environment:

    Kernel mode, APCs disabled, WorkingSetMutex and AddressCreation mutexes
    held.

--*/

{
    KIRQL OldIrql;

    LOCK_PFN (OldIrql);
    MmResidentAvailablePages += Amount;
    MM_BUMP_COUNTER(26, Amount);
    UNLOCK_PFN (OldIrql);
    return;
}


LOGICAL
MiCheckPdeForDeletion (
    IN PMMPTE PointerPde,
    IN LOGICAL FlushTb
    )

/*++

Routine Description:

    This routine checks to see if the share count of the specified PDE is 1,
    and if it is, the PDE is deleted.

Arguments:

    PointerPde - Supplies the PDE to check.

    FlushTb - Supplies TRUE if the TB needs to be flushed, FALSE if not.

Return Value:

    TRUE if the PDE no longer exists upon return.
    FALSE if the PDE still exists on return.

Environment:

    Kernel mode, APCs disabled, WorkingSetMutex and AddressCreation mutexes
    held.

--*/

{
    LOGICAL Status;
    KIRQL OldIrql;
    PFN_NUMBER PageFrameIndex;
    PMMPFN Pfn1;
    PMMPTE PointerPte;
    PMI_PROCESS_VLM_INFO VlmInfo;
    PMMPTE LowestPde;

    PointerPte = MiGetPteAddress (PointerPde);
    if (PointerPte->u.Hard.Valid == 0) {

        //
        // Page Directory does not exist.
        //

        return TRUE;
    }

    if (PointerPde->u.Hard.Valid == 0) {

        //
        // Page directory entry does not exist.
        //

        return TRUE;
    }
    PageFrameIndex = MI_GET_PAGE_FRAME_FROM_PTE(PointerPde);
    Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);
    Status = FALSE;
    LOCK_PFN (OldIrql);

    VlmInfo = (PMI_PROCESS_VLM_INFO) (((PMMPTE)PDE_BASE64)->u.Long);
    LowestPde = MiGetPdeAddress64 (MM_LOWEST_USER_ADDRESS64);

    PointerPte = MiGetVirtualAddressMappedByPte (PointerPde);

    if (VlmInfo->UsedPageTableEntries[PointerPde - LowestPde] == 0) {
        ASSERT (Pfn1->u2.ShareCount == 1);
        MiDecrementShareAndValidCount (Pfn1->PteFrame);
        MI_SET_PFN_DELETED (Pfn1);
        MiDecrementShareCountOnly (PageFrameIndex);
        if (FlushTb == TRUE) {
            KeFlushSingleTb (PointerPte,
                             TRUE,
                             FALSE,
                             (PHARDWARE_PTE)PointerPde,
                             ZeroPte.u.Flush);
        }
        else {
             *PointerPde = ZeroPte;
        }
        Status = TRUE;
    }
#if DBG
    else {
        ULONG i;
        ULONG count;
        PULONG p;

        count = 0;
        p = (PULONG)PointerPte;

        for (i = 0; i < PAGE_SIZE; i += sizeof(p)) {
            p += 1;
            count += 1;
        }
        ASSERT (count != 0);
    }
#endif

    UNLOCK_PFN (OldIrql);
    return Status;
}

VOID
MiDeleteVlmAddressSpace (
    IN PEPROCESS Process
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
    PMMVAD Vad;
    PMMPTE PointerPde;
    PMMPTE PointerPte1;
    PMMPTE PointerPte2;
    PMMPFN Pfn1;
    MMPTE PteContents;
    KIRQL OldIrql;
    PCONTROL_AREA ControlArea;
    ULONG Type;
    PMI_PROCESS_VLM_INFO VlmInfo;
    SIZE_T CommitReduction;
    ULONGLONG RegionSize;

    Vad = (PMMVAD)Process->CloneRoot;

    if (Vad != (PMMVAD)NULL) {
        VlmInfo = (PMI_PROCESS_VLM_INFO) (((PMMPTE)PDE_BASE64)->u.Long);

        do {

            MiRemoveVad64 (Vad);

            Type = DELETE_TYPE_PRIVATE;
            if (Vad->u.VadFlags.PrivateMemory == 0) {
                Type = DELETE_TYPE_SHARED;
            }
            CommitReduction = MiDecommitOrDeletePages64 (
                           MiGetPteAddress64 (MI_VPN_TO_VA64 (Vad->StartingVpn)),
                           MiGetPteAddress64 (MI_VPN_TO_VA_ENDING64 (Vad->EndingVpn)),
                           Process,
                           Type,
                           FALSE);


            RegionSize = (((ULONGLONG)(Vad->EndingVpn - Vad->StartingVpn + 1)) << PAGE_SHIFT);
            VlmInfo->VirtualSize -= RegionSize;

            ExAcquireFastLock (&MiVlmStatisticsLock, &OldIrql);
            if (Type != DELETE_TYPE_SHARED) {
                MiVlmCommitChargeInPages -= CommitReduction;
            }
            ExReleaseFastLock (&MiVlmStatisticsLock, OldIrql);

            if (Type == DELETE_TYPE_SHARED) {

                //
                // Decrement the count of the number of views for the
                // Segment object.  This requires the PFN mutex to be held (it is already).
                //

                ControlArea = Vad->ControlArea;

                LOCK_PFN (OldIrql);
                ControlArea->NumberOfMappedViews -= 1;
                ControlArea->NumberOfUserReferences -= 1;

                //
                // Check to see if the control area (segment) should be deleted.
                // This routine releases the PFN lock.
                //

                MiCheckControlArea (ControlArea, Process, OldIrql);
            }
            else {
                VlmInfo->CommitCharge -= CommitReduction;
            }

            ExFreePool (Vad);
            Vad = (PMMVAD)Process->CloneRoot;

        } while (Vad != (PMMVAD)NULL);
    }

    //
    // Delete any page directory pages.
    //

    PointerPde = MiGetPdeAddress64 (MM_LOWEST_USER_ADDRESS64);
    PointerPte1 = MiGetPteAddress (PointerPde);
    PointerPde = MiGetPdeAddress64 (MM_HIGHEST_USER_ADDRESS64);
    PointerPte2 = MiGetPteAddress (PointerPde);

    //
    // Free the use count allocation stashed in the PDE_BASE64.
    //

    if (PointerPte1->u.Hard.Valid) {
        ASSERT (MiGetPteAddress (PDE_BASE64) == PointerPte1);
        ASSERT ((PMMPTE)PDE_BASE64 != PointerPde);
        VlmInfo = (PMI_PROCESS_VLM_INFO) (((PMMPTE)PDE_BASE64)->u.Long);

#if DBG
        ASSERT (VlmInfo->CommitCharge == 0);
        ASSERT (VlmInfo->VirtualSize == 0);

        {
            ULONG i;
            PUSHORT UsedPageTableEntries;

            UsedPageTableEntries = VlmInfo->UsedPageTableEntries;

            for (i = 0; i < MM_USER_PAGE_TABLE_PAGES64; i += 1) {
                ASSERT ((*UsedPageTableEntries) == 0);
                UsedPageTableEntries += 1;
            }
        }
#endif

        ExFreePool (VlmInfo);
    }

    do {

        PteContents = *PointerPte1;
        if (PteContents.u.Hard.Valid) {

            Pfn1 = MI_PFN_ELEMENT (PteContents.u.Hard.PageFrameNumber);

            //
            // Decrement the share and valid counts of the page table
            // page which maps this PTE.
            //

            LOCK_PFN (OldIrql);
            MiDecrementShareAndValidCount (Pfn1->PteFrame);

            MI_SET_PFN_DELETED (Pfn1);

            //
            // Decrement the share count for the physical page.  As the page
            // is private it will be put on the free list.
            //

            MiDecrementShareCountOnly(MI_GET_PAGE_FRAME_FROM_PTE(&PteContents));
            ASSERT (Pfn1->u3.e2.ReferenceCount == 0);
            UNLOCK_PFN (OldIrql);
        }
        PointerPte1 += 1;

    } while (PointerPte1 <= PointerPte2);

    return;
}

VOID
MiFlushPteList64 (
    IN PMMPTE_FLUSH_LIST64 PteFlushList,
    IN ULONG AllProcessors,
    IN MMPTE FillPte
    )

/*++

Routine Description:

    This routine flushes all the PTEs in the pte flush list.
    Is the list has overflowed, the entire TB is flushed.

Arguments:

    PteFlushList - Supplies an optional pointer to the list to be flushed.

    AllProcessors - Supplies TRUE if the flush occurs on all processors.

    FillPte - Supplies the PTE to fill with.

Return Value:

    None.

Environment:

    Kernel mode, PFN lock held.

--*/

{
    ULONG count;
    ULONG i = 0;

    ASSERT (ARGUMENT_PRESENT (PteFlushList));
    MM_PFN_LOCK_ASSERT ();

    count = PteFlushList->Count;

    if (count != 0) {
        if (count != 1) {
            if (count < MM_MAXIMUM_FLUSH_COUNT) {
                KeFlushMultipleTb64 (count,
                                     &PteFlushList->FlushVpn[0],
                                     TRUE,
                                     (BOOLEAN)AllProcessors,
                                     &((PHARDWARE_PTE)PteFlushList->FlushPte[0]),
                                     FillPte.u.Flush);
            } else {

                //
                // Array has overflowed, flush the entire TB.
                //

                if (AllProcessors == TRUE) {
                    MiLockSystemSpaceAtDpcLevel();
                    KeFlushEntireTb (TRUE, TRUE);
                    MmFlushCounter = (MmFlushCounter + 1) & MM_FLUSH_COUNTER_MASK;
                    MiUnlockSystemSpaceFromDpcLevel();
                } else {
                    KeFlushEntireTb (TRUE, FALSE);
                }
            }
        } else {
            KeFlushSingleTb64 (PteFlushList->FlushVpn[0],
                               TRUE,
                               (BOOLEAN)AllProcessors,
                               (PHARDWARE_PTE)PteFlushList->FlushPte[0],
                               FillPte.u.Flush);
        }
        PteFlushList->Count = 0;
    }
    return;
}

BOOLEAN
MmIsAddressValid64 (
    IN PVOID64 VirtualAddress
    )

/*++

Routine Description:

    For a given virtual address this function returns TRUE if no page fault
    will occur for a read operation on the address, FALSE otherwise.

    Note that after this routine was called, if appropriate locks are not
    held, a non-faulting address could fault.

Arguments:

    VirtualAddress - Supplies the virtual address to check.

Return Value:

    TRUE if a no page fault would be generated reading the virtual address,
    FALSE otherwise.

Environment:

    Kernel mode.

--*/

{
    PMMPTE PointerPte;
    PMMPTE PointerPde;

    PointerPde = MiGetPdeAddress64 (VirtualAddress);

    //
    // Get Page Directory Page.
    //

    PointerPte = MiGetPteAddress (PointerPde);
    if (PointerPte->u.Hard.Valid == 0) {
        return FALSE;
    }

    if (PointerPde->u.Hard.Valid == 0) {
        return FALSE;
    }

    PointerPte = MiGetPteAddress64 (VirtualAddress);
    if (PointerPte->u.Hard.Valid == 0) {
        return FALSE;
    }
    return TRUE;
}

NTSTATUS
NtMapViewOfVlmSection (
    IN HANDLE SectionHandle,
    IN HANDLE ProcessHandle,
    IN OUT PVOID64 *BaseAddress,
    IN OUT PULONGLONG SectionOffset OPTIONAL,
    IN OUT PULONGLONG ViewSize,
    IN ULONG AllocationType,
    IN ULONG Protect
    )

/*++

Routine Description:

    This function maps a view in the specified subject process to
    the section object.

Arguments:

    SectionHandle - Supplies an open handle to a section object.

    ProcessHandle - Supplies an open handle to a process object.

    BaseAddress - Supplies a pointer to a variable that will receive
                  the base address of the view. If the initial value
                  of this argument is not null, then the view will
                  be allocated starting at the specified virtual
                  address rounded down to the next 64kb address
                  boundary. If the initial value of this argument is
                  null, then the operating system will determine
                  where to allocate the view using the information
                  specified by the ZeroBits argument value and the
                  section allocation attributes (i.e. based and
                  tiled).

    SectionOffset - Supplies the offset from the beginning of the
                    section to the view in bytes. This value is
                    rounded down to the next host page size boundary.

    ViewSize - Supplies a pointer to a variable that will receive
               the actual size in bytes of the view. If the value
               of this argument is zero, then a view of the
               section will be mapped starting at the specified
               section offset and continuing to the end of the
               section. Otherwise the initial value of this
               argument specifies the size of the view in bytes
               and is rounded up to the next host page size
               boundary.

    AllocationType - Supplies the type of allocation.  Only 0 is valid.

    Protect - Supplies the protection desired for the region of
              initially committed pages.

        Protect Values

         PAGE_NOACCESS - No access to the committed region
                         of pages is allowed. An attempt to read,
                         write, or execute the committed region
                         results in an access violation (i.e. a GP
                         fault).

         PAGE_READONLY - Read only and execute access to the
                         committed region of pages is allowed. An
                         attempt to write the committed region results
                         in an access violation.

         PAGE_READWRITE - Read, write, and execute access to
                          the region of committed pages is allowed. If
                          write access to the underlying section is
                          allowed, then a single copy of the pages are
                          shared. Otherwise the pages are shared read
                          only/copy on write.

Return Value:

    Returns the status.

--*/

{
    PSECTION Section;
    PEPROCESS Process;
    KPROCESSOR_MODE PreviousMode;
    NTSTATUS Status;
    PVOID64 CapturedBase;
    ULONGLONG CapturedViewSize;
    ULONGLONG TempViewSize;
    ULONGLONG CapturedOffset;
    ACCESS_MASK DesiredSectionAccess;
    ULONG ProtectMaskForAccess;
    PCONTROL_AREA ControlArea;
    LOGICAL Attached;
    ULONG ReleasedWsMutex;

    PAGED_CODE();

    Attached = FALSE;

    if (MI_VLM_ENABLED() == FALSE) {
        return STATUS_NOT_IMPLEMENTED;
    }

    //
    // Check the allocation type field.
    //

    if (AllocationType != 0) {
        return STATUS_INVALID_PARAMETER_6;
    }

    //
    // Check the protection field.  This could raise an exception.
    //

    try {
        ProtectMaskForAccess = MiMakeProtectionMask (Protect) & 0x7;
    } except (EXCEPTION_EXECUTE_HANDLER) {
        return GetExceptionCode();
    }

    DesiredSectionAccess = MmMakeSectionAccess[ProtectMaskForAccess];

    PreviousMode = KeGetPreviousMode();

    //
    // Establish an exception handler, probe the specified addresses
    // for write access and capture the initial values.
    //

    CapturedOffset = 0;
    try {
        if (PreviousMode != KernelMode) {
            ProbeForWrite (BaseAddress, sizeof(PVOID64), sizeof(PVOID64));
            ProbeForWrite (ViewSize, sizeof(ULONGLONG), sizeof(ULONGLONG));
            if (ARGUMENT_PRESENT (SectionOffset)) {
                if (PreviousMode != KernelMode) {
                    ProbeForWrite (SectionOffset,
                                   sizeof(ULONGLONG),
                                   sizeof(ULONGLONG));
                }
            }
        }

        if (ARGUMENT_PRESENT (SectionOffset)) {
            CapturedOffset = *SectionOffset;
        }

        //
        // Capture the base address.
        //

        CapturedBase = *BaseAddress;

        //
        // Capture the region size.
        //

        CapturedViewSize = *ViewSize;

    } except (ExSystemExceptionFilter()) {

        //
        // If an exception occurs during the probe or capture
        // of the initial values, then handle the exception and
        // return the exception code as the status value.
        //

        return GetExceptionCode();
    }

    if ((ARGUMENT_PRESENT (SectionOffset)) &&
        ((CapturedOffset & (X64K - 1)) != 0)) {
        return STATUS_MAPPED_ALIGNMENT;
    }

    if (((ULONGLONG)CapturedBase & (X64K - 1)) != 0) {
        return STATUS_MAPPED_ALIGNMENT;
    }

    //
    // Make sure the specified starting and ending addresses are
    // within the user part of the virtual address space.
    //

    if ((CapturedBase != NULL ) &&
        ((CapturedBase > (PVOID64)MM_HIGHEST_USER_ADDRESS64) ||
        (CapturedBase < (PVOID64)MM_LOWEST_USER_ADDRESS64))) {

        //
        // Invalid base address.
        //

        return STATUS_INVALID_PARAMETER_3;
    }

    if (CapturedBase != NULL) {

        if ((ULONGLONG)CapturedBase + CapturedViewSize < (ULONGLONG)CapturedBase) {

            //
            // the requested region wraps
            //

            return STATUS_INVALID_PARAMETER_5;
        }


        if ((ULONGLONG)CapturedBase + CapturedViewSize > (ULONGLONG)MM_HIGHEST_USER_ADDRESS64 - X64K) {

            //
            // the requested region goes beyond the end of user memory - flag it
            //

            return STATUS_INVALID_PARAMETER_5;
        }
    }
    else if (CapturedViewSize > (ULONGLONG)MM_LARGEST_VLM_RANGE - X64K) {

        //
        // Invalid view size;
        //

        return STATUS_INVALID_PARAMETER_5;
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
    // Reference the section object, if a view is mapped to the section
    // object, the object is not dereferenced as the virtual address
    // descriptor contains a pointer to the section object.
    //

    Status = ObReferenceObjectByHandle ( SectionHandle,
                                         DesiredSectionAccess,
                                         MmSectionObjectType,
                                         PreviousMode,
                                         (PVOID *)&Section,
                                         NULL );

    if (!NT_SUCCESS(Status)) {
        goto ErrorReturn1;
    }

    if (Section->Segment->ControlArea->u.Flags.Vlm == 0) {

        //
        // This is not a VLM section.
        //

        Status = STATUS_INVALID_VLM_OPERATION;
        goto ErrorReturn;
    }

    //
    // Check to make sure the section offset is within the section.
    //

    if ((CapturedOffset + CapturedViewSize) >
                                (ULONGLONG)Section->SizeOfSection.QuadPart) {
        Status = STATUS_INVALID_VIEW_SIZE;
        goto ErrorReturn;
    }

    if (CapturedViewSize == 0) {

        //
        // Set the view size to be size of the section less the offset.
        //

        TempViewSize = Section->SizeOfSection.QuadPart -
                                                CapturedOffset;

        CapturedViewSize = TempViewSize;

        if ((TempViewSize == 0) ||
            (((ULONGLONG)MM_HIGHEST_VAD_ADDRESS64 - (ULONGLONG)CapturedBase) <
                                                        CapturedViewSize)) {

            //
            // Invalid region size;
            //

            Status = STATUS_INVALID_VIEW_SIZE;
            goto ErrorReturn;
        }

    } else {

        //
        // Check to make sure the view size plus the offset is less
        // than the size of the section.
        //

        if ((CapturedViewSize + CapturedOffset) >
                     (ULONGLONG)Section->SizeOfSection.QuadPart) {

            Status = STATUS_INVALID_VIEW_SIZE;
            goto ErrorReturn;
        }
    }

    ControlArea = Section->Segment->ControlArea;

    if (PsGetCurrentProcess() != Process) {
        KeAttachProcess (&Process->Pcb);
        Attached = TRUE;
    }

    //
    // Get the address creation mutex to block multiple threads
    // creating or deleting address space at the same time.
    //

    LOCK_ADDRESS_SPACE (Process);

    //
    // Make sure the address space was not deleted, if so, return an error.
    //

    if (Process->AddressSpaceDeleted != 0) {
        UNLOCK_ADDRESS_SPACE (Process);
        Status = STATUS_PROCESS_IS_TERMINATING;
        goto ErrorReturn;
    }

    //
    // Don't allow POSIX apps to do VLM operations.
    //

    if ((Process->CloneRoot != NULL) &&
         (Process->ForkWasSuccessful != MM_NO_FORK_ALLOWED)) {
        UNLOCK_ADDRESS_SPACE (Process);
        Status = STATUS_INVALID_VLM_OPERATION;
        goto ErrorReturn;
    }
    Process->ForkWasSuccessful = MM_NO_FORK_ALLOWED;

    ReleasedWsMutex = FALSE;

    Status = MiMapViewOfVlmDataSection (ControlArea,
                                     Process,
                                     &CapturedBase,
                                     &CapturedOffset,
                                     &CapturedViewSize,
                                     Section,
                                     ProtectMaskForAccess,
                                     AllocationType,
                                     &ReleasedWsMutex);

    if (!ReleasedWsMutex) {
        UNLOCK_WS (Process);
    }
    UNLOCK_ADDRESS_SPACE (Process);

    if (Attached == TRUE) {
        KeDetachProcess();
        Attached = FALSE;
    }

    if (!NT_SUCCESS(Status) ) {
        goto ErrorReturn;
    }

    //
    // Establish an exception handler and write the size and base
    // address.
    //

    try {

        *ViewSize = CapturedViewSize;
        *BaseAddress = CapturedBase;

        if (ARGUMENT_PRESENT(SectionOffset)) {
            *SectionOffset = CapturedOffset;
        }

    } except (EXCEPTION_EXECUTE_HANDLER) {
        NOTHING;
    }

ErrorReturn:
    if (Attached == TRUE) {
        KeDetachProcess();
        Attached = FALSE;
    }

    ObDereferenceObject (Section);
ErrorReturn1:
    ObDereferenceObject (Process);
    return Status;
}

NTSTATUS
MiMapViewOfVlmDataSection (
    IN PCONTROL_AREA ControlArea,
    IN PEPROCESS Process,
    IN PVOID64 *CapturedBase,
    IN PULONGLONG SectionOffset,
    IN PULONGLONG CapturedViewSize,
    IN PSECTION Section,
    IN ULONG ProtectionMask,
    IN ULONG AllocationType,
    IN PULONG ReleasedWsMutex
    )

/*++

Routine Description:

    This routine maps the specified physical section into the
    specified process's address space.

Arguments:

    See NtMapViewOfVlmSection above...

    ControlArea - Supplies the control area for the section.

    Process - Supplies the process pointer which is receiving the section.

    ProtectionMask - Supplies the initial page protection-mask.

    ReleasedWsMutex - Supplies FALSE. If the working set mutex is
                      not held when returning this must be set to TRUE
                      so the caller will release the mutex.

Return Value:

    Status of the map view operation.

Environment:

    Kernel Mode, working set mutex and address creation mutex held.

--*/

{
    PMMVAD Vad;
    volatile PVOID64 Va;
    PVOID64 StartingAddress;
    PVOID64 EndingAddress;
    KIRQL OldIrql;
    PSUBSECTION Subsection;
    ULONG PteOffset;
    PMMPTE PointerPte;
    PMMPTE LastPte;
    MMPTE TempPte;
    ULONG QuotaCharge;
    PMMPTE TheFirstPrototypePte;
    PMMPTE Pde;
    PMMPTE LastPde;
    LOGICAL PdeBuilt;
    NTSTATUS ReturnStatus;
    PMI_PROCESS_VLM_INFO VlmInfo;

    //
    // Check to see if a purge operation is in progress and if so, wait
    // for the purge to complete.  In addition, up the count of mapped
    // views for this control area.
    //

    if ((Process->CloneRoot != NULL) &&
         (Process->ForkWasSuccessful != MM_NO_FORK_ALLOWED)) {
        *ReleasedWsMutex = TRUE;
        return STATUS_INVALID_VLM_OPERATION;
    }

    QuotaCharge = 0;

    Process->ForkWasSuccessful = MM_NO_FORK_ALLOWED;

    MiCheckPurgeAndUpMapCount (ControlArea);

    //
    // Calculate the first prototype PTE field in the Vad.
    //

    ASSERT (ControlArea->u.Flags.GlobalOnlyPerSession == 0);

    Subsection = (PSUBSECTION)(ControlArea + 1);

    *SectionOffset = (ULONGLONG)MI_ALIGN_TO_SIZE64 (*SectionOffset, X64K);
    PteOffset = (ULONG)(*SectionOffset >> PAGE_SHIFT);

    //
    // Make sure the PTEs are not in the extended part of the
    // segment.
    //

    while (PteOffset >= Subsection->PtesInSubsection) {
        PteOffset -= Subsection->PtesInSubsection;
        Subsection = Subsection->NextSubsection;
        ASSERT (Subsection != NULL);
    }

    TheFirstPrototypePte = &Subsection->SubsectionBase[PteOffset];

//fixfix - make copy on write invalid    CapturedCopyOnWrite = Section->u.Flags.CopyOnWrite;

    LOCK_WS (Process);

    if (*CapturedBase == NULL) {

        //
        // The section is not based, find an empty range.
        // This could raise an exception.

        try {

            //
            // Find a starting address on a 64k boundary.
            //

            StartingAddress = MiFindEmptyAddressRangeInTree64 (
                               *CapturedViewSize,
                               X64K,
                               (PMMADDRESS_NODE)(Process->CloneRoot));

        } except (EXCEPTION_EXECUTE_HANDLER) {

            LOCK_PFN (OldIrql);
            ControlArea->NumberOfMappedViews -= 1;
            ControlArea->NumberOfUserReferences -= 1;
            UNLOCK_PFN (OldIrql);
            return GetExceptionCode();
        }

        EndingAddress = (PVOID64)(((ULONGLONG)StartingAddress +
                                   *CapturedViewSize - 1L) | (PAGE_SIZE - 1L));

    } else {

        StartingAddress = MI_ALIGN_TO_SIZE64 (*CapturedBase, X64K);

        //
        // Check to make sure the specified base address to ending address
        // is currently unused.
        //

        EndingAddress = (PVOID64)(((ULONGLONG)StartingAddress +
                                   *CapturedViewSize - 1L) | (PAGE_SIZE - 1L));

        Vad = MiCheckForConflictingVad64 (StartingAddress, EndingAddress);
        if (Vad != (PMMVAD)NULL) {
            LOCK_PFN (OldIrql);
            ControlArea->NumberOfMappedViews -= 1;
            ControlArea->NumberOfUserReferences -= 1;
            UNLOCK_PFN (OldIrql);
            return STATUS_CONFLICTING_ADDRESSES;
        }
    }

    //
    // An unoccupied address range has been found, build the virtual
    // address descriptor to describe this range.
    //

    try  {

        Vad = ExAllocatePoolWithTag (NonPagedPool,
                                     sizeof(MMVAD),
                                     MMVADKEY);
        if (Vad == NULL) {
            ExRaiseStatus (STATUS_INSUFFICIENT_RESOURCES);
        }
        RtlZeroMemory (Vad, sizeof(MMVAD));

        Vad->StartingVpn = MI_VA_TO_VPN64 (StartingAddress);
        Vad->EndingVpn = MI_VA_TO_VPN64 (EndingAddress);
        Vad->FirstPrototypePte = TheFirstPrototypePte;

        //
        // The protection is in the PTE template field of the VAD.
        //

        Vad->ControlArea = ControlArea;

        Vad->u2.VadFlags2.Inherit = 0;
        Vad->u.VadFlags.Protection = ProtectionMask;
        Vad->u2.VadFlags2.CopyOnWrite = 0; //fixfix

        Vad->u2.VadFlags2.FileOffset = (ULONG)(*SectionOffset >> 16);

        PteOffset += (ULONG) (Vad->EndingVpn - Vad->StartingVpn);

        if (PteOffset < Subsection->PtesInSubsection ) {
            Vad->LastContiguousPte = &Subsection->SubsectionBase[PteOffset];

        } else {
            Vad->LastContiguousPte = &Subsection->SubsectionBase[
                                        (Subsection->PtesInSubsection - 1) +
                                        Subsection->UnusedPtes];
        }

        ASSERT (Vad->FirstPrototypePte <= Vad->LastContiguousPte);
        MiInsertVad64 (Vad);

    } except (EXCEPTION_EXECUTE_HANDLER) {

        LOCK_PFN (OldIrql);
        ControlArea->NumberOfMappedViews -= 1;
        ControlArea->NumberOfUserReferences -= 1;
        UNLOCK_PFN (OldIrql);

        if (Vad != (PMMVAD)NULL) {

            //
            // The pool allocation succeeded, but the quota charge
            // in InsertVad failed, deallocate the pool and return
            // an error.
            //

            ExFreePool (Vad);
            return GetExceptionCode();
        }
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Build the top level page directory page to describe this range.
    // The individual page table pages are materialized when the process
    // faults.
    //

    Pde = MiGetPdeAddress64 (StartingAddress);
    PdeBuilt = MiMakePdeExistAndMakeValid64 (Pde, Process, FALSE, FALSE);

    if (PdeBuilt == FALSE) {
        ASSERT (Pde == MiGetPdeAddress64 (StartingAddress));
        LOCK_PFN (OldIrql);
        ControlArea->NumberOfMappedViews -= 1;
        ControlArea->NumberOfUserReferences -= 1;
        UNLOCK_PFN (OldIrql);
        MiRemoveVad64 (Vad);
        ExFreePool (Vad);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    *ReleasedWsMutex = TRUE;
    UNLOCK_WS (Process);

    //
    // Update the current virtual size in the process header.
    //

    *CapturedViewSize = (char * POINTER_64)EndingAddress - (char * POINTER_64)StartingAddress + 1L;

    VlmInfo = (PMI_PROCESS_VLM_INFO) (((PMMPTE)PDE_BASE64)->u.Long);
    VlmInfo->VirtualSize += *CapturedViewSize;
    if (VlmInfo->VirtualSize > VlmInfo->VirtualSizePeak) {
        VlmInfo->VirtualSizePeak = VlmInfo->VirtualSize;
    }

    *CapturedBase = StartingAddress;

    return STATUS_SUCCESS;
}

NTSTATUS
NtUnmapViewOfVlmSection(
    IN HANDLE ProcessHandle,
    IN OUT PVOID64 *BaseAddress
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
    PMMVAD Vad;
    ULONGLONG RegionSize;
    PVOID64 UnMapImageBase;
    PVOID64 CapturedBase;
    NTSTATUS status;
    PMMPTE StartingPte;
    PMMPTE EndingPte;
    PCONTROL_AREA ControlArea;
    KIRQL OldIrql;
    ULONG CommitReduction;
    PMI_PROCESS_VLM_INFO VlmInfo;

    if (MI_VLM_ENABLED() == FALSE) {
        return STATUS_NOT_IMPLEMENTED;
    }

    PreviousMode = KeGetPreviousMode();

    status = ObReferenceObjectByHandle ( ProcessHandle,
                                         PROCESS_VM_OPERATION,
                                         PsProcessType,
                                         PreviousMode,
                                         (PVOID *)&Process,
                                         NULL );
    if (!NT_SUCCESS(status)) {
        return status;
    }

    UnMapImageBase = NULL;

    //
    // If the specified process is not the current process, attach
    // to the specified process.
    //

    KeAttachProcess (&Process->Pcb);

    //
    // Probe and capture the specified address
    //

    PreviousMode = KeGetPreviousMode();

    if (PreviousMode != KernelMode) {
        try {
            ProbeForRead (BaseAddress, sizeof(PVOID64), sizeof(PVOID64));

            //
            // Capture the base address.
            //

            CapturedBase = *BaseAddress;
        } except (ExSystemExceptionFilter()) {

            //
            // If an exception occurs during the probe or capture
            // of the initial value, then handle the exception and
            // return the exception code as the status value.
            //

            KeDetachProcess();
            ObDereferenceObject (Process);
            return GetExceptionCode();
        }
    }
    else {
        CapturedBase = *BaseAddress;
    }

    //
    // Get the address creation mutex to block multiple threads from
    // creating or deleting address space at the same time and
    // get the working set mutex so virtual address descriptors can
    // be inserted and walked.  Raise IRQL to block APCs.
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

    if (Process->ForkWasSuccessful != MM_NO_FORK_ALLOWED) {
        status = STATUS_NOT_MAPPED_VIEW;
        goto ErrorReturn;
    }

    //
    // Find the associated Vad.
    //

    Vad = (PMMVAD)MiLocateAddress64 (MI_VA_TO_VPN64 (CapturedBase));

    if ((Vad == (PMMVAD)NULL) || (Vad->u.VadFlags.PrivateMemory)) {

        //
        // No Virtual Address Descriptor located for Base Address.
        //

        status = STATUS_NOT_MAPPED_VIEW;
        goto ErrorReturn;
    }

    ASSERT (Vad->u.VadFlags.NoChange == 0);

    RegionSize = PAGE_SIZE + (((ULONGLONG)(Vad->EndingVpn - Vad->StartingVpn)) << PAGE_SHIFT);

    MiRemoveVad64 (Vad);

#if 0
    //
    // Return commitment for page table pages if possible.
    //

    MiReturnPageTablePageCommitment (MI_VPN_TO_VA (Vad->StartingVpn),
                                     MI_VPN_TO_VA_ENDING (Vad->EndingVpn),
                                     Process,
                                     PreviousVad,
                                     NextVad);
#endif //0

    StartingPte = MiGetPteAddress64 (MI_VPN_TO_VA64 (Vad->StartingVpn));
    EndingPte = MiGetPteAddress64 (MI_VPN_TO_VA64 (Vad->EndingVpn));

    CommitReduction = MiDecommitOrDeletePages64 (StartingPte,
                                                 EndingPte,
                                                 Process,
                                                 DELETE_TYPE_SHARED,
                                                 TRUE);


    //
    // Decrement the count of the number of views for the
    // Segment object.  This requires the PFN mutex to be held (it is already).
    //

    ControlArea = Vad->ControlArea;
    LOCK_PFN (OldIrql);
    ControlArea->NumberOfMappedViews -= 1;
    ControlArea->NumberOfUserReferences -= 1;

    //
    // Check to see if the control area (segment) should be deleted.
    // This routine releases the PFN lock.
    //

    MiCheckControlArea (ControlArea, Process, OldIrql);

    ExFreePool (Vad);

    //
    // Update the current virtual size in the process header.
    //

    VlmInfo = (PMI_PROCESS_VLM_INFO) (((PMMPTE)PDE_BASE64)->u.Long);
    VlmInfo->VirtualSize -= RegionSize;

    status = STATUS_SUCCESS;

ErrorReturn:

    UNLOCK_WS (Process);
    UNLOCK_ADDRESS_SPACE (Process);

    KeDetachProcess();
    ObDereferenceObject (Process);

    return status;
}


NTSTATUS
NtProtectVirtualMemory64(
    IN HANDLE ProcessHandle,
    IN OUT PVOID64 *BaseAddress,
    IN OUT PULONGLONG RegionSize,
    IN ULONG NewProtect,
    OUT PULONG OldProtect
    )

{
    PEPROCESS Process;
    KPROCESSOR_MODE PreviousMode;
    NTSTATUS Status;
    LOGICAL Attached;
    PVOID64 CapturedBase;
    ULONGLONG CapturedRegionSize;
    ULONG ProtectionMask;
    PMMVAD FoundVad;
    PVOID64 StartingAddress;
    PVOID64 EndingAddress;
    PMMPTE PointerPte;
    PMMPTE LastPte;
    PMMPTE PointerPde;
    PMMPTE PointerProtoPte;
    PMMPTE LastProtoPte;
    PMMPFN Pfn1;
    ULONG CapturedOldProtect;
    MMPTE TempPte;
    MMPTE PteContents;
    PVOID64 Va;
    ULONG DoAgain;
    PMI_PROCESS_VLM_INFO VlmInfo;
    PMMPTE LowestPde;

    PAGED_CODE();

    Attached = FALSE;

    //
    // Check the protection field.  This could raise an exception.
    //

    if ((NewProtect & PAGE_GUARD) ||
        (NewProtect & PAGE_NOCACHE)) {
        return STATUS_INVALID_PAGE_PROTECTION;
    }

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

            ProbeForWrite (BaseAddress, sizeof(PVOID64), sizeof(PVOID64));
            ProbeForWrite (RegionSize, sizeof(ULONGLONG), sizeof(ULONGLONG));
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

    //
    // Make sure the specified starting and ending addresses are
    // within the user part of the virtual address space.
    //

    if (CapturedBase > (PVOID64)MM_HIGHEST_USER_ADDRESS64) {

        //
        // Invalid base address.
        //

        return STATUS_INVALID_PARAMETER_2;
    }

    if ((ULONGLONG)MM_HIGHEST_USER_ADDRESS64 - (ULONGLONG)CapturedBase <
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


    //
    // Get the address creation mutex to block multiple threads from
    // creating or deleting address space at the same time.
    // Get the working set mutex so PTEs can be modified.
    // Block APCs so an APC which takes a page
    // fault does not corrupt various structures.
    //

    LOCK_WS_AND_ADDRESS_SPACE (Process);

    //
    // Make sure the address space was not deleted, if so, return an error.
    //

    if (Process->AddressSpaceDeleted != 0) {
        Status = STATUS_PROCESS_IS_TERMINATING;
        goto ErrorFound;
    }

    EndingAddress = (PVOID64)(((ULONGLONG)CapturedBase + CapturedRegionSize - 1) |
                        (PAGE_SIZE - 1));
    StartingAddress = (PVOID64)PAGE_ALIGN64(CapturedBase);

    FoundVad = MiCheckForConflictingVad64 (StartingAddress, EndingAddress);

    if (FoundVad == NULL) {

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

    if ((MI_VA_TO_VPN64 (StartingAddress) < FoundVad->StartingVpn) ||
        (MI_VA_TO_VPN64 (EndingAddress) > FoundVad->EndingVpn)) {

        //
        // Not within the section virtual address descriptor,
        // return an error.
        //

        Status = STATUS_CONFLICTING_ADDRESSES;
        goto ErrorFound;
    }

    ASSERT (FoundVad->u.VadFlags.PhysicalMapping == 0);
    ASSERT (FoundVad->u.VadFlags.NoChange == 0);

    if (FoundVad->u.VadFlags.PrivateMemory == 0) {


        //
        // For mapped sections, the NO_CACHE and the COPY_ON_WRITE attribute
        // are not allowed.
        //

        if ((NewProtect & PAGE_NOCACHE) ||
            (ProtectionMask & MM_COPY_ON_WRITE_MASK)) {

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
                                        MI_VA_TO_VPN64 (StartingAddress));
            LastProtoPte = MiGetProtoPteAddress (FoundVad,
                                        MI_VA_TO_VPN64 (EndingAddress));

            //
            // Release the working set mutex and acquire the section
            // commit mutex.  Check all the prototype PTEs described by
            // the virtual address range to ensure they are committed.
            //

            UNLOCK_WS (Process);
            ExAcquireFastMutex (&MmSectionCommitMutex);

            while (PointerProtoPte <= LastProtoPte) {

                //
                // Check to see if the prototype PTE is committed, if
                // not return an error.
                //

                if (PointerProtoPte->u.Long == 0) {

                    //
                    // Error, this prototype PTE is not committed.
                    //

                    ExReleaseFastMutex (&MmSectionCommitMutex);
                    Status = STATUS_NOT_COMMITTED;
                    goto ErrorFoundNoWs;
                }
                PointerProtoPte += 1;
            }

            //
            // The range is committed, release the section commitment
            // mutex, acquire the working set mutex and update the local PTEs.
            //

            ExReleaseFastMutex (&MmSectionCommitMutex);

            //
            // Set the protection on the section pages.  This could
            // get a quota exceeded exception.
            //

            LOCK_WS (Process);
        }

        //
        // Set the protection on the pages.
        //

        //
        // The address range is committed, change the protection.
        //

        PointerPde = MiGetPdeAddress64 (StartingAddress);
        PointerPte = MiGetPteAddress64 (StartingAddress);
        LastPte = MiGetPteAddress64 (EndingAddress);

        MiMakePdeExistAndMakeValid(PointerPde, Process, FALSE);

        //
        // Capture the protection for the first page.
        //

        if (PointerPte->u.Long != 0) {

            CapturedOldProtect = PAGE_READWRITE;
            if (PointerPte->u.Hard.Valid == 1) {
                if (PointerPte->u.Hard.Write == 0) {
                    CapturedOldProtect = PAGE_READONLY;
                }
            } else {
                ASSERT (PointerPte->u.Soft.PageFileHigh == MI_PTE_LOOKUP_NEEDED);
                ASSERT (PointerPte->u.Soft.Prototype == 1);
                CapturedOldProtect =
                    MI_CONVERT_FROM_PTE_PROTECTION (PointerPte->u.Soft.Protection);
            }

        } else {

            //
            // Get the protection from the VAD.
            //

            CapturedOldProtect =
               MI_CONVERT_FROM_PTE_PROTECTION(FoundVad->u.VadFlags.Protection);
        }

        LowestPde = MiGetPdeAddress64 (MM_LOWEST_USER_ADDRESS64);
        VlmInfo = (PMI_PROCESS_VLM_INFO) (((PMMPTE)PDE_BASE64)->u.Long);

        //
        // For all the PTEs in the specified address range, set the
        // protection depending on the state of the PTE.
        //

        while (PointerPte <= LastPte) {

            if (MiIsPteOnPdeBoundary(PointerPte)) {

                PointerPde = MiGetPteAddress (PointerPte);

                MiMakePdeExistAndMakeValid(PointerPde, Process, FALSE);
            }

            PteContents = *PointerPte;

            if (PteContents.u.Hard.Valid == 1) {

                if (ProtectionMask != MM_NOACCESS) {

                    //
                    // Set the protection into both the PTE and the original PTE
                    // in the PFN database.
                    //

                    Pfn1 = MI_PFN_ELEMENT (PteContents.u.Hard.PageFrameNumber);

                    ASSERT (Pfn1->u3.e1.PrototypePte == 1);

                    //
                    // The PTE is a private page which is valid, if the
                    // specified protection is no-access or guard page
                    // remove the PTE from the working set.
                    //

                    MI_MAKE_VALID_PTE (TempPte,
                                       PointerPte->u.Hard.PageFrameNumber,
                                       ProtectionMask,
                                       NULL);
                    if (TempPte.u.Hard.Write) {
                        MI_SET_PTE_DIRTY (TempPte);
                    }

                    //
                    // Flush the TB as we have changed the protection
                    // of a valid PTE.
                    //

                    MiFlushTbAndCapture (PointerPte,
                                         TempPte.u.Flush,
                                         Pfn1);
                } else {

                    //
                    // No access, remove page from memory.
                    //

                    MiMakeValidPageNoAccess64 (PointerPte);
                }

            } else {

                if (PointerPte->u.Long == ZeroPte.u.Long) {
                    *PointerPte = PrototypePte;
                    PointerPte->u.Soft.Protection = ProtectionMask;

                    VlmInfo->UsedPageTableEntries[PointerPde - LowestPde] += 1;

                } else {
                    ASSERT (PointerPte->u.Soft.Prototype == 1);
                    ASSERT (PointerPte->u.Soft.PageFileHigh == MI_PTE_LOOKUP_NEEDED);
                    PointerPte->u.Soft.Protection = ProtectionMask;
                }
            }
            PointerPte += 1;
        } //end while

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

        if ( !MiIsEntireRangeCommitted64 (StartingAddress,
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

        //
        // The address range is committed, change the protection.
        //

        PointerPde = MiGetPdeAddress64 (StartingAddress);
        PointerPte = MiGetPteAddress64 (StartingAddress);
        LastPte = MiGetPteAddress64 (EndingAddress);

        MiMakePdeExistAndMakeValid(PointerPde, Process, FALSE);

        //
        // Capture the protection for the first page.
        //

        if (PointerPte->u.Long != 0) {

            CapturedOldProtect = MiGetPageProtection (PointerPte, Process);

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

            if (MiIsPteOnPdeBoundary(PointerPte)) {

                PointerPde = MiGetPteAddress (PointerPte);

                MiMakePdeExistAndMakeValid(PointerPde, Process, FALSE);
            }

            PteContents = *PointerPte;

            if (PteContents.u.Hard.Valid == 1) {

                if (ProtectionMask != MM_NOACCESS) {

                    //
                    // Set the protection into both the PTE and the original PTE
                    // in the PFN database.
                    //

                    Pfn1 = MI_PFN_ELEMENT (PteContents.u.Hard.PageFrameNumber);

                    Pfn1->OriginalPte.u.Soft.Protection = ProtectionMask;
                    MI_MAKE_VALID_PTE (TempPte,
                                       PointerPte->u.Hard.PageFrameNumber,
                                       ProtectionMask,
                                       NULL);
                    if (TempPte.u.Hard.Write) {
                        MI_SET_PTE_DIRTY (TempPte);
                    }

                    //
                    // Flush the TB as we have changed the protection
                    // of a valid PTE.
                    //

                    MiFlushTbAndCapture (PointerPte,
                                         TempPte.u.Flush,
                                         Pfn1);
                } else {

                    //
                    // No access, remove page from memory, make it transition.
                    //

                    MiMakeValidPageNoAccess64 (PointerPte);
                }
            } else {

                //
                // the page is not present.
                //

                if (PteContents.u.Soft.Transition == 1) {

                    //
                    // The only reason a PTE would be transition is if it has
                    // been made no access.  If the protection is not no access,
                    // make it valid with the right protection.  otherwise, it
                    // is already in the desired state, so don't do anything.
                    //

                    if (ProtectionMask != MM_NOACCESS) {
                        MiMakeNoAccessPageValid64 (PointerPte, ProtectionMask);
                    }

                } else {

                    //
                    // Must be page file space or demand zero.
                    //

                    PointerPte->u.Soft.Protection = ProtectionMask;
                    ASSERT (PointerPte->u.Long != 0);
                }
            }
            PointerPte += 1;
        } //end while
    }

    //
    // Common completion code.
    //

    CapturedRegionSize = (char * POINTER_64)EndingAddress - (char * POINTER_64)StartingAddress + 1L;

    Status = STATUS_SUCCESS;

ErrorFound:

    UNLOCK_WS (Process);
ErrorFoundNoWs:

    UNLOCK_ADDRESS_SPACE (Process);

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

            ProbeForWrite (BaseAddress, sizeof(PVOID64), sizeof(PVOID64));
            ProbeForWrite (RegionSize, sizeof(ULONGLONG), sizeof(ULONGLONG));
            ProbeForWriteUlong (OldProtect);
        }

        *RegionSize = CapturedRegionSize;
        *BaseAddress = StartingAddress;
        *OldProtect = CapturedOldProtect;

    } except (EXCEPTION_EXECUTE_HANDLER) {
        NOTHING;
    }

    return Status;
}

ULONG
MiIsEntireRangeCommitted64 (
    IN PVOID64 StartingAddress,
    IN PVOID64 EndingAddress,
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

    Kernel mode, APCs disabled, WorkingSetMutex and AddressCreation mutexes
    held.

--*/

{
    PMMPTE PointerPte;
    PMMPTE LastPte;
    PMMPTE PointerPde;
    LOGICAL FirstTime;
    PVOID64 Va;

    PAGED_CODE();

    FirstTime = TRUE;

    PointerPde = MiGetPdeAddress64 (StartingAddress);
    PointerPte = MiGetPteAddress64 (StartingAddress);
    LastPte = MiGetPteAddress64 (EndingAddress);

    ASSERT (Vad->u.VadFlags.PrivateMemory == 1);

    Va = StartingAddress;

    while (PointerPte <= LastPte) {

        if (MiIsPteOnPdeBoundary(PointerPte) || FirstTime) {

            //
            // This is a PDE boundary, check to see if the entire
            // PDE page exists.
            //

            FirstTime = FALSE;
            PointerPde = MiGetPteAddress (PointerPte);

            if (PointerPde->u.Long == 0) {

                //
                // No page directory entry for private memory means
                // we're not committed.
                //

                ASSERT (Vad->u.VadFlags.PrivateMemory == 1);
                return FALSE;
            }
#if 0
            while (!MiDoesPdeExistAndMakeValid(PointerPde, Process, FALSE)) {

                //
                // No PDE exists for the starting address, check the VAD
                // to see if the pages are committed.
                //

                PointerPde += 1;

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
#endif //0
        }

        //
        // The page table page exists, check each PTE for commitment.
        //

        if (PointerPte->u.Long == 0) {

            //
            // This page has not been committed, check the VAD.
            //

#if 0
            if (Vad->u.VadFlags.MemCommit == 0) {
#endif

                //
                // The entire range to be decommitted is not committed,
                // return an error.
                //

                return FALSE;
#if 0
            }
#endif
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
        Va = (PVOID64)((ULONGLONG)Va + PAGE_SIZE);
    }
    return TRUE;
}


VOID
MiMakeValidPageNoAccess64 (
    IN PMMPTE PointerPte
    )
{
    PMMPFN Pfn1;
    PMMPTE PointerPde;
    MMPTE PteContents;
    KIRQL OldIrql;
    MMPTE TempPte;
    ULONG_PTR Vpn;

    Vpn = (ULONG_PTR)((ULONGLONG)MiGetVirtualAddressMappedByPte64(PointerPte) >>
                    PAGE_SHIFT);

    TempPte = PrototypePte;
    TempPte.u.Soft.Protection = MM_NOACCESS;

    PointerPde = MiGetPteAddress (PointerPte);
    LOCK_PFN (OldIrql);

    PteContents = *PointerPte;
    Pfn1 = MI_PFN_ELEMENT (PteContents.u.Hard.PageFrameNumber);

    if (Pfn1->u3.e1.PrototypePte == 1) {

        //
        // Capture the state of the modified bit for this pte.
        //

        MI_CAPTURE_DIRTY_BIT_TO_PFN (&PteContents, Pfn1);

        //
        // Decrement the share and valid counts of the page table
        // page which maps this PTE.
        //

        MiDecrementShareAndValidCount (MI_GET_PAGE_FRAME_FROM_PTE(PointerPde));

        //
        // Decrement the share count for the physical page.
        //

        MiDecrementShareCount (MI_GET_PAGE_FRAME_FROM_PTE(&PteContents));

    } else {

        //
        // Make the PTE transition, but don't decrement the reference count
        // only the share count.
        //

        ASSERT (Pfn1->u3.e2.ReferenceCount != 0);
        Pfn1->u3.e2.ReferenceCount += 1;
        MiDecrementShareCount (MI_GET_PAGE_FRAME_FROM_PTE(&PteContents));
        TempPte = PteContents;
        MI_MAKE_VALID_PTE_TRANSITION (TempPte, MM_NOACCESS);
    }

    KeFlushSingleTb64 (Vpn,
                     TRUE,
                     FALSE,
                     (PHARDWARE_PTE)PointerPte,
                     TempPte.u.Flush);
    UNLOCK_PFN (OldIrql);
    return;
}

VOID
MiMakeNoAccessPageValid64 (
    IN PMMPTE PointerPte,
    IN ULONG Protect
    )

{
    MMPTE PteContents;
    MMPTE TempPte;
    KIRQL OldIrql;
    PMMPFN Pfn1;
    PFN_NUMBER PageFrameIndex;

    LOCK_PFN (OldIrql);
    PteContents = *PointerPte;
    PageFrameIndex = MI_GET_PAGE_FRAME_FROM_TRANSITION_PTE(&PteContents);
    Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);
    MI_MAKE_VALID_PTE (TempPte,
                       PageFrameIndex,
                       Protect,
                       NULL);
    if (TempPte.u.Hard.Write) {
        MI_SET_PTE_DIRTY (TempPte);
    }

    //
    // Note that VLM no access pages are marked as transition pages and
    // are not in any page tables.  Therefore their sharecount is zero.
    //
    // The subtlety is that the reference count on this VLM no access page
    // should never go to 0 because then the page will actually be freed
    // to the standby list - and we'd only want to do that if we were actually
    // getting rid of the page (as opposed to making it no-access).
    //
    // Thus, the ASSERT below cannot be turned on.
    //
    // ASSERT (Pfn1->u2.ShareCount == 1);
    //

    ASSERT (Pfn1->u3.e2.ReferenceCount != 0);
    Pfn1->u2.ShareCount += 1;
    *PointerPte = TempPte;
    UNLOCK_PFN (OldIrql);
    return;
}

NTSTATUS
MiGetVlmInfo (
    IN PMEMORY_VLM_COUNTERS VlmUserInfo
    )

/*++

Routine Description:

    This function returns the various VLM information values.

Arguments:

    VlmInfo - Supplies a VLM information structure to fill in.

Return Value:

    Returns the status.

Environment:

    Kernel mode.  The caller is responsible for ensuring that VLM is enabled.

--*/

{
    PMMPTE PointerPte;
    PEPROCESS TargetProcess;
    PMI_PROCESS_VLM_INFO VlmInfo;

    RtlZeroMemory (VlmUserInfo, sizeof (*VlmUserInfo));

    VlmUserInfo->SystemCommitCharge = MiVlmCommitChargeInPages;
    VlmUserInfo->SystemPeakCommitCharge = MiVlmCommitChargeInPagesPeak;
    VlmUserInfo->SystemSharedCommitCharge = MmSharedCommitVlm;
    VlmUserInfo->VirtualSizeAvailable =
        MM_HIGHEST_USER_ADDRESS64 - MM_LOWEST_USER_ADDRESS64 + 1;


    TargetProcess = PsGetCurrentProcess();

    LOCK_WS (TargetProcess);

    if (TargetProcess->AddressSpaceDeleted != 0) {
        UNLOCK_WS (TargetProcess);
        return STATUS_PROCESS_IS_TERMINATING;
    }

    PointerPte = MiGetPteAddress (PDE_BASE64);

    if (PointerPte->u.Long != 0) {

        ASSERT (PointerPte->u.Hard.Valid == 1);

        VlmInfo = (PMI_PROCESS_VLM_INFO) (((PMMPTE)PDE_BASE64)->u.Long);

        VlmUserInfo->VirtualSize = VlmInfo->VirtualSize;
        VlmUserInfo->PeakVirtualSize = VlmInfo->VirtualSizePeak;

        VlmUserInfo->VirtualSizeAvailable -= VlmInfo->VirtualSize;

        VlmUserInfo->CommitCharge = VlmInfo->CommitCharge;
        VlmUserInfo->PeakCommitCharge = VlmInfo->CommitChargePeak;
    }

    UNLOCK_WS (TargetProcess);

    return STATUS_SUCCESS;
}

NTSTATUS
NtQueryVirtualMemory64(
    IN HANDLE ProcessHandle,
    IN PVOID64 *pBaseAddress,
    IN MEMORY_INFORMATION_CLASS MemoryInformationClass,
    OUT PVOID MemoryInformation,
    IN ULONG MemoryInformationLength,
    OUT PULONG ReturnLength OPTIONAL
    )

/*++

Routine Description:

    This function provides the capability to determine the state,
    protection, and type of a region of pages within the virtual address
    space of the subject process.

    The state of the first page within the region is determined  and then
    subsequent entries in the process address map are scanned from the
    base address upward until either the entire range of pages has been
    scanned or until a page with a nonmatching set of attributes is
    encountered. The region attributes, the length of the region of pages
    with matching attributes, and an appropriate status value are
    returned.

    If the entire region of pages does not have a matching set of
    attributes, then the returned length parameter value can be used to
    calculate the address and length of the region of pages that was not
    scanned.

Arguments:


    ProcessHandle - An open handle to a process object.

    pBaseAddress - Pointer to the base address of the region of pages to be
        queried. This value is rounded down to the next host-page-
        address boundary.

        Note this is a 32-bit pointer to a 64-bit pointer. This is
        necessary because all system service arguments are canonicalized
        to 32 bits.

    MemoryInformationClass - The memory information class about which
        to retrieve information.

    MemoryInformation - A pointer to a buffer that receives the
        specified information.  The format and content of the buffer
        depend on the specified information class.


        MemoryBasicInformation - Data type is PMEMORY_BASIC_INFORMATION.

            MEMORY_BASIC_INFORMATION Structure


            ULONG RegionSize - The size of the region in bytes
                beginning at the base address in which all pages have
                identical attributes.

            ULONG State - The state of the pages within the region.

                State Values                        State Values

                MEM_COMMIT - The state of the pages within the region
                    is committed.

                MEM_FREE - The state of the pages within the region
                    is free.

                MEM_RESERVE - The state of the pages within the
                    region is reserved.

            ULONG Protect - The protection of the pages within the
                region.


                Protect Values                        Protect Values

                PAGE_NOACCESS - No access to the region of pages is
                    allowed. An attempt to read, write, or execute
                    within the region results in an access violation
                    (i.e., a GP fault).

                PAGE_EXECUTE - Execute access to the region of pages
                    is allowed. An attempt to read or write within
                    the region results in an access violation.

                PAGE_READONLY - Read-only and execute access to the
                    region of pages is allowed. An attempt to write
                    within the region results in an access violation.

                PAGE_READWRITE - Read, write, and execute access to
                    the region of pages is allowed. If write access
                    to the underlying section is allowed, then a
                    single copy of the pages are shared. Otherwise,
                    the pages are shared read-only/copy-on-write.

                PAGE_GUARD - Read, write, and execute access to the
                    region of pages is allowed; however, access to
                    the region causes a "guard region entered"
                    condition to be raised in the subject process.

                PAGE_NOCACHE - Disable the placement of committed
                    pages into the data cache.

            ULONG Type - The type of pages within the region.


                Type Values

                MEM_PRIVATE - The pages within the region are
                    private.

                MEM_MAPPED - The pages within the region are mapped
                    into the view of a section.

                MEM_IMAGE - The pages within the region are mapped
                    into the view of an image section.

    MemoryInformationLength - Specifies the length in bytes  of
        the memory information buffer.

    ReturnLength - An optional pointer which, if specified,
        receives the number of bytes placed in the process
        information buffer.


Return Value:

    Returns the status

    TBS


Environment:

    Kernel mode.

--*/

{
    KPROCESSOR_MODE PreviousMode;
    PEPROCESS TargetProcess;
    NTSTATUS Status;
    PMMVAD Vad;
    PVOID64 Va;
    BOOLEAN Found = FALSE;
    ULONGLONG TheRegionSize;
    ULONG NewProtect;
    ULONG NewState;
    ULONG_PTR BaseVpn;
    PVOID64 BaseAddress;

    MEMORY_BASIC_INFORMATION_VLM Info;

    if (MI_VLM_ENABLED() == FALSE) {
        return STATUS_NOT_IMPLEMENTED;
    }

    //
    // The only supported option is MEMORY_BASIC_INFORMATION_VLM, make
    // sure the user's buffer is large enough for this.
    //

    //
    // Check argument validity.
    //
    switch (MemoryInformationClass) {
        case MemoryBasicInformation:
                if (MemoryInformationLength < sizeof(MEMORY_BASIC_INFORMATION_VLM)) {
                    return STATUS_INFO_LENGTH_MISMATCH;
                }
                break;

        default:
            return STATUS_INVALID_INFO_CLASS;
    }

    PreviousMode = KeGetPreviousMode();

    if (PreviousMode != KernelMode) {

        //
        // Check arguments.
        //

        try {
            ProbeForRead(pBaseAddress, sizeof(PVOID64), sizeof(ULONG));
            BaseAddress = *pBaseAddress;

            ProbeForWrite(MemoryInformation,
                          MemoryInformationLength,
                          sizeof(ULONGLONG));

            if (ARGUMENT_PRESENT(ReturnLength)) {
                ProbeForWriteUlong(ReturnLength);
            }

        } except (EXCEPTION_EXECUTE_HANDLER) {

            //
            // If an exception occurs during the probe or capture
            // of the initial values, then handle the exception and
            // return the exception code as the status value.
            //

            return GetExceptionCode();
        }
    } else {
        BaseAddress = *pBaseAddress;
    }

    if (BaseAddress == (PVOID64)NULL) {
        BaseAddress = (PVOID64)MM_LOWEST_USER_ADDRESS64;
    }

    if ((BaseAddress > (PVOID64)MM_HIGHEST_USER_ADDRESS64) ||
        (BaseAddress < (PVOID64)MM_LOWEST_USER_ADDRESS64)) {
        return STATUS_INVALID_PARAMETER;
    }

    if ( ProcessHandle == NtCurrentProcess() ) {
        TargetProcess = PsGetCurrentProcess();
    } else {
        Status = ObReferenceObjectByHandle ( ProcessHandle,
                                             PROCESS_QUERY_INFORMATION,
                                             PsProcessType,
                                             PreviousMode,
                                             (PVOID *)&TargetProcess,
                                             NULL );

        if (!NT_SUCCESS(Status)) {
            return Status;
        }
    }

    //
    // If the specified process is not the current process, attach
    // to the specified process.
    //

    KeAttachProcess (&TargetProcess->Pcb);

    //
    // Get working set mutex and block APCs.
    //

    LOCK_WS_AND_ADDRESS_SPACE (TargetProcess);

    //
    // Make sure the address space was not deleted, if so, return an error.
    //

    if (TargetProcess->AddressSpaceDeleted != 0) {
        UNLOCK_WS (TargetProcess);
        UNLOCK_ADDRESS_SPACE (TargetProcess);
        KeDetachProcess();
        if ( ProcessHandle != NtCurrentProcess() ) {
            ObDereferenceObject (TargetProcess);
        }
        return STATUS_PROCESS_IS_TERMINATING;
    }

    if (TargetProcess->ForkWasSuccessful != MM_NO_FORK_ALLOWED) {
        Vad = NULL;
    } else {
        Vad = TargetProcess->CloneRoot;
    }

    //
    // Locate the VAD that contains the base address or the VAD
    // which follows the base address.
    //

    BaseVpn = MI_VA_TO_VPN64 (BaseAddress);

    for (;;) {

        if (Vad == (PMMVAD)NULL) {
            break;
        }

        if ((BaseVpn >= Vad->StartingVpn) &&
            (BaseVpn <= Vad->EndingVpn)) {
            Found = TRUE;
            break;
        }

        if (BaseVpn < Vad->StartingVpn) {
            if (Vad->LeftChild == (PMMVAD)NULL) {
                break;
            }
            Vad = Vad->LeftChild;

        } else {
            if (BaseVpn < Vad->EndingVpn) {
                break;
            }
            if (Vad->RightChild == (PMMVAD)NULL) {
                break;
            }
            Vad = Vad->RightChild;
        }
    }

    if (!Found) {

        //
        // There is no virtual address allocated at the base
        // address.  Return the size of the hole starting at
        // the base address.
        //

        if (Vad == NULL) {
            TheRegionSize = MM_LARGEST_VLM_RANGE + 1;
        } else {
            if (Vad->StartingVpn < BaseVpn) {

                //
                // We are looking at the Vad which occupies the range
                // just before the desired range.  Get the next Vad.
                //

                Vad = MiGetNextVad (Vad);
                if (Vad == NULL) {
                    TheRegionSize = (ULONGLONG)MM_HIGHEST_VAD_ADDRESS64 -
                                                (ULONGLONG)PAGE_ALIGN64(BaseAddress);
                } else {
                    TheRegionSize = (ULONGLONG)MI_VPN_TO_VA64 (Vad->StartingVpn) -
                                                (ULONGLONG)PAGE_ALIGN64(BaseAddress);
                }
            } else {
                TheRegionSize = (ULONGLONG)MI_VPN_TO_VA64 (Vad->StartingVpn) -
                                                (ULONGLONG)PAGE_ALIGN64(BaseAddress);
            }
        }

        UNLOCK_WS (TargetProcess);
        UNLOCK_ADDRESS_SPACE (TargetProcess);
        KeDetachProcess();

        if (ProcessHandle != NtCurrentProcess()) {
            ObDereferenceObject (TargetProcess);
        }

        //
        // Establish an exception handler and write the information and
        // returned length.
        //

        if (MemoryInformationClass == MemoryBasicInformation) {
            try {

                ((PMEMORY_BASIC_INFORMATION_VLM)MemoryInformation)->AllocationBase =
                                         (PVOID64)MM_LOWEST_USER_ADDRESS64;
                ((PMEMORY_BASIC_INFORMATION_VLM)MemoryInformation)->AllocationProtect =
                                                                            0;
                ((PMEMORY_BASIC_INFORMATION_VLM)MemoryInformation)->BaseAddressAsUlongLong =
                                         (ULONGLONG)PAGE_ALIGN64(BaseAddress);
                ((PMEMORY_BASIC_INFORMATION_VLM)MemoryInformation)->RegionSize =
                                                                    TheRegionSize;
                ((PMEMORY_BASIC_INFORMATION_VLM)MemoryInformation)->State = MEM_FREE;
                ((PMEMORY_BASIC_INFORMATION_VLM)MemoryInformation)->Protect = PAGE_NOACCESS;
                ((PMEMORY_BASIC_INFORMATION_VLM)MemoryInformation)->Type = 0;

                if (ARGUMENT_PRESENT(ReturnLength)) {
                    *ReturnLength = sizeof(MEMORY_BASIC_INFORMATION_VLM);
                }

            } except (EXCEPTION_EXECUTE_HANDLER) {

                //
                // Just return success.
                //
            }

            return STATUS_SUCCESS;
        }
        return STATUS_INVALID_ADDRESS;
    }

    //
    // Found a vad.
    //

    Va = PAGE_ALIGN64(BaseAddress);
    Info.BaseAddress = Va;

    //
    //
    // There is a page mapped at the base address.
    //

    if (Vad->u.VadFlags.PrivateMemory) {
        Info.Type = MEM_PRIVATE;
    } else {
        Info.Type = MEM_MAPPED;
    }

    Info.State = MiQueryAddressState64 (Va, Vad, TargetProcess, &Info.Protect);

    Va = (PVOID64)((ULONGLONG)Va + PAGE_SIZE);

    while (MI_VA_TO_VPN64 (Va) <= Vad->EndingVpn) {

        NewState = MiQueryAddressState64 (Va,
                                        Vad,
                                        TargetProcess,
                                        &NewProtect);

        if ((NewState != Info.State) || (NewProtect != Info.Protect)) {

            //
            // The state for this address does not match, calculate
            // size and return.
            //

            break;
        }
        Va = (PVOID64)((ULONGLONG)Va + PAGE_SIZE);
    } // end while

    Info.RegionSize = ((ULONGLONG)Va - (ULONGLONG)Info.BaseAddress);
    Info.AllocationBase = MI_VPN_TO_VA64 (Vad->StartingVpn);
    Info.AllocationProtect = MI_CONVERT_FROM_PTE_PROTECTION (
                                             Vad->u.VadFlags.Protection);

    //
    // A range has been found, release the mutexes, detach from the
    // target process and return the information.
    //

    UNLOCK_WS (TargetProcess);
    UNLOCK_ADDRESS_SPACE (TargetProcess);
    KeDetachProcess();

    if ( ProcessHandle != NtCurrentProcess() ) {
        ObDereferenceObject (TargetProcess);
    }


    if ( MemoryInformationClass == MemoryBasicInformation ) {
        try {

            *(PMEMORY_BASIC_INFORMATION_VLM)MemoryInformation = Info;

            if (ARGUMENT_PRESENT(ReturnLength)) {
                *ReturnLength = sizeof(MEMORY_BASIC_INFORMATION_VLM);
            }

        } except (EXCEPTION_EXECUTE_HANDLER) {
            NOTHING;
        }
        return STATUS_SUCCESS;
    }
    return Status;
}



ULONG
MiQueryAddressState64 (
    IN PVOID64 Va,
    IN PMMVAD Vad,
    IN PEPROCESS TargetProcess,
    OUT PULONG ReturnedProtect
    )

/*++

Routine Description:


Arguments:

Return Value:

    Returns the state (MEM_COMMIT, MEM_RESERVE, MEM_PRIVATE).

Environment:

    Kernel mode.  Working set lock and address creation lock held.

--*/

{
    PMMPTE PointerPte;
    PMMPTE PointerPde;
    MMPTE CapturedProtoPte;
    PMMPTE ProtoPte;
    LOGICAL PteIsZero;
    ULONG State;
    ULONG Protect;
    ULONG VadProtect;

    VadProtect = MI_CONVERT_FROM_PTE_PROTECTION (Vad->u.VadFlags.Protection);
#ifdef LARGE_PAGES
    if (Vad->u.VadFlags.LargePages) {
        *ReturnedProtect = VadProtect;
        return MEM_COMMIT;
    }
#endif //LARGE_PAGES

    PointerPde = MiGetPdeAddress64 (Va);
    PointerPte = MiGetPteAddress64 (Va);

    ASSERT ((Vad->StartingVpn <= MI_VA_TO_VPN64 (Va)) &&
            (Vad->EndingVpn >= MI_VA_TO_VPN64 (Va)));

    PteIsZero = TRUE;

    //
    // Note that if the PDE and PTE are not valid, then we must be
    // in a no-access guard page type VAD.
    //
    if (MmIsAddressValid (PointerPde) && MmIsAddressValid(PointerPte)) {

        //
        // A PTE exists at this address, see if it is zero.
        //

        if (PointerPte->u.Long != 0) {

            PteIsZero = FALSE;

            //
            // There is a non-zero PTE at this address, use
            // it to build the information block.
            //

            if (MiIsPteDecommittedPage (PointerPte)) {
                Protect = 0;
                State = MEM_RESERVE;
            } else {

                State = MEM_COMMIT;

                Protect = PAGE_READWRITE;
                if (PointerPte->u.Hard.Valid == 1) {
                    if (PointerPte->u.Hard.Write == 0) {
                        Protect = PAGE_READONLY;
                    }
                } else if (PointerPte->u.Soft.Prototype) {
                    Protect = VadProtect;

                    if (PointerPte->u.Soft.PageFileHigh == MI_PTE_LOOKUP_NEEDED) {
                        Protect = MI_CONVERT_FROM_PTE_PROTECTION (PointerPte->u.Soft.Protection);
                    }

                    ProtoPte = MiGetProtoPteAddress(Vad,
                                                MI_VA_TO_VPN64 (Va));
                    CapturedProtoPte.u.Long = 0;
                    if (ProtoPte) {
                        CapturedProtoPte = MiCaptureSystemPte (ProtoPte,
                                                           TargetProcess);
                    }
                    if (CapturedProtoPte.u.Long == 0) {
                        State = MEM_RESERVE;
                        Protect = 0;
                    }

                } else {
                    Protect = MI_CONVERT_FROM_PTE_PROTECTION (PointerPte->u.Soft.Protection);
                }
            }
        }
    }

    if (PteIsZero == TRUE) {

        //
        // There is no PDE at this address, the template from
        // the VAD supplies the information unless the VAD is
        // for an image file.  For image files the individual
        // protection is on the prototype PTE.
        //

        //
        // Get the default protection information.
        //

        State = MEM_RESERVE;
        Protect = 0;

        if ((Vad->u.VadFlags.PrivateMemory == 0) &&
            (Vad->ControlArea != (PCONTROL_AREA)NULL)) {

            //
            // This VAD refers to a section.  Even though the PTE is
            // zero, the actual page may be committed in the section.
            //

            ProtoPte = MiGetProtoPteAddress(Vad, MI_VA_TO_VPN64 (Va));

            CapturedProtoPte.u.Long = 0;
            if (ProtoPte) {
                CapturedProtoPte = MiCaptureSystemPte (ProtoPte,
                                                       TargetProcess);
            }

            if (CapturedProtoPte.u.Long != 0) {
                State = MEM_COMMIT;
                Protect = VadProtect;
            }

        } else {

            //
            // Get the protection from the corresponding VAD.
            //

            if (Vad->u.VadFlags.MemCommit) {
                State = MEM_COMMIT;
                Protect = VadProtect;
            }
        }
    }

    *ReturnedProtect = Protect;
    return State;
}


#else

NTSTATUS
NtAllocateVirtualMemory64 (
    IN HANDLE ProcessHandle,
    IN OUT PVOID64 *BaseAddress,
    IN ULONG ZeroBits,
    IN OUT PULONGLONG RegionSize,
    IN ULONG AllocationType,
    IN ULONG Protect
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NtFreeVirtualMemory64(
    IN HANDLE ProcessHandle,
    IN OUT PVOID64 *BaseAddress,
    IN OUT PULONGLONG RegionSize,
    IN ULONG FreeType
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NtMapViewOfVlmSection (
    IN HANDLE SectionHandle,
    IN HANDLE ProcessHandle,
    IN OUT PVOID64 *BaseAddress,
    IN OUT PULONGLONG SectionOffset OPTIONAL,
    IN OUT PULONGLONG ViewSize,
    IN ULONG AllocationType,
    IN ULONG Protect
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NtUnmapViewOfVlmSection(
    IN HANDLE ProcessHandle,
    IN OUT PVOID64 *BaseAddress
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NtQueryVirtualMemory64(
    IN HANDLE ProcessHandle,
    IN PVOID64 *pBaseAddress,
    IN MEMORY_INFORMATION_CLASS MemoryInformationClass,
    OUT PVOID MemoryInformation,
    IN ULONG MemoryInformationLength,
    OUT PULONG ReturnLength OPTIONAL
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NtProtectVirtualMemory64(
    IN HANDLE ProcessHandle,
    IN OUT PVOID64 *BaseAddress,
    IN OUT PULONGLONG RegionSize,
    IN ULONG NewProtect,
    OUT PULONG OldProtect
    )

{
    return STATUS_NOT_IMPLEMENTED;
}

#endif //VLM_SUPPORT
