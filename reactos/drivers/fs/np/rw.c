/* $Id: rw.c,v 1.9 2003/06/21 19:55:55 hbirr Exp $
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
  
  if (ReadFcb->Data == NULL)
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
  DPRINT("Length %d Buffer %x\n",Length,Buffer);

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
           KeAcquireSpinLock(&ReadFcb->DataListLock, &OldIrql);
	}

     if (Pipe->PipeReadMode == FILE_PIPE_BYTE_STREAM_MODE)
       {
         DPRINT("Byte stream mode\n");
         /* Byte stream mode */
	 while (Length > 0 && ReadFcb->ReadDataAvailable > 0)
	   {
	     CopyLength = RtlMin(ReadFcb->ReadDataAvailable, Length);
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
	     CopyLength = RtlMin(ReadFcb->ReadDataAvailable, Length);
	     memcpy(Buffer, ReadFcb->Data, CopyLength);

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
  
  if (Fcb->Data == NULL)
    {
      DPRINT("Pipe is NOT writable!\n");
      Status = STATUS_UNSUCCESSFUL;
      Length = 0;
      goto done;
    }

  Status = STATUS_SUCCESS;
  Buffer = MmGetSystemAddressForMdl (Irp->MdlAddress);
  DPRINT("Length %d Buffer %x Offset %x\n",Length,Buffer,Offset);

  KeAcquireSpinLock(&Fcb->DataListLock, &OldIrql);
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
          KeAcquireSpinLock(&Fcb->DataListLock, &OldIrql);
        }
      if (Pipe->PipeReadMode == FILE_PIPE_BYTE_STREAM_MODE)
        {
          DPRINT("Byte stream mode\n");
	  while (Length > 0 && Fcb->WriteQuotaAvailable > 0)
	    {
              CopyLength = RtlMin(Length, Fcb->WriteQuotaAvailable);
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
              CopyLength = RtlMin(Length, Fcb->WriteQuotaAvailable);
	      memcpy(Buffer, Fcb->Data, CopyLength);

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
