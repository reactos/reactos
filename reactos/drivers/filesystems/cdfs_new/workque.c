/*++

Copyright (c) 1989-2000 Microsoft Corporation

Module Name:

    WorkQue.c

Abstract:

    This module implements the Work queue routines for the Cdfs File
    system.


--*/

#include "CdProcs.h"

//
//  The Bug check file id for this module
//

#define BugCheckFileId                   (CDFS_BUG_CHECK_WORKQUE)

//
//  The following constant is the maximum number of ExWorkerThreads that we
//  will allow to be servicing a particular target device at any one time.
//

#define FSP_PER_DEVICE_THRESHOLD         (2)

//
//  Local support routines
//

VOID
CdAddToWorkque (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, CdFsdPostRequest)
#pragma alloc_text(PAGE, CdOplockComplete)
#pragma alloc_text(PAGE, CdPrePostIrp)
#endif


NTSTATUS
CdFsdPostRequest (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine enqueues the request packet specified by IrpContext to the
    work queue associated with the FileSystemDeviceObject.  This is a FSD
    routine.

Arguments:

    IrpContext - Pointer to the IrpContext to be queued to the Fsp.

    Irp - I/O Request Packet.

Return Value:

    STATUS_PENDING

--*/

{
    PAGED_CODE();

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_IRP( Irp );

    //
    //  Posting is a three step operation.  First lock down any buffers
    //  in the Irp.  Next cleanup the IrpContext for the post and finally
    //  add this to a workque.
    //

    CdPrePostIrp( IrpContext, Irp );

    CdAddToWorkque( IrpContext, Irp );

    //
    //  And return to our caller
    //

    return STATUS_PENDING;
}


VOID
CdPrePostIrp (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine performs any neccessary work before STATUS_PENDING is
    returned with the Fsd thread.  This routine is called within the
    filesystem and by the oplock package.

Arguments:

    Context - Pointer to the IrpContext to be queued to the Fsp

    Irp - I/O Request Packet.

Return Value:

    None.

--*/

{
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation( Irp );
    BOOLEAN RemovedFcb;

    PAGED_CODE();

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_IRP( Irp );

    //
    //  Case on the type of the operation.
    //

    switch (IrpContext->MajorFunction) {

    case IRP_MJ_CREATE :

        //
        //  If called from the oplock package then there is an
        //  Fcb to possibly teardown.  We will call the teardown
        //  routine and release the Fcb if still present.  The cleanup
        //  code in create will know not to release this Fcb because
        //  we will clear the pointer.
        //

        if ((IrpContext->TeardownFcb != NULL) &&
            *(IrpContext->TeardownFcb) != NULL) {

            CdTeardownStructures( IrpContext, *(IrpContext->TeardownFcb), &RemovedFcb );

            if (!RemovedFcb) {

                CdReleaseFcb( IrpContext, *(IrpContext->TeardownFcb) );
            }

            *(IrpContext->TeardownFcb) = NULL;
            IrpContext->TeardownFcb = NULL;
        }

        break;

    //
    //  We need to lock the user's buffer, unless this is an MDL-read,
    //  in which case there is no user buffer.
    //

    case IRP_MJ_READ :

        if (!FlagOn( IrpContext->MinorFunction, IRP_MN_MDL )) {

            CdLockUserBuffer( IrpContext, IrpSp->Parameters.Read.Length );
        }

        break;

    //
    //  We also need to check whether this is a query file operation.
    //

    case IRP_MJ_DIRECTORY_CONTROL :

        if (IrpContext->MinorFunction == IRP_MN_QUERY_DIRECTORY) {

            CdLockUserBuffer( IrpContext, IrpSp->Parameters.QueryDirectory.Length );
        }

        break;
    }

    //
    //  Cleanup the IrpContext for the post.
    //

    SetFlag( IrpContext->Flags, IRP_CONTEXT_FLAG_MORE_PROCESSING );
    CdCleanupIrpContext( IrpContext, TRUE );

    //
    //  Mark the Irp to show that we've already returned pending to the user.
    //

    IoMarkIrpPending( Irp );

    return;
}


VOID
CdOplockComplete (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine is called by the oplock package when an oplock break has
    completed, allowing an Irp to resume execution.  If the status in
    the Irp is STATUS_SUCCESS, then we queue the Irp to the Fsp queue.
    Otherwise we complete the Irp with the status in the Irp.

    If we are completing due to an error then check if there is any
    cleanup to do.

Arguments:

    Irp - I/O Request Packet.

Return Value:

    None.

--*/

{
    BOOLEAN RemovedFcb;

    PAGED_CODE();

    //
    //  Check on the return value in the Irp.  If success then we
    //  are to post this request.
    //

    if (Irp->IoStatus.Status == STATUS_SUCCESS) {

        //
        //  Check if there is any cleanup work to do.
        //

        switch (IrpContext->MajorFunction) {

        case IRP_MJ_CREATE :

            //
            //  If called from the oplock package then there is an
            //  Fcb to possibly teardown.  We will call the teardown
            //  routine and release the Fcb if still present.  The cleanup
            //  code in create will know not to release this Fcb because
            //  we will clear the pointer.
            //

            if (IrpContext->TeardownFcb != NULL) {

                CdTeardownStructures( IrpContext, *(IrpContext->TeardownFcb), &RemovedFcb );

                if (!RemovedFcb) {

                    CdReleaseFcb( IrpContext, *(IrpContext->TeardownFcb) );
                }

                *(IrpContext->TeardownFcb) = NULL;
                IrpContext->TeardownFcb = NULL;
            }

            break;
        }

        //
        //  Insert the Irp context in the workqueue.
        //

        CdAddToWorkque( IrpContext, Irp );

    //
    //  Otherwise complete the request.
    //

    } else {

        CdCompleteRequest( IrpContext, Irp, Irp->IoStatus.Status );
    }

    return;
}


//
//  Local support routine
//

VOID
CdAddToWorkque (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine is called to acually store the posted Irp to the Fsp
    workque.

Arguments:

    IrpContext - Pointer to the IrpContext to be queued to the Fsp

    Irp - I/O Request Packet.

Return Value:

    None.

--*/

{
    PVOLUME_DEVICE_OBJECT Vdo;
    KIRQL SavedIrql;
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation( Irp );

    //
    //  Check if this request has an associated file object, and thus volume
    //  device object.
    //

    if (IrpSp->FileObject != NULL) {


        Vdo = CONTAINING_RECORD( IrpSp->DeviceObject,
                                 VOLUME_DEVICE_OBJECT,
                                 DeviceObject );

        //
        //  Check to see if this request should be sent to the overflow
        //  queue.  If not, then send it off to an exworker thread.
        //

        KeAcquireSpinLock( &Vdo->OverflowQueueSpinLock, &SavedIrql );

        if (Vdo->PostedRequestCount > FSP_PER_DEVICE_THRESHOLD) {

            //
            //  We cannot currently respond to this IRP so we'll just enqueue it
            //  to the overflow queue on the volume.
            //

            InsertTailList( &Vdo->OverflowQueue,
                            &IrpContext->WorkQueueItem.List );

            Vdo->OverflowQueueCount += 1;

            KeReleaseSpinLock( &Vdo->OverflowQueueSpinLock, SavedIrql );

            return;

        } else {

            //
            //  We are going to send this Irp to an ex worker thread so up
            //  the count.
            //

            Vdo->PostedRequestCount += 1;

            KeReleaseSpinLock( &Vdo->OverflowQueueSpinLock, SavedIrql );
        }
    }

    //
    //  Send it off.....
    //

    ExInitializeWorkItem( &IrpContext->WorkQueueItem,
                          CdFspDispatch,
                          IrpContext );

    ExQueueWorkItem( &IrpContext->WorkQueueItem, CriticalWorkQueue );

    return;
}



