/* $Id: finfo.c,v 1.1 2001/05/05 15:11:57 ekohl Exp $
 *
 * COPYRIGHT:  See COPYING in the top level directory
 * PROJECT:    ReactOS kernel
 * FILE:       services/fs/ms/finfo.c
 * PURPOSE:    Mailslot filesystem
 * PROGRAMMER: Eric Kohl <ekohl@rz-online.de>
 */

/* INCLUDES ******************************************************************/

#include <ddk/ntddk.h>
#include "msfs.h"

//#define NDEBUG
#include <debug.h>



static NTSTATUS
MsfsQueryMailslotInformation(PMSFS_FCB Fcb,
			     PFILE_MAILSLOT_QUERY_INFORMATION Buffer);

static NTSTATUS
MsfsSetMailslotInformation(PMSFS_FCB Fcb,
			   PFILE_MAILSLOT_SET_INFORMATION Buffer);

/* FUNCTIONS *****************************************************************/

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
   NTSTATUS Status;
   PVOID Buffer;

   DPRINT1("MsfsQueryInformation(DeviceObject %p Irp %p)\n", DeviceObject, Irp);

   IoStack = IoGetCurrentIrpStackLocation (Irp);
   FileInformationClass = IoStack->Parameters.QueryFile.FileInformationClass;
   DeviceExtension = DeviceObject->DeviceExtension;
   FileObject = IoStack->FileObject;
   Fcb = (PMSFS_FCB)FileObject->FsContext;
   Mailslot = Fcb->Mailslot;

   DPRINT1("Mailslot name: %wZ\n", &Mailslot->Name);

   /* querying information is not permitted on client side */
   if (Fcb->Mailslot->ServerFcb != Fcb)
     {
	Status = STATUS_ACCESS_DENIED;

	Irp->IoStatus.Status = Status;
	Irp->IoStatus.Information = 0;

	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	return(Status);
     }

  // FIXME : determine Buffer for result :
  if (Irp->MdlAddress)
    Buffer = MmGetSystemAddressForMdl (Irp->MdlAddress);
  else
    Buffer = Irp->UserBuffer;

   switch (FileInformationClass)
     {
     case FileMailslotQueryInformation:
	Status = MsfsQueryMailslotInformation(Fcb,
					      (PFILE_MAILSLOT_QUERY_INFORMATION)Buffer);
	break;
     default:
	Status = STATUS_NOT_IMPLEMENTED;
     }

   Irp->IoStatus.Status = Status;
   Irp->IoStatus.Information = 0;
   IoCompleteRequest (Irp, IO_NO_INCREMENT);

   return(Status);
}


static NTSTATUS
MsfsQueryMailslotInformation(PMSFS_FCB Fcb,
			     PFILE_MAILSLOT_QUERY_INFORMATION Buffer)
{
   PMSFS_MAILSLOT Mailslot;
   KIRQL oldIrql;

   Mailslot = Fcb->Mailslot;

   Buffer->MaxMessageSize = Mailslot->MaxMessageSize;
   Buffer->Timeout = Mailslot->TimeOut;

   KeAcquireSpinLock(&Mailslot->MessageListLock, &oldIrql);
   Buffer->MessageCount = Mailslot->MessageCount;
   if (Mailslot->MessageCount == 0)
     {
	Buffer->NextSize = 0;
     }
   else
     {
	/* FIXME: read size of first message (head) */
	Buffer->NextSize = 0;
     }
   KeReleaseSpinLock(&Mailslot->MessageListLock, oldIrql);

   return(STATUS_SUCCESS);
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
   PVOID Buffer;
   NTSTATUS Status;

   DPRINT1("MsfsSetInformation(DeviceObject %p Irp %p)\n", DeviceObject, Irp);

   IoStack = IoGetCurrentIrpStackLocation (Irp);
   FileInformationClass = IoStack->Parameters.QueryFile.FileInformationClass;
   FileObject = IoStack->FileObject;
   Fcb = (PMSFS_FCB)FileObject->FsContext;
   Mailslot = Fcb->Mailslot;

   DPRINT1("Mailslot name: %wZ\n", &Mailslot->Name);

   /* setting information is not permitted on client side */
   if (Fcb->Mailslot->ServerFcb != Fcb)
     {
	Status = STATUS_ACCESS_DENIED;

	Irp->IoStatus.Status = Status;
	Irp->IoStatus.Information = 0;

	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	return(Status);
     }

   // FIXME : determine Buffer for result :
   if (Irp->MdlAddress)
     Buffer = MmGetSystemAddressForMdl (Irp->MdlAddress);
   else
     Buffer = Irp->UserBuffer;

  DPRINT("FileInformationClass %d\n", FileInformationClass);
  DPRINT("Buffer %x\n", Buffer);

   switch (FileInformationClass)
     {
     case FileMailslotSetInformation:
	Status = MsfsSetMailslotInformation(Fcb,
					    (PFILE_MAILSLOT_SET_INFORMATION)Buffer);
	break;
     default:
	Status = STATUS_NOT_IMPLEMENTED;
     }

   Irp->IoStatus.Status = Status;
   Irp->IoStatus.Information = 0;
   IoCompleteRequest(Irp, IO_NO_INCREMENT);

   return(Status);
}


static NTSTATUS
MsfsSetMailslotInformation(PMSFS_FCB Fcb,
			   PFILE_MAILSLOT_SET_INFORMATION Buffer)
{
   Fcb->Mailslot->TimeOut = Buffer->Timeout;

   return(STATUS_SUCCESS);
}


/* EOF */
