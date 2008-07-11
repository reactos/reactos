/*
 * COPYRIGHT:        See COPYRIGHT.TXT
 * PROJECT:          Ext2 File System Driver for WinNT/2K/XP
 * FILE:             close.c
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
#pragma alloc_text(PAGE, Ext2Close)
#pragma alloc_text(PAGE, Ext2QueueCloseRequest)
#pragma alloc_text(PAGE, Ext2DeQueueCloseRequest)
#endif

NTSTATUS
Ext2Close (IN PEXT2_IRP_CONTEXT IrpContext)
{
    PDEVICE_OBJECT  DeviceObject;
    NTSTATUS        Status = STATUS_SUCCESS;
    PEXT2_VCB       Vcb;
    BOOLEAN         VcbResourceAcquired = FALSE;
    PFILE_OBJECT    FileObject;
    PEXT2_FCB       Fcb;
    BOOLEAN         FcbResourceAcquired = FALSE;
    PEXT2_CCB       Ccb;
    BOOLEAN         bDeleteVcb = FALSE;
    BOOLEAN         bBeingClosed = FALSE;
    BOOLEAN         bSkipLeave = FALSE;

    __try {

        ASSERT(IrpContext != NULL);
        ASSERT((IrpContext->Identifier.Type == EXT2ICX) &&
            (IrpContext->Identifier.Size == sizeof(EXT2_IRP_CONTEXT)));
        
        DeviceObject = IrpContext->DeviceObject;
        if (IsExt2FsDevice(DeviceObject)) {
            Status = STATUS_SUCCESS;
            __leave;
        }
        
        Vcb = (PEXT2_VCB) DeviceObject->DeviceExtension;
        ASSERT(Vcb != NULL);
        ASSERT((Vcb->Identifier.Type == EXT2VCB) &&
            (Vcb->Identifier.Size == sizeof(EXT2_VCB)));

        if (!ExAcquireResourceExclusiveLite(
            &Vcb->MainResource,
            IsFlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT) )) {
            DEBUG(DL_INF, ("Ext2Close: PENDING ... Vcb: %xh/%xh\n",
                                 Vcb->OpenFileHandleCount, Vcb->ReferenceCount));

            Status = STATUS_PENDING;
            __leave;
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
                __leave;
            }
            ASSERT(Fcb != NULL);
            Ccb = (PEXT2_CCB) FileObject->FsContext2;
        }
        

        DEBUG(DL_DBG, ( "Ext2Close: (VCB) bBeingClosed = %d Vcb = %p ReferCount = %d\n",
                         bBeingClosed, Vcb, Vcb->ReferenceCount));

        if (Fcb->Identifier.Type == EXT2VCB) {

            if ((!bBeingClosed) && (Vcb->ReferenceCount <= 1)&& 
                (!IsMounted(Vcb) || IsDispending(Vcb))) {
                bDeleteVcb = TRUE;
            }

            if (Ccb) {

                Ext2DerefXcb(&Vcb->ReferenceCount);
                Ext2FreeCcb(Ccb);

                if (FileObject) {
                    FileObject->FsContext2 = Ccb = NULL;
                }
            }

            Status = STATUS_SUCCESS;
            __leave;
        }
        
        if ( Fcb->Identifier.Type != EXT2FCB ||
             Fcb->Identifier.Size != sizeof(EXT2_FCB)) {
            __leave;
        }

        if (!ExAcquireResourceExclusiveLite(
            &Fcb->MainResource,
            IsFlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT) )) {
            Status = STATUS_PENDING;
            __leave;
        }
        FcbResourceAcquired = TRUE;

        Fcb->Header.IsFastIoPossible = FastIoIsNotPossible;

        if (!Ccb) {
            Status = STATUS_SUCCESS;
            __leave;
        }

        ASSERT((Ccb->Identifier.Type == EXT2CCB) &&
            (Ccb->Identifier.Size == sizeof(EXT2_CCB)));

        if (IsFlagOn(Fcb->Flags, FCB_STATE_BUSY)) {
            SetFlag(IrpContext->Flags, IRP_CONTEXT_FLAG_FILE_BUSY);
            DEBUG(DL_USR, ( "Ext2Close: busy bit set: %wZ\n", &Fcb->Mcb->FullName ));
            Status = STATUS_PENDING;
            __leave;
        }

        Ext2DerefXcb(&Vcb->ReferenceCount);
        
        if ((!bBeingClosed) && !Vcb->ReferenceCount && 
            (!IsMounted(Vcb) || IsDispending(Vcb))) {
            bDeleteVcb = TRUE;
            DEBUG(DL_DBG, ( "Ext2Close: Vcb is being released.\n"));
        }

        DEBUG(DL_INF, ( "Ext2Close: Fcb = %p OpenHandleCount= %u ReferenceCount=%u NonCachedCount=%u %wZ\n",
                         Fcb, Fcb->OpenHandleCount, Fcb->ReferenceCount, Fcb->NonCachedOpenCount, &Fcb->Mcb->FullName ));

        if (Ccb) {

            Ext2FreeCcb(Ccb);

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
        
        Status = STATUS_SUCCESS;

    } __finally {

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

                     if (Ext2CheckDismount(IrpContext, Vcb, FALSE)) {
                        if ((Vpb->RealDevice->Vpb != Vpb) &&
                            !IsFlagOn(Vpb->Flags, VPB_PERSISTENT)) {
                            DEC_MEM_COUNT(PS_VPB, Vpb, sizeof(VPB));
                            DEBUG(DL_DBG, ( "Ext2Close: freeing Vpb %p\n", Vpb));
                            ExFreePoolWithTag(Vpb, TAG_VPB);
                        }
                        Ext2ClearVpbFlag(Vcb->Vpb, VPB_MOUNTED);
                    }
                }
            }
        }
    }
    
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

VOID
Ext2DeQueueCloseRequest (IN PVOID Context)
{
    PEXT2_IRP_CONTEXT IrpContext;
    
    IrpContext = (PEXT2_IRP_CONTEXT) Context;
    ASSERT(IrpContext);
    ASSERT((IrpContext->Identifier.Type == EXT2ICX) &&
        (IrpContext->Identifier.Size == sizeof(EXT2_IRP_CONTEXT)));
    
    __try {

        __try {

            FsRtlEnterFileSystem();
            Ext2Close(IrpContext);

        } __except (Ext2ExceptionFilter(IrpContext, GetExceptionInformation())) {

            Ext2ExceptionHandler(IrpContext);
        }

    } __finally {

        FsRtlExitFileSystem();
    }
}
