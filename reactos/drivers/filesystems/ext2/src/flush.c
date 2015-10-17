/*
 * COPYRIGHT:        See COPYRIGHT.TXT
 * PROJECT:          Ext2 File System Driver for WinNT/2K/XP
 * FILE:             flush.c
 * PROGRAMMER:       Matt Wu <mattwu@163.com>
 * HOMEPAGE:         http://www.ext2fsd.com
 * UPDATE HISTORY:
 */

/* INCLUDES *****************************************************************/

#include "ext2fs.h"

/* GLOBALS ***************************************************************/

extern PEXT2_GLOBAL Ext2Global;

/* DEFINITIONS *************************************************************/


NTSTATUS NTAPI
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

    if (IsVcbReadOnly(Vcb)) {
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
            Ext2SaveInode(IrpContext, Fcb->Vcb, Fcb->Inode);
        }
    }

    if (IsDirectory(Fcb)) {
        return STATUS_SUCCESS;
    }

    DEBUG(DL_INF, ( "Ext2FlushFile: Flushing File Inode=%xh %S ...\n",
                    Fcb->Inode->i_ino, Fcb->Mcb->ShortName.Buffer));

    CcFlushCache(&(Fcb->SectionObject), NULL, 0, &IoStatus);
    ClearFlag(Fcb->Flags, FCB_FILE_MODIFIED);

    return IoStatus.Status;
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

    BOOLEAN                 MainResourceAcquired = FALSE;

    _SEH2_TRY {

        ASSERT(IrpContext);

        ASSERT((IrpContext->Identifier.Type == EXT2ICX) &&
               (IrpContext->Identifier.Size == sizeof(EXT2_IRP_CONTEXT)));

        DeviceObject = IrpContext->DeviceObject;

        //
        // This request is not allowed on the main device object
        //
        if (IsExt2FsDevice(DeviceObject)) {
            Status = STATUS_INVALID_DEVICE_REQUEST;
            _SEH2_LEAVE;
        }

        Vcb = (PEXT2_VCB) DeviceObject->DeviceExtension;
        ASSERT(Vcb != NULL);
        ASSERT((Vcb->Identifier.Type == EXT2VCB) &&
               (Vcb->Identifier.Size == sizeof(EXT2_VCB)));

        ASSERT(IsMounted(Vcb));
        if (IsVcbReadOnly(Vcb)) {
            Status =  STATUS_SUCCESS;
            _SEH2_LEAVE;
        }

        Irp = IrpContext->Irp;
        IrpSp = IoGetCurrentIrpStackLocation(Irp);

        FileObject = IrpContext->FileObject;
        FcbOrVcb = (PEXT2_FCBVCB) FileObject->FsContext;
        ASSERT(FcbOrVcb != NULL);

        Ccb = (PEXT2_CCB) FileObject->FsContext2;
        if (Ccb == NULL) {
            Status =  STATUS_SUCCESS;
            _SEH2_LEAVE;
        }

        MainResourceAcquired =
            ExAcquireResourceExclusiveLite(&FcbOrVcb->MainResource,
                                           IsFlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT));

        ASSERT(MainResourceAcquired);
        DEBUG(DL_INF, ("Ext2Flush-pre:  total mcb records=%u\n",
                       FsRtlNumberOfRunsInLargeMcb(&Vcb->Extents)));

        if (FcbOrVcb->Identifier.Type == EXT2VCB) {

            Ext2VerifyVcb(IrpContext, Vcb);
            Status = Ext2FlushFiles(IrpContext, (PEXT2_VCB)(FcbOrVcb), FALSE);
            if (NT_SUCCESS(Status)) {
                _SEH2_LEAVE;
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

        DEBUG(DL_INF, ("Ext2Flush-post: total mcb records=%u\n",
                       FsRtlNumberOfRunsInLargeMcb(&Vcb->Extents)));


    } _SEH2_FINALLY {

        if (MainResourceAcquired) {
            ExReleaseResourceLite(&FcbOrVcb->MainResource);
        }

        if (!IrpContext->ExceptionInProgress) {

            if (Vcb && Irp && IrpSp && !IsVcbReadOnly(Vcb)) {

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

                Status = (DriverStatus == STATUS_INVALID_DEVICE_REQUEST) ?
                         Status : DriverStatus;

                IrpContext->Irp = Irp = NULL;
            }

            Ext2CompleteIrpContext(IrpContext, Status);
        }
    } _SEH2_END;

    return Status;
}