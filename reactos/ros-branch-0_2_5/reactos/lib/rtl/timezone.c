/* $Id: timezone.c,v 1.1 2004/05/31 19:29:02 gdalsnes Exp $
 *
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * PURPOSE:           Timezone functions
 * FILE:              lib/rtl/timezone.c
 * PROGRAMER:         Eric Kohl
 * REVISION HISTORY:
 *                    29/05/2001: Created
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <ntos/registry.h>
#include <ntos/time.h>

#define NDEBUG
#include <debug.h>


/* FUNCTIONS *****************************************************************/

/*
 * @implemented
 */
NTSTATUS STDCALL
RtlQueryTimeZoneInformation(PTIME_ZONE_INFORMATION TimeZoneInformation)
{
   RTL_QUERY_REGISTRY_TABLE QueryTable[8];
   UNICODE_STRING StandardName;
   UNICODE_STRING DaylightName;
   NTSTATUS Status;

   DPRINT("RtlQueryTimeZoneInformation()\n");

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

   Status = RtlQueryRegistryValues(RTL_REGISTRY_CONTROL,
                                   L"TimeZoneInformation",
                                   QueryTable,
                                   NULL,
                                   NULL);

   return Status;
}


/*
 * @implemented
 */
NTSTATUS STDCALL
RtlSetTimeZoneInformation(PTIME_ZONE_INFORMATION TimeZoneInformation)
{
   ULONG Length;
   NTSTATUS Status;

   DPRINT("RtlSetTimeZoneInformation()\n");

   Status = RtlWriteRegistryValue(RTL_REGISTRY_CONTROL,
                                  L"TimeZoneInformation",
                                  L"Bias",
                                  REG_DWORD,
                                  &TimeZoneInformation->Bias,
                                  sizeof(LONG));
   if (!NT_SUCCESS(Status))
   {
      return Status;
   }

   Length = (wcslen(TimeZoneInformation->StandardName) + 1) * sizeof(WCHAR);
   Status = RtlWriteRegistryValue(RTL_REGISTRY_CONTROL,
                                  L"TimeZoneInformation",
                                  L"Standard Name",
                                  REG_SZ,
                                  TimeZoneInformation->StandardName,
                                  Length);
   if (!NT_SUCCESS(Status))
   {
      return Status;
   }

   Status = RtlWriteRegistryValue(RTL_REGISTRY_CONTROL,
                                  L"TimeZoneInformation",
                                  L"Standard Bias",
                                  REG_DWORD,
                                  &TimeZoneInformation->StandardBias,
                                  sizeof(LONG));
   if (!NT_SUCCESS(Status))
   {
      return Status;
   }

   Status = RtlWriteRegistryValue(RTL_REGISTRY_CONTROL,
                                  L"TimeZoneInformation",
                                  L"Standard Start",
                                  REG_BINARY,
                                  &TimeZoneInformation->StandardDate,
                                  sizeof(SYSTEMTIME));
   if (!NT_SUCCESS(Status))
   {
      return Status;
   }

   Length = (wcslen(TimeZoneInformation->DaylightName) + 1) * sizeof(WCHAR);
   Status = RtlWriteRegistryValue(RTL_REGISTRY_CONTROL,
                                  L"TimeZoneInformation",
                                  L"Daylight Name",
                                  REG_SZ,
                                  TimeZoneInformation->DaylightName,
                                  Length);
   if (!NT_SUCCESS(Status))
   {
      return Status;
   }

   Status = RtlWriteRegistryValue(RTL_REGISTRY_CONTROL,
                                  L"TimeZoneInformation",
                                  L"Daylight Bias",
                                  REG_DWORD,
                                  &TimeZoneInformation->DaylightBias,
                                  sizeof(LONG));
   if (!NT_SUCCESS(Status))
   {
      return Status;
   }

   Status = RtlWriteRegistryValue(RTL_REGISTRY_CONTROL,
                                  L"TimeZoneInformation",
                                  L"Daylight Start",
                                  REG_BINARY,
                                  &TimeZoneInformation->DaylightDate,
                                  sizeof(SYSTEMTIME));

   return Status;
}

/* EOF */
