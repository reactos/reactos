/* $Id: rw.c,v 1.1 2001/05/05 15:11:57 ekohl Exp $
 *
 * COPYRIGHT:  See COPYING in the top level directory
 * PROJECT:    ReactOS kernel
 * FILE:       services/fs/ms/rw.c
 * PURPOSE:    Mailslot filesystem
 * PROGRAMMER: Eric Kohl <ekohl@rz-online.de>
 */

/* INCLUDES ******************************************************************/

#include <ddk/ntddk.h>
#include <ntos/minmax.h>
#include "msfs.h"

//#define NDEBUG
#include <debug.h>


/* FUNCTIONS *****************************************************************/

NTSTATUS STDCALL
MsfsRead(PDEVICE_OBJECT DeviceObject,
	 PIRP Irp)
{
   PIO_STACK_LOCATION IoStack;
   PFILE_OBJECT FileObject;
   PMSFS_MAILSLOT Mailslot;
   PMSFS_FCB Fcb;
   PMSFS_MESSAGE Message;
   KIRQL oldIrql;
   ULONG Length;
   ULONG LengthRead = 0;
   PVOID Buffer;
   NTSTATUS Status;

   DPRINT("MsfsRead(DeviceObject %p Irp %p)\n", DeviceObject, Irp);

   IoStack = IoGetCurrentIrpStackLocation (Irp);
   FileObject = IoStack->FileObject;
   Fcb = (PMSFS_FCB)FileObject->FsContext;
   Mailslot = Fcb->Mailslot;

   DPRINT("MailslotName: %wZ\n", &Mailslot->Name);

   /* reading is not permitted on client side */
   if (Fcb->Mailslot->ServerFcb != Fcb)
     {
	Irp->IoStatus.Status = STATUS_ACCESS_DENIED;
	Irp->IoStatus.Information = 0;

	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	return(STATUS_ACCESS_DENIED);
     }

   Length = IoStack->Parameters.Read.Length;
   if (Irp->MdlAddress)
     Buffer = MmGetSystemAddressForMdl (Irp->MdlAddress);
   else
     Buffer = Irp->UserBuffer;

   Status = KeWaitForSingleObject(&Mailslot->MessageEvent,
				  UserRequest,
				  KernelMode,
				  FALSE,
				  NULL); /* FIXME: handle timeout */
CHECKPOINT1;
   if ((NT_SUCCESS(Status)) && (Mailslot->MessageCount > 0))
     {
	/* copy current message into buffer */
	Message = CONTAINING_RECORD(Mailslot->MessageListHead.Flink,
				    MSFS_MESSAGE,
				    MessageListEntry);
	memcpy(Buffer, &Message->Buffer, min(Message->Size,Length));
	LengthRead = Message->Size;
CHECKPOINT1;
	KeAcquireSpinLock(&Mailslot->MessageListLock, &oldIrql);
CHECKPOINT1;
	RemoveHeadList(&Mailslot->MessageListHead);
CHECKPOINT1;
	KeReleaseSpinLock(&Mailslot->MessageListLock, oldIrql);
CHECKPOINT1;
	ExFreePool(Message);
CHECKPOINT1;
	Mailslot->MessageCount--;
CHECKPOINT1;
	if (Mailslot->MessageCount == 0)
	  {
CHECKPOINT1;
	     KeClearEvent(&Mailslot->MessageEvent);
	  }
     }
CHECKPOINT1;

   Irp->IoStatus.Status = Status;
   Irp->IoStatus.Information = LengthRead;

   IoCompleteRequest(Irp, IO_NO_INCREMENT);

   return(Status);
}


NTSTATUS STDCALL
MsfsWrite(PDEVICE_OBJECT DeviceObject,
	  PIRP Irp)
{
   PIO_STACK_LOCATION IoStack;
   PFILE_OBJECT FileObject;
   PMSFS_MAILSLOT Mailslot;
   PMSFS_FCB Fcb;
   PMSFS_MESSAGE Message;
   KIRQL oldIrql;
   ULONG Length;
   PVOID Buffer;

   DPRINT1("MsfsWrite(DeviceObject %p Irp %p)\n", DeviceObject, Irp);

   IoStack = IoGetCurrentIrpStackLocation (Irp);
   FileObject = IoStack->FileObject;
   Fcb = (PMSFS_FCB)FileObject->FsContext;
   Mailslot = Fcb->Mailslot;

   DPRINT1("MailslotName: %wZ\n", &Mailslot->Name);

   /* writing is not permitted on server side */
   if (Fcb->Mailslot->ServerFcb == Fcb)
     {
	Irp->IoStatus.Status = STATUS_ACCESS_DENIED;
	Irp->IoStatus.Information = 0;

	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	return(STATUS_ACCESS_DENIED);
     }

   Length = IoStack->Parameters.Write.Length;
   if (Irp->MdlAddress)
     Buffer = MmGetSystemAddressForMdl (Irp->MdlAddress);
   else
     Buffer = Irp->UserBuffer;

   DPRINT("Length: %lu Message: %s\n", Length, (PUCHAR)Buffer);

   /* Allocate new message */
   Message = ExAllocatePool(NonPagedPool,
			    sizeof(MSFS_MESSAGE) + Length);
   if (Message == NULL)
     {
	Irp->IoStatus.Status = STATUS_NO_MEMORY;
	Irp->IoStatus.Information = 0;

	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	return(STATUS_NO_MEMORY);
     }

   Message->Size = Length;
   memcpy(&Message->Buffer, Buffer, Length);

   KeAcquireSpinLock(&Mailslot->MessageListLock, &oldIrql);
   InsertTailList(&Mailslot->MessageListHead, &Message->MessageListEntry);
   KeReleaseSpinLock(&Mailslot->MessageListLock, oldIrql);

   Mailslot->MessageCount++;
   if (Mailslot->MessageCount == 1)
     {
	KeSetEvent(&Mailslot->MessageEvent,
		   0,
		   FALSE);
     }

   Irp->IoStatus.Status = STATUS_SUCCESS;
   Irp->IoStatus.Information = Length;

   IoCompleteRequest(Irp, IO_NO_INCREMENT);

   return(STATUS_SUCCESS);
}

/* EOF */
