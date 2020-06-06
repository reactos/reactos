/*
 * PROJECT:     ReactOS TimeZone Utilities Library
 * LICENSE:     GPL-2.0 (https://spdx.org/licenses/GPL-2.0)
 * PURPOSE:     Provides time-zone utility wrappers around Win32 functions,
 *              that are used by different ReactOS modules such as
 *              timedate.cpl, syssetup.dll.
 * COPYRIGHT:   Copyright 2004-2005 Eric Kohl
 *              Copyright 2016 Carlo Bramini
 *              Copyright 2020 Hermes Belusca-Maito
 */

#include <stdlib.h>
#include <windef.h>
#include <winbase.h>
#include <winreg.h>

#include "tzlib.h"

BOOL
GetTimeZoneListIndex(
    IN OUT PULONG pIndex)
{
    LONG lError;
    HKEY hKey;
    DWORD dwType;
    DWORD dwValueSize;
    DWORD Length;
    LPWSTR Buffer;
    LPWSTR Ptr, End;
    BOOL bFound = FALSE;
    unsigned long iLanguageID;
    WCHAR szLanguageIdString[9];

    if (*pIndex == -1)
    {
        *pIndex = 85; /* fallback to GMT time zone */

        lError = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                               L"SYSTEM\\CurrentControlSet\\Control\\NLS\\Language",
                               0,
                               KEY_QUERY_VALUE,
                               &hKey);
        if (lError != ERROR_SUCCESS)
        {
            return FALSE;
        }

        dwValueSize = sizeof(szLanguageIdString);
        lError = RegQueryValueExW(hKey,
                                  L"Default",
                                  NULL,
                                  NULL,
                                  (LPBYTE)szLanguageIdString,
                                  &dwValueSize);
        if (lError != ERROR_SUCCESS)
        {
            RegCloseKey(hKey);
            return FALSE;
        }

        iLanguageID = wcstoul(szLanguageIdString, NULL, 16);
        RegCloseKey(hKey);
    }
    else
    {
        iLanguageID = *pIndex;
    }

    lError = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                           L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Time Zones",
                           0,
                           KEY_QUERY_VALUE,
                           &hKey);
    if (lError != ERROR_SUCCESS)
    {
        return FALSE;
    }

    dwValueSize = 0;
    lError = RegQueryValueExW(hKey,
                              L"IndexMapping",
                              NULL,
                              &dwType,
                              NULL,
                              &dwValueSize);
    if ((lError != ERROR_SUCCESS) || (dwType != REG_MULTI_SZ))
    {
        RegCloseKey(hKey);
        return FALSE;
    }

    Buffer = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwValueSize);
    if (Buffer == NULL)
    {
        RegCloseKey(hKey);
        return FALSE;
    }

    lError = RegQueryValueExW(hKey,
                              L"IndexMapping",
                              NULL,
                              &dwType,
                              (LPBYTE)Buffer,
                              &dwValueSize);

    RegCloseKey(hKey);

    if ((lError != ERROR_SUCCESS) || (dwType != REG_MULTI_SZ))
    {
        HeapFree(GetProcessHeap(), 0, Buffer);
        return FALSE;
    }

    Ptr = Buffer;
    while (*Ptr != 0)
    {
        Length = wcslen(Ptr);
        if (wcstoul(Ptr, NULL, 16) == iLanguageID)
            bFound = TRUE;

        Ptr = Ptr + Length + 1;
        if (*Ptr == 0)
            break;

        if (bFound)
        {
            *pIndex = wcstoul(Ptr, &End, 10);
            HeapFree(GetProcessHeap(), 0, Buffer);
            return TRUE;
        }

        Length = wcslen(Ptr);
        Ptr = Ptr + Length + 1;
    }

    HeapFree(GetProcessHeap(), 0, Buffer);
    return FALSE;
}

LONG
QueryTimeZoneData(
    IN HKEY hZoneKey,
    OUT PULONG Index OPTIONAL,
    OUT PREG_TZI_FORMAT TimeZoneInfo,
    OUT PWCHAR Description OPTIONAL,
    IN OUT PULONG DescriptionSize OPTIONAL,
    OUT PWCHAR StandardName OPTIONAL,
    IN OUT PULONG StandardNameSize OPTIONAL,
    OUT PWCHAR DaylightName OPTIONAL,
    IN OUT PULONG DaylightNameSize OPTIONAL)
{
    LONG lError;
    DWORD dwValueSize;

    if (Index)
    {
        dwValueSize = sizeof(*Index);
        lError = RegQueryValueExW(hZoneKey,
                                  L"Index",
                                  NULL,
                                  NULL,
                                  (LPBYTE)Index,
                                  &dwValueSize);
        if (lError != ERROR_SUCCESS)
            *Index = 0;
    }

    /* The time zone information structure is mandatory for a valid time zone */
    dwValueSize = sizeof(*TimeZoneInfo);
    lError = RegQueryValueExW(hZoneKey,
                              L"TZI",
                              NULL,
                              NULL,
                              (LPBYTE)TimeZoneInfo,
                              &dwValueSize);
    if (lError != ERROR_SUCCESS)
        return lError;

    if (Description && DescriptionSize && *DescriptionSize > 0)
    {
        lError = RegQueryValueExW(hZoneKey,
                                  L"Display",
                                  NULL,
                                  NULL,
                                  (LPBYTE)Description,
                                  DescriptionSize);
        if (lError != ERROR_SUCCESS)
            *Description = 0;
    }

    if (StandardName && StandardNameSize && *StandardNameSize > 0)
    {
        lError = RegQueryValueExW(hZoneKey,
                                  L"Std",
                                  NULL,
                                  NULL,
                                  (LPBYTE)StandardName,
                                  StandardNameSize);
        if (lError != ERROR_SUCCESS)
            *StandardName = 0;
    }

    if (DaylightName && DaylightNameSize && *DaylightNameSize > 0)
    {
        lError = RegQueryValueExW(hZoneKey,
                                  L"Dlt",
                                  NULL,
                                  NULL,
                                  (LPBYTE)DaylightName,
                                  DaylightNameSize);
        if (lError != ERROR_SUCCESS)
            *DaylightName = 0;
    }

    return ERROR_SUCCESS;
}

//
// NOTE: Very similar to the EnumDynamicTimeZoneInformation() function
// introduced in Windows 8.
//
VOID
EnumerateTimeZoneList(
    IN PENUM_TIMEZONE_CALLBACK Callback,
    IN PVOID Context OPTIONAL)
{
    LONG lError;
    HKEY hZonesKey;
    HKEY hZoneKey;
    DWORD dwIndex;
    DWORD dwNameSize;
    WCHAR szKeyName[256];

    /* Open the registry key containing the list of time zones */
    lError = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                           L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Time Zones",
                           0,
                           KEY_ENUMERATE_SUB_KEYS,
                           &hZonesKey);
    if (lError != ERROR_SUCCESS)
        return;

    /* Enumerate it */
    for (dwIndex = 0; ; dwIndex++)
    {
        dwNameSize = sizeof(szKeyName);
        lError = RegEnumKeyExW(hZonesKey,
                               dwIndex,
                               szKeyName,
                               &dwNameSize,
                               NULL,
                               NULL,
                               NULL,
                               NULL);
        // if (lError != ERROR_SUCCESS && lError != ERROR_MORE_DATA)
        if (lError == ERROR_NO_MORE_ITEMS)
            break;

        /* Open the time zone sub-key */
        if (RegOpenKeyExW(hZonesKey,
                          szKeyName,
                          0,
                          KEY_QUERY_VALUE,
                          &hZoneKey))
        {
            /* We failed, continue with another sub-key */
            continue;
        }

        /* Call the user-provided callback */
        lError = Callback(hZoneKey, Context);
        // lError = QueryTimeZoneData(hZoneKey, Context);

        RegCloseKey(hZoneKey);
    }

    RegCloseKey(hZonesKey);
}

// Returns TRUE if AutoDaylight is ON.
// Returns FALSE if AutoDaylight is OFF.
BOOL
GetAutoDaylight(VOID)
{
    LONG lError;
    HKEY hKey;
    DWORD dwType;
    DWORD dwDisabled;
    DWORD dwValueSize;

    lError = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                           L"SYSTEM\\CurrentControlSet\\Control\\TimeZoneInformation",
                           0,
                           KEY_QUERY_VALUE,
                           &hKey);
    if (lError != ERROR_SUCCESS)
        return FALSE;

    // NOTE: On Vista+: REG_DWORD "DynamicDaylightTimeDisabled"
    dwValueSize = sizeof(dwDisabled);
    lError = RegQueryValueExW(hKey,
                              L"DisableAutoDaylightTimeSet",
                              NULL,
                              &dwType,
                              (LPBYTE)&dwDisabled,
                              &dwValueSize);

    RegCloseKey(hKey);

    if ((lError != ERROR_SUCCESS) || (dwType != REG_DWORD) || (dwValueSize != sizeof(dwDisabled)))
    {
        /*
         * The call failed (non zero) because the registry value isn't available,
         * which means auto-daylight shouldn't be disabled.
         */
        dwDisabled = FALSE;
    }

    return !dwDisabled;
}

VOID
SetAutoDaylight(
    IN BOOL EnableAutoDaylightTime)
{
    LONG lError;
    HKEY hKey;
    DWORD dwDisabled = TRUE;

    lError = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                           L"SYSTEM\\CurrentControlSet\\Control\\TimeZoneInformation",
                           0,
                           KEY_SET_VALUE,
                           &hKey);
    if (lError != ERROR_SUCCESS)
        return;

    if (!EnableAutoDaylightTime)
    {
        /* Auto-Daylight disabled: set the value to TRUE */
        // NOTE: On Vista+: REG_DWORD "DynamicDaylightTimeDisabled"
        RegSetValueExW(hKey,
                       L"DisableAutoDaylightTimeSet",
                       0,
                       REG_DWORD,
                       (LPBYTE)&dwDisabled,
                       sizeof(dwDisabled));
    }
    else
    {
        /* Auto-Daylight enabled: just delete the value */
        RegDeleteValueW(hKey, L"DisableAutoDaylightTimeSet");
    }

    RegCloseKey(hKey);
}
