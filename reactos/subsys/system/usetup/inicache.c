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
/* $Id: inicache.c,v 1.1 2002/11/13 18:25:18 ekohl Exp $
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS text-mode setup
 * FILE:            subsys/system/usetup/inicache.c
 * PURPOSE:         INI file parser that caches contents of INI file in memory
 * PROGRAMMER:      Royce Mitchell III
 *                  Eric Kohl
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include "usetup.h"
#include "inicache.h"


/* PRIVATE FUNCTIONS ********************************************************/

static PINICACHEKEY
IniCacheFreeKey(PINICACHEKEY Key)
{
  PINICACHEKEY Next;

  if (Key == NULL)
    {
      return(NULL);
    }

  Next = Key->Next;
  if (Key->Name != NULL)
    {
      RtlFreeHeap(ProcessHeap,
		  0,
		  Key->Name);
      Key->Name = NULL;
    }

  if (Key->Value != NULL)
    {
      RtlFreeHeap(ProcessHeap,
		  0,
		  Key->Value);
      Key->Value = NULL;
    }

  RtlFreeHeap(ProcessHeap,
	      0,
	      Key);

  return(Next);
}


static PINICACHESECTION
IniCacheFreeSection(PINICACHESECTION Section)
{
  PINICACHESECTION Next;

  if (Section == NULL)
    {
      return(NULL);
    }

  Next = Section->Next;
  while (Section->FirstKey != NULL)
    {
      Section->FirstKey = IniCacheFreeKey(Section->FirstKey);
    }
  Section->LastKey = NULL;

  if (Section->Name != NULL)
    {
      RtlFreeHeap(ProcessHeap,
		  0,
		  Section->Name);
      Section->Name = NULL;
    }

  RtlFreeHeap(ProcessHeap,
	      0,
	      Section);

  return(Next);
}


static PINICACHEKEY
IniCacheFindKey(PINICACHESECTION Section,
		PWCHAR Name,
		ULONG NameLength)
{
  PINICACHEKEY Key;

  Key = Section->FirstKey;
  while (Key != NULL)
    {
      if (NameLength == wcslen(Key->Name))
	{
	  if (_wcsnicmp(Key->Name, Name, NameLength) == 0)
	    break;
	}

      Key = Key->Next;
    }

  return(Key);
}


static PINICACHEKEY
IniCacheAddKey(PINICACHESECTION Section,
	       PCHAR Name,
	       ULONG NameLength,
	       PCHAR Value,
	       ULONG ValueLength)
{
  PINICACHEKEY Key;
  ULONG i;

  Key = NULL;

  if (Section == NULL ||
      Name == NULL ||
      NameLength == 0 ||
      Value == NULL ||
      ValueLength == 0)
    {
      DPRINT("Invalid parameter\n");
      return(NULL);
    }

  Key = (PINICACHEKEY)RtlAllocateHeap(ProcessHeap,
				      0,
				      sizeof(INICACHEKEY));
  if (Key == NULL)
    {
      DPRINT("RtlAllocateHeap() failed\n");
      return(NULL);
    }

  RtlZeroMemory(Key,
		sizeof(INICACHEKEY));


  Key->Name = RtlAllocateHeap(ProcessHeap,
			      0,
			      (NameLength + 1) * sizeof(WCHAR));
  if (Key->Name == NULL)
    {
      DPRINT("RtlAllocateHeap() failed\n");
      RtlFreeHeap(ProcessHeap,
		  0,
		  Key);
      return(NULL);
    }

  /* Copy value name */
  for (i = 0; i < NameLength; i++)
    {
      Key->Name[i] = (WCHAR)Name[i];
    }
  Key->Name[NameLength] = 0;


  Key->Value = RtlAllocateHeap(ProcessHeap,
			       0,
			       (ValueLength + 1) * sizeof(WCHAR));
  if (Key->Value == NULL)
    {
      DPRINT("RtlAllocateHeap() failed\n");
      RtlFreeHeap(ProcessHeap,
		  0,
		  Key->Name);
      RtlFreeHeap(ProcessHeap,
		  0,
		  Key);
      return(NULL);
    }

  /* Copy value data */
  for (i = 0; i < ValueLength; i++)
    {
      Key->Value[i] = (WCHAR)Value[i];
    }
  Key->Value[ValueLength] = 0;


  if (Section->FirstKey == NULL)
    {
      Section->FirstKey = Key;
      Section->LastKey = Key;
    }
  else
    {
      Section->LastKey->Next = Key;
      Key->Prev = Section->LastKey;
      Section->LastKey = Key;
    }

  return(Key);
}


static PINICACHESECTION
IniCacheFindSection(PINICACHE Cache,
		    PWCHAR Name,
		    ULONG NameLength)
{
  PINICACHESECTION Section = NULL;

  if (Cache == NULL || Name == NULL || NameLength == 0)
    {
      return(NULL);
    }

  Section = Cache->FirstSection;

  /* iterate through list of sections */
  while (Section != NULL)
    {
      if (NameLength == wcslen(Section->Name))
	{
	  /* are the contents the same too? */
	  if (_wcsnicmp(Section->Name, Name, NameLength) == 0)
	    break;
	}

      /* get the next section*/
      Section = Section->Next;
    }

  return(Section);
}


static PINICACHESECTION
IniCacheAddSection(PINICACHE Cache,
		   PCHAR Name,
		   ULONG NameLength)
{
  PINICACHESECTION Section = NULL;
  ULONG i;

  if (Cache == NULL || Name == NULL || NameLength == 0)
    {
      DPRINT("Invalid parameter\n");
      return(NULL);
    }

  Section = (PINICACHESECTION)RtlAllocateHeap(ProcessHeap,
					      0,
					      sizeof(INICACHESECTION));
  if (Section == NULL)
    {
      DPRINT("RtlAllocateHeap() failed\n");
      return(NULL);
    }
  RtlZeroMemory(Section,
		sizeof(INICACHESECTION));

  /* Allocate and initialize section name */
  Section->Name = RtlAllocateHeap(ProcessHeap,
				  0,
				  (NameLength + 1) * sizeof(WCHAR));
  if (Section->Name == NULL)
    {
      DPRINT("RtlAllocateHeap() failed\n");
      RtlFreeHeap(ProcessHeap,
		  0,
		  Section);
      return(NULL);
    }

  /* Copy section name */
  for (i = 0; i < NameLength; i++)
    {
      Section->Name[i] = (WCHAR)Name[i];
    }
  Section->Name[NameLength] = 0;

  /* Append section */
  if (Cache->FirstSection == NULL)
    {
      Cache->FirstSection = Section;
      Cache->LastSection = Section;
    }
  else
    {
      Cache->LastSection->Next = Section;
      Section->Prev = Cache->LastSection;
      Cache->LastSection = Section;
    }

  return(Section);
}


static PCHAR
IniCacheSkipWhitespace(PCHAR Ptr)
{
  while (*Ptr != 0 && isspace(*Ptr))
    Ptr++;

  return((*Ptr == 0) ? NULL : Ptr);
}


static PCHAR
IniCacheSkipToNextSection(PCHAR Ptr)
{
  while (*Ptr != 0 && *Ptr != '[')
    {
      while (*Ptr != 0 && *Ptr != L'\n')
	{
	  Ptr++;
	}
      Ptr++;
    }

  return((*Ptr == 0) ? NULL : Ptr);
}


static PCHAR
IniCacheGetSectionName(PCHAR Ptr,
		       PCHAR *NamePtr,
		       PULONG NameSize)
{
  ULONG Size = 0;
  CHAR Name[256];

  *NamePtr = NULL;
  *NameSize = 0;

  /* skip whitespace */
  while (*Ptr != 0 && isspace(*Ptr))
    {
      Ptr++;
    }

  *NamePtr = Ptr;

  while (*Ptr != 0 && *Ptr != ']')
    {
      Size++;
      Ptr++;
    }

  Ptr++;

  while (*Ptr != 0 && *Ptr != L'\n')
    {
      Ptr++;
    }
  Ptr++;

  *NameSize = Size;

  strncpy(Name, *NamePtr, Size);
  Name[Size] = 0;

  DPRINT("SectionName: '%s'\n", Name);

  return(Ptr);
}


static PCHAR
IniCacheGetKeyName(PCHAR Ptr,
		   PCHAR *NamePtr,
		   PULONG NameSize)
{
  ULONG Size = 0;

  *NamePtr = NULL;
  *NameSize = 0;

  /* skip whitespace */
  while (*Ptr != 0 && isspace(*Ptr))
    {
      Ptr++;
    }

  *NamePtr = Ptr;

  while (*Ptr != 0 && !isspace(*Ptr) && *Ptr != '=')
    {
      Size++;
      Ptr++;
    }

  *NameSize = Size;

  return(Ptr);
}


static PCHAR
IniCacheGetKeyValue(PCHAR Ptr,
		    PCHAR *DataPtr,
		    PULONG DataSize)
{
  ULONG Size = 0;

  *DataPtr = NULL;
  *DataSize = 0;

  /* skip whitespace */
  while (*Ptr != 0 && isspace(*Ptr))
    {
      Ptr++;
    }

  /* check and skip '=' */
  if (*Ptr != '=')
    {
      return(NULL);
    }
  Ptr++;

  /* skip whitespace */
  while (*Ptr != 0 && isspace(*Ptr))
    {
      Ptr++;
    }

  /* check for quoted data */
  if (*Ptr == '\"')
    {
      Ptr++;
      *DataPtr = Ptr;

      while (*Ptr != 0 && *Ptr != '\"')
	{
	  Ptr++;
	  Size++;
	}
      Ptr++;
    }
  else
    {
      *DataPtr = Ptr;

      while (*Ptr != 0 && !isspace(*Ptr) && *Ptr != '\n')
	{
	  Ptr++;
	  Size++;
	}
    }

  /* Skip to next line */
  while (*Ptr != 0 && *Ptr != '\n')
    {
      Ptr++;
    }
  Ptr++;

  *DataSize = Size;

  return(Ptr);
}




/* PUBLIC FUNCTIONS *********************************************************/

NTSTATUS
IniCacheLoad(PINICACHE *Cache,
	     PUNICODE_STRING FileName)
{
  OBJECT_ATTRIBUTES ObjectAttributes;
  FILE_STANDARD_INFORMATION FileInfo;
  IO_STATUS_BLOCK IoStatusBlock;
  HANDLE FileHandle;
  NTSTATUS Status;
  PCHAR FileBuffer;
  ULONG FileLength;
  PCHAR Ptr;
  LARGE_INTEGER FileOffset;

  ULONG i;
  PINICACHESECTION Section;
  PINICACHEKEY Key;

  PCHAR SectionName;
  ULONG SectionNameSize;

  PCHAR KeyName;
  ULONG KeyNameSize;

  PCHAR KeyValue;
  ULONG KeyValueSize;

  *Cache = NULL;

  /* Open ini file */
  InitializeObjectAttributes(&ObjectAttributes,
			     FileName,
			     0,
			     NULL,
			     NULL);

  Status = NtOpenFile(&FileHandle,
		      GENERIC_READ | SYNCHRONIZE,
		      &ObjectAttributes,
		      &IoStatusBlock,
		      FILE_SHARE_READ,
		      FILE_NON_DIRECTORY_FILE);
  if (!NT_SUCCESS(Status))
    {
      DPRINT("NtOpenFile() failed (Status %lx)\n", Status);
      return(Status);
    }

  DPRINT("NtOpenFile() successful\n");

  /* Query file size */
  Status = NtQueryInformationFile(FileHandle,
				  &IoStatusBlock,
				  &FileInfo,
				  sizeof(FILE_STANDARD_INFORMATION),
				  FileStandardInformation);
  if (Status == STATUS_PENDING)
    {
      DPRINT("NtQueryInformationFile() returns STATUS_PENDING\n");

    }
  else if (!NT_SUCCESS(Status))
    {
      DPRINT("NtQueryInformationFile() failed (Status %lx)\n", Status);
      NtClose(FileHandle);
      return(Status);
    }

  FileLength = FileInfo.EndOfFile.u.LowPart;

  DPRINT("File size: %lu\n", FileLength);

  /* Allocate file buffer */
  FileBuffer = RtlAllocateHeap(ProcessHeap,
			       0,
			       FileLength + 1);
  if (FileBuffer == NULL)
    {
      DPRINT1("RtlAllocateHeap() failed\n");
      NtClose(FileHandle);
      return(STATUS_INSUFFICIENT_RESOURCES);
    }

  /* Read file */
  FileOffset.QuadPart = 0ULL;
  Status = NtReadFile(FileHandle,
		      NULL,
		      NULL,
		      NULL,
		      &IoStatusBlock,
		      FileBuffer,
		      FileLength,
		      &FileOffset,
		      NULL);

  if (Status == STATUS_PENDING)
    {
      DPRINT("NtReadFile() returns STATUS_PENDING\n");

      Status = IoStatusBlock.Status;
    }

  /* Append string terminator */
  FileBuffer[FileLength] = 0;

  NtClose(FileHandle);

  if (!NT_SUCCESS(Status))
    {
      DPRINT("NtReadFile() failed (Status %lx)\n", Status);
      RtlFreeHeap(ProcessHeap,
		  0,
		  FileBuffer);
      return(Status);
    }


  /* Allocate inicache header */
  *Cache = (PINICACHE)RtlAllocateHeap(ProcessHeap,
				      0,
				      sizeof(INICACHE));
  if (*Cache == NULL)
    {
      DPRINT("RtlAllocateHeap() failed\n");
      return(STATUS_INSUFFICIENT_RESOURCES);
    }

  /* Initialize inicache header */
  RtlZeroMemory(*Cache,
		sizeof(INICACHE));

  /* Parse ini file */
  Section = NULL;
  Ptr = FileBuffer;
  while (Ptr != NULL && *Ptr != 0)
    {
      Ptr = IniCacheSkipWhitespace(Ptr);
      if (Ptr == NULL)
	continue;

      if (*Ptr == '[')
	{
	  Section = NULL;
	  Ptr++;

	  Ptr = IniCacheGetSectionName(Ptr,
				       &SectionName,
				       &SectionNameSize);

	  Section = IniCacheAddSection(*Cache,
				       SectionName,
				       SectionNameSize);
	  if (Section == NULL)
	    {
	      DPRINT("IniCacheAddSection() failed\n");
	      Ptr = IniCacheSkipToNextSection(Ptr);
	      continue;
	    }
	}
      else
	{
	  if (Section == NULL)
	    {
	      Ptr = IniCacheSkipToNextSection(Ptr);
	      continue;
	    }

	  Ptr = IniCacheGetKeyName(Ptr,
				   &KeyName,
				   &KeyNameSize);

	  Ptr = IniCacheGetKeyValue(Ptr,
				    &KeyValue,
				    &KeyValueSize);

	  Key = IniCacheAddKey(Section,
			       KeyName,
			       KeyNameSize,
			       KeyValue,
			       KeyValueSize);
	  if (Key == NULL)
	    {
	      DPRINT("IniCacheAddKey() failed\n");
	    }
	}
    }

  /* Free file buffer */
  RtlFreeHeap(ProcessHeap,
	      0,
	      FileBuffer);

  return(Status);
}


VOID
IniCacheDestroy(PINICACHE Cache)
{
  if (Cache == NULL)
    {
      return;
    }

  while (Cache->FirstSection != NULL)
    {
      Cache->FirstSection = IniCacheFreeSection(Cache->FirstSection);
    }
  Cache->LastSection = NULL;

  RtlFreeHeap(ProcessHeap,
	      0,
	      Cache);
}


PINICACHESECTION
IniCacheGetSection(PINICACHE Cache,
		   PWCHAR Name)
{
  PINICACHESECTION Section = NULL;

  if (Cache == NULL || Name == NULL)
    {
      DPRINT("Invalid parameter\n");
      return(NULL);
    }

  /* iterate through list of sections */
  Section = Cache->FirstSection;
  while (Section != NULL)
    {
      DPRINT("Comparing '%S' and '%S'\n", Section->Name, Name);

      /* Are the section names the same? */
      if (_wcsicmp(Section->Name, Name) == 0)
	return(Section);

      /* Get the next section */
      Section = Section->Next;
    }

  DPRINT("Section not found\n");

  return(NULL);
}


NTSTATUS
IniCacheGetKey(PINICACHESECTION Section,
	       PWCHAR KeyName,
	       PWCHAR *KeyValue)
{
  PINICACHEKEY Key;

  if (Section == NULL || KeyName == NULL || KeyValue == NULL)
    {
      DPRINT("Invalid parameter\n");
      return(STATUS_INVALID_PARAMETER);
    }

  *KeyValue = NULL;

  Key = IniCacheFindKey(Section, KeyName, wcslen(KeyName));
  if (Key == NULL)
    {
      return(STATUS_INVALID_PARAMETER);
    }

  *KeyValue = Key->Value;

  return(STATUS_SUCCESS);
}


PINICACHEITERATOR
IniCacheFindFirstValue(PINICACHESECTION Section,
		       PWCHAR *KeyName,
		       PWCHAR *KeyValue)
{
  PINICACHEITERATOR Iterator;
  PINICACHEKEY Key;

  if (Section == NULL || KeyName == NULL || KeyValue == NULL)
    {
      DPRINT("Invalid parameter\n");
      return(NULL);
    }

  Key = Section->FirstKey;
  if (Key == NULL)
    {
      DPRINT("Invalid parameter\n");
      return(NULL);
    }

  *KeyName = Key->Name;
  *KeyValue = Key->Value;

  Iterator = (PINICACHEITERATOR)RtlAllocateHeap(ProcessHeap,
						0,
						sizeof(INICACHEITERATOR));
  if (Iterator == NULL)
    {
      DPRINT("RtlAllocateHeap() failed\n");
      return(NULL);
    }

  Iterator->Section = Section;
  Iterator->Key = Key;

  return(Iterator);
}


BOOLEAN
IniCacheFindNextValue(PINICACHEITERATOR Iterator,
		      PWCHAR *KeyName,
		      PWCHAR *KeyValue)
{
  PINICACHEKEY Key;

  if (Iterator == NULL || KeyName == NULL || KeyValue == NULL)
    {
      DPRINT("Invalid parameter\n");
      return(FALSE);
    }

  Key = Iterator->Key->Next;
  if (Key == NULL)
    {
      DPRINT("No more entries\n");
      return(FALSE);
    }

  *KeyName = Key->Name;
  *KeyValue = Key->Value;

  Iterator->Key = Key;

  return(TRUE);
}


VOID
IniCacheFindClose(PINICACHEITERATOR Iterator)
{
  if (Iterator == NULL)
    return;

  RtlFreeHeap(ProcessHeap,
	      0,
	      Iterator);
}

/* EOF */
