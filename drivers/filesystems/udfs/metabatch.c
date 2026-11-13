/* Copyright 2026 KosBeg. SPDX-License-Identifier: GPL-2.0-or-later
 * Bounded writeback of new, unpublished ordinary-file generations.
 */
#include "udffs.h"
#include "udf_info/fe_batch.h"

static BOOLEAN
UDFNewMetadataVolume(PVCB Vcb)
{
    return Vcb->VcbCondition == VcbMounted && !Vcb->CDR_Mode &&
        Vcb->SectorSize == 512 &&
        (Vcb->UdfRevision == 0x201 || Vcb->UdfRevision == 0x250) &&
        Vcb->TargetDeviceObject->DeviceType == FILE_DEVICE_DISK &&
        !FlagOn(Vcb->VcbState, VCB_STATE_VOLUME_READ_ONLY | UDF_VCB_FLAGS_DEAD);
}

/* FELoc.Length is the logical FE header, not its rounded allocation size.
 * The serializer separately proves the complete transfer is one sector. */
BOOLEAN
UDFCanStageFileEntry(PVCB Vcb, PUDF_FILE_INFO FileInfo)
{
    PUDF_DATALOC_INFO dloc = FileInfo->Dloc;
    return !Vcb->CDR_Mode && Vcb->SectorSize == 512 &&
        dloc->FELoc.Mapping && dloc->FELoc.Offset == 0 &&
        dloc->FELoc.Length > 0 && dloc->FELoc.Length <= 512 &&
        UDFGetExtentLength(dloc->FELoc.Mapping) == 512 &&
        !(dloc->FELoc.Mapping[0].extLength & ~UDF_EXTENT_LENGTH_MASK);
}

BOOLEAN
UDFMayDeferNewCreate(PIRP_CONTEXT IrpContext, PVCB Vcb)
{
    PIO_STACK_LOCATION irpSp;
    ULONG options;
    if (!UDFNewMetadataVolume(Vcb) || !IrpContext || !IrpContext->Irp ||
        IrpContext->MajorFunction != IRP_MJ_CREATE)
        return FALSE;
    irpSp = IoGetCurrentIrpStackLocation(IrpContext->Irp);
    options = irpSp->Parameters.Create.Options;
    return FlagOn(options, FILE_NON_DIRECTORY_FILE) &&
        !FlagOn(options, FILE_DIRECTORY_FILE | FILE_WRITE_THROUGH | FILE_NO_INTERMEDIATE_BUFFERING |
                         FILE_DELETE_ON_CLOSE | FILE_OPEN_BY_FILE_ID);
}

BOOLEAN
UDFNewFileCacheable(PVCB Vcb, PUDF_FILE_INFO FileInfo)
{
    PFCB fcb, parent;
    PLCB lcb;
    if (!UDFNewMetadataVolume(Vcb) || !FileInfo || !FileInfo->Dloc ||
        !FlagOn(FileInfo->Dloc->FE_Flags, UDF_FE_FLAG_NEW_UNPUBLISHED) ||
        !FileInfo->Dloc->FileEntry || !FileInfo->Dloc->FELoc.Mapping ||
        !(fcb = FileInfo->Fcb) || fcb->FileInfo != FileInfo ||
        FileInfo->Dloc->CommonFcb != fcb || fcb->FcbReference <= 0 ||
        !FlagOn(fcb->FcbState, FCB_STATE_IN_FCB_TABLE) ||
        FlagOn(fcb->FcbState, UDF_FCB_DIRECTORY | UDF_FCB_DELETE_ON_CLOSE | UDF_FCB_DELETED) ||
        UDFIsADirectory(FileInfo) || UDFIsAStream(FileInfo) ||
        UDFIsAStreamDir(FileInfo) || UDFHasAStreamDir(FileInfo) ||
        UDFGetFileNameLinkCount(FileInfo) != 1 ||
        (FileInfo->NextLinkedFile && FileInfo->NextLinkedFile != FileInfo) ||
        (FileInfo->PrevLinkedFile && FileInfo->PrevLinkedFile != FileInfo))
        return FALSE;
    if (!FileInfo->ParentFile || !(parent = FileInfo->ParentFile->Fcb) ||
        parent->FcbReference <= 0 || parent->FileInfo != FileInfo->ParentFile ||
        fcb->ParentFcb != parent || IsListEmpty(&fcb->ParentLcbQueue) ||
        fcb->ParentLcbQueue.Flink->Flink != &fcb->ParentLcbQueue)
        return FALSE;
    lcb = CONTAINING_RECORD(fcb->ParentLcbQueue.Flink, struct LCB, ChildFcbLinks);
    return lcb->ParentFcb == parent && lcb->ChildFcb == fcb;
}

BOOLEAN
UDFDeferNewFileMetadata(PIRP_CONTEXT IrpContext, PVCB Vcb, PUDF_FILE_INFO FileInfo)
{
    PIO_STACK_LOCATION irpSp;
#ifdef UDF_METADATA_BATCH_DIAGNOSTICS
    if (IrpContext && IrpContext->MajorFunction == IRP_MJ_CLEANUP) {
        UDF_METADATA_COUNT(Vcb,19);
        if (FileInfo && FileInfo->Dloc && (FileInfo->Dloc->FE_Flags & UDF_FE_FLAG_NEW_UNPUBLISHED))
            UDF_METADATA_COUNT(Vcb,20);
        if (UDFNewFileCacheable(Vcb, FileInfo)) UDF_METADATA_COUNT(Vcb,21);
    }
#endif
    if (!UDFNewFileCacheable(Vcb, FileInfo) || !IrpContext || !IrpContext->Irp ||
        FlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_WRITE_THROUGH))
        return FALSE;
    irpSp = IoGetCurrentIrpStackLocation(IrpContext->Irp);
    if (!irpSp->FileObject || FlagOn(irpSp->FileObject->Flags,
                                   FO_WRITE_THROUGH | FO_NO_INTERMEDIATE_BUFFERING))
        return FALSE;
    if (IrpContext->MajorFunction == IRP_MJ_CLEANUP) {
        UDF_METADATA_COUNT(Vcb,25);
        return FALSE;
    }
    if (IrpContext->MajorFunction == IRP_MJ_WRITE) {
        /* A zero-byte allocation update changes no payload. For real data
         * accept only the cached paging path, with a resident authoritative
         * in-ICB buffer; WT stack requests always reach the lower device. */
        return !FlagOn(irpSp->Flags, SL_WRITE_THROUGH) &&
            !(FlagOn(IrpContext->Irp->Flags, IRP_NOCACHE) &&
              !FlagOn(IrpContext->Irp->Flags, IRP_PAGING_IO));
    }
    return FALSE;
}

/* A cached FILE_OBJECT can outlive cleanup. Its existing Cache Manager
 * reference and the caller's FCB/FI lifetime hold retain the payload; no new
 * private data queue or duplicate handle reference is created here. Drain
 * before any FE/FID publication, under one Main resource and no Paging lock.
 * A volume cohort calls this BEFORE acquiring all cohort Main/Paging locks.
 * Success alone retires the pending generation; failure retains the bit. */
NTSTATUS
UDFDrainNewFileData(PIRP_CONTEXT IrpContext, PVCB Vcb,
                    PUDF_FILE_INFO FileInfo)
{
    PUDF_DATALOC_INFO dloc;
    PFCB fcb;
    IO_STATUS_BLOCK io;
    NTSTATUS status = STATUS_SUCCESS;
    BOOLEAN acquired = FALSE;
    if (!FileInfo || !(dloc = FileInfo->Dloc) ||
        !(dloc->FE_Flags & UDF_FE_FLAG_CACHED_DATA_PENDING))
        return STATUS_SUCCESS;
    fcb = dloc->CommonFcb;
    if (!fcb || !fcb->FileInfo || fcb->FileInfo->Dloc != dloc ||
        !fcb->FcbNonpaged)
        return STATUS_FILE_CORRUPT_ERROR;
    /* STATUS_CANT_WAIT is the dispatcher's internal retry token. These
     * are terminal guard failures: callers complete/free their IRP context,
     * so returning that token would retry a freed context. Retain the data
     * owner and report an ordinary failure instead.
     * Never recurse into Cc from paging I/O or invert Main -> Paging.
     * Keep the generation on an unexpected caller instead of deadlocking. */
    if ((IrpContext->Irp && FlagOn(IrpContext->Irp->Flags, IRP_PAGING_IO)) ||
        ExIsResourceAcquiredSharedLite(&fcb->FcbNonpaged->FcbPagingIoResource))
        return STATUS_INVALID_DEVICE_STATE;
    if (!ExIsResourceAcquiredExclusiveLite(&fcb->FcbNonpaged->FcbResource)) {
        if (ExIsResourceAcquiredSharedLite(&fcb->FcbNonpaged->FcbResource))
            return STATUS_INVALID_DEVICE_STATE;
        UDFAcquireFcbExclusive(IrpContext, fcb, FALSE);
        acquired = TRUE;
    }
    _SEH2_TRY {
        if (dloc->FE_Flags & UDF_FE_FLAG_CACHED_DATA_PENDING) {
            io.Status = STATUS_SUCCESS;
            io.Information = 0;
            if (fcb->FcbNonpaged->SegmentObject.DataSectionObject) {
                UDF_METADATA_COUNT(Vcb,48);
                CcFlushCache(&fcb->FcbNonpaged->SegmentObject, NULL, 0, &io);
            }
            status = io.Status;
            if (NT_SUCCESS(status)) {
                dloc->FE_Flags &= ~UDF_FE_FLAG_CACHED_DATA_PENDING;
                UDF_METADATA_COUNT(Vcb,49);
            } else {
                UDF_METADATA_COUNT(Vcb,50);
            }
        }
    } _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER) {
        status = _SEH2_GetExceptionCode();
        UDF_METADATA_COUNT(Vcb,50);
    } _SEH2_END;
    if (acquired)
        UDFReleaseFcb(IrpContext, fcb);
    return status;
}

/* Full-volume data flush already pins each canonical FCB before this phase.
 * Overlap only those independent data drains; retain the original Cc path,
 * exact lower I/O completion and data-before-FE-before-FID barrier. The
 * caller owns VCB exclusive and NO ordinary-file Main/Paging resource. */
#define UDF_DATA_DRAIN_HELPERS 2

typedef struct _UDF_PARALLEL_DATA_DRAIN {
    PVCB Vcb;
    PDEVICE_OBJECT RealDevice;
    PFCB* Files;
    LONG Count;
    volatile LONG Next;
    volatile LONG Visited;
    volatile LONG FirstError;
    HANDLE Thread[UDF_DATA_DRAIN_HELPERS];
    uint32 Threads;
} UDF_PARALLEL_DATA_DRAIN;

static BOOLEAN
UDFParallelDataOwner(PVCB Vcb, PFCB Fcb)
{
    PUDF_DATALOC_INFO dloc;
    PEXTENT_MAP map;
    uint32 i;
    if (!Fcb || !Fcb->FcbNonpaged || !UDFNewFileCacheable(Vcb, Fcb->FileInfo) ||
        !(dloc = Fcb->FileInfo->Dloc) ||
        !FlagOn(dloc->FE_Flags, UDF_FE_FLAG_CACHED_DATA_PENDING) ||
        dloc->DataLoc.Offset || !dloc->DataLoc.Mapping ||
        FlagOn(Fcb->FcbState, UDF_FCB_EMBEDDED_DATA) ||
        Fcb->Header.FileSize.QuadPart <= 0 ||
        Fcb->Header.ValidDataLength.QuadPart < Fcb->Header.FileSize.QuadPart ||
        Fcb->Header.PagingIoResource != &Fcb->FcbNonpaged->FcbPagingIoResource ||
        !Fcb->FcbNonpaged->SegmentObject.DataSectionObject ||
        Fcb->FcbNonpaged->SegmentObject.ImageSectionObject)
        return FALSE;
    /* No allocation/namespace/parent work may be introduced by a helper.
     * Already-allocated recording conversion remains protected by this
     * file's original Main/Paging resources in the normal paging path. */
    for (i = 0, map = dloc->DataLoc.Mapping;
         map[i].extLength & UDF_EXTENT_LENGTH_MASK; i++) {
        if (!map[i].extLocation ||
            (map[i].extLength >> 30) == EXTENT_NOT_RECORDED_NOT_ALLOCATED ||
            (map[i].extLength >> 30) == EXTENT_NEXT_EXTENT_ALLOCDESC)
            return FALSE;
    }
    return i != 0;
}

static VOID
UDFRunParallelDataDrain(UDF_PARALLEL_DATA_DRAIN* Drain,
                       PIRP_CONTEXT IrpContext)
{
    while (NT_SUCCESS((NTSTATUS)InterlockedCompareExchange(
                &Drain->FirstError, STATUS_SUCCESS, STATUS_SUCCESS))) {
        LONG index = InterlockedIncrement(&Drain->Next) - 1;
        PFCB fcb;
        NTSTATUS status = STATUS_SUCCESS;
        if (index >= Drain->Count)
            break;
        fcb = Drain->Files[index];
        UDFAcquireFcbExclusive(IrpContext, fcb, FALSE);
        _SEH2_TRY {
            if (UDFParallelDataOwner(Drain->Vcb, fcb))
                status = UDFDrainNewFileData(IrpContext, Drain->Vcb, fcb->FileInfo);
        } _SEH2_FINALLY {
            UDFReleaseFcb(IrpContext, fcb);
        } _SEH2_END;
        InterlockedIncrement(&Drain->Visited);
        if (!NT_SUCCESS(status))
            InterlockedCompareExchange(&Drain->FirstError, status, STATUS_SUCCESS);
    }
}

static VOID NTAPI
UDFParallelDataDrainThread(PVOID Parameter)
{
    UDF_PARALLEL_DATA_DRAIN* drain = Parameter;
    struct IRP_CONTEXT context;
    struct THREAD_CONTEXT threadContext = {0};
    RtlZeroMemory(&context, sizeof(context));
    context.NodeIdentifier.NodeTypeCode = UDF_NODE_TYPE_IRP_CONTEXT;
    context.NodeIdentifier.NodeByteSize = sizeof(context);
    context.Flags = IRP_CONTEXT_FLAG_ON_STACK | IRP_CONTEXT_FSP_FLAGS |
                    IRP_CONTEXT_FLAG_METADATA_WRITER;
    context.MajorFunction = IRP_MJ_FLUSH_BUFFERS;
    context.Vcb = drain->Vcb;
    context.RealDevice = drain->RealDevice;
    FsRtlEnterFileSystem();
    UDFSetThreadContext(&context, &threadContext);
    _SEH2_TRY {
        UDFRunParallelDataDrain(drain, &context);
    } _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER) {
        InterlockedCompareExchange(&drain->FirstError,
                                  _SEH2_GetExceptionCode(), STATUS_SUCCESS);
    } _SEH2_END;
    UDFRestoreThreadContext(&context);
    FsRtlExitFileSystem();
    PsTerminateSystemThread(STATUS_SUCCESS);
}

static NTSTATUS
UDFDrainVolumeDataParallel(PIRP_CONTEXT IrpContext, PVCB Vcb,
                          PFCB* Files, uint32 Count)
{
    UDF_PARALLEL_DATA_DRAIN* drain;
    PFCB volumeFcb;
    PCCB ccb;
    PFILE_OBJECT file;
    OBJECT_ATTRIBUTES attributes;
    NTSTATUS status;
    uint32 i;
    ASSERT_EXCLUSIVE_VCB(Vcb);
    /* First scope is a real explicit volume FlushFileBuffers. Shutdown,
     * lock/dismount, close workers and other request contexts stay serial. */
    if (KeGetCurrentIrql() != PASSIVE_LEVEL ||
        Count < UDF_FE_BATCH_CAPACITY || Count > 0x7ffffffcu ||
        !IrpContext->Irp || IrpContext->MajorFunction != IRP_MJ_FLUSH_BUFFERS ||
        FlagOn(IrpContext->Irp->Flags, IRP_PAGING_IO) || Vcb->SparingTable ||
        FlagOn(Vcb->TargetDeviceObject->Flags, DO_VERIFY_VOLUME))
        return STATUS_SUCCESS;
    file = IoGetCurrentIrpStackLocation(IrpContext->Irp)->FileObject;
    if (!file || UDFDecodeFileObject(file, &volumeFcb, &ccb) != UserVolumeOpen ||
        volumeFcb != Vcb->VolumeDasdFcb)
        return STATUS_SUCCESS;
    drain = MyAllocatePool__(NonPagedPool, sizeof(*drain));
    if (!drain)
        return STATUS_SUCCESS; // Original per-cohort serial drains remain.
    RtlZeroMemory(drain, sizeof(*drain));
    drain->Vcb = Vcb;
    drain->RealDevice = IrpContext->RealDevice;
    drain->Files = Files;
    drain->Count = (LONG)Count;
    status = UDFOpenIntegrityForWrite(IrpContext, Vcb, FALSE);
    if (!NT_SUCCESS(status)) {
        MyFreePool__(drain);
        return status;
    }
    InitializeObjectAttributes(&attributes, NULL, OBJ_KERNEL_HANDLE, NULL, NULL);
    _SEH2_TRY {
        for (i = 0; i < UDF_DATA_DRAIN_HELPERS; i++) {
            HANDLE handle;
            status = PsCreateSystemThread(&handle, SYNCHRONIZE, &attributes,
                NULL, NULL, UDFParallelDataDrainThread, drain);
            if (!NT_SUCCESS(status))
                break; // Fewer helpers, or unchanged serial fallback.
            drain->Thread[drain->Threads++] = handle;
        }
        if (drain->Threads)
            UDFRunParallelDataDrain(drain, IrpContext);
    } _SEH2_FINALLY {
        /* Every private context, snapshot pin and shared job byte remains
         * alive until ALL created threads terminate, including partial
         * creation failure, data error or an exception in the caller. */
        for (i = 0; i < drain->Threads; i++) {
            /* Private kernel thread handle with SYNCHRONIZE access; a
             * nonalertable kernel wait without timeout ends at termination. */
            NTSTATUS waitStatus = ZwWaitForSingleObject(drain->Thread[i], FALSE, NULL);
            ASSERT(waitStatus == STATUS_SUCCESS);
            UNREFERENCED_PARAMETER(waitStatus);
            ZwClose(drain->Thread[i]);
        }
        status = (NTSTATUS)InterlockedCompareExchange(&drain->FirstError,
                                    STATUS_SUCCESS, STATUS_SUCCESS);
        ASSERT(!NT_SUCCESS(status) || !drain->Threads || drain->Visited == drain->Count);
        MyFreePool__(drain);
    } _SEH2_END;
    return status;
}

NTSTATUS
UDFReadNewFileData(PVCB Vcb, PUDF_FILE_INFO FileInfo,
    int64 Offset, SIZE_T Length, int8* Buffer, PBOOLEAN Handled)
{
    PUDF_DATALOC_INFO dloc = FileInfo->Dloc;
    *Handled = FALSE;
    if (!FlagOn(dloc->FE_Flags, UDF_FE_FLAG_NEW_UNPUBLISHED) ||
        (((PFILE_ENTRY)dloc->FileEntry)->icbTag.flags & ICB_FLAG_ALLOC_MASK) != ICB_FLAG_AD_IN_ICB ||
        dloc->DataLoc.Offset != dloc->FileEntryLen)
        return STATUS_SUCCESS;
    *Handled = TRUE;
    if (Offset < 0 || (uint64)Offset > (uint64)dloc->DataLoc.Length ||
        Length > (uint64)dloc->DataLoc.Length - (uint64)Offset ||
        (uint64)dloc->FileEntryLen + (uint64)dloc->DataLoc.Length > Vcb->SectorSize)
        return STATUS_FILE_CORRUPT_ERROR;
    if (Length)
        RtlCopyMemory(Buffer, (int8*)dloc->FileEntry + dloc->FileEntryLen + (SIZE_T)Offset, Length);
    return STATUS_SUCCESS;
}

NTSTATUS
UDFTakeMetadataWritebackError(PVCB Vcb)
{
    return (NTSTATUS)InterlockedExchange(&Vcb->MetadataWritebackError, STATUS_SUCCESS);
}

VOID
UDFRememberMetadataWritebackError(PIRP_CONTEXT IrpContext, PVCB Vcb,
    PUDF_FILE_INFO FileInfo, NTSTATUS Status)
{
    UNREFERENCED_PARAMETER(FileInfo);
    if (!NT_SUCCESS(Status) && IrpContext &&
        FlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_METADATA_WRITER))
        InterlockedCompareExchange(&Vcb->MetadataWritebackError, Status, STATUS_SUCCESS);
}

typedef struct _UDF_FE_OWNER {
    PFCB Fcb;
    PUDF_FILE_INFO FileInfo;
    PUDF_DATALOC_INFO Dloc;
    uint32 Lba;
    uint32 Partition;
    BOOLEAN Staged;
    BOOLEAN MainHeld;
    BOOLEAN PagingHeld;
    int8 Bytes[512];
} UDF_FE_OWNER;

/* Only pointer slots are snapshotted; the per-IO owner/buffer and resource
 * cohort stays bounded to64. Every slot owns one FCB reference, transferred
 * exactly once into a cohort or released by the outer unwind. */
#define UDF_FE_SNAPSHOT_BUDGET (4 * 1024 * 1024)
typedef struct _UDF_FE_VOLUME_SNAPSHOT {
    uint32 Count;
    uint32 Next;
    PFCB* Files;
} UDF_FE_VOLUME_SNAPSHOT;

typedef struct _UDF_FE_WRITEBACK {
    PIRP_CONTEXT IrpContext;
    PVCB Vcb;
    uint32 Count;
    uint32 StagedCount;
    UDF_FE_OWNER Owner[UDF_FE_BATCH_CAPACITY];
    UDF_FE_BATCH_WRITE Write[UDF_FE_BATCH_CAPACITY];
    uint32 Order[UDF_FE_BATCH_CAPACITY];
    int8 Transfer[UDF_FE_BATCH_CAPACITY * UDF_FE_BATCH_SECTOR];
} UDF_FE_WRITEBACK;

static NTSTATUS
UDFBatchSectorWrite(void* Context, uint32 Partition, uint32 Lba,
    uint32 Sectors, int8* Buffer, PSIZE_T WrittenBytes)
{
    UDF_FE_WRITEBACK* batch = Context;
    if (Partition >= batch->Vcb->PartitionMaps)
        return STATUS_FILE_CORRUPT_ERROR;
#ifdef UDF_METADATA_BATCH_DIAGNOSTICS
    InterlockedIncrement(&batch->Vcb->MetadataBatchDiag[2]);
    InterlockedExchangeAdd(&batch->Vcb->MetadataBatchDiag[3], Sectors);
#endif
    /* Same translated primary/mirror lower path as an aligned FE extent.
     * All FEs are independent; no FID is part of this transfer. */
    return UDFWriteSectors(batch->IrpContext, batch->Vcb, TRUE, Lba,
                          Sectors, FALSE, Buffer, WrittenBytes);
}

static BOOLEAN
UDFNewEntryDirty(PUDF_FILE_INFO FileInfo)
{
    PUDF_DATALOC_INFO d = FileInfo->Dloc;
    return (d->FE_Flags & UDF_FE_FLAG_FE_MODIFIED) || d->FELoc.Modified ||
        d->DataLoc.Modified || d->AllocLoc.Modified;
}

static VOID
UDFHoldBatchOwner(PIRP_CONTEXT IrpContext, PVCB Vcb,
    UDF_FE_WRITEBACK* Batch, PFCB Fcb)
{
    uint32 i;
    if (!Fcb || Batch->Count == UDF_FE_BATCH_CAPACITY || Fcb->FcbReference != 1 ||
        Fcb->FcbCleanup != 0 || !UDFNewFileCacheable(Vcb, Fcb->FileInfo) ||
        !UDFNewEntryDirty(Fcb->FileInfo))
        return;
    for (i = 0; i < Batch->Count; i++)
        if (Batch->Owner[i].Fcb == Fcb)
            return;
    /* Caller holds FcbTableMutex and VcbMutex, while VCB exclusive blocks
     * queued close workers and new namespace operations. */
    Fcb->FcbReference++;
    Batch->Owner[Batch->Count++].Fcb = Fcb;
    UNREFERENCED_PARAMETER(IrpContext);
}

/* The caller holds VCB exclusive, FcbTableMutex, VcbMutex and UdfDataMutex.
 * DelayedCloseQueue owns these FCB lifetimes until a close worker removes
 * the entry; that worker releases UdfDataMutex before consuming its hold.
 * Inspect only pending closes, retaining the old signed FileId ordering
 * and exact table membership. No reference or queue state changes here. */
static uint32
UDFSelectQueuedMetadataOwners(PIRP_CONTEXT IrpContext, PVCB Vcb,
    PFCB CurrentFcb, UDF_FE_OWNER* Candidates, uint32 Limit)
{
    PLIST_ENTRY entry;
    uint32 count = 0;
    if (!Limit)
        return 0;
    for (entry = UdfData.DelayedCloseQueue.Flink;
         entry != &UdfData.DelayedCloseQueue; entry = entry->Flink) {
        PIRP_CONTEXT_LITE close = CONTAINING_RECORD(entry,
            struct IRP_CONTEXT_LITE, DelayedCloseLinks);
        PFCB fcb = close->Fcb;
        uint32 position, move;
        if (!fcb || fcb == CurrentFcb || fcb->Vcb != Vcb)
            continue;
        UDF_METADATA_COUNT(Vcb,7);
        if (!InterlockedCompareExchange(&fcb->MetadataCloseQueued,0,0) ||
            fcb->FcbReference != 1 || fcb->FcbCleanup ||
            !UDFNewFileCacheable(Vcb,fcb->FileInfo) ||
            !UDFNewEntryDirty(fcb->FileInfo) ||
            UDFLookupFcbTable(IrpContext,Vcb,fcb->FileId) != fcb)
            continue;
        position = 0;
        while (position < count &&
               Candidates[position].Fcb->FileId.QuadPart < fcb->FileId.QuadPart)
            position++;
        /* One registered FCB per FileId. Be conservative if a queue entry
         * is repeated: the reference table walk could only select it once. */
        if ((position < count &&
             Candidates[position].Fcb->FileId.QuadPart == fcb->FileId.QuadPart) ||
            position >= Limit)
            continue;
        if (count < Limit)
            count++;
        for (move = count - 1; move > position; move--)
            Candidates[move].Fcb = Candidates[move-1].Fcb;
        Candidates[position].Fcb = fcb;
    }
    return count;
}

static NTSTATUS
UDFReleaseMetadataOwner(PIRP_CONTEXT IrpContext, PVCB Vcb, PFCB Fcb)
{
    BOOLEAN removed = FALSE;
    NTSTATUS status = STATUS_SUCCESS;
    UDFAcquireFcbExclusive(IrpContext, Fcb, FALSE);
    UDFLockVcb(IrpContext, Vcb);
    ASSERT(Fcb->FcbReference > 0);
    Fcb->FcbReference--;
    UDFUnlockVcb(IrpContext, Vcb);
    if (!Fcb->FcbReference) {
        BOOLEAN writer = FlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_METADATA_WRITER) != 0;
        /* Teardown is void, but its final FE/FID flush can fail. Route that
         * error through the writeback latch even in a user volume flush;
         * otherwise a later successful tree retry could hide the failure. */
        SetFlag(IrpContext->Flags, IRP_CONTEXT_FLAG_METADATA_WRITER);
        _SEH2_TRY {
            UDFTeardownStructures(IrpContext, Fcb, FALSE, &removed);
            status = writer ? (NTSTATUS)InterlockedCompareExchange(
                &Vcb->MetadataWritebackError,STATUS_SUCCESS,STATUS_SUCCESS) :
                UDFTakeMetadataWritebackError(Vcb);
        } _SEH2_FINALLY {
            if (!writer)
                ClearFlag(IrpContext->Flags, IRP_CONTEXT_FLAG_METADATA_WRITER);
        } _SEH2_END;
    }
    if (!removed)
        UDFReleaseFcb(IrpContext, Fcb);
    return status;
}


/* A volume cohort already owns all of these regular-file Main/Paging
 * resources and has committed their complete FEs. Publish only contiguous
 * in-range FIDs under their canonical directory Main resource. No cache or
 * reference survives this call; unsupported entries remain for tree flush. */
#define UDF_FID_WINDOW_BYTES 16384

typedef struct _UDF_FID_WINDOW_ITEM {
    uint32 Owner;
    uint32 Offset;
    uint32 Length;
    PDIR_INDEX_ITEM Index;
} UDF_FID_WINDOW_ITEM;

typedef struct _UDF_FID_PUBLISH_WINDOW {
    uint32 Order[UDF_FE_BATCH_CAPACITY];
    UDF_FID_WINDOW_ITEM Item[UDF_FE_BATCH_CAPACITY];
    int8 Bytes[UDF_FID_WINDOW_BYTES];
} UDF_FID_PUBLISH_WINDOW;

static BOOLEAN
UDFNewFidRecorded(PVCB Vcb, PUDF_FILE_INFO Directory, int64 Offset, SIZE_T Length)
{
    SIZE_T available;
    uint32 flags, lba;
    if (Offset < 0 || Directory->Dloc->DataLoc.Offset ||
        Directory->Dloc->DataLoc.Length < Offset ||
        Length > (uint64)(Directory->Dloc->DataLoc.Length - Offset))
        return FALSE;
    while (Length) {
        lba = UDFExtentOffsetToLba(Vcb, Directory->Dloc->DataLoc.Mapping,
                                  Offset, NULL, &available, &flags, NULL);
        if (lba == LBA_OUT_OF_EXTENT || !available ||
            flags != EXTENT_RECORDED_ALLOCATED)
            return FALSE;
        available = min(available, Length);
        Offset += available;
        Length -= available;
    }
    return TRUE;
}

static PDIR_INDEX_ITEM
UDFNewFidReady(PVCB Vcb, UDF_FE_OWNER* Owner, PUDF_FILE_INFO Directory)
{
    PUDF_FILE_INFO fi = Owner->FileInfo;
    PDIR_INDEX_HDR header;
    PDIR_INDEX_ITEM index;
    uint32 frame, count;
    if (!Owner->Staged || !Owner->MainHeld || !Owner->PagingHeld ||
        Owner->Fcb->FileInfo != fi || !UDFNewFileCacheable(Vcb, fi) ||
        fi->Dloc != Owner->Dloc || fi->ParentFile != Directory ||
        Owner->Partition >= Vcb->PartitionMaps ||
        Owner->Dloc->FELoc.Mapping[0].extLocation != Owner->Lba ||
        UDFNewEntryDirty(fi) ||
        RtlCompareMemory(Owner->Bytes, Owner->Dloc->FileEntry, 512) != 512 ||
        !fi->FileIdent || fi->FileIdentLen < sizeof(FILE_IDENT_DESC) ||
        fi->FileIdentLen > UDF_FID_WINDOW_BYTES ||
        fi->FileIdent->descTag.tagIdent != TID_FILE_IDENT_DESC ||
        (uint32)sizeof(FILE_IDENT_DESC) + fi->FileIdent->lengthOfImpUse +
            fi->FileIdent->lengthFileIdent > fi->FileIdentLen ||
        fi->FileIdent->icb.extLocation.partitionReferenceNum != Owner->Partition ||
        fi->FileIdent->icb.extLocation.logicalBlockNum !=
            UDFPhysLbaToPart(Vcb,Owner->Partition,Owner->Lba))
        return NULL;
    header = Directory->Dloc->DirIndex;
    frame = fi->Index >> UDF_DIR_INDEX_FRAME_SH;
    if (!header || frame >= header->FrameCount)
        return NULL;
    count = frame + 1 == header->FrameCount ?
        header->LastFrameCount : UDF_DIR_INDEX_FRAME;
    if ((fi->Index & (UDF_DIR_INDEX_FRAME - 1)) >= count)
        return NULL;
    if (!((PDIR_INDEX_ITEM*)(header + 1))[frame])
        return NULL;
    index = UDFDirIndex(header, fi->Index);
    if (!index || index->FileInfo != fi || index->Length != fi->FileIdentLen ||
        !FlagOn(index->FI_Flags,UDF_FI_FLAG_FI_MODIFIED) ||
        FlagOn(index->FileCharacteristics,FILE_DELETED | FILE_PARENT) ||
        index->FileEntryLoc.logicalBlockNum != fi->FileIdent->icb.extLocation.logicalBlockNum ||
        index->FileEntryLoc.partitionReferenceNum != fi->FileIdent->icb.extLocation.partitionReferenceNum ||
        !UDFNewFidRecorded(Vcb,Directory,index->Offset,index->Length))
        return NULL;
    return index;
}

static NTSTATUS
UDFPublishNewVolumeIdentifiers(UDF_FE_WRITEBACK* Batch)
{
    PVCB vcb = Batch->Vcb;
    PIRP_CONTEXT context = Batch->IrpContext;
    UDF_FID_PUBLISH_WINDOW* window;
    PFCB parent = NULL;
    uint32 count = 0, i, j, first, limit;
    NTSTATUS status = STATUS_SUCCESS;
    BOOLEAN parentHeld = FALSE;

    ASSERT_EXCLUSIVE_VCB(vcb);
    if (vcb->SparingTable)
        return STATUS_SUCCESS;
    window = MyAllocatePool__(NonPagedPool,sizeof(*window));
    if (!window)
        return STATUS_SUCCESS; // Mandatory tree flush uses scalar FlushFI.
    _SEH2_TRY {
        /* Group by canonical parent first. Offset/index inspection below
         * happens only after that directory resource has been acquired. */
        for (i = 0; i < Batch->Count; i++) {
            UDF_FE_OWNER* owner = &Batch->Owner[i];
            if (owner->Staged && owner->MainHeld && owner->PagingHeld &&
                owner->Fcb->FileInfo == owner->FileInfo &&
                UDFNewFileCacheable(vcb,owner->FileInfo) &&
                FlagOn(owner->FileInfo->ParentFile->Fcb->FcbState,
                       UDF_FCB_DIRECTORY | UDF_FCB_ROOT_DIRECTORY)) {
                uintptr_t key = (uintptr_t)owner->FileInfo->ParentFile->Fcb;
                for (j = count; j && (uintptr_t)Batch->Owner[
                        window->Order[j-1]].FileInfo->ParentFile->Fcb > key; j--)
                    window->Order[j] = window->Order[j-1];
                window->Order[j] = i;
                count++;
            }
        }
        first = 0;
        while (first < count && NT_SUCCESS(status)) {
            PUDF_FILE_INFO directory;
            uint32 entries = 0, partition;
            BOOLEAN tagged = FALSE;
            parent = Batch->Owner[window->Order[first]].FileInfo->ParentFile->Fcb;
            for (limit = first + 1; limit < count &&
                    Batch->Owner[window->Order[limit]].FileInfo->ParentFile->Fcb == parent; limit++);
            UDFAcquireFcbExclusive(context,parent,FALSE);
            parentHeld = TRUE;
            directory = parent->FileInfo;
            if (directory && directory->Fcb == parent && directory->Dloc &&
                directory->Dloc->CommonFcb == parent && directory->Dloc->DirIndex &&
                directory->Dloc->FELoc.Mapping && !directory->Dloc->DataLoc.Offset &&
                !parent->FcbCleanup && UDFIsADirectory(directory) &&
                !UDFIsAStreamDir(directory) && !UDFHasAStreamDir(directory) &&
                !FlagOn(parent->FcbState,UDF_FCB_DELETED | UDF_FCB_DELETE_ON_CLOSE)) {
                partition = UDFGetRefPartNumByPhysLba(vcb,
                    directory->Dloc->FELoc.Mapping[0].extLocation);
                if (partition < vcb->PartitionMaps) {
                    /* All cohort FEs already committed. Keep pending,
                     * reference-free reservations owned until they can be
                     * replaced by these valid FIDs in the same buffer.
                     * UDFReTagDirectory drains itself if storage mode changed;
                     * parent FE publication still drains unconditionally. */
                    status = STATUS_SUCCESS;
                    if (NT_SUCCESS(status)) {
                        for (i = first; i < limit; i++) {
                            uint32 owner = window->Order[i];
                            PDIR_INDEX_ITEM index = UDFNewFidReady(vcb,&Batch->Owner[owner],directory);
                            if (!index)
                                continue;
                            for (j = entries; j && window->Item[j-1].Offset > index->Offset; j--)
                                window->Item[j] = window->Item[j-1];
                            window->Item[j].Owner = owner;
                            window->Item[j].Index = index;
                            window->Item[j].Offset = index->Offset;
                            window->Item[j].Length = index->Length;
                            entries++;
                        }
                    }
                    for (i = 0; i < entries && NT_SUCCESS(status); ) {
                        uint32 end = i + 1, length = window->Item[i].Length;
                        SIZE_T written = 0;
                        PEXTENT_MAP mapping;
                        int64 logicalLength;
                        BOOLEAN previous = FlagOn(context->Flags,IRP_CONTEXT_FLAG_METADATA_PUBLISH) != 0;
                        while (end < entries && window->Item[end].Offset ==
                                (uint64)window->Item[i].Offset + length &&
                                window->Item[end].Length <= UDF_FID_WINDOW_BYTES - length) {
                            length += window->Item[end++].Length;
                        }
                        if (end == i + 1 ||
                            window->Item[i].Length > UDF_FID_WINDOW_BYTES) {
                            i = end;
                            continue;
                        }
                        /* Finish any required in-ICB -> external retag before
                         * preparing final FID bytes, so it cannot immediately
                         * re-dirty the just-published generations afterward. */
                        if (!tagged) {
                            status = UDFReTagDirectory(context,vcb,directory);
                            if (!NT_SUCCESS(status))
                                break;
                            tagged = TRUE;
                        }
                        length = 0;
                        for (j = i; j < end; j++) {
                            UDF_FID_WINDOW_ITEM* item = &window->Item[j];
                            UDF_FE_OWNER* owner = &Batch->Owner[item->Owner];
                            PFILE_IDENT_DESC fid = (PFILE_IDENT_DESC)(window->Bytes + length);
                            uint32 lba;
                            if (item->Length < sizeof(FILE_IDENT_DESC) ||
                                item->Length > UDF_FID_WINDOW_BYTES - length) {
                                status = STATUS_FILE_CORRUPT_ERROR;
                                break;
                            }
                            if (UDFNewFidReady(vcb,owner,directory) != item->Index) {
                                status = STATUS_FILE_CORRUPT_ERROR;
                                break;
                            }
                            lba = UDFExtentOffsetToLba(vcb,directory->Dloc->DataLoc.Mapping,
                                                       item->Offset,NULL,NULL,NULL,NULL);
                            RtlCopyMemory(fid,owner->FileInfo->FileIdent,item->Length);
                            fid->fileCharacteristics = item->Index->FileCharacteristics;
                            UDFSetUpTag(vcb,&fid->descTag,(uint16)item->Length,
                                       UDFPhysLbaToPart(vcb,partition,lba),0);
                            length += item->Length;
                        }
                        if (!NT_SUCCESS(status))
                            break;
                        mapping = directory->Dloc->DataLoc.Mapping;
                        logicalLength = directory->Dloc->DataLoc.Length;
                        if (FlagOn(context->Flags,IRP_CONTEXT_FLAG_METADATA_WRITER) ||
                            context->MajorFunction == IRP_MJ_FLUSH_BUFFERS)
                            SetFlag(context->Flags,IRP_CONTEXT_FLAG_METADATA_PUBLISH);
                        _SEH2_TRY {
                            status = UDFWriteFile__(context,vcb,directory,
                                window->Item[i].Offset,length,FALSE,window->Bytes,&written);
                        } _SEH2_FINALLY {
                            if (!previous)
                                ClearFlag(context->Flags,IRP_CONTEXT_FLAG_METADATA_PUBLISH);
                        } _SEH2_END;
                        if (NT_SUCCESS(status) && written != length)
                            status = STATUS_IO_DEVICE_ERROR;
                        if (!NT_SUCCESS(status))
                            break; // Every FI in this window remains dirty/NEW.
                        if (directory->Dloc->DataLoc.Mapping != mapping ||
                            directory->Dloc->DataLoc.Length != logicalLength) {
                            status = STATUS_FILE_CORRUPT_ERROR;
                            break;
                        }
                        length = 0;
                        for (j = i; j < end; j++) {
                            UDF_FID_WINDOW_ITEM* item = &window->Item[j];
                            UDF_FE_OWNER* owner = &Batch->Owner[item->Owner];
                            if (UDFNewFidReady(vcb,owner,directory) != item->Index) {
                                status = STATUS_FILE_CORRUPT_ERROR;
                                break;
                            }
                            RtlCopyMemory(owner->FileInfo->FileIdent,window->Bytes+length,item->Length);
                            item->Index->FI_Flags &= ~UDF_FI_FLAG_FI_MODIFIED;
                            owner->Dloc->FE_Flags &= ~UDF_FE_FLAG_NEW_UNPUBLISHED;
                            UDF_METADATA_COUNT(vcb,31);
                            UDF_METADATA_COUNT(vcb,35);
                            UDF_METADATA_COUNT(vcb,58);
                            length += item->Length;
                        }
                        i = end;
                    }
                }
            }
            UDFReleaseFcb(context,parent);
            parentHeld = FALSE;
            first = limit;
        }
    } _SEH2_FINALLY {
        if (parentHeld)
            UDFReleaseFcb(context,parent);
        MyFreePool__(window);
    } _SEH2_END;
    return status;
}

static NTSTATUS
UDFWritebackNewFileBatchInternal(PIRP_CONTEXT IrpContext, PVCB Vcb,
    PFCB CurrentFcb, UDF_FE_VOLUME_SNAPSHOT* Snapshot, PBOOLEAN More)
{
    UDF_FE_WRITEBACK* batch;
    uint32 i;
    NTSTATUS status = STATUS_SUCCESS;
    BOOLEAN tableHeld = FALSE, vcbHeld = FALSE, dataHeld = FALSE;
    BOOLEAN volume = Snapshot != NULL;
    if (More) *More = FALSE;

#ifdef UDF_METADATA_BATCH_DIAGNOSTICS
    UDF_METADATA_COUNT(Vcb,13);
    if (CurrentFcb) {
        if (CurrentFcb->FcbReference == 1) UDF_METADATA_COUNT(Vcb,14);
        if (!CurrentFcb->FcbCleanup) UDF_METADATA_COUNT(Vcb,15);
        if (CurrentFcb->FileInfo && CurrentFcb->FileInfo->Dloc &&
            (CurrentFcb->FileInfo->Dloc->FE_Flags & UDF_FE_FLAG_NEW_UNPUBLISHED))
            UDF_METADATA_COUNT(Vcb,16);
        if (UDFNewFileCacheable(Vcb,CurrentFcb->FileInfo)) UDF_METADATA_COUNT(Vcb,17);
    }
#endif
    ASSERT_EXCLUSIVE_VCB(Vcb);
    if (!UDFNewMetadataVolume(Vcb))
        return STATUS_SUCCESS;
    if (!volume && (!CurrentFcb || CurrentFcb->FcbReference != 1 ||
        CurrentFcb->FcbCleanup || !UDFNewFileCacheable(Vcb,CurrentFcb->FileInfo) ||
        !UDFNewEntryDirty(CurrentFcb->FileInfo)))
        return STATUS_SUCCESS;
    batch = MyAllocatePool__(NonPagedPool, sizeof(*batch));
    if (!batch) {
        if (volume) UDF_METADATA_COUNT(Vcb,46);
        return STATUS_SUCCESS; // Outer unwind drops pending pins; tree flush remains required.
    }
    RtlZeroMemory(batch, sizeof(*batch));
    batch->IrpContext = IrpContext;
    batch->Vcb = Vcb;

    _SEH2_TRY {
        if (volume) {
            /* The snapshot's references protect these objects independently
             * of later generic-table deletion or rebalancing. No cursor or
             * table-element pointer survives outside the capture lock. The
             * assignments below make no raising calls before ownership has
             * transferred into the cohort's ordinary finally block. */
            while (batch->Count < UDF_FE_BATCH_CAPACITY &&
                   Snapshot->Next < Snapshot->Count) {
                batch->Owner[batch->Count++].Fcb = Snapshot->Files[Snapshot->Next++];
            }
        } else {
            UDFLockFcbTable(IrpContext, Vcb);
            tableHeld = TRUE;
            UDFLockVcb(IrpContext, Vcb);
            vcbHeld = TRUE;
            UDFHoldBatchOwner(IrpContext, Vcb, batch, CurrentFcb);
            /* The table mutex keeps registered FCBs resident; VcbMutex
             * stabilizes their counts. UdfDataMutex is innermost, as both
             * existing queue paths release it before any close processing.
             * Drop it before taking the cohort's ordinary reference holds. */
            UDFLockUdfData();
            dataHeld = TRUE;
            uint32 first = batch->Count;
            uint32 candidates = UDFSelectQueuedMetadataOwners(IrpContext,Vcb,
                first ? CurrentFcb : NULL, batch->Owner + first,
                UDF_FE_BATCH_CAPACITY - first);
            UDFUnlockUdfData();
            dataHeld = FALSE;
            /* Only Count's prefix owns references. Capture uses the unused
             * Owner suffix, then transfers each pointer into that prefix.
             * A rejected hold can only write behind the next unread slot;
             * an exception releases only the already acquired references. */
            for (i = 0; i < candidates; i++) {
                PFCB candidate = batch->Owner[first+i].Fcb;
                UDFHoldBatchOwner(IrpContext,Vcb,batch,candidate);
            }
            UDFUnlockVcb(IrpContext, Vcb);
            vcbHeld = FALSE;
            UDFUnlockFcbTable(IrpContext, Vcb);
            tableHeld = FALSE;
        }

#ifdef UDF_METADATA_BATCH_DIAGNOSTICS
        InterlockedExchangeAdd(&Vcb->MetadataBatchDiag[5],batch->Count);
        if ((LONG)batch->Count > Vcb->MetadataBatchDiag[6])
            Vcb->MetadataBatchDiag[6] = batch->Count; // exclusive VCB
#endif
        /* Each captured owner is pinned. Flush data one file at a time
         * before the all-resource FE cohort interval; Cc may issue paging IO
         * recursively. No FE is prepared and no FID can be published after
         * a data failure. The existing finally path releases all pins. */
        for (i = 0; i < batch->Count; i++) {
            PFCB ownerFcb = batch->Owner[i].Fcb;
            UDFAcquireFcbExclusive(IrpContext, ownerFcb, FALSE);
            _SEH2_TRY {
                status = UDFDrainNewFileData(IrpContext, Vcb, ownerFcb->FileInfo);
            } _SEH2_FINALLY {
                UDFReleaseFcb(IrpContext, ownerFcb);
            } _SEH2_END;
            if (!NT_SUCCESS(status))
                _SEH2_LEAVE;
        }
        if (volume) {
            if (More) *More = batch->Count != 0;
            /* VCB exclusive excludes user create/rename/truncate. Captured
             * ordinary FCBs are unique and in the table's signed FileId
             * order. Paging callbacks take one Main then its Paging resource;
             * directories, aliases and streams cannot enter this cohort.
             * Do not call Cc or last-reference teardown while it is held. */
            for (i = 0; i < batch->Count; i++) {
                UDF_FE_OWNER* owner = &batch->Owner[i];
                ASSERT(!i || batch->Owner[i-1].Fcb->FileId.QuadPart <
                              owner->Fcb->FileId.QuadPart);
                UDFAcquireFcbExclusive(IrpContext,owner->Fcb,FALSE);
                owner->MainHeld = TRUE;
                UDFAcquirePagingIoExclusive(IrpContext,owner->Fcb);
                owner->PagingHeld = TRUE;
#ifdef UDF_METADATA_BATCH_DIAGNOSTICS
                UDF_METADATA_COUNT(Vcb,41);
#endif
            }
#ifdef UDF_METADATA_BATCH_DIAGNOSTICS
            UDF_METADATA_COUNT(Vcb,39);
            InterlockedExchangeAdd(&Vcb->MetadataBatchDiag[40],batch->Count);
#endif
        }
        /* Close-queue cohorts have no surviving FILE_OBJECT operations and
         * retain their old one-Main-at-a-time path. Volume cohorts may still
         * have paging writers, so their resources span prepare -> IO -> ACK. */
        for (i = 0; i < batch->Count; i++) {
            UDF_FE_OWNER* owner = &batch->Owner[i];
            if (!volume)
                UDFAcquireFcbExclusive(IrpContext, owner->Fcb, FALSE);
            _SEH2_TRY {
                if (UDFNewFileCacheable(Vcb, owner->Fcb->FileInfo)) {
                    uint32 partition;
                    owner->FileInfo = owner->Fcb->FileInfo;
                    owner->Dloc = owner->FileInfo->Dloc;
                    partition = UDFGetRefPartNumByPhysLba(Vcb,
                        owner->Dloc->FELoc.Mapping[0].extLocation);
                    if (partition < Vcb->PartitionMaps) {
                        status = UDFStageNewFileEntry(IrpContext, Vcb,
                            owner->FileInfo, partition, owner->Bytes, &owner->Staged);
                        if (NT_SUCCESS(status) && owner->Staged) {
                            /* Serialization may relocate a known-bad FE.
                             * Bind the staged bytes to its final mapping,
                             * never the address seen before preparation. */
                            owner->Lba = owner->Dloc->FELoc.Mapping[0].extLocation;
                            owner->Partition = UDFGetRefPartNumByPhysLba(Vcb, owner->Lba);
                            if (owner->FileInfo->Dloc != owner->Dloc ||
                                owner->Partition != partition) {
                                status = STATUS_FILE_CORRUPT_ERROR;
                            } else {
                                UDF_FE_BATCH_WRITE* write = &batch->Write[batch->StagedCount++];
                                write->Lba = owner->Lba;
                                write->Partition = owner->Partition;
                                write->Bytes = owner->Bytes;
                            }
                        }
                    }
                }
            } _SEH2_FINALLY {
                if (!volume) {
                    UDFReleaseFcb(IrpContext, owner->Fcb);
                }
            } _SEH2_END;
            if (!NT_SUCCESS(status))
                break;
        }
        if (NT_SUCCESS(status))
            status = UDFCommitFileEntryBatch(batch->Write, batch->StagedCount,
                batch->Transfer, batch->Order, UDFBatchSectorWrite, batch);
        if (NT_SUCCESS(status)) {
            for (i = 0; i < batch->Count; i++) {
                UDF_FE_OWNER* owner = &batch->Owner[i];
                if (!owner->Staged)
                    continue;
                if (!volume)
                    UDFAcquireFcbExclusive(IrpContext, owner->Fcb, FALSE);
                _SEH2_TRY {
                    /* Do not acknowledge a changed identity/generation. A
                     * normal per-file flush will serialize it again before
                     * its valid FID is written. */
                    if (owner->Fcb->FileInfo == owner->FileInfo &&
                        owner->FileInfo->Dloc == owner->Dloc &&
                        owner->Dloc->FELoc.Mapping[0].extLocation == owner->Lba &&
                        !owner->Dloc->DataLoc.Modified && !owner->Dloc->AllocLoc.Modified &&
                        RtlCompareMemory(owner->Bytes, owner->Dloc->FileEntry, 512) == 512) {
                        owner->Dloc->FE_Flags &= ~UDF_FE_FLAG_FE_MODIFIED;
                        owner->Dloc->FELoc.Modified = FALSE;
                    }
                } _SEH2_FINALLY {
                    if (!volume) {
                        UDFReleaseFcb(IrpContext, owner->Fcb);
                    }
                } _SEH2_END;
            }
        }
        if (volume && NT_SUCCESS(status))
            status = UDFPublishNewVolumeIdentifiers(batch);
    } _SEH2_FINALLY {
        if (dataHeld) {
            UDFUnlockUdfData();
        }
        if (vcbHeld) {
            UDFUnlockVcb(IrpContext, Vcb);
        }
        if (tableHeld) {
            UDFUnlockFcbTable(IrpContext, Vcb);
        }
        /* Drop every cohort resource before any per-owner last-reference
         * release can walk its parent directory or call the cache manager. */
        for (i = min(batch->Count, UDF_FE_BATCH_CAPACITY); i; i--) {
            UDF_FE_OWNER* owner = &batch->Owner[i-1];
            if (owner->PagingHeld) {
                UDFReleasePagingIo(IrpContext,owner->Fcb);
                owner->PagingHeld = FALSE;
            }
            if (owner->MainHeld) {
                UDFReleaseFcb(IrpContext,owner->Fcb);
                owner->MainHeld = FALSE;
            }
        }
        if (_SEH2_AbnormalTermination())
            status = STATUS_UNSUCCESSFUL;
        UDFRememberMetadataWritebackError(IrpContext, Vcb, NULL, status);
#ifdef UDF_METADATA_BATCH_DIAGNOSTICS
        InterlockedIncrement(&Vcb->MetadataBatchDiag[0]);
        InterlockedExchangeAdd(&Vcb->MetadataBatchDiag[1], batch->StagedCount);
        if (!NT_SUCCESS(status))
            InterlockedIncrement(&Vcb->MetadataBatchDiag[4]);
#endif
        for (i = 0; i < batch->Count; i++) {
            NTSTATUS releaseStatus = UDFReleaseMetadataOwner(
                IrpContext, Vcb, batch->Owner[i].Fcb);
            if (NT_SUCCESS(status))
                status = releaseStatus;
        }
        MyFreePool__(batch);
    } _SEH2_END;
    return status;
}

NTSTATUS
UDFWritebackNewFileBatch(PIRP_CONTEXT IrpContext, PVCB Vcb, PFCB CurrentFcb)
{
    return UDFWritebackNewFileBatchInternal(IrpContext,Vcb,CurrentFcb,NULL,NULL);
}

NTSTATUS
UDFWritebackNewVolumeFiles(PIRP_CONTEXT IrpContext, PVCB Vcb)
{
    UDF_FE_VOLUME_SNAPSHOT snapshot;
    PVOID restart = NULL;
    PFCB fcb;
    uint32 capacity;
    BOOLEAN more, tableHeld = FALSE, vcbHeld = FALSE;
    NTSTATUS status = STATUS_SUCCESS;
    ASSERT_EXCLUSIVE_VCB(Vcb);
    UDF_METADATA_COUNT(Vcb,38);
    if (!UDFNewMetadataVolume(Vcb))
        return STATUS_SUCCESS;
    RtlZeroMemory(&snapshot, sizeof(snapshot));

    /* Size without the VCB fast mutex; pool allocation must not extend the
     * capture critical section. A concurrent table shrink is harmless. If
     * more entries appear, only this bounded prefix is optimized; the full
     * mandatory tree flush still visits every remaining dirty generation. */
    UDFLockFcbTable(IrpContext, Vcb);
    _SEH2_TRY {
        capacity = RtlNumberGenericTableElements(&Vcb->FcbTable);
    } _SEH2_FINALLY {
        UDFUnlockFcbTable(IrpContext, Vcb);
    } _SEH2_END;
    if (!capacity)
        return STATUS_SUCCESS;
    if (capacity > UDF_FE_SNAPSHOT_BUDGET / sizeof(PFCB)) {
        UDF_METADATA_COUNT(Vcb,45);
        return STATUS_SUCCESS;
    }
    snapshot.Files = MyAllocatePool__(NonPagedPool, capacity * sizeof(PFCB));
    if (!snapshot.Files) {
        UDF_METADATA_COUNT(Vcb,44);
        return STATUS_SUCCESS;
    }

    _SEH2_TRY {
        UDFLockFcbTable(IrpContext, Vcb);
        tableHeld = TRUE;
        UDFLockVcb(IrpContext, Vcb);
        vcbHeld = TRUE;
        while (snapshot.Count < capacity &&
               (fcb = UDFGetNextFcb(IrpContext, Vcb, &restart))) {
            UDF_METADATA_COUNT(Vcb,7);
            UDF_METADATA_COUNT(Vcb,43);
            if (!fcb->FcbNonpaged || fcb->Header.PagingIoResource !=
                    &fcb->FcbNonpaged->FcbPagingIoResource ||
                fcb->FcbReference != 1 || fcb->FcbCleanup != 0 ||
                !UDFNewFileCacheable(Vcb, fcb->FileInfo) ||
                !UDFNewEntryDirty(fcb->FileInfo))
                continue;
            /* This pin keeps the exact canonical FI/Dloc/LCB lifetime even
             * if the last FILE_OBJECT close consumes its earlier reference
             * before this owner's cohort is reached. */
            fcb->FcbReference++;
            snapshot.Files[snapshot.Count++] = fcb;
            UDF_METADATA_COUNT(Vcb,42);
        }
        UDFUnlockVcb(IrpContext, Vcb);
        vcbHeld = FALSE;
        UDFUnlockFcbTable(IrpContext, Vcb);
        tableHeld = FALSE;

        status = UDFDrainVolumeDataParallel(IrpContext, Vcb,
                                           snapshot.Files, snapshot.Count);
        if (!NT_SUCCESS(status))
            _SEH2_LEAVE;
        do {
            status = UDFWritebackNewFileBatchInternal(IrpContext,Vcb,NULL,&snapshot,&more);
        } while (NT_SUCCESS(status) && more && snapshot.Next < snapshot.Count);
    } _SEH2_FINALLY {
        if (vcbHeld) {
            UDFUnlockVcb(IrpContext, Vcb);
        }
        if (tableHeld) {
            UDFUnlockFcbTable(IrpContext, Vcb);
        }
        if (_SEH2_AbnormalTermination())
            status = STATUS_UNSUCCESSFUL;
        /* A failed stage/write or a cohort allocation failure cannot leak
         * all the not-yet-transferred snapshot owners. No cohort resources
         * are held while this can perform a last-reference teardown. */
        while (snapshot.Next < snapshot.Count) {
            UDF_METADATA_COUNT(Vcb,47);
            NTSTATUS releaseStatus = UDFReleaseMetadataOwner(
                IrpContext,Vcb,snapshot.Files[snapshot.Next++]);
            if (NT_SUCCESS(status))
                status = releaseStatus;
        }
        MyFreePool__(snapshot.Files);
    } _SEH2_END;
    /* This is only a metadata phase, not the explicit data/tree/device
     * barrier. The caller still runs the complete original volume flush. */
    return status;
}
