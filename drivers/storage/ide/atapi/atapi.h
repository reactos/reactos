/*
 * PROJECT:     ReactOS ATA Port Driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Common header file
 * COPYRIGHT:   Copyright 2024 Dmitry Borisov (di.sean@protonmail.com)
 */

#pragma once

#include <ntddk.h>
#include <ntstrsafe.h>
#include <ntintsafe.h>

#include <wmilib.h>
#include <initguid.h>
#include <wmistr.h>
#include <wmidata.h>

#include <ata.h>
#include <ide.h>
#include <scsi.h>
#include <ntddscsi.h>
#include <ntdddisk.h>

#include <section_attribs.h>

#include <reactos/drivers/ntddata.h>


#define IDEPORT_TAG             'PedI'

#define ATAPORT_FDO_SIGNATURE   'odFI'
#define ATAPORT_PDO_SIGNATURE   'odPI'

#define IS_FDO(p)    (((PATAPORT_COMMON_EXTENSION)(p))->Signature == ATAPORT_FDO_SIGNATURE)
#define IS_PDO(p)    (((PATAPORT_COMMON_EXTENSION)(p))->Signature == ATAPORT_PDO_SIGNATURE)

#define IS_ATAPI(DeviceExtension)    (((DeviceExtension)->Flags & DEVICE_IS_ATAPI) != 0)
#define IS_AHCI(ChannelExtension)    (((ChannelExtension)->Flags & CHANNEL_AHCI) != 0)

#define ATAPORT_FN_FIELD   40
#define ATAPORT_SN_FIELD   40
#define ATAPORT_RN_FIELD   8

#define ATA_RESERVED_PAGES       4

#define ATA_MAX_LUN_COUNT        8

/* Maxumum size (ATA Information VPD page) */
#define ATA_SATL_BUFFER_SIZE     572

typedef struct _ATAPORT_CHANNEL_EXTENSION ATAPORT_CHANNEL_EXTENSION, *PATAPORT_CHANNEL_EXTENSION;
typedef struct _ATAPORT_DEVICE_EXTENSION ATAPORT_DEVICE_EXTENSION, *PATAPORT_DEVICE_EXTENSION;
#include "debug.h"
typedef enum
{
    DEV_ATA = 1,
    DEV_ATAPI,
    DEV_UNKNOWN
} ATA_DEVICE_CLASS;

typedef union _ATA_SCSI_ADDRESS
{
    /* The ordering between Lun, TargetId, and PathId is important */
    struct
    {
        /* 0-8 */
        UCHAR Lun;

        /* 0 - Master, 1 - Slave, 2 - Master (PC-98), 3 - Slave (PC-98) */
        UCHAR TargetId;

        /* 0 - Primary, 1 - Secondary */
        UCHAR PathId;

        UCHAR Reserved;
    };
    ULONG AsULONG;
} ATA_SCSI_ADDRESS, *PATA_SCSI_ADDRESS;

/* 4 targets * 8 LUNs = 32 entries (bits of ULONG) */
#define ATA_REQUEST_MASK_FROM_DEVICE(DeviceExtension) \
    (1 << ((DeviceExtension)->AtaScsiAddress.TargetId * 8 + \
           (DeviceExtension)->AtaScsiAddress.Lun))

typedef enum
{
    QueueItemQueued,
    QueueItemCancelled,
    QueueItemStarted,
} ATA_QUEUE_RESULT;

typedef enum
{
    SRB_TYPE_ENUM = 0,
    SRB_TYPE_CONFIG,
    SRB_TYPE_MAX,
} ATA_SRB_TYPE;

typedef enum
{
    POLL_REQUEST,
    POLL_RETRY,
} ATA_POLL_STATE;

typedef enum
{
    COMPLETE_IRP = 0,
    COMPLETE_NO_IRP,
    COMPLETE_START_AGAIN,
} ATA_COMPLETION_STATUS;

typedef ATA_COMPLETION_STATUS
(REQUEST_COMPLETION_ROUTINE)(
    _In_ PATAPORT_CHANNEL_EXTENSION Request);
typedef REQUEST_COMPLETION_ROUTINE *PREQUEST_COMPLETION_ROUTINE;

typedef struct _IDE_REGISTERS
{
    union
    {
        PUCHAR Data;
        PULONG DataDoubleWord;
    };
    union
    {
        PUCHAR Error;
        PUCHAR Feature;
    };
    union
    {
        PUCHAR SectorCount;
        PUCHAR InterruptReason;
    };
    PUCHAR LowLba;
    union
    {
        PUCHAR MidLba;
        PUCHAR ByteCountLow;
        PUCHAR SignatureLow;
    };
    union
    {
        PUCHAR HighLba;
        PUCHAR ByteCountHigh;
        PUCHAR SignatureHigh;
    };
    PUCHAR DriveSelect;
    union
    {
        PUCHAR Status;
        PUCHAR Command;
    };
    union
    {
        PUCHAR AlternateStatus;
        PUCHAR Control;
    };
} IDE_REGISTERS, *PIDE_REGISTERS;

typedef struct _ATA_TASKFILE
{
    union
    {
        UCHAR Feature;
        UCHAR Error;
    };
    UCHAR SectorCount;
    UCHAR LowLba;
    UCHAR MidLba;
    UCHAR HighLba;
    UCHAR Command;
    UCHAR DriveSelect;

    UCHAR FeatureEx;
    UCHAR SectorCountEx;
    UCHAR LowLbaEx;
    UCHAR MidLbaEx;
    UCHAR HighLbaEx;
} ATA_TASKFILE, *PATA_TASKFILE;

typedef struct _IDE_ENABLE_INTERRUPTS_CONTEXT
{
    PATAPORT_CHANNEL_EXTENSION ChannelExtension;
    BOOLEAN Enable;
} IDE_ENABLE_INTERRUPTS_CONTEXT, *PIDE_ENABLE_INTERRUPTS_CONTEXT;

typedef struct _REQUEST_QUEUE_ENTRY
{
    LIST_ENTRY ListEntry;
    PATAPORT_DEVICE_EXTENSION Context;
    ULONG SortKey;
} REQUEST_QUEUE_ENTRY, *PREQUEST_QUEUE_ENTRY;

C_ASSERT(sizeof(REQUEST_QUEUE_ENTRY) <= 4 * sizeof(PVOID));

typedef struct _ATA_DEVICE_REQUEST
{
    union
    {
        UCHAR Cdb[16];
        ATA_TASKFILE TaskFile;
    };

    PIRP Irp;
    PSCSI_REQUEST_BLOCK Srb;
    PVOID DataBuffer;
    PVOID MappedBuffer;
    PMDL Mdl;
    ULONG DataTransferLength;
    ULONG Flags;
    PREQUEST_COMPLETION_ROUTINE Complete;
    PATAPORT_DEVICE_EXTENSION DeviceExtension;
    PSCATTER_GATHER_LIST SgList;
    UCHAR Error;
    UCHAR Status;
    ULONG SrbStatus;
    ULONG OldDataTransferLength;
    PVOID OldDataBuffer;
} ATA_DEVICE_REQUEST, *PATA_DEVICE_REQUEST;

#define REQUEST_FLAG_DMA_ENABLED              0x00000001
#define REQUEST_FLAG_LBA48                    0x00000002
#define REQUEST_FLAG_SET_DEVICE_REGISTER      0x00000004
#define REQUEST_FLAG_PACKET_COMMAND           0x00000008
#define REQUEST_FLAG_SAVE_TASK_FILE           0x00000010
#define REQUEST_FLAG_REQUEST_SENSE            0x00000020
#define REQUEST_FLAG_DATA_IN                  0x00000040
#define REQUEST_FLAG_DATA_OUT                 0x00000080
#define REQUEST_FLAG_READ_WRITE_MULTIPLE      0x00000100
#define REQUEST_FLAG_COMPLETED                0x00000200
#define REQUEST_FLAG_POLLING_ACTIVE           0x00000400
#define REQUEST_FLAG_RESERVED_MAPPING         0x00000800
#define REQUEST_FLAG_HAS_TASK_FILE            0x00001000
#define REQUEST_FLAG_FUA                      0x00002000
#define REQUEST_FLAG_READ_WRITE               0x00004000
#define REQUEST_FLAG_NO_KEEP_AWAKE            0x00100000
#define REQUEST_FLAG_ASYNC_MODE               0x80000000

C_ASSERT(REQUEST_FLAG_DATA_IN == SRB_FLAGS_DATA_IN);
C_ASSERT(REQUEST_FLAG_DATA_OUT == SRB_FLAGS_DATA_OUT);
C_ASSERT(REQUEST_FLAG_NO_KEEP_AWAKE == SRB_FLAGS_NO_KEEP_AWAKE);

#include <pshpack1.h>
typedef struct _IDE_ACPI_TIMING_MODE_BLOCK
{
    struct
    {
        ULONG PioSpeed;
        ULONG DmaSpeed;
    } Drive[MAX_IDE_DEVICE];

    ULONG ModeFlags;
#define IDE_ACPI_TIMING_MODE_FLAG_UDMA                  0x01
#define IDE_ACPI_TIMING_MODE_FLAG_IORDY                 0x02
#define IDE_ACPI_TIMING_MODE_FLAG_UDMA2                 0x04
#define IDE_ACPI_TIMING_MODE_FLAG_IORDY2                0x08
#define IDE_ACPI_TIMING_MODE_FLAG_INDEPENDENT_TIMINGS   0x10
} IDE_ACPI_TIMING_MODE_BLOCK, *PIDE_ACPI_TIMING_MODE_BLOCK;

#define IDE_ACPI_TIMING_MODE_NOT_SUPPORTED              0xFFFFFFFF

typedef struct _IDE_ACPI_TASK_FILE
{
    UCHAR Feature;
    UCHAR SectorCount;
    UCHAR LowLba;
    UCHAR MidLba;
    UCHAR HighLba;
    UCHAR DriveSelect;
    UCHAR Command;
} IDE_ACPI_TASK_FILE, *PIDE_ACPI_TASK_FILE;
#include <poppack.h>

typedef struct _ATA_LEGACY_CHANNEL
{
    ULONG IoBase;
    ULONG Irq;
} ATA_LEGACY_CHANNEL, *PATA_LEGACY_CHANNEL;

typedef struct _ATAPORT_COMMON_EXTENSION
{
    ULONG Signature;
    PDEVICE_OBJECT Self;

    _Write_guarded_by_(_Global_interlock_)
    volatile LONG PageFiles;

    _Write_guarded_by_(_Global_interlock_)
    volatile LONG HibernateFiles;

    _Write_guarded_by_(_Global_interlock_)
    volatile LONG DumpFiles;
} ATAPORT_COMMON_EXTENSION, *PATAPORT_COMMON_EXTENSION;

typedef struct _ATAPORT_CHANNEL_EXTENSION
{
    ATAPORT_COMMON_EXTENSION Common;
    PDEVICE_OBJECT Ldo;
    IO_REMOVE_LOCK RemoveLock;

    IDE_REGISTERS Registers;

    ULONG Flags;
#define CHANNEL_CBUS_IDE                       0x80000000
#define CHANNEL_PCI_IDE                        0x00000001
#define CHANNEL_CONTROL_PORT_BASE_MAPPED       0x00000002
#define CHANNEL_COMMAND_PORT_BASE_MAPPED       0x00000004
#define CHANNEL_INTERRUPT_SHARED               0x00000008
#define CHANNEL_PRIMARY_ADDRESS_CLAIMED        0x00000010
#define CHANNEL_SECONDARY_ADDRESS_CLAIMED      0x00000020
#define CHANNEL_SYMLINK_CREATED                0x00000040
#define CHANNEL_HAS_CHIPSET_INTERFACE          0x00000080
#define CHANNEL_HAS_GTM                        0x00000100
#define CHANNEL_SIMPLEX_DMA                    0x00000200
#define CHANNEL_IGNORE_ACTIVE_DMA              0x00000400
#define CHANNEL_IO_TIMER_ACTIVE                0x00000800
#define CHANNEL_AHCI                           0x00001000
#define CHANNEL_IO32                           0x00002000
#define CHANNEL_IO_MODE_DETECTED               0x00004000

    ULONG ChannelNumber;
    UCHAR PortNumber;
    UCHAR PathId;

    PKINTERRUPT InterruptObject;
    ULONG InterruptVector;
    KIRQL InterruptLevel;
    KINTERRUPT_MODE InterruptMode;
    KAFFINITY InterruptAffinity;
    PKSERVICE_ROUTINE ServiceRoutine;

    KSPIN_LOCK QueueLock;
    LIST_ENTRY RequestList;
    ATA_DEVICE_REQUEST Request;
    PUCHAR DataBuffer;
    ULONG RequestBitmap;
    ULONG BytesToTransfer;
    ULONG CommandFlags;
    LONG TimerCount;
#define TIMER_STATE_STOPPED                  -1
#define TIMER_STATE_TIMEDOUT                 -2

    ULONG PollCount;
    ULONG PollInterval;
    ATA_POLL_STATE PollState;

    KDPC Dpc;
    KDPC DpcTest;
    KDPC PollingTimerDpc;
    KDPC RetryTimerDpc;

    ULONG MaximumTransferLength;

    union
    {
        AHCI_PORT_INTERFACE AhciPortInterface;
        PCIIDE_INTERFACE PciIdeInterface;
    };

    KSPIN_LOCK ChannelLock;
    FAST_MUTEX DeviceSyncMutex;
    SINGLE_LIST_ENTRY DeviceList;
    ULONG DeviceCount;
    PATAPORT_DEVICE_EXTENSION Device[MAX_IDE_DEVICE];

    IDE_ACPI_TIMING_MODE_BLOCK TimingMode;

    PVOID ReservedVaSpace;

    /* This data must be in non-paged pool */
    SCSI_REQUEST_BLOCK InternalSrb[SRB_TYPE_MAX];
    IRP InternalIrp[SRB_TYPE_MAX];
    ATA_TASKFILE TaskFile[SRB_TYPE_MAX];
    SENSE_DATA SenseData;

    PUCHAR CommandPortBase;
    PUCHAR ControlPortBase;

    KTIMER PollingTimer;
    KTIMER RetryTimer;

    ULONG TranslationState;
    UCHAR StandbyTimerPeriod;
    UCHAR SatlScratchBuffer[ATA_SATL_BUFFER_SIZE];
} ATAPORT_CHANNEL_EXTENSION, *PATAPORT_CHANNEL_EXTENSION;

typedef struct _ATAPORT_DEVICE_EXTENSION
{
    ATAPORT_COMMON_EXTENSION Common;
    IO_REMOVE_LOCK RemoveLock;

    PATAPORT_CHANNEL_EXTENSION ChannelExtension;

    ATA_SCSI_ADDRESS AtaScsiAddress;

    PULONG PowerIdleCounter;

    ULONG Flags;
#define DEVICE_PIO_ONLY                          0x00000001
#define DEVICE_IS_ATAPI                          0x00000002
#define DEVICE_IS_SUPER_FLOPPY                   0x00000004
#define DEVICE_HAS_CDB_INTERRUPT                 0x00000008
#define DEVICE_LBA_MODE                          0x00000010
#define DEVICE_LBA48                             0x00000020
#define DEVICE_HARDWARE_ERROR                    0x00000040
#define DEVICE_ENUM                              0x00000080
#define DEVICE_IS_NEC_CDR260                     0x00000100
#define DEVICE_HAS_MEDIA_STATUS                  0x00000200
#define DEVICE_HAS_FUA                           0x00000400
#define DEVICE_IO32                              0x00000800

    BOOLEAN DeviceClaimed;
    BOOLEAN ReportedMissing;

    UCHAR DeviceHead;
    UCHAR CdbSize;
    UCHAR MultiSectorTransfer;
    USHORT Cylinders;
    USHORT Heads;
    USHORT SectorsPerTrack;
    ULONG SectorSize;
    ULONG64 TotalSectors;

    KSPIN_LOCK QueueLock;
    LIST_ENTRY RequestList;
    ULONG QueueFlags;
#define QUEUE_FLAG_FROZEN                        0x00000001

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
    ULONG Signature;
    SINGLE_LIST_ENTRY ListEntry;

    PVOID GtfDataBuffer;
} ATAPORT_DEVICE_EXTENSION, *PATAPORT_DEVICE_EXTENSION;

extern UNICODE_STRING AtapDriverRegistryPath;
extern BOOLEAN AtapInPEMode;
extern const UCHAR AtapAtaCommand[12][2];

_Dispatch_type_(IRP_MJ_CREATE)
_Dispatch_type_(IRP_MJ_CLOSE)
CODE_SEG("PAGE")
DRIVER_DISPATCH_PAGED AtaDispatchCreateClose;

_Dispatch_type_(IRP_MJ_DEVICE_CONTROL)
DRIVER_DISPATCH_RAISED AtaDispatchDeviceControl;

_Dispatch_type_(IRP_MJ_POWER)
DRIVER_DISPATCH_RAISED AtaDispatchPower;

_Dispatch_type_(IRP_MJ_SCSI)
DRIVER_DISPATCH_RAISED AtaDispatchScsi;

_Dispatch_type_(IRP_MJ_SYSTEM_CONTROL)
CODE_SEG("PAGE")
DRIVER_DISPATCH_PAGED AtaDispatchWmi;

_Dispatch_type_(IRP_MJ_PNP)
CODE_SEG("PAGE")
DRIVER_DISPATCH_PAGED AtaDispatchPnp;

CODE_SEG("PAGE")
DRIVER_ADD_DEVICE AtaAddDevice;

CODE_SEG("PAGE")
DRIVER_UNLOAD AtaUnload;

CODE_SEG("INIT")
DRIVER_INITIALIZE DriverEntry;

KSERVICE_ROUTINE AtaIdeChannelIsr;
KSERVICE_ROUTINE AtaPciIdeChannelIsr;
IO_TIMER_ROUTINE AtaIoTimer;

KDEFERRED_ROUTINE AtaDpc;
KDEFERRED_ROUTINE AtaPollingTimerDpc;

DRIVER_LIST_CONTROL AtaPciIdeDmaPreparePrdTable;

CODE_SEG("PAGE")
NTSTATUS
NTAPI
AtaAddChannel(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PDEVICE_OBJECT PhysicalDeviceObject,
    _Out_ PATAPORT_CHANNEL_EXTENSION *FdoExtension);

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
    _In_ PATAPORT_CHANNEL_EXTENSION ChannelExtension,
    _In_ PATAPORT_DEVICE_EXTENSION DeviceExtension,
    _In_ PCWSTR KeyName,
    _Out_ PULONG KeyValue,
    _In_ ULONG DefaultValue);

CODE_SEG("PAGE")
VOID
AtaSetRegistryKey(
    _In_ PATAPORT_CHANNEL_EXTENSION ChannelExtension,
    _In_ PATAPORT_DEVICE_EXTENSION DeviceExtension,
    _In_ PCWSTR KeyName,
    _In_ ULONG KeyValue);

VOID
AtaPdoSetAddressingMode(
    _In_ PATAPORT_DEVICE_EXTENSION DeviceExtension);

CODE_SEG("PAGE")
NTSTATUS
AtaSendTaskFile(
    _In_ PATAPORT_DEVICE_EXTENSION DeviceExtension,
    _In_ PATA_TASKFILE TaskFile,
    _In_ ATA_SRB_TYPE SrbType,
    _In_ BOOLEAN DoPoll,
    _In_ ULONG TimeoutInSeconds,
    _In_opt_ PVOID Buffer,
    _In_ ULONG Length);

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
PATAPORT_DEVICE_EXTENSION
AtaFdoFindDeviceByPath(
    _In_ PATAPORT_CHANNEL_EXTENSION ChannelExtension,
    _In_ ATA_SCSI_ADDRESS AtaScsiAddress);

CODE_SEG("PAGE")
NTSTATUS
AtaFdoQueryBusRelations(
    _In_ PATAPORT_CHANNEL_EXTENSION ChannelExtension,
    _In_ PIRP Irp);

#if defined(ATA_DETECT_LEGACY_DEVICES)
CODE_SEG("INIT")
VOID
AtaDetectLegacyDevices(
    _In_ PDRIVER_OBJECT DriverObject);
#endif

CODE_SEG("PAGE")
NTSTATUS
AtaFdoStartDevice(
    _In_ PATAPORT_CHANNEL_EXTENSION ChannelExtension,
    _In_ PCM_RESOURCE_LIST ResourcesTranslated);

CODE_SEG("PAGE")
NTSTATUS
AtaFdoRemoveDevice(
    _In_ PATAPORT_CHANNEL_EXTENSION ChannelExtension,
    _In_ PIRP Irp);

CODE_SEG("PAGE")
NTSTATUS
AtaFdoPnp(
    _In_ PATAPORT_CHANNEL_EXTENSION ChannelExtension,
    _Inout_ PIRP Irp);

DECLSPEC_NOINLINE_FROM_PAGED
NTSTATUS
AtaPdoStartSrb(
    _In_ PATAPORT_DEVICE_EXTENSION DeviceExtension,
    _In_ PSCSI_REQUEST_BLOCK Srb);

BOOLEAN
AtaDevicePresent(
    _In_ PATAPORT_CHANNEL_EXTENSION ChannelExtension);

ULONG
AtaExecuteCommand(
    _In_ PATAPORT_CHANNEL_EXTENSION ChannelExtension);

CODE_SEG("PAGE")
NTSTATUS
AtaPdoHandleMiniportSmartVersion(
    _In_ PATAPORT_CHANNEL_EXTENSION ChannelExtension,
    _In_ PSCSI_REQUEST_BLOCK Srb);

CODE_SEG("PAGE")
NTSTATUS
AtaPdoHandleMiniportIdentify(
    _In_ PATAPORT_DEVICE_EXTENSION DeviceExtension,
    _In_ PSCSI_REQUEST_BLOCK Srb);

ULONG
AtaReqIoControl(
    _In_ PATAPORT_CHANNEL_EXTENSION ChannelExtension,
    _In_ PATAPORT_DEVICE_EXTENSION DeviceExtension,
    _In_ PSCSI_REQUEST_BLOCK Srb);

CODE_SEG("PAGE")
NTSTATUS
AtaPdoWmiRegistration(
    _In_ PATAPORT_DEVICE_EXTENSION DeviceExtension,
    _In_ BOOLEAN Register);

BOOLEAN
AtaProcessRequest(
    _In_ PATAPORT_CHANNEL_EXTENSION ChannelExtension,
    _In_ UCHAR IdeStatus,
    _In_ UCHAR DmaStatus);

ULONG
AtaReqExecuteScsi(
    _In_ PATAPORT_CHANNEL_EXTENSION ChannelExtension,
    _In_ PATAPORT_DEVICE_EXTENSION DeviceExtension,
    _In_ PSCSI_REQUEST_BLOCK Srb);

CODE_SEG("PAGE")
PVOID
AtaAcpiGetTaskFile(
    _In_ PATAPORT_DEVICE_EXTENSION DeviceExtension);

CODE_SEG("PAGE")
BOOLEAN
AtaAcpiGetTimingMode(
    _In_ PATAPORT_CHANNEL_EXTENSION ChannelExtension,
    _Out_ PIDE_ACPI_TIMING_MODE_BLOCK TimingMode);

CODE_SEG("PAGE")
VOID
AtaAcpiSetTimingMode(
    _In_ PATAPORT_CHANNEL_EXTENSION ChannelExtension,
    _In_ PIDE_ACPI_TIMING_MODE_BLOCK TimingMode,
    _In_ PIDENTIFY_DEVICE_DATA IdBlock1,
    _In_ PIDENTIFY_DEVICE_DATA IdBlock2);

CODE_SEG("PAGE")
VOID
AtaAcpiExecuteTaskFile(
    _In_ PATAPORT_DEVICE_EXTENSION DeviceExtension,
    _In_ PVOID GtfDataBuffer,
    _In_ PATA_TASKFILE TaskFile);

ULONG
AtaReqPrepareTaskFile(
    _In_ PATAPORT_CHANNEL_EXTENSION ChannelExtension,
    _In_ PATAPORT_DEVICE_EXTENSION DeviceExtension,
    _In_ PSCSI_REQUEST_BLOCK Srb);

VOID
AtaReqDataTransferReady(
    _In_ PATAPORT_CHANNEL_EXTENSION ChannelExtension);

typedef enum
{
    GetDeviceType,
    GetGenericType,
    GetPeripheralId
} DEVICE_TYPE_NAME;

typedef struct _ATAPORT_DEVICE_NAME
{
    PCSTR DeviceType;
    PCSTR GenericType;
    PCSTR PeripheralId;
} ATAPORT_DEVICE_NAME, *PATAPORT_DEVICE_NAME;

CODE_SEG("PAGE")
PCSTR
AtaTypeCodeToName(
    _In_ PATAPORT_DEVICE_EXTENSION DeviceExtension,
    _In_ DEVICE_TYPE_NAME Type);

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
UCHAR
CdbGetAllocationLength6(
    _In_ PCDB Cdb)
{
    return Cdb->CDB6GENERIC.CommandUniqueBytes[3];
}

FORCEINLINE
USHORT
CdbGetAllocationLength10(
    _In_ PCDB Cdb)
{
    return (Cdb->CDB10.TransferBlocksMsb << 8) |
           (Cdb->CDB10.TransferBlocksLsb << 0);
}

FORCEINLINE
ULONG
CdbGetAllocationLength16(
    _In_ PCDB Cdb)
{
    return (Cdb->CDB16.TransferLength[0] << 24) |
           (Cdb->CDB16.TransferLength[1] << 16) |
           (Cdb->CDB16.TransferLength[2] << 8) |
           (Cdb->CDB16.TransferLength[3] << 0);
}

FORCEINLINE
USHORT
CdbGetTransferLength10(
    _In_ PCDB Cdb)
{
    /* Bytes 7:8 */
    return (Cdb->CDB10.TransferBlocksMsb << 8) |
           (Cdb->CDB10.TransferBlocksLsb << 0);
}

FORCEINLINE
ULONG
CdbGetTransferLength12(
    _In_ PCDB Cdb)
{
    /* Bytes 6:9 */
    return (Cdb->CDB12.TransferLength[0] << 24) |
           (Cdb->CDB12.TransferLength[1] << 16) |
           (Cdb->CDB12.TransferLength[2] << 8) |
           (Cdb->CDB12.TransferLength[3] << 0);
}

FORCEINLINE
ULONG
CdbGetTransferLength16(
    _In_ PCDB Cdb)
{
    /* Bytes 10:13 */
    return (Cdb->CDB16.TransferLength[0] << 24) |
           (Cdb->CDB16.TransferLength[1] << 16) |
           (Cdb->CDB16.TransferLength[2] << 8) |
           (Cdb->CDB16.TransferLength[3] << 0);
}

FORCEINLINE
ULONG
CdbGetLogicalBlockAddress6(
    _In_ PCDB Cdb)
{
    /* Bytes 2:3 */
    return (Cdb->CDB6READWRITE.LogicalBlockMsb0 << 8) |
           (Cdb->CDB6READWRITE.LogicalBlockLsb << 0);
}

FORCEINLINE
ULONG
CdbGetLogicalBlockAddress10(
    _In_ PCDB Cdb)
{
    /* Bytes 2:5 */
    return (Cdb->CDB10.LogicalBlockByte0 << 24) |
           (Cdb->CDB10.LogicalBlockByte1 << 16) |
           (Cdb->CDB10.LogicalBlockByte2 << 8) |
           (Cdb->CDB10.LogicalBlockByte3 << 0);
}

FORCEINLINE
ULONG
CdbGetLogicalBlockAddress12(
    _In_ PCDB Cdb)
{
    /* Bytes 2:5 */
    return (Cdb->CDB12.LogicalBlock[0] << 24) |
           (Cdb->CDB12.LogicalBlock[1] << 16) |
           (Cdb->CDB12.LogicalBlock[2] << 8) |
           (Cdb->CDB12.LogicalBlock[3] << 0);
}

FORCEINLINE
ULONG64
CdbGetLogicalBlockAddress16(
    _In_ PCDB Cdb)
{
    /* Bytes 2:9 */
    return ((ULONG64)Cdb->CDB16.LogicalBlock[0] << 56) |
           ((ULONG64)Cdb->CDB16.LogicalBlock[1] << 48) |
           ((ULONG64)Cdb->CDB16.LogicalBlock[2] << 40) |
           ((ULONG64)Cdb->CDB16.LogicalBlock[3] << 32) |
           ((ULONG64)Cdb->CDB16.LogicalBlock[4] << 24) |
           ((ULONG64)Cdb->CDB16.LogicalBlock[5] << 16) |
           ((ULONG64)Cdb->CDB16.LogicalBlock[6] << 8) |
           ((ULONG64)Cdb->CDB16.LogicalBlock[7] << 0);
}

#include <pshpack1.h>

typedef struct _MODE_CACHING_PAGE_SPC5
{
    UCHAR PageCode:6;
    UCHAR Reserved:1;
    UCHAR PageSavable:1;
    UCHAR PageLength;
    UCHAR ReadDisableCache:1;
    UCHAR MultiplicationFactor:1;
    UCHAR WriteCacheEnable:1;
    UCHAR Reserved2:5;
    UCHAR WriteRetensionPriority:4;
    UCHAR ReadRetensionPriority:4;
    UCHAR DisablePrefetchTransfer[2];
    UCHAR MinimumPrefetch[2];
    UCHAR MaximumPrefetch[2];
    UCHAR MaximumPrefetchCeiling[2];
    UCHAR NV_DIS:1;
    UCHAR SYNC_PROG:2;
    UCHAR Reserved1:2;
    UCHAR DisableReadAHead:1;
    UCHAR LBCSS:1;
    UCHAR FSW:1;
    UCHAR NumberOfCacheSegments;
    UCHAR CacheSegmentSize[2];
    UCHAR Reserved3;
    UCHAR Obsolete[3];
} MODE_CACHING_PAGE_SPC5, *PMODE_CACHING_PAGE_SPC5;

C_ASSERT(sizeof(MODE_CACHING_PAGE_SPC5) == 20);

typedef struct _MODE_CONTROL_EXTENSION_PAGE
{
    UCHAR PageCode:6;
    UCHAR SubPageFormat:1;
    UCHAR PageSavable:1;
    UCHAR SubPageCode;
    UCHAR PageLength[2];
    UCHAR IALUAE:1;
    UCHAR SCSIP:1;
    UCHAR TCMOS:1;
    UCHAR Reserved:5;
    UCHAR InitialCommandPriority:4;
    UCHAR Reserved1:4;
    UCHAR MaximumSenseDataLength;
    UCHAR Reserved2[25];
} MODE_CONTROL_EXTENSION_PAGE, *PMODE_CONTROL_EXTENSION_PAGE;

C_ASSERT(sizeof(MODE_CONTROL_EXTENSION_PAGE) == 32);

#include <poppack.h>

C_ASSERT(sizeof(MODE_INFO_EXCEPTIONS) == 12);
C_ASSERT(sizeof(MODE_CONTROL_PAGE) == 12);
C_ASSERT(sizeof(MODE_READ_WRITE_RECOVERY_PAGE) == 12);
C_ASSERT(sizeof(POWER_CONDITION_PAGE) == 12);
C_ASSERT(sizeof(VPD_ATA_INFORMATION_PAGE) == 572);
C_ASSERT(sizeof(VPD_BLOCK_LIMITS_PAGE) == 0x3c+4);
C_ASSERT(sizeof(VPD_BLOCK_DEVICE_CHARACTERISTICS_PAGE) == 0x3c+4);

#include "atahw.h"
