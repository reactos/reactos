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
/* $Id: mft.c,v 1.2 2003/09/15 16:01:16 ea Exp $
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             services/fs/ntfs/mft.c
 * PURPOSE:          NTFS filesystem driver
 * PROGRAMMER:       Eric Kohl 
 * Updated	by       Valentin Verkhovsky  2003/09/12		 	
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>

//#define NDEBUG
#include <debug.h>

#include "ntfs.h"


#define __min(a,b)  (((a) < (b)) ? (a) : (b))


/* FUNCTIONS ****************************************************************/


NTSTATUS
NtfsOpenMft(PDEVICE_OBJECT DeviceObject,
	    PDEVICE_EXTENSION Vcb)
{
  PVOID Buffer;
  PVOID Bitmap;
  PFILE_RECORD_HEADER MftRecord;
  PFILE_RECORD_HEADER file;
  PATTRIBUTE Attribute;
  PATTRIBUTE AttrData;
  PRESIDENT_ATTRIBUTE ResAttr;

  NTSTATUS Status;
  ULONG BytesPerFileRecord;	
  DPRINT1("NtfsOpenMft() called\n");
  ULONG n = 0;

  BytesPerFileRecord = Vcb->NtfsInfo.ClustersPerFileRecord * 
						Vcb->NtfsInfo.BytesPerCluster;
  
  Buffer = ExAllocatePool(NonPagedPool,
			  BytesPerFileRecord);
  if (Buffer == NULL)
    {
      return(STATUS_INSUFFICIENT_RESOURCES);
    }
  
 
  Status = NtfsReadRawSectors(DeviceObject,
			      Vcb->NtfsInfo.MftStart.u.LowPart * Vcb->NtfsInfo.SectorsPerCluster,
			      BytesPerFileRecord / Vcb->NtfsInfo.BytesPerSector,
			      Vcb->NtfsInfo.BytesPerSector,
			      (PVOID)Buffer);


    if (NT_SUCCESS(Status))
    {
      
      DbgPrint("Enumerate  MFT \n");
      MftRecord = Buffer;

      FixupUpdateSequenceArray(MftRecord);
    
	  ULONG i;
	  Attribute = FindAttribute(MftRecord, AttributeBitmap, 0);
      	  	
	
      /* Get number of file records*/
	  n = AttributeLength(FindAttribute(MftRecord, AttributeData,0))
		  / BytesPerFileRecord;
	  	 	  
	  file = ExAllocatePool(NonPagedPool, BytesPerFileRecord);
	 
	  if (file == NULL)
	  {
		  return(STATUS_INSUFFICIENT_RESOURCES);
	  }
     
      /* Enumerate MFT Records */ 
	  
	  for ( i=0; i < n; i++)
	  {
          
		         
		  ReadFileRecord(i, file, Vcb, MftRecord, DeviceObject);
         		 
		  if (file->Ntfs.Type == 'ELIF' && (file->Flags & 1) != 0)
		  {
			 
			  DbgPrint("\n");
			  DbgPrint("File  %lu\n", i);
              DbgPrint("\n");
			  /* Enumerate attributtes */

	          EnumerAttribute(file, Vcb, DeviceObject);
			  DbgPrint("\n");
              DbgPrint("\n");
				 	
		
		  }

	  }


  
	}
    
	ExFreePool(Buffer);
  
	ExFreePool(file);


  return(Status);
}

PATTRIBUTE FindAttribute(PFILE_RECORD_HEADER file,

						 ATTRIBUTE_TYPE type, PWSTR name)
{
	PATTRIBUTE attr = (PATTRIBUTE)((PVOID)file + file->AttributeOffset);
	
	while (attr->AttributeType !=-1)
	{
		
	 if(attr->AttributeType == type) {
	     return attr;
		}
      attr = (PATTRIBUTE)((ULONG)attr + attr->Length);
	}

	return 0;
}


ULONG AttributeLengthAllocated(PATTRIBUTE attr)
{
	
	PRESIDENT_ATTRIBUTE ResAttr;
	PNONRESIDENT_ATTRIBUTE NresAttr;
	 if(attr->Nonresident == FALSE)
	 {
		 ResAttr  = (PRESIDENT_ATTRIBUTE)attr;
		 return ResAttr->ValueLength;
	 }

	else  
	{
		NresAttr = (PNONRESIDENT_ATTRIBUTE)attr;
		
		return NresAttr->AllocatedSize;
	}
	

}

VOID ReadAttribute(PATTRIBUTE attr, PVOID buffer, PDEVICE_EXTENSION Vcb, 
				    PDEVICE_OBJECT DeviceObject)
{
	if(attr->Nonresident == FALSE)
	{
		PRESIDENT_ATTRIBUTE ResAttr = (PRESIDENT_ATTRIBUTE)attr;
		memcpy(buffer, (PRESIDENT_ATTRIBUTE)((PVOID)ResAttr + ResAttr->ValueOffset),
			ResAttr->ValueLength);

        
	}

	
		PNONRESIDENT_ATTRIBUTE NresAttr = (PNONRESIDENT_ATTRIBUTE)attr;
		ReadExternalAttribute(NresAttr, 0, (ULONG)(NresAttr->LastVcn) + 1,
			buffer, Vcb, DeviceObject);
    



}



ULONG AttributeLength(PATTRIBUTE  attr)
{
	PRESIDENT_ATTRIBUTE ResAttr;
	PNONRESIDENT_ATTRIBUTE NresAttr;
	
	if(attr->Nonresident == FALSE)
	{
		ResAttr = (PRESIDENT_ATTRIBUTE)attr;
		return ResAttr->ValueLength;
	}

	else
	{
		NresAttr = (PNONRESIDENT_ATTRIBUTE)attr;
		return NresAttr->DataSize;
	}

}



VOID ReadFileRecord(ULONG index, PFILE_RECORD_HEADER file,
					  PDEVICE_EXTENSION Vcb, PFILE_RECORD_HEADER Mft,
					   PDEVICE_OBJECT DeviceObject)

{
	
	PVOID p;
	ULONG clusters = Vcb->NtfsInfo.ClustersPerFileRecord;
    ULONG BytesPerFileRecord = clusters * Vcb->NtfsInfo.BytesPerCluster;
	

	p =  ExAllocatePool(NonPagedPool, BytesPerFileRecord);

	ULONGLONG vcn = index * BytesPerFileRecord / Vcb->NtfsInfo.BytesPerCluster;

	ReadVCN(Mft, AttributeData, vcn, clusters, p, Vcb,DeviceObject);

	LONG m = (Vcb->NtfsInfo.BytesPerCluster / BytesPerFileRecord) - 1; 
		
	ULONG n = m > 0 ? (index & m) : 0;

	memcpy(file, p + n * BytesPerFileRecord, BytesPerFileRecord);

	ExFreePool(p);
	FixupUpdateSequenceArray(file);


}



VOID ReadExternalAttribute(PNONRESIDENT_ATTRIBUTE NresAttr,
						   ULONGLONG vcn, ULONG count, PVOID buffer,
						                       PDEVICE_EXTENSION Vcb,
											   PDEVICE_OBJECT DeviceObject)
{
	

	ULONGLONG lcn, runcount;
	ULONG readcount, left;

    PUCHAR bytes = (PUCHAR)buffer;

	for (left = count; left>0; left -=readcount) 
	{
		FindRun(NresAttr, vcn, &lcn, &runcount);

		readcount = (ULONG)(__min(runcount, left));

		
		ULONG n = readcount * Vcb->NtfsInfo.BytesPerCluster;
    	
		if (lcn == 0)
			memset(bytes, 0, n);
		else
		  ReadLCN(lcn, readcount, bytes, Vcb, DeviceObject);

		vcn += readcount;
		bytes += n;

	
	}
	

}


BOOL FindRun(PNONRESIDENT_ATTRIBUTE NresAttr, ULONGLONG vcn,
			 PULONGLONG lcn, PULONGLONG count)
{
	PUCHAR run; 

	ULONGLONG base = NresAttr->StartVcn;

 
	if (vcn < NresAttr->StartVcn || vcn > NresAttr->LastVcn)
		return FALSE;

	*lcn = 0;


    for (run = (PUCHAR)((ULONG)NresAttr + NresAttr->RunArrayOffset);
	*run != 0; run += RunLength(run))  {
	
		*lcn += RunLCN(run);
		*count = RunCount(run);
	    	
		if(base <= vcn && vcn < base + *count)
		{
			*lcn = RunLCN(run) == 0 ? 0 : *lcn + vcn - base;
			*count -= (ULONG)(vcn - base);
    
		
			return TRUE;
		}
		else
			base += *count;
		
				
	}
    
	return FALSE;
}



VOID ReadVCN(PFILE_RECORD_HEADER file, ATTRIBUTE_TYPE type,
			 ULONGLONG vcn, ULONG count, PVOID buffer,
			 PDEVICE_EXTENSION Vcb, PDEVICE_OBJECT DeviceObject)
{

	PNONRESIDENT_ATTRIBUTE NresAttr;
	PATTRIBUTE attr;
	attr = FindAttribute(file, type,0);
	
	NresAttr = (PNONRESIDENT_ATTRIBUTE) attr;

	if (NresAttr == 0 || (vcn < NresAttr->StartVcn ||vcn > NresAttr->LastVcn))
	{
		
	  PATTRIBUTE attrList = FindAttribute(file,AttributeAttributeList,0);
	  DbgPrint("Exeption \n");
	  //KeDebugCheck(0);
	}

	ReadExternalAttribute(NresAttr, vcn, count, buffer, Vcb, DeviceObject);


}

BOOL bitset(PUCHAR bitmap, ULONG i)
{
	return (bitmap[i>>3] & (1 << (i & 7))) !=0;
}


VOID FixupUpdateSequenceArray(PFILE_RECORD_HEADER file)
{
	PUSHORT usa = (PUSHORT)((PVOID)file + file->Ntfs.UsaOffset);
	PUSHORT sector = (PUSHORT)file;
    ULONG i;

	for( i =1; i < file->Ntfs.UsaCount; i++)
	{
		sector[255] = usa[i];
		sector += 256;

	}

}


VOID ReadLCN(ULONGLONG lcn, ULONG count, PVOID buffer, PDEVICE_EXTENSION Vcb,
			 PDEVICE_OBJECT DeviceObject)
{
	
	LARGE_INTEGER DiskSector;
	DiskSector.QuadPart = lcn;
	
	
	NtfsReadRawSectors(DeviceObject, DiskSector.u.LowPart * Vcb->NtfsInfo.SectorsPerCluster, 
	 count * Vcb->NtfsInfo.SectorsPerCluster, Vcb->NtfsInfo.BytesPerSector, buffer);
}


VOID EnumerAttribute(PFILE_RECORD_HEADER file, PDEVICE_EXTENSION Vcb,
					 PDEVICE_OBJECT DeviceObject)
						 
{
	
	ULONGLONG lcn;
	ULONGLONG runcount;
	ULONG  size;
	PATTRIBUTE attr = (PATTRIBUTE)((PVOID)file + file->AttributeOffset);
	
	while (attr->AttributeType !=-1)
	{
		
	  if(NtfsDumpAttribute(attr))
	  {

		  PNONRESIDENT_ATTRIBUTE NresAttr = (PNONRESIDENT_ATTRIBUTE)attr;


          FindRun(NresAttr,0,&lcn, &runcount);
		  
		  DbgPrint("  AllocatedSize %I64d  DataSize %I64d\n", NresAttr->AllocatedSize, NresAttr->DataSize);
		  DbgPrint("  logical sectors:  %lu", lcn);
		  DbgPrint("-%lu\n", lcn + runcount -1);  
		  
	  }
          attr = (PATTRIBUTE)((ULONG)attr + attr->Length);
	
	}


}



	/* EOF */
