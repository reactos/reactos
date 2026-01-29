/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            dll/win32/kernel32/client/versionansi.c
 * PURPOSE:         Version checking (ANSI)
 */

/* INCLUDES *****************************************************************/

#include <k32.h>

/* FUNCTIONS ****************************************************************/

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

/* EOF */
