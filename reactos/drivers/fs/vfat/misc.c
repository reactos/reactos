/* $Id: misc.c,v 1.6 2003/02/09 18:02:55 hbirr Exp $
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             services/fs/vfat/misc.c
 * PURPOSE:          VFAT Filesystem
 * PROGRAMMER:       Hartmut Birr
 *
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <wchar.h>

#define NDEBUG
#include <debug.h>

#include "vfat.h"

/* FUNCTIONS ****************************************************************/

static LONG QueueCount = 0;

NTSTATUS VfatDispatchRequest (
        IN PVFAT_IRP_CONTEXT IrpContext)
{
   DPRINT ("VfatDispatchRequest (IrpContext %x), MajorFunction %x\n", IrpContext, IrpContext->MajorFunction);

   assert (IrpContext);

   switch (IrpContext->MajorFunction)
   {
      case IRP_MJ_CLOSE:
         return VfatClose (IrpContext);
      case IRP_MJ_CREATE:
         return VfatCreate (IrpContext);
      case IRP_MJ_READ:
         return VfatRead (IrpContext);
      case IRP_MJ_WRITE:
         return VfatWrite (IrpContext);
      case IRP_MJ_FILE_SYSTEM_CONTROL:
         return VfatFileSystemControl(IrpContext);
      case IRP_MJ_QUERY_INFORMATION:
         return VfatQueryInformation (IrpContext);
      case IRP_MJ_SET_INFORMATION:
         return VfatSetInformation (IrpContext);
      case IRP_MJ_DIRECTORY_CONTROL:
         return VfatDirectoryControl(IrpContext);
      case IRP_MJ_QUERY_VOLUME_INFORMATION:
         return VfatQueryVolumeInformation(IrpContext);
      case IRP_MJ_SET_VOLUME_INFORMATION:
         return VfatSetVolumeInformation(IrpContext);
      case IRP_MJ_LOCK_CONTROL:
         return VfatLockControl(IrpContext);
      case IRP_MJ_CLEANUP:
         return VfatCleanup(IrpContext);
      case IRP_MJ_FLUSH_BUFFERS:
         return VfatFlush(IrpContext);
      default:
         DPRINT1 ("Unexpected major function %x\n", IrpContext->MajorFunction);
         IrpContext->Irp->IoStatus.Status = STATUS_DRIVER_INTERNAL_ERROR;
         IoCompleteRequest(IrpContext->Irp, IO_NO_INCREMENT);
         VfatFreeIrpContext(IrpContext);
         return STATUS_DRIVER_INTERNAL_ERROR;
   }
}

NTSTATUS VfatLockControl(
   IN PVFAT_IRP_CONTEXT IrpContext
   )
{
   PVFATFCB Fcb;
   PVFATCCB Ccb;
   NTSTATUS Status;

   DPRINT("VfatLockControl(IrpContext %x)\n", IrpContext);
 
   assert(IrpContext);

   Ccb = (PVFATCCB)IrpContext->FileObject->FsContext2;
   Fcb = Ccb->pFcb;

   if (IrpContext->DeviceObject == VfatGlobalData->DeviceObject)
   {
      Status = STATUS_INVALID_DEVICE_REQUEST;
      goto Fail;
   }

   if (Fcb->entry.Attrib & FILE_ATTRIBUTE_DIRECTORY)
   {
      Status = STATUS_INVALID_PARAMETER;
      goto Fail;
   }

   Status = FsRtlProcessFileLock(&Fcb->FileLock,
                                 IrpContext->Irp,
                                 NULL
                                 );

   VfatFreeIrpContext(IrpContext);
   return Status;

Fail:;
   IrpContext->Irp->IoStatus.Status = Status;
   IofCompleteRequest(IrpContext->Irp, NT_SUCCESS(Status) ? IO_DISK_INCREMENT : IO_NO_INCREMENT);
   VfatFreeIrpContext(IrpContext);
   return Status;
}

NTSTATUS STDCALL VfatBuildRequest (
        IN PDEVICE_OBJECT DeviceObject,
        IN PIRP Irp)
{
   NTSTATUS Status;
   PVFAT_IRP_CONTEXT IrpContext;

   DPRINT ("VfatBuildRequest (DeviceObject %x, Irp %x)\n", DeviceObject, Irp);

   assert (DeviceObject);
   assert (Irp);

   FsRtlEnterFileSystem();
   IrpContext = VfatAllocateIrpContext(DeviceObject, Irp);
   if (IrpContext == NULL)
   {
      Status = STATUS_INSUFFICIENT_RESOURCES;
      Irp->IoStatus.Status = Status;
      IoCompleteRequest (Irp, IO_NO_INCREMENT);
   }
   else
   {
      Status = VfatDispatchRequest (IrpContext);
   }
   FsRtlExitFileSystem();
   return Status;
}

VOID VfatFreeIrpContext (PVFAT_IRP_CONTEXT IrpContext)
{
   assert (IrpContext);
   ExFreeToNPagedLookasideList(&VfatGlobalData->IrpContextLookasideList, IrpContext);
}

PVFAT_IRP_CONTEXT VfatAllocateIrpContext(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
   PVFAT_IRP_CONTEXT IrpContext;
   PIO_STACK_LOCATION Stack;
   UCHAR MajorFunction;
   DPRINT ("VfatAllocateIrpContext(DeviceObject %x, Irp %x)\n", DeviceObject, Irp);

   assert (DeviceObject);
   assert (Irp);

   IrpContext = ExAllocateFromNPagedLookasideList(&VfatGlobalData->IrpContextLookasideList);
   if (IrpContext)
   {
      RtlZeroMemory(IrpContext, sizeof(IrpContext));
      IrpContext->Irp = Irp;
      IrpContext->DeviceObject = DeviceObject;
      IrpContext->DeviceExt = DeviceObject->DeviceExtension;
      IrpContext->Stack = IoGetCurrentIrpStackLocation(Irp);
      assert (IrpContext->Stack);
      MajorFunction = IrpContext->MajorFunction = IrpContext->Stack->MajorFunction;
      IrpContext->MinorFunction = IrpContext->Stack->MinorFunction;
      IrpContext->FileObject = IrpContext->Stack->FileObject;
      if (MajorFunction == IRP_MJ_FILE_SYSTEM_CONTROL ||
          MajorFunction == IRP_MJ_DEVICE_CONTROL ||
          MajorFunction == IRP_MJ_SHUTDOWN)
      {
         IrpContext->Flags |= IRPCONTEXT_CANWAIT;
      }
      else if (MajorFunction != IRP_MJ_CLEANUP &&
               MajorFunction != IRP_MJ_CLOSE &&
               IoIsOperationSynchronous(Irp))
      {
         IrpContext->Flags |= IRPCONTEXT_CANWAIT;
      }
   }
   return IrpContext;
}

VOID STDCALL VfatDoRequest (PVOID IrpContext)
{
   ULONG Count = InterlockedDecrement(&QueueCount);
   DPRINT ("VfatDoRequest (IrpContext %x), MajorFunction %x, %d\n", IrpContext, ((PVFAT_IRP_CONTEXT)IrpContext)->MajorFunction, Count);
   VfatDispatchRequest((PVFAT_IRP_CONTEXT)IrpContext);

}

NTSTATUS VfatQueueRequest(PVFAT_IRP_CONTEXT IrpContext)
{
   ULONG Count = InterlockedIncrement(&QueueCount);
   DPRINT ("VfatQueueRequest (IrpContext %x), %d\n", IrpContext, Count);

   assert (IrpContext != NULL);
   assert (IrpContext->Irp != NULL);

   IrpContext->Flags |= IRPCONTEXT_CANWAIT;
   IoMarkIrpPending (IrpContext->Irp);
   ExInitializeWorkItem (&IrpContext->WorkQueueItem, VfatDoRequest, IrpContext);
   ExQueueWorkItem(&IrpContext->WorkQueueItem, CriticalWorkQueue);
   return STATUS_PENDING;
}

PVOID VfatGetUserBuffer(IN PIRP Irp)
{
   assert(Irp);

   if (Irp->MdlAddress)
   {
      return MmGetSystemAddressForMdl(Irp->MdlAddress);
   }
   else
   {
      return Irp->UserBuffer;
   }
}

NTSTATUS VfatLockUserBuffer(IN PIRP Irp, IN ULONG Length, IN LOCK_OPERATION Operation)
{
   assert(Irp);

   if (Irp->MdlAddress)
   {
      return STATUS_SUCCESS;
   }

   IoAllocateMdl(Irp->UserBuffer, Length, FALSE, FALSE, Irp);

   if (!Irp->MdlAddress)
   {
      return STATUS_INSUFFICIENT_RESOURCES;
   }

   MmProbeAndLockPages(Irp->MdlAddress, Irp->RequestorMode, Operation);

   return STATUS_SUCCESS;
}


