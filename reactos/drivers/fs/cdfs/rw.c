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
/* $Id: rw.c,v 1.10 2003/02/13 22:24:15 hbirr Exp $
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
    return(STATUS_SUCCESS);

  Ccb = (PCCB)FileObject->FsContext2;
  Fcb = (PFCB)FileObject->FsContext;

  if (ReadOffset >= Fcb->Entry.DataLengthL)
    return(STATUS_END_OF_FILE);

  DPRINT("Reading %d bytes at %d\n", Length, ReadOffset);

  if (!(IrpFlags & (IRP_NOCACHE|IRP_PAGING_IO)))
    {
      LARGE_INTEGER FileOffset;
      IO_STATUS_BLOCK IoStatus;

      if (ReadOffset + Length > Fcb->Entry.DataLengthL)
         Length = Fcb->Entry.DataLengthL - ReadOffset;
      if (FileObject->PrivateCacheMap == NULL)
      {
	  CcRosInitializeFileCache(FileObject, PAGE_SIZE);
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

  if ((ReadOffset % BLOCKSIZE) != 0 || (Length % BLOCKSIZE) != 0)
    {
      return STATUS_INVALID_PARAMETER;
    }
  if (ReadOffset + Length > ROUND_UP(Fcb->Entry.DataLengthL, BLOCKSIZE))
    Length = ROUND_UP(Fcb->Entry.DataLengthL, BLOCKSIZE) - ReadOffset;

  Status = CdfsReadSectors(DeviceExt->StorageDevice,
			   Fcb->Entry.ExtentLocationL + (ReadOffset / BLOCKSIZE),
			   Length / BLOCKSIZE,
			   Buffer);
  if (NT_SUCCESS(Status))
    {
      *LengthRead = Length;
      if (Length + ReadOffset > Fcb->Entry.DataLengthL)
      {
	memset(Buffer + Fcb->Entry.DataLengthL - ReadOffset, 
	       0, Length + ReadOffset - Fcb->Entry.DataLengthL);
      }
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
