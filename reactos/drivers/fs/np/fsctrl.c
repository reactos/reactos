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

#include <ddk/ntddk.h>
#include "npfs.h"

#define NDEBUG
#include <debug.h>

/* FUNCTIONS *****************************************************************/

static NTSTATUS
NpfsConnectPipe(PNPFS_FCB Fcb)
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

  KeUnlockMutex(&Pipe->FcbListLock);

  /* no listening client fcb found */
  DPRINT("No listening client fcb found -- waiting for client\n");

  Fcb->PipeState = FILE_PIPE_LISTENING_STATE;

  Status = KeWaitForSingleObject(&Fcb->ConnectEvent,
				 UserRequest,
				 KernelMode,
				 FALSE,
				 NULL);
  if (!NT_SUCCESS(Status))
    {
      DPRINT("KeWaitForSingleObject() failed (Status %lx)\n", Status);
      return Status;
    }

  Fcb->PipeState = FILE_PIPE_CONNECTED_STATE;

  DPRINT("Client Fcb: %p\n", Fcb->OtherSide);

  return STATUS_PIPE_CONNECTED;
}


static NTSTATUS
NpfsDisconnectPipe(PNPFS_FCB Fcb)
{
  DPRINT("NpfsDisconnectPipe()\n");

  if (Fcb->PipeState == FILE_PIPE_DISCONNECTED_STATE)
    return STATUS_SUCCESS;

  if (Fcb->PipeState == FILE_PIPE_CONNECTED_STATE)
    {
      Fcb->PipeState = FILE_PIPE_DISCONNECTED_STATE;
      /* FIXME: Shouldn't this be FILE_PIPE_CLOSING_STATE? */
      Fcb->OtherSide->PipeState = FILE_PIPE_DISCONNECTED_STATE;

      /* FIXME: remove data queue(s) */

      Fcb->OtherSide->OtherSide = NULL;
      Fcb->OtherSide = NULL;

      DPRINT("Pipe disconnected\n");
      return STATUS_SUCCESS;
    }

  if (Fcb->PipeState == FILE_PIPE_CLOSING_STATE)
    {
      Fcb->PipeState = FILE_PIPE_DISCONNECTED_STATE;
      Fcb->OtherSide = NULL;

      /* FIXME: remove data queue(s) */

      DPRINT("Pipe disconnected\n");
      return STATUS_SUCCESS;
    }

  return STATUS_UNSUCCESSFUL;
}


static NTSTATUS
NpfsWaitPipe(PIRP Irp,
	     PNPFS_FCB Fcb)
{
  PNPFS_PIPE Pipe;
  PLIST_ENTRY current_entry;
  PNPFS_FCB ServerFcb;
  PNPFS_WAIT_PIPE WaitPipe;
  NTSTATUS Status;

  DPRINT("NpfsWaitPipe\n");

  WaitPipe = (PNPFS_WAIT_PIPE)Irp->AssociatedIrp.SystemBuffer;
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
static NTSTATUS
NpfsGetState(PIRP Irp,
	     PIO_STACK_LOCATION IrpSp)
{
  PNPFS_GET_STATE Reply;
  PNPFS_PIPE Pipe;
  PNPFS_FCB Fcb;

  /* Validate parameters */
  if (IrpSp->Parameters.DeviceIoControl.OutputBufferLength < sizeof(NPFS_GET_STATE))
    {
      DPRINT("Status (0x%X).\n", STATUS_INVALID_PARAMETER);
      return STATUS_INVALID_PARAMETER;
    }

  Fcb = IrpSp->FileObject->FsContext;
  Reply = (PNPFS_GET_STATE)Irp->AssociatedIrp.SystemBuffer;
  Pipe = Fcb->Pipe;

  Reply->WriteModeMessage = (Pipe->WriteMode == FILE_PIPE_MESSAGE_MODE);
  Reply->ReadModeMessage = (Pipe->ReadMode == FILE_PIPE_MESSAGE_MODE);
  Reply->NonBlocking = (Pipe->CompletionMode == FILE_PIPE_QUEUE_OPERATION);
  Reply->InBufferSize = Pipe->InboundQuota;
  Reply->OutBufferSize = Pipe->OutboundQuota;
  Reply->Timeout = Pipe->TimeOut;

  DPRINT("Status (0x%X).\n", STATUS_SUCCESS);

  return STATUS_SUCCESS;
}


/*
 * FUNCTION: Set state of a pipe
 * ARGUMENTS:
 *     Irp   = Pointer to I/O request packet
 *     IrpSp = Pointer to current stack location of Irp
 * RETURNS:
 *     Status of operation
 */
static NTSTATUS
NpfsSetState(PIRP Irp,
	     PIO_STACK_LOCATION IrpSp)
{
  PNPFS_SET_STATE Request;
  PNPFS_PIPE Pipe;
  PNPFS_FCB Fcb;

  /* Validate parameters */
  if (IrpSp->Parameters.DeviceIoControl.InputBufferLength < sizeof(NPFS_SET_STATE))
    {
      DPRINT("Status (0x%X).\n", STATUS_INVALID_PARAMETER);
      return STATUS_INVALID_PARAMETER;
    }

  Fcb = IrpSp->FileObject->FsContext;
  Request = (PNPFS_SET_STATE)Irp->AssociatedIrp.SystemBuffer;
  Pipe = Fcb->Pipe;

  Pipe->WriteMode =
    Request->WriteModeMessage ? FILE_PIPE_MESSAGE_MODE : FILE_PIPE_BYTE_STREAM_MODE;
  Pipe->ReadMode =
    Request->WriteModeMessage ? FILE_PIPE_MESSAGE_MODE : FILE_PIPE_BYTE_STREAM_MODE;
  Pipe->CompletionMode =
    Request->NonBlocking ? FILE_PIPE_QUEUE_OPERATION : FILE_PIPE_COMPLETE_OPERATION;
  Pipe->InboundQuota = Request->InBufferSize;
  Pipe->OutboundQuota = Request->OutBufferSize;
  Pipe->TimeOut = Request->Timeout;

  DPRINT("Status (0x%X).\n", STATUS_SUCCESS);

  return STATUS_SUCCESS;
}


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
	Status = NpfsConnectPipe(Fcb);
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

      case FSCTL_PIPE_GET_STATE:
	DPRINT("Get state\n");
	Status = NpfsGetState(Irp, (PIO_STACK_LOCATION)IoStack);
	break;

      case FSCTL_PIPE_SET_STATE:
	DPRINT("Set state\n");
	Status = NpfsSetState(Irp, (PIO_STACK_LOCATION)IoStack);
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
	DPRINT("IoControlCode: %x\n", IoStack->Parameters.FileSystemControl.FsControlCode)
	Status = STATUS_UNSUCCESSFUL;
    }

  Irp->IoStatus.Status = Status;
  Irp->IoStatus.Information = 0;

  IoCompleteRequest(Irp, IO_NO_INCREMENT);

  return(Status);
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
