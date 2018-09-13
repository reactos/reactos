//
//  APITHK.C
//
//  This file has API thunks that allow shlwapi to load and run on
//  multiple versions of NT or Win95.  Since this component needs
//  to load on the base-level NT 4.0 and Win95, any calls to system
//  APIs introduced in later OS versions must be done via GetProcAddress.
// 
//  Also, any code that may need to access data structures that are
//  post-4.0 specific can be added here.
//
//  NOTE:  this file does *not* use the standard precompiled header,
//         so it can set _WIN32_WINNT to a later version.
//


#include "priv.h"       // Don't use precompiled header here
#include <userenv.h>

//
// helper fn to see if this is a Terminal Server that was installed in "Remote Administration" mode
//
BOOL IsTSRemoteAdministrationMode()
{
    static BOOL s_bTSRemoteAdminCached = FALSE;
    static BOOL s_bIsTSRemoteAdmin = FALSE;

    if (!s_bTSRemoteAdminCached)
    {
        HKEY hkey;

        s_bTSRemoteAdminCached = TRUE;

        // can't be a "Remote Admin" TS machine if TS isin't even installed!
        if (IsOS(OS_WIN2000TERMINAL))
        {
            // "System\\CurrentControlSet\\Control\\Terminal Server" == REG_CONTROL_TSERVER (from sdk\inc\hydra\regapi.h)
            if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                             TEXT("System\\CurrentControlSet\\Control\\Terminal Server"),
                             0,
                             KEY_QUERY_VALUE,
                             &hkey) == ERROR_SUCCESS)
            {
                DWORD dwRegValue;
                DWORD cbValue = sizeof(dwRegValue);

                // "TSAppCompat" == REG_TERMSRV_APP_COMPAT (from sdk\inc\hydra\regapi.h)
                if (RegQueryValueEx(hkey,
                                    TEXT("TSAppCompat"),
                                    NULL,
                                    NULL,
                                    (LPBYTE)&dwRegValue,
                                    &cbValue) == ERROR_SUCCESS)
                {
                    // if the TSAppCompat flag is NOT set, then this is a TS Remote Admin type install
                    s_bIsTSRemoteAdmin = !dwRegValue;
                }

                RegCloseKey(hkey);
            }
        }
    }
    
    return s_bIsTSRemoteAdmin;
}


/*----------------------------------------------------------
Purpose: Returns TRUE/FALSE if the platform is the given OS_ value.

*/
LWSTDAPI_(BOOL) IsOS(DWORD dwOS)
{
    BOOL bRet;
    static OSVERSIONINFOEXA s_osvi = {0};
    static BOOL s_bVersionCached = FALSE;

    if (!s_bVersionCached)
    {
        s_bVersionCached = TRUE;
        s_osvi.dwOSVersionInfoSize = sizeof(s_osvi);
        if (!GetVersionExA((OSVERSIONINFOA*)&s_osvi))
        {
            // If it failed, it must be a down level platform
            s_osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOA);
            GetVersionExA((OSVERSIONINFOA*)&s_osvi);
        }
    }

    switch (dwOS)
    {
#ifndef UNIX
    case OS_TERMINALCLIENT:
        // WARNING: this will only return TRUE for remote TS sessions. If you want to
        // see if TS is enabled or if the user is on the TS console, the use OS_WIN2000TERMINAL
        bRet = GetSystemMetrics(SM_REMOTESESSION);
        break;

    case OS_WIN2000TERMINAL:
        // WARNING: this will return TRUE even for someone who is on the TS console!
        // if you want to know if this is a remote session, use OS_TERMINALCLIENT
        bRet = ((VER_SUITE_TERMINAL & s_osvi.wSuiteMask) &&
                s_osvi.dwMajorVersion >= 5);
        break;

    case OS_TERMINALREMOTEADMIN:
        // this checks to see if TS has been installed in the "Remote Administration" mode.
        bRet = IsTSRemoteAdministrationMode();
        break;

    case OS_WIN2000:
        bRet = ((VER_NT_WORKSTATION == s_osvi.wProductType ||
                 VER_NT_SERVER == s_osvi.wProductType ||
                 VER_NT_DOMAIN_CONTROLLER == s_osvi.wProductType) &&
                s_osvi.dwMajorVersion >= 5);
        break;

    case OS_WIN2000PRO:
        bRet = (VER_NT_WORKSTATION == s_osvi.wProductType &&
                s_osvi.dwMajorVersion == 5);
        break;

    case OS_WIN2000ADVSERVER:
        bRet = ((VER_NT_SERVER == s_osvi.wProductType ||
                VER_NT_DOMAIN_CONTROLLER == s_osvi.wProductType) &&
                s_osvi.dwMajorVersion == 5 &&
                (VER_SUITE_ENTERPRISE & s_osvi.wSuiteMask) &&
                !(VER_SUITE_DATACENTER & s_osvi.wSuiteMask));
        break;

    case OS_WIN2000DATACENTER:
        bRet = ((VER_NT_SERVER == s_osvi.wProductType ||
                VER_NT_DOMAIN_CONTROLLER == s_osvi.wProductType) &&
                s_osvi.dwMajorVersion == 5 &&
                (VER_SUITE_DATACENTER & s_osvi.wSuiteMask));
        break;

    case OS_WIN2000SERVER:
        bRet = ((VER_NT_SERVER == s_osvi.wProductType ||
                VER_NT_DOMAIN_CONTROLLER == s_osvi.wProductType) &&
                !(VER_SUITE_DATACENTER & s_osvi.wSuiteMask) && 
                !(VER_SUITE_ENTERPRISE & s_osvi.wSuiteMask)  && 
                s_osvi.dwMajorVersion == 5);
        break;

    case OS_WIN2000EMBED:
        bRet = (VER_SUITE_EMBEDDEDNT & s_osvi.wSuiteMask);
        break;

    case OS_SERVERAPPLIANCE:
        // Server Appliance can be only NT_SERVER (not a domain controller).
        bRet = ((VER_NT_SERVER == s_osvi.wProductType ) && 
                 s_osvi.dwMajorVersion == 5 &&
                 (VER_SUITE_ENTERPRISE & s_osvi.wSuiteMask) &&
                 (VER_SUITE_SERVERAPPLIANCE & s_osvi.wSuiteMask) &&
                !(VER_SUITE_DATACENTER & s_osvi.wSuiteMask));
        break;


#endif
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

    case OS_WIN95GOLD:
        bRet = (VER_PLATFORM_WIN32_WINDOWS == s_osvi.dwPlatformId &&
                s_osvi.dwMajorVersion == 4 && s_osvi.dwMinorVersion == 0 &&
                LOWORD(s_osvi.dwBuildNumber) == 1995);
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

typedef BOOL (* PFNGFAXW) (LPCWSTR, GET_FILEEX_INFO_LEVELS, LPVOID );

STDAPI_(BOOL) MyGetLastWriteTime (LPCWSTR pszPath, FILETIME *pft)
{
    if (g_bRunningOnNT5OrHigher)
    {
        static PFNGFAXW s_pfn = NULL;
        WIN32_FILE_ATTRIBUTE_DATA fad;

        if (!s_pfn)
        {
            s_pfn = (PFNGFAXW) GetProcAddress(GetModuleHandle(TEXT("kernel32")), "GetFileAttributesExW");
        }

        ASSERT(s_pfn);
        
        if (s_pfn && s_pfn(pszPath, GetFileExInfoStandard, &fad))
        {
            *pft = fad.ftLastWriteTime;
            return TRUE;
        }
    }
    else
    {
        HANDLE hFile = CreateFileW(pszPath, GENERIC_READ, FILE_SHARE_READ,
                NULL, OPEN_EXISTING,
                FILE_ATTRIBUTE_NORMAL, 0);
        if (hFile != INVALID_HANDLE_VALUE)
        {
            GetFileTime(hFile, NULL, NULL, pft);

            CloseHandle(hFile);
            return TRUE;
        }
    }
    return FALSE;
}

STDAPI_(BOOL) NT5_ExpandEnvironmentStringsForUserW (HANDLE hToken, LPCWSTR lpSrc, LPWSTR lpDest, DWORD dwSize)

{
    RIPMSG(g_bRunningOnNT5OrHigher, "Cannot invoke NT5_ExpandEnvironmentStringsForUserW when not on NT5 or higher");
    return(ExpandEnvironmentStringsForUserW(hToken, lpSrc, lpDest, dwSize));
}

