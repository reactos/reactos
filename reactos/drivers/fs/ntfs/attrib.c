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
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/* $Id: attrib.c,v 1.2 2003/01/17 18:51:13 ekohl Exp $
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             drivers/fs/ntfs/attrib.c
 * PURPOSE:          NTFS filesystem driver
 * PROGRAMMER:       Eric Kohl
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>

#define NDEBUG
#include <debug.h>

#include "ntfs.h"


/* FUNCTIONS ****************************************************************/

VOID
NtfsDumpFileNameAttribute(PATTRIBUTE Attribute)
{
  PRESIDENT_ATTRIBUTE ResAttr;
  PFILENAME_ATTRIBUTE FileNameAttr;

  DbgPrint("  $FILE_NAME ");

  ResAttr = (PRESIDENT_ATTRIBUTE)Attribute;
//  DbgPrint(" Length %lu  Offset %hu ", ResAttr->ValueLength, ResAttr->ValueOffset);

  FileNameAttr = (PFILENAME_ATTRIBUTE)((PVOID)ResAttr + ResAttr->ValueOffset);
  DbgPrint(" '%.*S' ", FileNameAttr->NameLength, FileNameAttr->Name);
}


VOID
NtfsDumpVolumeNameAttribute(PATTRIBUTE Attribute)
{
  PRESIDENT_ATTRIBUTE ResAttr;
  PWCHAR VolumeName;

  DbgPrint("  $VOLUME_NAME ");

  ResAttr = (PRESIDENT_ATTRIBUTE)Attribute;
//  DbgPrint(" Length %lu  Offset %hu ", ResAttr->ValueLength, ResAttr->ValueOffset);

  VolumeName = (PWCHAR)((PVOID)ResAttr + ResAttr->ValueOffset);
  DbgPrint(" '%.*S' ", ResAttr->ValueLength/2, VolumeName);
}


VOID
NtfsDumpVolumeInformationAttribute(PATTRIBUTE Attribute)
{
  PRESIDENT_ATTRIBUTE ResAttr;
  PVOLINFO_ATTRIBUTE VolInfoAttr;

  DbgPrint("  $VOLUME_INFORMATION ");

  ResAttr = (PRESIDENT_ATTRIBUTE)Attribute;
//  DbgPrint(" Length %lu  Offset %hu ", ResAttr->ValueLength, ResAttr->ValueOffset);

  VolInfoAttr = (PVOLINFO_ATTRIBUTE)((PVOID)ResAttr + ResAttr->ValueOffset);
  DbgPrint(" NTFS Version %u.%u  Flags 0x%04hx ",
	   VolInfoAttr->MajorVersion,
	   VolInfoAttr->MinorVersion,
	   VolInfoAttr->Flags);
}


VOID
NtfsDumpAttribute(PATTRIBUTE Attribute)
{
  PNONRESIDENT_ATTRIBUTE NresAttr;
  PRESIDENT_ATTRIBUTE ResAttr;
  UNICODE_STRING Name;
  PUCHAR Ptr;
  UCHAR RunHeader;
  ULONG RunLength;
  ULONG RunStart;

  switch (Attribute->AttributeType)
    {
      case AttributeStandardInformation:
	DbgPrint("  $STANDARD_INFORMATION ");
	break;

      case AttributeAttributeList:
	DbgPrint("  $ATTRIBUTE_LIST ");
	break;

      case AttributeFileName:
	NtfsDumpFileNameAttribute(Attribute);
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
      Name.Buffer = (PWCHAR)((ULONG)Attribute + Attribute->NameOffset);

      DbgPrint("'%wZ' ", &Name);
    }

  DbgPrint("(%s)\n",
	   Attribute->Nonresident ? "non-resident" : "resident");

  if (Attribute->Nonresident != 0)
    {
      NresAttr = (PNONRESIDENT_ATTRIBUTE)Attribute;
      Ptr = (PUCHAR)((ULONG)NresAttr + NresAttr->RunArrayOffset);
      while (*Ptr != 0)
	{
	  RunHeader = *Ptr++;

	  switch (RunHeader & 0x0F)
	    {
	      case 1:
		RunLength = (ULONG)*Ptr++;
		break;

	      case 2:
		RunLength = *((PUSHORT)Ptr);
		Ptr += 2;
		break;

	      case 3:
		RunLength = *Ptr++;
		RunLength += *Ptr++ << 8;
		RunLength += *Ptr++ << 16;
		break;

	      case 4:
		RunLength = *((PULONG)Ptr);
		Ptr += 4;
		break;

	      default:
		DbgPrint("RunLength size of %hu not implemented!\n", RunHeader & 0x0F);
		KeBugCheck(0);
	    }

	  switch (RunHeader >> 4)
	    {
	      case 1:
		RunStart = (ULONG)*Ptr;
		Ptr++;
		break;

	      case 2:
		RunStart = *((PUSHORT)Ptr);
		Ptr += 2;
		break;

	      case 3:
		RunStart = *Ptr++;
		RunStart += *Ptr++ << 8;
		RunStart += *Ptr++ << 16;
		break;

	      case 4:
		RunStart = *((PULONG)Ptr);
		Ptr += 4;
		break;

	      default:
		DbgPrint("RunStart size of %hu not implemented!\n", RunHeader >> 4);
		KeBugCheck(0);
	    }

	  DbgPrint("    AllocatedSize %I64d  DataSize %I64d\n", NresAttr->AllocatedSize, NresAttr->DataSize);
//	  DbgPrint("    Run: Header %hx  Start %lu  Length %lu\n", RunHeader, RunStart, RunLength);
	  if (RunLength == 1)
	    {
	      DbgPrint("    logical sector %lu (0x%lx)\n", RunStart, RunStart);
	    }
	  else
	    {
	      DbgPrint("    logical sectors %lu-%lu (0x%lx-0x%lx)\n",
		       RunStart, RunStart + RunLength - 1,
		       RunStart, RunStart + RunLength - 1);
	    }
	}
    }


}


/* EOF */
