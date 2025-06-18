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
VOID
AtaPataResetChannel(
    _In_ PATAPORT_PORT_DATA PortData)
{
    ATA_WRITE(PortData->Pata.Registers.Control, IDE_DC_RESET_CONTROLLER | IDE_DC_ALWAYS);
    KeStallExecutionProcessor(20);
    ATA_WRITE(PortData->Pata.Registers.Control, IDE_DC_REENABLE_CONTROLLER | IDE_DC_ALWAYS);
    KeStallExecutionProcessor(20);
}

static
BOOLEAN
AtaPataDevicePresent(
    _In_ PATAPORT_PORT_DATA PortData,
    _In_ ULONG TargetId)
{
    UCHAR IdeStatus;

    /* Select the device */
    ATA_SELECT_DEVICE(PortData, TargetId, IDE_DRIVE_SELECT | ((TargetId & 1) << 4));

    /* Do a quick check first */
    IdeStatus = ATA_READ(PortData->Pata.Registers.Status);
    TRACE("Device status %02x\n", IdeStatus);
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
AtaPataHandleReset(
    _In_ PATAPORT_PORT_DATA PortData)
{
    ULONG i, TargetBitmap = 0, MaxTargetId = PortData->ChanExt->MaxTargetId;

    // TODO: implement
    ASSERT(0);

    for (i = 0; i < MaxTargetId; ++i)
    {
        if (!AtaPataDevicePresent(PortData, i))
            continue;

        TargetBitmap |= 1 << i;
    }

    AtaPataResetChannel(PortData);
    AtaFsmCompletePortEnumEvent(PortData, TargetBitmap);
}

static
VOID
AtaPataHandlePortEnum(
    _In_ PATAPORT_PORT_DATA PortData)
{
    ULONG i, TargetBitmap = 0, MaxTargetId = PortData->ChanExt->MaxTargetId;
    UCHAR IdeStatus;

    for (i = 0; i < MaxTargetId; ++i)
    {
        if (PortData->Worker.BadTargetBitmap & (1 << i))
            continue;

        if (!AtaPataDevicePresent(PortData, i))
            continue;

        /* Wait for busy to clear */
        ATA_WAIT_ON_BUSY(&PortData->Pata.Registers, &IdeStatus, ATA_TIME_BUSY_ENUM);
        if (IdeStatus & IDE_STATUS_BUSY)
        {
            /*
             * This status indicates that the previous command hasn't been completed
             * and the device is left in an unexpected state.
             * A device reset is required to recover.
             */
            WARN("Device is busy %02x\n", IdeStatus);

            ASSERT(0); // todo implement
            AtaFsmResetPort(PortData, i);
            return;
        }

        TargetBitmap |= 1 << i;
    }

    AtaFsmCompletePortEnumEvent(PortData, TargetBitmap);
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

        default:
            ASSERT(FALSE);
            UNREACHABLE;
    }
}
