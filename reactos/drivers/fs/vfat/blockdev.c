/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             drivers/fs/vfat/blockdev.c
 * PURPOSE:          Temporary sector reading support
 * PROGRAMMER:       David Welch (welch@cwcom.net)
 *                   Hartmut Birr
 * UPDATE HISTORY: 
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>

#define NDEBUG
#include <debug.h>

#include "vfat.h"

/* FUNCTIONS ***************************************************************/

NTSTATUS STDCALL
VfatReadWriteCompletion (IN PDEVICE_OBJECT DeviceObject,
			 IN PIRP Irp,
			 IN PVOID Context)
{
   PMDL Mdl;

   DPRINT("VfatReadBlockDeviceCompletion(DeviceObject %x, Irp %x, Context %x)\n",
          DeviceObject, Irp, Context);

   while ((Mdl = Irp->MdlAddress))
     {
       Irp->MdlAddress = Mdl->Next;
       
       MmUnlockPages(Mdl);
       IoFreeMdl(Mdl);
     }

   *Irp->UserIosb = Irp->IoStatus;
   if (Irp->PendingReturned)
     {
       KeSetEvent(Irp->UserEvent, IO_NO_INCREMENT, FALSE);
     }

   IoFreeIrp(Irp);

   return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS STDCALL
VfatReadWritePartialCompletion (IN PDEVICE_OBJECT DeviceObject,
				IN PIRP Irp,
				IN PVOID Context)
{
  PVFAT_IRP_CONTEXT IrpContext;
  PMDL Mdl;

  DPRINT("VfatReadWritePartialCompletion() called\n");

  IrpContext = (PVFAT_IRP_CONTEXT)Context;

  while ((Mdl = Irp->MdlAddress))
    {
      Irp->MdlAddress = Mdl->Next;
      IoFreeMdl(Mdl);
    }
  if (Irp->PendingReturned)
    {
      IrpContext->Flags |= IRPCONTEXT_PENDINGRETURNED;
    }
  if (!NT_SUCCESS(Irp->IoStatus.Status))
    {
      IrpContext->Irp->IoStatus.Status = Irp->IoStatus.Status;
    }
  if (0 == InterlockedDecrement((PLONG)&IrpContext->RefCount) &&
      IrpContext->Flags & IRPCONTEXT_PENDINGRETURNED)
    {
      KeSetEvent(&IrpContext->Event, IO_NO_INCREMENT, FALSE);
    }
  IoFreeIrp(Irp);

  DPRINT("VfatReadWritePartialCompletion() done\n");

  return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS
VfatReadDisk (IN PDEVICE_OBJECT pDeviceObject,
	      IN PLARGE_INTEGER ReadOffset,
	      IN ULONG ReadLength,
	      IN OUT PUCHAR Buffer,
	      IN BOOLEAN Override)
{
  PIO_STACK_LOCATION Stack;
  PIRP Irp;
  IO_STATUS_BLOCK IoStatus;
  KEVENT event;
  NTSTATUS Status;

  KeInitializeEvent (&event, NotificationEvent, FALSE);

  DPRINT ("VfatReadSectors(pDeviceObject %x, Offset %I64x, Length %d, Buffer %x)\n",
	  pDeviceObject, ReadOffset->QuadPart, ReadLength, Buffer);

  DPRINT ("Building synchronous FSD Request...\n");
  Irp = IoBuildSynchronousFsdRequest (IRP_MJ_READ,
				      pDeviceObject,
				      Buffer,
				      ReadLength,
				      ReadOffset,
				      &event,
				      &IoStatus);
  if (Irp == NULL)
    {
      DPRINT("IoBuildSynchronousFsdRequest failed\n");
      return(STATUS_UNSUCCESSFUL);
    }

  if (Override)
    {
      Stack = IoGetNextIrpStackLocation(Irp);
      Stack->Flags |= SL_OVERRIDE_VERIFY_VOLUME;
    }

  IoSetCompletionRoutine(Irp,
                         VfatReadWriteCompletion,
			 NULL,
			 TRUE,
			 TRUE,
			 TRUE);

  DPRINT ("Calling IO Driver... with irp %x\n", Irp);
  Status = IoCallDriver (pDeviceObject, Irp);

  DPRINT ("Waiting for IO Operation for %x\n", Irp);
  if (Status == STATUS_PENDING)
    {
      DPRINT ("Operation pending\n");
      KeWaitForSingleObject (&event, Suspended, KernelMode, FALSE, NULL);
      DPRINT ("Getting IO Status... for %x\n", Irp);
      Status = IoStatus.Status;
    }

  if (!NT_SUCCESS (Status))
    {
      DPRINT ("IO failed!!! VfatReadSectors : Error code: %x\n", Status);
      DPRINT ("(pDeviceObject %x, Offset %I64x, Size %d, Buffer %x\n",
	      pDeviceObject, ReadOffset->QuadPart, ReadLength, Buffer);
      return (Status);
    }
  DPRINT ("Block request succeeded for %x\n", Irp);
  return (STATUS_SUCCESS);
}

NTSTATUS
VfatReadDiskPartial (IN PVFAT_IRP_CONTEXT IrpContext,
		     IN PLARGE_INTEGER ReadOffset,
		     IN ULONG ReadLength,
		     ULONG BufferOffset,
		     IN BOOLEAN Wait)
{
  PIRP Irp;
  NTSTATUS Status;
  PVOID Buffer;

  DPRINT ("VfatReadDiskPartial(IrpContext %x, ReadOffset %I64x, ReadLength %d, BufferOffset %x, Wait %d)\n",
	  IrpContext, ReadOffset->QuadPart, ReadLength, BufferOffset, Wait);

  DPRINT ("Building synchronous FSD Request...\n");

  Buffer = MmGetMdlVirtualAddress(IrpContext->Irp->MdlAddress) + BufferOffset;

  Irp = IoBuildSynchronousFsdRequest (IRP_MJ_READ,
                                      IrpContext->DeviceExt->StorageDevice,
				      NULL,
				      ReadLength,
				      ReadOffset,
				      NULL,
                                      NULL);
  if (Irp == NULL)
    {
      DPRINT("IoBuildSynchronousFsdRequest failed\n");
      return(STATUS_UNSUCCESSFUL);
    }

  if (!IoAllocateMdl(Buffer, ReadLength, FALSE, FALSE, Irp))
    {
      DPRINT("IoAllocateMdl failed\n");
      IoFreeIrp(Irp);
      return STATUS_UNSUCCESSFUL;
    }

  IoBuildPartialMdl(IrpContext->Irp->MdlAddress, Irp->MdlAddress, Buffer, ReadLength);

  IoSetCompletionRoutine(Irp,
                         VfatReadWritePartialCompletion,
			 IrpContext,
			 TRUE,
			 TRUE,
			 TRUE);

  if (Wait)
    {
      KeInitializeEvent(&IrpContext->Event, NotificationEvent, FALSE);
      IrpContext->RefCount = 1;
    }
  else
    {
      InterlockedIncrement((PLONG)&IrpContext->RefCount);
    }

  DPRINT ("Calling IO Driver... with irp %x\n", Irp);
  Status = IoCallDriver (IrpContext->DeviceExt->StorageDevice, Irp);

  if (Wait && Status == STATUS_PENDING)
    {
      KeWaitForSingleObject(&IrpContext->Event, Executive, KernelMode, FALSE, NULL);
      Status = IrpContext->Irp->IoStatus.Status;
    }

  DPRINT("%x\n", Status);
  return Status;
}


NTSTATUS
VfatWriteDiskPartial (IN PVFAT_IRP_CONTEXT IrpContext,
		      IN PLARGE_INTEGER WriteOffset,
		      IN ULONG WriteLength,
		      IN ULONG BufferOffset,
		      IN BOOLEAN Wait)
{
  PIRP Irp;
  NTSTATUS Status;
  PVOID Buffer;

  DPRINT ("VfatWriteDiskPartial(IrpContext %x, WriteOffset %I64x, WriteLength %d, BufferOffset %x, Wait %d)\n",
	  IrpContext, WriteOffset->QuadPart, WriteLength, BufferOffset, Wait);

  Buffer = MmGetMdlVirtualAddress(IrpContext->Irp->MdlAddress) + BufferOffset;

  DPRINT ("Building synchronous FSD Request...\n");
  Irp = IoBuildSynchronousFsdRequest (IRP_MJ_WRITE,
				      IrpContext->DeviceExt->StorageDevice,
				      NULL,
				      WriteLength,
				      WriteOffset, 
				      NULL, 
				      NULL);

  if (!Irp)
    {
      DPRINT ("WRITE failed!!!\n");
      return (STATUS_UNSUCCESSFUL);
    }

  if (!IoAllocateMdl(Buffer, WriteLength, FALSE, FALSE, Irp))
    {
      DPRINT("IoAllocateMdl failed\n");
      IoFreeIrp(Irp);
      return STATUS_UNSUCCESSFUL;
    }
  IoBuildPartialMdl(IrpContext->Irp->MdlAddress, Irp->MdlAddress, Buffer, WriteLength);

  IoSetCompletionRoutine(Irp,
                         VfatReadWritePartialCompletion,
			 IrpContext,
			 TRUE,
			 TRUE,
			 TRUE);

  if (Wait)
    {
      KeInitializeEvent(&IrpContext->Event, NotificationEvent, FALSE);
      IrpContext->RefCount = 1;
    }
  else
    {
      InterlockedIncrement((PLONG)&IrpContext->RefCount);
    }


  DPRINT ("Calling IO Driver...\n");
  Status = IoCallDriver (IrpContext->DeviceExt->StorageDevice, Irp);
  if (Wait && Status == STATUS_PENDING)
    {
      KeWaitForSingleObject(&IrpContext->Event, Executive, KernelMode, FALSE, NULL);
      Status = IrpContext->Irp->IoStatus.Status;
    }

  return Status;
}

NTSTATUS
VfatBlockDeviceIoControl (IN PDEVICE_OBJECT DeviceObject,
			  IN ULONG CtlCode,
			  IN PVOID InputBuffer,
			  IN ULONG InputBufferSize,
			  IN OUT PVOID OutputBuffer,
			  IN OUT PULONG OutputBufferSize,
			  IN BOOLEAN Override)
{
  PIO_STACK_LOCATION Stack;
  KEVENT Event;
  PIRP Irp;
  IO_STATUS_BLOCK IoStatus;
  NTSTATUS Status;

  DPRINT("VfatBlockDeviceIoControl(DeviceObject %x, CtlCode %x, "
         "InputBuffer %x, InputBufferSize %x, OutputBuffer %x, "
         "OutputBufferSize %x (%x)\n", DeviceObject, CtlCode,
         InputBuffer, InputBufferSize, OutputBuffer, OutputBufferSize,
         OutputBufferSize ? *OutputBufferSize : 0);

  KeInitializeEvent (&Event, NotificationEvent, FALSE);

  DPRINT("Building device I/O control request ...\n");
  Irp = IoBuildDeviceIoControlRequest(CtlCode,
				      DeviceObject,
				      InputBuffer,
				      InputBufferSize,
				      OutputBuffer,
				      (OutputBufferSize) ? *OutputBufferSize : 0,
				      FALSE,
				      &Event,
				      &IoStatus);
  if (Irp == NULL)
    {
      DPRINT("IoBuildDeviceIoControlRequest failed\n");
      return STATUS_INSUFFICIENT_RESOURCES;
    }

  if (Override)
    {
      Stack = IoGetNextIrpStackLocation(Irp);
      Stack->Flags |= SL_OVERRIDE_VERIFY_VOLUME;
    }

  DPRINT ("Calling IO Driver... with irp %x\n", Irp);
  Status = IoCallDriver(DeviceObject, Irp);

  DPRINT ("Waiting for IO Operation for %x\n", Irp);
  if (Status == STATUS_PENDING)
    {
      DPRINT ("Operation pending\n");
      KeWaitForSingleObject (&Event, Suspended, KernelMode, FALSE, NULL);
      DPRINT ("Getting IO Status... for %x\n", Irp);

      Status = IoStatus.Status;
    }

  if (OutputBufferSize)
    {
      *OutputBufferSize = IoStatus.Information;
    }

  DPRINT("Returning Status %x\n", Status);

  return Status;
}
