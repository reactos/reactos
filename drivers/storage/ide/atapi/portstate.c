/*
 * PROJECT:     ReactOS ATA Port Driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Port state machine core logic
 * COPYRIGHT:   Copyright 2024-2025 Dmitry Borisov <di.sean@protonmail.com>
 */

/* INCLUDES *******************************************************************/

#include "atapi.h"

/* FUNCTIONS ******************************************************************/

static
VOID
AtaPortQueueEvent(
    _In_ PATAPORT_PORT_DATA PortData,
    _In_opt_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_opt_ PATA_DEVICE_REQUEST FailedRequest,
    _In_ ATA_PORT_ACTION Action)
{
    ASSERT(KeGetCurrentIrql() > DISPATCH_LEVEL);

    if (Action & (ACTION_PORT_RESET | ACTION_DEVICE_ERROR))
    {
        if (Action & ACTION_DEVICE_ERROR)
        {
            if (FailedRequest)
            {
                ASSERT_REQUEST(FailedRequest);

                DevExt = CONTAINING_RECORD(FailedRequest->Device, ATAPORT_DEVICE_EXTENSION, Device);

                if (FailedRequest->Flags & REQUEST_FLAG_INTERNAL)
                {
                    Action &= ~ACTION_DEVICE_ERROR;
                }
                else
                {
                    ASSERT(PortData->Worker.FailedRequest == NULL);
                    PortData->Worker.FailedRequest = FailedRequest;
                }
            }
            else
            {
                Action &= ~ACTION_DEVICE_ERROR;
                Action |= ACTION_PORT_RESET;
            }
        }

        /* Special case for the internal request */
        if ((PortData->ActiveSlotsBitmap & 1) &&
            (PortData->Slots[0]->Flags & REQUEST_FLAG_INTERNAL))
        {
            PATA_DEVICE_REQUEST InternalRequest = &PortData->Worker.InternalRequest;

            if (Action & ACTION_PORT_RESET)
                InternalRequest->SrbStatus = SRB_STATUS_BUS_RESET;

            PortData->ActiveSlotsBitmap &= ~1;

            /* Internal request failed, kick off the port thread */
            AtaReqStartCompletionDpc(InternalRequest);
        }

        /* Error recovery, save any commands pending on this port */
        PortData->Worker.PausedSlotsBitmap |= PortData->ActiveSlotsBitmap;
        PortData->ActiveSlotsBitmap = 0;

        PortData->AbortChannel(PortData->ChannelContext, !!(Action & ACTION_PORT_RESET));
    }

    if (DevExt)
        _InterlockedOr(&DevExt->Worker.EventsPending, Action);
    _InterlockedOr(&PortData->Worker.EventsPending, Action);

    /* Kick off the port thread */
    if (PortData->InterruptFlags & PORT_INT_FLAG_IS_IO_ACTIVE)
    {
        _InterlockedAnd(&PortData->InterruptFlags, ~PORT_INT_FLAG_IS_IO_ACTIVE);
        KeInsertQueueDpc(&PortData->Worker.Dpc, NULL, NULL);
    }
}

static
ATA_PORT_ACTION
AtaPortClearPortAction(
    _In_ PATA_WORKER_CONTEXT Context,
    _In_ ATA_PORT_ACTION Action)
{
    return _InterlockedAnd(&Context->EventsPending, ~(ULONG)Action);
}

static
BOOLEAN
AtaPortClearDeviceAction(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ ATA_PORT_ACTION Action)
{
    return !!(_InterlockedAnd(&DevExt->Worker.EventsPending, ~(ULONG)Action) & Action);
}

static
PATAPORT_DEVICE_EXTENSION
AtaPortFindDeviceForAction(
    _In_ PATAPORT_PORT_DATA PortData,
    _In_ ATA_PORT_ACTION Action)
{
    PATAPORT_DEVICE_EXTENSION DevExt, Result = NULL;
    PATAPORT_CHANNEL_EXTENSION ChanExt;
    PSINGLE_LIST_ENTRY Entry;
    KIRQL OldIrql, OldLevel;

    ChanExt = CONTAINING_RECORD(PortData, ATAPORT_CHANNEL_EXTENSION, PortData);

    KeAcquireSpinLock(&ChanExt->PdoListLock, &OldLevel);
    OldIrql = KeAcquireInterruptSpinLock(PortData->InterruptObject);

    for (Entry = ChanExt->PdoList.Next; Entry != NULL; Entry = Entry->Next)
    {
        DevExt = CONTAINING_RECORD(Entry, ATAPORT_DEVICE_EXTENSION, ListEntry);

        if (DevExt->Device.AtaScsiAddress.PathId != PortData->PortNumber)
            continue;

        if (DevExt->ReportedMissing)
            continue;

        if (DevExt->Worker.Flags & DEV_WORKER_FLAG_REMOVED)
            continue;

        if (DevExt->Worker.EventsPending & Action)
        {
            Result = DevExt;
            break;
        }
    }

    if (!Result)
        AtaPortClearPortAction(&PortData->Worker, Action);

    KeReleaseInterruptSpinLock(PortData->InterruptObject, OldIrql);
    KeReleaseSpinLock(&ChanExt->PdoListLock, OldLevel);

    return Result;
}

static
VOID
AtaPortOnAsyncNotification(
    _In_ PATAPORT_PORT_DATA PortData,
    _In_ ULONG DeviceBitmap)
{
    ASSERT(KeGetCurrentIrql() > DISPATCH_LEVEL);

    KeInsertQueueDpc(&PortData->Worker.NotificationDpc, UlongToPtr(DeviceBitmap), NULL);
}

static
VOID
AtaPortOnResetNotification(
    _In_ PATAPORT_PORT_DATA PortData,
    _In_ ULONG DeviceBitmap)
{
    PATAPORT_CHANNEL_EXTENSION ChanExt;
    ATA_SCSI_ADDRESS AtaScsiAddress;
    PATAPORT_DEVICE_EXTENSION DevExt;

    /*
     * We are about to reset the channel which will in turn
     * cause the affected devices to lose their software settings.
     * Enqueue a config event so the state machine can re-initialize all devices later on.
     */
    _InterlockedOr(&PortData->Worker.EventsPending,
                   ACTION_ENUM_DEVICE |
                   ACTION_DEVICE_CONFIG |
                   ACTION_PORT_TIMING);

    ChanExt = CONTAINING_RECORD(PortData, ATAPORT_CHANNEL_EXTENSION, PortData);

    AtaScsiAddress.AsULONG = 0;
    while (TRUE)
    {
        ATA_PORT_ACTION Event;

        DevExt = AtaFdoFindNextDeviceByPath(ChanExt, &AtaScsiAddress, FALSE, NULL);
        if (!DevExt)
            break;

        if (!(DeviceBitmap & (1 << (ULONG)AtaScsiAddress.TargetId)))
            continue;

        if (DevExt->Device.DeviceFlags & DEVICE_PNP_STARTED)
            Event = ACTION_ENUM_DEVICE | ACTION_DEVICE_CONFIG;
        else
            Event = ACTION_ENUM_DEVICE; // ACPI _GTF is not ready yet

        _InterlockedOr(&DevExt->Worker.EventsPending, Event);
    }
}

static
VOID
AtaPortOnRequestComplete(
    _In_ PATAPORT_PORT_DATA PortData,
    _In_ ULONG CommandsCompleted)
{
    ULONG Slot;

    ASSERT(KeGetCurrentIrql() > DISPATCH_LEVEL);

    PortData->ActiveSlotsBitmap &= ~CommandsCompleted;

    while (_BitScanForward(&Slot, CommandsCompleted) != 0)
    {
        PATA_DEVICE_REQUEST Request;

        CommandsCompleted &= ~(1 << Slot);

        Request = PortData->Slots[Slot];
        ASSERT_REQUEST(Request);
        ASSERT(Request->Slot == Slot);

        InterlockedPushEntrySList(&AtapCompletionQueueList, &Request->CompletionEntry);
    }

    KeInsertQueueDpc(&AtapCompletionDpc, NULL, NULL);
}

static
VOID
AtaPortMarkDeviceFailed(
    _In_ PATAPORT_PORT_DATA PortData,
    _In_ PATAPORT_DEVICE_EXTENSION DevExt)
{
    /* Ignore bad or incompatible devices, so we do not access them at all */
    if (++DevExt->Worker.ResetRetryCount >= 4)
    {
        ERR("CH %lu: Too many reset attempts for the device %u, giving up\n",
            PortData->PortNumber,
            DevExt->Device.AtaScsiAddress.TargetId);

        DevExt->Worker.Flags |= DEV_WORKER_FLAG_REMOVED;

        PortData->Worker.BadDeviceBitmap |= 1 << (ULONG)DevExt->Device.AtaScsiAddress.TargetId;
    }
}

static
NTSTATUS
AtaPortEnumeratePort(
    _In_ PATAPORT_PORT_DATA PortData)
{
    PATAPORT_CHANNEL_EXTENSION ChanExt;
    ATA_SCSI_ADDRESS AtaScsiAddress;

    _InterlockedOr(&PortData->InterruptFlags, PORT_INT_FLAG_IGNORE_LINK_IRQ);
    PortData->Worker.DeviceCount = PortData->EnumerateChannel(PortData->ChannelContext);
    _InterlockedAnd(&PortData->InterruptFlags, ~PORT_INT_FLAG_IGNORE_LINK_IRQ);

    INFO("CH %lu: Detected %lu devices\n", PortData->PortNumber, PortData->Worker.DeviceCount);

    if (AtaPortClearPortAction(&PortData->Worker, ACTION_ENUM_PORT))
    {
        /* Defer the completion */
        PortData->Worker.Flags |= WORKER_FLAG_COMPLETE_PORT_ENUM_EVENT;
    }

    ChanExt = CONTAINING_RECORD(PortData, ATAPORT_CHANNEL_EXTENSION, PortData);

    /* Remove detached devices */
    AtaScsiAddress.AsULONG = 0;
    while (TRUE)
    {
        PATAPORT_DEVICE_EXTENSION DevExt;

        DevExt = AtaFdoFindNextDeviceByPath(ChanExt, &AtaScsiAddress, FALSE, NULL);
        if (!DevExt)
            break;

        if (AtaScsiAddress.TargetId >= PortData->Worker.DeviceCount)
            DevExt->Worker.Flags |= DEV_WORKER_FLAG_REMOVED;
    }

    return STATUS_SUCCESS;
}

static
NTSTATUS
AtaPortResetPort(
    _In_ PATAPORT_PORT_DATA PortData)
{
    _InterlockedOr(&PortData->InterruptFlags, PORT_INT_FLAG_IGNORE_LINK_IRQ);
    PortData->ResetChannel(PortData->ChannelContext);
    _InterlockedAnd(&PortData->InterruptFlags, ~PORT_INT_FLAG_IGNORE_LINK_IRQ);

    AtaPortEnumeratePort(PortData);
    AtaPortOnResetNotification(PortData, MAXULONG);

    AtaPortClearPortAction(&PortData->Worker, ACTION_PORT_RESET);

    /*
     * Reset bus timings, as attached devices
     * may reset their current transfer mode to default during the processing of a software reset
     * and the subsequent IDENTIFY DEVICE command would end up using incorrect timings.
     */
    AtaPortSelectTimings(PortData, TRUE);

    if (++PortData->Worker.ResetRetryCount >= 10)
    {
        ERR("CH %lu: Too many port reset attempts, giving up\n", PortData->PortNumber);
        PortData->Worker.BadDeviceBitmap = MAXULONG;
    }

    return STATUS_SUCCESS;
}

static
NTSTATUS
AtaPortEnumerateDevice(
    _In_ PATAPORT_PORT_DATA PortData,
    _In_ ATA_PORT_ACTION Action)
{
    PATAPORT_DEVICE_EXTENSION DevExt;

    if (Action == ACTION_ENUM_DEVICE_NEW)
    {
        DevExt = PortData->Worker.EnumDevExt;
        ASSERT(DevExt);
        AtaReqFreezeQueue(DevExt, QUEUE_FLAG_FROZEN_PORT_BUSY);
    }
    else
    {
        DevExt = AtaPortFindDeviceForAction(PortData, Action);
        if (!DevExt)
            return STATUS_SUCCESS;
    }

    if (PortData->Worker.BadDeviceBitmap & (1 << (ULONG)DevExt->Device.AtaScsiAddress.TargetId))
    {
        DevExt->Worker.EnumStatus = DEV_STATUS_NO_DEVICE;
    }
    else
    {
        DevExt->Worker.EnumStatus = AtaPortIdentifyDevice(PortData, DevExt);
        if (DevExt->Worker.EnumStatus == DEV_STATUS_FAILED)
        {
            AtaPortMarkDeviceFailed(PortData, DevExt);
            return STATUS_ADAPTER_HARDWARE_ERROR;
        }
    }

    if ((DevExt->Worker.EnumStatus != DEV_STATUS_SAME_DEVICE) &&
        !(DevExt->Device.DeviceFlags & DEVICE_UNINITIALIZED))
    {
        DevExt->Worker.Flags |= DEV_WORKER_FLAG_REMOVED;
    }

    if (AtaPortClearDeviceAction(DevExt, Action))
    {
        if (Action == ACTION_ENUM_DEVICE_NEW)
            AtaPortClearPortAction(&PortData->Worker, Action);

        KeSetEvent(&DevExt->Worker.EnumerationEvent, 0, FALSE);
    }

    return STATUS_SUCCESS;
}

static
NTSTATUS
AtaPortRecoveryFromError(
    _In_ PATAPORT_PORT_DATA PortData)
{
    PATAPORT_DEVICE_EXTENSION DevExt;
    NTSTATUS Status;

    DevExt = AtaPortFindDeviceForAction(PortData, ACTION_DEVICE_ERROR);
    if (!DevExt)
        return STATUS_SUCCESS;

    Status = AtaPortDeviceProcessError(PortData, DevExt);
    if (Status == STATUS_ADAPTER_HARDWARE_ERROR)
    {
        AtaPortMarkDeviceFailed(PortData, DevExt);
        return Status;
    }

    AtaPortClearDeviceAction(DevExt, ACTION_DEVICE_ERROR);
    return STATUS_SUCCESS;
}

static
NTSTATUS
AtaPortConfigureDevice(
    _In_ PATAPORT_PORT_DATA PortData)
{
    PATAPORT_DEVICE_EXTENSION DevExt;
    NTSTATUS Status;

    DevExt = AtaPortFindDeviceForAction(PortData, ACTION_DEVICE_CONFIG);
    if (!DevExt)
        return STATUS_SUCCESS;

    Status = AtaPortDeviceProcessConfig(PortData, DevExt);
    if (Status == STATUS_ADAPTER_HARDWARE_ERROR)
    {
        AtaPortMarkDeviceFailed(PortData, DevExt);
        return Status;
    }

    if (AtaPortClearDeviceAction(DevExt, ACTION_DEVICE_CONFIG))
        KeSetEvent(&DevExt->Worker.ConfigureEvent, 0, FALSE);

    return STATUS_SUCCESS;
}

static
NTSTATUS
AtaPortDeviceChangePower(
    _In_ PATAPORT_PORT_DATA PortData)
{
    PATAPORT_DEVICE_EXTENSION DevExt;
    NTSTATUS Status;

    DevExt = AtaPortFindDeviceForAction(PortData, ACTION_DEVICE_POWER);
    if (!DevExt)
        return STATUS_SUCCESS;

    Status = AtaPortDeviceProcessPowerChange(PortData, DevExt);
    if (Status == STATUS_ADAPTER_HARDWARE_ERROR)
    {
        AtaPortMarkDeviceFailed(PortData, DevExt);
        return Status;
    }

    AtaPortClearDeviceAction(DevExt, ACTION_DEVICE_POWER);
    return STATUS_SUCCESS;
}

static
NTSTATUS
AtaPortSetTransferMode(
    _In_ PATAPORT_PORT_DATA PortData)
{
    AtaPortSelectTimings(PortData, FALSE);
    AtaPortClearPortAction(&PortData->Worker, ACTION_PORT_TIMING);

    return STATUS_SUCCESS;
}

static
BOOLEAN
AtaPortGetNextEvent(
    _In_ PATAPORT_PORT_DATA PortData,
    _Out_ ATA_PORT_ACTION* Action)
{
    KIRQL OldIrql;
    ULONG i, EventIndex;
    BOOLEAN Success;

    OldIrql = KeAcquireInterruptSpinLock(PortData->InterruptObject);

    /* Handle events by priority order */
    Success = _BitScanForward(&EventIndex, PortData->Worker.EventsPending);
    if (!Success)
    {
        PATA_DEVICE_REQUEST Request;

        /* Resume port I/O */
        ASSERT(!(PortData->InterruptFlags & PORT_INT_FLAG_IS_IO_ACTIVE));
        _InterlockedOr(&PortData->InterruptFlags, PORT_INT_FLAG_IS_IO_ACTIVE);

        /* Complete failed request */
        Request = PortData->Worker.FailedRequest;
        if (Request && !(PortData->Worker.PausedSlotsBitmap & (1 << Request->Slot)))
        {
            AtaReqStartCompletionDpc(Request);
        }
        PortData->Worker.FailedRequest = NULL;

        /* Requeue saved commands */
        for (i = 0; i < MAX_SLOTS; ++i)
        {
            if (!(PortData->Worker.PausedSlotsBitmap & (1 << i)))
              continue;

            Request = PortData->Slots[i];
            ASSERT_REQUEST(Request);
            ASSERT(Request != &PortData->Worker.InternalRequest);

            Request->SrbStatus = SRB_STATUS_BUSY;
            Request->InternalState = REQUEST_STATE_REQUEUE;
            AtaReqStartCompletionDpc(Request);
        }

        PortData->Worker.PausedSlotsBitmap = 0;
    }

    KeReleaseInterruptSpinLock(PortData->InterruptObject, OldIrql);

    *Action = 1 << EventIndex;
    return Success;
}

static
VOID
AtaPortRunStateMachine(
    _In_ PATAPORT_PORT_DATA PortData)
{
    ATA_PORT_ACTION Action;
    NTSTATUS Status;

    while (TRUE)
    {
        if (!AtaPortGetNextEvent(PortData, &Action))
            break;

#if DBG
        if (PortData->Worker.StateLoopCount++ > 1500)
        {
            ERR("CH %lu: Loop detected %lx\n", PortData->PortNumber,
                PortData->Worker.EventsPending);
            ASSERT(FALSE);
        }
#endif

        switch (Action)
        {
            case ACTION_PORT_RESET:
                Status = AtaPortResetPort(PortData);
                break;
            case ACTION_ENUM_PORT:
                Status = AtaPortEnumeratePort(PortData);
                break;
            case ACTION_ENUM_DEVICE:
            case ACTION_ENUM_DEVICE_NEW:
                Status = AtaPortEnumerateDevice(PortData, Action);
                break;
            case ACTION_PORT_TIMING:
                Status = AtaPortSetTransferMode(PortData);
                break;
            case ACTION_DEVICE_CONFIG:
                Status = AtaPortConfigureDevice(PortData);
                break;
            case ACTION_DEVICE_ERROR:
                Status = AtaPortRecoveryFromError(PortData);
                break;
            case ACTION_DEVICE_POWER:
                Status = AtaPortDeviceChangePower(PortData);
                break;

            default:
                ASSERT(FALSE);
                UNREACHABLE;
        }

        if (Status == STATUS_ADAPTER_HARDWARE_ERROR)
            _InterlockedOr(&PortData->Worker.EventsPending, ACTION_PORT_RESET);
    }
}

static
VOID
AtaPortWaitForIdle(
    _In_ PATAPORT_PORT_DATA PortData)
{
    KIRQL OldIrql;
    BOOLEAN DoWait = FALSE;

    KeAcquireSpinLock(&PortData->QueueLock, &OldIrql);
    KeClearEvent(&PortData->QueueStoppedEvent);
    if (!AtaPortQueueEmpty(PortData))
    {
        PortData->QueueFlags |= PORT_QUEUE_FLAG_SIGNAL_STOP;
        DoWait = TRUE;
    }
    KeReleaseSpinLock(&PortData->QueueLock, OldIrql);
    if (DoWait)
    {
        KeWaitForSingleObject(&PortData->QueueStoppedEvent, Executive, KernelMode, FALSE, NULL);
    }
}

static
VOID
AtaPortEnterStateMachine(
    _In_ PATAPORT_PORT_DATA PortData)
{
    PATAPORT_CHANNEL_EXTENSION ChanExt;
    ATA_SCSI_ADDRESS AtaScsiAddress;

    ChanExt = CONTAINING_RECORD(PortData, ATAPORT_CHANNEL_EXTENSION, PortData);
    ASSERT(IS_FDO(ChanExt));

    AtaScsiAddress.AsULONG = 0;
    while (TRUE)
    {
        PATAPORT_DEVICE_EXTENSION DevExt;
        PULONG PowerIdleCounter;

        /*
         * Acquire a reference to make sure the device object is valid
         * for the duration of the port event handling.
         */
        DevExt = AtaFdoFindNextDeviceByPath(ChanExt,
                                            &AtaScsiAddress,
                                            FALSE,
                                            AtaPortWorkerThread);
        if (!DevExt)
            break;

        DevExt->Worker.Flags = DEV_WORKER_FLAG_HOLD_REFERENCE;
        DevExt->Worker.ResetRetryCount = 0;

        if (DevExt->Device.QueueFlags & QUEUE_FLAG_FROZEN_REMOVED)
        {
            DevExt->Worker.Flags |= DEV_WORKER_FLAG_REMOVED;
            continue;
        }

        /* Stop the Srb processing */
        AtaReqFreezeQueue(DevExt, QUEUE_FLAG_FROZEN_PORT_BUSY);

        /*
         * There is a chance that we can receive a power down request
         * while the state maching is being running, so mark the device as busy
         * to reduce the request probability.
         */
        PowerIdleCounter = DevExt->Device.PowerIdleCounter;
        if (PowerIdleCounter)
            PoSetDeviceBusy(PowerIdleCounter);
    }

    /* Wait for the port queue to become empty */
    AtaPortWaitForIdle(PortData);

    PortData->Worker.Flags = 0;
    PortData->Worker.BadDeviceBitmap = 0;
    PortData->Worker.ResetRetryCount = 0;
#if DBG
    PortData->Worker.StateLoopCount = 0;
#endif
}

static
VOID
AtaPortExitStateMachine(
    _In_ PATAPORT_PORT_DATA PortData)
{
    PATAPORT_CHANNEL_EXTENSION ChanExt;
    ATA_SCSI_ADDRESS AtaScsiAddress;

    ChanExt = CONTAINING_RECORD(PortData, ATAPORT_CHANNEL_EXTENSION, PortData);

    AtaScsiAddress.AsULONG = 0;
    while (TRUE)
    {
        PATAPORT_DEVICE_EXTENSION DevExt;
        KIRQL OldIrql;

        DevExt = AtaFdoFindNextDeviceByPath(ChanExt, &AtaScsiAddress, TRUE, NULL);
        if (!DevExt)
            break;

        if (DevExt->Worker.Flags & DEV_WORKER_FLAG_REMOVED)
        {
            KeAcquireSpinLock(&ChanExt->PdoListLock, &OldIrql);
            if (!DevExt->RemovalPending)
                PortData->Worker.Flags |= WORKER_FLAG_NEED_RESCAN;
            DevExt->RemovalPending = TRUE;
            KeReleaseSpinLock(&ChanExt->PdoListLock, OldIrql);

            AtaReqFreezeQueue(DevExt, QUEUE_FLAG_FROZEN_REMOVED);
            AtaReqFlushDeviceQueue(&DevExt->Device);

            AtaDeviceFlushPowerIrpQueue(DevExt);
            KeSetEvent(&DevExt->Worker.ConfigureEvent, 0, FALSE);

            DevExt->Worker.EnumStatus = DEV_STATUS_NO_DEVICE;
            KeSetEvent(&DevExt->Worker.EnumerationEvent, 0, FALSE);
        }
        else
        {
            /* Resume the Srb processing */
            AtaReqThawQueue(DevExt, QUEUE_FLAG_FROZEN_PORT_BUSY);
        }

        /* Release a reference */
        if (DevExt->Worker.Flags & DEV_WORKER_FLAG_HOLD_REFERENCE)
          IoReleaseRemoveLock(&DevExt->Common.RemoveLock, AtaPortWorkerThread);
    }

    if (PortData->Worker.Flags & WORKER_FLAG_COMPLETE_PORT_ENUM_EVENT)
    {
        /* We defer this event completion to avoid a BusRelations request */
        PortData->Worker.Flags &= ~WORKER_FLAG_NEED_RESCAN;

        KeSetEvent(&PortData->Worker.EnumerationEvent, 0, FALSE);
    }

    if (PortData->Worker.Flags & WORKER_FLAG_NEED_RESCAN)
        IoInvalidateDeviceRelations(ChanExt->Pdo, BusRelations);
}

static
VOID
AtaPortWorkerClearSignal(
    _In_ PATAPORT_PORT_DATA PortData)
{
    KIRQL OldIrql;

    KeAcquireSpinLock(&PortData->Worker.Lock, &OldIrql);
    KeClearEvent(&PortData->Worker.ThreadEvent);
    KeReleaseSpinLock(&PortData->Worker.Lock, OldIrql);
}

VOID
NTAPI
AtaPortWorkerThread(
    _In_ PVOID StartContext)
{
    PATAPORT_PORT_DATA PortData = StartContext;

    while (TRUE)
    {
        KeWaitForSingleObject(&PortData->Worker.ThreadEvent,
                              Executive,
                              KernelMode,
                              FALSE,
                              NULL);
        if (PortData->PortFlags & PORT_FLAG_EXIT_THREAD)
            break;

        AtaPortWorkerClearSignal(PortData);
        AtaPortEnterStateMachine(PortData);
        AtaPortRunStateMachine(PortData);
        AtaPortExitStateMachine(PortData);
    }

    PsTerminateSystemThread(STATUS_SUCCESS);
}

VOID
__cdecl
AtaPortNotification(
    _In_ PORT_NOTIFICATION_TYPE NotificationType,
    _In_ PVOID PortContext,
    ...)
{
    PATAPORT_PORT_DATA PortData = PortContext;
    va_list ap;

    va_start(ap, PortContext);

    TRACE("CH %lu: Notification %lu\n", PortData->PortNumber, NotificationType);

    switch (NotificationType)
    {
        case AtaRequestComplete:
        {
            AtaPortOnRequestComplete(PortData, (ULONG)va_arg(ap, ULONG));
            break;
        }

        case AtaResetDetected:
        {
            ASSERT(PortData->Worker.Thread == KeGetCurrentThread());
            ASSERT(!(PortData->InterruptFlags & PORT_INT_FLAG_IS_IO_ACTIVE));
            AtaPortOnResetNotification(PortData, (ULONG)va_arg(ap, ULONG));
            break;
        }

        case AtaBusChangeDetected:
        {
            ASSERT(KeGetCurrentIrql() > DISPATCH_LEVEL);

            if (!(PortData->InterruptFlags & PORT_INT_FLAG_IGNORE_LINK_IRQ))
                AtaPortQueueEvent(PortData, NULL, NULL, ACTION_PORT_RESET);
            else
                TRACE("CH %lu: Ignore link IRQ\n", PortData->PortNumber);
            break;
        }

        case AtaRequestFailed:
        {
            AtaPortQueueEvent(PortData,
                              NULL,
                              (PATA_DEVICE_REQUEST)va_arg(ap, PATA_DEVICE_REQUEST),
                              ACTION_DEVICE_ERROR);
            break;
        }

        case AtaAsyncNotificationDetected:
        {
            AtaPortOnAsyncNotification(PortData, (ULONG)va_arg(ap, ULONG));
            break;
        }

        default:
            ERR("CH %lu: Unsupported notification %lu\n", PortData->PortNumber, NotificationType);
            break;
    }

    va_end(ap);
}

ATA_COMPLETION_ACTION
AtaPortCompleteInternalRequest(
    _In_ PATA_DEVICE_REQUEST Request)
{
    PATAPORT_IO_CONTEXT Device = (PATAPORT_IO_CONTEXT)Request->Device;
    PATAPORT_PORT_DATA PortData = Device->PortData;

    ASSERT(Request->Flags & REQUEST_FLAG_INTERNAL);
    KeSetEvent(&PortData->Worker.CompletionEvent, IO_NO_INCREMENT, FALSE);

    return COMPLETE_IRP;
}

NTSTATUS
AtaPortSendRequest(
    _In_ PATAPORT_PORT_DATA PortData,
    _In_ PATAPORT_DEVICE_EXTENSION DevExt)
{
    PATA_DEVICE_REQUEST Request = &PortData->Worker.InternalRequest;
    KIRQL OldIrql;

    KeClearEvent(&PortData->Worker.CompletionEvent);

    Request->Flags |= REQUEST_FLAG_INTERNAL;
    Request->Device = (PATA_IO_CONTEXT_COMMON)&DevExt->Device;

    PortData->Worker.OldRequest = PortData->Slots[0];
    PortData->Slots[0] = Request;

    KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);

    KeAcquireSpinLockAtDpcLevel(&PortData->QueueLock);
    PortData->ActiveTimersBitmap |= 1 << 0;
    KeReleaseSpinLockFromDpcLevel(&PortData->QueueLock);

    AtaReqSendRequest(Request);

    KeLowerIrql(OldIrql);

    KeWaitForSingleObject(&PortData->Worker.CompletionEvent, Executive, KernelMode, FALSE, NULL);

    /* Stop the timer */
    KeAcquireSpinLock(&PortData->QueueLock, &OldIrql);
    PortData->ActiveTimersBitmap = 0;
    KeReleaseSpinLock(&PortData->QueueLock, OldIrql);

    PortData->Slots[0] = PortData->Worker.OldRequest;

    if ((Request->SrbStatus == SRB_STATUS_TIMEOUT) ||
        (Request->SrbStatus == SRB_STATUS_BUS_RESET))
    {
        return STATUS_ADAPTER_HARDWARE_ERROR;
    }

    if (Request->SrbStatus == SRB_STATUS_SUCCESS)
        return STATUS_SUCCESS;

    return STATUS_IO_DEVICE_ERROR;
}

VOID
AtaPortTimeout(
    _In_ PATAPORT_PORT_DATA PortData,
    _In_ ULONG Slot)
{
    PATA_DEVICE_REQUEST Request;

    ASSERT(KeGetCurrentIrql() > DISPATCH_LEVEL);

    Request = PortData->Slots[Slot];
    ASSERT_REQUEST(Request);

    ERR("CH %lu: %sSlot %lu (%08lx) timed out %lx (%lus)\n",
        PortData->PortNumber,
        IS_ATAPI(Request->Device) ? "" : "*",
        Slot,
        1 << Slot,
        Request->Flags,
        Request->TimeOut);
    if (Request->Flags & REQUEST_FLAG_PACKET_COMMAND)
    {
        INFO("CH %lu: CDB %02x:%02x:%02x:%02x:%02x:%02x:%02x\n",
             PortData->PortNumber,
             Request->Cdb[0],
             Request->Cdb[1],
             Request->Cdb[2],
             Request->Cdb[3],
             Request->Cdb[4],
             Request->Cdb[5],
             Request->Cdb[6]);
    }
    else
    {
        INFO("CH %lu: TF %02x:%02x:%02x:%02x:%02x:%02x:%02x\n",
             PortData->PortNumber,
             Request->TaskFile.Command,
             Request->TaskFile.Feature,
             Request->TaskFile.SectorCount,
             Request->TaskFile.LowLba,
             Request->TaskFile.MidLba,
             Request->TaskFile.HighLba,
             Request->TaskFile.DriveSelect);
    }

    Request->SrbStatus = SRB_STATUS_TIMEOUT;

    /* The active command has timed out, set the ATA outputs to something meaningful */
    Request->Output.Status = IDE_STATUS_ERROR;
    Request->Output.Error = IDE_ERROR_COMMAND_ABORTED;

    AtaPortQueueEvent(PortData, NULL, Request, ACTION_PORT_RESET | ACTION_DEVICE_ERROR);
}

DECLSPEC_NOINLINE_FROM_PAGED
VOID
AtaPortSignalWorkerThread(
    _In_ PATAPORT_PORT_DATA PortData)
{
    KIRQL OldIrql;

    KeAcquireSpinLock(&PortData->Worker.Lock, &OldIrql);
    KeSetEvent(&PortData->Worker.ThreadEvent, IO_NO_INCREMENT, FALSE);
    KeReleaseSpinLock(&PortData->Worker.Lock, OldIrql);
}

VOID
NTAPI
AtaPortWorkerSignalDpc(
    _In_ PKDPC Dpc,
    _In_opt_ PVOID DeferredContext,
    _In_opt_ PVOID SystemArgument1,
    _In_opt_ PVOID SystemArgument2)
{
    PATAPORT_PORT_DATA PortData = DeferredContext;

    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(SystemArgument1);
    UNREFERENCED_PARAMETER(SystemArgument2);

    AtaPortSignalWorkerThread(PortData);
}

DECLSPEC_NOINLINE_FROM_PAGED
VOID
AtaDeviceQueueEvent(
    _In_ PATAPORT_PORT_DATA PortData,
    _In_opt_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ ATA_PORT_ACTION Action)
{
    KIRQL OldIrql;

    OldIrql = KeAcquireInterruptSpinLock(PortData->InterruptObject);
    AtaPortQueueEvent(PortData, DevExt, NULL, Action);
    KeReleaseInterruptSpinLock(PortData->InterruptObject, OldIrql);
}
