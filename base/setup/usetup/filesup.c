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
/* COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS text-mode setup
 * FILE:            subsys/system/usetup/filesup.c
 * PURPOSE:         File support functions
 * PROGRAMMER:      Eric Kohl
 *                  Casper S. Hornstrup (chorns@users.sourceforge.net)
 */

/* INCLUDES *****************************************************************/

#include "usetup.h"

#define NDEBUG
#include <debug.h>

/* FUNCTIONS ****************************************************************/

static BOOLEAN HasCurrentCabinet = FALSE;
static WCHAR CurrentCabinetName[MAX_PATH];

NTSTATUS
SetupCreateDirectory(PWCHAR DirectoryName)
{
  OBJECT_ATTRIBUTES ObjectAttributes;
  IO_STATUS_BLOCK IoStatusBlock;
  UNICODE_STRING PathName;
  HANDLE DirectoryHandle;
  NTSTATUS Status;

  RtlCreateUnicodeString(&PathName,
			 DirectoryName);
  if (PathName.Length > sizeof(WCHAR) &&
      PathName.Buffer[PathName.Length / sizeof(WCHAR) - 2] == L'\\' &&
      PathName.Buffer[PathName.Length / sizeof(WCHAR) - 1] == L'.')
    {
       PathName.Length -= sizeof(WCHAR);
       PathName.Buffer[PathName.Length / sizeof(WCHAR)] = 0;
    }

  if (PathName.Length > sizeof(WCHAR) &&
      PathName.Buffer[PathName.Length / sizeof(WCHAR) - 1] == L'\\')
    {
      PathName.Length -= sizeof(WCHAR);
      PathName.Buffer[PathName.Length / sizeof(WCHAR)] = 0;
   }

  InitializeObjectAttributes(&ObjectAttributes,
			     &PathName,
			     OBJ_CASE_INSENSITIVE | OBJ_INHERIT,
			     NULL,
			     NULL);

  Status = NtCreateFile(&DirectoryHandle,
			DIRECTORY_ALL_ACCESS,
			&ObjectAttributes,
			&IoStatusBlock,
			NULL,
			FILE_ATTRIBUTE_DIRECTORY,
			FILE_SHARE_READ | FILE_SHARE_WRITE,
			FILE_OPEN_IF,
			FILE_DIRECTORY_FILE,
			NULL,
			0);
  if (NT_SUCCESS(Status))
    {
      NtClose(DirectoryHandle);
    }

  RtlFreeUnicodeString(&PathName);

  return(Status);
}


NTSTATUS
SetupCopyFile(PWCHAR SourceFileName,
	      PWCHAR DestinationFileName)
{
  OBJECT_ATTRIBUTES ObjectAttributes;
  HANDLE FileHandleSource;
  HANDLE FileHandleDest;
  static IO_STATUS_BLOCK IoStatusBlock;
  FILE_STANDARD_INFORMATION FileStandard;
  FILE_BASIC_INFORMATION FileBasic;
  ULONG RegionSize;
  UNICODE_STRING FileName;
  NTSTATUS Status;
  PVOID SourceFileMap = 0;
  HANDLE SourceFileSection;
  SIZE_T SourceSectionSize = 0;
  LARGE_INTEGER ByteOffset;

#ifdef __REACTOS__
  RtlInitUnicodeString(&FileName,
		       SourceFileName);

  InitializeObjectAttributes(&ObjectAttributes,
			     &FileName,
			     OBJ_CASE_INSENSITIVE,
			     NULL,
			     NULL);

  Status = NtOpenFile(&FileHandleSource,
		      GENERIC_READ,
		      &ObjectAttributes,
		      &IoStatusBlock,
		      FILE_SHARE_READ,
		      FILE_SEQUENTIAL_ONLY);
  if(!NT_SUCCESS(Status))
    {
      DPRINT1("NtOpenFile failed: %x\n", Status);
      goto done;
    }
#else
  FileHandleSource = CreateFileW(SourceFileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
  if (FileHandleSource == INVALID_HANDLE_VALUE)
  {
    Status = STATUS_UNSUCCESSFUL;
    goto done;
  }
#endif

  Status = NtQueryInformationFile(FileHandleSource,
				  &IoStatusBlock,
				  &FileStandard,
				  sizeof(FILE_STANDARD_INFORMATION),
				  FileStandardInformation);
  if(!NT_SUCCESS(Status))
    {
      DPRINT1("NtQueryInformationFile failed: %x\n", Status);
      goto closesrc;
    }
  Status = NtQueryInformationFile(FileHandleSource,
				  &IoStatusBlock,&FileBasic,
				  sizeof(FILE_BASIC_INFORMATION),
				  FileBasicInformation);
  if(!NT_SUCCESS(Status))
    {
      DPRINT1("NtQueryInformationFile failed: %x\n", Status);
      goto closesrc;
    }

  Status = NtCreateSection( &SourceFileSection,
			    SECTION_MAP_READ,
			    NULL,
			    NULL,
			    PAGE_READONLY,
			    SEC_COMMIT,
			    FileHandleSource);
  if(!NT_SUCCESS(Status))
    {
      DPRINT1("NtCreateSection failed: %x\n", Status);
      goto closesrc;
    }

  Status = NtMapViewOfSection( SourceFileSection,
			       NtCurrentProcess(),
			       &SourceFileMap,
			       0,
			       0,
			       NULL,
			       &SourceSectionSize,
			       ViewUnmap,
			       0,
			       PAGE_READONLY );
  if(!NT_SUCCESS(Status))
    {
      DPRINT1("NtMapViewOfSection failed: %x\n", Status);
      goto closesrcsec;
    }

  RtlInitUnicodeString(&FileName,
		       DestinationFileName);

  InitializeObjectAttributes(&ObjectAttributes,
			     &FileName,
			     OBJ_CASE_INSENSITIVE,
			     NULL,
			     NULL);

  Status = NtCreateFile(&FileHandleDest,
			GENERIC_WRITE,
			&ObjectAttributes,
			&IoStatusBlock,
			NULL,
			FILE_ATTRIBUTE_NORMAL,
			0,
			FILE_OVERWRITE_IF,
			FILE_NO_INTERMEDIATE_BUFFERING |
			FILE_SEQUENTIAL_ONLY |
			FILE_SYNCHRONOUS_IO_NONALERT,
			NULL,
			0);
  if(!NT_SUCCESS(Status))
    {
      DPRINT1("NtCreateFile failed: %x\n", Status);
      goto unmapsrcsec;
    }

  RegionSize = (ULONG)PAGE_ROUND_UP(FileStandard.EndOfFile.u.LowPart);
  IoStatusBlock.Status = 0;
  ByteOffset.QuadPart = 0;
  Status = NtWriteFile(FileHandleDest,
		       NULL,
		       NULL,
		       NULL,
		       &IoStatusBlock,
		       SourceFileMap,
		       RegionSize,
		       &ByteOffset,
		       NULL);
  if(!NT_SUCCESS(Status))
    {
      DPRINT1("NtWriteFile failed: %x:%x, iosb: %p src: %p, size: %x\n", Status, IoStatusBlock.Status, &IoStatusBlock, SourceFileMap, RegionSize);
      goto closedest;
    }
  /* Copy file date/time from source file */
  Status = NtSetInformationFile(FileHandleDest,
				&IoStatusBlock,
				&FileBasic,
				sizeof(FILE_BASIC_INFORMATION),
				FileBasicInformation);
  if(!NT_SUCCESS(Status))
    {
      DPRINT1("NtSetInformationFile failed: %x\n", Status);
      goto closedest;
    }

  /* shorten the file back to it's real size after completing the write */
  Status = NtSetInformationFile(FileHandleDest,
		       &IoStatusBlock,
		       &FileStandard.EndOfFile,
		       sizeof(FILE_END_OF_FILE_INFORMATION),
		       FileEndOfFileInformation);

  if(!NT_SUCCESS(Status))
    {
      DPRINT1("NtSetInformationFile failed: %x\n", Status);
    }

 closedest:
  NtClose(FileHandleDest);
 unmapsrcsec:
  NtUnmapViewOfSection( NtCurrentProcess(), SourceFileMap );
 closesrcsec:
  NtClose(SourceFileSection);
 closesrc:
  NtClose(FileHandleSource);
 done:
  return(Status);
}

#ifdef __REACTOS__
NTSTATUS
SetupExtractFile(PWCHAR CabinetFileName,
        PWCHAR SourceFileName,
	      PWCHAR DestinationPathName)
{
  ULONG CabStatus;
  CAB_SEARCH Search;

  DPRINT("SetupExtractFile(CabinetFileName %S, SourceFileName %S, DestinationPathName %S)\n",
    CabinetFileName, SourceFileName, DestinationPathName);

  if (HasCurrentCabinet)
    {
      DPRINT("CurrentCabinetName: %S\n", CurrentCabinetName);
    }

  if ((HasCurrentCabinet) && (wcscmp(CabinetFileName, CurrentCabinetName) == 0))
    {
      DPRINT("Using same cabinet as last time\n");
    }
  else
    {
      DPRINT("Using new cabinet\n");

      if (HasCurrentCabinet)
        {
          CabinetCleanup();
        }

      wcscpy(CurrentCabinetName, CabinetFileName);

      CabinetInitialize();
      CabinetSetEventHandlers(NULL, NULL, NULL);
      CabinetSetCabinetName(CabinetFileName);

      CabStatus = CabinetOpen();
      if (CabStatus == CAB_STATUS_SUCCESS)
        {
          DPRINT("Opened cabinet %S\n", CabinetGetCabinetName());
          HasCurrentCabinet = TRUE;
        }
      else
        {
          DPRINT("Cannot open cabinet (%d)\n", CabStatus);
          return STATUS_UNSUCCESSFUL;
        }
    }

  CabinetSetDestinationPath(DestinationPathName);
  CabinetFindFirst( SourceFileName, &Search );
  CabStatus = CabinetExtractFile(&Search);
  if (CabStatus != CAB_STATUS_SUCCESS)
    {
      DPRINT("Cannot extract file %S (%d)\n", SourceFileName, CabStatus);
      return STATUS_UNSUCCESSFUL;
    }

  return STATUS_SUCCESS;
}
#endif

BOOLEAN
DoesFileExist(PWSTR PathName,
	      PWSTR FileName)
{
  OBJECT_ATTRIBUTES ObjectAttributes;
  IO_STATUS_BLOCK IoStatusBlock;
  UNICODE_STRING Name;
  WCHAR FullName[MAX_PATH];
  HANDLE FileHandle;
  NTSTATUS Status;

  wcscpy(FullName, PathName);
  if (FileName != NULL)
    {
      if (FileName[0] != L'\\')
	wcscat(FullName, L"\\");
      wcscat(FullName, FileName);
    }

  RtlInitUnicodeString(&Name,
		       FullName);

  InitializeObjectAttributes(&ObjectAttributes,
			     &Name,
			     OBJ_CASE_INSENSITIVE,
			     NULL,
			     NULL);

  Status = NtOpenFile(&FileHandle,
		      GENERIC_READ,
		      &ObjectAttributes,
		      &IoStatusBlock,
		      0,
		      FILE_SYNCHRONOUS_IO_NONALERT);
  if (!NT_SUCCESS(Status))
    {
      return(FALSE);
    }

  NtClose(FileHandle);

  return(TRUE);
}

/* EOF */
