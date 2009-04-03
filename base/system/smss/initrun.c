/*
 * PROJECT:         ReactOS Session Manager
 * LICENSE:         GPL v2 or later - See COPYING in the top level directory
 * FILE:            base/system/smss/initrun.c
 * PURPOSE:         Run all programs in the boot execution list.
 * PROGRAMMERS:     ReactOS Development Team
 */

/* INCLUDES ******************************************************************/
#include "smss.h"

#define NDEBUG
#include <debug.h>

//HANDLE Children[2] = {0, 0}; /* csrss, winlogon */


/**********************************************************************
 * SmpRunBootAppsQueryRoutine/6
 */
static NTSTATUS NTAPI
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
				SM_CREATE_FLAG_WAIT,
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
