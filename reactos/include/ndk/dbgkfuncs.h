/*++ NDK Version: 0095

Copyright (c) Alex Ionescu.  All rights reserved.

Header Name:

    dbgkfuncs.h

Abstract:

    Function definitions for the User Mode Debugging Facility.

Author:

    Alex Ionescu (alex.ionescu@reactos.com)   06-Oct-2004

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

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwCreateDebugObject(
    OUT PHANDLE DebugHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    IN BOOLEAN KillProcessOnExit
);

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwDebugContinue(
    IN HANDLE DebugObject,
    IN PCLIENT_ID AppClientId,
    IN NTSTATUS ContinueStatus
);

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwWaitForDebugEvent(
    IN HANDLE DebugObject,
    IN BOOLEAN Alertable,
    IN PLARGE_INTEGER Timeout OPTIONAL,
    OUT PDBGUI_WAIT_STATE_CHANGE StateChange
);
#endif
