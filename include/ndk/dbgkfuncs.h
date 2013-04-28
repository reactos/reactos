/*++ NDK Version: 0098

Copyright (c) Alex Ionescu.  All rights reserved.

Header Name:

    dbgkfuncs.h

Abstract:

    Function definitions for the User Mode Debugging Facility.

Author:

    Alex Ionescu (alexi@tinykrnl.org) - Updated - 27-Feb-2006

--*/

#ifndef _DBGKFUNCS_H
#define _DBGKFUNCS_H

//
// Dependencies
//
#include <umtypes.h>
#include <dbgktypes.h>

//
// Native calls
//
NTSYSCALLAPI
NTSTATUS
NTAPI
NtDebugActiveProcess(
    _In_ HANDLE Process,
    _In_ HANDLE DebugObject
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtCreateDebugObject(
    _Out_ PHANDLE DebugHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ POBJECT_ATTRIBUTES ObjectAttributes,
    _In_ ULONG Flags
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtDebugContinue(
    _In_ HANDLE DebugObject,
    _In_ PCLIENT_ID AppClientId,
    _In_ NTSTATUS ContinueStatus
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtWaitForDebugEvent(
    _In_ HANDLE DebugObject,
    _In_ BOOLEAN Alertable,
    _In_opt_ PLARGE_INTEGER Timeout,
    _Out_ PDBGUI_WAIT_STATE_CHANGE StateChange
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtRemoveProcessDebug(
    _In_ HANDLE Process,
    _In_ HANDLE DebugObject
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetInformationDebugObject(
    _In_ HANDLE DebugObject,
    _In_ DEBUGOBJECTINFOCLASS InformationClass,
    _In_ PVOID Information,
    _In_ ULONG InformationLength,
    _Out_opt_ PULONG ReturnLength
);

NTSYSAPI
NTSTATUS
NTAPI
ZwDebugActiveProcess(
    _In_ HANDLE Process,
    _In_ HANDLE DebugObject
);

NTSYSAPI
NTSTATUS
NTAPI
ZwCreateDebugObject(
    _Out_ PHANDLE DebugHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ POBJECT_ATTRIBUTES ObjectAttributes,
    _In_ ULONG Flags
);

NTSYSAPI
NTSTATUS
NTAPI
ZwDebugContinue(
    _In_ HANDLE DebugObject,
    _In_ PCLIENT_ID AppClientId,
    _In_ NTSTATUS ContinueStatus
);

NTSYSAPI
NTSTATUS
NTAPI
ZwRemoveProcessDebug(
    _In_ HANDLE Process,
    _In_ HANDLE DebugObject
);

NTSYSAPI
NTSTATUS
NTAPI
ZwWaitForDebugEvent(
    _In_ HANDLE DebugObject,
    _In_ BOOLEAN Alertable,
    _In_opt_ PLARGE_INTEGER Timeout,
    _Out_ PDBGUI_WAIT_STATE_CHANGE StateChange
);

NTSYSAPI
NTSTATUS
NTAPI
ZwSetInformationDebugObject(
    _In_ HANDLE DebugObject,
    _In_ DEBUGOBJECTINFOCLASS InformationClass,
    _In_ PVOID Information,
    _In_ ULONG InformationLength,
    _Out_opt_ PULONG ReturnLength
);
#endif
