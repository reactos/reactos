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
/* $Id: filesup.c,v 1.10 2004/01/18 22:34:40 hbirr Exp $
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS text-mode setup
 * FILE:            subsys/system/usetup/filesup.c
 * PURPOSE:         File support functions
 * PROGRAMMER:      Eric Kohl
 *                  Casper S. Hornstrup (chorns@users.sourceforge.net)
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <ntdll/rtl.h>

#include "usetup.h"
#include "filesup.h"
#include "cabinet.h"

#define NDEBUG
#include <debug.h>

/* FUNCTIONS ****************************************************************/


static BOOLEAN HasCurrentCabinet = FALSE;
static WCHAR CurrentCabinetName[MAX_PATH];

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
			0,
			FILE_CREATE,
			FILE_SYNCHRONOUS_IO_NONALERT | FILE_DIRECTORY_FILE,
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
		      FILE_SYNCHRONOUS_IO_NONALERT | FILE_SEQUENTIAL_ONLY);
  if (!NT_SUCCESS(Status))
    {
      return(Status);
    }

  Status = NtQueryInformationFile(FileHandleSource,
				  &IoStatusBlock,
				  &FileStandard,
				  sizeof(FILE_STANDARD_INFORMATION),
				  FileStandardInformation);
  if (!NT_SUCCESS(Status))
    {
     NtClose(FileHandleSource);
     return(Status);
    }

  Status = NtQueryInformationFile(FileHandleSource,
				  &IoStatusBlock,&FileBasic,
				  sizeof(FILE_BASIC_INFORMATION),
				  FileBasicInformation);
  if (!NT_SUCCESS(Status))
    {
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
			FILE_SYNCHRONOUS_IO_NONALERT | FILE_SEQUENTIAL_ONLY,
			NULL,
			0);
  if (!NT_SUCCESS(Status))
    {
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
	  if (Status == STATUS_END_OF_FILE)
	    {
	      DPRINT("STATUS_END_OF_FILE\n");
	      break;
	    }
	  NtClose(FileHandleSource);
	  NtClose(FileHandleDest);
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
	  NtFreeVirtualMemory(NtCurrentProcess(),
			      (PVOID *)&Buffer,
			      &RegionSize,
			      MEM_RELEASE);
	  NtClose(FileHandleSource);
	  NtClose(FileHandleDest);
	  return(Status);
	}
    }


  /* Copy file date/time from source file */
  Status = NtSetInformationFile(FileHandleDest,
				&IoStatusBlock,
				&FileBasic,
				sizeof(FILE_BASIC_INFORMATION),
				FileBasicInformation);
  if (!NT_SUCCESS(Status))
    {
      DPRINT("NtSetInformationFile() failed (Status %lx)\n", Status);
    }

  NtClose(FileHandleSource);
  NtClose(FileHandleDest);

  return(Status);
}


NTSTATUS
SetupExtractFile(PWCHAR CabinetFileName,
        PWCHAR SourceFileName,
	      PWCHAR DestinationPathName)
{
  ULONG CabStatus;

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
  CabStatus = CabinetExtractFile(SourceFileName);
  if (CabStatus != CAB_STATUS_SUCCESS)
    {
      DPRINT("Cannot extract file %S (%d)\n", SourceFileName, CabStatus);
      return STATUS_UNSUCCESSFUL;
    }

  return STATUS_SUCCESS;
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
		      FILE_SYNCHRONOUS_IO_NONALERT);
  if (!NT_SUCCESS(Status))
    {
      return(FALSE);
    }

  NtClose(FileHandle);

  return(TRUE);
}

/* EOF */
