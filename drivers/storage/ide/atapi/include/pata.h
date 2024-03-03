/*
 * PROJECT:     ReactOS ATA Port Driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     PATA definitions
 * COPYRIGHT:   Copyright 2024 Dmitry Borisov <di.sean@protonmail.com>
 */

#pragma once

/** Master/Slave devices */
#define CHANNEL_PCAT_MAX_DEVICES     2

/** Master/Slave devices for Bank 0 and Bank 1 */
#define CHANNEL_PC98_MAX_DEVICES     4

/** Delay of 400ns */
#define ATA_IO_WAIT()         KeStallExecutionProcessor(1)

/** Command or Data */
#define ATAPI_INT_REASON_COD      0x01

/** Read direction */
#define ATAPI_INT_REASON_IO       0x02

/** Bus release */
#define ATAPI_INT_REASON_RELEASE  0x04

/** Command Tag for the command */
#define ATAPI_INT_REASON_TAG      0xF8

/** We ignore the RELEASE and TAG bits */
#define ATAPI_INT_REASON_MASK             (ATAPI_INT_REASON_IO | ATAPI_INT_REASON_COD)

/** Status - Register contains Completion Status (NEC CDR-260) */
#define ATAPI_INT_REASON_STATUS_NEC       0x00

/** Status - Register contains Completion Status */
#define ATAPI_INT_REASON_STATUS          (ATAPI_INT_REASON_IO | ATAPI_INT_REASON_COD)

/** Data From Host - Receive command parameter data from the host */
#define ATAPI_INT_REASON_DATA_OUT        IDE_STATUS_DRQ

/** CDB - Ready for Command Packet */
#define ATAPI_INT_REASON_AWAIT_CDB       (IDE_STATUS_DRQ | ATAPI_INT_REASON_COD)

/** Data To Host - Send command parameter data to the host */
#define ATAPI_INT_REASON_DATA_IN         (ATAPI_INT_REASON_IO | IDE_STATUS_DRQ)

#define ATA_TIME_BUSY_SELECT    2000   //!< 20 ms
#define ATA_TIME_BUSY_NORMAL    50000  //!< 500 ms
#define ATA_TIME_BUSY_ENUM      100    //!< 1 ms
#define ATA_TIME_BUSY_POLL      5      //!< 50 us
#define ATA_TIME_BUSY_RESET     100000 //!< 10 s
#define ATA_TIME_BUSY_IDENTIFY  500000 //!< 5 s
#define ATA_TIME_RESET_SELECT   200000 //!< 2 s
#define ATA_TIME_DRQ_CLEAR      100    //!< 200 us

#define CMD_FLAG_NONE                   0x00000000
#define CMD_FLAG_TRANSFER_MASK          0x00000003
#define CMD_FLAG_AWAIT_CDB              0x00000004
#define CMD_FLAG_READ_TASK_FILE         0x00000008
#define CMD_FLAG_DATA_IN                0x00000040
#define CMD_FLAG_DATA_OUT               0x00000080
#define CMD_FLAG_AWAIT_INTERRUPT        0x80000000

#define CMD_FLAG_ATAPI_PIO_TRANSFER     0x00000001
#define CMD_FLAG_ATA_PIO_TRANSFER       0x00000002
#define CMD_FLAG_DMA_TRANSFER           0x00000003

C_ASSERT(CMD_FLAG_DMA_TRANSFER == (CMD_FLAG_ATAPI_PIO_TRANSFER | CMD_FLAG_ATA_PIO_TRANSFER));
C_ASSERT(CMD_FLAG_DATA_IN == REQUEST_FLAG_DATA_IN);
C_ASSERT(CMD_FLAG_DATA_OUT == REQUEST_FLAG_DATA_OUT);
C_ASSERT(CMD_FLAG_AWAIT_INTERRUPT == REQUEST_FLAG_POLL);

#define ATA_READ(Port)                  READ_PORT_UCHAR((Port))
#define ATA_WRITE(Port, Value)          WRITE_PORT_UCHAR((Port), (Value))
#define ATA_WRITE_ULONG(Port, Value)    WRITE_PORT_ULONG((Port), (Value))

FORCEINLINE
VOID
ATA_WRITE_BLOCK_16(
    _In_ PUSHORT Port,
    _In_ PUSHORT Buffer,
    _In_ ULONG Count)
{
    ASSERT(Buffer != NULL);
    ASSERT(Count != 0);

    WRITE_PORT_BUFFER_USHORT(Port, Buffer, Count);
}

FORCEINLINE
VOID
ATA_WRITE_BLOCK_32(
    _In_ PULONG Port,
    _In_ PULONG Buffer,
    _In_ ULONG Count)
{
    ASSERT(Buffer != NULL);
    ASSERT(Count != 0);

    WRITE_PORT_BUFFER_ULONG(Port, Buffer, Count);
}

FORCEINLINE
VOID
ATA_READ_BLOCK_16(
    _In_ PUSHORT Port,
    _In_ PUSHORT Buffer,
    _In_ ULONG Count)
{
    ASSERT(Buffer != NULL);
    ASSERT(Count != 0);

    READ_PORT_BUFFER_USHORT(Port, Buffer, Count);
}

FORCEINLINE
VOID
ATA_READ_BLOCK_32(
    _In_ PULONG Port,
    _In_ PULONG Buffer,
    _In_ ULONG Count)
{
    ASSERT(Buffer != NULL);
    ASSERT(Count != 0);

    READ_PORT_BUFFER_ULONG(Port, Buffer, Count);
}

#if defined(_M_IX86)
FORCEINLINE
VOID
ATA_SELECT_BANK(
    _In_ UCHAR Number)
{
    ASSERT((Number & ~1) == 0);

    /* The 0x432 port is used to select the primary (0) or secondary (1) IDE channel */
    WRITE_PORT_UCHAR((PUCHAR)0x432, Number);
}
#endif

FORCEINLINE
VOID
ATA_SELECT_DEVICE(
    _In_ PATAPORT_PORT_DATA PortData,
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ UCHAR DeviceSelect)
{
#if defined(_M_IX86)
    if (PortData->PortFlags & PORT_FLAG_CBUS_IDE)
    {
        ATA_SELECT_BANK(DevExt->AtaScsiAddress.TargetId >> 1);
    }
#endif

    ATA_WRITE(PortData->Pata.Registers.Device, DeviceSelect);
    ATA_IO_WAIT();
}

FORCEINLINE
BOOLEAN
ATA_WAIT_ON_BUSY(
    _In_ PIDE_REGISTERS Registers,
    _Inout_ PUCHAR IdeStatus,
    _In_ ULONG Timeout)
{
    ULONG i;

    if (!(*IdeStatus & IDE_STATUS_BUSY))
        return TRUE;

    for (i = 0; i < Timeout; ++i)
    {
        KeStallExecutionProcessor(10);

        *IdeStatus = ATA_READ(Registers->Status);
        if (!(*IdeStatus & IDE_STATUS_BUSY))
            return TRUE;

        if (*IdeStatus == 0xFF)
            break;
    }

    return FALSE;
}

FORCEINLINE
BOOLEAN
ATA_WAIT_FOR_IDLE(
    _In_ PIDE_REGISTERS Registers,
    _Inout_ PUCHAR IdeStatus)
{
    ULONG i;

    if (!(*IdeStatus & (IDE_STATUS_DRQ | IDE_STATUS_BUSY)))
        return TRUE;

    for (i = 0; i < ATA_TIME_DRQ_CLEAR; ++i)
    {
        KeStallExecutionProcessor(2);

        *IdeStatus = ATA_READ(Registers->Status);
        if (!(*IdeStatus & (IDE_STATUS_DRQ | IDE_STATUS_BUSY)))
            return TRUE;
    }

    return FALSE;
}
