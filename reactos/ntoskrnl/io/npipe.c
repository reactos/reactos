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
	ULONG ShareAccess,
	ULONG CreateDisposition,
	ULONG CreateOptions,
	BOOLEAN WriteModeMessage,
        BOOLEAN ReadModeMessage,
	BOOLEAN NonBlocking,
	ULONG MaxInstances,
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
			       IoFileObjectType);
   if (FileObject == NULL)
     {
	return(STATUS_UNSUCCESSFUL);
     }
   
   if (CreateOptions & FILE_SYNCHRONOUS_IO_ALERT)
     {
	FileObject->Flags = FileObject->Flags | FO_ALERTABLE_IO;
	FileObject->Flags = FileObject->Flags | FO_SYNCHRONOUS_IO;
     }
   if (CreateOptions & FILE_SYNCHRONOUS_IO_NONALERT)
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
   StackLoc->Parameters.Create.Options = (CreateOptions & FILE_VALID_OPTION_FLAGS);
   StackLoc->Parameters.Create.Options |= (CreateDisposition << 24);
//   StackLoc->Parameters.CreateNamedPipe.CreateDisposition =
//     CreateDisposition;
//   StackLoc->Parameters.CreateNamedPipe.CreateOptions = CreateOptions;
/*
 FIXME : this informations can't be added in Parameters struct because this struct
 must be four WORDs long
   StackLoc->Parameters.CreateNamedPipe.ShareAccess = ShareAccess;
   StackLoc->Parameters.CreateNamedPipe.WriteModeMessage = WriteModeMessage;
   StackLoc->Parameters.CreateNamedPipe.ReadModeMessage = ReadModeMessage;
   StackLoc->Parameters.CreateNamedPipe.NonBlocking = NonBlocking;
   StackLoc->Parameters.CreateNamedPipe.MaxInstances = MaxInstances;
   StackLoc->Parameters.CreateNamedPipe.InBufferSize = InBufferSize;
   StackLoc->Parameters.CreateNamedPipe.OutBufferSize = OutBufferSize;
   if (TimeOut != NULL)
     {
	StackLoc->Parameters.CreateNamedPipe.TimeOut = *TimeOut;
     }
   else
     {
	StackLoc->Parameters.CreateNamedPipe.TimeOut.QuadPart = 0;
     }
*/
   
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
