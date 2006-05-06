/* $Id$
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
MsfsQueryMailslotInformation(PMSFS_CCB Ccb,
			     PFILE_MAILSLOT_QUERY_INFORMATION Buffer,
			     PULONG BufferLength)
{
   PMSFS_MAILSLOT Mailslot;
   KIRQL oldIrql;

   if (*BufferLength < sizeof(FILE_MAILSLOT_QUERY_INFORMATION))
     return(STATUS_BUFFER_OVERFLOW);

   Mailslot = Ccb->Mailslot;

   Buffer->MaximumMessageSize = Mailslot->MaxMessageSize;
   Buffer->ReadTimeout = Mailslot->TimeOut;

   KeAcquireSpinLock(&Mailslot->MessageListLock, &oldIrql);
   Buffer->MessagesAvailable = Mailslot->MessageCount;
   if (Mailslot->MessageCount == 0)
     {
	Buffer->NextMessageSize = MAILSLOT_NO_MESSAGE;
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
MsfsSetMailslotInformation(PMSFS_CCB Ccb,
			   PFILE_MAILSLOT_SET_INFORMATION Buffer,
			   PULONG BufferLength)
{
   if (*BufferLength < sizeof(FILE_MAILSLOT_SET_INFORMATION))
     return(STATUS_BUFFER_OVERFLOW);

   Ccb->Mailslot->TimeOut = *Buffer->ReadTimeout;

   return(STATUS_SUCCESS);
}


NTSTATUS DEFAULTAPI
MsfsQueryInformation(PDEVICE_OBJECT DeviceObject,
		     PIRP Irp)
{
   PIO_STACK_LOCATION IoStack;
   FILE_INFORMATION_CLASS FileInformationClass;
   PFILE_OBJECT FileObject;
   PMSFS_DEVICE_EXTENSION DeviceExtension;
   PMSFS_CCB Ccb;
   PMSFS_MAILSLOT Mailslot;
   PVOID SystemBuffer;
   ULONG BufferLength;
   NTSTATUS Status;

   DPRINT("MsfsQueryInformation(DeviceObject %p Irp %p)\n", DeviceObject, Irp);

   IoStack = IoGetCurrentIrpStackLocation (Irp);
   FileInformationClass = IoStack->Parameters.QueryFile.FileInformationClass;
   DeviceExtension = DeviceObject->DeviceExtension;
   FileObject = IoStack->FileObject;
   Ccb = (PMSFS_CCB)FileObject->FsContext2;
   Mailslot = Ccb->Mailslot;

   DPRINT("Mailslot name: %wZ\n", &Mailslot->Name);

   /* querying information is not permitted on client side */
   if (Ccb->Mailslot->ServerCcb != Ccb)
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
	Status = MsfsQueryMailslotInformation(Ccb,
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


NTSTATUS DEFAULTAPI
MsfsSetInformation(PDEVICE_OBJECT DeviceObject,
		   PIRP Irp)
{
   PIO_STACK_LOCATION IoStack;
   FILE_INFORMATION_CLASS FileInformationClass;
   PFILE_OBJECT FileObject;
   PMSFS_CCB Ccb;
   PMSFS_MAILSLOT Mailslot;
   PVOID SystemBuffer;
   ULONG BufferLength;
   NTSTATUS Status;

   DPRINT("MsfsSetInformation(DeviceObject %p Irp %p)\n", DeviceObject, Irp);

   IoStack = IoGetCurrentIrpStackLocation (Irp);
   FileInformationClass = IoStack->Parameters.QueryFile.FileInformationClass;
   FileObject = IoStack->FileObject;
   Ccb = (PMSFS_CCB)FileObject->FsContext2;
   Mailslot = Ccb->Mailslot;

   DPRINT("Mailslot name: %wZ\n", &Mailslot->Name);

   /* setting information is not permitted on client side */
   if (Ccb->Mailslot->ServerCcb != Ccb)
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
	Status = MsfsSetMailslotInformation(Ccb,
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
