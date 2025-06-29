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
ATA_PORT_ACTION
AtaFsmClearPortAction(
    _In_ PATA_WORKER_CONTEXT Context,
    _In_ ATA_PORT_ACTION Action)
{
    return _InterlockedAnd(&Context->EventsPending, ~(ULONG)Action);
}

static
ATA_PORT_ACTION
AtaFsmClearDeviceAction(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ ATA_PORT_ACTION Action)
{
    return _InterlockedAnd(&DevExt->Worker.EventsPending, ~(ULONG)Action);
}

static
PATAPORT_DEVICE_EXTENSION
AtaFsmFindDeviceForAction(
    _In_ PATAPORT_PORT_DATA PortData,
    _In_ ATA_PORT_ACTION Action)
{
    PATAPORT_CHANNEL_EXTENSION ChanExt = PortData->ChanExt;
    PATAPORT_DEVICE_EXTENSION DevExt, Result = NULL;
    PSINGLE_LIST_ENTRY Entry;
    KIRQL OldIrql;

    KeAcquireSpinLockAtDpcLevel(&ChanExt->PdoListLock);

    /* Raise IRQL to avoid event lost */
    OldIrql = KeAcquireInterruptSpinLock(PortData->ChanExt->InterruptObject);

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
    {
        AtaFsmClearPortAction(&PortData->Worker, Action);
    }

    KeReleaseInterruptSpinLock(PortData->ChanExt->InterruptObject, OldIrql);

    KeReleaseSpinLockFromDpcLevel(&ChanExt->PdoListLock);

    return Result;
}

static
VOID
AtaFsmRemoveDevices(
    _In_ PATAPORT_PORT_DATA PortData,
    _In_ ULONG TargetBitmap)
{
    ATA_SCSI_ADDRESS AtaScsiAddress;

    AtaScsiAddress.AsULONG = 0;
    AtaScsiAddress.PathId = PortData->PortNumber;
    while (TRUE)
    {
        PATAPORT_DEVICE_EXTENSION DevExt;

        DevExt = AtaFdoFindNextDeviceByPath(PortData->ChanExt, &AtaScsiAddress, 0, NULL);
        if (!DevExt)
            break;

        if ((TargetBitmap & (1 << (ULONG)AtaScsiAddress.TargetId)))
            DevExt->Worker.Flags |= DEV_WORKER_FLAG_REMOVED;
    }
}

VOID
AtaFsmCompletePortEnumEvent(
    _In_ PATAPORT_PORT_DATA PortData,
    _In_ ULONG TargetBitmap)
{
    ULONG TargetDiff, TargetsToRemove;

    TargetDiff = TargetBitmap ^ PortData->Worker.TargetBitmap;
    TargetsToRemove = TargetDiff & PortData->Worker.TargetBitmap;

    if (IS_AHCI_EXT(PortData->ChanExt))
    {
        if ((TargetBitmap == 0) && !(PortData->PortFlags & PORT_FLAG_IS_PMP))
            AtaAhciEnterPhyListenMode(PortData);

        AtaAhciEnableInterrupts(PortData);
    }

    // TODO check pmp change

    if (TargetsToRemove != 0)
        AtaFsmRemoveDevices(PortData, TargetsToRemove);

    if (TargetDiff != 0)
        PortData->Worker.Flags |= WORKER_FLAG_NEED_RESCAN;

    if (TargetBitmap == 0)
        PortData->Worker.Flags |= WORKER_FLAG_PORT_REMOVED;

    PortData->Worker.Flags &= ~WORKER_FLAG_USE_LOCAL_DEVICE;

    AtaFsmSetState(&PortData->Worker, PORT_STATE_PULL_EVENT);

    _InterlockedExchange(&PortData->Worker.TargetBitmap, TargetBitmap);

    /* Complete the action and defer the PortData->Worker.EnumerationEvent */
    AtaFsmClearPortAction(&PortData->Worker, ACTION_ENUM_PORT);
}

VOID
AtaFsmCompleteDeviceEnumEvent(
    _In_ PATA_WORKER_CONTEXT Context,
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ ATA_DEVICE_STATUS Status)
{
    ATA_PORT_ACTION OldEvents;

    if ((Status != DEV_STATUS_SAME_DEVICE) && !(DevExt->Device.DeviceFlags & DEVICE_UNINITIALIZED))
    {
        DevExt->Worker.Flags |= DEV_WORKER_FLAG_REMOVED;

        if (!(DevExt->Worker.EventsPending & ACTION_ENUM_DEVICE_EXT))
        {
            Context->Flags |= WORKER_FLAG_NEED_RESCAN;
        }
    }

    _InterlockedExchange(&DevExt->Worker.EnumStatus, Status);

    OldEvents = AtaFsmClearDeviceAction(DevExt, ACTION_ENUM_DEVICE_EXT | ACTION_ENUM_DEVICE_INT);
    if (OldEvents & ACTION_ENUM_DEVICE_EXT)
    {
        AtaFsmClearPortAction(Context, ACTION_ENUM_DEVICE_EXT);

        KeSetEvent(&DevExt->Worker.EnumerationEvent, 0, FALSE);
    }

    AtaFsmSetState(Context, PORT_STATE_PULL_EVENT);
}

VOID
AtaFsmCompleteDeviceErrorEvent(
    _In_ PATAPORT_PORT_DATA PortData)
{
    AtaFsmClearPortAction(&PortData->Worker, ACTION_DEVICE_ERROR);
    AtaFsmSetState(&PortData->Worker, PORT_STATE_PULL_EVENT);
}

VOID
AtaFsmCompleteDeviceConfigEvent(
    _In_ PATA_WORKER_CONTEXT Context,
    _In_ PATAPORT_DEVICE_EXTENSION DevExt)
{
    if (AtaFsmClearDeviceAction(DevExt, ACTION_DEVICE_CONFIG) & ACTION_DEVICE_CONFIG)
    {
        KeSetEvent(&DevExt->Worker.ConfigureEvent, 0, FALSE);
    }

    AtaFsmSetState(Context, PORT_STATE_PULL_EVENT);
}

VOID
AtaFsmCompleteDevicePowerEvent(
    _In_ PATA_WORKER_CONTEXT Context,
    _In_ PATAPORT_DEVICE_EXTENSION DevExt)
{
    AtaFsmClearDeviceAction(DevExt, ACTION_DEVICE_POWER);
    AtaFsmSetState(Context, PORT_STATE_PULL_EVENT);
}

VOID
AtaFsmResetPort(
    _In_ PATAPORT_PORT_DATA PortData,
    _In_ ULONG TargetId)
{
    ATA_SCSI_ADDRESS AtaScsiAddress;
    PATAPORT_DEVICE_EXTENSION DevExt;

    INFO("PORT %lu: Call reset\n", PortData->PortNumber);

    if (++PortData->Worker.ResetRetryCount > PORT_MAX_RESET_RETRY_COUNT)
    {
        ERR("PORT %lu: Too many reset attempts\n", PortData->PortNumber);
        AtaFsmCompletePortEnumEvent(PortData, 0);
        return;
    }

    if (TargetId != (ULONG)-1)
    {
        // TODO
    }

    /*
     * We are about to hard reset the port which will in turn
     * cause the attached devices to lose their software settings.
     * Enqueue a config event so the state machine can re-initialize all devices later on.
     */
    _InterlockedOr(&PortData->Worker.EventsPending, ACTION_ENUM_DEVICE_INT | ACTION_DEVICE_CONFIG);

    AtaScsiAddress.AsULONG = 0;
    AtaScsiAddress.PathId = PortData->PortNumber;
    while (TRUE)
    {
        ATA_PORT_ACTION Event;

        DevExt = AtaFdoFindNextDeviceByPath(PortData->ChanExt, &AtaScsiAddress, 0, NULL);
        if (!DevExt)
            break;

        if (DevExt->Device.DeviceFlags & DEVICE_PNP_STARTED)
            Event = ACTION_ENUM_DEVICE_INT | ACTION_DEVICE_CONFIG;
        else
            Event = ACTION_ENUM_DEVICE_INT; // ACPI GTF is not ready yet

        _InterlockedOr(&DevExt->Worker.EventsPending, Event);
    }

    if (IS_AHCI_EXT(PortData->ChanExt))
    {
        /* Need a device to send requests during port enumeration */
        PortData->Worker.Flags |= WORKER_FLAG_USE_LOCAL_DEVICE;

        PortData->Worker.PmpDetectRetryCount = 0;

        AtaFsmSetState(&PortData->Worker, PORT_STATE_PROCESS_AHCI);
        AtaFsmSetLocalState(&PortData->Worker, AHCI_STATE_ENUM_RESET);
    }
    else
    {
        AtaFsmSetState(&PortData->Worker, PORT_STATE_PROCESS_PATA);
        AtaFsmSetLocalState(&PortData->Worker, PATA_STATE_RESET);
    }
}

static
VOID
AtaFsmHandleEventEnumPort(
    _In_ PATAPORT_PORT_DATA PortData)
{
    PortData->Worker.Flags |= WORKER_FLAG_DEFER_ENUM_PORT_COMPLETE;

    if (IS_AHCI_EXT(PortData->ChanExt))
    {
        INFO("PORT %lu: Process enumeration request\n", PortData->PortNumber);

        AtaFsmResetPort(PortData, (ULONG)-1);
    }
    else
    {
        INFO("PORT %p: Process enumeration request\n", PortData->Pata.CommandPortBase);

        AtaFsmSetState(&PortData->Worker, PORT_STATE_PROCESS_PATA);
        AtaFsmSetLocalState(&PortData->Worker, PATA_STATE_PORT_ENUM);
    }
}

static
VOID
AtaFsmHandleEventEnumDevice(
    _In_ PATAPORT_PORT_DATA PortData)
{
    PATAPORT_DEVICE_EXTENSION DevExt;

    DevExt = AtaFsmFindDeviceForAction(PortData, ACTION_ENUM_DEVICE_INT);
    if (!DevExt)
    {
        /* We need the enum device extension to discover new devices */
        if (PortData->Worker.EventsPending & ACTION_ENUM_DEVICE_EXT)
        {
            DevExt = PortData->Worker.EnumDevExt;

            if (PortData->Worker.Flags & WORKER_FLAG_PORT_REMOVED)
            {
                AtaFsmCompleteDeviceEnumEvent(&PortData->Worker, DevExt, DEV_STATUS_NO_DEVICE);
                DevExt = NULL;
            }
        }
    }
    PortData->Worker.DevExt = DevExt;

    if (DevExt)
    {
        PortData->Worker.IdentifyAttempt = 0;

        AtaFsmSetState(&PortData->Worker, PORT_STATE_PROCESS_ID);
        AtaFsmSetLocalState(&PortData->Worker, PORT_STATE_ID_IDENTIFY);
    }
    else
    {
        /* If no devices to enumerate - we are done */
        AtaFsmSetState(&PortData->Worker, PORT_STATE_PULL_EVENT);
    }
}

static
VOID
AtaFsmHandleEventDeviceError(
    _In_ PATAPORT_PORT_DATA PortData)
{
    if (!PortData->Worker.FailedRequest)
    {
        AtaFsmResetPort(PortData, (ULONG)-1);
        AtaFsmCompleteDeviceErrorEvent(PortData);
    }
    else
    {
        AtaFsmSetState(&PortData->Worker, PORT_STATE_PROCESS_RECOVERY);
        AtaFsmSetLocalState(&PortData->Worker, DEVICE_STATE_NEED_RECOVERY);
    }
}

static
VOID
AtaFsmHandleEventDeviceConfig(
    _In_ PATAPORT_PORT_DATA PortData)
{
    PATAPORT_DEVICE_EXTENSION DevExt;

    DevExt = AtaFsmFindDeviceForAction(PortData, ACTION_DEVICE_CONFIG);
    PortData->Worker.DevExt = DevExt;
    if (!DevExt)
    {
        /* If no devices to configure - we are done */
        AtaFsmSetState(&PortData->Worker, PORT_STATE_PULL_EVENT);
    }
    else
    {
        AtaFsmSetState(&PortData->Worker, PORT_STATE_PROCESS_CONFIG);
        AtaFsmSetLocalState(&PortData->Worker, CFG_STATE_ENTER);
    }
}

static
VOID
AtaFsmHandleEventDeviceTiming(
    _In_ PATAPORT_PORT_DATA PortData)
{
    AtaPortUpdateTimingInformation(PortData);
    AtaFsmClearPortAction(&PortData->Worker, ACTION_DEVICE_TIMING);

    KeSetEvent(&PortData->Worker.SetTimingsEvent, 0, FALSE);

    // TODO:
    /* PortData->Worker.Flags |= WORKER_FLAG_NEED_RESCAN; */
}

static
VOID
AtaFsmHandleEventDevicePower(
    _In_ PATAPORT_PORT_DATA PortData)
{
    PATAPORT_DEVICE_EXTENSION DevExt;

    DevExt = AtaFsmFindDeviceForAction(PortData, ACTION_DEVICE_POWER);
    PortData->Worker.DevExt = DevExt;
    if (!DevExt)
    {
        AtaFsmSetState(&PortData->Worker, PORT_STATE_PULL_EVENT);
    }
    else
    {
        AtaFsmSetState(&PortData->Worker, PORT_STATE_PROCESS_POWER);
        AtaFsmSetLocalState(&PortData->Worker, PWR_STATE_ENTER);
    }
}

static
VOID
AtaPortHandleEnterStateMachine(
    _In_ PATAPORT_PORT_DATA PortData)
{
    ATA_SCSI_ADDRESS AtaScsiAddress;

    PortData->Worker.ResetRetryCount = 0;
    PortData->Worker.BadTargetBitmap = 0;
#if DBG
    PortData->Worker.StateLoopCount = 0;
#endif

    AtaScsiAddress.AsULONG = 0;
    AtaScsiAddress.PathId = PortData->PortNumber;
    while (TRUE)
    {
        PATAPORT_DEVICE_EXTENSION DevExt;
        PULONG PowerIdleCounter;

        /*
         * Acquire a reference to make sure the device object is valid
         * for the duration of the port event handling.
         */
        DevExt = AtaFdoFindNextDeviceByPath(PortData->ChanExt,
                                            &AtaScsiAddress,
                                            0,
                                            AtaPortWorkerDpc);
        if (!DevExt)
            break;

        /* Stop the Srb processing */
        AtaReqFreezeQueue(DevExt, QUEUE_FLAG_FROZEN_PORT_BUSY);

        DevExt->Worker.Flags = DEV_WORKER_FLAG_HOLD_REFERENCE;

        /*
         * There is a chance that we can receive a power down request
         * while the state maching is being running, so mark the device as busy
         * to reduce the request probability.
         */
        PowerIdleCounter = DevExt->Device.PowerIdleCounter;
        if (PowerIdleCounter)
            PoSetDeviceBusy(PowerIdleCounter);
    }

    KeAcquireSpinLockAtDpcLevel(&PortData->QueueLock);

    /* Wait for the port queue to become empty */
    if (!AtaPortQueueEmpty(PortData))
    {
        PortData->QueueFlags |= PORT_QUEUE_FLAG_SIGNAL_STOP;
        PortData->Worker.Flags |= WORKER_FLAG_DISABLE_FSM;
    }

    KeReleaseSpinLockFromDpcLevel(&PortData->QueueLock);

    AtaFsmSetState(&PortData->Worker, PORT_STATE_RESUME_PORT_IO);
}

static
VOID
AtaPortHandleResumePortIo(
    _In_ PATAPORT_PORT_DATA PortData)
{
    /* Process the next event on the port */
    AtaFsmSetState(&PortData->Worker, PORT_STATE_PULL_EVENT);
}

static
VOID
AtaPortHandlePullEvent(
    _In_ PATAPORT_PORT_DATA PortData)
{
    KIRQL OldIrql;
    ATA_SCSI_ADDRESS AtaScsiAddress;
    BOOLEAN NeedRescan;
    ULONG i, EventIndex;

    if (PortData->Worker.EventsPending == 0)
    {
        /* Process removed devices */
        AtaScsiAddress.AsULONG = 0;
        AtaScsiAddress.PathId = PortData->PortNumber;
        while (TRUE)
        {
            PATAPORT_DEVICE_EXTENSION DevExt;

            DevExt = AtaFdoFindNextDeviceByPath(PortData->ChanExt,
                                                &AtaScsiAddress,
                                                SEARCH_REMOVED_DEV,
                                                NULL);
            if (!DevExt)
                break;

            if (!(DevExt->Worker.Flags & DEV_WORKER_FLAG_REMOVED))
                continue;

            KeAcquireSpinLockAtDpcLevel(&PortData->ChanExt->PdoListLock);
            DevExt->RemovalPending = TRUE;
            KeReleaseSpinLockFromDpcLevel(&PortData->ChanExt->PdoListLock);

            AtaReqFreezeQueue(DevExt, QUEUE_FLAG_FROZEN_REMOVED);
            AtaReqFlushDeviceQueue(&DevExt->Device);

            AtaDeviceFlushPowerIrpQueue(DevExt);
            AtaFsmCompleteDeviceConfigEvent(&PortData->Worker, DevExt);
            AtaFsmCompleteDeviceEnumEvent(&PortData->Worker, DevExt, DEV_STATUS_NO_DEVICE);
        }

        if (PortData->Worker.Flags & WORKER_FLAG_DEFER_ENUM_PORT_COMPLETE)
        {
            /* We defer this event to avoid a costly BusRelations request */
            PortData->Worker.Flags &= ~WORKER_FLAG_NEED_RESCAN;

            KeSetEvent(&PortData->Worker.EnumerationEvent, 0, FALSE);
        }
    }

    OldIrql = KeAcquireInterruptSpinLock(PortData->ChanExt->InterruptObject);

    /* Handle events by priority order */
    if (_BitScanForward(&EventIndex, PortData->Worker.EventsPending) != 0)
    {
        ATA_PORT_ACTION Action;

        KeReleaseInterruptSpinLock(PortData->ChanExt->InterruptObject, OldIrql);

        Action = 1 << EventIndex;

        /* Process the next event on the port */
        switch (Action)
        {
            case ACTION_DEVICE_ERROR:
                AtaFsmHandleEventDeviceError(PortData);
                break;
            case ACTION_ENUM_PORT:
                AtaFsmHandleEventEnumPort(PortData);
                break;
            case ACTION_ENUM_DEVICE_EXT:
            case ACTION_ENUM_DEVICE_INT:
                AtaFsmHandleEventEnumDevice(PortData);
                break;
            case ACTION_DEVICE_TIMING:
                AtaFsmHandleEventDeviceTiming(PortData);
                break;
            case ACTION_DEVICE_CONFIG:
                AtaFsmHandleEventDeviceConfig(PortData);
                break;
            case ACTION_DEVICE_POWER:
                AtaFsmHandleEventDevicePower(PortData);
                break;

            default:
                ASSERT(FALSE);
                UNREACHABLE;
        }
        return;
    }

    /* Resume port I/O */
    PortData->InterruptFlags &= ~(PORT_INT_FLAG_IS_DPC_ACTIVE | PORT_INT_FLAG_IGNORE_LINK_IRQ);
    PortData->InterruptFlags |= PORT_INT_FLAG_IS_IO_ACTIVE;

    /* Requeue saved commands */
    for (i = 0; i < AHCI_MAX_COMMAND_SLOTS; ++i)
    {
        PATA_DEVICE_REQUEST Request;

        if (!(PortData->Worker.PausedSlotsBitmap & (1 << i)))
            continue;

        Request = PortData->Slots[i];
        ASSERT_REQUEST(Request);
        ASSERT(Request != PortData->Worker.InternalRequest);

        Request->SrbStatus = SRB_STATUS_BUSY;
        Request->InternalState = REQUEST_STATE_REQUEUE;
        AtaReqStartCompletionDpc(Request);
    }

    NeedRescan = !!(PortData->Worker.Flags & WORKER_FLAG_NEED_RESCAN);

    /* All device events handled */
    PortData->Worker.Flags = WORKER_FLAG_DISABLE_FSM;
    PortData->Worker.PausedSlotsBitmap = 0;
    PortData->Worker.PausedQueuedSlotsBitmap = 0;
    PortData->Worker.FailedRequest = NULL;

    if (IS_AHCI_EXT(PortData->ChanExt))
        AtaAhciEnableInterrupts(PortData);

    KeReleaseInterruptSpinLock(PortData->ChanExt->InterruptObject, OldIrql);

    if (PortData->Worker.Flags & WORKER_FLAG_DEFER_ENUM_PORT_COMPLETE)
    {
        /* We defer this event to avoid a BusRelations request */
        PortData->Worker.Flags &= ~WORKER_FLAG_NEED_RESCAN;

        KeSetEvent(&PortData->Worker.EnumerationEvent, 0, FALSE);
    }

    AtaScsiAddress.AsULONG = 0;
    AtaScsiAddress.PathId = PortData->PortNumber;
    while (TRUE)
    {
        PATAPORT_DEVICE_EXTENSION DevExt;

        DevExt = AtaFdoFindNextDeviceByPath(PortData->ChanExt, &AtaScsiAddress, 0, NULL);
        if (!DevExt)
            break;

        /* Resume the Srb processing */
        AtaReqThawQueue(DevExt, QUEUE_FLAG_FROZEN_PORT_BUSY);

        /* Release a reference */
        if (DevExt->Worker.Flags & DEV_WORKER_FLAG_HOLD_REFERENCE)
        {
            IoReleaseRemoveLock(&DevExt->Common.RemoveLock, AtaPortWorkerDpc);
        }
    }

    if (NeedRescan)
        IoInvalidateDeviceRelations(PortData->ChanExt->Common.Self, BusRelations);
}

static
VOID
AtaPortRunStateMachine(
    _In_ PATAPORT_PORT_DATA PortData)
{
#if DBG
    if (PortData->Worker.StateLoopCount++ > 1500)
    {
        ERR("Loop detected %lx\n", PortData->Worker.EventsPending);
        ASSERT(FALSE);
    }
#endif

    switch (PortData->Worker.PortState)
    {
        case PORT_STATE_ENTER_STATE_MACHINE:
            AtaPortHandleEnterStateMachine(PortData);
            break;
        case PORT_STATE_RESUME_PORT_IO:
            AtaPortHandleResumePortIo(PortData);
            break;
        case PORT_STATE_PULL_EVENT:
            AtaPortHandlePullEvent(PortData);
            break;
        case PORT_STATE_PROCESS_ID:
            AtaDeviceIdRunStateMachine(&PortData->Worker);
            break;
        case PORT_STATE_PROCESS_CONFIG:
            AtaDeviceConfigRunStateMachine(&PortData->Worker);
            break;
        case PORT_STATE_PROCESS_RECOVERY:
            AtaDeviceRecoveryRunStateMachine(PortData);
            break;
        case PORT_STATE_PROCESS_AHCI:
            AtaAhciRunStateMachine(PortData);
            break;
        case PORT_STATE_PROCESS_PATA:
            AtaPataRunStateMachine(PortData);
            break;
        case PORT_STATE_PROCESS_POWER:
            AtaPowerRunStateMachine(&PortData->Worker);
            break;

        default:
            ASSERT(FALSE);
            UNREACHABLE;
    }
}

static
VOID
AtaFsmSetTimer(
    _In_ PATAPORT_PORT_DATA PortData)
{
    LARGE_INTEGER DueTime;

    PortData->Worker.Flags &= ~(WORKER_FLAG_DISABLE_FSM | WORKER_FLAG_NEED_TIMER);

    DueTime.QuadPart = Int32x32To64(PORT_TIMER_TICK_MS, -10000);
    KeSetTimer(&PortData->Worker.Timer, DueTime, &PortData->Worker.Dpc);
}

static
VOID
AtaFsmSendCommand(
    _In_ PATAPORT_PORT_DATA PortData)
{
    PATA_DEVICE_REQUEST Request = PortData->Worker.InternalRequest;

    if (Request->Flags & REQUEST_FLAG_PACKET_COMMAND)
    {
        INFO("PORT %lu: Send CDB %02x:%02x:%02x:%02x:%02x:%02x:%02x\n",
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
        PATA_TASKFILE TaskFile = &Request->TaskFile;

        INFO("PORT %lu: Send TF %02x:%02x:%02x:%02x:%02x:%02x:%02x\n",
             PortData->PortNumber,
             TaskFile->Command,
             TaskFile->Feature,
             TaskFile->SectorCount,
             TaskFile->LowLba,
             TaskFile->MidLba,
             TaskFile->HighLba,
             TaskFile->DriveSelect);
    }

    PortData->Worker.Flags &= ~WORKER_FLAG_NEED_REQUEST;

    if (PortData->Worker.Flags & WORKER_FLAG_USE_LOCAL_DEVICE)
        Request->Device = &PortData->Worker.InternalDevice;
    else
        Request->Device = &(PortData->Worker.DevExt)->Device;

    PortData->Worker.OldRequest = PortData->Slots[0];

    Request->Flags |= REQUEST_FLAG_INTERNAL;

    PortData->Slots[0] = Request;

    KeAcquireSpinLockAtDpcLevel(&PortData->QueueLock);
    PortData->ActiveTimersBitmap |= 1 << 0;
    KeReleaseSpinLockFromDpcLevel(&PortData->QueueLock);

    PortData->SendRequest(Request);
}

VOID
NTAPI
AtaPortWorkerDpc(
    _In_ PKDPC Dpc,
    _In_opt_ PVOID DeferredContext,
    _In_opt_ PVOID SystemArgument1,
    _In_opt_ PVOID SystemArgument2)
{
    PATAPORT_PORT_DATA PortData = DeferredContext;

    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(SystemArgument1);
    UNREFERENCED_PARAMETER(SystemArgument2);

    KeAcquireSpinLockAtDpcLevel(&PortData->Worker.Lock);

    if (!(PortData->Worker.Flags & WORKER_FLAG_DISABLE_FSM))
    {
        do
        {
            AtaPortRunStateMachine(PortData);
        }
        while (!(PortData->Worker.Flags & WORKER_FLAG_DISABLE_FSM));

        if (PortData->Worker.Flags & WORKER_FLAG_NEED_TIMER)
            AtaFsmSetTimer(PortData);

        if (PortData->Worker.Flags & WORKER_FLAG_NEED_REQUEST)
            AtaFsmSendCommand(PortData);
    }

    KeReleaseSpinLockFromDpcLevel(&PortData->Worker.Lock);
}

ATA_COMPLETION_ACTION
AtaPortCompleteInternalRequest(
    _In_ PATA_DEVICE_REQUEST Request)
{
    PATAPORT_PORT_DATA PortData = Request->Device->PortData;

    ASSERT(Request->Flags & REQUEST_FLAG_INTERNAL);

    /* Stop the timer */
    KeAcquireSpinLockAtDpcLevel(&PortData->QueueLock);
    PortData->ActiveTimersBitmap = 0;
    KeReleaseSpinLockFromDpcLevel(&PortData->QueueLock);

    PortData->Slots[0] = PortData->Worker.OldRequest;

    if ((Request->SrbStatus == SRB_STATUS_TIMEOUT &&
        !(PortData->Worker.Flags & WORKER_FLAG_MANUAL_PORT_RECOVERY)) ||
        (Request->SrbStatus == SRB_STATUS_BUS_RESET))
    {
        ERR("Internal request failed %u\n", Request->SrbStatus);
        AtaFsmResetPort(PortData, (ULONG)-1);
    }

    PortData->Worker.Flags &= ~WORKER_FLAG_DISABLE_FSM;

    KeInsertQueueDpc(&PortData->Worker.Dpc, NULL, NULL);

    return COMPLETE_IRP;
}

VOID
AtaPortQueueEmptyEvent(
    _In_ PATA_WORKER_CONTEXT Worker)
{
    KeAcquireSpinLockAtDpcLevel(&Worker->Lock);
    Worker->Flags &= ~WORKER_FLAG_DISABLE_FSM;
    KeReleaseSpinLockFromDpcLevel(&Worker->Lock);

    KeInsertQueueDpc(&Worker->Dpc, NULL, NULL);
}

DECLSPEC_NOINLINE_FROM_PAGED
VOID
AtaDeviceQueueEvent(
    _In_ PATAPORT_PORT_DATA PortData,
    _In_opt_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ ATA_PORT_ACTION Action)
{
    KIRQL OldIrql;
    BOOLEAN IsDpcActive;

    OldIrql = KeAcquireInterruptSpinLock(PortData->ChanExt->InterruptObject);

    if (DevExt)
    {
        ASSERT(DevExt->Device.PortData == PortData);

        _InterlockedOr(&DevExt->Worker.EventsPending, Action);
    }

    _InterlockedOr(&PortData->Worker.EventsPending, Action);

    IsDpcActive = !!(PortData->InterruptFlags & PORT_INT_FLAG_IS_DPC_ACTIVE);
    if (!IsDpcActive)
    {
        PortData->InterruptFlags |= PORT_INT_FLAG_IS_DPC_ACTIVE;
        PortData->Worker.Flags = 0;

        /* Prevent any new commands to be issued to the the port */
        PortData->InterruptFlags &= ~PORT_INT_FLAG_IS_IO_ACTIVE;
    }

    KeReleaseInterruptSpinLock(PortData->ChanExt->InterruptObject, OldIrql);

    if (IsDpcActive)
        return;

    AtaFsmSetState(&PortData->Worker, PORT_STATE_ENTER_STATE_MACHINE);
    KeInsertQueueDpc(&PortData->Worker.Dpc, NULL, NULL);
}

/* Called at DIRQL */
VOID
AtaPortRecoveryFromError(
    _In_ PATAPORT_PORT_DATA PortData,
    _In_ ATA_PORT_ACTION Action,
    _In_ PATA_DEVICE_REQUEST Request)
{
    ASSERT(KeGetCurrentIrql() > DISPATCH_LEVEL);

    if (Request)
        ASSERT(PortData->ActiveSlotsBitmap != 0);

    _InterlockedOr(&PortData->Worker.EventsPending, Action);

    /* First call, start the state machine */
    if (!(PortData->InterruptFlags & PORT_INT_FLAG_IS_DPC_ACTIVE))
    {
        ASSERT(Request != PortData->Worker.InternalRequest);
        PortData->Worker.FailedRequest = Request;

        /* Prevent any new commands to be issued to the the port */
        PortData->InterruptFlags &= ~PORT_INT_FLAG_IS_IO_ACTIVE;

        /* Save any commands pending on this port */
        PortData->Worker.PausedSlotsBitmap = PortData->ActiveSlotsBitmap;
        PortData->Worker.PausedQueuedSlotsBitmap = PortData->ActiveQueuedSlotsBitmap;
    }
    /* Internal request failed, start the state machine */
    else if ((PortData->ActiveSlotsBitmap & 1) &&
             (PortData->Slots[0]->Flags & REQUEST_FLAG_INTERNAL))
    {
        PATA_DEVICE_REQUEST InternalRequest = PortData->Worker.InternalRequest;

        /* Disable further port interrupts */
        if (!Request && IS_AHCI_EXT(PortData->ChanExt))
        {
            AHCI_PORT_WRITE(PortData->Ahci.IoBase, PxInterruptEnable, 0);
        }

        _InterlockedAnd(&PortData->Worker.EventsPending, ~Action);

        if (!Request)
            InternalRequest->SrbStatus = SRB_STATUS_BUS_RESET;

        AtaReqStartCompletionDpc(InternalRequest);
    }
    PortData->ActiveSlotsBitmap = 0;
    PortData->ActiveQueuedSlotsBitmap = 0;

    if (!(PortData->InterruptFlags & PORT_INT_FLAG_IS_DPC_ACTIVE))
    {
        PortData->InterruptFlags |= PORT_INT_FLAG_IS_DPC_ACTIVE;
        PortData->Worker.Flags = 0;

        /* Disable further port interrupts */
        if (!Request && IS_AHCI_EXT(PortData->ChanExt))
        {
            AHCI_PORT_WRITE(PortData->Ahci.IoBase, PxInterruptEnable, 0);
        }

        AtaFsmSetState(&PortData->Worker, PORT_STATE_ENTER_STATE_MACHINE);
        KeInsertQueueDpc(&PortData->Worker.Dpc, NULL, NULL);
    }
}

/* Called at DIRQL */
VOID
AtaPortTimeout(
    _In_ PATAPORT_PORT_DATA PortData,
    _In_ ULONG Slot)
{
    PATA_DEVICE_REQUEST Request;

    ASSERT(KeGetCurrentIrql() > DISPATCH_LEVEL);

    Request = PortData->Slots[Slot];
    ASSERT_REQUEST(Request);

    ERR("PORT %lu: %sSlot %lu (%08lx) timed out %lx\n",
        PortData->PortNumber,
        IS_ATAPI(Request->Device) ? "" : "*",
        Slot,
        1 << Slot,
        Request->Flags);

    if (IS_AHCI_EXT(PortData->ChanExt))
    {

    }
    else
    {
        PortData->Pata.CommandFlags = CMD_FLAG_NONE;
        ERR("Device status %02x\n", ATA_READ(PortData->Pata.Registers.Status));
    }

    Request->SrbStatus = SRB_STATUS_TIMEOUT;

    /* The active command has timed out, set the ATA outputs to something meaningful */
    Request->Output.Status = IDE_STATUS_ERROR;
    Request->Output.Error = IDE_ERROR_COMMAND_ABORTED;

    AtaPortRecoveryFromError(PortData, ACTION_DEVICE_ERROR, Request);
}
