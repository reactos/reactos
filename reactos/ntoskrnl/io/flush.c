/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/io/flush.c
 * PURPOSE:         Flushing file buffer
 * PROGRAMMER:      David Welch (welch@cwcom.net)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <internal/ob.h>

#define NDEBUG
#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/


NTSTATUS
STDCALL
NtFlushWriteBuffer(VOID)
{
	UNIMPLEMENTED;
}

NTSTATUS
STDCALL
NtFlushBuffersFile (
	IN	HANDLE			FileHandle,
	OUT	PIO_STATUS_BLOCK	IoStatusBlock
	)
/*
 * FUNCTION: Flushes cached file data to disk
 * ARGUMENTS:
 *       FileHandle = Points to the file
 *	 IoStatusBlock = Caller must supply storage to receive the result of 
 *                       the flush buffers operation. The information field is
 *                       set to number of bytes flushed to disk.
 * RETURNS: Status 
 * REMARKS: This function maps to the win32 FlushFileBuffers
 */
{
   PFILE_OBJECT FileObject = NULL;
   PIRP Irp;
   PIO_STACK_LOCATION StackPtr;
   KEVENT Event;
   NTSTATUS Status;
      
   Status = ObReferenceObjectByHandle(FileHandle,
				      FILE_WRITE_DATA,
				      NULL,
				      UserMode,
				      (PVOID*)&FileObject,
				      NULL);
   if (Status != STATUS_SUCCESS)
     {
	return(Status);
     }
   
   KeInitializeEvent(&Event,NotificationEvent,FALSE);
   Irp = IoBuildSynchronousFsdRequest(IRP_MJ_FLUSH_BUFFERS,
				      FileObject->DeviceObject,
				      NULL,
				      0,
				      NULL,
				      &Event,
				      IoStatusBlock);

   StackPtr = IoGetNextIrpStackLocation(Irp);
   StackPtr->FileObject = FileObject;

   Status = IoCallDriver(FileObject->DeviceObject,Irp);
   if (Status==STATUS_PENDING)
     {
	KeWaitForSingleObject(&Event,Executive,KernelMode,FALSE,NULL);
	Status = Irp->IoStatus.Status;
     }
   return(Status);
}
