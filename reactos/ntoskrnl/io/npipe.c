/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/io/npipe.c
 * PURPOSE:         Named pipe helper function
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <internal/ob.h>
#include <internal/io.h>

#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

NTSTATUS STDCALL NtCreateNamedPipeFile(
	OUT	PHANDLE			NamedPipeFileHandle,
	IN	ACCESS_MASK		DesiredAccess,               
	IN	POBJECT_ATTRIBUTES	ObjectAttributes,     
	OUT	PIO_STATUS_BLOCK	IoStatusBlock,    
	IN	ULONG			FileAttributes,
	IN	ULONG			ShareAccess,
	IN	ULONG			OpenMode,  
	IN	ULONG			PipeType, 
	IN	ULONG			PipeRead, 
	IN	ULONG			PipeWait, 
	IN	ULONG			MaxInstances,
	IN	ULONG			InBufferSize,
	IN	ULONG			OutBufferSize,
	IN	PLARGE_INTEGER		TimeOut)
{
   PFILE_OBJECT FileObject;
   NTSTATUS Status;
   PIRP Irp;
   KEVENT Event;
   PIO_STACK_LOCATION StackLoc;
   
   DPRINT1("NtCreateNamedPipeFile(FileHandle %x, DesiredAccess %x, "
	    "ObjectAttributes %x ObjectAttributes->ObjectName->Buffer %S)\n",
	    NamedPipeFileHandle,DesiredAccess,ObjectAttributes,
	    ObjectAttributes->ObjectName->Buffer);   
   
   assert_irql(PASSIVE_LEVEL);
   
   *NamedPipeFileHandle=0;

   FileObject = ObCreateObject(NamedPipeFileHandle,
			       DesiredAccess,
			       ObjectAttributes,
			       IoFileType);
   if (FileObject == NULL)
     {
	return(STATUS_UNSUCCESSFUL);
     }
   
   if (OpenMode & FILE_SYNCHRONOUS_IO_ALERT)
     {
	FileObject->Flags = FileObject->Flags | FO_ALERTABLE_IO;
	FileObject->Flags = FileObject->Flags | FO_SYNCHRONOUS_IO;
     }
   if (OpenMode & FILE_SYNCHRONOUS_IO_NONALERT)
     {
	FileObject->Flags = FileObject->Flags | FO_SYNCHRONOUS_IO;
     }
      
   KeInitializeEvent(&Event, NotificationEvent, FALSE);
   
   DPRINT("FileObject %x\n", FileObject);
   DPRINT("FileObject->DeviceObject %x\n", FileObject->DeviceObject);
   Irp = IoAllocateIrp(FileObject->DeviceObject->StackSize, FALSE);
   if (Irp==NULL)
     {
	return(STATUS_UNSUCCESSFUL);
     }
   
   StackLoc = IoGetNextIrpStackLocation(Irp);
   StackLoc->MajorFunction = IRP_MJ_CREATE_NAMED_PIPE;
   StackLoc->MinorFunction = 0;
   StackLoc->Flags = 0;
   StackLoc->Control = 0;
   StackLoc->DeviceObject = FileObject->DeviceObject;
   StackLoc->FileObject = FileObject;
   StackLoc->Parameters.CreateNamedPipe.FileAttributes = FileAttributes;
   StackLoc->Parameters.CreateNamedPipe.OpenMode = OpenMode;
   StackLoc->Parameters.CreateNamedPipe.PipeType = PipeType;
   StackLoc->Parameters.CreateNamedPipe.PipeRead = PipeRead;
   StackLoc->Parameters.CreateNamedPipe.PipeWait = PipeWait;
   StackLoc->Parameters.CreateNamedPipe.MaxInstances = MaxInstances;
   StackLoc->Parameters.CreateNamedPipe.InBufferSize = InBufferSize;
   StackLoc->Parameters.CreateNamedPipe.OutBufferSize = OutBufferSize;
   if (TimeOut != NULL)
     {
	StackLoc->Parameters.CreateNamedPipe.Timeout = *TimeOut;
     }
   else
     {
	StackLoc->Parameters.CreateNamedPipe.Timeout.QuadPart = 0;
     }
   
   Status = IoCallDriver(FileObject->DeviceObject,Irp);
   if (Status == STATUS_PENDING)
     {
	KeWaitForSingleObject(&Event,Executive,KernelMode,FALSE,NULL);
	Status = IoStatusBlock->Status;
     }
   
   if (!NT_SUCCESS(Status))
     {
	DPRINT("Failing create request with status %x\n",Status);
	ZwClose(*NamedPipeFileHandle);
	(*NamedPipeFileHandle) = 0;
     }
   
   assert_irql(PASSIVE_LEVEL);
   DPRINT("Finished NtCreateFile() (*FileHandle) %x\n",
	  (*NamedPipeFileHandle));
   return(Status);

}
