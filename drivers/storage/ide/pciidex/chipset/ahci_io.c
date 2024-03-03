/*
 * PROJECT:     ReactOS ATA Bus Driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     AHCI I/O request handling
 * COPYRIGHT:   Copyright 2025 Dmitry Borisov <di.sean@protonmail.com>
 */

/* INCLUDES *******************************************************************/

#include "pciidex.h"

/* FUNCTIONS ******************************************************************/

static
VOID
AtaAhciBeginHostToDeviceFis(
    _In_ ATA_DEVICE_REQUEST* __restrict Request,
    _Out_ AHCI_FIS_HOST_TO_DEVICE* __restrict Fis)
{
    RtlZeroMemory(Fis, sizeof(*Fis));

    Fis->Type = AHCI_FIS_REGISTER_HOST_TO_DEVICE;
    Fis->Flags = UPDATE_COMMAND | DEV_NUMBER(Request->Device);

    if (Request->Flags & REQUEST_FLAG_SET_DEVICE_REGISTER)
        Fis->Device = Request->TaskFile.DriveSelect;
    else
        Fis->Device = IDE_DRIVE_SELECT;

    Fis->Control = IDE_DC_ALWAYS;
}

static
VOID
AtaAhciBuildPacketCommandFis(
    _In_ ATA_DEVICE_REQUEST* __restrict Request,
    _Out_ AHCI_FIS_HOST_TO_DEVICE* __restrict Fis)
{
    Fis->Command = IDE_COMMAND_ATAPI_PACKET;

    if (Request->Flags & REQUEST_FLAG_DMA)
    {
        /* DMA transfer */
        if ((Request->Device->TransportFlags & DEVICE_NEED_DMA_DIRECTION) &&
            (Request->Flags & REQUEST_FLAG_DATA_IN))
        {
            /* Some SATA-to-PATA bridges require the DMADIR bit to be set */
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
        ByteCount = min(Request->DataTransferLength, ATAPI_MAX_DRQ_DATA_BLOCK);
        Fis->LbaMid = (UCHAR)ByteCount;
        Fis->LbaHigh = ByteCount >> 8;
    }
}

static
VOID
AtaAhciBuildAtaCommandFis(
    _In_ ATA_DEVICE_REQUEST* __restrict Request,
    _Out_ AHCI_FIS_HOST_TO_DEVICE* __restrict Fis)
{
    PATA_TASKFILE TaskFile = &Request->TaskFile;

    Fis->Command = TaskFile->Command;
    Fis->Features = TaskFile->Feature;
    Fis->LbaLow = TaskFile->LowLba;
    Fis->LbaMid = TaskFile->MidLba;
    Fis->LbaHigh = TaskFile->HighLba;
    Fis->SectorCount = TaskFile->SectorCount;

    /* Unique queue tag */
    if (Request->Flags & REQUEST_FLAG_NCQ)
        Fis->SectorCount |= Request->Slot << 3;

    if (Request->Flags & REQUEST_FLAG_LBA48)
    {
        Fis->LbaLowEx = TaskFile->LowLbaEx;
        Fis->LbaMidEx = TaskFile->MidLbaEx;
        Fis->LbaHighEx = TaskFile->HighLbaEx;
        Fis->FeaturesEx = TaskFile->FeatureEx;
        Fis->SectorCountEx = TaskFile->SectorCountEx;
    }

    if (Request->Flags & REQUEST_FLAG_SET_ICC_FIELD)
        Fis->Icc = TaskFile->Icc;

    if (Request->Flags & REQUEST_FLAG_SET_AUXILIARY_FIELD)
        Fis->Auxiliary = TaskFile->Auxiliary;
}

/** Copy the CDB bytes (12 or 16 bytes) */
static
VOID
AtaAhciTransferACMDRegion(
    _In_ VOID* __restrict Destination,
    _In_ VOID* __restrict Source,
    _In_ ATA_IO_CONTEXT_COMMON* __restrict Device)
{
    /* Both addresses are 8-byte aligned */
#if defined(_WIN64)
    PULONG64 Dest = Destination, Src = Source;

    *Dest++ = *Src++;

    if ((Device->CdbSize & (sizeof(ULONG64) - 1)) == 0)
        *Dest = *Src;
    else
        *(PULONG)Dest = *(PULONG)Src;
#else
    PULONG Dest = Destination, Src = Source;

    *Dest++ = *Src++;
    *Dest++ = *Src++;
    *Dest++ = *Src++;

    if ((Device->CdbSize & (sizeof(ULONG64) - 1)) == 0)
        *Dest = *Src;
#endif
}
C_ASSERT(FIELD_OFFSET(ATA_DEVICE_REQUEST, Cdb) % sizeof(ULONG64) == 0);
C_ASSERT(FIELD_OFFSET(AHCI_COMMAND_TABLE, AtapiCommand) % sizeof(ULONG64) == 0);

BOOLEAN
AtaAhciStartIo(
    _In_ PVOID ChannelContext,
    _In_ PATA_DEVICE_REQUEST Request)
{
    PCHANNEL_DATA_AHCI ChanData = ChannelContext;
    PULONG IoBase = ChanData->IoBase;
    ULONG IssueSlot;

    IssueSlot = 1 << Request->Slot;

    ChanData->ActiveSlotsBitmap |= IssueSlot;

    if (Request->Flags & REQUEST_FLAG_NCQ)
    {
        ChanData->ActiveQueuedSlotsBitmap |= IssueSlot;
        AHCI_PORT_WRITE(IoBase, PxSataActive, IssueSlot);
    }

    if ((ChanData->ChanInfo & CHANNEL_FLAG_FBS_ENABLED) &&
        (ChanData->LastFbsDeviceNumber != DEV_NUMBER(Request->Device)))
    {
        ULONG Control;

        ChanData->LastFbsDeviceNumber = DEV_NUMBER(Request->Device);

        Control = AHCI_FBS_ENABLE;
        Control |= DEV_NUMBER(Request->Device) << AHCI_FBS_ISSUE_SHIFT;
        AHCI_PORT_WRITE(IoBase, PxFisSwitchingControl, Control);
    }

    AHCI_PORT_WRITE(IoBase, PxCommandIssue, IssueSlot);
    return FALSE;
}

VOID
AtaAhciPrepareIo(
    _In_ PVOID ChannelContext,
    _In_ PATA_DEVICE_REQUEST Request)
{
    PCHANNEL_DATA_AHCI ChanData = ChannelContext;
    PAHCI_COMMAND_TABLE CommandTable;
    PAHCI_COMMAND_HEADER CommandHeader;
    PAHCI_FIS_HOST_TO_DEVICE Fis;
    ULONG Control;

    Control = sizeof(*Fis) / sizeof(ULONG);
    Control |= DEV_NUMBER(Request->Device) << AHCI_COMMAND_HEADER_PMP_SHIFT;

    if (Request->Flags & (REQUEST_FLAG_DATA_IN | REQUEST_FLAG_DATA_OUT))
    {
        ASSERT(Request->Flags & REQUEST_FLAG_HAS_SG_LIST);

        Control |= Request->SgList->NumberOfElements << AHCI_COMMAND_HEADER_PRDT_LENGTH_SHIFT;

        if (Request->Flags & REQUEST_FLAG_DATA_OUT)
        {
            Control |= AHCI_COMMAND_HEADER_WRITE;
        }
    }

    CommandTable = ChanData->CommandTable[Request->Slot];

    Fis = &CommandTable->HostToDeviceFis;

    AtaAhciBeginHostToDeviceFis(Request, Fis);

    if (Request->Flags & REQUEST_FLAG_PACKET_COMMAND)
    {
        AtaAhciBuildPacketCommandFis(Request, Fis);
        AtaAhciTransferACMDRegion(CommandTable->AtapiCommand,
                                  Request->Cdb,
                                  Request->Device);

        Control |= AHCI_COMMAND_HEADER_ATAPI;
    }
    else
    {
        AtaAhciBuildAtaCommandFis(Request, Fis);
    }

    CommandHeader = &ChanData->CommandList->CommandHeader[Request->Slot];
    CommandHeader->Control = Control;
    CommandHeader->PrdByteCount = 0;

    KeFlushIoBuffers(Request->Mdl, !!(Request->Flags & REQUEST_FLAG_DATA_IN), TRUE);
}

VOID
AtaAhciPreparePrdTable(
    _In_ PVOID ChannelContext,
    _In_ PATA_DEVICE_REQUEST Request,
    _In_ SCATTER_GATHER_LIST* __restrict SgList)
{
    PCHANNEL_DATA_AHCI ChanData = ChannelContext;
    PAHCI_PRD_TABLE_ENTRY PrdTableEntry;
    ULONG i;
#if DBG
    BOOLEAN Is64BitDma = !!(ChanData->Controller->AhciCapabilities & AHCI_CAP_S64A);
#endif

    ASSUME(SgList->NumberOfElements > 0);
    ASSERT(SgList->NumberOfElements < AHCI_MAX_PRDT_ENTRIES);

    PrdTableEntry = ChanData->CommandTable[Request->Slot]->PrdTable;

    for (i = 0; i < SgList->NumberOfElements; ++i)
    {
        ASSERT(SgList->Elements[i].Length != 0);
        ASSERT(SgList->Elements[i].Length < AHCI_MAX_PRD_LENGTH);
        ASSERT((SgList->Elements[i].Length & ATA_MIN_BUFFER_ALIGNMENT) == 0);
        ASSERT(Is64BitDma || (SgList->Elements[i].Address.HighPart == 0));
        ASSERT(i < ChanData->MaximumPhysicalPages);

        PrdTableEntry->DataBaseLow = SgList->Elements[i].Address.LowPart;
        PrdTableEntry->DataBaseHigh = SgList->Elements[i].Address.HighPart;
        PrdTableEntry->ByteCount = SgList->Elements[i].Length - 1;

        ++PrdTableEntry;
    }

    /* Enable IRQ on last entry */
    --PrdTableEntry;
    PrdTableEntry->ByteCount |= AHCI_PRD_INTERRUPT_ON_COMPLETION;
}

BOOLEAN
AtaAhciAllocateSlot(
    _In_ PVOID ChannelContext,
    _In_ PATA_DEVICE_REQUEST Request,
    _In_ BOOLEAN Allocate)
{
    PCHANNEL_DATA_AHCI ChanData = ChannelContext;

    if (Allocate)
    {
        /*
         * We cannot issue commands to more than one device behind the Port Multiplier
         * that supports only command-based switching.
         */
        if ((ChanData->ChanInfo & CHANNEL_FLAG_IS_PMP) &&
            !(ChanData->ChanInfo & CHANNEL_FLAG_FBS_ENABLED))
        {
            if ((ChanData->LastPmpDeviceNumber != 0xFF) &&
                (ChanData->LastPmpDeviceNumber != DEV_NUMBER(Request->Device)))
            {
                return FALSE;
            }

            ChanData->LastPmpDeviceNumber = DEV_NUMBER(Request->Device);

            Request->Flags |= REQUEST_FLAG_DEVICE_EXCLUSIVE_ACCESS;
        }
    }
    else
    {
        if (Request->Flags & REQUEST_FLAG_DEVICE_EXCLUSIVE_ACCESS)
        {
            ChanData->LastPmpDeviceNumber = 0xFF;
        }
    }

    return TRUE;
}

static
VOID
AtaAhciPortCompleteCommands(
    _In_ PCHANNEL_DATA_AHCI ChanData,
    _In_ ULONG CommandsCompleted)
{
    ULONG Slot;

    while (_BitScanForward(&Slot, CommandsCompleted) != 0)
    {
        PATA_DEVICE_REQUEST Request;
        UCHAR SrbStatus;

        CommandsCompleted &= ~(1 << Slot);

        Request = ChanData->Slots[Slot];
        ASSERT(Request);
        ASSERT(Request->Slot == Slot);

        SrbStatus = SRB_STATUS_SUCCESS;

        if (!(Request->Flags & REQUEST_FLAG_NCQ) &&
            Request->Flags & (REQUEST_FLAG_DATA_IN | REQUEST_FLAG_DATA_OUT))
        {
            PAHCI_COMMAND_HEADER CommandHeader = &ChanData->CommandList->CommandHeader[Slot];

            /* This indicates a residual underrun */
            if (CommandHeader->PrdByteCount < Request->DataTransferLength)
            {
                Request->DataTransferLength = CommandHeader->PrdByteCount;
                SrbStatus = SRB_STATUS_DATA_OVERRUN;
            }
        }
        Request->SrbStatus = SrbStatus;

        /* Save the latest copy of the task file registers */
        if (Request->Flags & REQUEST_FLAG_SAVE_TASK_FILE)
        {
            AtaAhciSaveTaskFile(ChanData, Request, FALSE);
        }
    }
}

static
VOID
AtaAhciPortHandleInterrupt(
    _In_ PCHANNEL_DATA_AHCI ChanData)
{
    ULONG InterruptStatus, CommandsIssued, CommandsCompleted;
    AHCI_PORT_REGISTER CommandRegister;

    /* Clear all pending events */
    InterruptStatus = AHCI_PORT_READ(ChanData->IoBase, PxInterruptStatus);
    AHCI_PORT_WRITE(ChanData->IoBase, PxInterruptStatus, InterruptStatus);

    /* Clear interface errors */
    AHCI_PORT_WRITE(ChanData->IoBase, PxSataError,
                    AHCI_PORT_READ(ChanData->IoBase, PxSataError));

    /* Determine commands that have completed */
    if (ChanData->ActiveQueuedSlotsBitmap != 0)
        CommandRegister = PxSataActive;
    else
        CommandRegister = PxCommandIssue;
    CommandsIssued = AHCI_PORT_READ(ChanData->IoBase, CommandRegister);
    CommandsCompleted = ~CommandsIssued & ChanData->ActiveSlotsBitmap;

    /* Complete processed commands */
    if (CommandsCompleted != 0)
    {
        ChanData->ActiveSlotsBitmap &= ~CommandsCompleted;
        ChanData->ActiveQueuedSlotsBitmap &= ~CommandsCompleted;

        AtaAhciPortCompleteCommands(ChanData, CommandsCompleted);
        ChanData->PortNotification(AtaRequestComplete, ChanData->PortContext, CommandsCompleted);
    }

    /* Asynchronous notification received */
    if (InterruptStatus & AHCI_PXIRQ_SDBS)
    {
        ULONG Message = AHCI_PORT_READ(ChanData->IoBase, PxSataNotification);

        if (Message != 0)
        {
            AHCI_PORT_WRITE(ChanData->IoBase, PxSataNotification, Message);

            WARN("CH %lu: Notification %08lx arrived\n", ChanData->Channel, Message);

            ChanData->PortNotification(AtaAsyncNotificationDetected, ChanData->PortContext, 0x1);
        }
    }

    /* Handle various errors and link change events */
    if (InterruptStatus & AHCI_PXIRQ_PORT_STATUS)
    {
        AtaAhciHandlePortStateChange(ChanData, InterruptStatus);
    }

    if (InterruptStatus & AHCI_PXIRQ_FATAL_ERROR)
    {
        AtaAhciHandleFatalError(ChanData);
    }
}

BOOLEAN
NTAPI
AtaAhciHbaIsr(
    _In_ PKINTERRUPT Interrupt,
    _In_ PVOID Context)
{
    PATA_CONTROLLER Controller = Context;
    ULONG Port, InterruptStatus, PortInterruptBitmap;

    InterruptStatus = AHCI_HBA_READ(Controller->IoBase, HbaInterruptStatus);
    if (InterruptStatus == 0)
        return FALSE;

    PortInterruptBitmap = InterruptStatus & Controller->ChannelBitmap;
    while (_BitScanForward(&Port, PortInterruptBitmap) != 0)
    {
        PortInterruptBitmap &= ~(1 << Port);

        AtaAhciPortHandleInterrupt(Controller->Channels[Port]);
    }

    /* Clear pending HBA interrupts */
    AHCI_HBA_WRITE(Controller->IoBase, HbaInterruptStatus, InterruptStatus);

    return TRUE;
}
