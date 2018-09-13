/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

   allocvm.c

Abstract:

    This module contains the routines which implement the
    NtAllocateVirtualMemory service.

Author:

    Lou Perazzoli (loup) 22-May-1989
    Landy Wang (landyw) 02-June-1997

Revision History:

--*/

#include "mi.h"

#if DBG
PEPROCESS MmWatchProcess;
VOID MmFooBar(VOID);
#endif // DBG

ULONG MMVADKEY = ' daV'; //Vad

//
// This can be enabled via the registry.
//

LOGICAL MmSupportWriteWatch = FALSE;

NTSTATUS
MiResetVirtualMemory (
    IN PVOID StartingAddress,
    IN PVOID EndingAddress,
    IN PMMVAD Vad,
    IN PEPROCESS Process
    );

LOGICAL
MiCreatePageTablesForPhysicalRange (
    IN PEPROCESS Process,
    IN PVOID StartingAddress,
    IN PVOID EndingAddress
    );

VOID
MiPhysicalViewInserter (
    IN PEPROCESS Process,
    IN PMI_PHYSICAL_VIEW PhysicalView
    );

VOID
MiFlushAcquire (
    IN PCONTROL_AREA ControlArea
    );

VOID
MiFlushRelease (
    IN PCONTROL_AREA ControlArea
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,NtAllocateVirtualMemory)
#endif


NTSTATUS
NtAllocateVirtualMemory(
    IN HANDLE ProcessHandle,
    IN OUT PVOID *BaseAddress,
    IN ULONG_PTR ZeroBits,
    IN OUT PSIZE_T RegionSize,
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
                  system will determine where to allocate the region.
        
    ZeroBits - Supplies the number of high order address bits that
               must be zero in the base address of the section view. The
               value of this argument must be less than or equal to the
               maximum number of zero bits and is only used when memory
               management determines where to allocate the view (i.e. when
               BaseAddress is null).

               If ZeroBits is zero, then no zero bit constraints are applied.

               If ZeroBits is greater than 0 and less than 32, then it is
               the number of leading zero bits from bit 31.  Bits 63:32 are
               also required to be zero.  This retains compatibility
               with 32-bit systems.
                
               If ZeroBits is greater than 32, then it is considered as
               a mask and then number of leading zero are counted out
               in the mask.  This then becomes the zero bits argument.

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

         MEM_TOP_DOWN - The specified region should be created at the
              highest virtual address possible based on ZeroBits.

         MEM_RESET - Reset the state of the specified region so
              that if the pages are in page paging file, they
              are discarded and pages of zeroes are brought in.
              If the pages are in memory and modified, they are marked
              as not modified so they will not be written out to
              the paging file.  The contents are NOT zeroed.

              The Protect argument is ignored, but a valid protection
              must be specified.

    Protect - Supplies the protection desired for the committed
              region of pages.

        Protect Values:


         PAGE_NOACCESS - No access to the committed region
              of pages is allowed. An attempt to read,
              write, or execute the committed region
              results in an access violation (i.e. a GP
              fault).

         PAGE_EXECUTE - Execute access to the committed
              region of pages is allowed. An attempt to
              read or write the committed region results in
              an access violation.

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

         PAGE_NOCACHE - The region of pages should be allocated
              as non-cachable.

Return Value:

    Various NTSTATUS codes.

--*/

{
    PMMVAD Vad;
    PMMVAD FoundVad;
    PEPROCESS Process;
    KPROCESSOR_MODE PreviousMode;
    PVOID StartingAddress;
    PVOID EndingAddress;
    NTSTATUS Status;
    PVOID TopAddress;
    PVOID CapturedBase;
    SIZE_T CapturedRegionSize;
    PMMPTE PointerPte;
    PMMPTE CommitLimitPte;
    ULONG ProtectionMask;
    PMMPTE LastPte;
    PMMPTE PointerPde;
    PMMPTE PointerPpe;
    PMMPTE StartingPte;
    MMPTE TempPte;
    ULONG OldProtect;
    SSIZE_T QuotaCharge;
    SIZE_T QuotaFree;
    SIZE_T CopyOnWriteCharge;
    BOOLEAN PageFileChargeSucceeded;
    BOOLEAN Attached;
    LOGICAL ChargedExactQuota;
    MMPTE DecommittedPte;
    ULONG ChangeProtection;
    PVOID UsedPageDirectoryHandle;
    PVOID UsedPageTableHandle;
    PUCHAR Va;
    LOGICAL ChargedJobCommit;
    PMI_PHYSICAL_VIEW PhysicalView;
    PRTL_BITMAP BitMap;
    ULONG BitMapSize;
    ULONG BitMapBits;
#if defined(_MIALT4K_)
    PVOID OriginalBase;
    SIZE_T OriginalRegionSize;
    BOOLEAN EmulationFor4kPage = FALSE;
    PVOID StartingAddressFor4k;
    PVOID EndingAddressFor4k;
    SIZE_T CapturedRegionSizeFor4k;
    ULONG CapturedOldProtectFor4k;
    ULONG OriginalProtectionMask;
    ULONG AltFlags;
#endif

    PAGED_CODE();

    Attached = FALSE;

    //
    // Check the zero bits argument for correctness.
    //

#if defined (_WIN64)

    if (ZeroBits >= 32) {

        //
        // ZeroBits is a mask instead of a count.  Translate it to a count now.
        //

        ZeroBits = 64 - RtlFindMostSignificantBit (ZeroBits);        
    }
    else if (ZeroBits) {
        ZeroBits += 32;
    }

#endif

    if (ZeroBits > MM_MAXIMUM_ZERO_BITS) {
        return STATUS_INVALID_PARAMETER_3;
    }

    //
    // Check the AllocationType for correctness.
    //

    if ((AllocationType & ~(MEM_COMMIT | MEM_RESERVE | MEM_PHYSICAL |
                            MEM_TOP_DOWN | MEM_RESET | MEM_WRITE_WATCH)) != 0) {
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

    if (AllocationType & MEM_WRITE_WATCH) {

        //
        // Write watch address spaces can only be created with MEM_RESERVE.
        //

        if ((AllocationType & MEM_RESERVE) == 0) {
            return STATUS_INVALID_PARAMETER_5;
        }

        if (MmSupportWriteWatch == FALSE) {
            return STATUS_NOT_SUPPORTED;
        }
    }

    if (AllocationType & MEM_PHYSICAL) {

        //
        // MEM_PHYSICAL must be used with MEM_RESERVE and no other flags.
        // This memory is always read-write when allocated.
        //

        if (AllocationType != (MEM_RESERVE | MEM_PHYSICAL)) {
            return STATUS_INVALID_PARAMETER_5;
        }

        if (Protect != PAGE_READWRITE) {
            return STATUS_INVALID_PARAMETER_6;
        }
    }

    //
    // Check the protection field.  This could raise an exception.
    //

    try {
        ProtectionMask = MiMakeProtectionMask (Protect);
    } except (EXCEPTION_EXECUTE_HANDLER) {
        return GetExceptionCode();
    }

    ChangeProtection = FALSE;

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

#if defined(_MIALT4K_)

    OriginalBase = CapturedBase;
    OriginalRegionSize = CapturedRegionSize;

#endif

#if DBG
    if (MmDebug & MM_DBG_SHOW_NT_CALLS) {
        if ( MmWatchProcess ) {
            ;
        } else {
            DbgPrint("allocvm process handle %lx base address %lx zero bits %lx\n",
                ProcessHandle, CapturedBase, ZeroBits);
            DbgPrint("    region size %lx alloc type %lx protect %lx\n",
                CapturedRegionSize, AllocationType, Protect);
        }
    }
#endif

    //
    // Make sure the specified starting and ending addresses are
    // within the user part of the virtual address space.
    //

    if (CapturedBase > MM_HIGHEST_VAD_ADDRESS) {

        //
        // Invalid base address.
        //

        return STATUS_INVALID_PARAMETER_2;
    }

    if ((((ULONG_PTR)MM_HIGHEST_VAD_ADDRESS + 1) - (ULONG_PTR)CapturedBase) <
            CapturedRegionSize) {

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

            CapturedRegionSize = ROUND_TO_PAGES (CapturedRegionSize);

            //
            // If the number of zero bits is greater than zero, then calculate
            // the highest address.
            //

            if (ZeroBits != 0) {
                TopAddress = (PVOID)(((ULONG_PTR)MM_USER_ADDRESS_RANGE_LIMIT) >> ZeroBits);

                //
                // Keep the top address below the highest user vad address
                // regardless.
                //

                if (TopAddress > MM_HIGHEST_VAD_ADDRESS) {
                    Status = STATUS_INVALID_PARAMETER_3;
                    goto ErrorReturn;
                }

            } else {
                TopAddress = (PVOID)MM_HIGHEST_VAD_ADDRESS;
            }

#if defined (_WIN64)

            //
            // BUGBUG:
            // For testing purposes, always set the MEM_TOP_DOWN flag to reduce
            // the number of low 4GB address allocations.  This helps the
            // compiler people catch address truncation errors.
            // This must be removed before the final build.
            //

            AllocationType |= MEM_TOP_DOWN;

#endif
            //
            // Establish exception handler as MiFindEmptyAddressRange
            // will raise an exception if it fails.
            //

            try {

                if (AllocationType & MEM_TOP_DOWN) {

                    //
                    // Start from the top of memory downward.
                    //

                    StartingAddress = MiFindEmptyAddressRangeDown (
                                                  CapturedRegionSize,
                                                  TopAddress,
                                                  X64K);

                } else {

                    StartingAddress = MiFindEmptyAddressRange (
                                                    CapturedRegionSize,
                                                    X64K,
                                                    (ULONG)ZeroBits );
                }

            } except (EXCEPTION_EXECUTE_HANDLER) {
                Status = GetExceptionCode();
                goto ErrorReturn;
            }

            //
            // Calculate the ending address based on the top address.
            //

            EndingAddress = (PVOID)(((ULONG_PTR)StartingAddress +
                                  CapturedRegionSize - 1L) | (PAGE_SIZE - 1L));

            if (EndingAddress > TopAddress) {

                //
                // The allocation does not honor the zero bits argument.
                //

                Status = STATUS_NO_MEMORY;
                goto ErrorReturn;
            }

        } else {

            //
            // A non-NULL base address was specified. Check to make sure
            // the specified base address to ending address is currently
            // unused.
            //

            EndingAddress = (PVOID)(((ULONG_PTR)CapturedBase +
                                  CapturedRegionSize - 1L) | (PAGE_SIZE - 1L));

            //
            // Align the starting address on a 64k boundary.
            //

            StartingAddress = (PVOID)MI_64K_ALIGN(CapturedBase);

            //
            // See if a VAD overlaps with this starting/ending address pair.
            //

            if (MiCheckForConflictingVad (StartingAddress, EndingAddress) !=
                    (PMMVAD)NULL) {

                Status = STATUS_CONFLICTING_ADDRESSES;
                goto ErrorReturn;
            }
        }

        //
        // Calculate the page file quota for this address range.
        //

        if (AllocationType & MEM_COMMIT) {
            QuotaCharge = BYTES_TO_PAGES ((PCHAR)EndingAddress -
                                          (PCHAR)StartingAddress);

        } else {
            QuotaCharge = 0;
        }

        //
        // An unoccupied address range has been found, build the virtual
        // address descriptor to describe this range.
        //
        // Allocate and initialize a VAD for the specified address range.
        //

        BitMapSize = 0;

        if (AllocationType & MEM_PHYSICAL) {

            if (AllocationType & MEM_WRITE_WATCH) {
                Status = STATUS_INVALID_PARAMETER_5;
                goto ErrorReturn;
            }

            PhysicalView = (PMI_PHYSICAL_VIEW) ExAllocatePoolWithTag (
                                                   NonPagedPool,
                                                   sizeof(MI_PHYSICAL_VIEW),
                                                   MI_PHYSICAL_VIEW_KEY);

            if (PhysicalView == NULL) {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto ErrorReturn;
            }
        }
        else if (AllocationType & MEM_WRITE_WATCH) {

            ASSERT (AllocationType & MEM_RESERVE);

            BitMapBits = BYTES_TO_PAGES ((PCHAR)EndingAddress -
                                         (PCHAR)StartingAddress);

            BitMapSize = sizeof(RTL_BITMAP) + (ULONG)(((BitMapBits + 31) / 32) * 4);

            BitMap = ExAllocatePoolWithTag (NonPagedPool, BitMapSize, 'wwmM');

            if (BitMap == NULL) {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto ErrorReturn;
            }

            try {
    
                //
                // Charge quota for the nonpaged pool for the bitmap.  This is
                // done here rather than by using ExAllocatePoolWithQuota
                // so the process object is not referenced by the quota charge.
                //
    
                PsChargePoolQuota (Process, NonPagedPool, BitMapSize);
    
            } except (EXCEPTION_EXECUTE_HANDLER) {
                Status = GetExceptionCode();
                ExFreePool (BitMap);
                goto ErrorReturn;
            }

            PhysicalView = (PMI_PHYSICAL_VIEW) ExAllocatePoolWithTag (
                                                   NonPagedPool,
                                                   sizeof(MI_PHYSICAL_VIEW),
                                                   MI_WRITEWATCH_VIEW_KEY);

            if (PhysicalView == NULL) {
                ExFreePool (BitMap);
                PsReturnPoolQuota (Process, NonPagedPool, BitMapSize);
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto ErrorReturn;
            }

            RtlInitializeBitMap (BitMap,
                                 (PULONG)(BitMap + 1),
                                 BitMapBits);
    
            RtlClearAllBits (BitMap);
        }

        Vad = ExAllocatePoolWithTag (NonPagedPool,
                                     sizeof(MMVAD_SHORT),
                                     'SdaV');

        try  {

            if (Vad == NULL) {
                ExRaiseStatus(STATUS_INSUFFICIENT_RESOURCES);
            }

            Vad->StartingVpn = MI_VA_TO_VPN (StartingAddress);
            Vad->EndingVpn = MI_VA_TO_VPN (EndingAddress);

            Vad->u.LongFlags = 0;
            if (AllocationType & MEM_COMMIT) {
                Vad->u.VadFlags.MemCommit = 1;
            }

            if (AllocationType & MEM_PHYSICAL) {
                Vad->u.VadFlags.UserPhysicalPages = 1;
            }

            Vad->u.VadFlags.Protection = ProtectionMask;
            Vad->u.VadFlags.PrivateMemory = 1;

            Vad->u.VadFlags.CommitCharge = QuotaCharge;

            MiInsertVad (Vad);

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

            if (AllocationType & MEM_PHYSICAL) {
                ExFreePool (PhysicalView);
            }
            else if (BitMapSize != 0) {
                ExFreePool (PhysicalView);
                ExFreePool (BitMap);
                PsReturnPoolQuota (Process, NonPagedPool, BitMapSize);
            }

            goto ErrorReturn;
        }

        //
        // Initialize page directory and table pages for the physical range.
        //

        if (AllocationType & MEM_PHYSICAL) {

            if (MiCreatePageTablesForPhysicalRange (Process,
                                                    StartingAddress,
                                                    EndingAddress) == FALSE) {

                MiRemoveVad (Vad);
                ExFreePool (Vad);
                ExFreePool (PhysicalView);
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto ErrorReturn;
            }

            PhysicalView->Vad = Vad;
            PhysicalView->StartVa = StartingAddress;
            PhysicalView->EndVa = EndingAddress;

            //
            // Insert the physical view into this process' list using a
            // nonpaged wrapper since the PFN lock is required.
            //

            MiPhysicalViewInserter (Process, PhysicalView);
        }
        else if (BitMapSize != 0) {

            Vad->u.VadFlags.WriteWatch = 1;

            PhysicalView->Vad = Vad;
            PhysicalView->StartVa = StartingAddress;
            PhysicalView->EndVa = EndingAddress;
            PhysicalView->BitMap = BitMap;

            MiPhysicalViewInserter (Process, PhysicalView);
        }

        //
        // Unlock the working set lock, page faults can now be taken.
        //

        UNLOCK_WS_UNSAFE (Process);

        //
        // Update the current virtual size in the process header, the
        // address space lock protects this operation.
        //

        CapturedRegionSize = (PCHAR)EndingAddress - (PCHAR)StartingAddress + 1L;
        Process->VirtualSize += CapturedRegionSize;

        if (Process->VirtualSize > Process->PeakVirtualSize) {
            Process->PeakVirtualSize = Process->VirtualSize;
        }

#if defined(_MIALT4K_)

        if (Process->Wow64Process != NULL) {

            if (OriginalBase == NULL) {

                OriginalRegionSize = ROUND_TO_4K_PAGES(OriginalRegionSize);

                EndingAddress =  (PVOID)(((ULONG_PTR) StartingAddress +
                                OriginalRegionSize - 1L) | (PAGE_4K - 1L));

            } else {

                EndingAddress = (PVOID)(((ULONG_PTR)OriginalBase +
                                OriginalRegionSize - 1L) | (PAGE_4K - 1L));
            }

            CapturedRegionSize = (PCHAR)EndingAddress - (PCHAR)StartingAddress + 1L;

            //
            // Set the alternate permission table
            //

            AltFlags = (AllocationType & MEM_COMMIT) ? ALT_COMMIT : 0;

            MiProtectFor4kPage (StartingAddress,
                                CapturedRegionSize,
                                ProtectionMask,
                                ALT_ALLOCATE|AltFlags,
                                Process);
        }

#endif

        //
        // Release the address space lock, lower IRQL, detach, and dereference
        // the process object.
        //

        UNLOCK_ADDRESS_SPACE(Process);
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

            //
            // Return success at this point even if the results
            // cannot be written.
            //

            NOTHING;
        }

#if DBG
        if (MmDebug & MM_DBG_SHOW_NT_CALLS) {
            if ( MmWatchProcess ) {
                if ( MmWatchProcess == PsGetCurrentProcess() ) {
                    DbgPrint("\n+++ ALLOC Type %lx Base %lx Size %lx\n",
                        AllocationType,StartingAddress, CapturedRegionSize);
                    MmFooBar();
                }
            } else {
                DbgPrint("return allocvm status %lx baseaddr %lx size %lx\n",
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
            // Round up to page boundaries so good data is not reset.
            //

            EndingAddress = (PVOID)((ULONG_PTR)PAGE_ALIGN ((ULONG_PTR)CapturedBase +
                                    CapturedRegionSize) - 1);
            StartingAddress = (PVOID)PAGE_ALIGN((PUCHAR)CapturedBase + PAGE_SIZE - 1);
            if (StartingAddress > EndingAddress) {
                Status = STATUS_CONFLICTING_ADDRESSES;
                goto ErrorReturn;
            }
        } else {
            EndingAddress = (PVOID)(((ULONG_PTR)CapturedBase +
                                    CapturedRegionSize - 1) | (PAGE_SIZE - 1));
            StartingAddress = (PVOID)PAGE_ALIGN(CapturedBase);
        }

        CapturedRegionSize = (PCHAR)EndingAddress - (PCHAR)StartingAddress + 1;

        FoundVad = MiCheckForConflictingVad (StartingAddress, EndingAddress);

#if defined(_MIALT4K_)

        if (Process->Wow64Process != NULL) {

            EmulationFor4kPage = TRUE;

            OriginalProtectionMask = MiMakeProtectionMask(Protect);

            //
            // if it is allowed to change the protection, relax the protection
            //

            if (FoundVad->u.VadFlags.NoChange == 0) {

                Protect = MiMakeProtectForNativePage(StartingAddress,
                                                     Protect,
                                                     Process);

                ProtectionMask = MiMakeProtectionMask (Protect);
            }

        }

#endif

        if (FoundVad == (PMMVAD)NULL) {

            //
            // No virtual address is reserved at the specified base address,
            // return an error.
            //

            Status = STATUS_CONFLICTING_ADDRESSES;
            goto ErrorReturn;
        }

        if (FoundVad->u.VadFlags.UserPhysicalPages == 1) {
            Status = STATUS_CONFLICTING_ADDRESSES;
            goto ErrorReturn;
        }

        if (FoundVad->u.VadFlags.CommitCharge == MM_MAX_COMMIT) {

            //
            // This is a special VAD, don't let any commits occur.
            //

            Status = STATUS_CONFLICTING_ADDRESSES;
            goto ErrorReturn;
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
            goto ErrorReturn;
        }

        if (AllocationType == MEM_RESET) {
            Status = MiResetVirtualMemory (StartingAddress,
                                           EndingAddress,
                                           FoundVad,
                                           Process);
            goto done;

        } else if (FoundVad->u.VadFlags.PrivateMemory == 0) {


            //
            // The no cache option is not allowed for sections.
            //

            if (Protect & PAGE_NOCACHE) {
                Status = STATUS_INVALID_PAGE_PROTECTION;
                goto ErrorReturn;
            }

            if (FoundVad->u.VadFlags.NoChange == 1) {

                //
                // An attempt is made at changing the protection
                // of a SEC_NO_CHANGE section.
                //

                Status = MiCheckSecuredVad (FoundVad,
                                            CapturedBase,
                                            CapturedRegionSize,
                                            ProtectionMask);

                if (!NT_SUCCESS (Status)) {
                    goto ErrorReturn;
                }
            }

            if (FoundVad->ControlArea->FilePointer != NULL) {
                if (FoundVad->u2.VadFlags2.ExtendableFile == 0) {

                    //
                    // Only page file backed sections can be committed.
                    //

                    Status = STATUS_ALREADY_COMMITTED;
                    goto ErrorReturn;
                } else {

                    //
                    // Commit the requested portions of the extendable file.
                    //

                    SECTION Section;
                    LARGE_INTEGER NewSize;
                    PCONTROL_AREA ControlArea;

                    RtlZeroMemory (&Section, sizeof(SECTION));
                    ControlArea = FoundVad->ControlArea;
                    Section.Segment = ControlArea->Segment;
                    Section.u.LongFlags = ControlArea->u.LongFlags;
                    Section.InitialPageProtection = PAGE_READWRITE;
                    NewSize.QuadPart = FoundVad->u2.VadFlags2.FileOffset;
                    NewSize.QuadPart = NewSize.QuadPart << 16;
                    NewSize.QuadPart += 1 +
                           ((PCHAR)EndingAddress - (PCHAR)MI_VPN_TO_VA (FoundVad->StartingVpn));
                
                    //
                    // The working set and address space mutexes must be
                    // released prior to calling MmExtendSection otherwise
                    // a deadlock with the filesystem can occur.
                    //
                    // Prevent the control area from being deleted while
                    // the (potential) extension is ongoing.
                    //

                    MiFlushAcquire (ControlArea);

                    UNLOCK_WS_AND_ADDRESS_SPACE (Process);
                    
                    Status = MmExtendSection (&Section,
                                              &NewSize,
                                              FALSE);
                
                    MiFlushRelease (ControlArea);

                    if (NT_SUCCESS(Status)) {

                        LOCK_WS_AND_ADDRESS_SPACE (Process);

                        //
                        // The Vad and/or the control area may have been changed
                        // or deleted before the mutexes were regained above.
                        // So everything must be revalidated.  Note that
                        // if anything has changed, success is silently
                        // returned just as if the protection change had failed.
                        // It is the caller's fault if any of these has gone
                        // away and they will suffer.
                        //

                        if (Process->AddressSpaceDeleted != 0) {
                            // Status = STATUS_PROCESS_IS_TERMINATING;
                            goto ErrorReturn;
                        }

                        FoundVad = MiCheckForConflictingVad (StartingAddress,
                                                             EndingAddress);
                
                        if (FoundVad == (PMMVAD)NULL) {
                
                            //
                            // No virtual address is reserved at the specified
                            // base address, return an error.
                            //
                            // Status = STATUS_CONFLICTING_ADDRESSES;

                            goto ErrorReturn;
                        }
                
                        if (ControlArea != FoundVad->ControlArea) {
                            goto ErrorReturn;
                        }

                        if (FoundVad->u.VadFlags.UserPhysicalPages == 1) {
                            // Status = STATUS_CONFLICTING_ADDRESSES;

                            goto ErrorReturn;
                        }
                
                        if (FoundVad->u.VadFlags.CommitCharge == MM_MAX_COMMIT) {
                            //
                            // This is a special VAD, no commits are allowed.
                            //
                            // Status = STATUS_CONFLICTING_ADDRESSES;

                            goto ErrorReturn;
                        }
                
                        //
                        // Ensure that the starting and ending addresses are
                        // all within the same virtual address descriptor.
                        //
                
                        if ((MI_VA_TO_VPN (StartingAddress) < FoundVad->StartingVpn) ||
                            (MI_VA_TO_VPN (EndingAddress) > FoundVad->EndingVpn)) {
                
                            //
                            // Not within the section virtual address
                            // descriptor, return an error.
                            //
                            // Status = STATUS_CONFLICTING_ADDRESSES;

                            goto ErrorReturn;
                        }

                        if (FoundVad->u.VadFlags.NoChange == 1) {
            
                            //
                            // An attempt is made at changing the protection
                            // of a SEC_NO_CHANGE section.
                            //
            
                            NTSTATUS Status2;

                            Status2 = MiCheckSecuredVad (FoundVad,
                                                         CapturedBase,
                                                         CapturedRegionSize,
                                                         ProtectionMask);
            
                            if (!NT_SUCCESS (Status2)) {
                                goto ErrorReturn;
                            }
                        }
            
                        if (FoundVad->ControlArea->FilePointer == NULL) {
                            goto ErrorReturn;
                        }

                        if (FoundVad->u2.VadFlags2.ExtendableFile == 0) {
                            goto ErrorReturn;
                        }

#if defined(_MIALT4K_)

                        if (EmulationFor4kPage == TRUE) {

                           UNLOCK_WS_UNSAFE(Process);

                           StartingAddressFor4k = (PVOID)PAGE_4K_ALIGN(OriginalBase);

                           EndingAddressFor4k = (PVOID)(((ULONG_PTR)OriginalBase +
                                                 OriginalRegionSize - 1) | (PAGE_4K - 1));

                           CapturedRegionSizeFor4k = (ULONG_PTR)EndingAddressFor4k -
                                                (ULONG_PTR)StartingAddressFor4k + 1L;

                           if ((FoundVad->u.VadFlags.ImageMap == 1) ||
                               (FoundVad->u2.VadFlags2.CopyOnWrite == 1)) {

                               OriginalProtectionMask |= MM_PROTECTION_COPY_MASK;

                           }

                           MiProtectFor4kPage (StartingAddressFor4k,
                                               CapturedRegionSizeFor4k,
                                               OriginalProtectionMask,
                                               ALT_COMMIT,
                                               Process);

                           LOCK_WS_UNSAFE(Process);
                        }

#endif

                        try {
                            MiSetProtectionOnSection (Process,
                                                      FoundVad,
                                                      StartingAddress,
                                                      EndingAddress,
                                                      Protect,
                                                      &OldProtect,
                                                      TRUE);
                        } except (EXCEPTION_EXECUTE_HANDLER) {
                            NOTHING;
                        }

                        UNLOCK_WS_AND_ADDRESS_SPACE (Process);
                    }

                    goto ErrorReturn1;
                }
            }

            StartingPte = MiGetProtoPteAddress (FoundVad,
                                                MI_VA_TO_VPN(StartingAddress));
            PointerPte = StartingPte;
            LastPte = MiGetProtoPteAddress (FoundVad,
                                            MI_VA_TO_VPN(EndingAddress));

            UNLOCK_WS_UNSAFE (Process);

            ExAcquireFastMutexUnsafe (&MmSectionCommitMutex);

#if 0
            if (AllocationType & MEM_CHECK_COMMIT_STATE) {

                //
                // Make sure none of the pages are already committed.
                //

                while (PointerPte <= LastPte) {

                    //
                    // Check to see if the prototype PTE is committed.
                    // Note that prototype PTEs cannot be decommitted so
                    // the PTEs only need to be checked for zeroes.
                    //
                    //

                    if (PointerPte->u.Long != 0) {
                        ExReleaseFastMutexUnsafe (&MmSectionCommitMutex);
                        Status = STATUS_ALREADY_COMMITTED;
                        UNLOCK_ADDRESS_SPACE (Process);
                        goto ErrorReturn1;
                    }
                    PointerPte += 1;
                }
            }
#endif //0

            PointerPte = StartingPte;

            //
            // Check to ensure these pages can be committed if this
            // is a page file backed segment.  Note that page file quota
            // has already been charged for this.
            //

            QuotaCharge = 1 + LastPte - StartingPte;

            CopyOnWriteCharge = 0;

            if (MI_IS_PTE_PROTECTION_COPY_WRITE(ProtectionMask)) {

                //
                // If the protection is copy on write, charge for
                // the copy on writes.
                //

                CopyOnWriteCharge = QuotaCharge;
            }

            //
            // Charge commitment for the range.  Establish an
            // exception handler as this could raise an exception.
            //

            QuotaFree = 0;
            ChargedExactQuota = FALSE;
            Status = STATUS_SUCCESS;
            ChargedJobCommit = FALSE;

            for (; ; ) {
                try {
                    PageFileChargeSucceeded = FALSE;

                    if (CopyOnWriteCharge != 0) {
                        MiChargePageFileQuota (CopyOnWriteCharge, Process);
                    }

                    PageFileChargeSucceeded = TRUE;

                    //
                    // Note this job charging is unusual because it is not
                    // followed by an immediate process charge.
                    //

                    if (Process->CommitChargeLimit) {
                        if (Process->CommitCharge + CopyOnWriteCharge > Process->CommitChargeLimit) {
                            if (Process->Job) {
                                PsReportProcessMemoryLimitViolation ();
                            }
                            ExRaiseStatus (STATUS_COMMITMENT_LIMIT);
                        }
                    }
                    if (Process->JobStatus & PS_JOB_STATUS_REPORT_COMMIT_CHANGES) {
                        if (PsChangeJobMemoryUsage(CopyOnWriteCharge) == FALSE) {
                            ExRaiseStatus (STATUS_COMMITMENT_LIMIT);
                        }
                        ChargedJobCommit = TRUE;
                    }
    
                    if (MiChargeCommitment (QuotaCharge + CopyOnWriteCharge, NULL) == FALSE) {
                        ExRaiseStatus (STATUS_COMMITMENT_LIMIT);
                    }

                    MM_TRACK_COMMIT (MM_DBG_COMMIT_ALLOCVM1, QuotaCharge + CopyOnWriteCharge);

                    break;

                } except (EXCEPTION_EXECUTE_HANDLER) {

                    //
                    // An exception has occurred during the charging
                    // of commitment.  Release the held mutexes and return
                    // the exception status to the user.
                    //

                    if (PageFileChargeSucceeded && CopyOnWriteCharge != 0) {
                        MiReturnPageFileQuota (CopyOnWriteCharge, Process);
                    }

                    if (ChargedJobCommit == TRUE) {

                        //
                        // Temporarily up the process commit charge as the
                        // job code will be referencing it as though everything
                        // has succeeded.
                        //

                        Process->CommitCharge += CopyOnWriteCharge;
                        PsChangeJobMemoryUsage (-(SSIZE_T)CopyOnWriteCharge);
                        Process->CommitCharge -= CopyOnWriteCharge;
                    }

                    if (Status != STATUS_SUCCESS || PageFileChargeSucceeded == FALSE) {

                        //
                        // We have already tried for the precise charge or the
                        // attempt to charge pagefile quota failed, so just
                        // return an error.
                        //

                        ExReleaseFastMutexUnsafe (&MmSectionCommitMutex);
                        UNLOCK_ADDRESS_SPACE (Process);
                        goto ErrorReturn1;
                    }

                    //
                    // The commitment charging of quota failed, calculate the
                    // exact quota taking into account pages that may already be
                    // committed and retry the operation.
                    //

                    while (PointerPte <= LastPte) {

                        //
                        // Check to see if the prototype PTE is committed.
                        // Note that prototype PTEs cannot be decommitted so
                        // PTEs only need to be checked for zeroes.
                        //

                        if (PointerPte->u.Long != 0) {
                            QuotaFree -= 1;
                        }
                        PointerPte += 1;
                    }

                    PointerPte = StartingPte;

                    QuotaCharge += QuotaFree;
                    ChargedExactQuota = TRUE;
                    Status = GetExceptionCode();
                }
            }

            FoundVad->ControlArea->Segment->NumberOfCommittedPages +=
                                                        QuotaCharge;

            FoundVad->u.VadFlags.CommitCharge += CopyOnWriteCharge;
            Process->CommitCharge += CopyOnWriteCharge;
            if (Process->CommitCharge > Process->CommitChargePeak) {
                Process->CommitChargePeak = Process->CommitCharge;
            }
            MmSharedCommit += QuotaCharge;

            //
            // Commit all the pages.
            //

            TempPte = FoundVad->ControlArea->Segment->SegmentPteTemplate;

            QuotaFree = 0;

            while (PointerPte <= LastPte) {

                if (PointerPte->u.Long != 0) {

                    //
                    // Page is already committed, back out commitment.
                    //

                    QuotaFree += 1;
                } else {
                    MI_WRITE_INVALID_PTE (PointerPte, TempPte);
                }
                PointerPte += 1;
            }

            if (ChargedExactQuota == FALSE && QuotaFree != 0) {

                FoundVad->ControlArea->Segment->NumberOfCommittedPages -= QuotaFree;
                MmSharedCommit -= QuotaFree;
                ASSERT ((LONG)FoundVad->ControlArea->Segment->NumberOfCommittedPages >= 0);

                if (CopyOnWriteCharge != 0) {
                    FoundVad->u.VadFlags.CommitCharge -= QuotaFree;
                    if (ChargedJobCommit == TRUE) {
                        PsChangeJobMemoryUsage (-(SSIZE_T)QuotaFree);
                    }
                    Process->CommitCharge -= QuotaFree;
                    ExReleaseFastMutexUnsafe (&MmSectionCommitMutex);
                    MiReturnPageFileQuota (QuotaFree, Process);
                }
                else {
                    ExReleaseFastMutexUnsafe (&MmSectionCommitMutex);
                }

                ASSERT ((SSIZE_T)FoundVad->u.VadFlags.CommitCharge >= 0);

                MiReturnCommitment (
                        (CopyOnWriteCharge ? 2*QuotaFree : QuotaFree));

                MM_TRACK_COMMIT (MM_DBG_COMMIT_RETURN_ALLOCVM1,
                        (CopyOnWriteCharge ? 2*QuotaFree : QuotaFree));
            }
            else {
                ExReleaseFastMutexUnsafe (&MmSectionCommitMutex);
            }

#if defined(_MIALT4K_)

            //
            // We need to change the alternate table before PTEs are created
            // for the protection change.
            //

            if (EmulationFor4kPage == TRUE) {

                StartingAddressFor4k = (PVOID)PAGE_4K_ALIGN(OriginalBase);

                EndingAddressFor4k = (PVOID)(((ULONG_PTR)OriginalBase +
                                              OriginalRegionSize - 1) | (PAGE_4K - 1));

                CapturedRegionSizeFor4k = (ULONG_PTR)EndingAddressFor4k -
                    (ULONG_PTR)StartingAddressFor4k + 1L;

                if ((FoundVad->u.VadFlags.ImageMap == 1) ||
                    (FoundVad->u2.VadFlags2.CopyOnWrite == 1)) {

                    OriginalProtectionMask |= MM_PROTECTION_COPY_MASK;

                }

                //
                // Set the alternate permission table
                //

                MiProtectFor4kPage (StartingAddressFor4k,
                                    CapturedRegionSizeFor4k,
                                    OriginalProtectionMask,
                                    ALT_COMMIT,
                                    Process);
            }
#endif

            //
            // Change all the protection to be protected as specified.
            //

            LOCK_WS_UNSAFE (Process);

            try {
                MiSetProtectionOnSection (Process,
                                          FoundVad,
                                          StartingAddress,
                                          EndingAddress,
                                          Protect,
                                          &OldProtect,
                                          TRUE);
            } except (EXCEPTION_EXECUTE_HANDLER) {
                NOTHING;
            }

            UNLOCK_WS_AND_ADDRESS_SPACE (Process);

            if (Attached) {
                KeDetachProcess();
            }
            if ( ProcessHandle != NtCurrentProcess() ) {
                ObDereferenceObject (Process);
            }

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

#if defined(_MIALT4K_)
            if (EmulationFor4kPage == TRUE) {
                CapturedRegionSize = CapturedRegionSizeFor4k;
                StartingAddress = StartingAddressFor4k;
            }
#endif

#if DBG
            if (MmDebug & MM_DBG_SHOW_NT_CALLS) {
                if ( MmWatchProcess ) {
                    if ( MmWatchProcess == PsGetCurrentProcess() ) {
                        DbgPrint("\n+++ ALLOC Type %lx Base %lx Size %lx\n",
                            AllocationType,StartingAddress, CapturedRegionSize);
                        MmFooBar();
                    }
                } else {
                    DbgPrint("return allocvm status %lx baseaddr %lx size %lx\n",
                        Status, CapturedRegionSize, StartingAddress);
                }
            }
#endif

            return STATUS_SUCCESS;

        } else {

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

            PointerPpe = MiGetPpeAddress (StartingAddress);
            PointerPde = MiGetPdeAddress (StartingAddress);
            PointerPte = MiGetPteAddress (StartingAddress);
            LastPte = MiGetPteAddress (EndingAddress);

            //
            // Check to ensure these pages can be committed.
            //

            QuotaCharge = 1 + LastPte - PointerPte;

            //
            // Charge quota and commitment for the range.  Establish an
            // exception handler as this could raise an exception.
            //

            QuotaFree = 0;
            ChargedExactQuota = FALSE;
            Status = STATUS_SUCCESS;
            ChargedJobCommit = FALSE;

            for (; ; ) {
                try {
                    PageFileChargeSucceeded = FALSE;

                    if (Process->CommitChargeLimit) {
                        if (Process->CommitCharge + QuotaCharge > Process->CommitChargeLimit) {
                            if (Process->Job) {
                                PsReportProcessMemoryLimitViolation ();
                            }
                            ExRaiseStatus (STATUS_COMMITMENT_LIMIT);
                        }
                    }
                    if (Process->JobStatus & PS_JOB_STATUS_REPORT_COMMIT_CHANGES) {
                        if (PsChangeJobMemoryUsage(QuotaCharge) == FALSE) {
                            ExRaiseStatus (STATUS_COMMITMENT_LIMIT);
                        }
                        ChargedJobCommit = TRUE;
                    }

                    if (MiChargeCommitment (QuotaCharge, Process) == FALSE) {
                        ExRaiseStatus (STATUS_COMMITMENT_LIMIT);
                    }

                    MM_TRACK_COMMIT (MM_DBG_COMMIT_ALLOCVM2, QuotaCharge);

                    PageFileChargeSucceeded = TRUE;
                    MiChargePageFileQuota (QuotaCharge, Process);

                    FoundVad->u.VadFlags.CommitCharge += QuotaCharge;
                    Process->CommitCharge += QuotaCharge;
                    if (Process->CommitCharge > Process->CommitChargePeak) {
                        Process->CommitChargePeak = Process->CommitCharge;
                    }
                    break;

                } except (EXCEPTION_EXECUTE_HANDLER) {

                    //
                    // An exception has occurred during the charging
                    // of commitment.  Release the held mutexes and return
                    // the exception status to the user.
                    //

                    if (ChargedJobCommit == TRUE) {

                        //
                        // Temporarily up the process commit charge as the
                        // job code will be referencing it as though everything
                        // has succeeded.
                        //

                        Process->CommitCharge += QuotaCharge;
                        PsChangeJobMemoryUsage (-QuotaCharge);
                        Process->CommitCharge -= QuotaCharge;
                    }

                    if (PageFileChargeSucceeded) {
                        MiReturnCommitment (QuotaCharge);
                        MM_TRACK_COMMIT (MM_DBG_COMMIT_RETURN_ALLOCVM2, QuotaCharge);
                    }

                    if (Status != STATUS_SUCCESS) {

                        //
                        // We have already tried for the precise charge,
                        // return an error.
                        //

                        goto ErrorReturn;
                    }

                    Status = GetExceptionCode();

                    //
                    // Quota charge failed, calculate the exact quota
                    // taking into account pages that may already be
                    // committed and retry the operation.
                    //

                    QuotaFree = -(LONG)MiCalculatePageCommitment (
                                                        StartingAddress,
                                                        EndingAddress,
                                                        FoundVad,
                                                        Process);

                    if (QuotaFree == 0) {
                        goto ErrorReturn;
                    }

                    ChargedExactQuota = TRUE;
                    QuotaCharge += QuotaFree;
                    ASSERT (QuotaCharge >= 0);
                }
            }


            //
            // Build a demand zero PTE with the proper protection.
            //

            TempPte = ZeroPte;
            TempPte.u.Soft.Protection = ProtectionMask;

            DecommittedPte = ZeroPte;
            DecommittedPte.u.Soft.Protection = MM_DECOMMIT;

            //
            // Fill in all the page directory and page table pages with the
            // demand zero PTE.
            //

#if defined (_WIN64)
            MiMakePpeExistAndMakeValid (PointerPpe, Process, FALSE);
            if (PointerPde->u.Long == 0) {
                UsedPageDirectoryHandle = MI_GET_USED_PTES_HANDLE (PointerPte);
                MI_INCREMENT_USED_PTES_BY_HANDLE (UsedPageDirectoryHandle);
            }
#endif

            MiMakePdeExistAndMakeValid (PointerPde, Process, FALSE);

            if (FoundVad->u.VadFlags.MemCommit) {
                CommitLimitPte = MiGetPteAddress (MI_VPN_TO_VA (FoundVad->EndingVpn));
            } else {
                CommitLimitPte = NULL;
            }

            QuotaFree = 0;

            while (PointerPte <= LastPte) {

                if (MiIsPteOnPdeBoundary (PointerPte)) {

                    PointerPde = MiGetPteAddress (PointerPte);
                    PointerPpe = MiGetPteAddress (PointerPde);

                    if (MiIsPteOnPpeBoundary (PointerPte)) {
                        MiMakePpeExistAndMakeValid (PointerPpe, Process, FALSE);
                    }

#if defined (_WIN64)
                    if (PointerPde->u.Long == 0) {
                        MI_WRITE_INVALID_PTE (PointerPde, TempPte);
                        UsedPageDirectoryHandle = MI_GET_USED_PTES_HANDLE (PointerPte);

                        MI_INCREMENT_USED_PTES_BY_HANDLE (UsedPageDirectoryHandle);
                    }
#endif

                    //
                    // Pointing to the next page table page, make
                    // a page table page exist and make it valid.
                    //

                    MiMakePdeExistAndMakeValid (PointerPde, Process, FALSE);
                }

                if (PointerPte->u.Long == 0) {

                    if (PointerPte <= CommitLimitPte) {

                        //
                        // This page is implicitly committed.
                        //

                        QuotaFree += 1;

                    }

                    MI_WRITE_INVALID_PTE (PointerPte, TempPte);

                    //
                    // Increment the count of non-zero page table entries
                    // for this page table and the number of private pages
                    // for the process.
                    //

                    Va = MiGetVirtualAddressMappedByPte (PointerPte);
                    UsedPageTableHandle = MI_GET_USED_PTES_HANDLE (Va);

                    MI_INCREMENT_USED_PTES_BY_HANDLE (UsedPageTableHandle);
                } else {
                    if (PointerPte->u.Long == DecommittedPte.u.Long) {

                        //
                        // Only commit the page if it is already decommitted.
                        //

                        MI_WRITE_INVALID_PTE (PointerPte, TempPte);
                    } else {
                        QuotaFree += 1;

                        //
                        // Make sure the protection for the page is
                        // right.
                        //

                        if (!ChangeProtection &&
                            (Protect != MiGetPageProtection (PointerPte,
                                                            Process))) {
                            ChangeProtection = TRUE;
                        }
                    }
                }
                PointerPte += 1;
            }
        }

        if (ChargedExactQuota == FALSE && QuotaFree != 0) {
            ASSERT (QuotaFree >= 0);
            MiReturnCommitment (QuotaFree);
            MM_TRACK_COMMIT (MM_DBG_COMMIT_RETURN_ALLOCVM3, QuotaFree);
            MiReturnPageFileQuota (QuotaFree, Process);
            FoundVad->u.VadFlags.CommitCharge -= QuotaFree;
            if (ChargedJobCommit) {
                PsChangeJobMemoryUsage (-(SSIZE_T)QuotaFree);
            }
            Process->CommitCharge -= QuotaFree;
            ASSERT (FoundVad->u.VadFlags.CommitCharge >= 0);
        }
#if defined(_MIALT4K_)

        if (EmulationFor4kPage == TRUE) {

            UNLOCK_WS_UNSAFE (Process);

            StartingAddress = (PVOID) PAGE_4K_ALIGN(OriginalBase);

            EndingAddress = (PVOID)(((ULONG_PTR)OriginalBase +
                                    OriginalRegionSize - 1) | (PAGE_4K - 1));

            CapturedRegionSize = (ULONG_PTR)EndingAddress -
                                      (ULONG_PTR)StartingAddress + 1L;
            //
            // Set the alternate permission table
            //

            MiProtectFor4kPage (StartingAddress,
                                CapturedRegionSize,
                                OriginalProtectionMask,
                                ALT_COMMIT,
                                Process);

            LOCK_WS_UNSAFE (Process);
        }
#endif

        //
        // Previously reserved pages have been committed, or an error occurred,
        // release working set lock, address creation lock, detach,
        // dereference process and return status.
        //

done:
        UNLOCK_WS_AND_ADDRESS_SPACE (Process);

        if (ChangeProtection) {
            PVOID Start;
            SIZE_T Size;
            ULONG LastProtect;

            Start = StartingAddress;
            Size = CapturedRegionSize;
            MiProtectVirtualMemory (Process,
                                    &Start,
                                    &Size,
                                    Protect,
                                    &LastProtect);
        }

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

#if DBG
        if (MmDebug & MM_DBG_SHOW_NT_CALLS) {
            if ( MmWatchProcess ) {
                if ( MmWatchProcess == PsGetCurrentProcess() ) {
                    DbgPrint("\n+++ ALLOC Type %lx Base %lx Size %lx\n",
                        AllocationType,StartingAddress, CapturedRegionSize);
                    MmFooBar();
                }
            } else {
                DbgPrint("return allocvm status %lx baseaddr %lx size %lx\n",
                    Status, CapturedRegionSize, StartingAddress);
            }
        }
#endif

        return STATUS_SUCCESS;
    }

ErrorReturn:
        UNLOCK_WS_AND_ADDRESS_SPACE (Process);

ErrorReturn1:
        if (Attached) {
            KeDetachProcess();
        }
        if ( ProcessHandle != NtCurrentProcess() ) {
            ObDereferenceObject (Process);
        }
        return Status;
}

NTSTATUS
MiResetVirtualMemory (
    IN PVOID StartingAddress,
    IN PVOID EndingAddress,
    IN PMMVAD Vad,
    IN PEPROCESS Process
    )

/*++

Routine Description:


Arguments:

    StartingAddress - Supplies the starting address of the range.

    RegionsSize - Supplies the size.

    Process - Supplies the current process.

Return Value:

Environment:

    Kernel mode, APCs disabled, WorkingSetMutex and AddressCreation mutexes
    held.

--*/

{
    PMMPTE PointerPte;
    PMMPTE ProtoPte;
    PMMPTE PointerPde;
    PMMPTE PointerPpe;
    PMMPTE LastPte;
    MMPTE PteContents;
    ULONG Waited;
    ULONG PfnHeld;
    ULONG First;
    KIRQL OldIrql;
    PMMPFN Pfn1;

    if (Vad->u.VadFlags.PrivateMemory == 0) {

        if (Vad->ControlArea->FilePointer != NULL) {

            //
            // Only page file backed sections can be committed.
            //

            return STATUS_USER_MAPPED_FILE;
        }
    }

    PfnHeld = FALSE;

    First = TRUE;
    PointerPte = MiGetPteAddress (StartingAddress);
    LastPte = MiGetPteAddress (EndingAddress);

    //
    // Examine all the PTEs in the range.
    //

    while (PointerPte <= LastPte) {

        if (MiIsPteOnPdeBoundary (PointerPte) || (First)) {

            if (MiIsPteOnPpeBoundary (PointerPte)) {

                PointerPpe = MiGetPdeAddress (PointerPte);

                if (!MiDoesPpeExistAndMakeValid(PointerPpe,
                                                Process,
                                                (BOOLEAN)PfnHeld,
                                                &Waited)) {

                    //
                    // This page directory parent entry is empty,
                    // go to the next one.
                    //

                    PointerPpe += 1;
                    PointerPde = MiGetVirtualAddressMappedByPte (PointerPpe);
                    PointerPte = MiGetVirtualAddressMappedByPte (PointerPde);
                    continue;
                }
            }

            //
            // Pointing to the next page table page, make
            // a page table page exist and make it valid.
            //

            First = FALSE;
            PointerPde = MiGetPteAddress (PointerPte);
            if (!MiDoesPdeExistAndMakeValid(PointerPde,
                                            Process,
                                            (BOOLEAN)PfnHeld,
                                            &Waited)) {

                //
                // This page directory entry is empty, go to the next one.
                //

                PointerPde += 1;
                PointerPte = MiGetVirtualAddressMappedByPte (PointerPde);
                continue;
            }
        }

        PteContents = *PointerPte;
        ProtoPte = NULL;

        if ((PteContents.u.Hard.Valid == 0) &&
            (PteContents.u.Soft.Prototype == 1))  {

            //
            // This is a prototype PTE, evaluate the prototype PTE.
            //

            ProtoPte = MiGetProtoPteAddress(Vad,
                                            MI_VA_TO_VPN (
                                            MiGetVirtualAddressMappedByPte(PointerPte)));
            if (!PfnHeld) {
                PfnHeld = TRUE;
                LOCK_PFN (OldIrql);
            }

            MiMakeSystemAddressValidPfnWs (ProtoPte, Process);

            //
            // The working set mutex may be released in order to make the
            // prototype PTE which resides in paged pool resident.  If this
            // occurs, the page directory and/or page table of the original
            // user address may get trimmed.  Account for that here.
            //

            if (MiMakeSystemAddressValidPfnWs (ProtoPte, Process) != 0) {

                //
                // Working set mutex was released, restart from the top.
                //

                First = TRUE;
                continue;
            }

            PteContents = *ProtoPte;
        }
        if (PteContents.u.Hard.Valid == 1) {
            if (!PfnHeld) {
                LOCK_PFN (OldIrql);
                PfnHeld = TRUE;
                continue;
            }

            Pfn1 = MI_PFN_ELEMENT (PteContents.u.Hard.PageFrameNumber);
            if (Pfn1->u3.e2.ReferenceCount == 1) {

                //
                // Only this process has the page mapped.
                //

                Pfn1->u3.e1.Modified = 0;
                MiReleasePageFileSpace (Pfn1->OriginalPte);
                Pfn1->OriginalPte.u.Soft.PageFileHigh = 0;
            }

            if ((!ProtoPte) && (MI_IS_PTE_DIRTY (PteContents))) {

                //
                // Clear the dirty bit and flush tb if it is NOT a prototype
                // PTE.
                //

                MI_SET_PTE_CLEAN (PteContents);
                KeFlushSingleTb (MiGetVirtualAddressMappedByPte (PointerPte),
                                 TRUE,
                                 FALSE,
                                 (PHARDWARE_PTE)PointerPte,
                                 PteContents.u.Flush);
            }

        } else if (PteContents.u.Soft.Transition == 1) {
            if (!PfnHeld) {
                LOCK_PFN (OldIrql);
                PfnHeld = TRUE;
                continue;
            }
            Pfn1 = MI_PFN_ELEMENT (PteContents.u.Trans.PageFrameNumber);
            if ((Pfn1->u3.e1.PageLocation == ModifiedPageList) &&
                (Pfn1->u3.e2.ReferenceCount == 0)) {

                //
                // Remove from the modified list, release the page
                // file space and insert on the standby list.
                //

                Pfn1->u3.e1.Modified = 0;
                MiUnlinkPageFromList (Pfn1);
                MiReleasePageFileSpace (Pfn1->OriginalPte);
                Pfn1->OriginalPte.u.Soft.PageFileHigh = 0;
                MiInsertPageInList (MmPageLocationList[StandbyPageList],
                                    MI_GET_PAGE_FRAME_FROM_TRANSITION_PTE(&PteContents));
            }
        } else {
            if (PteContents.u.Soft.PageFileHigh != 0) {
                if (!PfnHeld) {
                    LOCK_PFN (OldIrql);
                }
                PfnHeld = FALSE;
                MiReleasePageFileSpace (PteContents);
                UNLOCK_PFN (OldIrql);
                if (ProtoPte) {
                    ProtoPte->u.Soft.PageFileHigh = 0;
                } else {
                    PointerPte->u.Soft.PageFileHigh = 0;
                }
            } else {
                if (PfnHeld) {
                    UNLOCK_PFN (OldIrql);
                }
                PfnHeld = FALSE;
            }
        }
        PointerPte += 1;
    }
    if (PfnHeld) {
        UNLOCK_PFN (OldIrql);
    }
    return STATUS_SUCCESS;
}

LOGICAL
MiCreatePageTablesForPhysicalRange (
    IN PEPROCESS Process,
    IN PVOID StartingAddress,
    IN PVOID EndingAddress
    )

/*++

Routine Description:

    This routine initializes page directory and page table pages for a
    user-controlled physical range of pages.

Arguments:

    Process - Supplies the current process.

    StartingAddress - Supplies the starting address of the range.

    EndingAddress - Supplies the ending address of the range.

Return Value:

    TRUE if the page tables were created, FALSE if not.

Environment:

    Kernel mode, APCs disabled, WorkingSetMutex and AddressCreation mutexes
    held.

--*/

{
    MMPTE PteContents;
    PMMPTE LastPte;
    PMMPTE LastPde;
    PMMPTE PointerPte;
    PMMPTE PointerPde;
    PMMPTE PointerPpe;
    PUCHAR Va;
    PVOID UsedPageDirectoryHandle;
    PVOID UsedPageTableHandle;
    LOGICAL FirstTime;
    KIRQL OldIrql;
    PMMPFN Pfn1;
    ULONG PagesNeeded;

    FirstTime = TRUE;
    PointerPpe = MiGetPpeAddress (StartingAddress);
    PointerPde = MiGetPdeAddress (StartingAddress);
    PointerPte = MiGetPteAddress (StartingAddress);
    LastPde = MiGetPdeAddress (EndingAddress);
    LastPte = MiGetPteAddress (EndingAddress);

    //
    // Charge resident available pages for all of the page directory and table
    // pages as they will not be paged until the VAD is freed.
    //

    if (LastPte != PointerPte) {
        PagesNeeded = ADDRESS_AND_SIZE_TO_SPAN_PAGES (PointerPte,
                               (ULONG)(LastPte - PointerPte) * sizeof (MMPTE));

#if defined (_WIN64)
        if (LastPde != PointerPde) {
            PagesNeeded += ADDRESS_AND_SIZE_TO_SPAN_PAGES (PointerPde,
                                   (ULONG)(LastPde - PointerPde) * sizeof (MMPTE));
        }
#endif
    }
    else {
        PagesNeeded = 1;
#if defined (_WIN64)
        PagesNeeded += 1;
#endif
    }

    LOCK_PFN (OldIrql);

    if ((SPFN_NUMBER)PagesNeeded > MI_NONPAGABLE_MEMORY_AVAILABLE() - 20) {
        UNLOCK_PFN (OldIrql);
        return FALSE;
    }

    MmResidentAvailablePages -= PagesNeeded;
    MM_BUMP_COUNTER(58, PagesNeeded);
    UNLOCK_PFN (OldIrql);

    //
    // Fill in all the page directory and page table pages with the zero PTE.
    //

#if defined (_WIN64)
    MiMakePpeExistAndMakeValid (PointerPpe, Process, FALSE);
    if (PointerPde->u.Long == 0) {
        UsedPageDirectoryHandle = MI_GET_USED_PTES_HANDLE (PointerPte);
        MI_INCREMENT_USED_PTES_BY_HANDLE (UsedPageDirectoryHandle);
    }
#endif

    MiMakePdeExistAndMakeValid (PointerPde, Process, FALSE);

    while (PointerPte <= LastPte) {

        if (MiIsPteOnPdeBoundary (PointerPte) || FirstTime == TRUE) {

            PointerPde = MiGetPteAddress (PointerPte);
            PointerPpe = MiGetPteAddress (PointerPde);

#if defined (_WIN64)
            if (MiIsPteOnPpeBoundary (PointerPte) || FirstTime == TRUE) {
                MiMakePpeExistAndMakeValid (PointerPpe, Process, FALSE);

                //
                // Up the sharecount so the page directory page will not get
                // trimmed even if it has no currently valid entries.
                //

                PteContents = *PointerPpe;
                Pfn1 = MI_PFN_ELEMENT (PteContents.u.Hard.PageFrameNumber);
                LOCK_PFN (OldIrql);
                Pfn1->u2.ShareCount += 1;
                UNLOCK_PFN (OldIrql);
            }
#endif

            FirstTime = FALSE;

#if defined (_WIN64)
            if (PointerPde->u.Long == 0) {
                UsedPageDirectoryHandle = MI_GET_USED_PTES_HANDLE (PointerPte);
                MI_INCREMENT_USED_PTES_BY_HANDLE (UsedPageDirectoryHandle);
            }
#endif

            //
            // Pointing to the next page table page, make
            // a page table page exist and make it valid.
            //

            MiMakePdeExistAndMakeValid (PointerPde, Process, FALSE);

            //
            // Up the sharecount so the page table page will not get
            // trimmed even if it has no currently valid entries.
            //

            PteContents = *PointerPde;
            Pfn1 = MI_PFN_ELEMENT (PteContents.u.Hard.PageFrameNumber);
            LOCK_PFN (OldIrql);
            Pfn1->u2.ShareCount += 1;
            UNLOCK_PFN (OldIrql);
        }

        ASSERT (PointerPte->u.Long == 0);

        //
        // Increment the count of non-zero page table entries
        // for this page table - even though this entry is still zero,
        // this is a special case.
        //

        Va = MiGetVirtualAddressMappedByPte (PointerPte);
        UsedPageTableHandle = MI_GET_USED_PTES_HANDLE (Va);

        MI_INCREMENT_USED_PTES_BY_HANDLE (UsedPageTableHandle);

        PointerPte += 1;
    }
    return TRUE;
}

VOID
MiDeletePageTablesForPhysicalRange (
    IN PVOID StartingAddress,
    IN PVOID EndingAddress
    )

/*++

Routine Description:

    This routine deletes page directory and page table pages for a
    user-controlled physical range of pages.

    Even though PTEs may be zero in this range, UsedPageTable counts were
    incremented for these special ranges and must be decremented now.

Arguments:

    StartingAddress - Supplies the starting address of the range.

    EndingAddress - Supplies the ending address of the range.

Return Value:

    None.

Environment:

    Kernel mode, APCs disabled, WorkingSetMutex and AddressCreation mutexes
    held.

--*/

{
    PVOID TempVa;
    MMPTE PteContents;
    PMMPTE LastPte;
    PMMPTE LastPde;
    PMMPTE PointerPte;
    PMMPTE PointerPde;
    PMMPTE PointerPpe;
    ULONG PagesNeeded;
    PEPROCESS CurrentProcess;
    PVOID UsedPageTableHandle;
    PVOID UsedPageDirectoryHandle;
    KIRQL OldIrql;
    PMMPFN Pfn1;

    CurrentProcess = PsGetCurrentProcess();

    PointerPde = MiGetPdeAddress (StartingAddress);
    PointerPte = MiGetPteAddress (StartingAddress);
    LastPde = MiGetPdeAddress (EndingAddress);
    LastPte = MiGetPteAddress (EndingAddress);

    UsedPageTableHandle = MI_GET_USED_PTES_HANDLE (StartingAddress);

    //
    // Each PTE is already zeroed - just delete the containing pages.
    //

    //
    // Restore resident available pages for all of the page directory and table
    // pages as they can now be paged again.
    //

    if (LastPte != PointerPte) {
        PagesNeeded = ADDRESS_AND_SIZE_TO_SPAN_PAGES (PointerPte,
                               (ULONG)(LastPte - PointerPte) * sizeof (MMPTE));

#if defined (_WIN64)
        if (LastPde != PointerPde) {
            PagesNeeded += ADDRESS_AND_SIZE_TO_SPAN_PAGES (PointerPde,
                                   (ULONG)(LastPde - PointerPde) * sizeof (MMPTE));
        }
#endif
    }
    else {
        PagesNeeded = 1;
#if defined (_WIN64)
        PagesNeeded += 1;
#endif
    }

    LOCK_PFN (OldIrql);

    MmResidentAvailablePages += PagesNeeded;
    MM_BUMP_COUNTER(59, PagesNeeded);

    while (StartingAddress <= EndingAddress) {

        ASSERT (PointerPte->u.Long == 0);

        PointerPte += 1;

        MI_DECREMENT_USED_PTES_BY_HANDLE (UsedPageTableHandle);

        if ((MiIsPteOnPdeBoundary(PointerPte)) || (PointerPte > LastPte)) {

            //
            // The virtual address is on a page directory boundary or it is
            // the last address in the entire range.
            //
            // If all the entries have been eliminated from the previous
            // page table page, delete the page table page itself.
            //

            PointerPde = MiGetPteAddress (PointerPte - 1);
            ASSERT (PointerPde->u.Hard.Valid == 1);

            //
            // Down the sharecount on the finished page table page.
            //

            PteContents = *PointerPde;
            Pfn1 = MI_PFN_ELEMENT (PteContents.u.Hard.PageFrameNumber);
            ASSERT (Pfn1->u2.ShareCount > 1);
            Pfn1->u2.ShareCount -= 1;

            //
            // If all the entries have been eliminated from the previous
            // page table page, delete the page table page itself.
            //

            if (MI_GET_USED_PTES_FROM_HANDLE (UsedPageTableHandle) == 0) {

                TempVa = MiGetVirtualAddressMappedByPte(PointerPde);
                MiDeletePte (PointerPde,
                             TempVa,
                             FALSE,
                             CurrentProcess,
                             NULL,
                             NULL);

#if defined (_WIN64)
                UsedPageDirectoryHandle = MI_GET_USED_PTES_HANDLE (PointerPte - 1);
                MI_DECREMENT_USED_PTES_BY_HANDLE (UsedPageDirectoryHandle);
#endif
            }

#if defined (_WIN64)
            if ((MiIsPteOnPpeBoundary(PointerPte)) || (PointerPte > LastPte)) {

                PointerPpe = MiGetPteAddress (PointerPde);
                ASSERT (PointerPpe->u.Hard.Valid == 1);

                UsedPageDirectoryHandle = MI_GET_USED_PTES_HANDLE (PointerPte - 1);
                //
                // Down the sharecount on the finished page directory page.
                //

                PteContents = *PointerPpe;
                Pfn1 = MI_PFN_ELEMENT (PteContents.u.Hard.PageFrameNumber);
                ASSERT (Pfn1->u2.ShareCount > 1);
                Pfn1->u2.ShareCount -= 1;

                //
                // If all the entries have been eliminated from the previous
                // page directory page, delete the page directory page itself.
                //

                if (MI_GET_USED_PTES_FROM_HANDLE (UsedPageDirectoryHandle) == 0) {
                    TempVa = MiGetVirtualAddressMappedByPte(PointerPpe);
                    MiDeletePte (PointerPpe,
                                 TempVa,
                                 FALSE,
                                 CurrentProcess,
                                 NULL,
                                 NULL);
                }
            }
#endif

            if (PointerPte > LastPte) {
                break;
            }

            //
            // Release the PFN lock.  This prevents a single thread
            // from forcing other high priority threads from being
            // blocked while a large address range is deleted.
            //

            UNLOCK_PFN (OldIrql);
            UsedPageTableHandle = MI_GET_USED_PTES_HANDLE ((PVOID)((PUCHAR)StartingAddress + PAGE_SIZE));
            LOCK_PFN (OldIrql);
        }

        StartingAddress = (PVOID)((PUCHAR)StartingAddress + PAGE_SIZE);
    }

    UNLOCK_PFN (OldIrql);

    //
    // All done, return.
    //

    return;
}


//
// Commented out, no longer used.
//
#if 0
BOOLEAN
MiIsEntireRangeDecommitted (
    IN PVOID StartingAddress,
    IN PVOID EndingAddress,
    IN PMMVAD Vad,
    IN PEPROCESS Process
    )

/*++

Routine Description:

    This routine examines the range of pages from the starting address
    up to and including the ending address and returns TRUE if every
    page in the range is either not committed or decommitted, FALSE otherwise.

Arguments:

    StartingAddress - Supplies the starting address of the range.

    EndingAddress - Supplies the ending address of the range.

    Vad - Supplies the virtual address descriptor which describes the range.

    Process - Supplies the current process.

Return Value:

    TRUE if the entire range is either decommitted or not committed.
    FALSE if any page within the range is committed.

Environment:

    Kernel mode, APCs disabled, WorkingSetMutex and AddressCreation mutexes
    held.

--*/

{
    PMMPTE PointerPte;
    PMMPTE LastPte;
    PMMPTE PointerPde;
    ULONG Waited;
    ULONG FirstTime;
    PVOID Va;

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

    //
    // A page table page exists, examine the individual PTEs to ensure
    // none are in the committed state.
    //

    while (PointerPte <= LastPte) {

        //
        // Check to see if a page table page (PDE) exists if the PointerPte
        // address is on a page boundary or this is the first time through
        // the loop.
        //

        if (MiIsPteOnPdeBoundary (PointerPte) || (FirstTime)) {

            //
            // This is a PDE boundary, check to see if the entire
            // PDE page exists.
            //

            FirstTime = FALSE;
            PointerPde = MiGetPteAddress (PointerPte);

            while (!MiDoesPdeExistAndMakeValid (PointerPde,
                                                Process,
                                                FALSE,
                                                &Waited)) {

                //
                // No PDE exists for the starting address, check the VAD
                // to see whether the pages are committed or not.
                //

                PointerPde += 1;
                PointerPte = MiGetVirtualAddressMappedByPte (PointerPde);

                if (PointerPte > LastPte) {

                    //
                    // No page table page exists, if explicit commitment
                    // via VAD indicates PTEs of zero should be committed,
                    // return an error.
                    //

                    if (EndingAddress <= Vad->CommittedAddress) {

                        //
                        // The entire range is committed, return an error.
                        //

                        return FALSE;
                    } else {

                        //
                        // All pages are decommitted, return TRUE.
                        //

                        return TRUE;
                    }
                }

                Va = MiGetVirtualAddressMappedByPte (PointerPte);

                //
                // Make sure the range thus far is not committed.
                //

                if (Va <= Vad->CommittedAddress) {

                    //
                    // This range is committed, return an error.
                    //

                    return FALSE;
                }
            }
        }

        //
        // The page table page exists, check each PTE for commitment.
        //

        if (PointerPte->u.Long == 0) {

            //
            // This PTE for the page is zero, check the VAD.
            //

            if (Va <= Vad->CommittedAddress) {

                //
                // The entire range is committed, return an error.
                //

                return FALSE;
            }
        } else {

            //
            // Has this page been explicitly decommitted?
            //

            if (!MiIsPteDecommittedPage (PointerPte)) {

                //
                // This page is committed, return an error.
                //

                return FALSE;
            }
        }
        PointerPte += 1;
        Va = (PVOID)((PCHAR)(Va) + PAGE_SIZE);
    }
    return TRUE;
}
#endif //0

#if DBG
VOID
MmFooBar(VOID){}
#endif
