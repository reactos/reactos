/*
 * COPYRIGHT:        GNU GENERAL PUBLIC LICENSE VERSION 2
 * PROJECT:          ReiserFs file system driver for Windows NT/2000/XP/Vista.
 * FILE:             lockctl.c
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
#pragma alloc_text(PAGE, RfsdLockControl)
#endif

NTSTATUS
RfsdLockControl (IN PRFSD_IRP_CONTEXT IrpContext)
{
    PDEVICE_OBJECT  DeviceObject;
    BOOLEAN         CompleteRequest;
    NTSTATUS        Status = STATUS_UNSUCCESSFUL;
    PFILE_OBJECT    FileObject;
    PRFSD_FCB       Fcb;
    PIRP            Irp;

    PAGED_CODE();

    _SEH2_TRY {

        ASSERT(IrpContext != NULL);
        
        ASSERT((IrpContext->Identifier.Type == RFSDICX) &&
            (IrpContext->Identifier.Size == sizeof(RFSD_IRP_CONTEXT)));
        
        DeviceObject = IrpContext->DeviceObject;
        
        if (DeviceObject == RfsdGlobal->DeviceObject) {

            CompleteRequest = TRUE;
            Status = STATUS_INVALID_DEVICE_REQUEST;
            _SEH2_LEAVE;
        }
        
        FileObject = IrpContext->FileObject;
        
        Fcb = (PRFSD_FCB) FileObject->FsContext;
        
        ASSERT(Fcb != NULL);
        
        if (Fcb->Identifier.Type == RFSDVCB) {

            CompleteRequest = TRUE;
            Status = STATUS_INVALID_PARAMETER;
            _SEH2_LEAVE;
        }
        
        ASSERT((Fcb->Identifier.Type == RFSDFCB) &&
            (Fcb->Identifier.Size == sizeof(RFSD_FCB)));
        
        if (FlagOn(Fcb->RfsdMcb->FileAttr, FILE_ATTRIBUTE_DIRECTORY)) {

            CompleteRequest = TRUE;
            Status = STATUS_INVALID_PARAMETER;
            _SEH2_LEAVE;
        }
        
        Irp = IrpContext->Irp;
        
        //
        // While the file has any byte range locks we set IsFastIoPossible to
        // FastIoIsQuestionable so that the FastIoCheckIfPossible function is
        // called to check the locks for any fast I/O read/write operation.
        //
        if (Fcb->Header.IsFastIoPossible != FastIoIsQuestionable) {

            RfsdPrint((DBG_INFO,
                "RfsdLockControl: %-16.16s %-31s %s\n",
                RfsdGetCurrentProcessName(),
                "FastIoIsQuestionable",
                Fcb->AnsiFileName.Buffer
                ));

            Fcb->Header.IsFastIoPossible = FastIoIsQuestionable;
        }
        
        //
        // FsRtlProcessFileLock acquires FileObject->FsContext->Resource while
        // modifying the file locks and calls IoCompleteRequest when it's done.
        //
        
        CompleteRequest = FALSE;
        
        Status = FsRtlProcessFileLock(
            &Fcb->FileLockAnchor,
            Irp,
            NULL        );
        
        if (!NT_SUCCESS(Status)) {
            RfsdPrint((DBG_ERROR, 
                "RfsdLockControl: %-16.16s %-31s *** Status: %s (%#x) ***\n",
                RfsdGetCurrentProcessName(),
                "IRP_MJ_LOCK_CONTROL",
                RfsdNtStatusToString(Status),
                Status          ));
        }

    } _SEH2_FINALLY {

        if (!IrpContext->ExceptionInProgress) {
            if (!CompleteRequest) {
                IrpContext->Irp = NULL;
            }
            
            RfsdCompleteIrpContext(IrpContext, Status);
        }
    } _SEH2_END;
    
    return Status;
}
