/* $Id: rw.c,v 1.36 2002/04/20 03:46:40 phreak Exp $
 *
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
#include <internal/ob.h>

#define NDEBUG
#include <internal/debug.h>

/* DATA ********************************************************************/



/* FUNCTIONS ***************************************************************/


/**********************************************************************
 * NAME							EXPORTED
 *	NtReadFile
 *
 * DESCRIPTION
 *
 * ARGUMENTS
 *
 * RETURN VALUE
 *
 * REVISIONS
 *
 */
NTSTATUS STDCALL NtReadFile(HANDLE			FileHandle,
			    HANDLE			EventHandle,
			    PIO_APC_ROUTINE		ApcRoutine,
			    PVOID			ApcContext,
			    PIO_STATUS_BLOCK	IoStatusBlock,
			    PVOID			Buffer,
			    ULONG			Length,
			    PLARGE_INTEGER		ByteOffset,
			    PULONG			Key)
{
   NTSTATUS Status;
   PFILE_OBJECT FileObject;
   PIRP Irp;
   PIO_STACK_LOCATION StackPtr;
   PKEVENT ptrEvent = NULL;
   IO_STATUS_BLOCK IoSB;
   
   DPRINT("NtReadFile(FileHandle %x Buffer %x Length %x ByteOffset %x, "
	  "IoStatusBlock %x)\n", FileHandle, Buffer, Length, ByteOffset,
	  IoStatusBlock);

   Status = ObReferenceObjectByHandle(FileHandle,
				      FILE_READ_DATA,
				      IoFileObjectType,
				      UserMode,
				      (PVOID*)&FileObject,
				      NULL);
   if( !NT_SUCCESS( Status ) )
     return Status;

   if (ByteOffset == NULL)
     {
	ByteOffset = &FileObject->CurrentByteOffset;
     }
   
   if (EventHandle != NULL)
     {
	Status = ObReferenceObjectByHandle(EventHandle,
					   SYNCHRONIZE,
					   ExEventObjectType,
					   UserMode,
					   (PVOID*)&ptrEvent,
					   NULL);
	if (!NT_SUCCESS(Status))
	  {
	     ObDereferenceObject(FileObject);
	     return Status;
	  }
     }
   else 
     {
       ptrEvent = &FileObject->Event;
       KeResetEvent( ptrEvent );
     }
   
   Irp = IoBuildSynchronousFsdRequest(IRP_MJ_READ,
				      FileObject->DeviceObject,
				      Buffer,
				      Length,
				      ByteOffset,
				      ptrEvent,
				      EventHandle ? IoStatusBlock : &IoSB);
   
   Irp->Overlay.AsynchronousParameters.UserApcRoutine = ApcRoutine;
   Irp->Overlay.AsynchronousParameters.UserApcContext = ApcContext;
   
   StackPtr = IoGetNextIrpStackLocation(Irp);
   StackPtr->FileObject = FileObject;
   if (Key != NULL)
     {
	StackPtr->Parameters.Read.Key = *Key;
     }
   else
     {
	StackPtr->Parameters.Read.Key = 0;
     }
   
   Status = IoCallDriver(FileObject->DeviceObject,
			 Irp);
   if (EventHandle == NULL && Status == STATUS_PENDING && 
       (FileObject->Flags & FO_SYNCHRONOUS_IO))
     {
	BOOLEAN Alertable;
	
	if (FileObject->Flags & FO_ALERTABLE_IO)
	  {
	     Alertable = TRUE;
	  }
	else
	  {
	     Alertable = FALSE;
	  } 
	
	Status = KeWaitForSingleObject(ptrEvent,
				       Executive,
				       KernelMode,
				       Alertable,
				       NULL);
	if( !NT_SUCCESS( Status ) )
	  {
	    DPRINT1( "WaitForSingleObject failed: %x\n", Status );
	  }
	else Status = IoSB.Status;
     }
   if (IoStatusBlock && EventHandle == NULL)
     {
       *IoStatusBlock = IoSB;
     }
   return (Status);
}


/**********************************************************************
 * NAME							EXPORTED
 *	NtWriteFile
 *
 * DESCRIPTION
 *
 * ARGUMENTS
 *
 * RETURN VALUE
 *
 * REVISIONS
 *
 */
NTSTATUS STDCALL NtWriteFile(HANDLE			FileHandle,
			     HANDLE			EventHandle,
			     PIO_APC_ROUTINE		ApcRoutine,
			     PVOID			ApcContext,
			     PIO_STATUS_BLOCK	IoStatusBlock,
			     PVOID			Buffer,
			     ULONG			Length,
			     PLARGE_INTEGER		ByteOffset,
			     PULONG			Key)
{
   NTSTATUS Status;
   PFILE_OBJECT FileObject;
   PIRP Irp;
   PIO_STACK_LOCATION StackPtr;
   PKEVENT ptrEvent;
   IO_STATUS_BLOCK IoSB;
   
   DPRINT("NtWriteFile(FileHandle %x, Buffer %x, Length %d)\n",
	  FileHandle, Buffer, Length);
   
   Status = ObReferenceObjectByHandle(FileHandle,
				      FILE_WRITE_DATA,
				      IoFileObjectType,
				      UserMode,
				      (PVOID*)&FileObject,
				      NULL);
   if (!NT_SUCCESS(Status))
     {
	return(Status);
     }
   
   if (ByteOffset == NULL)
     {
	ByteOffset = &FileObject->CurrentByteOffset;
     }

   if (EventHandle != NULL)
     {
	Status = ObReferenceObjectByHandle(EventHandle,
					   SYNCHRONIZE,
					   ExEventObjectType,
					   UserMode,
					   (PVOID*)&ptrEvent,
					   NULL);
	if (!NT_SUCCESS(Status))
	  {
	     ObDereferenceObject(FileObject);
	     return(Status);
	  }
     }
   else
     {
	ptrEvent = &FileObject->Event;
	KeResetEvent( ptrEvent );
     }

   Irp = IoBuildSynchronousFsdRequest(IRP_MJ_WRITE,
				      FileObject->DeviceObject,
				      Buffer,
				      Length,
				      ByteOffset,
				      ptrEvent,
				      EventHandle ? IoStatusBlock : &IoSB);
   
   Irp->Overlay.AsynchronousParameters.UserApcRoutine = ApcRoutine;
   Irp->Overlay.AsynchronousParameters.UserApcContext = ApcContext;
   
   DPRINT("FileObject->DeviceObject %x\n",FileObject->DeviceObject);
   
   StackPtr = IoGetNextIrpStackLocation(Irp);
   StackPtr->FileObject = FileObject;
   if (Key != NULL)
     {
	StackPtr->Parameters.Write.Key = *Key;
     }
   else
     {
	StackPtr->Parameters.Write.Key = 0;
     }
   Status = IoCallDriver(FileObject->DeviceObject, Irp);
   if (EventHandle == NULL && Status == STATUS_PENDING && 
       !(FileObject->Flags & FO_SYNCHRONOUS_IO))
     {
	KeWaitForSingleObject(ptrEvent,
			      Executive,
			      KernelMode,
			      FileObject->Flags & FO_ALERTABLE_IO ? TRUE : FALSE,
			      NULL);
	Status = IoSB.Status;
     }
   if (IoStatusBlock && EventHandle == NULL)
     {
       *IoStatusBlock = IoSB;
     }
   return(Status);
}


/**********************************************************************
 * NAME							EXPORTED
 *	NtReadFileScatter
 *	
 * DESCRIPTION
 *
 * ARGUMENTS
 *
 * RETURN VALUE
 *
 * REVISIONS
 *
 */
NTSTATUS
STDCALL
NtReadFileScatter (
	IN	HANDLE			FileHandle, 
	IN	HANDLE			Event			OPTIONAL, 
	IN	PIO_APC_ROUTINE		UserApcRoutine		OPTIONAL, 
	IN	PVOID			UserApcContext		OPTIONAL, 
	OUT	PIO_STATUS_BLOCK	UserIoStatusBlock, 
	IN	FILE_SEGMENT_ELEMENT	BufferDescription [], 
	IN	ULONG			BufferLength, 
	IN	PLARGE_INTEGER		ByteOffset, 
	IN	PULONG			Key			OPTIONAL
	)
{
	UNIMPLEMENTED;
}


/**********************************************************************
 * NAME							EXPORTED
 *	NtWriteFileGather
 *	
 * DESCRIPTION
 *
 * ARGUMENTS
 *
 * RETURN VALUE
 *
 * REVISIONS
 *
 */
NTSTATUS
STDCALL
NtWriteFileGather (
	IN	HANDLE			FileHandle, 
	IN	HANDLE			Event OPTIONAL, 
	IN	PIO_APC_ROUTINE		ApcRoutine		OPTIONAL, 
	IN	PVOID			ApcContext		OPTIONAL, 
	OUT	PIO_STATUS_BLOCK	IoStatusBlock,
	IN	FILE_SEGMENT_ELEMENT	BufferDescription [],
	IN	ULONG			BufferLength, 
	IN	PLARGE_INTEGER		ByteOffset, 
	IN	PULONG			Key			OPTIONAL
	)
{
	UNIMPLEMENTED;
}


/* EOF */
