/*
 *  ReactOS kernel
 *  Copyright (C) 2002 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/* $Id: rw.c,v 1.5 2002/09/09 17:26:24 hbirr Exp $
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             services/fs/cdfs/rw.c
 * PURPOSE:          CDROM (ISO 9660) filesystem driver
 * PROGRAMMER:       Art Yerkes
 * UPDATE HISTORY: 
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <ntos/minmax.h>

#define NDEBUG
#include <debug.h>

#include "cdfs.h"


/* GLOBALS *******************************************************************/

#define ROUND_UP(N, S) ((((N) + (S) - 1) / (S)) * (S))
#define ROUND_DOWN(N, S) ((N) - ((N) % (S)))


/* FUNCTIONS ****************************************************************/

static NTSTATUS
CdfsReadFile(PDEVICE_EXTENSION DeviceExt,
	     PFILE_OBJECT FileObject,
	     PUCHAR Buffer,
	     ULONG Length,
	     ULONG ReadOffset,
	     ULONG IrpFlags,
	     PULONG LengthRead)
/*
 * FUNCTION: Reads data from a file
 */
{
  NTSTATUS Status = STATUS_SUCCESS;
  PUCHAR TempBuffer;
  ULONG TempLength;
  PCCB Ccb;
  PFCB Fcb;

  DPRINT("CdfsReadFile(ReadOffset %lu  Length %lu)\n", ReadOffset, Length);

  *LengthRead = 0;

  if (Length == 0)
    return STATUS_SUCCESS;

  Ccb = (PCCB)FileObject->FsContext2;
  Fcb = Ccb->Fcb;

  if (ReadOffset + Length > Fcb->Entry.DataLengthL)
    Length = Fcb->Entry.DataLengthL - ReadOffset;

  DPRINT("Reading %d bytes at %d\n", Length, ReadOffset);

  if (Length == 0)
    return(STATUS_UNSUCCESSFUL);

  if (!(IrpFlags & (IRP_NOCACHE|IRP_PAGING_IO)))
    {
      LARGE_INTEGER FileOffset;
      IO_STATUS_BLOCK IoStatus;

      if (FileObject->PrivateCacheMap == NULL)
      {
	  CcRosInitializeFileCache(FileObject, &Fcb->RFCB.Bcb, PAGESIZE);
      }

      FileOffset.QuadPart = (LONGLONG)ReadOffset;
      CcCopyRead(FileObject,
		 &FileOffset,
		 Length,
		 TRUE,
		 Buffer,
		 &IoStatus);
      *LengthRead = IoStatus.Information;

      return(IoStatus.Status);
    }

  if ((ReadOffset % BLOCKSIZE) != 0)
    {
      TempLength = min(Length, BLOCKSIZE - (ReadOffset % BLOCKSIZE));
      TempBuffer = ExAllocatePool(NonPagedPool, BLOCKSIZE);

      Status = CdfsReadSectors(DeviceExt->StorageDevice,
			       Fcb->Entry.ExtentLocationL + (ReadOffset / BLOCKSIZE),
			       1,
			       TempBuffer);
      if (NT_SUCCESS(Status))
	{
	  memcpy(Buffer, TempBuffer + (ReadOffset % BLOCKSIZE), TempLength);
	  (*LengthRead) = (*LengthRead) + TempLength;
	  Length = Length - TempLength;
	  Buffer = Buffer + TempLength;
	  ReadOffset = ReadOffset + TempLength;
	}
      ExFreePool(TempBuffer);
    }

  DPRINT("Status %lx\n", Status);

  if ((Length / BLOCKSIZE) != 0 && NT_SUCCESS(Status))
    {
      TempLength = ROUND_DOWN(Length, BLOCKSIZE);
      Status = CdfsReadSectors(DeviceExt->StorageDevice,
			       Fcb->Entry.ExtentLocationL + (ReadOffset / BLOCKSIZE),
			       Length / BLOCKSIZE,
			       Buffer);
      if (NT_SUCCESS(Status))
	{
	  (*LengthRead) = (*LengthRead) + TempLength;
	  Length = Length - TempLength;
	  Buffer = Buffer + TempLength;
	  ReadOffset = ReadOffset + TempLength;
	}
    }

  DPRINT("Status %lx\n", Status);

  if (Length > 0 && NT_SUCCESS(Status))
    {
      TempBuffer = ExAllocatePool(NonPagedPool, BLOCKSIZE);

      Status = CdfsReadSectors(DeviceExt->StorageDevice,
			       Fcb->Entry.ExtentLocationL + (ReadOffset / BLOCKSIZE),
			       1,
			       TempBuffer);
      if (NT_SUCCESS(Status))
	{
	  memcpy(Buffer, TempBuffer, Length);
	  (*LengthRead) = (*LengthRead) + Length;
	}
      ExFreePool(TempBuffer);
    }

  return(Status);
}


NTSTATUS STDCALL
CdfsRead(PDEVICE_OBJECT DeviceObject,
	 PIRP Irp)
{
  PDEVICE_EXTENSION DeviceExt;
  PIO_STACK_LOCATION Stack;
  PFILE_OBJECT FileObject;
  PVOID Buffer;
  ULONG ReadLength;
  LARGE_INTEGER ReadOffset;
  ULONG ReturnedReadLength = 0;
  NTSTATUS Status = STATUS_SUCCESS;

  DPRINT("CdfsRead(DeviceObject %x, Irp %x)\n",DeviceObject,Irp);

  DeviceExt = DeviceObject->DeviceExtension;
  Stack = IoGetCurrentIrpStackLocation(Irp);
  FileObject = Stack->FileObject;

  ReadLength = Stack->Parameters.Read.Length;
  ReadOffset = Stack->Parameters.Read.ByteOffset;
  Buffer = MmGetSystemAddressForMdl(Irp->MdlAddress);

  Status = CdfsReadFile(DeviceExt,
			FileObject,
			Buffer,
			ReadLength,
			ReadOffset.u.LowPart,
			Irp->Flags,
			&ReturnedReadLength);

ByeBye:
  if (NT_SUCCESS(Status))
    {
      if (FileObject->Flags & FO_SYNCHRONOUS_IO)
	{
	  FileObject->CurrentByteOffset.QuadPart = 
	    ReadOffset.QuadPart + ReturnedReadLength;
	}
      Irp->IoStatus.Information = ReturnedReadLength;
    }
  else
    {
      Irp->IoStatus.Information = 0;
    }

  Irp->IoStatus.Status = Status;
  IoCompleteRequest(Irp,IO_NO_INCREMENT);

  return(Status);
}


NTSTATUS STDCALL
CdfsWrite(PDEVICE_OBJECT DeviceObject,
	  PIRP Irp)
{
  DPRINT("CdfsWrite(DeviceObject %x Irp %x)\n",DeviceObject,Irp);

  Irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
  Irp->IoStatus.Information = 0;
  return(STATUS_NOT_SUPPORTED);
}

/* EOF */
