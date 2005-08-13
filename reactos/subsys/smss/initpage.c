/* $Id$
 *
 * initpage.c - 
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
SmpPagingFilesQueryRoutine(PWSTR ValueName,
			  ULONG ValueType,
			  PVOID ValueData,
			  ULONG ValueLength,
			  PVOID Context,
			  PVOID EntryContext)
{
  UNICODE_STRING FileName;
  LARGE_INTEGER InitialSize;
  LARGE_INTEGER MaximumSize;
  NTSTATUS Status;
  LPWSTR p;

  DPRINT("ValueName '%S'  Type %lu  Length %lu\n", ValueName, ValueType, ValueLength);
  DPRINT("ValueData '%S'\n", (PWSTR)ValueData);

  if (ValueType != REG_SZ)
    {
      return(STATUS_SUCCESS);
    }

  /*
   * Format: "<path>[ <initial_size>[ <maximum_size>]]"
   */
  if ((p = wcschr(ValueData, ' ')) != NULL)
    {
      *p = L'\0';
      InitialSize.QuadPart = wcstoul(p + 1, &p, 0) * 256 * 4096;
      if (*p == ' ')
	{
	  MaximumSize.QuadPart = wcstoul(p + 1, NULL, 0) * 256 * 4096;
	}
      else
	MaximumSize = InitialSize;
    }
  else
    {
      InitialSize.QuadPart = 50 * 4096;
      MaximumSize.QuadPart = 80 * 4096;
    }

  if (!RtlDosPathNameToNtPathName_U ((LPWSTR)ValueData,
				     &FileName,
				     NULL,
				     NULL))
    {
      return (STATUS_SUCCESS);
    }

  DPRINT("SMSS: Created paging file %wZ with size %dKB\n",
	 &FileName, InitialSize.QuadPart / 1024);
  Status = NtCreatePagingFile(&FileName,
			      &InitialSize,
			      &MaximumSize,
			      0);

  RtlFreeUnicodeString(&FileName);

  return(STATUS_SUCCESS);
}


NTSTATUS
SmCreatePagingFiles(VOID)
{
  RTL_QUERY_REGISTRY_TABLE QueryTable[2];
  NTSTATUS Status;

  DPRINT("SM: creating system paging files\n");
  /*
   * Disable paging file on MiniNT/Live CD.
   */
  if (RtlCheckRegistryKey(RTL_REGISTRY_CONTROL, L"MiniNT") == STATUS_SUCCESS)
    {
      return STATUS_SUCCESS;
    }

  RtlZeroMemory(&QueryTable,
		sizeof(QueryTable));

  QueryTable[0].Name = L"PagingFiles";
  QueryTable[0].QueryRoutine = SmpPagingFilesQueryRoutine;

  Status = RtlQueryRegistryValues(RTL_REGISTRY_CONTROL,
				  L"\\Session Manager\\Memory Management",
				  QueryTable,
				  NULL,
				  NULL);

  return(Status);
}


/* EOF */
