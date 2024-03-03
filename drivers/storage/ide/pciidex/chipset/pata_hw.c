/*
 * PROJECT:     ReactOS ATA Bus Driver
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     PATA hardware support
 * COPYRIGHT:   Copyright 2026 Dmitry Borisov <di.sean@protonmail.com>
 */

/* INCLUDES *******************************************************************/

#include "pciidex.h"

/* FUNCTIONS ******************************************************************/

static
BOOLEAN
PataIsDevicePresent(
    _In_ PCHANNEL_DATA_PATA ChanData,
    _In_ ULONG DeviceNumber)
{
    UCHAR IdeStatus;

    /* Select the device */
    ATA_SELECT_DEVICE(ChanData, DeviceNumber, IDE_DRIVE_SELECT | ((DeviceNumber & 1) << 4));

    /* Do a quick check first */
    IdeStatus = ChanData->ReadStatus(ChanData);
    INFO("CH %lu: Device %lu status %02x\n", ChanData->Channel, DeviceNumber, IdeStatus);
    if (IdeStatus == 0xFF || IdeStatus == 0x7F)
        return FALSE;

    /* Look at controller */
    ATA_WRITE(ChanData->Regs.ByteCountLow, 0x55, ChanData, MRES_TF);
    ATA_WRITE(ChanData->Regs.ByteCountLow, 0xAA, ChanData, MRES_TF);
    ATA_WRITE(ChanData->Regs.ByteCountLow, 0x55, ChanData, MRES_TF);
    if (ATA_READ(ChanData->Regs.ByteCountLow, ChanData, MRES_TF) != 0x55)
        return FALSE;
    ATA_WRITE(ChanData->Regs.ByteCountHigh, 0xAA, ChanData, MRES_TF);
    ATA_WRITE(ChanData->Regs.ByteCountHigh, 0x55, ChanData, MRES_TF);
    ATA_WRITE(ChanData->Regs.ByteCountHigh, 0xAA, ChanData, MRES_TF);
    if (ATA_READ(ChanData->Regs.ByteCountHigh, ChanData, MRES_TF) != 0xAA)
        return FALSE;

    return TRUE;
}

static
VOID
PataIssueSoftwareReset(
    _In_ PCHANNEL_DATA_PATA ChanData)
{
    ATA_WRITE(ChanData->Regs.Control, IDE_DC_RESET_CONTROLLER | IDE_DC_ALWAYS,
              ChanData, MRES_CTRL);
    KeStallExecutionProcessor(20);
    ATA_WRITE(ChanData->Regs.Control, IDE_DC_DISABLE_INTERRUPTS | IDE_DC_ALWAYS,
              ChanData, MRES_CTRL);
    KeStallExecutionProcessor(20);
}

static
VOID
PataDrainDeviceBuffer(
    _In_ PCHANNEL_DATA_PATA ChanData)
{
    ULONG i;
    USHORT LocalBuffer;

    /* Try to clear the DRQ indication */
    for (i = 0; i < ATA_MAX_TRANSFER_LENGTH / sizeof(USHORT); ++i)
    {
        UCHAR IdeStatus = ChanData->ReadStatus(ChanData);

        if (!(IdeStatus & IDE_STATUS_DRQ))
            break;

        ATA_READ_BLOCK_16((PUSHORT)ChanData->Regs.Data,
                          &LocalBuffer,
                          1,
                          ChanData,
                          MRES_TF);
    }

    if (i > 0)
        INFO("CH %lu: Total drained %lu bytes\n", ChanData->Channel, i * sizeof(USHORT));
}

ULONG
PataChannelGetMaximumDeviceCount(
    _In_ PVOID ChannelContext)
{
    PCHANNEL_DATA_PATA ChanData = ChannelContext;

#if defined(_M_IX86)
    if (ChanData->ChanInfo & CHANNEL_FLAG_CBUS)
        return 4;
#endif

    return (ChanData->ChanInfo & CHANNEL_FLAG_NO_SLAVE) ? 1 : 2;
}

VOID
PataResetChannel(
    _In_ PVOID ChannelContext)
{
    PCHANNEL_DATA_PATA ChanData = ChannelContext;
    ULONG i, DeviceNumber, DeviceCount, DeviceBitmap = 0;

    ChanData->IsPollingActive = FALSE;

#if defined(_M_IX86)
    if (ChanData->ChanInfo & CHANNEL_FLAG_CBUS)
        ChanData->LastAtaBankId = 0xFF;
#endif

    /*
     * Reset the start/stop bus master bit, also needed on some chips to recover,
     * see for example Intel 82371AB/EB/MB Specification Update 297738-017 #10.
     */
    if (ChanData->Regs.Dma != NULL)
        PciIdeDmaStop(ChanData);

    PataDrainDeviceBuffer(ChanData);

    DeviceCount = PataChannelGetMaximumDeviceCount(ChanData);
    for (DeviceNumber = 0; DeviceNumber < DeviceCount; ++DeviceNumber)
    {
        if (PataIsDevicePresent(ChanData, DeviceNumber))
            DeviceBitmap |= 1 << DeviceNumber;
    }
    /* Restore the device selection */
    ATA_SELECT_DEVICE(ChanData, 0, IDE_DRIVE_SELECT);

    WARN("CH %lu: Resetting IDE channel, devices map 0x%lx\n", ChanData->Channel, DeviceBitmap);

#if defined(_M_IX86)
    if (ChanData->ChanInfo & CHANNEL_FLAG_CBUS)
    {
        /* Reset the secondary IDE channel if present */
        if (DeviceBitmap & ((1 << 2) | (1 << 3)))
        {
            WRITE_PORT_UCHAR((PUCHAR)PC98_ATA_BANK, 1);
            PataIssueSoftwareReset(ChanData);
        }

        WRITE_PORT_UCHAR((PUCHAR)PC98_ATA_BANK, 0);
        ChanData->LastAtaBankId = 0xFF;
    }
#endif
    PataIssueSoftwareReset(ChanData);

    for (DeviceNumber = 0; DeviceNumber < DeviceCount; ++DeviceNumber)
    {
        UCHAR IdeStatus;

        /* The reset will cause the master device to be selected */
        if ((DeviceNumber % 2) != 0)
        {
            /* Always check BSY for the master device regardless of its presence */
            if (!(DeviceBitmap & (1 << DeviceNumber)))
                continue;

            for (i = ATA_TIME_RESET_SELECT; i > 0; i--)
            {
                /* Select the device again */
                ATA_SELECT_DEVICE(ChanData,
                                  DeviceNumber,
                                  IDE_DRIVE_SELECT | ((DeviceNumber & 1) << 4));

                /* Check whether the selection was successful */
                ATA_WRITE(ChanData->Regs.ByteCountLow, 0xAA, ChanData, MRES_TF);
                ATA_WRITE(ChanData->Regs.ByteCountLow, 0x55, ChanData, MRES_TF);
                ATA_WRITE(ChanData->Regs.ByteCountLow, 0xAA, ChanData, MRES_TF);
                if (ATA_READ(ChanData->Regs.ByteCountLow, ChanData, MRES_TF) == 0xAA)
                    break;

                AtaSleep();
            }
            if (i == 0)
            {
                ERR("CH %lu: Device %lu selection timeout %02x\n",
                    ChanData->Channel, DeviceNumber, ChanData->ReadStatus(ChanData));
                continue;
            }
        }

        /* Now wait for busy to clear */
        for (i = 0; i < ATA_TIME_BUSY_RESET; ++i)
        {
            IdeStatus = ChanData->ReadStatus(ChanData);
            if (IdeStatus == 0xFF)
                break;

            if (!(IdeStatus & IDE_STATUS_BUSY))
                break;

            AtaSleep();
        }

        if (IdeStatus & IDE_STATUS_BUSY)
        {
            ERR("CH %lu: Failed to reset device %lu, status %02x\n",
                ChanData->Channel, DeviceNumber, IdeStatus);
        }
        else
        {
            INFO("CH %lu: Device %lu online with status %02x\n",
                 ChanData->Channel, DeviceNumber, IdeStatus);
        }
    }

    AtaChanEnableInterruptsSync(ChanData, TRUE);
}

ATA_CONNECTION_STATUS
PataIdentifyDevice(
    _In_ PVOID ChannelContext,
    _In_ ULONG DeviceNumber)
{
    PCHANNEL_DATA_PATA ChanData = ChannelContext;
    UCHAR IdeStatus;

    if (!PataIsDevicePresent(ChanData, DeviceNumber))
        return CONN_STATUS_NO_DEVICE;

    /* Wait for busy to clear */
    IdeStatus = ATA_WAIT(ChanData, ATA_TIME_BUSY_SELECT, IDE_STATUS_BUSY, 0);
    if (IdeStatus & (IDE_STATUS_BUSY | IDE_STATUS_DRQ))
    {
        /*
         * This status indicates that the previous command hasn't been completed
         * and the device is left in an unexpected state.
         * A device reset is required to recover.
         */
        WARN("CH %lu: Device %lu is busy %02x\n", ChanData->Channel, DeviceNumber, IdeStatus);
        return CONN_STATUS_FAILURE;
    }

    /*
     * Also we do not check the device signature,
     * because early ATAPI drives (NEC CDR-260, and some other devices)
     * report an ATA signature.
     */
    return CONN_STATUS_DEV_UNKNOWN;
}

ULONG
PataEnumerateChannel(
    _In_ PVOID ChannelContext)
{
    PCHANNEL_DATA_PATA ChanData = ChannelContext;

    /*
     * By default, we do not issue the software reset to detect the device signature,
     * since attached devices may reset their current transfer mode.
     * We need to know what transfer mode is actually set by BIOS in order to
     * select any transfer mode correctly by our generic PATA chipset driver
     * (PciIdeAcpiSetTransferMode() or PciIdeBiosSetTransferMode()).
     */
    return PataChannelGetMaximumDeviceCount(ChanData);
}
