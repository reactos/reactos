/*
 * PROJECT:     ReactOS ATA Port Driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     PATA hardware support
 * COPYRIGHT:   Copyright 2024-2025 Dmitry Borisov <di.sean@protonmail.com>
 */

/* INCLUDES *******************************************************************/

#include "atapi.h"

/* FUNCTIONS ******************************************************************/

static
BOOLEAN
AtaPataIsDevicePresent(
    _In_ PATAPORT_PORT_DATA PortData,
    _In_ ULONG TargetId)
{
    UCHAR IdeStatus;

    /* Select the device */
    ATA_SELECT_DEVICE(PortData, TargetId, IDE_DRIVE_SELECT | ((TargetId & 1) << 4));

    /* Do a quick check first */
    IdeStatus = ATA_READ(PortData->Pata.Registers.Status);
    INFO("PORT %p: Device %u status %02x\n", PortData->Pata.Registers.Data, TargetId, IdeStatus);
    if (IdeStatus == 0xFF || IdeStatus == 0x7F)
        return FALSE;

    /* Look at controller */
    ATA_WRITE(PortData->Pata.Registers.ByteCountLow, 0x55);
    ATA_WRITE(PortData->Pata.Registers.ByteCountLow, 0xAA);
    ATA_WRITE(PortData->Pata.Registers.ByteCountLow, 0x55);
    if (ATA_READ(PortData->Pata.Registers.ByteCountLow) != 0x55)
        return FALSE;
    ATA_WRITE(PortData->Pata.Registers.ByteCountHigh, 0xAA);
    ATA_WRITE(PortData->Pata.Registers.ByteCountHigh, 0x55);
    ATA_WRITE(PortData->Pata.Registers.ByteCountHigh, 0xAA);
    if (ATA_READ(PortData->Pata.Registers.ByteCountHigh) != 0xAA)
        return FALSE;

    return TRUE;
}

static
VOID
AtaPataSoftwareReset(
    _In_ PATAPORT_PORT_DATA PortData)
{
    ATA_WRITE(PortData->Pata.Registers.Control, IDE_DC_RESET_CONTROLLER | IDE_DC_ALWAYS);
    KeStallExecutionProcessor(20);
    ATA_WRITE(PortData->Pata.Registers.Control, IDE_DC_REENABLE_CONTROLLER | IDE_DC_ALWAYS);
    KeStallExecutionProcessor(20);
}

static
VOID
AtaPataDrainDeviceBuffer(
    _In_ PATAPORT_PORT_DATA PortData)
{
    PATA_DEVICE_REQUEST Request = PortData->Slots[PATA_CHANNEL_SLOT];
    UCHAR IdeStatus;
    ULONG i;

    if (Request && (Request->Flags & REQUEST_FLAG_DMA))
        return;

    /* Try to clear the DRQ indication */
    for (i = 0; i < ATA_MAX_TRANSFER_LENGTH / sizeof(USHORT); ++i)
    {
        IdeStatus = ATA_READ(PortData->Pata.Registers.Status);
        if (!(IdeStatus & IDE_STATUS_DRQ))
            break;

        READ_PORT_USHORT((PUSHORT)PortData->Pata.Registers.Data);
    }

    if (i > 0)
        INFO("Total drained %lu bytes\n", i * sizeof(USHORT));
}

static
ULONG
AtaPataGetTargetBitmap(
    _In_ PATAPORT_PORT_DATA PortData,
    _In_ BOOLEAN Reset)

{
    ULONG i, TargetBitmap = 0, MaxTargetId = PortData->ChanExt->MaxTargetId;

    for (i = 0; i < MaxTargetId; ++i)
    {
        if (PortData->Worker.BadTargetBitmap & (1 << i))
            continue;

        if (!AtaPataIsDevicePresent(PortData, i))
            continue;

        if (Reset)
            AtaPataDrainDeviceBuffer(PortData);

        TargetBitmap |= 1 << i;
    }

    return TargetBitmap;
}

static
VOID
AtaPataResetChannel(
    _In_ PATAPORT_PORT_DATA PortData)
{
#if defined(_M_IX86)
    if (PortData->PortFlags & PORT_FLAG_CBUS_IDE)
    {
        /* Reset the secondary IDE channel if present */
        if (PortData->Worker.PreResetTargetBitmap & ((1 << 2) | (1 << 3)))
        {
            ATA_SELECT_BANK(1);
            AtaPataSoftwareReset(PortData);
        }

        ATA_SELECT_BANK(0);
    }
#endif

    AtaPataSoftwareReset(PortData);
}

static
VOID
AtaPataHandlePortEnum(
    _In_ PATAPORT_PORT_DATA PortData)
{
    AtaFsmCompletePortEnumEvent(PortData, AtaPataGetTargetBitmap(PortData, FALSE));
}

static
VOID
AtaPataHandleReset(
    _In_ PATAPORT_PORT_DATA PortData)
{
    PortData->Worker.PreResetTargetBitmap = AtaPataGetTargetBitmap(PortData, TRUE);

    WARN("PORT %p: Resetting IDE channel, targets 0x%lx\n",
         PortData->Pata.Registers.Data,
         PortData->Worker.PreResetTargetBitmap);

    AtaPataResetChannel(PortData);
    AtaFsmSetLocalState(&PortData->Worker, PATA_STATE_RESET_SELECT_NEXT_DEVICE);
}

static
VOID
AtaPataHandleResetSelectNextDevice(
    _In_ PATAPORT_PORT_DATA PortData)
{
    /* Select next device */
    if (!_BitScanForward(&PortData->Worker.CurrentTargetId, PortData->Worker.PreResetTargetBitmap))
    {
        AtaFsmCompletePortEnumEvent(PortData, AtaPataGetTargetBitmap(PortData, FALSE));
        return;
    }

    TRACE("PORT %p: Selected target %lu\n",
         PortData->Pata.Registers.Data,
         PortData->Worker.CurrentTargetId);

    /* The reset will cause the master device to be selected */
    if ((PortData->Worker.CurrentTargetId & 1) != 0)
    {
        PortData->Worker.TimeOut = ATA_TIME_RESET_SELECT / PORT_TIMER_TICK_MS;
        AtaFsmSetLocalState(&PortData->Worker, PATA_STATE_RESET_WAIT_FOR_REGISTER_ACCESS);
    }
    else
    {
        PortData->Worker.TimeOut = ATA_TIME_BUSY_RESET / PORT_TIMER_TICK_MS;
        AtaFsmSetLocalState(&PortData->Worker, PATA_STATE_RESET_WAIT_FOR_BSY);
    }
}

static
VOID
AtaPataHandleResetWaitForRegisterAccess(
    _In_ PATAPORT_PORT_DATA PortData)
{
    /* Select the device again */
    ATA_SELECT_DEVICE(PortData,
                      PortData->Worker.CurrentTargetId,
                      IDE_DRIVE_SELECT | ((PortData->Worker.CurrentTargetId & 1) << 4));

    /* Check whether the selection was successful */
    ATA_WRITE(PortData->Pata.Registers.ByteCountLow, 0xAA);
    ATA_WRITE(PortData->Pata.Registers.ByteCountLow, 0x55);
    ATA_WRITE(PortData->Pata.Registers.ByteCountLow, 0xAA);
    if (ATA_READ(PortData->Pata.Registers.ByteCountLow) == 0xAA)
    {
        PortData->Worker.TimeOut = ATA_TIME_BUSY_RESET / PORT_TIMER_TICK_MS;
        AtaFsmSetLocalState(&PortData->Worker, PATA_STATE_RESET_WAIT_FOR_BSY);
        return;
    }

    /* Retry after the time interval */
    if (--PortData->Worker.TimeOut != 0)
    {
        AtaFsmRequestTimer(&PortData->Worker);
        return;
    }

    ERR("PORT %p: Selection timeout at target %u\n",
        PortData->Pata.Registers.Data,
        PortData->Worker.CurrentTargetId);
    AtaFsmResetPort(PortData, PortData->Worker.CurrentTargetId);
}

static
VOID
AtaPataHandleResetWaitForBusy(
    _In_ PATAPORT_PORT_DATA PortData)
{
    UCHAR IdeStatus;

    /* Now wait for busy to clear */
    IdeStatus = ATA_READ(PortData->Pata.Registers.Status);
    if (!(IdeStatus & IDE_STATUS_BUSY) || (IdeStatus == 0xFF))
    {
        INFO("PORT %p: Target %u online with status %02x\n",
            PortData->Pata.Registers.Data,
            PortData->Worker.CurrentTargetId,
            IdeStatus);
        PortData->Worker.PreResetTargetBitmap &= ~(1 << PortData->Worker.CurrentTargetId);
        AtaFsmSetLocalState(&PortData->Worker, PATA_STATE_RESET_SELECT_NEXT_DEVICE);
        return;
    }

    /* Retry after the time interval */
    if (--PortData->Worker.TimeOut != 0)
    {
        AtaFsmRequestTimer(&PortData->Worker);
        return;
    }

    ERR("PORT %p: Failed to reset the device at target %u, status %02x\n",
        PortData->Pata.Registers.Data,
        PortData->Worker.CurrentTargetId,
        IdeStatus);
    AtaFsmResetPort(PortData, PortData->Worker.CurrentTargetId);
}

VOID
AtaPataRunStateMachine(
    _In_ PATAPORT_PORT_DATA PortData)
{
    switch (PortData->Worker.LocalState)
    {
        case PATA_STATE_PORT_ENUM:
            AtaPataHandlePortEnum(PortData);
            break;
        case PATA_STATE_RESET:
            AtaPataHandleReset(PortData);
            break;
        case PATA_STATE_RESET_SELECT_NEXT_DEVICE:
            AtaPataHandleResetSelectNextDevice(PortData);
            break;
        case PATA_STATE_RESET_WAIT_FOR_REGISTER_ACCESS:
            AtaPataHandleResetWaitForRegisterAccess(PortData);
            break;
        case PATA_STATE_RESET_WAIT_FOR_BSY:
            AtaPataHandleResetWaitForBusy(PortData);
            break;

        default:
            ASSERT(FALSE);
            UNREACHABLE;
    }
}
