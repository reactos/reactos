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
    SYSDBG_COMMAND Command,
    PVOID InputBuffer,
    ULONG InputBufferLength,
    PVOID OutputBuffer,
    ULONG OutputBufferLength,
    PULONG ReturnLength,
    KPROCESSOR_MODE PreviousMode
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
     ULONG ComponentId,
     ULONG Level
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetDebugFilterState(
    ULONG ComponentId,
    ULONG Level,
    BOOLEAN State
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtSystemDebugControl(
    SYSDBG_COMMAND ControlCode,
    PVOID InputBuffer,
    ULONG InputBufferLength,
    PVOID OutputBuffer,
    ULONG OutputBufferLength,
    PULONG ReturnLength
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
    SYSDBG_COMMAND ControlCode,
    PVOID InputBuffer,
    ULONG InputBufferLength,
    PVOID OutputBuffer,
    ULONG OutputBufferLength,
    PULONG ReturnLength
);
#endif
