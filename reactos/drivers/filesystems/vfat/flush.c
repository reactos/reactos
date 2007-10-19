/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             drivers/fs/vfat/flush.c
 * PURPOSE:          VFAT Filesystem
 * PROGRAMMER:
 */

/* INCLUDES *****************************************************************/

#define NDEBUG
#include "vfat.h"

/* FUNCTIONS ****************************************************************/

static NTSTATUS VfatFlushFile(PDEVICE_EXTENSION DeviceExt, PVFATFCB Fcb)
{
   IO_STATUS_BLOCK IoStatus;
   NTSTATUS Status;

   DPRINT("VfatFlushFile(DeviceExt %x, Fcb %x) for '%wZ'\n", DeviceExt, Fcb, &Fcb->PathNameU);

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

NTSTATUS VfatFlushVolume(PDEVICE_EXTENSION DeviceExt, PVFATFCB VolumeFcb)
{
   PLIST_ENTRY ListEntry;
   PVFATFCB Fcb;
   NTSTATUS Status, ReturnStatus = STATUS_SUCCESS;

   DPRINT("VfatFlushVolume(DeviceExt %x, FatFcb %x)\n", DeviceExt, VolumeFcb);

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

   /* FIXME: Flush the buffers from storage device */

   if (!NT_SUCCESS(Status))
   {
      DPRINT1("VfatFlushFile failed, status = %x\n", Status);
      ReturnStatus = Status;
   }

   return ReturnStatus;
}

NTSTATUS VfatFlush(PVFAT_IRP_CONTEXT IrpContext)
{
  NTSTATUS Status;
  PVFATFCB Fcb;
  /*
   * This request is not allowed on the main device object.
   */
  if (IrpContext->DeviceObject == VfatGlobalData->DeviceObject)
  {
     Status = STATUS_INVALID_DEVICE_REQUEST;
     goto ByeBye;
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

ByeBye:
  IrpContext->Irp->IoStatus.Status = Status;
  IrpContext->Irp->IoStatus.Information = 0;
  IoCompleteRequest (IrpContext->Irp, IO_NO_INCREMENT);
  VfatFreeIrpContext(IrpContext);

  return (Status);
}

/* EOF */
