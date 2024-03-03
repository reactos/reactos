/*
 * PROJECT:     ReactOS ATA Port Driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     PATA request handling
 * COPYRIGHT:   Copyright 2024 Dmitry Borisov <di.sean@protonmail.com>
 */

/* INCLUDES *******************************************************************/

#include "atapi.h"

/* FUNCTIONS ******************************************************************/

static
inline
UCHAR
AtaPciIdeDmaReadStatus(
    _In_ PATAPORT_PORT_DATA PortData)
{
    return ATA_READ(PortData->Pata.PciIdeInterface.IoBase + DMA_STATUS);
}

static
VOID
AtaPciIdeDmaPrepare(
    _In_ PATAPORT_PORT_DATA PortData)
{
    PUCHAR IoBase = PortData->Pata.PciIdeInterface.IoBase;

    /* Clear the interrupt and error bits in the status register */
    ATA_WRITE(IoBase + DMA_COMMAND, DMA_COMMAND_STOP);
    ATA_WRITE(IoBase + DMA_STATUS, DMA_STATUS_INTERRUPT | DMA_STATUS_ERROR);

    /* Set the address to the beginning of the physical region descriptor table */
    ATA_WRITE_ULONG((PULONG)(IoBase + DMA_PRDT_PHYSICAL_ADDRESS),
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
        Command = DMA_COMMAND_WRITE_TO_SYSTEM_MEMORY;
    else
        Command = DMA_COMMAND_READ_FROM_SYSTEM_MEMORY;

    /* Begin transaction */
    ATA_WRITE(PortData->Pata.PciIdeInterface.IoBase + DMA_COMMAND, Command | DMA_COMMAND_START);
}

static
VOID
AtaPciIdeDmaStop(
    _In_ PATAPORT_PORT_DATA PortData)
{
    PUCHAR IoBase = PortData->Pata.PciIdeInterface.IoBase;

    /* Wait one PIO transfer cycle */
    (VOID)ATA_READ(IoBase + DMA_STATUS);

    /* Clear the interrupt bit in the status register */
    ATA_WRITE(IoBase + DMA_COMMAND, DMA_COMMAND_STOP);
    ATA_WRITE(IoBase + DMA_STATUS, DMA_STATUS_INTERRUPT);
}

static
VOID
AtaPciIdeDmaClear(
    _In_ PATAPORT_PORT_DATA PortData,
    _In_ UCHAR DmaStatus)
{
    ATA_WRITE(PortData->Pata.PciIdeInterface.IoBase + DMA_STATUS, DmaStatus);
}

static
VOID
AtaPataSaveTaskFile(
    _In_ PATAPORT_PORT_DATA PortData,
    _Inout_ PATA_DEVICE_REQUEST Request)
{
    PATA_TASKFILE TaskFile = &Request->TaskFile;

    TaskFile->Error = ATA_READ(PortData->Pata.Registers.Error);
    TaskFile->SectorCount = ATA_READ(PortData->Pata.Registers.SectorCount);
    TaskFile->LowLba = ATA_READ(PortData->Pata.Registers.LbaLow);
    TaskFile->MidLba = ATA_READ(PortData->Pata.Registers.LbaMid);
    TaskFile->HighLba = ATA_READ(PortData->Pata.Registers.LbaHigh);
    TaskFile->DriveSelect = ATA_READ(PortData->Pata.Registers.Device);
    TaskFile->Command = ATA_READ(PortData->Pata.Registers.Command);

    if (Request->Flags & REQUEST_FLAG_LBA48)
    {
        UCHAR Control = ATA_READ(PortData->Pata.Registers.Control);

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

static
VOID
AtaPataSendCdb(
    _In_ PATAPORT_PORT_DATA PortData,
    _In_ PATA_DEVICE_REQUEST Request)
{
    /* Command packet transfer */
    if (!(Request->DevExt->CdbSize & (sizeof(ULONG) - 1)) &&
        (PortData->PortFlags & PORT_FLAG_IO32))
    {
        ATA_WRITE_BLOCK_32((PULONG)PortData->Pata.Registers.Data,
                           (PULONG)Request->Cdb,
                           Request->DevExt->CdbSize / sizeof(USHORT));
    }
    else
    {
        ATA_WRITE_BLOCK_16((PUSHORT)PortData->Pata.Registers.Data,
                           (PUSHORT)Request->Cdb,
                           Request->DevExt->CdbSize);
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
    PATA_DEVICE_REQUEST Request;
    UCHAR SrbStatus;

    ASSERT(!(IdeStatus & IDE_STATUS_BUSY));

    switch (PortData->Pata.CommandFlags & CMD_FLAG_TRANSFER_MASK)
    {
        case CMD_FLAG_DMA_TRANSFER:
        {
            if ((DmaStatus & DMA_STATUS_ERROR) ||
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
                    SrbStatus = SRB_STATUS_BUS_RESET;
                else
                    SrbStatus = SRB_STATUS_ERROR;
                break;
            }

            /* No data transfer */
            if (!(PortData->Pata.CommandFlags & (CMD_FLAG_DATA_IN | CMD_FLAG_DATA_OUT)))
            {
                if (IdeStatus & IDE_STATUS_DRQ)
                    SrbStatus = SRB_STATUS_BUS_RESET;
                else
                    SrbStatus = SRB_STATUS_SUCCESS;
                break;
            }

            if (PortData->Pata.CommandFlags & CMD_FLAG_DATA_IN)
            {
                if (!(IdeStatus & IDE_STATUS_DRQ))
                {
                    ERR("DRQ not set, status 0x%02x\n", IdeStatus);
                    SrbStatus = SRB_STATUS_BUS_RESET;
                    break;
                }

                /* Read the next data block */
                AtaPataPioDataIn(PortData, PortData->Pata.ByteCount);

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
                    SrbStatus = SRB_STATUS_SUCCESS;
                    break;
                }

                if (!(IdeStatus & IDE_STATUS_DRQ))
                {
                    ERR("DRQ not set, status 0x%02x %lu/%lu\n",
                        IdeStatus,
                        PortData->Pata.BytesToTransfer,
                        PortData->Slots[0]->DataTransferLength);
                    SrbStatus = SRB_STATUS_BUS_RESET;
                    break;
                }

                /* Write the next data block */
                AtaPataPioDataOut(PortData, PortData->Pata.ByteCount);
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

                    Request = PortData->Slots[0];
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
                    Request = PortData->Slots[0];

                    /* The NEC CDR-260 drive clears CoD and IO on command completion */
                    if (!(Request->DevExt->Flags & DEVICE_IS_NEC_CDR260))
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
                        SrbStatus = SRB_STATUS_ERROR;
                    else if (PortData->Pata.BytesToTransfer != 0)
                        SrbStatus = SRB_STATUS_DATA_OVERRUN;
                    else
                        SrbStatus = SRB_STATUS_SUCCESS;
                    break;
                }

                default:
                {
                    WARN("Invalid interrupt reason %02x %02x, %08lx, flags %08lx\n",
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

    Request = PortData->Slots[0];

    if (SrbStatus == SRB_STATUS_DATA_OVERRUN)
    {
        ASSERT(Request->DataTransferLength >= PortData->Pata.BytesToTransfer);
        Request->DataTransferLength -= PortData->Pata.BytesToTransfer;
    }
    else if (SrbStatus != SRB_STATUS_SUCCESS)
    {
        Request->Error = ATA_READ(PortData->Pata.Registers.Error);
    }
    Request->SrbStatus = SrbStatus;
    Request->Status = IdeStatus;

    /* Save the latest copy of the task file registers */
    if (Request->Flags & REQUEST_FLAG_SAVE_TASK_FILE)
    {
        AtaPataSaveTaskFile(PortData, Request);
    }

    PortData->ActiveSlotsBitmap &= ~(1 << 0);
    PortData->Pata.CommandFlags = CMD_FLAG_NONE;

    /* Complete the request */
    return TRUE;
}

BOOLEAN
AtaPataPreparePioDataTransfer(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ PATA_DEVICE_REQUEST Request)
{
    ASSERT(!IS_AHCI(DevExt));
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

        Request->TaskFile.Command = AtaReadWriteCommand(Request, Request->DevExt);
        if (Request->TaskFile.Command == 0)
        {
            Request->Flags |= REQUEST_DMA_FLAGS;
            return FALSE;
        }

        return TRUE;
    }

    /* PIO is not available */
    return FALSE;
}

BOOLEAN
AtaPataPoll(
    _In_ PATAPORT_PORT_DATA PortData)
{
    BOOLEAN RequestCompleted;
    UCHAR IdeStatus;

    while (TRUE)
    {
        /* Do a quick check here for a busy */
        IdeStatus = ATA_READ(PortData->Pata.Registers.Status);
        if (!ATA_WAIT_ON_BUSY(&PortData->Pata.Registers, &IdeStatus, ATA_TIME_BUSY_POLL))
        {
            /* Device is still busy, schedule a timer to retry the poll */
            RequestCompleted = FALSE;
            break;
        }

        RequestCompleted = AtaPataProcessRequest(PortData, IdeStatus, 0);
        if (RequestCompleted)
            break;

        /* Need to wait for a valid status */
        ATA_IO_WAIT();
    }

    return RequestCompleted;
}

VOID
NTAPI
AtaPataChannelDpc(
    _In_ PKDPC Dpc,
    _In_opt_ PVOID DeferredContext,
    _In_opt_ PVOID SystemArgument1,
    _In_opt_ PVOID SystemArgument2)
{
    PATAPORT_PORT_DATA PortData = DeferredContext;
    PATA_DEVICE_REQUEST Request = PortData->Slots[0];

    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(SystemArgument1);
    UNREFERENCED_PARAMETER(SystemArgument2);

    KeAcquireSpinLockAtDpcLevel(&PortData->PortLock);

    PortData->ActiveSlotsBitmap &= ~(1 << 0);

    if (Request->SrbStatus != SRB_STATUS_SUCCESS &&
        Request->SrbStatus != SRB_STATUS_DATA_OVERRUN)
    {
        AtaReqCompleteFailedRequest(Request);
        KeReleaseSpinLockFromDpcLevel(&PortData->PortLock);
        return;
    }

    KeReleaseSpinLockFromDpcLevel(&PortData->PortLock);

    AtaReqCompleteRequest(Request);
}

static
UCHAR
AtaPataExecutePacketCommand(
    _In_ PATAPORT_PORT_DATA PortData,
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ PATA_DEVICE_REQUEST Request)
{
    ULONG CommandFlags;
    USHORT ByteCount;
    UCHAR IdeStatus, Features;

    CommandFlags = (Request->Flags & REQUEST_FLAG_POLL) ^ REQUEST_FLAG_POLL;
    CommandFlags |= (Request->Flags & (REQUEST_FLAG_DATA_IN | REQUEST_FLAG_DATA_OUT));
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
        ByteCount = min(PortData->Pata.BytesToTransfer, MAXUSHORT - 1);
        Features = IDE_FEATURE_PIO;
    }
    ATA_WRITE(PortData->Pata.Registers.ByteCountLow, (UCHAR)ByteCount);
    ATA_WRITE(PortData->Pata.Registers.ByteCountHigh, ByteCount >> 8);
    ATA_WRITE(PortData->Pata.Registers.Features, Features);
    ATA_WRITE(PortData->Pata.Registers.Command, IDE_COMMAND_ATAPI_PACKET);

    /* Wait for an interrupt that signals that device is ready to accept the command packet */
    if (DevExt->Flags & DEVICE_HAS_CDB_INTERRUPT)
        return SRB_STATUS_PENDING;

    /* Need to wait for a valid status */
    ATA_IO_WAIT();

    IdeStatus = ATA_READ(PortData->Pata.Registers.Status);
    if (!ATA_WAIT_ON_BUSY(&PortData->Pata.Registers, &IdeStatus, ATA_TIME_BUSY_NORMAL))
    {
        ERR("BSY timeout, status 0x%02x\n", IdeStatus);

        PortData->Pata.CommandFlags = CMD_FLAG_NONE;
        return SRB_STATUS_BUS_RESET;
    }

    /* Command packet transfer */
    if (AtaPataProcessRequest(PortData, IdeStatus, 0))
        return Request->SrbStatus;

    return SRB_STATUS_PENDING;
}

static
UCHAR
AtaPataExecuteAtaCommand(
    _In_ PATAPORT_PORT_DATA PortData,
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ PATA_DEVICE_REQUEST Request)
{
    ULONG CommandFlags, BlockCount;
    UCHAR IdeStatus;

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

    CommandFlags = (Request->Flags & REQUEST_FLAG_POLL) ^ REQUEST_FLAG_POLL;
    CommandFlags |= (Request->Flags & (REQUEST_FLAG_DATA_IN | REQUEST_FLAG_DATA_OUT));

    /* DMA transfer */
    if (Request->Flags & REQUEST_FLAG_DMA)
    {
        PortData->Pata.CommandFlags = CommandFlags | CMD_FLAG_DMA_TRANSFER;

        /* Start the DMA engine */
        AtaPciIdeDmaStart(PortData, Request);

        /* Wait for an interrupt */
        return SRB_STATUS_PENDING;
    }

    /* PIO transfer */
    PortData->Pata.CommandFlags = CommandFlags | CMD_FLAG_ATA_PIO_TRANSFER;

    /* Byte count per DRQ data block */
    if (Request->Flags & REQUEST_FLAG_READ_WRITE_MULTIPLE)
        BlockCount = Request->DevExt->MultiSectorTransfer;
    else
        BlockCount = 1;
    PortData->Pata.ByteCount = BlockCount * Request->DevExt->SectorSize;

    /*
     * For PIO writes we need to send the first data block
     * to make the device start generating interrupts.
     */
    if ((Request->Flags & REQUEST_FLAG_DATA_OUT) && !(Request->Flags & REQUEST_FLAG_POLL))
    {
        /* Need to wait for a valid status */
        ATA_IO_WAIT();

        IdeStatus = ATA_READ(PortData->Pata.Registers.Status);
        if (!ATA_WAIT_ON_BUSY(&PortData->Pata.Registers, &IdeStatus, ATA_TIME_BUSY_NORMAL))
        {
            ERR("BSY timeout, status 0x%02x\n", IdeStatus);

            PortData->Pata.CommandFlags = CMD_FLAG_NONE;
            return SRB_STATUS_BUS_RESET;
        }

        if (AtaPataProcessRequest(PortData, IdeStatus, 0))
            return Request->SrbStatus;
    }

    return SRB_STATUS_PENDING;
}

static
UCHAR
AtaPataExecuteCommand(
    _In_ PATAPORT_PORT_DATA PortData,
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ PATA_DEVICE_REQUEST Request)
{
    UCHAR IdeStatus;

    /* Select the device */
    ATA_SELECT_DEVICE(PortData, DevExt, DevExt->DeviceSelect);

    /* Wait for busy to clear */
    IdeStatus = ATA_READ(PortData->Pata.Registers.Status);
    ATA_WAIT_ON_BUSY(&PortData->Pata.Registers, &IdeStatus, ATA_TIME_BUSY_SELECT);
    if (IdeStatus & IDE_STATUS_BUSY)
        return SRB_STATUS_SELECTION_TIMEOUT;

    /* Execute the command */
    if (Request->Flags & REQUEST_FLAG_PACKET_COMMAND)
        return AtaPataExecutePacketCommand(PortData, DevExt, Request);
    else
        return AtaPataExecuteAtaCommand(PortData, DevExt, Request);
}

UCHAR
AtaPataStartIo(
    _In_ PATAPORT_PORT_DATA PortData,
    _In_ PATA_DEVICE_REQUEST Request)
{
    PATAPORT_DEVICE_EXTENSION DevExt = Request->DevExt;
    PATAPORT_CHANNEL_EXTENSION ChanExt = DevExt->ChanExt;
    KIRQL OldIrql;
    UCHAR SrbStatus;

    // TODO hack
    Request->Flags &= ~REQUEST_FLAG_POLL;

    /* The IDE interface has one slot per channel */
    PortData->TimerCount[0] = Request->TimeOut;
    PortData->ActiveSlotsBitmap |= 1 << 0;

    PortData->Pata.BytesToTransfer = Request->DataTransferLength;
    PortData->Pata.DataBuffer = Request->DataBuffer;

    OldIrql = KeAcquireInterruptSpinLock(ChanExt->InterruptObject);

    SrbStatus = AtaPataExecuteCommand(PortData, DevExt, Request);

    KeReleaseInterruptSpinLock(ChanExt->InterruptObject, OldIrql);

    return SrbStatus;
}

VOID
AtaPciIdePreparePrdTable(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ PATA_DEVICE_REQUEST Request,
    _In_ SCATTER_GATHER_LIST* __restrict SgList)
{
    PPRD_TABLE_ENTRY PrdTableEntry;
    ULONG i;

    ASSUME(SgList->NumberOfElements > 0);

    PrdTableEntry = DevExt->PortData->Pata.PciIdeInterface.PrdTable;

    for (i = 0; i < SgList->NumberOfElements; ++i)
    {
        ULONG Address = SgList->Elements[i].Address.LowPart;
        ULONG Length = SgList->Elements[i].Length;

        /* Alignment */
        ASSERT((Address % sizeof(USHORT)) == 0);

        /* 32-bit DMA */
        ASSERT(SgList->Elements[i].Address.HighPart == 0);

        while (Length > 0)
        {
            ULONG TransferLength;

            if (((Address & PRD_LENGTH_MASK) + Length) > PRD_LIMIT)
            {
                TransferLength = PRD_LIMIT - (Address & PRD_LENGTH_MASK);
            }
            else
            {
                TransferLength = Length;
            }

            /* Bit 0 is reserved */
            ASSERT(!(TransferLength & 1));

            PrdTableEntry->Address = Address;
            PrdTableEntry->Length = TransferLength & PRD_LENGTH_MASK;

            ++PrdTableEntry;

            Address += TransferLength;
            Length -= TransferLength;
        }
    }

    /* Last entry */
    --PrdTableEntry;
    PrdTableEntry->Length |= PRD_END_OF_TABLE;

    KeFlushIoBuffers(Request->Mdl, !!(Request->Flags & REQUEST_FLAG_DATA_IN), TRUE);

    AtaPciIdeDmaPrepare(DevExt->PortData);
}

BOOLEAN
AtaPataResetDevice(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt)
{
    // TODO
#if 0
    ATA_WRITE(Registers->Control, IDE_DC_RESET_CONTROLLER);
    KeStallExecutionProcessor(20);
    ATA_WRITE(Registers->Control, IDE_DC_DISABLE_INTERRUPTS);
    KeStallExecutionProcessor(20);
#endif

    return TRUE;
}

BOOLEAN
AtaPataEnumerateDevice(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt)
{
    PATAPORT_PORT_DATA PortData = DevExt->PortData;
    UCHAR IdeStatus;

    /* Select the device */
    ATA_SELECT_DEVICE(PortData, DevExt, DevExt->DeviceSelect);

    /* Do a quick check first */
    IdeStatus = ATA_READ(PortData->Pata.Registers.Status);
    if (IdeStatus == 0xFF || IdeStatus == 0x7F || IdeStatus == 0)
    {
        DevExt->WorkerContext.ConnectionStatus = CONN_STATUS_NO_DEVICE;
        return TRUE;
    }

    /* Look at controller */
    ATA_WRITE(PortData->Pata.Registers.ByteCountLow, 0x55);
    ATA_WRITE(PortData->Pata.Registers.ByteCountLow, 0xAA);
    ATA_WRITE(PortData->Pata.Registers.ByteCountLow, 0x55);
    if (ATA_READ(PortData->Pata.Registers.ByteCountLow) != 0x55)
    {
        DevExt->WorkerContext.ConnectionStatus = CONN_STATUS_NO_DEVICE;
        return TRUE;
    }
    ATA_WRITE(PortData->Pata.Registers.ByteCountHigh, 0xAA);
    ATA_WRITE(PortData->Pata.Registers.ByteCountHigh, 0x55);
    ATA_WRITE(PortData->Pata.Registers.ByteCountHigh, 0xAA);
    if (ATA_READ(PortData->Pata.Registers.ByteCountHigh) != 0xAA)
    {
        DevExt->WorkerContext.ConnectionStatus = CONN_STATUS_NO_DEVICE;
        return TRUE;
    }

    /* Wait for busy to clear */
    ATA_WAIT_ON_BUSY(&PortData->Pata.Registers, &IdeStatus, ATA_TIME_BUSY_ENUM);
    if (IdeStatus & IDE_STATUS_BUSY)
    {
        WARN("Device is busy %02x\n", IdeStatus);
        return FALSE;
    }

    DevExt->WorkerContext.ConnectionStatus = CONN_STATUS_DEV_UNKNOWN;
    return TRUE;
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
    if (DmaStatus & DMA_STATUS_INTERRUPT)
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
        if (Handled)
        {
            WARN("Spurious bus-master interrupt %02x, %02x, flags %08lx\n",
                 IdeStatus, DmaStatus, PortData->Pata.CommandFlags);
        }

        return Handled;
    }

    if (AtaPataProcessRequest(PortData, IdeStatus, DmaStatus))
        KeInsertQueueDpc(&PortData->Pata.Dpc, NULL, NULL);

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

    if (AtaPataProcessRequest(PortData, IdeStatus, 0))
        KeInsertQueueDpc(&PortData->Pata.Dpc, NULL, NULL);

    return TRUE;
}
