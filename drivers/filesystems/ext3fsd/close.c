/*
 * COPYRIGHT:        See COPYRIGHT.TXT
 * PROJECT:          Ext2 File System Driver for WinNT/2K/XP
 * FILE:             close.c
 * PROGRAMMER:       Matt Wu <mattwu@163.com>
 * HOMEPAGE:         http://ext2.yeah.net
 * UPDATE HISTORY:   12 Jul 2008 (Pierre Schweitzer <heis_spiter@hotmail.com>)
 *                     Replaced SEH support with PSEH support
 */

/* INCLUDES *****************************************************************/

#include "ext2fs.h"

/* GLOBALS ***************************************************************/

extern PEXT2_GLOBAL Ext2Global;

/* DEFINITIONS *************************************************************/

VOID
Ext2CloseFinal (
    IN PEXT2_IRP_CONTEXT IrpContext,
    IN PNTSTATUS pStatus,
    IN PEXT2_VCB Vcb,
    IN BOOLEAN VcbResourceAcquired,
    IN PEXT2_FCB Fcb,
    IN BOOLEAN FcbResourceAcquired,
    IN BOOLEAN bDeleteVcb,
    IN BOOLEAN bBeingClosed,
    IN BOOLEAN bSkipLeave   );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, Ext2Close)
#pragma alloc_text(PAGE, Ext2QueueCloseRequest)
#pragma alloc_text(PAGE, Ext2DeQueueCloseRequest)
#endif


/* FUNCTIONS ***************************************************************/


_SEH_DEFINE_LOCALS(Ext2CloseFinal)
{
    PEXT2_IRP_CONTEXT       IrpContext;
    PNTSTATUS               pStatus;
    PEXT2_VCB               Vcb;
    BOOLEAN                 VcbResourceAcquired;
    PEXT2_FCB               Fcb;
    BOOLEAN                 FcbResourceAcquired;
    BOOLEAN                 bDeleteVcb;
    BOOLEAN                 bBeingClosed;
    BOOLEAN                 bSkipLeave;
};

_SEH_FINALLYFUNC(Ext2CloseFinal_PSEH)
{
    _SEH_ACCESS_LOCALS(Ext2CloseFinal);
    Ext2CloseFinal(_SEH_VAR(IrpContext), _SEH_VAR(pStatus), _SEH_VAR(Vcb),
                   _SEH_VAR(VcbResourceAcquired), _SEH_VAR(Fcb),
                   _SEH_VAR(FcbResourceAcquired), _SEH_VAR(bDeleteVcb),
                   _SEH_VAR(bBeingClosed), _SEH_VAR(bSkipLeave));
}

VOID
Ext2CloseFinal (
    IN PEXT2_IRP_CONTEXT    IrpContext,
    IN PNTSTATUS            pStatus,
    IN PEXT2_VCB            Vcb,
    IN BOOLEAN              VcbResourceAcquired,
    IN PEXT2_FCB            Fcb,
    IN BOOLEAN              FcbResourceAcquired,
    IN BOOLEAN              bDeleteVcb,
    IN BOOLEAN              bBeingClosed,
    IN BOOLEAN              bSkipLeave
    )
{
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

        if (*pStatus == STATUS_PENDING) {

            Ext2QueueCloseRequest(IrpContext);

        } else {

            Ext2CompleteIrpContext(IrpContext, *pStatus);

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

NTSTATUS
Ext2Close (IN PEXT2_IRP_CONTEXT IrpContext)
{
    PDEVICE_OBJECT  DeviceObject;
    NTSTATUS        Status = STATUS_SUCCESS;
    PEXT2_VCB       Vcb;
    PFILE_OBJECT    FileObject;
    PEXT2_FCB       Fcb;
    PEXT2_CCB       Ccb;

    _SEH_TRY {

        _SEH_DECLARE_LOCALS(Ext2CloseFinal);
        _SEH_VAR(IrpContext) = IrpContext;
        _SEH_VAR(pStatus) = &Status;
        _SEH_VAR(VcbResourceAcquired) = FALSE;
        _SEH_VAR(FcbResourceAcquired) = FALSE;
        _SEH_VAR(bDeleteVcb) = FALSE;
        _SEH_VAR(bBeingClosed) = FALSE;
        _SEH_VAR(bSkipLeave) = FALSE;

        ASSERT(IrpContext != NULL);
        ASSERT((IrpContext->Identifier.Type == EXT2ICX) &&
            (IrpContext->Identifier.Size == sizeof(EXT2_IRP_CONTEXT)));
        
        DeviceObject = IrpContext->DeviceObject;
        if (IsExt2FsDevice(DeviceObject)) {
            Status = STATUS_SUCCESS;
            _SEH_LEAVE;
        }
        
        Vcb = (PEXT2_VCB) DeviceObject->DeviceExtension;
        _SEH_VAR(Vcb) = Vcb;
        ASSERT(Vcb != NULL);
        ASSERT((Vcb->Identifier.Type == EXT2VCB) &&
            (Vcb->Identifier.Size == sizeof(EXT2_VCB)));

        if (!ExAcquireResourceExclusiveLite(
            &Vcb->MainResource,
            IsFlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT) )) {
            DEBUG(DL_INF, ("Ext2Close: PENDING ... Vcb: %xh/%xh\n",
                                 Vcb->OpenFileHandleCount, Vcb->ReferenceCount));

            Status = STATUS_PENDING;
            _SEH_LEAVE;
        }
        _SEH_VAR(VcbResourceAcquired) = TRUE;

        _SEH_VAR(bSkipLeave) = TRUE;
        if (IsFlagOn(Vcb->Flags, VCB_BEING_CLOSED)) {
            _SEH_VAR(bBeingClosed) = TRUE;
        } else {
            SetLongFlag(Vcb->Flags, VCB_BEING_CLOSED);
            _SEH_VAR(bBeingClosed) = FALSE;
        }

        if (IsFlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_DELAY_CLOSE)) {

            FileObject = NULL;
            Fcb = IrpContext->Fcb;
            _SEH_VAR(Fcb) = Fcb;
            Ccb = IrpContext->Ccb;

        } else {

            FileObject = IrpContext->FileObject;
            Fcb = (PEXT2_FCB) FileObject->FsContext;
            _SEH_VAR(Fcb) = Fcb;
            if (!Fcb) {
                Status = STATUS_SUCCESS;
                _SEH_LEAVE;
            }
            ASSERT(Fcb != NULL);
            Ccb = (PEXT2_CCB) FileObject->FsContext2;
        }
        

        DEBUG(DL_DBG, ( "Ext2Close: (VCB) bBeingClosed = %d Vcb = %p ReferCount = %d\n",
                         _SEH_VAR(bBeingClosed), Vcb, Vcb->ReferenceCount));

        if (Fcb->Identifier.Type == EXT2VCB) {

            if ((!_SEH_VAR(bBeingClosed)) && (Vcb->ReferenceCount <= 1)&& 
                (!IsMounted(Vcb) || IsDispending(Vcb))) {
                _SEH_VAR(bDeleteVcb) = TRUE;
            }

            if (Ccb) {

                Ext2DerefXcb(&Vcb->ReferenceCount);
                Ext2FreeCcb(Ccb);

                if (FileObject) {
                    FileObject->FsContext2 = Ccb = NULL;
                }
            }

            Status = STATUS_SUCCESS;
            _SEH_LEAVE;
        }
        
        if ( Fcb->Identifier.Type != EXT2FCB ||
             Fcb->Identifier.Size != sizeof(EXT2_FCB)) {
            _SEH_LEAVE;
        }

        if (!ExAcquireResourceExclusiveLite(
            &Fcb->MainResource,
            IsFlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT) )) {
            Status = STATUS_PENDING;
            _SEH_LEAVE;
        }
        _SEH_VAR(FcbResourceAcquired) = TRUE;

        Fcb->Header.IsFastIoPossible = FastIoIsNotPossible;

        if (!Ccb) {
            Status = STATUS_SUCCESS;
            _SEH_LEAVE;
        }

        ASSERT((Ccb->Identifier.Type == EXT2CCB) &&
            (Ccb->Identifier.Size == sizeof(EXT2_CCB)));

        if (IsFlagOn(Fcb->Flags, FCB_STATE_BUSY)) {
            SetFlag(IrpContext->Flags, IRP_CONTEXT_FLAG_FILE_BUSY);
            DEBUG(DL_USR, ( "Ext2Close: busy bit set: %wZ\n", &Fcb->Mcb->FullName ));
            Status = STATUS_PENDING;
            _SEH_LEAVE;
        }

        Ext2DerefXcb(&Vcb->ReferenceCount);
        
        if ((!_SEH_VAR(bBeingClosed)) && !Vcb->ReferenceCount && 
            (!IsMounted(Vcb) || IsDispending(Vcb))) {
            _SEH_VAR(bDeleteVcb) = TRUE;
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

            if (_SEH_VAR(FcbResourceAcquired)) {
                ExReleaseResourceLite(&Fcb->MainResource);
                _SEH_VAR(FcbResourceAcquired) = FALSE;
            }

            Ext2FreeFcb(Fcb);

            if (FileObject) {
                FileObject->FsContext = Fcb = NULL;
            }
        }
        
        Status = STATUS_SUCCESS;

    }
    _SEH_FINALLY(Ext2CloseFinal_PSEH)
    _SEH_END;
    
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

_SEH_DEFINE_LOCALS(Ext2ExceptionFilter)
{
    PEXT2_IRP_CONTEXT IrpContext;
};

_SEH_FILTER(Ext2ExceptionFilter_PSEH)
{
    _SEH_ACCESS_LOCALS(Ext2ExceptionFilter);
    return Ext2ExceptionFilter(_SEH_VAR(IrpContext), _SEH_GetExceptionPointers());
}

_SEH_WRAP_FINALLY(FsRtlExitFileSystem_PSEH_finally, FsRtlExitFileSystem);

VOID
Ext2DeQueueCloseRequest (IN PVOID Context)
{
    PEXT2_IRP_CONTEXT IrpContext;
    
    IrpContext = (PEXT2_IRP_CONTEXT) Context;
    ASSERT(IrpContext);
    ASSERT((IrpContext->Identifier.Type == EXT2ICX) &&
        (IrpContext->Identifier.Size == sizeof(EXT2_IRP_CONTEXT)));
    
    _SEH_TRY {

        _SEH_DECLARE_LOCALS(Ext2ExceptionFilter);
        _SEH_VAR(IrpContext) = IrpContext;

        _SEH_TRY {

            FsRtlEnterFileSystem();
            Ext2Close(IrpContext);

        }
        _SEH_EXCEPT(Ext2ExceptionFilter_PSEH) {

            Ext2ExceptionHandler(IrpContext);
        }
        _SEH_END;

    }
    _SEH_FINALLY(FsRtlExitFileSystem_PSEH_finally)
    _SEH_END;
}
