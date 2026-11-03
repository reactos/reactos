////////////////////////////////////////////////////////////////////
// Copyright (C) Alexander Telyatnikov, Ivan Keliukh, Yegor Anchishkin, SKIF Software, 1999-2013. Kiev, Ukraine
// All rights reserved
// This file was released under the GPLv2 on June 2015.
////////////////////////////////////////////////////////////////////
/*************************************************************************
*
* File: deviosup.cpp
*
* Module: UDF File System Driver (Kernel mode execution only)
*
* Description:
*   Non-cached I/O support.  Builds arrays of disk runs from extent
*   mappings and dispatches them to the lower driver, reusing the
*   original IRP where possible to avoid bounce-buffer overhead.
*
*************************************************************************/

#include "udffs.h"

#define BugCheckFileId  (UDFS_BUG_CHECK_DEVIOSUP)

// -----------------------------------------------------------------------
//  Sector arithmetic helpers (match the reference driver style)
// -----------------------------------------------------------------------

#define UdfSectorAlign(V, L)        (((L) + ((V)->SectorSize - 1)) & ~((V)->SectorSize - 1))
#define UdfSectorTruncate(V, L)     ((L) & ~((V)->SectorSize - 1))
#define UdfLlSectorTruncate(V, L)   (((LONGLONG)(L)) & ~(((LONGLONG)(V)->SectorSize) - 1))
#define UdfSectorOffset(V, L)       ((ULONG)((L) & ((V)->SectorSize - 1)))
#define UdfSectorSize(V)            ((V)->SectorSize)

#define UDFAllocateIoContext()      \
    FsRtlAllocatePoolWithTag(NonPagedPool, sizeof(UDF_IO_CONTEXT), TAG_IO_CONTEXT)

// -----------------------------------------------------------------------
//  Forward declarations
// -----------------------------------------------------------------------

static
BOOLEAN
UDFLookupAllocation(
    _In_ PIRP_CONTEXT IrpContext,
    _In_ PFCB Fcb,
    _In_ LONGLONG FileOffset,
    _Out_ PLONGLONG DiskOffset,
    _Out_ PULONG ByteCount
    );

static
BOOLEAN
UDFPrepareBuffers(
    _In_ PIRP_CONTEXT IrpContext,
    _In_ PIRP Irp,
    _In_ PFCB Fcb,
    _In_ PVOID UserBuffer,
    _In_ ULONG UserBufferOffset,
    _In_ LONGLONG StartingOffset,
    _In_ ULONG ByteCount,
    _Inout_ PIO_RUN IoRuns,
    _Out_ PULONG RunCount,
    _Out_ PULONG ThisByteCount,
    _Out_ PBOOLEAN SparseRuns
    );

static
VOID
UDFSingleAsync(
    _In_ PIRP_CONTEXT IrpContext,
    _In_ LONGLONG ByteOffset,
    _In_ ULONG ByteCount
    );

static
VOID
UDFMultipleAsync(
    _In_ PIRP_CONTEXT IrpContext,
    _In_ ULONG RunCount,
    _In_ PIO_RUN IoRuns
    );

static
VOID
UDFWaitSync(
    _In_ PIRP_CONTEXT IrpContext
    );

static
BOOLEAN
UDFFinishBuffers(
    _In_ PIRP_CONTEXT IrpContext,
    _In_ PIO_RUN IoRuns,
    _In_ ULONG RunCount,
    _In_ BOOLEAN FinalCleanup
    );

static
NTSTATUS
UDFConvertToRecorded(
    _In_ PIRP_CONTEXT IrpContext,
    _In_ PFCB Fcb,
    _In_ LONGLONG StartingOffset,
    _In_ ULONG ByteCount
    );

IO_COMPLETION_ROUTINE UDFSingleSyncCompletionRoutine;
IO_COMPLETION_ROUTINE UDFSingleAsyncCompletionRoutine;
IO_COMPLETION_ROUTINE UDFMultiSyncCompletionRoutine;

// -----------------------------------------------------------------------
//  Public entry points
// -----------------------------------------------------------------------

/*************************************************************************
*
* Function: UDFNonCachedIo()
*
* Description:
*   Perform non-cached I/O (read or write) by building IO_RUN arrays
*   from the file's extent mapping and dispatching them to the lower
*   driver.  Direction is determined from IrpContext->MajorFunction.
*
*   For a single aligned contiguous extent the original IRP is reused
*   directly (UDFSingleAsync).  For fragmented or unaligned extents
*   associated IRPs with partial MDLs are used (UDFMultipleAsync).
*
*************************************************************************/

NTSTATUS
UDFNonCachedIo(
    _In_ PIRP_CONTEXT IrpContext,
    _In_ PFCB Fcb,
    _In_ LONGLONG StartingOffset,
    _In_ ULONG ByteCount
    )
{
    NTSTATUS Status = STATUS_SUCCESS;

    struct UDF_IO_RUN IoRuns[UDF_MAX_PARALLEL_IOS];
    ULONG RunCount = 0;
    ULONG CleanupRunCount = 0;

    PVOID UserBuffer;
    ULONG UserBufferOffset = 0;
    LONGLONG CurrentOffset = StartingOffset;
    ULONG RemainingByteCount;
    ULONG ThisByteCount;

    BOOLEAN Unaligned;
    BOOLEAN SparseRuns;
    BOOLEAN FlushIoBuffers = FALSE;
    BOOLEAN FirstPass = TRUE;
    BOOLEAN IsWrite = (IrpContext->MajorFunction == IRP_MJ_WRITE);

    //  Validate preconditions.

    ASSERT(IrpContext->Irp->MdlAddress != NULL);

    // I/O when (volume offset == device offset) must be sector aligned.
    if (Fcb == Fcb->Vcb->VolumeDasdFcb) 
    {
        if (UdfSectorOffset(Fcb->Vcb, StartingOffset) ||
            UdfSectorOffset(Fcb->Vcb, ByteCount)
        ) 
        {
            return STATUS_INVALID_PARAMETER;
        }
        IrpContext->Vcb = Fcb->Vcb;

        UDFSingleAsync(IrpContext, StartingOffset, ByteCount);
        if (FlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT))
        {
            UDFWaitSync(IrpContext);
            return IrpContext->Irp->IoStatus.Status;
        }
        
        ClearFlag(IrpContext->Flags, IRP_CONTEXT_FLAG_ALLOC_IO);
        return STATUS_PENDING;
    }

    //  For writes with a sub-sector tail (e.g. paging I/O truncated to
    //  FileSize), round up to a full sector.  The MDL covers whole pages
    //  and MM zero-fills beyond FileSize, so the extra bytes are zeros.

    if (IsWrite && UdfSectorOffset(Fcb->Vcb, ByteCount)) {
        ByteCount = UdfSectorAlign(Fcb->Vcb, ByteCount);
    }

    RemainingByteCount = ByteCount;

    //  Get the data extent info.

    PUDF_FILE_INFO FileInfo = Fcb->FileInfo;
    if (!FileInfo || !FileInfo->Dloc) {
        return STATUS_DRIVER_INTERNAL_ERROR;
    }

    PEXTENT_INFO ExtInfo = &FileInfo->Dloc->DataLoc;

    //  Embedded (in-ICB) data must be handled by the caller before
    //  reaching this point.  Assert that we have a real disk mapping.

    ASSERT(ExtInfo->Offset == 0 && ExtInfo->Mapping != NULL);

    //  Get the mapped system address for the user buffer.

    UserBuffer = UDFMapUserBuffer(IrpContext->Irp);
    if (!UserBuffer) {
        return STATUS_INVALID_USER_BUFFER;
    }

    //  Make sure IrpContext->Vcb is set for the lower-level routines.

    PVCB Vcb = Fcb->Vcb;
    IrpContext->Vcb = Vcb;

    //
    //  For writes: pre-scan extents and convert NOT_RECORDED to RECORDED
    //  if needed.  This must happen before building I/O runs.
    //

    if (IsWrite) {

        LONGLONG ScanOffset = StartingOffset;
        ULONG ScanRemaining = ByteCount;
        ULONG ScanUserOffset = 0;
        BOOLEAN NeedConversion = FALSE;
        LONGLONG DiskOffset;
        ULONG CurrentByteCount;

        while (ScanRemaining != 0) {

            BOOLEAN Recorded = UDFLookupAllocation(IrpContext, Fcb,
                                                   ScanOffset,
                                                   &DiskOffset,
                                                   &CurrentByteCount);

            if (CurrentByteCount > ScanRemaining) {
                CurrentByteCount = ScanRemaining;
            }

            // A failed extension can leave a cached paging write past the
            // last extent.  UDFLookupAllocation then returns zero bytes;
            // without this check the pre-scan repeats the same offset forever.
            if (CurrentByteCount == 0) {
                IrpContext->Irp->IoStatus.Information = 0;
                IrpContext->Irp->IoStatus.Status = STATUS_DISK_FULL;
                return STATUS_DISK_FULL;
            }

            if (!Recorded) {
                NeedConversion = TRUE;
            } else {
                ASSERT(!UdfSectorOffset(Vcb, DiskOffset));
            }

            ASSERT(!UdfSectorOffset(Vcb, ScanUserOffset));

            ScanRemaining -= CurrentByteCount;
            ScanOffset += CurrentByteCount;
            ScanUserOffset += CurrentByteCount;
        }

        if (NeedConversion) {

            ASSERT(!UdfSectorOffset(Vcb, ByteCount));

            Status = UDFConvertToRecorded(IrpContext, Fcb,
                                          StartingOffset, ByteCount);

            if (!NT_SUCCESS(Status)) {
                return Status;
            }
        }
    }

    //  The caller must have set up the IoContext before calling us.
    //  (Stack-allocated for sync, pool-allocated for async.)

    BOOLEAN Wait = BooleanFlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT);

    ASSERT(IrpContext->IoContext != NULL);

    _SEH2_TRY {

        //
        //  Loop while there are more bytes to transfer.
        //

        do {

            RtlZeroMemory(IoRuns, sizeof(IoRuns));

            //
            //  Build the next batch of I/O runs from extent mapping.
            //

            Unaligned = UDFPrepareBuffers(IrpContext,
                                          IrpContext->Irp,
                                          Fcb,
                                          UserBuffer,
                                          UserBufferOffset,
                                          CurrentOffset,
                                          RemainingByteCount,
                                          IoRuns,
                                          &CleanupRunCount,
                                          &ThisByteCount,
                                          &SparseRuns);

            RunCount = CleanupRunCount;

            //
            //  No runs — all sparse, we are done.
            //

            if (RunCount == 0) {

                Status = STATUS_SUCCESS;
                IrpContext->Irp->IoStatus.Status = STATUS_SUCCESS;
                IrpContext->Irp->IoStatus.Information = ByteCount;
                _SEH2_LEAVE;
            }

            //
            //  If the entire request fits in a single aligned run and this
            //  is the first (and only) pass, we can reuse the original IRP.
            //

            if (RunCount == 1 && !Unaligned && !SparseRuns && FirstPass) {

                UDFSingleAsync(IrpContext,
                               IoRuns[0].DiskOffset,
                               IoRuns[0].DiskByteCount);

                CleanupRunCount = 0;

                if (Wait) {

                    UDFWaitSync(IrpContext);
                    Status = IrpContext->Irp->IoStatus.Status;

                } else {

                    //  Async — the completion routine will release
                    //  the resource and complete the IRP.

                    ClearFlag(IrpContext->Flags, IRP_CONTEXT_FLAG_ALLOC_IO);
                    Status = STATUS_PENDING;
                }

                _SEH2_LEAVE;
            }

            //
            //  Multiple runs or unaligned require synchronous wait.
            //  If the caller can't wait, post the request.
            //

            if (!Wait) {

                UDFRaiseStatus(IrpContext, STATUS_CANT_WAIT);
            }

            UDFMultipleAsync(IrpContext, RunCount, IoRuns);

            //
            //  Wait for all associated IRPs to complete.
            //

            UDFWaitSync(IrpContext);

            Status = IrpContext->Irp->IoStatus.Status;

            if (!NT_SUCCESS(Status)) {

                _SEH2_LEAVE;
            }

            //
            //  Post-I/O: copy data from scratch/aux buffers to user buffer
            //  (reads only — unaligned sectors need partial-sector extraction).
            //

            if (Unaligned &&
                UDFFinishBuffers(IrpContext, IoRuns, RunCount, FALSE)) {

                FlushIoBuffers = TRUE;
            }

            CleanupRunCount = 0;

            //
            //  Advance to the next batch.
            //

            RemainingByteCount -= ThisByteCount;
            CurrentOffset += ThisByteCount;
            UserBuffer = Add2Ptr(UserBuffer, ThisByteCount, PVOID);
            UserBufferOffset += ThisByteCount;

            FirstPass = FALSE;

        } while (RemainingByteCount != 0);

        //
        //  All bytes transferred — update the master IRP.
        //

        IrpContext->Irp->IoStatus.Status = STATUS_SUCCESS;
        IrpContext->Irp->IoStatus.Information = ByteCount;

        //
        //  Flush the hardware cache if we performed any copy operations.
        //

        if (FlushIoBuffers) {

            KeFlushIoBuffers(IrpContext->Irp->MdlAddress, !IsWrite, FALSE);
        }

    } _SEH2_FINALLY {

        //
        //  Perform final cleanup on the IoRuns if necessary.
        //

        if (CleanupRunCount != 0) {

            UDFFinishBuffers(IrpContext, IoRuns, CleanupRunCount, TRUE);
        }

    } _SEH2_END;

    return Status;
}

// -----------------------------------------------------------------------
//  Local support routines
// -----------------------------------------------------------------------

/*************************************************************************
*
* Function: UDFCanUseDirectIo()
*
* Description:
*   Returns TRUE if the request can use the direct IRP dispatch path.
*
*************************************************************************/

/*************************************************************************
*
* Function: UDFLookupAllocation()
*
* Description:
*   Wrapper around UDFExtentOffsetToLba — maps a file byte offset
*   to a disk byte offset and contiguous byte count.
*   Returns TRUE if the extent is recorded, FALSE if sparse/unrecorded.
*
*************************************************************************/

static
BOOLEAN
UDFLookupAllocation(
    _In_ PIRP_CONTEXT IrpContext,
    _In_ PFCB Fcb,
    _In_ LONGLONG FileOffset,
    _Out_ PLONGLONG DiskOffset,
    _Out_ PULONG ByteCount
    )
{
    PVCB Vcb = Fcb->Vcb;
    PUDF_FILE_INFO FileInfo = Fcb->FileInfo;
    PEXTENT_INFO ExtInfo = &FileInfo->Dloc->DataLoc;
    PEXTENT_MAP Extent = ExtInfo->Mapping;

    uint32 SectorOff, Flags, Index;
    SIZE_T AvailLength;

    UNREFERENCED_PARAMETER(IrpContext);

    uint32 Lba = UDFExtentOffsetToLba(Vcb, Extent, FileOffset,
                                      &SectorOff, &AvailLength, &Flags, &Index);

    if (Lba == LBA_OUT_OF_EXTENT) {

        //  Past end of extent — treat as unrecorded.

        *DiskOffset = 0;
        *ByteCount = 0;
        return FALSE;
    }

    *ByteCount = (ULONG)AvailLength;

    if (Flags != EXTENT_RECORDED_ALLOCATED) {

        //  Unrecorded / not-allocated — sparse.

        *DiskOffset = 0;
        return FALSE;
    }

    //  Apply sector relocation (sparing table / VAT).

    uint32 BlockCount = (uint32)(AvailLength >> Vcb->SectorShift);

    PEXTENT_MAP RelocExtent = UDFRelocateSectors(Vcb, Lba, BlockCount);
    if (RelocExtent == NULL) {

        //  Allocation failure — fall back to buffered path.

        *DiskOffset = 0;
        *ByteCount = 0;
        return FALSE;
    }
    if (RelocExtent != UDF_NO_EXTENT_MAP) {

        //  Sectors were relocated — use the first fragment only;
        //  the caller will re-enter for subsequent fragments.

        Lba = RelocExtent->extLocation;
        AvailLength = RelocExtent->extLength;
        *ByteCount = (ULONG)AvailLength;
        MyFreePool__(RelocExtent);
    }

    //  Fixed-packet address translation (no-op on non-FP media).

    Lba = UDFFixFPAddress(Vcb, Lba);

    //  Recorded extent — compute the disk byte offset including
    //  any sub-sector offset.

    *DiskOffset = (((LONGLONG)Lba) << Vcb->SectorShift) + SectorOff;

    return TRUE;
}

/*************************************************************************
*
* Function: UDFConvertToRecorded()
*
* Description:
*   Pre-pass for writes: converts NOT_RECORDED_NOT_ALLOCATED and
*   NOT_RECORDED_ALLOCATED extents in the given range to
*   RECORDED_ALLOCATED so that direct I/O dispatch can proceed.
*
*   The caller must ensure the write range is fully sector-aligned
*   (no partial-sector zero padding is performed here).
*
*   Note: this may reallocate the extent mapping (ExtInfo->Mapping).
*
*************************************************************************/

static
NTSTATUS
UDFConvertToRecorded(
    _In_ PIRP_CONTEXT IrpContext,
    _In_ PFCB Fcb,
    _In_ LONGLONG StartingOffset,
    _In_ ULONG ByteCount
    )
{
    NTSTATUS Status;
    PVCB Vcb = Fcb->Vcb;
    PEXTENT_INFO ExtInfo = &Fcb->FileInfo->Dloc->DataLoc;
    PEXTENT_MAP Extent;

    LONGLONG CurrentOffset = StartingOffset;
    ULONG Remaining = ByteCount;

    uint32 SectorOff, Flags, Index;
    SIZE_T AvailLength;
    ULONG ThisByteCount;

    while (Remaining != 0) {

        Extent = ExtInfo->Mapping;

        UDFExtentOffsetToLba(Vcb, Extent, CurrentOffset,
                             &SectorOff, &AvailLength, &Flags, &Index);

        ThisByteCount = (ULONG)AvailLength;

        if (ThisByteCount > Remaining) {
            ThisByteCount = Remaining;
        }

        if (Flags == EXTENT_NOT_RECORDED_NOT_ALLOCATED) {

            Status = UDFMarkNotAllocatedAsAllocated(IrpContext, Vcb,
                         CurrentOffset, ThisByteCount, ExtInfo);

            if (!NT_SUCCESS(Status)) {
                return Status;
            }

            //  Mapping pointer may have changed — re-read flags.

            Extent = ExtInfo->Mapping;

            UDFExtentOffsetToLba(Vcb, Extent, CurrentOffset,
                                 &SectorOff, &AvailLength, &Flags, &Index);
        }

        if (Flags == EXTENT_NOT_RECORDED_ALLOCATED) {

            Status = UDFMarkAllocatedAsRecorded(Vcb,
                         CurrentOffset, ThisByteCount, ExtInfo);

            if (!NT_SUCCESS(Status)) {
                return Status;
            }
        }

        Remaining -= ThisByteCount;
        CurrentOffset += ThisByteCount;
    }

    return STATUS_SUCCESS;
}

/*************************************************************************
*
* Function: UDFPrepareBuffers()
*
* Description:
*   Builds the IO_RUN array for the next batch of I/O.  Maps file
*   offsets to disk offsets, handles sparse extents (zeroing the user
*   buffer), aligned extents (using the user buffer directly), and
*   unaligned extents (using scratch space in the user buffer or an
*   allocated auxiliary buffer).
*
*   Returns TRUE if any unaligned entries were added.
*
*************************************************************************/

static
BOOLEAN
UDFPrepareBuffers(
    _In_ PIRP_CONTEXT IrpContext,
    _In_ PIRP Irp,
    _In_ PFCB Fcb,
    _In_ PVOID UserBuffer,
    _In_ ULONG UserBufferOffset,
    _In_ LONGLONG StartingOffset,
    _In_ ULONG ByteCount,
    _Inout_ PIO_RUN IoRuns,
    _Out_ PULONG RunCount,
    _Out_ PULONG ThisByteCount,
    _Out_ PBOOLEAN SparseRuns
    )
{
    PVCB Vcb = Fcb->Vcb;

    BOOLEAN Recorded;
    BOOLEAN FoundUnaligned = FALSE;
    PIO_RUN ThisIoRun = IoRuns;

    ULONG RemainingByteCount = ByteCount;
    LONGLONG CurrentFileOffset = StartingOffset;

    PVOID CurrentUserBuffer = UserBuffer;
    ULONG CurrentUserBufferOffset = UserBufferOffset;

    PVOID ScratchUserBuffer = UserBuffer;
    ULONG ScratchUserBufferOffset = UserBufferOffset;

    LONGLONG DiskOffset;
    ULONG CurrentByteCount;

    *RunCount = 0;
    *ThisByteCount = 0;
    *SparseRuns = FALSE;

    //
    //  Loop while there are more bytes to process and available
    //  entries in the IoRun array.
    //

    while (TRUE) {

        *RunCount += 1;

        ThisIoRun->UserBuffer = CurrentUserBuffer;

        //
        //  Map the current file offset to a disk offset.
        //

        Recorded = UDFLookupAllocation(IrpContext,
                                       Fcb,
                                       CurrentFileOffset,
                                       &DiskOffset,
                                       &CurrentByteCount);

        //
        //  Limit to the data requested.
        //

        if (CurrentByteCount > RemainingByteCount) {
            CurrentByteCount = RemainingByteCount;
        }

        // Every run must advance the request.  A zero-length lookup here
        // would otherwise loop forever for both reads and writes.
        if (CurrentByteCount == 0) {
            ExRaiseStatus(STATUS_FILE_CORRUPT_ERROR);
        }

        //
        //  Handle unrecorded (sparse) data.
        //

        if (!Recorded) {

            *RunCount -= 1;

            RtlZeroMemory(CurrentUserBuffer, CurrentByteCount);
            *SparseRuns = TRUE;

            ScratchUserBuffer = Add2Ptr(CurrentUserBuffer,
                                        CurrentByteCount, PVOID);
            ScratchUserBufferOffset += CurrentByteCount;

        //
        //  Handle unaligned transfers.
        //
        //  Unaligned if:
        //    - Disk offset not sector-aligned, OR
        //    - Buffer offset not sector-aligned, OR
        //    - Byte count not sector-aligned AND less than one sector
        //

        } else if (UdfSectorOffset(Vcb, DiskOffset) ||
                   UdfSectorOffset(Vcb, CurrentUserBufferOffset) ||
                   (UdfSectorOffset(Vcb, CurrentByteCount) &&
                    CurrentByteCount < UdfSectorSize(Vcb))) {

            //
            //  Unaligned I/O requires synchronous operation.
            //

            ASSERT(FlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT));

            ThisIoRun->TransferBufferOffset = UdfSectorOffset(Vcb, DiskOffset);

            ThisIoRun->DiskOffset = UdfLlSectorTruncate(Vcb, DiskOffset);

            //
            //  Strategy A: use earlier scratch space in the user buffer.
            //

            if ((ScratchUserBufferOffset + ThisIoRun->TransferBufferOffset < CurrentUserBufferOffset) &&
                (ThisIoRun->TransferBufferOffset + CurrentByteCount >= UdfSectorSize(Vcb))) {

                ThisIoRun->DiskByteCount = UdfSectorTruncate(Vcb,
                    ThisIoRun->TransferBufferOffset + CurrentByteCount);
                CurrentByteCount = ThisIoRun->DiskByteCount - ThisIoRun->TransferBufferOffset;
                ThisIoRun->TransferByteCount = CurrentByteCount;

                ThisIoRun->TransferBuffer = ScratchUserBuffer;
                ThisIoRun->TransferMdl = Irp->MdlAddress;
                ThisIoRun->TransferVirtualAddress = Add2Ptr(Irp->UserBuffer,
                                                            ScratchUserBufferOffset,
                                                            PVOID);

                ScratchUserBuffer = Add2Ptr(ScratchUserBuffer,
                                            ThisIoRun->DiskByteCount, PVOID);
                ScratchUserBufferOffset += ThisIoRun->DiskByteCount;

            //
            //  Strategy B: allocate an auxiliary NonPagedPool buffer.
            //

            } else {

                ThisIoRun->DiskByteCount = UdfSectorAlign(Vcb,
                    ThisIoRun->TransferBufferOffset + CurrentByteCount);

                if (ThisIoRun->DiskByteCount > PAGE_SIZE) {
                    ThisIoRun->DiskByteCount = PAGE_SIZE;
                }

                if (ThisIoRun->TransferBufferOffset + CurrentByteCount > ThisIoRun->DiskByteCount) {
                    CurrentByteCount = ThisIoRun->DiskByteCount - ThisIoRun->TransferBufferOffset;
                }

                ThisIoRun->TransferByteCount = CurrentByteCount;

                ThisIoRun->TransferBuffer =
                    FsRtlAllocatePoolWithTag(NonPagedPool,
                                             PAGE_SIZE,
                                             TAG_IO_BUFFER);

                ThisIoRun->TransferMdl = IoAllocateMdl(ThisIoRun->TransferBuffer,
                                                       PAGE_SIZE,
                                                       FALSE,
                                                       FALSE,
                                                       NULL);

                ThisIoRun->TransferVirtualAddress = ThisIoRun->TransferBuffer;

                if (ThisIoRun->TransferMdl == NULL) {

                    IrpContext->Irp->IoStatus.Information = 0;
                    ExRaiseStatus(STATUS_INSUFFICIENT_RESOURCES);
                }

                MmBuildMdlForNonPagedPool(ThisIoRun->TransferMdl);
            }

            FoundUnaligned = TRUE;

        //
        //  Aligned transfer — use the user's buffer and MDL directly.
        //

        } else {

            CurrentByteCount = UdfSectorTruncate(Vcb, CurrentByteCount);

            ThisIoRun->DiskOffset = DiskOffset;
            ThisIoRun->DiskByteCount = CurrentByteCount;

            ThisIoRun->TransferBuffer = CurrentUserBuffer;
            ThisIoRun->TransferMdl = Irp->MdlAddress;
            ThisIoRun->TransferVirtualAddress = Add2Ptr(Irp->UserBuffer,
                                                        CurrentUserBufferOffset,
                                                        PVOID);

            ScratchUserBuffer = Add2Ptr(CurrentUserBuffer,
                                        CurrentByteCount, PVOID);
            ScratchUserBufferOffset += CurrentByteCount;
        }

        //
        //  Update position and check termination conditions.
        //

        RemainingByteCount -= CurrentByteCount;

        *ThisByteCount += CurrentByteCount;

        if (RemainingByteCount == 0 || *RunCount == UDF_MAX_PARALLEL_IOS) {
            break;
        }

        ThisIoRun = IoRuns + *RunCount;
        CurrentUserBuffer = Add2Ptr(CurrentUserBuffer, CurrentByteCount, PVOID);
        CurrentUserBufferOffset += CurrentByteCount;
        CurrentFileOffset += CurrentByteCount;
    }

    return FoundUnaligned;
}

/*************************************************************************
*
* Function: UDFFinishBuffers()
*
* Description:
*   Post-I/O processing for unaligned transfers.  Copies data from
*   scratch/auxiliary buffers to the final user buffer position,
*   frees allocated MDLs and buffers.
*
*   Walks the IoRun array BACKWARDS since scratch space may overlap.
*
*   Returns TRUE if any data was copied (caller should flush I/O buffers).
*
*************************************************************************/

static
BOOLEAN
UDFFinishBuffers(
    _In_ PIRP_CONTEXT IrpContext,
    _In_ PIO_RUN IoRuns,
    _In_ ULONG RunCount,
    _In_ BOOLEAN FinalCleanup
    )
{
    BOOLEAN FlushIoBuffers = FALSE;
    ULONG RemainingEntries = RunCount;
    PIO_RUN ThisIoRun = &IoRuns[RunCount - 1];

    while (RemainingEntries != 0) {

        //
        //  Handle unaligned transfers (TransferByteCount != 0).
        //

        if (ThisIoRun->TransferByteCount != 0) {

            if (!FinalCleanup) {

                //
                //  If using scratch space in user buffer, use MoveMemory
                //  (may overlap).  Otherwise use CopyMemory.
                //

                if (ThisIoRun->TransferMdl == IrpContext->Irp->MdlAddress) {

                    RtlMoveMemory(ThisIoRun->UserBuffer,
                                  Add2Ptr(ThisIoRun->TransferBuffer,
                                          ThisIoRun->TransferBufferOffset,
                                          PVOID),
                                  ThisIoRun->TransferByteCount);

                } else {

                    RtlCopyMemory(ThisIoRun->UserBuffer,
                                  Add2Ptr(ThisIoRun->TransferBuffer,
                                          ThisIoRun->TransferBufferOffset,
                                          PVOID),
                                  ThisIoRun->TransferByteCount);
                }

                FlushIoBuffers = TRUE;
            }

            //
            //  Free any MDL and buffer we allocated (not the original Irp's).
            //

            if (ThisIoRun->TransferMdl != IrpContext->Irp->MdlAddress) {

                if (ThisIoRun->TransferMdl != NULL) {
                    IoFreeMdl(ThisIoRun->TransferMdl);
                }

                if (ThisIoRun->TransferBuffer != NULL) {
                    ExFreePool(ThisIoRun->TransferBuffer);
                }
            }
        }

        //
        //  Clean up any associated IRP that was allocated but
        //  not yet sent (error path).
        //

        if (ThisIoRun->SavedIrp != NULL) {

            if (ThisIoRun->SavedIrp->MdlAddress != NULL) {
                IoFreeMdl(ThisIoRun->SavedIrp->MdlAddress);
            }

            IoFreeIrp(ThisIoRun->SavedIrp);
        }

        ThisIoRun -= 1;
        RemainingEntries -= 1;
    }

    return FlushIoBuffers;
}

/*************************************************************************
*
* Function: UDFSingleAsync()
*
* Description:
*   Sends a single contiguous aligned read/write by reusing the
*   original IRP.  Sets up a completion routine and dispatches
*   to the lower driver.
*
*************************************************************************/

static
VOID
UDFSingleAsync(
    _In_ PIRP_CONTEXT IrpContext,
    _In_ LONGLONG ByteOffset,
    _In_ ULONG ByteCount
    )
{
    PIO_STACK_LOCATION IrpSp;
    PIO_COMPLETION_ROUTINE CompletionRoutine;

    //
    //  Pick the right completion routine based on whether the caller
    //  can wait.  Sync signals the event; async releases resources
    //  and completes the IRP.
    //

    if (FlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT)) {

        CompletionRoutine = UDFSingleSyncCompletionRoutine;

    } else {

        CompletionRoutine = UDFSingleAsyncCompletionRoutine;
    }

    IrpContext->IoContext->ExpectedByteCount = ByteCount;
    IoSetCompletionRoutine(IrpContext->Irp,
                           CompletionRoutine,
                           IrpContext->IoContext,
                           TRUE,
                           TRUE,
                           TRUE);

    //
    //  Set up the next stack location for the lower driver.
    //

    IrpSp = IoGetNextIrpStackLocation(IrpContext->Irp);

    IrpSp->MajorFunction = IrpContext->MajorFunction;
    IrpSp->Parameters.Read.Length = ByteCount;
    IrpSp->Parameters.Read.ByteOffset.QuadPart = ByteOffset;
    IrpSp->Flags |= SL_OVERRIDE_VERIFY_VOLUME;
    if (IrpContext->MajorFunction == IRP_MJ_WRITE &&
        FlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_WRITE_THROUGH)) {
        IrpSp->Flags |= SL_WRITE_THROUGH;
    }

    //
    //  Issue the I/O request.
    //

    (VOID)IoCallDriver(IrpContext->Vcb->TargetDeviceObject, IrpContext->Irp);
}

/*************************************************************************
*
* Function: UDFMultipleAsync()
*
* Description:
*   Sends multiple I/O runs using associated IRPs with partial MDLs.
*   Each associated IRP is dispatched to the lower driver.
*   The completion routine tracks them and signals when all are done.
*
*************************************************************************/

static
VOID
UDFMultipleAsync(
    _In_ PIRP_CONTEXT IrpContext,
    _In_ ULONG RunCount,
    _In_ PIO_RUN IoRuns
    )
{
    PIO_STACK_LOCATION IrpSp;
    PMDL Mdl;
    PIRP Irp;
    PIRP MasterIrp;
    ULONG UnwindRunCount;

    MasterIrp = IrpContext->Irp;

    //
    //  Phase 1: Allocate associated IRPs and build partial MDLs.
    //  If any allocation fails, UDFFinishBuffers will clean up
    //  the ones already created (via SavedIrp).
    //

    for (UnwindRunCount = 0;
         UnwindRunCount < RunCount;
         UnwindRunCount++) {

        IoRuns[UnwindRunCount].SavedIrp =
        Irp = IoMakeAssociatedIrp(MasterIrp,
                (CCHAR)(IrpContext->Vcb->TargetDeviceObject->StackSize + 1));

        if (Irp == NULL) {

            IrpContext->Irp->IoStatus.Information = 0;
            ExRaiseStatus(STATUS_INSUFFICIENT_RESOURCES);
        }

        //
        //  Allocate and build a partial MDL for the request.
        //

        // `IoAllocateMdl` refuses buffers above about 64 MB on x86 and 32 MB on amd64 on ReactOS 
        // and NT 5.x, but `MmCreateMdl` doesn't have a cap like that. It doesn't attach the MDL 
        // to the IRP though so gotta do that manually.
        if (!(Mdl = IoAllocateMdl(IoRuns[UnwindRunCount].TransferVirtualAddress,
                            IoRuns[UnwindRunCount].DiskByteCount,
                            FALSE,
                            FALSE,
                            Irp))) {
            
            if (!(Mdl = MmCreateMdl(NULL, IoRuns[UnwindRunCount].TransferVirtualAddress, 
                                IoRuns[UnwindRunCount].DiskByteCount))) {

                IrpContext->Irp->IoStatus.Information = 0;
                ExRaiseStatus(STATUS_INSUFFICIENT_RESOURCES);
            }

            Irp->MdlAddress = Mdl;
        }

        IoBuildPartialMdl(IoRuns[UnwindRunCount].TransferMdl,
                          Mdl,
                          IoRuns[UnwindRunCount].TransferVirtualAddress,
                          IoRuns[UnwindRunCount].DiskByteCount);

        //
        //  Get the first IRP stack location in the associated IRP.
        //

        IoSetNextIrpStackLocation(Irp);
        IrpSp = IoGetCurrentIrpStackLocation(Irp);

        IrpSp->MajorFunction = IrpContext->MajorFunction;
        IrpSp->Parameters.Read.Length = IoRuns[UnwindRunCount].DiskByteCount;
        IrpSp->Parameters.Read.ByteOffset.QuadPart = IoRuns[UnwindRunCount].DiskOffset;

        //
        //  Set the completion routine.
        //

        IoSetCompletionRoutine(Irp,
                               UDFMultiSyncCompletionRoutine,
                               IrpContext->IoContext,
                               TRUE,
                               TRUE,
                               TRUE);

        //
        //  Set up the next stack location for the disk driver.
        //

        IrpSp = IoGetNextIrpStackLocation(Irp);

        IrpSp->MajorFunction = IrpContext->MajorFunction;
        IrpSp->Parameters.Read.Length = IoRuns[UnwindRunCount].DiskByteCount;
        IrpSp->Parameters.Read.ByteOffset.QuadPart = IoRuns[UnwindRunCount].DiskOffset;
        if (IrpContext->MajorFunction == IRP_MJ_WRITE &&
            FlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_WRITE_THROUGH)) {
            IrpSp->Flags |= SL_WRITE_THROUGH;
        }
    }

    //
    //  Initialise the I/O context counters.
    //

    IrpContext->IoContext->IrpCount = RunCount;
    IrpContext->IoContext->MasterIrp = MasterIrp;
    IrpContext->IoContext->Status = STATUS_SUCCESS;

    //
    //  Prevent the I/O system from completing the master IRP
    //  when the associated IRPs complete.
    //

    MasterIrp->AssociatedIrp.IrpCount = 1;

    //
    //  Phase 2: Dispatch all associated IRPs.
    //

    for (UnwindRunCount = 0;
         UnwindRunCount < RunCount;
         UnwindRunCount++) {

        Irp = IoRuns[UnwindRunCount].SavedIrp;
        IoRuns[UnwindRunCount].SavedIrp = NULL;

        (VOID)IoCallDriver(IrpContext->Vcb->TargetDeviceObject, Irp);
    }
}

/*************************************************************************
*
* Function: UDFWaitSync()
*
* Description:
*   Waits for the completion of one or more I/O requests started by
*   UDFSingleAsync or UDFMultipleAsync.
*
*************************************************************************/

static
VOID
UDFWaitSync(
    _In_ PIRP_CONTEXT IrpContext
    )
{
    KeWaitForSingleObject(&IrpContext->IoContext->SyncEvent,
                          Executive,
                          KernelMode,
                          FALSE,
                          NULL);

    KeClearEvent(&IrpContext->IoContext->SyncEvent);
}

// -----------------------------------------------------------------------
//  Completion routines
// -----------------------------------------------------------------------

/*************************************************************************
*
* Function: UDFSingleSyncCompletionRoutine()
*
* Description:
*   Completion routine for a single synchronous I/O (UDFSingleAsync).
*   Zeros Information on error, signals the event.
*
*************************************************************************/

NTSTATUS
NTAPI
UDFSingleSyncCompletionRoutine(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIRP Irp,
    _In_reads_opt_(_Inexpressible_("varies")) PVOID Context
    )
{
    UNREFERENCED_PARAMETER(DeviceObject);

    if (NT_SUCCESS(Irp->IoStatus.Status) &&
        Irp->IoStatus.Information != ((PUDF_IO_CONTEXT)Context)->ExpectedByteCount) {
        Irp->IoStatus.Status = STATUS_IO_DEVICE_ERROR;
    }
    if (!NT_SUCCESS(Irp->IoStatus.Status)) {
        Irp->IoStatus.Information = 0;
    }

    KeSetEvent(&((PUDF_IO_CONTEXT)Context)->SyncEvent, 0, FALSE);

    return STATUS_MORE_PROCESSING_REQUIRED;
}

/*************************************************************************
*
* Function: UDFSingleAsyncCompletionRoutine()
*
* Description:
*   Completion routine for a single asynchronous I/O (UDFSingleAsync).
*   Sets Information, marks the IRP pending, releases the FCB resource,
*   and frees the IoContext.  The I/O manager will complete the IRP.
*
*************************************************************************/

NTSTATUS
NTAPI
UDFSingleAsyncCompletionRoutine(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIRP Irp,
    _In_reads_opt_(_Inexpressible_("varies")) PVOID Context
    )
{
    PUDF_IO_CONTEXT IoContext = (PUDF_IO_CONTEXT)Context;

    UNREFERENCED_PARAMETER(DeviceObject);

    //
    //  Set the information field to the requested byte count on success,
    //  zero on error.
    //

    if (NT_SUCCESS(Irp->IoStatus.Status) &&
        Irp->IoStatus.Information != IoContext->ExpectedByteCount) {
        Irp->IoStatus.Status = STATUS_IO_DEVICE_ERROR;
    }
    Irp->IoStatus.Information = 0;

    if (NT_SUCCESS(Irp->IoStatus.Status)) {

        Irp->IoStatus.Information = IoContext->RequestedByteCount;
    }

    //
    //  Mark the IRP pending so the I/O manager will complete it.
    //

    IoMarkIrpPending(Irp);

    //
    //  Release the FCB resource that was held for the duration of the I/O.
    //

    ExReleaseResourceForThreadLite(IoContext->Resource,
                                   IoContext->ResourceThreadId);

    //
    //  Free the pool-allocated context.
    //

    if (IoContext->AllocatedContext) {
        ExFreePool(IoContext);
    }

    return STATUS_SUCCESS;
}

/*************************************************************************
*
* Function: UDFMultiSyncCompletionRoutine()
*
* Description:
*   Completion routine for associated IRPs (UDFMultipleAsync).
*   Records the first error, frees the MDL and IRP, and signals
*   the event when all associated IRPs have completed.
*
*************************************************************************/

NTSTATUS
NTAPI
UDFMultiSyncCompletionRoutine(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIRP Irp,
    _In_reads_opt_(_Inexpressible_("varies")) PVOID Context
    )
{
    PUDF_IO_CONTEXT IoContext = (PUDF_IO_CONTEXT)Context;

    UNREFERENCED_PARAMETER(DeviceObject);

    //
    //  Record the first error.
    //

    /* Our own stack retains this run's physical length while the lower
     * stack is used by the target device. Runs can complete out of order. */
    if (NT_SUCCESS(Irp->IoStatus.Status) &&
        Irp->IoStatus.Information !=
            IoGetCurrentIrpStackLocation(Irp)->Parameters.Read.Length) {
        Irp->IoStatus.Status = STATUS_IO_DEVICE_ERROR;
    }
    if (!NT_SUCCESS(Irp->IoStatus.Status)) {

        InterlockedCompareExchange(&IoContext->Status,
                                   Irp->IoStatus.Status, STATUS_SUCCESS);
        IoContext->MasterIrp->IoStatus.Information = 0;
    }

    //
    //  Free the MDL and IRP — IoCompleteRequest won't do this
    //  because we return STATUS_MORE_PROCESSING_REQUIRED.
    //

    IoFreeMdl(Irp->MdlAddress);
    IoFreeIrp(Irp);

    if (InterlockedDecrement(&IoContext->IrpCount) == 0) {

        //
        //  All associated IRPs done — propagate status to master.
        //

        IoContext->MasterIrp->IoStatus.Status = IoContext->Status;
        KeSetEvent(&IoContext->SyncEvent, 0, FALSE);
    }

    return STATUS_MORE_PROCESSING_REQUIRED;
}

/*

 Function: UDFPerformDevIoCtrl()

 Description:
    UDF FSD will invoke this rotine to send IOCTL's to physical
    device

 Return Value: STATUS_SUCCESS/Error

*/
NTSTATUS
UDFPerformDevIoCtrl(
    IN ULONG IoControlCode,
    IN PDEVICE_OBJECT DeviceObject,
    IN PVOID InputBuffer ,
    IN ULONG InputBufferLength,
    OUT PVOID OutputBuffer ,
    IN ULONG OutputBufferLength,
    IN BOOLEAN OverrideVerify,
    OUT PIO_STATUS_BLOCK Iosb OPTIONAL
    )
{
    NTSTATUS Status;
    KEVENT Event;
    PIRP Irp;
    IO_STATUS_BLOCK LocalIosb;
    PIO_STATUS_BLOCK IosbToUse = &LocalIosb;

    PAGED_CODE();

    // Check if the user gave us an Iosb.

    if (ARGUMENT_PRESENT(Iosb)) {

        IosbToUse = Iosb;
    }

    IosbToUse->Status = 0;
    IosbToUse->Information = 0;

    // Initialize the event.

    KeInitializeEvent(&Event, NotificationEvent, FALSE);

    // Attempt to allocate the IRP.  If unsuccessful, raise
    // STATUS_INSUFFICIENT_RESOURCES.

    Irp = IoBuildDeviceIoControlRequest(IoControlCode,
        DeviceObject,
        InputBuffer,
        InputBufferLength,
        OutputBuffer,
        OutputBufferLength,
        FALSE,
        &Event,
        IosbToUse);

    if (!Irp) {

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    if (OverrideVerify) {

        SetFlag(IoGetNextIrpStackLocation(Irp)->Flags, SL_OVERRIDE_VERIFY_VOLUME);
    }

    Status = IoCallDriver(DeviceObject, Irp);

    // We check for device not ready by first checking Status
    // and then if status pending was returned, the Iosb status
    // value.

    if (Status == STATUS_PENDING) {

        Status = KeWaitForSingleObject(&Event,
                                       Executive,
                                       KernelMode,
                                       FALSE,
                                       NULL);

        Status = IosbToUse->Status;
    }

    NT_ASSERT(!(OverrideVerify && (STATUS_VERIFY_REQUIRED == Status)));

    return Status;
} // end UDFPerformDevIoCtrl()

/*************************************************************************
*
* Paging file support. Its blocks are recorded once, when its size is set,
* so that paging writes never change metadata; its paging I/O then goes
* straight to those blocks without taking any file system lock, as the
* memory manager may page while other threads hold them.
*
*************************************************************************/

NTSTATUS
UDFPreparePagingFile(
    _In_ PIRP_CONTEXT IrpContext,
    _In_ PFCB Fcb
    )
{
    LONGLONG offset = 0, size = Fcb->Header.FileSize.QuadPart;
    NTSTATUS status = STATUS_SUCCESS;
    ULONG chunk;

    size = UdfSectorAlign(Fcb->Vcb, size);
    UDFAcquirePagingIoExclusive(IrpContext, Fcb);
    while (offset < size && NT_SUCCESS(status)) {
        chunk = (ULONG)min(size - offset, (LONGLONG)(64 * 1024 * 1024));
        status = UDFConvertToRecorded(IrpContext, Fcb, offset, chunk);
        offset += chunk;
    }
    UDFReleasePagingIo(IrpContext, Fcb);
    if (NT_SUCCESS(status))
        status = UDFFlushFile__(IrpContext, Fcb->Vcb, Fcb->FileInfo, 0);
    return status;
}

NTSTATUS
UDFPagingFileIo(
    _In_ PIRP_CONTEXT IrpContext,
    _In_ PFCB Fcb,
    _In_ LONGLONG StartingOffset,
    _In_ ULONG ByteCount
    )
{
    struct UDF_IO_CONTEXT ioContext;
    LONGLONG size = Fcb->Header.FileSize.QuadPart;
    NTSTATUS status;

    IrpContext->Irp->IoStatus.Information = 0;
    if (StartingOffset < 0 || StartingOffset >= size || !ByteCount)
        return StartingOffset < 0 ? STATUS_INVALID_PARAMETER : STATUS_SUCCESS;
    if (StartingOffset + ByteCount > size)
        ByteCount = (ULONG)UdfSectorAlign(Fcb->Vcb, size - StartingOffset);
    if (UdfSectorOffset(Fcb->Vcb, StartingOffset) || UdfSectorOffset(Fcb->Vcb, ByteCount))
        return STATUS_INVALID_PARAMETER;

    RtlZeroMemory(&ioContext, sizeof(ioContext));
    KeInitializeEvent(&ioContext.SyncEvent, NotificationEvent, FALSE);
    IrpContext->IoContext = &ioContext;
    ClearFlag(IrpContext->Flags, IRP_CONTEXT_FLAG_ALLOC_IO);
    SetFlag(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT);
    status = UDFNonCachedIo(IrpContext, Fcb, StartingOffset, ByteCount);
    IrpContext->IoContext = NULL;
    return status;
}
