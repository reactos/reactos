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
/* $Id: filesup.c,v 1.1 2002/11/13 18:25:18 ekohl Exp $
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

  ObjectAttributes.Length = sizeof(OBJECT_ATTRIBUTES);
  ObjectAttributes.RootDirectory = NULL;
  ObjectAttributes.ObjectName = &PathName;
  ObjectAttributes.Attributes = OBJ_CASE_INSENSITIVE | OBJ_INHERIT;
  ObjectAttributes.SecurityDescriptor = NULL;
  ObjectAttributes.SecurityQualityOfService = NULL;

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

/* EOF */
