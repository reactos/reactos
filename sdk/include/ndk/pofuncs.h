/*++ NDK Version: 0098

Copyright (c) Alex Ionescu.  All rights reserved.

Header Name:

    pofuncs.h

Abstract:

    Function definitions for the Power Subsystem.

Author:

    Alex Ionescu (alexi@tinykrnl.org) - Updated - 27-Feb-2006

--*/

#ifndef _POFUNCS_H
#define _POFUNCS_H
#ifndef _PO_DDK_

//
// Dependencies
//
#include <umtypes.h>

//
// Native Calls
//
NTSYSCALLAPI
NTSTATUS
NTAPI
NtInitiatePowerAction(
    _In_ POWER_ACTION SystemAction,
    _In_ SYSTEM_POWER_STATE MinSystemState,
    _In_ ULONG Flags,
    _In_ BOOLEAN Asynchronous
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtPowerInformation(
    _In_ POWER_INFORMATION_LEVEL PowerInformationLevel,
    _In_bytecount_(InputBufferLength) PVOID InputBuffer,
    _In_ ULONG InputBufferLength,
    _Out_bytecap_(OutputBufferLength) PVOID OutputBuffer,
    _In_ ULONG OutputBufferLength
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetSystemPowerState(
    _In_ POWER_ACTION SystemAction,
    _In_ SYSTEM_POWER_STATE MinSystemState,
    _In_ ULONG Flags
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtGetDevicePowerState(
    _In_ HANDLE Device,
    _In_ PDEVICE_POWER_STATE PowerState
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtRequestWakeupLatency(
    _In_ LATENCY_TIME latency
);

NTSYSCALLAPI
BOOLEAN
NTAPI
NtIsSystemResumeAutomatic(VOID);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetThreadExecutionState(
    _In_ EXECUTION_STATE esFlags,
    _Out_ EXECUTION_STATE *PreviousFlags
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtInitiatePowerAction(
    _In_ POWER_ACTION SystemAction,
    _In_ SYSTEM_POWER_STATE MinSystemState,
    _In_ ULONG Flags,
    _In_ BOOLEAN Asynchronous
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtRequestDeviceWakeup(
    _In_ HANDLE Device
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtCancelDeviceWakeupRequest(
    _In_ HANDLE Device
);
#endif

NTSYSAPI
NTSTATUS
NTAPI
ZwInitiatePowerAction(
    _In_ POWER_ACTION SystemAction,
    _In_ SYSTEM_POWER_STATE MinSystemState,
    _In_ ULONG Flags,
    _In_ BOOLEAN Asynchronous
);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
ZwPowerInformation(
    _In_ POWER_INFORMATION_LEVEL InformationLevel,
    _In_reads_bytes_opt_(InputBufferLength) PVOID InputBuffer,
    _In_ ULONG InputBufferLength,
    _Out_writes_bytes_opt_(OutputBufferLength) PVOID OutputBuffer,
    _In_ ULONG OutputBufferLength
);

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwSetSystemPowerState(
    _In_ POWER_ACTION SystemAction,
    _In_ SYSTEM_POWER_STATE MinSystemState,
    _In_ ULONG Flags
);
#endif
