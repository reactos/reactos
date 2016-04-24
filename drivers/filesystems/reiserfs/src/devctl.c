/*
 * COPYRIGHT:        GNU GENERAL PUBLIC LICENSE VERSION 2
 * PROJECT:          ReiserFs file system driver for Windows NT/2000/XP/Vista.
 * FILE:             devctl.c
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

#ifdef _PREFAST_
IO_COMPLETION_ROUTINE RfsdDeviceControlCompletion;
#endif // _PREFAST_

NTSTATUS NTAPI
RfsdDeviceControlCompletion (IN PDEVICE_OBJECT   DeviceObject,
                IN PIRP             Irp,
                IN PVOID            Context);

#ifdef ALLOC_PRAGMA
//#pragma alloc_text(PAGE, RfsdDeviceControlCompletion)
#pragma alloc_text(PAGE, RfsdDeviceControl)
#pragma alloc_text(PAGE, RfsdDeviceControlNormal)
#if RFSD_UNLOAD
#pragma alloc_text(PAGE, RfsdPrepareToUnload)
#endif
#endif

NTSTATUS NTAPI
RfsdDeviceControlCompletion (IN PDEVICE_OBJECT   DeviceObject,
                 IN PIRP             Irp,
                 IN PVOID            Context)
{
    if (Irp->PendingReturned) {
        IoMarkIrpPending(Irp);
    }

    return STATUS_SUCCESS;
}

NTSTATUS
RfsdDeviceControlNormal (IN PRFSD_IRP_CONTEXT IrpContext)
{
    PDEVICE_OBJECT  DeviceObject;
    BOOLEAN         CompleteRequest = TRUE;
    NTSTATUS        Status = STATUS_UNSUCCESSFUL;

    PRFSD_VCB       Vcb;

    PIRP            Irp;
    PIO_STACK_LOCATION IrpSp;
    PIO_STACK_LOCATION NextIrpSp;

    PDEVICE_OBJECT  TargetDeviceObject;

    PAGED_CODE();

    _SEH2_TRY {

        ASSERT(IrpContext != NULL);
        
        ASSERT((IrpContext->Identifier.Type == RFSDICX) &&
            (IrpContext->Identifier.Size == sizeof(RFSD_IRP_CONTEXT)));
        
        CompleteRequest = TRUE;

        DeviceObject = IrpContext->DeviceObject;
    
        if (DeviceObject == RfsdGlobal->DeviceObject)  {
            Status = STATUS_INVALID_DEVICE_REQUEST;
            _SEH2_LEAVE;
        }
        
        Irp = IrpContext->Irp;
        IrpSp = IoGetCurrentIrpStackLocation(Irp);

        Vcb = (PRFSD_VCB) IrpSp->FileObject->FsContext;

        if (!((Vcb) && (Vcb->Identifier.Type == RFSDVCB) &&
              (Vcb->Identifier.Size == sizeof(RFSD_VCB)))) {
            Status = STATUS_INVALID_PARAMETER;
            _SEH2_LEAVE;
        }
        
        TargetDeviceObject = Vcb->TargetDeviceObject;
        
        //
        // Pass on the IOCTL to the driver below
        //
        
        CompleteRequest = FALSE;

        NextIrpSp = IoGetNextIrpStackLocation( Irp );
        *NextIrpSp = *IrpSp;

        IoSetCompletionRoutine(
            Irp,
            RfsdDeviceControlCompletion,
            NULL,
            FALSE,
            TRUE,
            TRUE );
        
        Status = IoCallDriver(TargetDeviceObject, Irp);

    } _SEH2_FINALLY  {

        if (!IrpContext->ExceptionInProgress) {
            if (IrpContext) {
                if (!CompleteRequest) {
                    IrpContext->Irp = NULL;
                }

                RfsdCompleteIrpContext(IrpContext, Status);
            }
        }
    } _SEH2_END;
    
    return Status;
}

#if RFSD_UNLOAD

__drv_mustHoldCriticalRegion
NTSTATUS
RfsdPrepareToUnload (IN PRFSD_IRP_CONTEXT IrpContext)
{
    PDEVICE_OBJECT  DeviceObject;
    NTSTATUS        Status = STATUS_UNSUCCESSFUL;
    BOOLEAN         GlobalDataResourceAcquired = FALSE;

    PAGED_CODE();

    _SEH2_TRY {

        ASSERT(IrpContext != NULL);
        
        ASSERT((IrpContext->Identifier.Type == RFSDICX) &&
            (IrpContext->Identifier.Size == sizeof(RFSD_IRP_CONTEXT)));
        
        DeviceObject = IrpContext->DeviceObject;
        
        if (DeviceObject != RfsdGlobal->DeviceObject) {
            Status = STATUS_INVALID_DEVICE_REQUEST;
            _SEH2_LEAVE;
        }
        
        ExAcquireResourceExclusiveLite(
            &RfsdGlobal->Resource,
            TRUE );
        
        GlobalDataResourceAcquired = TRUE;
        
        if (FlagOn(RfsdGlobal->Flags, RFSD_UNLOAD_PENDING)) {
            RfsdPrint((DBG_ERROR, "RfsdPrepareUnload:  Already ready to unload.\n"));
            
            Status = STATUS_ACCESS_DENIED;
            
            _SEH2_LEAVE;
        }
        
        {
            PRFSD_VCB               Vcb;
            PLIST_ENTRY             ListEntry;

            ListEntry = RfsdGlobal->VcbList.Flink;

            while (ListEntry != &(RfsdGlobal->VcbList)) {

                Vcb = CONTAINING_RECORD(ListEntry, RFSD_VCB, Next);
                ListEntry = ListEntry->Flink;

                if (Vcb && (!Vcb->ReferenceCount) &&
                    IsFlagOn(Vcb->Flags, VCB_DISMOUNT_PENDING)) {
                    RfsdRemoveVcb(Vcb);
                    RfsdClearVpbFlag(Vcb->Vpb, VPB_MOUNTED);

                    RfsdFreeVcb(Vcb);
                }
            }
        }

        if (!IsListEmpty(&(RfsdGlobal->VcbList))) {

            RfsdPrint((DBG_ERROR, "RfsdPrepareUnload:  Mounted volumes exists.\n"));
            
            Status = STATUS_ACCESS_DENIED;
            
            _SEH2_LEAVE;
        }
        
        IoUnregisterFileSystem(RfsdGlobal->DeviceObject);

#ifdef _MSC_VER
#pragma prefast( suppress: 28175, "allowed to unload" )
#endif
        RfsdGlobal->DriverObject->DriverUnload = DriverUnload;
        
        SetFlag(RfsdGlobal->Flags ,RFSD_UNLOAD_PENDING);
        
        RfsdPrint((DBG_INFO, "RfsdPrepareToUnload: Driver is ready to unload.\n"));
        
        Status = STATUS_SUCCESS;

    } _SEH2_FINALLY {

        if (GlobalDataResourceAcquired) {
            ExReleaseResourceForThreadLite(
                &RfsdGlobal->Resource,
                ExGetCurrentResourceThread()
                );
        }
        
        if (!IrpContext->ExceptionInProgress) {
            RfsdCompleteIrpContext(IrpContext, Status);
        }
    } _SEH2_END;
    
    return Status;
}

#endif

__drv_mustHoldCriticalRegion
NTSTATUS
RfsdDeviceControl (IN PRFSD_IRP_CONTEXT IrpContext)
{
    PIRP                Irp;
    PIO_STACK_LOCATION  IoStackLocation;
    ULONG               IoControlCode;
    NTSTATUS            Status;

    PAGED_CODE();

    ASSERT(IrpContext);
    
    ASSERT((IrpContext->Identifier.Type == RFSDICX) &&
        (IrpContext->Identifier.Size == sizeof(RFSD_IRP_CONTEXT)));
    
    Irp = IrpContext->Irp;
    
    IoStackLocation = IoGetCurrentIrpStackLocation(Irp);
    
    IoControlCode =
        IoStackLocation->Parameters.DeviceIoControl.IoControlCode;
    
    switch (IoControlCode) {

#if RFSD_UNLOAD
    case IOCTL_PREPARE_TO_UNLOAD:
        Status = RfsdPrepareToUnload(IrpContext);
        break;
#endif      
    default:
        Status = RfsdDeviceControlNormal(IrpContext);
    }
    
    return Status;
}
