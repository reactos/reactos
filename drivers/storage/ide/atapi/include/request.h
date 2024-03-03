/*
 * PROJECT:     ReactOS ATA Port Driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     ATA device request (IRP context) definitions
 * COPYRIGHT:   Copyright 2024 Dmitry Borisov <di.sean@protonmail.com>
 */

#define QUEUE_ENTRY_FROM_IRP(Irp) \
    ((PREQUEST_QUEUE_ENTRY)&(((PIRP)(Irp))->Tail.Overlay.DriverContext[0]))

#define IRP_FROM_QUEUE_ENTRY(QueueEntry) \
    CONTAINING_RECORD(QueueEntry, IRP, Tail.Overlay.DriverContext[0])

#define LIST_ENTRY_FROM_IRP(Irp) \
    ((PLIST_ENTRY)&(Irp)->Tail.Overlay.DriverContext[0])

#define IRP_FROM_LIST_ENTRY(ListEntry) \
    (CONTAINING_RECORD(ListEntry, IRP, Tail.Overlay.DriverContext[0]))

typedef enum
{
    COMPLETE_IRP = 0,
    COMPLETE_NO_IRP,
    COMPLETE_START_AGAIN,
} ATA_COMPLETION_ACTION;

typedef struct _REQUEST_QUEUE_ENTRY
{
    LIST_ENTRY ListEntry;
    PVOID Context;
    ULONG SortKey;
} REQUEST_QUEUE_ENTRY, *PREQUEST_QUEUE_ENTRY;

C_ASSERT(sizeof(REQUEST_QUEUE_ENTRY) <= 4 * sizeof(PVOID));

typedef struct _ATA_DEVICE_REQUEST ATA_DEVICE_REQUEST, *PATA_DEVICE_REQUEST;

typedef ATA_COMPLETION_ACTION
(REQUEST_COMPLETION_ROUTINE)(
    _In_ PATA_DEVICE_REQUEST Request);
typedef REQUEST_COMPLETION_ROUTINE *PREQUEST_COMPLETION_ROUTINE;

typedef struct _ATA_TASKFILE
{
    union
    {
        UCHAR Feature;
        UCHAR Error;
    };
    UCHAR SectorCount;
    UCHAR LowLba;        // LBA bits 0-7
    UCHAR MidLba;        // LBA bits 8-15
    UCHAR HighLba;       // LBA bits 16-23
    union
    {
        UCHAR Command;
        UCHAR Status;
    };
    UCHAR DriveSelect;

    UCHAR FeatureEx;
    UCHAR SectorCountEx;
    UCHAR LowLbaEx;      // LBA bits 24-31
    UCHAR MidLbaEx;      // LBA bits 32-39
    UCHAR HighLbaEx;     // LBA bits 40-47

    UCHAR Icc;
    ULONG Auxiliary;
} ATA_TASKFILE, *PATA_TASKFILE;

typedef struct _ATA_DEVICE_REQUEST
{
    PVOID DataBuffer;
    PSCATTER_GATHER_LIST SgList;
    PATAPORT_DEVICE_EXTENSION DevExt;
    ULONG DataTransferLength;
    ULONG Flags;
    PMDL Mdl;
    PIRP Irp;
    PSCSI_REQUEST_BLOCK Srb;
    UCHAR SrbStatus;
    UCHAR Error;
    UCHAR Status;
    union
    {
        UCHAR Cdb[16];
        ATA_TASKFILE TaskFile;
    };
    PREQUEST_COMPLETION_ROUTINE Complete;
    ULONG Tag;
    ULONG Slot;
    ULONG TimeOut;
    union
    {
        struct
        {
            UCHAR InternalState;
#define REQUEST_STATE_NONE           0
#define REQUEST_STATE_NOT_STARTED    1
#define REQUEST_STATE_REQUEUE        2
#define REQUEST_STATE_FREEZE_QUEUE   3

            UCHAR TranslationState;
            UCHAR RetryCount;
            UCHAR Reserved;
        };
        ULONG State;
    };
} ATA_DEVICE_REQUEST, *PATA_DEVICE_REQUEST;

/** ATA protocols */
/*@{*/
/** DMA command */
#define REQUEST_FLAG_DMA                      0x00000001

/** ATAPI PACKET command */
#define REQUEST_FLAG_PACKET_COMMAND           0x00000002

/** DMA Queued command */
#define REQUEST_FLAG_NCQ                      0x00000004

/** Data-In command */
#define REQUEST_FLAG_DATA_IN                  0x00000040

/** Data-Out command */
#define REQUEST_FLAG_DATA_OUT                 0x00000080
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

/** Has extra bits in the auxiliary field */
#define REQUEST_FLAG_SET_AUXILIARY_FIELD      0x00001000

/** Has extra bits in the isochronous command completion field */
#define REQUEST_FLAG_SET_ICC_FIELD            0x00002000

/** Exclusive port access required */
#define REQUEST_FLAG_EXCLUSIVE                0x00004000

/** The request owns the MDL and will also release it */
#define REQUEST_FLAG_HAS_MDL                  0x00008000

/** The request owns the S/G list and will also release it */
#define REQUEST_FLAG_HAS_SG_LIST              0x00010000

/** The request owns the local buffer */
#define REQUEST_FLAG_HAS_LOCAL_BUFFER         0x00020000

/** The request owns the reserved memory mapping and will also release it */
#define REQUEST_FLAG_RESERVED_MAPPING         0x00040000

/** Internal command */
#define REQUEST_FLAG_INTERNAL                 0x00080000

/** Use the DMA engine for the transfer */
#define REQUEST_FLAG_PROGRAM_DMA              0x00100000

/** ATA or SCSI pass-through command */
#define REQUEST_FLAG_PASSTHROUGH              0x00200000

/** Polled command (PATA only) */
#define REQUEST_FLAG_SYNC_MODE                0x80000000

/**
 * Exclusive port access required.
 * Since there is only one Received FIS structure, it should be necessary to
 * synchronize RFIS reads amongst multiple I/O requests.
 *
 * We also mark all requests that modify the device extension state
 * (IDE_COMMAND_IDENTIFY, IDE_COMMAND_ATAPI_IDENTIFY,
 * IDE_COMMAND_SET_FEATURE, and others) as REQUEST_FLAG_EXCLUSIVE.
 */
#define REQUEST_EXCLUSIVE_ACCESS_FLAGS \
    (REQUEST_FLAG_EXCLUSIVE | REQUEST_FLAG_SAVE_TASK_FILE)

C_ASSERT(REQUEST_FLAG_DATA_IN == SRB_FLAGS_DATA_IN);
C_ASSERT(REQUEST_FLAG_DATA_OUT == SRB_FLAGS_DATA_OUT);
