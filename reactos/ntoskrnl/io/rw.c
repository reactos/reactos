/*
 * COPYRIGHT:      See COPYING in the top level directory
 * PROJECT:        ReactOS kernel
 * FILE:           ntoskrnl/io/rw.c
 * PURPOSE:        Implements read/write APIs
 * PROGRAMMER:     David Welch (welch@cwcom.net)
 * UPDATE HISTORY:
 *                 30/05/98: Created
 */

/* INCLUDES ****************************************************************/

#include <ddk/ntddk.h>
#include <internal/io.h>
#include <string.h>
#include <internal/string.h>
#include <internal/ob.h>

#define NDEBUG
#include <internal/debug.h>

/* FUNCTIONS ***************************************************************/

NTSTATUS NtReadFile(HANDLE FileHandle,
                    HANDLE EventHandle,
		    PIO_APC_ROUTINE ApcRoutine,
		    PVOID ApcContext,
		    PIO_STATUS_BLOCK IoStatusBlock,
		    PVOID Buffer,
		    ULONG Length,
		    PLARGE_INTEGER ByteOffset,
		    PULONG Key)
{
   return(ZwReadFile(FileHandle,
		     EventHandle,
		     ApcRoutine,
		     ApcContext,
		     IoStatusBlock,
		     Buffer,
		     Length,
		     ByteOffset,
		     Key));
}

NTSTATUS ZwReadFile(HANDLE FileHandle,
                    HANDLE EventHandle,
		    PIO_APC_ROUTINE ApcRoutine,
		    PVOID ApcContext,
		    PIO_STATUS_BLOCK IoStatusBlock,
		    PVOID Buffer,
		    ULONG Length,
		    PLARGE_INTEGER ByteOffset,
		    PULONG Key)
{
   NTSTATUS Status;
   PFILE_OBJECT FileObject;
   PIRP Irp;
   PIO_STACK_LOCATION StackPtr;
   PKEVENT ptrEvent = NULL;
   KEVENT Event;
   
   assert(KeGetCurrentIrql()==PASSIVE_LEVEL);
   
   DPRINT("ZwReadFile(FileHandle %x Buffer %x Length %x ByteOffset %x, "
	  "IoStatusBlock %x)\n",
	  FileHandle,Buffer,Length,ByteOffset,IoStatusBlock);
   
   Status = ObReferenceObjectByHandle(FileHandle,
				      FILE_READ_DATA,
				      IoFileType,
				      UserMode,
				      (PVOID *) &FileObject,
				      NULL);
   if (!NT_SUCCESS(STATUS_SUCCESS))
     {
	DPRINT("ZwReadFile() = %x\n",Status);
	return(Status);
     }
   
   DPRINT("ByteOffset %x FileObject->CurrentByteOffset %d\n",
	  ByteOffset,
	  FileObject->CurrentByteOffset.LowPart);
   if (ByteOffset==NULL)
     {
	ByteOffset = &(FileObject->CurrentByteOffset);
     }
   
   if (EventHandle != NULL)
     {
	Status = ObReferenceObjectByHandle(EventHandle,
					   SYNCHRONIZE,
					   ExEventType,
					   UserMode,
					   (PVOID *)ptrEvent,
					   NULL);
	if (!NT_SUCCESS(Status))
	  {
	     return(Status);
	  }
     }
   else
     {
	KeInitializeEvent(&Event,NotificationEvent,FALSE);
	ptrEvent = &Event;
     }
					   
   DPRINT("FileObject %x\n",FileObject);
   Irp = IoBuildSynchronousFsdRequest(IRP_MJ_READ,
				      FileObject->DeviceObject,
				      Buffer,
				      Length,
				      ByteOffset,
				      ptrEvent,
				      IoStatusBlock);

   StackPtr = IoGetNextIrpStackLocation(Irp);
   StackPtr->FileObject = FileObject;
   if (Key!=NULL)
   {
         StackPtr->Parameters.Read.Key = *Key;
   }
   else
   {
        StackPtr->Parameters.Read.Key = 0;
   }
   
   Status = IoCallDriver(FileObject->DeviceObject,Irp);
   if (Status == STATUS_PENDING  && (FileObject->Flags & FO_SYNCHRONOUS_IO))
     {
       KeWaitForSingleObject(&Event,Executive,KernelMode,FALSE,NULL);
       Status = IoStatusBlock->Status;
     }
   DPRINT("ZwReadFile() = %x\n",Status);
   return(Status);
}

NTSTATUS NtWriteFile(HANDLE FileHandle,
		     HANDLE EventHandle,
		     PIO_APC_ROUTINE ApcRoutine,
		     PVOID ApcContext,
		     PIO_STATUS_BLOCK IoStatusBlock,
		     PVOID Buffer,
		     ULONG Length,
		     PLARGE_INTEGER ByteOffset,
		     PULONG Key)
{
   return(ZwWriteFile(FileHandle,
		      EventHandle,
		      ApcRoutine,
		      ApcContext,
		      IoStatusBlock,
		      Buffer,
		      Length,
		      ByteOffset,
		      Key));
}

NTSTATUS ZwWriteFile(HANDLE FileHandle,
		     HANDLE EventHandle,
		     PIO_APC_ROUTINE ApcRoutine,
		     PVOID ApcContext,
		     PIO_STATUS_BLOCK IoStatusBlock,
		     PVOID Buffer,
		     ULONG Length,
		     PLARGE_INTEGER ByteOffset,
		     PULONG Key)
{
   NTSTATUS Status;
   PFILE_OBJECT FileObject;
   PIRP Irp;
   PIO_STACK_LOCATION StackPtr;
   KEVENT Event;
   
   DPRINT("ZwWriteFile(FileHandle %x, Buffer %x, Length %d)\n",
	  FileHandle,Buffer,Length);
   
   Status = ObReferenceObjectByHandle(FileHandle,
				      FILE_WRITE_DATA,
				      IoFileType,
				      UserMode,
				      (PVOID *) &FileObject,
				      NULL);
   if (Status != STATUS_SUCCESS)
     {
	return(Status);
     }
   if (ByteOffset==NULL)
     {
	ByteOffset = &(FileObject->CurrentByteOffset);
     }
   
   KeInitializeEvent(&Event,NotificationEvent,FALSE);
   Irp = IoBuildSynchronousFsdRequest(IRP_MJ_WRITE,
				      FileObject->DeviceObject,
				      Buffer,
				      Length,
				      ByteOffset,
				      &Event,
				      IoStatusBlock);
   DPRINT("FileObject->DeviceObject %x\n",FileObject->DeviceObject);
   StackPtr = IoGetNextIrpStackLocation(Irp);
   StackPtr->FileObject = FileObject;
   if (Key!=NULL)
   {
         StackPtr->Parameters.Write.Key = *Key;
   }
   else
   {
        StackPtr->Parameters.Write.Key = 0;
   }
   Status = IoCallDriver(FileObject->DeviceObject,Irp);
   if (Status == STATUS_PENDING && (FileObject->Flags & FO_SYNCHRONOUS_IO))
     {
	KeWaitForSingleObject(&Event,Executive,KernelMode,FALSE,NULL);
        Status = Irp->IoStatus.Status;
     }
   return(Status);
}

NTSTATUS STDCALL NtReadFileScatter(IN HANDLE FileHandle, 
				   IN HANDLE Event OPTIONAL, 
				   IN PIO_APC_ROUTINE UserApcRoutine OPTIONAL, 
				   IN  PVOID UserApcContext OPTIONAL, 
				   OUT PIO_STATUS_BLOCK UserIoStatusBlock, 
				   IN FILE_SEGMENT_ELEMENT BufferDescription[], 
				   IN ULONG BufferLength, 
				   IN PLARGE_INTEGER ByteOffset, 
				   IN PULONG Key OPTIONAL)
{
   return(ZwReadFileScatter(FileHandle,
			    Event,
			    UserApcRoutine,
			    UserApcContext,
			    UserIoStatusBlock,
			    BufferDescription,
			    BufferLength,
			    ByteOffset,
			    Key));
}

NTSTATUS STDCALL ZwReadFileScatter(IN HANDLE FileHandle, 
				   IN HANDLE Event OPTIONAL, 
				   IN PIO_APC_ROUTINE UserApcRoutine OPTIONAL, 
				   IN  PVOID UserApcContext OPTIONAL, 
				   OUT PIO_STATUS_BLOCK UserIoStatusBlock, 
				   IN FILE_SEGMENT_ELEMENT BufferDescription[],
				   IN ULONG BufferLength, 
				   IN PLARGE_INTEGER ByteOffset, 
				   IN PULONG Key OPTIONAL)
{
   UNIMPLEMENTED;
}


NTSTATUS STDCALL NtWriteFileGather(IN HANDLE FileHandle, 
				   IN HANDLE Event OPTIONAL, 
				   IN PIO_APC_ROUTINE ApcRoutine OPTIONAL, 
				   IN PVOID ApcContext OPTIONAL, 
				   OUT PIO_STATUS_BLOCK IoStatusBlock,
				   IN FILE_SEGMENT_ELEMENT BufferDescription[],
				   IN ULONG BufferLength, 
				   IN PLARGE_INTEGER ByteOffset, 
				   IN PULONG Key OPTIONAL)
{
   return(ZwWriteFileGather(FileHandle,
			    Event,
			    ApcRoutine,
			    ApcContext,
			    IoStatusBlock,
			    BufferDescription,
			    BufferLength,
			    ByteOffset,
			    Key));
}

NTSTATUS STDCALL ZwWriteFileGather(IN HANDLE FileHandle, 
				   IN HANDLE Event OPTIONAL, 
				   IN PIO_APC_ROUTINE ApcRoutine OPTIONAL, 
				   IN PVOID ApcContext OPTIONAL, 
				   OUT PIO_STATUS_BLOCK IoStatusBlock,
				   IN FILE_SEGMENT_ELEMENT BufferDescription[],
				   IN ULONG BufferLength, 
				   IN PLARGE_INTEGER ByteOffset, 
				   IN PULONG Key OPTIONAL)
{
   UNIMPLEMENTED;
}
