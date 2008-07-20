/*
 * COPYRIGHT:        See COPYRIGHT.TXT
 * PROJECT:          Ext2 File System Driver for WinNT/2K/XP
 * FILE:             flush.c
 * PROGRAMMER:       Matt Wu <mattwu@163.com>
 * HOMEPAGE:         http://ext2.yeah.net
 * UPDATE HISTORY:   15 Jul 2008 (Pierre Schweitzer <heis_spiter@hotmail.com>)
 *                     Replaced SEH support with PSEH support
 */

/* INCLUDES *****************************************************************/

#include "ext2fs.h"

/* GLOBALS ***************************************************************/

extern PEXT2_GLOBAL Ext2Global;

/* DEFINITIONS *************************************************************/

VOID
Ext2FlushFinal (
    IN PEXT2_IRP_CONTEXT IrpContext,
    IN PNTSTATUS pStatus,
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp,
    IN PEXT2_VCB Vcb,
    IN PEXT2_FCBVCB FcbOrVcb,
    IN BOOLEAN MainResourceAcquired    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, Ext2FlushFile)
#pragma alloc_text(PAGE, Ext2FlushFiles)
#pragma alloc_text(PAGE, Ext2FlushVolume)
#pragma alloc_text(PAGE, Ext2Flush)
#endif


/* FUNCTIONS ***************************************************************/


NTSTATUS
Ext2FlushCompletionRoutine (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context  )

{
    if (Irp->PendingReturned)
        IoMarkIrpPending( Irp );


    if (Irp->IoStatus.Status == STATUS_INVALID_DEVICE_REQUEST)
        Irp->IoStatus.Status = STATUS_SUCCESS;

    return STATUS_SUCCESS;
}

NTSTATUS
Ext2FlushFiles(
    IN PEXT2_IRP_CONTEXT    IrpContext,
    IN PEXT2_VCB            Vcb,
    IN BOOLEAN              bShutDown
    )
{
    IO_STATUS_BLOCK    IoStatus;

    PEXT2_FCB       Fcb;
    PLIST_ENTRY     ListEntry;

    if (IsFlagOn(Vcb->Flags, VCB_READ_ONLY) ||
        IsFlagOn(Vcb->Flags, VCB_WRITE_PROTECTED)) {
        return STATUS_SUCCESS;
    }

    IoStatus.Status = STATUS_SUCCESS;

    DEBUG(DL_INF, ( "Flushing Files ...\n"));

    // Flush all Fcbs in Vcb list queue.
    for (ListEntry = Vcb->FcbList.Flink;
         ListEntry != &Vcb->FcbList;
         ListEntry = ListEntry->Flink ) {

        Fcb = CONTAINING_RECORD(ListEntry, EXT2_FCB, Next);
        ExAcquireResourceExclusiveLite(
                &Fcb->MainResource, TRUE);
        IoStatus.Status = Ext2FlushFile(IrpContext, Fcb, NULL);
        ExReleaseResourceLite(&Fcb->MainResource);
    }

    return IoStatus.Status;
}

NTSTATUS
Ext2FlushVolume (
    IN PEXT2_IRP_CONTEXT    IrpContext,
    IN PEXT2_VCB            Vcb, 
    IN BOOLEAN              bShutDown
    )
{
    IO_STATUS_BLOCK    IoStatus;
   
    DEBUG(DL_INF, ( "Ext2FlushVolume: Flushing Vcb ...\n"));

    ExAcquireSharedStarveExclusive(&Vcb->PagingIoResource, TRUE);
    ExReleaseResourceLite(&Vcb->PagingIoResource);

    CcFlushCache(&(Vcb->SectionObject), NULL, 0, &IoStatus);

    return IoStatus.Status;       
}

NTSTATUS
Ext2FlushFile (
    IN PEXT2_IRP_CONTEXT    IrpContext,
    IN PEXT2_FCB            Fcb,
    IN PEXT2_CCB            Ccb
    )
{
    IO_STATUS_BLOCK    IoStatus;

    ASSERT(Fcb != NULL);
    ASSERT((Fcb->Identifier.Type == EXT2FCB) &&
        (Fcb->Identifier.Size == sizeof(EXT2_FCB)));

    /* update timestamp and achieve attribute */
    if (Ccb != NULL) {

        if (!IsFlagOn(Ccb->Flags, CCB_LAST_WRITE_UPDATED)) {

            LARGE_INTEGER   SysTime;
            KeQuerySystemTime(&SysTime);

            Fcb->Inode->i_mtime = Ext2LinuxTime(SysTime);
            Fcb->Mcb->LastWriteTime = Ext2NtTime(Fcb->Inode->i_mtime);
            Ext2SaveInode(IrpContext, Fcb->Vcb,
                          Fcb->Mcb->iNo, Fcb->Inode);
        }
    }

    if (IsDirectory(Fcb)) {
        return STATUS_SUCCESS;
    }

    DEBUG(DL_INF, ( "Ext2FlushFile: Flushing File Inode=%xh %S ...\n", 
               Fcb->Mcb->iNo, Fcb->Mcb->ShortName.Buffer));

    CcFlushCache(&(Fcb->SectionObject), NULL, 0, &IoStatus);
    ClearFlag(Fcb->Flags, FCB_FILE_MODIFIED);

    return IoStatus.Status;
}

_SEH_DEFINE_LOCALS(Ext2FlushFinal)
{
    PEXT2_IRP_CONTEXT IrpContext;
    PNTSTATUS pStatus;
    PIRP Irp;
    PIO_STACK_LOCATION IrpSp;
    PEXT2_VCB Vcb;
    PEXT2_FCBVCB FcbOrVcb;
    BOOLEAN MainResourceAcquired;
};

_SEH_FINALLYFUNC(Ext2FlushFinal_PSEH)
{
    _SEH_ACCESS_LOCALS(Ext2FlushFinal);
    Ext2FlushFinal(_SEH_VAR(IrpContext), _SEH_VAR(pStatus), _SEH_VAR(Irp),
                   _SEH_VAR(IrpSp), _SEH_VAR(Vcb), _SEH_VAR(FcbOrVcb),
                   _SEH_VAR(MainResourceAcquired));
}

VOID
Ext2FlushFinal (
    IN PEXT2_IRP_CONTEXT    IrpContext,
    IN PNTSTATUS            pStatus,
    IN PIRP                 Irp,
    IN PIO_STACK_LOCATION   IrpSp,
    IN PEXT2_VCB            Vcb,
    IN PEXT2_FCBVCB         FcbOrVcb,
    IN BOOLEAN              MainResourceAcquired
    )
{
    if (MainResourceAcquired) {
        ExReleaseResourceLite(&FcbOrVcb->MainResource);
    }

    if (!IrpContext->ExceptionInProgress) {

        if (Vcb && Irp && IrpSp && (!IsFlagOn(Vcb->Flags, VCB_READ_ONLY))) {

            // Call the disk driver to flush the physial media.
            NTSTATUS DriverStatus;
            PIO_STACK_LOCATION NextIrpSp;

            NextIrpSp = IoGetNextIrpStackLocation(Irp);

            *NextIrpSp = *IrpSp;

            IoSetCompletionRoutine( Irp,
                                    Ext2FlushCompletionRoutine,
                                    NULL,
                                    TRUE,
                                    TRUE,
                                    TRUE );

            DriverStatus = IoCallDriver(Vcb->TargetDeviceObject, Irp);

            *pStatus = (DriverStatus == STATUS_INVALID_DEVICE_REQUEST) ?
                      *pStatus : DriverStatus;

            IrpContext->Irp = Irp = NULL;
        }

        Ext2CompleteIrpContext(IrpContext, *pStatus);
    }
}

NTSTATUS
Ext2Flush (IN PEXT2_IRP_CONTEXT IrpContext)
{
    NTSTATUS                Status = STATUS_SUCCESS;

    PIRP                    Irp = NULL;
    PIO_STACK_LOCATION      IrpSp = NULL;

    PEXT2_VCB               Vcb = NULL;
    PEXT2_FCB               Fcb = NULL;
    PEXT2_FCBVCB            FcbOrVcb = NULL;
    PEXT2_CCB               Ccb = NULL;
    PFILE_OBJECT            FileObject = NULL;

    PDEVICE_OBJECT          DeviceObject = NULL;

    _SEH_TRY {

        _SEH_DECLARE_LOCALS(Ext2FlushFinal);
        _SEH_VAR(IrpContext) = IrpContext;
        _SEH_VAR(pStatus) = &Status;
        _SEH_VAR(Irp) = NULL;
        _SEH_VAR(IrpSp) = NULL;
        _SEH_VAR(Vcb) = NULL;
        _SEH_VAR(FcbOrVcb) = NULL;
        _SEH_VAR(MainResourceAcquired) = FALSE;

        ASSERT(IrpContext);
    
        ASSERT((IrpContext->Identifier.Type == EXT2ICX) &&
            (IrpContext->Identifier.Size == sizeof(EXT2_IRP_CONTEXT)));

        DeviceObject = IrpContext->DeviceObject;
        
        //
        // This request is not allowed on the main device object
        //
        if (IsExt2FsDevice(DeviceObject)) {
            Status = STATUS_INVALID_DEVICE_REQUEST;
            _SEH_LEAVE;
        }
        
        Vcb = (PEXT2_VCB) DeviceObject->DeviceExtension;
        _SEH_VAR(Vcb) = Vcb;
        ASSERT(Vcb != NULL);
        ASSERT((Vcb->Identifier.Type == EXT2VCB) &&
            (Vcb->Identifier.Size == sizeof(EXT2_VCB)));

        ASSERT(IsMounted(Vcb));
        if ( IsFlagOn(Vcb->Flags, VCB_READ_ONLY) ||
             IsFlagOn(Vcb->Flags, VCB_WRITE_PROTECTED)) {
            Status =  STATUS_SUCCESS;
            _SEH_LEAVE;
        }

        Irp = IrpContext->Irp;
        _SEH_VAR(Irp) = Irp;
        IrpSp = IoGetCurrentIrpStackLocation(Irp);
        _SEH_VAR(IrpSp) = IrpSp;

        FileObject = IrpContext->FileObject;
        FcbOrVcb = (PEXT2_FCBVCB) FileObject->FsContext;
        _SEH_VAR(FcbOrVcb) = FcbOrVcb;
        ASSERT(FcbOrVcb != NULL);

        Ccb = (PEXT2_CCB) FileObject->FsContext2;
        if (Ccb == NULL) {
            Status =  STATUS_SUCCESS;
            _SEH_LEAVE;
        }

        _SEH_VAR(MainResourceAcquired) = 
        ExAcquireResourceExclusiveLite(&FcbOrVcb->MainResource,
                IsFlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT));

        ASSERT(_SEH_VAR(MainResourceAcquired));
        DEBUG(DL_USR, ("Ext2Flush-pre:  total mcb records=%u\n",
                           FsRtlNumberOfRunsInLargeMcb(&Vcb->Extents)));

        if (FcbOrVcb->Identifier.Type == EXT2VCB) {

            Ext2VerifyVcb(IrpContext, Vcb);
            Status = Ext2FlushFiles(IrpContext, (PEXT2_VCB)(FcbOrVcb), FALSE);
            if (NT_SUCCESS(Status)) {
                _SEH_LEAVE;
            }

            Status = Ext2FlushVolume(IrpContext, (PEXT2_VCB)(FcbOrVcb), FALSE);

            if (NT_SUCCESS(Status) && IsFlagOn(Vcb->Volume->Flags, FO_FILE_MODIFIED)) {
                ClearFlag(Vcb->Volume->Flags, FO_FILE_MODIFIED);
            }

        } else if (FcbOrVcb->Identifier.Type == EXT2FCB) {

            Fcb = (PEXT2_FCB)(FcbOrVcb);

            Status = Ext2FlushFile(IrpContext, Fcb, Ccb);
            if (NT_SUCCESS(Status)) {
                if (IsFlagOn(FileObject->Flags, FO_FILE_MODIFIED)) {
                    Fcb->Mcb->FileAttr |= FILE_ATTRIBUTE_ARCHIVE;
                    ClearFlag(FileObject->Flags, FO_FILE_MODIFIED);
                }
            }
        }

        DEBUG(DL_USR, ("Ext2Flush-post: total mcb records=%u\n",
                        FsRtlNumberOfRunsInLargeMcb(&Vcb->Extents)));


    }
    _SEH_FINALLY(Ext2FlushFinal_PSEH)
    _SEH_END;

    return Status;
}
