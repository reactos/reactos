/* $Id: dir.c,v 1.13 2002/04/27 19:22:09 hbirr Exp $
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

#include <ddk/ntddk.h>
#include <internal/io.h>

#define NDEBUG
#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/



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
	UNIMPLEMENTED;
}


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
   IO_STATUS_BLOCK IoSB;
   
   DPRINT("NtQueryDirectoryFile()\n");
   
   Status = ObReferenceObjectByHandle(FileHandle,
				      FILE_LIST_DIRECTORY,
				      IoFileObjectType,
				      UserMode,
				      (PVOID *)&FileObject,
				      NULL);
   
   if (Status != STATUS_SUCCESS)
     {
	ObDereferenceObject(FileObject);
	return(Status);
     }
   DeviceObject = FileObject->DeviceObject;
   
   Irp = IoAllocateIrp(DeviceObject->StackSize, TRUE);
   if (Irp==NULL)
     {
	ObDereferenceObject(FileObject);
	return STATUS_UNSUCCESSFUL;
     }
   
   
   Irp->UserIosb = &IoSB;
   Irp->UserEvent = &FileObject->Event;
   KeResetEvent( &FileObject->Event );
   Irp->UserBuffer=FileInformation;
   
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
	if (FileObject->Flags & FO_ALERTABLE_IO)
	  {
	     KeWaitForSingleObject(&FileObject->Event,Executive,KernelMode,TRUE,NULL);
	  }
	else
	  {
	     KeWaitForSingleObject(&FileObject->Event,Executive,KernelMode,FALSE,NULL);
	  }
	Status = IoSB.Status;
     }
   if (IoStatusBlock)
     {
       *IoStatusBlock = IoSB;
     }
   return(Status);
}

NTSTATUS STDCALL NtQueryOleDirectoryFile(VOID)
{
   UNIMPLEMENTED;
}


/* EOF */
