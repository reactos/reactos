/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Power Manager public API routines
 * COPYRIGHT:   Copyright 2023 George Bișoc <george.bisoc@reactos.org>
 */

/* INCLUDES *******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* PUBLIC FUNCTIONS ***********************************************************/

NTSTATUS
NTAPI
PoCallDriver(
    _In_ PDEVICE_OBJECT DeviceObject,
    _Inout_ __drv_aliasesMem PIRP Irp)
{
    PIO_STACK_LOCATION IrpStack;

    /* We must not be above the permitted IRQL */
    ASSERT_IRQL_LESS_OR_EQUAL(DISPATCH_LEVEL);

    /* The passed IRP must be a Power IRP */
    IrpStack = IoGetNextIrpStackLocation(Irp);
    POP_ASSERT_IRP_IS_POWER(IrpStack);

    /* Forward that IRP to the device */
    return IoCallDriver(DeviceObject, Irp);
}

NTSTATUS
NTAPI
PoRequestPowerIrp(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ UCHAR MinorFunction,
    _In_ POWER_STATE PowerState,
    _In_opt_ PREQUEST_POWER_COMPLETE CompletionFunction,
    _In_opt_ __drv_aliasesMem PVOID Context,
    _Outptr_opt_ PIRP *Irp)
{
    /* Invoke the private helper to do the deed */
    return PopRequestPowerIrp(DeviceObject,
                              MinorFunction,
                              PowerState,
                              FALSE,
                              CompletionFunction,
                              Context,
                              Irp);
}

VOID
NTAPI
PoStartNextPowerIrp(
    _Inout_ PIRP Irp)
{
    UNREFERENCED_PARAMETER(Irp);
    NOTHING;
}

PVOID
NTAPI
PoRegisterSystemState(
    _Inout_opt_ PVOID StateHandle,
    _In_ EXECUTION_STATE Flags)
{
    /* FIXME */
    UNIMPLEMENTED;
    return NULL;
}

VOID
NTAPI
PoUnregisterSystemState(
    _Inout_ PVOID StateHandle)
{
    /* FIXME */
    UNIMPLEMENTED;
    NOTHING;
}

VOID
NTAPI
PoSetSystemState(
    _In_ EXECUTION_STATE Flags)
{
    /* FIXME */
    UNIMPLEMENTED;
    NOTHING;
}

PULONG
NTAPI
PoRegisterDeviceForIdleDetection(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ ULONG ConservationIdleTime,
    _In_ ULONG PerformanceIdleTime,
    _In_ DEVICE_POWER_STATE State)
{
    /* FIXME */
    UNIMPLEMENTED;
    return NULL;
}

POWER_STATE
NTAPI
PoSetPowerState(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ POWER_STATE_TYPE Type,
    _In_ POWER_STATE State)
{
    /* FIXME */
    POWER_STATE ps;
    ps.DeviceState = PowerDeviceD0;
    UNIMPLEMENTED;
    return ps;
}

NTSTATUS
NTAPI
PoRegisterPowerSettingCallback(
    _In_opt_ PDEVICE_OBJECT DeviceObject,
    _In_ LPCGUID SettingGuid,
    _In_ PPOWER_SETTING_CALLBACK Callback,
    _In_opt_ PVOID Context,
    _Outptr_opt_ PVOID *Handle)
{
    /* FIXME */
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
PoUnregisterPowerSettingCallback(
    _Inout_ PVOID Handle)
{
    /* FIXME */
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

VOID
NTAPI
PoSetSystemWake(
    _Inout_ PIRP Irp)
{
    PIO_STACK_LOCATION IrpSp;

    /* We must not be above the permitted IRQL */
    ASSERT_IRQL_LESS_OR_EQUAL(DISPATCH_LEVEL);

    /* This has to be a wake power IRP, otherwise the IRP we got is bogus */
    IrpSp = IoGetCurrentIrpStackLocation(Irp);
    POP_ASSERT_IRP_IS_WAKE(IrpSp);

    /*
     * The IRP of the device that contributed to the waking of the system is
     * this one, so set it as "woke IRP". Setting the woke bit context within
     * the current stack location makes sense, because a wake IRP woke up the
     * system at the moment it was during IRP completion, of which there are no
     * further stack locations in the driver stack space. With that said, the IRP
     * alaready walked most of the device driver stack.
     */
    IrpSp->Parameters.Power.SystemContext = POP_SYS_CONTEXT_WOKE_SYSTEM;
}

BOOLEAN
NTAPI
PoGetSystemWake(
    _In_ PIRP Irp)
{
    PIO_STACK_LOCATION IrpSp;

    /* We must not be above the permitted IRQL */
    ASSERT_IRQL_LESS_OR_EQUAL(DISPATCH_LEVEL);

    /* This has to be a wake power IRP, otherwise the IRP we got is bogus */
    IrpSp = IoGetCurrentIrpStackLocation(Irp);
    POP_ASSERT_IRP_IS_WAKE(IrpSp);

    /* Does this IRP contributed to the waking of the system? */
    if (IrpSp->Parameters.Power.SystemContext == POP_SYS_CONTEXT_WOKE_SYSTEM)
    {
        /* That's the guy */
        return TRUE;
    }

    return FALSE;
}

VOID
NTAPI
PoSetDeviceBusyEx(
    _Inout_ PULONG IdlePointer)
{
    /* FIXME */
    UNIMPLEMENTED;
    NOTHING;
}

VOID
NTAPI
PoStartDeviceBusy(
    _Inout_ PULONG IdlePointer)
{
    /* FIXME */
    UNIMPLEMENTED;
    NOTHING;
}

VOID
NTAPI
PoEndDeviceBusy(
    _Inout_ PULONG IdlePointer)
{
    /* FIXME */
    UNIMPLEMENTED;
    NOTHING;
}

BOOLEAN
NTAPI
PoQueryWatchdogTime(
    _In_ PDEVICE_OBJECT Pdo,
    _Out_ PULONG SecondsRemaining)
{
    /* FIXME */
    UNIMPLEMENTED;
    return FALSE;
}

NTSTATUS
NTAPI
PoSetPowerRequest(
    _Inout_ PVOID PowerRequest,
    _In_ POWER_REQUEST_TYPE Type)
{
    /* FIXME */
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

VOID
NTAPI
PoDeletePowerRequest(
    _Inout_ PVOID PowerRequest)
{
    /* FIXME */
    UNIMPLEMENTED;
    NOTHING;
}

NTSTATUS
NTAPI
PoCreatePowerRequest(
    _Outptr_ PVOID *PowerRequest,
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_opt_ PCOUNTED_REASON_CONTEXT Context)
{
    /* FIXME */
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
PoClearPowerRequest(
    _Inout_ PVOID PowerRequest,
    _In_ POWER_REQUEST_TYPE Type)
{
    /* FIXME */
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTKERNELAPI
NTSTATUS
PoCreateThermalRequest(
    _Outptr_ PVOID *ThermalRequest,
    _In_ PDEVICE_OBJECT TargetDeviceObject,
    _In_ PDEVICE_OBJECT PolicyDeviceObject,
    _In_ PCOUNTED_REASON_CONTEXT Context,
    _In_ ULONG Flags)
{
    /* FIXME */
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
VOID
PoDeleteThermalRequest(
    _Inout_ PVOID ThermalRequest)
{
    /* FIXME */
    UNIMPLEMENTED;
    NOTHING;
}

/* EOF */
