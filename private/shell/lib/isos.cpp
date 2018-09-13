#include "proj.h"

/*----------------------------------------------------------
Purpose: Returns TRUE/FALSE if the platform is the given OS_ value.

*/
STDAPI_(BOOL) staticIsOS(DWORD dwOS)
{
    BOOL bRet;
    static OSVERSIONINFOA s_osvi;
    static BOOL s_bVersionCached = FALSE;

    if (!s_bVersionCached)
    {
        s_bVersionCached = TRUE;

        s_osvi.dwOSVersionInfoSize = sizeof(s_osvi);
        GetVersionExA(&s_osvi);
    }

    switch (dwOS)
    {
    case OS_WINDOWS:
        bRet = (VER_PLATFORM_WIN32_WINDOWS == s_osvi.dwPlatformId);
        break;

    case OS_NT:
#ifndef UNIX
        bRet = (VER_PLATFORM_WIN32_NT == s_osvi.dwPlatformId);
#else
        bRet = ((VER_PLATFORM_WIN32_NT == s_osvi.dwPlatformId) ||
                (VER_PLATFORM_WIN32_UNIX == s_osvi.dwPlatformId));
#endif
        break;

    case OS_WIN95:
        bRet = (VER_PLATFORM_WIN32_WINDOWS == s_osvi.dwPlatformId &&
                s_osvi.dwMajorVersion >= 4);
        break;

    case OS_MEMPHIS:
        bRet = (VER_PLATFORM_WIN32_WINDOWS == s_osvi.dwPlatformId &&
                (s_osvi.dwMajorVersion > 4 || 
                 s_osvi.dwMajorVersion == 4 && s_osvi.dwMinorVersion >= 10));
        break;

    case OS_MEMPHIS_GOLD:
        bRet = (VER_PLATFORM_WIN32_WINDOWS == s_osvi.dwPlatformId &&
                s_osvi.dwMajorVersion == 4 && s_osvi.dwMinorVersion == 10 &&
                LOWORD(s_osvi.dwBuildNumber) == 1998);
        break;

    case OS_NT4:
#ifndef UNIX
        bRet = (VER_PLATFORM_WIN32_NT == s_osvi.dwPlatformId &&
#else
        bRet = ((VER_PLATFORM_WIN32_NT == s_osvi.dwPlatformId ||
                (VER_PLATFORM_WIN32_UNIX == s_osvi.dwPlatformId)) &&
#endif
                s_osvi.dwMajorVersion >= 4);
        break;

    case OS_NT5:
        bRet = (VER_PLATFORM_WIN32_NT == s_osvi.dwPlatformId &&
                s_osvi.dwMajorVersion >= 5);
        break;

    default:
        bRet = FALSE;
        break;
    }

    return bRet;
}   
