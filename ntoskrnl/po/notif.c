/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Power notifications routines
 * COPYRIGHT:   Copyright 2023 George Bișoc <george.bisoc@reactos.org>
 */

/* INCLUDES *******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

ERESOURCE PopNotifyDeviceLock;

/* PRIVATE FUNCTIONS **********************************************************/

/* PUBLIC FUNCTIONS ***********************************************************/

/**
 * @brief
 * Frees a device notification block.
 *
 * @param[in] NotifyBlock
 * A pointer to a device notify block to be freed.
 */
NTSTATUS
NTAPI
PoCancelDeviceNotify(
    _In_ PVOID NotifyBlock)
{
    /* On Windows 7 and above this function is no longer supported */
    UNREFERENCED_PARAMETER(NotifyBlock);
    return STATUS_NOT_SUPPORTED;
}

/**
 * @brief
 * Register a device for power notifications. The parameters of this
 * function are yet to be investigated.
 */
NTSTATUS
NTAPI
PoRegisterDeviceNotify(
    _Out_ PVOID Unknown0,
    _In_ ULONG Unknown1,
    _In_ ULONG Unknown2,
    _In_ ULONG Unknown3,
    _In_ PVOID Unknown4,
    _In_ PVOID Unknown5)
{
    /* On Windows 7 and above this function is no longer supported */
    return STATUS_NOT_SUPPORTED;
}

/**
 * @brief
 * Notifies all the system time callbacks of a change in system time set.
 */
VOID
NTAPI
PoNotifySystemTimeSet(VOID)
{
    KIRQL OldIrql;

    /* Notify the system time set callback only if Win32k registered one */
    if (PopEventCallout)
    {
        KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);
        ExNotifyCallback(SetSystemTimeCallback, NULL, NULL);
        PopRequestPolicyWorker(PolicyWorkerTimeChange);
        PopCheckForPendingWorkers();
        KeLowerIrql(OldIrql);
    }
}

/* EOF */
