/* $Id: create.c,v 1.6 2001/05/10 23:38:31 ekohl Exp $
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


/* FUNCTIONS *****************************************************************/

NTSTATUS STDCALL
NpfsCreate(PDEVICE_OBJECT DeviceObject,
	   PIRP Irp)
{
   PIO_STACK_LOCATION IoStack;
   PFILE_OBJECT FileObject;
   NTSTATUS Status;
   PNPFS_PIPE Pipe;
   PNPFS_FCB Fcb;
   PNPFS_PIPE current;
   PLIST_ENTRY current_entry;
   PNPFS_DEVICE_EXTENSION DeviceExt;
   KIRQL oldIrql;
   
   DPRINT1("NpfsCreate(DeviceObject %p Irp %p)\n", DeviceObject, Irp);
   
   DeviceExt = (PNPFS_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
   IoStack = IoGetCurrentIrpStackLocation(Irp);
   FileObject = IoStack->FileObject;
   
   Fcb = ExAllocatePool(NonPagedPool, sizeof(NPFS_FCB));
   if (Fcb == NULL)
     {
	Irp->IoStatus.Status = STATUS_NO_MEMORY;
	Irp->IoStatus.Information = 0;
	
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	
	return(STATUS_NO_MEMORY);
     }
   
   KeLockMutex(&DeviceExt->PipeListLock);
   current_entry = DeviceExt->PipeListHead.Flink;
   while (current_entry != &DeviceExt->PipeListHead)
     {
	current = CONTAINING_RECORD(current_entry,
				    NPFS_PIPE,
				    PipeListEntry);
	
	if (RtlCompareUnicodeString(&Pipe->PipeName,
				    &current->PipeName,
				    TRUE) == 0)
	  {
	     break;
	  }
	
	current_entry = current_entry->Flink;
     }
   
   if (current_entry == &DeviceExt->PipeListHead)
     {
	ExFreePool(Fcb);
	KeUnlockMutex(&DeviceExt->PipeListLock);
	
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
   
   /* search for unconnected server fcb */
   
   
   KeUnlockMutex(&DeviceExt->PipeListLock);
   
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
   
   if (RtlCreateUnicodeString(&Pipe->PipeName, FileObject->FileName.Buffer) == 0)
     {
	ExFreePool(Pipe);
	ExFreePool(Fcb);
	
	Irp->IoStatus.Status = STATUS_NO_MEMORY;
	Irp->IoStatus.Information = 0;
	
	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	return(STATUS_NO_MEMORY);
     }
   
   Pipe->ReferenceCount = 0;
   InitializeListHead(&Pipe->FcbListHead);
   KeInitializeSpinLock(&Pipe->FcbListLock);
   
   Pipe->MaxInstances = Buffer->MaxInstances;
   Pipe->TimeOut = Buffer->TimeOut;
   
   KeLockMutex(&DeviceExt->PipeListLock);
   current_entry = DeviceExt->PipeListHead.Flink;
   while (current_entry != &DeviceExt->PipeListHead)
     {
	current = CONTAINING_RECORD(current_entry,
				    NPFS_PIPE,
				    PipeListEntry);
	
	if (RtlCompareUnicodeString(&Pipe->PipeName, &current->PipeName, TRUE) == 0)
	  {
	     break;
	  }
	
	current_entry = current_entry->Flink;
     }
   
   if (current_entry != &DeviceExt->PipeListHead)
     {
	RtlFreeUnicodeString(&Pipe->PipeName);
	ExFreePool(Pipe);
	
	Pipe = current;
     }
   else
     {
	InsertTailList(&DeviceExt->PipeListHead, &Pipe->PipeListEntry);
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
   
   KeInitializeEvent(&Fcb->ConnectEvent,
		     SynchronizationEvent,
		     FALSE);
   
   KeUnlockMutex(&DeviceExt->PipeListLock);
   
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
   PNPFS_DEVICE_EXTENSION DeviceExt;
   PIO_STACK_LOCATION IoStack;
   PFILE_OBJECT FileObject;
   PNPFS_FCB Fcb;
   PNPFS_PIPE Pipe;
   KIRQL oldIrql;

   DPRINT("NpfsClose(DeviceObject %p Irp %p)\n", DeviceObject, Irp);

   IoStack = IoGetCurrentIrpStackLocation(Irp);
   DeviceExt = (PNPFS_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
   FileObject = IoStack->FileObject;
   Fcb =  FileObject->FsContext;
   Pipe = Fcb->Pipe;

   DPRINT("Closing pipe %wZ\n", &Pipe->PipeName);

   KeLockMutex(&DeviceExt->PipeListLock);

   Pipe->ReferenceCount--;

   KeAcquireSpinLock(&Pipe->FcbListLock, &oldIrql);
   RemoveEntryList(&Fcb->FcbListEntry);
   KeReleaseSpinLock(&Pipe->FcbListLock, oldIrql);
   ExFreePool(Fcb);
   FileObject->FsContext = NULL;

   if (Pipe->ReferenceCount == 0)
     {
	RtlFreeUnicodeString(&Pipe->PipeName);
	RemoveEntryList(&Pipe->PipeListEntry);
	ExFreePool(Pipe);
     }

   KeUnlockMutex(&DeviceExt->PipeListLock);

   Irp->IoStatus.Status = STATUS_SUCCESS;
   Irp->IoStatus.Information = 0;
   
   IoCompleteRequest(Irp, IO_NO_INCREMENT);
   
   return(STATUS_SUCCESS);
}

/* EOF */
