/* $Id: page.c,v 1.15 2002/04/07 18:36:13 phreak Exp $
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

NTSTATUS STDCALL IoPageWrite(PFILE_OBJECT FileObject,
			    PMDL Mdl,
			    PLARGE_INTEGER Offset,
			    PIO_STATUS_BLOCK StatusBlock,
			    BOOLEAN PagingIo)
{
   PIRP Irp;
   PIO_STACK_LOCATION StackPtr;
   NTSTATUS Status;
   
   DPRINT("IoPageWrite(FileObject %x, Mdl %x)\n",
	  FileObject, Mdl);
   
   ObReferenceObjectByPointer(FileObject,
			      STANDARD_RIGHTS_REQUIRED,
			      IoFileObjectType,
			      UserMode);
   KeResetEvent( &FileObject->Event );   
   Irp = IoBuildSynchronousFsdRequestWithMdl(IRP_MJ_WRITE,
					     FileObject->DeviceObject,
					     Mdl,
					     Offset,
					     &FileObject->Event,
					     StatusBlock,
					     PagingIo);
   StackPtr = IoGetNextIrpStackLocation(Irp);
   StackPtr->FileObject = FileObject;
   DPRINT("Before IoCallDriver\n");
   Status = IoCallDriver(FileObject->DeviceObject,Irp);
   DPRINT("Status %d STATUS_PENDING %d\n",Status,STATUS_PENDING);
   if (Status == STATUS_PENDING && (FileObject->Flags & FO_SYNCHRONOUS_IO))
     {
	DPRINT("Waiting for io operation\n");
	if (FileObject->Flags & FO_ALERTABLE_IO)
	  {
	     KeWaitForSingleObject(&FileObject->Event,Executive,KernelMode,TRUE,NULL);
	  }
	else
	  {
	     DPRINT("Non-alertable wait\n");
	     KeWaitForSingleObject(&FileObject->Event,Executive,KernelMode,FALSE,NULL);
	  }
	Status = StatusBlock->Status;
     }
   return(Status);
}


NTSTATUS STDCALL 
IoPageRead(PFILE_OBJECT FileObject,
	   PMDL Mdl,
	   PLARGE_INTEGER Offset,
	   PIO_STATUS_BLOCK StatusBlock,
	   BOOLEAN PagingIo)
{
   PIRP Irp;
   PIO_STACK_LOCATION StackPtr;
   NTSTATUS Status;
   
   DPRINT("IoPageRead(FileObject %x, Mdl %x)\n",
	  FileObject, Mdl);
   
   ObReferenceObjectByPointer(FileObject,
			      STANDARD_RIGHTS_REQUIRED,
			      IoFileObjectType,
			      UserMode);
   KeResetEvent( &FileObject->Event );   
   Irp = IoBuildSynchronousFsdRequestWithMdl(IRP_MJ_READ,
					     FileObject->DeviceObject,
					     Mdl,
					     Offset,
					     &FileObject->Event,
					     StatusBlock,
					     PagingIo);
   StackPtr = IoGetNextIrpStackLocation(Irp);
   StackPtr->FileObject = FileObject;
   DPRINT("Before IoCallDriver\n");
   Status = IoCallDriver(FileObject->DeviceObject, Irp);
   DPRINT("Status %d STATUS_PENDING %d\n",Status,STATUS_PENDING);
   if (Status==STATUS_PENDING && (FileObject->Flags & FO_SYNCHRONOUS_IO))
     {
	DPRINT("Waiting for io operation\n");
	if (FileObject->Flags & FO_ALERTABLE_IO)
	  {
	     KeWaitForSingleObject(&FileObject->Event,Executive,KernelMode,TRUE,NULL);
	  }
	else
	  {
	     DPRINT("Non-alertable wait\n");
	     KeWaitForSingleObject(&FileObject->Event,Executive,KernelMode,FALSE,NULL);
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
