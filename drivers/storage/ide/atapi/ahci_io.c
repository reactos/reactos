/*
 * PROJECT:     ReactOS ATA Port Driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     AHCI I/O request handling
 * COPYRIGHT:   Copyright 2024-2025 Dmitry Borisov <di.sean@protonmail.com>
 */

/* INCLUDES *******************************************************************/

#include "atapi.h"

/* FUNCTIONS ******************************************************************/

static
VOID
AtaAhciBeginHostToDeviceFis(
    _In_ ATA_DEVICE_REQUEST* __restrict Request,
    _Out_ AHCI_FIS_HOST_TO_DEVICE* __restrict Fis)
{
    RtlZeroMemory(Fis, sizeof(*Fis));

    Fis->Type = AHCI_FIS_REGISTER_HOST_TO_DEVICE;
    Fis->Flags = UPDATE_COMMAND | Request->Device->AtaScsiAddress.TargetId;

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
        if ((Request->Device->DeviceFlags & DEVICE_NEED_DMA_DIRECTION) &&
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
    _In_ ATAPORT_IO_CONTEXT* __restrict Device)
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
    _In_ PATAPORT_PORT_DATA PortData,
    _In_ PATA_DEVICE_REQUEST Request)
{
    PULONG IoBase = PortData->Ahci.IoBase;
    ULONG IssueSlot;

    IssueSlot = 1 << Request->Slot;

    if (Request->Flags & REQUEST_FLAG_NCQ)
    {
        PortData->ActiveQueuedSlotsBitmap |= IssueSlot;

        AHCI_PORT_WRITE(IoBase, PxSataActive, IssueSlot);
    }

    if (PortData->PortFlags & PORT_FLAG_FBS_ENABLED)
    {
        AHCI_PORT_WRITE(IoBase,
                        PxFisSwitchingControl,
                        (Request->Device->AtaScsiAddress.TargetId << AHCI_FBS_ISSUE_SHIFT) |
                        AHCI_FBS_ENABLE);
    }

    AHCI_PORT_WRITE(IoBase, PxCommandIssue, IssueSlot);

    return FALSE;
}

VOID
AtaAhciPrepareIo(
    _In_ PATAPORT_PORT_DATA PortData,
    _In_ PATA_DEVICE_REQUEST Request)
{
    PAHCI_COMMAND_TABLE CommandTable;
    PAHCI_COMMAND_HEADER CommandHeader;
    PAHCI_FIS_HOST_TO_DEVICE Fis;
    ULONG Control;

    Control = sizeof(*Fis) / sizeof(ULONG);
    Control |= Request->Device->AtaScsiAddress.TargetId << AHCI_COMMAND_HEADER_PMP_SHIFT;

    if (Request->Flags & (REQUEST_FLAG_DATA_IN | REQUEST_FLAG_DATA_OUT))
    {
        ASSERT(Request->Flags & REQUEST_FLAG_HAS_SG_LIST);

        Control |= Request->SgList->NumberOfElements << AHCI_COMMAND_HEADER_PRDT_LENGTH_SHIFT;

        if (Request->Flags & REQUEST_FLAG_DATA_OUT)
        {
            Control |= AHCI_COMMAND_HEADER_WRITE;
        }
    }

    CommandTable = PortData->Ahci.CommandTable[Request->Slot];

    Fis = &CommandTable->HostToDeviceFis;

    AtaAhciBeginHostToDeviceFis(Request, Fis);

    switch (Request->Flags & (REQUEST_FLAG_PACKET_COMMAND | REQUEST_FLAG_RST_COMMAND))
    {
        case REQUEST_FLAG_PACKET_COMMAND:
        {
            AtaAhciBuildPacketCommandFis(Request, Fis);
            AtaAhciTransferACMDRegion(CommandTable->AtapiCommand,
                                      Request->Cdb,
                                      Request->Device);

            Control |= AHCI_COMMAND_HEADER_ATAPI;
            break;
        }

        case REQUEST_FLAG_RST_COMMAND:
        {
            Fis->Flags &= ~UPDATE_COMMAND;
            Fis->Device = 0;

            if (Request->TaskFile.Command != 0)
            {
                Fis->Control |= IDE_DC_RESET_CONTROLLER;

                Control |= AHCI_COMMAND_HEADER_RESET | AHCI_COMMAND_HEADER_CLEAR_BUSY_UPON_OK;
            }
            break;
        }

        default:
        {
            AtaAhciBuildAtaCommandFis(Request, Fis);
            break;
        }
    }

    CommandHeader = &PortData->Ahci.CommandList->CommandHeader[Request->Slot];
    CommandHeader->Control = Control;
    CommandHeader->PrdByteCount = 0;

    KeFlushIoBuffers(Request->Mdl, !!(Request->Flags & REQUEST_FLAG_DATA_IN), TRUE);
}

VOID
AtaAhciPreparePrdTable(
    _In_ PATAPORT_PORT_DATA PortData,
    _In_ PATA_DEVICE_REQUEST Request,
    _In_ SCATTER_GATHER_LIST* __restrict SgList)
{
    PAHCI_PRD_TABLE_ENTRY PrdTableEntry;
    ULONG i;
#if DBG
    BOOLEAN Is64BitDma = !!(PortData->ChanExt->AhciCapabilities & AHCI_CAP_S64A);
#endif

    ASSUME(SgList->NumberOfElements > 0);
    ASSERT(SgList->NumberOfElements < AHCI_MAX_PRDT_ENTRIES);

    PrdTableEntry = PortData->Ahci.CommandTable[Request->Slot]->PrdTable;

    for (i = 0; i < SgList->NumberOfElements; ++i)
    {
        ASSERT(SgList->Elements[i].Length != 0);
        ASSERT(SgList->Elements[i].Length < AHCI_MAX_PRD_LENGTH);
        ASSERT((SgList->Elements[i].Length % sizeof(USHORT)) == 0);
        ASSERT(Is64BitDma || (SgList->Elements[i].Address.HighPart == 0));
        ASSERT(i < PortData->ChanExt->MapRegisterCount);

        PrdTableEntry->DataBaseLow = SgList->Elements[i].Address.LowPart;
        PrdTableEntry->DataBaseHigh = SgList->Elements[i].Address.HighPart;
        PrdTableEntry->ByteCount = SgList->Elements[i].Length - 1;

        ++PrdTableEntry;
    }

    /* Enable IRQ on last entry */
    --PrdTableEntry;
    PrdTableEntry->ByteCount |= AHCI_PRD_INTERRUPT_ON_COMPLETION;
}

static
VOID
AtaAhciPortCompleteCommands(
    _In_ PATAPORT_PORT_DATA PortData,
    _In_ ULONG CommandsCompleted)
{
    PATAPORT_CHANNEL_EXTENSION ChanExt = PortData->ChanExt;
    ULONG i;

    for (i = 0; i < AHCI_MAX_COMMAND_SLOTS; ++i)
    {
        PATA_DEVICE_REQUEST Request;
        UCHAR SrbStatus;

        if (!(CommandsCompleted & (1 << i)))
            continue;

        Request = PortData->Slots[i];
        ASSERT_REQUEST(Request);
        ASSERT(Request->Slot == i);

        SrbStatus = SRB_STATUS_SUCCESS;

        if (!(Request->Flags & REQUEST_FLAG_NCQ) &&
            Request->Flags & (REQUEST_FLAG_DATA_IN | REQUEST_FLAG_DATA_OUT))
        {
            PAHCI_COMMAND_HEADER CommandHeader;

            CommandHeader = &PortData->Ahci.CommandList->CommandHeader[i];

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
            AtaAhciSaveTaskFile(PortData, Request, FALSE);
        }

        InterlockedPushEntrySList(&ChanExt->CompletionQueueList, &Request->CompletionEntry);
    }

    KeInsertQueueDpc(&ChanExt->CompletionDpc, NULL, NULL);
}

VOID
AtaAhciPortHandleInterrupt(
    _In_ PATAPORT_PORT_DATA PortData)
{

    ULONG InterruptStatus, CommandsIssued, CommandsCompleted;
    AHCI_PORT_REGISTER CommandRegister;

    /* Clear all pending events */
    InterruptStatus = AHCI_PORT_READ(PortData->Ahci.IoBase, PxInterruptStatus);
    AHCI_PORT_WRITE(PortData->Ahci.IoBase, PxInterruptStatus, InterruptStatus);

    /* Clear interface errors */
    AHCI_PORT_WRITE(PortData->Ahci.IoBase, PxSataError,
                    AHCI_PORT_READ(PortData->Ahci.IoBase, PxSataError));

    /* Determine commands that have completed */
    if (PortData->ActiveQueuedSlotsBitmap != 0)
        CommandRegister = PxSataActive;
    else
        CommandRegister = PxCommandIssue;
    CommandsIssued = AHCI_PORT_READ(PortData->Ahci.IoBase, CommandRegister);
    CommandsCompleted = ~CommandsIssued & PortData->ActiveSlotsBitmap;

    if ((PortData->ActiveSlotsBitmap | PortData->ActiveQueuedSlotsBitmap) == 0)
        ASSERT(CommandsIssued == 0);

    /* Complete processed commands */
    if (CommandsCompleted != 0)
    {
        PortData->ActiveSlotsBitmap &= ~CommandsCompleted;
        PortData->ActiveQueuedSlotsBitmap &= ~CommandsCompleted;

        AtaAhciPortCompleteCommands(PortData, CommandsCompleted);
    }

    /* Asynchronous notification received */
    if (InterruptStatus & AHCI_PXIRQ_SDBS)
    {
        ULONG Message = AHCI_PORT_READ(PortData->Ahci.IoBase, PxSataNotification);

        // TODO
        if (Message != 0)
        {
            WARN("Notification %08lx arrived\n", Message);

            AHCI_PORT_WRITE(PortData->Ahci.IoBase, PxSataNotification, Message);
        }
    }

    /* Handle various errors and link change events */
    if (InterruptStatus & AHCI_PXIRQ_PORT_STATUS)
    {
        AtaAhciHandlePortStateChange(PortData, InterruptStatus);
    }

    if (InterruptStatus & AHCI_PXIRQ_FATAL_ERROR)
    {
        AtaAhciHandleFatalError(PortData);
    }
}

BOOLEAN
NTAPI
AtaHbaIsr(
    _In_ PKINTERRUPT Interrupt,
    _In_ PVOID Context)
{
    PATAPORT_CHANNEL_EXTENSION ChanExt = Context;
    ULONG i, InterruptStatus, PortInterruptBitmap;

    InterruptStatus = AHCI_HBA_READ(ChanExt->IoBase, HbaInterruptStatus);

    if (InterruptStatus == 0)
        return FALSE;

    PortInterruptBitmap = InterruptStatus & ChanExt->PortBitmap;

    for (i = 0; i < AHCI_MAX_PORTS; ++i)
    {
        if (!(PortInterruptBitmap & (1 << i)))
            continue;

        AtaAhciPortHandleInterrupt(&ChanExt->PortData[i]);
    }

    /* Clear pending HBA interrupts */
    AHCI_HBA_WRITE(ChanExt->IoBase, HbaInterruptStatus, InterruptStatus);

    return TRUE;
}
