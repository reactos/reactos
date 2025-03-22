/*
 * PROJECT:     ReactOS ATA Port Driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     AHCI request handling
 * COPYRIGHT:   Copyright 2024 Dmitry Borisov <di.sean@protonmail.com>
 */

/* INCLUDES *******************************************************************/

#include "atapi.h"

/* GLOBALS ********************************************************************/



/* FUNCTIONS ******************************************************************/

static
BOOLEAN
AtaAhciPollRegister(
    _In_ PULONG IoBase,
    _In_ AHCI_PORT_REGISTER Register,
    _In_ ULONG Mask,
    _In_ ULONG Value,
    _In_ ULONG Timeout)
{
    ULONG i, Data;

    ASSUME(Timeout > 0);

    for (i = Timeout; i > 0; --i)
    {
        Data = AHCI_PORT_READ(IoBase, Register);

        if ((Data & Mask) == Value)
            return TRUE;

        KeStallExecutionProcessor(10);
    }

    return FALSE;
}

static
VOID
AtaAhciSendComReset(
    _In_ PATAPORT_PORT_DATA PortData)
{
    ULONG SataControl;

    SataControl = AHCI_PORT_READ(PortData->Ahci.IoBase, PxSataControl);
    SataControl &= ~AHCI_PXCTL_DET_MASK;
    SataControl |= AHCI_PXCTL_DET_RESET;
    AHCI_PORT_WRITE(PortData->Ahci.IoBase, PxSataControl, SataControl);

    KeStallExecutionProcessor(1000);

    SataControl = AHCI_PORT_READ(PortData->Ahci.IoBase, PxSataControl);
    SataControl &= ~AHCI_PXCTL_DET_MASK;
    SataControl |= AHCI_PXCTL_DET_IDLE;
    AHCI_PORT_WRITE(PortData->Ahci.IoBase, PxSataControl, SataControl);
}

static
VOID
AtaAhciSetupDmaMemoryAddress(
    _In_ PATAPORT_CHANNEL_EXTENSION ChanExt,
    _In_ PATAPORT_PORT_DATA PortData)
{
    /* Physical address of the allocated command list */
    AHCI_PORT_WRITE(PortData->Ahci.IoBase,
                    PxCommandListBaseLow,
                    (ULONG)PortData->Ahci.CommandListPhys);
    if (ChanExt->AhciCapabilities & AHCI_CAP_S64A)
    {
        AHCI_PORT_WRITE(PortData->Ahci.IoBase,
                        PxCommandListBaseHigh,
                        (ULONG)(PortData->Ahci.CommandListPhys >> 32));
    }

    /* Physical address of the allocated FIS receive area */
    AHCI_PORT_WRITE(PortData->Ahci.IoBase,
                    PxFisBaseLow,
                    (ULONG)PortData->Ahci.ReceivedFisPhys);
    if (ChanExt->AhciCapabilities & AHCI_CAP_S64A)
    {
        AHCI_PORT_WRITE(PortData->Ahci.IoBase,
                        PxFisBaseHigh,
                        (ULONG)(PortData->Ahci.ReceivedFisPhys >> 32));
    }
}

static
BOOLEAN
AtaAhciWaitStopCommandListProcess(
    _In_ PATAPORT_PORT_DATA PortData)
{
    BOOLEAN Success;

    Success = AtaAhciPollRegister(PortData->Ahci.IoBase,
                                  PxCmdStatus,
                                  AHCI_PXCMD_CR,
                                  0,
                                  AHCI_DELAY_CR_START);
    if (!Success)
    {
        WARN("Failed to stop the command list DMA engine %08lx\n",
             AHCI_PORT_READ(PortData->Ahci.IoBase, PxCmdStatus));
    }

    return Success;
}

static
BOOLEAN
AtaAhciWaitStopFisReceiveProcess(
    _In_ PATAPORT_PORT_DATA PortData)
{
    BOOLEAN Success;

    Success = AtaAhciPollRegister(PortData->Ahci.IoBase,
                                  PxCmdStatus,
                                  AHCI_PXCMD_FR,
                                  0,
                                  AHCI_DELAY_FR_STOP);
    if (!Success)
    {
        WARN("Failed to stop the FIS Receive DMA engine %08lx\n",
             AHCI_PORT_READ(PortData->Ahci.IoBase, PxCmdStatus));
    }

    return Success;
}

static
VOID
AtaAhciStartCommandListProcess(
    _In_ PATAPORT_PORT_DATA PortData)
{
    ULONG CmdStatus;

    /* Set the ST bit */
    CmdStatus = AHCI_PORT_READ(PortData->Ahci.IoBase, PxCmdStatus);
    CmdStatus |= AHCI_PXCMD_ST;
    AHCI_PORT_WRITE(PortData->Ahci.IoBase, PxCmdStatus, CmdStatus);
}

static
BOOLEAN
AtaAhciCommandListOverride(
    _In_ PATAPORT_CHANNEL_EXTENSION ChanExt,
    _In_ PATAPORT_PORT_DATA PortData)
{
    ULONG CmdStatus;

    if (!(ChanExt->AhciCapabilities & AHCI_CAP_SCLO))
        return TRUE;

    CmdStatus = AHCI_PORT_READ(PortData->Ahci.IoBase, PxCmdStatus);
    CmdStatus |= AHCI_PXCMD_CLO;
    AHCI_PORT_WRITE(PortData->Ahci.IoBase, PxCmdStatus, CmdStatus);

    return AtaAhciPollRegister(PortData->Ahci.IoBase,
                               PxCmdStatus,
                               AHCI_PXCMD_CLO,
                               0,
                               AHCI_DELAY_CLO_CLEAR);
}

static
BOOLEAN
AtaAhciIdleState(
    _In_ PATAPORT_CHANNEL_EXTENSION ChanExt,
    _In_ PATAPORT_PORT_DATA PortData)
{
    ULONG CmdStatus;

    CmdStatus = AHCI_PORT_READ(PortData->Ahci.IoBase, PxCmdStatus);

    /* Already in idle state */
    if (!(CmdStatus & (AHCI_PXCMD_ST | AHCI_PXCMD_CR | AHCI_PXCMD_FRE | AHCI_PXCMD_FR)))
        return TRUE;

    /* Stop the command list DMA engine */
    CmdStatus &= ~AHCI_PXCMD_ST;
    AHCI_PORT_WRITE(PortData->Ahci.IoBase, PxCmdStatus, CmdStatus);

    if (!AtaAhciWaitStopCommandListProcess(PortData))
        return FALSE;

    AtaAhciCommandListOverride(ChanExt, PortData);

    /* Stop the FIS Receive DMA engine */
    CmdStatus = AHCI_PORT_READ(PortData->Ahci.IoBase, PxCmdStatus);
    CmdStatus &= ~AHCI_PXCMD_FRE;
    AHCI_PORT_WRITE(PortData->Ahci.IoBase, PxCmdStatus, CmdStatus);

    if (!AtaAhciWaitStopFisReceiveProcess(PortData))
        return FALSE;

    return TRUE;
}

static
ULONG
AtaAhciHandleFatalError(
    _In_ PATAPORT_PORT_DATA PortData,
    _In_ ULONG InterruptStatus)
{
    ULONG FirstTaskFileData, SecondTaskFileData;
    ULONG CmdStatus, CurrentCommandSlot, SlotsBitmap, FailedSlot;
    PATA_DEVICE_REQUEST Request;

    FirstTaskFileData = AHCI_PORT_READ(PortData->Ahci.IoBase, PxTaskFileData);
    CmdStatus = AHCI_PORT_READ(PortData->Ahci.IoBase, PxCmdStatus);

    CurrentCommandSlot = AHCI_PXCMD_CCS(CmdStatus);

    /* Clear the ST bit. This also clears PxCI, PxSACT, and PxCMD.CCS */
    CmdStatus &= ~AHCI_PXCMD_ST;
    AHCI_PORT_WRITE(PortData->Ahci.IoBase, PxCmdStatus, CmdStatus);

    if (!AtaAhciWaitStopCommandListProcess(PortData))
    {
        /* The interface is hung */
        return ACTION_RESET;
    }

    SecondTaskFileData = AHCI_PORT_READ(PortData->Ahci.IoBase, PxTaskFileData);
    if (SecondTaskFileData & (IDE_STATUS_BUSY | IDE_STATUS_DRQ))
    {
        /* Put the device to the idle state */
        ERR("Busy TFD %08lx\n", SecondTaskFileData);
        return ACTION_RESET;
    }

    /* Clear errors */
    AHCI_PORT_WRITE(PortData->Ahci.IoBase, PxSataError, AHCI_PORT_READ(PortData->Ahci.IoBase, PxSataError));

    if (PortData->ActiveQueuedSlotsBitmap != 0)
    {
        AtaAhciStartCommandListProcess(PortData);
        return ACTION_ERROR;
    }

    if (IsPowerOfTwo(PortData->ActiveSlotsBitmap))
    {
        /* We have exactly one slot is outstanding */
        SlotsBitmap = PortData->ActiveSlotsBitmap;
    }
    else if (PortData->ActiveSlotsBitmap & (1 << CurrentCommandSlot))
    {
        SlotsBitmap = 1 << CurrentCommandSlot;
    }
    else
    {
        /* Indicates that the error bit is spurious *or* the exact slot is not known */
        if (PortData->ActiveSlotsBitmap != 0)
        {
            ERR("Invalid slot received from the HBA %08lx --> %08lx\n",
                PortData->ActiveSlotsBitmap, 1 << CurrentCommandSlot);
        }
        else
        {
            WARN("Spurious error interrupt %08lx\n", InterruptStatus);
        }

        return ACTION_RESET;
    }

    AtaAhciStartCommandListProcess(PortData);

    NT_VERIFY(_BitScanForward(&FailedSlot, SlotsBitmap) != 0);

    Request = PortData->Slots[FailedSlot];
    ASSERT(Request);
    ASSERT(Request->Slot == FailedSlot);

    /* Save the error */
    Request->Status = (FirstTaskFileData & AHCI_PXTFD_STATUS_MASK);
    Request->Error = (FirstTaskFileData & AHCI_PXTFD_ERROR_MASK) >> AHCI_PXTFD_ERROR_SHIFT;
    Request->SrbStatus = SRB_STATUS_ERROR;
    if (Request->Flags & REQUEST_FLAG_SAVE_TASK_FILE)
    {
        AtaAhciSaveTaskFile(PortData, Request);
    }

    AtaReqCompleteFailedRequest(Request);
    return ACTION_NONE;
}

static
VOID
AtaAhciPortCompleteCommands(
    _In_ PATAPORT_PORT_DATA PortData,
    _In_ ULONG CommandsCompleted)
{
    ULONG i;

    for (i = 0; i < AHCI_MAX_COMMAND_SLOTS; ++i)
    {
        PATA_DEVICE_REQUEST Request;
        UCHAR SrbStatus;

        if (!(CommandsCompleted & (1 << i)))
            continue;

        Request = PortData->Slots[i];
        ASSERT(Request);
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

        AtaReqCompleteRequest(Request);
    }
}

static
VOID
AtaAhciSaveTaskFileEx(
    _In_ AHCI_FIS_DEVICE_TO_HOST* __restrict Fis,
    _Inout_ ATA_DEVICE_REQUEST* __restrict Request)
{
    PATA_TASKFILE TaskFile = &Request->TaskFile;

    TaskFile->Error = Fis->Error;
    TaskFile->SectorCount = Fis->SectorCount;
    TaskFile->LowLba = Fis->LbaLow;
    TaskFile->MidLba = Fis->LbaMid;
    TaskFile->HighLba = Fis->LbaHigh;
    TaskFile->DriveSelect = Fis->Device;
    TaskFile->Status = Fis->Status;

    if (Request->Flags & REQUEST_FLAG_LBA48)
    {
        TaskFile->FeatureEx = 0; // Fis byte 11 is reserved
        TaskFile->SectorCountEx = Fis->SectorCountEx;
        TaskFile->LowLbaEx = Fis->LbaLowEx;
        TaskFile->MidLbaEx = Fis->LbaMidEx;
        TaskFile->HighLbaEx = Fis->LbaHighEx;
    }

    Request->Flags |= REQUEST_FLAG_HAS_TASK_FILE;
}

static
VOID
AtaAhciStartHostToDeviceFis(
    _In_ ATA_DEVICE_REQUEST* __restrict Request,
    _Out_ AHCI_FIS_HOST_TO_DEVICE* __restrict Fis)
{
    Fis->Type = AHCI_FIS_REGISTER_HOST_TO_DEVICE;
    Fis->Flags = UPDATE_COMMAND;
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
        if ((Request->Flags & DEVICE_NEED_DMA_DIRECTION) &&
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
        ByteCount = min(Request->DataTransferLength, MAXUSHORT - 1);
        Fis->LbaMid = (UCHAR)ByteCount;
        Fis->LbaHigh = ByteCount >> 8;
    }
}

static
VOID
AhciBuildCommandFis(
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

static
VOID
AtaAhciBuildCommandHeader(
    _In_ PATAPORT_PORT_DATA PortData,
    _In_ PATA_DEVICE_REQUEST Request)
{
    PATAPORT_AHCI_SLOT_DATA SlotData;
    PAHCI_FIS_HOST_TO_DEVICE Fis;
    PAHCI_COMMAND_HEADER CommandHeader;
    ULONG Control;

    Control = sizeof(*Fis) / sizeof(ULONG);
    if (Request->Flags & (REQUEST_FLAG_DATA_IN | REQUEST_FLAG_DATA_OUT))
        Control |= Request->SgList->NumberOfElements << AHCI_COMMAND_HEADER_PRDT_LENGTH_SHIFT;
    if (Request->Flags & REQUEST_FLAG_DATA_OUT)
        Control |= AHCI_COMMAND_HEADER_WRITE;

    SlotData = &PortData->Ahci.Slot[Request->Slot];

    Fis = &SlotData->CommandTable->HostToDeviceFis;
    RtlZeroMemory(Fis, sizeof(*Fis));

    AtaAhciStartHostToDeviceFis(Request, Fis);

    if (Request->Flags & REQUEST_FLAG_PACKET_COMMAND)
    {
        AtaAhciBuildPacketCommandFis(Request, Fis);

        /* Copy the CDB bytes */
        RtlCopyMemory(SlotData->CommandTable->AtapiCommand,
                      Request->Cdb,
                      Request->DevExt->CdbSize);

        Control |= AHCI_COMMAND_HEADER_ATAPI;
    }
    else
    {
        AhciBuildCommandFis(Request, Fis);
    }

    CommandHeader = &PortData->Ahci.CommandList->CommandHeader[Request->Slot];
    CommandHeader->Control = Control;
    CommandHeader->PrdByteCount = 0;
    CommandHeader->CommandTableBaseLow = (ULONG)SlotData->CommandTablePhys;
    CommandHeader->CommandTableBaseHigh = (ULONG)(SlotData->CommandTablePhys >> 32);
}

UCHAR
AtaAhciStartIo(
    _In_ PATAPORT_PORT_DATA PortData,
    _In_ PATA_DEVICE_REQUEST Request)
{
    ASSERT(!(PortData->ActiveSlotsBitmap & (1 << Request->Slot)));
    ASSERT(!(PortData->ActiveQueuedSlotsBitmap & (1 << Request->Slot)));
    ASSERT(PortData->Slots[Request->Slot] == Request);

    AtaAhciBuildCommandHeader(PortData, Request);

    PortData->TimerCount[Request->Slot] = Request->TimeOut;
    PortData->ActiveSlotsBitmap |= 1 << Request->Slot;

    if (Request->Flags & REQUEST_FLAG_NCQ)
    {
        PortData->ActiveQueuedSlotsBitmap |= 1 << Request->Slot;

        AHCI_PORT_WRITE(PortData->Ahci.IoBase, PxSataActive, 1 << Request->Slot);
    }
    AHCI_PORT_WRITE(PortData->Ahci.IoBase, PxCommandIssue, 1 << Request->Slot);

    return SRB_STATUS_PENDING;
}

VOID
AtaAhciPreparePrdTable(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ PATA_DEVICE_REQUEST Request,
    _In_ SCATTER_GATHER_LIST* __restrict SgList)
{
    PAHCI_PRD_TABLE_ENTRY PrdTableEntry;
    ULONG i;

    ASSUME(SgList->NumberOfElements > 0);
    ASSERT(SgList->NumberOfElements < AHCI_MAX_PRDT_ENTRIES);

    PrdTableEntry = DevExt->PortData->Ahci.Slot[Request->Slot].CommandTable->PrdTable;

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

    KeFlushIoBuffers(Request->Mdl, !!(Request->Flags & REQUEST_FLAG_DATA_IN), TRUE);
}

BOOLEAN
AtaAhciEnumerateDevice(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt)
{
    PATAPORT_PORT_DATA PortData = DevExt->PortData;
    PATAPORT_CHANNEL_EXTENSION ChanExt = DevExt->ChanExt;
    ULONG CmdStatus, SataStatus, Signature;

    if (!AtaAhciIdleState(ChanExt, PortData))
        return FALSE;

    if (DevExt->Flags & DEVICE_ENUM)
        AtaAhciSetupDmaMemoryAddress(ChanExt, PortData);

    /* Move to the active interface state for the DET value be accurate */
    CmdStatus = AHCI_PORT_READ(PortData->Ahci.IoBase, PxCmdStatus);
    CmdStatus &= ~AHCI_PXCMD_ICC_MASK;
    CmdStatus |= AHCI_PXCMD_ICC_ACTIVE;
    AHCI_PORT_WRITE(PortData->Ahci.IoBase, PxCmdStatus, CmdStatus);

    AtaAhciPollRegister(PortData->Ahci.IoBase,
                        PxCmdStatus,
                        AHCI_PXCMD_ICC_MASK,
                        AHCI_PXCMD_ICC_IDLE,
                        AHCI_DELAY_INTERFACE_CHANGE);

    CmdStatus = AHCI_PORT_READ(PortData->Ahci.IoBase, PxCmdStatus);
    if (ChanExt->AhciCapabilities & AHCI_CAP_SSS)
    {
        CmdStatus |= AHCI_PXCMD_SUD;
    }
    if (CmdStatus & AHCI_PXCMD_CPD)
    {
        CmdStatus |= AHCI_PXCMD_POD;
    }
    AHCI_PORT_WRITE(PortData->Ahci.IoBase, PxCmdStatus, CmdStatus);

    CmdStatus = AHCI_PORT_READ(PortData->Ahci.IoBase, PxCmdStatus);
    CmdStatus |= AHCI_PXCMD_FRE;
    AHCI_PORT_WRITE(PortData->Ahci.IoBase, PxCmdStatus, CmdStatus);
    AtaAhciPollRegister(PortData->Ahci.IoBase,
                        PxCmdStatus,
                        AHCI_PXCMD_FRE,
                        AHCI_PXCMD_FRE,
                        AHCI_DELAY_FR_START);

    SataStatus = AHCI_PORT_READ(PortData->Ahci.IoBase, PxSataStatus);

    if ((SataStatus & AHCI_PXSSTS_DET_MASK) != AHCI_PXSSTS_DET_DEVICE_PHY)
    {
        AtaAhciSendComReset(PortData);

        if (!AtaAhciPollRegister(PortData->Ahci.IoBase,
                                 PxSataStatus,
                                 AHCI_PXSSTS_DET_MASK,
                                 AHCI_PXSSTS_DET_DEVICE_PHY,
                                 AHCI_DELAY_DET))
        {
            DevExt->WorkerContext.ConnectionStatus = CONN_STATUS_NO_DEVICE;
            return TRUE;
        }
    }

    /* Clear errors */
    AHCI_PORT_WRITE(PortData->Ahci.IoBase, PxSataError, 0xFFFFFFFF);

    if (!AtaAhciPollRegister(PortData->Ahci.IoBase,
                             PxTaskFileData,
                             IDE_STATUS_BUSY | IDE_STATUS_DRQ,
                             0,
                             AHCI_DELAY_WAIT_DRIVE))
    {
        DevExt->WorkerContext.ConnectionStatus = CONN_STATUS_NO_DEVICE;
        goto Done;
    }

    AtaAhciStartCommandListProcess(PortData);

    Signature = AHCI_PORT_READ(PortData->Ahci.IoBase, PxSignature);
    if (Signature == AHCI_PXSIG_ATAPI)
        DevExt->WorkerContext.ConnectionStatus = CONN_STATUS_DEV_ATAPI;
    else
        DevExt->WorkerContext.ConnectionStatus = CONN_STATUS_DEV_ATA;

Done:
    /* Clear errors */
    AHCI_PORT_WRITE(PortData->Ahci.IoBase, PxSataError, 0xFFFFFFFF);

    /* Clear interrupts */
    AHCI_PORT_WRITE(PortData->Ahci.IoBase, PxInterruptStatus, 0xFFFFFFFF);

    /* Re-enable interrupts */
    AHCI_PORT_WRITE(PortData->Ahci.IoBase, PxInterruptEnable, AHCI_PORT_INTERRUPT_MASK);

    return TRUE;
}

BOOLEAN
AtaAhciResetDevice(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt)
{
    PATAPORT_PORT_DATA PortData = DevExt->PortData;

    /* Hardware is gone */
    if (AHCI_HBA_READ(DevExt->ChanExt->IoBase, HbaAhciVersion) == 0xFFFFFFFF)
        return FALSE;

    /* Disable interrupts to ignore possible link change events */
    AHCI_PORT_WRITE(PortData->Ahci.IoBase, PxInterruptEnable, 0);

    /* Transmit the COMRESET signal */
    AtaAhciSendComReset(PortData);

    return TRUE;
}

VOID
AtaAhciSaveTaskFile(
    _In_ PATAPORT_PORT_DATA PortData,
    _Inout_ PATA_DEVICE_REQUEST Request)
{
    PAHCI_FIS_DEVICE_TO_HOST Fis = &PortData->Ahci.ReceivedFis->DeviceToHostFis;

    AtaAhciSaveTaskFileEx(Fis, Request);
}

BOOLEAN
AtaAhciDowngradeInterfaceSpeed(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt)
{
    PATAPORT_PORT_DATA PortData = DevExt->PortData;
    ULONG SataSpeed, SataControl;

    SataSpeed = AHCI_PORT_READ(PortData->Ahci.IoBase, PxSataStatus) & AHCI_PXSSTS_SPD_MASK;

    if (SataSpeed != AHCI_PXSSTS_SPD_SATA1)
    {
        SataControl = AHCI_PORT_READ(PortData->Ahci.IoBase, PxSataControl);

        if ((SataControl & AHCI_PXCTL_SPD_MASK) != AHCI_PXCTL_SPD_LIMIT_NONE)
        {
            SataControl &= ~AHCI_PXCTL_SPD_MASK;
            SataControl |= SataSpeed - AHCI_PXCTL_SPD_LIMIT_LEVEL;

            WARN("Downgrading interface speed to %08lx\n", SataControl);

            AHCI_PORT_WRITE(PortData->Ahci.IoBase, PxSataControl, SataControl);
            return TRUE;
        }
    }

    return FALSE;
}

VOID
NTAPI
AtaAhciPortDpc(
    _In_ PKDPC Dpc,
    _In_opt_ PVOID DeferredContext,
    _In_opt_ PVOID SystemArgument1,
    _In_opt_ PVOID SystemArgument2)
{
    PATAPORT_PORT_DATA PortData = DeferredContext;
    ULONG InterruptStatus, CommandsIssued, CommandsCompleted;
    AHCI_PORT_REGISTER CommandRegister;

    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(SystemArgument1);
    UNREFERENCED_PARAMETER(SystemArgument2);

    KeAcquireSpinLockAtDpcLevel(&PortData->PortLock);

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
    {
        ASSERT(CommandsIssued == 0);
    }

    /* Stop all timers tied to the completed slots */
    PortData->ActiveSlotsBitmap &= ~CommandsCompleted;
    PortData->ActiveQueuedSlotsBitmap &= ~CommandsCompleted;

    /* Handle errors */
    if (InterruptStatus & (AHCI_PXIRQ_FATAL_ERROR | AHCI_PXIRQ_PORT_STATUS))
    {
        ATA_DEVICE_ACTION Action;

        if (InterruptStatus & AHCI_PXIRQ_PCS)
        {
            Action = ACTION_DEV_CHANGE;
        }
        else
        {
            Action = AtaAhciHandleFatalError(PortData, InterruptStatus);
        }

        /* Perform recovery actions */
        if (Action != ACTION_NONE)
        {
            // TODO
        }
    }

    KeReleaseSpinLockFromDpcLevel(&PortData->PortLock);

    /* Complete processed commands */
    AtaAhciPortCompleteCommands(PortData, CommandsCompleted);

    /* Re-enable interrupts */
    AHCI_PORT_WRITE(PortData->Ahci.IoBase, PxInterruptEnable, AHCI_PORT_INTERRUPT_MASK);
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

    InterruptBitmap = InterruptStatus & ChanExt->PortBitmap;

    for (i = 0; i < AHCI_MAX_PORTS; ++i)
    {
        PULONG IoBase;

        if (!(InterruptBitmap & (1 << i)))
            continue;

        IoBase = AHCI_PORT_BASE(ChanExt->IoBase, i);

        /* Disable further port interrupts */
        AHCI_PORT_WRITE(IoBase, PxInterruptEnable, 0);

        KeInsertQueueDpc(&ChanExt->PortData[i].Ahci.Dpc, NULL, NULL);
    }

    /* Clear pending HBA interrupts */
    AHCI_HBA_WRITE(ChanExt->IoBase, HbaInterruptStatus, InterruptStatus);

    return TRUE;
}

CODE_SEG("PAGE")
VOID
AtaFdoFreePortMemory(
    _In_ PATAPORT_CHANNEL_EXTENSION ChanExt)
{
    // todo
}

static
CODE_SEG("PAGE")
BOOLEAN
AtaAhciPortAllocateMemory(
    _In_ PATAPORT_CHANNEL_EXTENSION ChanExt,
    _Out_ PATAPORT_PORT_DATA PortData,
    _Out_ PATAPORT_PORT_INFO PortInfo)
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

    CommandTableLength = FIELD_OFFSET(AHCI_COMMAND_TABLE, PrdTable[ChanExt->MapRegisterCount]);

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

            PortData->Ahci.Slot[SlotNumber].CommandTable = (PVOID)BufferVa;
            PortData->Ahci.Slot[SlotNumber].CommandTablePhys = BufferPa;

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

    /* Local buffer */
    Length += ATA_LOCAL_BUFFER_SIZE;

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
    PortData->Ahci.CommandList = (PVOID)BufferVa;
    PortData->Ahci.CommandListPhys = BufferPa;
    BufferVa += CommandListSize;
    BufferPa += CommandListSize;

    /* Alignment requirement */
    ASSERT((ULONG_PTR)PortData->Ahci.CommandListPhys % AHCI_COMMAND_LIST_ALIGNMENT == 0);

    /* Received FIS */
    if (TRUE) // TODO !FBS
    {
        BufferVa = ALIGN_UP_BY(BufferVa, AHCI_RECEIVED_FIS_ALIGNMENT);
        BufferPa = ALIGN_UP_BY(BufferPa, AHCI_RECEIVED_FIS_ALIGNMENT);
        PortData->Ahci.ReceivedFis = (PVOID)BufferVa;
        PortData->Ahci.ReceivedFisPhys = BufferPa;
        BufferVa += sizeof(AHCI_RECEIVED_FIS);
        BufferPa += sizeof(AHCI_RECEIVED_FIS);

        /* Alignment requirement */
        ASSERT((ULONG_PTR)PortData->Ahci.ReceivedFisPhys % AHCI_RECEIVED_FIS_ALIGNMENT == 0);
    }

    /* Local buffer */
    PortInfo->LocalBuffer = (PVOID)BufferVa;
    PortData->Ahci.LocalSgList.Elements[0].Address.QuadPart = BufferPa;
    PortData->Ahci.LocalSgList.Elements[0].Length = ATA_LOCAL_BUFFER_SIZE;
    PortData->Ahci.LocalSgList.NumberOfElements = 1;

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
        PortData->Ahci.ReceivedFis = (PVOID)BufferVa;
        PortData->Ahci.ReceivedFisPhys = BufferPa;

        /* Alignment requirement */
        ASSERT(BufferPa % AHCI_RECEIVED_FIS_FBS_ALIGNMENT == 0);
    }

    /* 32-bit DMA */
    if (!(ChanExt->AhciCapabilities & AHCI_CAP_S64A))
    {
        ASSERT((ULONG)(PortData->Ahci.CommandListPhys >> 32) == 0);
        ASSERT((ULONG)(PortData->Ahci.ReceivedFisPhys >> 32) == 0);
    }

    return TRUE;
}

CODE_SEG("PAGE")
BOOLEAN
AtaAhciPortInit(
    _In_ PATAPORT_CHANNEL_EXTENSION ChanExt,
    _Out_ PATAPORT_PORT_DATA PortData,
    _Out_ PATAPORT_PORT_INFO PortInfo)
{
    ULONG CmdStatus;

    PAGED_CODE();

    /* Disable interrupts */
    AHCI_PORT_WRITE(PortData->Ahci.IoBase, PxInterruptEnable, 0);

    /* Clear interrupts */
    AHCI_PORT_WRITE(PortData->Ahci.IoBase, PxInterruptStatus, 0xFFFFFFFF);

    /* Clear the error register */
    AHCI_PORT_WRITE(PortData->Ahci.IoBase, PxSataError, 0xFFFFFFFF);

    /* Stop the command list DMA engine */
    CmdStatus = AHCI_PORT_READ(PortData->Ahci.IoBase, PxCmdStatus);
    CmdStatus &= ~AHCI_PXCMD_ST;
    AHCI_PORT_WRITE(PortData->Ahci.IoBase, PxCmdStatus, CmdStatus);

    /* Allocate shared memory */
    if (!AtaAhciPortAllocateMemory(ChanExt, PortData, PortInfo))
        return FALSE;

    return TRUE;
}

CODE_SEG("PAGE")
NTSTATUS
AtaAhciInitHba(
    _In_ PATAPORT_CHANNEL_EXTENSION ChanExt)
{
    ULONG i, GlobalControl, AhciVersion;

    PAGED_CODE();

    TRACE("Initializing HBA [%04X:%04X]\n", ChanExt->DeviceID, ChanExt->VendorID);

    /* Set AE before accessing other AHCI registers */
    GlobalControl = AHCI_HBA_READ(ChanExt->IoBase, HbaGlobalControl);
    GlobalControl |= AHCI_GHC_AE;
    AHCI_HBA_WRITE(ChanExt->IoBase, HbaGlobalControl, GlobalControl);

    AhciVersion = AHCI_HBA_READ(ChanExt->IoBase, HbaAhciVersion);
    if (AhciVersion >= AHCI_VERSION_1_2)
    {
        ChanExt->AhciCapabilitiesEx = AHCI_HBA_READ(ChanExt->IoBase, HbaCapabilitiesEx);

        /* if (ChanExt->AhciCapabilitiesEx & AHCI_CAP2_BOH) */ // todo
    }

    ChanExt->AhciCapabilities = AHCI_HBA_READ(ChanExt->IoBase, HbaCapabilities);
    ChanExt->PortBitmap = AHCI_HBA_READ(ChanExt->IoBase, HbaPortBitmap);
    ChanExt->NumberOfPorts = CountSetBits(ChanExt->PortBitmap);

    if (ChanExt->NumberOfPorts == 0)
    {
        ASSERT(ChanExt->NumberOfPorts == 0);
        return STATUS_DEVICE_HARDWARE_ERROR;
    }

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
        return STATUS_IO_TIMEOUT;
    }

    /* Disable interrupts and re-enable AE */
    GlobalControl |= AHCI_GHC_AE;
    GlobalControl &= ~AHCI_GHC_IE;
    AHCI_HBA_WRITE(ChanExt->IoBase, HbaGlobalControl, GlobalControl);

    /* Clear interrupts */
    AHCI_HBA_WRITE(ChanExt->IoBase, HbaInterruptStatus, 0xFFFFFFFF);

    INFO("Version %08lx, PI %08lx, CAP %08lx, CAP2 %08lx\n",
         AhciVersion,
         ChanExt->PortBitmap,
         ChanExt->AhciCapabilities,
         ChanExt->AhciCapabilitiesEx);

    return STATUS_SUCCESS;
}
