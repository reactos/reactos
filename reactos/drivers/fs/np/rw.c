/* $Id: rw.c,v 1.6 2002/05/07 22:41:22 hbirr Exp $
 *
 * COPYRIGHT:  See COPYING in the top level directory
 * PROJECT:    ReactOS kernel
 * FILE:       services/fs/np/rw.c
 * PURPOSE:    Named pipe filesystem
 * PROGRAMMER: David Welch <welch@cwcom.net>
 */

/* INCLUDES ******************************************************************/

#include <ddk/ntddk.h>
#include "npfs.h"

#define NDEBUG
#include <debug.h>


/* FUNCTIONS *****************************************************************/

static inline PNPFS_PIPE_DATA
NpfsAllocatePipeData(PVOID Data,
		     ULONG Size)
{
  PNPFS_PIPE_DATA PipeData;

  PipeData = ExAllocateFromNPagedLookasideList(&NpfsPipeDataLookasideList);
  if (!PipeData)
    {
      return NULL;
    }

  PipeData->Data = Data;
  PipeData->Size = Size;
  PipeData->Offset = 0;

  return PipeData;
}


static inline PNPFS_PIPE_DATA
NpfsInitializePipeData(
  PVOID Data,
  ULONG Size)
{
  PNPFS_PIPE_DATA PipeData;
  PVOID Buffer;

  Buffer = ExAllocatePool(NonPagedPool, Size);
  if (!Buffer)
  {
    return NULL;
  }

  RtlMoveMemory(Buffer, Data, Size);

  PipeData = NpfsAllocatePipeData(Buffer, Size);
  if (!PipeData)
  {
    ExFreePool(Buffer);
  }

  return PipeData;
}


NTSTATUS STDCALL
NpfsRead(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
  PIO_STACK_LOCATION IoStack;
  PFILE_OBJECT FileObject;
  NTSTATUS Status;
  PNPFS_DEVICE_EXTENSION DeviceExt;
  PWSTR PipeName;
  KIRQL OldIrql;
  PLIST_ENTRY CurrentEntry;
  PNPFS_PIPE_DATA Current;
  ULONG Information;
  PNPFS_FCB Fcb;
  PNPFS_FCB ReadFcb;
  PNPFS_PIPE Pipe;
  ULONG Length;
  PVOID Buffer;
  ULONG CopyLength;

  DPRINT("NpfsRead(DeviceObject %p  Irp %p)\n", DeviceObject, Irp);
  
  DeviceExt = (PNPFS_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
  IoStack = IoGetCurrentIrpStackLocation(Irp);
  FileObject = IoStack->FileObject;
  Fcb = FileObject->FsContext;
  Pipe = Fcb->Pipe;
  ReadFcb = Fcb->OtherSide;

  if (ReadFcb == NULL)
    {
      DPRINT("Pipe is NOT connected!\n");
      Status = STATUS_UNSUCCESSFUL;
      Information = 0;
      goto done;
    }

  if (Irp->MdlAddress == NULL)
    {
      DPRINT("Irp->MdlAddress == NULL\n");
      Status = STATUS_UNSUCCESSFUL;
      Information = 0;
      goto done;
    }

  Status = STATUS_SUCCESS;
  Length = IoStack->Parameters.Read.Length;
  Information = 0;

  Buffer = MmGetSystemAddressForMdl(Irp->MdlAddress);
  DPRINT("Length %d Buffer %x\n",Length,Buffer);

  KeAcquireSpinLock(&ReadFcb->DataListLock, &OldIrql);
  while (1)
  {
     /* FIXME: check if in blocking mode */
     if (IsListEmpty(&ReadFcb->DataListHead))
     {
        KeResetEvent(&Fcb->ReadEvent);
        KeReleaseSpinLock(&ReadFcb->DataListLock, OldIrql);
	if (Information > 0)
	{
	   Status = STATUS_SUCCESS;
	   goto done;
	}
        if (Fcb->PipeState != FILE_PIPE_CONNECTED_STATE)
	{
	   Status = STATUS_PIPE_BROKEN;
	   goto done;
	}
        /* Wait for ReadEvent to become signaled */
        DPRINT("Waiting for readable data (%S)\n", Pipe->PipeName.Buffer);
        Status = KeWaitForSingleObject(&Fcb->ReadEvent,
				       UserRequest,
				       KernelMode,
				       FALSE,
				       NULL);
        DPRINT("Finished waiting (%S)! Status: %x\n", Pipe->PipeName.Buffer, Status);
        KeAcquireSpinLock(&ReadFcb->DataListLock, &OldIrql);
     }

     if (Pipe->PipeReadMode == FILE_PIPE_BYTE_STREAM_MODE)
     {
        DPRINT("Byte stream mode\n");

        /* Byte stream mode */
        CurrentEntry = NULL;
        while (Length > 0 && !IsListEmpty(&ReadFcb->DataListHead))
	{
	   CurrentEntry = RemoveHeadList(&ReadFcb->DataListHead);
	   Current = CONTAINING_RECORD(CurrentEntry, NPFS_PIPE_DATA, ListEntry);

	   DPRINT("Took pipe data at %p off the queue\n", Current);

	   CopyLength = RtlMin(Current->Size, Length);
	   RtlCopyMemory(Buffer,
			 ((PVOID)((PVOID)Current->Data + Current->Offset)),
			 CopyLength);
	   Buffer += CopyLength;
	   Length -= CopyLength;
	   Information += CopyLength;

	   /* Update the data buffer */
	   Current->Offset += CopyLength;
	   Current->Size -= CopyLength;
	   if (Current->Size == 0)
	   {
	       NpfsFreePipeData(Current);
	       CurrentEntry = NULL;
	   }
	}

        if (CurrentEntry && Current->Size > 0)
	{
	   DPRINT("Putting pipe data at %p back in queue\n", Current);

	   /* The caller's buffer could not contain the complete message,
	      so put it back on the queue */
	   InsertHeadList(&ReadFcb->DataListHead, &Current->ListEntry);
	}

	if (Length == 0)
	{
	   break;
	}
     }
     else
     {
        DPRINT("Message mode\n");

        /* Message mode */
        if (!IsListEmpty(&ReadFcb->DataListHead))
	{
	   CurrentEntry = RemoveHeadList(&ReadFcb->DataListHead);
	   Current = CONTAINING_RECORD(CurrentEntry, NPFS_PIPE_DATA, ListEntry);

	   DPRINT("Took pipe data at %p off the queue\n", Current);

	   /* Truncate the message if the receive buffer is too small */
	   CopyLength = RtlMin(Current->Size, Length);
	   RtlCopyMemory(Buffer, Current->Data, CopyLength);
	   Information = CopyLength;

	   Current->Offset += CopyLength;
	   NpfsFreePipeData(Current);
	}
	if (Information > 0)
	{
	   break;
	}
     }
  }
  /* reset ReaderEvent */
  KeReleaseSpinLock(&ReadFcb->DataListLock, OldIrql);


done:
  Irp->IoStatus.Status = Status;
  Irp->IoStatus.Information = Information;

  IoCompleteRequest(Irp, IO_NO_INCREMENT);

  return(Status);
}


NTSTATUS STDCALL
NpfsWrite(PDEVICE_OBJECT DeviceObject,
	  PIRP Irp)
{
  PIO_STACK_LOCATION IoStack;
  PFILE_OBJECT FileObject;
  PNPFS_FCB Fcb = NULL;
  PNPFS_PIPE Pipe = NULL;
  PUCHAR Buffer;
  NTSTATUS Status = STATUS_SUCCESS;
  ULONG Length;
  ULONG Offset;
  KIRQL OldIrql;
  PNPFS_PIPE_DATA PipeData;

  DPRINT("NpfsWrite()\n");

  IoStack = IoGetCurrentIrpStackLocation(Irp);
  FileObject = IoStack->FileObject;
  DPRINT("FileObject %p\n", FileObject);
  DPRINT("Pipe name %wZ\n", &FileObject->FileName);

  Fcb = FileObject->FsContext;
  Pipe = Fcb->Pipe;

  Length = IoStack->Parameters.Write.Length;
  Offset = IoStack->Parameters.Write.ByteOffset.u.LowPart;

  if (Irp->MdlAddress == NULL)
    {
      DbgPrint ("Irp->MdlAddress == NULL\n");
      Status = STATUS_UNSUCCESSFUL;
      Length = 0;
      goto done;
    }

  if (Fcb->OtherSide == NULL)
    {
      DPRINT("Pipe is NOT connected!\n");
      Status = STATUS_UNSUCCESSFUL;
      Length = 0;
      goto done;
    }

  Buffer = MmGetSystemAddressForMdl (Irp->MdlAddress);
  DPRINT("Length %d Buffer %x Offset %x\n",Length,Buffer,Offset);

  PipeData = NpfsInitializePipeData(Buffer, Length);
  if (PipeData)
    {
      DPRINT("Attaching pipe data at %p (%d bytes)\n", PipeData, Length);

      KeAcquireSpinLock(&Fcb->DataListLock, &OldIrql);
      InsertTailList(&Fcb->DataListHead, &PipeData->ListEntry);

      /* signal the readers ReadEvent */
      KeSetEvent(&Fcb->OtherSide->ReadEvent, IO_NO_INCREMENT, FALSE);

      KeReleaseSpinLock(&Fcb->DataListLock, OldIrql);

    }
  else
    {
      Length = 0;
      Status = STATUS_INSUFFICIENT_RESOURCES;
    }

done:
  Irp->IoStatus.Status = Status;
  Irp->IoStatus.Information = Length;
  
  IoCompleteRequest(Irp, IO_NO_INCREMENT);
  
  return(Status);
}

/* EOF */
