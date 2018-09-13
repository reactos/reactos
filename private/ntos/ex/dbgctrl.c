/*++

Copyright (c) 1992-1993  Microsoft Corporation

Module Name:

    dbgctrl.c

Abstract:

    This module implements the NtDebugControl service

Author:

    Chuck Lenzmeier (chuckl) 2-Dec-1992

Environment:

    Kernel mode only.

Revision History:

--*/

#include "exp.h"

#pragma hdrstop
#include "kdp.h"

#if defined(ALLOC_PRAGMA)
#pragma alloc_text(PAGE, NtSystemDebugControl)
#endif


NTSTATUS
NtSystemDebugControl (
    IN SYSDBG_COMMAND Command,
    IN PVOID InputBuffer,
    IN ULONG InputBufferLength,
    OUT PVOID OutputBuffer,
    IN ULONG OutputBufferLength,
    OUT PULONG ReturnLength OPTIONAL
    )

/*++

Routine Description:

    This function controls the system debugger.

Arguments:

    Command - The command to be executed.  One of the following:

        SysDbgQueryTraceInformation
        SysDbgSetTracepoint
        SysDbgSetSpecialCall
        SysDbgClearSpecialCalls
        SysDbgQuerySpecialCalls

    InputBuffer - A pointer to a buffer describing the input data for
        the request, if any.  The structure of this buffer varies
        depending upon Command.

    InputBufferLength - The length in bytes of InputBuffer.

    OutputBuffer - A pointer to a buffer that is to receive the output
        data for the request, if any.  The structure of this buffer
        varies depending upon Command.

    OutputBufferLength - The length in bytes of OutputBuffer.

    ReturnLength - A optional pointer to a ULONG that is to receive the
        output data length for the request.

Return Value:

    Returns one of the following status codes:

        STATUS_SUCCESS - normal, successful completion.

        STATUS_INVALID_INFO_CLASS - The Command parameter did not
            specify a valid value.

        STATUS_INFO_LENGTH_MISMATCH - The value of the Length field in the
            Parameters buffer was not correct.

        STATUS_ACCESS_VIOLATION - Either the Parameters buffer pointer
            or a pointer within the Parameters buffer specified an
            invalid address.

        STATUS_INSUFFICIENT_RESOURCES - Insufficient system resources exist
            for this request to complete.

--*/

{
    NTSTATUS status = STATUS_SUCCESS;
    BOOLEAN releaseModuleResoure = FALSE;
    ULONG length = 0;
    KPROCESSOR_MODE PreviousMode;

    PreviousMode = KeGetPreviousMode();

    if (!SeSinglePrivilegeCheck( SeDebugPrivilege, PreviousMode)) {
        return STATUS_ACCESS_DENIED;
    }

    //
    // Operate within a try block in order to catch errors.
    //

    try {

        //
        // Probe input and output buffers, if previous mode is not
        // kernel.
        //

        if ( PreviousMode != KernelMode ) {

            if ( InputBufferLength != 0 ) {
                ProbeForRead( InputBuffer, InputBufferLength, sizeof(ULONG) );
            }

            if ( OutputBufferLength != 0 ) {
                ProbeForWrite( OutputBuffer, OutputBufferLength, sizeof(ULONG) );
            }

            if ( ARGUMENT_PRESENT(ReturnLength) ) {
                ProbeForWriteUlong( ReturnLength );
            }
        }

        //
        // Switch on the command code.
        //

        switch ( Command ) {

#if i386

        case SysDbgQueryTraceInformation:

            status = KdGetTraceInformation(
                        OutputBuffer,
                        OutputBufferLength,
                        &length
                        );

            break;

        case SysDbgSetTracepoint:

            if ( InputBufferLength != sizeof(DBGKD_MANIPULATE_STATE64) ) {
                return STATUS_INFO_LENGTH_MISMATCH;
            }

            KdSetInternalBreakpoint( InputBuffer );

            break;

        case SysDbgSetSpecialCall:

            if ( InputBufferLength != sizeof(PVOID) ) {
                return STATUS_INFO_LENGTH_MISMATCH;
            }

            KdSetSpecialCall( InputBuffer, NULL );

            break;

        case SysDbgClearSpecialCalls:

            KdClearSpecialCalls( );

            break;

        case SysDbgQuerySpecialCalls:

            status = KdQuerySpecialCalls(
                        OutputBuffer,
                        OutputBufferLength,
                        &length
                        );

            break;

#endif

        case SysDbgBreakPoint:
            if (KdDebuggerEnabled) {
                DbgBreakPointWithStatus(DBG_STATUS_DEBUG_CONTROL);
            } else {
                status = STATUS_UNSUCCESSFUL;
            }
            break;

        default:

            //
            // Invalid Command.
            //

            status = STATUS_INVALID_INFO_CLASS;
        }

        if ( ARGUMENT_PRESENT(ReturnLength) ) {
            *ReturnLength = length;
        }
    }

    except ( EXCEPTION_EXECUTE_HANDLER ) {

        if ( releaseModuleResoure ) {
            ExReleaseResource( &PsLoadedModuleResource );
            KeLeaveCriticalRegion();
        }

        status = GetExceptionCode();

    }

    return status;

} // NtSystemDebugControl
