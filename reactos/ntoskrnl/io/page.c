/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ke/bug.c
 * PURPOSE:         Graceful system shutdown if a bug is detected
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <internal/io.h>

#define NDEBUG
#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

NTSTATUS IoPageRead(PFILE_OBJECT FileObject,
		    PVOID Address,
		    PLARGE_INTEGER Offset,
		    PIO_STATUS_BLOCK StatusBlock)
{
   PIRP Irp;
   KEVENT Event;
   PIO_STACK_LOCATION StackPtr;
   NTSTATUS Status;
   
   DPRINT("IoPageRead(FileObject %x, Address %x)\n",
	  FileObject,Address);
   
   ObReferenceObjectByPointer(FileObject,
			      STANDARD_RIGHTS_REQUIRED,
			      IoFileType,
			      UserMode);
   
   KeInitializeEvent(&Event,NotificationEvent,FALSE);
   Irp = IoBuildSynchronousFsdRequest(IRP_MJ_READ,
				      FileObject->DeviceObject,
				      Address,
				      4096,
				      Offset,
				      &Event,
				      StatusBlock);
   StackPtr = IoGetNextIrpStackLocation(Irp);
   StackPtr->FileObject = FileObject;
   DPRINT("Before IoCallDriver\n");
   Status = IoCallDriver(FileObject->DeviceObject,Irp);
   DPRINT("Status %d STATUS_PENDING %d\n",Status,STATUS_PENDING);
   if (Status==STATUS_PENDING && (FileObject->Flags & FO_SYNCHRONOUS_IO))
     {
	DPRINT("Waiting for io operation\n");
	if (FileObject->Flags & FO_ALERTABLE_IO)
	  {
	     KeWaitForSingleObject(&Event,Executive,KernelMode,TRUE,NULL);
	  }
	else
	  {
	     DPRINT("Non-alertable wait\n");
	     KeWaitForSingleObject(&Event,Executive,KernelMode,FALSE,NULL);
	  }
	Status = StatusBlock->Status;
     }
   return(Status);
}
