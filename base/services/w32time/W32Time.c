/*
 * PROJECT:     ReactOS W32Time Service
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Create W32Time Service that reqularly syncs clock to Internet Time
 * COPYRIGHT:   Copyright 2018 Doug Lyons
 */

#include<windows.h>
#include <debug.h>
#include <strsafe.h>

#include "timedate.h"

SERVICE_STATUS ServiceStatus; 
SERVICE_STATUS_HANDLE hStatus;
static WCHAR ServiceName[] = L"W32Time";
 
int InitService(VOID);
ULONG GetServerTime(LPWSTR lpAddress);

/* Copied from internettime.c */
static VOID
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
        return;
    }

    /* Add on the time passed since 1st Jan 1900 */
    li = *(LARGE_INTEGER *)&ftNew;
    li.QuadPart += (LONGLONG)10000000 * ulTime;
    ftNew = * (FILETIME *)&li;

    /* Convert back to a system time */
    if (!FileTimeToSystemTime(&ftNew, &stNew))
    {
        return;
    }

    /* Use SystemSetTime with SystemTime = TRUE to set System Time */
    SystemSetTime(&stNew, TRUE);
}

/* Copied from dateandtime.c */
BOOL
SystemSetTime(LPSYSTEMTIME lpSystemTime,
              BOOL SystemTime)
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
                 * Check the second parameter for SystemTime and if TRUE set System Time.
                 * Otherwise, if FALSE set the Local Time.
                 * Call SetLocalTime twice to ensure correct results.
                 * See MSDN https://msdn.microsoft.com/en-us/library/ms724936(VS.85).aspx
                 * First time sets correct DST and second time uses correct DST.
                 */
                if (SystemTime)
                {
                    Ret = SetSystemTime(lpSystemTime);
                }
                else
                {
                    Ret = SetLocalTime(lpSystemTime) &&
                          SetLocalTime(lpSystemTime);
                }

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

static DWORD
GetIntervalSetting(VOID)
{
    HKEY hKey;
    DWORD dwData;
    DWORD dwSize = sizeof(dwData);
    LONG lRet;

    dwData = 0;
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                      L"SYSTEM\\CurrentControlSet\\Services\\W32Time\\TimeProviders\\NtpClient",
                      0,
                      KEY_QUERY_VALUE,
                      &hKey) == ERROR_SUCCESS)
    {
        lRet = RegQueryValueExW(hKey,
                                L"SpecialPollInterval",
                                NULL,
                                NULL,
                                (LPBYTE)&dwData,
                                &dwSize);
        RegCloseKey(hKey);
    }

    if (lRet != ERROR_SUCCESS)
        return 0;
    else
        return dwData;
}


int
SetTime(VOID)
{
    ULONG ulTime;
    LONG lRet;
    HKEY hKey;
    DWORD dwIndex = 0;
    DWORD dwValSize;
    WCHAR szValName[MAX_VALUE_NAME];
    WCHAR szData[MAX_VALUE_NAME];
    DWORD cbName = sizeof(szData);
    WCHAR szDefault[MAX_VALUE_NAME] = L"";
    size_t Size;

    DPRINT("Entered SetTime.\n");

    lRet = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                         L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\DateTime\\Servers",
                         0,
                         KEY_QUERY_VALUE,
                         &hKey);
    if (lRet != ERROR_SUCCESS)
    {
        return 1;
    }

    while (TRUE)
    {
        dwValSize = sizeof(szValName);
        szValName[0] = UNICODE_NULL;
        lRet = RegEnumValueW(hKey,
                             dwIndex,
                             szValName,
                             &dwValSize,
                             NULL,
                             NULL,
                             (LPBYTE)szData,
                             &cbName);
        if (lRet == ERROR_SUCCESS)
        {
            /* Get data from default registry value */
            if (szValName[0] == UNICODE_NULL) /* if this is the "(Default)" key */
            {
                StringCbCopyNW(szDefault, sizeof(szDefault), szData, cbName);
            }
            dwIndex++;
        }
        else if (lRet != ERROR_MORE_DATA)
        {
            break;
        }
    }


    dwIndex = 0;
    while (TRUE)
    {
        dwValSize = sizeof(szValName);
        szValName[0] = UNICODE_NULL;
        lRet = RegEnumValueW(hKey,
                             dwIndex,
                             szValName,
                             &dwValSize,
                             NULL,
                             NULL,
                             (LPBYTE)szData,
                             &cbName);
        if (lRet == ERROR_SUCCESS)
        {
            /* Get data from selected registry value */
            if (StringCchLength(szValName, MAX_VALUE_NAME, &Size) == S_OK &&
                 StringCchLength(szData, MAX_VALUE_NAME, &Size) == S_OK)
            {
                if (wcscmp(szValName, szDefault) == 0)
                {
                    StringCbCopyNW(szDefault, sizeof(szDefault), szData, cbName);
                }
            }
            dwIndex++;
        }
        else if (lRet != ERROR_MORE_DATA)
        {
            break;
        }
    }

    RegCloseKey(hKey);

    DPRINT("Time Server is '%S'.\n", szDefault);

    ulTime = GetServerTime(szDefault);

    if (ulTime != 0)
    {
        UpdateSystemTime(ulTime);
        return 0;
    }
    else
        return 1;
}

/* Control handler function */
VOID WINAPI
ControlHandler(DWORD request) 
{ 
    switch (request) 
    { 
        case SERVICE_CONTROL_STOP: 
            DPRINT("W32Time Service stopped.\n");

            ServiceStatus.dwWin32ExitCode = 0; 
            ServiceStatus.dwCurrentState  = SERVICE_STOPPED; 
            SetServiceStatus(hStatus, &ServiceStatus);
            return; 
 
        case SERVICE_CONTROL_SHUTDOWN: 
            DPRINT("W32Time Service stopped.\n");

            ServiceStatus.dwWin32ExitCode = 0; 
            ServiceStatus.dwCurrentState  = SERVICE_STOPPED; 
            SetServiceStatus(hStatus, &ServiceStatus);
            return; 
        
        default:
            break;
    } 
 
    /* Report current status */
    SetServiceStatus(hStatus, &ServiceStatus);
 
    return; 
}

VOID
WINAPI
ServiceMain(DWORD argc, LPWSTR *argv)
{
    int   result;
    DWORD dwPollInterval;
 
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
                                          ControlHandler);  /* (LPHANDLER_FUNCTION) */
    if (!hStatus) 
    { 
        /* Registering Control Handler failed */
        return; 
    }  
    /* Initialize Service */
    result = InitService();

    if (result) 
    {
        /* In general we do not want to stop this service for a single Internet read failure
        but there may be other reasons for which we really might want to stop it.
        Therefore this code is left here to make it easy to stop this service
        when the correct conditions can be determined, but it is left commented out.

        ServiceStatus.dwCurrentState  = SERVICE_STOPPED; 
        ServiceStatus.dwWin32ExitCode = -1; 
        SetServiceStatus(hStatus, &ServiceStatus); 
        return;
        */
    }

    /* We report the running status to SCM. */
    ServiceStatus.dwCurrentState = SERVICE_RUNNING; 
    SetServiceStatus(hStatus, &ServiceStatus);
 
    /* The worker loop of a service */
    while (ServiceStatus.dwCurrentState == SERVICE_RUNNING)
    {
        dwPollInterval = GetIntervalSetting();
        result = SetTime();

        if (result)
            DPRINT("W32Time Service failed to set clock.\n");
        else
            DPRINT("W32Time Service successfully set clock.\n");

        if (result)
        {
            /* Commented out for the reasons given above about stopping this service.
            ServiceStatus.dwCurrentState  = SERVICE_STOPPED; 
            ServiceStatus.dwWin32ExitCode = -1; 
            SetServiceStatus(hStatus, &ServiceStatus);
            return;
            */
        }

        Sleep(dwPollInterval);
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


HRESULT WINAPI W32TimeSyncNow(LPCWSTR cmdline, UINT blocking, UINT flags)
{
    int result;
    result = SetTime();
    if (result)
    {
        DPRINT("W32TimeSyncNow failed and clock not set.\n");
        return 1;
    }
    else
    {
        DPRINT("W32TimeSyncNow succeeded and clock set.\n");
        return 0;
    }
}

/* Service initialization */
int InitService(VOID) 
{ 
    int result;
    result = SetTime();
    if (result)
        DPRINT("W32Time Service failed to start successfully and clock not set.\n");
    else
        DPRINT("W32Time Service started successfully and clock set.\n");
    return result; 
} 
