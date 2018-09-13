/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

   readwrt.c

Abstract:

    This module contains the routines which implement the capability
    to read and write the virtual memory of a target process.

Author:

    Lou Perazzoli (loup) 22-May-1989
    Landy Wang (landyw) 02-June-1997

Revision History:

--*/

#include "mi.h"

//
// The maximum amount to try to Probe and Lock is 14 pages, this
// way it always fits in a 16 page allocation.
//

#define MAX_LOCK_SIZE ((ULONG)(14 * PAGE_SIZE))

//
// The maximum to move in a single block is 64k bytes.
//

#define MAX_MOVE_SIZE (LONG)0x10000

//
// The minimum to move is a single block is 128 bytes.
//

#define MINIMUM_ALLOCATION (LONG)128

//
// Define the pool move threshold value.
//

#define POOL_MOVE_THRESHOLD 511

//
// Define forward referenced procedure prototypes.
//

NTSTATUS
MiValidateUserTransfer(
     IN PVOID BaseAddress,
     IN PVOID Buffer,
     IN ULONG BufferSize
     );

ULONG
MiGetExceptionInfo (
    IN PEXCEPTION_POINTERS ExceptionPointers,
    IN PULONG_PTR BadVa1,
    IN PULONG_PTR BadVa2
    );

NTSTATUS
MiDoMappedCopy (
     IN PEPROCESS FromProcess,
     IN PVOID FromAddress,
     IN PEPROCESS ToProcess,
     OUT PVOID ToAddress,
     IN ULONG BufferSize,
     IN KPROCESSOR_MODE PreviousMode,
     OUT PULONG NumberOfBytesRead
     );

NTSTATUS
MiDoPoolCopy (
     IN PEPROCESS FromProcess,
     IN PVOID FromAddress,
     IN PEPROCESS ToProcess,
     OUT PVOID ToAddress,
     IN ULONG BufferSize,
     IN KPROCESSOR_MODE PreviousMode,
     OUT PULONG NumberOfBytesRead
     );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,MiGetExceptionInfo)
#pragma alloc_text(PAGE,NtReadVirtualMemory)
#pragma alloc_text(PAGE,NtWriteVirtualMemory)
#pragma alloc_text(PAGE,MiDoMappedCopy)
#pragma alloc_text(PAGE,MiDoPoolCopy)
#pragma alloc_text(PAGE,MiValidateUserTransfer)
#endif

#define COPY_STACK_SIZE 64

NTSTATUS
NtReadVirtualMemory (
     IN HANDLE ProcessHandle,
     IN PVOID BaseAddress,
     OUT PVOID Buffer,
     IN ULONG BufferSize,
     OUT PULONG NumberOfBytesRead OPTIONAL
     )

/*++

Routine Description:

    This function copies the specified address range from the specified
    process into the specified address range of the current process.

Arguments:

     ProcessHandle - Supplies an open handle to a process object.

     BaseAddress - Supplies the base address in the specified process
          to be read.

     Buffer - Supplies the address of a buffer which receives the
          contents from the specified process address space.

     BufferSize - Supplies the requested number of bytes to read from
          the specified process.

     NumberOfBytesRead - Receives the actual number of bytes
          transferred into the specified buffer.

Return Value:

    TBS

--*/

{

    ULONG BytesCopied;
    KPROCESSOR_MODE PreviousMode;
    PEPROCESS Process;
    NTSTATUS Status;

    PAGED_CODE();

    //
    // Get the previous mode and probe output argument if necessary.
    //

    PreviousMode = KeGetPreviousMode();
    if (PreviousMode != KernelMode) {

        Status = MiValidateUserTransfer(BaseAddress, Buffer, BufferSize);
        if (Status != STATUS_SUCCESS) {
            return Status;
        }

        if (ARGUMENT_PRESENT(NumberOfBytesRead)) {
            try {
                ProbeForWriteUlong(NumberOfBytesRead);

            } except(EXCEPTION_EXECUTE_HANDLER) {
                return GetExceptionCode();
            }
        }
    }

    //
    // If the buffer size is not zero, then attempt to read data from the
    // specified process address space into the current process address
    // space.
    //

    BytesCopied = 0;
    Status = STATUS_SUCCESS;
    if (BufferSize != 0) {

        //
        // Reference the target process.
        //

        Status = ObReferenceObjectByHandle(ProcessHandle,
                                           PROCESS_VM_READ,
                                           PsProcessType,
                                           PreviousMode,
                                           (PVOID *)&Process,
                                           NULL);

        //
        // If the process was successfully referenced, then attempt to
        // read the specified memory either by direct mapping or copying
        // through nonpaged pool.
        //

        if (Status == STATUS_SUCCESS) {

            Status = MmCopyVirtualMemory (Process,
                                          BaseAddress,
                                          PsGetCurrentProcess(),
                                          Buffer,
                                          BufferSize,
                                          PreviousMode,
                                          &BytesCopied);

            //
            // Dereference the target process.
            //

            ObDereferenceObject(Process);
        }
    }

    //
    // If requested, return the number of bytes read.
    //

    if (ARGUMENT_PRESENT(NumberOfBytesRead)) {
        try {
            *NumberOfBytesRead = BytesCopied;

        } except(EXCEPTION_EXECUTE_HANDLER) {
            NOTHING;
        }
    }

    return Status;
}
NTSTATUS
NtWriteVirtualMemory(
     IN HANDLE ProcessHandle,
     OUT PVOID BaseAddress,
     IN PVOID Buffer,
     IN ULONG BufferSize,
     OUT PULONG NumberOfBytesWritten OPTIONAL
     )

/*++

Routine Description:

    This function copies the specified address range from the current
    process into the specified address range of the specified process.

Arguments:

     ProcessHandle - Supplies an open handle to a process object.

     BaseAddress - Supplies the base address to be written to in the
          specified process.

     Buffer - Supplies the address of a buffer which contains the
          contents to be written into the specified process
          address space.

     BufferSize - Supplies the requested number of bytes to write
          into the specified process.

     NumberOfBytesWritten - Receives the actual number of
          bytes transferred into the specified address
          space.

Return Value:

    TBS

--*/

{
    ULONG BytesCopied;
    KPROCESSOR_MODE PreviousMode;
    PEPROCESS Process;
    NTSTATUS Status;

    PAGED_CODE();

    //
    // Get the previous mode and probe output argument if necessary.
    //

    PreviousMode = KeGetPreviousMode();
    if (PreviousMode != KernelMode) {

        Status = MiValidateUserTransfer(BaseAddress, Buffer, BufferSize);
        if (Status != STATUS_SUCCESS) {
            return Status;
        }

        if (ARGUMENT_PRESENT(NumberOfBytesWritten)) {
            try {
                ProbeForWriteUlong(NumberOfBytesWritten);

            } except(EXCEPTION_EXECUTE_HANDLER) {
                return GetExceptionCode();
            }
        }
    }

    //
    // If the buffer size is not zero, then attempt to write data from the
    // current process address space into the target process address space.
    //

    BytesCopied = 0;
    Status = STATUS_SUCCESS;
    if (BufferSize != 0) {

        //
        // Reference the target process.
        //

        Status = ObReferenceObjectByHandle(ProcessHandle,
                                           PROCESS_VM_WRITE,
                                           PsProcessType,
                                           PreviousMode,
                                           (PVOID *)&Process,
                                           NULL);

        //
        // If the process was successfully referenced, then attempt to
        // write the specified memory either by direct mapping or copying
        // through nonpaged pool.
        //

        if (Status == STATUS_SUCCESS) {

            Status = MmCopyVirtualMemory (PsGetCurrentProcess(),
                                          Buffer,
                                          Process,
                                          BaseAddress,
                                          BufferSize,
                                          PreviousMode,
                                          &BytesCopied);

            //
            // Dereference the target process.
            //

            ObDereferenceObject(Process);
        }
    }

    //
    // If requested, return the number of bytes read.
    //

    if (ARGUMENT_PRESENT(NumberOfBytesWritten)) {
        try {
            *NumberOfBytesWritten = BytesCopied;

        } except(EXCEPTION_EXECUTE_HANDLER) {
            NOTHING;
        }
    }

    return Status;
}


NTSTATUS
MmCopyVirtualMemory(
    IN PEPROCESS FromProcess,
    IN PVOID FromAddress,
    IN PEPROCESS ToProcess,
    OUT PVOID ToAddress,
    IN ULONG BufferSize,
    IN KPROCESSOR_MODE PreviousMode,
    OUT PULONG NumberOfBytesCopied
    )
{
    NTSTATUS Status;
    KIRQL OldIrql;
    PEPROCESS ProcessToLock;

    if (BufferSize == 0) {
        ASSERT (FALSE);         // No one should call with a zero size.
        return STATUS_SUCCESS;
    }

    ProcessToLock = FromProcess;
    if (FromProcess == PsGetCurrentProcess()) {
        ProcessToLock = ToProcess;
    }

    //
    // Make sure the process still has an address space.
    //

    MiLockSystemSpace(OldIrql);
    if (ProcessToLock->AddressSpaceDeleted != 0) {
        MiUnlockSystemSpace(OldIrql);
        return STATUS_PROCESS_IS_TERMINATING;
    }
    ProcessToLock->VmOperation += 1;
    MiUnlockSystemSpace(OldIrql);


    //
    // If the buffer size is greater than the pool move threshold,
    // then attempt to write the memory via direct mapping.
    //

    if (BufferSize > POOL_MOVE_THRESHOLD) {
        Status = MiDoMappedCopy(FromProcess,
                                FromAddress,
                                ToProcess,
                                ToAddress,
                                BufferSize,
                                PreviousMode,
                                NumberOfBytesCopied);

        //
        // If the completion status is not a working quota problem,
        // then finish the service. Otherwise, attempt to write the
        // memory through nonpaged pool.
        //

        if (Status != STATUS_WORKING_SET_QUOTA) {
            goto CompleteService;
        }

        *NumberOfBytesCopied = 0;
    }

    //
    // There was not enough working set quota to write the memory via
    // direct mapping or the size of the write was below the pool move
    // threshold. Attempt to write the specified memory through nonpaged
    // pool.
    //

    Status = MiDoPoolCopy(FromProcess,
                          FromAddress,
                          ToProcess,
                          ToAddress,
                          BufferSize,
                          PreviousMode,
                          NumberOfBytesCopied);

    //
    // Dereference the target process.
    //

CompleteService:

    //
    // Indicate that the vm operation is complete.
    //

    MiLockSystemSpace(OldIrql);
    ProcessToLock->VmOperation -= 1;
    if ((ProcessToLock->VmOperation == 0) &&
        (ProcessToLock->VmOperationEvent != NULL)) {
       KeSetEvent (ProcessToLock->VmOperationEvent, 0, FALSE);
    }
    MiUnlockSystemSpace(OldIrql);

    return Status;
}


ULONG
MiGetExceptionInfo (
    IN PEXCEPTION_POINTERS ExceptionPointers,
    IN PULONG_PTR BadVa1,
    IN PULONG_PTR BadVa2
    )

/*++

Routine Description:

    This routine examines a exception record and extracts the virtual
    address of an access violation, guard page violation, or in-page error.

Arguments:

    ExceptionPointers - Supplies a pointer to the exception record.

    BadVa - Receives the virtual address which caused the access violation.

Return Value:

    EXECUTE_EXCEPTION_HANDLER

--*/

{
    PEXCEPTION_RECORD ExceptionRecord;

    PAGED_CODE();

    //
    // If the exception code is an access violation, guard page violation,
    // or an in-page read error, then return the faulting address. Otherwise.
    // return a special address value.
    //

    *BadVa2 = 0;
    ExceptionRecord = ExceptionPointers->ExceptionRecord;
    if ((ExceptionRecord->ExceptionCode == STATUS_ACCESS_VIOLATION) ||
        (ExceptionRecord->ExceptionCode == STATUS_GUARD_PAGE_VIOLATION) ||
        (ExceptionRecord->ExceptionCode == STATUS_IN_PAGE_ERROR)) {

        //
        // The virtual address which caused the exception is the 2nd
        // parameter in the exception information array.
        //

        *BadVa1 = ExceptionRecord->ExceptionInformation[1];
        if (ExceptionRecord->NumberParameters == 3) {
            *BadVa2 = ExceptionRecord->ExceptionInformation[2];
        }

    } else {

        //
        // Unexpected exception - set the number of bytes copied to zero.
        //

        *BadVa2 = 0xFFFFFFFF;
    }

    return EXCEPTION_EXECUTE_HANDLER;
}

NTSTATUS
MiDoMappedCopy (
    IN PEPROCESS FromProcess,
    IN PVOID FromAddress,
    IN PEPROCESS ToProcess,
    OUT PVOID ToAddress,
    IN ULONG BufferSize,
    IN KPROCESSOR_MODE PreviousMode,
    OUT PULONG NumberOfBytesRead
    )

/*++

Routine Description:

    This function copies the specified address range from the specified
    process into the specified address range of the current process.

Arguments:

     FromProcess - Supplies an open handle to a process object.

     FromAddress - Supplies the base address in the specified process
                   to be read.

     ToProcess - Supplies an open handle to a process object.

     ToAddress - Supplies the address of a buffer which receives the
                 contents from the specified process address space.

     BufferSize - Supplies the requested number of bytes to read from
                  the specified process.

     PreviousMode - Supplies the previous processor mode.

     NumberOfBytesRead - Receives the actual number of bytes
                         transferred into the specified buffer.

Return Value:

    TBS

--*/

{

    ULONG AmountToMove;
    ULONG_PTR BadVa1;
    ULONG_PTR BadVa2;
    PEPROCESS CurrentProcess;
    LOGICAL Moving;
    LOGICAL Probing;
    BOOLEAN LockedMdlPages;
    PVOID InVa;
    ULONG LeftToMove;
    PULONG MappedAddress;
    ULONG MaximumMoved;
    PMDL Mdl;
    PFN_NUMBER MdlHack[(sizeof(MDL)/sizeof(PFN_NUMBER)) + (MAX_LOCK_SIZE >> PAGE_SHIFT) + 1];
    PVOID OutVa;
    LOGICAL MappingFailed;

    PAGED_CODE();

    MappingFailed = FALSE;

    //
    // Get the address of the current process object and initialize copy
    // parameters.
    //

    CurrentProcess = PsGetCurrentProcess();

    InVa = FromAddress;
    OutVa = ToAddress;

    MaximumMoved = MAX_LOCK_SIZE;
    if (BufferSize <= MAX_LOCK_SIZE) {
        MaximumMoved = BufferSize;
    }

    Mdl = (PMDL)&MdlHack[0];

    //
    // Map the data into the system part of the address space, then copy it.
    //

    LeftToMove = BufferSize;
    AmountToMove = MaximumMoved;
    while (LeftToMove > 0) {

        if (LeftToMove < AmountToMove) {

            //
            // Set to move the remaining bytes.
            //

            AmountToMove = LeftToMove;
        }

        KeDetachProcess();
        KeAttachProcess (&FromProcess->Pcb);

        //
        // We may be touching a user's memory which could be invalid,
        // declare an exception handler.
        //

        try {

            //
            // Probe to make sure that the specified buffer is accessible in
            // the target process.
            //

            MappedAddress = NULL;
            LockedMdlPages = FALSE;

            if ((InVa == FromAddress) && (PreviousMode != KernelMode)){
                Probing = TRUE;
                ProbeForRead (FromAddress, BufferSize, sizeof(CHAR));
            }
            Probing = FALSE;

            //
            // Initialize MDL for request.
            //

            MmInitializeMdl (Mdl, (PVOID)InVa, AmountToMove);

            Moving = TRUE;
            MmProbeAndLockPages (Mdl, PreviousMode, IoReadAccess);
            Moving = FALSE;

            LockedMdlPages = TRUE;

            MappedAddress = MmMapLockedPagesSpecifyCache (Mdl,
                                                          KernelMode,
                                                          MmCached,
                                                          NULL,
                                                          FALSE,
                                                          HighPagePriority);

            if (MappedAddress == NULL) {
                MappingFailed = TRUE;
                ExRaiseStatus(STATUS_INSUFFICIENT_RESOURCES);
            }

            //
            // Deattach from the FromProcess and attach to the ToProcess.
            //

            KeDetachProcess();
            KeAttachProcess (&ToProcess->Pcb);

            //
            // Now operating in the context of the ToProcess.
            //
            if ((InVa == FromAddress) && (PreviousMode != KernelMode)){
                Probing = TRUE;
                ProbeForWrite (ToAddress, BufferSize, sizeof(CHAR));
            }
            Probing = FALSE;

            RtlCopyMemory (OutVa, MappedAddress, AmountToMove);

        } except (MiGetExceptionInfo (GetExceptionInformation(),
                                      &BadVa1,
                                      &BadVa2)) {


            //
            // If an exception occurs during the move operation or probe,
            // return the exception code as the status value.
            //

            KeDetachProcess();
            if (MappedAddress != NULL) {
                MmUnmapLockedPages (MappedAddress, Mdl);
            }
            if (LockedMdlPages == TRUE) {
                MmUnlockPages (Mdl);
            }

            if (GetExceptionCode() == STATUS_WORKING_SET_QUOTA) {
                return STATUS_WORKING_SET_QUOTA;
            }

            if ((Probing == TRUE) || (MappingFailed == TRUE)) {
                return GetExceptionCode();

            } else {

                //
                // The failure occurred during the move operation, determine
                // which move failed, and calculate the number of bytes
                // actually moved.
                //

                if (Moving == TRUE) {
                    if (BadVa1 != 0xFFFFFFFF) {
                        *NumberOfBytesRead = (ULONG)((ULONG_PTR)BadVa2 - (ULONG_PTR)FromAddress);
                    }

                } else {
                    *NumberOfBytesRead = BufferSize - LeftToMove;
                }
            }

            return STATUS_PARTIAL_COPY;
        }
        MmUnmapLockedPages (MappedAddress, Mdl);
        MmUnlockPages (Mdl);

        LeftToMove -= AmountToMove;
        InVa = (PVOID)((ULONG_PTR)InVa + AmountToMove);
        OutVa = (PVOID)((ULONG_PTR)OutVa + AmountToMove);
    }

    KeDetachProcess();

    //
    // Set number of bytes moved.
    //

    *NumberOfBytesRead = BufferSize;
    return STATUS_SUCCESS;
}

NTSTATUS
MiDoPoolCopy (
     IN PEPROCESS FromProcess,
     IN PVOID FromAddress,
     IN PEPROCESS ToProcess,
     OUT PVOID ToAddress,
     IN ULONG BufferSize,
     IN KPROCESSOR_MODE PreviousMode,
     OUT PULONG NumberOfBytesRead
     )

/*++

Routine Description:

    This function copies the specified address range from the specified
    process into the specified address range of the current process.

Arguments:

     ProcessHandle - Supplies an open handle to a process object.

     BaseAddress - Supplies the base address in the specified process
                   to be read.

     Buffer - Supplies the address of a buffer which receives the
              contents from the specified process address space.

     BufferSize - Supplies the requested number of bytes to read from
                  the specified process.

     PreviousMode - Supplies the previous processor mode.

     NumberOfBytesRead - Receives the actual number of bytes
                         transferred into the specified buffer.

Return Value:

    TBS

--*/

{

    ULONG AmountToMove;
    ULONG_PTR BadVa1;
    ULONG_PTR BadVa2;
    PEPROCESS CurrentProcess;
    LOGICAL Moving;
    LOGICAL Probing;
    PVOID InVa;
    ULONG LeftToMove;
    ULONG MaximumMoved;
    PVOID OutVa;
    PVOID PoolArea;
    LONGLONG StackArray[COPY_STACK_SIZE];
    ULONG FreePool;

    PAGED_CODE();

    ASSERT (BufferSize != 0);

    //
    // Get the address of the current process object and initialize copy
    // parameters.
    //

    CurrentProcess = PsGetCurrentProcess();

    InVa = FromAddress;
    OutVa = ToAddress;

    //
    // Allocate non-paged memory to copy in and out of.
    //

    MaximumMoved = MAX_MOVE_SIZE;
    if (BufferSize <= MAX_MOVE_SIZE) {
        MaximumMoved = BufferSize;
    }

    FreePool = FALSE;
    if (BufferSize <= sizeof(StackArray)) {
        PoolArea = (PULONG)&StackArray[0];
    } else {
        do {
            PoolArea = ExAllocatePoolWithTag (NonPagedPool, MaximumMoved, 'wRmM');
            if (PoolArea != NULL) {
                FreePool = TRUE;
                break;
            }

            MaximumMoved = MaximumMoved >> 1;
            if (MaximumMoved <= sizeof(StackArray)) {
                PoolArea = (PULONG)&StackArray[0];
                break;
            }
        } while (TRUE);
    }

    //
    // Copy the data into pool, then copy back into the ToProcess.
    //

    LeftToMove = BufferSize;
    AmountToMove = MaximumMoved;
    while (LeftToMove > 0) {

        if (LeftToMove < AmountToMove) {

            //
            // Set to move the remaining bytes.
            //

            AmountToMove = LeftToMove;
        }

        KeDetachProcess();
        KeAttachProcess (&FromProcess->Pcb);

        //
        // We may be touching a user's memory which could be invalid,
        // declare an exception handler.
        //

        try {

            //
            // Probe to make sure that the specified buffer is accessible in
            // the target process.
            //

            if ((InVa == FromAddress) && (PreviousMode != KernelMode)){
                Probing = TRUE;
                ProbeForRead (FromAddress, BufferSize, sizeof(CHAR));
            }

            Probing = FALSE;

            Moving = TRUE;

            RtlCopyMemory (PoolArea, InVa, AmountToMove);

            Moving = FALSE;

            KeDetachProcess();
            KeAttachProcess (&ToProcess->Pcb);

            //
            // Now operating in the context of the ToProcess.
            //

            if ((InVa == FromAddress) && (PreviousMode != KernelMode)){
                Probing = TRUE;
                ProbeForWrite (ToAddress, BufferSize, sizeof(CHAR));
            }
            Probing = FALSE;

            RtlCopyMemory (OutVa, PoolArea, AmountToMove);

        } except (MiGetExceptionInfo (GetExceptionInformation(),
                                      &BadVa1,
                                      &BadVa2)) {

            //
            // If an exception occurs during the move operation or probe,
            // return the exception code as the status value.
            //

            KeDetachProcess();

            if (FreePool) {
                ExFreePool (PoolArea);
            }
            if (Probing == TRUE) {
                return GetExceptionCode();

            } else {

                //
                // The failure occurred during the move operation, determine
                // which move failed, and calculate the number of bytes
                // actually moved.
                //

                if (Moving == TRUE) {

                    //
                    // The failure occurred getting the data.
                    //

                    if (BadVa1 != 0xFFFFFFFF) {
                        *NumberOfBytesRead = (ULONG)((ULONG_PTR)(BadVa2 - (ULONG_PTR)FromAddress));
                    }

                } else {

                    //
                    // The failure occurred writing the data.
                    //

                    *NumberOfBytesRead = BufferSize - LeftToMove;
                }
            }

            return STATUS_PARTIAL_COPY;
        }

        LeftToMove -= AmountToMove;
        InVa = (PVOID)((ULONG_PTR)InVa + AmountToMove);
        OutVa = (PVOID)((ULONG_PTR)OutVa + AmountToMove);
    }

    if (FreePool) {
        ExFreePool (PoolArea);
    }
    KeDetachProcess();

    //
    // Set number of bytes moved.
    //

    *NumberOfBytesRead = BufferSize;
    return STATUS_SUCCESS;
}


NTSTATUS
MiValidateUserTransfer(
     IN PVOID BaseAddressPointer,
     IN PVOID BufferPointer,
     IN ULONG BufferSize
     )

/*++

Routine Description:

    This function checks whether the parameter source and destination address
    ranges lie within the user virtual address space.  This function does NOT
    guarantee that the addresses are in fact mapped or that faults will not
    occur - it just validates the address range.

Arguments:

     BaseAddress - Supplies the base address within a process to be read.

     Buffer - Supplies the address of a buffer which sends or receives the
              contents from the specified process address space.

     BufferSize - Supplies the requested number of bytes to read or write from
                  the specified process.

Return Value:

    STATUS_SUCCESS if valid user ranges, error otherwise.

Environment:

    None.

--*/

{
    ULONG_PTR   BaseAddress;
    ULONG_PTR   Buffer;

    BaseAddress = (ULONG_PTR)BaseAddressPointer;
    Buffer = (ULONG_PTR)BufferPointer;

    PAGED_CODE();

    //
    // check that both the base address and the target buffer qualify
    // as valid user address ranges.  Wrapping is not allowed.
    //
    if (BaseAddress + BufferSize < BaseAddress) {
        return STATUS_ACCESS_VIOLATION;
    }

    if (((PVOID)BaseAddress > MM_HIGHEST_USER_ADDRESS) ||
        ((PVOID)(BaseAddress + BufferSize) > MM_HIGHEST_USER_ADDRESS)) {

            return STATUS_ACCESS_VIOLATION;
    }

    if (Buffer + BufferSize < Buffer) {
        return STATUS_ACCESS_VIOLATION;
    }

    if (((PVOID)Buffer > MM_HIGHEST_USER_ADDRESS) ||
        ((PVOID)(Buffer + BufferSize) > MM_HIGHEST_USER_ADDRESS)) {

            return STATUS_ACCESS_VIOLATION;
    }

    return STATUS_SUCCESS;
}
