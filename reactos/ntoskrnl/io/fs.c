/* $Id: fs.c,v 1.13 2000/05/13 13:51:00 dwelch Exp $
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

#define NDEBUG
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

NTSTATUS
STDCALL
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

   DPRINT("NtFsControlFile(DeviceHandle %x Event %x UserApcRoutine %x "
          "UserApcContext %x IoStatusBlock %x IoControlCode %x "
          "InputBuffer %x InputBufferSize %x OutputBuffer %x "
          "OutputBufferSize %x)\n",
          DeviceHandle,Event,UserApcRoutine,UserApcContext,IoStatusBlock,
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
				       IoStatusBlock);
   
   Irp->Overlay.AsynchronousParameters.UserApcRoutine = ApcRoutine;
   Irp->Overlay.AsynchronousParameters.UserApcContext = ApcContext;

   StackPtr = IoGetNextIrpStackLocation(Irp);
   StackPtr->DeviceObject = DeviceObject;
   StackPtr->Parameters.FileSystemControl.InputBufferLength = InputBufferSize;
   StackPtr->Parameters.FileSystemControl.OutputBufferLength = 
     OutputBufferSize;
   StackPtr->MajorFunction = IRP_MJ_FILE_SYSTEM_CONTROL;
   
   Status = IoCallDriver(DeviceObject,Irp);
   if (Status == STATUS_PENDING && (FileObject->Flags & FO_SYNCHRONOUS_IO))
     {
	KeWaitForSingleObject(&KEvent,Executive,KernelMode,FALSE,NULL);
	return(IoStatusBlock->Status);
     }
   return(Status);
}

VOID IoInitFileSystemImplementation(VOID)
{
   InitializeListHead(&FileSystemListHead);
   KeInitializeSpinLock(&FileSystemListLock);
}

NTSTATUS IoAskFileSystemToMountDevice(PDEVICE_OBJECT DeviceObject,
				      PDEVICE_OBJECT DeviceToMount)
{
   PIRP Irp;
   KEVENT Event;
   IO_STATUS_BLOCK IoStatusBlock;
   NTSTATUS Status;
   
   DPRINT("IoAskFileSystemToMountDevice(DeviceObject %x, DeviceToMount %x)\n",
	  DeviceObject,DeviceToMount);
   
   assert_irql(PASSIVE_LEVEL);
   
   KeInitializeEvent(&Event,NotificationEvent,FALSE);
   Irp = IoBuildFilesystemControlRequest(IRP_MN_MOUNT_VOLUME,
					 DeviceObject,
					 &Event,
					 &IoStatusBlock,
					 DeviceToMount);
   Status = IoCallDriver(DeviceObject,Irp);
   if (Status==STATUS_PENDING)
     {
	KeWaitForSingleObject(&Event,Executive,KernelMode,FALSE,NULL);
	Status = IoStatusBlock.Status;
     }
   return(Status);
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
   
   assert_irql(PASSIVE_LEVEL);
   
   DPRINT("IoTryToMountStorageDevice(DeviceObject %x)\n",DeviceObject);
   
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

VOID STDCALL IoRegisterFileSystem(PDEVICE_OBJECT DeviceObject)
{
   FILE_SYSTEM_OBJECT* fs;
   
   DPRINT("IoRegisterFileSystem(DeviceObject %x)\n",DeviceObject);
   
   fs=ExAllocatePool(NonPagedPool,sizeof(FILE_SYSTEM_OBJECT));
   assert(fs!=NULL);
   
   fs->DeviceObject = DeviceObject;   
   ExInterlockedInsertTailList(&FileSystemListHead,&fs->Entry,
			       &FileSystemListLock);
}

VOID STDCALL IoUnregisterFileSystem(PDEVICE_OBJECT DeviceObject)
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
PDEVICE_OBJECT
STDCALL
IoGetBaseFileSystemDeviceObject (
	IN	PFILE_OBJECT	FileObject
	)
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


NTSTATUS
STDCALL
IoRegisterFsRegistrationChange (
	IN	PDRIVER_OBJECT		DriverObject,
	IN	PFSDNOTIFICATIONPROC	FSDNotificationProc
	)
{
	UNIMPLEMENTED;
	return (STATUS_NOT_IMPLEMENTED);
}


VOID
STDCALL
IoUnregisterFsRegistrationChange (
	DWORD	Unknown0,
	DWORD	Unknown1
	)
{
	UNIMPLEMENTED;
}


/* EOF */
