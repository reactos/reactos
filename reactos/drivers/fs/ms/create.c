/* $Id: create.c,v 1.1 2001/05/05 15:11:57 ekohl Exp $
 *
 * COPYRIGHT:  See COPYING in the top level directory
 * PROJECT:    ReactOS kernel
 * FILE:       services/fs/ms/create.c
 * PURPOSE:    Mailslot filesystem
 * PROGRAMMER: Eric Kohl <ekohl@rz-online.de>
 */

/* INCLUDES ******************************************************************/

#include <ddk/ntddk.h>
#include "msfs.h"

//#define NDEBUG
#include <debug.h>


/* FUNCTIONS *****************************************************************/

NTSTATUS STDCALL
MsfsCreate(PDEVICE_OBJECT DeviceObject,
	   PIRP Irp)
{
   PIO_STACK_LOCATION IoStack;
   PFILE_OBJECT FileObject;
   PMSFS_DEVICE_EXTENSION DeviceExtension;
   PMSFS_MAILSLOT Mailslot;
   PMSFS_FCB Fcb;
   PMSFS_MAILSLOT current;
   PLIST_ENTRY current_entry;
   KIRQL oldIrql;

   DPRINT1("MsfsCreate(DeviceObject %p Irp %p)\n", DeviceObject, Irp);

   IoStack = IoGetCurrentIrpStackLocation(Irp);
   DeviceExtension = DeviceObject->DeviceExtension;
   FileObject = IoStack->FileObject;

   DPRINT1("Mailslot name: %wZ\n", &FileObject->FileName);

   Fcb = ExAllocatePool(NonPagedPool, sizeof(MSFS_FCB));
   if (Fcb == NULL)
     {
	Irp->IoStatus.Status = STATUS_NO_MEMORY;
	Irp->IoStatus.Information = 0;

	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	return(STATUS_NO_MEMORY);
     }

   KeLockMutex(&DeviceExtension->MailslotListLock);
   current_entry = DeviceExtension->MailslotListHead.Flink;
   while (current_entry != &DeviceExtension->MailslotListHead)
     {
	current = CONTAINING_RECORD(current_entry,
				    MSFS_MAILSLOT,
				    MailslotListEntry);

	if (!RtlCompareUnicodeString(&FileObject->FileName, &current->Name, TRUE))
	  {
	     break;
	  }

	current_entry = current_entry->Flink;
     }

   if (current_entry == &DeviceExtension->MailslotListHead)
     {
	ExFreePool(Fcb);
	KeUnlockMutex(&DeviceExtension->MailslotListLock);

	Irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
	Irp->IoStatus.Information = 0;

	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	return(STATUS_UNSUCCESSFUL);
     }

   Mailslot = current;

   KeAcquireSpinLock(&Mailslot->FcbListLock, &oldIrql);
   InsertTailList(&Mailslot->FcbListHead, &Fcb->FcbListEntry);
   KeReleaseSpinLock(&Mailslot->FcbListLock, oldIrql);

   Mailslot->ReferenceCount++;

   Fcb->Mailslot = Mailslot;

   KeUnlockMutex(&DeviceExtension->MailslotListLock);

   FileObject->FsContext = Fcb;

   Irp->IoStatus.Status = STATUS_SUCCESS;
   Irp->IoStatus.Information = 0;

   IoCompleteRequest(Irp, IO_NO_INCREMENT);

   return(STATUS_SUCCESS);
}


NTSTATUS STDCALL
MsfsCreateMailslot(PDEVICE_OBJECT DeviceObject,
		   PIRP Irp)
{
   PIO_STACK_LOCATION IoStack;
   PFILE_OBJECT FileObject;
   PMSFS_DEVICE_EXTENSION DeviceExtension;
   PMSFS_MAILSLOT Mailslot;
   PMSFS_FCB Fcb;
   KIRQL oldIrql;
   PLIST_ENTRY current_entry;
   PMSFS_MAILSLOT current;
   PIO_MAILSLOT_CREATE_BUFFER Buffer;

   DPRINT1("MsfsCreateMailslot(DeviceObject %p Irp %p)\n", DeviceObject, Irp);

   IoStack = IoGetCurrentIrpStackLocation(Irp);
   DeviceExtension = DeviceObject->DeviceExtension;
   FileObject = IoStack->FileObject;
   Buffer = (PIO_MAILSLOT_CREATE_BUFFER)Irp->Tail.Overlay.AuxiliaryBuffer;

   DPRINT1("Mailslot name: %wZ\n", &FileObject->FileName);

   Mailslot = ExAllocatePool(NonPagedPool, sizeof(MSFS_MAILSLOT));
   if (Mailslot == NULL)
     {
	Irp->IoStatus.Status = STATUS_NO_MEMORY;
	Irp->IoStatus.Information = 0;

	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	return(STATUS_NO_MEMORY);
     }

   if (!RtlCreateUnicodeString(&Mailslot->Name, FileObject->FileName.Buffer))
     {
	ExFreePool(Mailslot);
	ExFreePool(Fcb);

	Irp->IoStatus.Status = STATUS_NO_MEMORY;
	Irp->IoStatus.Information = 0;

	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	return(STATUS_NO_MEMORY);
     }

   Fcb = ExAllocatePool(NonPagedPool, sizeof(MSFS_FCB));
   if (Fcb == NULL)
     {
	ExFreePool(Mailslot);

	Irp->IoStatus.Status = STATUS_NO_MEMORY;
	Irp->IoStatus.Information = 0;

	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	return(STATUS_NO_MEMORY);
     }

   Mailslot->ReferenceCount = 0;
   InitializeListHead(&Mailslot->FcbListHead);
   KeInitializeSpinLock(&Mailslot->FcbListLock);

   Mailslot->MaxMessageSize = Buffer->MaxMessageSize;
   Mailslot->MessageCount = 0;
   Mailslot->TimeOut = Buffer->TimeOut;
   KeInitializeEvent(&Mailslot->MessageEvent,
		     NotificationEvent,
		     FALSE);

   InitializeListHead(&Mailslot->MessageListHead);
   KeInitializeSpinLock(&Mailslot->MessageListLock);

   KeLockMutex(&DeviceExtension->MailslotListLock);
   current_entry = DeviceExtension->MailslotListHead.Flink;
   while (current_entry != &DeviceExtension->MailslotListHead)
     {
	current = CONTAINING_RECORD(current_entry,
				    MSFS_MAILSLOT,
				    MailslotListEntry);

	if (!RtlCompareUnicodeString(&Mailslot->Name, &current->Name, TRUE))
	  {
	     break;
	  }

	current_entry = current_entry->Flink;
     }

   if (current_entry != &DeviceExtension->MailslotListHead)
     {
	RtlFreeUnicodeString(&Mailslot->Name);
	ExFreePool(Mailslot);
	
	Mailslot = current;
     }
   else
     {
	InsertTailList(&DeviceExtension->MailslotListHead,
		       &Mailslot->MailslotListEntry);
     }

   KeAcquireSpinLock(&Mailslot->FcbListLock, &oldIrql);
   InsertTailList(&Mailslot->FcbListHead, &Fcb->FcbListEntry);
   KeReleaseSpinLock(&Mailslot->FcbListLock, oldIrql);

   Mailslot->ReferenceCount++;
   Mailslot->ServerFcb = Fcb;
   Fcb->Mailslot = Mailslot;

   KeUnlockMutex(&DeviceExtension->MailslotListLock);

   FileObject->FsContext = Fcb;

   Irp->IoStatus.Status = STATUS_SUCCESS;
   Irp->IoStatus.Information = 0;

   IoCompleteRequest(Irp, IO_NO_INCREMENT);

   return(STATUS_SUCCESS);
}


NTSTATUS STDCALL
MsfsClose(PDEVICE_OBJECT DeviceObject,
	  PIRP Irp)
{
   PIO_STACK_LOCATION IoStack;
   PFILE_OBJECT FileObject;
   PMSFS_DEVICE_EXTENSION DeviceExtension;
   PMSFS_MAILSLOT Mailslot;
   PMSFS_FCB Fcb;
   PMSFS_MESSAGE Message;
   KIRQL oldIrql;

   DPRINT1("MsfsClose(DeviceObject %p Irp %p)\n", DeviceObject, Irp);

   IoStack = IoGetCurrentIrpStackLocation(Irp);
   DeviceExtension = DeviceObject->DeviceExtension;
   FileObject = IoStack->FileObject;

   KeLockMutex(&DeviceExtension->MailslotListLock);

   if (DeviceExtension->MailslotListHead.Flink == &DeviceExtension->MailslotListHead)
     {
	KeUnlockMutex(&DeviceExtension->MailslotListLock);
CHECKPOINT1;

	Irp->IoStatus.Status = STATUS_SUCCESS;
	Irp->IoStatus.Information = 0;

	IoCompleteRequest(Irp, IO_NO_INCREMENT);
CHECKPOINT1;

	return(STATUS_SUCCESS);
     }

   Fcb = FileObject->FsContext;
   Mailslot = Fcb->Mailslot;

   DPRINT1("Mailslot name: %wZ\n", &Mailslot->Name);

   Mailslot->ReferenceCount--;
CHECKPOINT1;
   if (Mailslot->ServerFcb == Fcb)
     {
CHECKPOINT1;
	/* delete all messages from message-list */
	KeAcquireSpinLock(&Mailslot->MessageListLock, &oldIrql);

	while (Mailslot->MessageListHead.Flink != &Mailslot->MessageListHead)
	  {
	     Message = CONTAINING_RECORD(Mailslot->MessageListHead.Flink,
					 MSFS_MESSAGE,
					 MessageListEntry);
	     RemoveEntryList(Mailslot->MessageListHead.Flink);
	     ExFreePool(Message);
	  }
	Mailslot->MessageCount = 0;

	KeReleaseSpinLock(&Mailslot->MessageListLock, oldIrql);
	Mailslot->ServerFcb = NULL;
     }
CHECKPOINT1;

   KeAcquireSpinLock(&Mailslot->FcbListLock, &oldIrql);
   RemoveEntryList(&Fcb->FcbListEntry);
   KeReleaseSpinLock(&Mailslot->FcbListLock, oldIrql);
   ExFreePool(Fcb);
   FileObject->FsContext = NULL;
CHECKPOINT1;

   if (Mailslot->ReferenceCount == 0)
     {
	DPRINT1("ReferenceCount == 0: Deleting mailslot data\n");
	RtlFreeUnicodeString(&Mailslot->Name);
	RemoveEntryList(&Mailslot->MailslotListEntry);
	ExFreePool(Mailslot);
     }
CHECKPOINT1;

   KeUnlockMutex(&DeviceExtension->MailslotListLock);
CHECKPOINT1;

   Irp->IoStatus.Status = STATUS_SUCCESS;
   Irp->IoStatus.Information = 0;

   IoCompleteRequest(Irp, IO_NO_INCREMENT);
CHECKPOINT1;

   return(STATUS_SUCCESS);
}

/* EOF */
