/* $Id: dir.c,v 1.26 2004/12/23 12:34:59 ekohl Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/io/dir.c
 * PURPOSE:         Directory functions
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/



/*
 * @implemented
 */
NTSTATUS
STDCALL
NtNotifyChangeDirectoryFile (
	IN	HANDLE			FileHandle,
	IN	HANDLE			Event		OPTIONAL, 
	IN	PIO_APC_ROUTINE		ApcRoutine	OPTIONAL, 
	IN	PVOID			ApcContext	OPTIONAL, 
	OUT	PIO_STATUS_BLOCK	IoStatusBlock,
	OUT	PVOID			Buffer,
	IN	ULONG			BufferSize,
	IN	ULONG			CompletionFilter,
	IN	BOOLEAN			WatchTree
	)
{
   PIRP Irp;
   PDEVICE_OBJECT DeviceObject;
   PFILE_OBJECT FileObject;
   NTSTATUS Status;
   PIO_STACK_LOCATION IoStack;
   KPROCESSOR_MODE PreviousMode;
   
   DPRINT("NtNotifyChangeDirectoryFile()\n");

   PreviousMode = ExGetPreviousMode();

   Status = ObReferenceObjectByHandle(FileHandle,
				      FILE_LIST_DIRECTORY,
				      IoFileObjectType,
				      PreviousMode,
				      (PVOID *)&FileObject,
				      NULL);
   
   if (Status != STATUS_SUCCESS)
     {
	return(Status);
     }
   DeviceObject = FileObject->DeviceObject;
   
   Irp = IoAllocateIrp(DeviceObject->StackSize, TRUE);
   if (Irp==NULL)
     {
	ObDereferenceObject(FileObject);
	return STATUS_UNSUCCESSFUL;
     }
   
   if (Event == NULL)
     {
       Event = &FileObject->Event;
     }

   /* Trigger FileObject/Event dereferencing */
   Irp->Tail.Overlay.OriginalFileObject = FileObject;
   Irp->RequestorMode = PreviousMode;
   Irp->UserIosb = IoStatusBlock;
   Irp->Tail.Overlay.Thread = PsGetCurrentThread();
   Irp->UserEvent = Event;
   KeResetEvent( Event );
   Irp->UserBuffer = Buffer;
   Irp->Overlay.AsynchronousParameters.UserApcRoutine = ApcRoutine;
   Irp->Overlay.AsynchronousParameters.UserApcContext = ApcContext;
   
   IoStack = IoGetNextIrpStackLocation(Irp);
   
   IoStack->MajorFunction = IRP_MJ_DIRECTORY_CONTROL;
   IoStack->MinorFunction = IRP_MN_NOTIFY_CHANGE_DIRECTORY;
   IoStack->Flags = 0;
   IoStack->Control = 0;
   IoStack->DeviceObject = DeviceObject;
   IoStack->FileObject = FileObject;
   
   if (WatchTree)
     {
	IoStack->Flags = SL_WATCH_TREE;
     }

   IoStack->Parameters.NotifyDirectory.CompletionFilter = CompletionFilter;
   IoStack->Parameters.NotifyDirectory.Length = BufferSize;
   
   Status = IoCallDriver(FileObject->DeviceObject,Irp);

   /* FIXME: Should we wait here or not for synchronously opened files? */

   return Status;
}


/*
 * @implemented
 */
NTSTATUS
STDCALL 
NtQueryDirectoryFile(
	IN	HANDLE			FileHandle,
	IN	HANDLE			PEvent		OPTIONAL,
	IN	PIO_APC_ROUTINE		ApcRoutine	OPTIONAL,
	IN	PVOID			ApcContext	OPTIONAL,
	OUT	PIO_STATUS_BLOCK	IoStatusBlock,
	OUT	PVOID			FileInformation,
	IN	ULONG			Length,
	IN	FILE_INFORMATION_CLASS	FileInformationClass,
	IN	BOOLEAN			ReturnSingleEntry,
	IN	PUNICODE_STRING		FileName	OPTIONAL,
	IN	BOOLEAN			RestartScan
	)
/*
 * FUNCTION: Queries a directory file.
 * ARGUMENTS:
 *	  FileHandle = Handle to a directory file
 *        EventHandle  = Handle to the event signaled on completion
 *	  ApcRoutine = Asynchroneous procedure callback, called on completion
 *	  ApcContext = Argument to the apc.
 *	  IoStatusBlock = Caller supplies storage for extended status information.
 *	  FileInformation = Caller supplies storage for the resulting information.
 *
 *		FileNameInformation  		FILE_NAMES_INFORMATION
 *		FileDirectoryInformation  	FILE_DIRECTORY_INFORMATION
 *		FileFullDirectoryInformation 	FILE_FULL_DIRECTORY_INFORMATION
 *		FileBothDirectoryInformation	FILE_BOTH_DIR_INFORMATION
 *
 *	  Length = Size of the storage supplied
 *	  FileInformationClass = Indicates the type of information requested.  
 *	  ReturnSingleEntry = Specify true if caller only requests the first 
 *                            directory found.
 *	  FileName = Initial directory name to query, that may contain wild 
 *                   cards.
 *        RestartScan = Number of times the action should be repeated
 * RETURNS: Status [ STATUS_SUCCESS, STATUS_ACCESS_DENIED, STATUS_INSUFFICIENT_RESOURCES,
 *		     STATUS_INVALID_PARAMETER, STATUS_INVALID_DEVICE_REQUEST, STATUS_BUFFER_OVERFLOW,
 *		     STATUS_INVALID_INFO_CLASS, STATUS_NO_SUCH_FILE, STATUS_NO_MORE_FILES ]
 */
{
   PIRP Irp;
   PDEVICE_OBJECT DeviceObject;
   PFILE_OBJECT FileObject;
   NTSTATUS Status;
   PIO_STACK_LOCATION IoStack;
   KPROCESSOR_MODE PreviousMode;
   
   DPRINT("NtQueryDirectoryFile()\n");

   PreviousMode = ExGetPreviousMode();

   Status = ObReferenceObjectByHandle(FileHandle,
				      FILE_LIST_DIRECTORY,
				      IoFileObjectType,
				      PreviousMode,
				      (PVOID *)&FileObject,
				      NULL);
   
   if (Status != STATUS_SUCCESS)
     {
	return(Status);
     }
   DeviceObject = FileObject->DeviceObject;
   
   Irp = IoAllocateIrp(DeviceObject->StackSize, TRUE);
   if (Irp==NULL)
     {
	ObDereferenceObject(FileObject);
	return STATUS_UNSUCCESSFUL;
     }
   
   /* Trigger FileObject/Event dereferencing */
   Irp->Tail.Overlay.OriginalFileObject = FileObject;
   Irp->RequestorMode = PreviousMode;
   Irp->UserIosb = IoStatusBlock;
   Irp->UserEvent = &FileObject->Event;
   Irp->Tail.Overlay.Thread = PsGetCurrentThread();
   KeResetEvent( &FileObject->Event );
   Irp->UserBuffer=FileInformation;
   Irp->Overlay.AsynchronousParameters.UserApcRoutine = ApcRoutine;
   Irp->Overlay.AsynchronousParameters.UserApcContext = ApcContext;
   
   IoStack = IoGetNextIrpStackLocation(Irp);
   
   IoStack->MajorFunction = IRP_MJ_DIRECTORY_CONTROL;
   IoStack->MinorFunction = IRP_MN_QUERY_DIRECTORY;
   IoStack->Flags = 0;
   IoStack->Control = 0;
   IoStack->DeviceObject = DeviceObject;
   IoStack->FileObject = FileObject;
   
   if (RestartScan)
     {
	IoStack->Flags = IoStack->Flags | SL_RESTART_SCAN;
     }
   if (ReturnSingleEntry)
     {
	IoStack->Flags = IoStack->Flags | SL_RETURN_SINGLE_ENTRY;
     }
   if (((PFILE_DIRECTORY_INFORMATION)FileInformation)->FileIndex != 0)
     {
	IoStack->Flags = IoStack->Flags | SL_INDEX_SPECIFIED;
     }

   IoStack->Parameters.QueryDirectory.FileInformationClass = 
     FileInformationClass;
   IoStack->Parameters.QueryDirectory.FileName = FileName;
   IoStack->Parameters.QueryDirectory.Length = Length;
   
   Status = IoCallDriver(FileObject->DeviceObject,Irp);
   if (Status==STATUS_PENDING && !(FileObject->Flags & FO_SYNCHRONOUS_IO))
     {
	KeWaitForSingleObject(&FileObject->Event,
			      Executive,
			      PreviousMode,
			      FileObject->Flags & FO_ALERTABLE_IO,
			      NULL);
	Status = IoStatusBlock->Status;
     }

   return(Status);
}

NTSTATUS STDCALL NtQueryOleDirectoryFile(VOID)
{
   UNIMPLEMENTED;
   return(STATUS_NOT_IMPLEMENTED);
}


/* EOF */
