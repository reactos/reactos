/* $Id: fs.c,v 1.22 2002/04/10 09:57:31 ekohl Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/io/fs.c
 * PURPOSE:         Filesystem functions
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <internal/io.h>
#include <internal/pool.h>

#define NDEBUG
#include <internal/debug.h>

/* TYPES *******************************************************************/

typedef struct
{
  PDEVICE_OBJECT DeviceObject;
  LIST_ENTRY Entry;
} FILE_SYSTEM_OBJECT;

typedef struct
{
  LIST_ENTRY FsChangeNotifyList;
  PDRIVER_OBJECT DriverObject;
  PFSDNOTIFICATIONPROC FSDNotificationProc;
}  FS_CHANGE_NOTIFY_ENTRY, *PFS_CHANGE_NOTIFY_ENTRY;

/* GLOBALS ******************************************************************/

static KSPIN_LOCK FileSystemListLock;
static LIST_ENTRY FileSystemListHead;

static KSPIN_LOCK FsChangeNotifyListLock;
static LIST_ENTRY FsChangeNotifyListHead;

#define TAG_FILE_SYSTEM       TAG('F', 'S', 'Y', 'S')
#define TAG_FS_CHANGE_NOTIFY  TAG('F', 'S', 'C', 'N')


static VOID IopNotifyFileSystemChange(PDEVICE_OBJECT DeviceObject,
				      BOOLEAN DriverActive);

/* FUNCTIONS *****************************************************************/

NTSTATUS STDCALL
NtFsControlFile (
	IN	HANDLE			DeviceHandle,
	IN	HANDLE			EventHandle	OPTIONAL, 
	IN	PIO_APC_ROUTINE		ApcRoutine	OPTIONAL, 
	IN	PVOID			ApcContext	OPTIONAL, 
	OUT	PIO_STATUS_BLOCK	IoStatusBlock, 
	IN	ULONG			IoControlCode,
	IN	PVOID			InputBuffer, 
	IN	ULONG			InputBufferSize,
	OUT	PVOID			OutputBuffer,
	IN	ULONG			OutputBufferSize
	)
{
   NTSTATUS Status;
   PFILE_OBJECT FileObject;
   PDEVICE_OBJECT DeviceObject;
   PIRP Irp;
   PIO_STACK_LOCATION StackPtr;
   KEVENT KEvent;
   IO_STATUS_BLOCK IoSB;

   DPRINT("NtFsControlFile(DeviceHandle %x EventHandle %x ApcRoutine %x "
          "ApcContext %x IoStatusBlock %x IoControlCode %x "
          "InputBuffer %x InputBufferSize %x OutputBuffer %x "
          "OutputBufferSize %x)\n",
          DeviceHandle,EventHandle,ApcRoutine,ApcContext,IoStatusBlock,
          IoControlCode,InputBuffer,InputBufferSize,OutputBuffer,
          OutputBufferSize);

   Status = ObReferenceObjectByHandle(DeviceHandle,
				      FILE_READ_DATA | FILE_WRITE_DATA,
				      NULL,
				      KernelMode,
				      (PVOID *) &FileObject,
				      NULL);
   
   if (!NT_SUCCESS(Status))
     {
	return(Status);
     }
   
   DeviceObject = FileObject->DeviceObject;

   KeInitializeEvent(&KEvent,NotificationEvent,TRUE);

   Irp = IoBuildDeviceIoControlRequest(IoControlCode,
				       DeviceObject,
				       InputBuffer,
				       InputBufferSize,
				       OutputBuffer,
				       OutputBufferSize,
				       FALSE,
				       &KEvent,
				       &IoSB);
   
   Irp->Overlay.AsynchronousParameters.UserApcRoutine = ApcRoutine;
   Irp->Overlay.AsynchronousParameters.UserApcContext = ApcContext;

   StackPtr = IoGetNextIrpStackLocation(Irp);
   StackPtr->FileObject = FileObject;
   StackPtr->DeviceObject = DeviceObject;
   StackPtr->Parameters.FileSystemControl.InputBufferLength = InputBufferSize;
   StackPtr->Parameters.FileSystemControl.OutputBufferLength = 
     OutputBufferSize;
   StackPtr->MajorFunction = IRP_MJ_FILE_SYSTEM_CONTROL;
   
   Status = IoCallDriver(DeviceObject,Irp);
   if (Status == STATUS_PENDING && !(FileObject->Flags & FO_SYNCHRONOUS_IO))
     {
	KeWaitForSingleObject(&KEvent,Executive,KernelMode,FALSE,NULL);
	Status = IoSB.Status;
     }
   if (IoStatusBlock)
     {
        *IoStatusBlock = IoSB;
     }
   return(Status);
}


VOID
IoInitFileSystemImplementation(VOID)
{
  InitializeListHead(&FileSystemListHead);
  KeInitializeSpinLock(&FileSystemListLock);

  InitializeListHead(&FsChangeNotifyListHead);
  KeInitializeSpinLock(&FsChangeNotifyListLock);
}


VOID
IoShutdownRegisteredFileSystems(VOID)
{
   KIRQL oldlvl;
   PLIST_ENTRY current_entry;
   FILE_SYSTEM_OBJECT* current;
   PIRP Irp;
   KEVENT Event;
   IO_STATUS_BLOCK IoStatusBlock;
   NTSTATUS Status;

   DPRINT("IoShutdownRegisteredFileSystems()\n");

   KeAcquireSpinLock(&FileSystemListLock,&oldlvl);
   KeInitializeEvent(&Event,NotificationEvent,FALSE);

   current_entry = FileSystemListHead.Flink;
   while (current_entry!=(&FileSystemListHead))
     {
	current = CONTAINING_RECORD(current_entry,FILE_SYSTEM_OBJECT,Entry);

	/* send IRP_MJ_SHUTDOWN */
	Irp = IoBuildSynchronousFsdRequest(IRP_MJ_SHUTDOWN,
					   current->DeviceObject,
					   NULL,
					   0,
					   0,
					   &Event,
					   &IoStatusBlock);

	Status = IoCallDriver(current->DeviceObject,Irp);
	if (Status==STATUS_PENDING)
	  {
	     KeWaitForSingleObject(&Event,Executive,KernelMode,FALSE,NULL);
	  }

	current_entry = current_entry->Flink;
     }

   KeReleaseSpinLock(&FileSystemListLock,oldlvl);
}


NTSTATUS
IoAskFileSystemToMountDevice(PDEVICE_OBJECT DeviceObject,
			     PDEVICE_OBJECT DeviceToMount)
{
   PIRP Irp;
   IO_STATUS_BLOCK IoStatusBlock;
   NTSTATUS Status;
   PKEVENT Event;   // KEVENT must be allocated from non paged pool, not stack
   
   DPRINT("IoAskFileSystemToMountDevice(DeviceObject %x, DeviceToMount %x)\n",
	  DeviceObject,DeviceToMount);
   
   assert_irql(PASSIVE_LEVEL);
   Event = ExAllocatePool( NonPagedPool, sizeof( KEVENT ) );
   if( Event == 0 )
     return STATUS_INSUFFICIENT_RESOURCES;
   KeInitializeEvent(Event,NotificationEvent,FALSE);
   Irp = IoBuildFilesystemControlRequest(IRP_MN_MOUNT_VOLUME,
					 DeviceObject,
					 Event,
					 &IoStatusBlock,
					 DeviceToMount);
   Status = IoCallDriver(DeviceObject,Irp);
   if (Status==STATUS_PENDING)
     {
	KeWaitForSingleObject(Event,Executive,KernelMode,FALSE,NULL);
	Status = IoStatusBlock.Status;
     }
   ExFreePool( Event );
   return(Status);
}


NTSTATUS
IoAskFileSystemToLoad(PDEVICE_OBJECT DeviceObject)
{
   UNIMPLEMENTED;
}


NTSTATUS
IoTryToMountStorageDevice(IN PDEVICE_OBJECT DeviceObject,
			  IN BOOLEAN AllowRawMount)
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
   
   assert_irql(PASSIVE_LEVEL);
   
   DPRINT1("IoTryToMountStorageDevice(DeviceObject %x)\n",DeviceObject);
   
   KeAcquireSpinLock(&FileSystemListLock,&oldlvl);
   current_entry = FileSystemListHead.Flink;
   while (current_entry!=(&FileSystemListHead))
     {
	current = CONTAINING_RECORD(current_entry,FILE_SYSTEM_OBJECT,Entry);
	KeReleaseSpinLock(&FileSystemListLock,oldlvl);
	Status = IoAskFileSystemToMountDevice(current->DeviceObject, 
					      DeviceObject);
	KeAcquireSpinLock(&FileSystemListLock,&oldlvl);
	switch (Status)
	  {
	   case STATUS_FS_DRIVER_REQUIRED:
	     KeReleaseSpinLock(&FileSystemListLock,oldlvl);
	     (void)IoAskFileSystemToLoad(DeviceObject);
	     KeAcquireSpinLock(&FileSystemListLock,&oldlvl);
	     current_entry = FileSystemListHead.Flink;
	     break;
	   
	   case STATUS_SUCCESS:
	     DeviceObject->Vpb->Flags = DeviceObject->Vpb->Flags |
	                                VPB_MOUNTED;
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


/**********************************************************************
 * NAME							EXPORTED
 * 	IoVerifyVolume
 *
 * DESCRIPTION
 *	Veriyfy the file system type and volume information or mount
 *	a file system.
 *
 * ARGUMENTS
 *	DeviceObject
 *		Device to verify or mount
 *
 *	AllowRawMount
 *		...
 *
 * RETURN VALUE
 *	Status
 */
NTSTATUS STDCALL
IoVerifyVolume(IN PDEVICE_OBJECT DeviceObject,
	       IN BOOLEAN AllowRawMount)
{
#if 0
  IO_STATUS_BLOCK IoStatusBlock,
  KEVENT Event;
  PIRP Irp;
#endif
  NTSTATUS Status;

  DPRINT1("IoVerifyVolume(DeviceObject %x  AllowRawMount %x)\n",
	 DeviceObject,
	 AllowRawMount);

  Status = STATUS_SUCCESS;

  KeWaitForSingleObject(&DeviceObject->DeviceLock,
			Executive,
			KernelMode,
			FALSE,
			NULL);

  if (DeviceObject->Vpb->Flags & VPB_MOUNTED)
    {
      /* FIXME: Issue verify request to the FSD */
#if 0
      KeInitializeEvent(&Event,...);

      Irp = IoBuildFilesystemControlRequest(IRP_MN_VERIFY_VOLUME,
					    DeviceObject,
					    &Event,
					    &IoStatusBlock,
					    &DeviceToMount)


#endif

      if (NT_SUCCESS(Status))
	{
	  KeSetEvent(&DeviceObject->DeviceLock,
		     IO_NO_INCREMENT,
		     FALSE);
	  return(STATUS_SUCCESS);
	}
    }

  if (Status == STATUS_WRONG_VOLUME)
    {
      /* FIXME: Replace existing VPB by a new one */

    }

  /* Start mount sequence */
  Status = IoTryToMountStorageDevice(DeviceObject,
				     AllowRawMount);

  KeSetEvent(&DeviceObject->DeviceLock,
	     IO_NO_INCREMENT,
	     FALSE);

  return(Status);
}


VOID STDCALL
IoRegisterFileSystem(PDEVICE_OBJECT DeviceObject)
{
   FILE_SYSTEM_OBJECT* fs;
   
   DPRINT("IoRegisterFileSystem(DeviceObject %x)\n",DeviceObject);
   
   fs = ExAllocatePoolWithTag(NonPagedPool, sizeof(FILE_SYSTEM_OBJECT),
			      TAG_FILE_SYSTEM);
   assert(fs!=NULL);
   
   fs->DeviceObject = DeviceObject;
   ExInterlockedInsertTailList(&FileSystemListHead,&fs->Entry,
			       &FileSystemListLock);
   IopNotifyFileSystemChange(DeviceObject, TRUE);
}


VOID STDCALL
IoUnregisterFileSystem(PDEVICE_OBJECT DeviceObject)
{
   KIRQL oldlvl;
   PLIST_ENTRY current_entry;
   FILE_SYSTEM_OBJECT* current;

   DPRINT("IoUnregisterFileSystem(DeviceObject %x)\n",DeviceObject);
   
   KeAcquireSpinLock(&FileSystemListLock,&oldlvl);
   current_entry = FileSystemListHead.Flink;
   while (current_entry!=(&FileSystemListHead))
     {
	current = CONTAINING_RECORD(current_entry,FILE_SYSTEM_OBJECT,Entry);
	if (current->DeviceObject == DeviceObject)
	  {
	     RemoveEntryList(current_entry);
	     ExFreePool(current);
	     KeReleaseSpinLock(&FileSystemListLock,oldlvl);
	     IopNotifyFileSystemChange(DeviceObject, FALSE);
	     return;
	  }
	current_entry = current_entry->Flink;
     }
   KeReleaseSpinLock(&FileSystemListLock,oldlvl);
}


/**********************************************************************
 * NAME							EXPORTED
 * 	IoGetBaseFileSystemDeviceObject@4
 *
 * DESCRIPTION
 *	Get the DEVICE_OBJECT associated to
 *	a FILE_OBJECT.
 *
 * ARGUMENTS
 *	FileObject
 *
 * RETURN VALUE
 *
 * NOTE
 * 	From Bo Branten's ntifs.h v13.
 */
PDEVICE_OBJECT STDCALL
IoGetBaseFileSystemDeviceObject(IN PFILE_OBJECT FileObject)
{
	PDEVICE_OBJECT	DeviceObject = NULL;
	PVPB		Vpb = NULL;

	/*
	 * If the FILE_OBJECT's VPB is defined,
	 * get the device from it.
	 */
	if (NULL != (Vpb = FileObject->Vpb)) 
	{
		if (NULL != (DeviceObject = Vpb->DeviceObject))
		{
			/* Vpb->DeviceObject DEFINED! */
			return DeviceObject;
		}
	}
	/*
	 * If that failed, try the VPB
	 * in the FILE_OBJECT's DeviceObject.
	 */
	DeviceObject = FileObject->DeviceObject;
	if (NULL == (Vpb = DeviceObject->Vpb)) 
	{
		/* DeviceObject->Vpb UNDEFINED! */
		return DeviceObject;
	}
	/*
	 * If that pointer to the VPB is again
	 * undefined, return directly the
	 * device object from the FILE_OBJECT.
	 */
	return (
		(NULL == Vpb->DeviceObject)
			? DeviceObject
			: Vpb->DeviceObject
		);
}


static VOID
IopNotifyFileSystemChange(PDEVICE_OBJECT DeviceObject,
			  BOOLEAN DriverActive)
{
  PFS_CHANGE_NOTIFY_ENTRY ChangeEntry;
  PLIST_ENTRY Entry;
  KIRQL oldlvl;

  KeAcquireSpinLock(&FsChangeNotifyListLock,&oldlvl);
  Entry = FsChangeNotifyListHead.Flink;
  while (Entry != &FsChangeNotifyListHead)
    {
      ChangeEntry = CONTAINING_RECORD(Entry, FS_CHANGE_NOTIFY_ENTRY, FsChangeNotifyList);

      (ChangeEntry->FSDNotificationProc)(DeviceObject, DriverActive);

      Entry = Entry->Flink;
    }
  KeReleaseSpinLock(&FsChangeNotifyListLock,oldlvl);
}


NTSTATUS STDCALL
IoRegisterFsRegistrationChange(IN PDRIVER_OBJECT DriverObject,
			       IN PFSDNOTIFICATIONPROC FSDNotificationProc)
{
  PFS_CHANGE_NOTIFY_ENTRY Entry;

  Entry = ExAllocatePoolWithTag(NonPagedPool,
				sizeof(FS_CHANGE_NOTIFY_ENTRY),
				TAG_FS_CHANGE_NOTIFY);
  if (Entry == NULL)
    return(STATUS_INSUFFICIENT_RESOURCES);

  Entry->DriverObject = DriverObject;
  Entry->FSDNotificationProc = FSDNotificationProc;

  ExInterlockedInsertHeadList(&FsChangeNotifyListHead,
			      &Entry->FsChangeNotifyList,
			      &FsChangeNotifyListLock);

  return(STATUS_SUCCESS);
}


VOID STDCALL
IoUnregisterFsRegistrationChange(IN PDRIVER_OBJECT DriverObject,
				 IN PFSDNOTIFICATIONPROC FSDNotificationProc)
{
  PFS_CHANGE_NOTIFY_ENTRY ChangeEntry;
  PLIST_ENTRY Entry;
  KIRQL oldlvl;

  Entry = FsChangeNotifyListHead.Flink;
  while (Entry != &FsChangeNotifyListHead)
    {
      ChangeEntry = CONTAINING_RECORD(Entry, FS_CHANGE_NOTIFY_ENTRY, FsChangeNotifyList);
      if (ChangeEntry->DriverObject == DriverObject &&
	  ChangeEntry->FSDNotificationProc == FSDNotificationProc)
	{
	  KeAcquireSpinLock(&FsChangeNotifyListLock,&oldlvl);
	  RemoveEntryList(Entry);
	  KeReleaseSpinLock(&FsChangeNotifyListLock,oldlvl);

	  ExFreePool(Entry);
	  return;
	}

      Entry = Entry->Flink;
    }
}

/* EOF */
