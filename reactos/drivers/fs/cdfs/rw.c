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
/* $Id: rw.c,v 1.1 2002/04/15 20:39:49 ekohl Exp $
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

//#define NDEBUG
#include <debug.h>

#include "cdfs.h"


/* FUNCTIONS ****************************************************************/

static NTSTATUS
CdfsReadFile(PDEVICE_EXTENSION DeviceExt,
	     PFILE_OBJECT FileObject,
	     PVOID Buffer,
	     ULONG Length,
	     ULONG Offset)
/*
 * FUNCTION: Reads data from a file
 */
{
  NTSTATUS Status;
  PCCB Ccb;
  PFCB Fcb;

  DPRINT("CdfsReadFile(Offset %lu  Length %lu)\n", Offset, Length);

  if (Length == 0)
    return STATUS_SUCCESS;

  Ccb = (PCCB)FileObject->FsContext2;
  Fcb = Ccb->Fcb;

  if (Offset + Length > Fcb->Entry.DataLengthL)
    Length = Fcb->Entry.DataLengthL - Offset;

  DPRINT( "Reading %d bytes at %d\n", Offset, Length );

  if (Length == 0)
    return(STATUS_UNSUCCESSFUL);

  Status = CdfsReadSectors(DeviceExt->StorageDevice,
			   Fcb->Entry.ExtentLocationL + Offset / BLOCKSIZE,
			   Length / BLOCKSIZE,
			   Buffer);

  return(Status);
}


NTSTATUS STDCALL
CdfsRead(PDEVICE_OBJECT DeviceObject,
	 PIRP Irp)
{
  ULONG Length;
  PVOID Buffer;
  ULONG Offset;
  PIO_STACK_LOCATION Stack = IoGetCurrentIrpStackLocation(Irp);
  PFILE_OBJECT FileObject = Stack->FileObject;
  PDEVICE_EXTENSION DeviceExt = DeviceObject->DeviceExtension;
  NTSTATUS Status;

  DPRINT("CdfsRead(DeviceObject %x, Irp %x)\n",DeviceObject,Irp);

  Buffer = MmGetSystemAddressForMdl(Irp->MdlAddress);
  Length = Stack->Parameters.Read.Length;
  Offset = Stack->Parameters.Read.ByteOffset.u.LowPart;

  Status = CdfsReadFile(DeviceExt,FileObject,Buffer,Length,Offset);

  Irp->IoStatus.Status = Status;
  Irp->IoStatus.Information = Length;
  IoCompleteRequest(Irp,IO_NO_INCREMENT);
  return(Status);
}


NTSTATUS STDCALL
CdfsWrite(PDEVICE_OBJECT DeviceObject,
	  PIRP Irp)
{
  DPRINT("CdfsWrite(DeviceObject %x Irp %x)\n",DeviceObject,Irp);

  Irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
  Irp->IoStatus.Information = 0;
  return(STATUS_UNSUCCESSFUL);
}

/* EOF */
