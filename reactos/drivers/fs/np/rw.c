/* $Id$
 *
 * COPYRIGHT:  See COPYING in the top level directory
 * PROJECT:    ReactOS kernel
 * FILE:       drivers/fs/np/rw.c
 * PURPOSE:    Named pipe filesystem
 * PROGRAMMER: David Welch <welch@cwcom.net>
 */

/* INCLUDES ******************************************************************/

#include <ddk/ntddk.h>
#include <rosrtl/minmax.h>
#include "npfs.h"

#define NDEBUG
#include <debug.h>

/* FUNCTIONS *****************************************************************/

#ifndef NDEBUG
VOID HexDump(PUCHAR Buffer, ULONG Length)
{
  CHAR Line[65];
  UCHAR ch;
  const char Hex[] = "0123456789ABCDEF";
  int i, j;

  DbgPrint("---------------\n");

  for (i = 0; i < ROUND_UP(Length, 16); i+= 16)
    {
      memset(Line, ' ', 64);
      Line[64] = 0;

      for (j = 0; j < 16 && j + i < Length; j++)
        {
          ch = Buffer[i + j];
          Line[3*j + 0] = Hex[ch >> 4];
	  Line[3*j + 1] = Hex[ch & 0x0f];
	  Line[48 + j] = isprint(ch) ? ch : '.';
        }
      DbgPrint("%s\n", Line);
    }
  DbgPrint("---------------\n");
}
#endif


NTSTATUS STDCALL
NpfsRead(PDEVICE_OBJECT DeviceObject,
	 PIRP Irp)
{
  PIO_STACK_LOCATION IoStack;
  PFILE_OBJECT FileObject;
  NTSTATUS Status;
  PNPFS_DEVICE_EXTENSION DeviceExt;
  KIRQL OldIrql;
  ULONG Information;
  PNPFS_FCB Fcb;
  PNPFS_FCB WriterFcb;
  PNPFS_PIPE Pipe;
  ULONG Length;
  PVOID Buffer;
  ULONG CopyLength;
  ULONG TempLength;

  DPRINT("NpfsRead(DeviceObject %p  Irp %p)\n", DeviceObject, Irp);

  DeviceExt = (PNPFS_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
  IoStack = IoGetCurrentIrpStackLocation(Irp);
  FileObject = IoStack->FileObject;
  Fcb = FileObject->FsContext;
  Pipe = Fcb->Pipe;
  WriterFcb = Fcb->OtherSide;

  if (Irp->MdlAddress == NULL)
    {
      DPRINT("Irp->MdlAddress == NULL\n");
      Status = STATUS_UNSUCCESSFUL;
      Information = 0;
      goto done;
    }

  if (Fcb->Data == NULL)
    {
      DPRINT("Pipe is NOT readable!\n");
      Status = STATUS_UNSUCCESSFUL;
      Information = 0;
      goto done;
    }

  Status = STATUS_SUCCESS;
  Length = IoStack->Parameters.Read.Length;
  Information = 0;

  Buffer = MmGetSystemAddressForMdl(Irp->MdlAddress);
  KeAcquireSpinLock(&Fcb->DataListLock, &OldIrql);
  while (1)
    {
      /* FIXME: check if in blocking mode */
      if (Fcb->ReadDataAvailable == 0)
	{
	  if (Fcb->PipeState == FILE_PIPE_CONNECTED_STATE)
	    {
	      KeSetEvent(&WriterFcb->Event, IO_NO_INCREMENT, FALSE);
	    }
	  KeReleaseSpinLock(&Fcb->DataListLock, OldIrql);
	  if (Information > 0)
	    {
	      Status = STATUS_SUCCESS;
	      goto done;
	    }

	  if (Fcb->PipeState != FILE_PIPE_CONNECTED_STATE)
	    {
	      DPRINT("PipeState: %x\n", Fcb->PipeState);
	      Status = STATUS_PIPE_BROKEN;
	      goto done;
	    }

	  /* Wait for ReadEvent to become signaled */
	  DPRINT("Waiting for readable data (%S)\n", Pipe->PipeName.Buffer);
	  Status = KeWaitForSingleObject(&Fcb->Event,
				         UserRequest,
				         KernelMode,
				         FALSE,
				         NULL);
	  DPRINT("Finished waiting (%S)! Status: %x\n", Pipe->PipeName.Buffer, Status);

	  KeAcquireSpinLock(&Fcb->DataListLock, &OldIrql);
	}

      if (Pipe->ReadMode == FILE_PIPE_BYTE_STREAM_MODE)
	{
	  DPRINT("Byte stream mode\n");
	  /* Byte stream mode */
	  while (Length > 0 && Fcb->ReadDataAvailable > 0)
	    {
	      CopyLength = RtlRosMin(Fcb->ReadDataAvailable, Length);
	      if (Fcb->ReadPtr + CopyLength <= Fcb->Data + Fcb->MaxDataLength)
		{
		  memcpy(Buffer, Fcb->ReadPtr, CopyLength);
		  Fcb->ReadPtr += CopyLength;
		  if (Fcb->ReadPtr == Fcb->Data + Fcb->MaxDataLength)
		    {
		      Fcb->ReadPtr = Fcb->Data;
		    }
		}
	      else
		{
		  TempLength = Fcb->Data + Fcb->MaxDataLength - Fcb->ReadPtr;
		  memcpy(Buffer, Fcb->ReadPtr, TempLength);
		  memcpy(Buffer + TempLength, Fcb->Data, CopyLength - TempLength);
		  Fcb->ReadPtr = Fcb->Data + CopyLength - TempLength;
		}

	      Buffer += CopyLength;
	      Length -= CopyLength;
	      Information += CopyLength;

	      Fcb->ReadDataAvailable -= CopyLength;
	      Fcb->WriteQuotaAvailable += CopyLength;
	    }

	  if (Length == 0)
	    {
	      if (Fcb->PipeState == FILE_PIPE_CONNECTED_STATE)
	        {
	          KeSetEvent(&WriterFcb->Event, IO_NO_INCREMENT, FALSE);
	        }
	      KeResetEvent(&Fcb->Event);
	      break;
	    }
	}
      else
	{
	  DPRINT("Message mode\n");

	  /* Message mode */
	  if (Fcb->ReadDataAvailable)
	    {
	      /* Truncate the message if the receive buffer is too small */
	      CopyLength = RtlRosMin(Fcb->ReadDataAvailable, Length);
	      memcpy(Buffer, Fcb->Data, CopyLength);

#ifndef NDEBUG
	      DPRINT("Length %d Buffer %x\n",CopyLength,Buffer);
	      HexDump((PUCHAR)Buffer, CopyLength);
#endif

	      Information = CopyLength;

	      if (Fcb->ReadDataAvailable > Length)
	        {
	          memmove(Fcb->Data, Fcb->Data + Length,
	                  Fcb->ReadDataAvailable - Length);
	          Fcb->ReadDataAvailable -= Length;
	          Status = STATUS_MORE_ENTRIES;
	        }
	      else
	        {
                  KeResetEvent(&Fcb->Event);
                  if (Fcb->PipeState == FILE_PIPE_CONNECTED_STATE)
                    {
                      KeSetEvent(&WriterFcb->Event, IO_NO_INCREMENT, FALSE);
                    }
	          Fcb->ReadDataAvailable = 0;
	          Fcb->WriteQuotaAvailable = Fcb->MaxDataLength;
	        }
	    }

	  if (Information > 0)
	    {
	      break;
	    }
	}
    }

  KeReleaseSpinLock(&Fcb->DataListLock, OldIrql);

done:
  Irp->IoStatus.Status = Status;
  Irp->IoStatus.Information = Information;

  IoCompleteRequest(Irp, IO_NO_INCREMENT);

  DPRINT("NpfsRead done (Status %lx)\n", Status);

  return Status;
}


NTSTATUS STDCALL
NpfsWrite(PDEVICE_OBJECT DeviceObject,
	  PIRP Irp)
{
  PIO_STACK_LOCATION IoStack;
  PFILE_OBJECT FileObject;
  PNPFS_FCB Fcb = NULL;
  PNPFS_FCB ReaderFcb;
  PNPFS_PIPE Pipe = NULL;
  PUCHAR Buffer;
  NTSTATUS Status = STATUS_SUCCESS;
  ULONG Length;
  ULONG Offset;
  KIRQL OldIrql;
  ULONG Information;
  ULONG CopyLength;
  ULONG TempLength;

  DPRINT("NpfsWrite()\n");

  IoStack = IoGetCurrentIrpStackLocation(Irp);
  FileObject = IoStack->FileObject;
  DPRINT("FileObject %p\n", FileObject);
  DPRINT("Pipe name %wZ\n", &FileObject->FileName);

  Fcb = FileObject->FsContext;
  ReaderFcb = Fcb->OtherSide;
  Pipe = Fcb->Pipe;

  Length = IoStack->Parameters.Write.Length;
  Offset = IoStack->Parameters.Write.ByteOffset.u.LowPart;
  Information = 0;

  if (Irp->MdlAddress == NULL)
    {
      DPRINT("Irp->MdlAddress == NULL\n");
      Status = STATUS_UNSUCCESSFUL;
      Length = 0;
      goto done;
    }

  if (ReaderFcb == NULL)
    {
      DPRINT("Pipe is NOT connected!\n");
      if (Fcb->PipeState == FILE_PIPE_LISTENING_STATE)
        Status = STATUS_PIPE_LISTENING;
      else if (Fcb->PipeState == FILE_PIPE_DISCONNECTED_STATE)
        Status = STATUS_PIPE_DISCONNECTED;
      else
        Status = STATUS_UNSUCCESSFUL;
      Length = 0;
      goto done;
    }

  if (ReaderFcb->Data == NULL)
    {
      DPRINT("Pipe is NOT writable!\n");
      Status = STATUS_UNSUCCESSFUL;
      Length = 0;
      goto done;
    }

  Status = STATUS_SUCCESS;
  Buffer = MmGetSystemAddressForMdl (Irp->MdlAddress);

  KeAcquireSpinLock(&ReaderFcb->DataListLock, &OldIrql);
#ifndef NDEBUG
  DPRINT("Length %d Buffer %x Offset %x\n",Length,Buffer,Offset);
  HexDump(Buffer, Length);
#endif

  while(1)
    {
      if (ReaderFcb->WriteQuotaAvailable == 0)
	{
	  KeSetEvent(&ReaderFcb->Event, IO_NO_INCREMENT, FALSE);
	  KeReleaseSpinLock(&ReaderFcb->DataListLock, OldIrql);
	  if (Fcb->PipeState != FILE_PIPE_CONNECTED_STATE)
	    {
	      Status = STATUS_PIPE_BROKEN;
	      goto done;
	    }

	  DPRINT("Waiting for buffer space (%S)\n", Pipe->PipeName.Buffer);
	  Status = KeWaitForSingleObject(&Fcb->Event,
				         UserRequest,
				         KernelMode,
				         FALSE,
				         NULL);
	  DPRINT("Finished waiting (%S)! Status: %x\n", Pipe->PipeName.Buffer, Status);

	  /*
	   * It's possible that the event was signaled because the
	   * other side of pipe was closed.
	   */
	  if (Fcb->PipeState != FILE_PIPE_CONNECTED_STATE)
	    {
	      DPRINT("PipeState: %x\n", Fcb->PipeState);
	      Status = STATUS_PIPE_BROKEN;
	      goto done;
	    }
	  KeAcquireSpinLock(&ReaderFcb->DataListLock, &OldIrql);
	}

      if (Pipe->WriteMode == FILE_PIPE_BYTE_STREAM_MODE)
	{
	  DPRINT("Byte stream mode\n");
	  while (Length > 0 && ReaderFcb->WriteQuotaAvailable > 0)
	    {
	      CopyLength = RtlRosMin(Length, ReaderFcb->WriteQuotaAvailable);
	      if (ReaderFcb->WritePtr + CopyLength <= ReaderFcb->Data + ReaderFcb->MaxDataLength)
		{
		  memcpy(ReaderFcb->WritePtr, Buffer, CopyLength);
		  ReaderFcb->WritePtr += CopyLength;
		  if (ReaderFcb->WritePtr == ReaderFcb->Data + ReaderFcb->MaxDataLength)
		    {
		      ReaderFcb->WritePtr = ReaderFcb->Data;
		    }
		}
	      else
		{
		  TempLength = ReaderFcb->Data + ReaderFcb->MaxDataLength - ReaderFcb->WritePtr;
		  memcpy(ReaderFcb->WritePtr, Buffer, TempLength);
		  memcpy(ReaderFcb->Data, Buffer + TempLength, CopyLength - TempLength);
		  ReaderFcb->WritePtr = ReaderFcb->Data + CopyLength - TempLength;
		}

	      Buffer += CopyLength;
	      Length -= CopyLength;
	      Information += CopyLength;

	      ReaderFcb->ReadDataAvailable += CopyLength;
	      ReaderFcb->WriteQuotaAvailable -= CopyLength;
	    }

	  if (Length == 0)
	    {
	      KeSetEvent(&ReaderFcb->Event, IO_NO_INCREMENT, FALSE);
	      KeResetEvent(&Fcb->Event);
	      break;
	    }
	}
      else
	{
	  DPRINT("Message mode\n");
	  if (Length > 0)
	    {
	      CopyLength = RtlRosMin(Length, ReaderFcb->WriteQuotaAvailable);
	      memcpy(ReaderFcb->Data, Buffer, CopyLength);

	      Information = CopyLength;
	      ReaderFcb->ReadDataAvailable = CopyLength;
	      ReaderFcb->WriteQuotaAvailable = 0;
	    }

   	  if (Information > 0)
   	    {
   	      KeSetEvent(&ReaderFcb->Event, IO_NO_INCREMENT, FALSE);
   	      KeResetEvent(&Fcb->Event);
   	      break;
   	    }
	}
    }

  KeReleaseSpinLock(&ReaderFcb->DataListLock, OldIrql);

done:
  Irp->IoStatus.Status = Status;
  Irp->IoStatus.Information = Information;

  IoCompleteRequest(Irp, IO_NO_INCREMENT);

  DPRINT("NpfsWrite done (Status %lx)\n", Status);

  return Status;
}

/* EOF */
