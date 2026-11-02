#include "udffs.h"
#include "udf_info/recovery_core.h"

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
        UdfsRecoveryMountFree(&io, plan);
        return STATUS_SUCCESS;
    }
    if (status != UdfsRecoveryProbeValidLayout) {
        UDFPrint(("UDFS recovery: no complete plan %u; read-only policy\n",
                  (ULONG)status));
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
    UdfsRecoveryPhaseBegin(&io, &phase);
    status = UdfsRecoveryApplyRepair(&reader, plan, &after);
    UdfsRecoveryPhaseEnd(&io, &phase, "apply/reprobe", status);
    UdfsRecoveryMountFree(&io, plan);
    if (status != UdfsRecoveryProbeValidLayout) {
        UDFPrint(("UDFS recovery: apply/reprobe failed %u\n", (ULONG)status));
        return status == UdfsRecoveryProbeNoMemory ?
               STATUS_INSUFFICIENT_RESOURCES : STATUS_IO_DEVICE_ERROR;
    }
    UDFPrint(("UDFS recovery: checked bitmap/LVID commit complete\n"));
    return STATUS_SUCCESS;
}
