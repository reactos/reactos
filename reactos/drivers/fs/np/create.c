/* $Id: create.c,v 1.3 2000/03/26 22:00:09 dwelch Exp $
 *
 * COPYRIGHT:  See COPYING in the top level directory
 * PROJECT:    ReactOS kernel
 * FILE:       services/fs/np/create.c
 * PURPOSE:    Named pipe filesystem
 * PROGRAMMER: David Welch <welch@cwcom.net>
 */

/* INCLUDES ******************************************************************/

#include <ddk/ntddk.h>

//#define NDEBUG
#include <internal/debug.h>

#include "npfs.h"

/* FUNCTIONS *****************************************************************/

NTSTATUS NpfsCreate(PDEVICE_OBJECT DeviceObject,
		    PIRP Irp)		    
{
   return(Status);
}

NTSTATUS NpfsCreateNamedPipe(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
   PIO_STACK_LOCATION IoStack;
   PFILE_OBJECT FileObject;
   NTSTATUS Status;
   PNPFS_DEVICE_EXTENSION DeviceExt;
   PWSTR PipeName;
   PNPFS_FSCONTEXT PipeDescr;
   NTSTATUS Status;
   KIRQL oldIrql;
   PLIST_ENTRY current_entry;
   PNPFS_CONTEXT current;
   
   DeviceExt = (PNPFS_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
   IoStack = IoGetCurrentIrpStackLocation(Irp);
   FileObject = IoStack->FileObject;
   
   PipeName = FileObject->FileName.Buffer;
   
   PipeDescr = ExAllocatePool(NonPagedPool, sizeof(NPFS_FSCONTEXT));
   if (PipeDescr == NULL)
     {
	Irp->IoStatus.Status = STATUS_NO_MEMORY;
	Irp->IoStatus.Information = 0;
	
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	
	return(STATUS_NO_MEMORY);
     }
   PipeDescr->Name = ExAllocatePool(NonPagedPool,
				    (wcslen(PipeName) + 1) * sizeof(WCHAR));
   if (PipeDescr->Name == NULL)
     {
	ExFreePool(PipeDescr);
	
	Irp->IoStatus.Status = STATUS_NO_MEMORY;
	Irp->IoStatus.Information = 0;
	
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
     }
   
   wcscpy(PipeDescr->Name, PipeName);
   PipeDescr->FileAttributes = 
     IoStack->Parameters.CreateNamedPipe.FileAttributes;
   PipeDescr->OpenMode = 
     IoStack->Parameters.CreateNamedPipe.OpenMode;
   PipeDescr->PipeType =
     IoStack->Parameters.CreateNamedPipe.PipeType;
   PipeDescr->PipeRead =
     IoStack->Parameters.CreateNamedPipe.PipeRead;
   PipeDescr->PipeWait =
     IoStack->Parameters.CreateNamedPipe.PipeWait;
   PipeDescr->MaxInstances =
     IoStack->Parameters.CreateNamedPipe.MaxInstances;
   PipeDescr->InBufferSize =
     IoStack->Parameters.CreateNamedPipe.InBufferSize;
   PipeDescr->OutBufferSize =
     IoStack->Parameters.CreateNamedPipe.OutBufferSize;
   PipeDescr->Timeout =
     IoStack->Parameters.CreateNamedPipe.Timeout;
   KeInitializeSpinLock(&PipeDescr->MsgListLock);
   InitializeListHead(&PipeDescr->MsgListHead);
   
   /*
    * Check for duplicate names
    */
   KeAcquireSpinLock(&PipeListLock, &oldIrql);
   current_entry = PipeListHead.Flink;
   while (current_entry != &PipeListHead)
     {
	current = CONTAINING_RECORD(current_entry,
				    NPFS_FSCONTEXT,
				    ListEntry);
	
	if (wcscmp(current->Name, PipeDescr->Name) == 0)
	  {
	     KeReleaseSpinLock(&PipeListLock, oldIrql);
	     ExFreePool(PipeDescr->Name);
	     ExFreePool(PipeDescr);
	     
	     Irp->IoStatus.Status = STATUS_OBJECT_NAME_COLLISION;
	     Irp->IoStatus.Information = 0;
	     
	     IoCompleteRequest(Irp, IO_NO_INCREMENT);
	  }
	
	current_entry = current_entry->Flink;
     }
   InsertTailList(&PipeListHead,
		  &PipeDescr->ListEntry);
   KeReleaseSpinLock(&PipeListLock, oldIrql);
   
   
   
   FileObject->FsContext = PipeDescr;
   
   Irp->IoStatus.Status = Status;
   Irp->IoStatus.Information = 0;
   
   IoCompleteRequest(Irp, IO_NO_INCREMENT);
   
   return(Status);
}


/* EOF */
