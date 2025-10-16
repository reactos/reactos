/*
 * PROJECT:     PCI IDE bus driver extension
 * LICENSE:     See COPYING in the top level directory
 * PURPOSE:     Common header file
 * COPYRIGHT:   Copyright 2005 Hervé Poussineau <hpoussin@reactos.org>
 *              Copyright 2023 Dmitry Borisov <di.sean@protonmail.com>
 */

#pragma once

#include <ntddk.h>
#include <ntstrsafe.h>
#include <ntintsafe.h>
#include <initguid.h>
#include <wdmguid.h>
#include <ide.h>
#include <reactos/drivers/ata/ntddata.h>

#include "debug.h"

#define TAG_PCIIDEX    'XedI'

#define IS_FDO(p) \
    ((((PCOMMON_DEVICE_EXTENSION)(p))->Flags & DO_IS_FDO) != 0)

#define IS_PRIMARY_CHANNEL(PdoExtension)    (PdoExtension->Channel == 0)

#if defined(_MSC_VER)
#pragma section("PAGECONS", read)
#endif

/** Pageable read-only data */
#define PCIIDEX_PAGED_DATA    DATA_SEG("PAGECONS")

/*
 * Legacy ranges and interrupts
 */
#define PCIIDE_LEGACY_RESOURCE_COUNT                3
#define PCIIDE_LEGACY_COMMAND_IO_RANGE_LENGTH       8
#define PCIIDE_LEGACY_CONTROL_IO_RANGE_LENGTH       1
#define PCIIDE_LEGACY_PRIMARY_COMMAND_BASE      0x1F0
#define PCIIDE_LEGACY_PRIMARY_CONTROL_BASE      0x3F6
#define PCIIDE_LEGACY_PRIMARY_IRQ                  14
#define PCIIDE_LEGACY_SECONDARY_COMMAND_BASE    0x170
#define PCIIDE_LEGACY_SECONDARY_CONTROL_BASE    0x376
#define PCIIDE_LEGACY_SECONDARY_IRQ                15

/*
 * Programming Interface Register
 */
#define PCIIDE_PROGIF_PRIMARY_CHANNEL_NATIVE_MODE              0x01
#define PCIIDE_PROGIF_PRIMARY_CHANNEL_NATIVE_MODE_CAPABLE      0x02
#define PCIIDE_PROGIF_SECONDARY_CHANNEL_NATIVE_MODE            0x04
#define PCIIDE_PROGIF_SECONDARY_CHANNEL_NATIVE_MODE_CAPABLE    0x08
#define PCIIDE_PROGIF_DMA_CAPABLE                              0x80

#define PCIIDE_DMA_IO_BAR                           4

#define PCIIDE_CONTROL_IO_BAR_OFFSET                2

#define MAX_CHANNELS   32

#define PCI_VEN_AMD                  0x1022
#define PCI_VEN_NVIDIA               0x10DE
#define PCI_VEN_PC_TECH              0x1042
#define PCI_VEN_CMD                  0x1095
#define PCI_VEN_TOSHIBA              0x1179
#define PCI_VEN_INTEL                0x8086

typedef struct _PDO_DEVICE_EXTENSION    PDO_DEVICE_EXTENSION, *PPDO_DEVICE_EXTENSION;
typedef struct _ATA_CONTROLLER          ATA_CONTROLLER, *PATA_CONTROLLER;

_IRQL_requires_max_(APC_LEVEL)
typedef BOOLEAN
(ATA_PCI_MATCH_FN)(
    _In_ PVOID Context,
    _In_ ULONG BusNumber,
    _In_ PCI_SLOT_NUMBER PciSlot,
    _In_ PPCI_COMMON_HEADER PciConfig);
typedef ATA_PCI_MATCH_FN *PATA_PCI_MATCH_FN;

_IRQL_requires_max_(APC_LEVEL)
typedef IDE_CHANNEL_STATE
(CONTROLLER_CHANNEL_ENABLED)(
    _In_ PATA_CONTROLLER Controller,
    _In_ ULONG Channel);
typedef CONTROLLER_CHANNEL_ENABLED *PCONTROLLER_CHANNEL_ENABLED;

_IRQL_requires_max_(DISPATCH_LEVEL)
typedef VOID
(CONTROLLER_START)(
    _In_ PATA_CONTROLLER Controller);
typedef CONTROLLER_START *PCONTROLLER_START;

_IRQL_requires_max_(DISPATCH_LEVEL)
typedef VOID
(CHANNEL_SET_MODE_EX)(
    _In_ PATA_CONTROLLER Controller,
    _In_ ULONG Channel,
    _In_reads_(MAX_IDE_DEVICE) PCHANNEL_DEVICE_DATA* DeviceList);
typedef CHANNEL_SET_MODE_EX *PCHANNEL_SET_MODE_EX;

typedef struct _ATA_PCI_ENABLE_BITS
{
    UCHAR Register;
    UCHAR Mask;
    UCHAR Value;
} ATA_PCI_ENABLE_BITS, *PATA_PCI_ENABLE_BITS;

typedef struct _ATA_CONTROLLER
{
    CM_PARTIAL_RESOURCE_DESCRIPTOR InterruptDesc;
    struct
    {
        USHORT VendorID;
        USHORT DeviceID;
        USHORT Command;
        UCHAR RevisionID;
        UCHAR ProgIf;
        UCHAR SubClass;
        UCHAR BaseClass;
        UCHAR CacheLineSize;
        USHORT SubVendorID;
        USHORT SubSystemID;
    } Pci;

    struct
    {
        ULONG Flags;
#define RANGE_IS_VALID   0x01
#define RANGE_IS_MEMORY  0x02
#define RANGE_IS_MAPPED  0x04
        ULONG Length;
        PVOID IoBase;
    } AccessRange[PCI_TYPE0_ADDRESSES];

    BUS_INTERFACE_STANDARD BusInterface;
    PATA_CHANNEL_DATA ChanDataBlock;
    PCONTROLLER_OBJECT HwSyncObject;
    PCONTROLLER_START Start;
    const ATA_PCI_ENABLE_BITS* ChannelEnableBits;
    PCONTROLLER_CHANNEL_ENABLED ChannelEnabledTest;
    ULONG AlignmentRequirement;
    ULONG MaxChannels;

    ULONG Flags;
#define CTRL_FLAG_PCI_IDE              0x00000001
#define CTRL_FLAG_IN_LEGACY_MOVE       0x00000002
#define CTRL_FLAG_PIO_ONLY             0x00000004
#define CTRL_FLAG_IS_SIMPLEX           0x00000008
#define CTRL_FLAG_HAS_INTERRUPT_RES    0x00000010

    KSPIN_LOCK Lock;
    PATA_CHANNEL_DATA Channels[MAX_CHANNELS];
} ATA_CONTROLLER, *PATA_CONTROLLER;

typedef struct _ATA_CHANNEL_DATA
{
    PDMA_ADAPTER AdapterObject;
    PPCIIDE_PRD_TABLE_ENTRY PrdTable;

    PATA_CONTROLLER Controller;
    PCHANNEL_PREPARE_IO PrepareIo;
    PCHANNEL_SET_MODE_EX SetTransferMode;
    ULONG Channel;
    ULONG TransferModeSupportedBitmap;
    ULONG MaximumTransferLength;
    ULONG PrdTablePhysicalAddress;
    IDE_REGISTERS Regs;
    ULONG ChanInfo;
    ULONG HwFlags;
    PUCHAR HwExt[ANYSIZE_ARRAY];
} ATA_CHANNEL_DATA, *PATA_CHANNEL_DATA;

typedef struct _PCIIDEX_DRIVER_EXTENSION
{
    PCONTROLLER_PROPERTIES HwGetControllerProperties;
    ULONG MiniControllerExtensionSize;
} PCIIDEX_DRIVER_EXTENSION, *PPCIIDEX_DRIVER_EXTENSION;

typedef struct _COMMON_DEVICE_EXTENSION
{
    _Write_guarded_by_(_Global_interlock_)
    volatile LONG PageFiles;

    _Write_guarded_by_(_Global_interlock_)
    volatile LONG HibernateFiles;

    _Write_guarded_by_(_Global_interlock_)
    volatile LONG DumpFiles;

    union
    {
        /** Lower device object. This applies to FDO only. */
        PDEVICE_OBJECT LowerDeviceObject;

        /** Parent FDO. This applies to PDO only. */
        PVOID FdoExt;
    };

    PDEVICE_OBJECT Self;

    ULONG Flags;
#define DO_IS_FDO        0x80000000

    IO_REMOVE_LOCK RemoveLock;
} COMMON_DEVICE_EXTENSION, *PCOMMON_DEVICE_EXTENSION;

typedef struct _FDO_DEVICE_EXTENSION
{
    COMMON_DEVICE_EXTENSION Common;
    PDRIVER_OBJECT DriverObject;
    PDEVICE_OBJECT Pdo;
    ULONG ControllerNumber;
    FAST_MUTEX PdoListSyncMutex;
    _Guarded_by_(PdoListSyncMutex)
    LIST_ENTRY PdoListHead;
    ATA_CONTROLLER Controller;
    IDE_CONTROLLER_PROPERTIES Properties;

    /* Must be the last entry */
    PUCHAR MiniControllerExtension[0];
} FDO_DEVICE_EXTENSION, *PFDO_DEVICE_EXTENSION;

typedef struct _PDO_DEVICE_EXTENSION
{
    COMMON_DEVICE_EXTENSION Common;
    ULONG Channel;
    ULONG Flags;
#define PDO_FLAG_NOT_PRESENT           0x00000001
#define PDO_FLAG_REPORTED_MISSING      0x00000002

    PATA_CHANNEL_DATA ChanData;
    ULONG MaximumTransferLength;
    ULONG MaximumPhysicalPages;
    ULONG TransferModeSupportedBitmap;
    LIST_ENTRY ListEntry;
} PDO_DEVICE_EXTENSION, *PPDO_DEVICE_EXTENSION;

CODE_SEG("PAGE")
DRIVER_INITIALIZE DriverEntry;

CODE_SEG("PAGE")
DRIVER_UNLOAD PciIdeXUnload;

CODE_SEG("PAGE")
DRIVER_ADD_DEVICE PciIdeXAddDevice;

_Dispatch_type_(IRP_MJ_PNP)
CODE_SEG("PAGE")
DRIVER_DISPATCH_PAGED PciIdeXDispatchPnp;

_Dispatch_type_(IRP_MJ_SYSTEM_CONTROL)
CODE_SEG("PAGE")
DRIVER_DISPATCH_PAGED PciIdeXDispatchWmi;

_Dispatch_type_(IRP_MJ_POWER)
DRIVER_DISPATCH_RAISED PciIdeXDispatchPower;

IO_COMPLETION_ROUTINE PciIdeXPdoCompletionRoutine;

CHANNEL_SET_MODE AtaCtrlSetTransferMode;

CODE_SEG("PAGE")
NTSTATUS
PciIdeXFdoDispatchPnp(
    _In_ PFDO_DEVICE_EXTENSION FdoExtension,
    _Inout_ PIRP Irp);

CODE_SEG("PAGE")
NTSTATUS
PciIdeXPdoRemoveDevice(
    _In_ PPDO_DEVICE_EXTENSION PdoExtension,
    _In_ PIRP Irp,
    _In_ BOOLEAN FinalRemove,
    _In_ BOOLEAN LockNeeded);

CODE_SEG("PAGE")
IDE_CHANNEL_STATE
PciIdeXChannelState(
    _In_ PFDO_DEVICE_EXTENSION FdoExtension,
    _In_ ULONG Channel);

CODE_SEG("PAGE")
BOOLEAN
PciFindDevice(
    _In_ __inner_callback PATA_PCI_MATCH_FN MatchFunction,
    _In_ PVOID Context);

VOID
PciRead(
    _In_ PATA_CONTROLLER Controller,
    _Out_writes_bytes_all_(BufferLength) PVOID Buffer,
    _In_ ULONG ConfigDataOffset,
    _In_ ULONG BufferLength);

VOID
PciWrite(
    _In_ PATA_CONTROLLER Controller,
    _In_reads_bytes_(BufferLength) PVOID Buffer,
    _In_ ULONG ConfigDataOffset,
    _In_ ULONG BufferLength);

CODE_SEG("PAGE")
NTSTATUS
PciIdeXPnpRepeatRequest(
    _In_ PCOMMON_DEVICE_EXTENSION CommonExt,
    _In_ PIRP Irp,
    _In_opt_ PDEVICE_CAPABILITIES DeviceCapabilities);

CODE_SEG("PAGE")
NTSTATUS
PciIdeXPnpQueryDeviceUsageNotification(
    _In_ PCOMMON_DEVICE_EXTENSION CommonExt,
    _In_ PIRP Irp);

CODE_SEG("PAGE")
NTSTATUS
PciIdeXPnpQueryPnpDeviceState(
    _In_ PCOMMON_DEVICE_EXTENSION CommonExt,
    _In_ PIRP Irp);

CODE_SEG("PAGE")
PVOID
AtaCtrlPciGetBar(
    _In_ PATA_CONTROLLER Controller,
    _In_ ULONG Index);

CODE_SEG("PAGE")
IDE_CHANNEL_STATE
PciIdeXGetChannelState(
    _In_ PATA_CONTROLLER Controller,
    _In_ ULONG Channel);

CODE_SEG("PAGE")
NTSTATUS
PciIdeCreateChannelData(
    _In_ PATA_CONTROLLER Controller,
    _In_ ULONG HwExtensionSize);

CODE_SEG("PAGE")
NTSTATUS
PciIdeStartController(
    _In_ PATA_CONTROLLER Controller);

CODE_SEG("PAGE")
NTSTATUS
PciIdeStartChannel(
    _In_ PATA_CONTROLLER Controller,
    _In_ PPDO_DEVICE_EXTENSION PdoExt,
    _In_ PCM_RESOURCE_LIST ResourcesTranslated);

CODE_SEG("PAGE")
VOID
PciIdeStopChannel(
    _In_ PATA_CONTROLLER Controller,
    _In_ PPDO_DEVICE_EXTENSION PdoExt);

VOID
PciIdeSataSetTransferMode(
    _In_ PATA_CONTROLLER Controller,
    _In_ ULONG Channel,
    _In_reads_(MAX_IDE_DEVICE) PCHANNEL_DEVICE_DATA* DeviceList);

CODE_SEG("PAGE")
NTSTATUS
AmdGetControllerProperties(
    _Inout_ PATA_CONTROLLER Controller);

CODE_SEG("PAGE")
NTSTATUS
CmdGetControllerProperties(
    _Inout_ PATA_CONTROLLER Controller);

CODE_SEG("PAGE")
NTSTATUS
IntelGetControllerProperties(
    _Inout_ PATA_CONTROLLER Controller);

CODE_SEG("PAGE")
NTSTATUS
PcTechGetControllerProperties(
    _Inout_ PATA_CONTROLLER Controller);

CODE_SEG("PAGE")
NTSTATUS
ToshibaGetControllerProperties(
    _Inout_ PATA_CONTROLLER Controller);
