/* $Id: finfo.c,v 1.4 2002/09/07 15:12:02 chorns Exp $
 *
 * COPYRIGHT:  See COPYING in the top level directory
 * PROJECT:    ReactOS kernel
 * FILE:       services/fs/ms/finfo.c
 * PURPOSE:    Mailslot filesystem
 * PROGRAMMER: Eric Kohl <ekohl@rz-online.de>
 */

/* INCLUDES ******************************************************************/

#include "msfs.h"

#define NDEBUG
#include <debug.h>


/* FUNCTIONS *****************************************************************/

static NTSTATUS
MsfsQueryMailslotInformation(PMSFS_FCB Fcb,
			     PFILE_MAILSLOT_QUERY_INFORMATION Buffer,
			     PULONG BufferLength)
{
   PMSFS_MAILSLOT Mailslot;
   KIRQL oldIrql;

   if (*BufferLength < sizeof(FILE_MAILSLOT_QUERY_INFORMATION))
     return(STATUS_BUFFER_OVERFLOW);

   Mailslot = Fcb->Mailslot;

   Buffer->MaximumMessageSize = Mailslot->MaxMessageSize;
   Buffer->ReadTimeout = Mailslot->TimeOut;

   KeAcquireSpinLock(&Mailslot->MessageListLock, &oldIrql);
   Buffer->MessagesAvailable = Mailslot->MessageCount;
   if (Mailslot->MessageCount == 0)
     {
	Buffer->NextMessageSize = 0;
     }
   else
     {
	/* FIXME: read size of first message (head) */
	Buffer->NextMessageSize = 0;
     }
   KeReleaseSpinLock(&Mailslot->MessageListLock, oldIrql);
   
   *BufferLength -= sizeof(FILE_MAILSLOT_QUERY_INFORMATION);
   
   return(STATUS_SUCCESS);
}


static NTSTATUS
MsfsSetMailslotInformation(PMSFS_FCB Fcb,
			   PFILE_MAILSLOT_SET_INFORMATION Buffer,
			   PULONG BufferLength)
{
   if (*BufferLength < sizeof(FILE_MAILSLOT_SET_INFORMATION))
     return(STATUS_BUFFER_OVERFLOW);

   Fcb->Mailslot->TimeOut = Buffer->ReadTimeout;

   return(STATUS_SUCCESS);
}


NTSTATUS STDCALL
MsfsQueryInformation(PDEVICE_OBJECT DeviceObject,
		     PIRP Irp)
{
   PIO_STACK_LOCATION IoStack;
   FILE_INFORMATION_CLASS FileInformationClass;
   PFILE_OBJECT FileObject;
   PMSFS_DEVICE_EXTENSION DeviceExtension;
   PMSFS_FCB Fcb;
   PMSFS_MAILSLOT Mailslot;
   PVOID SystemBuffer;
   ULONG BufferLength;
   NTSTATUS Status;

   DPRINT("MsfsQueryInformation(DeviceObject %p Irp %p)\n", DeviceObject, Irp);

   IoStack = IoGetCurrentIrpStackLocation (Irp);
   FileInformationClass = IoStack->Parameters.QueryFile.FileInformationClass;
   DeviceExtension = DeviceObject->DeviceExtension;
   FileObject = IoStack->FileObject;
   Fcb = (PMSFS_FCB)FileObject->FsContext;
   Mailslot = Fcb->Mailslot;

   DPRINT("Mailslot name: %wZ\n", &Mailslot->Name);

   /* querying information is not permitted on client side */
   if (Fcb->Mailslot->ServerFcb != Fcb)
     {
	Status = STATUS_ACCESS_DENIED;

	Irp->IoStatus.Status = Status;
	Irp->IoStatus.Information = 0;

	IoCompleteRequest(Irp,
			  IO_NO_INCREMENT);

	return(Status);
     }

   SystemBuffer = Irp->AssociatedIrp.SystemBuffer;
   BufferLength = IoStack->Parameters.QueryFile.Length;

   switch (FileInformationClass)
     {
     case FileMailslotQueryInformation:
	Status = MsfsQueryMailslotInformation(Fcb,
					      SystemBuffer,
					      &BufferLength);
	break;
     default:
	Status = STATUS_NOT_IMPLEMENTED;
     }

   Irp->IoStatus.Status = Status;
   if (NT_SUCCESS(Status))
     Irp->IoStatus.Information =
       IoStack->Parameters.QueryFile.Length - BufferLength;
   else
     Irp->IoStatus.Information = 0;
   IoCompleteRequest(Irp,
		     IO_NO_INCREMENT);

   return(Status);
}


NTSTATUS STDCALL
MsfsSetInformation(PDEVICE_OBJECT DeviceObject,
		   PIRP Irp)
{
   PIO_STACK_LOCATION IoStack;
   FILE_INFORMATION_CLASS FileInformationClass;
   PFILE_OBJECT FileObject;
   PMSFS_FCB Fcb;
   PMSFS_MAILSLOT Mailslot;
   PVOID SystemBuffer;
   ULONG BufferLength;
   NTSTATUS Status;

   DPRINT("MsfsSetInformation(DeviceObject %p Irp %p)\n", DeviceObject, Irp);

   IoStack = IoGetCurrentIrpStackLocation (Irp);
   FileInformationClass = IoStack->Parameters.QueryFile.FileInformationClass;
   FileObject = IoStack->FileObject;
   Fcb = (PMSFS_FCB)FileObject->FsContext;
   Mailslot = Fcb->Mailslot;

   DPRINT("Mailslot name: %wZ\n", &Mailslot->Name);

   /* setting information is not permitted on client side */
   if (Fcb->Mailslot->ServerFcb != Fcb)
     {
	Status = STATUS_ACCESS_DENIED;

	Irp->IoStatus.Status = Status;
	Irp->IoStatus.Information = 0;

	IoCompleteRequest(Irp,
			  IO_NO_INCREMENT);

	return(Status);
     }

   SystemBuffer = Irp->AssociatedIrp.SystemBuffer;
   BufferLength = IoStack->Parameters.QueryFile.Length;

   DPRINT("FileInformationClass %d\n", FileInformationClass);
   DPRINT("SystemBuffer %x\n", SystemBuffer);

   switch (FileInformationClass)
     {
     case FileMailslotSetInformation:
	Status = MsfsSetMailslotInformation(Fcb,
					    SystemBuffer,
					    &BufferLength);
	break;
     default:
	Status = STATUS_NOT_IMPLEMENTED;
     }

   Irp->IoStatus.Status = Status;
   Irp->IoStatus.Information = 0;
   IoCompleteRequest(Irp,
		     IO_NO_INCREMENT);

   return(Status);
}

/* EOF */
