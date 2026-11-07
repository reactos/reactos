/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS system libraries
 * PURPOSE:           Timezone functions
 * FILE:              lib/rtl/timezone.c
 * PROGRAMER:         Eric Kohl
 */

/* INCLUDES *****************************************************************/

#include <rtl.h>

#define NDEBUG
#include <debug.h>

/* FUNCTIONS ***************************************************************/

/*
 * @implemented
 */
NTSTATUS
NTAPI
RtlQueryTimeZoneInformation(PRTL_TIME_ZONE_INFORMATION TimeZoneInformation)
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

    QueryTable[1].Name = L"StandardName";
    QueryTable[1].Flags = RTL_QUERY_REGISTRY_DIRECT;
    QueryTable[1].EntryContext = &StandardName;

    QueryTable[2].Name = L"StandardBias";
    QueryTable[2].Flags = RTL_QUERY_REGISTRY_DIRECT;
    QueryTable[2].EntryContext = &TimeZoneInformation->StandardBias;

    QueryTable[3].Name = L"StandardStart";
    QueryTable[3].Flags = RTL_QUERY_REGISTRY_DIRECT;
    QueryTable[3].EntryContext = &TimeZoneInformation->StandardDate;

    QueryTable[4].Name = L"DaylightName";
    QueryTable[4].Flags = RTL_QUERY_REGISTRY_DIRECT;
    QueryTable[4].EntryContext = &DaylightName;

    QueryTable[5].Name = L"DaylightBias";
    QueryTable[5].Flags = RTL_QUERY_REGISTRY_DIRECT;
    QueryTable[5].EntryContext = &TimeZoneInformation->DaylightBias;

    QueryTable[6].Name = L"DaylightStart";
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
NTSTATUS
NTAPI
RtlQueryDynamicTimeZoneInformation(PRTL_DYNAMIC_TIME_ZONE_INFORMATION TimeZoneInformation)
{
    RTL_QUERY_REGISTRY_TABLE QueryTable[10];
    UNICODE_STRING StandardName;
    UNICODE_STRING DaylightName;
    UNICODE_STRING TimeZoneKeyName;
    NTSTATUS Status;

    DPRINT("RtlQueryDynamicTimeZoneInformation()\n");

    RtlZeroMemory(QueryTable,
                  sizeof(QueryTable));

    StandardName.Length = 0;
    StandardName.MaximumLength = 32 * sizeof(WCHAR);
    StandardName.Buffer = TimeZoneInformation->StandardName;

    DaylightName.Length = 0;
    DaylightName.MaximumLength = 32 * sizeof(WCHAR);
    DaylightName.Buffer = TimeZoneInformation->DaylightName;

    TimeZoneKeyName.Length = 0;
    TimeZoneKeyName.MaximumLength = 128 * sizeof(WCHAR);
    TimeZoneKeyName.Buffer = TimeZoneInformation->TimeZoneKeyName;

    QueryTable[0].Name = L"Bias";
    QueryTable[0].Flags = RTL_QUERY_REGISTRY_DIRECT;
    QueryTable[0].EntryContext = &TimeZoneInformation->Bias;

    QueryTable[1].Name = L"StandardName";
    QueryTable[1].Flags = RTL_QUERY_REGISTRY_DIRECT;
    QueryTable[1].EntryContext = &StandardName;

    QueryTable[2].Name = L"StandardBias";
    QueryTable[2].Flags = RTL_QUERY_REGISTRY_DIRECT;
    QueryTable[2].EntryContext = &TimeZoneInformation->StandardBias;

    QueryTable[3].Name = L"StandardStart";
    QueryTable[3].Flags = RTL_QUERY_REGISTRY_DIRECT;
    QueryTable[3].EntryContext = &TimeZoneInformation->StandardDate;

    QueryTable[4].Name = L"DaylightName";
    QueryTable[4].Flags = RTL_QUERY_REGISTRY_DIRECT;
    QueryTable[4].EntryContext = &DaylightName;

    QueryTable[5].Name = L"DaylightBias";
    QueryTable[5].Flags = RTL_QUERY_REGISTRY_DIRECT;
    QueryTable[5].EntryContext = &TimeZoneInformation->DaylightBias;

    QueryTable[6].Name = L"DaylightStart";
    QueryTable[6].Flags = RTL_QUERY_REGISTRY_DIRECT;
    QueryTable[6].EntryContext = &TimeZoneInformation->DaylightDate;

    QueryTable[7].Name = L"TimeZoneKeyName";
    QueryTable[7].Flags = RTL_QUERY_REGISTRY_DIRECT;
    QueryTable[7].EntryContext = &TimeZoneKeyName;
    
    LONG DynamicDaylightTimeDisabled = 0;
    QueryTable[8].Name = L"DynamicDaylightTimeDisabled";
    QueryTable[8].Flags = RTL_QUERY_REGISTRY_DIRECT;
    QueryTable[8].EntryContext = &DynamicDaylightTimeDisabled;
    TimeZoneInformation->DynamicDaylightTimeDisabled = DynamicDaylightTimeDisabled ? 1 : 0;

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
NTSTATUS
NTAPI
RtlSetTimeZoneInformation(PRTL_TIME_ZONE_INFORMATION TimeZoneInformation)
{
    SIZE_T Length;
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
                                   L"StandardName",
                                   REG_SZ,
                                   TimeZoneInformation->StandardName,
                                   (ULONG)Length);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    Status = RtlWriteRegistryValue(RTL_REGISTRY_CONTROL,
                                   L"TimeZoneInformation",
                                   L"StandardBias",
                                   REG_DWORD,
                                   &TimeZoneInformation->StandardBias,
                                   sizeof(LONG));
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    Status = RtlWriteRegistryValue(RTL_REGISTRY_CONTROL,
                                   L"TimeZoneInformation",
                                   L"StandardStart",
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
                                   L"DaylightName",
                                   REG_SZ,
                                   TimeZoneInformation->DaylightName,
                                   (ULONG)Length);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    Status = RtlWriteRegistryValue(RTL_REGISTRY_CONTROL,
                                   L"TimeZoneInformation",
                                   L"DaylightBias",
                                   REG_DWORD,
                                   &TimeZoneInformation->DaylightBias,
                                   sizeof(LONG));
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    Status = RtlWriteRegistryValue(RTL_REGISTRY_CONTROL,
                                   L"TimeZoneInformation",
                                   L"DaylightStart",
                                   REG_BINARY,
                                   &TimeZoneInformation->DaylightDate,
                                   sizeof(SYSTEMTIME));

    /* SetTimeZoneInformation does not support dynamic daylight time */
    WCHAR EmptyString[] = L"";
    Status = RtlWriteRegistryValue(RTL_REGISTRY_CONTROL,
                                L"TimeZoneInformation",
                                L"TimeZoneKeyName",
                                REG_SZ,
                                &EmptyString,
                                sizeof(EmptyString));

    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    LONG Value = 1;
    Status = RtlWriteRegistryValue(RTL_REGISTRY_CONTROL,
                                   L"TimeZoneInformation",
                                   L"DynamicDaylightTimeDisabled",
                                   REG_DWORD,
                                   &Value,
                                   sizeof(Value));

    return Status;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
RtlSetDynamicTimeZoneInformation(PRTL_DYNAMIC_TIME_ZONE_INFORMATION TimeZoneInformation)
{
    SIZE_T Length;
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
                                   L"StandardName",
                                   REG_SZ,
                                   TimeZoneInformation->StandardName,
                                   (ULONG)Length);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    Status = RtlWriteRegistryValue(RTL_REGISTRY_CONTROL,
                                   L"TimeZoneInformation",
                                   L"StandardBias",
                                   REG_DWORD,
                                   &TimeZoneInformation->StandardBias,
                                   sizeof(LONG));
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    Status = RtlWriteRegistryValue(RTL_REGISTRY_CONTROL,
                                   L"TimeZoneInformation",
                                   L"StandardStart",
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
                                   L"DaylightName",
                                   REG_SZ,
                                   TimeZoneInformation->DaylightName,
                                   (ULONG)Length);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    Status = RtlWriteRegistryValue(RTL_REGISTRY_CONTROL,
                                   L"TimeZoneInformation",
                                   L"DaylightBias",
                                   REG_DWORD,
                                   &TimeZoneInformation->DaylightBias,
                                   sizeof(LONG));
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    Status = RtlWriteRegistryValue(RTL_REGISTRY_CONTROL,
                                   L"TimeZoneInformation",
                                   L"DaylightStart",
                                   REG_BINARY,
                                   &TimeZoneInformation->DaylightDate,
                                   sizeof(SYSTEMTIME));

    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    Length = (wcslen(TimeZoneInformation->TimeZoneKeyName) + 1) * sizeof(WCHAR);
    Status = RtlWriteRegistryValue(RTL_REGISTRY_CONTROL,
                                   L"TimeZoneInformation",
                                   L"TimeZoneKeyName",
                                   REG_SZ,
                                   &TimeZoneInformation->TimeZoneKeyName,
                                   Length);

    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    LONG DynamicDisabled = TimeZoneInformation->DynamicDaylightTimeDisabled == TRUE ? 1 : 0;

    Status = RtlWriteRegistryValue(RTL_REGISTRY_CONTROL,
                                   L"TimeZoneInformation",
                                   L"DynamicDaylightTimeDisabled",
                                   REG_DWORD,
                                   &DynamicDisabled,
                                   sizeof(DynamicDisabled));

    return Status;
}

/* EOF */
