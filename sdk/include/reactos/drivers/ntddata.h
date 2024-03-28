/*
 * PROJECT:     ReactOS Storage Stack
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     ATA driver definitions
 * COPYRIGHT:   Copyright 2024 Dmitry Borisov (di.sean@protonmail.com)
 */

#pragma once

/*
 * 256 sectors of 512 bytes (128 kB).
 * This ensures that the sector count register will not overflow
 * in LBA-28 and CHS modes.
 */
#define ATA_MAX_TRANSFER_LENGTH 0x20000

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

#define MAX_AHCI_DEVICES     32

#define AHCI_COMMAND_LIST_SIZE     0x400
#define AHCI_RECEIVED_FIS_SIZE     0x400

#include <pshpack1.h>

typedef struct _AHCI_PORT_REGISTERS
{
    volatile ULONG CommandListBaseLow;
    volatile ULONG CommandListBaseHigh;

    volatile ULONG FisBaseLow;
    volatile ULONG FisBaseHigh;

    volatile ULONG InterruptStatus;
    volatile ULONG InterruptEnable;
    volatile ULONG CmdStatus;
    volatile ULONG Reserved;
    volatile ULONG TaskFileData;
    volatile ULONG Signature;
    volatile ULONG SataStatus;
    volatile ULONG SataControl;
    volatile ULONG SataError;
    volatile ULONG SataActive;
    volatile ULONG CommandIssue;
    volatile ULONG SataNotification;
    volatile ULONG FisSwitchingControl;
    volatile ULONG Reserved2[11];
    volatile ULONG Reserved3[4];
} AHCI_PORT_REGISTERS, *PAHCI_PORT_REGISTERS;

typedef struct _AHCI_HOST_BUS_ADAPTER
{
    volatile ULONG Capabilities;

    volatile ULONG GlobalControl;
#define AHCI_GHC_AE             0x80000000

    volatile ULONG InterruptStatus;
    volatile ULONG PortsImplemented;
    volatile ULONG AhciVersion;
    volatile ULONG CoalescingControl;
    volatile ULONG CoalescingPorts;
    volatile ULONG EnclosureManagementLocation;
    volatile ULONG EnclosureManagementControl;
    volatile ULONG CapabilitiesEx;
    volatile ULONG BiosControl;
    volatile ULONG Reserved[29 + 24];
    AHCI_PORT_REGISTERS Port[MAX_AHCI_DEVICES];
} AHCI_HOST_BUS_ADAPTER, *PAHCI_HOST_BUS_ADAPTER;

typedef struct _AHCI_RECEIVED_FIS
{
    UCHAR DmaSetup[0x1C];
    ULONG Reserved;

    UCHAR PioSetup[0x14];
    ULONG Reserved2[3];

    UCHAR DeviceToHost[0x14];
    ULONG Reserved3;

    UCHAR DeviceBits[0x08];

    UCHAR UnknownFis[0x40];

    UCHAR Reserved4[0x60];
} AHCI_RECEIVED_FIS, *PAHCI_RECEIVED_FIS;

typedef struct _AHCI_COMMAND_LIST_ENTRY
{
    ULONG Control;
    ULONG PrdByteCount;
    ULONG CommandTableBaseLow;
    ULONG CommandTableBaseHigh;
    ULONG Reserved[4];
} AHCI_COMMAND_LIST_ENTRY, *PAHCI_COMMAND_LIST_ENTRY;

typedef struct _AHCI_PRD
{
    ULONG DataBaseLow;
    ULONG DataBaseHigh;
    ULONG Reserved;
    ULONG ByteCount;
} AHCI_PRD, *PAHCI_PRD;

#include <poppack.h>

typedef struct _AHCI_DEVICE_EXTENSION
{
    PAHCI_HOST_BUS_ADAPTER Hba;
} AHCI_DEVICE_EXTENSION, *PAHCI_DEVICE_EXTENSION;

typedef struct _AHCI_PORT_INTERFACE
{
    USHORT Size;
    USHORT Version;
    PVOID Context;
    PINTERFACE_REFERENCE InterfaceReference;
    PINTERFACE_DEREFERENCE InterfaceDereference;

    PAHCI_PORT_REGISTERS Port;
} AHCI_PORT_INTERFACE, *PAHCI_PORT_INTERFACE;

DEFINE_GUID(GUID_AHCI_PORT_INTERFACE,
            0xD677FBCF, 0xABED, 0x47C8, 0x80, 0xA3, 0xE4, 0x34, 0x7E, 0xA4, 0x96, 0x46);
