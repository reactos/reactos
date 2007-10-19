/* $Id$
 *
 * initrun.c - Run all programs in the boot execution list
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

//HANDLE Children[2] = {0, 0}; /* csrss, winlogon */


/**********************************************************************
 * SmpRunBootAppsQueryRoutine/6
 */
static NTSTATUS STDCALL
SmpRunBootAppsQueryRoutine(PWSTR ValueName,
			  ULONG ValueType,
			  PVOID ValueData,
			  ULONG ValueLength,
			  PVOID Context,
			  PVOID EntryContext)
{
  WCHAR Description [MAX_PATH];
  WCHAR ImageName [MAX_PATH];
  WCHAR ImagePath [MAX_PATH];
  WCHAR CommandLine [MAX_PATH];
  PWSTR p1, p2;
  ULONG len;
  NTSTATUS Status;

  DPRINT("ValueName '%S'  Type %lu  Length %lu\n", ValueName, ValueType, ValueLength);
  DPRINT("ValueData '%S'\n", (PWSTR)ValueData);

  if (ValueType != REG_SZ)
    {
      return(STATUS_SUCCESS);
    }

  /* Extract the description */
  p1 = wcschr((PWSTR)ValueData, L' ');
  len = p1 - (PWSTR)ValueData;
  memcpy(Description,ValueData, len * sizeof(WCHAR));
  Description[len] = 0;

  /* Extract the image name */
  p1++;
  p2 = wcschr(p1, L' ');
  if (p2 != NULL)
    len = p2 - p1;
  else
    len = wcslen(p1);
  memcpy(ImageName, p1, len * sizeof(WCHAR));
  ImageName[len] = 0;

  /* Extract the command line */
  if (p2 == NULL)
    {
      CommandLine[0] = 0;
    }
  else
    {
      p2++;
      wcscpy(CommandLine, p2);
    }

  DPRINT("Running %S...\n", Description);
  DPRINT("ImageName: '%S'\n", ImageName);
  DPRINT("CommandLine: '%S'\n", CommandLine);

  /* initialize executable path */
  wcscpy(ImagePath, L"\\SystemRoot\\system32\\");
  wcscat(ImagePath, ImageName);
  wcscat(ImagePath, L".exe");

  /* Create NT process */
  Status = SmCreateUserProcess (ImagePath,
		  		CommandLine,
				TRUE, /* wait */
				NULL, NULL);
  if (!NT_SUCCESS(Status))
  {
		DPRINT1("SM: %s: running '$S' failed (Status=0x%08lx)\n",
			__FUNCTION__, Status);
  }
  return(STATUS_SUCCESS);
}


/**********************************************************************
 * SmRunBootApplications/0
 *
 * DESCRIPTION
 *
 * Run native applications listed in the registry.
 *
 *  Key:
 *    \Registry\Machine\SYSTEM\CurrentControlSet\Control\Session Manager
 *
 *  Value (format: "<description> <executable> <command line>":
 *    BootExecute = "autocheck autochk *"
 */
NTSTATUS
SmRunBootApplications(VOID)
{
  RTL_QUERY_REGISTRY_TABLE QueryTable[2];
  NTSTATUS Status;

  RtlZeroMemory(&QueryTable,
		sizeof(QueryTable));

  QueryTable[0].Name = L"BootExecute";
  QueryTable[0].QueryRoutine = SmpRunBootAppsQueryRoutine;

  Status = RtlQueryRegistryValues(RTL_REGISTRY_CONTROL,
				  L"\\Session Manager",
				  QueryTable,
				  NULL,
				  NULL);
  if (!NT_SUCCESS(Status))
    {
      DPRINT1("%s: RtlQueryRegistryValues() failed! (Status %lx)\n",
	__FUNCTION__,
	Status);
    }

  return(Status);
}


/* EOF */
