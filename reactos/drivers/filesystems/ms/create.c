/* $Id$
 *
 * COPYRIGHT:  See COPYING in the top level directory
 * PROJECT:    ReactOS kernel
 * FILE:       services/fs/ms/create.c
 * PURPOSE:    Mailslot filesystem
 * PROGRAMMER: Eric Kohl <ekohl@rz-online.de>
 */

/* INCLUDES ******************************************************************/

#include "msfs.h"

#define NDEBUG
#include <debug.h>


/* FUNCTIONS *****************************************************************/

NTSTATUS DEFAULTAPI
MsfsCreate(PDEVICE_OBJECT DeviceObject,
	   PIRP Irp)
{
   PIO_STACK_LOCATION IoStack;
   PFILE_OBJECT FileObject;
   PMSFS_DEVICE_EXTENSION DeviceExtension;
   PMSFS_MAILSLOT Mailslot;
   PMSFS_CCB Ccb;
   PMSFS_MAILSLOT current = NULL;
   PLIST_ENTRY current_entry;
   KIRQL oldIrql;

   DPRINT("MsfsCreate(DeviceObject %p Irp %p)\n", DeviceObject, Irp);

   IoStack = IoGetCurrentIrpStackLocation(Irp);
   DeviceExtension = DeviceObject->DeviceExtension;
   FileObject = IoStack->FileObject;

   DPRINT("Mailslot name: %wZ\n", &FileObject->FileName);

   Ccb = ExAllocatePool(NonPagedPool, sizeof(MSFS_CCB));
   if (Ccb == NULL)
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
	ExFreePool(Ccb);
	KeUnlockMutex(&DeviceExtension->MailslotListLock);

	Irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
	Irp->IoStatus.Information = 0;

	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	return(STATUS_UNSUCCESSFUL);
     }

   Mailslot = current;

   KeAcquireSpinLock(&Mailslot->CcbListLock, &oldIrql);
   InsertTailList(&Mailslot->CcbListHead, &Ccb->CcbListEntry);
   KeReleaseSpinLock(&Mailslot->CcbListLock, oldIrql);

   Mailslot->ReferenceCount++;

   Ccb->Mailslot = Mailslot;

   KeUnlockMutex(&DeviceExtension->MailslotListLock);

   FileObject->FsContext2 = Ccb;
   FileObject->Flags |= FO_MAILSLOT;

   Irp->IoStatus.Status = STATUS_SUCCESS;
   Irp->IoStatus.Information = 0;

   IoCompleteRequest(Irp, IO_NO_INCREMENT);

   return(STATUS_SUCCESS);
}


NTSTATUS DEFAULTAPI
MsfsCreateMailslot(PDEVICE_OBJECT DeviceObject,
		   PIRP Irp)
{
   PEXTENDED_IO_STACK_LOCATION IoStack;
   PFILE_OBJECT FileObject;
   PMSFS_DEVICE_EXTENSION DeviceExtension;
   PMSFS_MAILSLOT Mailslot;
   PMSFS_CCB Ccb;
   KIRQL oldIrql;
   PLIST_ENTRY current_entry;
   PMSFS_MAILSLOT current;
   PMAILSLOT_CREATE_PARAMETERS Buffer;

   DPRINT("MsfsCreateMailslot(DeviceObject %p Irp %p)\n", DeviceObject, Irp);

   IoStack = (PEXTENDED_IO_STACK_LOCATION)IoGetCurrentIrpStackLocation(Irp);
   DeviceExtension = DeviceObject->DeviceExtension;
   FileObject = IoStack->FileObject;
   Buffer = IoStack->Parameters.CreateMailslot.Parameters;

   DPRINT("Mailslot name: %wZ\n", &FileObject->FileName);

   Mailslot = ExAllocatePool(NonPagedPool, sizeof(MSFS_MAILSLOT));
   if (Mailslot == NULL)
     {
	Irp->IoStatus.Status = STATUS_NO_MEMORY;
	Irp->IoStatus.Information = 0;

	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	return(STATUS_NO_MEMORY);
     }

   Mailslot->Name.Length = FileObject->FileName.Length;
   Mailslot->Name.MaximumLength = Mailslot->Name.Length + sizeof(UNICODE_NULL);
   Mailslot->Name.Buffer = ExAllocatePool(NonPagedPool, Mailslot->Name.MaximumLength);
   if (Mailslot->Name.Buffer == NULL)
     {
	ExFreePool(Mailslot);

	Irp->IoStatus.Status = STATUS_NO_MEMORY;
	Irp->IoStatus.Information = 0;

	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	return(STATUS_NO_MEMORY);
     }

   RtlCopyUnicodeString(&Mailslot->Name, &FileObject->FileName);

   Ccb = ExAllocatePool(NonPagedPool, sizeof(MSFS_CCB));
   if (Ccb == NULL)
     {
	ExFreePool(Mailslot);

	Irp->IoStatus.Status = STATUS_NO_MEMORY;
	Irp->IoStatus.Information = 0;

	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	return(STATUS_NO_MEMORY);
     }

   Mailslot->ReferenceCount = 0;
   InitializeListHead(&Mailslot->CcbListHead);
   KeInitializeSpinLock(&Mailslot->CcbListLock);

   Mailslot->MaxMessageSize = Buffer->MaximumMessageSize;
   Mailslot->MessageCount = 0;
   Mailslot->TimeOut = Buffer->ReadTimeout;
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

   KeAcquireSpinLock(&Mailslot->CcbListLock, &oldIrql);
   InsertTailList(&Mailslot->CcbListHead, &Ccb->CcbListEntry);
   KeReleaseSpinLock(&Mailslot->CcbListLock, oldIrql);

   Mailslot->ReferenceCount++;
   Mailslot->ServerCcb = Ccb;
   Ccb->Mailslot = Mailslot;

   KeUnlockMutex(&DeviceExtension->MailslotListLock);

   FileObject->FsContext2 = Ccb;
   FileObject->Flags |= FO_MAILSLOT;

   Irp->IoStatus.Status = STATUS_SUCCESS;
   Irp->IoStatus.Information = 0;

   IoCompleteRequest(Irp, IO_NO_INCREMENT);

   return(STATUS_SUCCESS);
}


NTSTATUS DEFAULTAPI
MsfsClose(PDEVICE_OBJECT DeviceObject,
	  PIRP Irp)
{
   PIO_STACK_LOCATION IoStack;
   PFILE_OBJECT FileObject;
   PMSFS_DEVICE_EXTENSION DeviceExtension;
   PMSFS_MAILSLOT Mailslot;
   PMSFS_CCB Ccb;
   PMSFS_MESSAGE Message;
   KIRQL oldIrql;

   DPRINT("MsfsClose(DeviceObject %p Irp %p)\n", DeviceObject, Irp);

   IoStack = IoGetCurrentIrpStackLocation(Irp);
   DeviceExtension = DeviceObject->DeviceExtension;
   FileObject = IoStack->FileObject;

   KeLockMutex(&DeviceExtension->MailslotListLock);

   if (DeviceExtension->MailslotListHead.Flink == &DeviceExtension->MailslotListHead)
     {
	KeUnlockMutex(&DeviceExtension->MailslotListLock);

	Irp->IoStatus.Status = STATUS_SUCCESS;
	Irp->IoStatus.Information = 0;

	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	return(STATUS_SUCCESS);
     }

   Ccb = FileObject->FsContext2;
   Mailslot = Ccb->Mailslot;

   DPRINT("Mailslot name: %wZ\n", &Mailslot->Name);

   Mailslot->ReferenceCount--;
   if (Mailslot->ServerCcb == Ccb)
     {
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
	Mailslot->ServerCcb = NULL;
     }

   KeAcquireSpinLock(&Mailslot->CcbListLock, &oldIrql);
   RemoveEntryList(&Ccb->CcbListEntry);
   KeReleaseSpinLock(&Mailslot->CcbListLock, oldIrql);
   ExFreePool(Ccb);
   FileObject->FsContext2 = NULL;

   if (Mailslot->ReferenceCount == 0)
     {
	DPRINT1("ReferenceCount == 0: Deleting mailslot data\n");
	RtlFreeUnicodeString(&Mailslot->Name);
	RemoveEntryList(&Mailslot->MailslotListEntry);
	ExFreePool(Mailslot);
     }

   KeUnlockMutex(&DeviceExtension->MailslotListLock);

   Irp->IoStatus.Status = STATUS_SUCCESS;
   Irp->IoStatus.Information = 0;

   IoCompleteRequest(Irp, IO_NO_INCREMENT);

   return(STATUS_SUCCESS);
}

/* EOF */
