/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/io/fs.c
 * PURPOSE:         Filesystem functions
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <internal/kernel.h>
#include <internal/linkage.h>
#include <ddk/ntddk.h>

#include <internal/debug.h>

/* TYPES *******************************************************************/

typedef struct
{
   PDEVICE_OBJECT DeviceObject;
   LIST_ENTRY Entry;
} FILE_SYSTEM_OBJECT;

/* GLOBALS ******************************************************************/

static KSPIN_LOCK FileSystemListLock;
static LIST_ENTRY FileSystemListHead;

/* FUNCTIONS *****************************************************************/

VOID IoInitFileSystemImplementation(VOID)
{
   InitializeListHead(&FileSystemListHead);
   KeInitializeSpinLock(&FileSystemListLock);
}

NTSTATUS IoAskFileSystemToMountDevice(PDEVICE_OBJECT DeviceObject)
{
   UNIMPLEMENTED;
}

NTSTATUS IoAskFileSystemToLoad(PDEVICE_OBJECT DeviceObject)
{
   UNIMPLEMENTED;
}

NTSTATUS IoTryToMountStorageDevice(PDEVICE_OBJECT DeviceObject)
/*
 * FUNCTION: Trys to mount a storage device
 * ARGUMENTS:
 *         DeviceObject = Device to try and mount
 * RETURNS: Status
 */
{
   KIRQL oldlvl;
   PLIST_ENTRY current_entry;
   FILE_SYSTEM_OBJECT* current;
   NTSTATUS Status;
   
   KeAcquireSpinLock(&FileSystemListLock,&oldlvl);
   current_entry = FileSystemListHead.Flink;
   while (current_entry!=NULL)
     {
	current = CONTAINING_RECORD(current_entry,FILE_SYSTEM_OBJECT,Entry);
	Status = IoAskFileSystemToMountDevice(DeviceObject);
	switch (Status)
	  {
	   case STATUS_FS_DRIVER_REQUIRED:
	     KeReleaseSpinLock(&FileSystemListLock,oldlvl);
	     (void)IoAskFileSystemToLoad(DeviceObject);
	     KeAcquireSpinLock(&FileSystemListLock,&oldlvl);
	     current_entry = FileSystemListHead.Flink;
	     break;
	   
	   case STATUS_SUCCESS:
	     KeReleaseSpinLock(&FileSystemListLock,oldlvl);
	     return(STATUS_SUCCESS);
	     
	   case STATUS_UNRECOGNIZED_VOLUME:
	   default:
	     current_entry = current_entry->Flink;
	  }
     }
   KeReleaseSpinLock(&FileSystemListLock,oldlvl);
   return(STATUS_UNRECOGNIZED_VOLUME);
}

VOID IoRegisterFileSystem(PDEVICE_OBJECT DeviceObject)
{
   FILE_SYSTEM_OBJECT* fs;
   
   fs=ExAllocatePool(NonPagedPool,sizeof(FILE_SYSTEM_OBJECT));
   assert(fs!=NULL);
   
   fs->DeviceObject = DeviceObject;   
   ExInterlockedInsertTailList(&FileSystemListHead,&fs->Entry,
			       &FileSystemListLock);
}

VOID IoUnregisterFileSystem(PDEVICE_OBJECT DeviceObject)
{
   KIRQL oldlvl;
   PLIST_ENTRY current_entry;
   FILE_SYSTEM_OBJECT* current;
   
   KeAcquireSpinLock(&FileSystemListLock,&oldlvl);
   current_entry = FileSystemListHead.Flink;
   while (current_entry!=NULL)
     {
	current = CONTAINING_RECORD(current_entry,FILE_SYSTEM_OBJECT,Entry);
	if (current->DeviceObject == DeviceObject)
	  {
	     RemoveEntryFromList(&FileSystemListHead,current_entry);
	     KeReleaseSpinLock(&FileSystemListLock,oldlvl);
	     return;
	  }
	current_entry = current_entry->Flink;
     }
   KeReleaseSpinLock(&FileSystemListLock,oldlvl);
}


