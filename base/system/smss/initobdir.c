/*
 * PROJECT:         ReactOS Session Manager
 * LICENSE:         GPL v2 or later - See COPYING in the top level directory
 * FILE:            base/system/smss/initobdir.c
 * PURPOSE:         Object directories.
 * PROGRAMMERS:     ReactOS Development Team
 */

/* INCLUDES ******************************************************************/
#include "smss.h"

#define NDEBUG
#include <debug.h>

static NTSTATUS NTAPI
SmpObjectDirectoryQueryRoutine(PWSTR ValueName,
			      ULONG ValueType,
			      PVOID ValueData,
			      ULONG ValueLength,
			      PVOID Context,
			      PVOID EntryContext)
{
  OBJECT_ATTRIBUTES ObjectAttributes;
  UNICODE_STRING UnicodeString;
  HANDLE WindowsDirectory;
  NTSTATUS Status = STATUS_SUCCESS;

  DPRINT("ValueName '%S'  Type %lu  Length %lu\n", ValueName, ValueType, ValueLength);
  DPRINT("ValueData '%S'\n", (PWSTR)ValueData);
  if (ValueType != REG_SZ)
    {
      return(STATUS_SUCCESS);
    }

  RtlInitUnicodeString(&UnicodeString,
		       (PWSTR)ValueData);

  InitializeObjectAttributes(&ObjectAttributes,
			     &UnicodeString,
			     0,
			     NULL,
			     NULL);

  Status = ZwCreateDirectoryObject(&WindowsDirectory,
				   0,
				   &ObjectAttributes);

  return(Status);
}


NTSTATUS
SmCreateObjectDirectories(VOID)
{
  RTL_QUERY_REGISTRY_TABLE QueryTable[2];
  NTSTATUS Status;

  RtlZeroMemory(&QueryTable,
		sizeof(QueryTable));

  QueryTable[0].Name = L"ObjectDirectories";
  QueryTable[0].QueryRoutine = SmpObjectDirectoryQueryRoutine;

  Status = RtlQueryRegistryValues(RTL_REGISTRY_CONTROL,
				  SM_REGISTRY_ROOT_NAME,
				  QueryTable,
				  NULL,
				  NULL);

  return(Status);
}

/* EOF */
