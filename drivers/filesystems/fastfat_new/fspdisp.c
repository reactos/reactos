/*++

Copyright (c) 1989-2000 Microsoft Corporation

Module Name:

    FspDisp.c

Abstract:

    This module implements the main dispatch procedure/thread for the Fat
    Fsp


--*/

#include "fatprocs.h"

//
//  Internal support routine, spinlock wrapper.
//

PVOID
FatRemoveOverflowEntry (
    IN PVOLUME_DEVICE_OBJECT VolDo
    );

//
//  Define our local debug trace level
//

#define Dbg                              (DEBUG_TRACE_FSP_DISPATCHER)

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, FatFspDispatch)
#endif


VOID
NTAPI
FatFspDispatch (
    _In_ PVOID Context
    )

/*++

Routine Description:

    This is the main FSP thread routine that is executed to receive
    and dispatch IRP requests.  Each FSP thread begins its execution here.
    There is one thread created at system initialization time and subsequent
    threads created as needed.

Arguments:


    Context - Supplies the thread id.

Return Value:

    None - This routine never exits

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;


    PIRP Irp;
    PIRP_CONTEXT IrpContext;
    PIO_STACK_LOCATION IrpSp;
    BOOLEAN VcbDeleted;
    BOOLEAN  ExceptionCompletedIrp = FALSE;

    PVOLUME_DEVICE_OBJECT VolDo;

    UCHAR MajorFunction = 0;

    PAGED_CODE();

    IrpContext = (PIRP_CONTEXT)Context;

    Irp = IrpContext->OriginatingIrp;

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    //
    //  Now because we are the Fsp we will force the IrpContext to
    //  indicate true on Wait.
    //

    SetFlag(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT | IRP_CONTEXT_FLAG_IN_FSP);

    //
    //  If this request has an associated volume device object, remember it.
    //

    if ( IrpSp->FileObject != NULL ) {

        VolDo = CONTAINING_RECORD( IrpSp->DeviceObject,
                                   VOLUME_DEVICE_OBJECT,
                                   DeviceObject );
    } else {

        VolDo = NULL;
    }

    //
    //  Now case on the function code.  For each major function code,
    //  either call the appropriate FSP routine or case on the minor
    //  function and then call the FSP routine.  The FSP routine that
    //  we call is responsible for completing the IRP, and not us.
    //  That way the routine can complete the IRP and then continue
    //  post processing as required.  For example, a read can be
    //  satisfied right away and then read can be done.
    //
    //  We'll do all of the work within an exception handler that
    //  will be invoked if ever some underlying operation gets into
    //  trouble (e.g., if FatReadSectorsSync has trouble).
    //

    while ( TRUE ) {

        ExceptionCompletedIrp = FALSE;

        DebugTrace(0, Dbg, "FatFspDispatch: Irp = %p\n", Irp);

        //
        //  If this Irp was top level, note it in our thread local storage.
        //

        FsRtlEnterFileSystem();

        if ( FlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_RECURSIVE_CALL) ) {

            IoSetTopLevelIrp( (PIRP)FSRTL_FSP_TOP_LEVEL_IRP );

        } else {

            IoSetTopLevelIrp( Irp );
        }

        MajorFunction = IrpContext->MajorFunction;

        _SEH2_TRY {

            switch ( MajorFunction ) {

                //
                //  For Create Operation,
                //

                case IRP_MJ_CREATE:

                    Status = FatCommonCreate( IrpContext, Irp );
                    break;

                //
                //  For close operations.  We do a little kludge here in case
                //  this close causes a volume to go away.  It will NULL the
                //  VolDo local variable so that we will not try to look at
                //  the overflow queue.
                //

                case IRP_MJ_CLOSE:

                {
                    PVCB Vcb;
                    PFCB Fcb;
                    PCCB Ccb;
                    TYPE_OF_OPEN TypeOfOpen;

                    //
                    //  Extract and decode the file object
                    //

                    TypeOfOpen = FatDecodeFileObject( IrpSp->FileObject, &Vcb, &Fcb, &Ccb );

                    //
                    //  Do the close.  We have a slightly different format
                    //  for this call because of the async closes.
                    //

                    Status = FatCommonClose( Vcb,
                                             Fcb,
                                             Ccb,
                                             TypeOfOpen,
                                             TRUE,
                                             TRUE,
                                             &VcbDeleted );

                    //
                    //  If the VCB was deleted, do not try to access it later.
                    //

                    if (VcbDeleted) {

                        VolDo = NULL;
                    }

                    NT_ASSERT(Status == STATUS_SUCCESS);

                    FatCompleteRequest( IrpContext, Irp, Status );

                    break;
                }

                //
                //  For read operations
                //

                case IRP_MJ_READ:

                    (VOID) FatCommonRead( IrpContext, Irp );
                    break;

                //
                //  For write operations,
                //

                case IRP_MJ_WRITE:

                    (VOID) FatCommonWrite( IrpContext, Irp );
                    break;

                //
                //  For Query Information operations,
                //

                case IRP_MJ_QUERY_INFORMATION:

                    (VOID) FatCommonQueryInformation( IrpContext, Irp );
                    break;

                //
                //  For Set Information operations,
                //

                case IRP_MJ_SET_INFORMATION:

                    (VOID) FatCommonSetInformation( IrpContext, Irp );
                    break;

                //
                //  For Query EA operations,
                //

                case IRP_MJ_QUERY_EA:

                    (VOID) FatCommonQueryEa( IrpContext, Irp );
                    break;

                //
                //  For Set EA operations,
                //

                case IRP_MJ_SET_EA:

                    (VOID) FatCommonSetEa( IrpContext, Irp );
                    break;

                //
                //  For Flush buffers operations,
                //

                case IRP_MJ_FLUSH_BUFFERS:

                    (VOID) FatCommonFlushBuffers( IrpContext, Irp );
                    break;

                //
                //  For Query Volume Information operations,
                //

                case IRP_MJ_QUERY_VOLUME_INFORMATION:

                    (VOID) FatCommonQueryVolumeInfo( IrpContext, Irp );
                    break;

                //
                //  For Set Volume Information operations,
                //

                case IRP_MJ_SET_VOLUME_INFORMATION:

                    (VOID) FatCommonSetVolumeInfo( IrpContext, Irp );
                    break;

                //
                //  For File Cleanup operations,
                //

                case IRP_MJ_CLEANUP:

                    (VOID) FatCommonCleanup( IrpContext, Irp );
                    break;

                //
                //  For Directory Control operations,
                //

                case IRP_MJ_DIRECTORY_CONTROL:

                    (VOID) FatCommonDirectoryControl( IrpContext, Irp );
                    break;

                //
                //  For File System Control operations,
                //

                case IRP_MJ_FILE_SYSTEM_CONTROL:

                    (VOID) FatCommonFileSystemControl( IrpContext, Irp );
                    break;

                //
                //  For Lock Control operations,
                //

                case IRP_MJ_LOCK_CONTROL:

                    (VOID) FatCommonLockControl( IrpContext, Irp );
                    break;

                //
                //  For Device Control operations,
                //

                case IRP_MJ_DEVICE_CONTROL:

                    (VOID) FatCommonDeviceControl( IrpContext, Irp );
                    break;

                //
                //  For the Shutdown operation,
                //

                case IRP_MJ_SHUTDOWN:

                    (VOID) FatCommonShutdown( IrpContext, Irp );
                    break;

                //
                //  For plug and play operations.
                //

                case IRP_MJ_PNP:

                    //
                    //  I don't believe this should ever occur, but allow for the unexpected.
                    //

                    (VOID) FatCommonPnp( IrpContext, Irp );
                    break;

                //
                //  For any other major operations, return an invalid
                //  request.
                //

                default:

                    FatCompleteRequest( IrpContext, Irp, STATUS_INVALID_DEVICE_REQUEST );
                    break;

            }

        } _SEH2_EXCEPT(FatExceptionFilter( IrpContext, _SEH2_GetExceptionInformation() )) {

            //
            //  We had some trouble trying to perform the requested
            //  operation, so we'll abort the I/O request with
            //  the error status that we get back from the
            //  execption code.
            //

            (VOID) FatProcessException( IrpContext, Irp, _SEH2_GetExceptionCode() );
            ExceptionCompletedIrp = TRUE;
        } _SEH2_END;

        IoSetTopLevelIrp( NULL );

        FsRtlExitFileSystem();


        if (MajorFunction == IRP_MJ_CREATE && !ExceptionCompletedIrp && Status != STATUS_PENDING) {

            //
            // Creates are completed here. IrpContext is also freed here.
            //

            FatCompleteRequest( IrpContext, Irp, Status );
        }

        //
        //  If there are any entries on this volume's overflow queue, service
        //  them.
        //

        if ( VolDo != NULL ) {

            PVOID Entry;

            //
            //  We have a volume device object so see if there is any work
            //  left to do in its overflow queue.
            //

            Entry = FatRemoveOverflowEntry( VolDo );

            //
            //  There wasn't an entry, break out of the loop and return to
            //  the Ex Worker thread.
            //

            if ( Entry == NULL ) {

                break;
            }

            //
            //  Extract the IrpContext, Irp, and IrpSp, and loop.
            //

            IrpContext = CONTAINING_RECORD( Entry,
                                            IRP_CONTEXT,
                                            WorkQueueItem.List );

            SetFlag(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT | IRP_CONTEXT_FLAG_IN_FSP);

            Irp = IrpContext->OriginatingIrp;

            IrpSp = IoGetCurrentIrpStackLocation( Irp );

            continue;

        } else {

            break;
        }
    }

    return;
}


//
//  Internal support routine, spinlock wrapper.
//

PVOID
FatRemoveOverflowEntry (
    IN PVOLUME_DEVICE_OBJECT VolDo
    )
{
    PVOID Entry;
    KIRQL SavedIrql;

    KeAcquireSpinLock( &VolDo->OverflowQueueSpinLock, &SavedIrql );

    if (VolDo->OverflowQueueCount > 0) {

        //
        //  There is overflow work to do in this volume so we'll
        //  decrement the Overflow count, dequeue the IRP, and release
        //  the Event
        //

        VolDo->OverflowQueueCount -= 1;

        Entry = RemoveHeadList( &VolDo->OverflowQueue );

    } else {

        VolDo->PostedRequestCount -= 1;

        Entry = NULL;
    }

    KeReleaseSpinLock( &VolDo->OverflowQueueSpinLock, SavedIrql );

    return Entry;
}


