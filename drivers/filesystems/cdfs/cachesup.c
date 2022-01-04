/*++

Copyright (c) 1990-2000 Microsoft Corporation

Module Name:

    Cache.c

Abstract:

    This module implements the cache management routines for the Cdfs
    FSD and FSP, by calling the Common Cache Manager.


--*/

#include "cdprocs.h"

//
//  The Bug check file id for this module
//

#define BugCheckFileId                   (CDFS_BUG_CHECK_CACHESUP)

//
//  Local debug trace level
//

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, CdCompleteMdl)
#pragma alloc_text(PAGE, CdCreateInternalStream)
#pragma alloc_text(PAGE, CdDeleteInternalStream)
#pragma alloc_text(PAGE, CdPurgeVolume)
#endif


VOID
CdCreateInternalStream (
    _In_ PIRP_CONTEXT IrpContext,
    _In_ PVCB Vcb,
    _Inout_ PFCB Fcb,
    _In_ PUNICODE_STRING Name
    )

/*++

Routine Description:

    This function creates an internal stream file for interaction
    with the cache manager.  The Fcb here can be for either a
    directory stream or for a path table stream.

Arguments:

    Vcb - Vcb for this volume.

    Fcb - Points to the Fcb for this file.  It is either an Index or
        Path Table Fcb.

Return Value:

    None.

--*/

{
    PFILE_OBJECT StreamFile = NULL;
    BOOLEAN DecrementReference = FALSE;

    BOOLEAN CleanupDirContext = FALSE;
    BOOLEAN UpdateFcbSizes = FALSE;

    DIRENT Dirent = {0};
    DIRENT_ENUM_CONTEXT DirContext = {0};

    PAGED_CODE();

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_FCB( Fcb );

    //
    //  We may only have the Fcb shared.  Lock the Fcb and do a
    //  safe test to see if we need to really create the file object.
    //

    CdLockFcb( IrpContext, Fcb );

    if (Fcb->FileObject != NULL) {

        CdUnlockFcb( IrpContext, Fcb );
        return;
    }

    //
    //  Use a try-finally to facilitate cleanup.
    //

    _SEH2_TRY {

        //
        //  Create the internal stream.  The Vpb should be pointing at our volume
        //  device object at this point.
        //

        StreamFile = IoCreateStreamFileObjectLite( NULL, Vcb->Vpb->RealDevice );

        if (StreamFile == NULL) {

            CdRaiseStatus( IrpContext, STATUS_INSUFFICIENT_RESOURCES );
        }

        //
        //  Initialize the fields of the file object.
        //

        StreamFile->ReadAccess = TRUE;
        StreamFile->WriteAccess = FALSE;
        StreamFile->DeleteAccess = FALSE;

        StreamFile->SectionObjectPointer = &Fcb->FcbNonpaged->SegmentObject;

        //
        //  Set the file object type and increment the Vcb counts.
        //

        CdSetFileObject( IrpContext,
                         StreamFile,
                         StreamFileOpen,
                         Fcb,
                         NULL );

        //
        //  We'll give stream file objects a name to aid IO profiling etc. We
        //  NULL this in CdDeleteInternalStream before OB deletes the file object,
        //  and before CdRemovePrefix is called (which frees Fcb names).
        //

        StreamFile->FileName = *Name;

        //
        //  We will reference the current Fcb twice to keep it from going
        //  away in the error path.  Otherwise if we dereference it
        //  below in the finally clause a close could cause the Fcb to
        //  be deallocated.
        //

        CdLockVcb( IrpContext, Vcb );
        CdIncrementReferenceCounts( IrpContext, Fcb, 2, 0 );
        CdUnlockVcb( IrpContext, Vcb );
        DecrementReference = TRUE;

        //
        //  Initialize the cache map for the file.
        //

        CcInitializeCacheMap( StreamFile,
                              (PCC_FILE_SIZES)&Fcb->AllocationSize,
                              TRUE,
                              &CdData.CacheManagerCallbacks,
                              Fcb );

        //
        //  Go ahead and store the stream file into the Fcb.
        //

        Fcb->FileObject = StreamFile;
        StreamFile = NULL;

        //
        //  If this is the first file object for a directory then we need to
        //  read the self entry for this directory and update the sizes
        //  in the Fcb.  We know that the Fcb has been initialized so
        //  that we have a least one sector available to read.
        //

        if (!FlagOn( Fcb->FcbState, FCB_STATE_INITIALIZED )) {

            ULONG NewDataLength;

            //
            //  Initialize the search structures.
            //

            CdInitializeDirContext( IrpContext, &DirContext );
            CdInitializeDirent( IrpContext, &Dirent );
            CleanupDirContext = TRUE;

            //
            //  Read the dirent from disk and transfer the data to the
            //  in-memory dirent.
            //

            CdLookupDirent( IrpContext,
                            Fcb,
                            Fcb->StreamOffset,
                            &DirContext );

            CdUpdateDirentFromRawDirent( IrpContext, Fcb, &DirContext, &Dirent );

            //
            //  Verify that this really for the self entry.  We do this by
            //  updating the name in the dirent and then checking that it matches
            //  one of the hard coded names.
            //

            CdUpdateDirentName( IrpContext, &Dirent, FALSE );

            if (Dirent.CdFileName.FileName.Buffer != CdUnicodeSelfArray) {

                CdRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR );
            }

            //
            //  If the data sizes are different then update the header
            //  and Mcb for this Fcb.
            //

            NewDataLength = BlockAlign( Vcb, Dirent.DataLength + Fcb->StreamOffset );

            if (NewDataLength == 0) {

                CdRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR );
            }

            if (NewDataLength != Fcb->FileSize.QuadPart) {

                Fcb->AllocationSize.QuadPart =
                Fcb->FileSize.QuadPart =
                Fcb->ValidDataLength.QuadPart = NewDataLength;

                CcSetFileSizes( Fcb->FileObject, (PCC_FILE_SIZES) &Fcb->AllocationSize );

                CdTruncateAllocation( IrpContext, Fcb, 0 );
                CdAddInitialAllocation( IrpContext,
                                        Fcb,
                                        Dirent.StartingOffset,
                                        NewDataLength );

                UpdateFcbSizes = TRUE;
            }

            //
            //  Check for the existence flag and transform to hidden.
            //

            if (FlagOn( Dirent.DirentFlags, CD_ATTRIBUTE_HIDDEN )) {

                SetFlag( Fcb->FileAttributes, FILE_ATTRIBUTE_HIDDEN );
            }

            //
            //  Convert the time to NT time.
            //

            CdConvertCdTimeToNtTime( IrpContext,
                                     Dirent.CdTime,
                                     (PLARGE_INTEGER) &Fcb->CreationTime );

            //
            //  Update the Fcb flags to indicate we have read the
            //  self entry.
            //

            SetFlag( Fcb->FcbState, FCB_STATE_INITIALIZED );

            //
            //  If we updated the sizes then we want to purge the file.  Go
            //  ahead and unpin and then purge the first page.
            //

            CdCleanupDirContext( IrpContext, &DirContext );
            CdCleanupDirent( IrpContext, &Dirent );
            CleanupDirContext = FALSE;

            if (UpdateFcbSizes) {

                CcPurgeCacheSection( &Fcb->FcbNonpaged->SegmentObject,
                                     NULL,
                                     0,
                                     FALSE );
            }
        }

    } _SEH2_FINALLY {

        //
        //  Cleanup any dirent structures we may have used.
        //

        if (CleanupDirContext) {

            CdCleanupDirContext( IrpContext, &DirContext );
            CdCleanupDirent( IrpContext, &Dirent );
        }

        //
        //  If we raised then we need to dereference the file object.
        //

        if (StreamFile != NULL) {

            //
            //  Null the name pointer, since the stream file object never actually
            //  'owns' the names, we just point it to existing ones.
            //

            StreamFile->FileName.Buffer = NULL;
            StreamFile->FileName.MaximumLength = StreamFile->FileName.Length = 0;

            ObDereferenceObject( StreamFile );
            Fcb->FileObject = NULL;
        }

        //
        //  Dereference and unlock the Fcb.
        //

        if (DecrementReference) {

            CdLockVcb( IrpContext, Vcb );
            CdDecrementReferenceCounts( IrpContext, Fcb, 1, 0 );
            CdUnlockVcb( IrpContext, Vcb );
        }

        CdUnlockFcb( IrpContext, Fcb );
    } _SEH2_END;

    return;
}


VOID
CdDeleteInternalStream (
    _In_ PIRP_CONTEXT IrpContext,
    _Inout_ PFCB Fcb
    )

/*++

Routine Description:

    This function creates an internal stream file for interaction
    with the cache manager.  The Fcb here can be for either a
    directory stream or for a path table stream.

Arguments:

    Fcb - Points to the Fcb for this file.  It is either an Index or
        Path Table Fcb.

Return Value:

    None.

--*/

{
    PFILE_OBJECT FileObject;

    PAGED_CODE();

    UNREFERENCED_PARAMETER( IrpContext );

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_FCB( Fcb );

    //
    //  Lock the Fcb.
    //

    CdLockFcb( IrpContext, Fcb );

    //
    //  Capture the file object.
    //

    FileObject = Fcb->FileObject;
    Fcb->FileObject = NULL;

    //
    //  It is now safe to unlock the Fcb.
    //

    CdUnlockFcb( IrpContext, Fcb );

    //
    //  Dereference the file object if present.
    //

    if (FileObject != NULL) {

        if (FileObject->PrivateCacheMap != NULL) {

            CcUninitializeCacheMap( FileObject, NULL, NULL );
        }

        //
        //  Null the name pointer, since the stream file object never actually
        //  'owns' the names, we just point it to existing ones.
        //

        FileObject->FileName.Buffer = NULL;
        FileObject->FileName.MaximumLength = FileObject->FileName.Length = 0;

        ObDereferenceObject( FileObject );
    }
}


NTSTATUS
CdCompleteMdl (
    _In_ PIRP_CONTEXT IrpContext,
    _Inout_ PIRP Irp
    )

/*++

Routine Description:

    This routine performs the function of completing Mdl reads.
    It should be called only from CdFsdRead.

Arguments:

    Irp - Supplies the originating Irp.

Return Value:

    NTSTATUS - Will always be STATUS_SUCCESS.

--*/

{
    PFILE_OBJECT FileObject;

    PAGED_CODE();

    //
    // Do completion processing.
    //

    FileObject = IoGetCurrentIrpStackLocation( Irp )->FileObject;

    CcMdlReadComplete( FileObject, Irp->MdlAddress );

    //
    // Mdl is now deallocated.
    //

    Irp->MdlAddress = NULL;

    //
    // Complete the request and exit right away.
    //

    CdCompleteRequest( IrpContext, Irp, STATUS_SUCCESS );

    return STATUS_SUCCESS;
}



_Requires_lock_held_(_Global_critical_region_)
NTSTATUS
CdPurgeVolume (
    _In_ PIRP_CONTEXT IrpContext,
    _In_ PVCB Vcb,
    _In_ BOOLEAN DismountUnderway
    )

/*++

Routine Description:

    This routine is called to purge the volume.  The purpose is to make all the stale file
    objects in the system go away in order to lock the volume.

    The Vcb is already acquired exclusively.  We will lock out all file operations by
    acquiring the global file resource.  Then we will walk through all of the Fcb's and
    perform the purge.

Arguments:

    Vcb - Vcb for the volume to purge.

    DismountUnderway - Indicates that we are trying to delete all of the objects.
        We will purge the Path Table and VolumeDasd and dereference all
        internal streams.

Return Value:

    NTSTATUS - The first failure of the purge operation.

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;

    PVOID RestartKey = NULL;
    PFCB ThisFcb = NULL;
    PFCB NextFcb;

    BOOLEAN RemovedFcb;

    PAGED_CODE();

    ASSERT_EXCLUSIVE_VCB( Vcb);

    //
    //  Force any remaining Fcb's in the delayed close queue to be closed.
    //

    CdFspClose( Vcb );

    //
    //  Acquire the global file resource.
    //

    CdAcquireAllFiles( IrpContext, Vcb );

    //
    //  Loop through each Fcb in the Fcb Table and perform the flush.
    //

    while (TRUE) {

        //
        //  Lock the Vcb to lookup the next Fcb.
        //

        CdLockVcb( IrpContext, Vcb );
        NextFcb = CdGetNextFcb( IrpContext, Vcb, &RestartKey );

        //
        //  Reference the NextFcb if present.
        //

        if (NextFcb != NULL) {

            NextFcb->FcbReference += 1;
        }

        //
        //  If the last Fcb is present then decrement reference count and call teardown
        //  to see if it should be removed.
        //

        if (ThisFcb != NULL) {

            ThisFcb->FcbReference -= 1;

            CdUnlockVcb( IrpContext, Vcb );

            CdTeardownStructures( IrpContext, ThisFcb, &RemovedFcb );

        } else {

            CdUnlockVcb( IrpContext, Vcb );
        }

        //
        //  Break out of the loop if no more Fcb's.
        //

        if (NextFcb == NULL) {

            break;
        }

        //
        //  Move to the next Fcb.
        //

        ThisFcb = NextFcb;

        //
        //  If there is a image section then see if that can be closed.
        //

        if (ThisFcb->FcbNonpaged->SegmentObject.ImageSectionObject != NULL) {

            MmFlushImageSection( &ThisFcb->FcbNonpaged->SegmentObject, MmFlushForWrite );
        }

        //
        //  If there is a data section then purge this.  If there is an image
        //  section then we won't be able to.  Remember this if it is our first
        //  error.
        //

        if ((ThisFcb->FcbNonpaged->SegmentObject.DataSectionObject != NULL) &&
            !CcPurgeCacheSection( &ThisFcb->FcbNonpaged->SegmentObject,
                                   NULL,
                                   0,
                                   FALSE ) &&
            (Status == STATUS_SUCCESS)) {

            Status = STATUS_UNABLE_TO_DELETE_SECTION;
        }

        //
        //  Dereference the internal stream if dismounting.
        //

        if (DismountUnderway &&
            (SafeNodeType( ThisFcb ) != CDFS_NTC_FCB_DATA) &&
            (ThisFcb->FileObject != NULL)) {

            CdDeleteInternalStream( IrpContext, ThisFcb );
        }
    }

    //
    //  Now look at the path table and volume Dasd Fcb's.
    //

    if (DismountUnderway) {

        if (Vcb->PathTableFcb != NULL) {

            ThisFcb = Vcb->PathTableFcb;
            InterlockedIncrement( (LONG*)&Vcb->PathTableFcb->FcbReference );

            if ((ThisFcb->FcbNonpaged->SegmentObject.DataSectionObject != NULL) &&
                !CcPurgeCacheSection( &ThisFcb->FcbNonpaged->SegmentObject,
                                       NULL,
                                       0,
                                       FALSE ) &&
                (Status == STATUS_SUCCESS)) {

                Status = STATUS_UNABLE_TO_DELETE_SECTION;
            }

            CdDeleteInternalStream( IrpContext, ThisFcb );

            InterlockedDecrement( (LONG*)&ThisFcb->FcbReference );

            CdTeardownStructures( IrpContext, ThisFcb, &RemovedFcb );
        }

        if (Vcb->VolumeDasdFcb != NULL) {

            ThisFcb = Vcb->VolumeDasdFcb;
            InterlockedIncrement( (LONG*)&ThisFcb->FcbReference );

            if ((ThisFcb->FcbNonpaged->SegmentObject.DataSectionObject != NULL) &&
                !CcPurgeCacheSection( &ThisFcb->FcbNonpaged->SegmentObject,
                                       NULL,
                                       0,
                                       FALSE ) &&
                (Status == STATUS_SUCCESS)) {

                Status = STATUS_UNABLE_TO_DELETE_SECTION;
            }

            InterlockedDecrement( (LONG*)&ThisFcb->FcbReference );

            CdTeardownStructures( IrpContext, ThisFcb, &RemovedFcb );
        }
    }

    //
    //  Release all of the files.
    //

    CdReleaseAllFiles( IrpContext, Vcb );

    return Status;
}


