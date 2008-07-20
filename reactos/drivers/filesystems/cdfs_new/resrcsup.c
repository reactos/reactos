/*++

Copyright (c) 1989-2000 Microsoft Corporation

Module Name:

    ResrcSup.c

Abstract:

    This module implements the Cdfs Resource acquisition routines


--*/

#include "CdProcs.h"

//
//  The Bug check file id for this module
//

#define BugCheckFileId                   (CDFS_BUG_CHECK_RESRCSUP)

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, CdAcquireForCache)
#pragma alloc_text(PAGE, CdAcquireForCreateSection)
#pragma alloc_text(PAGE, CdAcquireResource)
#pragma alloc_text(PAGE, CdNoopAcquire)
#pragma alloc_text(PAGE, CdNoopRelease)
#pragma alloc_text(PAGE, CdReleaseForCreateSection)
#pragma alloc_text(PAGE, CdReleaseFromCache)
#endif


BOOLEAN
CdAcquireResource (
    IN PIRP_CONTEXT IrpContext,
    IN PERESOURCE Resource,
    IN BOOLEAN IgnoreWait,
    IN TYPE_OF_ACQUIRE Type
    )

/*++

Routine Description:

    This is the single routine used to acquire file system resources.  It
    looks at the IgnoreWait flag to determine whether to try to acquire the
    resource without waiting.  Returning TRUE/FALSE to indicate success or
    failure.  Otherwise it is driven by the WAIT flag in the IrpContext and
    will raise CANT_WAIT on a failure.

Arguments:

    Resource - This is the resource to try and acquire.

    IgnoreWait - If TRUE then this routine will not wait to acquire the
        resource and will return a boolean indicating whether the resource was
        acquired.  Otherwise we use the flag in the IrpContext and raise
        if the resource is not acquired.

    Type - Indicates how we should try to get the resource.

Return Value:

    BOOLEAN - TRUE if the resource is acquired.  FALSE if not acquired and
        IgnoreWait is specified.  Otherwise we raise CANT_WAIT.

--*/

{
    BOOLEAN Wait = FALSE;
    BOOLEAN Acquired;
    PAGED_CODE();

    //
    //  We look first at the IgnoreWait flag, next at the flag in the Irp
    //  Context to decide how to acquire this resource.
    //

    if (!IgnoreWait && FlagOn( IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT )) {

        Wait = TRUE;
    }

    //
    //  Attempt to acquire the resource either shared or exclusively.
    //

    switch (Type) {
        case AcquireExclusive:
        
            Acquired = ExAcquireResourceExclusiveLite( Resource, Wait );
            break;

        case AcquireShared:
            
            Acquired = ExAcquireResourceSharedLite( Resource, Wait );
            break;

        case AcquireSharedStarveExclusive:
            
            Acquired = ExAcquireSharedStarveExclusive( Resource, Wait );
            break;

        default:
        	Acquired = FALSE;
            ASSERT( FALSE );
    }

    //
    //  If not acquired and the user didn't specifiy IgnoreWait then
    //  raise CANT_WAIT.
    //

    if (!Acquired && !IgnoreWait) {

        CdRaiseStatus( IrpContext, STATUS_CANT_WAIT );
    }

    return Acquired;
}


BOOLEAN
CdAcquireForCache (
    IN PFCB Fcb,
    IN BOOLEAN Wait
    )

/*++

Routine Description:

    The address of this routine is specified when creating a CacheMap for
    a file.  It is subsequently called by the Lazy Writer for synchronization.

Arguments:

    Fcb -  The pointer supplied as context to the cache initialization
           routine.

    Wait - TRUE if the caller is willing to block.

Return Value:

    None

--*/

{
    PAGED_CODE();

    ASSERT(IoGetTopLevelIrp() == NULL);
    IoSetTopLevelIrp((PIRP)FSRTL_CACHE_TOP_LEVEL_IRP);

    return ExAcquireResourceSharedLite( Fcb->Resource, Wait );
}


VOID
CdReleaseFromCache (
    IN PFCB Fcb
    )

/*++

Routine Description:

    The address of this routine is specified when creating a CacheMap for
    a virtual file.  It is subsequently called by the Lazy Writer to release
    a resource acquired above.

Arguments:

    Fcb -  The pointer supplied as context to the cache initialization
           routine.

Return Value:

    None

--*/

{
    PAGED_CODE();

    ASSERT(IoGetTopLevelIrp() == (PIRP)FSRTL_CACHE_TOP_LEVEL_IRP);
    IoSetTopLevelIrp( NULL );
    
    ExReleaseResourceLite( Fcb->Resource );
}


BOOLEAN
CdNoopAcquire (
    IN PVOID Fcb,
    IN BOOLEAN Wait
    )

/*++

Routine Description:

    This routine does nothing.

Arguments:

    Fcb - The Fcb/Vcb which was specified as a context parameter for this
          routine.

    Wait - TRUE if the caller is willing to block.

Return Value:

    TRUE

--*/

{
    PAGED_CODE();
    return TRUE;
}


VOID
CdNoopRelease (
    IN PVOID Fcb
    )

/*++

Routine Description:

    This routine does nothing.

Arguments:

    Fcb - The Fcb/Vcb which was specified as a context parameter for this
          routine.

Return Value:

    None

--*/

{
    PAGED_CODE();
}


VOID
CdAcquireForCreateSection (
    IN PFILE_OBJECT FileObject
    )

/*++

Routine Description:

    This is the callback routine for MM to use to acquire the file exclusively.

Arguments:

    FileObject - File object for a Cdfs stream.

Return Value:

    None

--*/

{
    PAGED_CODE();

    
    //
    //  Get the Fcb resource exclusively.
    //

    ExAcquireResourceExclusiveLite( &((PFCB) FileObject->FsContext)->FcbNonpaged->FcbResource,
                                TRUE );
                                
    //
    //  Take the File resource shared.  We need this later on when MM calls 
    //  QueryStandardInfo to get the file size.  
    //
    //  If we don't use StarveExclusive,  then we can get wedged behind an 
    //  exclusive waiter who is waiting on someone else holding it shared in the 
    //  read->initializecachemap path (which calls createsection) who is in turn 
    //  waiting on us to finish the create section.
    //

    ExAcquireSharedStarveExclusive( ((PFCB) FileObject->FsContext)->Resource,
                                    TRUE );
}


VOID
CdReleaseForCreateSection (
    IN PFILE_OBJECT FileObject
    )

/*++

Routine Description:

    This is the callback routine for MM to use to release a file acquired with
    the AcquireForCreateSection call above.

Arguments:

    FileObject - File object for a Cdfs stream.

Return Value:

    None

--*/

{
    PAGED_CODE();

    //
    //  Release the resources.
    //

    ExReleaseResourceLite( &((PFCB) FileObject->FsContext)->FcbNonpaged->FcbResource );
    ExReleaseResourceLite( ((PFCB) FileObject->FsContext)->Resource);
}

