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
/* $Id: mft.c,v 1.1 2002/07/15 15:37:33 ekohl Exp $
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             services/fs/ntfs/mft.c
 * PURPOSE:          NTFS filesystem driver
 * PROGRAMMER:       Eric Kohl
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>

//#define NDEBUG
#include <debug.h>

#include "ntfs.h"


/* FUNCTIONS ****************************************************************/


NTSTATUS
NtfsOpenMft(PDEVICE_OBJECT DeviceObject,
	    PDEVICE_EXTENSION Vcb)
{
  PVOID Buffer;
  PFILE_RECORD_HEADER RecordHeader;
  PATTRIBUTE Attribute;
  NTSTATUS Status;

  DPRINT1("NtfsOpenMft() called\n");

  Buffer = ExAllocatePool(NonPagedPool,
			  Vcb->NtfsInfo.BytesPerCluster);
  if (Buffer == NULL)
    {
      return(STATUS_INSUFFICIENT_RESOURCES);
    }


  /* read first MFT cluster */
  Status = NtfsReadRawSectors(DeviceObject,
			      Vcb->NtfsInfo.MftStart.u.LowPart * Vcb->NtfsInfo.SectorsPerCluster,
			      Vcb->NtfsInfo.SectorsPerCluster,
			      Vcb->NtfsInfo.BytesPerSector,
			      (PVOID)Buffer);
  if (NT_SUCCESS(Status))
    {
      /* Enumerate MFT records */
      RecordHeader = Buffer;
      while (((ULONG)RecordHeader - (ULONG)Buffer) < Vcb->NtfsInfo.BytesPerCluster)
	{
	  DbgPrint("\n");
//	  DbgPrint("Magic: %.04s\n", (PCHAR)&RecordHeader->Ntfs.Type);
//	  DbgPrint("Real size: %lx\n", RecordHeader->RealSize);
//	  DbgPrint("AllocSize: %lx\n", RecordHeader->AllocSize);

	  /* Enumerate attributes */
	  Attribute = (PATTRIBUTE)((ULONG)RecordHeader + 
				   RecordHeader->Ntfs.UsnOffset +
				   RecordHeader->Ntfs.UsnSize * sizeof(USHORT));
	  while (Attribute->AttributeType != 0xFFFFFFFF)
	    {
	      NtfsDumpAttribute(Attribute);

	      Attribute = (PATTRIBUTE)((ULONG)Attribute + Attribute->Length);
	    }


	  RecordHeader = (PFILE_RECORD_HEADER)((ULONG)RecordHeader + RecordHeader->BytesAllocated);
	}

    }

  ExFreePool(Buffer);

  return(Status);
}


/* EOF */
