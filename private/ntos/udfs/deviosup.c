/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    DevIoSup.c

Abstract:

    This module implements the low lever disk read/write support for Udfs.

Author:

    Dan Lovinger    [DanLo]   11-Jun-1996

Revision History:

--*/

#include "UdfProcs.h"

//
//  The Bug check file id for this module
//

#define BugCheckFileId                   (UDFS_BUG_CHECK_DEVIOSUP)

//
//  The local debug trace level
//

#define Dbg                              (UDFS_DEBUG_LEVEL_DEVIOSUP)

//
//  Local structure definitions
//

//
//  An array of these structures is passed to UdfMultipleAsync describing
//  a set of runs to execute in parallel.
//

typedef struct _IO_RUN {

    //
    //  Disk offset to read from and number of bytes to read.  These
    //  must be a multiple of a sector and the disk offset is also a
    //  multiple of sector.
    //

    LONGLONG DiskOffset;
    ULONG DiskByteCount;

    //
    //  Current position in user buffer.  This is the final destination for
    //  this portion of the Io transfer.
    //

    PVOID UserBuffer;

    //
    //  Buffer to perform the transfer to.  If this is the same as the
    //  user buffer above then we are using the user's buffer.  Otherwise
    //  we either allocated a temporary buffer or are using a different portion
    //  of the user's buffer.
    //
    //  TransferBuffer - Read full sectors into this location.  This can
    //      be a pointer into the user's buffer at the exact location the
    //      data should go.  It can also be an earlier point in the user's
    //      buffer if the complete I/O doesn't start on a sector boundary.
    //      It may also be a pointer into an allocated buffer.
    //
    //  TransferByteCount - Count of bytes to transfer to user's buffer.  A
    //      value of zero indicates that we did do the transfer into the
    //      user's buffer directly.
    //
    //  TransferBufferOffset - Offset in this buffer to begin the transfer
    //      to the user's buffer.
    //

    PVOID TransferBuffer;
    ULONG TransferByteCount;
    ULONG TransferBufferOffset;

    //
    //  This is the Mdl describing the locked pages in memory.  It may
    //  be allocated to describe the allocated buffer.  Or it may be
    //  the Mdl in the originating Irp.  The MdlOffset is the offset of
    //  the current buffer from the beginning of the buffer described by
    //  the Mdl below.  If the TransferMdl is not the same as the Mdl
    //  in the user's Irp then we know we have allocated it.
    //

    PMDL TransferMdl;
    PVOID TransferVirtualAddress;

    //
    //  Associated Irp used to perform the Io.
    //

    PIRP SavedIrp;

} IO_RUN;
typedef IO_RUN *PIO_RUN;

#define MAX_PARALLEL_IOS            5

//
//  Local support routines
//

BOOLEAN
UdfPrepareBuffers (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN PFCB Fcb,
    IN PVOID UserBuffer,
    IN ULONG UserBufferOffset,
    IN LONGLONG StartingOffset,
    IN ULONG ByteCount,
    IN PIO_RUN IoRuns,
    IN PULONG RunCount,
    IN PULONG ThisByteCount,
    OUT PBOOLEAN SparseRuns
    );

BOOLEAN
UdfFinishBuffers (
    IN PIRP_CONTEXT IrpContext,
    IN PIO_RUN IoRuns,
    IN ULONG RunCount,
    IN BOOLEAN FinalCleanup
    );

VOID
UdfMultipleAsync (
    IN PIRP_CONTEXT IrpContext,
    IN ULONG RunCount,
    IN PIO_RUN IoRuns
    );

VOID
UdfSingleAsync (
    IN PIRP_CONTEXT IrpContext,
    IN LONGLONG ByteOffset,
    IN ULONG ByteCount
    );

VOID
UdfWaitSync (
    IN PIRP_CONTEXT IrpContext
    );

NTSTATUS
UdfMultiSyncCompletionRoutine (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

NTSTATUS
UdfMultiAsyncCompletionRoutine (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

NTSTATUS
UdfSingleSyncCompletionRoutine (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

NTSTATUS
UdfSingleAsyncCompletionRoutine (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

#ifdef ALLOC_PRAGMA
#endif

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, UdfCreateUserMdl)
#pragma alloc_text(PAGE, UdfMultipleAsync)
#pragma alloc_text(PAGE, UdfNonCachedRead)
#pragma alloc_text(PAGE, UdfFinishBuffers)
#pragma alloc_text(PAGE, UdfPrepareBuffers)
#pragma alloc_text(PAGE, UdfSingleAsync)
#pragma alloc_text(PAGE, UdfWaitSync)
#pragma alloc_text(PAGE, UdfPerformDevIoCtrl)
#pragma alloc_text(PAGE, UdfReadSectors)
#endif



NTSTATUS
UdfNonCachedRead (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN LONGLONG StartingOffset,
    IN ULONG ByteCount
    )

/*++

Routine Description:

    This routine performs the non-cached reads of sectors.  This is done by
    performing the following in a loop.

        Fill in the IoRuns array for the next block of Io.
        Send the Io to the device.
        Perform any cleanup on the Io runs array.

    We will not do async Io to any request that generates non-aligned Io.
    Also we will not perform async Io if it will exceed the size of our
    IoRuns array.  These should be the unusual cases but we will raise
    or return CANT_WAIT in this routine if we detect this case.

Arguments:

    Fcb - Fcb representing the file to read.

    StartingOffset - Logical offset in the file to read from.

    ByteCount - Number of bytes to read.

Return Value:

    NTSTATUS - Status indicating the result of the operation.

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;

    IO_RUN IoRuns[MAX_PARALLEL_IOS];
    ULONG RunCount = 0;
    ULONG CleanupRunCount = 0;

    PVOID UserBuffer;
    ULONG UserBufferOffset = 0;
    LONGLONG CurrentOffset = StartingOffset;
    ULONG RemainingByteCount = ByteCount;
    ULONG ThisByteCount;

    BOOLEAN Unaligned;
    BOOLEAN SparseRuns;
    BOOLEAN FlushIoBuffers = FALSE;
    BOOLEAN FirstPass = TRUE;

    PAGED_CODE();

    //
    //  We want to make sure the user's buffer is locked in all cases.
    //

    if (IrpContext->Irp->MdlAddress == NULL) {

        UdfCreateUserMdl( IrpContext, ByteCount, TRUE );
    }

    //
    //  Use a try-finally to perform the final cleanup.
    //

    try {

        UdfMapUserBuffer( IrpContext, &UserBuffer);

        //
        //  Loop while there are more bytes to transfer.
        //

        do {

            //
            //  Call prepare buffers to set up the next entries
            //  in the IoRuns array.  Remember if there are any
            //  unaligned entries.
            //

            RtlZeroMemory( IoRuns, sizeof( IoRuns ));

            Unaligned = UdfPrepareBuffers( IrpContext,
                                           IrpContext->Irp,
                                           Fcb,
                                           UserBuffer,
                                           UserBufferOffset,
                                           CurrentOffset,
                                           RemainingByteCount,
                                           IoRuns,
                                           &CleanupRunCount,
                                           &ThisByteCount,
                                           &SparseRuns );


            RunCount = CleanupRunCount;

            //
            //  Quickly finish if we wound up having no IO to perform.  This will
            //  occur in the presence of unrecorded sectors.
            //

            ASSERT( !(SparseRuns && FlagOn( Fcb->FcbState, FCB_STATE_VMCB_MAPPING|FCB_STATE_EMBEDDED_DATA )));

            if (RunCount == 0) {

                try_leave( Status = IrpContext->Irp->IoStatus.Status = STATUS_SUCCESS );
            }

            //
            //  If this is an async request and there aren't enough entries
            //  in the Io array then post the request.  This routine will
            //  always raise if we are doing any unaligned Io for an
            //  async request.
            //

            if ((ThisByteCount < RemainingByteCount) &&
                !FlagOn( IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT )) {

                UdfRaiseStatus( IrpContext, STATUS_CANT_WAIT );
            }

            //
            //  If the entire Io is contained in a single run then
            //  we can pass the Io down to the driver.  Send the driver down
            //  and wait on the result if this is synchronous.  We cannot
            //  do this simple form (just chucking the IRP down) if some
            //  sparse runs were encountered.
            //

            if ((RunCount == 1) && !Unaligned && !SparseRuns && FirstPass) {

                UdfSingleAsync( IrpContext,
                                IoRuns[0].DiskOffset,
                                IoRuns[0].DiskByteCount );

                //
                //  No cleanup needed for the IoRuns array here.
                //

                CleanupRunCount = 0;

                //
                //  Wait if we are synchronous, otherwise return
                //

                if (FlagOn( IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT )) {

                    UdfWaitSync( IrpContext );

                    Status = IrpContext->Irp->IoStatus.Status;

                //
                //  Our completion routine will free the Io context but
                //  we do want to return STATUS_PENDING.
                //

                } else {

                    ClearFlag( IrpContext->Flags, IRP_CONTEXT_FLAG_ALLOC_IO );
                    Status = STATUS_PENDING;
                }

                try_leave( NOTHING );
            }

            //
            //  Otherwise we will perform multiple Io to read in the data.
            //
            
            UdfMultipleAsync( IrpContext, RunCount, IoRuns );

            //
            //  No cleanup needed on the IoRuns now.
            //

            CleanupRunCount = 0;

            //
            //  If this is a synchronous request then perform any necessary
            //  post-processing.
            //

            if (FlagOn( IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT )) {

                //
                //  Wait for the request to complete.
                //

                UdfWaitSync( IrpContext );

                Status = IrpContext->Irp->IoStatus.Status;

                //
                //  Exit this loop if there is an error.
                //

                if (!NT_SUCCESS( Status )) {

                    try_leave( NOTHING );
                }

                //
                //  Perform post read operations on the IoRuns if
                //  necessary.
                //

                if (Unaligned &&
                    UdfFinishBuffers( IrpContext, IoRuns, RunCount, FALSE )) {

                    FlushIoBuffers = TRUE;
                }

                //
                //  Exit this loop if there are no more bytes to transfer
                //  or we have any error.
                //

                RemainingByteCount -= ThisByteCount;
                CurrentOffset += ThisByteCount;
                UserBuffer = Add2Ptr( UserBuffer, ThisByteCount, PVOID );
                UserBufferOffset += ThisByteCount;

            //
            //  Otherwise this is an asynchronous request.  Always return
            //  STATUS_PENDING.
            //

            } else {

                ClearFlag( IrpContext->Flags, IRP_CONTEXT_FLAG_ALLOC_IO );
                CleanupRunCount = 0;
                try_leave( Status = STATUS_PENDING );
                break;
            }

            FirstPass = FALSE;
        } while (RemainingByteCount != 0);

        //
        //  Flush the hardware cache if we performed any copy operations.
        //

        if (FlushIoBuffers) {

            KeFlushIoBuffers( IrpContext->Irp->MdlAddress, TRUE, FALSE );
        }

    } finally {

        DebugUnwind( "UdfNonCachedRead" );

        //
        //  Perform final cleanup on the IoRuns if necessary.
        //

        if (CleanupRunCount != 0) {

            UdfFinishBuffers( IrpContext, IoRuns, CleanupRunCount, TRUE );
        }
    }

    return Status;
}


NTSTATUS
UdfCreateUserMdl (
    IN PIRP_CONTEXT IrpContext,
    IN ULONG BufferLength,
    IN BOOLEAN RaiseOnError
    )

/*++

Routine Description:

    This routine locks the specified buffer for read access (we only write into
    the buffer).  The file system requires this routine since it does not
    ask the I/O system to lock its buffers for direct I/O.  This routine
    may only be called from the Fsd while still in the user context.

    This routine is only called if there is not already an Mdl.

Arguments:

    BufferLength - Length of user buffer.

    RaiseOnError - Indicates if our caller wants this routine to raise on
        an error condition.

Return Value:

    NTSTATUS - Status from this routine.  Error status only returned if
        RaiseOnError is FALSE.

--*/

{
    NTSTATUS Status = STATUS_INSUFFICIENT_RESOURCES;
    PMDL Mdl;

    PAGED_CODE();

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_IRP( IrpContext->Irp );
    ASSERT( IrpContext->Irp->MdlAddress == NULL );

    //
    // Allocate the Mdl, and Raise if we fail.
    //

    Mdl = IoAllocateMdl( IrpContext->Irp->UserBuffer,
                         BufferLength,
                         FALSE,
                         FALSE,
                         IrpContext->Irp );

    if (Mdl != NULL) {

        //
        //  Now probe the buffer described by the Irp.  If we get an exception,
        //  deallocate the Mdl and return the appropriate "expected" status.
        //

        try {

            MmProbeAndLockPages( Mdl, IrpContext->Irp->RequestorMode, IoWriteAccess );

            Status = STATUS_SUCCESS;

        } except(EXCEPTION_EXECUTE_HANDLER) {

            Status = GetExceptionCode();

            IoFreeMdl( Mdl );
            IrpContext->Irp->MdlAddress = NULL;

            if (!FsRtlIsNtstatusExpected( Status )) {

                Status = STATUS_INVALID_USER_BUFFER;
            }
        }
    }

    //
    //  Check if we are to raise or return
    //

    if (Status != STATUS_SUCCESS) {

        if (RaiseOnError) {

            UdfRaiseStatus( IrpContext, Status );
        }
    }

    //
    //  Return the status code.
    //

    return Status;
}


NTSTATUS
UdfPerformDevIoCtrl (
    IN PIRP_CONTEXT IrpContext,
    IN ULONG IoControlCode,
    IN PDEVICE_OBJECT Device,
    OUT PVOID OutputBuffer OPTIONAL,
    IN ULONG OutputBufferLength,
    IN BOOLEAN InternalDeviceIoControl,
    IN BOOLEAN OverrideVerify,
    OUT PIO_STATUS_BLOCK Iosb OPTIONAL
    )

/*++

Routine Description:

    This routine is called to perform DevIoCtrl functions internally within
    the filesystem.  We take the status from the driver and return it to our
    caller.

Arguments:

    IoControlCode - Code to send to driver.

    Device - This is the device to send the request to.

    OutPutBuffer - Pointer to output buffer.

    OutputBufferLength - Length of output buffer above.

    InternalDeviceIoControl - Indicates if this is an internal or external
        Io control code.

    OverrideVerify - Indicates if we should tell the driver not to return
        STATUS_VERIFY_REQUIRED for mount and verify.

    Iosb - If specified, we return the results of the operation here.

Return Value:

    NTSTATUS - Status returned by next lower driver.

--*/

{
    NTSTATUS Status;
    PIRP Irp;
    KEVENT Event;
    IO_STATUS_BLOCK LocalIosb;
    PIO_STATUS_BLOCK IosbToUse = &LocalIosb;

    PAGED_CODE();

    //
    //  Check if the user gave us an Iosb.
    //

    if (ARGUMENT_PRESENT( Iosb )) {

        IosbToUse = Iosb;
    }

    IosbToUse->Status = 0;
    IosbToUse->Information = 0;

    KeInitializeEvent( &Event, NotificationEvent, FALSE );

    Irp = IoBuildDeviceIoControlRequest( IoControlCode,
                                         Device,
                                         NULL,
                                         0,
                                         OutputBuffer,
                                         OutputBufferLength,
                                         InternalDeviceIoControl,
                                         &Event,
                                         IosbToUse );

    if (Irp == NULL) {

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    if (OverrideVerify) {

        SetFlag( IoGetNextIrpStackLocation( Irp )->Flags, SL_OVERRIDE_VERIFY_VOLUME );
    }

    Status = IoCallDriver( Device, Irp );

    //
    //  We check for device not ready by first checking Status
    //  and then if status pending was returned, the Iosb status
    //  value.
    //

    if (Status == STATUS_PENDING) {

        (VOID) KeWaitForSingleObject( &Event,
                                      Executive,
                                      KernelMode,
                                      FALSE,
                                      (PLARGE_INTEGER)NULL );

        Status = IosbToUse->Status;
    }

    return Status;

    UNREFERENCED_PARAMETER( IrpContext );
}


NTSTATUS
UdfReadSectors (
    IN PIRP_CONTEXT IrpContext,
    IN LONGLONG StartingOffset,
    IN ULONG ByteCount,
    IN BOOLEAN ReturnError,
    IN OUT PVOID Buffer,
    IN PDEVICE_OBJECT TargetDeviceObject
    )

/*++

Routine Description:

    This routine is called to transfer sectors from the disk to a
    specified buffer.  It is used for mount and volume verify operations.

    This routine is synchronous, it will not return until the operation
    is complete or until the operation fails.

    The routine allocates an IRP and then passes this IRP to a lower
    level driver.  Errors may occur in the allocation of this IRP or
    in the operation of the lower driver.

Arguments:

    StartingOffset - Logical offset on the disk to start the read.  This
        must be on a sector boundary, no check is made here.

    ByteCount - Number of bytes to read.  This is an integral number of
        sectors, or otherwise a value we know the driver can handle,
        no check is made here to confirm this.

    ReturnError - Indicates whether we should return TRUE or FALSE
        to indicate an error or raise an error condition.  This only applies
        to the result of the IO.  Any other error may cause a raise.

    Buffer - Buffer to transfer the disk data into.

    TargetDeviceObject - The device object for the volume to be read.

Return Value:

    The final status of the operation.

--*/

{
    PLONGLONG UseStartingOffset;
    LONGLONG LocalStartingOffset;
    NTSTATUS Status;
    KEVENT  Event;
    PIRP Irp;

    PAGED_CODE();

    DebugTrace(( +1, Dbg,
                 "UdfReadSectors, %x%08x +%x -> %08x from DO %08x\n",
                 ((PLARGE_INTEGER)&StartingOffset)->HighPart,
                 ((PLARGE_INTEGER)&StartingOffset)->LowPart,
                 ByteCount,
                 Buffer,
                 TargetDeviceObject ));
    
    //
    //  For the time being, we assume that we only read sector-at-a-time.
    //  This simplifies sparing, and is the only way I am aware of this
    //  code would not be ready for blocksize != sectorsize.  It just is
    //  not worth writing dead (but straightforward) code right now.
    //

    ASSERT( IrpContext->Vcb == NULL || ByteCount == SectorSize( IrpContext->Vcb ));

    //
    //  If the volume is spared (and at a point where sparing is possible),
    //  check if a mapping needs to be performed.
    //
    
    if (IrpContext->Vcb &&
        IrpContext->Vcb->Pcb &&
        IrpContext->Vcb->Pcb->SparingMcb) {
        
        LONGLONG SparingPsn;
    
        if (FsRtlLookupLargeMcbEntry( IrpContext->Vcb->Pcb->SparingMcb,
                                      LlSectorsFromBytes( IrpContext->Vcb, StartingOffset ),
                                      &SparingPsn,
                                      NULL,
                                      NULL,
                                      NULL,
                                      NULL ) &&
            SparingPsn != -1) {

            StartingOffset = BytesFromSectors( IrpContext->Vcb, (ULONG) SparingPsn );
        }
    }
    
    //
    //  Initialize the event.
    //

    KeInitializeEvent( &Event, NotificationEvent, FALSE );

    //
    //  Correct the starting offset by the method 2 fixup if neccesary.  This also
    //  assumes sector-at-a-time and sector == block so we don't need to fragment
    //  the request or check if it spans a packet boundary.
    //
    //  We assume that no fixups are required until a Vcb exists.  This is true
    //  since volume recognition may proceed in the first packet.
    //

    UseStartingOffset = &StartingOffset;

    if (IrpContext->Vcb &&
        FlagOn( IrpContext->Vcb->VcbState, VCB_STATE_METHOD_2_FIXUP )) {

        LocalStartingOffset = UdfMethod2TransformByteOffset( IrpContext->Vcb, StartingOffset );
        UseStartingOffset = &LocalStartingOffset;

        DebugTrace(( 0, Dbg,
                     "UdfReadSectors, Method2 Fixup to %x%08x\n",
                     ((PLARGE_INTEGER)UseStartingOffset)->HighPart,
                     ((PLARGE_INTEGER)UseStartingOffset)->LowPart ));
    }

    //
    //  Attempt to allocate the IRP.  If unsuccessful, raise
    //  STATUS_INSUFFICIENT_RESOURCES.
    //

    Irp = IoBuildSynchronousFsdRequest( IRP_MJ_READ,
                                        TargetDeviceObject,
                                        Buffer,
                                        ByteCount,
                                        (PLARGE_INTEGER) UseStartingOffset,
                                        &Event,
                                        &IrpContext->Irp->IoStatus );

    if (Irp == NULL) {

        UdfRaiseStatus( IrpContext, STATUS_INSUFFICIENT_RESOURCES );
    }

    //
    //  Ignore the change line (verify) for mount and verify requests
    //

    SetFlag( IoGetNextIrpStackLocation( Irp )->Flags, SL_OVERRIDE_VERIFY_VOLUME );

    //
    //  Send the request down to the driver.  If an error occurs return
    //  it to the caller.
    //

    Status = IoCallDriver( TargetDeviceObject, Irp );

    //
    //  If the status was STATUS_PENDING then wait on the event.
    //

    if (Status == STATUS_PENDING) {

        Status = KeWaitForSingleObject( &Event,
                                        Executive,
                                        KernelMode,
                                        FALSE,
                                        NULL );

        //
        //  On a successful wait pull the status out of the IoStatus block.
        //

        if (NT_SUCCESS( Status )) {

            Status = IrpContext->Irp->IoStatus.Status;
        }
    }

    DebugTrace(( -1, Dbg, "UdfReadSectors -> %08x\n", Status ));
    
    //
    //  Check whether we should raise in the error case.
    //

    if (!NT_SUCCESS( Status ) && !ReturnError) {

        UdfNormalizeAndRaiseStatus( IrpContext, Status );
    }

    return Status;
}


//
//  Local support routine
//

BOOLEAN
UdfPrepareBuffers (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN PFCB Fcb,
    IN PVOID UserBuffer,
    IN ULONG UserBufferOffset,
    IN LONGLONG StartingOffset,
    IN ULONG ByteCount,
    IN PIO_RUN IoRuns,
    IN PULONG RunCount,
    IN PULONG ThisByteCount,
    IN PBOOLEAN SparseRuns
    )

/*++

Routine Description:

    This routine is the worker routine which looks up each run of an IO
    request and stores an entry for it in the IoRuns array.  If the run
    begins on an unaligned disk boundary then we will allocate a buffer
    and Mdl for the unaligned portion and put it in the IoRuns entry.

    This routine will raise CANT_WAIT if an unaligned transfer is encountered
    and this request can't wait.

Arguments:

    Irp - Originating Irp for this request.

    Fcb - This is the Fcb for this data stream.  It may be a file, directory,
        path table or the volume file.

    UserBuffer - Current position in the user's buffer.

    UserBufferOffset - Offset from the start of the original user buffer.

    StartingOffset - Offset in the stream to begin the read.

    ByteCount - Number of bytes to read.  We will fill the IoRuns array up
        to this point.  We will stop early if we exceed the maximum number
        of parallel Ios we support.

    IoRuns - Pointer to the IoRuns array.  The entire array is zeroes when
        this routine is called.

    RunCount - Number of entries in the IoRuns array filled here.

    ThisByteCount - Number of bytes described by the IoRun entries.  Will
        not exceed the ByteCount passed in.
        
    SparseRuns - Will indicate whether sparse runs were a component of the
        range returned.  While not part of the IoRuns, this will affect
        our ability to do simple IO.

Return Value:

    BOOLEAN - TRUE if one of the entries in an unaligned buffer (provided
        this is synchronous).  FALSE otherwise.

--*/

{
    PVCB Vcb;

    BOOLEAN Recorded;
    
    BOOLEAN FoundUnaligned = FALSE;
    PIO_RUN ThisIoRun = IoRuns;

    //
    //  Following indicate where we are in the current transfer.  Current
    //  position in the file and number of bytes yet to transfer from
    //  this position.
    //

    ULONG RemainingByteCount = ByteCount;
    LONGLONG CurrentFileOffset = StartingOffset;

    //
    //  Following indicate the state of the user's buffer.  We have
    //  the destination of the next transfer and its offset in the
    //  buffer.  We also have the next available position in the buffer
    //  available for a scratch buffer.  We will align this up to a sector
    //  boundary.
    //

    PVOID CurrentUserBuffer = UserBuffer;
    ULONG CurrentUserBufferOffset = UserBufferOffset;

    PVOID ScratchUserBuffer = UserBuffer;
    ULONG ScratchUserBufferOffset = UserBufferOffset;

    //
    //  The following is the next contiguous bytes on the disk to
    //  transfer.  Read from the allocation package.
    //

    LONGLONG DiskOffset;
    ULONG CurrentByteCount;

    PAGED_CODE();

    Vcb = Fcb->Vcb;

    //
    //  Initialize the RunCount, ByteCount and SparseRuns.
    //

    *RunCount = 0;
    *ThisByteCount = 0;
    *SparseRuns = FALSE;

    //
    //  Loop while there are more bytes to process or there are
    //  available entries in the IoRun array.
    //

    while (TRUE) {

        *RunCount += 1;

        //
        //  Initialize the current position in the IoRuns array.
        //  Find the user's buffer for this portion of the transfer.
        //

        ThisIoRun->UserBuffer = CurrentUserBuffer;

        //
        //  Find the allocation information for the current offset in the
        //  stream.
        //

        Recorded = UdfLookupAllocation( IrpContext,
                                        Fcb,
                                        CurrentFileOffset,
                                        &DiskOffset,
                                        &CurrentByteCount );

        //
        //  Limit ourselves to the data requested.
        //

        if (CurrentByteCount > RemainingByteCount) {

            CurrentByteCount = RemainingByteCount;
        }

        //
        //  Handle the case of unrecorded data first.
        //

        if (!Recorded) {

            //
            //  Note that we did not consume an entry.
            //

            *RunCount -= 1;

            //
            //  Immediately zero the user buffer and indicate that we found sparse
            //  runs to the caller.
            //

            RtlZeroMemory( CurrentUserBuffer, CurrentByteCount );
            *SparseRuns = TRUE;

            //
            //  Push the scratch buffer pointers forward so that we don't stomp
            //  on the zeroed buffer.
            //

            ScratchUserBuffer = Add2Ptr( CurrentUserBuffer,
                                         CurrentByteCount,
                                         PVOID );

            ScratchUserBufferOffset += CurrentByteCount;

        //
        //  Handle the case where this is an unaligned transfer.  The
        //  following must all be true for this to be an aligned transfer.
        //
        //      Disk offset on a 2048 byte boundary (Start of transfer)
        //
        //      Byte count is a multiple of 2048 (Length of transfer)
        //
        //      Current buffer offset is also on a 2048 byte boundary.
        //
        //  If the ByteCount is at least one sector then do the
        //  unaligned transfer only for the tail.  We can use the
        //  user's buffer for the aligned portion.
        //

        } else if (SectorOffset( Vcb, DiskOffset ) ||
                   SectorOffset( Vcb, CurrentUserBufferOffset ) ||
                   (SectorOffset( Vcb, CurrentByteCount ) &&
                    CurrentByteCount < SectorSize( Vcb ))) {

            //
            //  If we can't wait then raise.
            //

            if (!FlagOn( IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT )) {

                UdfRaiseStatus( IrpContext, STATUS_CANT_WAIT );
            }

            //
            //  Remember the offset and the number of bytes out of
            //  the transfer buffer to copy into the user's buffer.
            //  We will truncate the current read to end on a sector
            //  boundary.
            //

            ThisIoRun->TransferBufferOffset = SectorOffset( Vcb, DiskOffset );

            //
            //  Make sure this transfer ends on a sector boundary.
            //

            ThisIoRun->DiskOffset = LlSectorTruncate( Vcb, DiskOffset );

            //
            //  Check if we can use a free portion of the user's buffer.
            //  If we can copy the bytes to an earlier portion of the
            //  buffer then read into that location and slide the bytes
            //  up.
            //
            //  We can use the user's buffer if:
            //
            //      The temporary location in the buffer is before the
            //      final destination.
            //
            //      There is at least one sector of data to read.
            //

            if ((ScratchUserBufferOffset + ThisIoRun->TransferBufferOffset < CurrentUserBufferOffset) &&
                (ThisIoRun->TransferBufferOffset + CurrentByteCount >= SectorSize( Vcb ))) {

                ThisIoRun->DiskByteCount = SectorTruncate( Vcb, ThisIoRun->TransferBufferOffset + CurrentByteCount );
                CurrentByteCount = ThisIoRun->DiskByteCount - ThisIoRun->TransferBufferOffset;
                ThisIoRun->TransferByteCount = CurrentByteCount;

                //
                //  Point to the user's buffer and Mdl for this transfer.
                //

                ThisIoRun->TransferBuffer = ScratchUserBuffer;
                ThisIoRun->TransferMdl = Irp->MdlAddress;
                ThisIoRun->TransferVirtualAddress = Add2Ptr( Irp->UserBuffer,
                                                             ScratchUserBufferOffset,
                                                             PVOID );

                ScratchUserBuffer = Add2Ptr( ScratchUserBuffer,
                                             ThisIoRun->DiskByteCount,
                                             PVOID );

                ScratchUserBufferOffset += ThisIoRun->DiskByteCount;

            //
            //  Otherwise we need to allocate an auxilary buffer for the next sector.
            //

            } else {

                //
                //  Read up to a page containing the partial data
                //

                ThisIoRun->DiskByteCount = SectorAlign( Vcb, ThisIoRun->TransferBufferOffset + CurrentByteCount );

                if (ThisIoRun->DiskByteCount > PAGE_SIZE) {

                    ThisIoRun->DiskByteCount = PAGE_SIZE;
                }

                if (ThisIoRun->TransferBufferOffset + CurrentByteCount > ThisIoRun->DiskByteCount) {

                    CurrentByteCount = ThisIoRun->DiskByteCount - ThisIoRun->TransferBufferOffset;
                }

                ThisIoRun->TransferByteCount = CurrentByteCount;

                //
                //  Allocate a buffer for the non-aligned transfer.
                //

                ThisIoRun->TransferBuffer = FsRtlAllocatePoolWithTag( UdfNonPagedPool,
                                                                      PAGE_SIZE,
                                                                      TAG_IO_BUFFER );

                //
                //  Allocate and build the Mdl to describe this buffer.
                //

                ThisIoRun->TransferMdl = IoAllocateMdl( ThisIoRun->TransferBuffer,
                                                        PAGE_SIZE,
                                                        FALSE,
                                                        FALSE,
                                                        NULL );

                ThisIoRun->TransferVirtualAddress = ThisIoRun->TransferBuffer;

                if (ThisIoRun->TransferMdl == NULL) {

                    IrpContext->Irp->IoStatus.Information = 0;
                    UdfRaiseStatus( IrpContext, STATUS_INSUFFICIENT_RESOURCES );
                }

                MmBuildMdlForNonPagedPool( ThisIoRun->TransferMdl );
            }

            //
            //  Remember we found an unaligned transfer.
            //

            FoundUnaligned = TRUE;

        //
        //  Otherwise we use the buffer and Mdl from the original request.
        //

        } else {

            //
            //  Truncate the read length to a sector-aligned value.  We know
            //  the length must be at least one sector or we wouldn't be
            //  here now.
            //

            CurrentByteCount = SectorTruncate( Vcb, CurrentByteCount );

            //
            //  Read these sectors from the disk.
            //

            ThisIoRun->DiskOffset = DiskOffset;
            ThisIoRun->DiskByteCount = CurrentByteCount;

            //
            //  Use the user's buffer and Mdl as our transfer buffer
            //  and Mdl.
            //

            ThisIoRun->TransferBuffer = CurrentUserBuffer;
            ThisIoRun->TransferMdl = Irp->MdlAddress;
            ThisIoRun->TransferVirtualAddress = Add2Ptr( Irp->UserBuffer,
                                                         CurrentUserBufferOffset,
                                                         PVOID );

            ScratchUserBuffer = Add2Ptr( CurrentUserBuffer,
                                         CurrentByteCount,
                                         PVOID );

            ScratchUserBufferOffset += CurrentByteCount;
        }

        //
        //  Update our position in the transfer and the RunCount and
        //  ByteCount for the user.
        //

        RemainingByteCount -= CurrentByteCount;

        //
        //  Break out if no more positions in the IoRuns array or
        //  we have all of the bytes accounted for.
        //

        *ThisByteCount += CurrentByteCount;

        if ((RemainingByteCount == 0) || (*RunCount == MAX_PARALLEL_IOS)) {

            break;
        }

        //
        //  Update our pointers for the user's buffer.
        //

        ThisIoRun = IoRuns + *RunCount;
        CurrentUserBuffer = Add2Ptr( CurrentUserBuffer, CurrentByteCount, PVOID );
        CurrentUserBufferOffset += CurrentByteCount;
        CurrentFileOffset += CurrentByteCount;
    }

    return FoundUnaligned;
}


//
//  Local support routine
//

BOOLEAN
UdfFinishBuffers (
    IN PIRP_CONTEXT IrpContext,
    IN PIO_RUN IoRuns,
    IN ULONG RunCount,
    IN BOOLEAN FinalCleanup
    )

/*++

Routine Description:

    This routine is called to perform any data transferred required for
    unaligned Io or to perform the final cleanup of the IoRuns array.

    In all cases this is where we will deallocate any buffer and mdl
    allocated to perform the unaligned transfer.  If this is not the
    final cleanup then we also transfer the bytes to the user buffer
    and flush the hardware cache.

    We walk backwards through the run array because we may be shifting data
    in the user's buffer.  Typical case is where we allocated a buffer for
    the first part of a read and then used the user's buffer for the
    next section (but stored it at the beginning of the buffer.

Arguments:

    IoRuns - Pointer to the IoRuns array.

    RunCount - Number of entries in the IoRuns array filled here.

    FinalCleanup - Indicates if we should be deallocating temporary buffers
        (TRUE) or transferring bytes for a unaligned transfers and
        deallocating the buffers (FALSE).  Flush the system cache if
        transferring data.

Return Value:

    BOOLEAN - TRUE if this request needs the Io buffers to be flushed, FALSE otherwise.

--*/

{
    BOOLEAN FlushIoBuffers = FALSE;

    ULONG RemainingEntries = RunCount;
    PIO_RUN ThisIoRun = &IoRuns[RunCount - 1];

    PAGED_CODE();

    //
    //  Walk through each entry in the IoRun array.
    //

    while (RemainingEntries != 0) {

        //
        //  We only need to deal with the case of an unaligned transfer.
        //

        if (ThisIoRun->TransferByteCount != 0) {

            //
            //  If not the final cleanup then transfer the data to the
            //  user's buffer and remember that we will need to flush
            //  the user's buffer to memory.
            //

            if (!FinalCleanup) {

                //
                //  If we are shifting in the user's buffer then use
                //  MoveMemory.
                //

                if (ThisIoRun->TransferMdl == IrpContext->Irp->MdlAddress) {

                    RtlMoveMemory( ThisIoRun->UserBuffer,
                                   Add2Ptr( ThisIoRun->TransferBuffer,
                                            ThisIoRun->TransferBufferOffset,
                                            PVOID ),
                                   ThisIoRun->TransferByteCount );

                } else {

                    RtlCopyMemory( ThisIoRun->UserBuffer,
                                   Add2Ptr( ThisIoRun->TransferBuffer,
                                            ThisIoRun->TransferBufferOffset,
                                            PVOID ),
                                   ThisIoRun->TransferByteCount );
                }

                FlushIoBuffers = TRUE;
            }

            //
            //  Free any Mdl we may have allocated.  If the Mdl isn't
            //  present then we must have failed during the allocation
            //  phase.
            //

            if (ThisIoRun->TransferMdl != IrpContext->Irp->MdlAddress) {

                if (ThisIoRun->TransferMdl != NULL) {

                    IoFreeMdl( ThisIoRun->TransferMdl );
                }

                //
                //  Now free any buffer we may have allocated.  If the Mdl
                //  doesn't match the original Mdl then free the buffer.
                //

                if (ThisIoRun->TransferBuffer != NULL) {

                    UdfFreePool( &ThisIoRun->TransferBuffer );
                }
            }
        }

        //
        //  Now handle the case where we failed in the process
        //  of allocating associated Irps and Mdls.
        //

        if (ThisIoRun->SavedIrp != NULL) {

            if (ThisIoRun->SavedIrp->MdlAddress != NULL) {

                IoFreeMdl( ThisIoRun->SavedIrp->MdlAddress );
            }

            IoFreeIrp( ThisIoRun->SavedIrp );
        }

        //
        //  Move to the previous IoRun entry.
        //

        ThisIoRun -= 1;
        RemainingEntries -= 1;
    }

    //
    //  If we copied any data then flush the Io buffers.
    //

    return FlushIoBuffers;
}


//
//  Local support routine
//

VOID
UdfMultipleAsync (
    IN PIRP_CONTEXT IrpContext,
    IN ULONG RunCount,
    IN PIO_RUN IoRuns
    )

/*++

Routine Description:

    This routine first does the initial setup required of a Master IRP that is
    going to be completed using associated IRPs.  This routine should not
    be used if only one async request is needed, instead the single read
    async routines should be called.

    A context parameter is initialized, to serve as a communications area
    between here and the common completion routine.

    Next this routine reads or writes one or more contiguous sectors from
    a device asynchronously, and is used if there are multiple reads for a
    master IRP.  A completion routine is used to synchronize with the
    completion of all of the I/O requests started by calls to this routine.

    Also, prior to calling this routine the caller must initialize the
    IoStatus field in the Context, with the correct success status and byte
    count which are expected if all of the parallel transfers complete
    successfully.  After return this status will be unchanged if all requests
    were, in fact, successful.  However, if one or more errors occur, the
    IoStatus will be modified to reflect the error status and byte count
    from the first run (by Vbo) which encountered an error.  I/O status
    from all subsequent runs will not be indicated.

Arguments:

    RunCount - Supplies the number of multiple async requests
        that will be issued against the master irp.

    IoRuns - Supplies an array containing the Offset and ByteCount for the
        separate requests.

Return Value:

    None.

--*/

{
    PIO_COMPLETION_ROUTINE CompletionRoutine;
    PIO_STACK_LOCATION IrpSp;
    PMDL Mdl;
    PIRP Irp;
    PIRP MasterIrp;
    ULONG UnwindRunCount;

    PAGED_CODE();

    //
    //  Set up things according to whether this is truely async.
    //

    CompletionRoutine = UdfMultiSyncCompletionRoutine;

    if (!FlagOn( IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT )) {

        CompletionRoutine = UdfMultiAsyncCompletionRoutine;
    }

    //
    //  Initialize some local variables.
    //

    MasterIrp = IrpContext->Irp;

    //
    //  Itterate through the runs, doing everything that can fail.
    //  We let the cleanup in CdFinishBuffers clean up on error.
    //

    for (UnwindRunCount = 0;
         UnwindRunCount < RunCount;
         UnwindRunCount += 1) {

        //
        //  Create an associated IRP, making sure there is one stack entry for
        //  us, as well.
        //

        IoRuns[UnwindRunCount].SavedIrp =
        Irp = IoMakeAssociatedIrp( MasterIrp, (CCHAR)(IrpContext->Vcb->TargetDeviceObject->StackSize + 1) );

        if (Irp == NULL) {

            IrpContext->Irp->IoStatus.Information = 0;
            UdfRaiseStatus( IrpContext, STATUS_INSUFFICIENT_RESOURCES );
        }

        //
        // Allocate and build a partial Mdl for the request.
        //

        Mdl = IoAllocateMdl( IoRuns[UnwindRunCount].TransferVirtualAddress,
                             IoRuns[UnwindRunCount].DiskByteCount,
                             FALSE,
                             FALSE,
                             Irp );

        if (Mdl == NULL) {

            IrpContext->Irp->IoStatus.Information = 0;
            UdfRaiseStatus( IrpContext, STATUS_INSUFFICIENT_RESOURCES );
        }

        IoBuildPartialMdl( IoRuns[UnwindRunCount].TransferMdl,
                           Mdl,
                           IoRuns[UnwindRunCount].TransferVirtualAddress,
                           IoRuns[UnwindRunCount].DiskByteCount );

        //
        //  Get the first IRP stack location in the associated Irp
        //

        IoSetNextIrpStackLocation( Irp );
        IrpSp = IoGetCurrentIrpStackLocation( Irp );

        //
        //  Setup the Stack location to describe our read.
        //

        IrpSp->MajorFunction = IRP_MJ_READ;
        IrpSp->Parameters.Read.Length = IoRuns[UnwindRunCount].DiskByteCount;
        IrpSp->Parameters.Read.ByteOffset.QuadPart = IoRuns[UnwindRunCount].DiskOffset;

        //
        // Set up the completion routine address in our stack frame.
        //

        IoSetCompletionRoutine( Irp,
                                CompletionRoutine,
                                IrpContext->IoContext,
                                TRUE,
                                TRUE,
                                TRUE );

        //
        //  Setup the next IRP stack location in the associated Irp for the disk
        //  driver beneath us.
        //

        IrpSp = IoGetNextIrpStackLocation( Irp );

        //
        //  Setup the Stack location to do a read from the disk driver.
        //

        IrpSp->MajorFunction = IRP_MJ_READ;
        IrpSp->Parameters.Read.Length = IoRuns[UnwindRunCount].DiskByteCount;
        IrpSp->Parameters.Read.ByteOffset.QuadPart = IoRuns[UnwindRunCount].DiskOffset;
    }

    //
    //  We only need to set the associated IRP count in the master irp to
    //  make it a master IRP.  But we set the count to one more than our
    //  caller requested, because we do not want the I/O system to complete
    //  the I/O.  We also set our own count.
    //

    IrpContext->IoContext->IrpCount = RunCount;
    IrpContext->IoContext->MasterIrp = MasterIrp;

    //
    //  We set the count in the master Irp to 1 since typically we
    //  will clean up the associated irps ourselves.  Setting this to one
    //  means completing the last associated Irp with SUCCESS (in the async
    //  case) will complete the master irp.
    //

    MasterIrp->AssociatedIrp.IrpCount = 1;

    //
    //  Now that all the dangerous work is done, issue the Io requests
    //

    for (UnwindRunCount = 0;
         UnwindRunCount < RunCount;
         UnwindRunCount++) {

        Irp = IoRuns[UnwindRunCount].SavedIrp;
        IoRuns[UnwindRunCount].SavedIrp = NULL;

        //
        //  If IoCallDriver returns an error, it has completed the Irp
        //  and the error will be caught by our completion routines
        //  and dealt with as a normal IO error.
        //

        (VOID) IoCallDriver( IrpContext->Vcb->TargetDeviceObject, Irp );
    }

    return;
}


//
//  Local support routine
//

VOID
UdfSingleAsync (
    IN PIRP_CONTEXT IrpContext,
    IN LONGLONG ByteOffset,
    IN ULONG ByteCount
    )

/*++

Routine Description:

    This routine reads one or more contiguous sectors from a device
    asynchronously, and is used if there is only one read necessary to
    complete the IRP.  It implements the read by simply filling
    in the next stack frame in the Irp, and passing it on.  The transfer
    occurs to the single buffer originally specified in the user request.

Arguments:

    ByteOffset - Supplies the starting Logical Byte Offset to begin reading from

    ByteCount - Supplies the number of bytes to read from the device

Return Value:

    None.

--*/

{
    PIO_STACK_LOCATION IrpSp;
    PIO_COMPLETION_ROUTINE CompletionRoutine;

    PAGED_CODE();

    //
    //  Set up things according to whether this is truely async.
    //

    if (FlagOn( IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT )) {

        CompletionRoutine = UdfSingleSyncCompletionRoutine;

    } else {

        CompletionRoutine = UdfSingleAsyncCompletionRoutine;
    }

    //
    // Set up the completion routine address in our stack frame.
    //

    IoSetCompletionRoutine( IrpContext->Irp,
                            CompletionRoutine,
                            IrpContext->IoContext,
                            TRUE,
                            TRUE,
                            TRUE );

    //
    //  Setup the next IRP stack location in the associated Irp for the disk
    //  driver beneath us.
    //

    IrpSp = IoGetNextIrpStackLocation( IrpContext->Irp );

    //
    //  Setup the Stack location to do a read from the disk driver.
    //

    IrpSp->MajorFunction = IRP_MJ_READ;
    IrpSp->Parameters.Read.Length = ByteCount;
    IrpSp->Parameters.Read.ByteOffset.QuadPart = ByteOffset;

    //
    //  Issue the Io request
    //

    //
    //  If IoCallDriver returns an error, it has completed the Irp
    //  and the error will be caught by our completion routines
    //  and dealt with as a normal IO error.
    //

    (VOID)IoCallDriver( IrpContext->Vcb->TargetDeviceObject, IrpContext->Irp );

    //
    //  And return to our caller
    //

    return;
}


//
//  Local support routine
//

VOID
UdfWaitSync (
    IN PIRP_CONTEXT IrpContext
    )

/*++

Routine Description:

    This routine waits for one or more previously started I/O requests
    from the above routines, by simply waiting on the event.

Arguments:

Return Value:

    None

--*/

{
    PAGED_CODE();

    KeWaitForSingleObject( &IrpContext->IoContext->SyncEvent,
                           Executive,
                           KernelMode,
                           FALSE,
                           NULL );

    KeClearEvent( &IrpContext->IoContext->SyncEvent );

    return;
}


//
//  Local support routine
//

NTSTATUS
UdfMultiSyncCompletionRoutine (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )

/*++

Routine Description:

    This is the completion routine for all synchronous reads
    started via UdfMultipleAsync.

    The completion routine has has the following responsibilities:

        If the individual request was completed with an error, then
        this completion routine must see if this is the first error
        and remember the error status in the Context.

        If the IrpCount goes to 1, then it sets the event in the Context
        parameter to signal the caller that all of the asynch requests
        are done.

Arguments:

    DeviceObject - Pointer to the file system device object.

    Irp - Pointer to the associated Irp which is being completed.  (This
        Irp will no longer be accessible after this routine returns.)

    Context - The context parameter which was specified for all of
        the multiple asynch I/O requests for this MasterIrp.

Return Value:

    The routine returns STATUS_MORE_PROCESSING_REQUIRED so that we can
    immediately complete the Master Irp without being in a race condition
    with the IoCompleteRequest thread trying to decrement the IrpCount in
    the Master Irp.

--*/

{
    PUDF_IO_CONTEXT IoContext = Context;

    //
    //  If we got an error (or verify required), remember it in the Irp
    //

    if (!NT_SUCCESS( Irp->IoStatus.Status )) {

        InterlockedExchange( &IoContext->Status, Irp->IoStatus.Status );
        IoContext->MasterIrp->IoStatus.Information = 0;
    }

    //
    //  We must do this here since IoCompleteRequest won't get a chance
    //  on this associated Irp.
    //

    IoFreeMdl( Irp->MdlAddress );
    IoFreeIrp( Irp );

    if (InterlockedDecrement( &IoContext->IrpCount ) == 0) {

        //
        //  Update the Master Irp with any error status from the associated Irps.
        //

        IoContext->MasterIrp->IoStatus.Status = IoContext->Status;
        KeSetEvent( &IoContext->SyncEvent, 0, FALSE );
    }

    UNREFERENCED_PARAMETER( DeviceObject );

    return STATUS_MORE_PROCESSING_REQUIRED;
}


//
//  Local support routine
//

NTSTATUS
UdfMultiAsyncCompletionRoutine (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )

/*++

Routine Description:

    This is the completion routine for all asynchronous reads
    started via UdfMultipleAsync.

    The completion routine has has the following responsibilities:

        If the individual request was completed with an error, then
        this completion routine must see if this is the first error
        and remember the error status in the Context.

Arguments:

    DeviceObject - Pointer to the file system device object.

    Irp - Pointer to the associated Irp which is being completed.  (This
        Irp will no longer be accessible after this routine returns.)

    Context - The context parameter which was specified for all of
             the multiple asynch I/O requests for this MasterIrp.

Return Value:

    Currently always returns STATUS_SUCCESS.

--*/

{
    PUDF_IO_CONTEXT IoContext = Context;
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation( Irp );

    //
    //  If we got an error (or verify required), remember it in the Irp
    //

    if (!NT_SUCCESS( Irp->IoStatus.Status )) {

        InterlockedExchange( &IoContext->Status, Irp->IoStatus.Status );
    }

    //
    //  Decrement IrpCount and see if it goes to zero.
    //

    if (InterlockedDecrement( &IoContext->IrpCount ) == 0) {

        //
        //  Mark the master Irp pending
        //

        IoMarkIrpPending( IoContext->MasterIrp );

        //
        //  Update the Master Irp with any error status from the associated Irps.
        //

        IoContext->MasterIrp->IoStatus.Status = IoContext->Status;

        //
        //  Update the information field with the correct value.
        //

        IoContext->MasterIrp->IoStatus.Information = 0;

        if (NT_SUCCESS( IoContext->MasterIrp->IoStatus.Status )) {

            IoContext->MasterIrp->IoStatus.Information = IoContext->RequestedByteCount;
        }

        //
        //  Now release the resource
        //

        ExReleaseResourceForThread( IoContext->Resource,
                                    IoContext->ResourceThreadId );

        //
        //  and finally, free the context record.
        //

        UdfFreeIoContext( IoContext );

        //
        //  Return success in this case.
        //

        return STATUS_SUCCESS;

    } else {

        //
        //  We need to cleanup the associated Irp and its Mdl.
        //

        IoFreeMdl( Irp->MdlAddress );
        IoFreeIrp( Irp );

        return STATUS_MORE_PROCESSING_REQUIRED;
    }

    UNREFERENCED_PARAMETER( DeviceObject );
}


//
//  Local support routine
//

NTSTATUS
UdfSingleSyncCompletionRoutine (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )

/*++

Routine Description:

    This is the completion routine for all reads started via UdfSingleAsync.

    The completion routine has has the following responsibilities:

        It sets the event in the Context parameter to signal the caller
        that all of the asynch requests are done.

Arguments:

    DeviceObject - Pointer to the file system device object.

    Irp - Pointer to the Irp for this request.  (This Irp will no longer
        be accessible after this routine returns.)

    Context - The context parameter which was specified in the call to
        UdfSingleAsynch.

Return Value:

    The routine returns STATUS_MORE_PROCESSING_REQUIRED so that we can
    immediately complete the Master Irp without being in a race condition
    with the IoCompleteRequest thread trying to decrement the IrpCount in
    the Master Irp.

--*/

{
    //
    //  Store the correct information field into the Irp.
    //

    if (!NT_SUCCESS( Irp->IoStatus.Status )) {

        Irp->IoStatus.Information = 0;
    }

    KeSetEvent( &((PUDF_IO_CONTEXT)Context)->SyncEvent, 0, FALSE );

    return STATUS_MORE_PROCESSING_REQUIRED;

    UNREFERENCED_PARAMETER( DeviceObject );
}


//
//  Local support routine
//

NTSTATUS
UdfSingleAsyncCompletionRoutine (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )

/*++

Routine Description:

    This is the completion routine for all asynchronous reads
    started via UdfSingleAsynch.

Arguments:

    DeviceObject - Pointer to the file system device object.

    Irp - Pointer to the Irp for this request.  (This Irp will no longer
        be accessible after this routine returns.)

    Context - The context parameter which was specified in the call to
        UdfSingleAsynch.

Return Value:

    Currently always returns STATUS_SUCCESS.

--*/

{
    //
    //  Update the information field with the correct value for bytes read.
    //

    Irp->IoStatus.Information = 0;

    if (NT_SUCCESS( Irp->IoStatus.Status )) {

        Irp->IoStatus.Information = ((PUDF_IO_CONTEXT) Context)->RequestedByteCount;
    }

    //
    //  Mark the Irp pending
    //

    IoMarkIrpPending( Irp );

    //
    //  Now release the resource
    //

    ExReleaseResourceForThread( ((PUDF_IO_CONTEXT) Context)->Resource,
                                ((PUDF_IO_CONTEXT) Context)->ResourceThreadId );

    //
    //  and finally, free the context record.
    //

    UdfFreeIoContext( (PUDF_IO_CONTEXT) Context );
    return STATUS_SUCCESS;

    UNREFERENCED_PARAMETER( DeviceObject );
}

