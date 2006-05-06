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
   PMSFS_FCB Fcb;
   KIRQL oldIrql;

   if (*BufferLength < sizeof(FILE_MAILSLOT_QUERY_INFORMATION))
     return(STATUS_BUFFER_OVERFLOW);

   Fcb = Ccb->Fcb;

   Buffer->MaximumMessageSize = Fcb->MaxMessageSize;
   Buffer->ReadTimeout = Fcb->TimeOut;

   KeAcquireSpinLock(&Fcb->MessageListLock, &oldIrql);
   Buffer->MessagesAvailable = Fcb->MessageCount;
   if (Fcb->MessageCount == 0)
     {
	Buffer->NextMessageSize = MAILSLOT_NO_MESSAGE;
     }
   else
     {
	/* FIXME: read size of first message (head) */
	Buffer->NextMessageSize = 0;
     }
   KeReleaseSpinLock(&Fcb->MessageListLock, oldIrql);

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

   Ccb->Fcb->TimeOut = *Buffer->ReadTimeout;

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
   PMSFS_FCB Fcb;
   PMSFS_CCB Ccb;
   PVOID SystemBuffer;
   ULONG BufferLength;
   NTSTATUS Status;

   DPRINT("MsfsQueryInformation(DeviceObject %p Irp %p)\n", DeviceObject, Irp);

   IoStack = IoGetCurrentIrpStackLocation (Irp);
   FileInformationClass = IoStack->Parameters.QueryFile.FileInformationClass;
   DeviceExtension = DeviceObject->DeviceExtension;
   FileObject = IoStack->FileObject;
   Ccb = (PMSFS_CCB)FileObject->FsContext2;
   Fcb = Ccb->Fcb;

   DPRINT("Mailslot name: %wZ\n", &Fcb->Name);

   /* querying information is not permitted on client side */
   if (Ccb->Fcb->ServerCcb != Ccb)
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
   PMSFS_FCB Fcb;
   PMSFS_CCB Ccb;
   PVOID SystemBuffer;
   ULONG BufferLength;
   NTSTATUS Status;

   DPRINT("MsfsSetInformation(DeviceObject %p Irp %p)\n", DeviceObject, Irp);

   IoStack = IoGetCurrentIrpStackLocation (Irp);
   FileInformationClass = IoStack->Parameters.QueryFile.FileInformationClass;
   FileObject = IoStack->FileObject;
   Ccb = (PMSFS_CCB)FileObject->FsContext2;
   Fcb = Ccb->Fcb;

   DPRINT("Mailslot name: %wZ\n", &Fcb->Name);

   /* setting information is not permitted on client side */
   if (Ccb->Fcb->ServerCcb != Ccb)
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
