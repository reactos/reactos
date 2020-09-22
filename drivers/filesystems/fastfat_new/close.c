/*++

Copyright (c) 1989-2000 Microsoft Corporation

Module Name:

    Close.c

Abstract:

    This module implements the File Close routine for Fat called by the
    dispatch driver.


--*/

#include "fatprocs.h"

//
//  The Bug check file id for this module
//

#define BugCheckFileId                   (FAT_BUG_CHECK_CLOSE)

//
//  The local debug trace level
//

#define Dbg                              (DEBUG_TRACE_CLOSE)

ULONG FatMaxDelayedCloseCount;


#define FatAcquireCloseMutex() {                        \
    NT_ASSERT(KeAreApcsDisabled());                     \
    ExAcquireFastMutexUnsafe( &FatCloseQueueMutex );    \
}

#define FatReleaseCloseMutex() {                        \
    NT_ASSERT(KeAreApcsDisabled());                     \
    ExReleaseFastMutexUnsafe( &FatCloseQueueMutex );    \
}

//
//  Local procedure prototypes
//

_Requires_lock_held_(_Global_critical_region_)
VOID
FatQueueClose (
    IN PCLOSE_CONTEXT CloseContext,
    IN BOOLEAN DelayClose
    );

_Requires_lock_held_(_Global_critical_region_)
PCLOSE_CONTEXT
FatRemoveClose (
    PVCB Vcb OPTIONAL,
    PVCB LastVcbHint OPTIONAL
    );

IO_WORKITEM_ROUTINE FatCloseWorker;

VOID
NTAPI
FatCloseWorker (
    _In_        PDEVICE_OBJECT DeviceObject,
    _In_opt_    PVOID Context
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, FatFsdClose)
#pragma alloc_text(PAGE, FatFspClose)
#pragma alloc_text(PAGE, FatRemoveClose)
#pragma alloc_text(PAGE, FatCommonClose)
#pragma alloc_text(PAGE, FatCloseWorker)
#endif


_Function_class_(IRP_MJ_CLOSE)
_Function_class_(DRIVER_DISPATCH)
NTSTATUS
NTAPI
FatFsdClose (
    _In_ PVOLUME_DEVICE_OBJECT VolumeDeviceObject,
    _Inout_ PIRP Irp
    )

/*++

Routine Description:

    This routine implements the FSD part of Close.

Arguments:

    VolumeDeviceObject - Supplies the volume device object where the
        file exists

    Irp - Supplies the Irp being processed

Return Value:

    NTSTATUS - The FSD status for the IRP

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    PIO_STACK_LOCATION IrpSp;
    PFILE_OBJECT FileObject;

    PVCB Vcb;
    PFCB Fcb;
    PCCB Ccb;
    TYPE_OF_OPEN TypeOfOpen;

    BOOLEAN TopLevel;
    BOOLEAN VcbDeleted = FALSE;

    PAGED_CODE();

    //
    //  If we were called with our file system device object instead of a
    //  volume device object, just complete this request with STATUS_SUCCESS
    //

    if (FatDeviceIsFatFsdo( VolumeDeviceObject))  {

        Irp->IoStatus.Status = STATUS_SUCCESS;
        Irp->IoStatus.Information = FILE_OPENED;

        IoCompleteRequest( Irp, IO_DISK_INCREMENT );

        return STATUS_SUCCESS;
    }

    DebugTrace(+1, Dbg, "FatFsdClose\n", 0);

    //
    //  Call the common Close routine
    //

    FsRtlEnterFileSystem();

    TopLevel = FatIsIrpTopLevel( Irp );

    //
    //  Get a pointer to the current stack location and the file object
    //

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    FileObject = IrpSp->FileObject;

    //
    //  Decode the file object and set the read-only bit in the Ccb.
    //

    TypeOfOpen = FatDecodeFileObject( FileObject, &Vcb, &Fcb, &Ccb );

    if (Ccb && IsFileObjectReadOnly(FileObject)) {

        SetFlag( Ccb->Flags, CCB_FLAG_READ_ONLY );
    }

    _SEH2_TRY {

        PCLOSE_CONTEXT CloseContext = NULL;

        //
        //  If we are top level, WAIT can be TRUE, otherwise make it FALSE
        //  to avoid deadlocks, unless this is a top
        //  level request not originating from the system process.
        //

        BOOLEAN Wait = TopLevel && (PsGetCurrentProcess() != FatData.OurProcess);

#if (NTDDI_VERSION >= NTDDI_WIN8)

        //
        //  To catch the odd case where a close comes in without a preceding cleanup,
        //  call the oplock package to get rid of any oplock state.  This can only
        //  be safely done in the FSD path.
        //
        
        if ((Fcb != NULL) &&
            !FlagOn( FileObject->Flags, FO_CLEANUP_COMPLETE ) &&
            FatIsFileOplockable( Fcb )) {
        
            //
            //  This is equivalent to handling cleanup, and it always cleans up any
            //  oplock immediately.  Also, we don't need any locking of the FCB here;
            //  the oplock's own lock will be sufficient for this purpose.
            //
        
            FsRtlCheckOplockEx( FatGetFcbOplock(Fcb),
                                Irp,
                                0,
                                NULL,
                                NULL,
                                NULL );
        }
#endif

        //
        //  Metadata streams have had close contexts preallocated. Pull one out now, while we're 
        //  guaranteed the VCB exists.
        //

        if ( (TypeOfOpen == VirtualVolumeFile) || (TypeOfOpen == DirectoryFile) || (TypeOfOpen == EaFile)
            ) {

            CloseContext = FatAllocateCloseContext( Vcb );
            NT_ASSERT( CloseContext != NULL );
            CloseContext->Free = TRUE;

        }

        //
        //  Call the common Close routine if we are not delaying this close.
        //

        if ((((TypeOfOpen == UserFileOpen) || 
                (TypeOfOpen == UserDirectoryOpen)) &&
                 FlagOn(Fcb->FcbState, FCB_STATE_DELAY_CLOSE) &&
                 !FatData.ShutdownStarted) ||
                (FatCommonClose( Vcb, Fcb, Ccb, TypeOfOpen, Wait, TopLevel, &VcbDeleted ) == STATUS_PENDING)) {

            //
            //  Ship it off to the delayed close queue if we tried to close, and got STATUS_PENDING, or
            //  if the user open told us to delay the close.
            //

            //
            //  Metadata streams have had close contexts preallocated. If we have a user open,
            //  pull the close context out of the Ccb.
            //

            if( CloseContext == NULL ) {
            
                //
                //  Free up any query template strings before using the close context fields,
                //  which overlap (union)
                //

                FatDeallocateCcbStrings( Ccb );

                CloseContext = &Ccb->CloseContext;
                CloseContext->Free = FALSE;
                
                SetFlag( Ccb->Flags, CCB_FLAG_CLOSE_CONTEXT );
            }

            //
            //  If the status is pending, then let's get the information we
            //  need into the close context we already have bagged, complete
            //  the request, and post it.  It is important we allocate nothing
            //  in the close path.
            //

            CloseContext->Vcb = Vcb;
            CloseContext->Fcb = Fcb;
            CloseContext->TypeOfOpen = TypeOfOpen;

            //
            //  Send it off, either to an ExWorkerThread or to the async
            //  close list.
            //

            FatQueueClose( CloseContext,
                           (BOOLEAN)(Fcb && FlagOn(Fcb->FcbState, FCB_STATE_DELAY_CLOSE)));

        } else {
            
            //
            //  The close proceeded synchronously, so for the metadata objects we
            //  can now drop the close context we preallocated.
            //
            
            if ((TypeOfOpen == VirtualVolumeFile) ||
                (TypeOfOpen == DirectoryFile) ||
                (TypeOfOpen == EaFile)
                ) {

                if (CloseContext != NULL) {

                    ExFreePool( CloseContext );
                    
                }
            }
        }

        FatCompleteRequest( FatNull, Irp, Status );

    } 
    _SEH2_EXCEPT(FatExceptionFilter( NULL, _SEH2_GetExceptionInformation() )) {

        //
        //  We had some trouble trying to perform the requested
        //  operation, so we'll abort the I/O request with the 
        //  error status that we get back from the exception code.
        //

        Status = FatProcessException( NULL, Irp, _SEH2_GetExceptionCode() );
    } _SEH2_END;

    if (TopLevel) { IoSetTopLevelIrp( NULL ); }

    FsRtlExitFileSystem();

    //
    //  And return to our caller
    //

    DebugTrace(-1, Dbg, "FatFsdClose -> %08lx\n", Status);

    UNREFERENCED_PARAMETER( VolumeDeviceObject );

    return Status;
}

VOID
NTAPI
FatCloseWorker (
    _In_        PDEVICE_OBJECT DeviceObject,
    _In_opt_    PVOID Context
    )
/*++

Routine Description:

    This routine is a shim between the IO worker package and FatFspClose.

Arguments:

    DeviceObject - Registration device object, unused
    Context - Context value, unused

Return Value:

    None.

--*/
{
    PAGED_CODE();

    UNREFERENCED_PARAMETER( DeviceObject );
    
    FsRtlEnterFileSystem();
    
    FatFspClose (Context);
    
    FsRtlExitFileSystem();
}


_Requires_lock_held_(_Global_critical_region_)
VOID
FatFspClose (
    IN PVCB Vcb OPTIONAL
    )

/*++

Routine Description:

    This routine implements the FSP part of Close.

Arguments:

    Vcb - If present, tells us to only close file objects opened on the
        specified volume.

Return Value:

    None.

--*/

{
    PCLOSE_CONTEXT CloseContext;
    PVCB CurrentVcb = NULL;
    PVCB LastVcb = NULL;
    BOOLEAN FreeContext = FALSE;
    BOOLEAN TopLevel = FALSE;

    ULONG LoopsWithVcbHeld = 0;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "FatFspClose\n", 0);

    //
    //  Set the top level IRP for the true FSP operation.
    //
    
    if (!ARGUMENT_PRESENT( Vcb )) {
        
        IoSetTopLevelIrp( (PIRP)FSRTL_FSP_TOP_LEVEL_IRP );
        TopLevel = TRUE;
    }

    while ((CloseContext = FatRemoveClose(Vcb, LastVcb)) != NULL) {

        //
        //  If we are in the FSP (i.e. Vcb == NULL), then try to keep ahead of
        //  creates by doing several closes with one acquisition of the Vcb.
        //
        //  Note that we cannot be holding the Vcb on entry to FatCommonClose
        //  if this is last close as we will try to acquire FatData, and
        //  worse the volume (and therefore the Vcb) may go away.
        //

        if (!ARGUMENT_PRESENT(Vcb)) {
             
            if (!FatData.ShutdownStarted) {

                if (CloseContext->Vcb != CurrentVcb) {

                    LoopsWithVcbHeld = 0;

                    //
                    //  Release a previously held Vcb, if any.
                    //

                    if (CurrentVcb != NULL) {

                        ExReleaseResourceLite( &CurrentVcb->Resource);
                    }

                    //
                    //  Get the new Vcb.
                    //

                    CurrentVcb = CloseContext->Vcb;
                    (VOID)ExAcquireResourceExclusiveLite( &CurrentVcb->Resource, TRUE );

                } else {

                    //
                    //  Share the resource occasionally if we seem to be finding a lot
                    //  of closes for a single volume.
                    //

                    if (++LoopsWithVcbHeld >= 20) {

                        if (ExGetSharedWaiterCount( &CurrentVcb->Resource ) +
                            ExGetExclusiveWaiterCount( &CurrentVcb->Resource )) {

                            ExReleaseResourceLite( &CurrentVcb->Resource);
                            (VOID)ExAcquireResourceExclusiveLite( &CurrentVcb->Resource, TRUE );
                        }

                        LoopsWithVcbHeld = 0;
                    }
                }

                //
                //  Now check the Open count.  We may be about to delete this volume!
                //
                //  The test below must be <= 1 because there could still be outstanding
                //  stream references on this VCB that are not counted in the OpenFileCount.
                //  For example if there are no open files OpenFileCount could be zero and we would
                //  not release the resource here.  The call to FatCommonClose() below may cause
                //  the VCB to be torn down and we will try to release memory we don't
                //  own later.
                //

                if (CurrentVcb->OpenFileCount <= 1) {
                    ExReleaseResourceLite( &CurrentVcb->Resource);
                    CurrentVcb = NULL;
                }
            //
            //  If shutdown has started while processing our list, drop the
            //  current Vcb resource.
            //

            } else if (CurrentVcb != NULL) {

                ExReleaseResourceLite( &CurrentVcb->Resource);
                CurrentVcb = NULL;
            }
        }

        LastVcb = CurrentVcb;

        //
        //  Call the common Close routine.  Protected in a try {} except {}
        //

        _SEH2_TRY {

            //
            //  The close context either is in the CCB, automatically freed,
            //  or was from pool for a metadata fileobject, CCB is NULL, and
            //  we'll need to free it.
            //

            FreeContext = CloseContext->Free;

            (VOID)FatCommonClose( CloseContext->Vcb,
                                  CloseContext->Fcb,
                                  (FreeContext ? NULL :
                                                 CONTAINING_RECORD( CloseContext, CCB, CloseContext)),
                                  CloseContext->TypeOfOpen,
                                  TRUE,
                                  TopLevel,
                                  NULL );

        } _SEH2_EXCEPT(FatExceptionFilter( NULL, _SEH2_GetExceptionInformation() )) {

            //
            //  Ignore anything we expect.
            //

            NOTHING;
        } _SEH2_END;

        //
        //  Drop the context if it came from pool.
        //
        
        if (FreeContext) {

            ExFreePool( CloseContext );
        }
    }

    //
    //  Release a previously held Vcb, if any.
    //

    if (CurrentVcb != NULL) {

        ExReleaseResourceLite( &CurrentVcb->Resource);
    }

    //
    //  Clean up the top level IRP hint if we owned it.
    //
    
    if (!ARGUMENT_PRESENT( Vcb )) {
        
        IoSetTopLevelIrp( NULL );
    }
    
    //
    //  And return to our caller
    //

    DebugTrace(-1, Dbg, "FatFspClose -> NULL\n", 0);
}


_Requires_lock_held_(_Global_critical_region_)    
VOID
FatQueueClose (
    IN PCLOSE_CONTEXT CloseContext,
    IN BOOLEAN DelayClose
    )

/*++

Routine Description:

    Enqueue a deferred close to one of the two delayed close queues.

Arguments:

    CloseContext - a close context to enqueue for the delayed close thread.
    
    DelayClose - whether this should go on the delayed close queue (unreferenced
        objects).

Return Value:

    None.

--*/

{
    BOOLEAN StartWorker = FALSE;

    FatAcquireCloseMutex();

    if (DelayClose) {

        InsertTailList( &FatData.DelayedCloseList,
                        &CloseContext->GlobalLinks );
        InsertTailList( &CloseContext->Vcb->DelayedCloseList,
                        &CloseContext->VcbLinks );

        FatData.DelayedCloseCount += 1;

        if ((FatData.DelayedCloseCount > FatMaxDelayedCloseCount) &&
            !FatData.AsyncCloseActive) {

            FatData.AsyncCloseActive = TRUE;
            StartWorker = TRUE;
        }

    } else {

        InsertTailList( &FatData.AsyncCloseList,
                        &CloseContext->GlobalLinks );
        InsertTailList( &CloseContext->Vcb->AsyncCloseList,
                        &CloseContext->VcbLinks );

        FatData.AsyncCloseCount += 1;

        if (!FatData.AsyncCloseActive) {

            FatData.AsyncCloseActive = TRUE;
            StartWorker = TRUE;
        }
    }

    FatReleaseCloseMutex();

    if (StartWorker) {

        IoQueueWorkItem( FatData.FatCloseItem, FatCloseWorker, CriticalWorkQueue, NULL );
    }
}


_Requires_lock_held_(_Global_critical_region_)    
PCLOSE_CONTEXT
FatRemoveClose (
    PVCB Vcb OPTIONAL,
    PVCB LastVcbHint OPTIONAL
    )

/*++

Routine Description:

    Dequeue a deferred close from one of the two delayed close queues.

Arguments:

    Vcb - if specified, only returns close for this volume.
    
    LastVcbHint - if specified and other starvation avoidance is required by
        the system condition, will attempt to return closes for this volume.

Return Value:

    A close to perform.

--*/

{
    PLIST_ENTRY Entry;
    PCLOSE_CONTEXT CloseContext;
    BOOLEAN WorkerThread;

    PAGED_CODE();

    FatAcquireCloseMutex();

    //
    //  Remember if this is the worker thread, so we can pull down the active
    //  flag should we run everything out.
    //
    
    WorkerThread = (Vcb == NULL);

    //
    //  If the queues are above the limits by a significant amount, we have
    //  to try hard to pull them down.  To do this, we will aggressively try
    //  to find closes for the last volume the caller looked at.  This will
    //  make sure we fully utilize the acquisition of the volume, which can
    //  be a hugely expensive resource to get (create/close/cleanup use it
    //  exclusively).
    //
    //  Only do this in the delayed close thread.  We will know this is the
    //  case by seeing a NULL mandatory Vcb.
    //

    if (Vcb == NULL && LastVcbHint != NULL) {

        //
        //  Flip over to aggressive at twice the legal limit, and flip it
        //  off at the legal limit.
        //
        
        if (!FatData.HighAsync && FatData.AsyncCloseCount > FatMaxDelayedCloseCount*2) {

            FatData.HighAsync = TRUE;
        
        } else if (FatData.HighAsync && FatData.AsyncCloseCount < FatMaxDelayedCloseCount) {

            FatData.HighAsync = FALSE;
        }
            
        if (!FatData.HighDelayed && FatData.DelayedCloseCount > FatMaxDelayedCloseCount*2) {

            FatData.HighDelayed = TRUE;
        
        } else if (FatData.HighDelayed && FatData.DelayedCloseCount < FatMaxDelayedCloseCount) {

            FatData.HighDelayed = FALSE;
        }

        if (FatData.HighAsync || FatData.HighDelayed) {

            Vcb = LastVcbHint;
        }
    }
        
    //
    //  Do the case when we don't care about which Vcb the close is on.
    //  This is the case when we are in an ExWorkerThread and aren't
    //  under pressure.
    //

    if (Vcb == NULL) {

        AnyClose:

        //
        //  First check the list of async closes.
        //

        if (!IsListEmpty( &FatData.AsyncCloseList )) {

            Entry = RemoveHeadList( &FatData.AsyncCloseList );
            FatData.AsyncCloseCount -= 1;

            CloseContext = CONTAINING_RECORD( Entry,
                                              CLOSE_CONTEXT,
                                              GlobalLinks );

            RemoveEntryList( &CloseContext->VcbLinks );

        //
        //  Do any delayed closes over half the limit, unless shutdown has
        //  started (then kill them all).
        //

        } else if (!IsListEmpty( &FatData.DelayedCloseList ) &&
                   (FatData.DelayedCloseCount > FatMaxDelayedCloseCount/2 ||
                    FatData.ShutdownStarted)) {

            Entry = RemoveHeadList( &FatData.DelayedCloseList );
            FatData.DelayedCloseCount -= 1;

            CloseContext = CONTAINING_RECORD( Entry,
                                              CLOSE_CONTEXT,
                                              GlobalLinks );

            RemoveEntryList( &CloseContext->VcbLinks );

        //
        //  There are no more closes to perform; show that we are done.
        //

        } else {

            CloseContext = NULL;

            if (WorkerThread) {
                
                FatData.AsyncCloseActive = FALSE;
            }
        }

    //
    //  We're running down a specific volume.
    //
    
    } else {


        //
        //  First check the list of async closes.
        //

        if (!IsListEmpty( &Vcb->AsyncCloseList )) {

            Entry = RemoveHeadList( &Vcb->AsyncCloseList );
            FatData.AsyncCloseCount -= 1;

            CloseContext = CONTAINING_RECORD( Entry,
                                              CLOSE_CONTEXT,
                                              VcbLinks );

            RemoveEntryList( &CloseContext->GlobalLinks );

        //
        //  Do any delayed closes.
        //

        } else if (!IsListEmpty( &Vcb->DelayedCloseList )) {

            Entry = RemoveHeadList( &Vcb->DelayedCloseList );
            FatData.DelayedCloseCount -= 1;

            CloseContext = CONTAINING_RECORD( Entry,
                                              CLOSE_CONTEXT,
                                              VcbLinks );
        
            RemoveEntryList( &CloseContext->GlobalLinks );
        
        //
        //  If we were trying to run down the queues but didn't find anything for this
        //  volume, flip over to accept anything and try again.
        //

        } else if (LastVcbHint) {

            goto AnyClose;
        
        //
        //  There are no more closes to perform; show that we are done.
        //

        } else {

            CloseContext = NULL;
        }
    }

    FatReleaseCloseMutex();

    return CloseContext;
}


_Requires_lock_held_(_Global_critical_region_)    
NTSTATUS
FatCommonClose (
    IN PVCB Vcb,
    IN PFCB Fcb,
    IN PCCB Ccb,
    IN TYPE_OF_OPEN TypeOfOpen,
    IN BOOLEAN Wait,
    IN BOOLEAN TopLevel,
    OUT PBOOLEAN VcbDeleted OPTIONAL
    )

/*++

Routine Description:

    This is the common routine for closing a file/directory called by both
    the fsd and fsp threads.

    Close is invoked whenever the last reference to a file object is deleted.
    Cleanup is invoked when the last handle to a file object is closed, and
    is called before close.

    The function of close is to completely tear down and remove the fcb/dcb/ccb
    structures associated with the file object.

Arguments:

    Fcb - Supplies the file to process.

    Wait - If this is TRUE we are allowed to block for the Vcb, if FALSE
        then we must try to acquire the Vcb anyway.

    TopLevel - If this is TRUE this is a top level request.

    VcbDeleted - Returns whether the VCB was deleted by this call.

Return Value:

    NTSTATUS - The return status for the operation

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    PDCB ParentDcb;
    BOOLEAN RecursiveClose;
    BOOLEAN LocalVcbDeleted;
    IRP_CONTEXT IrpContext;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "FatCommonClose...\n", 0);

    //
    //  Initialize the callers variable, if needed.
    //

    LocalVcbDeleted = FALSE;

    if (ARGUMENT_PRESENT( VcbDeleted )) {

        *VcbDeleted = LocalVcbDeleted;
    }

    //
    //  Special case the unopened file object
    //

    if (TypeOfOpen == UnopenedFileObject) {

        DebugTrace(0, Dbg, "Close unopened file object\n", 0);

        Status = STATUS_SUCCESS;

        DebugTrace(-1, Dbg, "FatCommonClose -> %08lx\n", Status);
        return Status;
    }

    //
    //  Set up our stack IrpContext.
    //

    RtlZeroMemory( &IrpContext, sizeof(IRP_CONTEXT) );

    IrpContext.NodeTypeCode = FAT_NTC_IRP_CONTEXT;
    IrpContext.NodeByteSize = sizeof( IrpContext );
    IrpContext.MajorFunction = IRP_MJ_CLOSE;
    IrpContext.Vcb = Vcb;
    
    if (Wait) {

        SetFlag( IrpContext.Flags, IRP_CONTEXT_FLAG_WAIT );
    }

    //
    //  Acquire exclusive access to the Vcb and enqueue the irp if we didn't
    //  get access.
    //

#ifdef _MSC_VER
#pragma prefast( suppress: 28137, "prefast wants Wait to be a constant, but that's not possible for fastfat" )
#endif
    if (!ExAcquireResourceExclusiveLite( &Vcb->Resource, Wait )) {

        return STATUS_PENDING;
    }

    //
    //  The following test makes sure that we don't blow away an Fcb if we
    //  are trying to do a Supersede/Overwrite open above us.  This test
    //  does not apply for the EA file.
    //

    if (FlagOn(Vcb->VcbState, VCB_STATE_FLAG_CREATE_IN_PROGRESS) &&
        Vcb->EaFcb != Fcb) {

        ExReleaseResourceLite( &Vcb->Resource );

        return STATUS_PENDING;
    }

    //
    //  Setting the following flag prevents recursive closes of directory file
    //  objects, which are handled in a special case loop.
    //

    if ( FlagOn(Vcb->VcbState, VCB_STATE_FLAG_CLOSE_IN_PROGRESS) ) {

        RecursiveClose = TRUE;

    } else {


        SetFlag(Vcb->VcbState, VCB_STATE_FLAG_CLOSE_IN_PROGRESS);
        RecursiveClose = FALSE;

        //
        //  Since we are at the top of the close chain, we need to add
        //  a reference to the VCB.  This will keep it from going away
        //  on us until we are ready to check for a dismount below.
        //

        Vcb->OpenFileCount += 1;
    }

    _SEH2_TRY {

        //
        //  Case on the type of open that we are trying to close.
        //

        switch (TypeOfOpen) {

        case VirtualVolumeFile:

            DebugTrace(0, Dbg, "Close VirtualVolumeFile\n", 0);

            //
            //  Remove this internal, residual open from the count.
            //

            InterlockedDecrement( (LONG*)&(Vcb->InternalOpenCount) );
            InterlockedDecrement( (LONG*)&(Vcb->ResidualOpenCount) );

            try_return( Status = STATUS_SUCCESS );
            break;

        case UserVolumeOpen:

            DebugTrace(0, Dbg, "Close UserVolumeOpen\n", 0);

            Vcb->DirectAccessOpenCount -= 1;
            Vcb->OpenFileCount -= 1;
            if (FlagOn(Ccb->Flags, CCB_FLAG_READ_ONLY)) { Vcb->ReadOnlyCount -= 1; }

            FatDeleteCcb( &IrpContext, &Ccb );

            try_return( Status = STATUS_SUCCESS );
            break;

        case EaFile:

            DebugTrace(0, Dbg, "Close EaFile\n", 0);

            //
            //  Remove this internal, residual open from the count.
            //

            InterlockedDecrement( (LONG*)&(Vcb->InternalOpenCount) );
            InterlockedDecrement( (LONG*)&(Vcb->ResidualOpenCount) );

            try_return( Status = STATUS_SUCCESS );
            break;

        case DirectoryFile:

            DebugTrace(0, Dbg, "Close DirectoryFile\n", 0);

            InterlockedDecrement( (LONG*)&Fcb->Specific.Dcb.DirectoryFileOpenCount );

            //
            //  Remove this internal open from the count.
            //

            InterlockedDecrement( (LONG*)&(Vcb->InternalOpenCount) );

            //
            //  If this is the root directory, it is a residual open
            //  as well.
            //

            if (NodeType( Fcb ) == FAT_NTC_ROOT_DCB) {

                InterlockedDecrement( (LONG*)&(Vcb->ResidualOpenCount) );
            }

            //
            //  If this is a recursive close, just return here.
            //

            if ( RecursiveClose ) {

                try_return( Status = STATUS_SUCCESS );

            } else {

                break;
            }


        case UserDirectoryOpen:
        case UserFileOpen:

            DebugTrace(0, Dbg, "Close UserFileOpen/UserDirectoryOpen\n", 0);

            //
            //  Uninitialize the cache map if we no longer need to use it
            //

            if ((NodeType(Fcb) == FAT_NTC_DCB) &&
                IsListEmpty(&Fcb->Specific.Dcb.ParentDcbQueue) &&
                (Fcb->OpenCount == 1) &&
                (Fcb->Specific.Dcb.DirectoryFile != NULL)) {

                PFILE_OBJECT DirectoryFileObject = Fcb->Specific.Dcb.DirectoryFile;

                DebugTrace(0, Dbg, "Uninitialize the stream file object\n", 0);

                CcUninitializeCacheMap( DirectoryFileObject, NULL, NULL );

                //
                //  Dereference the directory file.  This may cause a close
                //  Irp to be processed, so we need to do this before we destroy
                //  the Fcb.
                //

                Fcb->Specific.Dcb.DirectoryFile = NULL;
                ObDereferenceObject( DirectoryFileObject );
            }

            Fcb->OpenCount -= 1;            
            Vcb->OpenFileCount -= 1;
            if (FlagOn(Ccb->Flags, CCB_FLAG_READ_ONLY)) { Vcb->ReadOnlyCount -= 1; }
           
            FatDeleteCcb( &IrpContext, &Ccb );

            break;

        default:

#ifdef _MSC_VER
#pragma prefast( suppress: 28159, "if the type of open is unknown, we seriously messed up." )
#endif
            FatBugCheck( TypeOfOpen, 0, 0 );
        }

        //
        //  At this point we've cleaned up any on-disk structure that needs
        //  to be done, and we can now update the in-memory structures.
        //  Now if this is an unreferenced FCB or if it is
        //  an unreferenced DCB (not the root) then we can remove
        //  the fcb and set our ParentDcb to non null.
        //

        if (((NodeType(Fcb) == FAT_NTC_FCB) &&
             (Fcb->OpenCount == 0))

                ||

             ((NodeType(Fcb) == FAT_NTC_DCB) &&
              (IsListEmpty(&Fcb->Specific.Dcb.ParentDcbQueue)) &&
              (Fcb->OpenCount == 0) &&
              (Fcb->Specific.Dcb.DirectoryFileOpenCount == 0))) {

            ParentDcb = Fcb->ParentDcb;

            SetFlag( Vcb->VcbState, VCB_STATE_FLAG_DELETED_FCB );

            FatDeleteFcb( &IrpContext, &Fcb );

            //
            //  Uninitialize our parent's cache map if we no longer need
            //  to use it.
            //

            while ((NodeType(ParentDcb) == FAT_NTC_DCB) &&
                   IsListEmpty(&ParentDcb->Specific.Dcb.ParentDcbQueue) &&
                   (ParentDcb->OpenCount == 0) &&
                   (ParentDcb->Specific.Dcb.DirectoryFile != NULL)) {

                PFILE_OBJECT DirectoryFileObject;

                DirectoryFileObject = ParentDcb->Specific.Dcb.DirectoryFile;

                DebugTrace(0, Dbg, "Uninitialize our parent Stream Cache Map\n", 0);

                CcUninitializeCacheMap( DirectoryFileObject, NULL, NULL );

                ParentDcb->Specific.Dcb.DirectoryFile = NULL;

                ObDereferenceObject( DirectoryFileObject );

                //
                //  Now, if the ObDereferenceObject() caused the final close
                //  to come in, then blow away the Fcb and continue up,
                //  otherwise wait for Mm to to dereference its file objects
                //  and stop here..
                //

                if ( ParentDcb->Specific.Dcb.DirectoryFileOpenCount == 0) {

                    PDCB CurrentDcb;

                    CurrentDcb = ParentDcb;
                    ParentDcb = CurrentDcb->ParentDcb;

                    SetFlag( Vcb->VcbState, VCB_STATE_FLAG_DELETED_FCB );

                    FatDeleteFcb( &IrpContext, &CurrentDcb );

                } else {

                    break;
                }
            }
        }

        Status = STATUS_SUCCESS;

    try_exit: NOTHING;
    } _SEH2_FINALLY {

        DebugUnwind( FatCommonClose );

        //
        //  We are done processing the close.  If we are the top of the close
        //  chain, see if the VCB can go away.  We have biased the open count by
        //  one, so we need to take that into account.
        //

        if (!RecursiveClose) {

            //
            //  See if there is only one open left.  If so, it is ours.  We only want
            //  to check for a dismount if a dismount is not already in progress.
            //  We also only do this if the Vcb condition is not VcbGood and the 
            //  caller can handle the VCB going away. This is determined by whether
            //  they passed in the VcbDeleted argument. This request also needs
            //  to be top level.
            //

            if (Vcb->OpenFileCount == 1 &&
                Vcb->VcbCondition != VcbGood &&
                !FlagOn( Vcb->VcbState, VCB_STATE_FLAG_DISMOUNT_IN_PROGRESS ) &&
                ARGUMENT_PRESENT( VcbDeleted ) &&
                TopLevel) {

                //
                //  We need the global lock, which must be acquired before the
                //  VCB.  Since we already have the VCB, we have to drop and
                //  reacquire here.  Note that we always want to wait from this
                //  point on.  Note that the VCB cannot go away, since we have
                //  biased the open file count.
                //

                FatReleaseVcb( &IrpContext,
                               Vcb );

                SetFlag( IrpContext.Flags, IRP_CONTEXT_FLAG_WAIT );

#ifdef _MSC_VER
#pragma prefast( suppress: 28137, "prefast wants the wait parameter in this macro expansion to be a constant, unfortunately this is not possible" )
#endif
                FatAcquireExclusiveGlobal( &IrpContext );

                FatAcquireExclusiveVcb( &IrpContext,
                                        Vcb );

                //
                //  We have our locks in the correct order.  Remove our
                //  extra open and check for a dismount.  Note that if
                //  something changed while we dropped the lock, it will
                //  not matter, since the dismount code does the correct
                //  checks to make sure the volume can really go away.
                //

                Vcb->OpenFileCount -= 1;

                LocalVcbDeleted = FatCheckForDismount( &IrpContext,
                                                       Vcb,
                                                       FALSE );

                FatReleaseGlobal( &IrpContext );

                //
                //  Let the caller know what happened, if they want this information.
                //

                if (ARGUMENT_PRESENT( VcbDeleted )) {

                    *VcbDeleted = LocalVcbDeleted;
                }

            } else {

                //
                //  The volume cannot go away now.  Just remove our extra reference.
                //

                Vcb->OpenFileCount -= 1;
            }

            //
            //  If the VCB is still around, clear our recursion flag.
            //

            if (!LocalVcbDeleted) {

                ClearFlag( Vcb->VcbState, VCB_STATE_FLAG_CLOSE_IN_PROGRESS );
            }
        }

        //
        //  Only release the VCB if it did not go away.
        //

        if (!LocalVcbDeleted) {

            FatReleaseVcb( &IrpContext, Vcb );
        }

        DebugTrace(-1, Dbg, "FatCommonClose -> %08lx\n", Status);
    } _SEH2_END;

    return Status;
}

