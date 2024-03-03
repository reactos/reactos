/*
 * PROJECT:     ReactOS ATA Bus Driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     PATA I/O request handling
 * COPYRIGHT:   Copyright 2025 Dmitry Borisov <di.sean@protonmail.com>
 */

/* INCLUDES *******************************************************************/

#include "pciidex.h"

/* FUNCTIONS ******************************************************************/

VOID
PataSaveTaskFile(
    _In_ CHANNEL_DATA_PATA* __restrict ChanData,
    _Inout_ ATA_DEVICE_REQUEST* __restrict Request)
{
    PATA_TASKFILE TaskFile = &Request->Output;

    TaskFile->Error = ATA_READ(ChanData->Regs.Error);
    TaskFile->SectorCount = ATA_READ(ChanData->Regs.SectorCount);
    TaskFile->LowLba = ATA_READ(ChanData->Regs.LbaLow);
    TaskFile->MidLba = ATA_READ(ChanData->Regs.LbaMid);
    TaskFile->HighLba = ATA_READ(ChanData->Regs.LbaHigh);
    TaskFile->DriveSelect = ATA_READ(ChanData->Regs.Device);
    TaskFile->Command = ATA_READ(ChanData->Regs.Command);

    if (Request->Flags & REQUEST_FLAG_LBA48)
    {
        UCHAR Control = ATA_READ(ChanData->Regs.Control) | IDE_DC_ALWAYS;

        /* Read the extra information from the second byte of FIFO */
        ATA_WRITE(ChanData->Regs.Control, Control | IDE_HIGH_ORDER_BYTE);

        TaskFile->FeatureEx = ATA_READ(ChanData->Regs.Features);
        TaskFile->SectorCountEx = ATA_READ(ChanData->Regs.SectorCount);
        TaskFile->LowLbaEx = ATA_READ(ChanData->Regs.LbaLow);
        TaskFile->MidLbaEx = ATA_READ(ChanData->Regs.LbaMid);
        TaskFile->HighLbaEx = ATA_READ(ChanData->Regs.LbaHigh);

        ATA_WRITE(ChanData->Regs.Control, Control);
    }
}

UCHAR
PataReadStatus(
    _In_ PCHANNEL_DATA_PATA ChanData)
{
    return ATA_READ(ChanData->Regs.Status);
}

static
VOID
PataCompleteCommand(
    _In_ PCHANNEL_DATA_PATA ChanData,
    _In_ UCHAR IdeStatus,
    _In_ UCHAR SrbStatus)
{
    PATA_DEVICE_REQUEST Request = ChanData->Slots[PATA_CHANNEL_SLOT];

    Request->SrbStatus = SrbStatus;

    /* Handle failed commands */
    if (SrbStatus != SRB_STATUS_SUCCESS && SrbStatus != SRB_STATUS_DATA_OVERRUN)
    {
        if (!(Request->Device->TransportFlags & DEVICE_IS_ATAPI) ||
            (Request->Flags & REQUEST_FLAG_SAVE_TASK_FILE))
        {
            /* Save the current task file for the "ATA LBA field" (SAT-6 11.7) */
            ChanData->SaveTaskFile(ChanData, Request);
            Request->Flags |= REQUEST_FLAG_HAS_TASK_FILE;
        }
        else
        {
            Request->Output.Status = IdeStatus;
            Request->Output.Error = ATA_READ(ChanData->Regs.Error);
        }

        /* Request arbitration from the port worker */
        ChanData->PortNotification(AtaRequestFailed, ChanData->PortContext, Request);
        return;
    }

    ChanData->ActiveSlotsBitmap &= ~(1 << PATA_CHANNEL_SLOT);

    /* Save the latest copy of the task file registers */
    if (Request->Flags & REQUEST_FLAG_SAVE_TASK_FILE)
    {
        ChanData->SaveTaskFile(ChanData, Request);
        Request->Flags |= REQUEST_FLAG_HAS_TASK_FILE;
    }

    ChanData->PortNotification(AtaRequestComplete, ChanData->PortContext, 1 << PATA_CHANNEL_SLOT);
}

static
inline
UCHAR
PciIdeDmaReadStatus(
    _In_ PCHANNEL_DATA_PATA ChanData)
{
    return ATA_READ(ChanData->Regs.Dma + PCIIDE_DMA_STATUS);
}

static
VOID
PciIdeDmaPrepare(
    _In_ PCHANNEL_DATA_PATA ChanData)
{
    PUCHAR IoBase = ChanData->Regs.Dma;

    /* Clear the interrupt and error bits in the status register */
    ATA_WRITE(IoBase + PCIIDE_DMA_COMMAND, PCIIDE_DMA_COMMAND_STOP);
    ATA_WRITE(IoBase + PCIIDE_DMA_STATUS, PCIIDE_DMA_STATUS_INTERRUPT | PCIIDE_DMA_STATUS_ERROR);

    /* Set the address to the beginning of the physical region descriptor table */
    ATA_WRITE_ULONG((PULONG)(IoBase + PCIIDE_DMA_PRDT_PHYSICAL_ADDRESS),
                    ChanData->PrdTablePhysicalAddress);
}

static
VOID
PciIdeDmaStart(
    _In_ PCHANNEL_DATA_PATA ChanData,
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
    ATA_WRITE(ChanData->Regs.Dma + PCIIDE_DMA_COMMAND, Command);
}

VOID
PciIdeDmaStop(
    _In_ PCHANNEL_DATA_PATA ChanData)
{
    PUCHAR IoBase = ChanData->Regs.Dma;

    /* Wait one PIO transfer cycle */
    (VOID)ATA_READ(IoBase + PCIIDE_DMA_STATUS);

    /* Clear the interrupt bit in the status register */
    ATA_WRITE(IoBase + PCIIDE_DMA_COMMAND, PCIIDE_DMA_COMMAND_STOP);
    ATA_WRITE(IoBase + PCIIDE_DMA_STATUS, PCIIDE_DMA_STATUS_INTERRUPT);
}

static
VOID
PciIdeDmaClearInterrupt(
    _In_ PCHANNEL_DATA_PATA ChanData,
    _In_ UCHAR DmaStatus)
{
    ATA_WRITE(ChanData->Regs.Dma + PCIIDE_DMA_STATUS, DmaStatus);
}

static
VOID
PataChangeInterruptMode(
    _In_ PCHANNEL_DATA_PATA ChanData,
    _In_ PATA_IO_CONTEXT_COMMON Device,
    _In_ PATA_DEVICE_REQUEST Request)
{
    UCHAR Control;

    if (Request->Flags & REQUEST_FLAG_POLL)
    {
        ChanData->IsPollingActive = TRUE;
        Control = IDE_DC_DISABLE_INTERRUPTS;
    }
    else
    {
        ChanData->IsPollingActive = FALSE;
        Control = 0;
    }
    TRACE("Interrupt mode %s\n", Control ? "disable" : "enable");

    ATA_WRITE(ChanData->Regs.Control, Control | IDE_DC_ALWAYS);

    /* Seems to be needed */
    ATA_IO_WAIT();

    /* Workaround for VIA chipsets that reset the DEV bit after interrupt mode change */
    if (ChanData->Controller->Pci.VendorID == PCI_VEN_VIA)
    {
        ATA_SELECT_DEVICE(ChanData, DEV_NUMBER(Device), Device->DeviceSelect);
        ATA_WAIT(ChanData, ATA_TIME_BUSY_SELECT, IDE_STATUS_BUSY, 0);
    }
}

static
VOID
PataSendCdb(
    _In_ PCHANNEL_DATA_PATA ChanData,
    _In_ PATA_DEVICE_REQUEST Request)
{
    /* Command packet transfer */
    if (ChanData->ChanInfo & CHANNEL_FLAG_IO32)
    {
        ATA_WRITE_BLOCK_32((PULONG)ChanData->Regs.Data,
                           (PULONG)Request->Cdb,
                           Request->Device->CdbSize / sizeof(USHORT));
    }
    else
    {
        ATA_WRITE_BLOCK_16((PUSHORT)ChanData->Regs.Data,
                           (PUSHORT)Request->Cdb,
                           Request->Device->CdbSize);
    }
}

static
VOID
PataPioDataIn(
    _In_ PCHANNEL_DATA_PATA ChanData,
    _In_ ULONG ByteCount)
{
    ByteCount = min(ByteCount, ChanData->BytesToTransfer);

    /* Transfer the data block */
    if (!(ByteCount & (sizeof(ULONG) - 1)) &&
        (ChanData->ChanInfo & CHANNEL_FLAG_IO32))
    {
        ATA_READ_BLOCK_32((PULONG)ChanData->Regs.Data,
                          (PULONG)ChanData->DataBuffer,
                          ByteCount / sizeof(ULONG));
    }
    else
    {
        ATA_READ_BLOCK_16((PUSHORT)ChanData->Regs.Data,
                          (PUSHORT)ChanData->DataBuffer,
                          ByteCount / sizeof(USHORT));

        /* Read one last byte */
        if (ByteCount & (sizeof(USHORT) - 1))
        {
            DECLSPEC_ALIGN(2) UCHAR LocalBuffer[2];
            PUCHAR BufferEnd;

            /* Some hardware (Intel SCH) always expect 16- or 32-bit accesses for the data port */
            ATA_READ_BLOCK_16((PUSHORT)ChanData->Regs.Data, (PUSHORT)LocalBuffer, 1);

            BufferEnd = ChanData->DataBuffer + ByteCount - 1;
            *BufferEnd = LocalBuffer[0];
        }
    }

    ChanData->DataBuffer += ByteCount;
    ChanData->BytesToTransfer -= ByteCount;
}

static
VOID
PataPioDataOut(
    _In_ PCHANNEL_DATA_PATA ChanData,
    _In_ ULONG ByteCount)
{
    ByteCount = min(ByteCount, ChanData->BytesToTransfer);

    /* Transfer the data block */
    if (!(ByteCount & (sizeof(ULONG) - 1)) &&
        (ChanData->ChanInfo & CHANNEL_FLAG_IO32))
    {
        ATA_WRITE_BLOCK_32((PULONG)ChanData->Regs.Data,
                           (PULONG)ChanData->DataBuffer,
                           ByteCount / sizeof(ULONG));
    }
    else
    {
        ATA_WRITE_BLOCK_16((PUSHORT)ChanData->Regs.Data,
                           (PUSHORT)ChanData->DataBuffer,
                           ByteCount / sizeof(USHORT));

        /* Write one last byte */
        if (ByteCount & (sizeof(USHORT) - 1))
        {
            DECLSPEC_ALIGN(2) UCHAR LocalBuffer[2];
            PUCHAR BufferEnd;

            BufferEnd = ChanData->DataBuffer + ByteCount - 1;
            LocalBuffer[0] = *BufferEnd;
            LocalBuffer[1] = 0;

            ATA_WRITE_BLOCK_16((PUSHORT)ChanData->Regs.Data, (PUSHORT)LocalBuffer, 1);
        }
    }

    ChanData->DataBuffer += ByteCount;
    ChanData->BytesToTransfer -= ByteCount;
}

static
UCHAR
PataProcessAtapiRequest(
    _In_ PCHANNEL_DATA_PATA ChanData,
    _In_ PATA_DEVICE_REQUEST Request,
    _In_ UCHAR IdeStatus)
{
    UCHAR SrbStatus, InterruptReason;
    ULONG ByteCount;

    InterruptReason = ATA_READ(ChanData->Regs.InterruptReason);
    InterruptReason &= ATAPI_INT_REASON_MASK;
    InterruptReason |= IdeStatus & IDE_STATUS_DRQ;

    /*
     * The NEC CDR-C251 drive is not fully ATAPI-compliant
     * and clears BSY before raising the DRQ bit and updating the interrupt reason register.
     * As a workaround, we will wait a bit more in the case the valid IR is not quite there yet.
     */
    if ((InterruptReason == ATAPI_INT_REASON_COD) || (InterruptReason == ATAPI_INT_REASON_IO))
    {
#if DBG
        static SIZE_T WarningsGiven = 0;
        if (WarningsGiven++ < 2)
            WARN("Not fully compliant ATAPI device %02x %02x\n", IdeStatus, InterruptReason);
#endif
        IdeStatus = ATA_WAIT(ChanData, ATA_TIME_DRQ_ASSERT, IDE_STATUS_DRQ, IDE_STATUS_DRQ);

        InterruptReason = ATA_READ(ChanData->Regs.InterruptReason);
        InterruptReason &= ATAPI_INT_REASON_MASK;
        InterruptReason |= IdeStatus & IDE_STATUS_DRQ;
    }

    switch (InterruptReason)
    {
        case ATAPI_INT_REASON_AWAIT_CDB:
        {
            if (!(ChanData->CommandFlags & CMD_FLAG_AWAIT_CDB))
            {
                WARN("Invalid interrupt reason %02x %02x, flags %08lx\n",
                     InterruptReason,
                     IdeStatus,
                     ChanData->CommandFlags);

                SrbStatus = SRB_STATUS_BUS_RESET;
                break;
            }
            ChanData->CommandFlags &= ~CMD_FLAG_AWAIT_CDB;

            PataSendCdb(ChanData, Request);

            /* Start the DMA engine */
            if (Request->Flags & REQUEST_FLAG_DMA)
            {
                ChanData->CommandFlags |= CMD_FLAG_DMA_TRANSFER;
                PciIdeDmaStart(ChanData, Request);
            }

            SrbStatus = SRB_STATUS_PENDING;
            break;
        }

        case ATAPI_INT_REASON_DATA_IN:
        {
            if (ChanData->CommandFlags & (CMD_FLAG_DATA_OUT | CMD_FLAG_AWAIT_CDB))
            {
                WARN("Invalid interrupt reason %02x %02x, flags %08lx\n",
                     InterruptReason,
                     IdeStatus,
                     ChanData->CommandFlags);

                SrbStatus = SRB_STATUS_BUS_RESET;
                break;
            }

            ByteCount = ATA_READ(ChanData->Regs.ByteCountLow);
            ByteCount |= ATA_READ(ChanData->Regs.ByteCountHigh) << 8;

            PataPioDataIn(ChanData, ByteCount);
            SrbStatus = SRB_STATUS_PENDING;
            break;
        }

        case ATAPI_INT_REASON_DATA_OUT:
        {
            if (ChanData->CommandFlags & (CMD_FLAG_DATA_IN | CMD_FLAG_AWAIT_CDB))
            {
                WARN("Invalid interrupt reason %02x %02x, flags %08lx\n",
                     InterruptReason,
                     IdeStatus,
                     ChanData->CommandFlags);

                SrbStatus = SRB_STATUS_BUS_RESET;
                break;
            }

            ByteCount = ATA_READ(ChanData->Regs.ByteCountLow);
            ByteCount |= ATA_READ(ChanData->Regs.ByteCountHigh) << 8;

            PataPioDataOut(ChanData, ByteCount);
            SrbStatus = SRB_STATUS_PENDING;
            break;
        }

        case ATAPI_INT_REASON_STATUS_NEC:
        {
            /* The NEC CDR-260 drive always clears CoD and IO on command completion */
            if (!(Request->Device->TransportFlags & DEVICE_IS_NEC_CDR260))
            {
                WARN("Invalid interrupt reason %02x %02x, flags %08lx\n",
                     InterruptReason,
                     IdeStatus,
                     ChanData->CommandFlags);

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
            else if (ChanData->BytesToTransfer != 0)
            {
                /* This indicates a residual underrun */
                ASSERT(Request->DataTransferLength >= ChanData->BytesToTransfer);
                Request->DataTransferLength -= ChanData->BytesToTransfer;

                SrbStatus = SRB_STATUS_DATA_OVERRUN;
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
                 ChanData->CommandFlags);

            SrbStatus = SRB_STATUS_BUS_RESET;
            break;
        }
    }

    return SrbStatus;
}

static
UCHAR
PataProcessAtaRequest(
    _In_ PCHANNEL_DATA_PATA ChanData,
    _In_ PATA_DEVICE_REQUEST Request,
    _In_ UCHAR IdeStatus)
{
    /* Check for errors */
    if (IdeStatus & (IDE_STATUS_ERROR | IDE_STATUS_DEVICE_FAULT))
    {
        if (IdeStatus & IDE_STATUS_DRQ)
        {
            ERR("DRQ not cleared, status 0x%02x\n", IdeStatus);
            return SRB_STATUS_BUS_RESET;
        }
        else
        {
            return SRB_STATUS_ERROR;
        }
    }

    /* Non-data ATA command */
    if (!(ChanData->CommandFlags & (CMD_FLAG_DATA_IN | CMD_FLAG_DATA_OUT)))
    {
        // HACK: NP21/W IDE emulation bug, needs to be fixed on the emulator side
        if (IsNEC_98)
            IdeStatus &= ~IDE_STATUS_DRQ;

        if (IdeStatus & IDE_STATUS_DRQ)
        {
            ERR("DRQ not cleared, status 0x%02x\n", IdeStatus);
            return SRB_STATUS_BUS_RESET;
        }
        else
        {
            return SRB_STATUS_SUCCESS;
        }
    }

    /* Read command */
    if (ChanData->CommandFlags & CMD_FLAG_DATA_IN)
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
                return SRB_STATUS_ERROR;
            }
            else
            {
#if DBG
                static SIZE_T WarningsGiven = 0;
                if (WarningsGiven++ < 2)
                    WARN("Not fully compliant ATA device %02x\n", IdeStatus);
#endif

                /*
                 * The NEC CDR-C251 drive clears BSY before raising the DRQ bit
                 * while processing the ATAPI identify command.
                 * Give the device a chance to assert that bit.
                 */
                IdeStatus = ATA_WAIT(ChanData,
                                     ATA_TIME_DRQ_ASSERT,
                                     IDE_STATUS_DRQ,
                                     IDE_STATUS_DRQ);
                if (!(IdeStatus & IDE_STATUS_DRQ))
                {

                    ERR("DRQ not set, status 0x%02x\n", IdeStatus);
                    return SRB_STATUS_BUS_RESET;
                }
            }
        }

        /* Read the next data block */
        PataPioDataIn(ChanData, ChanData->DrqByteCount);

        if (ChanData->BytesToTransfer != 0)
            return SRB_STATUS_PENDING;

        /* All data has been transferred, wait for DRQ to clear */
        IdeStatus = ATA_WAIT(ChanData,
                             ATA_TIME_DRQ_CLEAR,
                             IDE_STATUS_BUSY | IDE_STATUS_DRQ,
                             0);
        if (IdeStatus & (IDE_STATUS_BUSY | IDE_STATUS_DRQ))
        {
            ERR("DRQ not cleared, status 0x%02x\n", IdeStatus);
            return SRB_STATUS_BUS_RESET;
        }

        if (IdeStatus & (IDE_STATUS_ERROR | IDE_STATUS_DEVICE_FAULT))
            return SRB_STATUS_ERROR;

        return SRB_STATUS_SUCCESS;
    }

    /* Write command */
    if (ChanData->BytesToTransfer == 0)
    {
        if (IdeStatus & IDE_STATUS_DRQ)
        {
            ERR("DRQ not cleared, status 0x%02x\n", IdeStatus);
            return SRB_STATUS_BUS_RESET;
        }
        else
        {
            return SRB_STATUS_SUCCESS;
        }
    }

    if (!(IdeStatus & IDE_STATUS_DRQ))
    {
        ERR("DRQ not set, status 0x%02x %lu/%lu\n",
            IdeStatus,
            ChanData->BytesToTransfer,
            Request->DataTransferLength);
        return SRB_STATUS_BUS_RESET;
    }

    /* Write the next data block */
    PataPioDataOut(ChanData, ChanData->DrqByteCount);
    return SRB_STATUS_PENDING;
}

static
BOOLEAN
PataProcessRequest(
    _In_ PCHANNEL_DATA_PATA ChanData,
    _In_ UCHAR IdeStatus,
    _In_ UCHAR DmaStatus)
{
    PATA_DEVICE_REQUEST Request = ChanData->Slots[PATA_CHANNEL_SLOT];
    UCHAR SrbStatus;

    ASSERT(!(IdeStatus & IDE_STATUS_BUSY));

    switch (ChanData->CommandFlags & CMD_FLAG_TRANSFER_MASK)
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
            SrbStatus = PataProcessAtaRequest(ChanData, Request, IdeStatus);
            break;
        }

        case CMD_FLAG_ATAPI_PIO_TRANSFER:
        {
            SrbStatus = PataProcessAtapiRequest(ChanData, Request, IdeStatus);
            break;
        }

        default:
        {
            ASSERT(FALSE);
            UNREACHABLE;
        }
    }

    if (SrbStatus == SRB_STATUS_PENDING)
        return FALSE;

    PataCompleteCommand(ChanData, IdeStatus, SrbStatus);
    return TRUE;
}

VOID
PataLoadTaskFile(
    _In_ CHANNEL_DATA_PATA* __restrict ChanData,
    _In_ ATA_DEVICE_REQUEST* __restrict Request)
{
    if (Request->Flags & REQUEST_FLAG_LBA48)
    {
        ATA_WRITE(ChanData->Regs.Features, Request->TaskFile.FeatureEx);
        ATA_WRITE(ChanData->Regs.SectorCount, Request->TaskFile.SectorCountEx);
        ATA_WRITE(ChanData->Regs.LbaLow, Request->TaskFile.LowLbaEx);
        ATA_WRITE(ChanData->Regs.LbaMid, Request->TaskFile.MidLbaEx);
        ATA_WRITE(ChanData->Regs.LbaHigh, Request->TaskFile.HighLbaEx);

        /* Store the extra information in the second byte of FIFO: */
    }
    ATA_WRITE(ChanData->Regs.Features, Request->TaskFile.Feature);
    ATA_WRITE(ChanData->Regs.SectorCount, Request->TaskFile.SectorCount);
    ATA_WRITE(ChanData->Regs.LbaLow, Request->TaskFile.LowLba);
    ATA_WRITE(ChanData->Regs.LbaMid, Request->TaskFile.MidLba);
    ATA_WRITE(ChanData->Regs.LbaHigh, Request->TaskFile.HighLba);

    if (Request->Flags & REQUEST_FLAG_SET_DEVICE_REGISTER)
    {
        ATA_WRITE(ChanData->Regs.Device, Request->TaskFile.DriveSelect);
    }
}

static
BOOLEAN
PataExecutePacketCommand(
    _In_ PCHANNEL_DATA_PATA ChanData,
    _In_ PATA_IO_CONTEXT_COMMON Device,
    _In_ PATA_DEVICE_REQUEST Request)
{
    ULONG CommandFlags;
    USHORT ByteCount;
    UCHAR Features;

    CommandFlags = (Request->Flags & (REQUEST_FLAG_DATA_IN | REQUEST_FLAG_DATA_OUT));
    CommandFlags |= ~Request->Flags & REQUEST_FLAG_POLL;
    CommandFlags |= CMD_FLAG_ATAPI_PIO_TRANSFER | CMD_FLAG_AWAIT_CDB;
    ChanData->CommandFlags = CommandFlags;

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
        ByteCount = min(ChanData->BytesToTransfer, ATAPI_MAX_DRQ_DATA_BLOCK);
        Features = IDE_FEATURE_PIO;
    }
    ATA_WRITE(ChanData->Regs.ByteCountLow, (UCHAR)(ByteCount >> 0));
    ATA_WRITE(ChanData->Regs.ByteCountHigh, (UCHAR)(ByteCount >> 8));
    ATA_WRITE(ChanData->Regs.Features, Features);
    ATA_WRITE(ChanData->Regs.Command, IDE_COMMAND_ATAPI_PACKET);

    /* Wait for an interrupt that signals that device is ready to accept the command packet */
    if (Device->TransportFlags & DEVICE_HAS_CDB_INTERRUPT)
        return FALSE;

    return TRUE;
}

static
BOOLEAN
PataExecuteAtaCommand(
    _In_ PCHANNEL_DATA_PATA ChanData,
    _In_ PATA_IO_CONTEXT_COMMON Device,
    _In_ PATA_DEVICE_REQUEST Request)
{
    ULONG CommandFlags, BlockCount;

    if ((ChanData->ChanInfo & CHANNEL_FLAG_DMA_BEFORE_CMD) && (Request->Flags & REQUEST_FLAG_DMA))
        PciIdeDmaStart(ChanData, Request);

    ChanData->LoadTaskFile(ChanData, Request);

    ATA_WRITE(ChanData->Regs.Command, Request->TaskFile.Command);

    CommandFlags = (Request->Flags & (REQUEST_FLAG_DATA_IN | REQUEST_FLAG_DATA_OUT));
    CommandFlags |= ~Request->Flags & REQUEST_FLAG_POLL;

    /* DMA transfer */
    if (Request->Flags & REQUEST_FLAG_DMA)
    {
        ChanData->CommandFlags = CommandFlags | CMD_FLAG_DMA_TRANSFER;

        /* Start the DMA engine */
        PciIdeDmaStart(ChanData, Request);
        return FALSE;
    }

    /* PIO transfer */
    ChanData->CommandFlags = CommandFlags | CMD_FLAG_ATA_PIO_TRANSFER;

    /* Set the byte count per DRQ data block */
    if (Request->Flags & REQUEST_FLAG_READ_WRITE_MULTIPLE)
        BlockCount = Device->MultiSectorCount;
    else
        BlockCount = 1;
    ChanData->DrqByteCount = BlockCount * Device->SectorSize;

    /*
     * For PIO writes we need to send the first data block
     * to make the device start generating interrupts.
     */
    if (Request->Flags & REQUEST_FLAG_DATA_OUT)
        return TRUE;

    return FALSE;
}

BOOLEAN
PataStartIo(
    _In_ PVOID ChannelContext,
    _In_ PATA_DEVICE_REQUEST Request)
{
    PCHANNEL_DATA_PATA ChanData = ChannelContext;
    PATA_IO_CONTEXT_COMMON Device = Request->Device;
    UCHAR IdeStatus;
    BOOLEAN IsPollingNeeded;

    ChanData->ActiveSlotsBitmap |= 1 << PATA_CHANNEL_SLOT;

    /* Select the device */
    ATA_SELECT_DEVICE(ChanData, DEV_NUMBER(Device), Device->DeviceSelect);

    /* Wait for busy to clear */
    IdeStatus = ATA_WAIT(ChanData, ATA_TIME_BUSY_SELECT, IDE_STATUS_BUSY, 0);
    if (IdeStatus & IDE_STATUS_BUSY)
    {
        WARN("Device is busy 0x%02x\n", IdeStatus);
        PataCompleteCommand(ChanData, IdeStatus, SRB_STATUS_SELECTION_TIMEOUT);
        return TRUE;
    }

    /* Disable interrupts for the polled commands */
    if (!!(Request->Flags & REQUEST_FLAG_POLL) != ChanData->IsPollingActive)
    {
        PataChangeInterruptMode(ChanData, Device, Request);
    }

    /* Execute the command */
    if (Request->Flags & REQUEST_FLAG_PACKET_COMMAND)
        IsPollingNeeded = PataExecutePacketCommand(ChanData, Device, Request);
    else
        IsPollingNeeded = PataExecuteAtaCommand(ChanData, Device, Request);

    if (Request->Flags & REQUEST_FLAG_POLL)
    {
        /* Queue a DPC to poll for the command */
        KeInsertQueueDpc(&ChanData->PollingTimerDpc, NULL, NULL);
        return FALSE;
    }

    ChanData->CommandFlags |= CMD_FLAG_AWAIT_INTERRUPT;

    if (!IsPollingNeeded)
        return FALSE;

    /* Need to wait for a valid status */
    ATA_IO_WAIT();

    /* Wait for busy to clear */
    IdeStatus = ATA_WAIT(ChanData, ATA_TIME_BUSY_NORMAL, IDE_STATUS_BUSY, 0);
    if (IdeStatus & IDE_STATUS_BUSY)
    {
        ERR("BSY timeout, status 0x%02x\n", IdeStatus);
        PataCompleteCommand(ChanData, IdeStatus, SRB_STATUS_BUS_RESET);
        return TRUE;
    }

    return PataProcessRequest(ChanData, IdeStatus, 0);
}

VOID
PataPrepareIo(
    _In_ PVOID ChannelContext,
    _In_ PATA_DEVICE_REQUEST Request)
{
    PCHANNEL_DATA_PATA ChanData = ChannelContext;

    /*
     * HACK: In an emulated PC-98 system IDE interrupts are often lost.
     * The reason for this problem is unknown (not the driver's fault).
     * Always poll for the command completion as a workaround.
     */
    if (IsNEC_98)
        Request->Flags |= REQUEST_FLAG_POLL;

    ChanData->BytesToTransfer = Request->DataTransferLength;
    ChanData->DataBuffer = Request->DataBuffer;
}

VOID
PciIdePreparePrdTable(
    _In_ PVOID ChannelContext,
    _In_ PATA_DEVICE_REQUEST Request,
    _In_ SCATTER_GATHER_LIST* __restrict SgList)
{
    PCHANNEL_DATA_PATA ChanData = ChannelContext;
    PPCIIDE_PRD_TABLE_ENTRY PrdTableEntry;
    ULONG i;
#if DBG
    ULONG DescriptorNumber = 0;
#endif

    ASSUME(SgList->NumberOfElements > 0);

    PrdTableEntry = ChanData->PrdTable;

    for (i = 0; i < SgList->NumberOfElements; ++i)
    {
        ULONG Address = SgList->Elements[i].Address.LowPart;
        ULONG Length = SgList->Elements[i].Length;

        ASSUME(Length != 0);

        /* 32-bit DMA */
        ASSERT(SgList->Elements[i].Address.HighPart == 0);

        /* The address of memory region must be word aligned */
        ASSERT((Address & ATA_MIN_BUFFER_ALIGNMENT) == 0);

        while (Length > 0)
        {
            ULONG TransferLength;

            /* If the memory region would cross a 64 kB boundary, split it */
            TransferLength = PCIIDE_PRD_LIMIT - (Address & (PCIIDE_PRD_LIMIT - 1));
            TransferLength = min(TransferLength, Length);

            /* The byte count must be word aligned */
            ASSERT((TransferLength % sizeof(USHORT)) == 0);

            /* Make sure we do not write beyond the end of the PRD table */
            ASSERT(DescriptorNumber++ < ChanData->MaximumPhysicalPages);

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

    PciIdeDmaPrepare(ChanData);
}

BOOLEAN
PataAllocateSlot(
    _In_ PVOID ChannelContext,
    _In_ PATA_DEVICE_REQUEST Request,
    _In_ BOOLEAN Allocate)
{
    UNREFERENCED_PARAMETER(ChannelContext);
    UNREFERENCED_PARAMETER(Request);
    UNREFERENCED_PARAMETER(Allocate);

    return TRUE;
}

BOOLEAN
NTAPI
PciIdeChannelIsr(
    _In_ PKINTERRUPT Interrupt,
    _In_ PVOID Context)
{
    PCHANNEL_DATA_PATA ChanData = Context;
    UCHAR IdeStatus, DmaStatus;
    BOOLEAN WasCleared;

    UNREFERENCED_PARAMETER(Interrupt);

    if (ChanData->CheckInterrupt && !(ChanData->CheckInterrupt(ChanData)))
        return FALSE;

    DmaStatus = PciIdeDmaReadStatus(ChanData);
    if (DmaStatus & PCIIDE_DMA_STATUS_INTERRUPT)
    {
        /* Always clear the DMA interrupt, even when the current command is a PIO command */
        if ((ChanData->CommandFlags & CMD_FLAG_TRANSFER_MASK) == CMD_FLAG_DMA_TRANSFER)
            PciIdeDmaStop(ChanData);
        else
            PciIdeDmaClearInterrupt(ChanData, DmaStatus);

        WasCleared = TRUE;
    }
    else
    {
        if ((ChanData->CommandFlags & CMD_FLAG_TRANSFER_MASK) == CMD_FLAG_DMA_TRANSFER)
            return FALSE;

        WasCleared = FALSE;
    }

    /* Acknowledge the IDE interrupt */
    IdeStatus = ChanData->ReadStatus(ChanData);

    /* This interrupt is not ours */
    if (!(ChanData->ActiveSlotsBitmap & (1 << PATA_CHANNEL_SLOT)) ||
        !(ChanData->CommandFlags & CMD_FLAG_AWAIT_INTERRUPT) ||
        (IdeStatus & IDE_STATUS_BUSY))
    {
        /* Do not log PCI shared interrupts */
        if (WasCleared)
        {
            WARN("Spurious bus-master interrupt %02x, %02x, flags %08lx\n",
                 IdeStatus, DmaStatus, ChanData->CommandFlags);
        }

        return WasCleared;
    }

    PataProcessRequest(ChanData, IdeStatus, DmaStatus);
    return TRUE;
}

BOOLEAN
NTAPI
PataChannelIsr(
    _In_ PKINTERRUPT Interrupt,
    _In_ PVOID Context)
{
    PCHANNEL_DATA_PATA ChanData = Context;
    UCHAR IdeStatus;

    UNREFERENCED_PARAMETER(Interrupt);

    if (ChanData->CheckInterrupt && !(ChanData->CheckInterrupt(ChanData)))
        return FALSE;

    /* Acknowledge the IDE interrupt */
    IdeStatus = ChanData->ReadStatus(ChanData);

    /* This interrupt is spurious or not ours */
    if (!(ChanData->ActiveSlotsBitmap & (1 << PATA_CHANNEL_SLOT)) ||
        !(ChanData->CommandFlags & CMD_FLAG_AWAIT_INTERRUPT) ||
        (IdeStatus & IDE_STATUS_BUSY))
    {
        WARN("Spurious IDE interrupt %02x, flags %08lx\n", IdeStatus, ChanData->CommandFlags);
        return FALSE;
    }

    PataProcessRequest(ChanData, IdeStatus, 0);
    return TRUE;
}

static
BOOLEAN
PataPoll(
    _In_ PCHANNEL_DATA_PATA ChanData)
{
    BOOLEAN IsRequestCompleted = FALSE;
    UCHAR IdeStatus;

    while (TRUE)
    {
        /* Need to wait for a valid status */
        ATA_IO_WAIT();

        /* Do a quick check here for a busy */
        IdeStatus = ATA_WAIT(ChanData, ATA_TIME_BUSY_POLL, IDE_STATUS_BUSY, 0);
        if (IdeStatus & IDE_STATUS_BUSY)
        {
            /* Device is still busy, schedule a timer to retry the poll */
            break;
        }

        IsRequestCompleted = PataProcessRequest(ChanData, IdeStatus, 0);
        if (IsRequestCompleted)
            break;
    }

    return IsRequestCompleted;
}

VOID
NTAPI
PataPollingTimerDpc(
    _In_ PKDPC Dpc,
    _In_opt_ PVOID DeferredContext,
    _In_opt_ PVOID SystemArgument1,
    _In_opt_ PVOID SystemArgument2)
{
    PCHANNEL_DATA_PATA ChanData = DeferredContext;
    KIRQL OldIrql;
    BOOLEAN IsRequestCompleted;

    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(SystemArgument1);
    UNREFERENCED_PARAMETER(SystemArgument2);

    OldIrql = KeAcquireInterruptSpinLock(ChanData->InterruptObject);

    if (!(ChanData->ActiveSlotsBitmap & (1 << PATA_CHANNEL_SLOT)) ||
        (ChanData->CommandFlags & CMD_FLAG_AWAIT_INTERRUPT))
    {
        IsRequestCompleted = FALSE;
    }
    else
    {
        IsRequestCompleted = PataPoll(ChanData);
    }

    KeReleaseInterruptSpinLock(ChanData->InterruptObject, OldIrql);

    if (!IsRequestCompleted)
        KeInsertQueueDpc(&ChanData->PollingTimerDpc, NULL, NULL);
}
