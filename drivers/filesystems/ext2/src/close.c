/*
 * COPYRIGHT:        See COPYRIGHT.TXT
 * PROJECT:          Ext2 File System Driver for WinNT/2K/XP
 * FILE:             close.c
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
#pragma alloc_text(PAGE, Ext2QueueCloseRequest)
#pragma alloc_text(PAGE, Ext2DeQueueCloseRequest)
#endif

NTSTATUS
Ext2Close (IN PEXT2_IRP_CONTEXT IrpContext)
{
    PDEVICE_OBJECT  DeviceObject;
    NTSTATUS        Status = STATUS_SUCCESS;
    PEXT2_VCB       Vcb = NULL;
    BOOLEAN         VcbResourceAcquired = FALSE;
    PFILE_OBJECT    FileObject;
    PEXT2_FCB       Fcb;
    BOOLEAN         FcbResourceAcquired = FALSE;
    PEXT2_CCB       Ccb;
    BOOLEAN         bDeleteVcb = FALSE;
    BOOLEAN         bBeingClosed = FALSE;
    BOOLEAN         bSkipLeave = FALSE;

    _SEH2_TRY {

        ASSERT(IrpContext != NULL);
        ASSERT((IrpContext->Identifier.Type == EXT2ICX) &&
               (IrpContext->Identifier.Size == sizeof(EXT2_IRP_CONTEXT)));

        DeviceObject = IrpContext->DeviceObject;
        if (IsExt2FsDevice(DeviceObject)) {
            Status = STATUS_SUCCESS;
            Vcb = NULL;
            _SEH2_LEAVE;
        }

        Vcb = (PEXT2_VCB) DeviceObject->DeviceExtension;
        ASSERT(Vcb != NULL);
        ASSERT((Vcb->Identifier.Type == EXT2VCB) &&
               (Vcb->Identifier.Size == sizeof(EXT2_VCB)));

        if (!ExAcquireResourceExclusiveLite(
                    &Vcb->MainResource,
                    IsFlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT) )) {
            DEBUG(DL_INF, ("Ext2Close: PENDING ... Vcb: %xh/%xh\n",
                           Vcb->OpenHandleCount, Vcb->ReferenceCount));

            Status = STATUS_PENDING;
            _SEH2_LEAVE;
        }
        VcbResourceAcquired = TRUE;

        bSkipLeave = TRUE;
        if (IsFlagOn(Vcb->Flags, VCB_BEING_CLOSED)) {
            bBeingClosed = TRUE;
        } else {
            SetLongFlag(Vcb->Flags, VCB_BEING_CLOSED);
            bBeingClosed = FALSE;
        }

        if (IsFlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_DELAY_CLOSE)) {

            FileObject = NULL;
            Fcb = IrpContext->Fcb;
            Ccb = IrpContext->Ccb;

        } else {

            FileObject = IrpContext->FileObject;
            Fcb = (PEXT2_FCB) FileObject->FsContext;
            if (!Fcb) {
                Status = STATUS_SUCCESS;
                _SEH2_LEAVE;
            }
            ASSERT(Fcb != NULL);
            Ccb = (PEXT2_CCB) FileObject->FsContext2;
        }

        DEBUG(DL_INF, ( "Ext2Close: (VCB) bBeingClosed = %d Vcb = %p ReferCount = %d\n",
                        bBeingClosed, Vcb, Vcb->ReferenceCount));

        if (Fcb->Identifier.Type == EXT2VCB) {

            if (Ccb) {

                Ext2DerefXcb(&Vcb->ReferenceCount);
                Ext2FreeCcb(Vcb, Ccb);

                if (FileObject) {
                    FileObject->FsContext2 = Ccb = NULL;
                }
            }

            Status = STATUS_SUCCESS;
            _SEH2_LEAVE;
        }

        if ( Fcb->Identifier.Type != EXT2FCB ||
                Fcb->Identifier.Size != sizeof(EXT2_FCB)) {
            _SEH2_LEAVE;
        }

        if (!ExAcquireResourceExclusiveLite(
                    &Fcb->MainResource,
                    IsFlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT) )) {
            Status = STATUS_PENDING;
            _SEH2_LEAVE;
        }
        FcbResourceAcquired = TRUE;

        Fcb->Header.IsFastIoPossible = FastIoIsNotPossible;

        if (!Ccb) {
            Status = STATUS_SUCCESS;
            _SEH2_LEAVE;
        }

        ASSERT((Ccb->Identifier.Type == EXT2CCB) &&
               (Ccb->Identifier.Size == sizeof(EXT2_CCB)));

        if (IsFlagOn(Fcb->Flags, FCB_STATE_BUSY)) {
            SetFlag(IrpContext->Flags, IRP_CONTEXT_FLAG_FILE_BUSY);
            DEBUG(DL_WRN, ( "Ext2Close: busy bit set: %wZ\n", &Fcb->Mcb->FullName ));
            Status = STATUS_PENDING;
            _SEH2_LEAVE;
        }

        DEBUG(DL_INF, ( "Ext2Close: Fcb = %p OpenHandleCount= %u ReferenceCount=%u NonCachedCount=%u %wZ\n",
                        Fcb, Fcb->OpenHandleCount, Fcb->ReferenceCount, Fcb->NonCachedOpenCount, &Fcb->Mcb->FullName ));

        if (Ccb) {

            Ext2FreeCcb(Vcb, Ccb);

            if (FileObject) {
                FileObject->FsContext2 = Ccb = NULL;
            }
        }

        if (0 == Ext2DerefXcb(&Fcb->ReferenceCount)) {

            //
            // Remove Fcb from Vcb->FcbList ...
            //

            if (FcbResourceAcquired) {
                ExReleaseResourceLite(&Fcb->MainResource);
                FcbResourceAcquired = FALSE;
            }

            Ext2FreeFcb(Fcb);

            if (FileObject) {
                FileObject->FsContext = Fcb = NULL;
            }
        }

        Ext2DerefXcb(&Vcb->ReferenceCount);
        Status = STATUS_SUCCESS;

    } _SEH2_FINALLY {

        if (NT_SUCCESS(Status) && Vcb != NULL && IsVcbInited(Vcb)) {
            /* for Ext2Fsd driver open/close, Vcb is NULL */
            if ((!bBeingClosed) && (Vcb->ReferenceCount == 0)&&
                (!IsMounted(Vcb) || IsDispending(Vcb))) {
                bDeleteVcb = TRUE;
            }
        }

        if (bSkipLeave && !bBeingClosed) {
            ClearFlag(Vcb->Flags, VCB_BEING_CLOSED);
        }

        if (FcbResourceAcquired) {
            ExReleaseResourceLite(&Fcb->MainResource);
        }

        if (VcbResourceAcquired) {
            ExReleaseResourceLite(&Vcb->MainResource);
        }

        if (!IrpContext->ExceptionInProgress) {

            if (Status == STATUS_PENDING) {

                Ext2QueueCloseRequest(IrpContext);

            } else {

                Ext2CompleteIrpContext(IrpContext, Status);

                if (bDeleteVcb) {

                    PVPB Vpb = Vcb->Vpb;
                    DEBUG(DL_DBG, ( "Ext2Close: Try to free Vcb %p and Vpb %p\n",
                                    Vcb, Vpb));

                    Ext2CheckDismount(IrpContext, Vcb, FALSE);
                }
            }
        }
    } _SEH2_END;

    return Status;
}

VOID
Ext2QueueCloseRequest (IN PEXT2_IRP_CONTEXT IrpContext)
{
    ASSERT(IrpContext);
    ASSERT((IrpContext->Identifier.Type == EXT2ICX) &&
           (IrpContext->Identifier.Size == sizeof(EXT2_IRP_CONTEXT)));

    if (IsFlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_DELAY_CLOSE)) {

        if (IsFlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_FILE_BUSY)) {
            Ext2Sleep(500); /* 0.5 sec*/
        } else {
            Ext2Sleep(50);  /* 0.05 sec*/
        }

    } else {

        SetFlag(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT);
        SetFlag(IrpContext->Flags, IRP_CONTEXT_FLAG_DELAY_CLOSE);

        IrpContext->Fcb = (PEXT2_FCB) IrpContext->FileObject->FsContext;
        IrpContext->Ccb = (PEXT2_CCB) IrpContext->FileObject->FsContext2;
    }

    ExInitializeWorkItem(
        &IrpContext->WorkQueueItem,
        Ext2DeQueueCloseRequest,
        IrpContext);

    ExQueueWorkItem(&IrpContext->WorkQueueItem, DelayedWorkQueue);
}

VOID NTAPI
Ext2DeQueueCloseRequest (IN PVOID Context)
{
    PEXT2_IRP_CONTEXT IrpContext;

    IrpContext = (PEXT2_IRP_CONTEXT) Context;
    ASSERT(IrpContext);
    ASSERT((IrpContext->Identifier.Type == EXT2ICX) &&
           (IrpContext->Identifier.Size == sizeof(EXT2_IRP_CONTEXT)));

    _SEH2_TRY {

        _SEH2_TRY {

            FsRtlEnterFileSystem();
            Ext2Close(IrpContext);

        } _SEH2_EXCEPT (Ext2ExceptionFilter(IrpContext, _SEH2_GetExceptionInformation())) {

            Ext2ExceptionHandler(IrpContext);
        } _SEH2_END;

    } _SEH2_FINALLY {

        FsRtlExitFileSystem();
    } _SEH2_END;
}
