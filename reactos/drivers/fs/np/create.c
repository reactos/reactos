/* $Id: create.c,v 1.5 2001/05/01 11:09:01 ekohl Exp $
 *
 * COPYRIGHT:  See COPYING in the top level directory
 * PROJECT:    ReactOS kernel
 * FILE:       services/fs/np/create.c
 * PURPOSE:    Named pipe filesystem
 * PROGRAMMER: David Welch <welch@cwcom.net>
 */

/* INCLUDES ******************************************************************/

#include <ddk/ntddk.h>

#include "npfs.h"

//#define NDEBUG
#include <debug.h>


/* GLOBALS *******************************************************************/

static LIST_ENTRY PipeListHead;
static KMUTEX PipeListLock;

/* FUNCTIONS *****************************************************************/

VOID NpfsInitPipeList(VOID)
{
   InitializeListHead(&PipeListHead);
   KeInitializeMutex(&PipeListLock, 0);
}

NTSTATUS STDCALL
NpfsCreate(PDEVICE_OBJECT DeviceObject,
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
   PNPFS_DEVICE_EXTENSION DeviceExt;
   KIRQL oldIrql;
   
   DPRINT1("NpfsCreate(DeviceObject %p Irp %p)\n", DeviceObject, Irp);
   
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
				    NPFS_PIPE,
				    PipeListEntry);
	
	if (wcscmp(Pipe->Name, current->Name) == 0)
	  {
	     break;
	  }
	
	current_entry = current_entry->Flink;
     }
   
   if (current_entry == &PipeListHead)
     {
	ExFreePool(Fcb);
	KeUnlockMutex(&PipeListLock);
	
	Irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
	Irp->IoStatus.Information = 0;
	
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	
	return(STATUS_UNSUCCESSFUL);
     }
   
   Pipe = current;
   
   KeAcquireSpinLock(&Pipe->FcbListLock, &oldIrql);
   InsertTailList(&Pipe->FcbListHead, &Fcb->FcbListEntry);
   KeReleaseSpinLock(&Pipe->FcbListLock, oldIrql);
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

NTSTATUS STDCALL
NpfsCreateNamedPipe(PDEVICE_OBJECT DeviceObject,
		    PIRP Irp)
{
   PIO_STACK_LOCATION IoStack;
   PFILE_OBJECT FileObject;
   NTSTATUS Status = STATUS_SUCCESS;
   PNPFS_DEVICE_EXTENSION DeviceExt;
   PWSTR PipeName;
   PNPFS_PIPE Pipe;
   PNPFS_FCB Fcb;
   KIRQL oldIrql;
   PLIST_ENTRY current_entry;
   PNPFS_PIPE current;
   PIO_PIPE_CREATE_BUFFER Buffer;
   
   DPRINT1("NpfsCreateNamedPipe(DeviceObject %p Irp %p)\n", DeviceObject, Irp);
   
   DeviceExt = (PNPFS_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
   IoStack = IoGetCurrentIrpStackLocation(Irp);
   FileObject = IoStack->FileObject;
   
   PipeName = FileObject->FileName.Buffer;
   
   Buffer = (PIO_PIPE_CREATE_BUFFER)Irp->Tail.Overlay.AuxiliaryBuffer;
   
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

	return(STATUS_NO_MEMORY);
     }
   
   wcscpy(Pipe->Name, PipeName);
   Pipe->ReferenceCount = 0;
   InitializeListHead(&Pipe->FcbListHead);
   KeInitializeSpinLock(&Pipe->FcbListLock);
   
   Pipe->MaxInstances = Buffer->MaxInstances;
   Pipe->TimeOut = Buffer->TimeOut;
   
   KeLockMutex(&PipeListLock);
   current_entry = PipeListHead.Flink;
   while (current_entry != &PipeListHead)
     {
	current = CONTAINING_RECORD(current_entry,
				    NPFS_PIPE,
				    PipeListEntry);
	
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
   
   KeAcquireSpinLock(&Pipe->FcbListLock, &oldIrql);
   InsertTailList(&Pipe->FcbListHead, &Fcb->FcbListEntry);
   KeReleaseSpinLock(&Pipe->FcbListLock, oldIrql);
   
   Fcb->WriteModeMessage = Buffer->WriteModeMessage;
   Fcb->ReadModeMessage = Buffer->ReadModeMessage;
   Fcb->NonBlocking = Buffer->NonBlocking;
   Fcb->InBufferSize = Buffer->InBufferSize;
   Fcb->OutBufferSize = Buffer->OutBufferSize;
   
   Fcb->Pipe = Pipe;
   Fcb->IsServer = TRUE;
   Fcb->OtherSide = NULL;
   
   KeUnlockMutex(&PipeListLock);
   
   FileObject->FsContext = Fcb;
   
   Irp->IoStatus.Status = Status;
   Irp->IoStatus.Information = 0;
   
   IoCompleteRequest(Irp, IO_NO_INCREMENT);
   
   return(Status);
}


NTSTATUS STDCALL
NpfsClose(PDEVICE_OBJECT DeviceObject,
	  PIRP Irp)
{
   PIO_STACK_LOCATION IoStack;
   PFILE_OBJECT FileObject;
   PNPFS_FCB Fcb;
   NTSTATUS Status;

   DPRINT1("NpfsClose(DeviceObject %p Irp %p)\n", DeviceObject, Irp);

   IoStack = IoGetCurrentIrpStackLocation(Irp);
   FileObject = IoStack->FileObject;
   Fcb =  FileObject->FsContext;

   DPRINT1("Closing pipe %S\n", Fcb->Pipe->Name);

   Status = STATUS_SUCCESS;

   Irp->IoStatus.Status = Status;
   Irp->IoStatus.Information = 0;
   
   IoCompleteRequest(Irp, IO_NO_INCREMENT);
   
   return(Status);
}

/* EOF */
