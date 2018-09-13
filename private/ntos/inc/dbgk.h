/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) 1989  Microsoft Corporation

Module Name:

    dbgk.h

Abstract:

    This header file describes public data structures and functions
    that make up the kernel mode portion of the Dbg subsystem.

Author:

    Mark Lucovsky (markl) 19-Jan-1990

Revision History:

--*/

#ifndef _DBGK_
#define _DBGK_

VOID
DbgkCreateThread(
    PVOID StartAddress
    );

VOID
DbgkExitThread(
    NTSTATUS ExitStatus
    );

VOID
DbgkExitProcess(
    NTSTATUS ExitStatus
    );

VOID
DbgkMapViewOfSection(
    IN HANDLE SectionHandle,
    IN PVOID BaseAddress,
    IN ULONG SectionOffset,
    IN ULONG_PTR ViewSize
    );

VOID
DbgkUnMapViewOfSection(
    IN PVOID BaseAddress
    );

BOOLEAN
DbgkForwardException (
    IN PEXCEPTION_RECORD ExceptionRecord,
    IN BOOLEAN DebugException,
    IN BOOLEAN SecondChance
    );

#endif // _DBGK_
