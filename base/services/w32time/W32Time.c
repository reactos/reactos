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

        if (LookupPrivilegeValue(NULL,
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
GetIntervalSetting()
{
    HKEY hKey;
    DWORD dwData;
    DWORD dwSize = sizeof(DWORD);

    dwData = 0;
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                      L"SYSTEM\\CurrentControlSet\\Services\\W32Time\\TimeProviders\\NtpClient",
                      0,
                      KEY_QUERY_VALUE,
                      &hKey) == ERROR_SUCCESS)
    {
        RegQueryValueExW(hKey,
                             L"SpecialPollInterval",
                             NULL,
                             NULL,
                             (LPBYTE)&dwData,
                             &dwSize);
        RegCloseKey(hKey);
    }
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
    DWORD dwNameSize;
    WCHAR szValName[MAX_VALUE_NAME];
    WCHAR szData[256];
    WCHAR szDefault[256];


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
        dwValSize = MAX_VALUE_NAME * sizeof(WCHAR);
        szValName[0] = L'\0';
        lRet = RegEnumValueW(hKey,
                             dwIndex,
                             szValName,
                             &dwValSize,
                             NULL,
                             NULL,
                             (LPBYTE)szData,
                             &dwNameSize);
        if (lRet == ERROR_SUCCESS)
        {
            /* Get data from default reg value */
            if (szValName[0] == UNICODE_NULL) // if this is the "(Default)" key
            {
                StringCbCopy(szDefault, wcslen(szDefault), szData);
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
        dwValSize = MAX_VALUE_NAME * sizeof(WCHAR);
        szValName[0] = L'\0';
        lRet = RegEnumValueW(hKey,
                             dwIndex,
                             szValName,
                             &dwValSize,
                             NULL,
                             NULL,
                             (LPBYTE)szData,
                             &dwNameSize);
        if (lRet == ERROR_SUCCESS)
        {
            /* Get date from selected reg value */
            if (wcscmp(szValName, szDefault) == 0) // if (Index == Default)
            {
                StringCbCopy(szDefault, wcslen(szDefault), szData);
            }
            dwIndex++;
        }
        else if (lRet != ERROR_MORE_DATA)
        {
            break;
        }
    }

    RegCloseKey(hKey);

    ulTime = GetServerTime(szDefault);

    if (ulTime != 0)
        UpdateSystemTime(ulTime);

    return 0;
}

// Control handler function
VOID WINAPI
ControlHandler(DWORD request) 
{ 
    switch(request) 
    { 
        case SERVICE_CONTROL_STOP: 
            DPRINT("W32Time Service stopped.\n");

            ServiceStatus.dwWin32ExitCode = 0; 
            ServiceStatus.dwCurrentState  = SERVICE_STOPPED; 
            SetServiceStatus (hStatus, &ServiceStatus);
            return; 
 
        case SERVICE_CONTROL_SHUTDOWN: 
            DPRINT1("W32Time Service stopped.\n");

            ServiceStatus.dwWin32ExitCode = 0; 
            ServiceStatus.dwCurrentState  = SERVICE_STOPPED; 
            SetServiceStatus (hStatus, &ServiceStatus);
            return; 
        
        default:
            break;
    } 
 
    // Report current status
    SetServiceStatus (hStatus,  &ServiceStatus);
 
    return; 
}

static VOID CALLBACK
ServiceMain(DWORD argc, LPWSTR *argv)
{
    int   error;
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
 
    hStatus = RegisterServiceCtrlHandler(ServiceName,
		                         ControlHandler);  // (LPHANDLER_FUNCTION)
    if (hStatus == (SERVICE_STATUS_HANDLE)0) 
    { 
        // Registering Control Handler failed
        return; 
    }  
    // Initialize Service 
    error = InitService(); 
    if (error) 
    {
        // Initialization failed
        ServiceStatus.dwCurrentState       = SERVICE_STOPPED; 
        ServiceStatus.dwWin32ExitCode      = -1; 
        SetServiceStatus(hStatus, &ServiceStatus); 
        return; 
    } 
    // We report the running status to SCM. 
    ServiceStatus.dwCurrentState = SERVICE_RUNNING; 
    SetServiceStatus (hStatus, &ServiceStatus);
 
    // The worker loop of a service
    while (ServiceStatus.dwCurrentState == SERVICE_RUNNING)
        {
            dwPollInterval = GetIntervalSetting();
            result = SetTime();
            if (result)
            {
                ServiceStatus.dwCurrentState       = SERVICE_STOPPED; 
                ServiceStatus.dwWin32ExitCode      = -1; 
                SetServiceStatus(hStatus, &ServiceStatus);
                return;
            }
            Sleep(dwPollInterval);
        }
    return; 
}

int wmain(int argc, WCHAR *argv[]) 
{

    SERVICE_TABLE_ENTRYW ServiceTable[2] =
    {
        {ServiceName, ServiceMain},
        {NULL, NULL}
    };

    UNREFERENCED_PARAMETER(argc);
    UNREFERENCED_PARAMETER(argv);

    DPRINT("W32Time: main() started\n");

    // Start the control dispatcher thread for our service
    StartServiceCtrlDispatcher(ServiceTable);

    DPRINT("W32Time: main() done\n");

    ExitThread(0);

    return 0; 
}
 
// Service initialization
int InitService(VOID) 
{ 
    int result;
    result = SetTime();
    DPRINT1("W32Time Service started.\n");
    return(result); 
} 



