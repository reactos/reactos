/* $Id: timezone.c,v 1.3 2002/09/07 15:12:41 chorns Exp $
 *
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * PURPOSE:           Timezone functions
 * FILE:              lib/ntdll/rtl/timezone.c
 * PROGRAMER:         Eric Kohl
 * REVISION HISTORY:
 *                    29/05/2001: Created
 */

/* INCLUDES *****************************************************************/

#define NTOS_USER_MODE
#include <ntos.h>

#define NDEBUG
#include <debug.h>

/* FUNCTIONS *****************************************************************/

NTSTATUS STDCALL
RtlQueryTimeZoneInformation(PTIME_ZONE_INFORMATION TimeZoneInformation)
{
   HANDLE KeyHandle;
   RTL_QUERY_REGISTRY_TABLE QueryTable[8];
   UNICODE_STRING StandardName;
   UNICODE_STRING DaylightName;
   NTSTATUS Status;
   
   DPRINT("RtlQueryTimeZoneInformation()\n");
   
   Status = RtlpGetRegistryHandle(RTL_REGISTRY_CONTROL,
				  L"TimeZoneInformation",
				  TRUE,
				  &KeyHandle);
   if (!NT_SUCCESS(Status))
     {
	DPRINT("RtlpGetRegistryHandle failed (Status %x)\n", Status);
	return Status;
     }
   
   RtlZeroMemory(QueryTable,
		 sizeof(QueryTable));
   
   StandardName.Length = 0;
   StandardName.MaximumLength = 32 * sizeof(WCHAR);
   StandardName.Buffer = TimeZoneInformation->StandardName;
   
   DaylightName.Length = 0;
   DaylightName.MaximumLength = 32 * sizeof(WCHAR);
   DaylightName.Buffer = TimeZoneInformation->DaylightName;
   
   QueryTable[0].Name = L"Bias";
   QueryTable[0].Flags = RTL_QUERY_REGISTRY_DIRECT;
   QueryTable[0].EntryContext = &TimeZoneInformation->Bias;
   
   QueryTable[1].Name = L"Standard Name";
   QueryTable[1].Flags = RTL_QUERY_REGISTRY_DIRECT;
   QueryTable[1].EntryContext = &StandardName;
   
   QueryTable[2].Name = L"Standard Bias";
   QueryTable[2].Flags = RTL_QUERY_REGISTRY_DIRECT;
   QueryTable[2].EntryContext = &TimeZoneInformation->StandardBias;
   
   QueryTable[3].Name = L"Standard Start";
   QueryTable[3].Flags = RTL_QUERY_REGISTRY_DIRECT;
   QueryTable[3].EntryContext = &TimeZoneInformation->StandardDate;
   
   QueryTable[4].Name = L"Daylight Name";
   QueryTable[4].Flags = RTL_QUERY_REGISTRY_DIRECT;
   QueryTable[4].EntryContext = &DaylightName;
   
   QueryTable[5].Name = L"Daylight Bias";
   QueryTable[5].Flags = RTL_QUERY_REGISTRY_DIRECT;
   QueryTable[5].EntryContext = &TimeZoneInformation->DaylightBias;
   
   QueryTable[6].Name = L"Daylight Start";
   QueryTable[6].Flags = RTL_QUERY_REGISTRY_DIRECT;
   QueryTable[6].EntryContext = &TimeZoneInformation->DaylightDate;
   
   Status = RtlQueryRegistryValues(RTL_REGISTRY_HANDLE,
				   (PWSTR)KeyHandle,
				   QueryTable,
				   NULL,
				   NULL);
   NtClose(KeyHandle);
   
   return Status;
}


NTSTATUS STDCALL
RtlSetTimeZoneInformation(PTIME_ZONE_INFORMATION TimeZoneInformation)
{
   HANDLE KeyHandle;
   ULONG Length;
   NTSTATUS Status;
   
   DPRINT("RtlSetTimeZoneInformation()\n");
   
   Status = RtlpGetRegistryHandle(RTL_REGISTRY_CONTROL,
				  L"TimeZoneInformation",
				  TRUE,
				  &KeyHandle);
   if (!NT_SUCCESS(Status))
     {
	DPRINT("RtlpGetRegistryHandle failed (Status %x)\n", Status);
	return Status;
     }
   
   Status = RtlWriteRegistryValue(RTL_REGISTRY_HANDLE,
				  (PWSTR)KeyHandle,
				  L"Bias",
				  REG_DWORD,
				  &TimeZoneInformation->Bias,
				  sizeof(LONG));
   if (!NT_SUCCESS(Status))
     {
	NtClose(KeyHandle);
	return Status;
     }
   
   Length = (wcslen(TimeZoneInformation->StandardName) + 1) * sizeof(WCHAR);
   Status = RtlWriteRegistryValue(RTL_REGISTRY_HANDLE,
				  (PWSTR)KeyHandle,
				  L"Standard Name",
				  REG_SZ,
				  TimeZoneInformation->StandardName,
				  Length);
   if (!NT_SUCCESS(Status))
     {
	NtClose(KeyHandle);
	return Status;
     }
   
   Status = RtlWriteRegistryValue(RTL_REGISTRY_HANDLE,
				  (PWSTR)KeyHandle,
				  L"Standard Bias",
				  REG_DWORD,
				  &TimeZoneInformation->StandardBias,
				  sizeof(LONG));
   if (!NT_SUCCESS(Status))
     {
	NtClose(KeyHandle);
	return Status;
     }
   
   Status = RtlWriteRegistryValue(RTL_REGISTRY_HANDLE,
				  (PWSTR)KeyHandle,
				  L"Standard Start",
				  REG_BINARY,
				  &TimeZoneInformation->StandardDate,
				  sizeof(SYSTEMTIME));
   if (!NT_SUCCESS(Status))
     {
	NtClose(KeyHandle);
	return Status;
     }
   
   Length = (wcslen(TimeZoneInformation->DaylightName) + 1) * sizeof(WCHAR);
   Status = RtlWriteRegistryValue(RTL_REGISTRY_HANDLE,
				  (PWSTR)KeyHandle,
				  L"Daylight Name",
				  REG_SZ,
				  TimeZoneInformation->DaylightName,
				  Length);
   if (!NT_SUCCESS(Status))
     {
	NtClose(KeyHandle);
	return Status;
     }
   
   Status = RtlWriteRegistryValue(RTL_REGISTRY_HANDLE,
				  (PWSTR)KeyHandle,
				  L"Daylight Bias",
				  REG_DWORD,
				  &TimeZoneInformation->DaylightBias,
				  sizeof(LONG));
   if (!NT_SUCCESS(Status))
     {
	NtClose(KeyHandle);
	return Status;
     }
   
   Status = RtlWriteRegistryValue(RTL_REGISTRY_HANDLE,
				  (PWSTR)KeyHandle,
				  L"Daylight Start",
				  REG_BINARY,
				  &TimeZoneInformation->DaylightDate,
				  sizeof(SYSTEMTIME));
   
   NtClose(KeyHandle);
   
   return Status;
}

/* EOF */
