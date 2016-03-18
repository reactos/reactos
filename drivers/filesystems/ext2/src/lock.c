/*
 * COPYRIGHT:        See COPYRIGHT.TXT
 * PROJECT:          Ext2 File System Driver for WinNT/2K/XP
 * FILE:             lock.c
 * PROGRAMMER:       Matt Wu <mattwu@163.com>
 * HOMEPAGE:         http://www.ext2fsd.com
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
    PFILE_OBJECT    FileObject;
    PEXT2_FCB       Fcb;
    PIRP            Irp;

    NTSTATUS        Status = STATUS_UNSUCCESSFUL;
    BOOLEAN         CompleteContext = TRUE;
    BOOLEAN         CompleteIrp = TRUE;
    BOOLEAN         bFcbAcquired = FALSE;

    _SEH2_TRY {

        ASSERT(IrpContext != NULL);

        ASSERT((IrpContext->Identifier.Type == EXT2ICX) &&
               (IrpContext->Identifier.Size == sizeof(EXT2_IRP_CONTEXT)));

        DeviceObject = IrpContext->DeviceObject;

        if (IsExt2FsDevice(DeviceObject)) {
            Status = STATUS_INVALID_DEVICE_REQUEST;
            _SEH2_LEAVE;
        }

        FileObject = IrpContext->FileObject;

        Fcb = (PEXT2_FCB) FileObject->FsContext;
        ASSERT(Fcb != NULL);
        if (Fcb->Identifier.Type == EXT2VCB) {
            Status = STATUS_INVALID_PARAMETER;
            _SEH2_LEAVE;
        }

        ASSERT((Fcb->Identifier.Type == EXT2FCB) &&
               (Fcb->Identifier.Size == sizeof(EXT2_FCB)));

        if (FlagOn(Fcb->Mcb->FileAttr, FILE_ATTRIBUTE_DIRECTORY)) {
            Status = STATUS_INVALID_PARAMETER;
            _SEH2_LEAVE;
        }

        ExAcquireResourceSharedLite(&Fcb->MainResource, TRUE);
        bFcbAcquired = TRUE;

        Irp = IrpContext->Irp;

        CompleteIrp = FALSE;

        Status = FsRtlCheckOplock( &Fcb->Oplock,
                                   Irp,
                                   IrpContext,
                                   Ext2OplockComplete,
                                   NULL );

        if (Status != STATUS_SUCCESS) {
            CompleteContext = FALSE;
            _SEH2_LEAVE;
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

    } _SEH2_FINALLY {

        if (bFcbAcquired) {
            ExReleaseResourceLite(&Fcb->MainResource);
        }

        if (!IrpContext->ExceptionInProgress) {

            if (!CompleteIrp) {
                IrpContext->Irp = NULL;
            }

            if (CompleteContext) {
                Ext2CompleteIrpContext(IrpContext, Status);
            }
        }
    } _SEH2_END;

    return Status;
}
