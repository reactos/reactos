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
/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS text-mode setup
 * FILE:            subsys/system/usetup/drivesup.c
 * PURPOSE:         Drive support functions
 * PROGRAMMER:      Eric Kohl
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <ntdll/rtl.h>

#include "usetup.h"
#include "drivesup.h"


/* FUNCTIONS ****************************************************************/

NTSTATUS
GetSourcePaths(PUNICODE_STRING SourcePath,
	       PUNICODE_STRING SourceRootPath)
{
  OBJECT_ATTRIBUTES ObjectAttributes;
  UNICODE_STRING LinkName;
  UNICODE_STRING SourceName;
  WCHAR SourceBuffer[MAX_PATH];
  HANDLE Handle;
  NTSTATUS Status;
  ULONG Length;
  PWCHAR Ptr;

  RtlInitUnicodeString(&LinkName,
		       L"\\SystemRoot");

  InitializeObjectAttributes(&ObjectAttributes,
			     &LinkName,
			     OBJ_OPENLINK,
			     NULL,
			     NULL);

  Status = NtOpenSymbolicLinkObject(&Handle,
				    SYMBOLIC_LINK_ALL_ACCESS,
				    &ObjectAttributes);
  if (!NT_SUCCESS(Status))
    return(Status);

  SourceName.Length = 0;
  SourceName.MaximumLength = MAX_PATH * sizeof(WCHAR);
  SourceName.Buffer = SourceBuffer;

  Status = NtQuerySymbolicLinkObject(Handle,
				     &SourceName,
				     &Length);
  NtClose(Handle);

  if (NT_SUCCESS(Status))
    {
      RtlCreateUnicodeString(SourcePath,
			     SourceName.Buffer);

      /* strip trailing directory */
      Ptr = wcsrchr(SourceName.Buffer, L'\\');
//      if ((Ptr != NULL) &&
//	  (wcsicmp(Ptr, L"\\reactos") == 0))
      if (Ptr != NULL)
	*Ptr = 0;

      RtlCreateUnicodeString(SourceRootPath,
			     SourceName.Buffer);
    }

  NtClose(Handle);

  return(STATUS_SUCCESS);
}


#if 0
CHAR
GetDriveLetter(IN ULONG DriveNumber,
	       IN ULONG PartitionNumber)
{
  OBJECT_ATTRIBUTES ObjectAttributes;
  UNICODE_STRING LinkName;
  WCHAR LinkBuffer[8];
  WCHAR Letter;
  HANDLE LinkHandle;
  UNICODE_STRING TargetName;
  WCHAR TargetBuffer[MAX_PATH];
//  WCHAR DeviceBuffer[MAX_PATH];
  ULONG Length;

  wcscpy(LinkBuffer,
	 L"\\??\\A:");

  RtlInitUnicodeString(&LinkName,
		       LinkBuffer);

  InitializeObjectAttributes(&ObjectAttributes,
			     &LinkName,
			     OBJ_OPENLINK,
			     NULL,
			     NULL);

  TargetName.Length = 0;
  TargetName.MaximumLength = MAX_PATH * sizeof(WCHAR);
  TargetName.Buffer = TargetBuffer;

  for (Letter = L'C'; Letter <= L'Z'; Letter++)
    {
      LinkBuffer[4] = Letter;
      TargetName.Length = 0;

      Status = NtOpenSymbolicLinkObject(&LinkHandle,
					SYMBOLIC_LINK_ALL_ACCESS,
					&ObjectAttributes);
      if (NT_SUCCESS(Status))
	{
	  Status = NtQuerySymbolicLinkObject(LinkHandle,
					     &TargetName,
					     &Length);
	  if (NT_SUCCESS(Status))
	    {



	    }
	  NtClose(LinkHandle);
	}
    }

  return((CHAR)0);
}
#endif

#if 0
STATUS
GetFileSystem()
{

}
#endif

/* EOF */
