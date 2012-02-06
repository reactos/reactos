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

#if 0
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
    PCWSTR DefaultPath = L"\\SystemRoot\\system32\\";
    PCWSTR DefaultExtension = L".exe";
    PWSTR ImageName = NULL;
    PWSTR ImagePath = NULL;
    PWSTR CommandLine = NULL;
    PWSTR p1 = (PWSTR)ValueData;
    PWSTR p2 = (PWSTR)ValueData;
    ULONG len = 0;
    BOOLEAN HasAutocheckToken;
    BOOLEAN HasNoExtension;
    BOOLEAN HasDefaultPath;
    NTSTATUS Status = STATUS_SUCCESS;

    DPRINT("ValueName '%S'  Type %lu  Length %lu\n", ValueName, ValueType, ValueLength);
    DPRINT("ValueData '%S'\n", (PWSTR)ValueData);

    if (ValueType != REG_SZ)
        return STATUS_SUCCESS;

    /* Skip leading spaces */
    while (*p1 == L' ')
       p1++;

    /* Get the next token */
    p2 = wcschr(p1, L' ');
    if (p2 == NULL)
        p2 = p1 + wcslen(p1);
    len = p2 - p1;

    /* Check whether or not we have the 'autocheck' token */
    HasAutocheckToken = ((len == 9) && (_wcsnicmp(p1, L"autocheck", 9) == 0));

    if (HasAutocheckToken)
    {
        /* Skip the current (autocheck) token */
        p1 = p2;

        /* Skip spaces */
        while (*p1 == L' ')
            p1++;

        /* Get the next token */
        p2 = wcschr(p1, L' ');
        if (p2 == NULL)
            p2 = p1 + wcslen(p1);
        len = p2 - p1;
    }

    /*
     * Now, p1-->p2 is the image name and len is its length.
     * If the image name is "" (empty string), then we stop
     * here, we don't execute anything and return STATUS_SUCCESS.
     */
    if (len == 0)
        return STATUS_SUCCESS;

    /* Allocate the image name buffer */
    ImageName = RtlAllocateHeap(SmpHeap, HEAP_ZERO_MEMORY, (len + 1) * sizeof(WCHAR));
    if (ImageName == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Done;
    }

    /* Extract the image name */
    memmove(ImageName, p1, len * sizeof(WCHAR));

    /* Skip the current token */
    p1 = p2;

    /* Skip spaces */
    while (*p1 == L' ')
        p1++;

    /* Get the length of the command line */
    len = wcslen(p1);

    /* Allocate the command line buffer */
    CommandLine = RtlAllocateHeap(SmpHeap, HEAP_ZERO_MEMORY, (len + 1) * sizeof(WCHAR));
    if (CommandLine == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Done;
    }

    /* Extract the command line. */
    memmove(CommandLine, p1, len * sizeof(WCHAR));

    /* Determine the image path length */
    HasDefaultPath = (_wcsnicmp(ImageName, DefaultPath, wcslen(DefaultPath)) == 0);
    HasNoExtension = (wcsrchr(ImageName, L'.') == NULL);

    len = wcslen(ImageName);

    if (!HasDefaultPath)
        len += wcslen(DefaultPath);

    if (HasNoExtension)
        len += wcslen(DefaultExtension);

    /* Allocate the image path buffer */
    ImagePath = RtlAllocateHeap(SmpHeap, HEAP_ZERO_MEMORY, (len + 1) * sizeof(WCHAR));
    if (ImagePath == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Done;
    }

    /* Build the image path */
    if (HasDefaultPath)
    {
        wcscpy(ImagePath, ImageName);
    }
    else
    {
        wcscpy(ImagePath, DefaultPath);
        wcscat(ImagePath, ImageName);
    }

    if (HasNoExtension)
        wcscat(ImagePath, DefaultExtension);

    DPRINT("ImageName  : '%S'\n", ImageName);
    DPRINT("ImagePath  : '%S'\n", ImagePath);
    DPRINT("CommandLine: '%S'\n", CommandLine);

    /* Create NT process */
    Status = SmCreateUserProcess(ImagePath,
                                 CommandLine,
                                 SM_CREATE_FLAG_WAIT,
                                 NULL, NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("SM: %s: running '%S' failed (Status=0x%08lx)\n",
                __FUNCTION__, ImageName, Status);

        if (HasAutocheckToken)
            PrintString("%S program not found - skipping AUTOCHECK\n", ImageName);

        /* No need to return an error */
        Status = STATUS_SUCCESS;
    }

Done:
    /* Free the buffers */
    if (ImagePath != NULL)
        RtlFreeHeap(SmpHeap, 0, ImagePath);

    if (CommandLine != NULL)
        RtlFreeHeap(SmpHeap, 0, CommandLine);

    if (ImageName != NULL)
        RtlFreeHeap(SmpHeap, 0, ImageName);

    return Status;
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
#endif

/* EOF */
