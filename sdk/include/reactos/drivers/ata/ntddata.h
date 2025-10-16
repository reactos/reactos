/*
 * PROJECT:     ReactOS Storage Stack
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     ATA driver definitions
 * COPYRIGHT:   Copyright 2025 Dmitry Borisov (di.sean@protonmail.com)
 */

#pragma once

#define PCIIDEX_INTERFACE_VERSION    1

/**
 * @brief 256 sectors of 512 bytes (128 kB).
 *
 * This ensures that the sector count register will not overflow in LBA-28 and CHS modes.
 * In the case of the sector count (0x100) being truncated to 8-bits, 0 still means 256 sectors.
 */
/*@{*/
#define ATA_MIN_SECTOR_SIZE           512
#define ATA_MAX_SECTORS_PER_IO        0x100
#define ATA_MAX_TRANSFER_LENGTH       (ATA_MAX_SECTORS_PER_IO * ATA_MIN_SECTOR_SIZE) // 0x20000
/*@}*/

/** Because of the reserved PRDT byte 0 */
#define ATA_MIN_BUFFER_ALIGNMENT      FILE_WORD_ALIGNMENT

#define NUM_TO_BITMAP(num)    (0xFFFFFFFF >> (RTL_BITS_OF(ULONG) - (num)))

#define PIO_MODE(n)      (n)
#define SWDMA_MODE(n)    (5 + (n))
#define MWDMA_MODE(n)    (8 + (n))
#define UDMA_MODE(n)     (11 + (n))

#define PIO_ALL \
    (PIO_MODE0 | PIO_MODE1 | PIO_MODE2 | PIO_MODE3 | PIO_MODE4)
#define SWDMA_ALL \
    (SWDMA_MODE0 | SWDMA_MODE1 | SWDMA_MODE2)
#define MWDMA_ALL \
    (MWDMA_MODE0 | MWDMA_MODE1 | MWDMA_MODE2)
#define UDMA_ALL \
    (UDMA_MODE0 | UDMA_MODE1 | UDMA_MODE2 | UDMA_MODE3 | UDMA_MODE4 | UDMA_MODE5 | UDMA_MODE6)
#define SATA_ALL \
    (PIO_ALL | MWDMA_ALL | UDMA_ALL)

#define MWDMA_MODES(MinMode, MaxMode) \
    (NUM_TO_BITMAP(MWDMA_MODE((MaxMode) + 1)) & ~NUM_TO_BITMAP(MWDMA_MODE(MinMode)))
#define UDMA_MODES(MinMode, MaxMode) \
    (NUM_TO_BITMAP(UDMA_MODE((MaxMode) + 1)) & ~NUM_TO_BITMAP(UDMA_MODE(MinMode)))

/** Offset from base address */
#define PCIIDE_DMA_SECONDARY_CHANNEL_OFFSET     8

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

typedef struct _ATA_CHANNEL_DATA        ATA_CHANNEL_DATA, *PATA_CHANNEL_DATA;

typedef struct _IDE_REGISTERS
{
    PUCHAR Data;
    union
    {
        PUCHAR Features;
        PUCHAR Error;
    };
    union
    {
        PUCHAR SectorCount;
        PUCHAR InterruptReason;
    };
    PUCHAR LbaLow;             ///< LBA bits 0-7, 24-31
    union
    {
        PUCHAR LbaMid;         ///< LBA bits 8-15, 32-39
        PUCHAR ByteCountLow;
        PUCHAR SignatureLow;
    };
    union
    {
        PUCHAR LbaHigh;        ///< LBA bits 16-23, 40-47
        PUCHAR ByteCountHigh;
        PUCHAR SignatureHigh;
    };
    PUCHAR Device;
    union
    {
        PUCHAR Command;
        PUCHAR Status;
    };
    union
    {
        PUCHAR Control;
        PUCHAR AlternateStatus;
    };
    PUCHAR Dma;
} IDE_REGISTERS, *PIDE_REGISTERS;

typedef struct _CHANNEL_DEVICE_DATA
{
    ULONG SupportedModes;
    ULONG PioMode;
    ULONG DmaMode;
    ULONG PioCycleTime;
    ULONG MwDmaCycleTime;
    ULONG SwDmaCycleTime;
    ULONG CurrentModes;
    BOOLEAN FixedDisk;
    BOOLEAN IoReadySupported;
} CHANNEL_DEVICE_DATA, *PCHANNEL_DEVICE_DATA;

typedef PCM_PARTIAL_RESOURCE_DESCRIPTOR
(CHANNEL_GET_INTERRUPT_RESOURCE)(
    _In_ PATA_CHANNEL_DATA ChanData);
typedef CHANNEL_GET_INTERRUPT_RESOURCE *PCHANNEL_GET_INTERRUPT_RESOURCE;

typedef VOID
(CHANNEL_SET_MODE)(
    _In_ PATA_CHANNEL_DATA ChanData,
    _In_reads_(MAX_IDE_DEVICE) PCHANNEL_DEVICE_DATA* DeviceList);
typedef CHANNEL_SET_MODE *PCHANNEL_SET_MODE;

typedef VOID
(CHANNEL_PREPARE_IO)(
    _In_ PATA_CHANNEL_DATA ChanData,
    _In_ UCHAR TargetId,
    _In_ BOOLEAN UseDma);
typedef CHANNEL_PREPARE_IO *PCHANNEL_PREPARE_IO;

/**
 * @brief Port interface with the PCIIDEX driver.
 *
 * This interface is ROS-specific.
 */
typedef struct _PCIIDE_INTERFACE
{
    /* Common interface header */
    USHORT Size;
    USHORT Version;
    PVOID Context;
    PINTERFACE_REFERENCE InterfaceReference;
    PINTERFACE_DEREFERENCE InterfaceDereference;

    PCHANNEL_GET_INTERRUPT_RESOURCE GetInterruptResource;
    PCONTROLLER_OBJECT HwSyncObject;
    PDEVICE_OBJECT HwSyncContext;
    PVOID Controller;
    PATA_CHANNEL_DATA ChanData;
    PDMA_ADAPTER AdapterObject;
    PDEVICE_OBJECT AdapterDeviceObject;
    PPCIIDE_PRD_TABLE_ENTRY PrdTable;
    PCHANNEL_PREPARE_IO PrepareIo;
    ULONG PrdTablePhysicalAddress;
    ULONG Channel;
    ULONG MaximumTransferLength;
    ULONG MaximumPhysicalPages;
    ULONG TransferModeSupportedBitmap;

    ULONG ChanInfo;
#define ATA_CHANNEL_FLAG_SIMPLEX                        0x00000001
#define ATA_CHANNEL_FLAG_FLAG_IO32                      0x00000002
#define ATA_CHANNEL_FLAG_DRIVE0_DMA_CAPABLE             0x00000004
#define ATA_CHANNEL_FLAG_DRIVE1_DMA_CAPABLE             0x00000008
#define ATA_CHANNEL_FLAG_CONTROL_PORT_BASE_MAPPED       0x00000010
#define ATA_CHANNEL_FLAG_COMMAND_PORT_BASE_MAPPED       0x00000020
#define ATA_CHANNEL_FLAG_DMA_PORT_BASE_MAPPED           0x00000040

    PCHANNEL_SET_MODE SetTransferMode;
    IDE_REGISTERS Regs;
} PCIIDE_INTERFACE, *PPCIIDE_INTERFACE;

DEFINE_GUID(GUID_PCIIDE_INTERFACE_ROS,
            0xD677FBCF, 0xABED, 0x47C8, 0x80, 0xA3, 0xE4, 0x34, 0x7E, 0xA4, 0x96, 0x47);
