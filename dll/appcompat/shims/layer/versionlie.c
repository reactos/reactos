/*
 * PROJECT:     ReactOS 'Layers' Shim library
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Version lie shims
 * COPYRIGHT:   Copyright 2016,2017 Mark Jansen (mark.jansen@reactos.org)
 */

#define WIN32_NO_STATUS
#include <windef.h>
#include <winbase.h>
#include <shimlib.h>
#include <strsafe.h>

/* Generic version info to use with the shims */
typedef struct VersionLieInfo
{
    DWORD FullVersion;
    WORD dwMajorVersion;
    WORD dwMinorVersion;
    DWORD dwBuildNumber;
    WORD dwPlatformId;
    WORD wServicePackMajor;
    WORD wServicePackMinor;
    char szCSDVersionA[20];
    WCHAR szCSDVersionW[20];
} VersionLieInfo;

VersionLieInfo g_Win95 = { 0xC3B60004, 4, 0, 950, VER_PLATFORM_WIN32_WINDOWS, 0, 0 };
VersionLieInfo g_WinNT4SP5 = { 0x05650004, 4, 0, 1381, VER_PLATFORM_WIN32_NT, 5, 0 };
VersionLieInfo g_Win98 = { 0xC0000A04, 4, 10, 0x040A08AE, VER_PLATFORM_WIN32_WINDOWS, 0, 0 };

VersionLieInfo g_Win2000 = { 0x08930005, 5, 0, 2195, VER_PLATFORM_WIN32_NT, 0, 0 };
VersionLieInfo g_Win2000SP1 = { 0x08930005, 5, 0, 2195, VER_PLATFORM_WIN32_NT, 1, 0 };
VersionLieInfo g_Win2000SP2 = { 0x08930005, 5, 0, 2195, VER_PLATFORM_WIN32_NT, 2, 0 };
VersionLieInfo g_Win2000SP3 = { 0x08930005, 5, 0, 2195, VER_PLATFORM_WIN32_NT, 3, 0 };

VersionLieInfo g_WinXP = { 0x0a280105, 5, 1, 2600, VER_PLATFORM_WIN32_NT, 0, 0 };
VersionLieInfo g_WinXPSP1 = { 0x0a280105, 5, 1, 2600, VER_PLATFORM_WIN32_NT, 1, 0 };
VersionLieInfo g_WinXPSP2 = { 0x0a280105, 5, 1, 2600, VER_PLATFORM_WIN32_NT, 2, 0 };
VersionLieInfo g_WinXPSP3 = { 0x0a280105, 5, 1, 2600, VER_PLATFORM_WIN32_NT, 3, 0 };

VersionLieInfo g_Win2k3RTM = { 0x0ece0205, 5, 2, 3790, VER_PLATFORM_WIN32_NT, 0, 0 };
VersionLieInfo g_Win2k3SP1 = { 0x0ece0205, 5, 2, 3790, VER_PLATFORM_WIN32_NT, 1, 0 };

VersionLieInfo g_WinVistaRTM = { 0x17700006, 6, 0, 6000, VER_PLATFORM_WIN32_NT, 0, 0 };
VersionLieInfo g_WinVistaSP1 = { 0x17710006, 6, 0, 6001, VER_PLATFORM_WIN32_NT, 1, 0 };
VersionLieInfo g_WinVistaSP2 = { 0x17720006, 6, 0, 6002, VER_PLATFORM_WIN32_NT, 2, 0 };

VersionLieInfo g_Win7RTM = { 0x1db00106, 6, 1, 7600, VER_PLATFORM_WIN32_NT, 0, 0 };
VersionLieInfo g_Win7SP1 = { 0x1db10106, 6, 1, 7601, VER_PLATFORM_WIN32_NT, 1, 0 };

VersionLieInfo g_Win8RTM = { 0x23f00206, 6, 2, 9200, VER_PLATFORM_WIN32_NT, 0, 0 };
VersionLieInfo g_Win81RTM = { 0x25800306, 6, 3, 9600, VER_PLATFORM_WIN32_NT, 0, 0 };

VersionLieInfo g_Win10RTM = { 0x47ba000a, 10, 0, 18362, VER_PLATFORM_WIN32_NT, 0, 0 };

VersionLieInfo g_Win2k16RTM = { 0x3fab000a, 10, 0, 16299, VER_PLATFORM_WIN32_NT, 0, 0 };
VersionLieInfo g_Win2k19RTM = { 0x4563000a, 10, 0, 17763, VER_PLATFORM_WIN32_NT, 0, 0 };

/* Fill the OSVERSIONINFO[EX][W|A] struct with the info from the generic VersionLieInfo */

BOOL FakeVersion(LPOSVERSIONINFOEXA pResult, VersionLieInfo* pFake)
{
    if (pResult->dwOSVersionInfoSize == sizeof(OSVERSIONINFOEXA) || pResult->dwOSVersionInfoSize == sizeof(OSVERSIONINFOA) ||
        pResult->dwOSVersionInfoSize == sizeof(OSVERSIONINFOEXW) || pResult->dwOSVersionInfoSize == sizeof(OSVERSIONINFOW))
    {
        pResult->dwMajorVersion = pFake->dwMajorVersion;
        pResult->dwMinorVersion = pFake->dwMinorVersion;
        pResult->dwBuildNumber = pFake->dwBuildNumber;
        pResult->dwPlatformId = pFake->dwPlatformId;
        if (pResult->dwOSVersionInfoSize == sizeof(OSVERSIONINFOEXA) || pResult->dwOSVersionInfoSize == sizeof(OSVERSIONINFOA))
        {
            if (FAILED(StringCbCopyA(pResult->szCSDVersion, sizeof(pResult->szCSDVersion), pFake->szCSDVersionA)))
                return FALSE;
            if (pResult->dwOSVersionInfoSize == sizeof(OSVERSIONINFOEXA) && pFake->dwPlatformId != VER_PLATFORM_WIN32_WINDOWS)
            {
                pResult->wServicePackMajor = pFake->wServicePackMajor;
                pResult->wServicePackMinor = pFake->wServicePackMinor;
            }
        }
        else
        {
            LPOSVERSIONINFOEXW pResultW = (LPOSVERSIONINFOEXW)pResult;
            if (FAILED(StringCbCopyW(pResultW->szCSDVersion, sizeof(pResultW->szCSDVersion), pFake->szCSDVersionW)))
                return FALSE;
            if (pResultW->dwOSVersionInfoSize == sizeof(OSVERSIONINFOEXW) && pFake->dwPlatformId != VER_PLATFORM_WIN32_WINDOWS)
            {
                pResultW->wServicePackMajor = pFake->wServicePackMajor;
                pResultW->wServicePackMinor = pFake->wServicePackMinor;
            }
        }
        return TRUE;
    }
    return FALSE;
}

typedef BOOL(WINAPI* GETVERSIONEXAPROC)(LPOSVERSIONINFOEXA);


#define SHIM_NS         Win95VersionLie
#include <setup_shim.inl>

DWORD WINAPI SHIM_OBJ_NAME(APIHook_GetVersion)()
{
    return g_Win95.FullVersion;
}

BOOL WINAPI SHIM_OBJ_NAME(APIHook_GetVersionExA)(LPOSVERSIONINFOEXA lpOsVersionInfo)
{
    if (CALL_SHIM(1, GETVERSIONEXAPROC)(lpOsVersionInfo))
    {
        return FakeVersion(lpOsVersionInfo, &g_Win95);
    }
    return FALSE;
}

/* We do not call about the actual type, FakeVersion will correctly handle it either way */
BOOL WINAPI SHIM_OBJ_NAME(APIHook_GetVersionExW)(LPOSVERSIONINFOEXA lpOsVersionInfo)
{
    if (CALL_SHIM(2, GETVERSIONEXAPROC)(lpOsVersionInfo))
    {
        return FakeVersion(lpOsVersionInfo, &g_Win95);
    }
    return FALSE;
}

#define SHIM_NUM_HOOKS  3
#define SHIM_SETUP_HOOKS \
    SHIM_HOOK(0, "KERNEL32.DLL", "GetVersion", SHIM_OBJ_NAME(APIHook_GetVersion)) \
    SHIM_HOOK(1, "KERNEL32.DLL", "GetVersionExA", SHIM_OBJ_NAME(APIHook_GetVersionExA)) \
    SHIM_HOOK(2, "KERNEL32.DLL", "GetVersionExW", SHIM_OBJ_NAME(APIHook_GetVersionExW))

#include <implement_shim.inl>




#define SHIM_NS         Win98VersionLie
#include <setup_shim.inl>

DWORD WINAPI SHIM_OBJ_NAME(APIHook_GetVersion)()
{
    return g_Win98.FullVersion;
}

BOOL WINAPI SHIM_OBJ_NAME(APIHook_GetVersionExA)(LPOSVERSIONINFOEXA lpOsVersionInfo)
{
    if (CALL_SHIM(0, GETVERSIONEXAPROC)(lpOsVersionInfo))
    {
        return FakeVersion(lpOsVersionInfo, &g_Win98);
    }
    return FALSE;
}

/* We do not call about the actual type, FakeVersion will correctly handle it either way */
BOOL WINAPI SHIM_OBJ_NAME(APIHook_GetVersionExW)(LPOSVERSIONINFOEXA lpOsVersionInfo)
{
    if (CALL_SHIM(1, GETVERSIONEXAPROC)(lpOsVersionInfo))
    {
        return FakeVersion(lpOsVersionInfo, &g_Win98);
    }
    return FALSE;
}

#define SHIM_NUM_HOOKS  3
#define SHIM_SETUP_HOOKS \
    SHIM_HOOK(2, "KERNEL32.DLL", "GetVersion", SHIM_OBJ_NAME(APIHook_GetVersion)) \
    SHIM_HOOK(0, "KERNEL32.DLL", "GetVersionExA", SHIM_OBJ_NAME(APIHook_GetVersionExA)) \
    SHIM_HOOK(1, "KERNEL32.DLL", "GetVersionExW", SHIM_OBJ_NAME(APIHook_GetVersionExW))

#include <implement_shim.inl>



#define SHIM_NS         WinNT4SP5VersionLie
#define VERSION_INFO    g_WinNT4SP5
#include "versionlie.inl"


#define SHIM_NS         Win2000VersionLie
#define VERSION_INFO    g_Win2000
#include "versionlie.inl"

#define SHIM_NS         Win2000SP1VersionLie
#define VERSION_INFO    g_Win2000SP1
#include "versionlie.inl"

#define SHIM_NS         Win2000SP2VersionLie
#define VERSION_INFO    g_Win2000SP2
#include "versionlie.inl"

#define SHIM_NS         Win2000SP3VersionLie
#define VERSION_INFO    g_Win2000SP3
#include "versionlie.inl"


#define SHIM_NS         WinXPVersionLie
#define VERSION_INFO    g_WinXP
#include "versionlie.inl"

#define SHIM_NS         WinXPSP1VersionLie
#define VERSION_INFO    g_WinXPSP1
#include "versionlie.inl"

#define SHIM_NS         WinXPSP2VersionLie
#define VERSION_INFO    g_WinXPSP2
#include "versionlie.inl"

#define SHIM_NS         WinXPSP3VersionLie
#define VERSION_INFO    g_WinXPSP3
#include "versionlie.inl"


#define SHIM_NS         Win2k3RTMVersionLie
#define VERSION_INFO    g_Win2k3RTM
#include "versionlie.inl"

#define SHIM_NS         Win2k3SP1VersionLie
#define VERSION_INFO    g_Win2k3SP1
#include "versionlie.inl"


#define SHIM_NS         VistaRTMVersionLie
#define VERSION_INFO    g_WinVistaRTM
#include "versionlie.inl"

#define SHIM_NS         VistaSP1VersionLie
#define VERSION_INFO    g_WinVistaSP1
#include "versionlie.inl"

#define SHIM_NS         VistaSP2VersionLie
#define VERSION_INFO    g_WinVistaSP2
#include "versionlie.inl"


#define SHIM_NS         Win7RTMVersionLie
#define VERSION_INFO    g_Win7RTM
#include "versionlie.inl"

#define SHIM_NS         Win7SP1VersionLie
#define VERSION_INFO    g_Win7SP1
#include "versionlie.inl"


#define SHIM_NS         Win8RTMVersionLie
#define VERSION_INFO    g_Win8RTM
#include "versionlie.inl"

#define SHIM_NS         Win81RTMVersionLie
#define VERSION_INFO    g_Win81RTM
#include "versionlie.inl"


#define SHIM_NS         Win10RTMVersionLie
#define VERSION_INFO    g_Win10RTM
#include "versionlie.inl"


#define SHIM_NS         Win2k16RTMVersionLie
#define VERSION_INFO    g_Win2k16RTM
#include "versionlie.inl"


#define SHIM_NS         Win2k19RTMVersionLie
#define VERSION_INFO    g_Win2k19RTM
#include "versionlie.inl"
