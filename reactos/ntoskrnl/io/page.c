/* $Id: page.c,v 1.7 2000/03/29 13:11:53 dwelch Exp $
 *
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

NTSTATUS STDCALL IoPageRead(PFILE_OBJECT FileObject,
			    PMDL Mdl,
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
			      IoFileObjectType,
			      UserMode);
   
   KeInitializeEvent(&Event,NotificationEvent,FALSE);
   Irp = IoBuildSynchronousFsdRequestWithMdl(IRP_MJ_READ,
					     FileObject->DeviceObject,
					     Mdl,
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


NTSTATUS STDCALL IoSynchronousPageWrite (DWORD	Unknown0,
					 DWORD	Unknown1,
					 DWORD	Unknown2,
					 DWORD	Unknown3,
					 DWORD	Unknown4)
{
   UNIMPLEMENTED;
   return (STATUS_NOT_IMPLEMENTED);
}


/* EOF */
