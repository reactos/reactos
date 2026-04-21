/*
 * PROJECT:     ReactOS Storage Stack
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 *              or MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Internal shared ATA driver definitions
 * COPYRIGHT:   Copyright 2026 Dmitry Borisov (di.sean@protonmail.com)
 */

#pragma once

/** @sa PCIIDE_INTERFACE */
#define PCIIDEX_INTERFACE_VERSION    1

/** Maximum number of devices (target ID) per channel */
#define ATA_MAX_DEVICE    15

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

/** Minimum DMA buffer alignment because of the reserved PRDT byte 0 */
#define ATA_MIN_BUFFER_ALIGNMENT      FILE_WORD_ALIGNMENT

#include <pshpack1.h>

/** IDE channel timing information block */
typedef struct _IDE_ACPI_TIMING_MODE_BLOCK
{
    struct
    {
        ULONG PioSpeed; ///< PIO cycle timing in ns
        ULONG DmaSpeed; ///< DMA cycle timing in ns
/** The mode is not supported */
#define IDE_ACPI_TIMING_MODE_NOT_SUPPORTED              0xFFFFFFFF // aka -1 ns
    } Drive[MAX_IDE_DEVICE];

    ULONG ModeFlags;
/** Use the UDMA mode on drive 0/1 */
#define IDE_ACPI_TIMING_MODE_FLAG_UDMA(Drive)           (0x01 << (2 * (Drive)))

/** Enable the IORDY signal on drive 0/1 */
#define IDE_ACPI_TIMING_MODE_FLAG_IORDY(Drive)          (0x02 << (2 * (Drive)))

/** Independent timing available */
#define IDE_ACPI_TIMING_MODE_FLAG_INDEPENDENT_TIMINGS   0x10
} IDE_ACPI_TIMING_MODE_BLOCK, *PIDE_ACPI_TIMING_MODE_BLOCK;

/** _GTF data buffer */
typedef struct _ATA_ACPI_TASK_FILE
{
    UCHAR Feature;
    UCHAR SectorCount;
    UCHAR LowLba;
    UCHAR MidLba;
    UCHAR HighLba;
    UCHAR DriveSelect;
    UCHAR Command;
} ATA_ACPI_TASK_FILE, *PATA_ACPI_TASK_FILE;

#include <poppack.h>

typedef struct _ATA_DEVICE_REQUEST     ATA_DEVICE_REQUEST, *PATA_DEVICE_REQUEST;

typedef enum _PORT_NOTIFICATION_TYPE
{
    AtaRequestComplete = 0,
    AtaResetDetected,
    AtaBusChangeDetected,
    AtaRequestFailed,
    AtaAsyncNotificationDetected,
} PORT_NOTIFICATION_TYPE, *PPORT_NOTIFICATION_TYPE;

typedef enum _ATA_CONNECTION_STATUS
{
    CONN_STATUS_FAILURE,
    CONN_STATUS_NO_DEVICE,
    CONN_STATUS_DEV_UNKNOWN,
    CONN_STATUS_DEV_ATA,
    CONN_STATUS_DEV_ATAPI,
} ATA_CONNECTION_STATUS;

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
    PUCHAR Scr;
} IDE_REGISTERS, *PIDE_REGISTERS;

typedef struct _CHANNEL_DEVICE_CONFIG
{
    ULONG SupportedModes;
    ULONG PioMode;
    ULONG DmaMode;
    ULONG MinPioCycleTime;
    ULONG MinMwDmaCycleTime;
    ULONG MinSwDmaCycleTime;
    ULONG CurrentModes;
    BOOLEAN IsFixedDisk;
    BOOLEAN IoReadySupported;
    BOOLEAN IsNewDevice;
    PCSTR FriendlyName;
    PIDENTIFY_DEVICE_DATA IdentifyDeviceData;
} CHANNEL_DEVICE_CONFIG, *PCHANNEL_DEVICE_CONFIG;

typedef enum
{
    COMPLETE_IRP = 0,
    COMPLETE_NO_IRP,
    COMPLETE_START_AGAIN,
} ATA_COMPLETION_ACTION;

typedef ATA_COMPLETION_ACTION
(REQUEST_COMPLETION_ROUTINE)(
    _In_ PATA_DEVICE_REQUEST Request);
typedef REQUEST_COMPLETION_ROUTINE *PREQUEST_COMPLETION_ROUTINE;

typedef struct _ATA_IO_CONTEXT_COMMON
{
    ULONG SectorSize;

    ULONG TransportFlags;
#define DEVICE_NUMBER_MASK                       0x0000000F
#define DEVICE_IS_ATAPI                          0x00000010
#define DEVICE_HAS_CDB_INTERRUPT                 0x00000020
#define DEVICE_NEED_DMA_DIRECTION                0x00000040
#define DEVICE_IS_NEC_CDR260                     0x00000080
#define DEVICE_QUEUE_DEPTH_MASK                  0x0000FF00

#define DEVICE_QUEUE_DEPTH_SHIFT                 8

    UCHAR MultiSectorCount;
    UCHAR CdbSize;
    UCHAR DeviceSelect;
} ATA_IO_CONTEXT_COMMON, *PATA_IO_CONTEXT_COMMON;

/** ATA Task File interface */
typedef struct _ATA_TASKFILE
{
    union
    {
        UCHAR Command;
        UCHAR Status;
    };
    union
    {
        UCHAR Feature;
        UCHAR Error;
    };
    UCHAR LowLba;        ///< LBA bits 0-7
    UCHAR MidLba;        ///< LBA bits 8-15
    UCHAR HighLba;       ///< LBA bits 16-23
    UCHAR DriveSelect;
    UCHAR LowLbaEx;      ///< LBA bits 24-31
    UCHAR MidLbaEx;      ///< LBA bits 32-39
    UCHAR HighLbaEx;     ///< LBA bits 40-47
    UCHAR FeatureEx;
    UCHAR SectorCount;
    UCHAR SectorCountEx;
    UCHAR Icc;           ///< Isochronous Command Completion
    ULONG Auxiliary;
} ATA_TASKFILE, *PATA_TASKFILE;

/** ATA device request context */
typedef struct _ATA_DEVICE_REQUEST
{
    union
    {
        /** Completion queue entry */
        SLIST_ENTRY CompletionEntry;

        /** Port queue entry */
        LIST_ENTRY PortEntry;
    };
    union
    {
        UCHAR Cdb[16];
        ATA_TASKFILE TaskFile;
    };
    PVOID DataBuffer;
    PSCATTER_GATHER_LIST SgList;
    PATA_IO_CONTEXT_COMMON Device;
    ULONG DataTransferLength;
    ULONG Flags;
    PMDL Mdl;
    PIRP Irp;
    PSCSI_REQUEST_BLOCK Srb;
    UCHAR SrbStatus;
    PREQUEST_COMPLETION_ROUTINE Complete;
    ULONG Tag;
    ULONG Slot;
    ULONG TimeOut;
    union
    {
        struct
        {
            UCHAR InternalState;
            UCHAR TranslationState;
            UCHAR Reserved1;
            UCHAR Reserved2;
        };
        ULONG State;
    };

    /**
     * @brief ATA normal/error outputs.
     *
     * The contents of registers are only valid
     * if flags has the REQUEST_FLAG_HAS_TASK_FILE bit set.
     *
     * The driver always updates it on ATA errors as we have to set the ATA LBA field.
     * For ATAPI errors only the Status and Error fields are updated, as an optimization.
     */
    ATA_TASKFILE Output;

#if DBG
    ULONG Signature;
#define ATA_DEVICE_REQUEST_SIGNATURE   'rATA'
#endif
} ATA_DEVICE_REQUEST, *PATA_DEVICE_REQUEST;

/** Initial state */
#define REQUEST_STATE_NONE           0

/** SRB not translated into the device request or translaton failed */
#define REQUEST_STATE_NOT_STARTED    1

/** Requeue the device request */
#define REQUEST_STATE_REQUEUE        2

/** Freeze the device queue upon completion of IRP */
#define REQUEST_STATE_FREEZE_QUEUE   3

/** ATA protocols */
/*@{*/
/** DMA ATA command */
#define REQUEST_FLAG_DMA                      0x00000001

/** ATAPI PACKET command */
#define REQUEST_FLAG_PACKET_COMMAND           0x00000002

/** DMA Queued command */
#define REQUEST_FLAG_NCQ                      0x00000004

/** Data-In command */
#define REQUEST_FLAG_DATA_IN                  0x00000040

/** Data-Out command */
#define REQUEST_FLAG_DATA_OUT                 0x00000080

/** Software Reset command (AHCI only) */
#define REQUEST_FLAG_RST_COMMAND              0x00040000
/*@}*/

/** 48-bit command */
#define REQUEST_FLAG_LBA48                    0x00000008

/** Multiple mode command */
#define REQUEST_FLAG_READ_WRITE_MULTIPLE      0x00000010

/** ATA read/write command */
#define REQUEST_FLAG_READ_WRITE               0x00000020

/** Forced unit access command */
#define REQUEST_FLAG_FUA                      0x00000100

/** Return the contents of task file registers to the caller */
#define REQUEST_FLAG_SAVE_TASK_FILE           0x00000200

/** Holds the saved contents of task file registers */
#define REQUEST_FLAG_HAS_TASK_FILE            0x00000400

/** Has extra bits in the device register */
#define REQUEST_FLAG_SET_DEVICE_REGISTER      0x00000800

/** Has extra bits in the Auxiliary field */
#define REQUEST_FLAG_SET_AUXILIARY_FIELD      0x00001000

/** Has extra bits in the Isochronous Command Completion field */
#define REQUEST_FLAG_SET_ICC_FIELD            0x00002000

/** Exclusive port access required */
#define REQUEST_FLAG_EXCLUSIVE                0x00004000

/** The request owns the local buffer */
#define REQUEST_FLAG_HAS_LOCAL_BUFFER         0x00008000

/** The request owns the S/G list and will also release it */
#define REQUEST_FLAG_HAS_SG_LIST              0x00010000

/** The request owns the MDL and will also release it */
#define REQUEST_FLAG_HAS_MDL                  0x00020000

/** The request owns the reserved memory mapping and will also release it */
#define REQUEST_FLAG_HAS_RESERVED_MAPPING     0x00080000

/** Copy of SRB_FLAGS_NO_KEEP_AWAKE */
#define REQUEST_FLAG_NO_KEEP_AWAKE            0x00100000

/** Use the DMA engine for the transfer */
#define REQUEST_FLAG_PROGRAM_DMA              0x00200000

/** Internal command */
#define REQUEST_FLAG_INTERNAL                 0x00400000

/** ATA or SCSI pass-through command */
#define REQUEST_FLAG_PASSTHROUGH              0x00800000

#define REQUEST_FLAG_DEVICE_EXCLUSIVE_ACCESS  0x01000000

/** Polled command */
#define REQUEST_FLAG_POLL                     0x80000000

/**
 * Exclusive port access required.
 *
 * 1) Since there is only one Received FIS structure, it should be necessary to
 * synchronize RFIS reads amongst multiple I/O requests.
 *
 * 2) We also mark all requests that modify the device extension state
 * (IDE_COMMAND_IDENTIFY, IDE_COMMAND_ATAPI_IDENTIFY,
 * IDE_COMMAND_SET_FEATURE, and others) as REQUEST_FLAG_EXCLUSIVE.
 */
#define REQUEST_EXCLUSIVE_ACCESS_FLAGS \
    (REQUEST_FLAG_SAVE_TASK_FILE | REQUEST_FLAG_EXCLUSIVE)

/** DMA command translation */
#define REQUEST_DMA_FLAGS \
    (REQUEST_FLAG_DMA | REQUEST_FLAG_PROGRAM_DMA)

typedef VOID
(__cdecl PORT_NOTIFICATION)(
    _In_ PORT_NOTIFICATION_TYPE NotificationType,
    _In_ PVOID PortContext,
    ...);
typedef PORT_NOTIFICATION *PPORT_NOTIFICATION;

typedef NTSTATUS
(CONTROLLER_ATTACH_CHANNEL)(
    _In_ PVOID ChannelContext,
    _In_ BOOLEAN Attach);
typedef CONTROLLER_ATTACH_CHANNEL *PCONTROLLER_ATTACH_CHANNEL;

typedef VOID
(CHANNEL_SET_DEVICE_DATA)(
    _In_ PVOID ChannelContext,
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIDENTIFY_DEVICE_DATA IdentifyDeviceData);
typedef CHANNEL_SET_DEVICE_DATA *PCHANNEL_SET_DEVICE_DATA;

typedef PVOID
(CHANNEL_GET_INIT_TASK_FILE)(
    _In_ PVOID ChannelContext,
    _In_ PDEVICE_OBJECT DeviceObject);
typedef CHANNEL_GET_INIT_TASK_FILE *PCHANNEL_GET_INIT_TASK_FILE;

typedef BOOLEAN
(CHANNEL_DOWNGRADE_INTERFACE_SPEED)(
    _In_ PVOID ChannelContext);
typedef CHANNEL_DOWNGRADE_INTERFACE_SPEED *PCHANNEL_DOWNGRADE_INTERFACE_SPEED;

typedef VOID
(CHANNEL_ABORT_CHANNEL)(
    _In_ PVOID ChannelContext,
    _In_ BOOLEAN DisableInterrupts);
typedef CHANNEL_ABORT_CHANNEL *PCHANNEL_ABORT_CHANNEL;

typedef VOID
(CHANNEL_RESET_CHANNEL)(
    _In_ PVOID ChannelContext);
typedef CHANNEL_RESET_CHANNEL *PCHANNEL_RESET_CHANNEL;

typedef ULONG
(CHANNEL_ENUMERATE_CHANNEL)(
    _In_ PVOID ChannelContext);
typedef CHANNEL_ENUMERATE_CHANNEL *PCHANNEL_ENUMERATE_CHANNEL;

typedef ATA_CONNECTION_STATUS
(CHANNEL_IDENTIFY_DEVICE)(
    _In_ PVOID ChannelContext,
    _In_ ULONG DeviceNumber);
typedef CHANNEL_IDENTIFY_DEVICE *PCHANNEL_IDENTIFY_DEVICE;

_IRQL_requires_(DISPATCH_LEVEL)
typedef VOID
(CHANNEL_SET_MODE)(
    _In_ PVOID ChannelContext,
    _In_reads_(ATA_MAX_DEVICE) PCHANNEL_DEVICE_CONFIG* DeviceList);
typedef CHANNEL_SET_MODE *PCHANNEL_SET_MODE;

_IRQL_requires_(DISPATCH_LEVEL)
typedef BOOLEAN
(CHANNEL_ALLOCATE_SLOT)(
    _In_ PVOID ChannelContext,
    _In_ PATA_DEVICE_REQUEST Request,
    _In_ BOOLEAN Allocate);
typedef CHANNEL_ALLOCATE_SLOT *PCHANNEL_ALLOCATE_SLOT;

_IRQL_requires_(DISPATCH_LEVEL)
typedef VOID
(CHANNEL_PREPARE_PRD_TABLE)(
    _In_ PVOID ChannelContext,
    _In_ PATA_DEVICE_REQUEST Request,
    _In_ SCATTER_GATHER_LIST* __restrict SgList);
typedef CHANNEL_PREPARE_PRD_TABLE *PCHANNEL_PREPARE_PRD_TABLE;

_IRQL_requires_(DISPATCH_LEVEL)
typedef VOID
(CHANNEL_PREPARE_IO)(
    _In_ PVOID ChannelContext,
    _In_ PATA_DEVICE_REQUEST Request);
typedef CHANNEL_PREPARE_IO *PCHANNEL_PREPARE_IO;

_IRQL_requires_(HIGH_LEVEL)
typedef BOOLEAN
(CHANNEL_START_IO)(
    _In_ PVOID ChannelContext,
    _In_ PATA_DEVICE_REQUEST Request);
typedef CHANNEL_START_IO *PCHANNEL_START_IO;

typedef NTSTATUS
(CONTROLLER_PNP_ADD_DEVICE)(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PDEVICE_OBJECT PhysicalDeviceObject,
    _Out_ PVOID *ControllerContext);
typedef CONTROLLER_PNP_ADD_DEVICE *PCONTROLLER_PNP_ADD_DEVICE;

typedef NTSTATUS
(CONTROLLER_PNP_START_DEVICE)(
    _In_ PVOID ControllerContext,
    _In_ PCM_RESOURCE_LIST ResourcesTranslated);
typedef CONTROLLER_PNP_START_DEVICE *PCONTROLLER_PNP_START_DEVICE;

typedef VOID
(CONTROLLER_PNP_REMOVE_DEVICE)(
    _In_ PVOID ControllerContext);
typedef CONTROLLER_PNP_REMOVE_DEVICE *PCONTROLLER_PNP_REMOVE_DEVICE;

/* ReactOS-specific legacy detection magic */
#define PCIIDEX_GET_CONTROLLER_INTERFACE_SIGNATURE    (0xFFFFFFFF - 0x1000)

/**
 * @brief Legacy detection interface with the PCIIDEX driver.
 *
 * @note This interface is ROS-specific.
 */
typedef struct _PCIIDEX_LEGACY_CONTROLLER_INTERFACE
{
    ULONG Version;
    PCONTROLLER_PNP_ADD_DEVICE AddDevice;
    PCONTROLLER_PNP_START_DEVICE StartDevice;
    PCONTROLLER_PNP_REMOVE_DEVICE RemoveDevice;
} PCIIDEX_LEGACY_CONTROLLER_INTERFACE, *PPCIIDEX_LEGACY_CONTROLLER_INTERFACE;

/**
 * @brief Channel interface with the PCIIDEX driver.
 *
 * This interface is ROS-specific.
 */
typedef struct _PCIIDEX_CHANNEL_INTERFACE
{
    /* Common interface header */
    USHORT Size;
    USHORT Version;
    PVOID Context;
    PINTERFACE_REFERENCE InterfaceReference;
    PINTERFACE_DEREFERENCE InterfaceDereference;

    PVOID ChannelContext;
    PCONTROLLER_ATTACH_CHANNEL AttachChannel;
    PKINTERRUPT InterruptObject;
    ULONG Channel;
    ULONG TransferModeSupported;
    ULONG MaximumTransferLength;
    ULONG MaximumPhysicalPages;
    ULONG QueueDepth;
    ULONG MaxTargetId;
    PCHANNEL_SET_DEVICE_DATA SetDeviceData;
    PCHANNEL_GET_INIT_TASK_FILE GetInitTaskFile;
    PCHANNEL_DOWNGRADE_INTERFACE_SPEED DowngradeInterfaceSpeed;
    PCHANNEL_ABORT_CHANNEL AbortChannel;
    PCHANNEL_RESET_CHANNEL ResetChannel;
    PCHANNEL_ENUMERATE_CHANNEL EnumerateChannel;
    PCHANNEL_IDENTIFY_DEVICE IdentifyDevice;
    PCHANNEL_SET_MODE SetTransferMode;
    PCHANNEL_ALLOCATE_SLOT AllocateSlot;
    PCHANNEL_PREPARE_PRD_TABLE PreparePrdTable;
    PCHANNEL_PREPARE_IO PrepareIo;
    PCHANNEL_START_IO StartIo;
    PCONTROLLER_OBJECT HwSyncObject;
    PDMA_ADAPTER DmaAdapter;
    PDEVICE_OBJECT ChannelObject;
    ULONG Flags;
#define ATA_CHANNEL_FLAG_PIO_VIA_DMA                    0x00000001
#define ATA_CHANNEL_FLAG_IS_EXTERNAL                    0x00000002
#define ATA_CHANNEL_FLAG_NCQ                            0x00000004
#define ATA_CHANNEL_FLAG_IS_AHCI                        0x00000008
#define ATA_CHANNEL_FLAG_PIO_FOR_LBA48_XFER             0x00000010

    PVOID* PortContext;
    PPORT_NOTIFICATION* PortNotification;
    PATA_DEVICE_REQUEST** Slots;
} PCIIDEX_CHANNEL_INTERFACE, *PPCIIDEX_CHANNEL_INTERFACE;

DEFINE_GUID(GUID_PCIIDE_INTERFACE_ROS,
            0xD677FBCF, 0xABED, 0x47C8, 0x80, 0xA3, 0xE4, 0x34, 0x7E, 0xA4, 0x96, 0x47);
