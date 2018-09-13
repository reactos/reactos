/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

   flushbuf.c

Abstract:

    This module contains the code to flush the write buffer or otherwise
    synchronize writes on the host processor.  Also, contains code
    to flush instruction cache of specified process.

Author:

    David N. Cutler 24-Apr-1991

Revision History:

--*/

#include "mi.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,NtFlushWriteBuffer)
#pragma alloc_text(PAGE,NtFlushInstructionCache)
#endif


NTSTATUS
NtFlushWriteBuffer (
   VOID
   )

/*++

Routine Description:

    This function flushes the write buffer on the current processor.

Arguments:

    None.

Return Value:

    STATUS_SUCCESS.

--*/

{
    PAGED_CODE();

    KeFlushWriteBuffer();
    return STATUS_SUCCESS;
}

ULONG
MiFlushRangeFilter (
    IN PEXCEPTION_POINTERS ExceptionPointers,
    IN PVOID *BaseAddress,
    IN PULONG Length,
    IN PBOOLEAN Retry
    )

/*++

Routine Description:

    This is the exception handler used by NtFlushInstructionCache to protect
    against bad virtual addresses passed to KeSweepIcacheRange.  If an
    access violation occurs, this routine causes NtFlushInstructionCache to
    restart the sweep at the page following the failing page.

Arguments:

    ExceptionPointers - Supplies exception information.

    BaseAddress - Supplies a pointer to address the base of the region
        being flushed.  If the failing address is not in the last page
        of the region, this routine updates BaseAddress to point to the
        next page of the region.

    Length - Supplies a pointer the length of the region being flushed.
        If the failing address is not in the last page of the region,
        this routine updates Length to reflect restarting the flush at
        the next page of the region.

    Retry - Supplies a pointer to a boolean that the caller has initialized
        to FALSE.  This routine sets this boolean to TRUE if an access
        violation occurs in a page before the last page of the flush region.

Return Value:

    EXCEPTION_EXECUTE_HANDLER.

--*/

{
    PEXCEPTION_RECORD ExceptionRecord;
    ULONG_PTR BadVa;
    ULONG_PTR NextVa;
    ULONG_PTR EndVa;

    ExceptionRecord = ExceptionPointers->ExceptionRecord;

    //
    // If the exception was an access violation, skip the current page of the
    // region and move to the next page.
    //

    if ( ExceptionRecord->ExceptionCode == STATUS_ACCESS_VIOLATION ) {

        //
        // Get the failing address, calculate the base address of the next page,
        // and calculate the address at the end of the region.
        //

        BadVa = ExceptionRecord->ExceptionInformation[1];
        NextVa = ROUND_TO_PAGES( BadVa + 1 );
        EndVa = *(PULONG_PTR)BaseAddress + *Length;

        //
        // If the next page didn't wrap, and the next page is below the end of
        // the region, update Length and BaseAddress appropriately and set Retry
        // to TRUE to indicate to NtFlushInstructionCache that it should call
        // KeSweepIcacheRange again.
        //

        if ( (NextVa > BadVa) && (NextVa < EndVa) ) {
            *Length = (ULONG) (EndVa - NextVa);
            *BaseAddress = (PVOID)NextVa;
            *Retry = TRUE;
        }
    }

    return EXCEPTION_EXECUTE_HANDLER;
}

NTSTATUS
NtFlushInstructionCache (
    IN HANDLE ProcessHandle,
    IN PVOID BaseAddress OPTIONAL,
    IN ULONG Length
    )

/*++

Routine Description:

    This function flushes the instruction cache for the specified process.

Arguments:

    ProcessHandle - Supplies a handle to the process in which the instruction
        cache is to be flushed. Must have PROCESS_VM_WRITE access to the
        specified process.

    BaseAddress - Supplies an optional pointer to base of the region that
        is flushed.

    Length - Supplies the length of the region that is flushed if the base
        address is specified.

Return Value:

    STATUS_SUCCESS.

--*/

{

    KPROCESSOR_MODE PreviousMode;
    PEPROCESS Process;
    NTSTATUS Status;
    BOOLEAN Retry;
    PVOID RangeBase;
    ULONG RangeLength;

    PAGED_CODE();

    PreviousMode = KeGetPreviousMode();

    //
    // If the base address is not specified, or the base address is specified
    // and the length is not zero, then flush the specified instruction cache
    // range.
    //

    if ((ARGUMENT_PRESENT(BaseAddress) == FALSE) || (Length != 0)) {

        //
        // If previous mode is user and the range specified falls in kernel
        // address space, return an error.
        //

        if ((ARGUMENT_PRESENT(BaseAddress) != FALSE) &&
            (PreviousMode != KernelMode)) {
            try {
                ProbeForRead(BaseAddress, Length, sizeof(UCHAR));
            } except(EXCEPTION_EXECUTE_HANDLER) {
                return GetExceptionCode();
            }
        }

        //
        // If the specified process is not the current process, then
        // the process must be attached to during the flush.
        //

        if (ProcessHandle != NtCurrentProcess()) {

            //
            // Reference the specified process checking for PROCESS_VM_WRITE
            // access.
            //

            Status = ObReferenceObjectByHandle(ProcessHandle,
                                               PROCESS_VM_WRITE,
                                               PsProcessType,
                                               PreviousMode,
                                               (PVOID *)&Process,
                                               NULL);

            if (!NT_SUCCESS(Status)) {
                return Status;
            }

            //
            // Attach to the process.
            //

            KeAttachProcess(&Process->Pcb);
        }

        //
        // If the base address is not specified, sweep the entire instruction
        // cache.  If the base address is specified, flush the specified range.
        //

        if (ARGUMENT_PRESENT(BaseAddress) == FALSE) {
            KeSweepIcache(FALSE);

        } else {

            //
            // Parts of the specified range may be invalid.  An exception
            // handler is used to skip over those parts.  Before calling
            // KeSweepIcacheRange, we set Retry to FALSE.  If an access
            // violation occurs in KeSweepIcacheRange, the MiFlushRangeFilter
            // exception filter is called.  It updates RangeBase and
            // RangeLength to skip over the failing page, and sets Retry to
            // TRUE.  As long as Retry is TRUE, we continue to call
            // KeSweepIcacheRange.
            //

            RangeBase = BaseAddress;
            RangeLength = Length;

            do {
                Retry = FALSE;
                try {
                    KeSweepIcacheRange(FALSE, RangeBase, RangeLength);
                } except(MiFlushRangeFilter(GetExceptionInformation(),
                                            &RangeBase,
                                            &RangeLength,
                                            &Retry)) {
                    if (GetExceptionCode() != STATUS_ACCESS_VIOLATION) {
                        Status = GetExceptionCode();
                    }
                }
            } while (Retry != FALSE);
        }

        //
        // If the specified process is not the current process, then
        // detach from it and dereference it.
        //

        if (ProcessHandle != NtCurrentProcess()) {
            KeDetachProcess();
            ObDereferenceObject(Process);
        }
    }

    return STATUS_SUCCESS;
}

