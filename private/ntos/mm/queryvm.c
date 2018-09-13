/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

   queryvm.c

Abstract:

    This module contains the routines which implement the
    NtQueryVirtualMemory service.

Author:

    Lou Perazzoli (loup) 21-Aug-1989
    Landy Wang (landyw) 02-June-1997

Revision History:

--*/

#include "mi.h"

extern POBJECT_TYPE IoFileObjectType;

NTSTATUS
MiGetWorkingSetInfo (
    IN PMEMORY_WORKING_SET_INFORMATION WorkingSetInfo,
    IN ULONG Length,
    IN PEPROCESS Process
    );

MMPTE
MiCaptureSystemPte (
    IN PMMPTE PointerProtoPte,
    IN PEPROCESS Process
    );

#if DBG
PEPROCESS MmWatchProcess;
VOID MmFooBar(VOID);
#endif // DBG

ULONG
MiQueryAddressState (
    IN PVOID Va,
    IN PMMVAD Vad,
    IN PEPROCESS TargetProcess,
    OUT PULONG ReturnedProtect
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,NtQueryVirtualMemory)
#pragma alloc_text(PAGE,MiQueryAddressState)
#pragma alloc_text(PAGELK,MiGetWorkingSetInfo)
#endif


NTSTATUS
NtQueryVirtualMemory (
    IN HANDLE ProcessHandle,
    IN PVOID BaseAddress,
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

    The state of the first page within the region is determined and then
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

    BaseAddress - The base address of the region of pages to be
        queried. This value is rounded down to the next host-page-
        address boundary.

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
    BOOLEAN PteIsZero;
    PVOID Va;
    BOOLEAN Found;
    SIZE_T TheRegionSize;
    ULONG NewProtect;
    ULONG NewState;
    PVOID FilePointer;
    ULONG_PTR BaseVpn;
    MEMORY_BASIC_INFORMATION Info;
    LOGICAL Attached;

    Found = FALSE;
    PteIsZero = FALSE;

    //
    // Make sure the user's buffer is large enough for the requested operation.
    //

    //
    // Check argument validity.
    //
    switch (MemoryInformationClass) {
        case MemoryBasicInformation:
                if (MemoryInformationLength < sizeof(MEMORY_BASIC_INFORMATION)) {
                    return STATUS_INFO_LENGTH_MISMATCH;
                }
                break;

        case MemoryWorkingSetInformation:
                if (MemoryInformationLength < sizeof(ULONG)) {
                    return STATUS_INFO_LENGTH_MISMATCH;
                }
                break;

        case MemoryMappedFilenameInformation:
                FilePointer = NULL;
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

            ProbeForWrite(MemoryInformation,
                          MemoryInformationLength,
                          sizeof(ULONG_PTR));

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
    }
    if (BaseAddress > MM_HIGHEST_USER_ADDRESS) {
        return STATUS_INVALID_PARAMETER;
    }

    if ((BaseAddress >= MM_HIGHEST_VAD_ADDRESS)
#if defined(MM_SHARED_USER_DATA_VA)
            ||
         (PAGE_ALIGN(BaseAddress) == (PVOID)MM_SHARED_USER_DATA_VA)
#endif
             ) {

        //
        // Indicate a reserved area from this point on.
        //

        if ( MemoryInformationClass == MemoryBasicInformation ) {

            try {
                ((PMEMORY_BASIC_INFORMATION)MemoryInformation)->AllocationBase =
                                      (PCHAR) MM_HIGHEST_VAD_ADDRESS + 1;
                ((PMEMORY_BASIC_INFORMATION)MemoryInformation)->AllocationProtect =
                                                                      PAGE_READONLY;
                ((PMEMORY_BASIC_INFORMATION)MemoryInformation)->BaseAddress =
                                                       PAGE_ALIGN(BaseAddress);
                ((PMEMORY_BASIC_INFORMATION)MemoryInformation)->RegionSize =
                                    ((PCHAR)MM_HIGHEST_USER_ADDRESS + 1) -
                                                (PCHAR)PAGE_ALIGN(BaseAddress);
                ((PMEMORY_BASIC_INFORMATION)MemoryInformation)->State = MEM_RESERVE;
                ((PMEMORY_BASIC_INFORMATION)MemoryInformation)->Protect = PAGE_NOACCESS;
                ((PMEMORY_BASIC_INFORMATION)MemoryInformation)->Type = MEM_PRIVATE;

                if (ARGUMENT_PRESENT(ReturnLength)) {
                    *ReturnLength = sizeof(MEMORY_BASIC_INFORMATION);
                }

#if defined(MM_SHARED_USER_DATA_VA)
                if (PAGE_ALIGN(BaseAddress) == (PVOID)MM_SHARED_USER_DATA_VA) {

                    //
                    // This is the page that is double mapped between
                    // user mode and kernel mode.
                    //

                    ((PMEMORY_BASIC_INFORMATION)MemoryInformation)->AllocationBase =
                                (PVOID)MM_SHARED_USER_DATA_VA;
                    ((PMEMORY_BASIC_INFORMATION)MemoryInformation)->Protect =
                                                                 PAGE_READONLY;
                    ((PMEMORY_BASIC_INFORMATION)MemoryInformation)->RegionSize =
                                                                 PAGE_SIZE;
                    ((PMEMORY_BASIC_INFORMATION)MemoryInformation)->State =
                                                                 MEM_COMMIT;
                }
#endif

            } except (EXCEPTION_EXECUTE_HANDLER) {

                //
                // Just return success.
                //
            }

            return STATUS_SUCCESS;
        } else {
            return STATUS_INVALID_ADDRESS;
        }
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

    if (MemoryInformationClass == MemoryWorkingSetInformation) {

        MmLockPagableSectionByHandle(ExPageLockHandle);

        Status = MiGetWorkingSetInfo (MemoryInformation,
                                      MemoryInformationLength,
                                      TargetProcess);
        MmUnlockPagableImageSection(ExPageLockHandle);

        if ( ProcessHandle != NtCurrentProcess() ) {
            ObDereferenceObject (TargetProcess);
        }
        try {

            if (ARGUMENT_PRESENT(ReturnLength)) {
                *ReturnLength = ((((PMEMORY_WORKING_SET_INFORMATION)
                                    MemoryInformation)->NumberOfEntries - 1) *
                                        sizeof(ULONG)) +
                                        sizeof(MEMORY_WORKING_SET_INFORMATION);
            }

        } except (EXCEPTION_EXECUTE_HANDLER) {
        }

        return STATUS_SUCCESS;
    }

    //
    // If the specified process is not the current process, attach
    // to the specified process.
    //

    if ( ProcessHandle != NtCurrentProcess() ) {
        KeAttachProcess (&TargetProcess->Pcb);
        Attached = TRUE;
    }
    else {
        Attached = FALSE;
    }

    //
    // Get working set mutex and block APCs.
    //

    LOCK_WS_AND_ADDRESS_SPACE (TargetProcess);

    //
    // Make sure the address space was not deleted, if so, return an error.
    //

    if (TargetProcess->AddressSpaceDeleted != 0) {
        UNLOCK_WS_AND_ADDRESS_SPACE (TargetProcess);
        if (Attached == TRUE) {
            KeDetachProcess();
            ObDereferenceObject (TargetProcess);
        }
        return STATUS_PROCESS_IS_TERMINATING;
    }

    //
    // Locate the VAD that contains the base address or the VAD
    // which follows the base address.
    //

    Vad = TargetProcess->VadRoot;
    BaseVpn = MI_VA_TO_VPN (BaseAddress);

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
            TheRegionSize = ((PCHAR)MM_HIGHEST_VAD_ADDRESS + 1) -
                                                (PCHAR)PAGE_ALIGN(BaseAddress);
        } else {
            if (Vad->StartingVpn < BaseVpn) {

                //
                // We are looking at the Vad which occupies the range
                // just before the desired range.  Get the next Vad.
                //

                Vad = MiGetNextVad (Vad);
                if (Vad == NULL) {
                    TheRegionSize = ((PCHAR)MM_HIGHEST_VAD_ADDRESS + 1) -
                                                (PCHAR)PAGE_ALIGN(BaseAddress);
                } else {
                    TheRegionSize = (PCHAR)MI_VPN_TO_VA (Vad->StartingVpn) -
                                                (PCHAR)PAGE_ALIGN(BaseAddress);
                }
            } else {
                TheRegionSize = (PCHAR)MI_VPN_TO_VA (Vad->StartingVpn) -
                                                (PCHAR)PAGE_ALIGN(BaseAddress);
            }
        }

        UNLOCK_WS_AND_ADDRESS_SPACE (TargetProcess);

        if (Attached == TRUE) {
            KeDetachProcess();
            ObDereferenceObject (TargetProcess);
        }

        //
        // Establish an exception handler and write the information and
        // returned length.
        //

        if ( MemoryInformationClass == MemoryBasicInformation ) {
            try {

                ((PMEMORY_BASIC_INFORMATION)MemoryInformation)->AllocationBase =
                                                                            NULL;
                ((PMEMORY_BASIC_INFORMATION)MemoryInformation)->AllocationProtect =
                                                                            0;
                ((PMEMORY_BASIC_INFORMATION)MemoryInformation)->BaseAddress =
                                                            PAGE_ALIGN(BaseAddress);
                ((PMEMORY_BASIC_INFORMATION)MemoryInformation)->RegionSize =
                                                                    TheRegionSize;
                ((PMEMORY_BASIC_INFORMATION)MemoryInformation)->State = MEM_FREE;
                ((PMEMORY_BASIC_INFORMATION)MemoryInformation)->Protect = PAGE_NOACCESS;
                ((PMEMORY_BASIC_INFORMATION)MemoryInformation)->Type = 0;

                if (ARGUMENT_PRESENT(ReturnLength)) {
                    *ReturnLength = sizeof(MEMORY_BASIC_INFORMATION);
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
    // Found a VAD.
    //

    Va = PAGE_ALIGN(BaseAddress);
    Info.BaseAddress = Va;

    //
    // There is a page mapped at the base address.
    //

    if (Vad->u.VadFlags.PrivateMemory) {
        Info.Type = MEM_PRIVATE;
    } else if (Vad->u.VadFlags.ImageMap == 0) {
        Info.Type = MEM_MAPPED;

        if ( MemoryInformationClass == MemoryMappedFilenameInformation ) {
            if (Vad->ControlArea) {
                FilePointer = Vad->ControlArea->FilePointer;
            }
            if ( !FilePointer ) {
                FilePointer = (PVOID)1;
            } else {
                ObReferenceObject(FilePointer);
            }
        }

    } else {
        Info.Type = MEM_IMAGE;
    }

    Info.State = MiQueryAddressState (Va, Vad, TargetProcess, &Info.Protect);

    Va = (PVOID)((PCHAR)Va + PAGE_SIZE);

    while (MI_VA_TO_VPN (Va) <= Vad->EndingVpn) {

        NewState = MiQueryAddressState (Va,
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
        Va = (PVOID)((PCHAR)Va + PAGE_SIZE);
    } // end while

    Info.RegionSize = ((PCHAR)Va - (PCHAR)Info.BaseAddress);
    Info.AllocationBase = MI_VPN_TO_VA (Vad->StartingVpn);
    Info.AllocationProtect = MI_CONVERT_FROM_PTE_PROTECTION (
                                             Vad->u.VadFlags.Protection);

    //
    // A range has been found, release the mutexes, deattach from the
    // target process and return the information.
    //

#if !(defined(_MIALT4K_))

    UNLOCK_WS_AND_ADDRESS_SPACE (TargetProcess);

#else

    UNLOCK_WS_UNSAFE (TargetProcess);

    if (TargetProcess->Wow64Process != NULL) {
        
        Info.BaseAddress = PAGE_4K_ALIGN(BaseAddress);

        MiQueryRegionFor4kPage(Info.BaseAddress,
                               MI_VPN_TO_VA_ENDING(Vad->EndingVpn),
                               &Info.RegionSize,
                               &Info.State,
                               &Info.Protect,
                               TargetProcess);
    }

    UNLOCK_ADDRESS_SPACE (TargetProcess);

#endif

    if (Attached == TRUE) {
        KeDetachProcess();
        ObDereferenceObject (TargetProcess);
    }

#if DBG
    if (MmDebug & MM_DBG_SHOW_NT_CALLS) {
        if ( !MmWatchProcess ) {
            DbgPrint("queryvm base %lx allocbase %lx protect %lx size %lx\n",
                Info.BaseAddress, Info.AllocationBase, Info.AllocationProtect,
                Info.RegionSize);
            DbgPrint("    state %lx  protect %lx  type %lx\n",
                Info.State, Info.Protect, Info.Type);
        }
    }
#endif //DBG

    if ( MemoryInformationClass == MemoryBasicInformation ) {
        try {

            *(PMEMORY_BASIC_INFORMATION)MemoryInformation = Info;

            if (ARGUMENT_PRESENT(ReturnLength)) {
                *ReturnLength = sizeof(MEMORY_BASIC_INFORMATION);
            }

        } except (EXCEPTION_EXECUTE_HANDLER) {
        }
        return STATUS_SUCCESS;
    }

    //
    // Try to return the name of the file that is mapped.
    //

    if ( !FilePointer ) {
        return STATUS_INVALID_ADDRESS;
    } else if ( FilePointer == (PVOID)1 ) {
        return STATUS_FILE_INVALID;
    }

    //
    // We have a referenced pointer to the file. Call ObQueryNameString
    // and get the file name
    //

    Status = ObQueryNameString(
                FilePointer,
                MemoryInformation,
                MemoryInformationLength,
                ReturnLength
                );
    ObDereferenceObject(FilePointer);
    return Status;
}


ULONG
MiQueryAddressState (
    IN PVOID Va,
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
    PMMPTE PointerPpe;
    MMPTE CapturedProtoPte;
    PMMPTE ProtoPte;
    ULONG PteIsZero;
    ULONG State;
    ULONG Protect;
    ULONG Waited;
    LOGICAL PteDetected;

#ifdef LARGE_PAGES
    if (Vad->u.VadFlags.LargePages) {
        *ReturnedProtect = MI_CONVERT_FROM_PTE_PROTECTION (
                                             Vad->u.VadFlags.Protection);
        return MEM_COMMIT;
    }
#endif //LARGE_PAGES

    PointerPpe = MiGetPpeAddress (Va);
    PointerPde = MiGetPdeAddress (Va);
    PointerPte = MiGetPteAddress (Va);

    ASSERT ((Vad->StartingVpn <= MI_VA_TO_VPN (Va)) &&
            (Vad->EndingVpn >= MI_VA_TO_VPN (Va)));

    PteIsZero = TRUE;
    PteDetected = FALSE;

    do {

        if (!MiDoesPpeExistAndMakeValid(PointerPpe,
                                       TargetProcess,
                                       FALSE,
                                       &Waited)) {
            break;
        }
    
        Waited = 0;

        if (!MiDoesPdeExistAndMakeValid(PointerPde,
                                       TargetProcess,
                                       FALSE,
                                       &Waited)) {
            break;
        }

        if (Waited == 0) {
            PteDetected = TRUE;
        }

    } while (Waited != 0);

    if (PteDetected == TRUE) {

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
                if (Vad->u.VadFlags.PhysicalMapping == 1) {

                    //
                    // Physical mapping, there is no corresponding
                    // PFN element to get the page protection from.
                    //

                    Protect = MI_CONVERT_FROM_PTE_PROTECTION (
                                             Vad->u.VadFlags.Protection);
                } else {
                    Protect = MiGetPageProtection (PointerPte,
                                                   TargetProcess);

                    if ((PointerPte->u.Soft.Valid == 0) &&
                        (PointerPte->u.Soft.Prototype == 1) &&
                        (Vad->u.VadFlags.PrivateMemory == 0) &&
                        (Vad->ControlArea != (PCONTROL_AREA)NULL)) {

                        //
                        // Make sure the protoPTE is committed.
                        //

                        ProtoPte = MiGetProtoPteAddress(Vad,
                                                    MI_VA_TO_VPN (Va));
                        CapturedProtoPte.u.Long = 0;
                        if (ProtoPte) {
                            CapturedProtoPte = MiCaptureSystemPte (ProtoPte,
                                                               TargetProcess);
                        }
                        if (CapturedProtoPte.u.Long == 0) {
                            State = MEM_RESERVE;
                            Protect = 0;
                        }
                    }
                }
            }
        }
    }

    if (PteIsZero) {

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

        if (Vad->u.VadFlags.PhysicalMapping == 1) {

            //
            // Must be banked memory, just return reserved.
            //

            NOTHING;

        } else if ((Vad->u.VadFlags.PrivateMemory == 0) &&
            (Vad->ControlArea != (PCONTROL_AREA)NULL)) {

            //
            // This VAD refers to a section.  Even though the PTE is
            // zero, the actual page may be committed in the section.
            //

            ProtoPte = MiGetProtoPteAddress(Vad, MI_VA_TO_VPN (Va));

            CapturedProtoPte.u.Long = 0;
            if (ProtoPte) {
                CapturedProtoPte = MiCaptureSystemPte (ProtoPte,
                                                       TargetProcess);
            }

            if (CapturedProtoPte.u.Long != 0) {
                State = MEM_COMMIT;

                if (Vad->u.VadFlags.ImageMap == 0) {
                    Protect = MI_CONVERT_FROM_PTE_PROTECTION (
                                              Vad->u.VadFlags.Protection);
                } else {

                    //
                    // This is an image file, the protection is in the
                    // prototype PTE.
                    //

                    Protect = MiGetPageProtection (&CapturedProtoPte,
                                                        TargetProcess);
                }
            }

        } else {

            //
            // Get the protection from the corresponding VAD.
            //

            if (Vad->u.VadFlags.MemCommit) {
                State = MEM_COMMIT;
                Protect = MI_CONVERT_FROM_PTE_PROTECTION (
                                            Vad->u.VadFlags.Protection);
            }
        }
    }

    *ReturnedProtect = Protect;
    return State;
}



NTSTATUS
MiGetWorkingSetInfo (
    IN PMEMORY_WORKING_SET_INFORMATION WorkingSetInfo,
    IN ULONG Length,
    IN PEPROCESS Process
    )

{
    PMDL Mdl;
    PMEMORY_WORKING_SET_INFORMATION Info;
    PMEMORY_WORKING_SET_BLOCK Entry;
    PMEMORY_WORKING_SET_BLOCK LastEntry;
    PMMWSLE Wsle;
    PMMWSLE LastWsle;
    ULONG WsSize;
    PMMPTE PointerPte;
    PMMPFN Pfn1;
    NTSTATUS status;
    LOGICAL Attached;

    //
    // Allocate an MDL to map the request.
    //

    Mdl = ExAllocatePoolWithTag (NonPagedPool,
                                 sizeof(MDL) + sizeof(PFN_NUMBER) +
                                     BYTES_TO_PAGES (Length) * sizeof(PFN_NUMBER),
                                 '  mM');

    if (Mdl == NULL) {
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    //
    // Initialize the MDL for the request.
    //

    MmInitializeMdl(Mdl, WorkingSetInfo, Length);

    try {
        MmProbeAndLockPages (Mdl, KeGetPreviousMode(), IoWriteAccess);
    } except (EXCEPTION_EXECUTE_HANDLER) {
        ExFreePool (Mdl);
        return GetExceptionCode();
    }

    Info = MmGetSystemAddressForMdlSafe (Mdl, NormalPagePriority);

    if (Info == NULL) {
        MmUnlockPages (Mdl);
        ExFreePool (Mdl);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    if (PsGetCurrentProcess() != Process) {
        KeAttachProcess (&Process->Pcb);
        Attached = TRUE;
    }
    else {
        Attached = FALSE;
    }

    LOCK_WS (Process);

    status = STATUS_SUCCESS;

    if (Process->AddressSpaceDeleted != 0) {
        status = STATUS_PROCESS_IS_TERMINATING;
    }

    WsSize = Process->Vm.WorkingSetSize;
    Info->NumberOfEntries = WsSize;

    if ((WsSize * sizeof(ULONG)) >= Length) {
        status = STATUS_INFO_LENGTH_MISMATCH;
    }

    if (status != STATUS_SUCCESS) {
        UNLOCK_WS (Process);
        if (Attached == TRUE) {
            KeDetachProcess ();
        }
        MmUnlockPages (Mdl);
        ExFreePool (Mdl);
        return status;
    }

    Wsle = MmWsle;
    LastWsle = &MmWsle[MmWorkingSetList->LastEntry];
    Entry = &Info->WorkingSetInfo[0];
    LastEntry = (PMEMORY_WORKING_SET_BLOCK)(
                            (PCHAR)Info + (Length & (~(sizeof(ULONG) - 1))));

    do {
        if (Wsle->u1.e1.Valid == 1) {
            Entry->VirtualPage = Wsle->u1.e1.VirtualPageNumber;
            PointerPte = MiGetPteAddress (Wsle->u1.VirtualAddress);
            ASSERT (PointerPte->u.Hard.Valid == 1);
            Pfn1 = MI_PFN_ELEMENT (PointerPte->u.Hard.PageFrameNumber);

            Entry->Shared = Pfn1->u3.e1.PrototypePte;
            if (Pfn1->u3.e1.PrototypePte == 0) {
                Entry->ShareCount = 0;
                Entry->Protection = MI_GET_PROTECTION_FROM_SOFT_PTE(&Pfn1->OriginalPte);
            } else {
                if (Pfn1->u2.ShareCount <= 7) {
                    Entry->ShareCount = Pfn1->u2.ShareCount;
                }
                else {
                    Entry->ShareCount = 7;
                }
                if (Wsle->u1.e1.SameProtectAsProto == 1) {
                    Entry->Protection = MI_GET_PROTECTION_FROM_SOFT_PTE(&Pfn1->OriginalPte);
                } else {
                    Entry->Protection = Wsle->u1.e1.Protection;
                }
            }
            Entry += 1;
        }
        Wsle += 1;
    }while ((Entry < LastEntry) && (Wsle <= LastWsle));

    UNLOCK_WS (Process);
    if (Attached == TRUE) {
        KeDetachProcess ();
    }
    MmUnlockPages (Mdl);
    ExFreePool (Mdl);
    return STATUS_SUCCESS;
}
