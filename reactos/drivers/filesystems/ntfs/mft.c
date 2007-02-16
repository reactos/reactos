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
/* $Id$
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             services/fs/ntfs/mft.c
 * PURPOSE:          NTFS filesystem driver
 * PROGRAMMER:       Eric Kohl
 *                   Updated by Valentin Verkhovsky  2003/09/12
 */

/* INCLUDES *****************************************************************/

#include "ntfs.h"

#define NDEBUG
#include <debug.h>

/* GLOBALS *****************************************************************/


/* FUNCTIONS ****************************************************************/


NTSTATUS
NtfsOpenMft (PDEVICE_EXTENSION Vcb)
{
//  PVOID Bitmap;
  PFILE_RECORD_HEADER MftRecord;
  PFILE_RECORD_HEADER FileRecord;
//  PATTRIBUTE Attribute;
//  PATTRIBUTE AttrData;
//  PRESIDENT_ATTRIBUTE ResAttr;

  NTSTATUS Status;
  ULONG BytesPerFileRecord;
  ULONG n;
  ULONG i;

  DPRINT1("NtfsOpenMft() called\n");

  BytesPerFileRecord = Vcb->NtfsInfo.BytesPerFileRecord;

  MftRecord = ExAllocatePool(NonPagedPool,
			     BytesPerFileRecord);
  if (MftRecord == NULL)
    {
      return STATUS_INSUFFICIENT_RESOURCES;
    }

  Status = NtfsReadSectors(Vcb->StorageDevice,
			   Vcb->NtfsInfo.MftStart.u.LowPart * Vcb->NtfsInfo.SectorsPerCluster,
			   BytesPerFileRecord / Vcb->NtfsInfo.BytesPerSector,
			   Vcb->NtfsInfo.BytesPerSector,
			   (PVOID)MftRecord,
			   FALSE);
  if (!NT_SUCCESS(Status))
    {
      ExFreePool(MftRecord);
      return Status;
    }


  FixupUpdateSequenceArray(MftRecord);

//  Attribute = FindAttribute(MftRecord, AttributeBitmap, 0);

  /* Get number of file records*/
  n = AttributeDataLength (FindAttribute (MftRecord, AttributeData, 0))
		  / BytesPerFileRecord;

  FileRecord = ExAllocatePool(NonPagedPool, BytesPerFileRecord);
  if (FileRecord == NULL)
    {
      ExFreePool(MftRecord);
      return(STATUS_INSUFFICIENT_RESOURCES);
    }

  /* Enumerate MFT Records */
  DPRINT("Enumerate MFT records\n");
  for ( i=0; i < n; i++)
    {
	  ReadFileRecord(Vcb, i, FileRecord, MftRecord);

	  if (FileRecord->Ntfs.Type == NRH_FILE_TYPE && (FileRecord->Flags & FRH_IN_USE))
	    {
	      DPRINT("\nFile  %lu\n\n", i);

	      /* Enumerate attributtes */
	      NtfsDumpFileAttributes (FileRecord);
	      DbgPrint("\n\n");
	     }
    }

  ExFreePool(FileRecord);
  ExFreePool(MftRecord);

  return Status;
}


PATTRIBUTE
FindAttribute (PFILE_RECORD_HEADER FileRecord,
	       ATTRIBUTE_TYPE Type,
	       PWSTR name)
{
  PATTRIBUTE Attribute;

  Attribute = (PATTRIBUTE)((ULONG_PTR)FileRecord + FileRecord->AttributeOffset);
  while (Attribute < (PATTRIBUTE)((ULONG_PTR)FileRecord + FileRecord->BytesInUse) &&
         Attribute->AttributeType != (ATTRIBUTE_TYPE)-1)
    {
      if (Attribute->AttributeType == Type)
	{
	  return Attribute;
	}

      Attribute = (PATTRIBUTE)((ULONG_PTR)Attribute + Attribute->Length);
    }

  return NULL;
}


ULONG
AttributeAllocatedLength (PATTRIBUTE Attribute)
{
  if (Attribute->Nonresident)
    {
      return ((PNONRESIDENT_ATTRIBUTE)Attribute)->AllocatedSize;
    }

  return ((PRESIDENT_ATTRIBUTE)Attribute)->ValueLength;
}


ULONG
AttributeDataLength (PATTRIBUTE Attribute)
{
  if (Attribute->Nonresident)
    {
      return ((PNONRESIDENT_ATTRIBUTE)Attribute)->DataSize;
    }

  return ((PRESIDENT_ATTRIBUTE)Attribute)->ValueLength;
}


VOID
ReadAttribute (PATTRIBUTE attr,
	       PVOID buffer,
	       PDEVICE_EXTENSION Vcb,
	       PDEVICE_OBJECT DeviceObject)
{
  PNONRESIDENT_ATTRIBUTE NresAttr = (PNONRESIDENT_ATTRIBUTE)attr;
  if (attr->Nonresident == FALSE)
    {
	memcpy (buffer,
		(PVOID)((ULONG_PTR)attr + ((PRESIDENT_ATTRIBUTE)attr)->ValueOffset),
		((PRESIDENT_ATTRIBUTE)attr)->ValueLength);
    }

  ReadExternalAttribute(Vcb, NresAttr, 0, (ULONG)(NresAttr->LastVcn) + 1,
		buffer);
}





NTSTATUS
ReadFileRecord (PDEVICE_EXTENSION Vcb,
		ULONG index,
		PFILE_RECORD_HEADER file,
		PFILE_RECORD_HEADER Mft)
{
  PVOID p;
  ULONG BytesPerFileRecord = Vcb->NtfsInfo.BytesPerFileRecord;
  ULONG clusters = max(BytesPerFileRecord / Vcb->NtfsInfo.BytesPerCluster, 1);
  ULONGLONG vcn = index * BytesPerFileRecord / Vcb->NtfsInfo.BytesPerCluster;
  LONG m = (Vcb->NtfsInfo.BytesPerCluster / BytesPerFileRecord) - 1;
  ULONG n = m > 0 ? (index & m) : 0;
  
  p = ExAllocatePool(NonPagedPool, clusters * Vcb->NtfsInfo.BytesPerCluster);

  ReadVCN (Vcb, Mft, AttributeData, vcn, clusters, p);

  memcpy(file, (PVOID)((ULONG_PTR)p + n * BytesPerFileRecord), BytesPerFileRecord);

  ExFreePool(p);

  FixupUpdateSequenceArray(file);

  return STATUS_SUCCESS;
}



VOID
ReadExternalAttribute (PDEVICE_EXTENSION Vcb,
		       PNONRESIDENT_ATTRIBUTE NresAttr,
		       ULONGLONG vcn,
		       ULONG count,
		       PVOID buffer)
{
  ULONGLONG lcn;
  ULONGLONG runcount;
  ULONG readcount;
  ULONG left;
  ULONG n;

    PUCHAR bytes = (PUCHAR)buffer;

  for (left = count; left>0; left -=readcount)
    {
		FindRun(NresAttr, vcn, &lcn, &runcount);

//		readcount = (ULONG)(__min(runcount, left));
		readcount = (ULONG)min (runcount, left);


		n = readcount * Vcb->NtfsInfo.BytesPerCluster;

		if (lcn == 0)
			memset(bytes, 0, n);
		else
		  ReadLCN(Vcb, lcn, readcount, bytes);

		vcn += readcount;
		bytes += n;


    }
}


VOID
ReadVCN (PDEVICE_EXTENSION Vcb,
	 PFILE_RECORD_HEADER file,
	 ATTRIBUTE_TYPE type,
	 ULONGLONG vcn,
	 ULONG count,
	 PVOID buffer)
{
  PNONRESIDENT_ATTRIBUTE NresAttr;
  PATTRIBUTE attr;

  attr = FindAttribute(file, type, 0);

  NresAttr = (PNONRESIDENT_ATTRIBUTE) attr;

  if (NresAttr == 0 || (vcn < NresAttr->StartVcn ||vcn > NresAttr->LastVcn))
    {
//      PATTRIBUTE attrList = FindAttribute(file,AttributeAttributeList,0);
      DbgPrint("Exeption \n");
//      KeDebugCheck(0);
    }

  ReadExternalAttribute(Vcb, NresAttr, vcn, count, buffer);
}


#if 0
BOOL bitset(PUCHAR bitmap, ULONG i)
{
	return (bitmap[i>>3] & (1 << (i & 7))) !=0;
}
#endif


VOID FixupUpdateSequenceArray(PFILE_RECORD_HEADER file)
{
	PUSHORT usa = (PUSHORT)((ULONG_PTR)file + file->Ntfs.UsaOffset);
	PUSHORT sector = (PUSHORT)file;
    ULONG i;

	for( i =1; i < file->Ntfs.UsaCount; i++)
	{
		sector[255] = usa[i];
		sector += 256;

	}

}


NTSTATUS
ReadLCN (PDEVICE_EXTENSION Vcb,
	 ULONGLONG lcn,
	 ULONG count,
	 PVOID buffer)
{
  LARGE_INTEGER DiskSector;

  DiskSector.QuadPart = lcn;

  return NtfsReadSectors (Vcb->StorageDevice,
			  DiskSector.u.LowPart * Vcb->NtfsInfo.SectorsPerCluster,
			  count * Vcb->NtfsInfo.SectorsPerCluster,
			  Vcb->NtfsInfo.BytesPerSector,
			  buffer,
			  FALSE);
}

/* EOF */
