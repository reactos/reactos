/*
 * PROJECT:     ReactOS W32Time Service
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Create W32Time Service that reqularly syncs clock to Internet Time
 * COPYRIGHT:   Copyright 2006 Ged Murphy <gedmurphy@gmail.com>
 *              Copyright 2018 Doug Lyons
 */

#include "w32time.h"
#include <debug.h>
#include <strsafe.h>

SERVICE_STATUS ServiceStatus;
SERVICE_STATUS_HANDLE hStatus;
HANDLE hStopEvent = NULL;
static WCHAR ServiceName[] = L"W32Time";

int InitService(VOID);

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


static DWORD
GetIntervalSetting(VOID)
{
    HKEY hKey;
    DWORD dwData;
    DWORD dwSize = sizeof(dwData);
    LONG lRet;

    dwData = 0;
    lRet = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                         L"SYSTEM\\CurrentControlSet\\Services\\W32Time\\TimeProviders\\NtpClient",
                         0,
                         KEY_QUERY_VALUE,
                         &hKey);
    if (lRet == ERROR_SUCCESS)
    {
        /*
         * This key holds the update interval in seconds.
         * It is useful for testing to set it to a value of 10 (Decimal).
         * This will cause the clock to try and update every 10 seconds.
         * So you can change the time and expect it to be set back correctly in 10-20 seconds.
         */
        lRet = RegQueryValueExW(hKey,
                                L"SpecialPollInterval",
                                NULL,
                                NULL,
                                (LPBYTE)&dwData,
                                &dwSize);
        RegCloseKey(hKey);
    }

    if (lRet != ERROR_SUCCESS || dwData < 120)
        return 9 * 60 * 60; // 9 hours, because Windows uses 9 hrs, 6 mins and 8 seconds by default.
    else
        return dwData;
}


DWORD
SetTime(VOID)
{
    ULONG ulTime;
    LONG lRet;
    HKEY hKey;
    WCHAR szData[MAX_VALUE_NAME] = L"";
    DWORD cbName = sizeof(szData);

    DPRINT("Entered SetTime.\n");

    lRet = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                         L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\DateTime\\Servers",
                         0,
                         KEY_QUERY_VALUE,
                         &hKey);
    if (lRet != ERROR_SUCCESS)
    {
        return lRet;
    }

    lRet = RegQueryValueExW(hKey, NULL, NULL, NULL, (LPBYTE)szData, &cbName);
    if (lRet == ERROR_SUCCESS)
    {
        cbName = sizeof(szData);
        lRet = RegQueryValueExW(hKey, szData, NULL, NULL, (LPBYTE)szData, &cbName);
    }

    RegCloseKey(hKey);

    DPRINT("Time Server is '%S'.\n", szData);

    /* Is the given string empty? */
    if (cbName == 0 || szData[0] == '\0')
    {
        DPRINT("The time NTP server couldn't be found, the string is empty!\n");
        return ERROR_INVALID_DATA;
    }

    ulTime = GetServerTime(szData);

    if (ulTime != 0)
    {
        return UpdateSystemTime(ulTime);
    }
    else
        return ERROR_GEN_FAILURE;
}


/* Control handler function */
VOID WINAPI
ControlHandler(DWORD request)
{
    switch (request)
    {
        case SERVICE_CONTROL_STOP:
        case SERVICE_CONTROL_SHUTDOWN:
            DPRINT("Stopping W32Time Service\n");

            ServiceStatus.dwWin32ExitCode = 0;
            ServiceStatus.dwCurrentState  = SERVICE_STOP_PENDING;
            SetServiceStatus(hStatus, &ServiceStatus);

            if (hStopEvent)
                SetEvent(hStopEvent);
            return;

        default:
            break;
    }

    return;
}


VOID
WINAPI
W32TmServiceMain(DWORD argc, LPWSTR *argv)
{
    int   result;
    DWORD dwInterval;

    UNREFERENCED_PARAMETER(argc);
    UNREFERENCED_PARAMETER(argv);

    ServiceStatus.dwServiceType             = SERVICE_WIN32;
    ServiceStatus.dwCurrentState            = SERVICE_START_PENDING;
    ServiceStatus.dwControlsAccepted        = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN;
    ServiceStatus.dwWin32ExitCode           = 0;
    ServiceStatus.dwServiceSpecificExitCode = 0;
    ServiceStatus.dwCheckPoint              = 0;
    ServiceStatus.dwWaitHint                = 0;

    hStatus = RegisterServiceCtrlHandlerW(ServiceName,
                                          ControlHandler);
    if (!hStatus)
    {
        /* Registering Control Handler failed */
        return;
    }

    /* Create the stop event */
    hStopEvent = CreateEventW(NULL, FALSE, FALSE, NULL);
    if (hStopEvent == NULL)
    {
        ServiceStatus.dwCurrentState  = SERVICE_STOPPED;
        ServiceStatus.dwWin32ExitCode = GetLastError();
        ServiceStatus.dwControlsAccepted = 0;
        SetServiceStatus(hStatus, &ServiceStatus);
        return;
    }

    /* Get the interval */
    dwInterval = GetIntervalSetting();

    /* We report the running status to SCM. */
    ServiceStatus.dwCurrentState = SERVICE_RUNNING;
    SetServiceStatus(hStatus, &ServiceStatus);

    /* The worker loop of a service */
    for (;;)
    {
        result = SetTime();

        if (result)
            DPRINT("W32Time Service failed to set clock.\n");
        else
            DPRINT("W32Time Service successfully set clock.\n");

        if (result)
        {
            /* In general we do not want to stop this service for a single
             * Internet read failure but there may be other reasons for which
             * we really might want to stop it.
             * Therefore this code is left here to make it easy to stop this
             * service when the correct conditions can be determined, but it
             * is left commented out.
            ServiceStatus.dwCurrentState  = SERVICE_STOPPED;
            ServiceStatus.dwWin32ExitCode = result;
            SetServiceStatus(hStatus, &ServiceStatus);
            return;
            */
        }

        if (WaitForSingleObject(hStopEvent, dwInterval * 1000) == WAIT_OBJECT_0)
        {
            CloseHandle(hStopEvent);
            hStopEvent = NULL;

            ServiceStatus.dwCurrentState  = SERVICE_STOPPED;
            ServiceStatus.dwWin32ExitCode = ERROR_SUCCESS;
            ServiceStatus.dwControlsAccepted = 0;
            SetServiceStatus(hStatus, &ServiceStatus);
            DPRINT("Stopped W32Time Service\n");
            return;
        }
    }
    return;
}



BOOL WINAPI
DllMain(HINSTANCE hinstDLL,
        DWORD fdwReason,
        LPVOID lpvReserved)
{
    switch (fdwReason)
    {
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hinstDLL);
            break;

        case DLL_PROCESS_DETACH:
            break;
    }

    return TRUE;
}


DWORD WINAPI
W32TimeSyncNow(LPCWSTR cmdline,
               UINT blocking,
               UINT flags)
{
    DWORD result;
    result = SetTime();
    if (result)
    {
        DPRINT("W32TimeSyncNow failed and clock not set.\n");
    }
    else
    {
        DPRINT("W32TimeSyncNow succeeded and clock set.\n");
    }
    return result;
}
