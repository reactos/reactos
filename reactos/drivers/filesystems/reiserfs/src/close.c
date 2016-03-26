/*
 * COPYRIGHT:        GNU GENERAL PUBLIC LICENSE VERSION 2
 * PROJECT:          ReiserFs file system driver for Windows NT/2000/XP/Vista.
 * FILE:             close.c
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

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, RfsdClose)
#pragma alloc_text(PAGE, RfsdQueueCloseRequest)
#pragma alloc_text(PAGE, RfsdDeQueueCloseRequest)
#endif

__drv_mustHoldCriticalRegion
NTSTATUS
RfsdClose (IN PRFSD_IRP_CONTEXT IrpContext)
{
    PDEVICE_OBJECT  DeviceObject;
    NTSTATUS        Status = STATUS_SUCCESS;
    PRFSD_VCB       Vcb = 0;
    BOOLEAN         VcbResourceAcquired = FALSE;
    PFILE_OBJECT    FileObject;
    PRFSD_FCB       Fcb = 0;
    BOOLEAN         FcbResourceAcquired = FALSE;
    PRFSD_CCB       Ccb;
    BOOLEAN         FreeVcb = FALSE;

    PAGED_CODE();

    _SEH2_TRY
    {
        ASSERT(IrpContext != NULL);
        
        ASSERT((IrpContext->Identifier.Type == RFSDICX) &&
            (IrpContext->Identifier.Size == sizeof(RFSD_IRP_CONTEXT)));
        
        DeviceObject = IrpContext->DeviceObject;

        if (DeviceObject == RfsdGlobal->DeviceObject)
        {
            Status = STATUS_SUCCESS;
            _SEH2_LEAVE;
        }
        
        Vcb = (PRFSD_VCB) DeviceObject->DeviceExtension;
        
        ASSERT(Vcb != NULL);
        
        ASSERT((Vcb->Identifier.Type == RFSDVCB) &&
            (Vcb->Identifier.Size == sizeof(RFSD_VCB)));

        ASSERT(IsMounted(Vcb));

#ifdef _MSC_VER
#pragma prefast( suppress: 28137, "by design" )
#endif
        if (!ExAcquireResourceExclusiveLite(
            &Vcb->MainResource,
            IrpContext->IsSynchronous )) {
            RfsdPrint((DBG_INFO, "RfsdClose: PENDING ... Vcb: %xh/%xh\n",
                                 Vcb->OpenFileHandleCount, Vcb->ReferenceCount));

            Status = STATUS_PENDING;
            _SEH2_LEAVE;
        }
        
        VcbResourceAcquired = TRUE;

        FileObject = IrpContext->FileObject;

        if (IsFlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_DELAY_CLOSE)) {
            Fcb = IrpContext->Fcb;
            Ccb = IrpContext->Ccb;
        } else {
            Fcb = (PRFSD_FCB) FileObject->FsContext;
        
            if (!Fcb)
            {
                Status = STATUS_SUCCESS;
                _SEH2_LEAVE;
            }

            ASSERT(Fcb != NULL);

            Ccb = (PRFSD_CCB) FileObject->FsContext2;
        }
        
        if (Fcb->Identifier.Type == RFSDVCB) {

            Vcb->ReferenceCount--;

            if (!Vcb->ReferenceCount && FlagOn(Vcb->Flags, VCB_DISMOUNT_PENDING))
            {
                FreeVcb = TRUE;
            }

            if (Ccb) {
                RfsdFreeCcb(Ccb);

                if (FileObject) {
                    FileObject->FsContext2 = Ccb = NULL;
                }
            }
            
            Status = STATUS_SUCCESS;
            
            _SEH2_LEAVE;
        }
        
        if (Fcb->Identifier.Type != RFSDFCB || Fcb->Identifier.Size != sizeof(RFSD_FCB)) {

#if DBG
            RfsdPrint((DBG_ERROR, "RfsdClose: Strange IRP_MJ_CLOSE by system!\n"));
            ExAcquireResourceExclusiveLite(
                &RfsdGlobal->CountResource,
                TRUE );

            RfsdGlobal->IRPCloseCount++;

            ExReleaseResourceForThreadLite(
                &RfsdGlobal->CountResource,
                ExGetCurrentResourceThread() );
#endif
            _SEH2_LEAVE;
        }

        ASSERT((Fcb->Identifier.Type == RFSDFCB) &&
            (Fcb->Identifier.Size == sizeof(RFSD_FCB)));

/*        
        if ( (!IsFlagOn(Vcb->Flags, VCB_READ_ONLY)) && 
             (!IsFlagOn(Fcb->Flags, FCB_PAGE_FILE))  )
*/
        {
#ifdef _MSC_VER
#pragma prefast( suppress: 28137, "by design" )
#endif
            if (!ExAcquireResourceExclusiveLite(
                &Fcb->MainResource,
                IrpContext->IsSynchronous )) {
                Status = STATUS_PENDING;
                _SEH2_LEAVE;
            }
            
            FcbResourceAcquired = TRUE;
        }
        
        if (!Ccb) {
            Status = STATUS_SUCCESS;
            _SEH2_LEAVE;
        }

        ASSERT((Ccb->Identifier.Type == RFSDCCB) &&
            (Ccb->Identifier.Size == sizeof(RFSD_CCB)));
        
        Fcb->ReferenceCount--;
        Vcb->ReferenceCount--;
        
        if (!Vcb->ReferenceCount && IsFlagOn(Vcb->Flags, VCB_DISMOUNT_PENDING)) {
            FreeVcb = TRUE;
        }

        RfsdPrint((DBG_INFO, "RfsdClose: OpenHandleCount: %u ReferenceCount: %u %s\n",
            Fcb->OpenHandleCount, Fcb->ReferenceCount, Fcb->AnsiFileName.Buffer ));

        if (Ccb) {

            RfsdFreeCcb(Ccb);

            if (FileObject) {
                FileObject->FsContext2 = Ccb = NULL;
            }
        }

        if (!Fcb->ReferenceCount)  {
            //
            // Remove Fcb from Vcb->FcbList ...
            //

            RemoveEntryList(&Fcb->Next);

            RfsdFreeFcb(Fcb);

            FcbResourceAcquired = FALSE;
        }
        
        Status = STATUS_SUCCESS;

    } _SEH2_FINALLY {

        if (FcbResourceAcquired) {
            ExReleaseResourceForThreadLite(
                &Fcb->MainResource,
                ExGetCurrentResourceThread() );
        }

        if (VcbResourceAcquired) {
            ExReleaseResourceForThreadLite(
                &Vcb->MainResource,
                ExGetCurrentResourceThread()  );
        }
        
        if (!IrpContext->ExceptionInProgress) {
            if (Status == STATUS_PENDING) {

                RfsdQueueCloseRequest(IrpContext);
/*                

                Status = STATUS_SUCCESS;

                if (IrpContext->Irp != NULL)
                {
                    IrpContext->Irp->IoStatus.Status = Status;
                    
                    RfsdCompleteRequest(
                        IrpContext->Irp,
                        !IsFlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_REQUEUED),
                        (CCHAR)
                        (NT_SUCCESS(Status) ? IO_DISK_INCREMENT : IO_NO_INCREMENT)
                        );
                    
                    IrpContext->Irp = NULL;
                }
*/
            } else {

                RfsdCompleteIrpContext(IrpContext, Status);

                if (FreeVcb) {

                    ExAcquireResourceExclusiveLite(
                        &RfsdGlobal->Resource, TRUE );

                    RfsdClearVpbFlag(Vcb->Vpb, VPB_MOUNTED);

                    RfsdRemoveVcb(Vcb);

                    ExReleaseResourceForThreadLite(
                        &RfsdGlobal->Resource,
                        ExGetCurrentResourceThread() );

                    RfsdFreeVcb(Vcb);
                }
            }
        }
    } _SEH2_END;
    
    return Status;
}

VOID
RfsdQueueCloseRequest (IN PRFSD_IRP_CONTEXT IrpContext)
{
    PAGED_CODE();

    ASSERT(IrpContext);
    
    ASSERT((IrpContext->Identifier.Type == RFSDICX) &&
        (IrpContext->Identifier.Size == sizeof(RFSD_IRP_CONTEXT)));
    
    if (!IsFlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_DELAY_CLOSE)) {
        SetFlag(IrpContext->Flags, IRP_CONTEXT_FLAG_DELAY_CLOSE);

        IrpContext->Fcb = (PRFSD_FCB) IrpContext->FileObject->FsContext;
        IrpContext->Ccb = (PRFSD_CCB) IrpContext->FileObject->FsContext2;

        IrpContext->FileObject = NULL;
    }

    // IsSynchronous means we can block (so we don't requeue it)
    IrpContext->IsSynchronous = TRUE;
    
    ExInitializeWorkItem(
        &IrpContext->WorkQueueItem,
        RfsdDeQueueCloseRequest,
        IrpContext);
    
    ExQueueWorkItem(&IrpContext->WorkQueueItem, CriticalWorkQueue);
}

VOID NTAPI
RfsdDeQueueCloseRequest (IN PVOID Context)
{
    PRFSD_IRP_CONTEXT IrpContext;

    PAGED_CODE();

    IrpContext = (PRFSD_IRP_CONTEXT) Context;
    
    ASSERT(IrpContext);
    
    ASSERT((IrpContext->Identifier.Type == RFSDICX) &&
        (IrpContext->Identifier.Size == sizeof(RFSD_IRP_CONTEXT)));
    
    _SEH2_TRY {

        _SEH2_TRY {

            FsRtlEnterFileSystem();
            RfsdClose(IrpContext);

        } _SEH2_EXCEPT (RfsdExceptionFilter(IrpContext, _SEH2_GetExceptionInformation())) {

            RfsdExceptionHandler(IrpContext);
        } _SEH2_END;

    } _SEH2_FINALLY {

        FsRtlExitFileSystem();
    } _SEH2_END;
}
