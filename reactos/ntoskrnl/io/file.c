/*
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

//#define NDEBUG
#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

NTSTATUS STDCALL NtQueryInformationFile(HANDLE FileHandle,
					PIO_STATUS_BLOCK IoStatusBlock,
					PVOID FileInformation,
					ULONG Length,
					FILE_INFORMATION_CLASS FileInformationClass)
{
  return ZwQueryInformationFile(FileHandle,
                                IoStatusBlock,
                                FileInformation,
                                Length,
                                FileInformationClass);
}

NTSTATUS STDCALL ZwQueryInformationFile(HANDLE FileHandle,
					PIO_STATUS_BLOCK IoStatusBlock,
					PVOID FileInformation,
					ULONG Length,
					FILE_INFORMATION_CLASS FileInformationClass)
{
  NTSTATUS Status;
  PFILE_OBJECT FileObject;
  PIRP Irp;
  PIO_STACK_LOCATION StackPtr;
  KEVENT Event;
   
  DPRINT("ZwQueryInformation(Handle %x StatBlk %x FileInfo %x Length %d Class %d)\n",
         FileHandle,
         IoStatusBlock,
         FileInformation,
         Length,
         FileInformationClass);
   
  /*  Get the file object from the file handle  */
  Status = ObReferenceObjectByHandle(FileHandle,
                                     FILE_READ_ATTRIBUTES,
                                     IoFileType,
                                     UserMode,
                                     (PVOID *) &FileObject,
                                     NULL);
  if (!NT_SUCCESS(Status))
    {
      return Status;
    }
  DPRINT("FileObject %x\n", FileObject);
   
  /*  initialize an event object to wait on for the request  */
  KeInitializeEvent(&Event, NotificationEvent, FALSE);

  /*  build the IRP to be sent to the driver for the request  */
  Irp = IoBuildSynchronousFsdRequest(IRP_MJ_QUERY_INFORMATION,
                                     FileObject->DeviceObject,
                                     FileInformation,
                                     Length,
                                     0,
                                     &Event,
                                     IoStatusBlock);
  StackPtr = IoGetNextIrpStackLocation(Irp);
  StackPtr->FileObject = FileObject;
  StackPtr->Parameters.QueryFile.Length = Length;
  StackPtr->Parameters.QueryFile.FileInformationClass = FileInformationClass;
   
  /*  Pass the IRP to the FSD (and wait for it if required) */
  DPRINT("FileObject->DeviceObject %x\n", FileObject->DeviceObject);
  Status = IoCallDriver(FileObject->DeviceObject, Irp);
  if (Status == STATUS_PENDING  && (FileObject->Flags & FO_SYNCHRONOUS_IO))
    {
      KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
      Status = Irp->IoStatus.Status;
    } 

  return Status;
}

NTSTATUS NtSetInformationFile(HANDLE FileHandle,
			      PIO_STATUS_BLOCK IoStatusBlock,
			      PVOID FileInformation,
			      ULONG Length,
			      FILE_INFORMATION_CLASS FileInformationClass)
{
  return ZwSetInformationFile(FileHandle,
                              IoStatusBlock,
                              FileInformation,
                              Length,
                              FileInformationClass);
}

NTSTATUS ZwSetInformationFile(HANDLE FileHandle,
			      PIO_STATUS_BLOCK IoStatusBlock,
			      PVOID FileInformation,
			      ULONG Length,
			      FILE_INFORMATION_CLASS FileInformationClass)
{
  NTSTATUS Status;
  PFILE_OBJECT FileObject;
  PIRP Irp;
  PIO_STACK_LOCATION StackPtr;
  KEVENT Event;
   
  DPRINT("ZwSetInformation(Handle %x StatBlk %x FileInfo %x Length %d Class %d)\n",
         FileHandle,
         IoStatusBlock,
         FileInformation,
         Length,
         FileInformationClass);
   
  /*  Get the file object from the file handle  */
  Status = ObReferenceObjectByHandle(FileHandle,
                                     FILE_WRITE_ATTRIBUTES,
                                     IoFileType,
                                     UserMode,
                                     (PVOID *) &FileObject,
                                     NULL);
  if (!NT_SUCCESS(Status))
    {
      return Status;
    }
  DPRINT("FileObject %x\n", FileObject);
   
  /*  initialize an event object to wait on for the request  */
  KeInitializeEvent(&Event, NotificationEvent, FALSE);

  /*  build the IRP to be sent to the driver for the request  */
  Irp = IoBuildSynchronousFsdRequest(IRP_MJ_SET_INFORMATION,
                                     FileObject->DeviceObject,
                                     FileInformation,
                                     Length,
                                     0,
                                     &Event,
                                     IoStatusBlock);
  StackPtr = IoGetNextIrpStackLocation(Irp);
  StackPtr->FileObject = FileObject;
  StackPtr->Parameters.SetFile.Length = Length;
  StackPtr->Parameters.SetFile.FileInformationClass = FileInformationClass;
   
  /*  Pass the IRP to the FSD (and wait for it if required) */
  DPRINT("FileObject->DeviceObject %x\n", FileObject->DeviceObject);
  Status = IoCallDriver(FileObject->DeviceObject, Irp);
  if (Status == STATUS_PENDING  && (FileObject->Flags & FO_SYNCHRONOUS_IO))
    {
      KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
      Status = Irp->IoStatus.Status;
    } 

  return Status;
}

PGENERIC_MAPPING IoGetFileObjectGenericMapping(VOID)
{
  UNIMPLEMENTED;
}

NTSTATUS STDCALL NtQueryAttributesFile(IN HANDLE FileHandle,
				       IN PVOID Buffer)
{
  return ZwQueryAttributesFile(FileHandle, Buffer);
}

NTSTATUS STDCALL ZwQueryAttributesFile(IN HANDLE FileHandle, IN PVOID Buffer)
{
  UNIMPLEMENTED;
}

NTSTATUS STDCALL NtQueryFullAttributesFile(IN HANDLE FileHandle, 
					   IN PVOID Attributes)
{
  return ZwQueryFullAttributesFile(FileHandle, Attributes);
}

NTSTATUS STDCALL ZwQueryFullAttributesFile(IN HANDLE FileHandle, 
					   IN PVOID Attributes)
{
  UNIMPLEMENTED;
}

NTSTATUS STDCALL NtQueryEaFile(IN HANDLE FileHandle,
			       OUT PIO_STATUS_BLOCK IoStatusBlock,
			       OUT PVOID Buffer,
			       IN ULONG Length,
			       IN BOOLEAN ReturnSingleEntry,
			       IN PVOID EaList OPTIONAL,
			       IN ULONG EaListLength,
			       IN PULONG EaIndex OPTIONAL,
			       IN BOOLEAN RestartScan)
{
  return NtQueryEaFile(FileHandle, 
                       IoStatusBlock, 
                       Buffer,
                       Length,
                       ReturnSingleEntry,
                       EaList,
                       EaListLength,
                       EaIndex,
                       RestartScan);
}

NTSTATUS STDCALL ZwQueryEaFile(IN HANDLE FileHandle,
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
}

NTSTATUS STDCALL NtSetEaFile(IN HANDLE FileHandle,
			     IN PIO_STATUS_BLOCK IoStatusBlock,	
			     PVOID EaBuffer, 
			     ULONG EaBufferSize)
{
  return ZwSetEaFile(FileHandle,
                     IoStatusBlock,	
                     EaBuffer, 
                     EaBufferSize);
}

NTSTATUS STDCALL ZwSetEaFile(IN HANDLE FileHandle,
			     IN PIO_STATUS_BLOCK IoStatusBlock,	
			     PVOID EaBuffer, 
			     ULONG EaBufferSize)
{
  UNIMPLEMENTED;
}


