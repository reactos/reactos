#include "udffs.h"
#include "udf_info/recovery_core.h"
#include <ntstrsafe.h>

NTKERNELAPI BOOLEAN NTAPI InbvCheckDisplayOwnership(VOID);
NTKERNELAPI VOID NTAPI InbvDisplayString(PCSTR String);

/* Show a recovery step on the boot screen as well as in the debug log.
 * It appears when the boot display is in text mode (for example /SOS);
 * once the boot display is handed over it is skipped. */
static VOID
UdfsRecoveryBootMessage(PCSTR Message)
{
    if (KeGetCurrentIrql() == PASSIVE_LEVEL && InbvCheckDisplayOwnership())
        InbvDisplayString(Message);
}

static const char*
UdfsRecoveryStatusText(UDFS_RECOVERY_PROBE_STATUS Status)
{
    switch (Status) {
    case UdfsRecoveryProbeValidLayout:  return "valid";
    case UdfsRecoveryProbeInvalidInput: return "invalid input";
    case UdfsRecoveryProbeIoError:      return "read error";
    case UdfsRecoveryProbeNoMemory:     return "out of memory";
    case UdfsRecoveryProbeCorrupt:      return "damaged structure";
    case UdfsRecoveryProbeGraphMismatch:return "space bitmap does not match the files";
    case UdfsRecoveryProbeLvidMismatch: return "integrity counters do not match";
    case UdfsRecoveryProbeUnsupported:  return "layout not supported by recovery";
    default:                            return "unknown";
    }
}

/* Why recovery stopped, on the boot screen, then a pause so it can be read.
 * Nothing can be read from the keyboard this early in boot, so instead of
 * waiting for a key the message stays up for a few seconds. */
static VOID
UdfsRecoveryBootDetails(const UDFS_RECOVERY_PROBE* Probe,
                        UDFS_RECOVERY_PROBE_STATUS Status, PCSTR Phase)
{
    CHAR line[200];
    LARGE_INTEGER delay;

    if (KeGetCurrentIrql() != PASSIVE_LEVEL || !InbvCheckDisplayOwnership())
        return;
    RtlStringCbPrintfA(line, sizeof(line),
        "UDF: %s stopped: %s (status %u, check line %lu)\r\n",
        Phase, UdfsRecoveryStatusText(Status), (ULONG)Status,
        (ULONG)Probe->FailureLine);
    InbvDisplayString(line);
    RtlStringCbPrintfA(line, sizeof(line),
        "UDF: object %u:%lx, %lu files and %lu folders checked\r\n",
        (ULONG)Probe->FailureObjectRef, (ULONG)Probe->FailureObjectLbn,
        (ULONG)Probe->FilesVisited, (ULONG)Probe->DirectoriesVisited);
    InbvDisplayString(line);
    if (Probe->FailureFeRecorded) {
        RtlStringCbPrintfA(line, sizeof(line),
            "UDF: file entry CRC length %lu, descriptors at %lu, length %lu\r\n",
            (ULONG)Probe->FailureFeCrcLength, (ULONG)Probe->FailureFeAdStart,
            (ULONG)Probe->FailureFeAdLength);
        InbvDisplayString(line);
    }
    if (Probe->FailureFidRecorded) {
        const UCHAR* h = Probe->FailureFidHead;
        RtlStringCbPrintfA(line, sizeof(line),
            "UDF: bad folder entry at %lx of %lx: %02x %02x %02x %02x %02x %02x %02x %02x\r\n",
            (ULONG)Probe->FailureFidOffset, (ULONG)Probe->FailureDirBytes,
            h[0], h[1], h[2], h[3], h[4], h[5], h[6], h[7]);
        InbvDisplayString(line);
    }
    if (Status == UdfsRecoveryProbeGraphMismatch) {
        RtlStringCbPrintfA(line, sizeof(line),
            "UDF: unreferenced %lu/%lu, referenced but free %lu/%lu, shared %lu\r\n",
            (ULONG)Probe->UnownedPhysicalBlocks, (ULONG)Probe->UnownedMetadataBlocks,
            (ULONG)Probe->ReferencedFreePhysicalBlocks,
            (ULONG)Probe->ReferencedFreeMetadataBlocks, (ULONG)Probe->OverlapBlocks);
        InbvDisplayString(line);
    }
    InbvDisplayString("UDF: continuing in 15 seconds...\r\n");
    delay.QuadPart = -15LL * 10 * 1000 * 1000;
    KeDelayExecutionThread(KernelMode, FALSE, &delay);
}

typedef struct _UDFS_RECOVERY_MOUNT_IO {
    PDEVICE_OBJECT Target;
#if defined(UDF_DBG) && defined(USE_KD_PRINT)
    ULONG Reads;
    ULONG Writes;
    ULONG Flushes;
#endif
} UDFS_RECOVERY_MOUNT_IO;

#if defined(UDF_DBG) && defined(USE_KD_PRINT)
typedef struct _UDFS_RECOVERY_PHASE {
    LARGE_INTEGER Start;
    LARGE_INTEGER Frequency;
    ULONG Reads;
    ULONG Writes;
    ULONG Flushes;
} UDFS_RECOVERY_PHASE;

/* One clock pair and one log per phase; no clock or print per sector. */
static void
UdfsRecoveryPhaseBegin(UDFS_RECOVERY_MOUNT_IO *io, UDFS_RECOVERY_PHASE *phase)
{
    phase->Start = KeQueryPerformanceCounter(&phase->Frequency);
    phase->Reads = io->Reads;
    phase->Writes = io->Writes;
    phase->Flushes = io->Flushes;
}

static void
UdfsRecoveryPhaseEnd(UDFS_RECOVERY_MOUNT_IO *io, UDFS_RECOVERY_PHASE *phase,
                     const char *name, UDFS_RECOVERY_PROBE_STATUS status)
{
    LARGE_INTEGER end = KeQueryPerformanceCounter(NULL);
    ULONG milliseconds = 0;
    if (phase->Frequency.QuadPart > 0 && end.QuadPart >= phase->Start.QuadPart)
        milliseconds = (ULONG)((end.QuadPart - phase->Start.QuadPart) * 1000 /
                              phase->Frequency.QuadPart);
    UDFPrint(("UDFS recovery: %s status %u, %lu ms, reads %lu writes %lu flushes %lu\n",
              name, (ULONG)status, milliseconds, io->Reads - phase->Reads,
              io->Writes - phase->Writes, io->Flushes - phase->Flushes));
}

#define UDFS_RECOVERY_COUNT(io, field) (++(io)->field)
#else
#define UdfsRecoveryPhaseBegin(io, phase) ((void)0)
#define UdfsRecoveryPhaseEnd(io, phase, name, status) ((void)0)
#define UDFS_RECOVERY_COUNT(io, field) ((void)0)
#endif

static int
UdfsRecoverySectorIo(void *context, uint32_t lba, void *buffer,
                     BOOLEAN write)
{
    UDFS_RECOVERY_MOUNT_IO *io = (UDFS_RECOVERY_MOUNT_IO *)context;
    LARGE_INTEGER offset;
    IO_STATUS_BLOCK iosb;
    KEVENT event;
    PIRP irp;
    PIO_STACK_LOCATION stack;
    NTSTATUS status;

    if (write) UDFS_RECOVERY_COUNT(io, Writes);
    else UDFS_RECOVERY_COUNT(io, Reads);
    offset.QuadPart = ((LONGLONG)lba) * UDFS_RECOVERY_SECTOR_SIZE;
    iosb.Status = STATUS_UNSUCCESSFUL;
    iosb.Information = 0;
    KeInitializeEvent(&event, NotificationEvent, FALSE);
    irp = IoBuildSynchronousFsdRequest(write ? IRP_MJ_WRITE : IRP_MJ_READ,
                                       io->Target, buffer,
                                       UDFS_RECOVERY_SECTOR_SIZE,
                                       &offset, &event, &iosb);
    if (!irp) return -1;
    stack = IoGetNextIrpStackLocation(irp);
    SetFlag(stack->Flags, SL_OVERRIDE_VERIFY_VOLUME);
    if (write) SetFlag(stack->Flags, SL_WRITE_THROUGH);
    status = IoCallDriver(io->Target, irp);
    if (status == STATUS_PENDING) {
        KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
        status = iosb.Status;
    } else if (NT_SUCCESS(status)) {
        status = iosb.Status;
    }
    return NT_SUCCESS(status) &&
           iosb.Information == UDFS_RECOVERY_SECTOR_SIZE ? 0 : -1;
}

static int
UdfsRecoveryMountReadSector(void *context, uint32_t lba,
                       uint8_t sector[UDFS_RECOVERY_SECTOR_SIZE])
{
    return UdfsRecoverySectorIo(context, lba, sector, FALSE);
}

static int
UdfsRecoveryMountWriteSector(void *context, uint32_t lba,
                        const uint8_t sector[UDFS_RECOVERY_SECTOR_SIZE])
{
    return UdfsRecoverySectorIo(context, lba, (PVOID)sector, TRUE);
}

static int
UdfsRecoveryMountFlush(void *context)
{
    UDFS_RECOVERY_MOUNT_IO *io = (UDFS_RECOVERY_MOUNT_IO *)context;
    IO_STATUS_BLOCK iosb;
    KEVENT event;
    PIRP irp;
    NTSTATUS status;

    UDFS_RECOVERY_COUNT(io, Flushes);
    iosb.Status = STATUS_UNSUCCESSFUL;
    iosb.Information = 0;
    KeInitializeEvent(&event, NotificationEvent, FALSE);
    irp = IoBuildSynchronousFsdRequest(IRP_MJ_FLUSH_BUFFERS, io->Target,
                                       NULL, 0, NULL, &event, &iosb);
    if (!irp) return -1;
    status = IoCallDriver(io->Target, irp);
    if (status == STATUS_PENDING) {
        KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
        status = iosb.Status;
    } else if (NT_SUCCESS(status)) {
        status = iosb.Status;
    }
    /* A lower device that declines FLUSH cannot establish the required
     * bitmap-before-LVID persistence order. */
    return NT_SUCCESS(status) ? 0 : -1;
}

static void *
UdfsRecoveryMountAllocate(void *context, size_t bytes)
{
    UNREFERENCED_PARAMETER(context);
    if (!bytes || bytes > 16 * 1024 * 1024)
        return NULL;
    return ExAllocatePoolWithTag(NonPagedPool, bytes, 'RduU');
}

static void
UdfsRecoveryMountFree(void *context, void *buffer)
{
    UNREFERENCED_PARAMETER(context);
    ExFreePoolWithTag(buffer, 'RduU');
}

NTSTATUS
UDFRecoverDirtyVolumeBeforeLoad(PIRP_CONTEXT IrpContext, PVCB Vcb)
{
    UDFS_RECOVERY_MOUNT_IO io;
    UDFS_RECOVERY_READER reader;
    UDFS_RECOVERY_PROBE before, after;
    UDFS_RECOVERY_REPAIR_PLAN *plan;
    UDFS_RECOVERY_PROBE_STATUS status;
    uint8_t integrityType;
    UDF_TIME_STAMP timestamp;
    LONGLONG now;
#if defined(UDF_DBG) && defined(USE_KD_PRINT)
    UDFS_RECOVERY_PHASE phase;
#endif

    UNREFERENCED_PARAMETER(IrpContext);
    if (Vcb->SectorSize != UDFS_RECOVERY_SECTOR_SIZE) {
        UDFPrint(("UDFS recovery: skipped, sector size %lu\n", (ULONG)Vcb->SectorSize));
        return STATUS_SUCCESS;
    }
    if (Vcb->SessionStartLba != 0) {
        UDFPrint(("UDFS recovery: skipped, session starts at %lx\n", (ULONG)Vcb->SessionStartLba));
        return STATUS_SUCCESS;
    }
    if (!Vcb->TrackMap) {
        UDFPrint(("UDFS recovery: skipped, no track map\n"));
        return STATUS_SUCCESS;
    }
    if (Vcb->TrackMap[Vcb->LastTrackNum].FirstLba != 0) {
        UDFPrint(("UDFS recovery: skipped, last track starts at %lx\n",
                  (ULONG)Vcb->TrackMap[Vcb->LastTrackNum].FirstLba));
        return STATUS_SUCCESS;
    }
    if (Vcb->SessionEndLba == UINT32_MAX) {
        UDFPrint(("UDFS recovery: skipped, sector count exceeds supported range\n"));
        return STATUS_SUCCESS;
    }
    if (Vcb->TargetDeviceObject->DeviceType != FILE_DEVICE_DISK) {
        UDFPrint(("UDFS recovery: skipped, device type %lu\n",
                  (ULONG)Vcb->TargetDeviceObject->DeviceType));
        return STATUS_SUCCESS;
    }

    RtlZeroMemory(&reader, sizeof(reader));
    RtlZeroMemory(&io, sizeof(io));
    io.Target = Vcb->TargetDeviceObject;
    reader.Context = &io;
    reader.ReadSector = UdfsRecoveryMountReadSector;
    reader.WriteSector = UdfsRecoveryMountWriteSector;
    reader.Flush = UdfsRecoveryMountFlush;
    reader.Allocate = UdfsRecoveryMountAllocate;
    reader.Free = UdfsRecoveryMountFree;
    reader.SectorCount = Vcb->SessionEndLba + 1;
    UdfsRecoveryPhaseBegin(&io, &phase);
    status = UdfsRecoveryPeekIntegrity(&reader, &integrityType);
    UdfsRecoveryPhaseEnd(&io, &phase, "peek", status);
    if (status != UdfsRecoveryProbeValidLayout) {
        /* A corrupt integrity chain cannot certify a clean checkpoint.
         * Preserve read access when the ordinary mount accepts the volume,
         * but never let an older CLOSED LVID authorize writes. */
        if (status == UdfsRecoveryProbeCorrupt ||
            status == UdfsRecoveryProbeNoMemory)
            Vcb->VcbState |= VCB_STATE_VOLUME_READ_ONLY;
        UDFPrint(("UDFS recovery: descriptor/integrity probe refused %u; existing mount policy\n",
                  (ULONG)status));
        return STATUS_SUCCESS;
    }
    if (integrityType != 0) {
        UDFPrint(("UDFS recovery: CLOSED checkpoint, no recovery required\n"));
        return STATUS_SUCCESS;
    }
    /* A dirty write-protected device remains available through the existing
     * read-only policy. Do not turn an optional repair into a mount failure. */
    if (UDFPerformDevIoCtrl(IOCTL_DISK_IS_WRITABLE, io.Target,
                            NULL, 0, NULL, 0, TRUE, NULL) ==
        STATUS_MEDIA_WRITE_PROTECTED) {
        UDFPrint(("UDFS recovery: skipped, device is write protected\n"));
        return STATUS_SUCCESS;
    }
    UdfsRecoveryBootMessage("UDF: volume was not shut down cleanly, checking it...\r\n");
    UdfsRecoveryPhaseBegin(&io, &phase);
    status = UdfsRecoveryProbeLayout(&reader, &before);
    UdfsRecoveryPhaseEnd(&io, &phase, "probe", status);
    UDFPrint(("UDFS recovery: anchor %lx VDS %lx, probe %u at %lx\n",
              (ULONG)before.AnchorLba, (ULONG)before.VdsStartLba,
              (ULONG)status, (ULONG)before.FailureLba));
    UDFPrint(("UDFS recovery: files %lu dirs %lu, physical blocks %lu, object %u:%lx, check line %lu\n",
              (ULONG)before.FilesVisited, (ULONG)before.DirectoriesVisited,
              (ULONG)before.PartitionBlocks, (ULONG)before.FailureObjectRef,
              (ULONG)before.FailureObjectLbn, (ULONG)before.FailureLine));
    if (before.FailureFidRecorded)
        UDFPrint(("UDFS recovery: bad entry at %lx of %lx: "
                  "%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
                  (ULONG)before.FailureFidOffset, (ULONG)before.FailureDirBytes,
                  before.FailureFidHead[0], before.FailureFidHead[1], before.FailureFidHead[2], before.FailureFidHead[3],
                  before.FailureFidHead[4], before.FailureFidHead[5], before.FailureFidHead[6], before.FailureFidHead[7],
                  before.FailureFidHead[8], before.FailureFidHead[9], before.FailureFidHead[10], before.FailureFidHead[11],
                  before.FailureFidHead[12], before.FailureFidHead[13], before.FailureFidHead[14], before.FailureFidHead[15]));
    if (status == UdfsRecoveryProbeGraphMismatch)
        UDFPrint(("UDFS recovery: in use but unreferenced %lu/%lu, referenced but free %lu/%lu, "
                  "shared %lu, dir fixes %lu/%lu/%lu (physical/metadata)\n",
                  (ULONG)before.UnownedPhysicalBlocks, (ULONG)before.UnownedMetadataBlocks,
                  (ULONG)before.ReferencedFreePhysicalBlocks,
                  (ULONG)before.ReferencedFreeMetadataBlocks,
                  (ULONG)before.OverlapBlocks, (ULONG)before.FidSplitCount,
                  (ULONG)before.MirrorDirectorySplitCount,
                  (ULONG)before.DirectoryLinkFixCount));
    /* The probe has not written anything yet. Incomplete validation keeps
     * the pre-existing dirty/read-only mount policy. */
    if (status == UdfsRecoveryProbeNoMemory ||
        status == UdfsRecoveryProbeIoError)
        return STATUS_SUCCESS;
    if (status == UdfsRecoveryProbeValidLayout &&
        before.LvidIntegrityType == 1)
        return STATUS_SUCCESS;
    if (before.PrevailingLvidLba == 0 ||
        before.LvidIntegrityType != 0 ||
        (status != UdfsRecoveryProbeValidLayout &&
         status != UdfsRecoveryProbeGraphMismatch &&
         status != UdfsRecoveryProbeLvidMismatch)) {
        UDFPrint(("UDFS recovery: unsupported/corrupt state %u, existing mount policy\n",
                  (ULONG)status));
        if (status == UdfsRecoveryProbeValidLayout) {
            UdfsRecoveryBootMessage("UDF: no problems found.\r\n");
        } else {
            UdfsRecoveryBootMessage("UDF: the volume could not be checked; it is mounted as before.\r\n");
            UdfsRecoveryBootDetails(&before, status, "the check");
        }
        return STATUS_SUCCESS;
    }

    plan = (UDFS_RECOVERY_REPAIR_PLAN *)
           UdfsRecoveryMountAllocate(&io, sizeof(*plan));
    if (!plan) {
        UDFPrint(("UDFS recovery: repair-plan allocation failed\n"));
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    KeQuerySystemTime((PLARGE_INTEGER)&now);
    RtlZeroMemory(&timestamp, sizeof(timestamp));
    UDFTimeToUDF(now, &timestamp);
    UdfsRecoveryPhaseBegin(&io, &phase);
    status = UdfsRecoveryPrepareRepair(&reader, &before,
                                        (const uint8_t *)&timestamp, plan);
    UdfsRecoveryPhaseEnd(&io, &phase, "plan", status);
    if (status == UdfsRecoveryProbeUnsupported) {
        UDFPrint(("UDFS recovery: ownership class not repairable; read-only policy\n"));
        UdfsRecoveryBootMessage("UDF: problems found that cannot be repaired; the volume is read-only.\r\n");
        UdfsRecoveryBootDetails(&before, status, "the repair plan");
        UdfsRecoveryMountFree(&io, plan);
        return STATUS_SUCCESS;
    }
    if (status != UdfsRecoveryProbeValidLayout) {
        UDFPrint(("UDFS recovery: no complete plan %u; read-only policy\n",
                  (ULONG)status));
        UdfsRecoveryBootMessage("UDF: problems found that cannot be repaired; the volume is read-only.\r\n");
        UdfsRecoveryBootDetails(&before, status, "the repair plan");
        UdfsRecoveryMountFree(&io, plan);
        return STATUS_SUCCESS;
    }
    UDFPrint(("UDFS recovery: %lu referenced-free physical, %lu referenced-free metadata, %lu unowned physical blocks; %lu bitmap sectors\n",
              (ULONG)before.ReferencedFreePhysicalBlocks,
              (ULONG)before.ReferencedFreeMetadataBlocks,
              (ULONG)before.UnownedPhysicalBlocks,
              (ULONG)plan->BitmapEditCount));
    if (before.UnownedPhysicalBlocks) {
        UDFPrint(("UDFS recovery: rolling back unlinked allocations after full FE/namespace check; incomplete new file is not recovered\n"));
    } else {
        UDFPrint(("UDFS recovery: metadata-only repair; unflushed file contents are not recovered\n"));
    }
    UdfsRecoveryBootMessage("UDF: repairing the volume...\r\n");
    UdfsRecoveryPhaseBegin(&io, &phase);
    status = UdfsRecoveryApplyRepair(&reader, plan, &after);
    UdfsRecoveryPhaseEnd(&io, &phase, "apply/reprobe", status);
    UdfsRecoveryMountFree(&io, plan);
    if (status != UdfsRecoveryProbeValidLayout) {
        UDFPrint(("UDFS recovery: apply/reprobe failed %u\n", (ULONG)status));
        UdfsRecoveryBootMessage("UDF: the repair failed.\r\n");
        UdfsRecoveryBootDetails(&after, status, "the repair");
        return status == UdfsRecoveryProbeNoMemory ?
               STATUS_INSUFFICIENT_RESOURCES : STATUS_IO_DEVICE_ERROR;
    }
    UDFPrint(("UDFS recovery: checked bitmap/LVID commit complete\n"));
    UdfsRecoveryBootMessage("UDF: the volume was repaired.\r\n");
    return STATUS_SUCCESS;
}
