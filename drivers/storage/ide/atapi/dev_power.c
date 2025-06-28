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

        KeAcquireSpinLockAtDpcLevel(&DevExt->Device.QueueLock);
        if (IsListEmpty(&DevExt->PowerIrpQueueList))
            Entry = NULL;
        else
            Entry = RemoveHeadList(&DevExt->PowerIrpQueueList);
        KeReleaseSpinLockFromDpcLevel(&DevExt->Device.QueueLock);

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
        if (AtaDevHasFlushCache(&DevExt->IdentifyDeviceData))
            Command = IDE_COMMAND_FLUSH_CACHE;

        if ((DevExt->Device.DeviceFlags & DEVICE_LBA48) &&
            AtaDevHasFlushCacheExt(&DevExt->IdentifyDeviceData))
        {
            Command = IDE_COMMAND_FLUSH_CACHE_EXT;
        }
    }

    return Command;
}

static
VOID
AtaDeviceHandleCompletePowerIrp(
    _In_ PATA_WORKER_CONTEXT Context)
{
    PATAPORT_DEVICE_EXTENSION DevExt = Context->DevExt;
    POWER_STATE PowerState;

    PowerState.DeviceState = DevExt->Common.DevicePowerState;
    PoSetPowerState(DevExt->Common.Self, DevicePowerState, PowerState);

    AtaPowerCompleteIrp(&DevExt->Common, Context->CurrentIrp, STATUS_SUCCESS);

    AtaFsmSetState(Context, PORT_STATE_PULL_EVENT);
}

static
VOID
AtaDeviceHandleSetIdleMode(
    _In_ PATA_WORKER_CONTEXT Context)
{
    PATA_DEVICE_REQUEST Request = Context->InternalRequest;

    Request->Flags = 0;
    Request->TimeOut = 20;
    Request->TaskFile.Feature = 0;
    Request->TaskFile.Command = IDE_COMMAND_IDLE_IMMEDIATE;
    AtaFsmIssueCommand(Context);
    AtaFsmSetLocalState(Context, PWR_STATE_COMPLETE_IRP);
}

static
VOID
AtaDeviceHandleSetStandbyMode(
    _In_ PATA_WORKER_CONTEXT Context)
{
    PATA_DEVICE_REQUEST Request = Context->InternalRequest;

    Request->Flags = 0;
    Request->TimeOut = 20;
    Request->TaskFile.Command = IDE_COMMAND_STANDBY_IMMEDIATE;
    AtaFsmIssueCommand(Context);
    AtaFsmSetLocalState(Context, PWR_STATE_COMPLETE_IRP);
}

static
VOID
AtaDeviceEnterPower(
    _In_ PATA_WORKER_CONTEXT Context)
{
    PATAPORT_DEVICE_EXTENSION DevExt = Context->DevExt;
    PLIST_ENTRY Entry = NULL;
    PIRP PowerIrp;
    PIO_STACK_LOCATION IoStack;
    DEVICE_POWER_STATE NewState;

    KeAcquireSpinLockAtDpcLevel(&DevExt->Device.QueueLock);
    if (IsListEmpty(&DevExt->PowerIrpQueueList))
        AtaFsmCompleteDevicePowerEvent(Context, DevExt);
    else
        Entry = RemoveHeadList(&DevExt->PowerIrpQueueList);
    KeReleaseSpinLockFromDpcLevel(&DevExt->Device.QueueLock);

    if (!Entry)
        return;

    PowerIrp = CONTAINING_RECORD(Entry, IRP, Tail.Overlay.ListEntry);
    Context->CurrentIrp = PowerIrp;

    IoStack = IoGetCurrentIrpStackLocation(PowerIrp);
    ASSERT(IoStack->MinorFunction == IRP_MN_SET_POWER);

    NewState = IoStack->Parameters.Power.State.DeviceState;
    if (NewState == DevExt->Common.DevicePowerState)
    {
        AtaFsmSetLocalState(Context, PWR_STATE_COMPLETE_IRP);
        return;
    }

    if (NewState != PowerDeviceD3)
    {
        /* Power up the device */
        AtaReqThawQueue(DevExt, QUEUE_FLAG_FROZEN_POWER);
        AtaFsmSetLocalState(Context, PWR_STATE_SET_IDLE);

        /* Restore the software settings after power up */
        _InterlockedOr(&DevExt->Worker.EventsPending, ACTION_DEVICE_CONFIG);
        _InterlockedOr(&Context->EventsPending, ACTION_DEVICE_CONFIG);
    }
    else
    {
        UCHAR Command;

        AtaReqFreezeQueue(DevExt, QUEUE_FLAG_FROZEN_POWER);

        /* Flush device caches */
        Command = AtaDeviceGetFlushCacheCommand(DevExt);
        if (Command != 0)
        {
            PATA_DEVICE_REQUEST Request = Context->InternalRequest;

            Request->Flags = 0;
            Request->TimeOut = 30;
            RtlZeroMemory(&Request->TaskFile, sizeof(Request->TaskFile));
            Request->TaskFile.Command = Command;
            AtaFsmIssueCommand(Context);
        }

        /* Power down the device */
        AtaFsmSetLocalState(Context, PWR_STATE_SET_STANDBY);
    }

    DevExt->Common.DevicePowerState = NewState;
}

VOID
AtaPowerRunStateMachine(
    _In_ PATA_WORKER_CONTEXT Context)
{
    switch (Context->LocalState)
    {
        case PWR_STATE_ENTER:
            AtaDeviceEnterPower(Context);
            break;
        case PWR_STATE_SET_STANDBY:
            AtaDeviceHandleSetStandbyMode(Context);
            break;
        case PWR_STATE_SET_IDLE:
            AtaDeviceHandleSetIdleMode(Context);
            break;
        case PWR_STATE_COMPLETE_IRP:
            AtaDeviceHandleCompletePowerIrp(Context);
            break;

        default:
            ASSERT(FALSE);
            UNREACHABLE;
    }
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
        IoCopyCurrentIrpStackLocationToNext(Irp);
        IoSetCompletionRoutine(Irp,
                               AtaPdoPowerUpCompletionRoutine,
                               DevExt,
                               TRUE,
                               TRUE,
                               TRUE);
        (VOID)PoCallDriver(DevExt->Device.ChanExt->Common.Self, Irp);
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
#if 0
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
    Status = PoCallDriver(ChanExt->Ldo, Irp);

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
