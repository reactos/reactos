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
/* $Id: infcache.c,v 1.1 2003/03/13 09:51:11 ekohl Exp $
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS text-mode setup
 * FILE:            subsys/system/usetup/infcache.c
 * PURPOSE:         INF file parser that caches contents of INF file in memory
 * PROGRAMMER:      Royce Mitchell III
 *                  Eric Kohl
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include "usetup.h"
#include "infcache.h"



typedef struct _INFCACHEKEY
{
  PWCHAR Name;
  PWCHAR Data;

  struct _INFCACHEKEY *Next;
  struct _INFCACHEKEY *Prev;
} INFCACHEKEY, *PINFCACHEKEY;


typedef struct _INFCACHESECTION
{
  struct _INFCACHESECTION *Next;
  struct _INFCACHESECTION *Prev;

  PINFCACHEKEY FirstKey;
  PINFCACHEKEY LastKey;

  LONG LineCount;

  WCHAR Name[1];
} INFCACHESECTION, *PINFCACHESECTION;


typedef struct _INFCACHE
{
  PINFCACHESECTION FirstSection;
  PINFCACHESECTION LastSection;
} INFCACHE, *PINFCACHE;


/* PRIVATE FUNCTIONS ********************************************************/

static PINFCACHEKEY
InfpCacheFreeKey(PINFCACHEKEY Key)
{
  PINFCACHEKEY Next;

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

  if (Key->Data != NULL)
    {
      RtlFreeHeap(ProcessHeap,
		  0,
		  Key->Data);
      Key->Data = NULL;
    }

  RtlFreeHeap(ProcessHeap,
	      0,
	      Key);

  return(Next);
}


static PINFCACHESECTION
InfpCacheFreeSection(PINFCACHESECTION Section)
{
  PINFCACHESECTION Next;

  if (Section == NULL)
    {
      return(NULL);
    }

  /* Release all keys */
  Next = Section->Next;
  while (Section->FirstKey != NULL)
    {
      Section->FirstKey = InfpCacheFreeKey(Section->FirstKey);
    }
  Section->LastKey = NULL;

  RtlFreeHeap(ProcessHeap,
	      0,
	      Section);

  return(Next);
}


static PINFCACHEKEY
InfpCacheFindKey(PINFCACHESECTION Section,
		PWCHAR Name,
		ULONG NameLength)
{
  PINFCACHEKEY Key;

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


static PINFCACHEKEY
InfpCacheAddKey(PINFCACHESECTION Section,
	       PCHAR Name,
	       ULONG NameLength,
	       PCHAR Data,
	       ULONG DataLength)
{
  PINFCACHEKEY Key;
  ULONG i;

  Key = NULL;

  if (Section == NULL ||
      Name == NULL ||
      NameLength == 0 ||
      Data == NULL ||
      DataLength == 0)
    {
      DPRINT("Invalid parameter\n");
      return(NULL);
    }

  Key = (PINFCACHEKEY)RtlAllocateHeap(ProcessHeap,
				      0,
				      sizeof(INFCACHEKEY));
  if (Key == NULL)
    {
      DPRINT("RtlAllocateHeap() failed\n");
      return(NULL);
    }

  RtlZeroMemory(Key,
		sizeof(INFCACHEKEY));

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

  Key->Data = RtlAllocateHeap(ProcessHeap,
			      0,
			      (DataLength + 1) * sizeof(WCHAR));
  if (Key->Data == NULL)
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
  for (i = 0; i < DataLength; i++)
    {
      Key->Data[i] = (WCHAR)Data[i];
    }
  Key->Data[DataLength] = 0;

  /* Append key */
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
  Section->LineCount++;

  return(Key);
}


static PINFCACHESECTION
InfpCacheFindSection(PINFCACHE Cache,
		    PWCHAR Name,
		    ULONG NameLength)
{
  PINFCACHESECTION Section = NULL;

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


static PINFCACHESECTION
InfpCacheAddSection(PINFCACHE Cache,
		   PCHAR Name,
		   ULONG NameLength)
{
  PINFCACHESECTION Section = NULL;
  ULONG i;

  if (Cache == NULL || Name == NULL || NameLength == 0)
    {
      DPRINT("Invalid parameter\n");
      return(NULL);
    }

  /* Allocate and initialize the new section */
  i = sizeof(INFCACHESECTION) + (NameLength * sizeof(WCHAR));
  Section = (PINFCACHESECTION)RtlAllocateHeap(ProcessHeap,
					      0,
					      i);
  if (Section == NULL)
    {
      DPRINT("RtlAllocateHeap() failed\n");
      return(NULL);
    }
  RtlZeroMemory(Section,
		i);

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
InfpCacheSkipWhitespace(PCHAR Ptr)
{
  while (*Ptr != 0 && isspace(*Ptr))
    Ptr++;

  return((*Ptr == 0) ? NULL : Ptr);
}


static PCHAR
InfpCacheSkipToNextSection(PCHAR Ptr)
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
InfpCacheGetSectionName(PCHAR Ptr,
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
InfpCacheGetKeyName(PCHAR Ptr,
		   PCHAR *NamePtr,
		   PULONG NameSize)
{
  ULONG Size = 0;

  *NamePtr = NULL;
  *NameSize = 0;

  while(Ptr && *Ptr)
    {
      *NamePtr = NULL;
      *NameSize = 0;
      Size = 0;

      /* skip whitespace and empty lines */
      while (isspace(*Ptr) || *Ptr == '\n' || *Ptr == '\r')
	{
	  Ptr++;
	}
      if (*Ptr == 0)
	{
	  continue;
	}

      *NamePtr = Ptr;

      while (*Ptr != 0 && !isspace(*Ptr) && *Ptr != '=' && *Ptr != ';')
	{
	  Size++;
	  Ptr++;
	}
      if (*Ptr == ';')
	{
	  while (*Ptr != 0 && *Ptr != '\r' && *Ptr != '\n')
	    {
	      Ptr++;
	    }
	}
      else
	{
	  *NameSize = Size;
	  break;
	}
    }

  return(Ptr);
}


static PCHAR
InfpCacheGetKeyValue(PCHAR Ptr,
		    PCHAR *DataPtr,
		    PULONG DataSize)
{
  ULONG Size = 0;

  *DataPtr = NULL;
  *DataSize = 0;

  /* Skip whitespace */
  while (*Ptr != 0 && isspace(*Ptr))
    {
      Ptr++;
    }

  /* Check and skip '=' */
  if (*Ptr != '=')
    {
      return(NULL);
    }
  Ptr++;

  /* Skip whitespace */
  while (*Ptr != 0 && isspace(*Ptr))
    {
      Ptr++;
    }

#if 0
  if (*Ptr == '"' && String)
    {
      Ptr++;

      /* Get data */
      *DataPtr = Ptr;
      while (*Ptr != '"')
	{
	  Ptr++;
	  Size++;
	}
      Ptr++;
      while (*Ptr && *Ptr != '\r' && *Ptr != '\n')
	{
	  Ptr++;
	}
    }
  else
    {
#endif
      /* Get data */
      *DataPtr = Ptr;
      while (*Ptr != 0 && *Ptr != '\r' && *Ptr != ';')
	{
	  Ptr++;
	  Size++;
	}
#if 0
    }
#endif

  /* Skip to next line */
  if (*Ptr == '\r')
    Ptr++;
  if (*Ptr == '\n')
    Ptr++;

  *DataSize = Size;

  return(Ptr);
}


NTSTATUS
InfpParse (PINFCACHE Cache,
	   PCHAR Start,
	   PCHAR End,
	   PULONG ErrorLine)
{
  PINFCACHESECTION Section;
  PINFCACHEKEY Key;
  PCHAR SectionName;
  ULONG SectionNameSize;
  PCHAR KeyName;
  ULONG KeyNameSize;
  PCHAR KeyValue;
  ULONG KeyValueSize;
  PCHAR Ptr;

  Section = NULL;
  Ptr = Start;
  while (Ptr != NULL && *Ptr != 0)
    {
      Ptr = InfpCacheSkipWhitespace(Ptr);
      if (Ptr == NULL)
	continue;

      if (*Ptr == '[')
	{
	  Section = NULL;
	  Ptr++;

	  Ptr = InfpCacheGetSectionName(Ptr,
				       &SectionName,
				       &SectionNameSize);

	  DPRINT1("[%.*s]\n", SectionNameSize, SectionName);

	  Section = InfpCacheAddSection(Cache,
				       SectionName,
				       SectionNameSize);
	  if (Section == NULL)
	    {
	      DPRINT("IniCacheAddSection() failed\n");
	      Ptr = InfpCacheSkipToNextSection(Ptr);
	      continue;
	    }
	}
      else
	{
	  if (Section == NULL)
	    {
	      Ptr = InfpCacheSkipToNextSection(Ptr);
	      continue;
	    }

	  Ptr = InfpCacheGetKeyName(Ptr,
				   &KeyName,
				   &KeyNameSize);

	  Ptr = InfpCacheGetKeyValue(Ptr,
				    &KeyValue,
				    &KeyValueSize);

	  DPRINT1("'%.*s' = '%.*s'\n", KeyNameSize, KeyName, KeyValueSize, KeyValue);

	  Key = InfpCacheAddKey(Section,
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

  return STATUS_SUCCESS;
}



/* PUBLIC FUNCTIONS *********************************************************/

NTSTATUS
InfOpenFile(PHINF InfHandle,
	    PUNICODE_STRING FileName,
	    PULONG ErrorLine)
{
  OBJECT_ATTRIBUTES ObjectAttributes;
  FILE_STANDARD_INFORMATION FileInfo;
  IO_STATUS_BLOCK IoStatusBlock;
  HANDLE FileHandle;
  NTSTATUS Status;
  PCHAR FileBuffer;
  ULONG FileLength;
  LARGE_INTEGER FileOffset;
  PINFCACHE Cache;


  *InfHandle = NULL;
  *ErrorLine = (ULONG)-1;

  /* Open the inf file */
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

  /* Allocate infcache header */
  Cache = (PINFCACHE)RtlAllocateHeap(ProcessHeap,
				      0,
				      sizeof(INFCACHE));
  if (Cache == NULL)
    {
      DPRINT("RtlAllocateHeap() failed\n");
      RtlFreeHeap(ProcessHeap,
		  0,
		  FileBuffer);
      return(STATUS_INSUFFICIENT_RESOURCES);
    }

  /* Initialize inicache header */
  RtlZeroMemory(Cache,
		sizeof(INFCACHE));

  /* Parse the inf buffer */
  Status = InfpParse (Cache,
		      FileBuffer,
		      FileBuffer + FileLength,
		      ErrorLine);
  if (!NT_SUCCESS(Status))
    {
      RtlFreeHeap(ProcessHeap,
		  0,
		  Cache);
      Cache = NULL;
    }

  /* Free file buffer */
  RtlFreeHeap(ProcessHeap,
	      0,
	      FileBuffer);

  *InfHandle = (HINF)Cache;

  return(Status);
}


VOID
InfCloseFile(HINF InfHandle)
{
  PINFCACHE Cache;

  Cache = (PINFCACHE)InfHandle;

  if (Cache == NULL)
    {
      return;
    }

  while (Cache->FirstSection != NULL)
    {
      Cache->FirstSection = InfpCacheFreeSection(Cache->FirstSection);
    }
  Cache->LastSection = NULL;

  RtlFreeHeap(ProcessHeap,
	      0,
	      Cache);
}


BOOLEAN
InfFindFirstLine (HINF InfHandle,
		  PCWSTR Section,
		  PCWSTR Key,
		  PINFCONTEXT Context)
{
  PINFCACHE Cache;
  PINFCACHESECTION CacheSection;
  PINFCACHEKEY CacheKey;

  if (InfHandle == NULL || Section == NULL || Context == NULL)
    {
      DPRINT("Invalid parameter\n");
      return FALSE;
    }

  Cache = (PINFCACHE)InfHandle;

  /* Iterate through list of sections */
  CacheSection = Cache->FirstSection;
  while (Section != NULL)
    {
      DPRINT("Comparing '%S' and '%S'\n", CacheSection->Name, Section);

      /* Are the section names the same? */
      if (_wcsicmp(CacheSection->Name, Section) == 0)
	{
	  if (Key != NULL)
	    {
	      CacheKey = InfpCacheFindKey(CacheSection, (PWCHAR)Key, wcslen(Key));
	    }
	  else
	    {
	      CacheKey = CacheSection->FirstKey;
	    }

	  if (CacheKey == NULL)
	    return FALSE;

	  Context->Inf = (PVOID)Cache;
	  Context->Section = (PVOID)CacheSection;
	  Context->Line = (PVOID)CacheKey;

	  return TRUE;
	}

      /* Get the next section */
      CacheSection = CacheSection->Next;
    }

  DPRINT("Section not found\n");

  return FALSE;
}


BOOLEAN
InfFindNextLine (PINFCONTEXT ContextIn,
		 PINFCONTEXT ContextOut)
{
  PINFCACHEKEY CacheKey;

  if (ContextIn == NULL || ContextOut == NULL)
    return FALSE;

  if (ContextIn->Line == NULL)
    return FALSE;

  CacheKey = (PINFCACHEKEY)ContextIn->Line;
  if (CacheKey->Next == NULL)
    return FALSE;

  ContextOut->Line = (PVOID)(CacheKey->Next);

  return TRUE;
}


LONG
InfGetLineCount(HINF InfHandle,
		PCWSTR Section)
{
  PINFCACHE Cache;
  PINFCACHESECTION CacheSection;

  if (InfHandle == NULL || Section == NULL)
    {
      DPRINT("Invalid parameter\n");
      return -1;
    }

  Cache = (PINFCACHE)InfHandle;

  /* Iterate through list of sections */
  CacheSection = Cache->FirstSection;
  while (Section != NULL)
    {
      DPRINT("Comparing '%S' and '%S'\n", CacheSection->Name, Section);

      /* Are the section names the same? */
      if (_wcsicmp(CacheSection->Name, Section) == 0)
	{
	  return CacheSection->LineCount;
	}

      /* Get the next section */
      CacheSection = CacheSection->Next;
    }

  DPRINT("Section not found\n");

  return -1;
}


BOOLEAN
InfGetData (PINFCONTEXT Context,
	    PWCHAR *Key,
	    PWCHAR *Data)
{
  PINFCACHEKEY CacheKey;

  if (Context == NULL || Context->Line == NULL || Data == NULL)
    {
      DPRINT("Invalid parameter\n");
      return FALSE;
    }

  CacheKey = (PINFCACHEKEY)Context->Line;

  if (Key != NULL)
    *Key = CacheKey->Name;

  if (Data != NULL)
    *Data = CacheKey->Data;

  return TRUE;
}


/* EOF */
