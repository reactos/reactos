/* $Id: file.c,v 1.19 2002/08/14 20:58:34 dwelch Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/io/file.c
 * PURPOSE:         Graceful system shutdown if a bug is detected
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <internal/io.h>
#include <internal/mm.h>

#define NDEBUG
#include <internal/debug.h>


/* GLOBALS *******************************************************************/

#define TAG_SYSB   TAG('S', 'Y', 'S', 'B')


/* FUNCTIONS *****************************************************************/

NTSTATUS STDCALL
NtQueryInformationFile(HANDLE FileHandle,
		       PIO_STATUS_BLOCK IoStatusBlock,
		       PVOID FileInformation,
		       ULONG Length,
		       FILE_INFORMATION_CLASS FileInformationClass)
{
   PFILE_OBJECT FileObject;
   NTSTATUS Status;
   PIRP Irp;
   PDEVICE_OBJECT DeviceObject;
   PIO_STACK_LOCATION StackPtr;
   PVOID SystemBuffer;
   IO_STATUS_BLOCK IoSB;
   
   assert(IoStatusBlock != NULL);
   assert(FileInformation != NULL);
   
   DPRINT("NtQueryInformationFile(Handle %x StatBlk %x FileInfo %x Length %d "
	  "Class %d)\n", FileHandle, IoStatusBlock, FileInformation,
	  Length, FileInformationClass);
   
   Status = ObReferenceObjectByHandle(FileHandle,
				      FILE_READ_ATTRIBUTES,
				      IoFileObjectType,
				      UserMode,
				      (PVOID *)&FileObject,
				      NULL);
   if (!NT_SUCCESS(Status))
     {
	return(Status);
     }
   DPRINT("FileObject %x\n", FileObject);
   
   DeviceObject = FileObject->DeviceObject;
   
   Irp = IoAllocateIrp(DeviceObject->StackSize,
		       TRUE);
   if (Irp == NULL)
     {
	ObDereferenceObject(FileObject);
	return STATUS_INSUFFICIENT_RESOURCES;
     }
   
   SystemBuffer = ExAllocatePoolWithTag(NonPagedPool,
					Length,
					TAG_SYSB);
   if (SystemBuffer == NULL)
     {
	IoFreeIrp(Irp);
	ObDereferenceObject(FileObject);
	return(STATUS_INSUFFICIENT_RESOURCES);
     }
   
   Irp->AssociatedIrp.SystemBuffer = SystemBuffer;
   Irp->UserIosb = &IoSB;
   Irp->UserEvent = &FileObject->Event;
   KeResetEvent( &FileObject->Event );
   
   StackPtr = IoGetNextIrpStackLocation(Irp);
   StackPtr->MajorFunction = IRP_MJ_QUERY_INFORMATION;
   StackPtr->MinorFunction = 0;
   StackPtr->Flags = 0;
   StackPtr->Control = 0;
   StackPtr->DeviceObject = DeviceObject;
   StackPtr->FileObject = FileObject;
   
   StackPtr->Parameters.QueryFile.FileInformationClass =
     FileInformationClass;
   StackPtr->Parameters.QueryFile.Length = Length;
   
   Status = IoCallDriver(FileObject->DeviceObject,
			 Irp);
   if (Status==STATUS_PENDING && !(FileObject->Flags & FO_SYNCHRONOUS_IO))
     {
	KeWaitForSingleObject(&FileObject->Event,
			      Executive,
			      KernelMode,
			      FALSE,
			      NULL);
	Status = IoSB.Status;
     }
  if (IoStatusBlock)
    {
      *IoStatusBlock = IoSB;
    }

  if (NT_SUCCESS(Status))
    {
      DPRINT("Information %lu\n", IoStatusBlock->Information);
      MmSafeCopyToUser(FileInformation,
		       SystemBuffer,
		       IoStatusBlock->Information);
    }
   
   ExFreePool(SystemBuffer);   
   return(Status);
}


NTSTATUS STDCALL
IoQueryFileInformation(IN PFILE_OBJECT FileObject,
		       IN FILE_INFORMATION_CLASS FileInformationClass,
		       IN ULONG Length,
		       OUT PVOID FileInformation,
		       OUT PULONG ReturnedLength)
{
   IO_STATUS_BLOCK IoStatusBlock;
   PIRP Irp;
   PDEVICE_OBJECT DeviceObject;
   PIO_STACK_LOCATION StackPtr;
   NTSTATUS Status;
   
   assert(FileInformation != NULL)
   
   Status = ObReferenceObjectByPointer(FileObject,
				       FILE_READ_ATTRIBUTES,
				       IoFileObjectType,
				       KernelMode);
   if (!NT_SUCCESS(Status))
     {
	return(Status);
     }
   
   DPRINT("FileObject %x\n", FileObject);
   
   DeviceObject = FileObject->DeviceObject;
   
   Irp = IoAllocateIrp(DeviceObject->StackSize,
		       TRUE);
   if (Irp == NULL)
     {
	ObDereferenceObject(FileObject);
	return STATUS_INSUFFICIENT_RESOURCES;
     }
   
   Irp->AssociatedIrp.SystemBuffer = FileInformation;
   Irp->UserIosb = &IoStatusBlock;
   Irp->UserEvent = &FileObject->Event;
   KeResetEvent( &FileObject->Event );
   
   StackPtr = IoGetNextIrpStackLocation(Irp);
   StackPtr->MajorFunction = IRP_MJ_QUERY_INFORMATION;
   StackPtr->MinorFunction = 0;
   StackPtr->Flags = 0;
   StackPtr->Control = 0;
   StackPtr->DeviceObject = DeviceObject;
   StackPtr->FileObject = FileObject;
   
   StackPtr->Parameters.QueryFile.FileInformationClass =
     FileInformationClass;
   StackPtr->Parameters.QueryFile.Length = Length;
   
   Status = IoCallDriver(FileObject->DeviceObject,
			 Irp);
   if (Status==STATUS_PENDING && !(FileObject->Flags & FO_SYNCHRONOUS_IO))
     {
	KeWaitForSingleObject(&FileObject->Event,
			      Executive,
			      KernelMode,
			      FALSE,
			      NULL);
	Status = IoStatusBlock.Status;
     }
   
   if (ReturnedLength != NULL)
     {
	*ReturnedLength = IoStatusBlock.Information;
     }
   
   
   return Status;
}


NTSTATUS STDCALL
NtSetInformationFile(HANDLE FileHandle,
		     PIO_STATUS_BLOCK IoStatusBlock,
		     PVOID FileInformation,
		     ULONG Length,
		     FILE_INFORMATION_CLASS FileInformationClass)
{
   PIO_STACK_LOCATION StackPtr;
   PFILE_OBJECT FileObject;
   PDEVICE_OBJECT DeviceObject;
   PIRP Irp;
   NTSTATUS Status;
   PVOID SystemBuffer;
   IO_STATUS_BLOCK IoSB;
   
   assert(IoStatusBlock != NULL)
   assert(FileInformation != NULL)
   
   DPRINT("NtSetInformationFile(Handle %x StatBlk %x FileInfo %x Length %d "
	  "Class %d)\n", FileHandle, IoStatusBlock, FileInformation,
	  Length, FileInformationClass);
   
   /*  Get the file object from the file handle  */
   Status = ObReferenceObjectByHandle(FileHandle,
				      FILE_WRITE_ATTRIBUTES,
				      IoFileObjectType,
				      UserMode,
				      (PVOID *)&FileObject,
				      NULL);
   if (!NT_SUCCESS(Status))
     {
	return Status;
     }
   
   DPRINT("FileObject %x\n", FileObject);
   
   DeviceObject = FileObject->DeviceObject;
   
   Irp = IoAllocateIrp(DeviceObject->StackSize,
		       TRUE);
   if (Irp == NULL)
     {
	ObDereferenceObject(FileObject);
	return STATUS_INSUFFICIENT_RESOURCES;
     }
   
   SystemBuffer = ExAllocatePoolWithTag(NonPagedPool,
					Length,
					TAG_SYSB);
   if (SystemBuffer == NULL)
     {
	IoFreeIrp(Irp);
	ObDereferenceObject(FileObject);
	return(STATUS_INSUFFICIENT_RESOURCES);
     }
   
   MmSafeCopyFromUser(SystemBuffer,
		      FileInformation,
		      Length);
   
   Irp->AssociatedIrp.SystemBuffer = SystemBuffer;
   Irp->UserIosb = &IoSB;
   Irp->UserEvent = &FileObject->Event;
   KeResetEvent( &FileObject->Event );
   
   StackPtr = IoGetNextIrpStackLocation(Irp);
   StackPtr->MajorFunction = IRP_MJ_SET_INFORMATION;
   StackPtr->MinorFunction = 0;
   StackPtr->Flags = 0;
   StackPtr->Control = 0;
   StackPtr->DeviceObject = DeviceObject;
   StackPtr->FileObject = FileObject;
   
   StackPtr->Parameters.SetFile.FileInformationClass =
     FileInformationClass;
   StackPtr->Parameters.SetFile.Length = Length;
   
   /*
    * Pass the IRP to the FSD (and wait for
    * it if required)
    */
   DPRINT("FileObject->DeviceObject %x\n", FileObject->DeviceObject);
   Status = IoCallDriver(FileObject->DeviceObject,
			 Irp);
   if (Status == STATUS_PENDING && !(FileObject->Flags & FO_SYNCHRONOUS_IO))
     {
	KeWaitForSingleObject(&FileObject->Event,
			      Executive,
			      KernelMode,
			      FALSE,
			      NULL);
	Status = IoSB.Status;
     }
   if (IoStatusBlock)
     {
       *IoStatusBlock = IoSB;
     }
   ExFreePool(SystemBuffer);
 
   return Status;
}


NTSTATUS STDCALL
NtQueryAttributesFile(IN POBJECT_ATTRIBUTES ObjectAttributes,
		      OUT PFILE_BASIC_INFORMATION FileInformation)
{
   UNIMPLEMENTED;
   return STATUS_NOT_IMPLEMENTED;
}


NTSTATUS STDCALL
NtQueryFullAttributesFile(IN POBJECT_ATTRIBUTES ObjectAttributes,
			  OUT PFILE_NETWORK_OPEN_INFORMATION FileInformation)
{
   UNIMPLEMENTED;
   return STATUS_NOT_IMPLEMENTED;
}


NTSTATUS STDCALL
NtQueryEaFile(IN HANDLE FileHandle,
	      OUT PIO_STATUS_BLOCK IoStatusBlock,
	      OUT PVOID Buffer,
	      IN ULONG Length,
	      IN BOOLEAN ReturnSingleEntry,
	      IN PVOID EaList OPTIONAL,
	      IN ULONG EaListLength,
	      IN PULONG EaIndex OPTIONAL,
	      IN BOOLEAN RestartScan)
{
   UNIMPLEMENTED;
   return STATUS_NOT_IMPLEMENTED;
}


NTSTATUS STDCALL
NtSetEaFile(IN HANDLE FileHandle,
	    IN PIO_STATUS_BLOCK	IoStatusBlock,
	    IN PVOID EaBuffer,
	    IN ULONG EaBufferSize)
{
   UNIMPLEMENTED;
   return STATUS_NOT_IMPLEMENTED;
}

/* EOF */
