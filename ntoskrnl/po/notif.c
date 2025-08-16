/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
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

NTSTATUS
NTAPI
PoCancelDeviceNotify(
    _In_ PVOID NotifyBlock)
{
    /* FIXME */
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

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
    /* FIXME */
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

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
