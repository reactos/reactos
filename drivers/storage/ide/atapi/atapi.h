/*
 * PROJECT:     ReactOS ATA Port Driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Common header file
 * COPYRIGHT:   Copyright 2024 Dmitry Borisov <di.sean@protonmail.com>
 */

#pragma once

#include <ntddk.h>
#include <ntstrsafe.h>
#include <ntintsafe.h>

#include <wmilib.h>
#include <initguid.h>
#include <wdmguid.h>
#include <wmistr.h>
#include <wmidata.h>

#include <ata.h>
#include <ide.h>
#include <scsi.h>
#include <ntddscsi.h>
#include <ntdddisk.h>

#include <pseh/pseh2.h>
#include <section_attribs.h>
#include <debug/driverdbg.h>
#include <reactos/drivers/ntddata.h>

typedef struct _ATAPORT_CHANNEL_EXTENSION ATAPORT_CHANNEL_EXTENSION, *PATAPORT_CHANNEL_EXTENSION;
typedef struct _ATAPORT_DEVICE_EXTENSION ATAPORT_DEVICE_EXTENSION, *PATAPORT_DEVICE_EXTENSION;

typedef enum
{
    DEV_ATA = 1,
    DEV_ATAPI = 2,
    DEV_UNKNOWN = 3
} ATA_DEVICE_CLASS;

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
    PUCHAR LbaLow;             //!< LBA bits 0-7, 24-31
    union
    {
        PUCHAR LbaMid;         //!< LBA bits 8-15, 32-39
        PUCHAR ByteCountLow;
        PUCHAR SignatureLow;
    };
    union
    {
        PUCHAR LbaHigh;        //!< LBA bits 16-23, 40-47
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
} IDE_REGISTERS, *PIDE_REGISTERS;

#include "include/debug.h"
#include "include/acpi.h"
#include "include/ahci.h"
// #include "include/pata.h"
#include "include/identify_funcs.h"
#include "include/scsiex.h"
#include "include/request.h"
#include "include/devevent.h"

#if defined(_MSC_VER)
#pragma section("PAGECONS", read)
#endif

/** Pageable read-only data */
#define ATAPORT_DATA    DATA_SEG("PAGECONS")

#define ASSUME(cond) \
    do { \
        ASSERT(cond); \
        __assume(cond); \
    } while (0)

#define ATAPORT_TAG             'PedI'

#define IS_FDO(p) \
    ((((PATAPORT_COMMON_EXTENSION)(p))->Flags & DO_IS_FDO) != 0)

#define IS_AHCI(p) \
    ((((PATAPORT_COMMON_EXTENSION)(p))->Flags & DO_IS_AHCI) != 0)

#define IS_PCIIDE(p) \
    ((((PATAPORT_COMMON_EXTENSION)(p))->Flags & DO_IS_PCI_IDE) != 0)

#define IS_LEGACY_IDE(p) \
    ((((PATAPORT_COMMON_EXTENSION)(p))->Flags & DO_IS_LEGACY_IDE) != 0)

#define IS_ATAPI(DevExt) \
    (((DevExt)->Flags & DEVICE_IS_ATAPI) != 0)

#define ATAPORT_FN_FIELD   40
#define ATAPORT_SN_FIELD   40
#define ATAPORT_RN_FIELD   8

#define ATA_RESERVED_PAGES       4

#define ATA_MAX_LUN_COUNT        8

/* Maximum size (ATA Information VPD page) */
#define ATA_LOCAL_BUFFER_SIZE    572

typedef union _ATA_SCSI_ADDRESS
{
    /*
     * The ordering between Lun, TargetId, and PathId fields is important
     * with address comparison.
     */
    struct
    {
        /**
         * The lun number 0-7.
         * @sa ATA_MAX_LUN_COUNT
         */
        UCHAR Lun;

        /**
         * PATA:
         * 0 - Master, 1 - Slave,
         * 2 - Master (PC-98), 3 - Slave (PC-98).
         *
         * AHCI:
         * The device number 0-15.
         */
        UCHAR TargetId;

        /**
         * PATA:
         * Always zero.
         *
         * AHCI:
         * The channel number 0-31.
         */
        UCHAR PathId;

        UCHAR Reserved;
    };
    ULONG AsULONG;
} ATA_SCSI_ADDRESS, *PATA_SCSI_ADDRESS;

typedef struct _ATA_ENABLE_INTERRUPTS_CONTEXT
{
    PATAPORT_CHANNEL_EXTENSION ChanExt;
    BOOLEAN Enable;
} ATA_ENABLE_INTERRUPTS_CONTEXT, *PATA_ENABLE_INTERRUPTS_CONTEXT;

typedef struct _ATAPORT_AHCI_SLOT_DATA
{
    PAHCI_COMMAND_TABLE CommandTable;
    ULONG64 CommandTablePhys;
} ATAPORT_AHCI_SLOT_DATA, *PATAPORT_AHCI_SLOT_DATA;

typedef struct _ATAPORT_PATA_PORT_DATA
{
    PCIIDE_INTERFACE PciIdeInterface;
    IDE_REGISTERS Registers;
    PUCHAR DataBuffer;
    ULONG BytesToTransfer;
    ULONG CommandFlags;
    ULONG ByteCount;
    PUCHAR CommandPortBase;
    PUCHAR ControlPortBase;
    IDE_ACPI_TIMING_MODE_BLOCK TimingMode;
    PVOID LocalBuffer;
    KDPC Dpc;
} ATAPORT_PATA_PORT_DATA, *PATAPORT_PATA_PORT_DATA;

typedef struct _ATAPORT_AHCI_PORT_DATA
{
    KDPC Dpc;
    PULONG IoBase;
    SCATTER_GATHER_LIST LocalSgList;
    PAHCI_RECEIVED_FIS ReceivedFis;
    PAHCI_COMMAND_LIST CommandList;
    ULONG64 ReceivedFisPhys;
    ULONG64 CommandListPhys;
    ATAPORT_AHCI_SLOT_DATA Slot[AHCI_MAX_COMMAND_SLOTS];
} ATAPORT_AHCI_PORT_DATA, *PATAPORT_AHCI_PORT_DATA;

typedef struct DECLSPEC_CACHEALIGN _ATAPORT_PORT_DATA
{
    KSPIN_LOCK QueueLock;
    LIST_ENTRY PortQueueList;

    ULONG FreeSlotsBitmap;
    ULONG LastUsedSlot;
    ULONG QueueFlags;
#define PORT_QUEUE_FLAG_EXCLUSIVE_MODE            0x00000001

    /**
     * >0 means we have native queued commands pending.
     * 0 means the slot queue is empty.
     * <0 means we have non-queued commands pending.
     */
    LONG AllocatedSlots;

    DECLSPEC_CACHEALIGN KSPIN_LOCK PortLock;
    ULONG ActiveSlotsBitmap;
    ULONG ActiveQueuedSlotsBitmap;
    ULONG PortFlags;
#define PORT_FLAG_ACTIVE             0x00000001
#define PORT_FLAG_NCQ                0x00000002

#define PORT_FLAG_IO32               0x00000004
#define PORT_FLAG_IO_MODE_DETECTED   0x00000008
#define PORT_FLAG_CBUS_IDE           0x80000000

    PATA_DEVICE_REQUEST Slots[AHCI_MAX_COMMAND_SLOTS];
    LONG TimerCount[AHCI_MAX_COMMAND_SLOTS];
    union
    {
        ATAPORT_PATA_PORT_DATA Pata;
        ATAPORT_AHCI_PORT_DATA Ahci;
    };
} ATAPORT_PORT_DATA, *PATAPORT_PORT_DATA;

typedef struct _ATAPORT_AHCI_PORT_INFO
{
    PVOID ReceivedFisOriginal;
    PVOID CommandListOriginal;
    PVOID CommandTableOriginal[AHCI_MAX_COMMAND_SLOTS];
    ULONG CommandTableSize[AHCI_MAX_COMMAND_SLOTS];
    ULONG CommandListSize;
    ULONG ReceivedFisSize;
    PHYSICAL_ADDRESS LocalBufferPa;
    PHYSICAL_ADDRESS ReceivedFisPhysOriginal;
    PHYSICAL_ADDRESS CommandListPhysOriginal;
    PHYSICAL_ADDRESS CommandTablePhysOriginal[AHCI_MAX_COMMAND_SLOTS];
    PVOID LocalBuffer;
} ATAPORT_PORT_INFO, *PATAPORT_PORT_INFO;

typedef VOID
(PREPARE_PRD_TABLE)(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ PATA_DEVICE_REQUEST Request,
    _In_ SCATTER_GATHER_LIST* __restrict SgList);
typedef PREPARE_PRD_TABLE *PPREPARE_PRD_TABLE;

typedef UCHAR
(START_IO)(
    _In_ PATAPORT_PORT_DATA PortData,
    _In_ PATA_DEVICE_REQUEST Request);
typedef START_IO *PSTART_IO;

typedef VOID
(SEND_REQUEST)(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ PATA_DEVICE_REQUEST Request);
typedef SEND_REQUEST *PSEND_REQUEST;

typedef struct _ATAPORT_COMMON_EXTENSION
{
    _Write_guarded_by_(_Global_interlock_)
    volatile LONG PageFiles;

    _Write_guarded_by_(_Global_interlock_)
    volatile LONG HibernateFiles;

    _Write_guarded_by_(_Global_interlock_)
    volatile LONG DumpFiles;

    ULONG Flags;
#define DO_IS_AHCI       0x00000001
#define DO_IS_PCI_IDE    0x00000002
#define DO_IS_LEGACY_IDE 0x00000004
#define DO_IS_FDO        0x80000000

    PDEVICE_OBJECT Self;

    IO_REMOVE_LOCK RemoveLock;
} ATAPORT_COMMON_EXTENSION, *PATAPORT_COMMON_EXTENSION;

typedef struct _ATAPORT_CHANNEL_EXTENSION
{
    ATAPORT_COMMON_EXTENSION Common;
    PULONG IoBase;
    ULONG PortBitmap;
    ULONG NumberOfPorts;
    PATAPORT_PORT_DATA PortData;
    PDMA_ADAPTER AdapterObject;
    PDEVICE_OBJECT AdapterDeviceObject;
    PKINTERRUPT InterruptObject;
    ULONG MaximumTransferLength;
    ULONG AhciCapabilities;
    ULONG AhciCapabilitiesEx;
    ULONG MapRegisterCount;
    PDEVICE_OBJECT Ldo;
    PVOID ReservedVaSpace;
    volatile LONG ReservedMappingLock;
    FAST_MUTEX DeviceSyncMutex;
    SINGLE_LIST_ENTRY DeviceList;
    ULONG DeviceCount;
    PATAPORT_DEVICE_EXTENSION Device[MAX_IDE_DEVICE];

    ULONG Flags;
#define CHANNEL_CONTROL_PORT_BASE_MAPPED       0x00000001
#define CHANNEL_COMMAND_PORT_BASE_MAPPED       0x00000002
#define CHANNEL_INTERRUPT_SHARED               0x00000004
#define CHANNEL_PRIMARY_ADDRESS_CLAIMED        0x00000008
#define CHANNEL_SECONDARY_ADDRESS_CLAIMED      0x00000010
#define CHANNEL_SYMLINK_CREATED                0x00000020
#define CHANNEL_HAS_GTM                        0x00000040
#define CHANNEL_IO_TIMER_ACTIVE                0x00000080

    ULONG ChannelNumber;
    UCHAR PortNumber;
    UCHAR PathId;
    KIRQL InterruptLevel;
    ULONG InterruptVector;
    ULONG IoLength;
    USHORT DeviceID;
    USHORT VendorID;
    KINTERRUPT_MODE InterruptMode;
    KAFFINITY InterruptAffinity;
    UNICODE_STRING StorageInterfaceName;
    PDEVICE_OBJECT Pdo;
    PATAPORT_PORT_INFO PortInfo;
    BUS_INTERFACE_STANDARD BusInterface;
} ATAPORT_CHANNEL_EXTENSION, *PATAPORT_CHANNEL_EXTENSION;

typedef struct _ATAPORT_DEVICE_EXTENSION
{
    ATAPORT_COMMON_EXTENSION Common;

    PATAPORT_CHANNEL_EXTENSION ChanExt;

    ATA_SCSI_ADDRESS AtaScsiAddress;

    PULONG PowerIdleCounter;

    ULONG Flags;
#define DEVICE_PIO_ONLY                          0x00000001
#define DEVICE_IS_ATAPI                          0x00000002
#define DEVICE_HAS_CDB_INTERRUPT                 0x00000004
#define DEVICE_LBA_MODE                          0x00000008
#define DEVICE_LBA48                             0x00000010
#define DEVICE_HAS_FUA                           0x00000020
#define DEVICE_NCQ                               0x00000040
#define DEVICE_IS_NEC_CDR260                     0x00000080
#define DEVICE_HAS_MEDIA_STATUS                  0x00000100
#define DEVICE_NEED_DMA_DIRECTION                0x00000200
#define DEVICE_SENSE_DATA_REPORTING              0x00000400
#define DEVICE_IS_SUPER_FLOPPY                   0x00000800

#define DEVICE_ENUM                              0x00001000
#define DEVICE_ACTIVE                            0x00002000
#define DEVICE_SIMPLEX_DMA                       0x00004000

    BOOLEAN DeviceClaimed;
    BOOLEAN ReportedMissing;

    UCHAR DeviceSelect;
    UCHAR CdbSize;
    UCHAR MultiSectorTransfer;
    USHORT Cylinders;
    USHORT Heads;
    USHORT SectorsPerTrack;
    ULONG SectorSize;
    ULONG64 TotalSectors;

    KSPIN_LOCK QueueLock;
    LIST_ENTRY RequestList;

    union
    {
        IDENTIFY_DEVICE_DATA IdentifyDeviceData;
        IDENTIFY_PACKET_DATA IdentifyPacketData;
    };
    INQUIRYDATA InquiryData;

    _Field_z_ CHAR FriendlyName[ATAPORT_FN_FIELD + sizeof(ANSI_NULL)];
    _Field_z_ CHAR RevisionNumber[ATAPORT_RN_FIELD + sizeof(ANSI_NULL)];
    _Field_z_ CHAR SerialNumber[ATAPORT_SN_FIELD + sizeof(ANSI_NULL)];

    WMILIB_CONTEXT WmiLibInfo;
    SINGLE_LIST_ENTRY ListEntry;

    PVOID GtfDataBuffer;

    KEVENT QueueStoppedEvent;
    PATA_DEVICE_REQUEST Requests;
    ULONG QueueFlags;
#define QUEUE_FLAG_FROZEN_DEVICE_ERROR       0x00000001
#define QUEUE_FLAG_FROZEN_SLOT               0x00000002
#define QUEUE_FLAG_FROZEN_PNP                0x00000004
#define QUEUE_FLAG_FROZEN_ENUM               0x00000008
#define QUEUE_FLAG_FROZEN_QUEUE_FREEZE       0x00000010
#define QUEUE_FLAG_SIGNAL_STOP               0x00000020
#define QUEUE_FLAG_FROZEN_QUEUE_LOCK         0x00080000

#define QUEUE_FLAGS_FROZEN \
    (QUEUE_FLAG_FROZEN_DEVICE_ERROR | \
     QUEUE_FLAG_FROZEN_SLOT | \
     QUEUE_FLAG_FROZEN_PNP | \
     QUEUE_FLAG_FROZEN_QUEUE_FREEZE | \
     QUEUE_FLAG_FROZEN_QUEUE_LOCK | \
     QUEUE_FLAG_FROZEN_ENUM)

#define QUEUE_FLAGS_BYPASS_FROZEN \
    (QUEUE_FLAGS_FROZEN & ~QUEUE_FLAG_FROZEN_QUEUE_LOCK)

#define QUEUE_FLAGS_BYPASS_LOCKED \
    (QUEUE_FLAGS_FROZEN & ~QUEUE_FLAG_FROZEN_QUEUE_FREEZE)

    ULONG MaxRequestsBitmap;
    ULONG FreeRequestsBitmap;
    ULONG MaxQueuedSlotsBitmap;

    ULONG RequestsCompleted;
    ULONG RequestsStarted;
    LIST_ENTRY BypassQueueList;

    SLIST_HEADER CompletionQueueList;
    LIST_ENTRY DeviceQueueList;
    PLIST_ENTRY NextDeviceQueueEntry;
    PATAPORT_PORT_DATA PortData;
    PSTART_IO StartIo;
    PSEND_REQUEST SendRequest;
    PPREPARE_PRD_TABLE PreparePrdTable;
    PATA_DEVICE_REQUEST InternalRequest;

    ATA_WORKER_CONTEXT WorkerContext;

    KDPC CompletionDpc;

    PVOID LocalBuffer;
} ATAPORT_DEVICE_EXTENSION, *PATAPORT_DEVICE_EXTENSION;

C_ASSERT(QUEUE_FLAG_FROZEN_QUEUE_FREEZE == SRB_FLAGS_BYPASS_FROZEN_QUEUE);
C_ASSERT(QUEUE_FLAG_FROZEN_QUEUE_LOCK == SRB_FLAGS_BYPASS_LOCKED_QUEUE);

#include "include/pata.h"
#include "include/atahw.h"

extern UNICODE_STRING AtapDriverRegistryPath;
extern BOOLEAN AtapInPEMode;

typedef enum
{
    GetDeviceType,
    GetGenericType,
    GetPeripheralId
} DEVICE_TYPE_NAME;

FORCEINLINE
ATA_SCSI_ADDRESS
AtaMarshallScsiAddress(
    _In_ ULONG PathId,
    _In_ ULONG TargetId,
    _In_ ULONG Lun)
{
    ATA_SCSI_ADDRESS AtaScsiAddress;

    AtaScsiAddress.Lun = Lun;
    AtaScsiAddress.TargetId = TargetId;
    AtaScsiAddress.PathId = PathId;
    AtaScsiAddress.Reserved = 0;

    return AtaScsiAddress;
}

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

/* acpi.c *********************************************************************/

CODE_SEG("PAGE")
BOOLEAN
AtaAcpiGetTimingMode(
    _In_ PATAPORT_CHANNEL_EXTENSION ChanExt,
    _Out_ PIDE_ACPI_TIMING_MODE_BLOCK TimingMode);

CODE_SEG("PAGE")
VOID
AtaAcpiSetTimingMode(
    _In_ PATAPORT_CHANNEL_EXTENSION ChanExt,
    _In_ PIDE_ACPI_TIMING_MODE_BLOCK TimingMode,
    _In_ PIDENTIFY_DEVICE_DATA IdBlock1,
    _In_ PIDENTIFY_DEVICE_DATA IdBlock2);

CODE_SEG("PAGE")
PVOID
AtaAcpiGetTaskFile(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt);

CODE_SEG("PAGE")
VOID
AtaAcpiSetDeviceData(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ PIDENTIFY_DEVICE_DATA IdBlock);

/* atapi.c ********************************************************************/

_Dispatch_type_(IRP_MJ_CREATE)
_Dispatch_type_(IRP_MJ_CLOSE)
CODE_SEG("PAGE")
DRIVER_DISPATCH_PAGED AtaDispatchCreateClose;

_Dispatch_type_(IRP_MJ_POWER)
DRIVER_DISPATCH_RAISED AtaDispatchPower;

CODE_SEG("PAGE")
DRIVER_ADD_DEVICE AtaAddDevice;

CODE_SEG("PAGE")
DRIVER_UNLOAD AtaUnload;

CODE_SEG("INIT")
DRIVER_INITIALIZE DriverEntry;

CODE_SEG("PAGE")
NTSTATUS
NTAPI
AtaAddChannel(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PDEVICE_OBJECT PhysicalDeviceObject,
    _Out_ PATAPORT_CHANNEL_EXTENSION *FdoExtension);

CODE_SEG("PAGE")
NTSTATUS
AtaFdoQueryInterface(
    _In_ PATAPORT_CHANNEL_EXTENSION ChanExt,
    _In_ const GUID* Guid,
    _Out_ PVOID Interface,
    _In_ ULONG Size);

CODE_SEG("PAGE")
NTSTATUS
AtaOpenRegistryKey(
    _In_ PHANDLE KeyHandle,
    _In_ HANDLE RootKey,
    _In_ PUNICODE_STRING KeyName,
    _In_ BOOLEAN Create);

CODE_SEG("PAGE")
VOID
AtaGetRegistryKey(
    _In_ PATAPORT_CHANNEL_EXTENSION ChanExt,
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ PCWSTR KeyName,
    _Out_ PULONG KeyValue,
    _In_ ULONG DefaultValue);

CODE_SEG("PAGE")
VOID
AtaSetRegistryKey(
    _In_ PATAPORT_CHANNEL_EXTENSION ChanExt,
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ PCWSTR KeyName,
    _In_ ULONG KeyValue);

/* data.c *********************************************************************/

CODE_SEG("PAGE")
PCSTR
AtaTypeCodeToName(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ DEVICE_TYPE_NAME Type);

/* devevent.c *****************************************************************/

KDEFERRED_ROUTINE AtaDeviceWorker;
REQUEST_COMPLETION_ROUTINE AdaDeviceCompleteInternalRequest;

DECLSPEC_NOINLINE_FROM_PAGED
VOID
AtaDeviceQueueAction(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ ATA_DEVICE_ACTION Action);

VOID
AtaDeviceTimeout(
    _In_ PATAPORT_PORT_DATA PortData,
    _In_ ULONG Slot);


VOID
AtaDeviceSetupIdentify(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ UCHAR Command);

BOOLEAN
AtaDeviceIdentifyDataEqual(
    _In_ PIDENTIFY_DEVICE_DATA IdentifyData1,
    _In_ PIDENTIFY_DEVICE_DATA IdentifyData2);

/* enum.c *********************************************************************/

VOID
AtaPdoSetAddressingMode(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt);

VOID
AtaCopyAtaString(
    _Out_writes_bytes_all_(Length) PUCHAR Destination,
    _In_reads_bytes_(Length) PUCHAR Source,
    _In_ ULONG Length);

CODE_SEG("PAGE")
PCHAR
AtaCopyIdString(
    _Out_writes_bytes_all_(MaxLength) PCHAR Destination,
    _In_reads_bytes_(MaxLength) PUCHAR Source,
    _In_ ULONG MaxLength,
    _In_ CHAR DefaultCharacter);

CODE_SEG("PAGE")
NTSTATUS
AtaFdoQueryBusRelations(
    _In_ PATAPORT_CHANNEL_EXTENSION ChanExt,
    _In_ PIRP Irp);

/* fdo.c **********************************************************************/

CODE_SEG("PAGE")
NTSTATUS
AtaFdoStartDevice(
    _In_ PATAPORT_CHANNEL_EXTENSION ChanExt,
    _In_ PCM_RESOURCE_LIST ResourcesTranslated);

CODE_SEG("PAGE")
NTSTATUS
AtaFdoRemoveDevice(
    _In_ PATAPORT_CHANNEL_EXTENSION ChanExt,
    _In_ PIRP Irp,
    _In_ BOOLEAN FinalRemove);

CODE_SEG("PAGE")
VOID
AtaFdoDeviceListRemove(
    _In_ PATAPORT_CHANNEL_EXTENSION ChanExt,
    _In_ PATAPORT_DEVICE_EXTENSION DevExt);

CODE_SEG("PAGE")
VOID
AtaFdoDeviceListInsert(
    _In_ PATAPORT_CHANNEL_EXTENSION ChanExt,
    _In_ PATAPORT_DEVICE_EXTENSION DevExt);

CODE_SEG("PAGE")
PATAPORT_DEVICE_EXTENSION
AtaFdoFindDeviceByPath(
    _In_ PATAPORT_CHANNEL_EXTENSION ChanExt,
    _In_ ATA_SCSI_ADDRESS AtaScsiAddress);

CODE_SEG("PAGE")
NTSTATUS
AtaFdoPnp(
    _In_ PATAPORT_CHANNEL_EXTENSION ChanExt,
    _Inout_ PIRP Irp);

/* hwahci.c *******************************************************************/

PREPARE_PRD_TABLE AtaAhciPreparePrdTable;
START_IO AtaAhciStartIo;
KSERVICE_ROUTINE AtaHbaIsr;
KDEFERRED_ROUTINE AtaAhciPortDpc;

CODE_SEG("PAGE")
NTSTATUS
AtaAhciInitHba(
    _In_ PATAPORT_CHANNEL_EXTENSION ChanExt);

CODE_SEG("PAGE")
BOOLEAN
AtaAhciPortInit(
    _In_ PATAPORT_CHANNEL_EXTENSION ChanExt,
    _Out_ PATAPORT_PORT_DATA PortData,
    _Out_ PATAPORT_PORT_INFO PortInfo);

CODE_SEG("PAGE")
VOID
AtaFdoFreePortMemory(
    _In_ PATAPORT_CHANNEL_EXTENSION ChanExt);

VOID
AtaAhciSaveTaskFile(
    _In_ PATAPORT_PORT_DATA PortData,
    _Inout_ PATA_DEVICE_REQUEST Request);

BOOLEAN
AtaAhciDowngradeInterfaceSpeed(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt);

BOOLEAN
AtaAhciResetDevice(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt);

BOOLEAN
AtaAhciEnumerateDevice(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt);

/* hwpata.c *******************************************************************/

PREPARE_PRD_TABLE AtaPciIdePreparePrdTable;
START_IO AtaPataStartIo;
KSERVICE_ROUTINE AtaPataChannelIsr;
KSERVICE_ROUTINE AtaPciIdeChannelIsr;
KDEFERRED_ROUTINE AtaPataChannelDpc;

BOOLEAN
AtaPataPreparePioDataTransfer(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ PATA_DEVICE_REQUEST Request);

BOOLEAN
AtaPataResetDevice(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt);

BOOLEAN
AtaPataEnumerateDevice(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt);

/* ioctl.c ********************************************************************/

_Dispatch_type_(IRP_MJ_DEVICE_CONTROL)
DRIVER_DISPATCH_RAISED AtaDispatchDeviceControl;

/* legacy.c *******************************************************************/

#if defined(ATA_DETECT_LEGACY_DEVICES)
CODE_SEG("INIT")
VOID
AtaDetectLegacyDevices(
    _In_ PDRIVER_OBJECT DriverObject);
#endif

/* pdo.c **********************************************************************/

_Dispatch_type_(IRP_MJ_PNP)
CODE_SEG("PAGE")
DRIVER_DISPATCH_PAGED AtaDispatchPnp;

CODE_SEG("PAGE")
VOID
AtaPdoFreeDevice(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt);

CODE_SEG("PAGE")
PATAPORT_DEVICE_EXTENSION
AtaPdoCreateDevice(
    _In_ PATAPORT_CHANNEL_EXTENSION ChanExt,
    _In_ ATA_SCSI_ADDRESS AtaScsiAddress);

CODE_SEG("PAGE")
VOID
AtaPdoUpdateScsiAddress(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ ATA_SCSI_ADDRESS AtaScsiAddress);

/* satl.c *********************************************************************/

CODE_SEG("PAGE")
VOID
AtaCreateStandardInquiryData(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt);

UCHAR
AtaReqExecuteScsi(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ PATA_DEVICE_REQUEST Request,
    _In_ PSCSI_REQUEST_BLOCK Srb);

UCHAR
AtaReqTranslateFixedError(
    _In_ PATA_DEVICE_REQUEST Request);

VOID
AtaReqBuildReadLogTaskFile(
    _In_ PATA_DEVICE_REQUEST Request,
    _In_ UCHAR LogAddress,
    _In_ UCHAR PageNumber,
    _In_ USHORT LogPageCount);

/* scsi.c *********************************************************************/

_Dispatch_type_(IRP_MJ_SCSI)
DRIVER_DISPATCH_RAISED AtaDispatchScsi;

SEND_REQUEST AtaReqSendRequest;
IO_TIMER_ROUTINE AtaReqPortIoTimer;
KDEFERRED_ROUTINE AtaReqCompletionDpc;

VOID
AtaReqCompleteFailedRequest(
    _In_ PATA_DEVICE_REQUEST Request);

VOID
AtaReqCompleteRequest(
    _In_ PATA_DEVICE_REQUEST Request);

UCHAR
AtaReqSetFixedSenseData(
    _In_ PSCSI_REQUEST_BLOCK Srb,
    _In_ SCSI_SENSE_CODE SenseCode);

VOID
AtaReqSetLbaInformation(
    _In_ PSCSI_REQUEST_BLOCK Srb,
    _In_ ULONG64 Lba);

BOOLEAN
AtaReqAllocateMdl(
    _In_ PATA_DEVICE_REQUEST Request);

VOID
AtaReqStartIo(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ PATA_DEVICE_REQUEST Request);

VOID
AtaReqStartCompletionDpc(
    _In_ PATA_DEVICE_REQUEST Request);

DECLSPEC_NOINLINE_FROM_PAGED
NTSTATUS
AtaReqStartSrb(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ PSCSI_REQUEST_BLOCK Srb);

DECLSPEC_NOINLINE_FROM_PAGED
VOID
AtaReqFreezeQueue(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ ULONG ReasonFlags);

DECLSPEC_NOINLINE_FROM_PAGED
VOID
AtaReqThawQueue(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ ULONG ReasonFlags);

DECLSPEC_NOINLINE_FROM_PAGED
BOOLEAN
AtaReqWaitForOutstandingIoToComplete(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ ULONG TimeOut);

DECLSPEC_NOINLINE_FROM_PAGED
VOID
AtaReqFlushDeviceQueue(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt);

/* smart.c ********************************************************************/

UCHAR
AtaReqSmartIoControl(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ PATA_DEVICE_REQUEST Request,
    _In_ PSCSI_REQUEST_BLOCK Srb);

CODE_SEG("PAGE")
NTSTATUS
AtaPdoHandleMiniportSmartVersion(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ PSCSI_REQUEST_BLOCK Srb);

CODE_SEG("PAGE")
NTSTATUS
AtaPdoHandleMiniportIdentify(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ PSCSI_REQUEST_BLOCK Srb);

/* timings.c ******************************************************************/



/* wmi.c **********************************************************************/

_Dispatch_type_(IRP_MJ_SYSTEM_CONTROL)
CODE_SEG("PAGE")
DRIVER_DISPATCH_PAGED AtaDispatchWmi;

CODE_SEG("PAGE")
NTSTATUS
AtaPdoWmiRegistration(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ BOOLEAN Register);
