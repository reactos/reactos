/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             drivers/fs/vfat/flush.c
 * PURPOSE:          VFAT Filesystem
 * PROGRAMMER:
 */

/* INCLUDES *****************************************************************/

#include "vfat.h"

#define NDEBUG
#include <debug.h>

/* FUNCTIONS ****************************************************************/

static
NTSTATUS
VfatFlushFile(
    PDEVICE_EXTENSION DeviceExt,
    PVFATFCB Fcb)
{
    IO_STATUS_BLOCK IoStatus;
    NTSTATUS Status;

    DPRINT("VfatFlushFile(DeviceExt %p, Fcb %p) for '%wZ'\n", DeviceExt, Fcb, &Fcb->PathNameU);

    CcFlushCache(&Fcb->SectionObjectPointers, NULL, 0, &IoStatus);
    if (IoStatus.Status == STATUS_INVALID_PARAMETER)
    {
        /* FIXME: Caching was possible not initialized */
        IoStatus.Status = STATUS_SUCCESS;
    }

    if (Fcb->Flags & FCB_IS_DIRTY)
    {
        Status = VfatUpdateEntry(Fcb);
        if (!NT_SUCCESS(Status))
        {
            IoStatus.Status = Status;
        }
    }
    return IoStatus.Status;
}

NTSTATUS
VfatFlushVolume(
    PDEVICE_EXTENSION DeviceExt,
    PVFATFCB VolumeFcb)
{
    PLIST_ENTRY ListEntry;
    PVFATFCB Fcb;
    NTSTATUS Status, ReturnStatus = STATUS_SUCCESS;
    PIRP Irp;
    KEVENT Event;
    IO_STATUS_BLOCK IoStatusBlock;

    DPRINT("VfatFlushVolume(DeviceExt %p, FatFcb %p)\n", DeviceExt, VolumeFcb);

    ListEntry = DeviceExt->FcbListHead.Flink;
    while (ListEntry != &DeviceExt->FcbListHead)
    {
        Fcb = CONTAINING_RECORD(ListEntry, VFATFCB, FcbListEntry);
        ListEntry = ListEntry->Flink;
        if (!vfatFCBIsDirectory(Fcb))
        {
            ExAcquireResourceExclusiveLite(&Fcb->MainResource, TRUE);
            Status = VfatFlushFile(DeviceExt, Fcb);
            ExReleaseResourceLite (&Fcb->MainResource);
            if (!NT_SUCCESS(Status))
            {
                DPRINT1("VfatFlushFile failed, status = %x\n", Status);
                ReturnStatus = Status;
            }
        }
        /* FIXME: Stop flushing if this is a removable media and the media was removed */
    }

    ListEntry = DeviceExt->FcbListHead.Flink;
    while (ListEntry != &DeviceExt->FcbListHead)
    {
        Fcb = CONTAINING_RECORD(ListEntry, VFATFCB, FcbListEntry);
        ListEntry = ListEntry->Flink;
        if (vfatFCBIsDirectory(Fcb))
        {
            ExAcquireResourceExclusiveLite(&Fcb->MainResource, TRUE);
            Status = VfatFlushFile(DeviceExt, Fcb);
            ExReleaseResourceLite (&Fcb->MainResource);
            if (!NT_SUCCESS(Status))
            {
                DPRINT1("VfatFlushFile failed, status = %x\n", Status);
                ReturnStatus = Status;
            }
        }
        /* FIXME: Stop flushing if this is a removable media and the media was removed */
    }

    Fcb = (PVFATFCB) DeviceExt->FATFileObject->FsContext;

    ExAcquireResourceExclusiveLite(&DeviceExt->FatResource, TRUE);
    Status = VfatFlushFile(DeviceExt, Fcb);
    ExReleaseResourceLite(&DeviceExt->FatResource);

    /* Prepare an IRP to flush device buffers */
    Irp = IoBuildSynchronousFsdRequest(IRP_MJ_FLUSH_BUFFERS,
                                       DeviceExt->StorageDevice,
                                       NULL, 0, NULL, &Event,
                                       &IoStatusBlock);
    if (Irp != NULL)
    {
        KeInitializeEvent(&Event, NotificationEvent, FALSE);

        Status = IoCallDriver(DeviceExt->StorageDevice, Irp);
        if (Status == STATUS_PENDING)
        {
            KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
            Status = Irp->IoStatus.Status;
        }

        /* Ignore device not supporting flush operation */
        if (Status == STATUS_INVALID_DEVICE_REQUEST)
        {
            DPRINT1("Flush not supported, ignored\n");
            Status = STATUS_SUCCESS;

        }
    }
    else
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
    }

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("VfatFlushFile failed, status = %x\n", Status);
        ReturnStatus = Status;
    }

    return ReturnStatus;
}

NTSTATUS
VfatFlush(
    PVFAT_IRP_CONTEXT IrpContext)
{
    NTSTATUS Status;
    PVFATFCB Fcb;

    /* This request is not allowed on the main device object. */
    if (IrpContext->DeviceObject == VfatGlobalData->DeviceObject)
    {
        IrpContext->Irp->IoStatus.Information = 0;
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    Fcb = (PVFATFCB)IrpContext->FileObject->FsContext;
    ASSERT(Fcb);

    if (Fcb->Flags & FCB_IS_VOLUME)
    {
        ExAcquireResourceExclusiveLite(&IrpContext->DeviceExt->DirResource, TRUE);
        Status = VfatFlushVolume(IrpContext->DeviceExt, Fcb);
        ExReleaseResourceLite(&IrpContext->DeviceExt->DirResource);
    }
    else
    {
        ExAcquireResourceExclusiveLite(&Fcb->MainResource, TRUE);
        Status = VfatFlushFile(IrpContext->DeviceExt, Fcb);
        ExReleaseResourceLite (&Fcb->MainResource);
    }

    IrpContext->Irp->IoStatus.Information = 0;
    return Status;
}

/* EOF */
