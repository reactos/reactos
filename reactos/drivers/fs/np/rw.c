/* $Id: rw.c,v 1.5 2001/11/20 20:34:29 ekohl Exp $
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

static inline VOID
NpfsFreePipeData(PNPFS_PIPE_DATA PipeData)
{
  if (PipeData->Data)
    {
      ExFreePool(PipeData->Data);
    }

  ExFreeToNPagedLookasideList(&NpfsPipeDataLookasideList, PipeData);
}


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
  BOOLEAN DataListEmpty;

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
      Length = 0;
      goto done;
    }

  if (Irp->MdlAddress == NULL)
    {
      DPRINT("Irp->MdlAddress == NULL\n");
      Status = STATUS_UNSUCCESSFUL;
      Length = 0;
      goto done;
    }

  Status = STATUS_SUCCESS;
  Length = IoStack->Parameters.Read.Length;

  Buffer = MmGetSystemAddressForMdl(Irp->MdlAddress);
  DPRINT("Length %d Buffer %x\n",Length,Buffer);

  KeAcquireSpinLock(&ReadFcb->DataListLock, &OldIrql);
  DataListEmpty = IsListEmpty(&ReadFcb->DataListHead);
  KeReleaseSpinLock(&ReadFcb->DataListLock, OldIrql);

  /* FIXME: check if in blocking mode */
  if (DataListEmpty == TRUE)
    {
      /* Wait for ReadEvent to become signaled */
      DPRINT("Waiting for readable data\n");
      Status = KeWaitForSingleObject(&Fcb->ReadEvent,
				     UserRequest,
				     KernelMode,
				     FALSE,
				     NULL);
      DPRINT("Finished waiting! Status: %x\n", Status);
    }

  KeAcquireSpinLock(&ReadFcb->DataListLock, &OldIrql);

  if (Pipe->PipeReadMode & FILE_PIPE_BYTE_STREAM_MODE)
    {
      DPRINT("Byte stream mode\n");

      /* Byte stream mode */
      Information = 0;
      CurrentEntry = ReadFcb->DataListHead.Flink;
       while ((Length > 0) && (CurrentEntry = RemoveHeadList(&ReadFcb->DataListHead)))
	{
	  Current = CONTAINING_RECORD(CurrentEntry, NPFS_PIPE_DATA, ListEntry);

	  DPRINT("Took pipe data at %p off the queue\n", Current);

	  CopyLength = RtlMin(Current->Size, Length);
	  RtlCopyMemory(Buffer,
			((PVOID)((ULONG_PTR)Current->Data + Current->Offset)),
			CopyLength);
	  Buffer += CopyLength;
	  Length -= CopyLength;
	  Information += CopyLength;

	  /* Update the data buffer */
	  Current->Offset += CopyLength;
	  Current->Size -= CopyLength;

	  CurrentEntry = CurrentEntry->Flink;
	}

      if ((CurrentEntry != &ReadFcb->DataListHead) && (Current->Offset != Current->Size))
	{
	  DPRINT("Putting pipe data at %p back in queue\n", Current);

	  /* The caller's buffer could not contain the complete message,
	     so put it back on the queue */
	  InsertHeadList(&ReadFcb->DataListHead, &Current->ListEntry);
	}
    }
  else
    {
      DPRINT("Message mode\n");

      /* Message mode */
      CurrentEntry = ReadFcb->DataListHead.Flink;
      if (CurrentEntry = RemoveHeadList(&ReadFcb->DataListHead))
	{
	  Current = CONTAINING_RECORD(CurrentEntry, NPFS_PIPE_DATA, ListEntry);

	  DPRINT("Took pipe data at %p off the queue\n", Current);

	  /* Truncate the message if the receive buffer is too small */
	  CopyLength = RtlMin(Current->Size, Length);
	  RtlCopyMemory(Buffer, Current->Data, CopyLength);
	  Information = CopyLength;

	  Current->Offset += CopyLength;

	  CurrentEntry = CurrentEntry->Flink;
	}
    }

  KeReleaseSpinLock(&ReadFcb->DataListLock, OldIrql);

  /* reset ReaderEvent */
  KeResetEvent(&Fcb->ReadEvent);

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
      KeReleaseSpinLock(&Fcb->DataListLock, OldIrql);

      /* signal the readers ReadEvent */
      KeSetEvent(&Fcb->OtherSide->ConnectEvent, IO_NO_INCREMENT, FALSE);
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
