/* $Id: fsctrl.c,v 1.6 2001/10/20 15:30:07 ekohl Exp $
 *
 * COPYRIGHT:  See COPYING in the top level directory
 * PROJECT:    ReactOS kernel
 * FILE:       services/fs/np/fsctrl.c
 * PURPOSE:    Named pipe filesystem
 * PROGRAMMER: David Welch <welch@cwcom.net>
 *             Eric Kohl <ekohl@rz-online.de>
 */

/* INCLUDES ******************************************************************/

#include <ddk/ntddk.h>
#include "npfs.h"

//#define NDEBUG
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
     return STATUS_PIPE_CONNECTED;

   if (Fcb->PipeState == FILE_PIPE_CLOSING_STATE)
     return STATUS_PIPE_CLOSING;

   /*
    * Acceptable states are: FILE_PIPE_DISCONNECTED_STATE and
    *                        FILE_PIPE_LISTENING_STATE
    */

   DPRINT("Waiting for connection...\n");

   Pipe = Fcb->Pipe;

   Fcb->PipeState = FILE_PIPE_LISTENING_STATE;

   /* search for a listening client fcb */

   current_entry = Pipe->ClientFcbListHead.Flink;
   while (current_entry != &Pipe->ClientFcbListHead)
     {
	ClientFcb = CONTAINING_RECORD(current_entry,
				      NPFS_FCB,
				      FcbListEntry);
	
	if ((ClientFcb->PipeState == FILE_PIPE_LISTENING_STATE)
	    || (ClientFcb->PipeState == FILE_PIPE_DISCONNECTED_STATE))
	  {
	     break;
	  }
	
	current_entry = current_entry->Flink;
     }
   
   if ((current_entry != &Pipe->ClientFcbListHead)
       && (ClientFcb->PipeState == FILE_PIPE_LISTENING_STATE))
     {
	/* found a listening client fcb */
	DPRINT("Listening client fcb found -- connecting\n");

	/* connect client and server fcb's */
	Fcb->OtherSide = ClientFcb;
	ClientFcb->OtherSide = Fcb;

	/* set connected state */
	Fcb->PipeState = FILE_PIPE_CONNECTED_STATE;
	ClientFcb->PipeState = FILE_PIPE_CONNECTED_STATE;

	/* FIXME: create and initialize data queues */

	/* signal client's connect event */
	KeSetEvent(&ClientFcb->ConnectEvent, IO_NO_INCREMENT, FALSE);

     }
   else if ((current_entry != &Pipe->ClientFcbListHead)
	    && (ClientFcb->PipeState == FILE_PIPE_DISCONNECTED_STATE))
     {
	/* found a disconnected client fcb */
	DPRINT("Disconnected client fcb found - notifying client\n");

	/* signal client's connect event */
	KeSetEvent(&ClientFcb->ConnectEvent, IO_NO_INCREMENT, FALSE);
     }
   else
     {
	/* no listening client fcb found */
	DPRINT("No listening client fcb found -- waiting for client\n");
	Status = KeWaitForSingleObject(&Fcb->ConnectEvent,
				       UserRequest,
				       KernelMode,
				       FALSE,
				       NULL);

	DPRINT("Finished waiting! Status: %x\n", Status);
     }


   DPRINT("Client Fcb: %p\n", Fcb->OtherSide);

   return STATUS_PIPE_CONNECTED;
}


static NTSTATUS
NpfsDisconnectPipe(PNPFS_FCB Fcb)
{
  PNPFS_FCB ServerFcb;

  DPRINT("NpfsDisconnectPipe()\n");

  if (Fcb->PipeState == FILE_PIPE_DISCONNECTED_STATE)
    return(STATUS_SUCCESS);

  if (Fcb->PipeState == FILE_PIPE_CONNECTED_STATE)
    {
      Fcb->PipeState = FILE_PIPE_DISCONNECTED_STATE;
      Fcb->OtherSide->PipeState = FILE_PIPE_DISCONNECTED_STATE;

      /* FIXME: remove data queue(s) */

      Fcb->OtherSide->OtherSide = NULL;
      Fcb->OtherSide = NULL;

      DPRINT("Pipe disconnected\n");
      return(STATUS_SUCCESS);
    }

  if (Fcb->PipeState == FILE_PIPE_CLOSING_STATE)
    {
      Fcb->PipeState = FILE_PIPE_DISCONNECTED_STATE;

      /* FIXME: remove data queue(s) */

      DPRINT("Pipe disconnected\n");
      return(STATUS_SUCCESS);
    }

  return(STATUS_UNSUCCESSFUL);
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

  /* search for listening server */
  current_entry = Pipe->ServerFcbListHead.Flink;
  while (current_entry != &Pipe->ServerFcbListHead)
    {
      ServerFcb = CONTAINING_RECORD(current_entry,
				    NPFS_FCB,
				    FcbListEntry);

      if (ServerFcb->PipeState == FILE_PIPE_LISTENING_STATE)
	break;

      current_entry = current_entry->Flink;
    }

  if (current_entry != &Pipe->ServerFcbListHead)
    {
      /* found a listening server fcb */
      DPRINT("Listening server fcb found -- connecting\n");

      Status = STATUS_SUCCESS;
    }
  else
    {
      /* no listening server fcb found -- wait for one */
      Fcb->PipeState = FILE_PIPE_LISTENING_STATE;

      Status = KeWaitForSingleObject(&Fcb->ConnectEvent,
				     UserRequest,
				     KernelMode,
				     FALSE,
				     &WaitPipe->Timeout);
    }

  return(Status);
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
   
   switch (IoStack->Parameters.FileSystemControl.IoControlCode)
     {
      case FSCTL_PIPE_LISTEN:
	DPRINT("Connecting pipe %wZ\n", &Pipe->PipeName);
	Status = NpfsConnectPipe(Fcb);
	break;

      case FSCTL_PIPE_DISCONNECT:
	DPRINT("Disconnecting pipe %wZ\n", &Pipe->PipeName);
	Status = NpfsDisconnectPipe(Fcb);
	break;

      case FSCTL_PIPE_WAIT:
	DPRINT("Waiting for pipe %wZ\n", &Pipe->PipeName);
	Status = NpfsWaitPipe(Irp, Fcb);
	break;

      default:
        DPRINT("IoControlCode: %x\n", IoStack->Parameters.FileSystemControl.IoControlCode)
        Status = STATUS_UNSUCCESSFUL;
     }
   
   Irp->IoStatus.Status = Status;
   Irp->IoStatus.Information = 0;
   
   IoCompleteRequest(Irp, IO_NO_INCREMENT);
   
   return(Status);
}

/* EOF */
