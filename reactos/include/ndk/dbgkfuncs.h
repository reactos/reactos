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

//
// Native calls
//
NTSYSCALLAPI
NTSTATUS
NTAPI
NtCreateDebugObject(
    OUT PHANDLE DebugHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    IN BOOLEAN KillProcessOnExit
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtDebugContinue(
    IN HANDLE DebugObject,
    IN PCLIENT_ID AppClientId,
    IN NTSTATUS ContinueStatus
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtWaitForDebugEvent(
    IN HANDLE DebugObject,
    IN BOOLEAN Alertable,
    IN PLARGE_INTEGER Timeout OPTIONAL,
    OUT PDBGUI_WAIT_STATE_CHANGE StateChange
);

NTSYSAPI
NTSTATUS
NTAPI
ZwCreateDebugObject(
    OUT PHANDLE DebugHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    IN BOOLEAN KillProcessOnExit
);

NTSYSAPI
NTSTATUS
NTAPI
ZwDebugContinue(
    IN HANDLE DebugObject,
    IN PCLIENT_ID AppClientId,
    IN NTSTATUS ContinueStatus
);

NTSYSAPI
NTSTATUS
NTAPI
ZwWaitForDebugEvent(
    IN HANDLE DebugObject,
    IN BOOLEAN Alertable,
    IN PLARGE_INTEGER Timeout OPTIONAL,
    OUT PDBGUI_WAIT_STATE_CHANGE StateChange
);
#endif
