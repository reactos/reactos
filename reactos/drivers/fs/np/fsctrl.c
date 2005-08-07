/* $Id$
 *
 * COPYRIGHT:  See COPYING in the top level directory
 * PROJECT:    ReactOS kernel
 * FILE:       drivers/fs/np/fsctrl.c
 * PURPOSE:    Named pipe filesystem
 * PROGRAMMER: David Welch <welch@cwcom.net>
 *             Eric Kohl
 */

/* INCLUDES ******************************************************************/

#include <ntifs.h>
#include "npfs.h"

#define NDEBUG
#include <debug.h>

/* FUNCTIONS *****************************************************************/

static VOID STDCALL
NpfsListeningCancelRoutine(IN PDEVICE_OBJECT DeviceObject,
                           IN PIRP Irp)
{
  PNPFS_WAITER_ENTRY Waiter;

  DPRINT1("NpfsListeningCancelRoutine() called\n");

  Waiter = (PNPFS_WAITER_ENTRY)&Irp->Tail.Overlay.DriverContext;

  IoReleaseCancelSpinLock(Irp->CancelIrql);


  KeLockMutex(&Waiter->Fcb->Pipe->FcbListLock);
  RemoveEntryList(&Waiter->Entry);
  KeUnlockMutex(&Waiter->Fcb->Pipe->FcbListLock);

  Irp->IoStatus.Status = STATUS_CANCELLED;
  Irp->IoStatus.Information = 0;
  IoCompleteRequest(Irp, IO_NO_INCREMENT);
}


static NTSTATUS
NpfsAddListeningServerInstance(PIRP Irp,
			       PNPFS_FCB Fcb)
{
  PNPFS_WAITER_ENTRY Entry;
  KIRQL oldIrql;

  Entry = (PNPFS_WAITER_ENTRY)&Irp->Tail.Overlay.DriverContext;

  Entry->Fcb = Fcb;

  KeLockMutex(&Fcb->Pipe->FcbListLock);

  IoMarkIrpPending(Irp);
  InsertTailList(&Fcb->Pipe->WaiterListHead, &Entry->Entry);

  IoAcquireCancelSpinLock(&oldIrql);
  if (!Irp->Cancel)
    {
      IoSetCancelRoutine(Irp, NpfsListeningCancelRoutine);
      IoReleaseCancelSpinLock(oldIrql);
      KeUnlockMutex(&Fcb->Pipe->FcbListLock);
      return STATUS_PENDING;
    }
  IoReleaseCancelSpinLock(oldIrql);

  RemoveEntryList(&Entry->Entry);

  Irp->IoStatus.Status = STATUS_CANCELLED;
  Irp->IoStatus.Information = 0;
  IoCompleteRequest(Irp, IO_NO_INCREMENT);
  KeUnlockMutex(&Fcb->Pipe->FcbListLock);

  return STATUS_CANCELLED;
}


static NTSTATUS
NpfsConnectPipe(PIRP Irp,
                PNPFS_FCB Fcb)
{
  PNPFS_PIPE Pipe;
  PLIST_ENTRY current_entry;
  PNPFS_FCB ClientFcb;
  NTSTATUS Status;

  DPRINT("NpfsConnectPipe()\n");

  if (Fcb->PipeState == FILE_PIPE_CONNECTED_STATE)
    {
      KeResetEvent(&Fcb->ConnectEvent);
      return STATUS_PIPE_CONNECTED;
    }

  if (Fcb->PipeState == FILE_PIPE_CLOSING_STATE)
    return STATUS_PIPE_CLOSING;

  DPRINT("Waiting for connection...\n");

  Pipe = Fcb->Pipe;

  /* search for a listening client fcb */
  KeLockMutex(&Pipe->FcbListLock);

  current_entry = Pipe->ClientFcbListHead.Flink;
  while (current_entry != &Pipe->ClientFcbListHead)
    {
      ClientFcb = CONTAINING_RECORD(current_entry,
				    NPFS_FCB,
				    FcbListEntry);

      if (ClientFcb->PipeState == 0)
	{
	  /* found a passive (waiting) client fcb */
	  DPRINT("Passive (waiting) client fcb found -- wake the client\n");
	  KeSetEvent(&ClientFcb->ConnectEvent, IO_NO_INCREMENT, FALSE);
	  break;
	}

#if 0
      if (ClientFcb->PipeState == FILE_PIPE_LISTENING_STATE)
	{
	  /* found a listening client fcb */
	  DPRINT("Listening client fcb found -- connecting\n");

	  /* connect client and server fcb's */
	  Fcb->OtherSide = ClientFcb;
	  ClientFcb->OtherSide = Fcb;

	  /* set connected state */
	  Fcb->PipeState = FILE_PIPE_CONNECTED_STATE;
	  ClientFcb->PipeState = FILE_PIPE_CONNECTED_STATE;

	  KeUnlockMutex(&Pipe->FcbListLock);

	  /* FIXME: create and initialize data queues */

	  /* signal client's connect event */
	  DPRINT("Setting the ConnectEvent for %x\n", ClientFcb);
	  KeSetEvent(&ClientFcb->ConnectEvent, IO_NO_INCREMENT, FALSE);

	  return STATUS_PIPE_CONNECTED;
	}
#endif

      current_entry = current_entry->Flink;
    }

  /* no listening client fcb found */
  DPRINT("No listening client fcb found -- waiting for client\n");

  Fcb->PipeState = FILE_PIPE_LISTENING_STATE;

  Status = NpfsAddListeningServerInstance(Irp, Fcb);

  KeUnlockMutex(&Pipe->FcbListLock);

  DPRINT("NpfsConnectPipe() done (Status %lx)\n", Status);

  return Status;
}


static NTSTATUS
NpfsDisconnectPipe(PNPFS_FCB Fcb)
{
   NTSTATUS Status;
   PNPFS_FCB OtherSide;
   PNPFS_PIPE Pipe;
   BOOL Server;

   DPRINT("NpfsDisconnectPipe()\n");

   Pipe = Fcb->Pipe;
   KeLockMutex(&Pipe->FcbListLock);

   if (Fcb->PipeState == FILE_PIPE_DISCONNECTED_STATE)
   {
      DPRINT("Pipe is already disconnected\n");
      Status = STATUS_SUCCESS;
   }
   else if (Fcb->PipeState == FILE_PIPE_CONNECTED_STATE)
   {
      Server = (Fcb->PipeEnd == FILE_PIPE_SERVER_END);
      OtherSide = Fcb->OtherSide;
      Fcb->OtherSide = NULL;
      Fcb->PipeState = FILE_PIPE_DISCONNECTED_STATE;
      /* Lock the server first */
      if (Server)
      {
         ExAcquireFastMutex(&Fcb->DataListLock);
	 ExAcquireFastMutex(&OtherSide->DataListLock);
      }
      else
      {
	 ExAcquireFastMutex(&OtherSide->DataListLock);
         ExAcquireFastMutex(&Fcb->DataListLock);
      }
      OtherSide->PipeState = FILE_PIPE_DISCONNECTED_STATE;
      OtherSide->OtherSide = NULL;
      /*
       * Signaling the write event. If is possible that an other
       * thread waits for an empty buffer.
       */
      KeSetEvent(&OtherSide->ReadEvent, IO_NO_INCREMENT, FALSE);
      KeSetEvent(&OtherSide->WriteEvent, IO_NO_INCREMENT, FALSE);
      if (Server)
      {
         ExReleaseFastMutex(&Fcb->DataListLock);
	 ExReleaseFastMutex(&OtherSide->DataListLock);
      }
      else
      {
	 ExReleaseFastMutex(&OtherSide->DataListLock);
	 ExReleaseFastMutex(&OtherSide->DataListLock);
      }
      Status = STATUS_SUCCESS;
   }
   else if (Fcb->PipeState == FILE_PIPE_LISTENING_STATE)
   {
      PLIST_ENTRY Entry;
      PNPFS_WAITER_ENTRY WaitEntry = NULL;
      BOOLEAN Complete = FALSE;
      PIRP Irp = NULL;

      Entry = Fcb->Pipe->WaiterListHead.Flink;
      while (Entry != &Fcb->Pipe->WaiterListHead)
      {
         WaitEntry = CONTAINING_RECORD(Entry, NPFS_WAITER_ENTRY, Entry);
	 if (WaitEntry->Fcb == Fcb)
	 {
            RemoveEntryList(Entry);
	    Irp = CONTAINING_RECORD(Entry, IRP, Tail.Overlay.DriverContext);
	    Complete = (NULL == IoSetCancelRoutine(Irp, NULL));
            break;
	 }
	 Entry = Entry->Flink;
      }

      if (Irp)
      {
         if (Complete)
	 {
	    Irp->IoStatus.Status = STATUS_PIPE_BROKEN;
            Irp->IoStatus.Information = 0;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
	 }
      }
      Fcb->PipeState = FILE_PIPE_DISCONNECTED_STATE;
      Status = STATUS_SUCCESS;
   }
   else if (Fcb->PipeState == FILE_PIPE_CLOSING_STATE)
   {
      Status = STATUS_PIPE_CLOSING;
   }
   else
   {
      Status = STATUS_UNSUCCESSFUL;
   }
   KeUnlockMutex(&Pipe->FcbListLock);
   return Status;
}


static NTSTATUS
NpfsWaitPipe(PIRP Irp,
	     PNPFS_FCB Fcb)
{
  PNPFS_PIPE Pipe;
  PLIST_ENTRY current_entry;
  PNPFS_FCB ServerFcb;
  PFILE_PIPE_WAIT_FOR_BUFFER WaitPipe;
  NTSTATUS Status;

  DPRINT("NpfsWaitPipe\n");

  WaitPipe = (PFILE_PIPE_WAIT_FOR_BUFFER)Irp->AssociatedIrp.SystemBuffer;
  Pipe = Fcb->Pipe;

  if (Fcb->PipeState != 0)
    {
      DPRINT("Pipe is not in passive (waiting) state!\n");
      return STATUS_UNSUCCESSFUL;
    }

  /* search for listening server */
  current_entry = Pipe->ServerFcbListHead.Flink;
  while (current_entry != &Pipe->ServerFcbListHead)
    {
      ServerFcb = CONTAINING_RECORD(current_entry,
				    NPFS_FCB,
				    FcbListEntry);

      if (ServerFcb->PipeState == FILE_PIPE_LISTENING_STATE)
	{
	  /* found a listening server fcb */
	  DPRINT("Listening server fcb found -- connecting\n");

	  return STATUS_SUCCESS;
	}

      current_entry = current_entry->Flink;
    }

  /* no listening server fcb found -- wait for one */
  Status = KeWaitForSingleObject(&Fcb->ConnectEvent,
				 UserRequest,
				 KernelMode,
				 FALSE,
				 &WaitPipe->Timeout);

  DPRINT("KeWaitForSingleObject() returned (Status %lx)\n", Status);

  return Status;
}


/*
 * FUNCTION: Return current state of a pipe
 * ARGUMENTS:
 *     Irp   = Pointer to I/O request packet
 *     IrpSp = Pointer to current stack location of Irp
 * RETURNS:
 *     Status of operation
 */

/*
 * FUNCTION: Peek at a pipe (get information about messages)
 * ARGUMENTS:
 *     Irp = Pointer to I/O request packet
 *     IoStack = Pointer to current stack location of Irp
 * RETURNS:
 *     Status of operation
 */
static NTSTATUS
NpfsPeekPipe(PIRP Irp,
	     PIO_STACK_LOCATION IoStack)
{
  ULONG OutputBufferLength;
  PNPFS_PIPE Pipe;
  PFILE_PIPE_PEEK_BUFFER Reply;
  PNPFS_FCB Fcb;
  NTSTATUS Status;

  DPRINT("NpfsPeekPipe\n");

  OutputBufferLength = IoStack->Parameters.DeviceIoControl.OutputBufferLength;

  /* Validate parameters */
  if (OutputBufferLength < sizeof(FILE_PIPE_PEEK_BUFFER))
    {
      DPRINT("Buffer too small\n");
      return STATUS_INVALID_PARAMETER;
    }

  Fcb = IoStack->FileObject->FsContext;
  Reply = (PFILE_PIPE_PEEK_BUFFER)Irp->AssociatedIrp.SystemBuffer;
  Pipe = Fcb->Pipe;

  Status = STATUS_NOT_IMPLEMENTED;

  return Status;
}


NTSTATUS STDCALL
NpfsFileSystemControl(PDEVICE_OBJECT DeviceObject,
		      PIRP Irp)
{
  PIO_STACK_LOCATION IoStack;
  PFILE_OBJECT FileObject;
  NTSTATUS Status;
  PNPFS_DEVICE_EXTENSION DeviceExt;
  PNPFS_PIPE Pipe;
  PNPFS_FCB Fcb;

  DPRINT("NpfsFileSystemContol(DeviceObject %p Irp %p)\n", DeviceObject, Irp);

  DeviceExt = (PNPFS_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
  IoStack = IoGetCurrentIrpStackLocation(Irp);
  DPRINT("IoStack: %p\n", IoStack);
  FileObject = IoStack->FileObject;
  DPRINT("FileObject: %p\n", FileObject);
  Fcb = FileObject->FsContext;
  DPRINT("Fcb: %p\n", Fcb);
  Pipe = Fcb->Pipe;
  DPRINT("Pipe: %p\n", Pipe);
  DPRINT("PipeName: %wZ\n", &Pipe->PipeName);

  Irp->IoStatus.Information = 0;

  switch (IoStack->Parameters.FileSystemControl.FsControlCode)
    {
      case FSCTL_PIPE_ASSIGN_EVENT:
	DPRINT("Assign event\n");
	Status = STATUS_NOT_IMPLEMENTED;
	break;

      case FSCTL_PIPE_DISCONNECT:
	DPRINT("Disconnecting pipe %wZ\n", &Pipe->PipeName);
	Status = NpfsDisconnectPipe(Fcb);
	break;

      case FSCTL_PIPE_LISTEN:
	DPRINT("Connecting pipe %wZ\n", &Pipe->PipeName);
	Status = NpfsConnectPipe(Irp, Fcb);
	break;

      case FSCTL_PIPE_PEEK:
	DPRINT("Peeking pipe %wZ\n", &Pipe->PipeName);
	Status = NpfsPeekPipe(Irp, (PIO_STACK_LOCATION)IoStack);
	break;

      case FSCTL_PIPE_QUERY_EVENT:
	DPRINT("Query event\n");
	Status = STATUS_NOT_IMPLEMENTED;
	break;

      case FSCTL_PIPE_TRANSCEIVE:
	DPRINT("Transceive\n");
	Status = STATUS_NOT_IMPLEMENTED;
	break;

      case FSCTL_PIPE_WAIT:
	DPRINT("Waiting for pipe %wZ\n", &Pipe->PipeName);
	Status = NpfsWaitPipe(Irp, Fcb);
	break;

      case FSCTL_PIPE_IMPERSONATE:
	DPRINT("Impersonate\n");
	Status = STATUS_NOT_IMPLEMENTED;
	break;

      case FSCTL_PIPE_SET_CLIENT_PROCESS:
	DPRINT("Set client process\n");
	Status = STATUS_NOT_IMPLEMENTED;
	break;

      case FSCTL_PIPE_QUERY_CLIENT_PROCESS:
	DPRINT("Query client process\n");
	Status = STATUS_NOT_IMPLEMENTED;
	break;

      case FSCTL_PIPE_INTERNAL_READ:
	DPRINT("Internal read\n");
	Status = STATUS_NOT_IMPLEMENTED;
	break;

      case FSCTL_PIPE_INTERNAL_WRITE:
	DPRINT("Internal write\n");
	Status = STATUS_NOT_IMPLEMENTED;
	break;

      case FSCTL_PIPE_INTERNAL_TRANSCEIVE:
	DPRINT("Internal transceive\n");
	Status = STATUS_NOT_IMPLEMENTED;
	break;

      case FSCTL_PIPE_INTERNAL_READ_OVFLOW:
	DPRINT("Internal read overflow\n");
	Status = STATUS_NOT_IMPLEMENTED;
	break;

      default:
	DPRINT("IoControlCode: %x\n", IoStack->Parameters.FileSystemControl.FsControlCode);
	Status = STATUS_UNSUCCESSFUL;
    }

  if (Status != STATUS_PENDING)
    {
      Irp->IoStatus.Status = Status;

      IoCompleteRequest(Irp, IO_NO_INCREMENT);
    }

  return Status;
}


NTSTATUS STDCALL
NpfsFlushBuffers(PDEVICE_OBJECT DeviceObject,
		 PIRP Irp)
{
  /* FIXME: Implement */

  Irp->IoStatus.Status = STATUS_SUCCESS;
  Irp->IoStatus.Information = 0;

  IoCompleteRequest(Irp, IO_NO_INCREMENT);

  return STATUS_SUCCESS;
}

/* EOF */
