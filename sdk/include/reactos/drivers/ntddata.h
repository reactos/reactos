/*
 * PROJECT:     ReactOS Storage Stack
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     ATA driver definitions
 * COPYRIGHT:   Copyright 2024 Dmitry Borisov (di.sean@protonmail.com)
 */

#pragma once

/**
 * 256 sectors of 512 bytes (128 kB).
 * This ensures that the sector count register will not overflow
 * in LBA-28 and CHS modes.
 */
/*@{*/

/**
 * In the case of the sector count being truncated to 8-bits,
 * 0 still means 256 sectors.
 */
#define ATA_MAX_SECTORS_PER_IO        0x100

/** 0x20000 */
#define ATA_MAX_TRANSFER_LENGTH      (ATA_MAX_SECTORS_PER_IO * 512)

/*@}*/

/* Offset from base address */
#define DMA_SECONDARY_CHANNEL_OFFSET     8

/*
 * IDE Bus Master I/O Registers
 */
#define DMA_COMMAND                      0
#define DMA_STATUS                       2
#define DMA_PRDT_PHYSICAL_ADDRESS        4

/*
 * IDE Bus Master Command Register
 */
#define DMA_COMMAND_STOP                         0x00
#define DMA_COMMAND_START                        0x01
#define DMA_COMMAND_READ_FROM_SYSTEM_MEMORY      0x00
#define DMA_COMMAND_WRITE_TO_SYSTEM_MEMORY       0x08

/*
 * IDE Bus Master Status Register
 */
#define DMA_STATUS_ACTIVE                        0x01
#define DMA_STATUS_ERROR                         0x02
#define DMA_STATUS_INTERRUPT                     0x04
#define DMA_STATUS_RESERVED1                     0x08
#define DMA_STATUS_RESERVED2                     0x10
#define DMA_STATUS_DRIVE0_DMA_CAPABLE            0x20
#define DMA_STATUS_DRIVE1_DMA_CAPABLE            0x40
#define DMA_STATUS_SIMPLEX                       0x80

/* 64 kB boundary */
#define PRD_LIMIT          0x10000

#include <pshpack1.h>

/*
 * Physical Region Descriptor Table Entry
 */
typedef struct _PRD_TABLE_ENTRY
{
    ULONG Address;
    ULONG Length;
#define PRD_LENGTH_MASK    0xFFFF
#define PRD_END_OF_TABLE   0x80000000
} PRD_TABLE_ENTRY, *PPRD_TABLE_ENTRY;

/*
 * Physical Region Descriptor Table
 */
typedef struct _PRD_TABLE
{
    PRD_TABLE_ENTRY Entry[ANYSIZE_ARRAY];
} PRD_TABLE, *PPRD_TABLE;

#include <poppack.h>

typedef struct _PCIIDE_INTERFACE
{
    USHORT Size;
    USHORT Version;
    PVOID Context;
    PINTERFACE_REFERENCE InterfaceReference;
    PINTERFACE_DEREFERENCE InterfaceDereference;

    PUCHAR IoBase;
    PVOID PrdTable;
    ULONG PrdTablePhysicalAddress;
    ULONG MaximumTransferLength;
    PDMA_ADAPTER AdapterObject;
    PDEVICE_OBJECT DeviceObject;
} PCIIDE_INTERFACE, *PPCIIDE_INTERFACE;

DEFINE_GUID(GUID_PCIIDE_INTERFACE,
            0xD677FBCF, 0xABED, 0x47C8, 0x80, 0xA3, 0xE4, 0x34, 0x7E, 0xA4, 0x96, 0x47);
