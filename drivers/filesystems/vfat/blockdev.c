/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             drivers/fs/vfat/blockdev.c
 * PURPOSE:          Temporary sector reading support
 * PROGRAMMER:       David Welch (welch@cwcom.net)
 * UPDATE HISTORY:
 */

/* INCLUDES *****************************************************************/

#define NDEBUG
#include "vfat.h"

/* FUNCTIONS ***************************************************************/

static NTSTATUS NTAPI
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

  DPRINT ("VfatReadDisk(pDeviceObject %x, Offset %I64x, Length %d, Buffer %x)\n",
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
      DPRINT ("IO failed!!! VfatReadDisk : Error code: %x\n", Status);
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
  PIO_STACK_LOCATION StackPtr;
  NTSTATUS Status;
  PVOID Buffer;

  DPRINT ("VfatReadDiskPartial(IrpContext %x, ReadOffset %I64x, ReadLength %d, BufferOffset %x, Wait %d)\n",
	  IrpContext, ReadOffset->QuadPart, ReadLength, BufferOffset, Wait);

  DPRINT ("Building asynchronous FSD Request...\n");

  Buffer = (PCHAR)MmGetMdlVirtualAddress(IrpContext->Irp->MdlAddress) + BufferOffset;

  Irp = IoAllocateIrp(IrpContext->DeviceExt->StorageDevice->StackSize, TRUE);
  if (Irp == NULL)
    {
      DPRINT("IoAllocateIrp failed\n");
      return(STATUS_UNSUCCESSFUL);
    }

  Irp->UserIosb = NULL;
  Irp->Tail.Overlay.Thread = PsGetCurrentThread();

  StackPtr = IoGetNextIrpStackLocation(Irp);
  StackPtr->MajorFunction = IRP_MJ_READ;
  StackPtr->MinorFunction = 0;
  StackPtr->Flags = 0;
  StackPtr->Control = 0;
  StackPtr->DeviceObject = IrpContext->DeviceExt->StorageDevice;
  StackPtr->FileObject = NULL;
  StackPtr->CompletionRoutine = NULL;
  StackPtr->Parameters.Read.Length = ReadLength;
  StackPtr->Parameters.Read.ByteOffset = *ReadOffset;

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
  PIO_STACK_LOCATION StackPtr;
  NTSTATUS Status;
  PVOID Buffer;

  DPRINT ("VfatWriteDiskPartial(IrpContext %x, WriteOffset %I64x, WriteLength %d, BufferOffset %x, Wait %d)\n",
	  IrpContext, WriteOffset->QuadPart, WriteLength, BufferOffset, Wait);

  Buffer = (PCHAR)MmGetMdlVirtualAddress(IrpContext->Irp->MdlAddress) + BufferOffset;

  DPRINT ("Building asynchronous FSD Request...\n");
  Irp = IoAllocateIrp(IrpContext->DeviceExt->StorageDevice->StackSize, TRUE);
  if (Irp == NULL)
    {
      DPRINT("IoAllocateIrp failed\n");
      return(STATUS_UNSUCCESSFUL);
    }

  Irp->UserIosb = NULL;
  Irp->Tail.Overlay.Thread = PsGetCurrentThread();

  StackPtr = IoGetNextIrpStackLocation(Irp);
  StackPtr->MajorFunction = IRP_MJ_WRITE;
  StackPtr->MinorFunction = 0;
  StackPtr->Flags = 0;
  StackPtr->Control = 0;
  StackPtr->DeviceObject = IrpContext->DeviceExt->StorageDevice;
  StackPtr->FileObject = NULL;
  StackPtr->CompletionRoutine = NULL;
  StackPtr->Parameters.Read.Length = WriteLength;
  StackPtr->Parameters.Read.ByteOffset = *WriteOffset;

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
			  IN PVOID InputBuffer OPTIONAL,
			  IN ULONG InputBufferSize,
			  IN OUT PVOID OutputBuffer OPTIONAL,
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
