/*
 * PROJECT:     ReactOS NVM Express Miniport Driver
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Common header file
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 */

#pragma once

#include <ntddk.h>
#include <storport.h>
#include <srbhelper.h>
#include <nvme.h>

/* Memory tags */
#define TAG_NVME_QUEUE      'QvNS'
#define TAG_NVME_IDENTIFY   'IvNS'

/* MLBAR bits 31:14 hold the register base, MUBAR has the upper half */
#define NVME_MLBAR_ADDRESS_MASK 0xFFFFC000

/* CAP.TO is in 500 ms units */
#define NVME_TIMEOUT_UNIT_MS    500

/* Queue entry sizes are reported as powers of two */
#define NVME_SQ_ENTRY_SHIFT     6
#define NVME_CQ_ENTRY_SHIFT     4

/* Smallest memory page size, 2^(12 + CAP.MPSMIN) */
#define NVME_MIN_PAGE_SHIFT     12

/* Smallest LBA data size a namespace can report */
#define NVME_MIN_BLOCK_SHIFT    9

/* Admin queue entries, the spec allows up to 4096 */
#define NVME_ADMIN_QUEUE_DEPTH  64

/* Namespaces map to LUNs, so this is what a one byte LUN can address */
#define NVME_MAX_NAMESPACES     255

/* Keeps any transfer within one PRP list page, even when it starts unaligned */
#define NVME_MAX_TRANSFER_LENGTH (1024 * 1024)

/* Single IO queue pair */
#define NVME_IO_QUEUE_ID        1
#define NVME_IO_QUEUE_DEPTH     1024

/* PRP entries that fit in one list page */
#define NVME_MAX_PRP_ENTRIES    (PAGE_SIZE / sizeof(ULONGLONG))

/* Command identifiers in flight on the IO queue */
#define NVME_MAX_OUTSTANDING    NVME_IO_QUEUE_DEPTH

/* Admin commands issued for SRBs use identifiers 0 to NVME_ADMIN_SLOTS - 1 */
#define NVME_ADMIN_SLOTS        16

/* Identifier of the one polled admin command that may be in flight */
#define NVME_POLLED_COMMAND_ID  NVME_ADMIN_SLOTS

typedef struct _NVME_NAMESPACE
{
    ULONG NamespaceId;
    ULONGLONG BlockCount;
    ULONG BlockSize;
    ULONG BlockShift;
    UCHAR Nguid[16];
    UCHAR Eui64[8];
    BOOLEAN ThinProvisioning;
    BOOLEAN Present;
} NVME_NAMESPACE, *PNVME_NAMESPACE;

/* SBC ASCQs the WDK has no names for */
#define NVME_SENSEQ_APPLICATION_TAG_CHECK_FAILED 0x02
#define NVME_SENSEQ_REFERENCE_TAG_CHECK_FAILED   0x03

/* CSTS polling interval */
#define NVME_POLL_INTERVAL_US   10000

/* Timeout for a polled admin command */
#define NVME_ADMIN_TIMEOUT_MS   60000

/* Least time given to a shutdown notification */
#define NVME_SHUTDOWN_TIMEOUT_MS 5000

/* Queue depth cap in dump mode, where uncached memory is scarce */
#define NVME_DUMP_QUEUE_DEPTH   64

/* Submission and completion queue pair, queue 0 is the admin pair */
typedef struct _NVME_QUEUE_PAIR
{
    PNVME_COMMAND SubmissionQueue;
    PHYSICAL_ADDRESS SubmissionAddress;
    ULONG SubmissionTail;

    PNVME_COMPLETION_ENTRY CompletionQueue;
    PHYSICAL_ADDRESS CompletionAddress;
    ULONG CompletionHead;

    /* Expected phase tag, starts at 1 and flips on every wrap */
    ULONG Phase;

    ULONG Depth;
    USHORT QueueId;
} NVME_QUEUE_PAIR, *PNVME_QUEUE_PAIR;

typedef struct _NVME_ADAPTER_EXTENSION *PNVME_ADAPTER_EXTENSION;
typedef struct _NVME_REQUEST_CONTEXT *PNVME_REQUEST_CONTEXT;

/* Called at interrupt time, with the queue lock held, when an admin command for an SRB completes */
typedef
VOID
NVME_ADMIN_COMPLETION(
    _In_ PNVME_ADAPTER_EXTENSION Adapter,
    _In_ PNVME_REQUEST_CONTEXT Context,
    _In_ PNVME_COMPLETION_ENTRY Completion);

typedef NVME_ADMIN_COMPLETION *PNVME_ADMIN_COMPLETION;

/* SRB extension, the PRP list or admin data page lives in the page aligned space after it */
typedef struct _NVME_REQUEST_CONTEXT
{
    PVOID Srb;

    /* NULL for commands that go to the IO queue */
    PNVME_ADMIN_COMPLETION AdminCompletion;

    NVME_COMMAND Command;
} NVME_REQUEST_CONTEXT;

typedef enum _NVME_ADAPTER_STATE
{
    NvmeAdapterStopped = 0,
    NvmeAdapterFound,
    NvmeAdapterRunning,
    NvmeAdapterFailed
} NVME_ADAPTER_STATE;

/* Device extension */
typedef struct _NVME_ADAPTER_EXTENSION
{
    NVME_ADAPTER_STATE State;

    /* Mapped registers and the doorbells after them */
    PNVME_CONTROLLER_REGISTERS Registers;
    PULONG Doorbells;

    NVME_CONTROLLER_CAPABILITIES Capabilities;
    NVME_VERSION Version;

    /* Bytes between doorbells, from CAP.DSTRD */
    ULONG DoorbellStride;

    /* CAP.TO in milliseconds */
    ULONG ReadyTimeout;

    /* CC.MPS page size */
    ULONG PageSize;
    ULONG PageShift;

    /* In bytes, from MDTS */
    ULONG MaximumTransferLength;

    ULONG AdminQueueDepth;

    NVME_QUEUE_PAIR AdminQueue;

    NVME_QUEUE_PAIR IoQueue;
    ULONG IoQueueDepth;

    /* Admin commands issued for SRBs, indexed by command identifier */
    PNVME_REQUEST_CONTEXT AdminRequests[NVME_ADMIN_SLOTS];

    /* Result of the polled admin command */
    NVME_COMPLETION_ENTRY PolledCompletion;
    BOOLEAN PolledDone;

    /* Uncached extension holding the queues and the scratch page */
    PVOID QueueMemory;
    PHYSICAL_ADDRESS QueueMemoryAddress;
    ULONG QueueMemorySize;

    /* Identify buffer, only used during startup */
    PVOID ScratchBuffer;
    PHYSICAL_ADDRESS ScratchAddress;

    /* From Identify Controller */
    USHORT VendorId;
    UCHAR PowerStateCount;
    ULONG NamespaceCount;
    UCHAR SerialNumber[20];
    UCHAR ModelNumber[40];
    UCHAR FirmwareRevision[8];

    /* VWC, flushes are required */
    BOOLEAN VolatileWriteCache;

    /* Indexed by NSID - 1 */
    NVME_NAMESPACE Namespaces[NVME_MAX_NAMESPACES];

    /* Outstanding requests, indexed by command identifier */
    PNVME_REQUEST_CONTEXT Requests[NVME_MAX_OUTSTANDING];

    /* Free command identifier stack */
    USHORT FreeCommandIds[NVME_MAX_OUTSTANDING];
    ULONG FreeCommandCount;


    /* Crash dump or hibernation stack */
    BOOLEAN DumpMode;

    BOOLEAN MessageInterrupts;

    /* Set from HwInitialize on, the queue lock is only usable once interrupts are connected */
    BOOLEAN InterruptsConnected;
} NVME_ADAPTER_EXTENSION;

/* Each queue has a submission tail doorbell followed by a completion head doorbell */
FORCEINLINE
PULONG
NvmpSubmissionDoorbell(
    _In_ PNVME_ADAPTER_EXTENSION Adapter,
    _In_ ULONG QueueId)
{
    return (PULONG)((PUCHAR)Adapter->Doorbells +
                    ((2 * QueueId) * Adapter->DoorbellStride));
}

FORCEINLINE
PULONG
NvmpCompletionDoorbell(
    _In_ PNVME_ADAPTER_EXTENSION Adapter,
    _In_ ULONG QueueId)
{
    return (PULONG)((PUCHAR)Adapter->Doorbells +
                    (((2 * QueueId) + 1) * Adapter->DoorbellStride));
}

/* Excludes the interrupt routine, which only holds its message lock in per message mode */
typedef struct _NVME_QUEUE_LOCK
{
    STOR_LOCK_HANDLE Handle;
    ULONG OldIrql;
} NVME_QUEUE_LOCK, *PNVME_QUEUE_LOCK;

FORCEINLINE
VOID
NvmpAcquireQueueLock(
    _In_ PNVME_ADAPTER_EXTENSION Adapter,
    _Out_ PNVME_QUEUE_LOCK Lock)
{
    if (!Adapter->InterruptsConnected)
        return;

    if (Adapter->MessageInterrupts)
        StorPortAcquireMSISpinLock(Adapter, 0, &Lock->OldIrql);
    else
        StorPortAcquireSpinLock(Adapter, InterruptLock, NULL, &Lock->Handle);
}

FORCEINLINE
VOID
NvmpReleaseQueueLock(
    _In_ PNVME_ADAPTER_EXTENSION Adapter,
    _In_ PNVME_QUEUE_LOCK Lock)
{
    if (!Adapter->InterruptsConnected)
        return;

    if (Adapter->MessageInterrupts)
        StorPortReleaseMSISpinLock(Adapter, 0, Lock->OldIrql);
    else
        StorPortReleaseSpinLock(Adapter, &Lock->Handle);
}

/* nvmectrl.c */

BOOLEAN
NvmpDisableController(
    _In_ PNVME_ADAPTER_EXTENSION Adapter);

BOOLEAN
NvmpEnableController(
    _In_ PNVME_ADAPTER_EXTENSION Adapter);

BOOLEAN
NvmpShutdownController(
    _In_ PNVME_ADAPTER_EXTENSION Adapter);

BOOLEAN
NvmpCreateAdminQueues(
    _In_ PNVME_ADAPTER_EXTENSION Adapter,
    _In_ PPORT_CONFIGURATION_INFORMATION ConfigInfo);

BOOLEAN
NvmpStartController(
    _In_ PNVME_ADAPTER_EXTENSION Adapter,
    _In_ PPORT_CONFIGURATION_INFORMATION ConfigInfo);

BOOLEAN
NvmpResetController(
    _In_ PNVME_ADAPTER_EXTENSION Adapter);

/* nvmequeue.c */

VOID
NvmpSubmitCommand(
    _In_ PNVME_ADAPTER_EXTENSION Adapter,
    _In_ PNVME_QUEUE_PAIR Queue,
    _In_ PNVME_COMMAND Command);

BOOLEAN
NvmpNextCompletion(
    _In_ PNVME_ADAPTER_EXTENSION Adapter,
    _In_ PNVME_QUEUE_PAIR Queue,
    _Out_ PNVME_COMPLETION_ENTRY Completion);

BOOLEAN
NvmpIssueAdminCommand(
    _In_ PNVME_ADAPTER_EXTENSION Adapter,
    _In_ PNVME_COMMAND Command,
    _Out_opt_ PNVME_COMPLETION_ENTRY Completion);

BOOLEAN
NvmpSubmitAdminRequest(
    _In_ PNVME_ADAPTER_EXTENSION Adapter,
    _In_ PNVME_REQUEST_CONTEXT Context);

BOOLEAN
NvmpDrainAdminQueue(
    _In_ PNVME_ADAPTER_EXTENSION Adapter);

/* nvmeid.c */

BOOLEAN
NvmpIdentifyController(
    _In_ PNVME_ADAPTER_EXTENSION Adapter);

BOOLEAN
NvmpIdentifyNamespace(
    _In_ PNVME_ADAPTER_EXTENSION Adapter,
    _In_ ULONG NamespaceId,
    _Out_ PNVME_NAMESPACE Namespace);

VOID
NvmpEnumerateNamespaces(
    _In_ PNVME_ADAPTER_EXTENSION Adapter);

/* nvmeioq.c */

BOOLEAN
NvmpCreateIoQueues(
    _In_ PNVME_ADAPTER_EXTENSION Adapter);

VOID
NvmpCompleteRequest(
    _In_ PNVME_ADAPTER_EXTENSION Adapter,
    _In_ PVOID Srb,
    _In_ UCHAR SrbStatus);

/* nvmeio.c */

VOID
NvmpInitializeCommandIds(
    _In_ PNVME_ADAPTER_EXTENSION Adapter);

VOID
NvmpFailOutstandingRequests(
    _In_ PNVME_ADAPTER_EXTENSION Adapter,
    _In_ UCHAR SrbStatus);

PVOID
NvmpContextPage(
    _In_ PNVME_REQUEST_CONTEXT Context);

BOOLEAN
NvmpBuildReadWrite(
    _In_ PNVME_ADAPTER_EXTENSION Adapter,
    _In_ PVOID Srb,
    _In_ PNVME_NAMESPACE Namespace,
    _In_ PNVME_REQUEST_CONTEXT Context,
    _In_ PCDB Cdb);

BOOLEAN
NvmpBuildFlush(
    _In_ PNVME_ADAPTER_EXTENSION Adapter,
    _In_ PVOID Srb);

VOID
NvmpPostCommand(
    _In_ PNVME_ADAPTER_EXTENSION Adapter,
    _In_ PVOID Srb);

VOID
NvmpCompleteFromEntry(
    _In_ PNVME_ADAPTER_EXTENSION Adapter,
    _In_ PNVME_COMPLETION_ENTRY Completion);

VOID
NvmpCompleteWithStatus(
    _In_ PNVME_ADAPTER_EXTENSION Adapter,
    _In_ PVOID Srb,
    _In_ NVME_COMMAND_STATUS Status);

/* nvmescsi.c */

PNVME_NAMESPACE
NvmpNamespaceFromLun(
    _In_ PNVME_ADAPTER_EXTENSION Adapter,
    _In_ ULONG Lun);

VOID
NvmpSetSenseData(
    _In_ PVOID Srb,
    _In_ UCHAR ScsiStatus,
    _In_ UCHAR SenseKey,
    _In_ UCHAR AdditionalSenseCode,
    _In_ UCHAR AdditionalSenseCodeQualifier);

VOID
NvmpCompleteIllegalRequest(
    _In_ PNVME_ADAPTER_EXTENSION Adapter,
    _In_ PVOID Srb,
    _In_ UCHAR SrbStatus,
    _In_ UCHAR AdditionalSenseCode);

BOOLEAN
NvmpBuildScsi(
    _In_ PNVME_ADAPTER_EXTENSION Adapter,
    _In_ PVOID Srb);

/* stornvme.c */

ULONG
NTAPI
StorNvmeFindAdapter(
    _In_ PVOID DeviceExtension,
    _In_ PVOID HwContext,
    _In_ PVOID BusInformation,
    _In_ PCHAR ArgumentString,
    _Inout_ PPORT_CONFIGURATION_INFORMATION ConfigInfo,
    _In_ PBOOLEAN Again);

BOOLEAN
NTAPI
StorNvmeInitialize(
    _In_ PVOID DeviceExtension);

BOOLEAN
NTAPI
StorNvmeBuildIo(
    _In_ PVOID DeviceExtension,
    _In_ PSCSI_REQUEST_BLOCK Srb);

BOOLEAN
NTAPI
StorNvmeStartIo(
    _In_ PVOID DeviceExtension,
    _In_ PSCSI_REQUEST_BLOCK Srb);

BOOLEAN
NTAPI
StorNvmeInterrupt(
    _In_ PVOID DeviceExtension);

BOOLEAN
NTAPI
StorNvmeMessageInterrupt(
    _In_ PVOID DeviceExtension,
    _In_ ULONG MessageId);

BOOLEAN
NTAPI
StorNvmeResetBus(
    _In_ PVOID DeviceExtension,
    _In_ ULONG PathId);

SCSI_ADAPTER_CONTROL_STATUS
NTAPI
StorNvmeAdapterControl(
    _In_ PVOID DeviceExtension,
    _In_ SCSI_ADAPTER_CONTROL_TYPE ControlType,
    _In_ PVOID Parameters);

SCSI_UNIT_CONTROL_STATUS
NTAPI
StorNvmeUnitControl(
    _In_ PVOID DeviceExtension,
    _In_ SCSI_UNIT_CONTROL_TYPE ControlType,
    _In_ PVOID Parameters);

