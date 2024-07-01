/*
 * PROJECT:     ReactOS ATA Port Driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     AHCI request handling
 * COPYRIGHT:   Copyright 2024 Dmitry Borisov <di.sean@protonmail.com>
 */

/* INCLUDES *******************************************************************/

#include "atapi.h"

/* GLOBALS ********************************************************************/

static DRIVER_CANCEL AtaAhciPortCancelPendingCommand;

extern
VOID
AtaReqCompleteRequest(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ PATA_DEVICE_REQUEST Request);

extern
VOID
AtaAhciExecuteCommand(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ PATA_DEVICE_REQUEST Request);

extern
ULONG
AtaReqPrepareDataTransfer(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ PATA_DEVICE_REQUEST Request);

/* FUNCTIONS ******************************************************************/

static
inline
VOID
AtaAhciPortStartDma(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ PATA_DEVICE_REQUEST Request,
    _In_ ULONG Slot)
{
    ASSERT(Request->Slot == Slot);
    ASSERT(!(DevExt->ActiveSlotsBitmap & 1 << Slot));
    ASSERT(!(DevExt->ActiveQueuedSlotsBitmap & 1 << Slot));

    ASSERT(DevExt->Slots[Slot] == Request);

    DevExt->TimerCount[Slot] = Request->TimeOut;
    DevExt->ActiveSlotsBitmap |= 1 << Slot;

    if (Request->Flags & REQUEST_FLAG_NCQ)
    {
        DevExt->ActiveQueuedSlotsBitmap |= 1 << Slot;

        AHCI_PORT_WRITE(DevExt->IoBase, PxSataActive, 1 << Slot);
    }
    AHCI_PORT_WRITE(DevExt->IoBase, PxCommandIssue, 1 << Slot);
}

VOID
AtaAhciPortResumeCommands(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt)
{
    KIRQL OldLevel;
    ULONG i, SlotsBitmap;
    PATA_DEVICE_REQUEST Request;

    KeAcquireSpinLock(&DevExt->DeviceLock, &OldLevel);

    ASSERT(!(DevExt->PortFlags & PORT_ACTIVE));
    ASSERT(DevExt->ActiveSlotsBitmap == 0);
    ASSERT(DevExt->ActiveQueuedSlotsBitmap == 0);

    /* Re-issue a single command to the device */
    while (_BitScanForward(&i, DevExt->PausedSlotsBitmap))
    {
        DevExt->PausedSlotsBitmap &= ~(1 << i);

        Request = DevExt->Slots[i];
        ASSERT(Request);

        /* We're already canceled */
        if (!IoSetCancelRoutine(Request->Irp, NULL))
            continue;

        KeReleaseSpinLock(&DevExt->DeviceLock, OldLevel);

        Request->Flags |= REQUEST_BYPASS_ACTIVE_QUEUE;
        AtaAhciExecuteCommand(DevExt, Request);
        return;
    }

    ASSERT(DevExt->PausedSlotsBitmap == 0);

    /* Start the Srb processing */
    KeAcquireSpinLockAtDpcLevel(&DevExt->QueueLock);
    DevExt->QueueFlags &= ~QUEUE_FLAG_FROZEN_PAUSED;
    KeReleaseSpinLockFromDpcLevel(&DevExt->QueueLock);

    DevExt->PortFlags |= PORT_ACTIVE;

    SlotsBitmap = DevExt->PreparedSlotsBitmap;
    DevExt->PreparedSlotsBitmap = 0;

    KeReleaseSpinLock(&DevExt->DeviceLock, OldLevel);

    /* Re-issue prepared commands to the device */
    for (i = 0; i < AHCI_MAX_COMMAND_SLOTS; ++i)
    {
        if (!(SlotsBitmap & (1 << i)))
            continue;

        Request = DevExt->Slots[i];
        ASSERT(Request);

        AtaAhciPortStartDma(DevExt, Request, i);
    }
}

static
VOID
NTAPI
AtaAhciPortCancelPendingCommand(
    _Inout_ PDEVICE_OBJECT DeviceObject,
    _Inout_ _IRQL_uses_cancel_ PIRP Irp)
{
    PATAPORT_DEVICE_EXTENSION DevExt;
    PATA_DEVICE_REQUEST Request;
    KIRQL OldLevel;

    UNREFERENCED_PARAMETER(DeviceObject);

    IoReleaseCancelSpinLock(Irp->CancelIrql);

    Request = Irp->Tail.Overlay.DriverContext[0];
    ASSERT(Request);

    DevExt = Request->DevExt;

    KeAcquireSpinLock(&DevExt->DeviceLock, &OldLevel);
    DevExt->PausedSlotsBitmap &= ~(1 << Request->Slot);
    KeReleaseSpinLock(&DevExt->DeviceLock, OldLevel);

    Request->Srb->InternalStatus = STATUS_CANCELLED;
    Request->SrbStatus = SRB_STATUS_ABORTED;

    /* We need to be at DISPATCH_LEVEL for the DMA API */
    KeRaiseIrql(DISPATCH_LEVEL, &OldLevel);
    AtaReqCompleteRequest(DevExt, Request);
    KeLowerIrql(OldLevel);
}

static
VOID
AtaAhciPortSaveCommands(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ ULONG CurrentCommandMask)
{
    ULONG i, CancelBitmap = 0;

    KeAcquireSpinLockAtDpcLevel(&DevExt->DeviceLock);

    DevExt->PausedSlotsBitmap |= DevExt->ActiveSlotsBitmap | CurrentCommandMask;
    DevExt->ActiveSlotsBitmap = 0;

    for (i = 0; i < AHCI_MAX_COMMAND_SLOTS; ++i)
    {
        PATA_DEVICE_REQUEST Request;
        PIRP Irp;

        if (!(DevExt->PausedSlotsBitmap & (1 << i)))
            continue;

        /* Stop all enabled timers */
        DevExt->TimerCount[i] = TIMER_STATE_STOPPED;

        Request = DevExt->Slots[i];
        ASSERT(Request);

        /* Allow pending IRPs to be cancelled */
        Irp = Request->Irp;
        Irp->Tail.Overlay.DriverContext[0] = Request;
        (VOID)IoSetCancelRoutine(Irp, AtaAhciPortCancelPendingCommand);

        /* This IRP has already been cancelled */
        if (Irp->Cancel && IoSetCancelRoutine(Irp, NULL))
        {
            DevExt->PausedSlotsBitmap &= ~(1 << i);

            CancelBitmap |= 1 << i;
            continue;
        }
    }

    KeReleaseSpinLockFromDpcLevel(&DevExt->DeviceLock);

    /* Complete cancelled IRPs */
    if (CancelBitmap != 0)
    {
        for (i = 0; i < AHCI_MAX_COMMAND_SLOTS; ++i)
        {
            PATA_DEVICE_REQUEST Request;

            if (!(CancelBitmap & (1 << i)))
                continue;

            Request = DevExt->Slots[i];
            ASSERT(Request);

            Request->Srb->InternalStatus = STATUS_CANCELLED;
            Request->SrbStatus = SRB_STATUS_ABORTED;

            AtaReqCompleteRequest(DevExt, Request);
        }
    }
}

static
VOID
AtaAhciPortHandleFatalError(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ ULONG InterruptStatus,
    _In_ ULONG CurrentCommandMask)
{
    DevExt->WorkerData.State = PORT_STATE_RECOVERING;
    DevExt->WorkerData.InterruptStatus = InterruptStatus;
    DevExt->WorkerData.CurrentSlotMask = CurrentCommandMask;
    DevExt->WorkerData.Flags = (DevExt->ActiveQueuedSlotsBitmap != 0) ? WORKER_FLAG_NCQ : 0;

    AtaAhciPortSaveCommands(DevExt, CurrentCommandMask);
    AtaAhciPortWorker(NULL, DevExt);
}

VOID
NTAPI
AtaAhciPortDpc(
    _In_ PKDPC Dpc,
    _In_opt_ PVOID DeferredContext,
    _In_opt_ PVOID SystemArgument1,
    _In_opt_ PVOID SystemArgument2)
{
    PATAPORT_DEVICE_EXTENSION DevExt = DeferredContext;
    ULONG i, InterruptStatus, CommandsIssued, CommandsCompleted;
    ULONG CurrentCommandMask;

    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(SystemArgument1);
    UNREFERENCED_PARAMETER(SystemArgument2);

    /* Clear all pending events */
    InterruptStatus = AHCI_PORT_READ(DevExt->IoBase, PxInterruptStatus);
    AHCI_PORT_WRITE(DevExt->IoBase, PxInterruptStatus, InterruptStatus);

    KeAcquireSpinLockAtDpcLevel(&DevExt->DeviceLock);

    /* Clear interface errors */
    AHCI_PORT_WRITE(DevExt->IoBase, PxSataError, AHCI_PORT_READ(DevExt->IoBase, PxSataError));

    CommandsIssued = AHCI_PORT_READ(DevExt->IoBase,
                                    (DevExt->ActiveQueuedSlotsBitmap != 0)
                                    ? PxSataActive : PxCommandIssue);
    CommandsCompleted = ~CommandsIssued & DevExt->ActiveSlotsBitmap;

    if ((DevExt->ActiveSlotsBitmap | DevExt->ActiveQueuedSlotsBitmap) == 0)
        ASSERT(CommandsIssued == 0);

    /* Fatal errors */
    if (InterruptStatus & AHCI_PXIRQ_FATAL_ERROR)
    {
        ULONG CmdStatus;

        /* Clear the ST bit. This also clears PxCI, PxSACT, and PxCMD.CCS */
        CmdStatus = AHCI_PORT_READ(DevExt->IoBase, PxCmdStatus);
        CmdStatus &= ~AHCI_PXCMD_ST;
        AHCI_PORT_WRITE(DevExt->IoBase, PxCmdStatus, CmdStatus);

        /* Stop the Srb processing */
        KeAcquireSpinLockAtDpcLevel(&DevExt->QueueLock);
        DevExt->QueueFlags |= QUEUE_FLAG_FROZEN_PAUSED;
        KeReleaseSpinLockFromDpcLevel(&DevExt->QueueLock);

        /* Don't issue any new commands to the HBA */
        DevExt->PortFlags &= ~PORT_ACTIVE;

        CurrentCommandMask = 0;

        /* Device errors (non-queued commands) */
        if ((InterruptStatus & AHCI_PXIRQ_TFES) && (DevExt->ActiveQueuedSlotsBitmap == 0))
        {
            if (IsPowerOfTwo(DevExt->ActiveSlotsBitmap))
            {
                /* We have exactly one slot is outstanding */
                CurrentCommandMask = DevExt->ActiveSlotsBitmap;
                CommandsCompleted &= ~CurrentCommandMask;
            }
            else
            {
                /*
                 * Indicates that the TFES bit is spurious *or* the exact slot is not known,
                 * so we need to re-issue the outstanding commands one by one.
                 *
                 * NOTE: QEMU does not implement PxCMD.CCS properly,
                 * we cannot rely on the values of PxCMD.CCS as those are undefined.
                 */
            }
        }
    }

    /* Stop all timers tied to the completed slots */
    DevExt->ActiveSlotsBitmap &= ~CommandsCompleted;
    DevExt->ActiveQueuedSlotsBitmap &= ~CommandsCompleted;

    KeReleaseSpinLockFromDpcLevel(&DevExt->DeviceLock);

    /* Complete processed commands */
    for (i = 0; i < AHCI_MAX_COMMAND_SLOTS; ++i)
    {
        PATA_DEVICE_REQUEST Request;
        ULONG SrbStatus;

        if (!(CommandsCompleted & (1 << i)))
            continue;

        Request = DevExt->Slots[i];
        ASSERT(Request);
        ASSERT(Request->Slot == i);

        SrbStatus = SRB_STATUS_SUCCESS;

        if (!(Request->Flags & REQUEST_FLAG_NCQ) &&
            Request->Flags & (REQUEST_FLAG_DATA_IN | REQUEST_FLAG_DATA_OUT))
        {
            PAHCI_COMMAND_HEADER CommandHeader;

            CommandHeader = &DevExt->PortData->CommandList->CommandHeader[i];

            /* This indicates a residual underrun */
            if ((CommandHeader->PrdByteCount < Request->DataTransferLength) &&
                (Request->InternalState != REQUEST_STATE_RECOVERY))
            {
                Request->DataTransferLength = CommandHeader->PrdByteCount;
                SrbStatus = SRB_STATUS_DATA_OVERRUN;
            }
        }
        Request->SrbStatus = SrbStatus;

        AtaReqCompleteRequest(DevExt, Request);
    }

    /* Perform recovery actions */
    if (InterruptStatus & AHCI_PXIRQ_FATAL_ERROR)
    {
        AtaAhciPortHandleFatalError(DevExt, InterruptStatus, CurrentCommandMask);
        return;
    }

    /* Re-enable interrupts */
    AHCI_PORT_WRITE(DevExt->IoBase, PxInterruptEnable, AHCI_PORT_INTERRUPT_MASK);
}

BOOLEAN
NTAPI
AtaHbaIsr(
    _In_ PKINTERRUPT Interrupt,
    _In_ PVOID Context)
{
    PATAPORT_CHANNEL_EXTENSION ChanExt = Context;
    ULONG i, InterruptStatus, InterruptBitmap;

    InterruptStatus = AHCI_HBA_READ(ChanExt->IoBase, HbaInterruptStatus);
    if (InterruptStatus == 0)
        return FALSE;

    InterruptBitmap = InterruptStatus & ChanExt->DeviceBitmap;

    for (i = 0; i < AHCI_MAX_PORTS; ++i)
    {
        PULONG IoBase;

        if (!(InterruptBitmap & (1 << i)))
            continue;

        IoBase = AHCI_PORT_BASE(ChanExt->IoBase, i);

        /* Disable further interrupts */
        AHCI_PORT_WRITE(IoBase, PxInterruptEnable, 0);

        KeInsertQueueDpc(&ChanExt->PortData[i].Dpc, NULL, NULL);
    }

    AHCI_HBA_WRITE(ChanExt->IoBase, HbaInterruptStatus, InterruptStatus);

    return TRUE;
}

static
BOOLEAN
AtaAhciPortPollRegister(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ AHCI_PORT_REGISTER Register,
    _In_ ULONG Mask,
    _In_ ULONG Value,
    _In_ ULONG TimeoutMilliseconds)
{
    PULONG IoBase = DevExt->IoBase;
    ULONG i, Data;

    /* Do a quick check first */
    for (i = 0; i < 10; ++i)
    {
        Data = AHCI_PORT_READ(IoBase, Register);

        if ((Data & Mask) == Value)
            return TRUE;

        KeStallExecutionProcessor(10);
    }

    /* Retry after the time interval */
    if (KeGetCurrentIrql() < DISPATCH_LEVEL)
    {
        KTIMER Timer;
        LARGE_INTEGER DueTime;

        /* 10 ms */
        KeInitializeTimer(&Timer);
        DueTime.QuadPart = UInt32x32To64(10, -10000);

        for (i = 0; i < TimeoutMilliseconds; i += 10)
        {
            Data = AHCI_PORT_READ(IoBase, Register);

            if ((Data & Mask) == Value)
                return TRUE;

            if (DevExt->WorkerData.PortWorkerStopped)
                return FALSE;

            KeSetTimer(&Timer, DueTime, NULL);
            KeWaitForSingleObject(&Timer, Executive, KernelMode, FALSE, NULL);
        }
    }
    else
    {
        for (i = 0; i < TimeoutMilliseconds; i += 1)
        {
            Data = AHCI_PORT_READ(IoBase, Register);

            if ((Data & Mask) == Value)
                return TRUE;

            if (DevExt->WorkerData.PortWorkerStopped)
                return FALSE;

            KeStallExecutionProcessor(1000);
        }
    }

    return FALSE;
}

static
BOOLEAN
AtaAhciPortWaitForStopCommandEngine(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt)
{
    return AtaAhciPortPollRegister(DevExt,
                                   PxCmdStatus,
                                   AHCI_PXCMD_CR,
                                   0,
                                   AHCI_CR_STOP_DELAY);
}

static
BOOLEAN
AtaAhciPortStartCommandEngine(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt)
{
    ULONG CmdStatus;

    CmdStatus = AHCI_PORT_READ(DevExt->IoBase, PxCmdStatus);
    CmdStatus |= AHCI_PXCMD_ST;
    AHCI_PORT_WRITE(DevExt->IoBase, PxCmdStatus, CmdStatus);

    return AtaAhciPortPollRegister(DevExt,
                                   PxCmdStatus,
                                   AHCI_PXCMD_CR,
                                   AHCI_PXCMD_CR,
                                   AHCI_CR_START_DELAY);
}

static
VOID
AtaAhciPortWorkerCompleteRequest(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ PATA_DEVICE_REQUEST Request,
    _In_ ULONG SrbStatus)

{
    KIRQL OldLevel;

    Request->SrbStatus = SrbStatus;

    KeRaiseIrql(DISPATCH_LEVEL, &OldLevel);
    AtaReqCompleteRequest(DevExt, Request);
    KeLowerIrql(OldLevel);
}

static
VOID
AtaAhciPortWorkerResumeCommands(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt)
{
    KIRQL OldLevel;

    KeRaiseIrql(DISPATCH_LEVEL, &OldLevel);
    AtaAhciPortResumeCommands(DevExt);
    KeLowerIrql(OldLevel);
}

static
BOOLEAN
AhciPortHandleTaskFileError(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ ULONG TaskFileData)
{
    PATA_DEVICE_REQUEST Request;
    ULONG SlotsBitmap, Slot;
    KIRQL OldLevel;

    Request = NULL;

    KeAcquireSpinLock(&DevExt->DeviceLock, &OldLevel);

    if (DevExt->WorkerData.Flags & WORKER_FLAG_NCQ)
    {
        /* Try to locate a slot to be used for the READ LOG EXT command */
        SlotsBitmap = DevExt->PausedSlotsBitmap;
    }
    else
    {
        /* Get the slot that caused the error */
        SlotsBitmap = DevExt->WorkerData.CurrentSlotMask;
    }

    while (_BitScanForward(&Slot, SlotsBitmap))
    {
        DevExt->PausedSlotsBitmap &= ~(1 << Slot);

        Request = DevExt->Slots[Slot];
        ASSERT(Request);

        /* We're already canceled */
        if (!IoSetCancelRoutine(Request->Irp, NULL))
            continue;

        break;
    }

    KeReleaseSpinLock(&DevExt->DeviceLock, OldLevel);

    if (!Request)
        return TRUE;

    if (Request->InternalState == REQUEST_STATE_RECOVERY)
    {
        /* REQUEST SENSE or READ LOG EXT issued and failed due to task file error */
        ERR("Recovery command failure\n");
    }
    else
    {
        Request->Status = (TaskFileData & AHCI_PXTFD_STATUS_MASK);
        Request->Error = (TaskFileData & AHCI_PXTFD_ERROR_MASK) >> AHCI_PXTFD_ERROR_SHIFT;

        Request->InternalState = REQUEST_STATE_NEED_RECOVERY;
    }

    AtaAhciPortWorkerCompleteRequest(DevExt, Request, SRB_STATUS_ERROR);

    return FALSE;
}

static
BOOLEAN
AhciPortProcessErrorRecovery(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt)
{
    ULONG TaskFileData;
    BOOLEAN DoResumeIo;

    AtaAhciPortWaitForStopCommandEngine(DevExt);

    AHCI_PORT_WRITE(DevExt->IoBase, PxSataError, 0xFFFFFFFF);

    TaskFileData = AHCI_PORT_READ(DevExt->IoBase, PxTaskFileData);

    AtaAhciPortStartCommandEngine(DevExt);

    if (DevExt->WorkerData.InterruptStatus & AHCI_PXIRQ_TFES)
        DoResumeIo = AhciPortHandleTaskFileError(DevExt, TaskFileData);
    else
        DoResumeIo = TRUE;

    /* We delay all pending commands until recovery command completion is indicated */
    if (DoResumeIo)
        AtaAhciPortWorkerResumeCommands(DevExt);

    /* Re-enable interrupts */
    AHCI_PORT_WRITE(DevExt->IoBase, PxInterruptEnable, AHCI_PORT_INTERRUPT_MASK);
    return FALSE;
}

VOID
NTAPI
AtaAhciPortWorker(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_opt_ PVOID Context)
{
    PATAPORT_DEVICE_EXTENSION DevExt = Context;
    BOOLEAN DoProcessAgain;

Again:
    DoProcessAgain = TRUE;

    switch (DevExt->WorkerData.State)
    {
        case PORT_STATE_RECOVERING:
            DoProcessAgain = AhciPortProcessErrorRecovery(DevExt);
            break;

        default:
            ASSERT(FALSE);
            UNREACHABLE;
    }

    if (DoProcessAgain && !DevExt->WorkerData.PortWorkerStopped)
    {
        /* Fire the worker to avoid doing too much work at DPC level */
        if (!DeviceObject)
        {
            IoQueueWorkItem(DevExt->WorkItem, AtaAhciPortWorker, DelayedWorkQueue, DevExt);
            return;
        }

        goto Again;
    }
}

static
VOID
AtaAhciBuildPacketCommandFis(
    _In_ PAHCI_FIS_HOST_TO_DEVICE Fis,
    _In_ PATA_DEVICE_REQUEST Request)
{
    Fis->Command = IDE_COMMAND_ATAPI_PACKET;

    if (Request->Flags & REQUEST_FLAG_DMA_ENABLED)
    {
        /* DMA transfer */
        if ((Request->Flags & DEVICE_NEED_DMA_DIRECTION) &&
            (Request->Flags & REQUEST_FLAG_DATA_IN))
        {
            Fis->Features = IDE_FEATURE_DMA | IDE_FEATURE_DMADIR;
        }
        else
        {
            Fis->Features = IDE_FEATURE_DMA;
        }
    }
    else
    {
        USHORT ByteCount;

        /* PIO transfer */
        ByteCount = min(Request->DataTransferLength, 0xFFFF - 1);
        Fis->LbaMid = (UCHAR)ByteCount;
        Fis->LbaHigh = ByteCount >> 8;
    }
}

static
VOID
AhciBuildCommandFis(
    _In_ PAHCI_FIS_HOST_TO_DEVICE Fis,
    _In_ PATA_DEVICE_REQUEST Request)
{
    Fis->Command = Request->TaskFile.Command;
    Fis->Features = Request->TaskFile.Feature;
    Fis->LbaLow = Request->TaskFile.LowLba;
    Fis->LbaMid = Request->TaskFile.MidLba;
    Fis->LbaHigh = Request->TaskFile.HighLba;
    Fis->SectorCount = Request->TaskFile.SectorCount;

    if (Request->Flags & REQUEST_FLAG_LBA48)
    {
        Fis->LbaLowEx = Request->TaskFile.LowLbaEx;
        Fis->LbaMidEx = Request->TaskFile.MidLbaEx;
        Fis->LbaHighEx = Request->TaskFile.HighLbaEx;
        Fis->FeaturesEx = Request->TaskFile.FeatureEx;
        Fis->SectorCountEx = Request->TaskFile.SectorCountEx;

        if (Request->Flags & REQUEST_FLAG_NCQ)
        {
            /* Unique queue tag */
            Fis->SectorCount |= Request->Slot << 3;
        }
    }

    if (Request->Flags & REQUEST_FLAG_SET_ICC_FIELD)
        Fis->Icc = Request->TaskFile.Icc;

    if (Request->Flags & REQUEST_FLAG_SET_AUXILIARY_FIELD)
        Fis->Auxiliary = Request->TaskFile.Auxiliary;
}

VOID
AtaAhciExecuteCommand(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ PATA_DEVICE_REQUEST Request)
{
    PATAPORT_AHCI_PORT_DATA PortData = DevExt->PortData;
    PATAPORT_AHCI_SLOT_DATA SlotData = &PortData->Slot[Request->Slot];
    PAHCI_FIS_HOST_TO_DEVICE Fis;
    PAHCI_COMMAND_HEADER CommandHeader;
    ULONG Control;

    Control = sizeof(*Fis) / sizeof(ULONG);
    if (Request->Flags & (REQUEST_FLAG_DATA_IN | REQUEST_FLAG_DATA_OUT))
        Control |= Request->SgList->NumberOfElements << AHCI_COMMAND_HEADER_PRDT_LENGTH_SHIFT;
    if (Request->Flags & REQUEST_FLAG_DATA_OUT)
        Control |= AHCI_COMMAND_HEADER_WRITE;

    Fis = &SlotData->CommandTable->HostToDeviceFis;
    RtlZeroMemory(Fis, sizeof(*Fis));

    Fis->Type = AHCI_FIS_REGISTER_HOST_TO_DEVICE;
    Fis->Flags = UPDATE_COMMAND;
    if (Request->Flags & REQUEST_FLAG_SET_DEVICE_REGISTER)
        Fis->Device = Request->TaskFile.DriveSelect;
    else
        Fis->Device = IDE_DRIVE_SELECT;
    Fis->Control = 0x08;

    if (Request->Flags & REQUEST_FLAG_PACKET_COMMAND)
    {
        AtaAhciBuildPacketCommandFis(Fis, Request);

        RtlCopyMemory(SlotData->CommandTable->AtapiCommand, Request->Cdb, DevExt->CdbSize);

        Control |= AHCI_COMMAND_HEADER_ATAPI;
    }
    else
    {
        AhciBuildCommandFis(Fis, Request);
    }

    CommandHeader = &PortData->CommandList->CommandHeader[Request->Slot];
    CommandHeader->Control = Control;
    CommandHeader->PrdByteCount = 0;
    CommandHeader->CommandTableBaseLow = (ULONG)SlotData->CommandTablePhys;
    CommandHeader->CommandTableBaseHigh = (ULONG)(SlotData->CommandTablePhys >> 32);

    KeAcquireSpinLockAtDpcLevel(&DevExt->DeviceLock);

    if ((DevExt->PortFlags & PORT_ACTIVE) ||
        (Request->Flags & REQUEST_BYPASS_ACTIVE_QUEUE))
    {
        AtaAhciPortStartDma(DevExt, Request, Request->Slot);
    }
    else
    {
        DevExt->PreparedSlotsBitmap |= 1 << Request->Slot;
    }

    KeReleaseSpinLockFromDpcLevel(&DevExt->DeviceLock);
}

VOID
NTAPI
AtaAhciPreparePrdTable(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIRP Irp,
    _In_ PSCATTER_GATHER_LIST SgList,
    _In_ PVOID Context)
{
    PATA_DEVICE_REQUEST Request = Context;
    PATAPORT_AHCI_PORT_DATA PortData = Request->DevExt->PortData;
    PAHCI_PRD_TABLE_ENTRY PrdTableEntry;
    ULONG i;

    UNREFERENCED_PARAMETER(DeviceObject);
    UNREFERENCED_PARAMETER(Irp);

    ASSERT(SgList->NumberOfElements > 0);

    PrdTableEntry = PortData->Slot[Request->Slot].CommandTable->PrdTable;

    for (i = 0; i < SgList->NumberOfElements; ++i)
    {
        ASSERT(SgList->Elements[i].Length != 0);
        ASSERT(SgList->Elements[i].Length < AHCI_MAX_PRD_LENGTH);

        PrdTableEntry->DataBaseLow = SgList->Elements[i].Address.LowPart;
        PrdTableEntry->DataBaseHigh = SgList->Elements[i].Address.HighPart;
        PrdTableEntry->ByteCount = SgList->Elements[i].Length - 1;

        ++PrdTableEntry;
    }

    /* Enable IRQ on last entry */
    --PrdTableEntry;
    PrdTableEntry->ByteCount |= AHCI_PRD_INTERRUPT_ON_COMPLETION;

    Request->SgList = SgList;

    AtaAhciExecuteCommand(Request->DevExt, Request);
}

static
BOOLEAN
AhciPortWaitPhy(
    _In_ PULONG IoBase)
{
    ULONG i, SataStatus, SataControl;

    SataControl = AHCI_PORT_READ(IoBase, PxSataControl);
    SataControl &= AHCI_PXCTL_DET_MASK;
    SataControl |= AHCI_PXCTL_IPM_DISABLE_ALL | AHCI_PXCTL_DET_IDLE;
    AHCI_PORT_WRITE(IoBase, PxSataControl, SataControl);

    for (i = 0; i < 2000; ++i)
    {
        SataStatus = AHCI_PORT_READ(IoBase, PxSataStatus);
        switch (SataStatus & AHCI_PXSSTS_DET_MASK)
        {
            case AHCI_PXSSTS_DET_DEVICE_NO_PHY:
            case AHCI_PXSSTS_DET_DEVICE_PHY:
                return TRUE;

            default:
                break;
        }

        KeStallExecutionProcessor(10);

        AHCI_PORT_WRITE(IoBase, PxSataError, 0xFFFFFFFF);
    }

    return FALSE;
}

static
BOOLEAN
AhciPortWaitDrive(
    _In_ PULONG IoBase)
{
    ULONG i, TaskFileData;

    for (i = 0; i < 100000; ++i)
    {
        TaskFileData = AHCI_PORT_READ(IoBase, PxSataStatus);

        if (!(TaskFileData & (AHCI_PXTFD_STATUS_BUSY | AHCI_PXTFD_STATUS_DRQ)))
            return TRUE;

        KeStallExecutionProcessor(10);

        AHCI_PORT_WRITE(IoBase, PxSataError, 0xFFFFFFFF);
    }

    return FALSE;
}

ULONG
AhciPortEnumerate(
    _In_ PATAPORT_CHANNEL_EXTENSION ChanExt,
    _In_ ULONG PortNumber)
{
    PULONG IoBase = AHCI_PORT_BASE(ChanExt->IoBase, PortNumber);
    ULONG i, Signature, CmdStatus;

    CmdStatus = AHCI_PORT_READ(IoBase, PxCmdStatus);

    /* Already started */
    if (CmdStatus & AHCI_PXCMD_ST)
        goto ReturnSignature;

    if (ChanExt->AhciCapabilities & AHCI_CAP_SSS)
    {
        CmdStatus |= AHCI_PXCMD_SUD;
    }
    if (CmdStatus & AHCI_PXCMD_CPD)
    {
        CmdStatus |= AHCI_PXCMD_POD;
    }
    CmdStatus &= ~(AHCI_PXCMD_ICC_MASK | AHCI_PXCMD_ALPE);
    CmdStatus |= AHCI_PXCMD_ICC_ACTIVE;
    AHCI_PORT_WRITE(IoBase, PxCmdStatus, CmdStatus);

    CmdStatus = AHCI_PORT_READ(IoBase, PxCmdStatus);
    CmdStatus |= AHCI_PXCMD_FRE;
    AHCI_PORT_WRITE(IoBase, PxCmdStatus, CmdStatus);

    for (i = 0; i < 50000; ++i)
    {
        CmdStatus = AHCI_PORT_READ(IoBase, PxCmdStatus);

        if (CmdStatus & AHCI_PXCMD_FRE)
            break;

        KeStallExecutionProcessor(10);
    }

    if (!AhciPortWaitPhy(IoBase))
    {
        INFO("Unable to detect device\n");
        return AHCI_PXSIG_INVALID;
    }

    if (!AhciPortWaitDrive(IoBase))
    {
        INFO("No device present\n");
        return AHCI_PXSIG_INVALID;
    }

    CmdStatus = AHCI_PORT_READ(IoBase, PxCmdStatus);
    CmdStatus |= AHCI_PXCMD_ST;
    AHCI_PORT_WRITE(IoBase, PxCmdStatus, CmdStatus);

    for (i = 0; i < 50000; ++i)
    {
        CmdStatus = AHCI_PORT_READ(IoBase, PxCmdStatus);

        if (CmdStatus & AHCI_PXCMD_CR)
            break;

        KeStallExecutionProcessor(10);
    }

    KeStallExecutionProcessor(1000);

ReturnSignature:
    Signature = AHCI_PORT_READ(IoBase, PxSignature);

    if (Signature != 0xFFFFFFFF)
        AHCI_PORT_WRITE(IoBase, PxInterruptEnable, AHCI_PORT_INTERRUPT_MASK);

    return Signature;
}

CODE_SEG("PAGE")
VOID
AtaFdoFreePortMemory(
    _In_ PATAPORT_CHANNEL_EXTENSION ChanExt)
{
#if 0 // TODO: check this later
    PATAPORT_AHCI_PORT_INFO PortInfo;
    PDMA_OPERATIONS DmaOperations;
    ULONG i;

    PAGED_CODE();

    if (ChanExt->PortData)
    {
        ExFreePoolWithTag(ChanExt->PortData, ATAPORT_TAG);
        ChanExt->PortData = NULL;
    }

    if (!ChanExt->AdapterObject)
        return;

    PortInfo = ChanExt->PortInfo;
    if (!PortInfo)
        return;

    DmaOperations = ChanExt->AdapterObject->DmaOperations;

    for (i = 0; i < AHCI_MAX_COMMAND_SLOTS; ++i)
    {
        if (PortInfo->CommandTableSize[i] == 0)
            continue;

        DmaOperations->FreeCommonBuffer(ChanExt->AdapterObject,
                                        PortInfo->CommandTableSize[i],
                                        PortInfo->CommandTablePhysOriginal[i],
                                        PortInfo->CommandTableOriginal,
                                        FALSE);
        PortInfo->CommandTableSize[i] = 0;
    }

    if (PortInfo->CommandListSize != 0)
    {
        DmaOperations->FreeCommonBuffer(ChanExt->AdapterObject,
                                        PortInfo->CommandListSize,
                                        PortInfo->CommandListPhysOriginal,
                                        PortInfo->CommandListOriginal,
                                        FALSE);

        PortInfo->CommandListSize = 0;
    }

    if (PortInfo->ReceivedFisSize != 0)
    {
        DmaOperations->FreeCommonBuffer(ChanExt->AdapterObject,
                                        PortInfo->ReceivedFisSize,
                                        PortInfo->ReceivedFisPhysOriginal,
                                        PortInfo->ReceivedFisOriginal,
                                        FALSE);

        PortInfo->ReceivedFisSize = 0;
    }

    ChanExt->PortInfo = NULL;
#endif
}

static
CODE_SEG("PAGE")
BOOLEAN
AhciPortAllocateMemory(
    _In_ PATAPORT_CHANNEL_EXTENSION ChanExt,
    _In_ PATAPORT_AHCI_PORT_DATA PortData,
    _In_ PATAPORT_AHCI_PORT_INFO PortInfo)
{
    PDMA_OPERATIONS DmaOperations;
    ULONG i, j, SlotNumber, CommandSlots, Length;
    ULONG CommandListSize, CommandTableLength, CommandTablesPerPage;
    PVOID Buffer;
    ULONG_PTR BufferVa;
    ULONG64 BufferPa;
    PHYSICAL_ADDRESS PhysicalAddress;

    PAGED_CODE();

    DmaOperations = ChanExt->AdapterObject->DmaOperations;

    CommandSlots = ((ChanExt->AhciCapabilities & AHCI_CAP_NCS) >> 8) + 1;

    CommandTableLength = FIELD_OFFSET(AHCI_COMMAND_TABLE,
                                      PrdTable[ChanExt->MapRegisterCount]);

    ASSERT(ChanExt->MapRegisterCount != 0 &&
           ChanExt->MapRegisterCount <= AHCI_MAX_PRDT_ENTRIES);

    /*
     * See ATA_MAX_TRANSFER_LENGTH, currently the MapRegisterCount is restricted to
     * a maximum of (0x20000 / PAGE_SIZE) + 1 = 33 pages.
     * Each command table will require 128 + 16 * 33 + (128 - 1) = 783 bytes of shared memory.
     */
    ASSERT(PAGE_SIZE > (CommandTableLength + (AHCI_COMMAND_TABLE_ALIGNMENT - 1)));

    /* Allocate one-page chunks to avoid having a large chunk of contiguous memory */
    CommandTablesPerPage = PAGE_SIZE / (CommandTableLength + (AHCI_COMMAND_TABLE_ALIGNMENT - 1));

    /* Allocate shared memory for the command tables */
    SlotNumber = 0;
    i = CommandSlots;
    while (i > 0)
    {
        ULONG Count, BlockSize;

        Count = min(i, CommandTablesPerPage);
        BlockSize = (CommandTableLength + (AHCI_COMMAND_TABLE_ALIGNMENT - 1)) * Count;

        Buffer = DmaOperations->AllocateCommonBuffer(ChanExt->AdapterObject,
                                                     BlockSize,
                                                     &PhysicalAddress,
                                                     FALSE);
        if (!Buffer)
            return FALSE;

        RtlZeroMemory(Buffer, BlockSize);

        PortInfo->CommandTableOriginal[SlotNumber] = Buffer;
        PortInfo->CommandTablePhysOriginal[SlotNumber].QuadPart = PhysicalAddress.QuadPart;
        PortInfo->CommandTableSize[SlotNumber] = BlockSize;

        BufferVa = (ULONG_PTR)Buffer;
        BufferPa = PhysicalAddress.QuadPart;

        /* Split the allocation into command tables */
        for (j = 0; j < Count; ++j)
        {
            BufferVa = ALIGN_UP_BY(BufferVa, AHCI_COMMAND_TABLE_ALIGNMENT);
            BufferPa = ALIGN_UP_BY(BufferPa, AHCI_COMMAND_TABLE_ALIGNMENT);

            /* Alignment requirement */
            ASSERT(BufferPa % AHCI_COMMAND_TABLE_ALIGNMENT == 0);

            /* 32-bit DMA */
            if (!(ChanExt->AhciCapabilities & AHCI_CAP_S64A))
            {
                ASSERT((ULONG)(BufferPa >> 32) == 0);
            }

            PortData->Slot[SlotNumber].CommandTable = (PVOID)BufferVa;
            PortData->Slot[SlotNumber].CommandTablePhys = BufferPa;

            ++SlotNumber;

            BufferVa += CommandTableLength;
            BufferPa += CommandTableLength;
        }

        i -= Count;
    }

    /* The maximum size of the command list is 1024 bytes */
    CommandListSize = FIELD_OFFSET(AHCI_COMMAND_LIST, CommandHeader[CommandSlots]);

    Length = CommandListSize + (AHCI_COMMAND_LIST_ALIGNMENT - 1);

    if (TRUE) // TODO !FBS
    {
        /* Add the receive area structure (256 bytes) */
        Length += sizeof(AHCI_RECEIVED_FIS);

        /* The command list is 1024-byte aligned, which saves us some bytes of allocation size */
        Length += ALIGN_UP_BY(CommandListSize, AHCI_RECEIVED_FIS_ALIGNMENT) - CommandListSize;
    }

    /* NCQ Command Error log buffer */
    Length += IDE_GP_LOG_SECTOR_SIZE;

    Buffer = DmaOperations->AllocateCommonBuffer(ChanExt->AdapterObject,
                                                 Length,
                                                 &PhysicalAddress,
                                                 FALSE);
    if (!Buffer)
        return FALSE;

    RtlZeroMemory(Buffer, Length);

    PortInfo->CommandListSize = Length;
    PortInfo->CommandListOriginal = Buffer;
    PortInfo->CommandListPhysOriginal.QuadPart = PhysicalAddress.QuadPart;

    BufferVa = (ULONG_PTR)Buffer;
    BufferPa = PhysicalAddress.QuadPart;

    /* Command list */
    BufferVa = ALIGN_UP_BY(BufferVa, AHCI_COMMAND_LIST_ALIGNMENT);
    BufferPa = ALIGN_UP_BY(BufferPa, AHCI_COMMAND_LIST_ALIGNMENT);
    PortData->CommandList = (PVOID)BufferVa;
    PortData->CommandListPhys = BufferPa;
    BufferVa += CommandListSize;
    BufferPa += CommandListSize;

    /* Alignment requirement */
    ASSERT((ULONG_PTR)PortData->CommandListPhys % AHCI_COMMAND_LIST_ALIGNMENT == 0);

    /* Received FIS */
    if (TRUE) // TODO !FBS
    {
        BufferVa = ALIGN_UP_BY(BufferVa, AHCI_RECEIVED_FIS_ALIGNMENT);
        BufferPa = ALIGN_UP_BY(BufferPa, AHCI_RECEIVED_FIS_ALIGNMENT);
        PortData->ReceivedFis = (PVOID)BufferVa;
        PortData->ReceivedFisPhys = BufferPa;
        BufferVa += sizeof(AHCI_RECEIVED_FIS);
        BufferPa += sizeof(AHCI_RECEIVED_FIS);

        /* Alignment requirement */
        ASSERT((ULONG_PTR)PortData->ReceivedFisPhys % AHCI_RECEIVED_FIS_ALIGNMENT == 0);
    }

    /* NCQ Command Error log buffer */
    PortData->LogBuffer = (PVOID)BufferVa;
    PortData->LogBufferPhys = BufferPa;

    if (FALSE) // TODO FBS
    {
        /* The FBS receive area is 4kB, optimize the most common case */
        if (AHCI_FBS_RECEIVE_AREA_SIZE == PAGE_SIZE)
        {
            /* Allocate a 4kB page which is also 4kB-aligned */
            Length = PAGE_SIZE;
        }
        else
        {
            /* Some other architectures (e.g. ia64) use a different page size */
            Length = AHCI_FBS_RECEIVE_AREA_SIZE + (AHCI_RECEIVED_FIS_FBS_ALIGNMENT - 1);
        }
        Buffer = DmaOperations->AllocateCommonBuffer(ChanExt->AdapterObject,
                                                     Length,
                                                     &PhysicalAddress,
                                                     FALSE);
        if (!Buffer)
            return FALSE;

        RtlZeroMemory(Buffer, Length);

        PortInfo->ReceivedFisSize = Length;
        PortInfo->ReceivedFisOriginal = Buffer;
        PortInfo->ReceivedFisPhysOriginal.QuadPart = PhysicalAddress.QuadPart;

        BufferVa = (ULONG_PTR)Buffer;
        BufferPa = PhysicalAddress.QuadPart;

        if (AHCI_FBS_RECEIVE_AREA_SIZE != PAGE_SIZE)
        {
            BufferVa = ALIGN_UP_BY(BufferVa, AHCI_RECEIVED_FIS_FBS_ALIGNMENT);
            BufferPa = ALIGN_UP_BY(BufferPa, AHCI_RECEIVED_FIS_FBS_ALIGNMENT);
        }
        PortData->ReceivedFis = (PVOID)BufferVa;
        PortData->ReceivedFisPhys = BufferPa;

        /* Alignment requirement */
        ASSERT(BufferPa % AHCI_RECEIVED_FIS_FBS_ALIGNMENT == 0);
    }

    /* 32-bit DMA */
    if (!(ChanExt->AhciCapabilities & AHCI_CAP_S64A))
    {
        ASSERT((ULONG)(PortData->CommandListPhys >> 32) == 0);
        ASSERT((ULONG)(PortData->ReceivedFisPhys >> 32) == 0);
    }

    return TRUE;
}

static
CODE_SEG("PAGE")
BOOLEAN
AhciPortIdle(
    _In_ PULONG PortRegisters)
{
    ULONG i, CmdStatus;

    PAGED_CODE();

    CmdStatus = AHCI_PORT_READ(PortRegisters, PxCmdStatus);

    /* Already in idle state */
    if (!(CmdStatus & (AHCI_PXCMD_ST | AHCI_PXCMD_CR | AHCI_PXCMD_FRE | AHCI_PXCMD_FR)))
        return TRUE;

    /* Stop the command list DMA engine */
    CmdStatus &= ~AHCI_PXCMD_ST;
    AHCI_PORT_WRITE(PortRegisters, PxCmdStatus, CmdStatus);

    for (i = 50000; i > 0; --i)
    {
        CmdStatus = AHCI_PORT_READ(PortRegisters, PxCmdStatus);
        if (!(CmdStatus & AHCI_PXCMD_CR))
            break;

        KeStallExecutionProcessor(10);
    }
    if (i == 0)
    {
        WARN("Failed to stop the command list DMA engine %08lx\n", CmdStatus);
        return FALSE;
    }

    if (!(CmdStatus & AHCI_PXCMD_FRE))
        return TRUE;

    /* Stop the FIS Receive DMA engine */
    CmdStatus &= ~AHCI_PXCMD_FRE;
    AHCI_PORT_WRITE(PortRegisters, PxCmdStatus, CmdStatus);

    for (i = 50000; i > 0; --i)
    {
        CmdStatus = AHCI_PORT_READ(PortRegisters, PxCmdStatus);
        if (!(CmdStatus & AHCI_PXCMD_FR))
            return TRUE;

        KeStallExecutionProcessor(10);
    }

    WARN("Failed to stop the FIS Receive DMA engine %08lx\n", CmdStatus);
    return TRUE;
}

static
CODE_SEG("PAGE")
BOOLEAN
AhciPortInit(
    _In_ PATAPORT_CHANNEL_EXTENSION ChanExt,
    _In_ ULONG PortNumber,
    _Inout_ PATAPORT_AHCI_PORT_DATA PortData,
    _Inout_ PATAPORT_AHCI_PORT_INFO PortInfo)
{
    PULONG IoBase = AHCI_PORT_BASE(ChanExt->IoBase, PortNumber);

    PAGED_CODE();

    TRACE("Initializing port #%u\n", PortNumber);

    /* Disable interrupts */
    AHCI_PORT_WRITE(IoBase, PxInterruptEnable, 0);

    /* Clear interrupts */
    AHCI_PORT_WRITE(IoBase, PxInterruptStatus, 0xFFFFFFFF);

    /* Clear the error register */
    AHCI_PORT_WRITE(IoBase, PxSataError, 0xFFFFFFFF);

    /* Put the DMA engine in idle state */
    if (!AhciPortIdle(IoBase))
        return FALSE;

    /* Allocate shared memory */
    if (!AhciPortAllocateMemory(ChanExt, PortData, PortInfo))
        return FALSE;

    /* Physical address of the allocated command list */
    AHCI_PORT_WRITE(IoBase, PxCommandListBaseLow, (ULONG)PortData->CommandListPhys);
    if (ChanExt->AhciCapabilities & AHCI_CAP_S64A)
    {
        AHCI_PORT_WRITE(IoBase, PxCommandListBaseHigh, (ULONG)(PortData->CommandListPhys >> 32));
    }

    /* Physical address of the allocated FIS receive area */
    AHCI_PORT_WRITE(IoBase, PxFisBaseLow, (ULONG)PortData->ReceivedFisPhys);
    if (ChanExt->AhciCapabilities & AHCI_CAP_S64A)
    {
        AHCI_PORT_WRITE(IoBase, PxFisBaseHigh, (ULONG)(PortData->ReceivedFisPhys >> 32));
    }

    return TRUE;
}

static
CODE_SEG("PAGE")
BOOLEAN
AtaFdoGetDmaAdapter(
    _In_ PATAPORT_CHANNEL_EXTENSION ChanExt)
{
    DEVICE_DESCRIPTION DeviceDescription = { 0 };

    PAGED_CODE();

    DeviceDescription.Version = DEVICE_DESCRIPTION_VERSION;
    DeviceDescription.Master = TRUE;
    DeviceDescription.ScatterGather = TRUE;
    DeviceDescription.Dma32BitAddresses = !(ChanExt->AhciCapabilities & AHCI_CAP_S64A);
    DeviceDescription.InterfaceType = PCIBus;
    DeviceDescription.MaximumLength = ATA_MAX_TRANSFER_LENGTH;

    ChanExt->AdapterObject = IoGetDmaAdapter(ChanExt->Ldo,
                                                      &DeviceDescription,
                                                      &ChanExt->MapRegisterCount);
    if (!ChanExt->AdapterObject)
        return FALSE;

    ChanExt->AdapterDeviceObject = ChanExt->Ldo;

    return TRUE;
}

CODE_SEG("PAGE")
NTSTATUS
AtaFdoAhciInit(
    _In_ PATAPORT_CHANNEL_EXTENSION ChanExt)
{
    ULONG i, GlobalControl, AhciVersion, NumberOfPorts;
    PATAPORT_AHCI_PORT_DATA PortData;
    PATAPORT_AHCI_PORT_INFO PortInfo;

    PAGED_CODE();

    TRACE("Initializing HBA [%04X:%04X]\n", ChanExt->DeviceID, ChanExt->VendorID);

    /* Set AE before accessing other AHCI registers */
    GlobalControl = AHCI_HBA_READ(ChanExt->IoBase, HbaGlobalControl);
    GlobalControl |= AHCI_GHC_AE;
    AHCI_HBA_WRITE(ChanExt->IoBase, HbaGlobalControl, GlobalControl);

    AhciVersion = AHCI_HBA_READ(ChanExt->IoBase, HbaAhciVersion);
    if (AhciVersion >= AHCI_VERSION_1_2)
    {
        ChanExt->AhciCapabilitiesEx =
            AHCI_HBA_READ(ChanExt->IoBase, HbaCapabilitiesEx);
    }

    ChanExt->AhciCapabilities = AHCI_HBA_READ(ChanExt->IoBase, HbaCapabilities);

    ChanExt->DeviceBitmap = AHCI_HBA_READ(ChanExt->IoBase, HbaPortBitmap);

    /* Reset the HBA */
    GlobalControl |= AHCI_GHC_HR;
    AHCI_HBA_WRITE(ChanExt->IoBase, HbaGlobalControl, GlobalControl);

    /* HBA reset may take up to 1 second */
    for (i = 100000; i > 0; --i)
    {
        GlobalControl = AHCI_HBA_READ(ChanExt->IoBase, HbaGlobalControl);
        if (!(GlobalControl & AHCI_GHC_HR))
            break;

        KeStallExecutionProcessor(10);
    }
    if (i == 0)
    {
        ERR("HBA reset failed\n");
        return STATUS_UNSUCCESSFUL;
    }

    /* Disable interrupts and re-enable AE */
    GlobalControl |= AHCI_GHC_AE;
    GlobalControl &= ~AHCI_GHC_IE;
    AHCI_HBA_WRITE(ChanExt->IoBase, HbaGlobalControl, GlobalControl);

    /* Clear interrupts */
    AHCI_HBA_WRITE(ChanExt->IoBase, HbaInterruptStatus, 0xFFFFFFFF);

    INFO("Ver %08lx, PI %08lx, CAP %08lx, CAP2 %08lx\n",
         AhciVersion,
         ChanExt->DeviceBitmap,
         ChanExt->AhciCapabilities,
         ChanExt->AhciCapabilitiesEx);

    NumberOfPorts = CountSetBits(ChanExt->DeviceBitmap);
    ASSERT(NumberOfPorts != 0);

    ChanExt->PortData = ExAllocatePoolZero(PagedPool,
                                           (sizeof(*PortData) + sizeof(*PortInfo)) * NumberOfPorts,
                                           ATAPORT_TAG);
    if (!ChanExt->PortData)
        return STATUS_INSUFFICIENT_RESOURCES;

    ChanExt->PortInfo = (PVOID)(ChanExt->PortData + NumberOfPorts);

    if (!AtaFdoGetDmaAdapter(ChanExt))
        return STATUS_INSUFFICIENT_RESOURCES;

    PortData = ChanExt->PortData;
    PortInfo = ChanExt->PortInfo;

    /* Initialize each port on the HBA */
    for (i = 0; i < AHCI_MAX_PORTS; ++i)
    {
        if (!(ChanExt->DeviceBitmap & (1 << i)))
            continue;

        if (!AhciPortInit(ChanExt, i, PortData, PortInfo))
        {
            WARN("Failed to initialize port %lu\n", i);

            AtaFdoFreePortMemory(ChanExt);

            ChanExt->DeviceBitmap &= ~(1 << i);
        }

        ++PortData;
        ++PortInfo;
    }

    return STATUS_SUCCESS;
}

static
VOID
AtaAhciPortTimeout(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ ULONG Slot)
{
    ERR("Command slot %lu (%08lx) timed out\n", Slot, 1 << Slot);
}

VOID
NTAPI
AtaAhciPortIoTimer(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_opt_ PVOID Context)
{
    PATAPORT_DEVICE_EXTENSION DevExt = Context;
    ULONG i;

    KeAcquireSpinLockAtDpcLevel(&DevExt->DeviceLock);

    for (i = 0; i < AHCI_MAX_COMMAND_SLOTS; ++i)
    {
        if (!(DevExt->ActiveSlotsBitmap & (1 << i)))
            continue;

        /*
         * Handle timeout of active command. This operation should carefully check
         * the current request state due to the asynchronous nature of AHCI.
         */
        if ((DevExt->TimerCount[i] > 0) && (--DevExt->TimerCount[i] == 0))
        {
            AtaAhciPortTimeout(DevExt, i);
        }
    }

    KeReleaseSpinLockFromDpcLevel(&DevExt->DeviceLock);
}
