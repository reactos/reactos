/*
 * PROJECT:     ReactOS ATA Port Driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     PATA I/O request handling
 * COPYRIGHT:   Copyright 2024-2025 Dmitry Borisov <di.sean@protonmail.com>
 */

/* INCLUDES *******************************************************************/

#include "atapi.h"

/* FUNCTIONS ******************************************************************/

static
VOID
AtaPataSaveTaskFile(
    _In_ ATAPORT_PORT_DATA* __restrict PortData,
    _Inout_ ATA_DEVICE_REQUEST* __restrict Request)
{
    PATA_TASKFILE TaskFile = &Request->Output;

    TaskFile->Error = ATA_READ(PortData->Pata.Registers.Error);
    TaskFile->SectorCount = ATA_READ(PortData->Pata.Registers.SectorCount);
    TaskFile->LowLba = ATA_READ(PortData->Pata.Registers.LbaLow);
    TaskFile->MidLba = ATA_READ(PortData->Pata.Registers.LbaMid);
    TaskFile->HighLba = ATA_READ(PortData->Pata.Registers.LbaHigh);
    TaskFile->DriveSelect = ATA_READ(PortData->Pata.Registers.Device);
    TaskFile->Command = ATA_READ(PortData->Pata.Registers.Command);

    if (Request->Flags & REQUEST_FLAG_LBA48)
    {
        UCHAR Control = ATA_READ(PortData->Pata.Registers.Control) | IDE_DC_ALWAYS;

        /* Read the extra information from the second byte of FIFO */
        ATA_WRITE(PortData->Pata.Registers.Control, Control | IDE_HIGH_ORDER_BYTE);

        TaskFile->FeatureEx = ATA_READ(PortData->Pata.Registers.Features);
        TaskFile->SectorCountEx = ATA_READ(PortData->Pata.Registers.SectorCount);
        TaskFile->LowLbaEx = ATA_READ(PortData->Pata.Registers.LbaLow);
        TaskFile->MidLbaEx = ATA_READ(PortData->Pata.Registers.LbaMid);
        TaskFile->HighLbaEx = ATA_READ(PortData->Pata.Registers.LbaHigh);

        ATA_WRITE(PortData->Pata.Registers.Control, Control);
    }

    Request->Flags |= REQUEST_FLAG_HAS_TASK_FILE;
}

/* Called at DIRQL */
static
VOID
AtaPataCompleteCommand(
    _In_ PATAPORT_PORT_DATA PortData,
    _In_ UCHAR IdeStatus)
{
    PATA_DEVICE_REQUEST Request;

    /* Make sure that the request handler won't be invoked from the interrupt handler */
    PortData->Pata.CommandFlags = CMD_FLAG_NONE;

    Request = PortData->Slots[PATA_CHANNEL_SLOT];

    /* Handle failed commands */
    if (Request->SrbStatus != SRB_STATUS_SUCCESS &&
        Request->SrbStatus != SRB_STATUS_DATA_OVERRUN)
    {
        if (!IS_ATAPI(Request->Device) || (Request->Flags & REQUEST_FLAG_SAVE_TASK_FILE))
        {
            /* Save the current task file for the "ATA LBA field" (SAT-5 11.7) */
            AtaPataSaveTaskFile(PortData, Request);
        }
        else
        {
            Request->Output.Status = IdeStatus;
            Request->Output.Error = ATA_READ(PortData->Pata.Registers.Error);
        }

        /* Request arbitration from the port worker */
        AtaPortRecoveryFromError(PortData, ACTION_DEVICE_ERROR, Request);
    }
    else
    {
        PortData->ActiveSlotsBitmap &= ~(1 << PATA_CHANNEL_SLOT);

        /* Save the latest copy of the task file registers */
        if (Request->Flags & REQUEST_FLAG_SAVE_TASK_FILE)
        {
            AtaPataSaveTaskFile(PortData, Request);
        }

        AtaReqStartCompletionDpc(Request);
    }
}

static
inline
UCHAR
AtaPciIdeDmaReadStatus(
    _In_ PATAPORT_PORT_DATA PortData)
{
    return ATA_READ(PortData->Pata.PciIdeInterface.IoBase + PCIIDE_DMA_STATUS);
}

static
VOID
AtaPciIdeDmaPrepare(
    _In_ PATAPORT_PORT_DATA PortData)
{
    PUCHAR IoBase = PortData->Pata.PciIdeInterface.IoBase;

    /* Clear the interrupt and error bits in the status register */
    ATA_WRITE(IoBase + PCIIDE_DMA_COMMAND, PCIIDE_DMA_COMMAND_STOP);
    ATA_WRITE(IoBase + PCIIDE_DMA_STATUS, PCIIDE_DMA_STATUS_INTERRUPT | PCIIDE_DMA_STATUS_ERROR);

    /* Set the address to the beginning of the physical region descriptor table */
    ATA_WRITE_ULONG((PULONG)(IoBase + PCIIDE_DMA_PRDT_PHYSICAL_ADDRESS),
                    PortData->Pata.PciIdeInterface.PrdTablePhysicalAddress);
}

static
VOID
AtaPciIdeDmaStart(
    _In_ PATAPORT_PORT_DATA PortData,
    _In_ PATA_DEVICE_REQUEST Request)
{
    UCHAR Command;

    ASSERT(Request->Flags & REQUEST_FLAG_HAS_SG_LIST);

    /* Transfer direction */
    if (Request->Flags & REQUEST_FLAG_DATA_IN)
        Command = PCIIDE_DMA_COMMAND_WRITE_TO_SYSTEM_MEMORY;
    else
        Command = PCIIDE_DMA_COMMAND_READ_FROM_SYSTEM_MEMORY;
    Command |= PCIIDE_DMA_COMMAND_START;

    /* Begin transaction */
    ATA_WRITE(PortData->Pata.PciIdeInterface.IoBase + PCIIDE_DMA_COMMAND, Command);
}

static
VOID
AtaPciIdeDmaStop(
    _In_ PATAPORT_PORT_DATA PortData)
{
    PUCHAR IoBase = PortData->Pata.PciIdeInterface.IoBase;

    /* Wait one PIO transfer cycle */
    (VOID)ATA_READ(IoBase + PCIIDE_DMA_STATUS);

    /* Clear the interrupt bit in the status register */
    ATA_WRITE(IoBase + PCIIDE_DMA_COMMAND, PCIIDE_DMA_COMMAND_STOP);
    ATA_WRITE(IoBase + PCIIDE_DMA_STATUS, PCIIDE_DMA_STATUS_INTERRUPT);
}

static
VOID
AtaPciIdeDmaClear(
    _In_ PATAPORT_PORT_DATA PortData,
    _In_ UCHAR DmaStatus)
{
    ATA_WRITE(PortData->Pata.PciIdeInterface.IoBase + PCIIDE_DMA_STATUS, DmaStatus);
}

static
VOID
AtaPataChangeInterruptMode(
    _In_ PATAPORT_PORT_DATA PortData,
    _In_ PATA_DEVICE_REQUEST Request)
{
    UCHAR Control;

    if (Request->Flags & REQUEST_FLAG_POLL)
    {
        PortData->PortFlags |= PORT_FLAG_IN_POLL_STATE;
        Control = IDE_DC_DISABLE_INTERRUPTS;
    }
    else
    {
        PortData->PortFlags &= ~PORT_FLAG_IN_POLL_STATE;
        Control = 0;
    }
    TRACE("Interrupt mode %s\n", Control ? "disable" : "enable");

    ATA_WRITE(PortData->Pata.Registers.Control, Control | IDE_DC_ALWAYS);
}

static
VOID
AtaPataSendCdb(
    _In_ PATAPORT_PORT_DATA PortData,
    _In_ PATA_DEVICE_REQUEST Request)
{
    /* Command packet transfer */
    if (PortData->PortFlags & PORT_FLAG_IO32)
    {
        ATA_WRITE_BLOCK_32((PULONG)PortData->Pata.Registers.Data,
                           (PULONG)Request->Cdb,
                           Request->Device->CdbSize / sizeof(USHORT));
    }
    else
    {
        ATA_WRITE_BLOCK_16((PUSHORT)PortData->Pata.Registers.Data,
                           (PUSHORT)Request->Cdb,
                           Request->Device->CdbSize);
    }

    /*
     * In polled mode (interrupts disabled)
     * the NEC CDR-260 drive clears BSY before updating the interrupt reason register.
     * As a workaround, we will wait for the phase change.
     */
    if ((Request->Flags & REQUEST_FLAG_POLL) &&
        (Request->Device->DeviceFlags & DEVICE_IS_NEC_CDR260))
    {
        ULONG i;

        for (i = 0; i < ATA_TIME_PHASE_CHANGE; ++i)
        {
            KeStallExecutionProcessor(10);

            if (ATA_READ(PortData->Pata.Registers.InterruptReason) != ATAPI_INT_REASON_COD)
                break;
        }
    }
}

static
VOID
AtaPataPioDataIn(
    _In_ PATAPORT_PORT_DATA PortData,
    _In_ ULONG ByteCount)
{
    ByteCount = min(ByteCount, PortData->Pata.BytesToTransfer);

    /* Transfer the data block */
    if (!(ByteCount & (sizeof(ULONG) - 1)) &&
        (PortData->PortFlags & PORT_FLAG_IO32))
    {
        ATA_READ_BLOCK_32((PULONG)PortData->Pata.Registers.Data,
                          (PULONG)PortData->Pata.DataBuffer,
                          ByteCount / sizeof(ULONG));
    }
    else
    {
        ATA_READ_BLOCK_16((PUSHORT)PortData->Pata.Registers.Data,
                          (PUSHORT)PortData->Pata.DataBuffer,
                          ByteCount / sizeof(USHORT));

        /* Read one last byte */
        if (ByteCount & (sizeof(USHORT) - 1))
        {
            PUCHAR Buffer = PortData->Pata.DataBuffer + ByteCount - 1;

            *Buffer = ATA_READ(PortData->Pata.Registers.Data);
        }
    }

    PortData->Pata.DataBuffer += ByteCount;
    PortData->Pata.BytesToTransfer -= ByteCount;
}

static
VOID
AtaPataPioDataOut(
    _In_ PATAPORT_PORT_DATA PortData,
    _In_ ULONG ByteCount)
{
    ByteCount = min(ByteCount, PortData->Pata.BytesToTransfer);

    /* Transfer the data block */
    if (!(ByteCount & (sizeof(ULONG) - 1)) &&
        (PortData->PortFlags & PORT_FLAG_IO32))
    {
        ATA_WRITE_BLOCK_32((PULONG)PortData->Pata.Registers.Data,
                           (PULONG)PortData->Pata.DataBuffer,
                           ByteCount / sizeof(ULONG));
    }
    else
    {
        ATA_WRITE_BLOCK_16((PUSHORT)PortData->Pata.Registers.Data,
                           (PUSHORT)PortData->Pata.DataBuffer,
                           ByteCount / sizeof(USHORT));

        /* Write one last byte */
        if (ByteCount & (sizeof(USHORT) - 1))
        {
            PUCHAR Buffer = PortData->Pata.DataBuffer + ByteCount - 1;

            ATA_WRITE(PortData->Pata.Registers.Data, *Buffer);
        }
    }

    PortData->Pata.DataBuffer += ByteCount;
    PortData->Pata.BytesToTransfer -= ByteCount;
}

static
BOOLEAN
AtaPataProcessRequest(
    _In_ PATAPORT_PORT_DATA PortData,
    _In_ UCHAR IdeStatus,
    _In_ UCHAR DmaStatus)
{
    PATA_DEVICE_REQUEST Request = PortData->Slots[PATA_CHANNEL_SLOT];
    UCHAR SrbStatus;

    ASSERT(!(IdeStatus & IDE_STATUS_BUSY));

    switch (PortData->Pata.CommandFlags & CMD_FLAG_TRANSFER_MASK)
    {
        case CMD_FLAG_DMA_TRANSFER:
        {
            if ((DmaStatus & PCIIDE_DMA_STATUS_ERROR) ||
                (IdeStatus & (IDE_STATUS_ERROR | IDE_STATUS_DEVICE_FAULT)))
            {
                SrbStatus = SRB_STATUS_ERROR;
            }
            else
            {
                SrbStatus = SRB_STATUS_SUCCESS;
            }
            break;
        }

        case CMD_FLAG_ATA_PIO_TRANSFER:
        {
            /* Check for errors */
            if (IdeStatus & (IDE_STATUS_ERROR | IDE_STATUS_DEVICE_FAULT))
            {
                if (IdeStatus & IDE_STATUS_DRQ)
                {
                    ERR("DRQ not cleared, status 0x%02x\n", IdeStatus);
                    SrbStatus = SRB_STATUS_BUS_RESET;
                }
                else
                {
                    SrbStatus = SRB_STATUS_ERROR;
                }
                break;
            }

            /* Non-data ATA command */
            if (!(PortData->Pata.CommandFlags & (CMD_FLAG_DATA_IN | CMD_FLAG_DATA_OUT)))
            {
#if 0 // TODO: Too strict?
                if (IdeStatus & IDE_STATUS_DRQ)
                {
                    ERR("DRQ not cleared, status 0x%02x\n", IdeStatus);
                    SrbStatus = SRB_STATUS_BUS_RESET;
                }
                else
#endif
                {
                    SrbStatus = SRB_STATUS_SUCCESS;
                }
                break;
            }

            if (PortData->Pata.CommandFlags & CMD_FLAG_DATA_IN)
            {
                if (!(IdeStatus & IDE_STATUS_DRQ))
                {
                    if ((IdeStatus == 0) &&
                        ((Request->TaskFile.Command == IDE_COMMAND_ATAPI_IDENTIFY) ||
                        (Request->TaskFile.Command == IDE_COMMAND_IDENTIFY)))
                    {
                        /*
                         * Some controllers indicate status 0x00
                         * when the selected device does not exist,
                         * no point in going further.
                         */
                        SrbStatus = SRB_STATUS_ERROR;
                    }
                    else
                    {
                        ERR("DRQ not set, status 0x%02x\n", IdeStatus);
                        SrbStatus = SRB_STATUS_BUS_RESET;
                    }
                    break;
                }

                /* Read the next data block */
                AtaPataPioDataIn(PortData, PortData->Pata.DrqByteCount);

                if (PortData->Pata.BytesToTransfer != 0)
                    return FALSE;

                /* All data has been transferred, wait for DRQ to clear */
                IdeStatus = ATA_READ(PortData->Pata.Registers.Status);
                if (!ATA_WAIT_FOR_IDLE(&PortData->Pata.Registers, &IdeStatus))
                {
                    ERR("DRQ not cleared, status 0x%02x\n", IdeStatus);
                    SrbStatus = SRB_STATUS_BUS_RESET;
                    break;
                }

                if (IdeStatus & (IDE_STATUS_ERROR | IDE_STATUS_DEVICE_FAULT))
                    SrbStatus = SRB_STATUS_ERROR;
                else
                    SrbStatus = SRB_STATUS_SUCCESS;
                break;
            }
            else
            {
                if (PortData->Pata.BytesToTransfer == 0)
                {
                    if (IdeStatus & IDE_STATUS_DRQ)
                    {
                        ERR("DRQ not cleared, status 0x%02x\n", IdeStatus);
                        SrbStatus = SRB_STATUS_BUS_RESET;
                    }
                    else
                    {
                        SrbStatus = SRB_STATUS_SUCCESS;
                    }
                    break;
                }

                if (!(IdeStatus & IDE_STATUS_DRQ))
                {
                    ERR("DRQ not set, status 0x%02x %lu/%lu\n",
                        IdeStatus,
                        PortData->Pata.BytesToTransfer,
                        Request->DataTransferLength);
                    SrbStatus = SRB_STATUS_BUS_RESET;
                    break;
                }

                /* Write the next data block */
                AtaPataPioDataOut(PortData, PortData->Pata.DrqByteCount);
            }

            return FALSE;
        }

        case CMD_FLAG_ATAPI_PIO_TRANSFER:
        {
            UCHAR InterruptReason;
            ULONG ByteCount;

            InterruptReason = ATA_READ(PortData->Pata.Registers.InterruptReason);
            InterruptReason &= ATAPI_INT_REASON_MASK;
            InterruptReason |= IdeStatus & IDE_STATUS_DRQ;

            switch (InterruptReason)
            {
                case ATAPI_INT_REASON_AWAIT_CDB:
                {
                    if (!(PortData->Pata.CommandFlags & CMD_FLAG_AWAIT_CDB))
                    {
                        WARN("Invalid interrupt reason %02x %02x, flags %08lx\n",
                             InterruptReason,
                             IdeStatus,
                             PortData->Pata.CommandFlags);

                        SrbStatus = SRB_STATUS_BUS_RESET;
                        break;
                    }
                    PortData->Pata.CommandFlags &= ~CMD_FLAG_AWAIT_CDB;

                    AtaPataSendCdb(PortData, Request);

                    /* Start the DMA engine */
                    if (Request->Flags & REQUEST_FLAG_DMA)
                    {
                        PortData->Pata.CommandFlags |= CMD_FLAG_DMA_TRANSFER;
                        AtaPciIdeDmaStart(PortData, Request);
                    }
                    return FALSE;
                }

                case ATAPI_INT_REASON_DATA_IN:
                {
                    if (PortData->Pata.CommandFlags & (CMD_FLAG_DATA_OUT | CMD_FLAG_AWAIT_CDB))
                    {
                        WARN("Invalid interrupt reason %02x %02x, flags %08lx\n",
                             InterruptReason,
                             IdeStatus,
                             PortData->Pata.CommandFlags);

                        SrbStatus = SRB_STATUS_BUS_RESET;
                        break;
                    }

                    ByteCount = ATA_READ(PortData->Pata.Registers.ByteCountLow);
                    ByteCount |= ATA_READ(PortData->Pata.Registers.ByteCountHigh) << 8;

                    AtaPataPioDataIn(PortData, ByteCount);
                    return FALSE;
                }

                case ATAPI_INT_REASON_DATA_OUT:
                {
                    if (PortData->Pata.CommandFlags & (CMD_FLAG_DATA_IN | CMD_FLAG_AWAIT_CDB))
                    {
                        WARN("Invalid interrupt reason %02x %02x, flags %08lx\n",
                             InterruptReason,
                             IdeStatus,
                             PortData->Pata.CommandFlags);

                        SrbStatus = SRB_STATUS_BUS_RESET;
                        break;
                    }

                    ByteCount = ATA_READ(PortData->Pata.Registers.ByteCountLow);
                    ByteCount |= ATA_READ(PortData->Pata.Registers.ByteCountHigh) << 8;

                    AtaPataPioDataOut(PortData, ByteCount);
                    return FALSE;
                }

                case ATAPI_INT_REASON_STATUS_NEC:
                {
                    /* The NEC CDR-260 drive always clears CoD and IO on command completion */
                    if (!(Request->Device->DeviceFlags & DEVICE_IS_NEC_CDR260))
                    {
                        WARN("Invalid interrupt reason %02x %02x, flags %08lx\n",
                             InterruptReason,
                             IdeStatus,
                             PortData->Pata.CommandFlags);

                        SrbStatus = SRB_STATUS_BUS_RESET;
                        break;
                    }
                    __fallthrough;
                }
                case ATAPI_INT_REASON_STATUS:
                {
                    if (IdeStatus & (IDE_STATUS_ERROR | IDE_STATUS_DEVICE_FAULT))
                    {
                        SrbStatus = SRB_STATUS_ERROR;
                    }
                    else if (PortData->Pata.BytesToTransfer != 0)
                    {
                        /* This indicates a residual underrun */
                        SrbStatus = SRB_STATUS_DATA_OVERRUN;

                        ASSERT(Request->DataTransferLength >= PortData->Pata.BytesToTransfer);
                        Request->DataTransferLength -= PortData->Pata.BytesToTransfer;
                    }
                    else
                    {
                        SrbStatus = SRB_STATUS_SUCCESS;
                    }
                    break;
                }

                default:
                {
                    WARN("Invalid interrupt reason %02x %02x, flags %08lx\n",
                         InterruptReason,
                         IdeStatus,
                         PortData->Pata.CommandFlags);

                    SrbStatus = SRB_STATUS_BUS_RESET;
                    break;
                }
            }
            break;
        }

        default:
        {
            ASSERT(FALSE);
            UNREACHABLE;
        }
    }

    Request->SrbStatus = SrbStatus;

    /* Complete the request */
    AtaPataCompleteCommand(PortData, IdeStatus);
    return TRUE;
}

static
BOOLEAN
AtaPataExecutePacketCommand(
    _In_ PATAPORT_PORT_DATA PortData,
    _In_ PATAPORT_IO_CONTEXT Device,
    _In_ PATA_DEVICE_REQUEST Request)
{
    ULONG CommandFlags;
    USHORT ByteCount;
    UCHAR Features;

    CommandFlags = (Request->Flags & (REQUEST_FLAG_DATA_IN | REQUEST_FLAG_DATA_OUT));
    CommandFlags |= ~Request->Flags & REQUEST_FLAG_POLL;
    CommandFlags |= CMD_FLAG_ATAPI_PIO_TRANSFER | CMD_FLAG_AWAIT_CDB;
    PortData->Pata.CommandFlags = CommandFlags;

    /* Prepare to transfer a device command */
    if (Request->Flags & REQUEST_FLAG_DMA)
    {
        /* DMA transfer */
        ByteCount = 0;
        Features = IDE_FEATURE_DMA;
    }
    else
    {
        /* PIO transfer */
        ByteCount = min(PortData->Pata.BytesToTransfer, ATAPI_MAX_DRQ_DATA_BLOCK);
        Features = IDE_FEATURE_PIO;
    }
    ATA_WRITE(PortData->Pata.Registers.ByteCountLow, (UCHAR)(ByteCount >> 0));
    ATA_WRITE(PortData->Pata.Registers.ByteCountHigh, (UCHAR)(ByteCount >> 8));
    ATA_WRITE(PortData->Pata.Registers.Features, Features);
    ATA_WRITE(PortData->Pata.Registers.Command, IDE_COMMAND_ATAPI_PACKET);

    /* Wait for an interrupt that signals that device is ready to accept the command packet */
    if (Device->DeviceFlags & DEVICE_HAS_CDB_INTERRUPT)
        return FALSE;

    /* Command packet transfer */
    return TRUE;
}

static
BOOLEAN
AtaPataExecuteAtaCommand(
    _In_ PATAPORT_PORT_DATA PortData,
    _In_ PATAPORT_IO_CONTEXT Device,
    _In_ PATA_DEVICE_REQUEST Request)
{
    ULONG CommandFlags, BlockCount;

    /* Prepare the registers */
    if (Request->Flags & REQUEST_FLAG_LBA48)
    {
        ATA_WRITE(PortData->Pata.Registers.Features, Request->TaskFile.FeatureEx);
        ATA_WRITE(PortData->Pata.Registers.SectorCount, Request->TaskFile.SectorCountEx);
        ATA_WRITE(PortData->Pata.Registers.LbaLow, Request->TaskFile.LowLbaEx);
        ATA_WRITE(PortData->Pata.Registers.LbaMid, Request->TaskFile.MidLbaEx);
        ATA_WRITE(PortData->Pata.Registers.LbaHigh, Request->TaskFile.HighLbaEx);

        /* Store the extra information in the second byte of FIFO */
    }
    ATA_WRITE(PortData->Pata.Registers.Features, Request->TaskFile.Feature);
    ATA_WRITE(PortData->Pata.Registers.SectorCount, Request->TaskFile.SectorCount);
    ATA_WRITE(PortData->Pata.Registers.LbaLow, Request->TaskFile.LowLba);
    ATA_WRITE(PortData->Pata.Registers.LbaMid, Request->TaskFile.MidLba);
    ATA_WRITE(PortData->Pata.Registers.LbaHigh, Request->TaskFile.HighLba);
    if (Request->Flags & REQUEST_FLAG_SET_DEVICE_REGISTER)
    {
        ATA_WRITE(PortData->Pata.Registers.Device, Request->TaskFile.DriveSelect);
    }
    ATA_WRITE(PortData->Pata.Registers.Command, Request->TaskFile.Command);

    CommandFlags = (Request->Flags & (REQUEST_FLAG_DATA_IN | REQUEST_FLAG_DATA_OUT));
    CommandFlags |= ~Request->Flags & REQUEST_FLAG_POLL;

    /* DMA transfer */
    if (Request->Flags & REQUEST_FLAG_DMA)
    {
        PortData->Pata.CommandFlags = CommandFlags | CMD_FLAG_DMA_TRANSFER;

        /* Start the DMA engine */
        AtaPciIdeDmaStart(PortData, Request);

        /* Wait for an interrupt */
        return FALSE;
    }

    /* PIO transfer */
    PortData->Pata.CommandFlags = CommandFlags | CMD_FLAG_ATA_PIO_TRANSFER;

    /* Set the byte count per DRQ data block */
    if (Request->Flags & REQUEST_FLAG_READ_WRITE_MULTIPLE)
        BlockCount = Device->MultiSectorCount;
    else
        BlockCount = 1;
    PortData->Pata.DrqByteCount = BlockCount * Device->SectorSize;

    /*
     * For PIO writes we need to send the first data block
     * to make the device start generating interrupts.
     */
    if (Request->Flags & REQUEST_FLAG_DATA_OUT)
        return TRUE;

    /* Wait for an interrupt */
    return FALSE;
}

BOOLEAN
AtaPataStartIo(
    _In_ PATAPORT_PORT_DATA PortData,
    _In_ PATA_DEVICE_REQUEST Request)
{
    PATAPORT_IO_CONTEXT Device = Request->Device;
    UCHAR IdeStatus;
    BOOLEAN IsPollingNeeded;

    // TODO move this to the state machine

    /* Select the device */
    ATA_SELECT_DEVICE(PortData, Device->AtaScsiAddress.TargetId, Device->DeviceSelect);

    /* Wait for busy to clear */
    IdeStatus = ATA_READ(PortData->Pata.Registers.Status);
    if (!ATA_WAIT_ON_BUSY(&PortData->Pata.Registers, &IdeStatus, ATA_TIME_BUSY_SELECT))
    {
        WARN("Device is busy 0x%02x\n", IdeStatus);
        Request->SrbStatus = SRB_STATUS_SELECTION_TIMEOUT;

        AtaPataCompleteCommand(PortData, IdeStatus);
        return TRUE;
    }

    /* Disable interrupts for polled commands */
    if (!!(Request->Flags & REQUEST_FLAG_POLL) !=
        !!(PortData->PortFlags & PORT_FLAG_IN_POLL_STATE))
    {
        AtaPataChangeInterruptMode(PortData, Request);
    }

    /* Execute the command */
    if (Request->Flags & REQUEST_FLAG_PACKET_COMMAND)
        IsPollingNeeded = AtaPataExecutePacketCommand(PortData, Device, Request);
    else
        IsPollingNeeded = AtaPataExecuteAtaCommand(PortData, Device, Request);

    if (!IsPollingNeeded || (Request->Flags & REQUEST_FLAG_POLL))
        return FALSE;

    /* Need to wait for a valid status */
    ATA_IO_WAIT();

    /* Wait for busy to clear */
    IdeStatus = ATA_READ(PortData->Pata.Registers.Status);
    if (!ATA_WAIT_ON_BUSY(&PortData->Pata.Registers, &IdeStatus, ATA_TIME_BUSY_NORMAL))
    {
        ERR("BSY timeout, status 0x%02x\n", IdeStatus);
        Request->SrbStatus = SRB_STATUS_BUS_RESET;

        AtaPataCompleteCommand(PortData, IdeStatus);
        return TRUE;
    }

    return AtaPataProcessRequest(PortData, IdeStatus, 0);
}

VOID
AtaPataPrepareIo(
    _In_ PATAPORT_PORT_DATA PortData,
    _In_ PATA_DEVICE_REQUEST Request)
{
    /*
     * HACK: In an emulated PC-98 system IDE interrupts are often lost.
     * The reason for this problem is unknown (not the driver's fault).
     * Always poll for the command completion as a workaround.
     */
    if (IsNEC_98)
        Request->Flags |= REQUEST_FLAG_POLL;

    PortData->Pata.BytesToTransfer = Request->DataTransferLength;
    PortData->Pata.DataBuffer = Request->DataBuffer;
}

VOID
AtaPciIdePreparePrdTable(
    _In_ PATAPORT_PORT_DATA PortData,
    _In_ PATA_DEVICE_REQUEST Request,
    _In_ SCATTER_GATHER_LIST* __restrict SgList)
{
    PPCIIDE_PRD_TABLE_ENTRY PrdTableEntry;
    ULONG i;
#if DBG
    ULONG DescriptorNumber = 0;
#endif

    ASSUME(SgList->NumberOfElements > 0);

    PrdTableEntry = PortData->Pata.PciIdeInterface.PrdTable;

    for (i = 0; i < SgList->NumberOfElements; ++i)
    {
        ULONG Address = SgList->Elements[i].Address.LowPart;
        ULONG Length = SgList->Elements[i].Length;

        ASSUME(Length != 0);

        /* 32-bit DMA */
        ASSERT(SgList->Elements[i].Address.HighPart == 0);

        /* The address of memory region must be word aligned */
        ASSERT((Address % sizeof(USHORT)) == 0);

        while (Length > 0)
        {
            ULONG TransferLength;

            /* If the memory region would cross a 64 kB boundary, split it */
            TransferLength = PCIIDE_PRD_LIMIT - (Address & (PCIIDE_PRD_LIMIT - 1));
            TransferLength = min(TransferLength, Length);

            /* The byte count must be word aligned */
            ASSERT((TransferLength % sizeof(USHORT)) == 0);

            /* Make sure we do not write beyond the end of the PRD table */
            ASSERT(DescriptorNumber++ < PortData->Pata.PciIdeInterface.MapRegisterCount);

            PrdTableEntry->Address = Address;
            PrdTableEntry->Length = TransferLength & PCIIDE_PRD_LENGTH_MASK;

            ++PrdTableEntry;

            Address += TransferLength;
            Length -= TransferLength;
        }
    }

    /* The PRD table cannot cross a 64 kB boundary */
    ASSERT((DescriptorNumber * sizeof(*PrdTableEntry)) < PCIIDE_PRD_LIMIT);

    /* Last entry */
    --PrdTableEntry;
    PrdTableEntry->Length |= PCIIDE_PRD_END_OF_TABLE;

    KeFlushIoBuffers(Request->Mdl, !!(Request->Flags & REQUEST_FLAG_DATA_IN), TRUE);

    AtaPciIdeDmaPrepare(PortData);
}

BOOLEAN
NTAPI
AtaPciIdeChannelIsr(
    _In_ PKINTERRUPT Interrupt,
    _In_ PVOID Context)
{
    PATAPORT_PORT_DATA PortData = Context;
    UCHAR IdeStatus, DmaStatus;
    BOOLEAN Handled;

    UNREFERENCED_PARAMETER(Interrupt);

    DmaStatus = AtaPciIdeDmaReadStatus(PortData);
    if (DmaStatus & PCIIDE_DMA_STATUS_INTERRUPT)
    {
        /* Always clear the DMA interrupt, even when the current command is a PIO command */
        if ((PortData->Pata.CommandFlags & CMD_FLAG_TRANSFER_MASK) == CMD_FLAG_DMA_TRANSFER)
            AtaPciIdeDmaStop(PortData);
        else
            AtaPciIdeDmaClear(PortData, DmaStatus);

        Handled = TRUE;
    }
    else
    {
        if ((PortData->Pata.CommandFlags & CMD_FLAG_TRANSFER_MASK) == CMD_FLAG_DMA_TRANSFER)
            return FALSE;

        Handled = FALSE;
    }

    /* Acknowledge the IDE interrupt */
    IdeStatus = ATA_READ(PortData->Pata.Registers.Status);

    /* This interrupt is not ours */
    if (!(PortData->Pata.CommandFlags & CMD_FLAG_AWAIT_INTERRUPT) ||
        (IdeStatus & IDE_STATUS_BUSY))
    {
        /* Ignore PCI shared interrupts */
        if (Handled)
        {
            WARN("Spurious bus-master interrupt %02x, %02x, flags %08lx\n",
                 IdeStatus, DmaStatus, PortData->Pata.CommandFlags);
        }

        return Handled;
    }

    AtaPataProcessRequest(PortData, IdeStatus, DmaStatus);
    return TRUE;
}

BOOLEAN
NTAPI
AtaPataChannelIsr(
    _In_ PKINTERRUPT Interrupt,
    _In_ PVOID Context)
{
    PATAPORT_PORT_DATA PortData = Context;
    UCHAR IdeStatus;

    UNREFERENCED_PARAMETER(Interrupt);

    /* Acknowledge the IDE interrupt */
    IdeStatus = ATA_READ(PortData->Pata.Registers.Status);

    /* This interrupt is spurious or not ours */
    if (!(PortData->Pata.CommandFlags & CMD_FLAG_AWAIT_INTERRUPT) ||
        (IdeStatus & IDE_STATUS_BUSY))
    {
        WARN("Spurious IDE interrupt %02x, flags %08lx\n",
             IdeStatus, PortData->Pata.CommandFlags);
        return FALSE;
    }

    AtaPataProcessRequest(PortData, IdeStatus, 0);
    return TRUE;
}

VOID
AtaPataPoll(
    _In_ PATAPORT_PORT_DATA PortData)
{
    BOOLEAN IsRequestCompleted;
    UCHAR IdeStatus;

    while (TRUE)
    {
        /* Need to wait for a valid status */
        ATA_IO_WAIT();

        /* Do a quick check here for a busy */
        IdeStatus = ATA_READ(PortData->Pata.Registers.Status);
        if (!ATA_WAIT_ON_BUSY(&PortData->Pata.Registers, &IdeStatus, ATA_TIME_BUSY_POLL))
        {
            /* Device is still busy, schedule a timer to retry the poll */
            break;
        }

        IsRequestCompleted = AtaPataProcessRequest(PortData, IdeStatus, 0);
        if (IsRequestCompleted)
            break;
    }
}

BOOLEAN
AtaPataDmaTransferToPioTransfer(
    _In_ PATA_DEVICE_REQUEST Request)
{
    ASSERT(!IS_AHCI(Request->Device));
    ASSERT(Request->Flags & REQUEST_DMA_FLAGS);

    /* ATAPI commands */
    if (Request->Flags & REQUEST_FLAG_PACKET_COMMAND)
    {
        Request->Flags &= ~REQUEST_DMA_FLAGS;
        return TRUE;
    }

    /*
     * For ATA commands there's no simple way to achieve this,
     * we have to fix the command opcode.
     * Determine if it's safe or allowed to change the command.
     */
    if (Request->Flags & REQUEST_FLAG_READ_WRITE)
    {
        Request->Flags &= ~REQUEST_DMA_FLAGS;

        Request->TaskFile.Command = AtaReadWriteCommand(Request);
        if (Request->TaskFile.Command == 0)
        {
            /* PIO is not available */
            Request->Flags |= REQUEST_DMA_FLAGS;
            return FALSE;
        }

        return TRUE;
    }

    /* PIO is not available */
    return FALSE;
}
