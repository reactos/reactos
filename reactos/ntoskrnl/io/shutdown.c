/* $Id: shutdown.c,v 1.4 2001/03/07 16:48:42 dwelch Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/io/shutdown.c
 * PURPOSE:         Implements shutdown notification
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <internal/pool.h>

#include <internal/debug.h>

/* LOCAL DATA ***************************************************************/

typedef struct _SHUTDOWN_ENTRY
{
   LIST_ENTRY ShutdownList;
   PDEVICE_OBJECT DeviceObject;
} SHUTDOWN_ENTRY, *PSHUTDOWN_ENTRY;

static LIST_ENTRY ShutdownListHead;
static KSPIN_LOCK ShutdownListLock;

#define TAG_SHUTDOWN_ENTRY    TAG('S', 'H', 'U', 'T')

/* FUNCTIONS *****************************************************************/

VOID IoInitShutdownNotification (VOID)
{
   InitializeListHead(&ShutdownListHead);
   KeInitializeSpinLock(&ShutdownListLock);
}

VOID IoShutdownRegisteredDevices(VOID)
{
   PSHUTDOWN_ENTRY ShutdownEntry;
   PLIST_ENTRY Entry;
   IO_STATUS_BLOCK StatusBlock;
   PIRP Irp;
   KEVENT Event;
   NTSTATUS Status;

   Entry = ShutdownListHead.Flink;
   while (Entry != &ShutdownListHead)
     {
	ShutdownEntry = CONTAINING_RECORD(Entry, SHUTDOWN_ENTRY, ShutdownList);

	KeInitializeEvent (&Event,
	                   NotificationEvent,
	                   FALSE);

	Irp = IoBuildSynchronousFsdRequest (IRP_MJ_SHUTDOWN,
	                                    ShutdownEntry->DeviceObject,
	                                    NULL,
	                                    0,
	                                    NULL,
	                                    &Event,
	                                    &StatusBlock);

	Status = IoCallDriver (ShutdownEntry->DeviceObject,
	                       Irp);
	if (Status == STATUS_PENDING)
	{
		KeWaitForSingleObject (&Event,
		                       Executive,
		                       KernelMode,
		                       FALSE,
		                       NULL);
	}

	Entry = Entry->Flink;
     }
}

NTSTATUS STDCALL IoRegisterShutdownNotification(PDEVICE_OBJECT DeviceObject)
{
   PSHUTDOWN_ENTRY Entry;

   Entry = ExAllocatePoolWithTag(NonPagedPool, sizeof(SHUTDOWN_ENTRY),
				 TAG_SHUTDOWN_ENTRY);
   if (Entry == NULL)
     return STATUS_INSUFFICIENT_RESOURCES;

   Entry->DeviceObject = DeviceObject;

   ExInterlockedInsertHeadList(&ShutdownListHead,
			       &Entry->ShutdownList,
			       &ShutdownListLock);

   DeviceObject->Flags |= DO_SHUTDOWN_REGISTERED;

   return STATUS_SUCCESS;
}

VOID STDCALL IoUnregisterShutdownNotification(PDEVICE_OBJECT DeviceObject)
{
   PSHUTDOWN_ENTRY ShutdownEntry;
   PLIST_ENTRY Entry;
   KIRQL oldlvl;

   Entry = ShutdownListHead.Flink;
   while (Entry != &ShutdownListHead)
     {
	ShutdownEntry = CONTAINING_RECORD(Entry, SHUTDOWN_ENTRY, ShutdownList);
	if (ShutdownEntry->DeviceObject == DeviceObject)
	  {
	    DeviceObject->Flags &= ~DO_SHUTDOWN_REGISTERED;

	    KeAcquireSpinLock(&ShutdownListLock,&oldlvl);
	    RemoveEntryList(Entry);
	    KeReleaseSpinLock(&ShutdownListLock,oldlvl);

	    ExFreePool(Entry);
	    return;
	  }

	Entry = Entry->Flink;
     }
}

/* EOF */
