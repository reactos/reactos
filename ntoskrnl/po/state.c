/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Power states (Active & Idle) management for System and Devices infrastructure
 * COPYRIGHT:   Copyright 2023 George Bișoc <george.bisoc@reactos.org>
 */

/* INCLUDES *******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

KDPC PopIdleScanDevicesDpc;
KTIMER PopIdleScanDevicesTimer;
LIST_ENTRY PopIdleDetectList;
ULONG PopIdleScanIntervalInSeconds = 1;
POWER_STATE_HANDLER PopDefaultPowerStateHandlers[PowerStateMaximum] = {0};

/* PRIVATE FUNCTIONS **********************************************************/

/* PUBLIC FUNCTIONS ***********************************************************/

_Function_class_(KDEFERRED_ROUTINE)
VOID
NTAPI
PopScanForIdleDevicesDpcRoutine(
    _In_ PKDPC Dpc,
    _In_ PVOID DeferredContext,
    _In_ PVOID SystemArgument1,
    _In_ PVOID SystemArgument2)
{
    /* FIXME */
    UNIMPLEMENTED;
    NOTHING;
}

VOID
NTAPI
PopCleanupPowerState(
    _In_ PPOWER_STATE PowerState)
{
    /* FIXME */
    UNIMPLEMENTED;
    NOTHING;
}

/* EOF */
