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

/*
 * @implemented
 */
DWORD
WINAPI
GetVersion(VOID)
{
    PPEB Peb = NtCurrentPeb();

    return (DWORD)( ((Peb->OSPlatformId ^ 2) << 30) |
                     (Peb->OSBuildNumber     << 16) |
                     (Peb->OSMinorVersion    << 8 ) |
                      Peb->OSMajorVersion );
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
            DPRINT1("VerifyVersionInfo -- Version mismatch\n");
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
