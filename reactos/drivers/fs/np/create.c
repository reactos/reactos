/* $Id$
 *
 * COPYRIGHT:  See COPYING in the top level directory
 * PROJECT:    ReactOS kernel
 * FILE:       drivers/fs/np/create.c
 * PURPOSE:    Named pipe filesystem
 * PROGRAMMER: David Welch <welch@cwcom.net>
 */

/* INCLUDES ******************************************************************/

#include <ddk/ntddk.h>

#include "npfs.h"

#define NDEBUG
#include <debug.h>

/* FUNCTIONS *****************************************************************/

static PNPFS_PIPE
NpfsFindPipe(PNPFS_DEVICE_EXTENSION DeviceExt,
	     PUNICODE_STRING PipeName)
{
  PLIST_ENTRY CurrentEntry;
  PNPFS_PIPE Pipe;

  CurrentEntry = DeviceExt->PipeListHead.Flink;
  while (CurrentEntry != &DeviceExt->PipeListHead)
    {
      Pipe = CONTAINING_RECORD(CurrentEntry, NPFS_PIPE, PipeListEntry);
      if (RtlCompareUnicodeString(PipeName,
				  &Pipe->PipeName,
				  TRUE) == 0)
	{
	  DPRINT("<%wZ> = <%wZ>\n", PipeName, &Pipe->PipeName);
	  return Pipe;
	}

       CurrentEntry = CurrentEntry->Flink;
     }

  return NULL;
}


static PNPFS_FCB
NpfsFindListeningServerInstance(PNPFS_PIPE Pipe)
{
  PLIST_ENTRY CurrentEntry;
  PNPFS_FCB ServerFcb;

  CurrentEntry = Pipe->ServerFcbListHead.Flink;
  while (CurrentEntry != &Pipe->ServerFcbListHead)
    {
      ServerFcb = CONTAINING_RECORD(CurrentEntry, NPFS_FCB, FcbListEntry);
      if (ServerFcb->PipeState == FILE_PIPE_LISTENING_STATE)
	{
	  DPRINT("Server found! Fcb %p\n", ServerFcb);
	  return ServerFcb;
	}
      CurrentEntry = CurrentEntry->Flink;
    }

  return NULL;
}


NTSTATUS STDCALL
NpfsCreate(PDEVICE_OBJECT DeviceObject,
	   PIRP Irp)
{
  PIO_STACK_LOCATION IoStack;
  PFILE_OBJECT FileObject;
  PNPFS_PIPE Pipe;
  PNPFS_FCB ClientFcb;
  PNPFS_FCB ServerFcb = NULL;
  PNPFS_DEVICE_EXTENSION DeviceExt;
  BOOLEAN SpecialAccess;

  DPRINT("NpfsCreate(DeviceObject %p Irp %p)\n", DeviceObject, Irp);

  DeviceExt = (PNPFS_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
  IoStack = IoGetCurrentIrpStackLocation(Irp);
  FileObject = IoStack->FileObject;
  DPRINT("FileObject %p\n", FileObject);
  DPRINT("FileName %wZ\n", &FileObject->FileName);

  Irp->IoStatus.Information = 0;

  SpecialAccess = ((IoStack->Parameters.Create.ShareAccess & 3) == 3);
  if (SpecialAccess)
    {
      DPRINT("NpfsCreate() open client end for special use!\n");
    }

  /*
   * Step 1. Find the pipe we're trying to open.
   */
  KeLockMutex(&DeviceExt->PipeListLock);
  Pipe = NpfsFindPipe(DeviceExt,
		      &FileObject->FileName);
  if (Pipe == NULL)
    {
      /* Not found, bail out with error. */
      DPRINT("No pipe found!\n");
      KeUnlockMutex(&DeviceExt->PipeListLock);
      Irp->IoStatus.Status = STATUS_OBJECT_NAME_NOT_FOUND;
      IoCompleteRequest(Irp, IO_NO_INCREMENT);
      return STATUS_OBJECT_NAME_NOT_FOUND;
    }

  KeUnlockMutex(&DeviceExt->PipeListLock);

  /*
   * Step 2. Search for listening server FCB.
   */

  /*
   * Acquire the lock for FCB lists. From now on no modifications to the
   * FCB lists are allowed, because it can cause various misconsistencies.
   */
  KeLockMutex(&Pipe->FcbListLock);

  if (!SpecialAccess)
    {
      ServerFcb = NpfsFindListeningServerInstance(Pipe);
      if (ServerFcb == NULL)
        {
          /* Not found, bail out with error for FILE_OPEN requests. */
          DPRINT("No listening server fcb found!\n");
          KeUnlockMutex(&Pipe->FcbListLock);
          Irp->IoStatus.Status = STATUS_PIPE_BUSY;
          IoCompleteRequest(Irp, IO_NO_INCREMENT);
          return STATUS_PIPE_BUSY;
        }
    }
  else if (IsListEmpty(&Pipe->ServerFcbListHead))
    {
      DPRINT("No server fcb found!\n");
      KeUnlockMutex(&Pipe->FcbListLock);
      Irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
      IoCompleteRequest(Irp, IO_NO_INCREMENT);
      return STATUS_UNSUCCESSFUL;
    }

  /*
   * Step 3. Create the client FCB.
   */
  ClientFcb = ExAllocatePool(NonPagedPool, sizeof(NPFS_FCB));
  if (ClientFcb == NULL)
    {
      DPRINT("No memory!\n");
      KeUnlockMutex(&Pipe->FcbListLock);
      Irp->IoStatus.Status = STATUS_NO_MEMORY;
      IoCompleteRequest(Irp, IO_NO_INCREMENT);
      return STATUS_NO_MEMORY;
    }

  ClientFcb->Thread = (struct ETHREAD *)Irp->Tail.Overlay.Thread;
  ClientFcb->Pipe = Pipe;
  ClientFcb->PipeEnd = FILE_PIPE_CLIENT_END;
  ClientFcb->OtherSide = NULL;
  ClientFcb->PipeState = SpecialAccess ? 0 : FILE_PIPE_DISCONNECTED_STATE;

  /* Initialize data list. */
  if (Pipe->OutboundQuota)
    {
      ClientFcb->Data = ExAllocatePool(NonPagedPool, Pipe->OutboundQuota);
      if (ClientFcb->Data == NULL)
        {
          DPRINT("No memory!\n");
          ExFreePool(ClientFcb);
          KeUnlockMutex(&Pipe->FcbListLock);
          Irp->IoStatus.Status = STATUS_NO_MEMORY;
          IoCompleteRequest(Irp, IO_NO_INCREMENT);
          return STATUS_NO_MEMORY;
        }
    }
  else
    {
      ClientFcb->Data = NULL;
    }

  ClientFcb->ReadPtr = ClientFcb->Data;
  ClientFcb->WritePtr = ClientFcb->Data;
  ClientFcb->ReadDataAvailable = 0;
  ClientFcb->WriteQuotaAvailable = Pipe->OutboundQuota;
  ClientFcb->MaxDataLength = Pipe->OutboundQuota;
  KeInitializeSpinLock(&ClientFcb->DataListLock);
  KeInitializeEvent(&ClientFcb->ConnectEvent, SynchronizationEvent, FALSE);
  KeInitializeEvent(&ClientFcb->Event, SynchronizationEvent, FALSE);

  /*
   * Step 4. Add the client FCB to a list and connect it if possible.
   */

  /* Add the client FCB to the pipe FCB list. */
  InsertTailList(&Pipe->ClientFcbListHead, &ClientFcb->FcbListEntry);

  /* Connect to listening server side */
  if (ServerFcb)
    {
      ClientFcb->OtherSide = ServerFcb;
      ServerFcb->OtherSide = ClientFcb;
      ClientFcb->PipeState = FILE_PIPE_CONNECTED_STATE;
      ServerFcb->PipeState = FILE_PIPE_CONNECTED_STATE;

      /* Wake server thread */
      DPRINT("Setting the ConnectEvent for %x\n", ServerFcb);
      KeSetEvent(&ServerFcb->ConnectEvent, 0, FALSE);
    }

  KeUnlockMutex(&Pipe->FcbListLock);

  FileObject->FsContext = ClientFcb;

  Irp->IoStatus.Status = STATUS_SUCCESS;
  IoCompleteRequest(Irp, IO_NO_INCREMENT);

  DPRINT("Success!\n");

  return STATUS_SUCCESS;
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
   PNAMED_PIPE_CREATE_PARAMETERS Buffer;
   BOOLEAN NewPipe = FALSE;

   DPRINT("NpfsCreateNamedPipe(DeviceObject %p Irp %p)\n", DeviceObject, Irp);

   DeviceExt = (PNPFS_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
   IoStack = IoGetCurrentIrpStackLocation(Irp);
   FileObject = IoStack->FileObject;
   DPRINT("FileObject %p\n", FileObject);
   DPRINT("Pipe name %wZ\n", &FileObject->FileName);

   Buffer = IoStack->Parameters.CreatePipe.Parameters;

   Irp->IoStatus.Information = 0;

   Fcb = ExAllocatePool(NonPagedPool, sizeof(NPFS_FCB));
   if (Fcb == NULL)
     {
	Irp->IoStatus.Status = STATUS_NO_MEMORY;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return STATUS_NO_MEMORY;
     }

   Fcb->Thread = (struct ETHREAD *)Irp->Tail.Overlay.Thread;
   KeLockMutex(&DeviceExt->PipeListLock);

   /*
    * First search for existing Pipe with the same name.
    */
   Pipe = NpfsFindPipe(DeviceExt,
		       &FileObject->FileName);
   if (Pipe != NULL)
     {
       /*
        * Found Pipe with the same name. Check if we are 
        * allowed to use it.
        */
       KeUnlockMutex(&DeviceExt->PipeListLock);

       if (Pipe->CurrentInstances >= Pipe->MaximumInstances)
         {
           DPRINT("Out of instances.\n");
           ExFreePool(Fcb);
           Irp->IoStatus.Status = STATUS_PIPE_BUSY;
           IoCompleteRequest(Irp, IO_NO_INCREMENT);
           return STATUS_PIPE_BUSY;
         }

       /* FIXME: Check pipe modes also! */
       if (Pipe->MaximumInstances != Buffer->MaximumInstances ||
           Pipe->TimeOut.QuadPart != Buffer->DefaultTimeout.QuadPart)
         {
           DPRINT("Asked for invalid pipe mode.\n");
           ExFreePool(Fcb);
           Irp->IoStatus.Status = STATUS_ACCESS_DENIED;
           IoCompleteRequest(Irp, IO_NO_INCREMENT);
           return STATUS_ACCESS_DENIED;
         }
     }
   else
     {
       NewPipe = TRUE;
       Pipe = ExAllocatePool(NonPagedPool, sizeof(NPFS_PIPE));
       if (Pipe == NULL)
         {
           KeUnlockMutex(&DeviceExt->PipeListLock);
           Irp->IoStatus.Status = STATUS_NO_MEMORY;
           Irp->IoStatus.Information = 0;
           IoCompleteRequest(Irp, IO_NO_INCREMENT);
           return STATUS_NO_MEMORY;
         }

       if (RtlCreateUnicodeString(&Pipe->PipeName, FileObject->FileName.Buffer) == FALSE)
         {
           KeUnlockMutex(&DeviceExt->PipeListLock);
           ExFreePool(Pipe);
           ExFreePool(Fcb);
           Irp->IoStatus.Status = STATUS_NO_MEMORY;
           Irp->IoStatus.Information = 0;
           IoCompleteRequest(Irp, IO_NO_INCREMENT);
           return STATUS_NO_MEMORY;
         }

       InitializeListHead(&Pipe->ServerFcbListHead);
       InitializeListHead(&Pipe->ClientFcbListHead);
       KeInitializeMutex(&Pipe->FcbListLock, 0);

       Pipe->PipeType = Buffer->NamedPipeType;
       Pipe->WriteMode = Buffer->ReadMode;
       Pipe->ReadMode = Buffer->ReadMode;
       Pipe->CompletionMode = Buffer->CompletionMode;
       Pipe->PipeConfiguration = IoStack->Parameters.CreatePipe.Options & 0x3;
       Pipe->MaximumInstances = Buffer->MaximumInstances;
       Pipe->CurrentInstances = 0;
       Pipe->TimeOut = Buffer->DefaultTimeout;
       if (!(IoStack->Parameters.Create.Options & FILE_PIPE_OUTBOUND) || 
           IoStack->Parameters.Create.Options & FILE_PIPE_FULL_DUPLEX)
         {
           if (Buffer->InboundQuota == 0)
             {
               Pipe->InboundQuota = DeviceExt->DefaultQuota;
             }
           else
             {
               Pipe->InboundQuota = PAGE_ROUND_UP(Buffer->InboundQuota);
               if (Pipe->InboundQuota < DeviceExt->MinQuota)
                 {
                   Pipe->InboundQuota = DeviceExt->MinQuota;
                 }
               else if (Pipe->InboundQuota > DeviceExt->MaxQuota)
                 {
                   Pipe->InboundQuota = DeviceExt->MaxQuota;
                 }
             }
         }
       else
         {
           Pipe->InboundQuota = 0;
         }

       if (IoStack->Parameters.Create.Options & (FILE_PIPE_FULL_DUPLEX|FILE_PIPE_OUTBOUND))
         {
           if (Buffer->OutboundQuota == 0)
             {
               Pipe->OutboundQuota = DeviceExt->DefaultQuota;
             }
           else
             {
               Pipe->OutboundQuota = PAGE_ROUND_UP(Buffer->OutboundQuota);
               if (Pipe->OutboundQuota < DeviceExt->MinQuota)
                 {
                   Pipe->OutboundQuota = DeviceExt->MinQuota;
                 }
               else if (Pipe->OutboundQuota > DeviceExt->MaxQuota)
                 {
                   Pipe->OutboundQuota = DeviceExt->MaxQuota;
                 }
             }
         }
       else
         {
           Pipe->OutboundQuota = 0;
         }

       InsertTailList(&DeviceExt->PipeListHead, &Pipe->PipeListEntry);
       KeUnlockMutex(&DeviceExt->PipeListLock);
     }

   if (Pipe->InboundQuota)
     {
       Fcb->Data = ExAllocatePool(NonPagedPool, Pipe->InboundQuota);
       if (Fcb->Data == NULL)
         {
           ExFreePool(Fcb);

           if (NewPipe)
             {
               RtlFreeUnicodeString(&Pipe->PipeName);
               ExFreePool(Pipe);
             }

           Irp->IoStatus.Status = STATUS_NO_MEMORY;
           IoCompleteRequest(Irp, IO_NO_INCREMENT);
           return STATUS_NO_MEMORY;
         }
     }
   else
     {
       Fcb->Data = NULL;
     }

   Fcb->ReadPtr = Fcb->Data;
   Fcb->WritePtr = Fcb->Data;
   Fcb->ReadDataAvailable = 0;
   Fcb->WriteQuotaAvailable = Pipe->InboundQuota;
   Fcb->MaxDataLength = Pipe->InboundQuota;
   KeInitializeSpinLock(&Fcb->DataListLock);

   Pipe->CurrentInstances++;

   KeLockMutex(&Pipe->FcbListLock);
   InsertTailList(&Pipe->ServerFcbListHead, &Fcb->FcbListEntry);
   KeUnlockMutex(&Pipe->FcbListLock);

   Fcb->Pipe = Pipe;
   Fcb->PipeEnd = FILE_PIPE_SERVER_END;
   Fcb->PipeState = FILE_PIPE_LISTENING_STATE;
   Fcb->OtherSide = NULL;

   KeInitializeEvent(&Fcb->ConnectEvent,
		     SynchronizationEvent,
		     FALSE);

   KeInitializeEvent(&Fcb->Event,
		     SynchronizationEvent,
		     FALSE);

   FileObject->FsContext = Fcb;

   Irp->IoStatus.Status = STATUS_SUCCESS;
   IoCompleteRequest(Irp, IO_NO_INCREMENT);

   DPRINT("Success!\n");

   return STATUS_SUCCESS;
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
   BOOL Server;

   DPRINT("NpfsClose(DeviceObject %p Irp %p)\n", DeviceObject, Irp);

   IoStack = IoGetCurrentIrpStackLocation(Irp);
   DeviceExt = (PNPFS_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
   FileObject = IoStack->FileObject;
   Fcb = FileObject->FsContext;

   if (Fcb == NULL)
   {
      DPRINT("Success!\n");
      Irp->IoStatus.Status = STATUS_SUCCESS;
      Irp->IoStatus.Information = 0;
      IoCompleteRequest(Irp, IO_NO_INCREMENT);
      return STATUS_SUCCESS;
   }

   DPRINT("Fcb %x\n", Fcb);
   Pipe = Fcb->Pipe;

   DPRINT("Closing pipe %wZ\n", &Pipe->PipeName);

   KeLockMutex(&Pipe->FcbListLock);

   Server = (Fcb->PipeEnd == FILE_PIPE_SERVER_END);

   if (Server)
   {
      /* FIXME: Clean up existing connections here ?? */
      DPRINT("Server\n");
      Pipe->CurrentInstances--;
   }
   else
   {
      DPRINT("Client\n");
   }

   if (Fcb->PipeState == FILE_PIPE_CONNECTED_STATE)
   {
      if (Fcb->OtherSide)
      {
         Fcb->OtherSide->PipeState = FILE_PIPE_CLOSING_STATE;
         Fcb->OtherSide->OtherSide = NULL;
         /*
          * Signaling the write event. If is possible that an other
          * thread waits for an empty buffer.
          */
         KeSetEvent(&Fcb->OtherSide->Event, IO_NO_INCREMENT, FALSE);
      }

      Fcb->PipeState = 0;
   }

   FileObject->FsContext = NULL;

   RemoveEntryList(&Fcb->FcbListEntry);
   if (Fcb->Data)
      ExFreePool(Fcb->Data);
   ExFreePool(Fcb);

   KeUnlockMutex(&Pipe->FcbListLock);

   if (IsListEmpty(&Pipe->ServerFcbListHead) &&
       IsListEmpty(&Pipe->ClientFcbListHead))
   {
      RtlFreeUnicodeString(&Pipe->PipeName);
      KeLockMutex(&DeviceExt->PipeListLock);
      RemoveEntryList(&Pipe->PipeListEntry);
      KeUnlockMutex(&DeviceExt->PipeListLock);
      ExFreePool(Pipe);
   }

   Irp->IoStatus.Status = STATUS_SUCCESS;
   Irp->IoStatus.Information = 0;
   IoCompleteRequest(Irp, IO_NO_INCREMENT);

   DPRINT("Success!\n");

   return STATUS_SUCCESS;
}

/* EOF */
