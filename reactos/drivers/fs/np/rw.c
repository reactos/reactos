/* $Id: rw.c,v 1.15 2004/05/10 19:58:10 navaraf Exp $
 *
 * COPYRIGHT:  See COPYING in the top level directory
 * PROJECT:    ReactOS kernel
 * FILE:       services/fs/np/rw.c
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
NpfsRead(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
  PIO_STACK_LOCATION IoStack;
  PFILE_OBJECT FileObject;
  NTSTATUS Status;
  PNPFS_DEVICE_EXTENSION DeviceExt;
  KIRQL OldIrql;
  ULONG Information;
  PNPFS_FCB Fcb;
  PNPFS_FCB ReadFcb;
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
  ReadFcb = Fcb->OtherSide;

  if (ReadFcb == NULL)
    {
      DPRINT("Pipe is NOT connected!\n");
      if (Fcb->PipeState == FILE_PIPE_LISTENING_STATE)
        Status = STATUS_PIPE_LISTENING;
      else if (Fcb->PipeState == FILE_PIPE_DISCONNECTED_STATE)
        Status = STATUS_PIPE_DISCONNECTED;
      else  
        Status = STATUS_PIPE_BROKEN;
      Information = 0;
      DPRINT("%x\n", Status);
      goto done;
    }

  if (Irp->MdlAddress == NULL)
    {
      DPRINT("Irp->MdlAddress == NULL\n");
      Status = STATUS_UNSUCCESSFUL;
      Information = 0;
      goto done;
    }
  
  if (ReadFcb->Data == NULL)
    {
      DPRINT("Pipe is NOT readable!\n");
      Status = STATUS_UNSUCCESSFUL;
      Information = 0;
      goto done;
    }

#ifdef FIN_WORKAROUND_READCLOSE
  if (ReadFcb->ReadDataAvailable == 0 &&
      ReadFcb->PipeState == FILE_PIPE_CLOSING_STATE)
    {
      DPRINT("Other end of pipe is closed!\n");
      Status = STATUS_PIPE_BROKEN;
      Information = 0;
      goto done;
    }
#endif

  Status = STATUS_SUCCESS;
  Length = IoStack->Parameters.Read.Length;
  Information = 0;

  Buffer = MmGetSystemAddressForMdl(Irp->MdlAddress);
  KeAcquireSpinLock(&ReadFcb->DataListLock, &OldIrql);
  while (1)
    {
      /* FIXME: check if in blocking mode */
      if (ReadFcb->ReadDataAvailable == 0)
        {
          KeResetEvent(&Fcb->Event);
	  KeSetEvent(&ReadFcb->Event, IO_NO_INCREMENT, FALSE);
          KeReleaseSpinLock(&ReadFcb->DataListLock, OldIrql);
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
#ifndef FIN_WORKAROUND_READCLOSE
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
#endif
          KeAcquireSpinLock(&ReadFcb->DataListLock, &OldIrql);
	}

     if (Pipe->PipeReadMode == FILE_PIPE_BYTE_STREAM_MODE)
       {
         DPRINT("Byte stream mode\n");
         /* Byte stream mode */
	 while (Length > 0 && ReadFcb->ReadDataAvailable > 0)
	   {
	     CopyLength = RtlRosMin(ReadFcb->ReadDataAvailable, Length);
	     if (ReadFcb->ReadPtr + CopyLength <= ReadFcb->Data + ReadFcb->MaxDataLength)
	       {
                 memcpy(Buffer, ReadFcb->ReadPtr, CopyLength);
		 ReadFcb->ReadPtr += CopyLength;
		 if (ReadFcb->ReadPtr == ReadFcb->Data + ReadFcb->MaxDataLength)
		   {
		     ReadFcb->ReadPtr = ReadFcb->Data;
		   }
	       }
	     else
	       {
	         TempLength = ReadFcb->Data + ReadFcb->MaxDataLength - ReadFcb->ReadPtr;
		 memcpy(Buffer, ReadFcb->ReadPtr, TempLength);
		 memcpy(Buffer + TempLength, ReadFcb->Data, CopyLength - TempLength);
		 ReadFcb->ReadPtr = ReadFcb->Data + CopyLength - TempLength;
	       }

	     Buffer += CopyLength;
	     Length -= CopyLength;
	     Information += CopyLength;

	     ReadFcb->ReadDataAvailable -= CopyLength;
	     ReadFcb->WriteQuotaAvailable += CopyLength;
	   }

	if (Length == 0)
	  {
	    KeSetEvent(&ReadFcb->Event, IO_NO_INCREMENT, FALSE);
	    break;
	  }
       }
     else
       {
         DPRINT("Message mode\n");

         /* Message mode */
	 if (ReadFcb->ReadDataAvailable)
	   {
	     /* Truncate the message if the receive buffer is too small */
	     CopyLength = RtlRosMin(ReadFcb->ReadDataAvailable, Length);
	     memcpy(Buffer, ReadFcb->Data, CopyLength);

#ifndef NDEBUG
             DPRINT("Length %d Buffer %x\n",CopyLength,Buffer);
             HexDump((PUCHAR)Buffer, CopyLength);
#endif

	     Information = CopyLength;
	     ReadFcb->ReadDataAvailable = 0;
	     ReadFcb->WriteQuotaAvailable = ReadFcb->MaxDataLength;
	   }
	 if (Information > 0)
	   {
	     KeSetEvent(&ReadFcb->Event, IO_NO_INCREMENT, FALSE);
	     break;
	   }
       }

#ifdef FIN_WORKAROUND_READCLOSE
       if (ReadFcb->ReadDataAvailable == 0 &&
           ReadFcb->PipeState == FILE_PIPE_CLOSING_STATE)
         {
           DPRINT("Other end of pipe is closed!\n");
           break;
         }
#endif
    }
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
  ULONG Information;
  ULONG CopyLength;
  ULONG TempLength;

  DPRINT("NpfsWrite()\n");

  IoStack = IoGetCurrentIrpStackLocation(Irp);
  FileObject = IoStack->FileObject;
  DPRINT("FileObject %p\n", FileObject);
  DPRINT("Pipe name %wZ\n", &FileObject->FileName);

  Fcb = FileObject->FsContext;
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

  if (Fcb->OtherSide == NULL)
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
  
  if (Fcb->Data == NULL)
    {
      DPRINT("Pipe is NOT writable!\n");
      Status = STATUS_UNSUCCESSFUL;
      Length = 0;
      goto done;
    }

  Status = STATUS_SUCCESS;
  Buffer = MmGetSystemAddressForMdl (Irp->MdlAddress);

  KeAcquireSpinLock(&Fcb->DataListLock, &OldIrql);
#ifndef NDEBUG
  DPRINT("Length %d Buffer %x Offset %x\n",Length,Buffer,Offset);
  HexDump(Buffer, Length);
#endif

  while(1)
    {
      if (Fcb->WriteQuotaAvailable == 0)
        {
          KeResetEvent(&Fcb->Event);
	  KeSetEvent(&Fcb->OtherSide->Event, IO_NO_INCREMENT, FALSE);
          KeReleaseSpinLock(&Fcb->DataListLock, OldIrql);
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
#ifndef FIN_WORKAROUND_READCLOSE
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
#endif
          KeAcquireSpinLock(&Fcb->DataListLock, &OldIrql);
        }
      if (Pipe->PipeWriteMode == FILE_PIPE_BYTE_STREAM_MODE)
        {
          DPRINT("Byte stream mode\n");
	  while (Length > 0 && Fcb->WriteQuotaAvailable > 0)
	    {
              CopyLength = RtlRosMin(Length, Fcb->WriteQuotaAvailable);
              if (Fcb->WritePtr + CopyLength <= Fcb->Data + Fcb->MaxDataLength)
	        {
                  memcpy(Fcb->WritePtr, Buffer, CopyLength);
		  Fcb->WritePtr += CopyLength;
		  if (Fcb->WritePtr == Fcb->Data + Fcb->MaxDataLength)
		    {
		      Fcb->WritePtr = Fcb->Data;
		    }
		}
	      else
	        {
		  TempLength = Fcb->Data + Fcb->MaxDataLength - Fcb->WritePtr;
		  memcpy(Fcb->WritePtr, Buffer, TempLength);
		  memcpy(Fcb->Data, Buffer + TempLength, CopyLength - TempLength);
		  Fcb->WritePtr = Fcb->Data + CopyLength - TempLength;
		}
		  
	      Buffer += CopyLength;
	      Length -= CopyLength;
              Information += CopyLength;

	      Fcb->ReadDataAvailable += CopyLength;
	      Fcb->WriteQuotaAvailable -= CopyLength;
	    }

	  if (Length == 0)
	    {
	      KeSetEvent(&Fcb->OtherSide->Event, IO_NO_INCREMENT, FALSE);
	      break;
	    }
	}
      else
        {
          if (Length > 0)
	    {
              CopyLength = RtlRosMin(Length, Fcb->WriteQuotaAvailable);
	      memcpy(Fcb->Data, Buffer, CopyLength);

	      Information = CopyLength;
	      Fcb->ReadDataAvailable = CopyLength;
	      Fcb->WriteQuotaAvailable = 0;
	    }
	  if (Information > 0)
	    {
	      KeSetEvent(&Fcb->OtherSide->Event, IO_NO_INCREMENT, FALSE);
	      break;
	    }
	}
    }
  KeReleaseSpinLock(&Fcb->DataListLock, OldIrql);

done:
  Irp->IoStatus.Status = Status;
  Irp->IoStatus.Information = Information;
  
  IoCompleteRequest(Irp, IO_NO_INCREMENT);

  return(Status);
}

/* EOF */
