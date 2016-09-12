/*
 * COPYRIGHT:        See COPYRIGHT.TXT
 * PROJECT:          Ext2 File System Driver for WinNT/2K/XP
 * FILE:             dispatch.c
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
#pragma alloc_text(PAGE, Ext2QueueRequest)
#pragma alloc_text(PAGE, Ext2DeQueueRequest)
#endif

/*
 *  Ext2OplockComplete
 *
 *    callback routine of FsRtlCheckOplock when an oplock break has
 *    completed, allowing an Irp to resume execution.
 *
 *  Arguments:
 *
 *    Context:  the IrpContext to be queued
 *    Irp:      the I/O request packet
 *
 *  Return Value:
 *    N/A
 */

VOID NTAPI
Ext2OplockComplete (
    IN PVOID Context,
    IN PIRP Irp
)
{
    //
    //  Check on the return value in the Irp.
    //

    if (Irp->IoStatus.Status == STATUS_SUCCESS) {

        //
        //  queue the Irp context in the workqueue.
        //

        Ext2QueueRequest((PEXT2_IRP_CONTEXT)Context);

    } else {

        //
        //  complete the request in case of failure
        //

        Ext2CompleteIrpContext( (PEXT2_IRP_CONTEXT) Context,
                                Irp->IoStatus.Status );
    }

    return;
}


/*
 *  Ext2LockIrp
 *
 *    performs buffer locking if we need pend the process of the Irp
 *
 *  Arguments:
 *    Context: the irp context
 *    Irp:     the I/O request packet.
 *
 *  Return Value:
 *    N/A
 */

VOID NTAPI
Ext2LockIrp (
    IN PVOID Context,
    IN PIRP Irp
)
{
    PIO_STACK_LOCATION IrpSp;
    PEXT2_IRP_CONTEXT IrpContext;

    if (Irp == NULL) {
        return;
    }

    IrpSp = IoGetCurrentIrpStackLocation(Irp);

    IrpContext = (PEXT2_IRP_CONTEXT) Context;

    if ( IrpContext->MajorFunction == IRP_MJ_READ ||
            IrpContext->MajorFunction == IRP_MJ_WRITE ) {

        //
        //  lock the user's buffer to MDL, if the I/O is bufferred
        //

        if (!IsFlagOn(IrpContext->MinorFunction, IRP_MN_MDL)) {

            Ext2LockUserBuffer( Irp, IrpSp->Parameters.Write.Length,
                                (IrpContext->MajorFunction == IRP_MJ_READ) ?
                                IoWriteAccess : IoReadAccess );
        }

    } else if (IrpContext->MajorFunction == IRP_MJ_DIRECTORY_CONTROL
               && IrpContext->MinorFunction == IRP_MN_QUERY_DIRECTORY) {

        ULONG Length = ((PEXTENDED_IO_STACK_LOCATION) IrpSp)->Parameters.QueryDirectory.Length;
        Ext2LockUserBuffer(Irp, Length, IoWriteAccess);

    } else if (IrpContext->MajorFunction == IRP_MJ_QUERY_EA) {

        ULONG Length = ((PEXTENDED_IO_STACK_LOCATION) IrpSp)->Parameters.QueryEa.Length;
        Ext2LockUserBuffer(Irp, Length, IoWriteAccess);

    } else if (IrpContext->MajorFunction == IRP_MJ_SET_EA) {
        ULONG Length = ((PEXTENDED_IO_STACK_LOCATION) IrpSp)->Parameters.SetEa.Length;
        Ext2LockUserBuffer(Irp, Length, IoReadAccess);

    } else if ( (IrpContext->MajorFunction == IRP_MJ_FILE_SYSTEM_CONTROL) &&
                (IrpContext->MinorFunction == IRP_MN_USER_FS_REQUEST) ) {
        PEXTENDED_IO_STACK_LOCATION EIrpSp = (PEXTENDED_IO_STACK_LOCATION)IrpSp;
        if ( (EIrpSp->Parameters.FileSystemControl.FsControlCode == FSCTL_GET_VOLUME_BITMAP) ||
                (EIrpSp->Parameters.FileSystemControl.FsControlCode == FSCTL_GET_RETRIEVAL_POINTERS) ||
                (EIrpSp->Parameters.FileSystemControl.FsControlCode == FSCTL_GET_RETRIEVAL_POINTER_BASE) ) {
            ULONG Length = EIrpSp->Parameters.FileSystemControl.OutputBufferLength;
            Ext2LockUserBuffer(Irp, Length, IoWriteAccess);
        }
    }

    //  Mark the request as pending status

    IoMarkIrpPending( Irp );

    return;
}

NTSTATUS
Ext2QueueRequest (IN PEXT2_IRP_CONTEXT IrpContext)
{
    ASSERT(IrpContext);

    ASSERT((IrpContext->Identifier.Type == EXT2ICX) &&
           (IrpContext->Identifier.Size == sizeof(EXT2_IRP_CONTEXT)));

    /* set the flags of "can wait" and "queued" */
    SetFlag(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT);
    SetFlag(IrpContext->Flags, IRP_CONTEXT_FLAG_REQUEUED);

    /* make sure the buffer is kept valid in system context */
    Ext2LockIrp(IrpContext, IrpContext->Irp);

    /* initialize workite*/
    ExInitializeWorkItem(
        &IrpContext->WorkQueueItem,
        Ext2DeQueueRequest,
        IrpContext );

    /* dispatch it */
    ExQueueWorkItem(&IrpContext->WorkQueueItem, CriticalWorkQueue);

    return STATUS_PENDING;
}


VOID NTAPI
Ext2DeQueueRequest (IN PVOID Context)
{
    PEXT2_IRP_CONTEXT IrpContext;

    IrpContext = (PEXT2_IRP_CONTEXT) Context;

    ASSERT(IrpContext);

    ASSERT((IrpContext->Identifier.Type == EXT2ICX) &&
           (IrpContext->Identifier.Size == sizeof(EXT2_IRP_CONTEXT)));

    _SEH2_TRY {

        _SEH2_TRY {

            FsRtlEnterFileSystem();

            if (!IrpContext->IsTopLevel) {
                IoSetTopLevelIrp((PIRP) FSRTL_FSP_TOP_LEVEL_IRP);
            }

            Ext2DispatchRequest(IrpContext);

        } _SEH2_EXCEPT (Ext2ExceptionFilter(IrpContext, _SEH2_GetExceptionInformation())) {

            Ext2ExceptionHandler(IrpContext);
        } _SEH2_END;

    } _SEH2_FINALLY {

        IoSetTopLevelIrp(NULL);

        FsRtlExitFileSystem();
    } _SEH2_END;
}


NTSTATUS
Ext2DispatchRequest (IN PEXT2_IRP_CONTEXT IrpContext)
{
    ASSERT(IrpContext);

    ASSERT((IrpContext->Identifier.Type == EXT2ICX) &&
           (IrpContext->Identifier.Size == sizeof(EXT2_IRP_CONTEXT)));

    switch (IrpContext->MajorFunction) {

    case IRP_MJ_CREATE:
        return Ext2Create(IrpContext);

    case IRP_MJ_CLOSE:
        return Ext2Close(IrpContext);

    case IRP_MJ_READ:
        return Ext2Read(IrpContext);

    case IRP_MJ_WRITE:
        return Ext2Write(IrpContext);

    case IRP_MJ_FLUSH_BUFFERS:
        return Ext2Flush(IrpContext);

    case IRP_MJ_QUERY_INFORMATION:
        return Ext2QueryFileInformation(IrpContext);

    case IRP_MJ_SET_INFORMATION:
        return Ext2SetFileInformation(IrpContext);

    case IRP_MJ_QUERY_VOLUME_INFORMATION:
        return Ext2QueryVolumeInformation(IrpContext);

    case IRP_MJ_SET_VOLUME_INFORMATION:
        return Ext2SetVolumeInformation(IrpContext);

    case IRP_MJ_DIRECTORY_CONTROL:
        return Ext2DirectoryControl(IrpContext);

    case IRP_MJ_FILE_SYSTEM_CONTROL:
        return Ext2FileSystemControl(IrpContext);

    case IRP_MJ_DEVICE_CONTROL:
        return Ext2DeviceControl(IrpContext);

    case IRP_MJ_LOCK_CONTROL:
        return Ext2LockControl(IrpContext);

    case IRP_MJ_CLEANUP:
        return Ext2Cleanup(IrpContext);

    case IRP_MJ_SHUTDOWN:
        return Ext2ShutDown(IrpContext);

#if (_WIN32_WINNT >= 0x0500)
    case IRP_MJ_PNP:
        return Ext2Pnp(IrpContext);
#endif //(_WIN32_WINNT >= 0x0500)        
    default:
        DEBUG(DL_ERR, ( "Ext2DispatchRequest: Unexpected major function: %xh\n",
                        IrpContext->MajorFunction));

        Ext2CompleteIrpContext(IrpContext, STATUS_DRIVER_INTERNAL_ERROR);

        return STATUS_DRIVER_INTERNAL_ERROR;
    }
}


NTSTATUS NTAPI
Ext2BuildRequest (PDEVICE_OBJECT   DeviceObject, PIRP Irp)
{
    BOOLEAN             AtIrqlPassiveLevel = FALSE;
    BOOLEAN             IsTopLevelIrp = FALSE;
    PEXT2_IRP_CONTEXT   IrpContext = NULL;
    NTSTATUS            Status = STATUS_UNSUCCESSFUL;

    _SEH2_TRY {

        _SEH2_TRY {

#if EXT2_DEBUG
            Ext2DbgPrintCall(DeviceObject, Irp);
#endif

            AtIrqlPassiveLevel = (KeGetCurrentIrql() == PASSIVE_LEVEL);

            if (AtIrqlPassiveLevel) {
                FsRtlEnterFileSystem();
            }

            if (!IoGetTopLevelIrp()) {
                IsTopLevelIrp = TRUE;
                IoSetTopLevelIrp(Irp);
            }

            IrpContext = Ext2AllocateIrpContext(DeviceObject, Irp);

            if (!IrpContext) {

                Status = STATUS_INSUFFICIENT_RESOURCES;
                Irp->IoStatus.Status = Status;

                Ext2CompleteRequest(Irp, TRUE, IO_NO_INCREMENT);

            } else {

                if ((IrpContext->MajorFunction == IRP_MJ_CREATE) &&
                        !AtIrqlPassiveLevel) {

                    DbgBreak();
                }

                Status = Ext2DispatchRequest(IrpContext);
            }
        } _SEH2_EXCEPT (Ext2ExceptionFilter(IrpContext, _SEH2_GetExceptionInformation())) {

            Status = Ext2ExceptionHandler(IrpContext);
        } _SEH2_END;

    } _SEH2_FINALLY {

        if (IsTopLevelIrp) {
            IoSetTopLevelIrp(NULL);
        }

        if (AtIrqlPassiveLevel) {
            FsRtlExitFileSystem();
        }
    } _SEH2_END;

    return Status;
}
