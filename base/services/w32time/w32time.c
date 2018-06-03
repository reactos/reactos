/*
 * PROJECT:     ReactOS Timedate Control Panel
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Create W32Time Service that reqularly syncs clock to Internet Time
 * COPYRIGHT:   Copyright 2006 Ged Murphy <gedmurphy@gmail.com>
 */

#include "w32time.h"

/* Get the domain name from the registry */
static DWORD
GetNTPServerAddress(LPWSTR *lpAddress)
{
    HKEY hKey;
    WCHAR szSel[4];
    DWORD dwSize;
    LONG lRet;

    *lpAddress = NULL;
    hKey = NULL;

    lRet = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                         L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\DateTime\\Servers",
                         0,
                         KEY_QUERY_VALUE,
                         &hKey);
    if (lRet != ERROR_SUCCESS)
        goto Exit;

    /* Get data from default value */
    dwSize = 4 * sizeof(WCHAR);
    lRet = RegQueryValueExW(hKey,
                            NULL,
                            NULL,
                            NULL,
                            (LPBYTE)szSel,
                            &dwSize);
    if (lRet != ERROR_SUCCESS)
        goto Exit;

    dwSize = 0;
    lRet = RegQueryValueExW(hKey,
                            szSel,
                            NULL,
                            NULL,
                            NULL,
                            &dwSize);
    if (lRet != ERROR_SUCCESS)
        goto Exit;

    (*lpAddress) = (LPWSTR)HeapAlloc(GetProcessHeap(),
                                     0,
                                     dwSize);
    if ((*lpAddress) == NULL)
    {
        lRet = ERROR_NOT_ENOUGH_MEMORY;
        goto Exit;
    }

    lRet = RegQueryValueExW(hKey,
                            szSel,
                            NULL,
                            NULL,
                            (LPBYTE)*lpAddress,
                            &dwSize);
    if (lRet != ERROR_SUCCESS)
        goto Exit;

Exit:
    if (hKey)
        RegCloseKey(hKey);
    if (lRet != ERROR_SUCCESS)
        HeapFree(GetProcessHeap(), 0, *lpAddress);

    return lRet;
}


/* Request the time from the current NTP server */
static DWORD
GetTimeFromServer(PULONG pulTime)
{
    LPWSTR lpAddress;
    DWORD dwError;

    dwError = GetNTPServerAddress(&lpAddress);
    if (dwError != ERROR_SUCCESS)
    {
        return dwError;
    }

    *pulTime = GetServerTime(lpAddress);
    if (*pulTime == 0)
    {
        dwError = ERROR_GEN_FAILURE;
    }

    HeapFree(GetProcessHeap(),
             0,
             lpAddress);

    return dwError;
}


BOOL
SystemSetTime(LPSYSTEMTIME lpSystemTime)
{
    HANDLE hToken;
    DWORD PrevSize;
    TOKEN_PRIVILEGES priv, previouspriv;
    BOOL Ret = FALSE;

    /*
     * Enable the SeSystemtimePrivilege privilege
     */

    if (OpenProcessToken(GetCurrentProcess(),
                         TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
                         &hToken))
    {
        priv.PrivilegeCount = 1;
        priv.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

        if (LookupPrivilegeValueW(NULL,
                                  SE_SYSTEMTIME_NAME,
                                  &priv.Privileges[0].Luid))
        {
            if (AdjustTokenPrivileges(hToken,
                                      FALSE,
                                      &priv,
                                      sizeof(previouspriv),
                                      &previouspriv,
                                      &PrevSize) &&
                GetLastError() == ERROR_SUCCESS)
            {
                /*
                 * We successfully enabled it, we're permitted to change the time.
                 */
                Ret = SetSystemTime(lpSystemTime);

                /*
                 * For the sake of security, restore the previous status again
                 */
                if (previouspriv.PrivilegeCount > 0)
                {
                    AdjustTokenPrivileges(hToken,
                                          FALSE,
                                          &previouspriv,
                                          0,
                                          NULL,
                                          0);
                }
            }
        }
        CloseHandle(hToken);
    }

    return Ret;
}


/*
 * NTP servers state the number of seconds passed since
 * 1st Jan, 1900. The time returned from the server
 * needs adding to that date to get the current Gregorian time
 */
static DWORD
UpdateSystemTime(ULONG ulTime)
{
    FILETIME ftNew;
    LARGE_INTEGER li;
    SYSTEMTIME stNew;

    /* Time at 1st Jan 1900 */
    stNew.wYear = 1900;
    stNew.wMonth = 1;
    stNew.wDay = 1;
    stNew.wHour = 0;
    stNew.wMinute = 0;
    stNew.wSecond = 0;
    stNew.wMilliseconds = 0;

    /* Convert to a file time */
    if (!SystemTimeToFileTime(&stNew, &ftNew))
    {
        return GetLastError();
    }

    /* Add on the time passed since 1st Jan 1900 */
    li = *(LARGE_INTEGER *)&ftNew;
    li.QuadPart += (LONGLONG)10000000 * ulTime;
    ftNew = * (FILETIME *)&li;

    /* Convert back to a system time */
    if (!FileTimeToSystemTime(&ftNew, &stNew))
    {
        return GetLastError();
    }

    if (!SystemSetTime(&stNew))
    {
        return GetLastError();
    }

    return ERROR_SUCCESS;
}


DWORD WINAPI
W32TimeSyncNow(LPCWSTR cmdline,
               UINT blocking,
               UINT flags)
{
    DWORD dwError;
    ULONG ulTime;

    dwError = GetTimeFromServer(&ulTime);
    if (dwError != ERROR_SUCCESS)
    {
        return dwError;
    }

    if (ulTime != 0)
    {
        dwError = UpdateSystemTime(ulTime);
    }

    return dwError;
}

