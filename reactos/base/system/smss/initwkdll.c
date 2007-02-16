/* $Id$
 *
 * initwkdll.c - Load the well known DLLs
 * 
 * ReactOS Operating System
 * 
 * --------------------------------------------------------------------
 *
 * This software is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING.LIB. If not, write
 * to the Free Software Foundation, Inc., 675 Mass Ave, Cambridge,
 * MA 02139, USA.  
 *
 * --------------------------------------------------------------------
 */

#include "smss.h"

#define NDEBUG
#include <debug.h>

static NTSTATUS STDCALL
SmpKnownDllsQueryRoutine(PWSTR ValueName,
			ULONG ValueType,
			PVOID ValueData,
			ULONG ValueLength,
			PVOID Context,
			PVOID EntryContext)
{
  OBJECT_ATTRIBUTES ObjectAttributes;
  IO_STATUS_BLOCK IoStatusBlock;
  UNICODE_STRING ImageName;
  HANDLE FileHandle;
  HANDLE SectionHandle;
  NTSTATUS Status;

  DPRINT("ValueName '%S'  Type %lu  Length %lu\n", ValueName, ValueType, ValueLength);
  DPRINT("ValueData '%S'  Context %p  EntryContext %p\n", (PWSTR)ValueData, Context, EntryContext);

  /* Ignore the 'DllDirectory' value */
  if (!_wcsicmp(ValueName, L"DllDirectory"))
    return STATUS_SUCCESS;

  /* Open the DLL image file */
  RtlInitUnicodeString(&ImageName,
		       ValueData);
  InitializeObjectAttributes(&ObjectAttributes,
			     &ImageName,
			     OBJ_CASE_INSENSITIVE,
			     (HANDLE)Context,
			     NULL);
  Status = NtOpenFile(&FileHandle,
		      SYNCHRONIZE | FILE_EXECUTE | FILE_READ_DATA,
		      &ObjectAttributes,
		      &IoStatusBlock,
		      FILE_SHARE_READ,
		      FILE_SYNCHRONOUS_IO_NONALERT | FILE_NON_DIRECTORY_FILE);
  if (!NT_SUCCESS(Status))
    {
      DPRINT1("NtOpenFile() failed (Status %lx)\n", Status);
      return STATUS_SUCCESS;
    }

  DPRINT("Opened file %wZ successfully\n", &ImageName);

  /* Check for valid image checksum */
  Status = LdrVerifyImageMatchesChecksum (FileHandle,
					  0,
					  0,
					  0);
  if (Status == STATUS_IMAGE_CHECKSUM_MISMATCH)
    {
      /* Raise a hard error (crash the system/BSOD) */
      NtRaiseHardError (Status,
			0,
			0,
			0,
			0,
			0);
    }
  else if (!NT_SUCCESS(Status))
    {
      DPRINT1("Failed to check the image checksum\n");

      NtClose(FileHandle);

      return STATUS_SUCCESS;
    }

  InitializeObjectAttributes(&ObjectAttributes,
			     &ImageName,
			     OBJ_CASE_INSENSITIVE | OBJ_PERMANENT,
			     (HANDLE)EntryContext,
			     NULL);
  Status = NtCreateSection(&SectionHandle,
			   SECTION_ALL_ACCESS,
			   &ObjectAttributes,
			   NULL,
			   PAGE_EXECUTE,
			   SEC_IMAGE,
			   FileHandle);
  if (NT_SUCCESS(Status))
    {
      DPRINT("Created section successfully\n");
      NtClose(SectionHandle);
    }

  NtClose(FileHandle);

  return STATUS_SUCCESS;
}


NTSTATUS
SmLoadKnownDlls(VOID)
{
  RTL_QUERY_REGISTRY_TABLE QueryTable[2];
  OBJECT_ATTRIBUTES ObjectAttributes;
  IO_STATUS_BLOCK IoStatusBlock;
  UNICODE_STRING DllDosPath;
  UNICODE_STRING DllNtPath;
  UNICODE_STRING Name;
  HANDLE ObjectDirHandle;
  HANDLE FileDirHandle;
  HANDLE SymlinkHandle;
  NTSTATUS Status;


  DPRINT("SM: loading well-known DLLs\n");

  DPRINT("SmLoadKnownDlls() called\n");

  /* Create 'KnownDlls' object directory */
  RtlInitUnicodeString(&Name,
		       L"\\KnownDlls");
  InitializeObjectAttributes(&ObjectAttributes,
			     &Name,
			     OBJ_PERMANENT | OBJ_CASE_INSENSITIVE | OBJ_OPENIF,
			     NULL,
			     NULL);
  Status = NtCreateDirectoryObject(&ObjectDirHandle,
				   DIRECTORY_ALL_ACCESS,
				   &ObjectAttributes);
  if (!NT_SUCCESS(Status))
    {
      DPRINT1("NtCreateDirectoryObject() failed (Status %lx)\n", Status);
      return Status;
    }

  RtlInitUnicodeString(&DllDosPath, NULL);

  RtlZeroMemory(&QueryTable,
		sizeof(QueryTable));

  QueryTable[0].Name = L"DllDirectory";
  QueryTable[0].Flags = RTL_QUERY_REGISTRY_DIRECT;
  QueryTable[0].EntryContext = &DllDosPath;

  Status = RtlQueryRegistryValues(RTL_REGISTRY_CONTROL,
				  L"\\Session Manager\\KnownDlls",
				  QueryTable,
				  NULL,
				  SmSystemEnvironment);
  if (!NT_SUCCESS(Status))
    {
      DPRINT1("RtlQueryRegistryValues() failed (Status %lx)\n", Status);
      return Status;
    }

  DPRINT("DllDosPath: '%wZ'\n", &DllDosPath);

  if (!RtlDosPathNameToNtPathName_U(DllDosPath.Buffer,
				    &DllNtPath,
				    NULL,
				    NULL))
    {
      DPRINT1("RtlDosPathNameToNtPathName_U() failed\n");
      return STATUS_OBJECT_NAME_INVALID;
    }

  DPRINT("DllNtPath: '%wZ'\n", &DllNtPath);

  /* Open the dll path directory */
  InitializeObjectAttributes(&ObjectAttributes,
			     &DllNtPath,
			     OBJ_CASE_INSENSITIVE,
			     NULL,
			     NULL);
  Status = NtOpenFile(&FileDirHandle,
		      SYNCHRONIZE | FILE_READ_DATA,
		      &ObjectAttributes,
		      &IoStatusBlock,
		      FILE_SHARE_READ | FILE_SHARE_WRITE,
		      FILE_SYNCHRONOUS_IO_NONALERT | FILE_DIRECTORY_FILE);

  RtlFreeHeap(RtlGetProcessHeap(),
              0,
              DllNtPath.Buffer);

  if (!NT_SUCCESS(Status))
    {
      DPRINT1("NtOpenFile failed (Status %lx)\n", Status);
      return Status;
    }

  /* Link 'KnownDllPath' the dll path directory */
  RtlInitUnicodeString(&Name,
		       L"KnownDllPath");
  InitializeObjectAttributes(&ObjectAttributes,
			     &Name,
			     OBJ_PERMANENT | OBJ_CASE_INSENSITIVE | OBJ_OPENIF,
			     ObjectDirHandle,
			     NULL);
  Status = NtCreateSymbolicLinkObject(&SymlinkHandle,
				      SYMBOLIC_LINK_ALL_ACCESS,
				      &ObjectAttributes,
				      &DllDosPath);
  if (!NT_SUCCESS(Status))
    {
      DPRINT1("NtCreateSymbolicLink() failed (Status %lx)\n", Status);
      return Status;
    }

  NtClose(SymlinkHandle);

  RtlZeroMemory(&QueryTable,
		sizeof(QueryTable));

  QueryTable[0].QueryRoutine = SmpKnownDllsQueryRoutine;
  QueryTable[0].EntryContext = ObjectDirHandle;

  Status = RtlQueryRegistryValues(RTL_REGISTRY_CONTROL,
				  L"\\Session Manager\\KnownDlls",
				  QueryTable,
				  (PVOID)FileDirHandle,
				  NULL);
  if (!NT_SUCCESS(Status))
    {
      DPRINT1("RtlQueryRegistryValues() failed (Status %lx)\n", Status);
    }

  DPRINT("SmLoadKnownDlls() done\n");

  return Status;
}


/* EOF */
