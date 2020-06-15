/*++

Copyright (c) 1989-2000 Microsoft Corporation

Module Name:

    Write.c

Abstract:

    This module implements the File Write routine for Write called by the
    dispatch driver.


--*/

#include "fatprocs.h"

//
//  The Bug check file id for this module
//

#define BugCheckFileId                   (FAT_BUG_CHECK_WRITE)

//
//  The local debug trace level
//

#define Dbg                              (DEBUG_TRACE_WRITE)

//
//  Macros to increment the appropriate performance counters.
//

#define CollectWriteStats(VCB,OPEN_TYPE,BYTE_COUNT) {                                        \
    PFILESYSTEM_STATISTICS Stats = &(VCB)->Statistics[KeGetCurrentProcessorNumber() % FatData.NumberProcessors].Common; \
    if (((OPEN_TYPE) == UserFileOpen)) {                                                     \
        Stats->UserFileWrites += 1;                                                          \
        Stats->UserFileWriteBytes += (ULONG)(BYTE_COUNT);                                    \
    } else if (((OPEN_TYPE) == VirtualVolumeFile || ((OPEN_TYPE) == DirectoryFile))) {       \
        Stats->MetaDataWrites += 1;                                                          \
        Stats->MetaDataWriteBytes += (ULONG)(BYTE_COUNT);                                    \
    }                                                                                        \
}

BOOLEAN FatNoAsync = FALSE;

//
//  Local support routines
//

KDEFERRED_ROUTINE FatDeferredFlushDpc;

VOID
NTAPI
FatDeferredFlushDpc (
    _In_ PKDPC Dpc,
    _In_opt_ PVOID DeferredContext,
    _In_opt_ PVOID SystemArgument1,
    _In_opt_ PVOID SystemArgument2
    );

WORKER_THREAD_ROUTINE FatDeferredFlush;

VOID
NTAPI
FatDeferredFlush (
    _In_ PVOID Parameter
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, FatDeferredFlush)
#pragma alloc_text(PAGE, FatCommonWrite)
#endif


_Function_class_(IRP_MJ_WRITE)
_Function_class_(DRIVER_DISPATCH)
NTSTATUS
NTAPI
FatFsdWrite (
    _In_ PVOLUME_DEVICE_OBJECT VolumeDeviceObject,
    _Inout_ PIRP Irp
    )

/*++

Routine Description:

    This routine implements the FSD part of the NtWriteFile API call

Arguments:

    VolumeDeviceObject - Supplies the volume device object where the
        file being Write exists

    Irp - Supplies the Irp being processed

Return Value:

    NTSTATUS - The FSD status for the IRP

--*/

{
    PFCB Fcb;
    NTSTATUS Status;
    PIRP_CONTEXT IrpContext = NULL;

    BOOLEAN ModWriter = FALSE;
    BOOLEAN TopLevel = FALSE;

    DebugTrace(+1, Dbg, "FatFsdWrite\n", 0);

    //
    //  Call the common Write routine, with blocking allowed if synchronous
    //

    FsRtlEnterFileSystem();

    //
    //  We are first going to do a quick check for paging file IO.  Since this
    //  is a fast path, we must replicate the check for the fsdo.
    //

    if (!FatDeviceIsFatFsdo( IoGetCurrentIrpStackLocation(Irp)->DeviceObject))  {

        Fcb = (PFCB)(IoGetCurrentIrpStackLocation(Irp)->FileObject->FsContext);

        if ((NodeType(Fcb) == FAT_NTC_FCB) &&
            FlagOn(Fcb->FcbState, FCB_STATE_PAGING_FILE)) {

            //
            //  Do the usual STATUS_PENDING things.
            //

            IoMarkIrpPending( Irp );

            //
            //  Perform the actual IO, it will be completed when the io finishes.
            //

            FatPagingFileIo( Irp, Fcb );

            FsRtlExitFileSystem();

            return STATUS_PENDING;
        }
    }

    _SEH2_TRY {

        TopLevel = FatIsIrpTopLevel( Irp );

        IrpContext = FatCreateIrpContext( Irp, CanFsdWait( Irp ) );

        //
        //  This is a kludge for the mod writer case.  The correct state
        //  of recursion is set in IrpContext, however, we much with the
        //  actual top level Irp field to get the correct WriteThrough
        //  behaviour.
        //

        if (IoGetTopLevelIrp() == (PIRP)FSRTL_MOD_WRITE_TOP_LEVEL_IRP) {

            ModWriter = TRUE;

            IoSetTopLevelIrp( Irp );
        }

        //
        //  If this is an Mdl complete request, don't go through
        //  common write.
        //

        if (FlagOn( IrpContext->MinorFunction, IRP_MN_COMPLETE )) {

            DebugTrace(0, Dbg, "Calling FatCompleteMdl\n", 0 );
            Status = FatCompleteMdl( IrpContext, Irp );

        } else {

            Status = FatCommonWrite( IrpContext, Irp );
        }

    } _SEH2_EXCEPT(FatExceptionFilter( IrpContext, _SEH2_GetExceptionInformation() )) {

        //
        //  We had some trouble trying to perform the requested
        //  operation, so we'll abort the I/O request with
        //  the error status that we get back from the
        //  execption code
        //

        Status = FatProcessException( IrpContext, Irp, _SEH2_GetExceptionCode() );
    } _SEH2_END;

//  NT_ASSERT( !(ModWriter && (Status == STATUS_CANT_WAIT)) );

    NT_ASSERT( !(ModWriter && TopLevel) );

    if (ModWriter) { IoSetTopLevelIrp((PIRP)FSRTL_MOD_WRITE_TOP_LEVEL_IRP); }

    if (TopLevel) { IoSetTopLevelIrp( NULL ); }

    FsRtlExitFileSystem();

    //
    //  And return to our caller
    //

    DebugTrace(-1, Dbg, "FatFsdWrite -> %08lx\n", Status);

    UNREFERENCED_PARAMETER( VolumeDeviceObject );

    return Status;
}


_Requires_lock_held_(_Global_critical_region_)    
NTSTATUS
FatCommonWrite (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This is the common write routine for NtWriteFile, called from both
    the Fsd, or from the Fsp if a request could not be completed without
    blocking in the Fsd.  This routine's actions are
    conditionalized by the Wait input parameter, which determines whether
    it is allowed to block or not.  If a blocking condition is encountered
    with Wait == FALSE, however, the request is posted to the Fsp, who
    always calls with WAIT == TRUE.

Arguments:

    Irp - Supplies the Irp to process

Return Value:

    NTSTATUS - The return status for the operation

--*/

{
    PVCB Vcb;
    PFCB FcbOrDcb;
    PCCB Ccb;

    VBO StartingVbo;
    ULONG ByteCount;
    ULONG FileSize = 0;
    ULONG InitialFileSize = 0;
    ULONG InitialValidDataLength = 0;

    PIO_STACK_LOCATION IrpSp;
    PFILE_OBJECT FileObject;
    TYPE_OF_OPEN TypeOfOpen;

    BOOLEAN PostIrp = FALSE;
    BOOLEAN OplockPostIrp = FALSE;
    BOOLEAN ExtendingFile = FALSE;
    BOOLEAN FcbOrDcbAcquired = FALSE;
    BOOLEAN SwitchBackToAsync = FALSE;
    BOOLEAN CalledByLazyWriter = FALSE;
    BOOLEAN ExtendingValidData = FALSE;
    BOOLEAN FcbAcquiredExclusive = FALSE;
    BOOLEAN FcbCanDemoteToShared = FALSE;
    BOOLEAN WriteFileSizeToDirent = FALSE;
    BOOLEAN RecursiveWriteThrough = FALSE;
    BOOLEAN UnwindOutstandingAsync = FALSE;
    BOOLEAN PagingIoResourceAcquired = FALSE;
    BOOLEAN SuccessfulPurge = FALSE;

    BOOLEAN SynchronousIo;
    BOOLEAN WriteToEof;
    BOOLEAN PagingIo;
    BOOLEAN NonCachedIo;
    BOOLEAN Wait;
    NTSTATUS Status = STATUS_SUCCESS;

    FAT_IO_CONTEXT StackFatIoContext;

    //
    // A system buffer is only used if we have to access the buffer directly
    // from the Fsp to clear a portion or to do a synchronous I/O, or a
    // cached transfer.  It is possible that our caller may have already
    // mapped a system buffer, in which case we must remember this so
    // we do not unmap it on the way out.
    //

    PVOID SystemBuffer = (PVOID) NULL;

    LARGE_INTEGER StartingByte;

    PAGED_CODE();

    //
    // Get current Irp stack location and file object
    //

    IrpSp = IoGetCurrentIrpStackLocation( Irp );
    FileObject = IrpSp->FileObject;


    DebugTrace(+1, Dbg, "FatCommonWrite\n", 0);
    DebugTrace( 0, Dbg, "Irp                 = %p\n", Irp);
    DebugTrace( 0, Dbg, "ByteCount           = %8lx\n", IrpSp->Parameters.Write.Length);
    DebugTrace( 0, Dbg, "ByteOffset.LowPart  = %8lx\n", IrpSp->Parameters.Write.ByteOffset.LowPart);
    DebugTrace( 0, Dbg, "ByteOffset.HighPart = %8lx\n", IrpSp->Parameters.Write.ByteOffset.HighPart);

    //
    // Initialize the appropriate local variables.
    //

    Wait          = BooleanFlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT);
    PagingIo      = BooleanFlagOn(Irp->Flags, IRP_PAGING_IO);
    NonCachedIo   = BooleanFlagOn(Irp->Flags,IRP_NOCACHE);
    SynchronousIo = BooleanFlagOn(FileObject->Flags, FO_SYNCHRONOUS_IO);

    //NT_ASSERT( PagingIo || FileObject->WriteAccess );

    //
    //  Extract the bytecount and do our noop/throttle checking.
    //

    ByteCount = IrpSp->Parameters.Write.Length;

    //
    //  If there is nothing to write, return immediately.
    //

    if (ByteCount == 0) {

        Irp->IoStatus.Information = 0;
        FatCompleteRequest( IrpContext, Irp, STATUS_SUCCESS );
        return STATUS_SUCCESS;
    }

    //
    //  See if we have to defer the write.
    //

    if (!NonCachedIo &&
        !CcCanIWrite(FileObject,
                     ByteCount,
                     (BOOLEAN)(Wait && !BooleanFlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_IN_FSP)),
                     BooleanFlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_DEFERRED_WRITE))) {

        BOOLEAN Retrying = BooleanFlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_DEFERRED_WRITE);

        FatPrePostIrp( IrpContext, Irp );

        SetFlag( IrpContext->Flags, IRP_CONTEXT_FLAG_DEFERRED_WRITE );

        CcDeferWrite( FileObject,
                      (PCC_POST_DEFERRED_WRITE)FatAddToWorkque,
                      IrpContext,
                      Irp,
                      ByteCount,
                      Retrying );

        return STATUS_PENDING;
    }

    //
    //  Determine our starting position and type.  If we are writing
    //  at EOF, then we will need additional synchronization before
    //  the IO is issued to determine where the data will go.
    //

    StartingByte = IrpSp->Parameters.Write.ByteOffset;
    StartingVbo = StartingByte.LowPart;

    WriteToEof = ( (StartingByte.LowPart == FILE_WRITE_TO_END_OF_FILE) &&
                   (StartingByte.HighPart == -1) );

    //
    //  Extract the nature of the write from the file object, and case on it
    //

    TypeOfOpen = FatDecodeFileObject(FileObject, &Vcb, &FcbOrDcb, &Ccb);

    NT_ASSERT( Vcb != NULL );

    //
    //  Save callers who try to do cached IO to the raw volume from themselves.
    //

    if (TypeOfOpen == UserVolumeOpen) {

        NonCachedIo = TRUE;
    }

    NT_ASSERT(!(NonCachedIo == FALSE && TypeOfOpen == VirtualVolumeFile));

    //
    //  Collect interesting statistics.  The FLAG_USER_IO bit will indicate
    //  what type of io we're doing in the FatNonCachedIo function.
    //

    if (PagingIo) {
        CollectWriteStats(Vcb, TypeOfOpen, ByteCount);

        if (TypeOfOpen == UserFileOpen) {
            SetFlag(IrpContext->Flags, IRP_CONTEXT_FLAG_USER_IO);
        } else {
            ClearFlag(IrpContext->Flags, IRP_CONTEXT_FLAG_USER_IO);
        }
    }

    //
    //  We must disallow writes to regular objects that would require us
    //  to maintain an AllocationSize of greater than 32 significant bits.
    //
    //  If this is paging IO, this is simply a case where we need to trim.
    //  This will occur in due course.
    //

    if (!PagingIo && !WriteToEof && (TypeOfOpen != UserVolumeOpen)) {


        if (!FatIsIoRangeValid( Vcb, StartingByte, ByteCount)) {


            Irp->IoStatus.Information = 0;
            FatCompleteRequest( IrpContext, Irp, STATUS_DISK_FULL );

            return STATUS_DISK_FULL;
        }
    }

    //
    //  Allocate if necessary and initialize a FAT_IO_CONTEXT block for
    //  all non cached Io.  For synchronous Io
    //  we use stack storage, otherwise we allocate pool.
    //

    if (NonCachedIo) {

        if (IrpContext->FatIoContext == NULL) {

            if (!Wait) {

                IrpContext->FatIoContext =
#ifndef __REACTOS__
                    FsRtlAllocatePoolWithTag( NonPagedPoolNx,
#else
                    FsRtlAllocatePoolWithTag( NonPagedPool,
#endif
                                              sizeof(FAT_IO_CONTEXT),
                                              TAG_FAT_IO_CONTEXT );

            } else {

                IrpContext->FatIoContext = &StackFatIoContext;

                SetFlag( IrpContext->Flags, IRP_CONTEXT_STACK_IO_CONTEXT );
            }
        }

        RtlZeroMemory( IrpContext->FatIoContext, sizeof(FAT_IO_CONTEXT) );

        if (Wait) {

            KeInitializeEvent( &IrpContext->FatIoContext->Wait.SyncEvent,
                               NotificationEvent,
                               FALSE );

        } else {

            if (PagingIo) {

                IrpContext->FatIoContext->Wait.Async.ResourceThreadId =
                    ExGetCurrentResourceThread();

            } else {

                IrpContext->FatIoContext->Wait.Async.ResourceThreadId =
                    ((ULONG_PTR)IrpContext->FatIoContext) | 3;
            }

            IrpContext->FatIoContext->Wait.Async.RequestedByteCount =
                ByteCount;

            IrpContext->FatIoContext->Wait.Async.FileObject = FileObject;
        }
        
    }

    //
    //  Check if this volume has already been shut down.  If it has, fail
    //  this write request.
    //

    if ( FlagOn(Vcb->VcbState, VCB_STATE_FLAG_SHUTDOWN) ) {

        Irp->IoStatus.Information = 0;
        FatCompleteRequest( IrpContext, Irp, STATUS_TOO_LATE );
        return STATUS_TOO_LATE;
    }

    //
    //  This case corresponds to a write of the volume file (only the first
    //  fat allowed, the other fats are written automatically in parallel).
    //
    //  We use an Mcb keep track of dirty sectors.  Actual entries are Vbos
    //  and Lbos (ie. bytes), though they are all added in sector chunks.
    //  Since Vbo == Lbo for the volume file, the Mcb entries
    //  alternate between runs of Vbo == Lbo, and holes (Lbo == 0).  We use
    //  the prior to represent runs of dirty fat sectors, and the latter
    //  for runs of clean fat.  Note that since the first part of the volume
    //  file (boot sector) is always clean (a hole), and an Mcb never ends in
    //  a hole, there must always be an even number of runs(entries) in the Mcb.
    //
    //  The strategy is to find the first and last dirty run in the desired
    //  write range (which will always be a set of pages), and write from the
    //  former to the later.  The may result in writing some clean data, but
    //  will generally be more efficient than writing each runs seperately.
    //

    if (TypeOfOpen == VirtualVolumeFile) {

        LBO DirtyLbo;
        LBO CleanLbo;

        VBO DirtyVbo;
        VBO StartingDirtyVbo;

        ULONG DirtyByteCount;
        ULONG CleanByteCount;

        ULONG WriteLength;

        BOOLEAN MoreDirtyRuns = TRUE;

        IO_STATUS_BLOCK RaiseIosb;

        DebugTrace(0, Dbg, "Type of write is Virtual Volume File\n", 0);

        //
        //  If we can't wait we have to post this.
        //

        if (!Wait) {

            DebugTrace( 0, Dbg, "Passing request to Fsp\n", 0 );

            Status = FatFsdPostRequest(IrpContext, Irp);

            return Status;
        }

        //
        //  If we weren't called by the Lazy Writer, then this write
        //  must be the result of a write-through or flush operation.
        //  Setting the IrpContext flag, will cause DevIoSup.c to
        //  write-through the data to the disk.
        //

        if (!FlagOn((ULONG_PTR)IoGetTopLevelIrp(), FSRTL_CACHE_TOP_LEVEL_IRP)) {

            SetFlag( IrpContext->Flags, IRP_CONTEXT_FLAG_WRITE_THROUGH );
        }

        //
        //  Assert an even number of entries in the Mcb, an odd number would
        //  mean that the Mcb is corrupt.
        //

        NT_ASSERT( (FsRtlNumberOfRunsInLargeMcb( &Vcb->DirtyFatMcb ) & 1) == 0);

        //
        //  We need to skip over any clean sectors at the start of the write.
        //
        //  Also check the two cases where there are no dirty fats in the
        //  desired write range, and complete them with success.
        //
        //      1) There is no Mcb entry corresponding to StartingVbo, meaning
        //         we are beyond the end of the Mcb, and thus dirty fats.
        //
        //      2) The run at StartingVbo is clean and continues beyond the
        //         desired write range.
        //

        if (!FatLookupMcbEntry( Vcb, &Vcb->DirtyFatMcb,
                                StartingVbo,
                                &DirtyLbo,
                                &DirtyByteCount,
                                NULL )

          || ( (DirtyLbo == 0) && (DirtyByteCount >= ByteCount) ) ) {

            DebugTrace(0, DEBUG_TRACE_DEBUG_HOOKS,
                       "No dirty fat sectors in the write range.\n", 0);

            FatCompleteRequest( IrpContext, Irp, STATUS_SUCCESS );
            return STATUS_SUCCESS;
        }

        DirtyVbo = (VBO)DirtyLbo;

        //
        //  If the last run was a hole (clean), up DirtyVbo to the next
        //  run, which must be dirty.
        //

        if (DirtyVbo == 0) {

            DirtyVbo = StartingVbo + DirtyByteCount;
        }

        //
        //  This is where the write will start.
        //

        StartingDirtyVbo = DirtyVbo;

        //
        //
        //  Now start enumerating the dirty fat sectors spanning the desired
        //  write range, this first one of which is now DirtyVbo.
        //

        while ( MoreDirtyRuns ) {

            //
            //  Find the next dirty run, if it is not there, the Mcb ended
            //  in a hole, or there is some other corruption of the Mcb.
            //

            if (!FatLookupMcbEntry( Vcb, &Vcb->DirtyFatMcb,
                                    DirtyVbo,
                                    &DirtyLbo,
                                    &DirtyByteCount,
                                    NULL )) {

#ifdef _MSC_VER
#pragma prefast( suppress:28931, "needed for debug build" )
#endif
                DirtyVbo = (VBO)DirtyLbo;

                DebugTrace(0, Dbg, "Last dirty fat Mcb entry was a hole: corrupt.\n", 0);
                
#ifdef _MSC_VER
#pragma prefast( suppress:28159, "things are seriously wrong if we get here" )                
#endif
                FatBugCheck( 0, 0, 0 );

            } else {

                DirtyVbo = (VBO)DirtyLbo;

                //
                //  This has to correspond to a dirty run, and must start
                //  within the write range since we check it at entry to,
                //  and at the bottom of this loop.
                //

                NT_ASSERT((DirtyVbo != 0) && (DirtyVbo < StartingVbo + ByteCount));

                //
                //  There are three ways we can know that this was the
                //  last dirty run we want to write.
                //
                //      1)  The current dirty run extends beyond or to the
                //          desired write range.
                //
                //      2)  On trying to find the following clean run, we
                //          discover that this is the last run in the Mcb.
                //
                //      3)  The following clean run extend beyond the
                //          desired write range.
                //
                //  In any of these cases we set MoreDirtyRuns = FALSE.
                //

                //
                //  If the run is larger than we are writing, we also
                //  must truncate the WriteLength.  This is benign in
                //  the equals case.
                //

                if (DirtyVbo + DirtyByteCount >= StartingVbo + ByteCount) {

                    DirtyByteCount = StartingVbo + ByteCount - DirtyVbo;

                    MoreDirtyRuns = FALSE;

                } else {

                    //
                    //  Scan the clean hole after this dirty run.  If this
                    //  run was the last, prepare to exit the loop
                    //

                    if (!FatLookupMcbEntry( Vcb, &Vcb->DirtyFatMcb,
                                            DirtyVbo + DirtyByteCount,
                                            &CleanLbo,
                                            &CleanByteCount,
                                            NULL )) {

                        MoreDirtyRuns = FALSE;

                    } else {

                        //
                        //  Assert that we actually found a clean run.
                        //  and compute the start of the next dirty run.
                        //

                        NT_ASSERT (CleanLbo == 0);

                        //
                        //  If the next dirty run starts beyond the desired
                        //  write, we have found all the runs we need, so
                        //  prepare to exit.
                        //

                        if (DirtyVbo + DirtyByteCount + CleanByteCount >=
                                                    StartingVbo + ByteCount) {

                            MoreDirtyRuns = FALSE;

                        } else {

                            //
                            //  Compute the start of the next dirty run.
                            //

                            DirtyVbo += DirtyByteCount + CleanByteCount;
                        }
                    }
                }
            }
        } // while ( MoreDirtyRuns )

        //
        //  At this point DirtyVbo and DirtyByteCount correctly reflect the
        //  final dirty run, constrained to the desired write range.
        //
        //  Now compute the length we finally must write.
        //

        WriteLength = (DirtyVbo + DirtyByteCount) - StartingDirtyVbo;

        //
        // We must now assume that the write will complete with success,
        // and initialize our expected status in RaiseIosb.  It will be
        // modified below if an error occurs.
        //

        RaiseIosb.Status = STATUS_SUCCESS;
        RaiseIosb.Information = ByteCount;

        //
        //  Loop through all the fats, setting up a multiple async to
        //  write them all.  If there are more than FAT_MAX_PARALLEL_IOS
        //  then we do several muilple asyncs.
        //

        {
            ULONG Fat;
            ULONG BytesPerFat;
            IO_RUN StackIoRuns[2];
            PIO_RUN IoRuns;

            BytesPerFat = FatBytesPerFat( &Vcb->Bpb );

            if ((ULONG)Vcb->Bpb.Fats > 2) {

                IoRuns = FsRtlAllocatePoolWithTag( PagedPool,
                                                   (ULONG)(Vcb->Bpb.Fats*sizeof(IO_RUN)),
                                                   TAG_IO_RUNS );

            } else {

                IoRuns = StackIoRuns;
            }

            for (Fat = 0; Fat < (ULONG)Vcb->Bpb.Fats; Fat++) {

                IoRuns[Fat].Vbo = StartingDirtyVbo;
                IoRuns[Fat].Lbo = Fat * BytesPerFat + StartingDirtyVbo;
                IoRuns[Fat].Offset = StartingDirtyVbo - StartingVbo;
                IoRuns[Fat].ByteCount = WriteLength;
            }

            //
            //  Keep track of meta-data disk ios.
            //

            Vcb->Statistics[KeGetCurrentProcessorNumber() % FatData.NumberProcessors].Common.MetaDataDiskWrites += Vcb->Bpb.Fats;

            _SEH2_TRY {

                FatMultipleAsync( IrpContext,
                                  Vcb,
                                  Irp,
                                  (ULONG)Vcb->Bpb.Fats,
                                  IoRuns );

            } _SEH2_FINALLY {

                if (IoRuns != StackIoRuns) {

                    ExFreePool( IoRuns );
                }
            } _SEH2_END;

#if (NTDDI_VERSION >= NTDDI_WIN8)

            //
            //  Account for DASD Ios
            //

            if (FatDiskAccountingEnabled) {

                PETHREAD ThreadIssuingIo = PsGetCurrentThread();

                PsUpdateDiskCounters( PsGetThreadProcess( ThreadIssuingIo ),
                                      0,
                                      WriteLength,
                                      0,
                                      1,
                                      0 );
            }

#endif
            //
            //  Wait for all the writes to finish
            //

            FatWaitSync( IrpContext );

            //
            //  If we got an error, or verify required, remember it.
            //

            if (!NT_SUCCESS( Irp->IoStatus.Status )) {

                DebugTrace( 0,
                            Dbg,
                            "Error %X while writing volume file.\n",
                            Irp->IoStatus.Status );

                RaiseIosb = Irp->IoStatus;
            }
        }

        //
        //  If the writes were a success, set the sectors clean, else
        //  raise the error status and mark the volume as needing
        //  verification.  This will automatically reset the volume
        //  structures.
        //
        //  If not, then mark this volume as needing verification to
        //  automatically cause everything to get cleaned up.
        //

        Irp->IoStatus = RaiseIosb;

        if ( NT_SUCCESS( Status = Irp->IoStatus.Status )) {

            FatRemoveMcbEntry( Vcb, &Vcb->DirtyFatMcb,
                               StartingDirtyVbo,
                               WriteLength );

        } else {

            FatNormalizeAndRaiseStatus( IrpContext, Status );
        }

        DebugTrace(-1, Dbg, "CommonWrite -> %08lx\n", Status );

        FatCompleteRequest( IrpContext, Irp, Status );
        return Status;
    }

    //
    //  This case corresponds to a general opened volume (DASD), ie.
    //  open ("a:").
    //

    if (TypeOfOpen == UserVolumeOpen) {

        LBO StartingLbo;
        LBO VolumeSize;

        //
        //  Precalculate the volume size since we're nearly always going
        //  to be wanting to use it.
        //

        VolumeSize = (LBO) Int32x32To64( Vcb->Bpb.BytesPerSector,
                                         (Vcb->Bpb.Sectors != 0 ? Vcb->Bpb.Sectors :
                                                                  Vcb->Bpb.LargeSectors));

        StartingLbo = StartingByte.QuadPart;

        DebugTrace(0, Dbg, "Type of write is User Volume.\n", 0);

        //
        //  If this is a write on a disk-based volume that is not locked, we need to limit
        //  the sectors we allow to be written within the volume.  Specifically, we only
        //  allow writes to the reserved area.  Note that extended DASD can still be used
        //  to write past the end of the volume.  We also allow kernel mode callers to force
        //  access via a flag in the IRP.  A handle that issued a dismount can write anywhere
        //  as well.
        //

        if ((Vcb->TargetDeviceObject->DeviceType == FILE_DEVICE_DISK) &&
            !FlagOn( Vcb->VcbState, VCB_STATE_FLAG_LOCKED ) &&
            !FlagOn( IrpSp->Flags, SL_FORCE_DIRECT_WRITE ) &&
            !FlagOn( Ccb->Flags, CCB_FLAG_COMPLETE_DISMOUNT )) {

            //
            //  First check for a write beyond the end of the volume.
            //

            if (!WriteToEof && (StartingLbo < VolumeSize)) {

                //
                //  This write is within the volume.  Make sure it is not beyond the reserved section.
                //

                if ((StartingLbo >= FatReservedBytes( &(Vcb->Bpb) )) ||
                    (ByteCount > (FatReservedBytes( &(Vcb->Bpb) ) - StartingLbo))) {

                    FatCompleteRequest( IrpContext, Irp, STATUS_ACCESS_DENIED );
                    return STATUS_ACCESS_DENIED;
                }
            }
        }

        //
        //  Verify that the volume for this handle is still valid, permitting
        //  operations to proceed on dismounted volumes via the handle which
        //  performed the dismount or sent a format unit command.
        //

        if (!FlagOn( Ccb->Flags, CCB_FLAG_COMPLETE_DISMOUNT | CCB_FLAG_SENT_FORMAT_UNIT )) {

            FatQuickVerifyVcb( IrpContext, Vcb );
        }

        //
        //  If the caller previously sent a format unit command, then we will allow
        //  their read/write requests to ignore the verify flag on the device, since some
        //  devices send a media change event after format unit, but we don't want to 
        //  process it yet since we're probably in the process of formatting the
        //  media.
        //
        
        if (FlagOn( Ccb->Flags, CCB_FLAG_SENT_FORMAT_UNIT )) {
        
            SetFlag( IrpContext->Flags, IRP_CONTEXT_FLAG_OVERRIDE_VERIFY );
        }

        if (!FlagOn( Ccb->Flags, CCB_FLAG_DASD_PURGE_DONE )) {

            BOOLEAN PreviousWait = BooleanFlagOn( IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT );

            //
            //  Grab the entire volume so that even the normally unsafe action
            //  of writing to an unlocked volume won't open us to a race between
            //  the flush and purge of the FAT below.
            //
            //  I really don't think this is particularly important to worry about,
            //  but a repro case for another bug happens to dance into this race
            //  condition pretty easily. Eh.
            //
            
            SetFlag( IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT );
            FatAcquireExclusiveVolume( IrpContext, Vcb );

            _SEH2_TRY {

                //
                //  If the volume isn't locked, flush and purge it.
                //

                if (!FlagOn(Vcb->VcbState, VCB_STATE_FLAG_LOCKED)) {

                    FatFlushFat( IrpContext, Vcb );
                    CcPurgeCacheSection( &Vcb->SectionObjectPointers,
                                         NULL,
                                         0,
                                         FALSE );

                    FatPurgeReferencedFileObjects( IrpContext, Vcb->RootDcb, Flush );
                }

            } _SEH2_FINALLY {

                FatReleaseVolume( IrpContext, Vcb );
                if (!PreviousWait) {
                    ClearFlag( IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT );
                }
            } _SEH2_END;

            SetFlag( Ccb->Flags, CCB_FLAG_DASD_PURGE_DONE |
                                 CCB_FLAG_DASD_FLUSH_DONE );
        }

        if (!FlagOn( Ccb->Flags, CCB_FLAG_ALLOW_EXTENDED_DASD_IO )) {

            //
            //  Make sure we don't try to write past end of volume,
            //  reducing the requested byte count if necessary.
            //

            if (WriteToEof || StartingLbo >= VolumeSize) {
                FatCompleteRequest( IrpContext, Irp, STATUS_SUCCESS );
                return STATUS_SUCCESS;
            }

            if (ByteCount > VolumeSize - StartingLbo) {

                ByteCount = (ULONG) (VolumeSize - StartingLbo);

                //
                //  For async writes we had set the byte count in the FatIoContext
                //  above, so fix that here.
                //

                if (!Wait) {

                    IrpContext->FatIoContext->Wait.Async.RequestedByteCount =
                        ByteCount;
                }
            }
        } else {

            //
            //  This has a peculiar interpretation, but just adjust the starting
            //  byte to the end of the visible volume.
            //

            if (WriteToEof) {

                StartingLbo = VolumeSize;
            }
        }

        //
        // For DASD we have to probe and lock the user's buffer
        //

        FatLockUserBuffer( IrpContext, Irp, IoReadAccess, ByteCount );

        //
        //  Set the FO_MODIFIED flag here to trigger a verify when this
        //  handle is closed.  Note that we can err on the conservative
        //  side with no problem, i.e. if we accidently do an extra
        //  verify there is no problem.
        //

        SetFlag( FileObject->Flags, FO_FILE_MODIFIED );

        //
        //  Write the data and wait for the results
        //

        FatSingleAsync( IrpContext,
                        Vcb,
                        StartingLbo,
                        ByteCount,
                        Irp );

#if (NTDDI_VERSION >= NTDDI_WIN8)

        //
        //  Account for DASD Ios
        //

        if (FatDiskAccountingEnabled) {

            PETHREAD ThreadIssuingIo = PsGetCurrentThread();

            PsUpdateDiskCounters( PsGetThreadProcess( ThreadIssuingIo ),
                                  0,
                                  ByteCount,
                                  0,
                                  1,
                                  0 );
        }

#endif

        if (!Wait) {

            //
            //  We, nor anybody else, need the IrpContext any more.
            //

            IrpContext->FatIoContext = NULL;

            FatDeleteIrpContext( IrpContext );

            DebugTrace(-1, Dbg, "FatNonCachedIo -> STATUS_PENDING\n", 0);

            return STATUS_PENDING;
        }

        FatWaitSync( IrpContext );

        //
        //  If the call didn't succeed, raise the error status
        //
        //  Also mark this volume as needing verification to automatically
        //  cause everything to get cleaned up.
        //

        if (!NT_SUCCESS( Status = Irp->IoStatus.Status )) {

            FatNormalizeAndRaiseStatus( IrpContext, Status );
        }

        //
        //  Update the current file position.  We assume that
        //  open/create zeros out the CurrentByteOffset field.
        //

        if (SynchronousIo && !PagingIo) {
            FileObject->CurrentByteOffset.QuadPart =
                StartingLbo + Irp->IoStatus.Information;
        }

        DebugTrace(-1, Dbg, "FatCommonWrite -> %08lx\n", Status );

        FatCompleteRequest( IrpContext, Irp, Status );
        return Status;
    }

    //
    //  At this point we know there is an Fcb/Dcb.
    //

    NT_ASSERT( FcbOrDcb != NULL );

    //
    //  Use a try-finally to free Fcb/Dcb and buffers on the way out.
    //

    _SEH2_TRY {

        //
        // This case corresponds to a normal user write file.
        //

        if ( TypeOfOpen == UserFileOpen
            ) {

            ULONG ValidDataLength;
            ULONG ValidDataToDisk;
            ULONG ValidDataToCheck;

            DebugTrace(0, Dbg, "Type of write is user file open\n", 0);

            //
            //  If this is a noncached transfer and is not a paging I/O, and
            //  the file has been opened cached, then we will do a flush here
            //  to avoid stale data problems.  Note that we must flush before
            //  acquiring the Fcb shared since the write may try to acquire
            //  it exclusive.
            //
            //  The Purge following the flush will guarentee cache coherency.
            //

            if (NonCachedIo && !PagingIo &&
                (FileObject->SectionObjectPointer->DataSectionObject != NULL)) {

                IO_STATUS_BLOCK IoStatus = {0};

                //
                //  We need the Fcb exclsuive to do the CcPurgeCache
                //

                if (!FatAcquireExclusiveFcb( IrpContext, FcbOrDcb )) {

                    DebugTrace( 0, Dbg, "Cannot acquire FcbOrDcb = %p shared without waiting\n", FcbOrDcb );

                    try_return( PostIrp = TRUE );
                }

                FcbOrDcbAcquired = TRUE;
                FcbAcquiredExclusive = TRUE;

                //
                //  Preacquire pagingio for the flush.
                //

                ExAcquireResourceExclusiveLite( FcbOrDcb->Header.PagingIoResource, TRUE );

#if (NTDDI_VERSION >= NTDDI_WIN7)

                //
                //  Remember that we are holding the paging I/O resource.
                //

                PagingIoResourceAcquired = TRUE;

                //
                //  We hold so that we will prevent a pagefault from occuring and seeing
                //  soon-to-be stale data from the disk. We used to believe this was
                //  something to be left to the app to synchronize; we now realize that
                //  noncached IO on a fileserver is doomed without the filesystem forcing
                //  the coherency issue. By only penalizing noncached coherency when
                //  needed, this is about the best we can do.
                //

                //
                // Now perform the coherency flush and purge operation. This version of the call
                // will try to invalidate mapped pages to prevent data corruption.
                //

                CcCoherencyFlushAndPurgeCache(  FileObject->SectionObjectPointer,
                                                WriteToEof ? &FcbOrDcb->Header.FileSize : &StartingByte,
                                                ByteCount,
                                                &IoStatus,
                                                0 );

                SuccessfulPurge = NT_SUCCESS( IoStatus.Status );

#else

                CcFlushCache( FileObject->SectionObjectPointer,
                              WriteToEof ? &FcbOrDcb->Header.FileSize : &StartingByte,
                              ByteCount,
                              &IoStatus );

                if (!NT_SUCCESS( IoStatus.Status )) {

                    ExReleaseResourceLite( FcbOrDcb->Header.PagingIoResource );
                    try_return( IoStatus.Status );
                }

                //
                //  Remember that we are holding the paging I/O resource.
                //

                PagingIoResourceAcquired = TRUE;

                //
                //  We hold so that we will prevent a pagefault from occuring and seeing
                //  soon-to-be stale data from the disk. We used to believe this was
                //  something to be left to the app to synchronize; we now realize that
                //  noncached IO on a fileserver is doomed without the filesystem forcing
                //  the coherency issue. By only penalizing noncached coherency when
                //  needed, this is about the best we can do.
                //

                SuccessfulPurge = CcPurgeCacheSection( FileObject->SectionObjectPointer,
                                                       WriteToEof ? &FcbOrDcb->Header.FileSize : &StartingByte,
                                                       ByteCount,
                                                       FALSE );

#endif

                if (!SuccessfulPurge && (FcbOrDcb->PurgeFailureModeEnableCount > 0)) {

                    //
                    //  Purge failure mode only applies to user files.
                    //
                    
                    NT_ASSERT( TypeOfOpen == UserFileOpen );

                    //
                    //  Do not swallow the purge failure if in purge failure
                    //  mode. Someone outside the file system intends to handle
                    //  the error and prevent any application compatibilty 
                    //  issue.
                    //
                    //  NOTE: If the file system were not preventing a pagefault
                    //  from processing while this write is in flight, which it does
                    //  by holding the paging resource across the write, it would
                    //  need to fail the operation even if a purge succeeded. If
                    //  not a memory mapped read could bring in a stale page before
                    //  the write makes it to disk.
                    //
                    
                    try_return( Status = STATUS_PURGE_FAILED );                
                }

                //
                //  Indicate we're OK with the fcb being demoted to shared access
                //  if that turns out to be possible later on after VDL extension
                //  is checked for.
                //
                //  PagingIo must be held all the way through.
                //
                
                FcbCanDemoteToShared = TRUE;
            }

            //
            //  We assert that Paging Io writes will never WriteToEof.
            //

            NT_ASSERT( WriteToEof ? !PagingIo : TRUE );
            
            //
            //  First let's acquire the Fcb shared.  Shared is enough if we
            //  are not writing beyond EOF.
            //

            if ( PagingIo ) {

                (VOID)ExAcquireResourceSharedLite( FcbOrDcb->Header.PagingIoResource, TRUE );
                PagingIoResourceAcquired = TRUE;

                if (!Wait) {

                    IrpContext->FatIoContext->Wait.Async.Resource =
                        FcbOrDcb->Header.PagingIoResource;
                }

                //
                //  Check to see if we colided with a MoveFile call, and if
                //  so block until it completes.
                //

                if (FcbOrDcb->MoveFileEvent) {

                    (VOID)KeWaitForSingleObject( FcbOrDcb->MoveFileEvent,
                                                 Executive,
                                                 KernelMode,
                                                 FALSE,
                                                 NULL );
                }

            } else {

                //
                //  We may already have the Fcb due to noncached coherency
                //  work done just above; however, we may still have to extend
                //  valid data length.  We can't demote this to shared, matching
                //  what occured before, until we figure that out a bit later. 
                //
                //  We kept ahold of it since our lockorder is main->paging,
                //  and paging must now held across the noncached write from
                //  the purge on.
                //
                
                //
                //  If this is async I/O, we will wait if there is an exclusive
                //  waiter.
                //

                if (!Wait && NonCachedIo) {

                    if (!FcbOrDcbAcquired &&
                        !FatAcquireSharedFcbWaitForEx( IrpContext, FcbOrDcb )) {

                        DebugTrace( 0, Dbg, "Cannot acquire FcbOrDcb = %p shared without waiting\n", FcbOrDcb );
                        try_return( PostIrp = TRUE );
                    }

                    //
                    //  Note we will have to release this resource elsewhere.  If we came
                    //  out of the noncached coherency path, we will also have to drop
                    //  the paging io resource.
                    //

                    IrpContext->FatIoContext->Wait.Async.Resource = FcbOrDcb->Header.Resource;

                    if (FcbCanDemoteToShared) {
                        
                        IrpContext->FatIoContext->Wait.Async.Resource2 = FcbOrDcb->Header.PagingIoResource;
                    }
                } else {

                    if (!FcbOrDcbAcquired &&
                        !FatAcquireSharedFcb( IrpContext, FcbOrDcb )) {

                        DebugTrace( 0, Dbg, "Cannot acquire FcbOrDcb = %p shared without waiting\n", FcbOrDcb );
                        try_return( PostIrp = TRUE );
                    }
                }

                FcbOrDcbAcquired = TRUE;
            }

            //
            //  Get a first tentative file size and valid data length.
            //  We must get ValidDataLength first since it is always
            //  increased second (in case we are unprotected) and
            //  we don't want to capture ValidDataLength > FileSize.
            //

            ValidDataToDisk = FcbOrDcb->ValidDataToDisk;
            ValidDataLength = FcbOrDcb->Header.ValidDataLength.LowPart;
            FileSize = FcbOrDcb->Header.FileSize.LowPart;

            NT_ASSERT( ValidDataLength <= FileSize );

            //
            // If are paging io, then we do not want
            // to write beyond end of file.  If the base is beyond Eof, we will just
            // Noop the call.  If the transfer starts before Eof, but extends
            // beyond, we will truncate the transfer to the last sector
            // boundary.
            //

            //
            //  Just in case this is paging io, limit write to file size.
            //  Otherwise, in case of write through, since Mm rounds up
            //  to a page, we might try to acquire the resource exclusive
            //  when our top level guy only acquired it shared. Thus, =><=.
            //

            if ( PagingIo ) {

                if (StartingVbo >= FileSize) {

                    DebugTrace( 0, Dbg, "PagingIo started beyond EOF.\n", 0 );

                    Irp->IoStatus.Information = 0;

                    try_return( Status = STATUS_SUCCESS );
                }

                if (ByteCount > FileSize - StartingVbo) {

                    DebugTrace( 0, Dbg, "PagingIo extending beyond EOF.\n", 0 );

                    ByteCount = FileSize - StartingVbo;
                }
            }

            //
            //  Determine if we were called by the lazywriter.
            //  (see resrcsup.c)
            //

            if (FcbOrDcb->Specific.Fcb.LazyWriteThread == PsGetCurrentThread()) {

                CalledByLazyWriter = TRUE;

                if (FlagOn( FcbOrDcb->Header.Flags, FSRTL_FLAG_USER_MAPPED_FILE )) {

                    //
                    //  Fail if the start of this request is beyond valid data length.
                    //  Don't worry if this is an unsafe test.  MM and CC won't
                    //  throw this page away if it is really dirty.
                    //

                    if ((StartingVbo + ByteCount > ValidDataLength) &&
                        (StartingVbo < FileSize)) {

                        //
                        //  It's OK if byte range is within the page containing valid data length,
                        //  since we will use ValidDataToDisk as the start point.
                        //

                        if (StartingVbo + ByteCount > ((ValidDataLength + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1))) {

                            //
                            //  Don't flush this now.
                            //

                            try_return( Status = STATUS_FILE_LOCK_CONFLICT );
                        }
                    }
                }
            }

            //
            //  This code detects if we are a recursive synchronous page write
            //  on a write through file object.
            //

            if (FlagOn(Irp->Flags, IRP_SYNCHRONOUS_PAGING_IO) &&
                FlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_RECURSIVE_CALL)) {

                PIRP TopIrp;

                TopIrp = IoGetTopLevelIrp();

                //
                //  This clause determines if the top level request was
                //  in the FastIo path.  Gack.  Since we don't have a
                //  real sharing protocol for the top level IRP field ...
                //  yet ... if someone put things other than a pure IRP in
                //  there we best be careful.
                //

                if ((ULONG_PTR)TopIrp > FSRTL_MAX_TOP_LEVEL_IRP_FLAG &&
                    NodeType(TopIrp) == IO_TYPE_IRP) {

                    PIO_STACK_LOCATION IrpStack;

                    IrpStack = IoGetCurrentIrpStackLocation(TopIrp);

                    //
                    //  Finally this routine detects if the Top irp was a
                    //  cached write to this file and thus we are the writethrough.
                    //

                    if ((IrpStack->MajorFunction == IRP_MJ_WRITE) &&
                        (IrpStack->FileObject->FsContext == FileObject->FsContext) &&
                        !FlagOn(TopIrp->Flags,IRP_NOCACHE)) {

                        RecursiveWriteThrough = TRUE;
                        SetFlag( IrpContext->Flags, IRP_CONTEXT_FLAG_WRITE_THROUGH );
                    }
                }
            }

            //
            //  Here is the deal with ValidDataLength and FileSize:
            //
            //  Rule 1: PagingIo is never allowed to extend file size.
            //
            //  Rule 2: Only the top level requestor may extend Valid
            //          Data Length.  This may be paging IO, as when a
            //          a user maps a file, but will never be as a result
            //          of cache lazy writer writes since they are not the
            //          top level request.
            //
            //  Rule 3: If, using Rules 1 and 2, we decide we must extend
            //          file size or valid data, we take the Fcb exclusive.
            //

            //
            // Now see if we are writing beyond valid data length, and thus
            // maybe beyond the file size.  If so, then we must
            // release the Fcb and reacquire it exclusive.  Note that it is
            // important that when not writing beyond EOF that we check it
            // while acquired shared and keep the FCB acquired, in case some
            // turkey truncates the file.
            //

            //
            //  Note that the lazy writer must not be allowed to try and
            //  acquire the resource exclusive.  This is not a problem since
            //  the lazy writer is paging IO and thus not allowed to extend
            //  file size, and is never the top level guy, thus not able to
            //  extend valid data length.
            //

            if ( !CalledByLazyWriter &&

                 !RecursiveWriteThrough &&

                 (WriteToEof ||
                  StartingVbo + ByteCount > ValidDataLength)) {

                //
                //  If this was an asynchronous write, we are going to make
                //  the request synchronous at this point, but only kinda.
                //  At the last moment, before sending the write off to the
                //  driver, we may shift back to async.
                //
                //  The modified page writer already has the resources
                //  he requires, so this will complete in small finite
                //  time.
                //

                if (!Wait) {

                    Wait = TRUE;
                    SetFlag( IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT );

                    if (NonCachedIo) {

                        NT_ASSERT( TypeOfOpen == UserFileOpen );

                        SwitchBackToAsync = TRUE;
                    }
                }

                //
                // We need Exclusive access to the Fcb/Dcb since we will
                // probably have to extend valid data and/or file.
                //

                //
                //  Y'know, the PagingIo case is a mapped page writer, and
                //  MmFlushSection or the mapped page writer itself already
                //  snatched up the main exclusive for us via the AcquireForCcFlush
                //  or AcquireForModWrite logic (the default logic parallels FAT's
                //  requirements since this order/model came first).  Should ASSERT
                //  this since it'll just go 1->2, and a few more unnecesary DPC
                //  transitions.
                //
                //  The preacquire is done to avoid inversion over the collided flush
                //  meta-resource in Mm.  The one time this is not true is at final
                //  system shutdown time, when Mm goes off and flushes all the dirty
                //  pages.  Since the callback is defined as Wait == FALSE he can't
                //  guarantee acquisition (though with clean process shutdown being
                //  enforced, it really should be now).  Permit this to float.
                //
                //  Note that since we're going to fall back on the acquisition aleady
                //  done for us, don't confuse things by thinking we did the work
                //  for it.
                //

                if ( PagingIo ) {

                    ExReleaseResourceLite( FcbOrDcb->Header.PagingIoResource );
                    PagingIoResourceAcquired = FALSE;

                } else {

                    //
                    //  The Fcb may already be acquired exclusive due to coherency
                    //  work performed earlier.  If so, obviously no work to do.
                    //
                    
                    if (!FcbAcquiredExclusive) {
                        
                        FatReleaseFcb( IrpContext, FcbOrDcb );
                        FcbOrDcbAcquired = FALSE;

                        if (!FatAcquireExclusiveFcb( IrpContext, FcbOrDcb )) {

                            DebugTrace( 0, Dbg, "Cannot acquire FcbOrDcb = %p shared without waiting\n", FcbOrDcb );

                            try_return( PostIrp = TRUE );
                        }

                        FcbOrDcbAcquired = TRUE;
                        
#ifdef _MSC_VER
#pragma prefast( suppress:28931, "convenient for debugging" )
#endif
                        FcbAcquiredExclusive = TRUE;
                    }
                }

                //
                //  Now that we have the Fcb exclusive, see if this write
                //  qualifies for being made async again.  The key point
                //  here is that we are going to update ValidDataLength in
                //  the Fcb before returning.  We must make sure this will
                //  not cause a problem.  One thing we must do is keep out
                //  the FastIo path.
                //

                if (SwitchBackToAsync) {

                    if ((FcbOrDcb->NonPaged->SectionObjectPointers.DataSectionObject != NULL) ||
                        (StartingVbo + ByteCount > FcbOrDcb->Header.ValidDataLength.LowPart) ||
                        FatNoAsync) {

                        RtlZeroMemory( IrpContext->FatIoContext, sizeof(FAT_IO_CONTEXT) );

                        KeInitializeEvent( &IrpContext->FatIoContext->Wait.SyncEvent,
                                           NotificationEvent,
                                           FALSE );

                        SwitchBackToAsync = FALSE;

                    } else {

                        if (!FcbOrDcb->NonPaged->OutstandingAsyncEvent) {

                            FcbOrDcb->NonPaged->OutstandingAsyncEvent =
#ifndef __REACTOS__
                                FsRtlAllocatePoolWithTag( NonPagedPoolNx,
#else
                                FsRtlAllocatePoolWithTag( NonPagedPool,
#endif
                                                          sizeof(KEVENT),
                                                          TAG_EVENT );

                            KeInitializeEvent( FcbOrDcb->NonPaged->OutstandingAsyncEvent,
                                               NotificationEvent,
                                               FALSE );
                        }

                        //
                        //  If we are transitioning from 0 to 1, reset the event.
                        //

                        if (ExInterlockedAddUlong( &FcbOrDcb->NonPaged->OutstandingAsyncWrites,
                                                   1,
                                                   &FatData.GeneralSpinLock ) == 0) {

                            KeClearEvent( FcbOrDcb->NonPaged->OutstandingAsyncEvent );
                        }

                        UnwindOutstandingAsync = TRUE;

                        IrpContext->FatIoContext->Wait.Async.NonPagedFcb = FcbOrDcb->NonPaged;
                    }
                }

                //
                //  Now that we have the Fcb exclusive, get a new batch of
                //  filesize and ValidDataLength.
                //

                ValidDataToDisk = FcbOrDcb->ValidDataToDisk;
                ValidDataLength = FcbOrDcb->Header.ValidDataLength.LowPart;
                FileSize = FcbOrDcb->Header.FileSize.LowPart;

                //
                //  If this is PagingIo check again if any pruning is
                //  required.  It is important to start from basic
                //  princples in case the file was *grown* ...
                //

                if ( PagingIo ) {

                    if (StartingVbo >= FileSize) {
                        Irp->IoStatus.Information = 0;
                        try_return( Status = STATUS_SUCCESS );
                    }
                    
                    ByteCount = IrpSp->Parameters.Write.Length;

                    if (ByteCount > FileSize - StartingVbo) {
                        ByteCount = FileSize - StartingVbo;
                    }
                }
            }

            //
            //  Remember the final requested byte count
            //

            if (NonCachedIo && !Wait) {

                IrpContext->FatIoContext->Wait.Async.RequestedByteCount =
                    ByteCount;
            }

            //
            //  Remember the initial file size and valid data length,
            //  just in case .....
            //

            InitialFileSize = FileSize;

            InitialValidDataLength = ValidDataLength;

            //
            //  Make sure the FcbOrDcb is still good
            //

            FatVerifyFcb( IrpContext, FcbOrDcb );

            //
            //  Check for writing to end of File.  If we are, then we have to
            //  recalculate a number of fields.
            //

            if ( WriteToEof ) {

                StartingVbo = FileSize;
                StartingByte = FcbOrDcb->Header.FileSize;

                //
                //  Since we couldn't know this information until now, perform the
                //  necessary bounds checking that we ommited at the top because
                //  this is a WriteToEof operation.
                //

                
                if (!FatIsIoRangeValid( Vcb, StartingByte, ByteCount)) {
                    
                    Irp->IoStatus.Information = 0;
                    try_return( Status = STATUS_DISK_FULL );
                }


            }

            //
            //  If this is a non paging write to a data stream object we have to
            //  check for access according to the current state op/filelocks.
            //
            //  Note that after this point, operations will be performed on the file.
            //  No modifying activity can occur prior to this point in the write
            //  path.
            //

            if (!PagingIo && TypeOfOpen == UserFileOpen) {

                Status = FsRtlCheckOplock( FatGetFcbOplock(FcbOrDcb),
                                           Irp,
                                           IrpContext,
                                           FatOplockComplete,
                                           FatPrePostIrp );

                if (Status != STATUS_SUCCESS) {

                    OplockPostIrp = TRUE;
                    PostIrp = TRUE;
                    try_return( NOTHING );
                }

                //
                //  This oplock call can affect whether fast IO is possible.
                //  We may have broken an oplock to no oplock held.  If the
                //  current state of the file is FastIoIsNotPossible then
                //  recheck the fast IO state.
                //

                if (FcbOrDcb->Header.IsFastIoPossible == FastIoIsNotPossible) {

                    FcbOrDcb->Header.IsFastIoPossible = FatIsFastIoPossible( FcbOrDcb );
                }

                //
                //  And finally check the regular file locks.
                //

                if (!FsRtlCheckLockForWriteAccess( &FcbOrDcb->Specific.Fcb.FileLock, Irp )) {

                    try_return( Status = STATUS_FILE_LOCK_CONFLICT );
                }
            }

            //
            //  Determine if we will deal with extending the file. Note that
            //  this implies extending valid data, and so we already have all
            //  of the required synchronization done.
            //

            if (!PagingIo && (StartingVbo + ByteCount > FileSize)) {

                ExtendingFile = TRUE;
            }

            if ( ExtendingFile ) {


                //
                //  EXTENDING THE FILE
                //

                //
                //  For an extending write on hotplug media, we are going to defer the metadata
                //  updates via Cc's lazy writer. They will also be flushed when the handle is closed.
                //

                if (FlagOn(Vcb->VcbState, VCB_STATE_FLAG_DEFERRED_FLUSH)) {
                    
                    SetFlag(IrpContext->Flags, IRP_CONTEXT_FLAG_DISABLE_WRITE_THROUGH);
                }

                //
                //  Update our local copy of FileSize
                //

                FileSize = StartingVbo + ByteCount;


                if (FcbOrDcb->Header.AllocationSize.QuadPart == FCB_LOOKUP_ALLOCATIONSIZE_HINT) {

                    FatLookupFileAllocationSize( IrpContext, FcbOrDcb );
                }

                //
                //  If the write goes beyond the allocation size, add some
                //  file allocation.
                //


                if ( (FileSize) > FcbOrDcb->Header.AllocationSize.LowPart ) {


                    BOOLEAN AllocateMinimumSize = TRUE;

                    //
                    //  Only do allocation chuncking on writes if this is
                    //  not the first allocation added to the file.
                    //

                    if (FcbOrDcb->Header.AllocationSize.LowPart != 0 ) {

                        ULONGLONG ApproximateClusterCount;
                        ULONGLONG TargetAllocation;
                        ULONGLONG AddedAllocation;                        
                        ULONGLONG Multiplier;
                        ULONG BytesPerCluster;
                        ULONG ClusterAlignedFileSize;

                        //
                        //  We are going to try and allocate a bigger chunk than
                        //  we actually need in order to maximize FastIo usage.
                        //
                        //  The multiplier is computed as follows:
                        //
                        //
                        //            (FreeDiskSpace            )
                        //  Mult =  ( (-------------------------) / 32 ) + 1
                        //            (FileSize - AllocationSize)
                        //
                        //          and max out at 32.
                        //
                        //  With this formula we start winding down chunking
                        //  as we get near the disk space wall.
                        //
                        //  For instance on an empty 1 MEG floppy doing an 8K
                        //  write, the multiplier is 6, or 48K to allocate.
                        //  When this disk is half full, the multipler is 3,
                        //  and when it is 3/4 full, the mupltiplier is only 1.
                        //
                        //  On a larger disk, the multiplier for a 8K read will
                        //  reach its maximum of 32 when there is at least ~8 Megs
                        //  available.
                        //

                        //
                        //  Small write performance note, use cluster aligned
                        //  file size in above equation.
                        //

                        //
                        //  We need to carefully consider what happens when we approach
                        //  a 2^32 byte filesize.  Overflows will cause problems.
                        //

                        BytesPerCluster = 1 << Vcb->AllocationSupport.LogOfBytesPerCluster;

                        //
                        //  This can overflow if the target filesize is in the last cluster.
                        //  In this case, we can obviously skip over all of this fancy
                        //  logic and just max out the file right now.
                        //


                        ClusterAlignedFileSize = ((FileSize) + (BytesPerCluster - 1)) &
                                                 ~(BytesPerCluster - 1);


                        if (ClusterAlignedFileSize != 0) {

                            //
                            //  This actually has a chance but the possibility of overflowing
                            //  the numerator is pretty unlikely, made more unlikely by moving
                            //  the divide by 32 up to scale the BytesPerCluster. However, even if it does the
                            //  effect is completely benign.
                            //
                            //  FAT32 with a 64k cluster and over 2^21 clusters would do it (and
                            //  so forth - 2^(16 - 5 + 21) == 2^32).  Since this implies a partition
                            //  of 32gb and a number of clusters (and cluster size) we plan to
                            //  disallow in format for FAT32, the odds of this happening are pretty
                            //  low anyway.    
                            Multiplier = ((Vcb->AllocationSupport.NumberOfFreeClusters *
                                           (BytesPerCluster >> 5)) /
                                          (ClusterAlignedFileSize -
                                           FcbOrDcb->Header.AllocationSize.LowPart)) + 1;
    
                            if (Multiplier > 32) { Multiplier = 32; }

                            // These computations will never overflow a ULONGLONG because a file is capped at 4GB, and 
                            // a single write can be a max of 4GB.
                            AddedAllocation = Multiplier * (ClusterAlignedFileSize - FcbOrDcb->Header.AllocationSize.LowPart);

                            TargetAllocation = FcbOrDcb->Header.AllocationSize.LowPart + AddedAllocation;
    
                            //
                            //  We know that TargetAllocation is in whole clusters. Now
                            //  we check if it exceeded the maximum valid FAT file size.
                            //  If it did, we fall back to allocating up to the maximum legal size.
                            //
    
                            if (TargetAllocation > ~BytesPerCluster + 1) {
    
                                TargetAllocation = ~BytesPerCluster + 1;
                                AddedAllocation = TargetAllocation - FcbOrDcb->Header.AllocationSize.LowPart;
                            }
    
                            //
                            //  Now do an unsafe check here to see if we should even
                            //  try to allocate this much.  If not, just allocate
                            //  the minimum size we need, if so so try it, but if it
                            //  fails, just allocate the minimum size we need.
                            //
    
                            ApproximateClusterCount = (AddedAllocation / BytesPerCluster);
    
                            if (ApproximateClusterCount <= Vcb->AllocationSupport.NumberOfFreeClusters) {
    
                                _SEH2_TRY {
    
                                    FatAddFileAllocation( IrpContext,
                                                          FcbOrDcb,
                                                          FileObject,
                                                          (ULONG)TargetAllocation );
    
                                    AllocateMinimumSize = FALSE;
                                    SetFlag( FcbOrDcb->FcbState, FCB_STATE_TRUNCATE_ON_CLOSE );
    
                                } _SEH2_EXCEPT( _SEH2_GetExceptionCode() == STATUS_DISK_FULL ?
                                          EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH ) {
    
                                      FatResetExceptionState( IrpContext );
                                } _SEH2_END;
                            }
                        }
                    }

                    if ( AllocateMinimumSize ) {


                        FatAddFileAllocation( IrpContext,
                                              FcbOrDcb,
                                              FileObject,
                                              FileSize );


                    }

                    //
                    //  Assert that the allocation worked
                    //


                    NT_ASSERT( FcbOrDcb->Header.AllocationSize.LowPart >= FileSize );


                }

                //
                //  Set the new file size in the Fcb
                //


                NT_ASSERT( FileSize <= FcbOrDcb->Header.AllocationSize.LowPart );

                
                FcbOrDcb->Header.FileSize.LowPart = FileSize;

                //
                //  Extend the cache map, letting mm knows the new file size.
                //  We only have to do this if the file is cached.
                //

                if (CcIsFileCached(FileObject)) {
                    CcSetFileSizes( FileObject, (PCC_FILE_SIZES)&FcbOrDcb->Header.AllocationSize );
                }
            }

            //
            //  Determine if we will deal with extending valid data.
            //

            if ( !CalledByLazyWriter &&
                 !RecursiveWriteThrough &&
                 (StartingVbo + ByteCount > ValidDataLength) ) {

                ExtendingValidData = TRUE;
            
            } else {

                //
                //  If not extending valid data, and we otherwise believe we
                //  could demote from exclusive to shared, do so.  This will
                //  occur when we synchronize tight for noncached coherency
                //  but must defer the demotion until after we decide about
                //  valid data length, which requires it exclusive.  Since we
                //  can't drop/re-pick the resources without letting a pagefault
                //  squirt through, the resource decision was kept up in the air
                //  until now.
                //
                //  Note that we've still got PagingIo exclusive in these cases.
                //
                
                if (FcbCanDemoteToShared) {

                    NT_ASSERT( FcbAcquiredExclusive && ExIsResourceAcquiredExclusiveLite( FcbOrDcb->Header.Resource ));
                    ExConvertExclusiveToSharedLite( FcbOrDcb->Header.Resource );
                    FcbAcquiredExclusive = FALSE;
                }
            }
            
            if (ValidDataToDisk > ValidDataLength) {
                
                ValidDataToCheck = ValidDataToDisk;
            
            } else {
                
                ValidDataToCheck = ValidDataLength;
            }



            //
            // HANDLE THE NON-CACHED CASE
            //

            if ( NonCachedIo ) {

                //
                // Declare some local variables for enumeration through the
                // runs of the file, and an array to store parameters for
                // parallel I/Os
                //

                ULONG SectorSize;

                ULONG BytesToWrite;

                DebugTrace(0, Dbg, "Non cached write.\n", 0);

                //
                //  Round up to sector boundry.  The end of the write interval
                //  must, however, be beyond EOF.
                //

                SectorSize = (ULONG)Vcb->Bpb.BytesPerSector;

                BytesToWrite = (ByteCount + (SectorSize - 1))
                                         & ~(SectorSize - 1);

                //
                //  All requests should be well formed and
                //  make sure we don't wipe out any data
                //

                if (((StartingVbo & (SectorSize - 1)) != 0) ||

                        ((BytesToWrite != ByteCount) &&
                         (StartingVbo + ByteCount < ValidDataLength))) {

                    NT_ASSERT( FALSE );

                    DebugTrace( 0, Dbg, "FatCommonWrite -> STATUS_NOT_IMPLEMENTED\n", 0);
                    try_return( Status = STATUS_NOT_IMPLEMENTED );
                }

                //
                // If this noncached transfer is at least one sector beyond
                // the current ValidDataLength in the Fcb, then we have to
                // zero the sectors in between.  This can happen if the user
                // has opened the file noncached, or if the user has mapped
                // the file and modified a page beyond ValidDataLength.  It
                // *cannot* happen if the user opened the file cached, because
                // ValidDataLength in the Fcb is updated when he does the cached
                // write (we also zero data in the cache at that time), and
                // therefore, we will bypass this test when the data
                // is ultimately written through (by the Lazy Writer).
                //
                //  For the paging file we don't care about security (ie.
                //  stale data), do don't bother zeroing.
                //
                //  We can actually get writes wholly beyond valid data length
                //  from the LazyWriter because of paging Io decoupling.
                //

                if (!CalledByLazyWriter &&
                    !RecursiveWriteThrough &&
                    (StartingVbo > ValidDataToCheck)) {

                    FatZeroData( IrpContext,
                                 Vcb,
                                 FileObject,
                                 ValidDataToCheck,
                                 StartingVbo - ValidDataToCheck );
                }

                //
                // Make sure we write FileSize to the dirent if we
                // are extending it and we are successful.  (This may or
                // may not occur Write Through, but that is fine.)
                //

                WriteFileSizeToDirent = TRUE;

                //
                //  Perform the actual IO
                //

                if (SwitchBackToAsync) {

                    Wait = FALSE;
                    ClearFlag( IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT );
                }

#ifdef SYSCACHE_COMPILE

#define MY_SIZE 0x1000000
#define LONGMAP_COUNTER

#ifdef BITMAP
                //
                //  Maintain a bitmap of IO started on this file.
                //

                {
                    PULONG WriteMask = FcbOrDcb->WriteMask;

                    if (NULL == WriteMask) {

                        WriteMask = FsRtlAllocatePoolWithTag( NonPagedPoolNx,
                                                              (MY_SIZE/PAGE_SIZE) / 8,
                                                              'wtaF' );

                        FcbOrDcb->WriteMask = WriteMask;
                        RtlZeroMemory(WriteMask, (MY_SIZE/PAGE_SIZE) / 8);
                    }

                    if (StartingVbo < MY_SIZE) {

                        ULONG Off = StartingVbo;
                        ULONG Len = BytesToWrite;

                        if (Off + Len > MY_SIZE) {
                            Len = MY_SIZE - Off;
                        }

                        while (Len != 0) {
                            WriteMask[(Off/PAGE_SIZE) / 32] |=
                                1 << (Off/PAGE_SIZE) % 32;

                            Off += PAGE_SIZE;
                            if (Len <= PAGE_SIZE) {
                                break;
                            }
                            Len -= PAGE_SIZE;
                        }
                    }
                }
#endif

#ifdef LONGMAP_COUNTER
                //
                //  Maintain a longmap of IO started on this file, each ulong containing
                //  the value of an ascending counter per write (gives us order information).
                //
                //  Unlike the old bitmask stuff, this is mostly well synchronized.
                //

                {
                    PULONG WriteMask = (PULONG)FcbOrDcb->WriteMask;

                    if (NULL == WriteMask) {

                        WriteMask = FsRtlAllocatePoolWithTag( NonPagedPoolNx,
                                                              (MY_SIZE/PAGE_SIZE) * sizeof(ULONG),
                                                              'wtaF' );

                        FcbOrDcb->WriteMask = WriteMask;
                        RtlZeroMemory(WriteMask, (MY_SIZE/PAGE_SIZE) * sizeof(ULONG));
                    }

                    if (StartingVbo < MY_SIZE) {

                        ULONG Off = StartingVbo;
                        ULONG Len = BytesToWrite;
                        ULONG Tick = InterlockedIncrement( &FcbOrDcb->WriteMaskData );

                        if (Off + Len > MY_SIZE) {
                            Len = MY_SIZE - Off;
                        }

                        while (Len != 0) {
                            InterlockedExchange( WriteMask + Off/PAGE_SIZE, Tick );

                            Off += PAGE_SIZE;
                            if (Len <= PAGE_SIZE) {
                                break;
                            }
                            Len -= PAGE_SIZE;
                        }
                    }
                }
#endif

#endif


                if (FatNonCachedIo( IrpContext,
                                    Irp,
                                    FcbOrDcb,
                                    StartingVbo,
                                    BytesToWrite,
                                    BytesToWrite,
                                    0) == STATUS_PENDING) {


                    UnwindOutstandingAsync = FALSE;

#ifdef _MSC_VER
#pragma prefast( suppress:28931, "convenient for debugging" )
#endif
                    Wait = TRUE;
                    SetFlag( IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT );

                    IrpContext->FatIoContext = NULL;
                    Irp = NULL;

                    //
                    //  As a matter of fact, if we hit this we are in deep trouble
                    //  if VDL is being extended. We are no longer attached to the
                    //  IRP, and have thus lost synchronization.  Note that we should
                    //  not hit this case anymore since we will not re-async vdl extension.
                    //
                    
                    NT_ASSERT( !ExtendingValidData );

                    try_return( Status = STATUS_PENDING );
                }

                //
                //  If the call didn't succeed, raise the error status
                //

                if (!NT_SUCCESS( Status = Irp->IoStatus.Status )) {

                    FatNormalizeAndRaiseStatus( IrpContext, Status );

                } else {

                    ULONG NewValidDataToDisk;

                    //
                    //  Else set the context block to reflect the entire write
                    //  Also assert we got how many bytes we asked for.
                    //

                    NT_ASSERT( Irp->IoStatus.Information == BytesToWrite );

                    Irp->IoStatus.Information = ByteCount;

                    //
                    //  Take this opportunity to update ValidDataToDisk.
                    //

                    NewValidDataToDisk = StartingVbo + ByteCount;

                    if (NewValidDataToDisk > FileSize) {
                        NewValidDataToDisk = FileSize;
                    }

                    if (FcbOrDcb->ValidDataToDisk < NewValidDataToDisk) {
                        FcbOrDcb->ValidDataToDisk = NewValidDataToDisk;
                    }
                }

                //
                // The transfer is either complete, or the Iosb contains the
                // appropriate status.
                //

                try_return( Status );

            } // if No Intermediate Buffering


            //
            // HANDLE CACHED CASE
            //

            else {

                NT_ASSERT( !PagingIo );

                //
                // We delay setting up the file cache until now, in case the
                // caller never does any I/O to the file, and thus
                // FileObject->PrivateCacheMap == NULL.
                //

                if ( FileObject->PrivateCacheMap == NULL ) {

                    DebugTrace(0, Dbg, "Initialize cache mapping.\n", 0);

                    //
                    //  Get the file allocation size, and if it is less than
                    //  the file size, raise file corrupt error.
                    //

                    if (FcbOrDcb->Header.AllocationSize.QuadPart == FCB_LOOKUP_ALLOCATIONSIZE_HINT) {

                        FatLookupFileAllocationSize( IrpContext, FcbOrDcb );
                    }

                    if ( FileSize > FcbOrDcb->Header.AllocationSize.LowPart ) {

                        FatPopUpFileCorrupt( IrpContext, FcbOrDcb );

                        FatRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR );
                    }

                    //
                    //  Now initialize the cache map.
                    //

                    FatInitializeCacheMap( FileObject,
                                           (PCC_FILE_SIZES)&FcbOrDcb->Header.AllocationSize,
                                           FALSE,
                                           &FatData.CacheManagerCallbacks,
                                           FcbOrDcb );

                    CcSetReadAheadGranularity( FileObject, READ_AHEAD_GRANULARITY );

                    //
                    //  Special case large floppy tranfers, and make the file
                    //  object write through.  For small floppy transfers,
                    //  set a timer to go off in a second and flush the file.
                    //
                    //

                    if (!FlagOn( FileObject->Flags, FO_WRITE_THROUGH ) &&
                        FlagOn(Vcb->VcbState, VCB_STATE_FLAG_DEFERRED_FLUSH)) {

                        if (((StartingByte.LowPart & (PAGE_SIZE-1)) == 0) &&
                            (ByteCount >= PAGE_SIZE)) {

                            SetFlag( FileObject->Flags, FO_WRITE_THROUGH );

                        } else {

                            LARGE_INTEGER OneSecondFromNow;
                            PDEFERRED_FLUSH_CONTEXT FlushContext;

                            //
                            //  Get pool and initialize the timer and DPC
                            //

#ifndef __REACTOS__
                            FlushContext = FsRtlAllocatePoolWithTag( NonPagedPoolNx,
#else
                            FlushContext = FsRtlAllocatePoolWithTag( NonPagedPool,
#endif
                                                                     sizeof(DEFERRED_FLUSH_CONTEXT),
                                                                     TAG_DEFERRED_FLUSH_CONTEXT );

                            KeInitializeTimer( &FlushContext->Timer );

                            KeInitializeDpc( &FlushContext->Dpc,
                                             FatDeferredFlushDpc,
                                             FlushContext );


                            //
                            //  We have to reference the file object here.
                            //

                            ObReferenceObject( FileObject );

                            FlushContext->File = FileObject;

                            //
                            //  Let'er rip!
                            //

                            OneSecondFromNow.QuadPart = (LONG)-1*1000*1000*10;

                            KeSetTimer( &FlushContext->Timer,
                                        OneSecondFromNow,
                                        &FlushContext->Dpc );
                        }
                    }
                }

                //
                // If this write is beyond valid data length, then we
                // must zero the data in between.
                //

                if ( StartingVbo > ValidDataToCheck ) {

                    //
                    // Call the Cache Manager to zero the data.
                    //

                    if (!FatZeroData( IrpContext,
                                      Vcb,
                                      FileObject,
                                      ValidDataToCheck,
                                      StartingVbo - ValidDataToCheck )) {

                        DebugTrace( 0, Dbg, "Cached Write could not wait to zero\n", 0 );

                        try_return( PostIrp = TRUE );
                    }
                }

                WriteFileSizeToDirent = BooleanFlagOn(IrpContext->Flags,
                                                      IRP_CONTEXT_FLAG_WRITE_THROUGH);


                //
                // DO A NORMAL CACHED WRITE, if the MDL bit is not set,
                //

                if (!FlagOn(IrpContext->MinorFunction, IRP_MN_MDL)) {

                    DebugTrace(0, Dbg, "Cached write.\n", 0);

                    //
                    //  Get hold of the user's buffer.
                    //

                    SystemBuffer = FatMapUserBuffer( IrpContext, Irp );

                    //
                    // Do the write, possibly writing through
                    //

#if (NTDDI_VERSION >= NTDDI_WIN8)
                    if (!CcCopyWriteEx( FileObject,
                                        &StartingByte,
                                        ByteCount,
                                        Wait,
                                        SystemBuffer,
                                        Irp->Tail.Overlay.Thread )) {
#else
                    if (!CcCopyWrite( FileObject,
                                      &StartingByte,
                                      ByteCount,
                                      Wait,
                                      SystemBuffer )) {
#endif

                        DebugTrace( 0, Dbg, "Cached Write could not wait\n", 0 );

                        try_return( PostIrp = TRUE );
                    }

                    Irp->IoStatus.Status = STATUS_SUCCESS;
                    Irp->IoStatus.Information = ByteCount;

                    try_return( Status = STATUS_SUCCESS );

                } else {

                    //
                    //  DO AN MDL WRITE
                    //

                    DebugTrace(0, Dbg, "MDL write.\n", 0);

                    NT_ASSERT( Wait );

                    CcPrepareMdlWrite( FileObject,
                                       &StartingByte,
                                       ByteCount,
                                       &Irp->MdlAddress,
                                       &Irp->IoStatus );

                    Status = Irp->IoStatus.Status;

                    try_return( Status );
                }
            }
        }

        //
        //  These two cases correspond to a system write directory file and
        //  ea file.
        //

        if (( TypeOfOpen == DirectoryFile ) || ( TypeOfOpen == EaFile)
            ) {

            ULONG SectorSize;

#if FASTFATDBG
            if ( TypeOfOpen == DirectoryFile ) {
                DebugTrace(0, Dbg, "Type of write is directoryfile\n", 0);                
            } else if ( TypeOfOpen == EaFile) {
                DebugTrace(0, Dbg, "Type of write is eafile\n", 0);
            }
#endif

            //
            //  Make sure the FcbOrDcb is still good
            //

            FatVerifyFcb( IrpContext, FcbOrDcb );

            //
            //  Synchronize here with people deleting directories and
            //  mucking with the internals of the EA file.
            //

            if (!ExAcquireSharedStarveExclusive( FcbOrDcb->Header.PagingIoResource,
                                          Wait )) {

                DebugTrace( 0, Dbg, "Cannot acquire FcbOrDcb = %p shared without waiting\n", FcbOrDcb );

                try_return( PostIrp = TRUE );
            }

            PagingIoResourceAcquired = TRUE;

            if (!Wait) {

                IrpContext->FatIoContext->Wait.Async.Resource =
                    FcbOrDcb->Header.PagingIoResource;
            }

            //
            //  Check to see if we colided with a MoveFile call, and if
            //  so block until it completes.
            //

            if (FcbOrDcb->MoveFileEvent) {

                (VOID)KeWaitForSingleObject( FcbOrDcb->MoveFileEvent,
                                             Executive,
                                             KernelMode,
                                             FALSE,
                                             NULL );
            }

            //
            //  If we weren't called by the Lazy Writer, then this write
            //  must be the result of a write-through or flush operation.
            //  Setting the IrpContext flag, will cause DevIoSup.c to
            //  write-through the data to the disk.
            //

            if (!FlagOn((ULONG_PTR)IoGetTopLevelIrp(), FSRTL_CACHE_TOP_LEVEL_IRP)) {

                SetFlag( IrpContext->Flags, IRP_CONTEXT_FLAG_WRITE_THROUGH );
            }

            //
            //  For the noncached case, assert that everything is sector
            //  alligned.
            //
            
#ifdef _MSC_VER
#pragma prefast( suppress:28931, "needed for debug build" )
#endif
            SectorSize = (ULONG)Vcb->Bpb.BytesPerSector;

            //
            //  We make several assumptions about these two types of files.
            //  Make sure all of them are true.
            //

            NT_ASSERT( NonCachedIo && PagingIo );
            NT_ASSERT( ((StartingVbo | ByteCount) & (SectorSize - 1)) == 0 );


                //
                //  These calls must always be within the allocation size, which is
                //  convienently the same as filesize, which conveniently doesn't
                //  get reset to a hint value when we verify the volume.
                //

                if (StartingVbo >= FcbOrDcb->Header.FileSize.LowPart) {

                    DebugTrace( 0, Dbg, "PagingIo dirent started beyond EOF.\n", 0 );

                    Irp->IoStatus.Information = 0;

                    try_return( Status = STATUS_SUCCESS );
                }

                if ( StartingVbo + ByteCount > FcbOrDcb->Header.FileSize.LowPart ) {

                    DebugTrace( 0, Dbg, "PagingIo dirent extending beyond EOF.\n", 0 );
                    ByteCount = FcbOrDcb->Header.FileSize.LowPart - StartingVbo;
                }

            
            //
            //  Perform the actual IO
            //

            if (FatNonCachedIo( IrpContext,
                                Irp,
                                FcbOrDcb,
                                StartingVbo,
                                ByteCount,
                                ByteCount,
                                0 ) == STATUS_PENDING) {

                IrpContext->FatIoContext = NULL;

                Irp = NULL;

                try_return( Status = STATUS_PENDING );
            }

            //
            //  The transfer is either complete, or the Iosb contains the
            //  appropriate status.
            //
            //  Also, mark the volume as needing verification to automatically
            //  clean up stuff.
            //

            if (!NT_SUCCESS( Status = Irp->IoStatus.Status )) {

                FatNormalizeAndRaiseStatus( IrpContext, Status );
            }

            try_return( Status );
        }

        //
        // This is the case of a user who openned a directory. No writing is
        // allowed.
        //

        if ( TypeOfOpen == UserDirectoryOpen ) {

            DebugTrace( 0, Dbg, "FatCommonWrite -> STATUS_INVALID_PARAMETER\n", 0);

            try_return( Status = STATUS_INVALID_PARAMETER );
        }

        //
        //  If we get this far, something really serious is wrong.
        //

        DebugDump("Illegal TypeOfOpen\n", 0, FcbOrDcb );

#ifdef _MSC_VER
#pragma prefast( suppress:28159, "things are seriously wrong if we get here" )
#endif
        FatBugCheck( TypeOfOpen, (ULONG_PTR) FcbOrDcb, 0 );

    try_exit: NOTHING;


        //
        //  If the request was not posted and there is still an Irp,
        //  deal with it.
        //

        if (Irp) {

            if ( !PostIrp ) {

                ULONG ActualBytesWrote;

                DebugTrace( 0, Dbg, "Completing request with status = %08lx\n",
                            Status);

                DebugTrace( 0, Dbg, "                   Information = %08lx\n",
                            Irp->IoStatus.Information);

                //
                //  Record the total number of bytes actually written
                //

                ActualBytesWrote = (ULONG)Irp->IoStatus.Information;

                //
                //  If the file was opened for Synchronous IO, update the current
                //  file position.
                //

                if (SynchronousIo && !PagingIo) {

                    FileObject->CurrentByteOffset.LowPart =
                         StartingVbo + (NT_ERROR( Status ) ? 0 : ActualBytesWrote);
                }

                //
                //  The following are things we only do if we were successful
                //

                if ( NT_SUCCESS( Status ) ) {

                    //
                    //  If this was not PagingIo, mark that the modify
                    //  time on the dirent needs to be updated on close.
                    //

                    if ( !PagingIo ) {

                        SetFlag( FileObject->Flags, FO_FILE_MODIFIED );
                    }

                    //
                    //  If we extended the file size and we are meant to
                    //  immediately update the dirent, do so. (This flag is
                    //  set for either Write Through or noncached, because
                    //  in either case the data and any necessary zeros are
                    //  actually written to the file.)
                    //

                    if ( ExtendingFile && WriteFileSizeToDirent ) {

                        NT_ASSERT( FileObject->DeleteAccess || FileObject->WriteAccess );

                        FatSetFileSizeInDirent( IrpContext, FcbOrDcb, NULL );

                        //
                        //  Report that a file size has changed.
                        //

                        FatNotifyReportChange( IrpContext,
                                               Vcb,
                                               FcbOrDcb,
                                               FILE_NOTIFY_CHANGE_SIZE,
                                               FILE_ACTION_MODIFIED );
                    }

                    if ( ExtendingFile && !WriteFileSizeToDirent ) {

                        SetFlag( FileObject->Flags, FO_FILE_SIZE_CHANGED );
                    }

                    if ( ExtendingValidData ) {

                        ULONG EndingVboWritten = StartingVbo + ActualBytesWrote;

                        //
                        //  Never set a ValidDataLength greater than FileSize.
                        //

                        if ( FileSize < EndingVboWritten ) {

                            FcbOrDcb->Header.ValidDataLength.LowPart = FileSize;

                        } else {

                            FcbOrDcb->Header.ValidDataLength.LowPart = EndingVboWritten;
                        }

                        //
                        //  Now, if we are noncached and the file is cached, we must
                        //  tell the cache manager about the VDL extension so that
                        //  async cached IO will not be optimized into zero-page faults
                        //  beyond where it believes VDL is.
                        //
                        //  In the cached case, since Cc did the work, it has updated
                        //  itself already.
                        //

                        if (NonCachedIo && CcIsFileCached(FileObject)) {
                            CcSetFileSizes( FileObject, (PCC_FILE_SIZES)&FcbOrDcb->Header.AllocationSize );
                        }
                    }
                    
                }
                
                //
                //  Note that we have to unpin repinned Bcbs here after the above
                //  work, but if we are going to post the request, we must do this
                //  before the post (below).
                //

                FatUnpinRepinnedBcbs( IrpContext );

            } else {

                //
                //  Take action if the Oplock package is not going to post the Irp.
                //

                if (!OplockPostIrp) {

                    FatUnpinRepinnedBcbs( IrpContext );

                    if ( ExtendingFile ) {

                        //
                        //  We need the PagingIo resource exclusive whenever we
                        //  pull back either file size or valid data length.
                        //

                        NT_ASSERT( FcbOrDcb->Header.PagingIoResource != NULL );

                        (VOID)ExAcquireResourceExclusiveLite(FcbOrDcb->Header.PagingIoResource, TRUE);

                        FcbOrDcb->Header.FileSize.LowPart = InitialFileSize;

                        NT_ASSERT( FcbOrDcb->Header.FileSize.LowPart <= FcbOrDcb->Header.AllocationSize.LowPart );

                        //
                        //  Pull back the cache map as well
                        //

                        if (FileObject->SectionObjectPointer->SharedCacheMap != NULL) {

                            *CcGetFileSizePointer(FileObject) = FcbOrDcb->Header.FileSize;
                        }

                        ExReleaseResourceLite( FcbOrDcb->Header.PagingIoResource );
                    }

                    DebugTrace( 0, Dbg, "Passing request to Fsp\n", 0 );

                    Status = FatFsdPostRequest(IrpContext, Irp);
                }
            }
        }

    } _SEH2_FINALLY {

        DebugUnwind( FatCommonWrite );

        if (_SEH2_AbnormalTermination()) {

            //
            //  Restore initial file size and valid data length
            //

            if (ExtendingFile || ExtendingValidData) {

                //
                //  We got an error, pull back the file size if we extended it.
                //

                FcbOrDcb->Header.FileSize.LowPart = InitialFileSize;
                FcbOrDcb->Header.ValidDataLength.LowPart = InitialValidDataLength;

                NT_ASSERT( FcbOrDcb->Header.FileSize.LowPart <= FcbOrDcb->Header.AllocationSize.LowPart );

                //
                //  Pull back the cache map as well
                //

                if (FileObject->SectionObjectPointer->SharedCacheMap != NULL) {

                    *CcGetFileSizePointer(FileObject) = FcbOrDcb->Header.FileSize;
                }
            }
        }

        //
        //  Check if this needs to be backed out.
        //

        if (UnwindOutstandingAsync) {

            ExInterlockedAddUlong( &FcbOrDcb->NonPaged->OutstandingAsyncWrites,
                                   0xffffffff,
                                   &FatData.GeneralSpinLock );
        }

        //
        //  If the FcbOrDcb has been acquired, release it.
        //

        if (FcbOrDcbAcquired && Irp) {

            FatReleaseFcb( NULL, FcbOrDcb );
        }

        if (PagingIoResourceAcquired && Irp) {

            ExReleaseResourceLite( FcbOrDcb->Header.PagingIoResource );
        }

        //
        //  Complete the request if we didn't post it and no exception
        //
        //  Note that FatCompleteRequest does the right thing if either
        //  IrpContext or Irp are NULL
        //

        if ( !PostIrp && !_SEH2_AbnormalTermination() ) {

            FatCompleteRequest( IrpContext, Irp, Status );
        }

        DebugTrace(-1, Dbg, "FatCommonWrite -> %08lx\n", Status );
    } _SEH2_END;

    return Status;
}


//
//  Local support routine
//

VOID
NTAPI
FatDeferredFlushDpc (
    _In_ PKDPC Dpc,
    _In_opt_ PVOID DeferredContext,
    _In_opt_ PVOID SystemArgument1,
    _In_opt_ PVOID SystemArgument2
    )

/*++

Routine Description:

    This routine is dispatched 1 second after a small write to a deferred
    write device that initialized the cache map.  It exqueues an executive
    worker thread to perform the actual task of flushing the file.

Arguments:

    DeferredContext - Contains the deferred flush context.

Return Value:

    None.

--*/

{
    PDEFERRED_FLUSH_CONTEXT FlushContext;

    UNREFERENCED_PARAMETER( SystemArgument1 );
    UNREFERENCED_PARAMETER( SystemArgument2 );
    UNREFERENCED_PARAMETER( Dpc );

    FlushContext = (PDEFERRED_FLUSH_CONTEXT)DeferredContext;

    //
    //  Send it off
    //

    ExInitializeWorkItem( &FlushContext->Item,
                          FatDeferredFlush,
                          FlushContext );

#ifdef _MSC_VER
#pragma prefast( suppress:28159, "prefast indicates this API is obsolete, but it's ok for fastfat to keep using it" )
#endif
    ExQueueWorkItem( &FlushContext->Item, CriticalWorkQueue );
}


//
//  Local support routine
//

VOID
NTAPI
FatDeferredFlush (
    _In_ PVOID Parameter
    )

/*++

Routine Description:

    This routine performs the actual task of flushing the file.

Arguments:

    DeferredContext - Contains the deferred flush context.

Return Value:

    None.

--*/

{

    PFILE_OBJECT File;
    PVCB Vcb;
    PFCB FcbOrDcb;
    PCCB Ccb;

    PAGED_CODE();

    File = ((PDEFERRED_FLUSH_CONTEXT)Parameter)->File;

    FatDecodeFileObject(File, &Vcb, &FcbOrDcb, &Ccb);
    NT_ASSERT( FcbOrDcb != NULL );
    
    //
    //  Make us appear as a top level FSP request so that we will
    //  receive any errors from the flush.
    //

    IoSetTopLevelIrp( (PIRP)FSRTL_FSP_TOP_LEVEL_IRP );

    ExAcquireResourceExclusiveLite( FcbOrDcb->Header.Resource, TRUE );
    ExAcquireResourceSharedLite( FcbOrDcb->Header.PagingIoResource, TRUE );
    
    CcFlushCache( File->SectionObjectPointer, NULL, 0, NULL );

    ExReleaseResourceLite( FcbOrDcb->Header.PagingIoResource );
    ExReleaseResourceLite( FcbOrDcb->Header.Resource );

    IoSetTopLevelIrp( NULL );

    ObDereferenceObject( File );

    ExFreePool( Parameter );

}


