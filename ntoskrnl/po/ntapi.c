/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Power Manager NT API system calls
 * COPYRIGHT:   Copyright 2023 George Bișoc <george.bisoc@reactos.org>
 */

/* INCLUDES *******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

WORK_QUEUE_ITEM PopUnlockMemoryWorkItem;
KEVENT PopUnlockMemoryCompleteEvent;

/* PRIVATE FUNCTIONS **********************************************************/

_Use_decl_annotations_
VOID
NTAPI
PopUnlockMemoryWorker(
    _In_ PVOID Parameter)
{
    UNIMPLEMENTED;
    return;
}

/* SYSTEM CALLS ***************************************************************/

NTSTATUS
NTAPI
NtInitiatePowerAction(
    _In_ POWER_ACTION SystemAction,
    _In_ SYSTEM_POWER_STATE MinSystemState,
    _In_ ULONG Flags,
    _In_ BOOLEAN Asynchronous)
{
    /* FIXME */
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
NtPowerInformation(
    _In_ POWER_INFORMATION_LEVEL PowerInformationLevel,
    _In_reads_bytes_opt_(InputBufferLength) PVOID InputBuffer,
    _In_ ULONG InputBufferLength,
    _Out_writes_bytes_opt_(OutputBufferLength) PVOID OutputBuffer,
    _In_ ULONG OutputBufferLength)
{
    /* FIXME */
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
NtGetDevicePowerState(
    _In_ HANDLE Device,
    _In_ PDEVICE_POWER_STATE PowerState)
{
    /* FIXME */
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

BOOLEAN
NTAPI
NtIsSystemResumeAutomatic(VOID)
{
    /* FIXME */
    UNIMPLEMENTED;;
    return FALSE;
}

NTSTATUS
NTAPI
NtRequestWakeupLatency(
    _In_ LATENCY_TIME Latency)
{
    /* FIXME */
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
NtSetThreadExecutionState(
    _In_ EXECUTION_STATE esFlags,
    _Out_ EXECUTION_STATE *PreviousFlags)
{
    /* FIXME */
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
NtSetSystemPowerState(
    _In_ POWER_ACTION SystemAction,
    _In_ SYSTEM_POWER_STATE MinSystemState,
    _In_ ULONG Flags)
{
    /* FIXME */
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
NtRequestDeviceWakeup(
    _In_ HANDLE DeviceHandle)
{
    /* FIXME */
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
NtCancelDeviceWakeupRequest(
    _In_ HANDLE DeviceHandle)
{
    /* FIXME */
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/* EOF */
