/* $Id: misc.c,v 1.1 2001/11/02 22:44:34 hbirr Exp $
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
      case IRP_MJ_CLEANUP:
         return VfatCleanup(IrpContext);
      default:
         DPRINT1 ("Unexpected major function %x\n", IrpContext->MajorFunction);
         IrpContext->Irp->IoStatus.Status = STATUS_DRIVER_INTERNAL_ERROR;
         IoCompleteRequest(IrpContext->Irp, IO_NO_INCREMENT);
         VfatFreeIrpContext(IrpContext);
         return STATUS_DRIVER_INTERNAL_ERROR;
   }
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
   ExFreePool(IrpContext);
}

// Copyed from ntoskrnl\io\irp.c and changed access to FileObject
BOOLEAN
STDCALL
VfatIoIsOperationSynchronous (
	IN	PIRP	Irp
	)
{
	ULONG		Flags = 0;
	PFILE_OBJECT	FileObject = NULL;
	PIO_STACK_LOCATION Stack;
	/*
	 * Check the associated FILE_OBJECT's
	 * flags first.
	 */
//	FileObject = Irp->Tail.Overlay.OriginalFileObject;
        Stack = IoGetCurrentIrpStackLocation(Irp);
        FileObject = Stack->FileObject;

        assert (FileObject);
	if (!(FO_SYNCHRONOUS_IO & FileObject->Flags))
	{
		/* Check IRP's flags. */
		Flags = Irp->Flags;
		if (!(	(IRP_SYNCHRONOUS_API | IRP_SYNCHRONOUS_PAGING_IO)
			& Flags
			))
		{
			return FALSE;
		}
	}
	/*
	 * Check more IRP's flags.
	 */
	Flags = Irp->Flags;
	if (	!(IRP_MOUNT_COMPLETION & Flags)
		|| (IRP_SYNCHRONOUS_PAGING_IO & Flags)
		)
	{
		return TRUE;
	}
	/*
	 * Otherwise, it is an
	 * asynchronous operation.
	 */
	return FALSE;
}

PVFAT_IRP_CONTEXT VfatAllocateIrpContext(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
   PVFAT_IRP_CONTEXT IrpContext;
   PIO_STACK_LOCATION Stack;
   UCHAR MajorFunction;
   DPRINT ("VfatAllocateIrpContext(DeviceObject %x, Irp %x)\n", DeviceObject, Irp);

   assert (DeviceObject);
   assert (Irp);

   IrpContext = ExAllocatePool (NonPagedPool, sizeof(VFAT_IRP_CONTEXT));
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
               VfatIoIsOperationSynchronous(Irp))
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



