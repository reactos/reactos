/* $Id: create.c,v 1.14 2002/09/08 10:22:11 chorns Exp $
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

#define NDEBUG
#include <debug.h>


/* GLOBALS *******************************************************************/


/* FUNCTIONS *****************************************************************/

NTSTATUS STDCALL
NpfsCreate(PDEVICE_OBJECT DeviceObject,
	   PIRP Irp)
{
   PIO_STACK_LOCATION IoStack;
   PFILE_OBJECT FileObject;
   PNPFS_PIPE Pipe;
   PNPFS_FCB ClientFcb;
   PNPFS_FCB ServerFcb;
   PNPFS_PIPE current;
   PLIST_ENTRY current_entry;
   PNPFS_DEVICE_EXTENSION DeviceExt;
   KIRQL oldIrql;
   ULONG Disposition;
   
   DPRINT1("NpfsCreate(DeviceObject %p Irp %p)\n", DeviceObject, Irp);
   
   DeviceExt = (PNPFS_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
   IoStack = IoGetCurrentIrpStackLocation(Irp);
   FileObject = IoStack->FileObject;
   Disposition = ((IoStack->Parameters.Create.Options >> 24) & 0xff);
   DPRINT("FileObject %p\n", FileObject);
   DPRINT("FileName %wZ\n", &FileObject->FileName);

   ClientFcb = ExAllocatePool(NonPagedPool, sizeof(NPFS_FCB));
   if (ClientFcb == NULL)
     {
	Irp->IoStatus.Status = STATUS_NO_MEMORY;
	Irp->IoStatus.Information = 0;
	
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	DPRINT("No memory!\n");
	
	return(STATUS_NO_MEMORY);
     }
   
   KeLockMutex(&DeviceExt->PipeListLock);
   current_entry = DeviceExt->PipeListHead.Flink;
   while (current_entry != &DeviceExt->PipeListHead)
     {
	current = CONTAINING_RECORD(current_entry,
				    NPFS_PIPE,
				    PipeListEntry);
	
	if (RtlCompareUnicodeString(&FileObject->FileName,
				    &current->PipeName,
				    TRUE) == 0)
	  {
	     break;
	  }
	
	current_entry = current_entry->Flink;
     }
   
   if (current_entry == &DeviceExt->PipeListHead)
     {
	ExFreePool(ClientFcb);
	KeUnlockMutex(&DeviceExt->PipeListLock);
	
	Irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
	Irp->IoStatus.Information = 0;
	
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	DPRINT("No pipe found!\n");
	
	return(STATUS_OBJECT_NAME_NOT_FOUND);
     }
   
   Pipe = current;
   
   ClientFcb->Pipe = Pipe;
   ClientFcb->PipeEnd = FILE_PIPE_CLIENT_END;
   ClientFcb->OtherSide = NULL;
   ClientFcb->PipeState = FILE_PIPE_DISCONNECTED_STATE;
   
   /* initialize data list */
   InitializeListHead(&ClientFcb->DataListHead);
   KeInitializeSpinLock(&ClientFcb->DataListLock);
   
   KeInitializeEvent(&ClientFcb->ConnectEvent,
		     SynchronizationEvent,
		     FALSE);
   
   KeInitializeEvent(&ClientFcb->ReadEvent,
		     SynchronizationEvent,
		     FALSE);
   
   KeAcquireSpinLock(&Pipe->FcbListLock, &oldIrql);
   InsertTailList(&Pipe->ClientFcbListHead, &ClientFcb->FcbListEntry);
   KeReleaseSpinLock(&Pipe->FcbListLock, oldIrql);
   
   Pipe->ReferenceCount++;
   
   KeUnlockMutex(&DeviceExt->PipeListLock);

#if 0
  if (Disposition == OPEN_EXISTING)
    {
      /* do not connect to listening servers */
      FileObject->FsContext = ClientFcb;

      Irp->IoStatus.Status = STATUS_SUCCESS;
      Irp->IoStatus.Information = 0;

      IoCompleteRequest(Irp, IO_NO_INCREMENT);
      DPRINT("Success!\n");

      return(STATUS_SUCCESS);
    }
#endif

   /* search for disconnected or listening server fcb */
   current_entry = Pipe->ServerFcbListHead.Flink;
   while (current_entry != &Pipe->ServerFcbListHead)
     {
	ServerFcb = CONTAINING_RECORD(current_entry,
				      NPFS_FCB,
				      FcbListEntry);
	if ((ServerFcb->PipeState == FILE_PIPE_LISTENING_STATE)
	    || (ServerFcb->PipeState == FILE_PIPE_DISCONNECTED_STATE))
	  {
	     DPRINT("Server found! Fcb %p\n", ServerFcb);
	     break;
	  }
	current_entry = current_entry->Flink;
     }
   
   if (current_entry == &Pipe->ServerFcbListHead)
     {
	DPRINT("No server fcb found!\n");

	FileObject->FsContext = ClientFcb;

	Irp->IoStatus.Status = STATUS_SUCCESS;
	Irp->IoStatus.Information = 0;

	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	return(STATUS_SUCCESS);
     }
   
   ClientFcb->OtherSide = ServerFcb;
   ServerFcb->OtherSide = ClientFcb;
   ClientFcb->PipeState = FILE_PIPE_CONNECTED_STATE;
   ServerFcb->PipeState = FILE_PIPE_CONNECTED_STATE;
   
   /* FIXME: create data queue(s) */
   
   /* wake server thread */
   KeSetEvent(&ServerFcb->ConnectEvent, 0, FALSE);
   
   FileObject->FsContext = ClientFcb;
   
   Irp->IoStatus.Status = STATUS_SUCCESS;
   Irp->IoStatus.Information = 0;
   
   IoCompleteRequest(Irp, IO_NO_INCREMENT);
   DPRINT("Success!\n");
   
   return(STATUS_SUCCESS);
}


NTSTATUS STDCALL
NpfsCreateNamedPipe(PDEVICE_OBJECT DeviceObject,
		    PIRP Irp)
{
   PIO_STACK_LOCATION IoStack;
   PFILE_OBJECT FileObject;
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
   DPRINT("FileObject %p\n", FileObject);
   DPRINT("Pipe name %wZ\n", &FileObject->FileName);
   
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
   InitializeListHead(&Pipe->ServerFcbListHead);
   InitializeListHead(&Pipe->ClientFcbListHead);
   KeInitializeSpinLock(&Pipe->FcbListLock);

   Pipe->PipeType = Buffer->WriteModeMessage ? FILE_PIPE_MESSAGE_TYPE : FILE_PIPE_BYTE_STREAM_TYPE;
   Pipe->PipeWriteMode = Buffer->WriteModeMessage ? FILE_PIPE_MESSAGE_MODE : FILE_PIPE_BYTE_STREAM_MODE;
   Pipe->PipeReadMode = Buffer->ReadModeMessage ? FILE_PIPE_MESSAGE_MODE : FILE_PIPE_BYTE_STREAM_MODE;
   Pipe->PipeBlockMode = Buffer->NonBlocking;
   Pipe->PipeConfiguration = IoStack->Parameters.Create.Options & 0x3;
   Pipe->MaximumInstances = Buffer->MaxInstances;
   Pipe->CurrentInstances = 0;
   Pipe->TimeOut = Buffer->TimeOut;
   Pipe->InboundQuota = Buffer->InBufferSize;
   Pipe->OutboundQuota = Buffer->OutBufferSize;
   
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
   Pipe->CurrentInstances++;
   
   KeAcquireSpinLock(&Pipe->FcbListLock, &oldIrql);
   InsertTailList(&Pipe->ServerFcbListHead, &Fcb->FcbListEntry);
   KeReleaseSpinLock(&Pipe->FcbListLock, oldIrql);
   
   Fcb->Pipe = Pipe;
   Fcb->PipeEnd = FILE_PIPE_SERVER_END;
   Fcb->OtherSide = NULL;
   Fcb->PipeState = FILE_PIPE_DISCONNECTED_STATE;
   Fcb->ReadDataAvailable = 0;
   Fcb->WriteQuotaAvailable = 0;

   /* initialize data list */
   InitializeListHead(&Fcb->DataListHead);
   KeInitializeSpinLock(&Fcb->DataListLock);

   KeInitializeEvent(&Fcb->ConnectEvent,
		     SynchronizationEvent,
		     FALSE);
   
   KeInitializeEvent(&Fcb->ReadEvent,
		     SynchronizationEvent,
		     FALSE);


   KeUnlockMutex(&DeviceExt->PipeListLock);
   
   FileObject->FsContext = Fcb;
   
   Irp->IoStatus.Status = STATUS_SUCCESS;
   Irp->IoStatus.Information = 0;
   
   IoCompleteRequest(Irp, IO_NO_INCREMENT);
   
   return(STATUS_SUCCESS);
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
  PLIST_ENTRY CurrentEntry;
  PNPFS_PIPE_DATA Current;


  DPRINT1("NpfsClose(DeviceObject %p Irp %p)\n", DeviceObject, Irp);

  IoStack = IoGetCurrentIrpStackLocation(Irp);
  DeviceExt = (PNPFS_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
  FileObject = IoStack->FileObject;
  Fcb =  FileObject->FsContext;

  if (Fcb == NULL)
    {
      Irp->IoStatus.Status = STATUS_SUCCESS;
      Irp->IoStatus.Information = 0;

      IoCompleteRequest(Irp, IO_NO_INCREMENT);

      return(STATUS_SUCCESS);
    }

  DPRINT("Fcb %x\n", Fcb);
  Pipe = Fcb->Pipe;

  DPRINT("Closing pipe %wZ\n", &Pipe->PipeName);

  KeLockMutex(&DeviceExt->PipeListLock);


   if (Fcb->PipeEnd == FILE_PIPE_SERVER_END)
    {
      /* FIXME: Clean up existing connections here ?? */
      DPRINT("Server\n");
      Pipe->CurrentInstances--;
      if (Fcb->PipeState == FILE_PIPE_CONNECTED_STATE)
      {
	   if (Fcb->OtherSide)
	   {
	      Fcb->OtherSide->PipeState = FILE_PIPE_CLOSING_STATE;
	   }
           Fcb->PipeState = FILE_PIPE_DISCONNECTED_STATE;
      }
    }
  Pipe->ReferenceCount--;

  if (Fcb->PipeEnd == FILE_PIPE_CLIENT_END)
  {
      DPRINT("Client\n");
      if (Fcb->PipeState == FILE_PIPE_CONNECTED_STATE)
      {
         if (Fcb->OtherSide)
	 {
	    Fcb->OtherSide->PipeState = FILE_PIPE_CLOSING_STATE;

	    /* Signaling the read event. If is possible that an other
	     * thread waits of read data.
	     */
            KeSetEvent(&Fcb->OtherSide->ReadEvent, IO_NO_INCREMENT, FALSE);
	 }
         Fcb->PipeState = FILE_PIPE_DISCONNECTED_STATE;
      }
  }
  
  FileObject->FsContext = NULL;

  if (Pipe->ReferenceCount == 0)
    {
      KeAcquireSpinLock(&Pipe->FcbListLock, &oldIrql);
      if (Fcb->OtherSide)
      {
         RemoveEntryList(&Fcb->OtherSide->FcbListEntry);
      }
      RemoveEntryList(&Fcb->FcbListEntry);
      KeReleaseSpinLock(&Pipe->FcbListLock, oldIrql);
      if (Fcb->OtherSide)
      {  
	 KeAcquireSpinLock(&Fcb->OtherSide->DataListLock, &oldIrql);
	 while (!IsListEmpty(&Fcb->OtherSide->DataListHead))
	 {
	     CurrentEntry = RemoveHeadList(&Fcb->OtherSide->DataListHead);
	     Current = CONTAINING_RECORD(CurrentEntry, NPFS_PIPE_DATA, ListEntry);

	     NpfsFreePipeData(Current);
	 }
         KeReleaseSpinLock(&Fcb->OtherSide->DataListLock, oldIrql);
         ExFreePool(Fcb->OtherSide);
      }

      KeAcquireSpinLock(&Fcb->DataListLock, &oldIrql);
      while (!IsListEmpty(&Fcb->DataListHead))
      {
         CurrentEntry = RemoveHeadList(&Fcb->DataListHead);
	 Current = CONTAINING_RECORD(CurrentEntry, NPFS_PIPE_DATA, ListEntry);
	 NpfsFreePipeData(Current);
      }
      KeReleaseSpinLock(&Fcb->DataListLock, oldIrql);
      ExFreePool(Fcb);
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
