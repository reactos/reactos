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
/* $Id: filesup.c,v 1.4 2003/01/17 13:18:15 ekohl Exp $
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS text-mode setup
 * FILE:            subsys/system/usetup/filesup.c
 * PURPOSE:         File support functions
 * PROGRAMMER:      Eric Kohl
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <ntdll/rtl.h>

#include "usetup.h"
#include "filesup.h"


/* FUNCTIONS ****************************************************************/


NTSTATUS
CreateDirectory(PWCHAR DirectoryName)
{
  OBJECT_ATTRIBUTES ObjectAttributes;
  IO_STATUS_BLOCK IoStatusBlock;
  UNICODE_STRING PathName;
  HANDLE DirectoryHandle;
  NTSTATUS Status;

  RtlCreateUnicodeString(&PathName,
			 DirectoryName);

#if 0
  ObjectAttributes.Length = sizeof(OBJECT_ATTRIBUTES);
  ObjectAttributes.RootDirectory = NULL;
  ObjectAttributes.ObjectName = &PathName;
  ObjectAttributes.Attributes = OBJ_CASE_INSENSITIVE | OBJ_INHERIT;
  ObjectAttributes.SecurityDescriptor = NULL;
  ObjectAttributes.SecurityQualityOfService = NULL;
#endif

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
			0,
			FILE_CREATE,
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
  IO_STATUS_BLOCK IoStatusBlock;
  FILE_STANDARD_INFORMATION FileStandard;
  FILE_BASIC_INFORMATION FileBasic;
  FILE_POSITION_INFORMATION FilePosition;
  PUCHAR Buffer;
  ULONG RegionSize;
  UNICODE_STRING FileName;
  NTSTATUS Status;

  Buffer = NULL;

  RtlInitUnicodeString(&FileName,
		       SourceFileName);

  InitializeObjectAttributes(&ObjectAttributes,
			     &FileName,
			     OBJ_CASE_INSENSITIVE,
			     NULL,
			     NULL);

  Status = NtOpenFile(&FileHandleSource,
		      FILE_READ_ACCESS,
		      &ObjectAttributes,
		      &IoStatusBlock,
		      FILE_SHARE_READ,
		      FILE_SYNCHRONOUS_IO_ALERT | FILE_SEQUENTIAL_ONLY);
  if (!NT_SUCCESS(Status))
  {
CHECKPOINT1;
    return(Status);
  }

  Status = NtQueryInformationFile(FileHandleSource,
				  &IoStatusBlock,
				  &FileStandard,
				  sizeof(FILE_STANDARD_INFORMATION),
				  FileStandardInformation);
  if (!NT_SUCCESS(Status))
  {
CHECKPOINT1;
    NtClose(FileHandleSource);
    return(Status);
  }

  Status = NtQueryInformationFile(FileHandleSource,
				  &IoStatusBlock,&FileBasic,
				  sizeof(FILE_BASIC_INFORMATION),
				  FileBasicInformation);
  if (!NT_SUCCESS(Status))
  {
CHECKPOINT1;
    NtClose(FileHandleSource);
    return(Status);
  }

  RtlInitUnicodeString(&FileName,
		       DestinationFileName);

  InitializeObjectAttributes(&ObjectAttributes,
			     &FileName,
			     OBJ_CASE_INSENSITIVE,
			     NULL,
			     NULL);

  Status = NtCreateFile(&FileHandleDest,
			FILE_WRITE_ACCESS,
			&ObjectAttributes,
			&IoStatusBlock,
			NULL,
			FILE_ATTRIBUTE_NORMAL,
			0,
			FILE_OVERWRITE_IF,
			FILE_SYNCHRONOUS_IO_ALERT | FILE_SEQUENTIAL_ONLY,
			NULL,
			0);
  if (!NT_SUCCESS(Status))
  {
CHECKPOINT1;
    NtClose(FileHandleSource);
    return(Status);
  }

  FilePosition.CurrentByteOffset.QuadPart = 0;

  Status = NtSetInformationFile(FileHandleSource,
				&IoStatusBlock,
				&FilePosition,
				sizeof(FILE_POSITION_INFORMATION),
				FilePositionInformation);
  if (!NT_SUCCESS(Status))
  {
CHECKPOINT1;
    NtClose(FileHandleSource);
    NtClose(FileHandleDest);
    return(Status);
  }

  Status = NtSetInformationFile(FileHandleDest,
				&IoStatusBlock,
				&FilePosition,
				sizeof(FILE_POSITION_INFORMATION),
				FilePositionInformation);
  if (!NT_SUCCESS(Status))
  {
CHECKPOINT1;
    NtClose(FileHandleSource);
    NtClose(FileHandleDest);
    return(Status);
  }

  RegionSize = PAGE_ROUND_UP(FileStandard.EndOfFile.u.LowPart);
  if (RegionSize > 0x100000)
  {
     RegionSize = 0x100000;
  }
  Status = NtAllocateVirtualMemory(NtCurrentProcess(),
				   (PVOID *)&Buffer,
				   2,
				   &RegionSize,
				   MEM_RESERVE | MEM_COMMIT,
				   PAGE_READWRITE);
  if (!NT_SUCCESS(Status))
  {
CHECKPOINT1;
    NtClose(FileHandleSource);
    NtClose(FileHandleDest);
    return(Status);
  }

  while (TRUE)
  {
    Status = NtReadFile(FileHandleSource,
			NULL,
			NULL,
			NULL,
			&IoStatusBlock,
			Buffer,
			RegionSize,
			NULL,
			NULL);
    if (!NT_SUCCESS(Status))
    {
      NtFreeVirtualMemory(NtCurrentProcess(),
			  (PVOID *)&Buffer,
			  &RegionSize,
			  MEM_RELEASE);
      NtClose(FileHandleSource);
      NtClose(FileHandleDest);
      if (Status == STATUS_END_OF_FILE)
      {
	DPRINT("STATUS_END_OF_FILE\n");
	break;
      }
CHECKPOINT1;
      return(Status);
    }

DPRINT("Bytes read %lu\n", IoStatusBlock.Information);

    Status = NtWriteFile(FileHandleDest,
			 NULL,
			 NULL,
			 NULL,
			 &IoStatusBlock,
			 Buffer,
			 IoStatusBlock.Information,
			 NULL,
			 NULL);
    if (!NT_SUCCESS(Status))
    {
CHECKPOINT1;
      NtFreeVirtualMemory(NtCurrentProcess(),
			  (PVOID *)&Buffer,
			  &RegionSize,
			  MEM_RELEASE);
      NtClose(FileHandleSource);
      NtClose(FileHandleDest);
      return(Status);
    }
  }

  return(STATUS_SUCCESS);
}


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
		      FILE_READ_ACCESS,
		      &ObjectAttributes,
		      &IoStatusBlock,
		      0,
		      FILE_SYNCHRONOUS_IO_ALERT);
  if (!NT_SUCCESS(Status))
  {
CHECKPOINT1;
    return(FALSE);
  }

  NtClose(FileHandle);

  return(TRUE);
}

/* EOF */
