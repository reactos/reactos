/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             drivers/fs/vfat/shutdown.c
 * PURPOSE:          VFAT Filesystem
 * PROGRAMMER:       Eric Kohl (ekohl@rz-online.de)
 */

/* INCLUDES *****************************************************************/

#define NDEBUG
#include "vfat.h"

/* FUNCTIONS ****************************************************************/

static NTSTATUS
VfatDiskShutDown(PVCB Vcb)
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
      Status = IoStatus.Status;
   }

   return Status;
}

NTSTATUS NTAPI
VfatShutdown(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
   NTSTATUS Status;
   PLIST_ENTRY ListEntry;
   PDEVICE_EXTENSION DeviceExt;
   ULONG eocMark;

   DPRINT("VfatShutdown(DeviceObject %x, Irp %x)\n",DeviceObject, Irp);

   /* FIXME: block new mount requests */

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
         if (DeviceExt->VolumeFcb->Flags & VCB_CLEAR_DIRTY)
         {
            /* set clean shutdown bit */
            Status = GetNextCluster(DeviceExt, 1, &eocMark);
            if (NT_SUCCESS(Status))
            {
               eocMark |= DeviceExt->CleanShutBitMask;
               if (NT_SUCCESS(WriteCluster(DeviceExt, 1, eocMark)))
                  DeviceExt->VolumeFcb->Flags &= ~VCB_IS_DIRTY;
            }
         }
         Status = VfatFlushVolume(DeviceExt, DeviceExt->VolumeFcb);
         if (NT_SUCCESS(Status))
         {
            Status = VfatDiskShutDown(DeviceExt);
            if (!NT_SUCCESS(Status))
	       DPRINT1("VfatDiskShutDown failed, status = %x\n", Status);
         }
         else
         {
	    DPRINT1("VfatFlushVolume failed, status = %x\n", Status);
	 }
         ExReleaseResourceLite(&DeviceExt->DirResource);

         /* FIXME: Unmount the logical volume */

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

   return(Status);
}

/* EOF */
