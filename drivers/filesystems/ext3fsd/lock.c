/*
 * COPYRIGHT:        See COPYRIGHT.TXT
 * PROJECT:          Ext2 File System Driver for WinNT/2K/XP
 * FILE:             lock.c
 * PROGRAMMER:       Matt Wu <mattwu@163.com>
 * HOMEPAGE:         http://ext2.yeah.net
 * UPDATE HISTORY: 
 */

/* INCLUDES *****************************************************************/

#include "ext2fs.h"

/* GLOBALS ***************************************************************/

extern PEXT2_GLOBAL Ext2Global;

/* DEFINITIONS *************************************************************/

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, Ext2LockControl)
#endif

NTSTATUS
Ext2LockControl (IN PEXT2_IRP_CONTEXT IrpContext)
{
    PDEVICE_OBJECT  DeviceObject;
    BOOLEAN         CompleteContext = TRUE;
    BOOLEAN         CompleteIrp = TRUE;
    NTSTATUS        Status = STATUS_UNSUCCESSFUL;
    PFILE_OBJECT    FileObject;
    PEXT2_FCB       Fcb;
    PIRP            Irp;
    
    __try {

        ASSERT(IrpContext != NULL);
        
        ASSERT((IrpContext->Identifier.Type == EXT2ICX) &&
            (IrpContext->Identifier.Size == sizeof(EXT2_IRP_CONTEXT)));
        
        DeviceObject = IrpContext->DeviceObject;
        
        if (IsExt2FsDevice(DeviceObject)) {
            Status = STATUS_INVALID_DEVICE_REQUEST;
            __leave;
        }
        
        FileObject = IrpContext->FileObject;
        
        Fcb = (PEXT2_FCB) FileObject->FsContext;
        
        ASSERT(Fcb != NULL);
        
        if (Fcb->Identifier.Type == EXT2VCB) {
            Status = STATUS_INVALID_PARAMETER;
            __leave;
        }
        
        ASSERT((Fcb->Identifier.Type == EXT2FCB) &&
            (Fcb->Identifier.Size == sizeof(EXT2_FCB)));
        
        if (FlagOn(Fcb->Mcb->FileAttr, FILE_ATTRIBUTE_DIRECTORY)) {
            Status = STATUS_INVALID_PARAMETER;
            __leave;
        }
        
        Irp = IrpContext->Irp;

        CompleteIrp = FALSE;

        Status = FsRtlCheckOplock( &Fcb->Oplock,
                                   Irp,
                                   IrpContext,
                                   Ext2OplockComplete,
                                   NULL );

        if (Status != STATUS_SUCCESS) {
            CompleteContext = FALSE;
            __leave;
        }
        
        //
        // FsRtlProcessFileLock acquires FileObject->FsContext->Resource while
        // modifying the file locks and calls IoCompleteRequest when it's done.
        //
        
        Status = FsRtlProcessFileLock(
            &Fcb->FileLockAnchor,
            Irp,
            NULL );
#if EXT2_DEBUG        
        if (!NT_SUCCESS(Status)) {
            DEBUG(DL_ERR, ( 
                "Ext2LockControl: %-16.16s %-31s Status: %#x ***\n",
                Ext2GetCurrentProcessName(),
                "IRP_MJ_LOCK_CONTROL",
                Status          ));
        }
#endif
        Fcb->Header.IsFastIoPossible = Ext2IsFastIoPossible(Fcb);

    } __finally {

        if (!IrpContext->ExceptionInProgress) {

            if (!CompleteIrp) {
                IrpContext->Irp = NULL;
            }

            if (CompleteContext) {
                Ext2CompleteIrpContext(IrpContext, Status);
            }
        }
    }
    
    return Status;
}
