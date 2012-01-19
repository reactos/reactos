/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            dll/win32/kernel32/misc/version.c
 * PURPOSE:         Version functions
 * PROGRAMMER:      Ariadne (ariadne@xs4all.nl)
                    Ged Murphy (gedmurphy@reactos.org)
 */

#include <k32.h>

#define NDEBUG
#include <debug.h>

/* FUNCTIONS ******************************************************************/

VOID
NTAPI
SetRosSpecificInfo(IN LPOSVERSIONINFOEXW VersionInformation)
{
    CHAR Buffer[sizeof(KEY_VALUE_PARTIAL_INFORMATION) + sizeof(DWORD)];
    PKEY_VALUE_PARTIAL_INFORMATION kvpInfo = (PVOID)Buffer;
    OBJECT_ATTRIBUTES ObjectAttributes;
    DWORD ReportAsWorkstation = 0;
    HANDLE hKey;
    DWORD dwSize;
    NTSTATUS Status;
    UNICODE_STRING KeyName = RTL_CONSTANT_STRING(L"\\Registry\\Machine\\SYSTEM\\CurrentControlSet\\Control\\ReactOS\\Settings\\Version");
    UNICODE_STRING ValName = RTL_CONSTANT_STRING(L"ReportAsWorkstation");

    InitializeObjectAttributes(&ObjectAttributes,
                               &KeyName,
                               OBJ_OPENIF | OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    /* Don't change anything if the key doesn't exist */
    Status = NtOpenKey(&hKey, KEY_READ, &ObjectAttributes);
    if (NT_SUCCESS(Status))
    {
        /* Get the value from the registry and make sure it's a 32-bit value */
        Status = NtQueryValueKey(hKey,
                                 &ValName,
                                 KeyValuePartialInformation,
                                 kvpInfo,
                                 sizeof(Buffer),
                                 &dwSize);
        if ((NT_SUCCESS(Status)) &&
            (kvpInfo->Type == REG_DWORD) &&
            (kvpInfo->DataLength == sizeof(DWORD)))
        {
            /* Is the value set? */
            ReportAsWorkstation = *(PULONG)kvpInfo->Data;
            if ((VersionInformation->wProductType == VER_NT_SERVER) &&
                (ReportAsWorkstation))
            {
                /* It is, modify the product type to report a workstation */
                VersionInformation->wProductType = VER_NT_WORKSTATION;
                DPRINT1("We modified the reported OS from NtProductServer to NtProductWinNt\n");
            }
        }

        /* Close the handle */
        NtClose(hKey);
     }
}

/*
 * @implemented
 */
DWORD
WINAPI
GetVersion(VOID)
{
    PPEB Peb = NtCurrentPeb();
    DWORD Result;

    Result = MAKELONG(MAKEWORD(Peb->OSMajorVersion, Peb->OSMinorVersion),
                      (Peb->OSPlatformId ^ 2) << 14);
    Result |= LOWORD(Peb->OSBuildNumber) << 16;
    return Result;
}

/*
 * @implemented
 */
BOOL
WINAPI
GetVersionExW(IN LPOSVERSIONINFOW lpVersionInformation)
{
    NTSTATUS Status;
    LPOSVERSIONINFOEXW lpVersionInformationEx;

    if ((lpVersionInformation->dwOSVersionInfoSize != sizeof(OSVERSIONINFOW)) &&
        (lpVersionInformation->dwOSVersionInfoSize != sizeof(OSVERSIONINFOEXW)))
    {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return FALSE;
    }

    Status = RtlGetVersion((PRTL_OSVERSIONINFOW)lpVersionInformation);
    if (Status == STATUS_SUCCESS)
    {
        if (lpVersionInformation->dwOSVersionInfoSize == sizeof(OSVERSIONINFOEXW))
        {
            lpVersionInformationEx = (PVOID)lpVersionInformation;
            lpVersionInformationEx->wReserved = 0;

            /* ReactOS specific changes */
            SetRosSpecificInfo(lpVersionInformationEx);
        }

        return TRUE;
    }

    return FALSE;
}

/*
 * @implemented
 */
BOOL
WINAPI
GetVersionExA(IN LPOSVERSIONINFOA lpVersionInformation)
{
    OSVERSIONINFOEXW VersionInformation;
    LPOSVERSIONINFOEXA lpVersionInformationEx;
    UNICODE_STRING CsdVersionW;
    NTSTATUS Status;
    ANSI_STRING CsdVersionA;

    if ((lpVersionInformation->dwOSVersionInfoSize != sizeof(OSVERSIONINFOA)) &&
        (lpVersionInformation->dwOSVersionInfoSize != sizeof(OSVERSIONINFOEXA)))
    {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return FALSE;
    }

    VersionInformation.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEXW);

    if (!GetVersionExW((LPOSVERSIONINFOW)&VersionInformation)) return FALSE;

    /* Copy back fields that match both supported structures */
    lpVersionInformation->dwMajorVersion = VersionInformation.dwMajorVersion;
    lpVersionInformation->dwMinorVersion = VersionInformation.dwMinorVersion;
    lpVersionInformation->dwBuildNumber = VersionInformation.dwBuildNumber;
    lpVersionInformation->dwPlatformId = VersionInformation.dwPlatformId;

    if (lpVersionInformation->dwOSVersionInfoSize == sizeof(OSVERSIONINFOEXA))
    {
        lpVersionInformationEx = (PVOID)lpVersionInformation;
        lpVersionInformationEx->wServicePackMajor = VersionInformation.wServicePackMajor;
        lpVersionInformationEx->wServicePackMinor = VersionInformation.wServicePackMinor;
        lpVersionInformationEx->wSuiteMask = VersionInformation.wSuiteMask;
        lpVersionInformationEx->wProductType = VersionInformation.wProductType;
        lpVersionInformationEx->wReserved = VersionInformation.wReserved;
    }

    /* Convert the CSD string */
    RtlInitEmptyAnsiString(&CsdVersionA,
                           lpVersionInformation->szCSDVersion,
                           sizeof(lpVersionInformation->szCSDVersion));
    RtlInitUnicodeString(&CsdVersionW, VersionInformation.szCSDVersion);
    Status = RtlUnicodeStringToAnsiString(&CsdVersionA, &CsdVersionW, FALSE);
    return (NT_SUCCESS(Status));
}

/*
 * @implemented
 */
BOOL
WINAPI
VerifyVersionInfoW(IN LPOSVERSIONINFOEXW lpVersionInformation,
                   IN DWORD dwTypeMask,
                   IN DWORDLONG dwlConditionMask)
{
    NTSTATUS Status;

    Status = RtlVerifyVersionInfo((PRTL_OSVERSIONINFOEXW)lpVersionInformation,
                                  dwTypeMask,
                                  dwlConditionMask);
    switch (Status)
    {
        case STATUS_INVALID_PARAMETER:
            SetLastError(ERROR_BAD_ARGUMENTS);
            return FALSE;

        case STATUS_REVISION_MISMATCH:
            DPRINT1("ReactOS returning version mismatch. Investigate!\n");
            SetLastError(ERROR_OLD_WIN_VERSION);
            return FALSE;

        default:
            /* RtlVerifyVersionInfo shouldn't report any other failure code! */
            ASSERT(NT_SUCCESS(Status));
            return TRUE;
    }
}

/*
 * @implemented
 */
BOOL
WINAPI
VerifyVersionInfoA(IN LPOSVERSIONINFOEXA lpVersionInformation,
                   IN DWORD dwTypeMask,
                   IN DWORDLONG dwlConditionMask)
{
    OSVERSIONINFOEXW viex;

    /* NOTE: szCSDVersion is ignored, we don't need to convert it to Unicode */
    viex.dwOSVersionInfoSize = sizeof(viex);
    viex.dwMajorVersion = lpVersionInformation->dwMajorVersion;
    viex.dwMinorVersion = lpVersionInformation->dwMinorVersion;
    viex.dwBuildNumber = lpVersionInformation->dwBuildNumber;
    viex.dwPlatformId = lpVersionInformation->dwPlatformId;
    viex.wServicePackMajor = lpVersionInformation->wServicePackMajor;
    viex.wServicePackMinor = lpVersionInformation->wServicePackMinor;
    viex.wSuiteMask = lpVersionInformation->wSuiteMask;
    viex.wProductType = lpVersionInformation->wProductType;
    viex.wReserved = lpVersionInformation->wReserved;
    return VerifyVersionInfoW(&viex, dwTypeMask, dwlConditionMask);
}
