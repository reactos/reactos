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

#include <ata.h>
#include <ide.h>

#include <scsi.h>
#include <reactos/drivers/ata/acpi.h>
#include <reactos/drivers/ata/ntddata.h>
#include "debug.h"
#include "ahci.h"

typedef struct _CHANNEL_DATA_PATA          CHANNEL_DATA_PATA, *PCHANNEL_DATA_PATA;
typedef struct _CHANNEL_DATA_COMMON        CHANNEL_DATA_COMMON, *PCHANNEL_DATA_COMMON;
typedef struct _PCIIDE_PRD_TABLE_ENTRY     PCIIDE_PRD_TABLE_ENTRY, *PPCIIDE_PRD_TABLE_ENTRY;

#define TAG_PCIIDEX    'XedI'

#define ASSUME(cond) \
    do { \
        ASSERT(cond); \
        __assume(cond); \
    } while (0)

#define IS_FDO(p) \
    ((((PCOMMON_DEVICE_EXTENSION)(p))->Flags & DO_IS_FDO) != 0)

#define IS_PRIMARY_CHANNEL(PdoExtension)    (PdoExtension->Channel == 0)

#if defined(_MSC_VER)
#pragma section("PAGECONS", read)
#endif

/** Pageable read-only data */
#define PCIIDEX_PAGED_DATA    DATA_SEG("PAGECONS")

#define MAX_CHANNELS   32

#define PORT_TIMER_TICK_MS    10 // Timer interval is 10ms

#define DEV_NUMBER(Device)    ((Device)->TransportFlags & DEVICE_NUMBER_MASK)

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
(CONTROLLER_STOP)(
    _In_ PATA_CONTROLLER Controller);
typedef CONTROLLER_STOP *PCONTROLLER_STOP;

typedef VOID
(CONTROLLER_FREE_RESOURCES)(
    _In_ PATA_CONTROLLER Controller);
typedef CONTROLLER_FREE_RESOURCES *PCONTROLLER_FREE_RESOURCES;

typedef NTSTATUS
(CONTROLLER_ATTACH_CHANNEL_EX)(
    _In_ PVOID ChannelContext,
    _In_ BOOLEAN Attach);
typedef CONTROLLER_ATTACH_CHANNEL_EX *PCONTROLLER_ATTACH_CHANNEL_EX;

_IRQL_requires_(DISPATCH_LEVEL)
typedef VOID
(CHANNEL_SET_MODE_EX)(
    _In_ PATA_CONTROLLER Controller,
    _In_ ULONG Channel,
    _In_reads_(MAX_IDE_DEVICE) PCHANNEL_DEVICE_CONFIG* DeviceList);
typedef CHANNEL_SET_MODE_EX *PCHANNEL_SET_MODE_EX;

typedef NTSTATUS
(CHANNEL_PARSE_RESOURCES)(
    _In_ PVOID ChannelContext,
    _In_ PCM_RESOURCE_LIST ResourcesTranslated);
typedef CHANNEL_PARSE_RESOURCES *PCHANNEL_PARSE_RESOURCES;

typedef VOID
(CHANNEL_FREE_RESOURCES)(
    _In_ PVOID ChannelContext);
typedef CHANNEL_FREE_RESOURCES *PCHANNEL_FREE_RESOURCES;

typedef NTSTATUS
(CHANNEL_ALLOCATE_MEMORY)(
    _In_ PVOID ChannelContext);
typedef CHANNEL_ALLOCATE_MEMORY *PCHANNEL_ALLOCATE_MEMORY;

typedef VOID
(CHANNEL_FREE_MEMORY)(
    _In_ PVOID ChannelContext);
typedef CHANNEL_FREE_MEMORY *PCHANNEL_FREE_MEMORY;

typedef VOID
(CHANNEL_ENABLE_INTERRUPTS)(
    _In_ PVOID ChannelContext,
    _In_ BOOLEAN Enable);
typedef CHANNEL_ENABLE_INTERRUPTS *PCHANNEL_ENABLE_INTERRUPTS;

typedef BOOLEAN
(CHANNEL_CHECK_INTERRUPT)(
    _In_ PCHANNEL_DATA_PATA ChanData);
typedef CHANNEL_CHECK_INTERRUPT *PCHANNEL_CHECK_INTERRUPT;

typedef VOID
(CHANNEL_LOAD_TASK_FILE)(
    _In_ CHANNEL_DATA_PATA* __restrict ChanData,
    _In_ ATA_DEVICE_REQUEST* __restrict Request);
typedef CHANNEL_LOAD_TASK_FILE *PCHANNEL_LOAD_TASK_FILE;

typedef VOID
(CHANNEL_SAVE_TASK_FILE)(
    _In_ CHANNEL_DATA_PATA* __restrict ChanData,
    _Inout_ ATA_DEVICE_REQUEST* __restrict Request);
typedef CHANNEL_SAVE_TASK_FILE *PCHANNEL_SAVE_TASK_FILE;

typedef UCHAR
(CHANNEL_READ_STATUS)(
    _In_ PCHANNEL_DATA_PATA ChanData);
typedef CHANNEL_READ_STATUS *PCHANNEL_READ_STATUS;

typedef struct _ATA_PCI_ENABLE_BITS
{
    UCHAR Register;
    UCHAR Mask;
    UCHAR ValueEnabled;
} ATA_PCI_ENABLE_BITS, *PATA_PCI_ENABLE_BITS;

typedef struct _ATA_CONTROLLER
{
    CM_PARTIAL_RESOURCE_DESCRIPTOR InterruptDesc;
    PKINTERRUPT InterruptObject;
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
    PVOID ChanDataBlock;
    PCONTROLLER_OBJECT HwSyncObject;
    PCONTROLLER_START Start;
    PCONTROLLER_STOP Stop;
    PCONTROLLER_FREE_RESOURCES FreeResources;
    PCONTROLLER_ATTACH_CHANNEL_EX AttachChannel;
    union
    {
        const ATA_PCI_ENABLE_BITS* ChannelEnableBits;
        PCONTROLLER_CHANNEL_ENABLED ChannelEnabledTest;
    };
    PVOID HwExt;
    ULONG AlignmentRequirement;
    ULONG MaxChannels;
    ULONG ChannelBitmap;

    PUCHAR IoBase;
    ULONG AhciVersion;
    ULONG AhciCapabilities;
    ULONG AhciCapabilitiesEx;

    ULONG QueueDepth;
    ULONG CtrlFlags;
    ULONG Flags;
#define CTRL_FLAG_IN_LEGACY_MODE       0x00000001
#define CTRL_FLAG_PIO_ONLY             0x00000002
#define CTRL_FLAG_IS_SIMPLEX           0x00000004
#define CTRL_FLAG_SATA_HBA_ACPI        0x00000008
#define CTRL_FLAG_USE_TEST_FUNCTION    0x00000010
#define CTRL_FLAG_IS_AHCI              0x00000020
#define CTRL_FLAG_NON_PNP              0x00000040

    KSPIN_LOCK Lock;
    PVOID Channels[MAX_CHANNELS];
} ATA_CONTROLLER, *PATA_CONTROLLER;

typedef struct _CHANNEL_DATA_COMMON
{
    PPDO_DEVICE_EXTENSION PdoExt;
    CM_PARTIAL_RESOURCE_DESCRIPTOR InterruptDesc;
    PKINTERRUPT InterruptObject;
    PCHANNEL_PARSE_RESOURCES ParseResources;
    PCHANNEL_FREE_RESOURCES FreeResources;
    PCHANNEL_ALLOCATE_MEMORY AllocateMemory;
    PCHANNEL_FREE_MEMORY FreeMemory;
    PCHANNEL_SET_MODE_EX SetTransferMode;
    PCHANNEL_PREPARE_PRD_TABLE PreparePrdTable;
    PCHANNEL_PREPARE_IO PrepareIo;
    PCHANNEL_START_IO StartIo;
    PCHANNEL_ENABLE_INTERRUPTS EnableInterrupts;
    PATA_CONTROLLER Controller;
    PDMA_ADAPTER DmaAdapter;
    ULONG TransferModeSupported;
    ULONG MaximumTransferLength;
    ULONG Channel;
    struct
    {
        ULONG TransferModeSupported;
        ULONG MaximumTransferLength;
    } Actual;
    ULONG MaximumPhysicalPages;
    ULONG ChanInfo;
#define CHANNEL_FLAG_IO32                           0x00000001
#define CHANNEL_FLAG_DRIVE0_DMA_CAPABLE             0x00000002
#define CHANNEL_FLAG_DRIVE1_DMA_CAPABLE             0x00000004
#define CHANNEL_FLAG_CONTROL_PORT_BASE_MAPPED       0x00000008
#define CHANNEL_FLAG_COMMAND_PORT_BASE_MAPPED       0x00000010
#define CHANNEL_FLAG_DMA_PORT_BASE_MAPPED           0x00000020
#define CHANNEL_FLAG_64_BIT_DMA                     0x00000040
#define CHANNEL_FLAG_PRIMARY_ADDRESS_CLAIMED        0x00000080
#define CHANNEL_FLAG_SECONDARY_ADDRESS_CLAIMED      0x00000100
#define CHANNEL_FLAG_PIO_VIA_DMA                    0x00000200
#define CHANNEL_FLAG_NO_SLAVE                       0x00000400
#define CHANNEL_FLAG_IS_EXTERNAL                    0x00000800
#define CHANNEL_FLAG_IS_PMP                         0x00001000
#define CHANNEL_FLAG_HAS_FBS                        0x00002000
#define CHANNEL_FLAG_FBS_ENABLED                    0x00004000
#define CHANNEL_FLAG_HAS_ACPI_GTM                   0x00008000
#define CHANNEL_FLAG_HAS_NCQ                        0x00010000
#define CHANNEL_FLAG_PIO_FOR_LBA48_XFER             0x00020000 // TODO: Not used yet
#define CHANNEL_FLAG_NO_ATAPI_DMA                   0x00040000
#define CHANNEL_FLAG_DMA_BEFORE_CMD                 0x00080000 // TODO: Not used yet
#define CHANNEL_FLAG_CBUS                           0x80000000

    PVOID PortContext;
    PPORT_NOTIFICATION PortNotification;
    ULONG ActiveSlotsBitmap;
    ULONG ActiveQueuedSlotsBitmap;
    PATA_DEVICE_REQUEST* Slots;
} CHANNEL_DATA_COMMON, *PCHANNEL_DATA_COMMON;

typedef struct _CHANNEL_DATA_PATA
{
    /** Common data, must be the first member. */
    CHANNEL_DATA_COMMON;

    PCHANNEL_LOAD_TASK_FILE LoadTaskFile;
    PCHANNEL_SAVE_TASK_FILE SaveTaskFile;
    PCHANNEL_READ_STATUS ReadStatus;
    PPCIIDE_PRD_TABLE_ENTRY PrdTable;
    ULONG PrdTablePhysicalAddress;
    IDE_REGISTERS Regs;
    PCHANNEL_CHECK_INTERRUPT CheckInterrupt;
    PUCHAR DataBuffer;
    ULONG BytesToTransfer;
    ULONG CommandFlags;
    ULONG DrqByteCount;
#if defined(_M_IX86)
    UCHAR LastAtaBankId;
#endif
    BOOLEAN IsPollingActive;
    KDPC PollingTimerDpc;
    IDE_ACPI_TIMING_MODE_BLOCK CurrentTimingMode;
    ULONG HwFlags;
    PUCHAR HwExt[ANYSIZE_ARRAY];
} CHANNEL_DATA_PATA, *PCHANNEL_DATA_PATA;

typedef struct _CHANNEL_INFO_AHCI
{
    ULONG64 ReceivedFisPhys;
    ULONG64 CommandListPhys;
    PVOID ReceivedFisOriginal;
    PVOID CommandListOriginal;
    PVOID CommandTableOriginal[AHCI_MAX_COMMAND_SLOTS];
    ULONG CommandTableSize[AHCI_MAX_COMMAND_SLOTS];
    ULONG CommandListSize;
    PHYSICAL_ADDRESS ReceivedFisPhysOriginal;
    PHYSICAL_ADDRESS CommandListPhysOriginal;
    PHYSICAL_ADDRESS CommandTablePhysOriginal[AHCI_MAX_COMMAND_SLOTS];
    PVOID LocalBuffer;
} CHANNEL_INFO_AHCI, *PCHANNEL_INFO_AHCI;

typedef struct _CHANNEL_DATA_AHCI
{
    /** Common data, must be the first member. */
    CHANNEL_DATA_COMMON;

    PVOID IoBase;
    PAHCI_RECEIVED_FIS ReceivedFis;
    PAHCI_COMMAND_LIST CommandList;
    PAHCI_COMMAND_TABLE CommandTable[AHCI_MAX_COMMAND_SLOTS];
    UCHAR LastPmpDeviceNumber;
    UCHAR LastFbsDeviceNumber;
    ULONG TotalPortCount;
    CHANNEL_INFO_AHCI Mem;
} CHANNEL_DATA_AHCI, *PCHANNEL_DATA_AHCI;

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
    /** Common data, must be the first member. */
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
    /** Common data, must be the first member. */
    COMMON_DEVICE_EXTENSION Common;

    ULONG Channel;
    ULONG Flags;
#define PDO_FLAG_NOT_PRESENT           0x00000001
#define PDO_FLAG_REPORTED_MISSING      0x00000002

    PCHANNEL_DATA_COMMON ChanData;

    LIST_ENTRY ListEntry;
} PDO_DEVICE_EXTENSION, *PPDO_DEVICE_EXTENSION;

#include "pata.h"

extern NTSYSAPI ULONG InitSafeBootMode;

/* acpi.c *********************************************************************/

BOOLEAN
AtaAcpiGetTimingMode(
    _In_ PDEVICE_OBJECT DeviceObject,
    _Out_ PIDE_ACPI_TIMING_MODE_BLOCK TimingMode);

NTSTATUS
AtaAcpiSetTimingMode(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIDE_ACPI_TIMING_MODE_BLOCK TimingMode,
    _In_opt_ PIDENTIFY_DEVICE_DATA IdBlock1,
    _In_opt_ PIDENTIFY_DEVICE_DATA IdBlock2);

CODE_SEG("PAGE")
PVOID
AtaAcpiGetTaskFile(
    _In_ PDEVICE_OBJECT DeviceObject);

CODE_SEG("PAGE")
VOID
AtaAcpiSetDeviceData(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIDENTIFY_DEVICE_DATA IdBlock);

/* fdo.c **********************************************************************/

CODE_SEG("PAGE")
CONTROLLER_ATTACH_CHANNEL AtaCtrlAttachChannel;

CODE_SEG("PAGE")
CHANNEL_SET_DEVICE_DATA AtaCtrlSetDeviceData;

CODE_SEG("PAGE")
CHANNEL_GET_INIT_TASK_FILE AtaCtrlGetInitTaskFile;

CHANNEL_SET_MODE AtaCtrlSetTransferMode;
CHANNEL_DOWNGRADE_INTERFACE_SPEED AtaCtrlDowngradeInterfaceSpeed;
CHANNEL_ABORT_CHANNEL AtaCtrlAbortChannel;

DECLSPEC_NOINLINE_FROM_PAGED
VOID
AtaChanEnableInterruptsSync(
    _In_ PVOID ChannelContext,
    _In_ BOOLEAN Enable);

CODE_SEG("PAGE")
PVOID
AtaCtrlPciGetBar(
    _In_ PATA_CONTROLLER Controller,
    _In_range_(0, PCI_TYPE0_ADDRESSES) ULONG Index,
    _In_ ULONG MinimumIoLength);

CODE_SEG("PAGE")
IDE_CHANNEL_STATE
PciIdeXGetChannelState(
    _In_ PATA_CONTROLLER Controller,
    _In_ ULONG Channel);

CODE_SEG("PAGE")
NTSTATUS
PciIdeXFdoDispatchPnp(
    _In_ PFDO_DEVICE_EXTENSION FdoExtension,
    _Inout_ PIRP Irp);

/* miniport.c *****************************************************************/

CODE_SEG("PAGE")
IDE_CHANNEL_STATE
PciIdeXChannelState(
    _In_ PFDO_DEVICE_EXTENSION FdoExtension,
    _In_ ULONG Channel);

/* pciidex.c ******************************************************************/

CODE_SEG("PAGE")
DRIVER_INITIALIZE DriverEntry;

CODE_SEG("PAGE")
DRIVER_UNLOAD PciIdeXUnload;

CODE_SEG("PAGE")
DRIVER_ADD_DEVICE PciIdeXAddDevice;

_Dispatch_type_(IRP_MJ_SYSTEM_CONTROL)
CODE_SEG("PAGE")
DRIVER_DISPATCH_PAGED PciIdeXDispatchWmi;

IO_COMPLETION_ROUTINE PciIdeXPdoCompletionRoutine;

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

VOID
AtaSleep(VOID);

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

/* pdo.c **********************************************************************/

_Dispatch_type_(IRP_MJ_PNP)
CODE_SEG("PAGE")
DRIVER_DISPATCH_PAGED PciIdeXDispatchPnp;

CODE_SEG("PAGE")
NTSTATUS
PciIdeXPdoRemoveDevice(
    _In_ PPDO_DEVICE_EXTENSION PdoExtension,
    _In_ PIRP Irp,
    _In_ BOOLEAN FinalRemove,
    _In_ BOOLEAN LockNeeded);

/* power.c ********************************************************************/

_Dispatch_type_(IRP_MJ_POWER)
DRIVER_DISPATCH_RAISED PciIdeXDispatchPower;

/* ahci_generic.c *************************************************************/

CODE_SEG("PAGE")
NTSTATUS
AhciGetControllerProperties(
    _Inout_ PATA_CONTROLLER Controller);

/* ahci_hw.c ******************************************************************/

CHANNEL_ENABLE_INTERRUPTS AtaAhciEnableInterrupts;
CHANNEL_ENUMERATE_CHANNEL AtaAhciEnumerateChannel;
CHANNEL_IDENTIFY_DEVICE AtaAhciIdentifyDevice;
CHANNEL_RESET_CHANNEL AtaAhciResetChannel;

VOID
AtaAhciSaveTaskFile(
    _In_ PCHANNEL_DATA_AHCI ChanData,
    _Inout_ PATA_DEVICE_REQUEST Request,
    _In_ BOOLEAN ProcessErrorStatus);

VOID
AtaAhciHandleFatalError(
    _In_ PCHANNEL_DATA_AHCI ChanData);

VOID
AtaAhciHandlePortStateChange(
    _In_ PCHANNEL_DATA_AHCI ChanData,
    _In_ ULONG InterruptStatus);

BOOLEAN
AtaAhciDowngradeInterfaceSpeed(
    _In_ PCHANNEL_DATA_AHCI ChanData);

ULONG
AtaAhciChannelGetMaximumDeviceCount(
    _In_ PVOID ChannelContext);

/* ahci_io.c ******************************************************************/

CHANNEL_START_IO AtaAhciStartIo;
CHANNEL_PREPARE_IO AtaAhciPrepareIo;
CHANNEL_PREPARE_PRD_TABLE AtaAhciPreparePrdTable;
CHANNEL_ALLOCATE_SLOT AtaAhciAllocateSlot;
KSERVICE_ROUTINE AtaAhciHbaIsr;

/* pata_generic.c *************************************************************/

CHANNEL_SET_MODE_EX SataSetTransferMode;

CODE_SEG("PAGE")
CONTROLLER_ATTACH_CHANNEL_EX PciIdeAttachChannel;

VOID
AtaSelectTimings(
    _In_reads_(MAX_IDE_DEVICE) PCHANNEL_DEVICE_CONFIG* DeviceList,
    _Out_writes_all_(MAX_IDE_DEVICE) PATA_TIMING Timings,
    _In_range_(>, 0) ULONG ClockPeriodPs,
    _In_ ULONG Flags);

CODE_SEG("PAGE")
VOID
PciIdeInitTaskFileIoResources(
    _In_ PCHANNEL_DATA_PATA ChanData,
    _In_ ULONG_PTR CommandPortBase,
    _In_ ULONG_PTR ControlPortBase,
    _In_ ULONG CommandBlockSpare);

CODE_SEG("PAGE")
IDE_CHANNEL_STATE
PciIdeGetChannelState(
    _In_ PATA_CONTROLLER Controller,
    _In_ ULONG Channel);

CODE_SEG("PAGE")
NTSTATUS
PciIdeConnectInterrupt(
    _In_ PVOID ChannelContext);

CODE_SEG("PAGE")
NTSTATUS
PciIdeCreateChannelData(
    _In_ PATA_CONTROLLER Controller,
    _In_ ULONG HwExtensionSize);

CODE_SEG("PAGE")
NTSTATUS
PciIdeGetControllerProperties(
    _Inout_ PATA_CONTROLLER Controller);

/* pata_hw.c ******************************************************************/

CHANNEL_RESET_CHANNEL PataResetChannel;
CHANNEL_ENUMERATE_CHANNEL PataEnumerateChannel;
CHANNEL_IDENTIFY_DEVICE PataIdentifyDevice;

ULONG
PataChannelGetMaximumDeviceCount(
    _In_ PVOID ChannelContext);

/* pata_io.c ******************************************************************/

CHANNEL_ALLOCATE_SLOT PataAllocateSlot;
CHANNEL_PREPARE_PRD_TABLE PciIdePreparePrdTable;
CHANNEL_PREPARE_IO PataPrepareIo;
CHANNEL_START_IO PataStartIo;
KSERVICE_ROUTINE PataChannelIsr;
KSERVICE_ROUTINE PciIdeChannelIsr;
CHANNEL_LOAD_TASK_FILE PataLoadTaskFile;
CHANNEL_SAVE_TASK_FILE PataSaveTaskFile;
CHANNEL_READ_STATUS PataReadStatus;
KDEFERRED_ROUTINE PataPollingTimerDpc;

VOID
PciIdeDmaStop(
    _In_ PCHANNEL_DATA_PATA ChanData);

/* misc ***********************************************************************/

CODE_SEG("PAGE")
NTSTATUS
AmdGetControllerProperties(
    _Inout_ PATA_CONTROLLER Controller);

CODE_SEG("PAGE")
NTSTATUS
AtiGetControllerProperties(
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
SvwGetControllerProperties(
    _Inout_ PATA_CONTROLLER Controller);

CODE_SEG("PAGE")
NTSTATUS
ToshibaGetControllerProperties(
    _Inout_ PATA_CONTROLLER Controller);

CODE_SEG("PAGE")
NTSTATUS
ViaGetControllerProperties(
    _Inout_ PATA_CONTROLLER Controller);

CHANNEL_SET_MODE_EX SvwSetTransferMode;

CODE_SEG("PAGE")
BOOLEAN
SvwHasUdmaCable(
    _In_ PATA_CONTROLLER Controller,
    _In_ ULONG Channel);

FORCEINLINE
ULONG
CountSetBits(
    _In_ ULONG x)
{
    x -= x >> 1 & 0x55555555;
    x = (x & 0x33333333) + (x >> 2 & 0x33333333);

    return ((x + (x >> 4)) & 0x0F0F0F0F) * 0x01010101 >> 24;
}

FORCEINLINE
BOOLEAN
IsPowerOfTwo(
    _In_ ULONG x)
{
    /* Also exclude zero numbers */
    return (x != 0) && ((x & (x - 1)) == 0);
}

FORCEINLINE
UCHAR
PciRead8(
    _In_ PATA_CONTROLLER Controller,
    _In_ ULONG ConfigDataOffset)
{
    UCHAR Result;

    PciRead(Controller, &Result, ConfigDataOffset, sizeof(Result));
    return Result;
}

FORCEINLINE
USHORT
PciRead16(
    _In_ PATA_CONTROLLER Controller,
    _In_ ULONG ConfigDataOffset)
{
    USHORT Result;

    PciRead(Controller, &Result, ConfigDataOffset, sizeof(Result));
    return Result;
}

FORCEINLINE
ULONG
PciRead32(
    _In_ PATA_CONTROLLER Controller,
    _In_ ULONG ConfigDataOffset)
{
    ULONG Result;

    PciRead(Controller, &Result, ConfigDataOffset, sizeof(Result));
    return Result;
}

FORCEINLINE
VOID
PciWrite8(
    _In_ PATA_CONTROLLER Controller,
    _In_ ULONG ConfigDataOffset,
    _In_ UCHAR Value)
{
    PciWrite(Controller, &Value, ConfigDataOffset, sizeof(Value));
}

FORCEINLINE
VOID
PciWrite16(
    _In_ PATA_CONTROLLER Controller,
    _In_ ULONG ConfigDataOffset,
    _In_ USHORT Value)
{
    PciWrite(Controller, &Value, ConfigDataOffset, sizeof(Value));
}

FORCEINLINE
VOID
PciWrite32(
    _In_ PATA_CONTROLLER Controller,
    _In_ ULONG ConfigDataOffset,
    _In_ ULONG Value)
{
    PciWrite(Controller, &Value, ConfigDataOffset, sizeof(Value));
}
