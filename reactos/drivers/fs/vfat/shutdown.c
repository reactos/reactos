/* $Id: shutdown.c,v 1.5 2003/02/09 18:02:55 hbirr Exp $
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             services/fs/vfat/shutdown.c
 * PURPOSE:          VFAT Filesystem
 * PROGRAMMER:       Eric Kohl (ekohl@rz-online.de)
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>

#define NDEBUG
#include <debug.h>

#include "vfat.h"

/* FUNCTIONS ****************************************************************/

NTSTATUS STDCALL
VfatShutdown(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
   NTSTATUS Status;
   PLIST_ENTRY ListEntry;
   PDEVICE_EXTENSION DeviceExt;

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
         Status = VfatFlushVolume(DeviceExt, DeviceExt->VolumeFcb);
         ExReleaseResourceLite(&DeviceExt->DirResource);
	 if (!NT_SUCCESS(Status))
	 {
	    DPRINT1("VfatFlushVolume failed, status = %x\n", Status);
	    Irp->IoStatus.Status = Status;
	 }
         /* FIXME: Unmount the logical volume */

	 ExReleaseResourceLite(&VfatGlobalData->VolumeListLock);
      }
      /* FIXME: Free all global acquired resources */

      Status = Irp->IoStatus.Status;
   }
   else
   {
      Status = STATUS_INVALID_DEVICE_REQUEST;
   }

   Irp->IoStatus.Status = Status;
   Irp->IoStatus.Information = 0;

   IoCompleteRequest(Irp, IO_NO_INCREMENT);
   return(Status);
}

/* EOF */
