/*
 * PROJECT:     ReactOS ATA Port Driver
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Common header file
 * COPYRIGHT:   Copyright 2026 Dmitry Borisov <di.sean@protonmail.com>
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

#include <sptilib.h>
#include <section_attribs.h>
#include <debug/driverdbg.h>
#include <reactos/drivers/ata/ata_shared.h>
#include <reactos/drivers/ata/ata_user.h>
#include <reactos/drivers/ata/identify_funcs.h>

typedef struct _ATAPORT_CHANNEL_EXTENSION ATAPORT_CHANNEL_EXTENSION, *PATAPORT_CHANNEL_EXTENSION;
typedef struct _ATAPORT_DEVICE_EXTENSION ATAPORT_DEVICE_EXTENSION, *PATAPORT_DEVICE_EXTENSION;
typedef struct _ATAPORT_PORT_DATA ATAPORT_PORT_DATA, *PATAPORT_PORT_DATA;
typedef struct _ATAPORT_IO_CONTEXT ATAPORT_IO_CONTEXT, *PATAPORT_IO_CONTEXT;

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
         * The device number 0 - Master, 1 - Slave,
         * 2 - Master (PC-98), 3 - Slave (PC-98).
         *
         * AHCI:
         * The device number 0-15.
         */
        UCHAR TargetId;

        /**
         * The ATA channel number 0-31.
         */
        UCHAR PathId;

        UCHAR IsValid;
    };
    ULONG AsULONG;
} ATA_SCSI_ADDRESS, *PATA_SCSI_ADDRESS;

typedef VOID
(STOP_IO_CALLBACK)(
    _In_ PATAPORT_IO_CONTEXT Device);
typedef STOP_IO_CALLBACK *PSTOP_IO_CALLBACK;

typedef struct _ATAPORT_IO_CONTEXT
{
    ATA_IO_CONTEXT_COMMON;

    ULONG DeviceFlags;
#define DEVICE_PIO_ONLY                          0x00000001
#define DEVICE_LBA_MODE                          0x00000002
#define DEVICE_LBA48                             0x00000004
#define DEVICE_HAS_FUA                           0x00000008
#define DEVICE_NCQ                               0x00000010
#define DEVICE_HAS_MEDIA_STATUS                  0x00000020
#define DEVICE_SENSE_DATA_REPORTING              0x00000040
#define DEVICE_IS_SUPER_FLOPPY                   0x00000080
#define DEVICE_IS_PDO_REMOVABLE                  0x00000100
#define DEVICE_UNINITIALIZED                     0x00000200
#define DEVICE_PNP_STARTED                       0x00000400
#define DEVICE_DESCRIPTOR_SENSE                  0x00000800
#define DEVICE_PIO_VIA_DMA                       0x00001000
#define DEVICE_PIO_FOR_LBA48_XFER                0x00002000
#define DEVICE_CACHE_INQUIRY                     0x00004000

    PULONG PowerIdleCounter;
    PVOID LocalBuffer;
    ATA_SCSI_ADDRESS AtaScsiAddress;
    PATAPORT_PORT_DATA PortData;
    KSPIN_LOCK QueueLock;

    LONG QueueFlags;
#define QUEUE_FLAG_FROZEN_PORT_BUSY          0x00000001
#define QUEUE_FLAG_FROZEN_SLOT               0x00000002
#define QUEUE_FLAG_FROZEN_PNP                0x00000004
#define QUEUE_FLAG_SIGNAL_STOP               0x00000008
#define QUEUE_FLAG_FROZEN_QUEUE_FREEZE       0x00000010
#define QUEUE_FLAG_FROZEN_POWER              0x00000020
#define QUEUE_FLAG_FROZEN_REMOVED            0x00000040
#define QUEUE_FLAG_FROZEN_QUEUE_LOCK         0x00080000

#define QUEUE_FLAGS_FROZEN \
    (QUEUE_FLAG_FROZEN_PORT_BUSY | \
     QUEUE_FLAG_FROZEN_SLOT | \
     QUEUE_FLAG_FROZEN_PNP | \
     QUEUE_FLAG_FROZEN_QUEUE_FREEZE | \
     QUEUE_FLAG_FROZEN_POWER | \
     QUEUE_FLAG_FROZEN_REMOVED | \
     QUEUE_FLAG_FROZEN_QUEUE_LOCK)

#define QUEUE_FLAGS_FROZEN_NOT_BYPASS \
    (QUEUE_FLAGS_FROZEN & ~(QUEUE_FLAG_FROZEN_QUEUE_FREEZE | QUEUE_FLAG_FROZEN_QUEUE_LOCK))

    USHORT Cylinders;
    USHORT Heads;
    USHORT SectorsPerTrack;
    ULONG64 TotalSectors;
    ULONG MaxRequestsBitmap;
    ULONG FreeRequestsBitmap;
    PATA_DEVICE_REQUEST Requests;
    LIST_ENTRY DeviceQueueList;
    KEVENT QueueStoppedEvent;
    PSCSI_REQUEST_BLOCK QuiescenceSrb;
#if DBG
    struct
    {
        ULONG RequestsStarted;
        ULONG RequestsCompleted;
    } Statistics;
#endif
} ATAPORT_IO_CONTEXT, *PATAPORT_IO_CONTEXT;

#include "debug.h"
#include "scsiex.h"

#if defined(_MSC_VER)
#pragma section("PAGECONS", read)
#endif

/** Pageable read-only data */
#define ATAPORT_PAGED_DATA    DATA_SEG("PAGECONS")

#define DECLARE_PAGED_UNICODE_STRING(Variable, Str) \
  static const ATAPORT_PAGED_DATA WCHAR Variable##_buffer[] = Str; \
  UNICODE_STRING Variable = { sizeof(Str) - sizeof(WCHAR), sizeof(Str), (PWCH)Variable##_buffer } \

#define DECLARE_PAGED_STRING(v, n) \
    static const ATAPORT_PAGED_DATA CHAR (v)[] = (n)

#define DECLARE_PAGED_WSTRING(v, n) \
    static const ATAPORT_PAGED_DATA WCHAR (v)[] = (n)

#define ASSUME(cond) \
    do { \
        ASSERT(cond); \
        __assume(cond); \
    } while (0)

#define ATAPORT_TAG             'PedI'

#define IS_FDO(p) \
    ((((PATAPORT_COMMON_EXTENSION)(p))->Flags & DO_IS_FDO) != 0)

#define IS_ATAPI(Device) \
    (((Device)->TransportFlags & DEVICE_IS_ATAPI) != 0)

#define NUM_TO_BITMAP(num)     (0xFFFFFFFF >> (RTL_BITS_OF(ULONG) - (num)))

/**
 * @brief
 * The maximum length of identifier strings for ATA devices excluding the terminating NULL.
 *
 * @sa
 * See MSDN note:
 * https://learn.microsoft.com/en-us/windows-hardware/drivers/install/identifiers-for-ide-devices
 *
 * and
 * IDENTIFY_DEVICE_DATA.SerialNumber
 * IDENTIFY_DEVICE_DATA.ModelNumber
 * IDENTIFY_DEVICE_DATA.FirmwareRevision
 */
/*@{*/
#define ATAPORT_FN_FIELD   40
#define ATAPORT_SN_FIELD   40
#define ATAPORT_RN_FIELD   8
/*@}*/

#define ATA_RESERVED_PAGES       4

#define ATA_MAX_LUN_COUNT        8

/* Maximum size (ATA Information VPD page) */
#define ATA_LOCAL_BUFFER_SIZE    572

#define MAX_SLOTS        32

#define ATA_MAX_LBA_28      0x0FFFFFFFULL
#define ATA_MAX_LBA_48      (1ULL << 48)

#define IDE_DRIVE_SELECT_SLAVE       0x10
#define IDE_HIGH_ORDER_BYTE          0x80
#define IDE_DRIVE_SELECT             0xA0

#define IDE_ERROR_WRITE_PROTECT      0x40

#define IDE_FEATURE_PIO         0x00
#define IDE_FEATURE_DMA         0x01
#define IDE_FEATURE_DMADIR      0x04

#define IDE_DEVICE_FUA_NCQ      0x80

#define IDE_DC_ALWAYS           0x08

#define IDE_COMMAND_REQUEST_SENSE_DATA_EXT      0x0B

#define SRB_FLAG_RETRY_COUNT_MASK  0x000000FF
#define SRB_FLAG_LOW_MEM_RETRY     0x00000100
#define SRB_FLAG_PIO_RETRY         0x00000200

#define SRB_SET_FLAGS(Srb, Flags) \
    ((Srb)->SrbExtension = (PVOID)((ULONG_PTR)(Srb)->SrbExtension | (Flags)))

#define SRB_CLEAR_FLAGS(Srb, Flags) \
    ((Srb)->SrbExtension = (PVOID)((ULONG_PTR)(Srb)->SrbExtension & ~(Flags)))

#define SRB_GET_FLAGS(Srb)   ((ULONG_PTR)(Srb)->SrbExtension)

/* Values are assigned by priority */
typedef enum _ATA_PORT_ACTION
{
    ACTION_PORT_RESET           = (1 << 0), // High priority
    ACTION_ENUM_PORT            = (1 << 1),
    ACTION_ENUM_DEVICE_NEW      = (1 << 2),
    ACTION_ENUM_DEVICE          = (1 << 3),
    ACTION_PORT_TIMING          = (1 << 4), // Set transfer timings after device enumeration only
    ACTION_DEVICE_CONFIG        = (1 << 5), // Also use the timing information for configuration
    ACTION_DEVICE_ERROR         = (1 << 6),
    ACTION_DEVICE_POWER         = (1 << 7),
} ATA_PORT_ACTION;

typedef enum _ATA_DEVICE_STATUS
{
    DEV_STATUS_NO_DEVICE,
    DEV_STATUS_NEW_DEVICE,
    DEV_STATUS_SAME_DEVICE,
    DEV_STATUS_FAILED,
} ATA_DEVICE_STATUS;

typedef enum _ATA_ERROR_LOG_VALUE
{
    EVENT_CODE_CRC_ERROR = 100,
    EVENT_CODE_BAD_STATE,
    EVENT_CODE_TIMEOUT,
    EVENT_CODE_DOWNSHIFT,
    EVENT_CODE_DMA_DISABLE,
    EVENT_CODE_NCQ_DISABLE,
} ATA_ERROR_LOG_VALUE;

typedef struct _ATA_WORKER_CONTEXT
{
    KSPIN_LOCK Lock;
    KEVENT ThreadEvent;
    volatile PATAPORT_DEVICE_EXTENSION EnumDevExt;
    volatile ULONG DeviceCount;
    volatile LONG EventsPending;
    ULONG Flags;
#define WORKER_FLAG_NEED_RESCAN                 0x00000001
#define WORKER_FLAG_COMPLETE_PORT_ENUM_EVENT    0x00000002
    ULONG PausedSlotsBitmap;
    ULONG BadDeviceBitmap;
    ULONG ResetRetryCount;
#if DBG
    ULONG StateLoopCount;
#endif
    KDPC Dpc;
    PATA_DEVICE_REQUEST FailedRequest;
    PATA_DEVICE_REQUEST OldRequest;
    KEVENT CompletionEvent;
    ATA_DEVICE_REQUEST InternalRequest;
    ATAPORT_IO_CONTEXT InternalDevice;
    KEVENT EnumerationEvent;
    KDPC NotificationDpc;
    PKTHREAD Thread;
} ATA_WORKER_CONTEXT, *PATA_WORKER_CONTEXT;

typedef struct _ATA_WORKER_DEVICE_CONTEXT
{
    volatile LONG EventsPending;
    volatile LONG EnumStatus;
    ULONG ResetRetryCount;
    ULONG Flags;
#define DEV_WORKER_FLAG_HOLD_REFERENCE          0x00000001
#define DEV_WORKER_FLAG_REMOVED                 0x00000002
    KEVENT EnumerationEvent;
    KEVENT ConfigureEvent;
} ATA_WORKER_DEVICE_CONTEXT, *PATA_WORKER_DEVICE_CONTEXT;

#define QUEUE_ENTRY_FROM_IRP(Irp) \
    ((PREQUEST_QUEUE_ENTRY)&(((PIRP)(Irp))->Tail.Overlay.DriverContext[0]))

#define IRP_FROM_QUEUE_ENTRY(QueueEntry) \
    (PIRP)CONTAINING_RECORD(QueueEntry, IRP, Tail.Overlay.DriverContext[0])

typedef struct _REQUEST_QUEUE_ENTRY
{
    LIST_ENTRY ListEntry;
    PVOID Context;
    ULONG SortKey;
} REQUEST_QUEUE_ENTRY, *PREQUEST_QUEUE_ENTRY;

/* Check for Irp->Tail.Overlay.DriverContext */
C_ASSERT(sizeof(REQUEST_QUEUE_ENTRY) <= 4 * sizeof(PVOID));

#define ASSERT_REQUEST(Request) \
    ASSERT((Request) && (Request)->Signature == ATA_DEVICE_REQUEST_SIGNATURE)

C_ASSERT(REQUEST_FLAG_DATA_IN == SRB_FLAGS_DATA_IN);
C_ASSERT(REQUEST_FLAG_DATA_OUT == SRB_FLAGS_DATA_OUT);
C_ASSERT(REQUEST_FLAG_NO_KEEP_AWAKE == SRB_FLAGS_NO_KEEP_AWAKE);

typedef struct _ATAPORT_PORT_DATA
{
    ULONG PortFlags;
#define PORT_FLAG_IS_SIMPLEX                      0x00000001
#define PORT_FLAG_PIO_VIA_DMA                     0x00000002
#define PORT_FLAG_IS_EXTERNAL                     0x00000004
#define PORT_FLAG_NCQ                             0x00000008
#define PORT_FLAG_IS_AHCI                         0x00000010
#define PORT_FLAG_SYMLINK_CREATED                 0x00000020
#define PORT_FLAG_IO_TIMER_ACTIVE                 0x00000040
#define PORT_FLAG_CHANNEL_ATTACHED                0x00000080
#define PORT_FLAG_PIO_ONLY                        0x00000100
#define PORT_FLAG_PIO_FOR_LBA48_XFER              0x00000200
#define PORT_FLAG_EXIT_THREAD                     0x80000000

    PVOID ChannelContext;
    PCHANNEL_ALLOCATE_SLOT AllocateSlot;
    PCHANNEL_PREPARE_PRD_TABLE PreparePrdTable;
    PCHANNEL_PREPARE_IO PrepareIo;
    PCHANNEL_START_IO StartIo;
    PDMA_ADAPTER DmaAdapter;
    PDEVICE_OBJECT ChannelObject;
    PATA_DEVICE_REQUEST Slots[MAX_SLOTS];
    LONG TimerCount[MAX_SLOTS];
    PKINTERRUPT InterruptObject;
    ULONG ActiveSlotsBitmap;
    volatile LONG InterruptFlags;
#define PORT_INT_FLAG_IS_IO_ACTIVE                0x00000001
#define PORT_INT_FLAG_IGNORE_LINK_IRQ             0x00000002

    KSPIN_LOCK QueueLock;
    LIST_ENTRY PortQueueList;
    ULONG ActiveTimersBitmap;
    ULONG FreeSlotsBitmap;
    ULONG LastUsedSlot;
    ULONG QueueFlags;
#define PORT_QUEUE_FLAG_EXCLUSIVE_MODE            0x00000001
#define PORT_QUEUE_FLAG_SIGNAL_STOP               0x00000002

    /**
     * <0: we have native queued commands pending.
     * =0: the slot queue is empty.
     * >0: we have non-queued commands pending.
     */
    LONG AllocatedSlots;

    volatile LONG ReservedMappingLock;
    PVOID ReservedVaSpace;
    PCONTROLLER_OBJECT HwSyncObject;
    PCHANNEL_ABORT_CHANNEL AbortChannel;
    PCHANNEL_RESET_CHANNEL ResetChannel;
    PCHANNEL_ENUMERATE_CHANNEL EnumerateChannel;
    PCHANNEL_IDENTIFY_DEVICE IdentifyDevice;
    PCHANNEL_SET_MODE SetTransferMode;
    PCHANNEL_SET_DEVICE_DATA SetDeviceData;
    PCHANNEL_GET_INIT_TASK_FILE GetInitTaskFile;
    PCHANNEL_DOWNGRADE_INTERFACE_SPEED DowngradeInterfaceSpeed;
    PCONTROLLER_ATTACH_CHANNEL AttachChannel;
    KEVENT QueueStoppedEvent;
    ULONG MaximumTransferLength;
    ULONG MaximumPhysicalPages;
    ULONG MaxTargetId;
    ULONG QueueDepth;
    ULONG PortNumber;
    ULONG MaxSlotsBitmap;
    PVOID LocalBuffer;
    SCATTER_GATHER_LIST LocalSgList;
    ATA_WORKER_CONTEXT Worker;
} ATAPORT_PORT_DATA, *PATAPORT_PORT_DATA;

typedef struct _ATAPORT_COMMON_EXTENSION
{
    _Write_guarded_by_(_Global_interlock_)
    volatile LONG PageFiles;

    _Write_guarded_by_(_Global_interlock_)
    volatile LONG HibernateFiles;

    _Write_guarded_by_(_Global_interlock_)
    volatile LONG DumpFiles;

    /** PNP device power state. */
    DEVICE_POWER_STATE DevicePowerState;

    /** PNP system power state. */
    SYSTEM_POWER_STATE SystemPowerState;

    union
    {
        /** Lower device object. This applies to FDO only. */
        PDEVICE_OBJECT LowerDeviceObject;

        /** Parent FDO. This applies to PDO only. */
        PVOID FdoExt;
    };

    ULONG Flags;
#define DO_IS_FDO        0x80000000

    PDEVICE_OBJECT Self;

    IO_REMOVE_LOCK RemoveLock;
} ATAPORT_COMMON_EXTENSION, *PATAPORT_COMMON_EXTENSION;

typedef struct _ATAPORT_CHANNEL_EXTENSION
{
    /** Common data, must be the first member. */
    ATAPORT_COMMON_EXTENSION Common;

    ATAPORT_PORT_DATA PortData;

    PDEVICE_OBJECT Pdo;
    KSPIN_LOCK PdoListLock;
    SINGLE_LIST_ENTRY PdoList;
    ULONG DeviceObjectNumber;
    ULONG ScsiPortNumber;
    UNICODE_STRING StorageInterfaceName;
} ATAPORT_CHANNEL_EXTENSION, *PATAPORT_CHANNEL_EXTENSION;

typedef struct _ATAPORT_DEVICE_EXTENSION
{
    /** Common data, must be the first member. */
    ATAPORT_COMMON_EXTENSION Common;

    ATAPORT_IO_CONTEXT Device;
    ATA_WORKER_DEVICE_CONTEXT Worker;

    BOOLEAN ReportedMissing;
    BOOLEAN RemovalPending;
    BOOLEAN NotPresent;
    BOOLEAN DeviceClaimed;

    /** PDO list entry. */
    SINGLE_LIST_ENTRY ListEntry;

    /** Power IRP queue list. */
    LIST_ENTRY PowerIrpQueueList;

    /** 512-byte block of device identify data. */
    union
    {
        IDENTIFY_DEVICE_DATA IdentifyDeviceData;
        IDENTIFY_PACKET_DATA IdentifyPacketData;
    };

    /** Standard inquiry data. */
    INQUIRYDATA InquiryData;

    /** Device strings for PNP and IOCTL operations. */
    /*@{*/
    _Field_z_ DECLSPEC_ALIGN(2) CHAR FriendlyName[ATAPORT_FN_FIELD + sizeof(ANSI_NULL)];
    _Field_z_ DECLSPEC_ALIGN(2) CHAR RevisionNumber[ATAPORT_RN_FIELD + sizeof(ANSI_NULL)];
    _Field_z_ DECLSPEC_ALIGN(2) CHAR SerialNumber[ATAPORT_SN_FIELD + sizeof(ANSI_NULL)];
    /*@}*/

    /**
     * Current device type.
     * @sa ATA_DEVICE_TYPE
     *
     * Passed to storprop.dll through registry (REG_DWORD).
     * @sa DD_ATA_REG_ATA_DEVICE_TYPE
     */
    ULONG DeviceType;

    /**
     * Bit map for the user-specified transfer modes on the device.
     *
     * Maintained by storprop.dll and always retrieved from the registry by the driver (REG_DWORD).
     * @sa DD_ATA_REG_XFER_MODE_ALLOWED
     */
    ULONG TransferModeUserAllowedMask;

    /**
     * Bit map for enabled/active transfer modes on the device.
     * Holds the value based off of identify data.
     */
    ULONG TransferModeCurrentBitmap;

    /**
     * Bit map for supported transfer modes on the device.
     * Holds the value based off of identify data.
     *
     * Passed to storprop.dll through registry (REG_DWORD).
     * @sa DD_ATA_REG_XFER_MODE_SUPPORTED
     */
    ULONG TransferModeSupportedBitmap;

    /**
     * Selected transfer modes on the device as a bit map
     * with 1 or 2 bits set: +1 for PIO mode (always set) and +1 for DMA mode (may be disabled).
     *
     * Passed to storprop.dll through registry (REG_DWORD).
     * @sa DD_ATA_REG_XFER_MODE_SELECTED
     */
    ULONG TransferModeSelectedBitmap;

    /**
     * Bit map for allowed transfer modes on the device.
     * DMA errors cause the upper bits to be cleared until PIO mode is reached.
     */
    ULONG TransferModeAllowedMask;

    /**
     * The minimum cycle time in nanoseconds that the device can support
     * for PIO and DMA transfer modes. -1 if not supported.
     *
     * @sa IDE_ACPI_TIMING_MODE_NOT_SUPPORTED
     */
    /*@{*/
    ULONG MinimumPioCycleTime;
    ULONG MinimumSingleWordDmaCycleTime;
    ULONG MinimumMultiWordDmaCycleTime;
    ULONG MinimumUltraDmaCycleTime;
    /*@}*/

    /** List of ATA commands to the drive created by the ACPI firmware. */
    PVOID GtfDataBuffer;

    /** Tracks the last time a DMA error was encountered. */
    LARGE_INTEGER LastDmaErrorTime;

    /** WMI data provider interface. */
    WMILIB_CONTEXT WmiLibInfo;
} ATAPORT_DEVICE_EXTENSION, *PATAPORT_DEVICE_EXTENSION;

C_ASSERT(QUEUE_FLAG_FROZEN_QUEUE_FREEZE == SRB_FLAGS_BYPASS_FROZEN_QUEUE);
C_ASSERT(QUEUE_FLAG_FROZEN_QUEUE_LOCK == SRB_FLAGS_BYPASS_LOCKED_QUEUE);

extern UNICODE_STRING AtapDriverRegistryPath;
extern BOOLEAN AtapInPEMode;

typedef enum
{
    GetDeviceType,
    GetGenericType,
    GetPeripheralId
} DEVICE_TYPE_NAME;

FORCEINLINE
BOOLEAN
AtaPortQueueEmpty(
    _In_ PATAPORT_PORT_DATA PortData)
{
    ULONG SlotsBitmap;

    SlotsBitmap = PortData->Worker.PausedSlotsBitmap | PortData->FreeSlotsBitmap;

    return (SlotsBitmap == PortData->MaxSlotsBitmap);
}

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

    /* This is used for address comparison. See AtaFdoFindNextDeviceByPath() */
    AtaScsiAddress.IsValid = 0xAA;

    return AtaScsiAddress;
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
BOOLEAN
AtaPacketCommandUseDma(
    _In_ UCHAR OpCode)
{
    return (OpCode == SCSIOP_READ6 ||
            OpCode == SCSIOP_WRITE6 ||
            OpCode == SCSIOP_READ ||
            OpCode == SCSIOP_WRITE ||
            OpCode == SCSIOP_READ12 ||
            OpCode == SCSIOP_WRITE12 ||
            OpCode == SCSIOP_READ16 ||
            OpCode == SCSIOP_WRITE16 ||
            OpCode == SCSIOP_READ_CD ||
            OpCode == SCSIOP_READ_CD_MSF);
}

FORCEINLINE
BOOLEAN
AtaCommandUseLba48(
    _In_ ULONG64 SectorNumber,
    _In_ ULONG SectorCount)
{
    /* Use the 48-bit command when reasonable */
    return (((SectorNumber + SectorCount) >= ATA_MAX_LBA_28) || (SectorCount > 0x100));
}

/* atapi.c ********************************************************************/

_Dispatch_type_(IRP_MJ_CREATE)
_Dispatch_type_(IRP_MJ_CLOSE)
CODE_SEG("PAGE")
DRIVER_DISPATCH_PAGED AtaDispatchCreateClose;

CODE_SEG("PAGE")
DRIVER_ADD_DEVICE AtaAddChannel;

CODE_SEG("PAGE")
DRIVER_UNLOAD AtaUnload;

CODE_SEG("INIT")
DRIVER_INITIALIZE DriverEntry;

IO_WORKITEM_ROUTINE AtaStorageNotificationWorker;
KDEFERRED_ROUTINE AtaStorageNotificationlDpc;

CODE_SEG("PAGE")
PCSTR
AtaTypeCodeToName(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ DEVICE_TYPE_NAME Type);

CODE_SEG("PAGE")
NTSTATUS
AtaOpenRegistryKey(
    _Out_ PHANDLE KeyHandle,
    _In_ HANDLE RootKey,
    _In_ PUNICODE_STRING KeyName,
    _In_ BOOLEAN Create);

CODE_SEG("PAGE")
VOID
AtaGetRegistryKey(
    _In_ PATAPORT_CHANNEL_EXTENSION ChanExt,
    _In_ UCHAR TargetId,
    _In_ PCWSTR KeyName,
    _Out_ PULONG KeyValue,
    _In_ ULONG DefaultValue);

CODE_SEG("PAGE")
VOID
AtaSetRegistryKey(
    _In_ PATAPORT_CHANNEL_EXTENSION ChanExt,
    _In_ UCHAR TargetId,
    _In_ PCWSTR KeyName,
    _In_ ULONG KeyValue);

VOID
AtaSetPortRegistryKey(
    _In_ PATAPORT_CHANNEL_EXTENSION ChanExt,
    _In_ PCWSTR KeyName,
    _In_ ULONG KeyValue);

CODE_SEG("PAGE")
NTSTATUS
AtaPnpQueryInterface(
    _In_ PATAPORT_COMMON_EXTENSION CommonExt,
    _In_ const GUID* Guid,
    _Out_ PVOID Interface,
    _In_ ULONG Version,
    _In_ ULONG Size);

CODE_SEG("PAGE")
NTSTATUS
AtaPnpRepeatRequest(
    _In_ PATAPORT_COMMON_EXTENSION CommonExt,
    _In_ PIRP Irp,
    _In_opt_ PDEVICE_CAPABILITIES DeviceCapabilities);

CODE_SEG("PAGE")
NTSTATUS
AtaPnpQueryDeviceUsageNotification(
    _In_ PATAPORT_COMMON_EXTENSION CommonExt,
    _In_ PIRP Irp);

CODE_SEG("PAGE")
NTSTATUS
AtaPnpQueryPnpDeviceState(
    _In_ PATAPORT_COMMON_EXTENSION CommonExt,
    _In_ PIRP Irp);

CODE_SEG("PAGE")
VOID
AtaPnpInitializeCommonExtension(
    _In_ PATAPORT_COMMON_EXTENSION CommonExt,
    _In_ PDEVICE_OBJECT SelfDeviceObject,
    _In_ ULONG Flags);

/* dev_config.c ***************************************************************/

NTSTATUS
AtaPortDeviceProcessConfig(
    _In_ PATAPORT_PORT_DATA PortData,
    _In_ PATAPORT_DEVICE_EXTENSION DevExt);

/* dev_error.c ****************************************************************/

NTSTATUS
AtaPortDeviceProcessError(
    _In_ PATAPORT_PORT_DATA PortData,
    _In_ PATAPORT_DEVICE_EXTENSION DevExt);

/* dev_identify.c *************************************************************/

NTSTATUS
AtaDeviceSendIdentify(
    _In_ PATAPORT_PORT_DATA PortData,
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ UCHAR Command);

ATA_DEVICE_STATUS
AtaPortIdentifyDevice(
    _In_ PATAPORT_PORT_DATA PortData,
    _In_ PATAPORT_DEVICE_EXTENSION DevExt);

/* dev_power.c ****************************************************************/

_Dispatch_type_(IRP_MJ_POWER)
DRIVER_DISPATCH_RAISED AtaDispatchPower;

VOID
AtaDeviceFlushPowerIrpQueue(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt);

UCHAR
AtaDeviceGetFlushCacheCommand(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt);

NTSTATUS
AtaPortDeviceProcessPowerChange(
    _In_ PATAPORT_PORT_DATA PortData,
    _In_ PATAPORT_DEVICE_EXTENSION DevExt);

NTSTATUS
AtaPortCheckDevicePowerState(
    _In_ PATAPORT_PORT_DATA PortData,
    _In_ PATAPORT_DEVICE_EXTENSION DevExt);

/* dev_timings.c **************************************************************/

VOID
AtaPortSelectTimings(
    _In_ PATAPORT_PORT_DATA PortData,
    _In_ BOOLEAN ForceCompatibleTimings);

/* enum.c *********************************************************************/

PUCHAR
AtaCopyIdStringUnsafe(
    _Out_writes_bytes_all_(Length) PUCHAR Destination,
    _In_reads_bytes_(Length) PUCHAR Source,
    _In_ ULONG Length);

PCHAR
AtaCopyIdStringSafe(
    _Out_writes_bytes_all_(MaxLength) PCHAR Destination,
    _In_reads_bytes_(MaxLength) PUCHAR Source,
    _In_ ULONG MaxLength,
    _In_ CHAR DefaultCharacter);

VOID
AtaSwapIdString(
    _Inout_updates_bytes_(WordCount * sizeof(USHORT)) PVOID Buffer,
    _In_range_(>, 0) ULONG WordCount);

VOID
AtaDeviceSetAddressingMode(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt);

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
AtaFdoPnp(
    _In_ PATAPORT_CHANNEL_EXTENSION ChanExt,
    _Inout_ PIRP Irp);

DECLSPEC_NOINLINE_FROM_PAGED
PATAPORT_DEVICE_EXTENSION
AtaFdoFindDeviceByPath(
    _In_ PATAPORT_CHANNEL_EXTENSION ChanExt,
    _In_ ATA_SCSI_ADDRESS AtaScsiAddress,
    _In_ PVOID ReferenceTag);

DECLSPEC_NOINLINE_FROM_PAGED
PATAPORT_DEVICE_EXTENSION
AtaFdoFindNextDeviceByPath(
    _In_ PATAPORT_CHANNEL_EXTENSION ChanExt,
    _Inout_ PATA_SCSI_ADDRESS AtaScsiAddress,
    _In_ BOOLEAN SearchRemoveDev,
    _In_ PVOID ReferenceTag);

DECLSPEC_NOINLINE_FROM_PAGED
VOID
AtaFdoDeviceListInsert(
    _In_ PATAPORT_CHANNEL_EXTENSION ChanExt,
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ BOOLEAN DoInsert);

/* ioctl.c ********************************************************************/

_Dispatch_type_(IRP_MJ_DEVICE_CONTROL)
DRIVER_DISPATCH_RAISED AtaDispatchDeviceControl;

/* pdo.c **********************************************************************/

IO_COMPLETION_ROUTINE AtaPdoCompletionRoutine;

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

/* portstate.c ****************************************************************/

KSTART_ROUTINE AtaPortWorkerThread;
KDEFERRED_ROUTINE AtaPortWorkerSignalDpc;
REQUEST_COMPLETION_ROUTINE AtaPortCompleteInternalRequest;
PORT_NOTIFICATION AtaPortNotification;

NTSTATUS
AtaPortSendRequest(
    _In_ PATAPORT_PORT_DATA PortData,
    _In_ PATAPORT_DEVICE_EXTENSION DevExt);

VOID
AtaPortTimeout(
    _In_ PATAPORT_PORT_DATA PortData,
    _In_ ULONG Slot);

DECLSPEC_NOINLINE_FROM_PAGED
VOID
AtaPortSignalWorkerThread(
    _In_ PATAPORT_PORT_DATA PortData);

DECLSPEC_NOINLINE_FROM_PAGED
VOID
AtaDeviceQueueEvent(
    _In_ PATAPORT_PORT_DATA PortData,
    _In_opt_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ ATA_PORT_ACTION Action);

/* satl.c *********************************************************************/

BOOLEAN
AtaReqDmaTransferToPioTransfer(
    _In_ PATA_DEVICE_REQUEST Request);

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
AtaReqSetFixedAtaSenseData(
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

IO_TIMER_ROUTINE AtaPortIoTimer;
KDEFERRED_ROUTINE AtaReqCompletionDpc;
extern SLIST_HEADER AtapCompletionQueueList;
extern KDPC AtapCompletionDpc;

VOID
AtaReqCompleteFailedRequest(
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
AtaReqSendRequest(
    _In_ PATA_DEVICE_REQUEST Request);

VOID
AtaReqStartCompletionDpc(
    _In_ PATA_DEVICE_REQUEST Request);

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
VOID
AtaReqWaitForOutstandingIoToComplete(
    _In_ PATAPORT_IO_CONTEXT Device,
    _In_ PSCSI_REQUEST_BLOCK Srb);

DECLSPEC_NOINLINE_FROM_PAGED
VOID
AtaReqFlushDeviceQueue(
    _In_ PATAPORT_IO_CONTEXT Device);

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

/* wmi.c **********************************************************************/

_Dispatch_type_(IRP_MJ_SYSTEM_CONTROL)
CODE_SEG("PAGE")
DRIVER_DISPATCH_PAGED AtaDispatchWmi;

CODE_SEG("PAGE")
NTSTATUS
AtaPdoWmiRegistration(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ BOOLEAN Register);
