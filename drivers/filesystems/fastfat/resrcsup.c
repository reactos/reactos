/*++

Copyright (c) 1989-2000 Microsoft Corporation

Module Name:

    ResrcSup.c

Abstract:

    This module implements the Fat Resource acquisition routines


--*/

#include "fatprocs.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, FatAcquireFcbForLazyWrite)
#pragma alloc_text(PAGE, FatAcquireFcbForReadAhead)
#pragma alloc_text(PAGE, FatAcquireExclusiveFcb)
#pragma alloc_text(PAGE, FatAcquireSharedFcb)
#pragma alloc_text(PAGE, FatAcquireSharedFcbWaitForEx)
#pragma alloc_text(PAGE, FatAcquireExclusiveVcb_Real)
#pragma alloc_text(PAGE, FatAcquireSharedVcb)
#pragma alloc_text(PAGE, FatNoOpAcquire)
#pragma alloc_text(PAGE, FatNoOpRelease)
#pragma alloc_text(PAGE, FatReleaseFcbFromLazyWrite)
#pragma alloc_text(PAGE, FatReleaseFcbFromReadAhead)
#pragma alloc_text(PAGE, FatAcquireForCcFlush)
#pragma alloc_text(PAGE, FatReleaseForCcFlush)
#pragma alloc_text(PAGE, FatFilterCallbackAcquireForCreateSection)
#endif

_Requires_lock_held_(_Global_critical_region_)
_When_(return != FALSE && NoOpCheck != FALSE, _Acquires_exclusive_lock_(Vcb->Resource))
FINISHED
FatAcquireExclusiveVcb_Real (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN BOOLEAN NoOpCheck
    )

/*++

Routine Description:

    This routine acquires exclusive access to the Vcb.

    After we acquire the resource check to see if this operation is legal.
    If it isn't (ie. we get an exception), release the resource.

Arguments:

    Vcb - Supplies the Vcb to acquire

    NoOpCheck - if TRUE then don't do any verification of the request/volume state.

Return Value:

    FINISHED - TRUE if we have the resource and FALSE if we needed to block
        for the resource but Wait is FALSE.

--*/

{
    PAGED_CODE();

#ifdef _MSC_VER
#pragma prefast( suppress: 28137, "prefast wants the wait to be a constant, but that isn't possible for the way fastfat is designed" )
#endif
    if (ExAcquireResourceExclusiveLite( &Vcb->Resource, BooleanFlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT))) {

        if (!NoOpCheck) {

            _SEH2_TRY {

                FatVerifyOperationIsLegal( IrpContext );

            } _SEH2_FINALLY {

                if ( _SEH2_AbnormalTermination() ) {

                    FatReleaseVcb( IrpContext, Vcb );
                }
            } _SEH2_END;
        }

        return TRUE;
    }

    return FALSE;
}

_Requires_lock_held_(_Global_critical_region_)
_When_(return != 0, _Acquires_shared_lock_(Vcb->Resource))
FINISHED
FatAcquireSharedVcb (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb
    )

/*++

Routine Description:

    This routine acquires shared access to the Vcb.

    After we acquire the resource check to see if this operation is legal.
    If it isn't (ie. we get an exception), release the resource.

Arguments:

    Vcb - Supplies the Vcb to acquire

Return Value:

    FINISHED - TRUE if we have the resource and FALSE if we needed to block
        for the resource but Wait is FALSE.

--*/

{
    PAGED_CODE();

    if (ExAcquireResourceSharedLite( &Vcb->Resource,
                                     BooleanFlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT))) {

        _SEH2_TRY {

            FatVerifyOperationIsLegal( IrpContext );

        } _SEH2_FINALLY {

            if ( _SEH2_AbnormalTermination() ) {

                FatReleaseVcb( IrpContext, Vcb );
            }
        } _SEH2_END;

        return TRUE;
    }

    return FALSE;
}

_Requires_lock_held_(_Global_critical_region_)
_Acquires_exclusive_lock_(*Fcb->Header.Resource)
FINISHED
FatAcquireExclusiveFcb (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb
    )

/*++

Routine Description:

    This routine acquires exclusive access to the Fcb.

    After we acquire the resource check to see if this operation is legal.
    If it isn't (ie. we get an exception), release the resource.

Arguments:

    Fcb - Supplies the Fcb to acquire

Return Value:

    FINISHED - TRUE if we have the resource and FALSE if we needed to block
        for the resource but Wait is FALSE.

--*/

{
    PAGED_CODE();

RetryFcbExclusive:

#ifdef _MSC_VER
#pragma prefast( suppress: 28137, "prefast wants the wait to be a constant, but that isn't possible for the way fastfat is designed" )
#endif
    if (ExAcquireResourceExclusiveLite( Fcb->Header.Resource, BooleanFlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT))) {

        //
        //  Check for anything other than a non-cached write if the
        //  async count is non-zero in the Fcb, or if others are waiting
        //  for the resource.  Then wait for all outstanding I/O to finish,
        //  drop the resource, and wait again.
        //

        if ((Fcb->NonPaged->OutstandingAsyncWrites != 0) &&
            ((IrpContext->MajorFunction != IRP_MJ_WRITE) ||
             !FlagOn(IrpContext->OriginatingIrp->Flags, IRP_NOCACHE) ||
             (ExGetSharedWaiterCount(Fcb->Header.Resource) != 0) ||
             (ExGetExclusiveWaiterCount(Fcb->Header.Resource) != 0))) {

            KeWaitForSingleObject( Fcb->NonPaged->OutstandingAsyncEvent,
                                   Executive,
                                   KernelMode,
                                   FALSE,
                                   (PLARGE_INTEGER) NULL );

            FatReleaseFcb( IrpContext, Fcb );

            goto RetryFcbExclusive;
        }

        _SEH2_TRY {

            FatVerifyOperationIsLegal( IrpContext );

        } _SEH2_FINALLY {

            if ( _SEH2_AbnormalTermination() ) {

                FatReleaseFcb( IrpContext, Fcb );
            }
        } _SEH2_END;

        return TRUE;
    }

    return FALSE;
}


 _Requires_lock_held_(_Global_critical_region_)
_Acquires_shared_lock_(*Fcb->Header.Resource)
FINISHED
FatAcquireSharedFcb (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb
    )

/*++

Routine Description:

    This routine acquires shared access to the Fcb.

    After we acquire the resource check to see if this operation is legal.
    If it isn't (ie. we get an exception), release the resource.

Arguments:

    Fcb - Supplies the Fcb to acquire

Return Value:

    FINISHED - TRUE if we have the resource and FALSE if we needed to block
        for the resource but Wait is FALSE.

--*/

{
    PAGED_CODE();

RetryFcbShared:

    if (ExAcquireResourceSharedLite( Fcb->Header.Resource, BooleanFlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT))) {

        //
        //  Check for anything other than a non-cached write if the
        //  async count is non-zero in the Fcb, or if others are waiting
        //  for the resource.  Then wait for all outstanding I/O to finish,
        //  drop the resource, and wait again.
        //

        if ((Fcb->NonPaged->OutstandingAsyncWrites != 0) &&
            ((IrpContext->MajorFunction != IRP_MJ_WRITE) ||
             !FlagOn(IrpContext->OriginatingIrp->Flags, IRP_NOCACHE) ||
             (ExGetSharedWaiterCount(Fcb->Header.Resource) != 0) ||
             (ExGetExclusiveWaiterCount(Fcb->Header.Resource) != 0))) {

            KeWaitForSingleObject( Fcb->NonPaged->OutstandingAsyncEvent,
                                   Executive,
                                   KernelMode,
                                   FALSE,
                                   (PLARGE_INTEGER) NULL );

            FatReleaseFcb( IrpContext, Fcb );

            goto RetryFcbShared;
        }

        _SEH2_TRY {

            FatVerifyOperationIsLegal( IrpContext );

        } _SEH2_FINALLY {

            if ( _SEH2_AbnormalTermination() ) {

                FatReleaseFcb( IrpContext, Fcb );
            }
        } _SEH2_END;


        return TRUE;

    } else {

        return FALSE;
    }
}


_Requires_lock_held_(_Global_critical_region_)
_When_(return != 0, _Acquires_shared_lock_(*Fcb->Header.Resource))
FINISHED
FatAcquireSharedFcbWaitForEx (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb
    )

/*++

Routine Description:

    This routine acquires shared access to the Fcb, waiting first for any
    exclusive accessors to get the Fcb first.

    After we acquire the resource check to see if this operation is legal.
    If it isn't (ie. we get an exception), release the resource.

Arguments:

    Fcb - Supplies the Fcb to acquire

Return Value:

    FINISHED - TRUE if we have the resource and FALSE if we needed to block
        for the resource but Wait is FALSE.

--*/

{
    PAGED_CODE();

    NT_ASSERT( FlagOn(IrpContext->OriginatingIrp->Flags, IRP_NOCACHE) );
    NT_ASSERT( !FlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT) );

RetryFcbSharedWaitEx:

    if (ExAcquireSharedWaitForExclusive( Fcb->Header.Resource, FALSE )) {

        //
        //  Check for anything other than a non-cached write if the
        //  async count is non-zero in the Fcb. Then wait for all
        //  outstanding I/O to finish, drop the resource, and wait again.
        //

        if ((Fcb->NonPaged->OutstandingAsyncWrites != 0) &&
            (IrpContext->MajorFunction != IRP_MJ_WRITE)) {

            KeWaitForSingleObject( Fcb->NonPaged->OutstandingAsyncEvent,
                                   Executive,
                                   KernelMode,
                                   FALSE,
                                   (PLARGE_INTEGER) NULL );

            FatReleaseFcb( IrpContext, Fcb );

            goto RetryFcbSharedWaitEx;
        }

        _SEH2_TRY {

            FatVerifyOperationIsLegal( IrpContext );

        } _SEH2_FINALLY {

            if ( _SEH2_AbnormalTermination() ) {

                FatReleaseFcb( IrpContext, Fcb );
            }
        } _SEH2_END;


        return TRUE;

    } else {

        return FALSE;
    }
}


_Requires_lock_held_(_Global_critical_region_)
BOOLEAN
NTAPI
FatAcquireFcbForLazyWrite (
    IN PVOID Fcb,
    IN BOOLEAN Wait
    )

/*++

Routine Description:

    The address of this routine is specified when creating a CacheMap for
    a file.  It is subsequently called by the Lazy Writer prior to its
    performing lazy writes to the file.

Arguments:

    Fcb - The Fcb which was specified as a context parameter for this
          routine.

    Wait - TRUE if the caller is willing to block.

Return Value:

    FALSE - if Wait was specified as FALSE and blocking would have
            been required.  The Fcb is not acquired.

    TRUE - if the Fcb has been acquired

--*/

{
    PAGED_CODE();

    //
    //  Check here for the EA File.  It turns out we need the normal
    //  resource shared in this case.  Otherwise we take the paging
    //  I/O resource shared.
    //

    //
    //  Note that we do not need to disable APC delivery to guard
    //  against a rogue user issuing a suspend APC. That is because
    //  it is guaranteed that the caller is either in the system context,
    //  to which a user cannot deliver a suspend APC, or the caller has
    //  already disabled kernel APC delivery before calling. This is true
    //  for all the other pre-acquire routines as well.
    //

    if (!ExAcquireResourceSharedLite( Fcb == ((PFCB)Fcb)->Vcb->EaFcb ?
                                  ((PFCB)Fcb)->Header.Resource :
                                  ((PFCB)Fcb)->Header.PagingIoResource,
                                  Wait )) {

        return FALSE;
    }

    //
    // We assume the Lazy Writer only acquires this Fcb once.
    // Therefore, it should be guaranteed that this flag is currently
    // clear (the ASSERT), and then we will set this flag, to insure
    // that the Lazy Writer will never try to advance Valid Data, and
    // also not deadlock by trying to get the Fcb exclusive.
    //


    NT_ASSERT( NodeType(((PFCB)Fcb)) == FAT_NTC_FCB );
    NT_ASSERT( ((PFCB)Fcb)->Specific.Fcb.LazyWriteThread == NULL );

    ((PFCB)Fcb)->Specific.Fcb.LazyWriteThread = PsGetCurrentThread();

    NT_ASSERT( NULL != PsGetCurrentThread() );

    if (NULL == FatData.LazyWriteThread) {

        FatData.LazyWriteThread = PsGetCurrentThread();
    }

    //
    //  This is a kludge because Cc is really the top level.  When it
    //  enters the file system, we will think it is a resursive call
    //  and complete the request with hard errors or verify.  It will
    //  then have to deal with them, somehow....
    //

    NT_ASSERT(IoGetTopLevelIrp() == NULL);

    IoSetTopLevelIrp((PIRP)FSRTL_CACHE_TOP_LEVEL_IRP);

    return TRUE;
}


_Requires_lock_held_(_Global_critical_region_)
VOID
NTAPI
FatReleaseFcbFromLazyWrite (
    IN PVOID Fcb
    )

/*++

Routine Description:

    The address of this routine is specified when creating a CacheMap for
    a file.  It is subsequently called by the Lazy Writer after its
    performing lazy writes to the file.

Arguments:

    Fcb - The Fcb which was specified as a context parameter for this
          routine.

Return Value:

    None

--*/

{
    PAGED_CODE();

    //
    //  Assert that this really is an fcb and that this thread really owns
    //  the lazy writer mark in the fcb.
    //

    NT_ASSERT( NodeType(((PFCB)Fcb)) == FAT_NTC_FCB );
    NT_ASSERT( NULL != PsGetCurrentThread() );
    NT_ASSERT( ((PFCB)Fcb)->Specific.Fcb.LazyWriteThread == PsGetCurrentThread() );

    //
    //  Release the lazy writer mark.
    //

    ((PFCB)Fcb)->Specific.Fcb.LazyWriteThread = NULL;

    //
    //  Check here for the EA File.  It turns out we needed the normal
    //  resource shared in this case.  Otherwise it was the PagingIoResource.
    //

    ExReleaseResourceLite( Fcb == ((PFCB)Fcb)->Vcb->EaFcb ?
                       ((PFCB)Fcb)->Header.Resource :
                       ((PFCB)Fcb)->Header.PagingIoResource );

    //
    //  Clear the kludge at this point.
    //

    NT_ASSERT(IoGetTopLevelIrp() == (PIRP)FSRTL_CACHE_TOP_LEVEL_IRP);

    IoSetTopLevelIrp( NULL );

    return;
}


_Requires_lock_held_(_Global_critical_region_)
BOOLEAN
NTAPI
FatAcquireFcbForReadAhead (
    IN PVOID Fcb,
    IN BOOLEAN Wait
    )

/*++

Routine Description:

    The address of this routine is specified when creating a CacheMap for
    a file.  It is subsequently called by the Lazy Writer prior to its
    performing read ahead to the file.

Arguments:

    Fcb - The Fcb which was specified as a context parameter for this
          routine.

    Wait - TRUE if the caller is willing to block.

Return Value:

    FALSE - if Wait was specified as FALSE and blocking would have
            been required.  The Fcb is not acquired.

    TRUE - if the Fcb has been acquired

--*/

{
    PAGED_CODE();

    //
    //  We acquire the normal file resource shared here to synchronize
    //  correctly with purges.
    //

    //
    //  Note that we do not need to disable APC delivery to guard
    //  against a rogue user issuing a suspend APC. That is because
    //  it is guaranteed that the caller is either in the system context,
    //  to which a user cannot deliver a suspend APC, or the caller has
    //  already disabled kernel APC delivery before calling. This is true
    //  for all the other pre-acquire routines as well.
    //

    if (!ExAcquireResourceSharedLite( ((PFCB)Fcb)->Header.Resource,
                                  Wait )) {

        return FALSE;
    }

    //
    //  This is a kludge because Cc is really the top level.  We it
    //  enters the file system, we will think it is a resursive call
    //  and complete the request with hard errors or verify.  It will
    //  have to deal with them, somehow....
    //

    NT_ASSERT(IoGetTopLevelIrp() == NULL);

    IoSetTopLevelIrp((PIRP)FSRTL_CACHE_TOP_LEVEL_IRP);

    return TRUE;
}


_Requires_lock_held_(_Global_critical_region_)
VOID
NTAPI
FatReleaseFcbFromReadAhead (
    IN PVOID Fcb
    )

/*++

Routine Description:

    The address of this routine is specified when creating a CacheMap for
    a file.  It is subsequently called by the Lazy Writer after its
    read ahead.

Arguments:

    Fcb - The Fcb which was specified as a context parameter for this
          routine.

Return Value:

    None

--*/

{
    PAGED_CODE();

    //
    //  Clear the kludge at this point.
    //

    NT_ASSERT(IoGetTopLevelIrp() == (PIRP)FSRTL_CACHE_TOP_LEVEL_IRP);

    IoSetTopLevelIrp( NULL );

    ExReleaseResourceLite( ((PFCB)Fcb)->Header.Resource );

    return;
}


_Function_class_(FAST_IO_ACQUIRE_FOR_CCFLUSH)
_Requires_lock_held_(_Global_critical_region_)
NTSTATUS
NTAPI
FatAcquireForCcFlush (
    IN PFILE_OBJECT FileObject,
    IN PDEVICE_OBJECT DeviceObject
    )
{
    PFCB Fcb;
    PCCB Ccb;
    PVCB Vcb;
    PFSRTL_COMMON_FCB_HEADER Header;
    TYPE_OF_OPEN Type;

    PAGED_CODE();
    UNREFERENCED_PARAMETER( DeviceObject );

    //
    //  Once again, the hack for making this look like
    //  a recursive call if needed. We cannot let ourselves
    //  verify under something that has resources held.
    //
    //  This value is good.  We should never try to acquire
    //  the file this way underneath of the cache.
    //

    NT_ASSERT( IoGetTopLevelIrp() != (PIRP)FSRTL_CACHE_TOP_LEVEL_IRP );

    if (IoGetTopLevelIrp() == NULL) {

        IoSetTopLevelIrp((PIRP)FSRTL_CACHE_TOP_LEVEL_IRP);
    }

    //
    //  Time for some exposition.
    //
    //  Lockorder for FAT is main->bcb->pagingio. Invert this at your obvious peril.
    //  The default logic for AcquireForCcFlush breaks this since in writethrough
    //  unpinrepinned we will grab the bcb then Mm will use the callback (which
    //  orders us with respect to the MmCollidedFlushEvent) to help us. If for
    //  directories/ea we then grab the main we are out of order.
    //
    //  Fortunately, we do not need main. We only need paging - just look at the write
    //  path. This is basic pre-acquisition.
    //
    //  Regular files require both resources, and are safe since we never pin them.
    //

    //
    //  Note that we do not need to disable APC delivery to guard
    //  against a rogue user issuing a suspend APC. That is because
    //  it is guaranteed that the caller is either in the system context,
    //  to which a user cannot deliver a suspend APC, or the caller has
    //  already disabled kernel APC delivery before calling. This is true
    //  for all the other pre-acquire routines as well.
    //

    Type = FatDecodeFileObject( FileObject, &Vcb, &Fcb, &Ccb );
    Header = (PFSRTL_COMMON_FCB_HEADER) FileObject->FsContext;

    if (Type < DirectoryFile) {

        if (Header->Resource) {

            if (!ExIsResourceAcquiredSharedLite( Header->Resource )) {

                ExAcquireResourceExclusiveLite( Header->Resource, TRUE );

            } else {

                ExAcquireResourceSharedLite( Header->Resource, TRUE );
            }
        }
    }

    if (Header->PagingIoResource) {

        ExAcquireResourceSharedLite( Header->PagingIoResource, TRUE );
    }

    return STATUS_SUCCESS;
}


_Requires_lock_held_(_Global_critical_region_)
NTSTATUS
NTAPI
FatReleaseForCcFlush (
    IN PFILE_OBJECT FileObject,
    IN PDEVICE_OBJECT DeviceObject
    )
{
    PFCB Fcb;
    PCCB Ccb;
    PVCB Vcb;
    PFSRTL_COMMON_FCB_HEADER Header;
    TYPE_OF_OPEN Type;

    PAGED_CODE();
    UNREFERENCED_PARAMETER( DeviceObject );

    //
    //  Clear up our hint.
    //

    if (IoGetTopLevelIrp() == (PIRP)FSRTL_CACHE_TOP_LEVEL_IRP) {

        IoSetTopLevelIrp( NULL );
    }

    Type = FatDecodeFileObject( FileObject, &Vcb, &Fcb, &Ccb );
    Header = (PFSRTL_COMMON_FCB_HEADER) FileObject->FsContext;

    if (Type < DirectoryFile) {

        if (Header->Resource) {

            ExReleaseResourceLite( Header->Resource );
        }
    }

    if (Header->PagingIoResource) {

        ExReleaseResourceLite( Header->PagingIoResource );
    }

    return STATUS_SUCCESS;
}


BOOLEAN
NTAPI
FatNoOpAcquire (
    IN PVOID Fcb,
    IN BOOLEAN Wait
    )

/*++

Routine Description:

    This routine does nothing.

Arguments:

    Fcb - The Fcb/Dcb/Vcb which was specified as a context parameter for this
          routine.

    Wait - TRUE if the caller is willing to block.

Return Value:

    TRUE

--*/

{
    UNREFERENCED_PARAMETER( Fcb );
    UNREFERENCED_PARAMETER( Wait );

    PAGED_CODE();

    //
    //  This is a kludge because Cc is really the top level.  We it
    //  enters the file system, we will think it is a resursive call
    //  and complete the request with hard errors or verify.  It will
    //  have to deal with them, somehow....
    //

    NT_ASSERT(IoGetTopLevelIrp() == NULL);

    IoSetTopLevelIrp((PIRP)FSRTL_CACHE_TOP_LEVEL_IRP);

    return TRUE;
}


VOID
NTAPI
FatNoOpRelease (
    IN PVOID Fcb
    )

/*++

Routine Description:

    This routine does nothing.

Arguments:

    Fcb - The Fcb/Dcb/Vcb which was specified as a context parameter for this
          routine.

Return Value:

    None

--*/

{
    PAGED_CODE();

    //
    //  Clear the kludge at this point.
    //

    NT_ASSERT(IoGetTopLevelIrp() == (PIRP)FSRTL_CACHE_TOP_LEVEL_IRP);

    IoSetTopLevelIrp( NULL );

    UNREFERENCED_PARAMETER( Fcb );

    return;
}


_Requires_lock_held_(_Global_critical_region_)
NTSTATUS
NTAPI
FatFilterCallbackAcquireForCreateSection (
    IN PFS_FILTER_CALLBACK_DATA CallbackData,
    OUT PVOID *CompletionContext
    )

/*++

Routine Description:

    This is the callback routine for MM to use to acquire the file exclusively.

    NOTE:  This routine expects the default FSRTL routine to be used to release
           the resource.  If this routine is ever changed to acquire something
           other than main, a corresponding release routine will be required.

Arguments:

    FS_FILTER_CALLBACK_DATA - Filter based callback data that provides the file object we
                              want to acquire.

    CompletionContext - Ignored.

Return Value:

    On success we return STATUS_FSFILTER_OP_COMPLETED_SUCCESSFULLY.

    If SyncType is SyncTypeCreateSection, we return a status that indicates whether there
    are any writers to this file.  Note that main is acquired, so new handles cannot be opened.

--*/

{
    PFCB Fcb;

    PAGED_CODE();

    NT_ASSERT( CallbackData->Operation == FS_FILTER_ACQUIRE_FOR_SECTION_SYNCHRONIZATION );
    NT_ASSERT( CallbackData->SizeOfFsFilterCallbackData == sizeof(FS_FILTER_CALLBACK_DATA) );

    //
    //  Grab the Fcb from the callback data file object.
    //

    Fcb = CallbackData->FileObject->FsContext;

    //
    //  Take main exclusive.
    //

    //
    //  Note that we do not need to disable APC delivery to guard
    //  against a rogue user issuing a suspend APC. That is because
    //  it is guaranteed that the caller is either in the system context,
    //  to which a user cannot deliver a suspend APC, or the caller has
    //  already disabled kernel APC delivery before calling. This is true
    //  for all the other pre-acquire routines as well.
    //

    if (Fcb->Header.Resource) {

        ExAcquireResourceExclusiveLite( Fcb->Header.Resource, TRUE );
    }

    //
    //  Return the appropriate status based on the type of synchronization and whether anyone
    //  has write access to this file.
    //

    if (CallbackData->Parameters.AcquireForSectionSynchronization.SyncType != SyncTypeCreateSection) {

        return STATUS_FSFILTER_OP_COMPLETED_SUCCESSFULLY;

    } else if (Fcb->ShareAccess.Writers == 0) {

        return STATUS_FILE_LOCKED_WITH_ONLY_READERS;

    } else {

        return STATUS_FILE_LOCKED_WITH_WRITERS;
    }

    UNREFERENCED_PARAMETER( CompletionContext );
}

