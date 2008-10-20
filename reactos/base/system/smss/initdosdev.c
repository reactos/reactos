/*
 * PROJECT:         ReactOS Session Manager
 * LICENSE:         GPL v2 or later - See COPYING in the top level directory
 * FILE:            base/system/smss/initdosdev.c
 * PURPOSE:         Define symbolic links to kernel devices (MS-DOS names).
 * PROGRAMMERS:     ReactOS Development Team
 */

/* INCLUDES ******************************************************************/
#include "smss.h"

#define NDEBUG
#include <debug.h>

static NTSTATUS STDCALL
SmpDosDevicesQueryRoutine(PWSTR ValueName,
			 ULONG ValueType,
			 PVOID ValueData,
			 ULONG ValueLength,
			 PVOID Context,
			 PVOID EntryContext)
{
  OBJECT_ATTRIBUTES ObjectAttributes;
  UNICODE_STRING DeviceName;
  UNICODE_STRING LinkName;
  HANDLE LinkHandle;
  WCHAR LinkBuffer[80];
  NTSTATUS Status;

  DPRINT("ValueName '%S'  Type %lu  Length %lu\n", ValueName, ValueType, ValueLength);
  DPRINT("ValueData '%S'\n", (PWSTR)ValueData);

  if (ValueType != REG_SZ)
    {
      return(STATUS_SUCCESS);
    }

  swprintf(LinkBuffer,
	   L"\\??\\%s",
	   ValueName);
  RtlInitUnicodeString(&LinkName,
		       LinkBuffer);
  RtlInitUnicodeString(&DeviceName,
		       (PWSTR)ValueData);

  DPRINT("SM: Linking %wZ --> %wZ\n",
	      &LinkName,
	      &DeviceName);

  /* create symbolic link */
  InitializeObjectAttributes(&ObjectAttributes,
			     &LinkName,
			     OBJ_PERMANENT|OBJ_CASE_INSENSITIVE,
			     NULL,
			     NULL);
  Status = NtCreateSymbolicLinkObject(&LinkHandle,
				      SYMBOLIC_LINK_ALL_ACCESS,
				      &ObjectAttributes,
				      &DeviceName);
  if (!NT_SUCCESS(Status))
    {
      DPRINT1("%s: NtCreateSymbolicLink( %wZ --> %wZ ) failed!\n",
		  __FUNCTION__,
		  &LinkName,
		  &DeviceName);
    }
  NtClose(LinkHandle);

  return(Status);
}


NTSTATUS
SmInitDosDevices(VOID)
{
  RTL_QUERY_REGISTRY_TABLE QueryTable[2];
  NTSTATUS Status;

  RtlZeroMemory(&QueryTable,
		sizeof(QueryTable));

  QueryTable[0].QueryRoutine = SmpDosDevicesQueryRoutine;

  Status = RtlQueryRegistryValues(RTL_REGISTRY_CONTROL,
				  L"\\Session Manager\\DOS Devices",
				  QueryTable,
				  NULL,
				  NULL);
  return(Status);
}

/* EOF */
