/* $Id: create.c,v 1.4 2000/05/13 13:51:08 dwelch Exp $
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

/* GLOBALS *******************************************************************/

static LIST_ENTRY PipeListHead;
static KMUTEX PipeListLock;

/* FUNCTIONS *****************************************************************/

VOID NpfsInitPipeList(VOID)
{
   InitializeListHead(&PipeListHead);
   KeInitializeMutex(&PipeListLock, 0);
}

NTSTATUS NpfsCreate(PDEVICE_OBJECT DeviceObject,
		    PIRP Irp)		    
{
   PIO_STACK_LOCATION IoStack;
   PFILE_OBJECT FileObject;
   NTSTATUS Status;
   PNPFS_PIPE Pipe;
   PNPFS_FCB Fcb;
   PWSTR PipeName;
   PNPFS_PIPE current;
   PLIST_ENTRY current_entry;
   
   DeviceExt = (PNPFS_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
   IoStack = IoGetCurrentIrpStackLocation(Irp);
   FileObject = IoStack->FileObject;
   
   PipeName = FileObject->FileName.Buffer;
   
   Fcb = ExAllocatePool(NonPagedPool, sizeof(NPFS_FCB));
   if (Fcb == NULL)
     {
	Irp->IoStatus.Status = STATUS_NO_MEMORY;
	Irp->IoStatus.Information = 0;
	
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	
	return(STATUS_NO_MEMORY);
     }
   
   KeLockMutex(&PipeListLock);
   current_entry = PipeListHead.Flink;
   while (current_entry != &PipeListHead)
     {
	current = CONTAINING_RECORD(current_entry,
				    PipeListEntry,
				    NPFS_PIPE);
	
	if (wcscmp(Pipe->Name, current->Name) == 0)
	  {
	     break;
	  }
	
	current_entry = current_entry->Flink;
     }
   
   if (current_entry == PipeListHead)
     {
	ExFreePool(Fcb);
	KeUnlockMutex(&PipeListLock);
	
	Irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
	Irp->IoStatus.Information = 0;
	
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	
	return(STATUS_UNSUCCESSFUL);
     }
   
   Pipe = current;
   
   KeAcquireSpinLock(&Pipe->FcbListHead, &oldIrql);
   InsertTailList(&Pipe->FcbListHead, &Fcb->FcbListEntry);
   KeReleaseSpinLock(&Pipe->FcbListHead, oldIrql);   
   Fcb->WriteModeMessage = FALSE;
   Fcb->ReadModeMessage = FALSE;
   Fcb->NonBlocking = FALSE;
   Fcb->InBufferSize = PAGESIZE;
   Fcb->OutBufferSize = PAGESIZE;
   Fcb->Pipe = Pipe;
   Fcb->IsServer = FALSE;
   Fcb->OtherSide = NULL;
   
   Pipe->ReferenceCount++;
   
   KeUnlockMutex(&PipeListLock);
   
   FileObject->FsContext = Fcb;
   
   Irp->IoStatus.Status = Status;
   Irp->IoStatus.Information = 0;
   
   IoCompleteRequest(Irp, IO_NO_INCREMENT);
   
   return(Status);
}

NTSTATUS NpfsCreateNamedPipe(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
   PIO_STACK_LOCATION IoStack;
   PFILE_OBJECT FileObject;
   NTSTATUS Status;
   PNPFS_DEVICE_EXTENSION DeviceExt;
   PWSTR PipeName;
   PNPFS_PIPE Pipe;
   PNPFS_FCB Fcb;
   NTSTATUS Status;
   KIRQL oldIrql;
   PLIST_ENTRY current_entry;
   PNPFS_PIPE current;
   
   DeviceExt = (PNPFS_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
   IoStack = IoGetCurrentIrpStackLocation(Irp);
   FileObject = IoStack->FileObject;
   
   PipeName = FileObject->FileName.Buffer;
   
   Pipe = ExAllocatePool(NonPagedPool, sizeof(NPFS_PIPE));
   if (Pipe == NULL)
     {
	Irp->IoStatus.Status = STATUS_NO_MEMORY;
	Irp->IoStatus.Information = 0;
	
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	
	return(STATUS_NO_MEMORY);
     }
   
   Fcb = ExAllocatePool(NonPagedPool, sizeof(NPFS_FCB));
   if (Fcb == NULL)
     {
	ExFreePool(Pipe);
	
	Irp->IoStatus.Status = STATUS_NO_MEMORY;
	Irp->IoStatus.Information = 0;
	
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	
	return(STATUS_NO_MEMORY);
     }
   
   Pipe->Name = ExAllocatePool(NonPagedPool,
			       (wcslen(PipeName) + 1) * sizeof(WCHAR));
   if (Pipe->Name == NULL)
     {
	ExFreePool(Pipe);
	ExFreePool(Fcb);	
	
	Irp->IoStatus.Status = STATUS_NO_MEMORY;
	Irp->IoStatus.Information = 0;
	
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
     }   
   
   wcscpy(Pipe->Name, PipeName);
   Pipe->ReferenceCount = 0;
   InitializeListHead(&Pipe->FcbListHead);
   KeInitializeSpinLock(&Pipe->FcbListLock);
   Pipe->MaxInstances = IoStack->Parameters.CreateNamedPipe.MaxInstances;
   Pipe->TimeOut = IoStack->Parameters.CreateNamedPipe.TimeOut;
   
   KeLockMutex(&PipeListLock);
   current_entry = PipeListHead.Flink;
   while (current_entry != &PipeListHead)
     {
	current = CONTAINING_RECORD(current_entry,
				    PipeListEntry,
				    NPFS_PIPE);
	
	if (wcscmp(Pipe->Name, current->Name) == 0)
	  {
	     break;
	  }
	
	current_entry = current_entry->Flink;
     }
   
   if (current_entry != &PipeListHead)
     {
	ExFreePool(Pipe->Name);
	ExFreePool(Pipe);
	
	Pipe = current;
     }
   else
     {
	InsertTailList(&PipeListHead, &Pipe->PipeListEntry);
     }
   Pipe->ReferenceCount++;
   
   KeAcquireSpinLock(&Pipe->FcbListHead, &oldIrql);
   InsertTailList(&Pipe->FcbListHead, &Fcb->FcbListEntry);
   KeReleaseSpinLock(&Pipe->FcbListHead, oldIrql);   
   Fcb->WriteModeMessage = 
     IoStack->Parameters.CreateNamedPipe.WriteModeMessage;
   Fcb->ReadModeMessage = IoStack->Parameters.CreateNamedPipe.ReadModeMessage;
   Fcb->NonBlocking = IoStack->Parameters.CreateNamedPipe.NonBlocking;
   Fcb->InBufferSize = IoStack->Parameters.CreateNamedPipe.InBufferSize;
   Fcb->OutBufferSize = IoStack->Parameters.CreateNamedPipe.OutBufferSize;
   Fcb->Pipe = Pipe;
   Fcb->IsServer = TRUE;
   Fcb->OtherSide = NULL;
   
   KeUnlockMutex(PipeListLock);
   
   FileObject->FsContext = Fcb;
   
   Irp->IoStatus.Status = Status;
   Irp->IoStatus.Information = 0;
   
   IoCompleteRequest(Irp, IO_NO_INCREMENT);
   
   return(Status);
}


/* EOF */
