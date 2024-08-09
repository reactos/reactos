/*
 * PROJECT:     ReactOS ATA Port Driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     PATA definitions
 * COPYRIGHT:   Copyright 2024 Dmitry Borisov <di.sean@protonmail.com>
 */

#pragma once

/* Master/Slave devices */
#define CHANNEL_PCAT_MAX_DEVICES     2

/* Master/Slave devices for Bank 0 and Bank 1 */
#define CHANNEL_PC98_MAX_DEVICES     4

/* Delay of 400ns */
#define ATA_IO_WAIT()         KeStallExecutionProcessor(1)

/* We ignore the REL and Tag bits */
#define ATAPI_INTERRUPT_REASON_MASK                   0x03

#define ATAPI_INTERRUPT_REASON_INVALID                0x00
#define ATAPI_INTERRUPT_REASON_CMD_COMPLETION         0x03
#define ATAPI_INTERRUPT_REASON_DATA_OUT               0x08
#define ATAPI_INTERRUPT_REASON_AWAIT_CDB              0x09
#define ATAPI_INTERRUPT_REASON_DATA_IN                0x0A

#define ATA_TIME_BUSY_SELECT    2000 /* 20 ms */
#define ATA_TIME_BUSY_NORMAL    200000
#define ATA_TIME_BUSY_ISR       3
#define ATA_TIME_BUSY_POLL      5
#define ATA_TIME_DRQ_NORMAL     100000
#define ATA_TIME_DRQ_ISR        1000

#define ATA_TIME_BUSY_ENUM      10

#define CMD_FLAG_NONE                   0x00000000
#define CMD_FLAG_TRANSFER_MASK          0x00000003
#define CMD_FLAG_CDB                    0x00000004
#define CMD_FLAG_READ_TASK_FILE         0x00000008
#define CMD_FLAG_DATA_IN                0x00000040
#define CMD_FLAG_DATA_OUT               0x00000080
#define CMD_FLAG_AWAIT_INTERRUPT        0x80000000

#define CMD_FLAG_ATAPI_PIO_TRANSFER     0x00000001
#define CMD_FLAG_ATA_PIO_TRANSFER       0x00000002
#define CMD_FLAG_DMA_TRANSFER           0x00000003

C_ASSERT(CMD_FLAG_DATA_IN == REQUEST_FLAG_DATA_IN);
C_ASSERT(CMD_FLAG_DATA_OUT == REQUEST_FLAG_DATA_OUT);
C_ASSERT(CMD_FLAG_AWAIT_INTERRUPT == REQUEST_FLAG_SYNC_MODE);

#define ATA_READ(Port, Value)           READ_PORT_UCHAR((Port), (Value))
#define ATA_WRITE(Port, Value)          WRITE_PORT_UCHAR((Port), (Value))
#define ATA_WRITE_ULONG(Port, Value)    WRITE_PORT_ULONG((Port), (Value))

FORCEINLINE
VOID
ATA_WRITE_BLOCK_16(
    _In_ PUSHORT Port,
    _In_ PUSHORT Buffer,
    _In_ ULONG Count)
{
    ASSERT(Buffer);
    ASSERT(Count);

    WRITE_PORT_BUFFER_USHORT(Port, Buffer, Count);
}

FORCEINLINE
VOID
ATA_WRITE_BLOCK_32(
    _In_ PULONG Port,
    _In_ PULONG Buffer,
    _In_ ULONG Count)
{
    ASSERT(Buffer);
    ASSERT(Count);

    WRITE_PORT_BUFFER_ULONG(Port, Buffer, Count);
}

FORCEINLINE
VOID
ATA_READ_BLOCK_16(
    _In_ PUSHORT Port,
    _In_ PUSHORT Buffer,
    _In_ ULONG Count)
{
    ASSERT(Buffer);
    ASSERT(Count);

    READ_PORT_BUFFER_USHORT(Port, Buffer, Count);
}

FORCEINLINE
VOID
ATA_READ_BLOCK_32(
    _In_ PULONG Port,
    _In_ PULONG Buffer,
    _In_ ULONG Count)
{
    ASSERT(Buffer);
    ASSERT(Count);

    READ_PORT_BUFFER_ULONG(Port, Buffer, Count);
}

#if defined(_M_IX86)
/** The 0x432 port is used to select the primary (0) or secondary (1) IDE channel */
FORCEINLINE
VOID
ATA_SELECT_BANK(
    _In_ UCHAR Number)
{
    ASSERT((Number & ~1) == 0);

    WRITE_PORT_UCHAR((PUCHAR)0x432, Number);
}
#endif

FORCEINLINE
VOID
ATA_SELECT_DEVICE(
    _In_ PATAPORT_CHANNEL_EXTENSION ChanExt,
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ UCHAR DeviceHead)
{
#if defined(_M_IX86)
    if (ChanExt->Flags & CHANNEL_CBUS_IDE)
    {
        ATA_SELECT_BANK(DevExt->AtaScsiAddress.TargetId >> 1);
    }
#endif

    ATA_WRITE(ChanExt->Registers.Device, DeviceHead);
    ATA_IO_WAIT();
}
