/*
 * COPYRIGHT:        GNU GENERAL PUBLIC LICENSE VERSION 2
 * PROJECT:          ReiserFs file system driver for Windows NT/2000/XP/Vista.
 * FILE:             pnp.c
 * PURPOSE:          
 * PROGRAMMER:       Mark Piper, Matt Wu, Bo Brantén.
 * HOMEPAGE:         
 * UPDATE HISTORY: 
 */

/* INCLUDES *****************************************************************/

#include "rfsd.h"

/* GLOBALS ***************************************************************/

extern PRFSD_GLOBAL RfsdGlobal;

/* DEFINITIONS *************************************************************/

#define DBG_PNP DBG_USER

#ifdef _PREFAST_
IO_COMPLETION_ROUTINE RfsdPnpCompletionRoutine;
#endif // _PREFAST_

NTSTATUS NTAPI
RfsdPnpCompletionRoutine (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp,
    IN PVOID          Contxt     );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, RfsdPnp)
#pragma alloc_text(PAGE, RfsdPnpQueryRemove)
#pragma alloc_text(PAGE, RfsdPnpRemove)
#pragma alloc_text(PAGE, RfsdPnpCancelRemove)
#pragma alloc_text(PAGE, RfsdPnpSurpriseRemove)
#endif

/* FUNCTIONS *************************************************************/

#if (_WIN32_WINNT >= 0x0500)

NTSTATUS NTAPI
RfsdPnpCompletionRoutine (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Contxt
    )
{
    PKEVENT Event = (PKEVENT) Contxt;

    KeSetEvent( Event, 0, FALSE );

    return STATUS_MORE_PROCESSING_REQUIRED;

    UNREFERENCED_PARAMETER( DeviceObject );
    UNREFERENCED_PARAMETER( Contxt );
}

__drv_mustHoldCriticalRegion
NTSTATUS
RfsdPnp (IN PRFSD_IRP_CONTEXT IrpContext)
{
    NTSTATUS            Status = STATUS_INVALID_PARAMETER;
    PIRP                Irp;
    PIO_STACK_LOCATION  IrpSp;
    PRFSD_VCB           Vcb = 0;
    PDEVICE_OBJECT      DeviceObject;

    PAGED_CODE();

    _SEH2_TRY {

        ASSERT(IrpContext);
        
        ASSERT((IrpContext->Identifier.Type == RFSDICX) &&
            (IrpContext->Identifier.Size == sizeof(RFSD_IRP_CONTEXT)));

        DeviceObject = IrpContext->DeviceObject;
    
        Vcb = (PRFSD_VCB) DeviceObject->DeviceExtension;
        
        ASSERT(Vcb != NULL);

        if ( !((Vcb->Identifier.Type == RFSDVCB) &&
               (Vcb->Identifier.Size == sizeof(RFSD_VCB)))) {
            _SEH2_LEAVE; // Status = STATUS_INVALID_PARAMETER
        }
        
        Irp = IrpContext->Irp;
        IrpSp = IoGetCurrentIrpStackLocation(Irp);

        SetFlag(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT);
   
        switch ( IrpSp->MinorFunction ) {

            case IRP_MN_QUERY_REMOVE_DEVICE:

                RfsdPrint((DBG_PNP, "RfsdPnp: RfsdPnpQueryRemove...\n"));
                Status = RfsdPnpQueryRemove(IrpContext, Vcb);

                break;
        
            case IRP_MN_REMOVE_DEVICE:

                RfsdPrint((DBG_PNP, "RfsdPnp: RfsdPnpRemove...\n"));
                Status = RfsdPnpRemove(IrpContext, Vcb);
                break;

            case IRP_MN_CANCEL_REMOVE_DEVICE:
    
                RfsdPrint((DBG_PNP, "RfsdPnp: RfsdPnpCancelRemove...\n"));
                Status = RfsdPnpCancelRemove(IrpContext, Vcb);
                break;

            case IRP_MN_SURPRISE_REMOVAL:
        
                RfsdPrint((DBG_PNP, "RfsdPnp: RfsdPnpSupriseRemove...\n"));
                Status = RfsdPnpSurpriseRemove(IrpContext, Vcb);
                break;

            default:
                break;
        }

    } _SEH2_FINALLY {

        if (!IrpContext->ExceptionInProgress) {
            Irp = IrpContext->Irp;

            if (Irp) {

                //
                // Here we need pass the IRP to the disk driver.
                //

                IoSkipCurrentIrpStackLocation( Irp );
    
                Status = IoCallDriver(Vcb->TargetDeviceObject, Irp);

                IrpContext->Irp = NULL;
            }

            RfsdCompleteIrpContext(IrpContext, Status);
        }
    } _SEH2_END;
        
    return Status;
}

__drv_mustHoldCriticalRegion
NTSTATUS
RfsdPnpQueryRemove (
    PRFSD_IRP_CONTEXT IrpContext,
    PRFSD_VCB         Vcb
    )

{
    NTSTATUS Status;
    KEVENT   Event;
    BOOLEAN  bDeleted = FALSE;
    BOOLEAN  VcbAcquired = FALSE;

    PAGED_CODE();

    _SEH2_TRY {

        RfsdPrint((DBG_PNP, "RfsdPnpQueryRemove by RfsdPnp ...\n"));

        RfsdPrint((DBG_PNP, "RfsdPnpQueryRemove: RfsdFlushVolume ...\n"));

#if (_WIN32_WINNT >= 0x0500)
        CcWaitForCurrentLazyWriterActivity();
#endif

        ExAcquireResourceExclusiveLite(
            &Vcb->MainResource, TRUE );
        VcbAcquired = TRUE;

        RfsdFlushFiles(Vcb, FALSE);

        RfsdFlushVolume(Vcb, FALSE);

        RfsdPrint((DBG_PNP, "RfsdPnpQueryRemove: RfsdLockVcb: Vcb=%xh FileObject=%xh ...\n",
                            Vcb, IrpContext->FileObject));
        Status = RfsdLockVcb(Vcb, IrpContext->FileObject);

        RfsdPrint((DBG_PNP, "RfsdPnpQueryRemove: RfsdPurgeVolume ...\n"));
        RfsdPurgeVolume(Vcb, TRUE);

        if (!NT_SUCCESS(Status)) {
            _SEH2_LEAVE;
        }

        ExReleaseResourceForThreadLite(
            &Vcb->MainResource,
            ExGetCurrentResourceThread() );
        VcbAcquired = FALSE;

        IoCopyCurrentIrpStackLocationToNext(IrpContext->Irp);

        KeInitializeEvent( &Event, NotificationEvent, FALSE );
        IoSetCompletionRoutine( IrpContext->Irp,
                                RfsdPnpCompletionRoutine,
                                &Event,
                                TRUE,
                                TRUE,
                                TRUE );

        RfsdPrint((DBG_PNP, "RfsdPnpQueryRemove: Call lower level driver...\n"));
        Status = IoCallDriver( Vcb->TargetDeviceObject, 
                                   IrpContext->Irp);

        if (Status == STATUS_PENDING) {

            KeWaitForSingleObject( &Event,
                                   Executive,
                                   KernelMode,
                                   FALSE,
                                   NULL );

            Status = IrpContext->Irp->IoStatus.Status;
        }

        if (NT_SUCCESS(Status)) {
            RfsdPrint((DBG_PNP, "RfsdPnpQueryRemove: RfsdCheckDismount ...\n"));
            bDeleted = RfsdCheckDismount(IrpContext, Vcb, TRUE);
            RfsdPrint((DBG_PNP, "RfsdPnpQueryRemove: RfsdFlushVolume bDelted=%xh ...\n", bDeleted));
        }

        ASSERT( !(NT_SUCCESS(Status) && !bDeleted));

    } _SEH2_FINALLY {

        if (VcbAcquired) {
            ExReleaseResourceForThreadLite(
                &Vcb->MainResource,
                ExGetCurrentResourceThread() );
        }

        RfsdCompleteRequest(
            IrpContext->Irp, FALSE, (CCHAR)(NT_SUCCESS(Status)?
            IO_DISK_INCREMENT : IO_NO_INCREMENT) );

        IrpContext->Irp = NULL;
    } _SEH2_END;
    
    return Status;
}

__drv_mustHoldCriticalRegion
NTSTATUS
RfsdPnpRemove (
    PRFSD_IRP_CONTEXT IrpContext,
    PRFSD_VCB         Vcb      )
{
    NTSTATUS Status;
    KEVENT   Event;
    BOOLEAN  bDeleted;

    PAGED_CODE();

    _SEH2_TRY {

        RfsdPrint((DBG_PNP, "RfsdPnpRemove by RfsdPnp ...\n"));
#if (_WIN32_WINNT >= 0x0500)
        CcWaitForCurrentLazyWriterActivity();
#endif
        ExAcquireResourceExclusiveLite(
            &Vcb->MainResource,  TRUE );

        Status = RfsdLockVcb(Vcb, IrpContext->FileObject);

        ExReleaseResourceForThreadLite(
            &Vcb->MainResource,
            ExGetCurrentResourceThread());

        //
        // Setup the Irp. We'll send it to the lower disk driver.
        //

        IoCopyCurrentIrpStackLocationToNext(IrpContext->Irp);

        KeInitializeEvent( &Event, NotificationEvent, FALSE );
        IoSetCompletionRoutine( IrpContext->Irp,
                                RfsdPnpCompletionRoutine,
                                &Event,
                                TRUE,
                                TRUE,
                                TRUE );

        Status = IoCallDriver( Vcb->TargetDeviceObject, 
                               IrpContext->Irp);

       if (Status == STATUS_PENDING) {

            KeWaitForSingleObject( &Event,
                                   Executive,
                                   KernelMode,
                                   FALSE,
                                   NULL );

            Status = IrpContext->Irp->IoStatus.Status;
       }

        ExAcquireResourceExclusiveLite(
            &Vcb->MainResource, TRUE );

        RfsdPurgeVolume(Vcb, FALSE);

        ExReleaseResourceForThreadLite(
            &Vcb->MainResource,
            ExGetCurrentResourceThread() );

        bDeleted = RfsdCheckDismount(IrpContext, Vcb, TRUE);

    } _SEH2_FINALLY {
        RfsdCompleteRequest(
            IrpContext->Irp, FALSE, (CCHAR)(NT_SUCCESS(Status)?
            IO_DISK_INCREMENT : IO_NO_INCREMENT) );

        IrpContext->Irp = NULL;
    } _SEH2_END;

    return Status;
}

__drv_mustHoldCriticalRegion
NTSTATUS
RfsdPnpSurpriseRemove (
    PRFSD_IRP_CONTEXT IrpContext,
    PRFSD_VCB         Vcb      )
{
    NTSTATUS Status;
    KEVENT   Event;
    BOOLEAN  bDeleted;

    PAGED_CODE();

    _SEH2_TRY {

        RfsdPrint((DBG_PNP, "RfsdPnpSupriseRemove by RfsdPnp ...\n"));
#if (_WIN32_WINNT >= 0x0500)
        CcWaitForCurrentLazyWriterActivity();
#endif
        ExAcquireResourceExclusiveLite(
            &Vcb->MainResource,  TRUE );

        Status = RfsdLockVcb(Vcb, IrpContext->FileObject);

        ExReleaseResourceForThreadLite(
            &Vcb->MainResource,
            ExGetCurrentResourceThread());

        //
        // Setup the Irp. We'll send it to the lower disk driver.
        //

        IoCopyCurrentIrpStackLocationToNext(IrpContext->Irp);

        KeInitializeEvent( &Event, NotificationEvent, FALSE );
        IoSetCompletionRoutine( IrpContext->Irp,
                                RfsdPnpCompletionRoutine,
                                &Event,
                                TRUE,
                                TRUE,
                                TRUE );

        Status = IoCallDriver( Vcb->TargetDeviceObject, 
                               IrpContext->Irp);

       if (Status == STATUS_PENDING) {

            KeWaitForSingleObject( &Event,
                                   Executive,
                                   KernelMode,
                                   FALSE,
                                   NULL );

            Status = IrpContext->Irp->IoStatus.Status;
       }

        ExAcquireResourceExclusiveLite(
            &Vcb->MainResource, TRUE );

        RfsdPurgeVolume(Vcb, FALSE);

        ExReleaseResourceForThreadLite(
            &Vcb->MainResource,
            ExGetCurrentResourceThread() );

        bDeleted = RfsdCheckDismount(IrpContext, Vcb, TRUE);

    } _SEH2_FINALLY {

        RfsdCompleteRequest(
            IrpContext->Irp, FALSE, (CCHAR)(NT_SUCCESS(Status)?
            IO_DISK_INCREMENT : IO_NO_INCREMENT) );

        IrpContext->Irp = NULL;
    } _SEH2_END;

    return Status;
}

__drv_mustHoldCriticalRegion
NTSTATUS
RfsdPnpCancelRemove (
    PRFSD_IRP_CONTEXT IrpContext,
    PRFSD_VCB         Vcb
    )
{
    NTSTATUS Status;

    PAGED_CODE();

    RfsdPrint((DBG_PNP, "RfsdPnpCancelRemove by RfsdPnp ...\n"));

    ExAcquireResourceExclusiveLite(
        &Vcb->MainResource,  TRUE );

    Status = RfsdUnlockVcb(Vcb, IrpContext->FileObject);

    ExReleaseResourceForThreadLite(
        &Vcb->MainResource,
        ExGetCurrentResourceThread());

    IoSkipCurrentIrpStackLocation(IrpContext->Irp);

    Status = IoCallDriver(Vcb->TargetDeviceObject, IrpContext->Irp);

    IrpContext->Irp = NULL;

    return Status;
}

#endif // (_WIN32_WINNT >= 0x0500)
