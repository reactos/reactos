/*++ NDK Version: 0098

Copyright (c) Alex Ionescu.  All rights reserved.

Header Name:

    kdfuncs.h

Abstract:

    Function definitions for the Kernel Debugger.

Author:

    Alex Ionescu (alexi@tinykrnl.org) - Updated - 27-Feb-2006

--*/

#ifndef _KDFUNCS_H
#define _KDFUNCS_H

//
// Dependencies
//
#include <umtypes.h>
#include <kdtypes.h>

#ifndef NTOS_MODE_USER

//
// Debugger API
//
NTSTATUS
NTAPI
KdSystemDebugControl(
    _In_ SYSDBG_COMMAND Command,
    _In_reads_bytes_(InputBufferLength) PVOID InputBuffer,
    _In_ ULONG InputBufferLength,
    _Out_writes_bytes_(OutputBufferLength) PVOID OutputBuffer,
    _In_ ULONG OutputBufferLength,
    _Out_opt_ PULONG ReturnLength,
    _In_ KPROCESSOR_MODE PreviousMode
);

BOOLEAN
NTAPI
KdPollBreakIn(
    VOID
);

#endif

//
// Native Calls
//
NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueryDebugFilterState(
    _In_ ULONG ComponentId,
    _In_ ULONG Level
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetDebugFilterState(
    _In_ ULONG ComponentId,
    _In_ ULONG Level,
    _In_ BOOLEAN State
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtSystemDebugControl(
    _In_ SYSDBG_COMMAND Command,
    _In_reads_bytes_(InputBufferLength) PVOID InputBuffer,
    _In_ ULONG InputBufferLength,
    _Out_writes_bytes_(OutputBufferLength) PVOID OutputBuffer,
    _In_ ULONG OutputBufferLength,
    _Out_opt_ PULONG ReturnLength
);

NTSYSAPI
NTSTATUS
NTAPI
ZwQueryDebugFilterState(
     ULONG ComponentId,
     ULONG Level
);

NTSYSAPI
NTSTATUS
NTAPI
ZwSetDebugFilterState(
    ULONG ComponentId,
    ULONG Level,
    BOOLEAN State
);

NTSYSAPI
NTSTATUS
NTAPI
ZwSystemDebugControl(
    _In_ SYSDBG_COMMAND Command,
    _In_reads_bytes_(InputBufferLength) PVOID InputBuffer,
    _In_ ULONG InputBufferLength,
    _Out_writes_bytes_(OutputBufferLength) PVOID OutputBuffer,
    _In_ ULONG OutputBufferLength,
    _Out_opt_ PULONG ReturnLength
);
#endif
