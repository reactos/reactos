/*
 *  ReactOS kernel
 *  Copyright (C) 2002,2003 ReactOS Team
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
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             drivers/filesystem/ntfs/attrib.c
 * PURPOSE:          NTFS filesystem driver
 * PROGRAMMER:       Eric Kohl
 * Updated	by       Valentin Verkhovsky  2003/09/12
 */

/* INCLUDES *****************************************************************/

#include "ntfs.h"

#define NDEBUG
#include <debug.h>

/* GLOBALS *****************************************************************/


/* FUNCTIONS ****************************************************************/



static ULONG
RunLength(PUCHAR run)
{
  return(*run & 0x0f) + ((*run >> 4) & 0x0f) + 1;
}


static LONGLONG
RunLCN(PUCHAR run)
{
	UCHAR n1 = *run & 0x0f;
	UCHAR n2 = (*run >> 4) & 0x0f;
	LONGLONG lcn = (n2 == 0) ? 0 : (CHAR)(run[n1 + n2]);
	LONG i = 0;

	for (i = n1 +n2 - 1; i > n1; i--)
		lcn = (lcn << 8) + run[i];
	return lcn;
}



static ULONGLONG
RunCount(PUCHAR run)
{
	UCHAR n =  *run & 0xf;
	ULONGLONG count = 0;
	ULONG i = 0;

	for (i = n; i > 0; i--)
		count = (count << 8) + run[i];
	return count;
}


BOOLEAN
FindRun (PNONRESIDENT_ATTRIBUTE NresAttr,
	 ULONGLONG vcn,
	 PULONGLONG lcn,
	 PULONGLONG count)
{
  PUCHAR run;

  ULONGLONG base = NresAttr->StartVcn;

  if (vcn < NresAttr->StartVcn || vcn > NresAttr->LastVcn)
    return FALSE;

  *lcn = 0;

  for (run = (PUCHAR)((ULONG_PTR)NresAttr + NresAttr->RunArrayOffset);
	*run != 0; run += RunLength(run))
    {
      *lcn += RunLCN(run);
      *count = RunCount(run);

      if (base <= vcn && vcn < base + *count)
	{
	  *lcn = (RunLCN(run) == 0) ? 0 : *lcn + vcn - base;
	  *count -= (ULONG)(vcn - base);

	  return TRUE;
	}
      else
	{
	  base += *count;
	}
    }

  return FALSE;
}


static VOID
NtfsDumpFileNameAttribute(PATTRIBUTE Attribute)
{
  PRESIDENT_ATTRIBUTE ResAttr;
  PFILENAME_ATTRIBUTE FileNameAttr;

  DbgPrint("  $FILE_NAME ");

  ResAttr = (PRESIDENT_ATTRIBUTE)Attribute;
//  DbgPrint(" Length %lu  Offset %hu ", ResAttr->ValueLength, ResAttr->ValueOffset);

  FileNameAttr = (PFILENAME_ATTRIBUTE)((ULONG_PTR)ResAttr + ResAttr->ValueOffset);
  DbgPrint(" '%.*S' ", FileNameAttr->NameLength, FileNameAttr->Name);
}


static VOID
NtfsDumpVolumeNameAttribute(PATTRIBUTE Attribute)
{
  PRESIDENT_ATTRIBUTE ResAttr;
  PWCHAR VolumeName;

  DbgPrint("  $VOLUME_NAME ");

  ResAttr = (PRESIDENT_ATTRIBUTE)Attribute;
//  DbgPrint(" Length %lu  Offset %hu ", ResAttr->ValueLength, ResAttr->ValueOffset);

  VolumeName = (PWCHAR)((ULONG_PTR)ResAttr + ResAttr->ValueOffset);
  DbgPrint(" '%.*S' ", ResAttr->ValueLength / sizeof(WCHAR), VolumeName);
}


static VOID
NtfsDumpVolumeInformationAttribute(PATTRIBUTE Attribute)
{
  PRESIDENT_ATTRIBUTE ResAttr;
  PVOLINFO_ATTRIBUTE VolInfoAttr;

  DbgPrint("  $VOLUME_INFORMATION ");

  ResAttr = (PRESIDENT_ATTRIBUTE)Attribute;
//  DbgPrint(" Length %lu  Offset %hu ", ResAttr->ValueLength, ResAttr->ValueOffset);

  VolInfoAttr = (PVOLINFO_ATTRIBUTE)((ULONG_PTR)ResAttr + ResAttr->ValueOffset);
  DbgPrint(" NTFS Version %u.%u  Flags 0x%04hx ",
	   VolInfoAttr->MajorVersion,
	   VolInfoAttr->MinorVersion,
	   VolInfoAttr->Flags);
}


static VOID
NtfsDumpAttribute (PATTRIBUTE Attribute)
{
  PNONRESIDENT_ATTRIBUTE NresAttr;
  UNICODE_STRING Name;

  ULONGLONG lcn = 0;
  ULONGLONG runcount = 0;

  switch (Attribute->AttributeType)
    {
      case AttributeFileName:
	NtfsDumpFileNameAttribute(Attribute);
	break;

      case AttributeStandardInformation:
	DbgPrint("  $STANDARD_INFORMATION ");
	break;

      case AttributeAttributeList:
	DbgPrint("  $ATTRIBUTE_LIST ");
	break;

      case AttributeObjectId:
	DbgPrint("  $OBJECT_ID ");
	break;

      case AttributeSecurityDescriptor:
	DbgPrint("  $SECURITY_DESCRIPTOR ");
	break;

      case AttributeVolumeName:
	NtfsDumpVolumeNameAttribute(Attribute);
	break;

      case AttributeVolumeInformation:
	NtfsDumpVolumeInformationAttribute(Attribute);
	break;

      case AttributeData:
	DbgPrint("  $DATA ");
	//DataBuf = ExAllocatePool(NonPagedPool,AttributeLengthAllocated(Attribute));
	break;

      case AttributeIndexRoot:
	DbgPrint("  $INDEX_ROOT ");
	break;

      case AttributeIndexAllocation:
	DbgPrint("  $INDEX_ALLOCATION ");
	break;

      case AttributeBitmap:
	DbgPrint("  $BITMAP ");
	break;

      case AttributeReparsePoint:
	DbgPrint("  $REPARSE_POINT ");
	break;

      case AttributeEAInformation:
	DbgPrint("  $EA_INFORMATION ");
	break;

      case AttributeEA:
	DbgPrint("  $EA ");
	break;

      case AttributePropertySet:
	DbgPrint("  $PROPERTY_SET ");
	break;

      case AttributeLoggedUtilityStream:
	DbgPrint("  $LOGGED_UTILITY_STREAM ");
	break;

      default:
	DbgPrint("  Attribute %lx ",
		 Attribute->AttributeType);
	break;
    }

  if (Attribute->NameLength != 0)
    {
      Name.Length = Attribute->NameLength * sizeof(WCHAR);
      Name.MaximumLength = Name.Length;
      Name.Buffer = (PWCHAR)((ULONG_PTR)Attribute + Attribute->NameOffset);

      DbgPrint("'%wZ' ", &Name);
    }

  DbgPrint("(%s)\n",
	   Attribute->Nonresident ? "non-resident" : "resident");

  if (Attribute->Nonresident)
    {
      NresAttr = (PNONRESIDENT_ATTRIBUTE)Attribute;

      FindRun (NresAttr,0,&lcn, &runcount);

      DbgPrint ("  AllocatedSize %I64u  DataSize %I64u\n",
		NresAttr->AllocatedSize, NresAttr->DataSize);
      DbgPrint ("  logical clusters: %I64u - %I64u\n",
		lcn, lcn + runcount - 1);
    }
}


VOID
NtfsDumpFileAttributes (PFILE_RECORD_HEADER FileRecord)
{
  PATTRIBUTE Attribute;

  Attribute = (PATTRIBUTE)((ULONG_PTR)FileRecord + FileRecord->AttributeOffset);
  while (Attribute < (PATTRIBUTE)((ULONG_PTR)FileRecord + FileRecord->BytesInUse) &&
         Attribute->AttributeType != (ATTRIBUTE_TYPE)-1)
    {
      NtfsDumpAttribute (Attribute);

      Attribute = (PATTRIBUTE)((ULONG_PTR)Attribute + Attribute->Length);
    }
}

/* EOF */
