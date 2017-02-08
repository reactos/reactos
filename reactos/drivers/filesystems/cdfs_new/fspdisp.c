/*++

Copyright (c) 1989-2000 Microsoft Corporation

Module Name:

    FspDisp.c

Abstract:

    This module implements the main dispatch procedure/thread for the Cdfs
    Fsp


--*/

#include "CdProcs.h"

//
//  The Bug check file id for this module
//

#define BugCheckFileId                   (CDFS_BUG_CHECK_FSPDISP)


VOID
CdFspDispatch (
    IN PIRP_CONTEXT IrpContext
    )

/*++

Routine Description:

    This is the main FSP thread routine that is executed to receive
    and dispatch IRP requests.  Each FSP thread begins its execution here.
    There is one thread created at system initialization time and subsequent
    threads created as needed.

Arguments:

    IrpContext - IrpContext for a request to process.

Return Value:

    None

--*/

{
    THREAD_CONTEXT ThreadContext;
    NTSTATUS Status;

    PIRP Irp = IrpContext->Irp;
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation( Irp );

    PVOLUME_DEVICE_OBJECT VolDo = NULL;

    //
    //  If this request has an associated volume device object, remember it.
    //

    if (IrpSp->FileObject != NULL) {

        VolDo = CONTAINING_RECORD( IrpSp->DeviceObject,
                                   VOLUME_DEVICE_OBJECT,
                                   DeviceObject );
    }

    //
    //  Now case on the function code.  For each major function code,
    //  either call the appropriate worker routine.  This routine that
    //  we call is responsible for completing the IRP, and not us.
    //  That way the routine can complete the IRP and then continue
    //  post processing as required.  For example, a read can be
    //  satisfied right away and then read can be done.
    //
    //  We'll do all of the work within an exception handler that
    //  will be invoked if ever some underlying operation gets into
    //  trouble.
    //

    while ( TRUE ) {

        //
        //  Set all the flags indicating we are in the Fsp.
        //

        SetFlag( IrpContext->Flags, IRP_CONTEXT_FSP_FLAGS );

        FsRtlEnterFileSystem();

        CdSetThreadContext( IrpContext, &ThreadContext );

        while (TRUE) {

            try {

                //
                //  Reinitialize for the next try at completing this
                //  request.
                //

                Status =
                IrpContext->ExceptionStatus = STATUS_SUCCESS;

                //
                //  Initialize the Io status field in the Irp.
                //

                Irp->IoStatus.Status = STATUS_SUCCESS;
                Irp->IoStatus.Information = 0;

                //
                //  Case on the major irp code.
                //

                switch (IrpContext->MajorFunction) {

                case IRP_MJ_CREATE :

                    CdCommonCreate( IrpContext, Irp );
                    break;

                case IRP_MJ_CLOSE :

                    ASSERT( FALSE );
                    break;

                case IRP_MJ_READ :

                    CdCommonRead( IrpContext, Irp );
                    break;

                case IRP_MJ_QUERY_INFORMATION :

                    CdCommonQueryInfo( IrpContext, Irp );
                    break;

                case IRP_MJ_SET_INFORMATION :

                    CdCommonSetInfo( IrpContext, Irp );
                    break;

                case IRP_MJ_QUERY_VOLUME_INFORMATION :

                    CdCommonQueryVolInfo( IrpContext, Irp );
                    break;

                case IRP_MJ_DIRECTORY_CONTROL :

                    CdCommonDirControl( IrpContext, Irp );
                    break;

                case IRP_MJ_FILE_SYSTEM_CONTROL :

                    CdCommonFsControl( IrpContext, Irp );
                    break;

                case IRP_MJ_DEVICE_CONTROL :

                    CdCommonDevControl( IrpContext, Irp );
                    break;

                case IRP_MJ_LOCK_CONTROL :

                    CdCommonLockControl( IrpContext, Irp );
                    break;

                case IRP_MJ_CLEANUP :

                    CdCommonCleanup( IrpContext, Irp );
                    break;

                case IRP_MJ_PNP :

                    ASSERT( FALSE );
                    CdCommonPnp( IrpContext, Irp );
                    break;

                default :

                    Status = STATUS_INVALID_DEVICE_REQUEST;
                    CdCompleteRequest( IrpContext, Irp, Status );
                }

            } except( CdExceptionFilter( IrpContext, GetExceptionInformation() )) {

                Status = CdProcessException( IrpContext, Irp, GetExceptionCode() );
            }

            //
            //  Break out of the loop if we didn't get CANT_WAIT.
            //

            if (Status != STATUS_CANT_WAIT) { break; }

            //
            //  We are retrying this request.  Cleanup the IrpContext for the retry.
            //

            SetFlag( IrpContext->Flags, IRP_CONTEXT_FLAG_MORE_PROCESSING );
            CdCleanupIrpContext( IrpContext, FALSE );
        }

        FsRtlExitFileSystem();

        //
        //  If there are any entries on this volume's overflow queue, service
        //  them.
        //

        if (VolDo != NULL) {

            KIRQL SavedIrql;
            PVOID Entry = NULL;

            //
            //  We have a volume device object so see if there is any work
            //  left to do in its overflow queue.
            //

            KeAcquireSpinLock( &VolDo->OverflowQueueSpinLock, &SavedIrql );

            if (VolDo->OverflowQueueCount > 0) {

                //
                //  There is overflow work to do in this volume so we'll
                //  decrement the Overflow count, dequeue the IRP, and release
                //  the Event
                //

                VolDo->OverflowQueueCount -= 1;

                Entry = RemoveHeadList( &VolDo->OverflowQueue );
            }

            KeReleaseSpinLock( &VolDo->OverflowQueueSpinLock, SavedIrql );

            //
            //  There wasn't an entry, break out of the loop and return to
            //  the Ex Worker thread.
            //

            if (Entry == NULL) { break; }

            //
            //  Extract the IrpContext , Irp, set wait to TRUE, and loop.
            //

            IrpContext = CONTAINING_RECORD( Entry,
                                            IRP_CONTEXT,
                                            WorkQueueItem.List );

            Irp = IrpContext->Irp;
            IrpSp = IoGetCurrentIrpStackLocation( Irp );

            continue;
        }

        break;
    }

    //
    //  Decrement the PostedRequestCount if there was a volume device object.
    //

    if (VolDo) {

        InterlockedDecrement( &VolDo->PostedRequestCount );
    }

    return;
}




