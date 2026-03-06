/*
 * PROJECT:     ReactOS ATA Bus Driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     PATA definitions
 * COPYRIGHT:   Copyright 2024-2025 Dmitry Borisov <di.sean@protonmail.com>
 */

#pragma once

#define PCI_VEN_ATI                  0x1002
#define PCI_VEN_AMD                  0x1022
#define PCI_VEN_NVIDIA               0x10DE
#define PCI_VEN_PC_TECH              0x1042
#define PCI_VEN_CMD                  0x1095
#define PCI_VEN_VIA                  0x1106
#define PCI_VEN_SERVERWORKS          0x1166
#define PCI_VEN_TOSHIBA              0x1179
#define PCI_VEN_INTEL                0x8086

/** Master/Slave devices */
#define CHANNEL_PCAT_MAX_DEVICES     2

/** Master/Slave devices for Bank 0 and Bank 1 */
#define CHANNEL_PC98_MAX_DEVICES     4

/** Delay of 400ns */
#define ATA_IO_WAIT()           KeStallExecutionProcessor(1)

#define IDE_DC_ALWAYS           0x08
#define IDE_DRIVE_SELECT        0xA0

#define IDE_HIGH_ORDER_BYTE     0x80

#define IDE_FEATURE_PIO         0x00
#define IDE_FEATURE_DMA         0x01
#define IDE_FEATURE_DMADIR      0x04

/**
 * If a larger transfer is attempted, the 16-bit ByteCount register might overflow.
 * In this case we round down the length to the closest multiple of 2.
 */
#define ATAPI_MAX_DRQ_DATA_BLOCK   0xFFFE

/**
 * Legacy ranges and interrupts
 */
/*@{*/
#define PCIIDE_LEGACY_RESOURCE_COUNT                3
#define PCIIDE_LEGACY_COMMAND_IO_RANGE_LENGTH       8
#define PCIIDE_LEGACY_CONTROL_IO_RANGE_LENGTH       1
#define PCIIDE_LEGACY_PRIMARY_COMMAND_BASE      0x1F0
#define PCIIDE_LEGACY_PRIMARY_CONTROL_BASE      0x3F6
#define PCIIDE_LEGACY_PRIMARY_IRQ                  14
#define PCIIDE_LEGACY_SECONDARY_COMMAND_BASE    0x170
#define PCIIDE_LEGACY_SECONDARY_CONTROL_BASE    0x376
#define PCIIDE_LEGACY_SECONDARY_IRQ                15
/*@}*/

#define PCIIDE_COMMAND_IO_RANGE_LENGTH              8
#define PCIIDE_CONTROL_IO_RANGE_LENGTH              4
#define PCIIDE_CONTROL_IO_BAR_OFFSET                2
#define PCIIDE_DMA_IO_BAR                           4
#define PCIIDE_DMA_IO_RANGE_LENGTH                  16

/** Offset from base address */
#define PCIIDE_DMA_SECONDARY_CHANNEL_OFFSET     8

/**
 * Programming Interface Register
 */
/*@{*/
#define PCIIDE_PROGIF_PRIMARY_CHANNEL_NATIVE_MODE              0x01
#define PCIIDE_PROGIF_PRIMARY_CHANNEL_NATIVE_MODE_CAPABLE      0x02
#define PCIIDE_PROGIF_SECONDARY_CHANNEL_NATIVE_MODE            0x04
#define PCIIDE_PROGIF_SECONDARY_CHANNEL_NATIVE_MODE_CAPABLE    0x08
#define PCIIDE_PROGIF_DMA_CAPABLE                              0x80
/*@}*/

/**
 * IDE Bus Master I/O Registers
 */
/*@{*/
#define PCIIDE_DMA_COMMAND                      0
#define PCIIDE_DMA_STATUS                       2
#define PCIIDE_DMA_PRDT_PHYSICAL_ADDRESS        4
/*@}*/

/**
 * IDE Bus Master Command Register
 */
/*@{*/
#define PCIIDE_DMA_COMMAND_STOP                         0x00
#define PCIIDE_DMA_COMMAND_START                        0x01
#define PCIIDE_DMA_COMMAND_READ_FROM_SYSTEM_MEMORY      0x00
#define PCIIDE_DMA_COMMAND_WRITE_TO_SYSTEM_MEMORY       0x08
/*@}*/

/**
 * IDE Bus Master Status Register
 */
/*@{*/
#define PCIIDE_DMA_STATUS_ACTIVE                        0x01
#define PCIIDE_DMA_STATUS_ERROR                         0x02
#define PCIIDE_DMA_STATUS_INTERRUPT                     0x04
#define PCIIDE_DMA_STATUS_RESERVED1                     0x08
#define PCIIDE_DMA_STATUS_RESERVED2                     0x10
#define PCIIDE_DMA_STATUS_DRIVE0_DMA_CAPABLE            0x20
#define PCIIDE_DMA_STATUS_DRIVE1_DMA_CAPABLE            0x40
#define PCIIDE_DMA_STATUS_SIMPLEX                       0x80
/*@}*/

/**
 * ATAPI Interrupt Status
 */
/*@{*/
/** Command or Data */
#define ATAPI_INT_REASON_COD      0x01

/** Read direction */
#define ATAPI_INT_REASON_IO       0x02

/** Bus release */
#define ATAPI_INT_REASON_RELEASE  0x04

/** Command Tag for the command */
#define ATAPI_INT_REASON_TAG      0xF8

#define ATAPI_INT_REASON_MASK             (ATAPI_INT_REASON_IO | ATAPI_INT_REASON_COD)

/** Status - Register contains Completion Status (NEC CDR-260) */
#define ATAPI_INT_REASON_STATUS_NEC       0x00

/** Status - Register contains Completion Status */
#define ATAPI_INT_REASON_STATUS           (ATAPI_INT_REASON_IO | ATAPI_INT_REASON_COD)

/** Data From Host - Receive command parameter data from the host */
#define ATAPI_INT_REASON_DATA_OUT         IDE_STATUS_DRQ

/** CDB - Ready for Command Packet */
#define ATAPI_INT_REASON_AWAIT_CDB        (IDE_STATUS_DRQ | ATAPI_INT_REASON_COD)

/** Data To Host - Send command parameter data to the host */
#define ATAPI_INT_REASON_DATA_IN          (ATAPI_INT_REASON_IO | IDE_STATUS_DRQ)
/*@}*/

#define ATA_TIME_BUSY_SELECT    3000    ///< 30 ms
#define ATA_TIME_BUSY_NORMAL    50000   ///< 500 ms
#define ATA_TIME_BUSY_POLL      5       ///< 50 us
#define ATA_TIME_DRQ_CLEAR      1000    ///< 10 ms
#define ATA_TIME_PHASE_CHANGE   100     ///< 1 ms

/* We keep the value as small as possible, since large timeout can break our error handling */
#define ATA_TIME_DRQ_ASSERT     15      ///< 150 us

#define ATA_TIME_RESET_SELECT   (2000 / PORT_TIMER_TICK_MS)  ///< 2 s
#define ATA_TIME_BUSY_RESET     (10000 / PORT_TIMER_TICK_MS) ///< 10 s

#define CMD_FLAG_NONE                   0x00000000
#define CMD_FLAG_TRANSFER_MASK          0x00000003
#define CMD_FLAG_AWAIT_CDB              0x00000004
#define CMD_FLAG_DATA_IN                0x00000040
#define CMD_FLAG_DATA_OUT               0x00000080
#define CMD_FLAG_AWAIT_INTERRUPT        0x80000000

#define CMD_FLAG_ATAPI_PIO_TRANSFER     0x00000001
#define CMD_FLAG_ATA_PIO_TRANSFER       0x00000002
#define CMD_FLAG_DMA_TRANSFER           0x00000003

/** The IDE interface has one slot per channel */
#define PATA_CHANNEL_SLOT           0
#define PATA_CHANNEL_QUEUE_DEPTH    1

C_ASSERT(CMD_FLAG_DMA_TRANSFER == (CMD_FLAG_ATAPI_PIO_TRANSFER | CMD_FLAG_ATA_PIO_TRANSFER));
C_ASSERT(CMD_FLAG_DATA_IN == REQUEST_FLAG_DATA_IN);
C_ASSERT(CMD_FLAG_DATA_OUT == REQUEST_FLAG_DATA_OUT);
C_ASSERT(CMD_FLAG_AWAIT_INTERRUPT == REQUEST_FLAG_POLL);

#include <pshpack1.h>
/**
 * Physical Region Descriptor Table Entry
 */
typedef struct _PCIIDE_PRD_TABLE_ENTRY
{
    ULONG Address;
    ULONG Length; //!< 0 means 0x10000 bytes
#define PCIIDE_PRD_LENGTH_MASK    0xFFFF
#define PCIIDE_PRD_END_OF_TABLE   0x80000000
} PCIIDE_PRD_TABLE_ENTRY, *PPCIIDE_PRD_TABLE_ENTRY;
#include <poppack.h>

/** 64 kB boundary */
#define PCIIDE_PRD_LIMIT          0x10000

typedef USHORT ATATIM;

typedef struct _ATA_TIMING
{
    ATATIM AddressSetup; // t1
    /* Register transfers (8-bit) */
    ATATIM CmdActive;    // t2
    ATATIM CmdRecovery;  // t2i
    /* PIO data transfers (16-bit) */
    ATATIM DataActive;   // t2
    ATATIM DataRecovery; // t2i
} ATA_TIMING, *PATA_TIMING;

#define SHARED_CMD_TIMINGS   0x00000001
#define SHARED_DATA_TIMINGS  0x00000002
#define SHARED_ADDR_TIMINGS  0x00000004

FORCEINLINE
ATATIM
CLAMP_TIMING(
    _In_ ATATIM Value,
    _In_ ATATIM Minimum,
    _In_ ATATIM Maximum)
{
    if (Value < Minimum)
        return Minimum;
    if (Value > Maximum)
        return Maximum;
    return Value;
}

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
    _In_ PCHANNEL_DATA_PATA ChanData,
    _In_ UCHAR DeviceNumber,
    _In_ UCHAR DeviceSelect)
{
#if defined(_M_IX86)
    /* NEC extension to allow 4 drives per channel */
    if ((ChanData->ChanInfo & CHANNEL_FLAG_CBUS) &&
        (ChanData->LastAtaBankId != DeviceNumber))
    {
        ChanData->LastAtaBankId = DeviceNumber;
        ATA_SELECT_BANK(DeviceNumber >> 1);
    }
#endif

    ATA_WRITE(ChanData->Regs.Device, DeviceSelect);
    ATA_IO_WAIT();
}

FORCEINLINE
UCHAR
ATA_WAIT(
    _In_ PCHANNEL_DATA_PATA ChanData,
    _In_range_(>, 0) ULONG Timeout,
    _In_ UCHAR Mask,
    _In_ UCHAR Value)
{
    UCHAR IdeStatus;
    ULONG i;

    for (i = 0; i < Timeout; ++i)
    {
        IdeStatus = ChanData->ReadStatus(ChanData);
        if ((IdeStatus & Mask) == Value)
            break;

        if (IdeStatus == 0xFF)
            break;

        KeStallExecutionProcessor(10);
    }

    return IdeStatus;
}
