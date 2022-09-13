/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             drivers/fs/vfat/shutdown.c
 * PURPOSE:          VFAT Filesystem
 * PROGRAMMER:       Eric Kohl
 */

/* INCLUDES *****************************************************************/

#include "vfat.h"

#define NDEBUG
#include <debug.h>

/* FUNCTIONS ****************************************************************/

static
NTSTATUS
VfatDiskShutDown(
    PVCB Vcb)
{
    PIRP Irp;
    KEVENT Event;
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatus;

    KeInitializeEvent(&Event, NotificationEvent, FALSE);
    Irp = IoBuildSynchronousFsdRequest(IRP_MJ_SHUTDOWN, Vcb->StorageDevice,
                                       NULL, 0, NULL, &Event, &IoStatus);
    if (Irp)
    {
        Status = IoCallDriver(Vcb->StorageDevice, Irp);
        if (Status == STATUS_PENDING)
        {
            KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
            Status = IoStatus.Status;
        }
    }
    else
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
    }

    return Status;
}


NTSTATUS
NTAPI
VfatShutdown(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp)
{
    NTSTATUS Status;
    PLIST_ENTRY ListEntry;
    PDEVICE_EXTENSION DeviceExt;

    DPRINT("VfatShutdown(DeviceObject %p, Irp %p)\n",DeviceObject, Irp);

    FsRtlEnterFileSystem();

    /* FIXME: block new mount requests */
    VfatGlobalData->ShutdownStarted = TRUE;

    if (DeviceObject == VfatGlobalData->DeviceObject)
    {
        Irp->IoStatus.Status = STATUS_SUCCESS;
        ExAcquireResourceExclusiveLite(&VfatGlobalData->VolumeListLock, TRUE);
        ListEntry = VfatGlobalData->VolumeListHead.Flink;
        while (ListEntry != &VfatGlobalData->VolumeListHead)
        {
            DeviceExt = CONTAINING_RECORD(ListEntry, VCB, VolumeListEntry);
            ListEntry = ListEntry->Flink;

            ExAcquireResourceExclusiveLite(&DeviceExt->DirResource, TRUE);

            /* Flush volume & files */
            Status = VfatFlushVolume(DeviceExt, DeviceExt->VolumeFcb);

            /* We're performing a clean shutdown */
            if (BooleanFlagOn(DeviceExt->VolumeFcb->Flags, VCB_CLEAR_DIRTY) &&
                BooleanFlagOn(DeviceExt->VolumeFcb->Flags, VCB_IS_DIRTY))
            {
                /* Drop the dirty bit */
                if (NT_SUCCESS(SetDirtyStatus(DeviceExt, FALSE)))
                    DeviceExt->VolumeFcb->Flags &= ~VCB_IS_DIRTY;
            }

            if (NT_SUCCESS(Status))
            {
                Status = VfatDiskShutDown(DeviceExt);
                if (!NT_SUCCESS(Status))
                {
                    DPRINT1("VfatDiskShutDown failed, status = %x\n", Status);
                }
            }
            else
            {
                DPRINT1("VfatFlushVolume failed, status = %x\n", Status);
            }
            ExReleaseResourceLite(&DeviceExt->DirResource);

            /* Unmount the logical volume */
#ifdef ENABLE_SWAPOUT
            VfatCheckForDismount(DeviceExt, FALSE);
#endif

            if (!NT_SUCCESS(Status))
                Irp->IoStatus.Status = Status;
        }
        ExReleaseResourceLite(&VfatGlobalData->VolumeListLock);

        /* FIXME: Free all global acquired resources */

        Status = Irp->IoStatus.Status;
    }
    else
    {
        Irp->IoStatus.Status = STATUS_INVALID_DEVICE_REQUEST;
        Status = STATUS_INVALID_DEVICE_REQUEST;
    }

    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    FsRtlExitFileSystem();

    return Status;
}

/* EOF */
