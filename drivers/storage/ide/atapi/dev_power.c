/*
 * PROJECT:     ReactOS ATA Port Driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Power management
 * COPYRIGHT:   Copyright 2024-2025 Dmitry Borisov <di.sean@protonmail.com>
 */

/* INCLUDES *******************************************************************/

#include "atapi.h"

/* GLOBALS ********************************************************************/

static IO_COMPLETION_ROUTINE AtaPdoPowerUpCompletionRoutine;
static REQUEST_POWER_COMPLETE AtaPdoPowerSetSysStateWithDevStateComplete;

/* FUNCTIONS ******************************************************************/

static
NTSTATUS
AtaPowerCompleteIrp(
    _In_ PATAPORT_COMMON_EXTENSION CommonExt,
    _In_ PIRP Irp,
    _In_ NTSTATUS Status)
{
    PoStartNextPowerIrp(Irp);

    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    IoReleaseRemoveLock(&CommonExt->RemoveLock, Irp);

    return Status;
}

VOID
AtaDeviceFlushPowerIrpQueue(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt)
{
    while (TRUE)
    {
        PLIST_ENTRY Entry;
        PIRP PowerIrp;
        KIRQL OldLevel;

        KeAcquireSpinLock(&DevExt->Device.QueueLock, &OldLevel);
        if (IsListEmpty(&DevExt->PowerIrpQueueList))
            Entry = NULL;
        else
            Entry = RemoveHeadList(&DevExt->PowerIrpQueueList);
        KeReleaseSpinLock(&DevExt->Device.QueueLock, OldLevel);

        if (!Entry)
            break;

        PowerIrp = CONTAINING_RECORD(Entry, IRP, Tail.Overlay.ListEntry);
        ASSERT(IoGetCurrentIrpStackLocation(PowerIrp)->MinorFunction == IRP_MN_SET_POWER);
        AtaPowerCompleteIrp(&DevExt->Common, PowerIrp, STATUS_NO_SUCH_DEVICE);
    }
}

UCHAR
AtaDeviceGetFlushCacheCommand(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt)
{
    UCHAR Command = 0;

    if (!IS_ATAPI(&DevExt->Device))
    {
        if ((DevExt->Device.DeviceFlags & DEVICE_LBA48) &&
            AtaDevHasFlushCacheExt(&DevExt->IdentifyDeviceData))
        {
            Command = IDE_COMMAND_FLUSH_CACHE_EXT;
        }
        else if (AtaDevHasFlushCache(&DevExt->IdentifyDeviceData))
        {
            Command = IDE_COMMAND_FLUSH_CACHE;
        }
    }

    return Command;
}

static
VOID
AtaDeviceCompletePowerIrp(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ PIRP Irp)
{
    POWER_STATE PowerState;

    PowerState.DeviceState = DevExt->Common.DevicePowerState;
    PoSetPowerState(DevExt->Common.Self, DevicePowerState, PowerState);

    AtaPowerCompleteIrp(&DevExt->Common, Irp, STATUS_SUCCESS);
}

static
NTSTATUS
AtaDeviceFlushCache(
    _In_ PATAPORT_PORT_DATA PortData,
    _In_ PATAPORT_DEVICE_EXTENSION DevExt)
{
    PATA_DEVICE_REQUEST Request = &PortData->Worker.InternalRequest;
    UCHAR Command;

    Command = AtaDeviceGetFlushCacheCommand(DevExt);
    if (Command == 0)
        return STATUS_SUCCESS;

    Request->Flags = 0;
    Request->TimeOut = 30;
    RtlZeroMemory(&Request->TaskFile, sizeof(Request->TaskFile));
    Request->TaskFile.Command = Command;

    return AtaPortSendRequest(PortData, DevExt);
}

static
NTSTATUS
AtaDeviceSetIdleMode(
    _In_ PATAPORT_PORT_DATA PortData,
    _In_ PATAPORT_DEVICE_EXTENSION DevExt)
{
    PATA_DEVICE_REQUEST Request = &PortData->Worker.InternalRequest;

    Request->Flags = 0;
    Request->TimeOut = 20;
    Request->TaskFile.Feature = 0;
    Request->TaskFile.Command = IDE_COMMAND_IDLE_IMMEDIATE;

    return AtaPortSendRequest(PortData, DevExt);
}

static
NTSTATUS
AtaDeviceSetStandbyMode(
    _In_ PATAPORT_PORT_DATA PortData,
    _In_ PATAPORT_DEVICE_EXTENSION DevExt)
{
    PATA_DEVICE_REQUEST Request = &PortData->Worker.InternalRequest;

    Request->Flags = 0;
    Request->TimeOut = 20;
    Request->TaskFile.Command = IDE_COMMAND_STANDBY_IMMEDIATE;

    return AtaPortSendRequest(PortData, DevExt);
}

NTSTATUS
AtaPortDeviceProcessPowerChange(
    _In_ PATAPORT_PORT_DATA PortData,
    _In_ PATAPORT_DEVICE_EXTENSION DevExt)
{
    PLIST_ENTRY Entry;
    PIRP PowerIrp;
    PIO_STACK_LOCATION IoStack;
    DEVICE_POWER_STATE NewState;
    KIRQL OldLevel;
    NTSTATUS Status;

    KeAcquireSpinLock(&DevExt->Device.QueueLock, &OldLevel);
    if (IsListEmpty(&DevExt->PowerIrpQueueList))
        Entry = NULL;
    else
        Entry = RemoveHeadList(&DevExt->PowerIrpQueueList);
    KeReleaseSpinLock(&DevExt->Device.QueueLock, OldLevel);
    if (!Entry)
        return STATUS_SUCCESS;

    PowerIrp = CONTAINING_RECORD(Entry, IRP, Tail.Overlay.ListEntry);

    IoStack = IoGetCurrentIrpStackLocation(PowerIrp);
    ASSERT(IoStack->MinorFunction == IRP_MN_SET_POWER);

    NewState = IoStack->Parameters.Power.State.DeviceState;
    if (NewState == DevExt->Common.DevicePowerState)
    {
        AtaDeviceCompletePowerIrp(DevExt, PowerIrp);
        return STATUS_SUCCESS;
    }

    if (NewState != PowerDeviceD3)
    {
        AtaReqThawQueue(DevExt, QUEUE_FLAG_FROZEN_POWER);

        /* Power up the device */
        Status = AtaDeviceSetIdleMode(PortData, DevExt);
        if (Status == STATUS_ADAPTER_HARDWARE_ERROR)
            return Status;

        /* Restore the software settings after power up */
        _InterlockedOr(&DevExt->Worker.EventsPending, ACTION_DEVICE_CONFIG);
        _InterlockedOr(&PortData->Worker.EventsPending, ACTION_DEVICE_CONFIG);
    }
    else
    {
        AtaReqFreezeQueue(DevExt, QUEUE_FLAG_FROZEN_POWER);

        /* Flush device caches */
        Status = AtaDeviceFlushCache(PortData, DevExt);
        if (Status == STATUS_ADAPTER_HARDWARE_ERROR)
            return Status;

        /* Power down the device */
        Status = AtaDeviceSetStandbyMode(PortData, DevExt);
        if (Status == STATUS_ADAPTER_HARDWARE_ERROR)
            return Status;
    }

    AtaDeviceCompletePowerIrp(DevExt, PowerIrp);

    DevExt->Common.DevicePowerState = NewState;
    return Status;
}

NTSTATUS
AtaPortCheckDevicePowerState(
    _In_ PATAPORT_PORT_DATA PortData,
    _In_ PATAPORT_DEVICE_EXTENSION DevExt)
{
    NTSTATUS Status;

    if (DevExt->Common.DevicePowerState == PowerDeviceD0)
        return STATUS_SUCCESS;

    INFO("CH %lu: Powering up idle device '%s'\n", PortData->PortNumber, DevExt->FriendlyName);

    Status = AtaDeviceSetIdleMode(PortData, DevExt);
    if (!NT_SUCCESS(Status))
    {
        /*
         * If the power on device has failed, the next command can still work.
         * The media access will result in a transition from the PM1:Idle/PM2:Standby state
         * to the PM0:Active state by the ATA spec.
         */
        ERR("CH %lu: Failed to power up device '%s' %lx\n",
            PortData->PortNumber, DevExt->FriendlyName, Status);
    }
    else
    {
        DevExt->Common.DevicePowerState = PowerDeviceD0;
    }

    return Status;
}

static
VOID
AtaPdoQueuePowerIrp(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ PIRP Irp)
{
    KIRQL OldLevel;

    KeAcquireSpinLock(&DevExt->Device.QueueLock, &OldLevel);
    InsertTailList(&DevExt->PowerIrpQueueList, &Irp->Tail.Overlay.ListEntry);
    KeReleaseSpinLock(&DevExt->Device.QueueLock, OldLevel);

    AtaDeviceQueueEvent(DevExt->Device.PortData, DevExt, ACTION_DEVICE_POWER);
}

static
NTSTATUS
NTAPI
AtaPdoPowerUpCompletionRoutine(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIRP Irp,
    _In_reads_opt_(_Inexpressible_("varies")) PVOID Context)
{
    PATAPORT_DEVICE_EXTENSION DevExt = Context;
    NTSTATUS Status;

    UNREFERENCED_PARAMETER(DeviceObject);

    Status = Irp->IoStatus.Status;

    INFO("Powering up device, lower driver status %lx\n", Status);

    if (!NT_SUCCESS(Status))
    {
        PoStartNextPowerIrp(Irp);

        IoReleaseRemoveLock(&DevExt->Common.RemoveLock, Irp);
        return STATUS_CONTINUE_COMPLETION;
    }

    AtaPdoQueuePowerIrp(DevExt, Irp);

    /* Defer IRP completion */
    return STATUS_MORE_PROCESSING_REQUIRED;
}

static
NTSTATUS
AtaPdoPowerSetDevicePowerState(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    DEVICE_POWER_STATE NewState;

    IoStack = IoGetCurrentIrpStackLocation(Irp);
    NewState = IoStack->Parameters.Power.State.DeviceState;

    IoMarkIrpPending(Irp);

    if (NewState < DevExt->Common.DevicePowerState)
    {
        PATAPORT_CHANNEL_EXTENSION ChanExt = DevExt->Common.FdoExt;

        IoCopyCurrentIrpStackLocationToNext(Irp);
        IoSetCompletionRoutine(Irp,
                               AtaPdoPowerUpCompletionRoutine,
                               DevExt,
                               TRUE,
                               TRUE,
                               TRUE);
        (VOID)PoCallDriver(ChanExt->Common.Self, Irp);
    }
    else
    {
        INFO("Powering down device\n");

        AtaPdoQueuePowerIrp(DevExt, Irp);
    }

    return STATUS_PENDING;
}

static
VOID
NTAPI
AtaPdoPowerSetSysStateWithDevStateComplete(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ UCHAR MinorFunction,
    _In_ POWER_STATE PowerState,
    _In_opt_ PVOID Context,
    _In_ PIO_STATUS_BLOCK IoStatus)
{
    PATAPORT_DEVICE_EXTENSION DevExt = DeviceObject->DeviceExtension;
    PIRP Irp = Context;

    UNREFERENCED_PARAMETER(MinorFunction);

    if (DevExt->Common.SystemPowerState != PowerState.SystemState)
    {
        DevExt->Common.SystemPowerState = PowerState.SystemState;
        PoSetPowerState(DeviceObject, SystemPowerState, PowerState);
    }

    AtaPowerCompleteIrp(&DevExt->Common, Irp, IoStatus->Status);
}

static
VOID
AtaPdoPowerGetDevStateForSysState(
    _In_ SYSTEM_POWER_STATE SystemState,
    _Out_ PDEVICE_POWER_STATE DeviceState)
{
    if (SystemState == PowerSystemWorking)
    {
        *DeviceState = PowerDeviceD0;
    }
    else
    {
        *DeviceState = PowerDeviceD3;
    }
}

static
NTSTATUS
AtaPdoPowerSetSystemPowerState(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    NTSTATUS Status;
    POWER_STATE NewPowerState;
    SYSTEM_POWER_STATE NewSystemState;

    IoStack = IoGetCurrentIrpStackLocation(Irp);
    NewSystemState = IoStack->Parameters.Power.State.SystemState;

    IoMarkIrpPending(Irp);

    AtaPdoPowerGetDevStateForSysState(NewSystemState, &NewPowerState.DeviceState);
    Status = PoRequestPowerIrp(DevExt->Common.Self,
                               IRP_MN_SET_POWER,
                               NewPowerState,
                               AtaPdoPowerSetSysStateWithDevStateComplete,
                               Irp,
                               NULL);
    if (!NT_SUCCESS(Status))
    {
        IO_STATUS_BLOCK IoStatusBlock;

        ERR("Failed to change power state for device '%s' %lx\n", DevExt->FriendlyName, Status);

        IoStatusBlock.Status = Status;
        AtaPdoPowerSetSysStateWithDevStateComplete(DevExt->Common.Self,
                                                    IRP_MN_SET_POWER,
                                                    NewPowerState,
                                                    Irp,
                                                    &IoStatusBlock);
    }

    return Status;
}

static
NTSTATUS
AtaPdoPower(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _Inout_ PIRP Irp)
{
    // TODO: Investigate how to fix this
    // *** Assertion failed: KeGetCurrentThread()->SystemAffinityActive == FALSE
    // ***   Source File: ../ntoskrnl/ke/dpc.c, line 926
#if 1
    NTSTATUS Status;

    UNREFERENCED_PARAMETER(AtaPdoPowerSetSystemPowerState);
    UNREFERENCED_PARAMETER(AtaPdoPowerGetDevStateForSysState);
    UNREFERENCED_PARAMETER(AtaPdoPowerSetSysStateWithDevStateComplete);
    UNREFERENCED_PARAMETER(AtaPdoPowerSetDevicePowerState);
    UNREFERENCED_PARAMETER(AtaPdoPowerUpCompletionRoutine);
    UNREFERENCED_PARAMETER(AtaPdoQueuePowerIrp);

    Status = Irp->IoStatus.Status;
    PoStartNextPowerIrp(Irp);
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return Status;
#else
    NTSTATUS Status;
    PIO_STACK_LOCATION IoStack;

    Status = IoAcquireRemoveLock(&DevExt->Common.RemoveLock, Irp);
    if (!NT_SUCCESS(Status))
    {
        PoStartNextPowerIrp(Irp);

        Irp->IoStatus.Status = Status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return Status;
    }

    IoStack = IoGetCurrentIrpStackLocation(Irp);
    switch (IoStack->MinorFunction)
    {
        case IRP_MN_QUERY_POWER:
        {
            switch (IoStack->Parameters.Power.Type)
            {
                case SystemPowerState:
                case DevicePowerState:
                    Status = AtaPowerCompleteIrp(&DevExt->Common, Irp, STATUS_SUCCESS);
                    break;

                default:
                    Status = AtaPowerCompleteIrp(&DevExt->Common, Irp, Irp->IoStatus.Status);
                    break;
            }
            break;
        }

        case IRP_MN_SET_POWER:
        {
            switch (IoStack->Parameters.Power.Type)
            {
                case SystemPowerState:
                    Status = AtaPdoPowerSetSystemPowerState(DevExt, Irp);
                    break;
                case DevicePowerState:
                    Status = AtaPdoPowerSetDevicePowerState(DevExt, Irp);
                    break;

                default:
                    Status = AtaPowerCompleteIrp(&DevExt->Common, Irp, Irp->IoStatus.Status);
                    break;
            }
            break;
        }

        default:
            Status = AtaPowerCompleteIrp(&DevExt->Common, Irp, Irp->IoStatus.Status);
            break;
    }

    return Status;
#endif
}

static
NTSTATUS
AtaFdoPower(
    _In_ PATAPORT_CHANNEL_EXTENSION ChanExt,
    _Inout_ PIRP Irp)
{
    NTSTATUS Status;

    Status = IoAcquireRemoveLock(&ChanExt->Common.RemoveLock, Irp);
    if (!NT_SUCCESS(Status))
    {
        PoStartNextPowerIrp(Irp);

        Irp->IoStatus.Status = Status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return Status;
    }

    PoStartNextPowerIrp(Irp);
    IoSkipCurrentIrpStackLocation(Irp);
    Status = PoCallDriver(ChanExt->Common.LowerDeviceObject, Irp);

    IoReleaseRemoveLock(&ChanExt->Common.RemoveLock, Irp);

    return Status;
}

NTSTATUS
NTAPI
AtaDispatchPower(
    _In_ PDEVICE_OBJECT DeviceObject,
    _Inout_ PIRP Irp)
{
    if (IS_FDO(DeviceObject->DeviceExtension))
        return AtaFdoPower(DeviceObject->DeviceExtension, Irp);
    else
        return AtaPdoPower(DeviceObject->DeviceExtension, Irp);
}
