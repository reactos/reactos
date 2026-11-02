/*
 * PROJECT:     ReactOS UDF File System Driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Copy-on-write checkpoints for the UDF 2.50 metadata partition
 * COPYRIGHT:   Copyright 2026 Fuad Amin Ishtiak
 */

#include "udffs.h"

#define UDF_BUG_CHECK_ID UDF_FILE_UDF_INFO

/* Locking: Vcb->CowResource is taken shared for metadata reads and
 * exclusive for metadata writes, checkpoints and start/stop. It is always
 * the innermost lock: it is taken inside the sector I/O layer, so nothing
 * else may be acquired while it is held. BitMapResource1, when needed, is
 * taken before it or after it is released. */

static VOID
CowLock(PVCB Vcb, BOOLEAN Exclusive)
{
    KeEnterCriticalRegion();
    if (Exclusive)
        ExAcquireResourceExclusiveLite(&Vcb->CowResource, TRUE);
    else
        ExAcquireResourceSharedLite(&Vcb->CowResource, TRUE);
}

static VOID
CowUnlock(PVCB Vcb)
{
    ExReleaseResourceLite(&Vcb->CowResource);
    KeLeaveCriticalRegion();
}

static __inline BOOLEAN
CowStale(PUDF_COW Cow, uint32 Unit)
{
    return (Cow->Stale[Unit >> 3] >> (Unit & 7)) & 1;
}

/* Metadata block reached through home address Lba, or LBA_OUT_OF_EXTENT.
 * RunEnd receives the home address just past the containing extent. */
static uint32
CowHomeToLbn(PUDF_COW Cow, uint32 Lba, uint32* RunEnd, BOOLEAN* Mirror)
{
    uint32 i;

    for (i = 0; i < Cow->MainRuns; i++) {
        PUDF_COW_RUN r = &Cow->Main[i];
        if (Lba >= r->Lba[0] && Lba - r->Lba[0] < r->Blocks) {
            *RunEnd = r->Lba[0] + r->Blocks;
            *Mirror = FALSE;
            return r->Lbn + (Lba - r->Lba[0]);
        }
    }
    for (i = 0; Cow->Duplicated && i < Cow->MirrorRuns; i++) {
        PUDF_COW_RUN r = &Cow->Mirror[i];
        if (Lba >= r->Lba[0] && Lba - r->Lba[0] < r->Blocks) {
            *RunEnd = r->Lba[0] + r->Blocks;
            *Mirror = TRUE;
            return r->Lbn + (Lba - r->Lba[0]);
        }
    }
    return LBA_OUT_OF_EXTENT;
}

/* Physical address of metadata block Lbn in Region; Avail receives the
 * blocks left in that extent. */
static uint32
CowLbnToPhys(PUDF_COW Cow, BOOLEAN Mirror, uint8 Region, uint32 Lbn, uint32* Avail)
{
    PUDF_COW_RUN runs = (Mirror && Cow->Duplicated) ? Cow->Mirror : Cow->Main;
    uint32 count = (Mirror && Cow->Duplicated) ? Cow->MirrorRuns : Cow->MainRuns;
    uint32 i;

    for (i = 0; i < count; i++) {
        if (Lbn >= runs[i].Lbn && Lbn - runs[i].Lbn < runs[i].Blocks) {
            *Avail = runs[i].Blocks - (Lbn - runs[i].Lbn);
            return runs[i].Lba[Region] + (Lbn - runs[i].Lbn);
        }
    }
    *Avail = 0;
    return LBA_OUT_OF_EXTENT;
}

static BOOLEAN
CowInRuns(PUDF_COW Cow, uint32 Lba)
{
    uint32 i, r;

    for (i = 0; i < Cow->MainRuns; i++)
        for (r = 0; r < 2; r++)
            if (Lba >= Cow->Main[i].Lba[r] && Lba - Cow->Main[i].Lba[r] < Cow->Main[i].Blocks)
                return TRUE;
    for (i = 0; Cow->Duplicated && i < Cow->MirrorRuns; i++)
        for (r = 0; r < 2; r++)
            if (Lba >= Cow->Mirror[i].Lba[r] && Lba - Cow->Mirror[i].Lba[r] < Cow->Mirror[i].Blocks)
                return TRUE;
    return FALSE;
}

static NTSTATUS
CowRaw(PIRP_CONTEXT IrpContext, PVCB Vcb, uint32 Lba, uint32 Blocks, PVOID Buffer, BOOLEAN Write)
{
    return UDFReadWriteSectorsDirect(IrpContext, Vcb,
               ((LONGLONG)Lba) << Vcb->SectorShift, Blocks << Vcb->SectorShift,
               Buffer, Write);
}

/* Read or write metadata blocks [Lbn, Lbn + Count) in one region. */
static NTSTATUS
CowRegionIo(PIRP_CONTEXT IrpContext, PVCB Vcb, PUDF_COW Cow, BOOLEAN Mirror,
            uint8 Region, uint32 Lbn, uint32 Count, int8* Buffer, BOOLEAN Write)
{
    NTSTATUS status;
    uint32 lba, avail, n;

    while (Count) {
        lba = CowLbnToPhys(Cow, Mirror, Region, Lbn, &avail);
        if (lba == LBA_OUT_OF_EXTENT)
            return STATUS_FILE_CORRUPT_ERROR;
        n = min(avail, Count);
        status = CowRaw(IrpContext, Vcb, lba, n, Buffer, Write);
        if (!NT_SUCCESS(status))
            return status;
        Lbn += n;
        Count -= n;
        Buffer += ((SIZE_T)n) << Vcb->SectorShift;
    }
    return STATUS_SUCCESS;
}

static uint32
CowUnitCount(PUDF_COW Cow, uint32 Unit)
{
    uint32 first = Unit * Cow->UnitBlocks;
    return min(Cow->UnitBlocks, Cow->MetadataBlocks - first);
}

static NTSTATUS
CowLoadUnit(PIRP_CONTEXT IrpContext, PVCB Vcb, PUDF_COW Cow, uint32 Unit, int8* Buffer)
{
    RtlZeroMemory(Buffer, Cow->UnitBytes);
    return CowRegionIo(IrpContext, Vcb, Cow, FALSE, Cow->Region,
                       Unit * Cow->UnitBlocks, CowUnitCount(Cow, Unit), Buffer, FALSE);
}

/* Write one unit image to Region, main copy and (if separate) mirror. */
static NTSTATUS
CowStoreUnit(PIRP_CONTEXT IrpContext, PVCB Vcb, PUDF_COW Cow, uint32 Unit,
             uint8 Region, int8* Buffer)
{
    uint32 first = Unit * Cow->UnitBlocks, count = CowUnitCount(Cow, Unit);
    NTSTATUS status;

    status = CowRegionIo(IrpContext, Vcb, Cow, FALSE, Region, first, count, Buffer, TRUE);
    if (NT_SUCCESS(status) && Cow->Duplicated)
        status = CowRegionIo(IrpContext, Vcb, Cow, TRUE, Region, first, count, Buffer, TRUE);
    return status;
}

/* Rewrite a Metadata (Mirror) File entry so its allocation descriptors
 * point to Region. One sector: this is the commit. */
static NTSTATUS
CowWriteFe(PIRP_CONTEXT IrpContext, PVCB Vcb, PUDF_FILE_INFO FileInfo,
           PUDF_COW_RUN Runs, uint32 Count, uint8 Region)
{
    PUDF_DATALOC_INFO dloc = FileInfo->Dloc;
    uint32 feLen = dloc->FileEntryLen, lba = dloc->FELoc.Mapping[0].extLocation;
    uint32 physRef = Vcb->MetadataPhysRef, adSize, i;
    uint16 allocMode = ((PFILE_ENTRY)dloc->FileEntry)->icbTag.flags & ICB_FLAG_ALLOC_MASK;
    int8* sector;
    int8* ad;
    NTSTATUS status;

    adSize = allocMode == ICB_FLAG_AD_SHORT ? sizeof(SHORT_AD) : sizeof(LONG_AD);
    if (feLen + Count * adSize > Vcb->SectorSize)
        return STATUS_FILE_CORRUPT_ERROR;
    sector = (int8*)MyAllocatePool__(NonPagedPool, Vcb->SectorSize);
    if (!sector)
        return STATUS_INSUFFICIENT_RESOURCES;
    RtlZeroMemory(sector, Vcb->SectorSize);
    RtlCopyMemory(sector, dloc->FileEntry, feLen);
    ad = sector + feLen;
    for (i = 0; i < Count; i++, ad += adSize) {
        uint32 pos = UDFPhysLbaToPart(Vcb, physRef, Runs[i].Lba[Region]);
        if (allocMode == ICB_FLAG_AD_SHORT) {
            PSHORT_AD s = (PSHORT_AD)ad;
            s->extLength = Runs[i].Blocks << Vcb->SectorShift;
            s->extPosition = pos;
        } else {
            PLONG_AD l = (PLONG_AD)ad;
            l->extLength = Runs[i].Blocks << Vcb->SectorShift;
            l->extLocation.logicalBlockNum = pos;
            l->extLocation.partitionReferenceNum = (uint16)physRef;
        }
    }
    if (dloc->FileEntry->tagIdent == TID_FILE_ENTRY)
        ((PFILE_ENTRY)sector)->lengthAllocDescs = Count * adSize;
    else
        ((PEXTENDED_FILE_ENTRY)sector)->lengthAllocDescs = Count * adSize;
    UDFSetUpTag(Vcb, (tag*)sector, (uint16)(feLen + Count * adSize),
                UDFPhysLbaToPart(Vcb, physRef, lba), 0);
    status = CowRaw(IrpContext, Vcb, lba, 1, sector, TRUE);
    MyFreePool__(sector);
    return status;
}

/* Point both Metadata File entries to Region. */
static NTSTATUS
CowCommit(PIRP_CONTEXT IrpContext, PVCB Vcb, PUDF_COW Cow, uint8 Region)
{
    NTSTATUS status;

    status = UDFBarrierFlushDevice(Vcb->TargetDeviceObject);
    if (!NT_SUCCESS(status))
        return status;
    status = CowWriteFe(IrpContext, Vcb, Vcb->MetadataFileInfo, Cow->Main, Cow->MainRuns, Region);
    if (!NT_SUCCESS(status))
        return status;
    status = UDFBarrierFlushDevice(Vcb->TargetDeviceObject);
    if (!NT_SUCCESS(status))
        return status;
    /* From here the main copy is committed; a failed mirror update leaves
     * the mirror one checkpoint behind, which the main copy overrides. */
    Cow->Region = Region;
    status = Cow->Duplicated ?
        CowWriteFe(IrpContext, Vcb, Vcb->MetadataMirrorFileInfo, Cow->Mirror, Cow->MirrorRuns, Region) :
        CowWriteFe(IrpContext, Vcb, Vcb->MetadataMirrorFileInfo, Cow->Main, Cow->MainRuns, Region);
    if (NT_SUCCESS(status))
        status = UDFBarrierFlushDevice(Vcb->TargetDeviceObject);
    if (!NT_SUCCESS(status))
        Vcb->VcbState |= VCB_STATE_VOLUME_READ_ONLY;
    return status;
}

/*
 * Sector I/O hook. Returns FALSE when the request does not touch the
 * metadata area, so the caller handles it as before.
 */
BOOLEAN
UDFCowIo(
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN LONGLONG StartingOffset,
    IN ULONG ByteCount,
    IN PVOID Buffer,
    IN BOOLEAN IsWrite,
    OUT NTSTATUS* Status
    )
{
    PUDF_COW cow;
    NTSTATUS status = STATUS_SUCCESS;
    uint32 lba, count, i, n, lbn, unit, offset, runEnd, end, dummy;
    BOOLEAN mirror, touches = FALSE, kick;
    int8* p;

    if (!Vcb->Cow || ((StartingOffset | ByteCount) & (Vcb->SectorSize - 1)) || !ByteCount)
        return FALSE;
    lba = (uint32)(StartingOffset >> Vcb->SectorShift);
    count = ByteCount >> Vcb->SectorShift;

    CowLock(Vcb, IsWrite);
    cow = Vcb->Cow;
    if (cow) {
        for (i = 0; i < count && !touches; i++)
            touches = CowHomeToLbn(cow, lba + i, &runEnd, &mirror) != LBA_OUT_OF_EXTENT;
    }
    if (!touches) {
        CowUnlock(Vcb);
        return FALSE;
    }

    for (i = 0; i < count && NT_SUCCESS(status); i += n) {
        p = (int8*)Buffer + (((SIZE_T)i) << Vcb->SectorShift);
        lbn = CowHomeToLbn(cow, lba + i, &runEnd, &mirror);
        if (lbn == LBA_OUT_OF_EXTENT) {
            for (n = 1; i + n < count &&
                 CowHomeToLbn(cow, lba + i + n, &dummy, &mirror) == LBA_OUT_OF_EXTENT; n++)
                ;
            status = CowRaw(IrpContext, Vcb, lba + i, n, p, IsWrite);
            continue;
        }
        unit = lbn / cow->UnitBlocks;
        offset = lbn % cow->UnitBlocks;
        end = min(cow->UnitBlocks - offset, runEnd - (lba + i));
        end = min(end, count - i);
        n = min(end, cow->MetadataBlocks - lbn);
        if (!n) {
            status = STATUS_FILE_CORRUPT_ERROR;
            break;
        }
        if (IsWrite) {
            if (!cow->Buffer[unit]) {
                int8* image = (int8*)MyAllocatePool__(NonPagedPool, cow->UnitBytes);
                if (!image) {
                    status = STATUS_INSUFFICIENT_RESOURCES;
                    break;
                }
                if (offset || n != CowUnitCount(cow, unit))
                    status = CowLoadUnit(IrpContext, Vcb, cow, unit, image);
                else
                    RtlZeroMemory(image, cow->UnitBytes);
                if (!NT_SUCCESS(status)) {
                    MyFreePool__(image);
                    break;
                }
                cow->Buffer[unit] = image;
                cow->DirtyUnits++;
            }
            RtlCopyMemory(cow->Buffer[unit] + (((SIZE_T)offset) << Vcb->SectorShift), p,
                          ((SIZE_T)n) << Vcb->SectorShift);
        } else if (cow->Buffer[unit]) {
            RtlCopyMemory(p, cow->Buffer[unit] + (((SIZE_T)offset) << Vcb->SectorShift),
                          ((SIZE_T)n) << Vcb->SectorShift);
        } else {
            status = CowRegionIo(IrpContext, Vcb, cow, mirror, cow->Region, lbn, n, p, FALSE);
        }
    }

    kick = IsWrite && (cow->DirtyUnits >= UDF_COW_DIRTY_KICK ||
           (IrpContext && FlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_WRITE_THROUGH)));
    if (kick)
        KeSetEvent(&cow->Wake, IO_NO_INCREMENT, FALSE);
    CowUnlock(Vcb);
    *Status = status;
    return TRUE;
}

/* Called with BitMapResource1 held from the core free routine. Keeps the
 * blocks allocated until a checkpoint no longer refers to them. */
BOOLEAN
UDFCowDeferFree(
    IN PVCB Vcb,
    IN PEXTENT_MAP Map
    )
{
    PUDF_COW cow = Vcb->Cow;
    PUDF_COW_FREE node;
    uint32 n;

    if (!cow || cow->ApplyThread == PsGetCurrentThread())
        return FALSE;
    for (n = 0; Map[n].extLength & UDF_EXTENT_LENGTH_MASK; n++)
        ;
    if (!n)
        return TRUE;
    node = (PUDF_COW_FREE)MyAllocatePool__(NonPagedPool,
               sizeof(UDF_COW_FREE) + n * sizeof(EXTENT_MAP));
    if (!node)
        return FALSE;
    RtlCopyMemory(node->Map, Map, (n + 1) * sizeof(EXTENT_MAP));
    node->Map[n].extLength = 0;
    node->Map[n].extLocation = 0;
    node->Next = cow->Pending;
    cow->Pending = node;
    return TRUE;
}

#define UDF_COW_TRIM_RANGES 256

typedef struct _UDF_COW_TRIM {
    DEVICE_MANAGE_DATA_SET_ATTRIBUTES Header;
    DEVICE_DATA_SET_RANGE Ranges[UDF_COW_TRIM_RANGES];
} UDF_COW_TRIM, *PUDF_COW_TRIM;

static VOID
CowTrim(PVCB Vcb, PUDF_COW Cow, PUDF_COW_FREE List)
{
    PUDF_COW_TRIM request;
    PUDF_COW_FREE node;
    uint32 count = 0, i, lba, len;
    NTSTATUS status;
    IO_STATUS_BLOCK iosb;

    if (Cow->TrimUnsupported || !List)
        return;
    request = (PUDF_COW_TRIM)MyAllocatePool__(NonPagedPool, sizeof(UDF_COW_TRIM));
    if (!request)
        return;
    for (node = List; node; node = node->Next) {
        for (i = 0; node->Map[i].extLength & UDF_EXTENT_LENGTH_MASK; i++) {
            if ((node->Map[i].extLength >> 30) == EXTENT_NOT_RECORDED_NOT_ALLOCATED)
                continue;
            lba = node->Map[i].extLocation;
            len = (node->Map[i].extLength & UDF_EXTENT_LENGTH_MASK) >> Vcb->SectorShift;
            /* Freed metadata blocks share the metadata area; only data
             * blocks outside it are handed back to the device. */
            if (!len || CowInRuns(Cow, lba) || CowInRuns(Cow, lba + len - 1))
                continue;
            if (count == UDF_COW_TRIM_RANGES)
                break;
            request->Ranges[count].StartingOffset = ((LONGLONG)lba) << Vcb->SectorShift;
            request->Ranges[count].LengthInBytes = ((ULONGLONG)len) << Vcb->SectorShift;
            count++;
        }
    }
    if (count) {
        RtlZeroMemory(&request->Header, sizeof(request->Header));
        request->Header.Size = sizeof(request->Header);
        request->Header.Action = DeviceDsmAction_Trim;
        request->Header.DataSetRangesOffset = FIELD_OFFSET(UDF_COW_TRIM, Ranges);
        request->Header.DataSetRangesLength = count * sizeof(DEVICE_DATA_SET_RANGE);
        status = UDFPerformDevIoCtrl(IOCTL_STORAGE_MANAGE_DATA_SET_ATTRIBUTES,
                     Vcb->TargetDeviceObject, request,
                     request->Header.DataSetRangesOffset + request->Header.DataSetRangesLength,
                     NULL, 0, TRUE, &iosb);
        if (status == STATUS_INVALID_DEVICE_REQUEST || status == STATUS_NOT_SUPPORTED ||
            status == STATUS_NOT_IMPLEMENTED || status == STATUS_INVALID_PARAMETER)
            Cow->TrimUnsupported = TRUE;
    }
    MyFreePool__(request);
}

static VOID
CowFreeList(PUDF_COW_FREE List)
{
    while (List) {
        PUDF_COW_FREE next = List->Next;
        MyFreePool__(List);
        List = next;
    }
}

static VOID
CowApplyFrees(PVCB Vcb, PUDF_COW Cow, PUDF_COW_FREE List)
{
    PUDF_COW_FREE node;

    if (!List)
        return;
    UDFAcquireResourceExclusive(&(Vcb->BitMapResource1), TRUE);
    if (Cow)
        Cow->ApplyThread = PsGetCurrentThread();
    for (node = List; node; node = node->Next)
        UDFMarkSpaceAsXXXNoProtect(Vcb, 0, node->Map, AS_DISCARDED);
    if (Cow)
        Cow->ApplyThread = NULL;
    UDFReleaseResource(&(Vcb->BitMapResource1));
    if (Cow)
        CowTrim(Vcb, Cow, List);
    CowFreeList(List);
}

static PUDF_COW_FREE
CowJoin(PUDF_COW_FREE A, PUDF_COW_FREE B)
{
    PUDF_COW_FREE tail = A;
    if (!A)
        return B;
    while (tail->Next)
        tail = tail->Next;
    tail->Next = B;
    return A;
}

/* Write all changed units to the other region and switch to it. */
static NTSTATUS
CowCheckpointLocked(PIRP_CONTEXT IrpContext, PVCB Vcb, PUDF_COW Cow)
{
    uint8 target = Cow->Region ^ 1;
    int8* scratch;
    uint32 u;
    NTSTATUS status = STATUS_SUCCESS;

    if (!Cow->DirtyUnits)
        return STATUS_SUCCESS;
    if (FlagOn(Vcb->VcbState, VCB_STATE_VOLUME_READ_ONLY | UDF_VCB_FLAGS_DEAD))
        return STATUS_MEDIA_WRITE_PROTECTED;
    scratch = (int8*)MyAllocatePool__(NonPagedPool, Cow->UnitBytes);
    if (!scratch)
        return STATUS_INSUFFICIENT_RESOURCES;
    for (u = 0; u < Cow->Units && NT_SUCCESS(status); u++) {
        int8* image = Cow->Buffer[u];
        if (!image && !CowStale(Cow, u))
            continue;
        if (!image) {
            status = CowLoadUnit(IrpContext, Vcb, Cow, u, scratch);
            image = scratch;
        }
        if (NT_SUCCESS(status))
            status = CowStoreUnit(IrpContext, Vcb, Cow, u, target, image);
    }
    MyFreePool__(scratch);
    if (NT_SUCCESS(status))
        status = CowCommit(IrpContext, Vcb, Cow, target);
    if (Cow->Region != target)
        return status;      // nothing committed; the changes stay buffered
    /* The old region now lags exactly by the units written from memory. */
    RtlZeroMemory(Cow->Stale, (Cow->Units + 7) / 8);
    for (u = 0; u < Cow->Units; u++) {
        if (Cow->Buffer[u]) {
            Cow->Stale[u >> 3] |= (uint8)(1 << (u & 7));
            MyFreePool__(Cow->Buffer[u]);
            Cow->Buffer[u] = NULL;
        }
    }
    Cow->DirtyUnits = 0;
    Cow->Checkpoints++;
    return status;
}

NTSTATUS
UDFCowCheckpoint(
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN BOOLEAN ApplyAllFrees
    )
{
    PUDF_COW cow = Vcb->Cow;
    PUDF_COW_FREE ready;
    NTSTATUS status;

    if (!cow)
        return STATUS_SUCCESS;
    /* A free waits until the checkpoint after the one that follows it, so
     * the sector write that dropped the reference is committed first. */
    UDFAcquireResourceExclusive(&(Vcb->BitMapResource1), TRUE);
    ready = cow->Waiting;
    if (ApplyAllFrees) {
        ready = CowJoin(ready, cow->Pending);
        cow->Waiting = NULL;
    } else {
        cow->Waiting = cow->Pending;
    }
    cow->Pending = NULL;
    UDFReleaseResource(&(Vcb->BitMapResource1));

    CowLock(Vcb, TRUE);
    status = CowCheckpointLocked(IrpContext, Vcb, cow);
    CowUnlock(Vcb);

    if (NT_SUCCESS(status)) {
        CowApplyFrees(Vcb, cow, ready);
    } else {
        UDFAcquireResourceExclusive(&(Vcb->BitMapResource1), TRUE);
        cow->Waiting = CowJoin(ready, cow->Waiting);
        UDFReleaseResource(&(Vcb->BitMapResource1));
    }
    return status;
}

static VOID NTAPI
CowThread(PVOID Context)
{
    PVCB Vcb = (PVCB)Context;
    PUDF_COW cow = Vcb->Cow;
    LARGE_INTEGER timeout;
    struct IRP_CONTEXT irpContext;

    timeout.QuadPart = -10LL * 1000 * 1000 * UDF_COW_INTERVAL_SEC;
    for (;;) {
        KeWaitForSingleObject(&cow->Wake, Executive, KernelMode, FALSE, &timeout);
        if (cow->Stop)
            break;
        if (Vcb->VcbCondition != VcbMounted ||
            FlagOn(Vcb->VcbState, VCB_STATE_VOLUME_READ_ONLY | UDF_VCB_FLAGS_DEAD))
            continue;
        RtlZeroMemory(&irpContext, sizeof(irpContext));
        irpContext.NodeIdentifier.NodeTypeCode = UDF_NODE_TYPE_IRP_CONTEXT;
        irpContext.NodeIdentifier.NodeByteSize = sizeof(irpContext);
        irpContext.MajorFunction = IRP_MJ_FLUSH_BUFFERS;
        irpContext.Vcb = Vcb;
        irpContext.RealDevice = Vcb->Vpb->RealDevice;
        irpContext.Flags = IRP_CONTEXT_FLAG_ON_STACK | IRP_CONTEXT_FLAG_WAIT;
        KeEnterCriticalRegion();
        (VOID)UDFCowCheckpoint(&irpContext, Vcb, FALSE);
        KeLeaveCriticalRegion();
    }
    PsTerminateSystemThread(STATUS_SUCCESS);
}

static VOID
CowStopThread(PUDF_COW Cow)
{
    if (!Cow->Thread)
        return;
    InterlockedExchange(&Cow->Stop, 1);
    KeSetEvent(&Cow->Wake, IO_NO_INCREMENT, FALSE);
    KeWaitForSingleObject(Cow->Thread, Executive, KernelMode, FALSE, NULL);
    ObDereferenceObject(Cow->Thread);
    Cow->Thread = NULL;
}

static VOID
CowFree(PUDF_COW Cow)
{
    uint32 u;

    if (Cow->Buffer) {
        for (u = 0; u < Cow->Units; u++)
            if (Cow->Buffer[u])
                MyFreePool__(Cow->Buffer[u]);
        MyFreePool__(Cow->Buffer);
    }
    if (Cow->Stale)
        MyFreePool__(Cow->Stale);
    for (u = 0; u < Cow->AltMapCount; u++)
        MyFreePool__(Cow->AltMaps[u]);
    CowFreeList(Cow->Pending);
    CowFreeList(Cow->Waiting);
    MyFreePool__(Cow);
}

/* Build the extent list of a Metadata (Mirror) File; FALSE if this layout
 * cannot be committed with one FE sector. */
static BOOLEAN
CowBuildRuns(PVCB Vcb, PUDF_FILE_INFO FileInfo, PUDF_COW_RUN Runs, uint32* Count)
{
    PEXTENT_MAP map = FileInfo->Dloc->DataLoc.Mapping;
    uint16 allocMode = ((PFILE_ENTRY)FileInfo->Dloc->FileEntry)->icbTag.flags & ICB_FLAG_ALLOC_MASK;
    uint32 adSize, i, lbn = 0;

    if (!map || !FileInfo->Dloc->FELoc.Mapping ||
        (allocMode != ICB_FLAG_AD_SHORT && allocMode != ICB_FLAG_AD_LONG))
        return FALSE;
    adSize = allocMode == ICB_FLAG_AD_SHORT ? sizeof(SHORT_AD) : sizeof(LONG_AD);
    for (i = 0; map[i].extLength & UDF_EXTENT_LENGTH_MASK; i++) {
        uint32 bytes = map[i].extLength & UDF_EXTENT_LENGTH_MASK;
        if (i == UDF_COW_MAX_RUNS || (map[i].extLength >> 30) != EXTENT_RECORDED_ALLOCATED ||
            (bytes & (Vcb->SectorSize - 1)) || !map[i].extLocation)
            return FALSE;
        Runs[i].Lbn = lbn;
        Runs[i].Blocks = bytes >> Vcb->SectorShift;
        Runs[i].Lba[0] = map[i].extLocation;
        Runs[i].Lba[1] = 0;
        lbn += Runs[i].Blocks;
    }
    *Count = i;
    return i && FileInfo->Dloc->FileEntryLen + i * adSize <= Vcb->SectorSize;
}

/* Mark Blocks at Lba allocated in the on-disk space bitmap now, so the
 * second region is never seen as free once a commit refers to it. */
static NTSTATUS
CowMarkOnDisk(PIRP_CONTEXT IrpContext, PVCB Vcb, uint32 Lba, uint32 Blocks, int8* Sector)
{
    uint32 ref = Vcb->MetadataPhysRef;
    uint32 root = Vcb->Partitions[ref].PartitionRoot;
    uint32 bitmapLba = root + Vcb->Partitions[ref].UspaceBitmap;
    uint32 bit = Lba - root, last = ~0u;
    NTSTATUS status = STATUS_SUCCESS;

    for (; Blocks; Blocks--, bit++) {
        uint32 byte = 24 + (bit >> 3);
        uint32 sector = bitmapLba + byte / Vcb->SectorSize;
        if (sector != last) {
            if (last != ~0u) {
                status = CowRaw(IrpContext, Vcb, last, 1, Sector, TRUE);
                if (!NT_SUCCESS(status))
                    return status;
            }
            status = CowRaw(IrpContext, Vcb, sector, 1, Sector, FALSE);
            if (!NT_SUCCESS(status))
                return status;
            last = sector;
        }
        Sector[byte % Vcb->SectorSize] &= (uint8)~(1 << (bit & 7));
    }
    if (last != ~0u)
        status = CowRaw(IrpContext, Vcb, last, 1, Sector, TRUE);
    return status;
}

NTSTATUS
UDFCowStart(
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb
    )
{
    PUDF_COW cow;
    NTSTATUS status = STATUS_NOT_SUPPORTED;
    uint32 i, u, ref, align;
    int8* sector = NULL;
    HANDLE thread;
    BOOLEAN bitmapUsable;

    ref = Vcb->MetadataPhysRef;
    if (Vcb->Cow || Vcb->VcbCondition != VcbMounted ||
        FlagOn(Vcb->VcbState, VCB_STATE_VOLUME_READ_ONLY | UDF_VCB_FLAGS_DEAD) ||
        !Vcb->MetadataFileInfo || !Vcb->MetadataMirrorFileInfo || !Vcb->MetadataBitmap ||
        Vcb->SectorSize != 512 || Vcb->CDR_Mode || Vcb->SparingTable ||
        ref >= Vcb->PartitionMaps || !Vcb->MetadataBitCount)
        return STATUS_NOT_SUPPORTED;
    bitmapUsable = Vcb->Partitions[ref].UspaceBitmap != 0xFFFFFFFF &&
                   Vcb->Partitions[ref].UspaceBitmap < Vcb->Partitions[ref].PartitionLen;

    cow = (PUDF_COW)MyAllocatePool__(NonPagedPool, sizeof(UDF_COW));
    if (!cow)
        return STATUS_INSUFFICIENT_RESOURCES;
    RtlZeroMemory(cow, sizeof(UDF_COW));
    cow->Duplicated = (Vcb->MetadataFlags & UDF_METADATA_DUPLICATED) != 0;
    if (!CowBuildRuns(Vcb, Vcb->MetadataFileInfo, cow->Main, &cow->MainRuns) ||
        (cow->Duplicated ?
            !CowBuildRuns(Vcb, Vcb->MetadataMirrorFileInfo, cow->Mirror, &cow->MirrorRuns) :
            Vcb->MetadataMirrorFileInfo->Dloc->FileEntryLen + cow->MainRuns * sizeof(LONG_AD) >
                Vcb->SectorSize))
        goto fail;
    cow->MetadataBlocks = Vcb->MetadataBitCount;
    if (cow->Main[cow->MainRuns - 1].Lbn + cow->Main[cow->MainRuns - 1].Blocks < cow->MetadataBlocks)
        goto fail;
    cow->UnitBlocks = UDF_COW_UNIT_BLOCKS;
    cow->UnitBytes = UDF_COW_UNIT_BLOCKS << Vcb->SectorShift;
    cow->Units = (cow->MetadataBlocks + cow->UnitBlocks - 1) / cow->UnitBlocks;
    cow->Buffer = (int8**)MyAllocatePool__(NonPagedPool, cow->Units * sizeof(int8*));
    cow->Stale = (uint8*)MyAllocatePool__(NonPagedPool, (cow->Units + 7) / 8);
    sector = (int8*)MyAllocatePool__(NonPagedPool, Vcb->SectorSize);
    if (!cow->Buffer || !cow->Stale || !sector) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto fail;
    }
    RtlZeroMemory(cow->Buffer, cow->Units * sizeof(int8*));
    RtlZeroMemory(cow->Stale, (cow->Units + 7) / 8);

    /* Second region: one aligned run per extent, the same size. */
    align = Vcb->MetadataAlignUnit ? Vcb->MetadataAlignUnit : 1;
    for (i = 0; i < cow->MainRuns + (cow->Duplicated ? cow->MirrorRuns : 0); i++) {
        PUDF_COW_RUN run = i < cow->MainRuns ? &cow->Main[i] : &cow->Mirror[i - cow->MainRuns];
        status = UDFAllocAlignedRun(IrpContext, Vcb, run->Blocks, align,
                                    &run->Lba[1], &cow->AltMaps[cow->AltMapCount]);
        if (!NT_SUCCESS(status))
            goto fail;
        cow->AltMapCount++;
        if (bitmapUsable) {
            status = CowMarkOnDisk(IrpContext, Vcb, run->Lba[1], run->Blocks, sector);
            if (!NT_SUCCESS(status))
                goto fail;
        }
    }
    status = UDFBarrierFlushDevice(Vcb->TargetDeviceObject);
    if (!NT_SUCCESS(status))
        goto fail;

    /* The second region holds nothing yet: every unit with a block in use
     * must be copied by the first checkpoint. */
    UDFAcquireResourceExclusive(&(Vcb->BitMapResource1), TRUE);
    for (u = 0; u < cow->Units; u++) {
        uint32 b, first = u * cow->UnitBlocks, count = CowUnitCount(cow, u);
        for (b = first; b < first + count; b++) {
            if (!(Vcb->MetadataBitmap[b >> 3] & (1 << (b & 7)))) {
                cow->Stale[u >> 3] |= (uint8)(1 << (u & 7));
                break;
            }
        }
    }
    UDFReleaseResource(&(Vcb->BitMapResource1));

    KeInitializeEvent(&cow->Wake, SynchronizationEvent, FALSE);
    UDFAcquireResourceExclusive(&(Vcb->BitMapResource1), TRUE);
    CowLock(Vcb, TRUE);
    Vcb->Cow = cow;
    CowUnlock(Vcb);
    UDFReleaseResource(&(Vcb->BitMapResource1));

    status = PsCreateSystemThread(&thread, THREAD_ALL_ACCESS, NULL, NULL, NULL, CowThread, Vcb);
    if (NT_SUCCESS(status)) {
        status = ObReferenceObjectByHandle(thread, THREAD_ALL_ACCESS, NULL, KernelMode,
                                           &cow->Thread, NULL);
        ZwClose(thread);
    }
    if (!NT_SUCCESS(status)) {
        /* Without the thread, checkpoints still run on every flush. */
        cow->Thread = NULL;
        status = STATUS_SUCCESS;
    }
    MyFreePool__(sector);
    UDFPrint(("UDF COW: on, %lu metadata blocks in %lu+%lu extents\n",
              cow->MetadataBlocks, cow->MainRuns, cow->MirrorRuns));
    return STATUS_SUCCESS;

fail:
    for (i = 0; i < cow->AltMapCount; i++)
        UDFMarkSpaceAsXXX(Vcb, 0, cow->AltMaps[i], AS_DISCARDED);
    if (sector)
        MyFreePool__(sector);
    UDFPrint(("UDF COW: not used for this volume (%lx)\n", status));
    CowFree(cow);
    return status;
}

/*
 * Commit what is buffered, move the disk back to the home region, free the
 * second region and turn copy-on-write off. Used before a full volume
 * flush and before the Metadata File grows, since both rewrite its FEs.
 */
NTSTATUS
UDFCowStop(
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb
    )
{
    PUDF_COW cow = Vcb->Cow;
    PUDF_COW_FREE all;
    NTSTATUS status;
    int8* scratch;
    uint32 u, i;

    if (!cow)
        return STATUS_SUCCESS;
    if (ExIsResourceAcquiredExclusiveLite(&Vcb->BitMapResource1))
        return STATUS_CANT_WAIT;
    CowStopThread(cow);

    status = UDFCowCheckpoint(IrpContext, Vcb, FALSE);
    if (!NT_SUCCESS(status))
        return status;

    CowLock(Vcb, TRUE);
    if (cow->Region != 0) {
        scratch = (int8*)MyAllocatePool__(NonPagedPool, cow->UnitBytes);
        status = scratch ? STATUS_SUCCESS : STATUS_INSUFFICIENT_RESOURCES;
        for (u = 0; u < cow->Units && NT_SUCCESS(status); u++) {
            if (!CowStale(cow, u))
                continue;
            status = CowLoadUnit(IrpContext, Vcb, cow, u, scratch);
            if (NT_SUCCESS(status))
                status = CowStoreUnit(IrpContext, Vcb, cow, u, 0, scratch);
        }
        if (scratch)
            MyFreePool__(scratch);
        if (NT_SUCCESS(status))
            status = CowCommit(IrpContext, Vcb, cow, 0);
        if (cow->Region != 0) {
            CowUnlock(Vcb);
            return NT_SUCCESS(status) ? STATUS_UNSUCCESSFUL : status;
        }
    }
    CowUnlock(Vcb);

    UDFAcquireResourceExclusive(&(Vcb->BitMapResource1), TRUE);
    CowLock(Vcb, TRUE);
    Vcb->Cow = NULL;
    CowUnlock(Vcb);
    all = CowJoin(cow->Waiting, cow->Pending);
    cow->Waiting = cow->Pending = NULL;
    UDFReleaseResource(&(Vcb->BitMapResource1));

    CowApplyFrees(Vcb, NULL, all);
    for (i = 0; i < cow->AltMapCount; i++)
        UDFMarkSpaceAsXXX(Vcb, 0, cow->AltMaps[i], AS_DISCARDED);
    UDFPrint(("UDF COW: off after %lu checkpoints\n", cow->Checkpoints));
    CowFree(cow);
    return status;
}

/* Volume teardown: no I/O is possible any more. */
VOID
UDFCowTeardown(
    IN PVCB Vcb
    )
{
    PUDF_COW cow = Vcb->Cow;

    if (!cow)
        return;
    CowStopThread(cow);
    Vcb->Cow = NULL;
    CowFree(cow);
}
