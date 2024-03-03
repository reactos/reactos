/*
 * PROJECT:     ReactOS Storage Stack
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     ATA driver definitions
 * COPYRIGHT:   Copyright 2025 Dmitry Borisov (di.sean@protonmail.com)
 */

#pragma once

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

    UCHAR ChannelNumber;
    PCM_RESOURCE_LIST ControllerResources;
    PCIIDE_TRANSFER_MODE_SELECT_FUNC ProgramTimingMode;
    PDMA_ADAPTER AdapterObject;
    PDEVICE_OBJECT DeviceObject;
    PCONTROLLER_OBJECT ControllerObject;
    ULONG MapRegisterCount;
    ULONG MaximumTransferLength;
    ULONG PrdTablePhysicalAddress;
    PUCHAR IoBase;
    PPCIIDE_PRD_TABLE_ENTRY PrdTable;
} PCIIDE_INTERFACE, *PPCIIDE_INTERFACE;

DEFINE_GUID(GUID_PCIIDE_INTERFACE_ROS,
            0xD677FBCF, 0xABED, 0x47C8, 0x80, 0xA3, 0xE4, 0x34, 0x7E, 0xA4, 0x96, 0x47);
