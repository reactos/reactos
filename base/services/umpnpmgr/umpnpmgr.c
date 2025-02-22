/*
 *  ReactOS kernel
 *  Copyright (C) 2005 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             base/services/umpnpmgr/umpnpmgr.c
 * PURPOSE:          User-mode Plug and Play manager
 * PROGRAMMER:       Eric Kohl (eric.kohl@reactos.org)
 *                   Hervé Poussineau (hpoussin@reactos.org)
 *                   Colin Finck (colin@reactos.org)
 */

/* INCLUDES *****************************************************************/

#include "precomp.h"

#define NDEBUG
#include <debug.h>


/* GLOBALS ******************************************************************/

static WCHAR ServiceName[] = L"PlugPlay";

static SERVICE_STATUS_HANDLE ServiceStatusHandle;
static SERVICE_STATUS ServiceStatus;

HKEY hEnumKey = NULL;
HKEY hClassKey = NULL;
BOOL g_IsUISuppressed = FALSE;
BOOL g_ShuttingDown = FALSE;

/* FUNCTIONS *****************************************************************/

static VOID
UpdateServiceStatus(
    _In_ DWORD dwState,
    _In_ DWORD dwCheckPoint)
{
    if ((dwState == SERVICE_STOPPED) || (dwState == SERVICE_STOP_PENDING))
        g_ShuttingDown = TRUE;

    ServiceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    ServiceStatus.dwCurrentState = dwState;
    ServiceStatus.dwWin32ExitCode = 0;
    ServiceStatus.dwServiceSpecificExitCode = 0;
    ServiceStatus.dwCheckPoint = dwCheckPoint;

    if (dwState == SERVICE_RUNNING)
        ServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_SHUTDOWN;
    else
        ServiceStatus.dwControlsAccepted = 0;

    if (dwState == SERVICE_START_PENDING ||
        dwState == SERVICE_STOP_PENDING ||
        dwState == SERVICE_PAUSE_PENDING ||
        dwState == SERVICE_CONTINUE_PENDING)
        ServiceStatus.dwWaitHint = 10000;
    else
        ServiceStatus.dwWaitHint = 0;

    SetServiceStatus(ServiceStatusHandle,
                     &ServiceStatus);
}


static DWORD WINAPI
ServiceControlHandler(DWORD dwControl,
                      DWORD dwEventType,
                      LPVOID lpEventData,
                      LPVOID lpContext)
{
    DPRINT1("ServiceControlHandler() called\n");

    switch (dwControl)
    {
        case SERVICE_CONTROL_STOP:
            DPRINT1("  SERVICE_CONTROL_STOP received\n");
            UpdateServiceStatus(SERVICE_STOP_PENDING, 1);
            /* Stop listening to RPC Messages */
            RpcMgmtStopServerListening(NULL);
            UpdateServiceStatus(SERVICE_STOPPED, 0);
            return ERROR_SUCCESS;

        case SERVICE_CONTROL_PAUSE:
            DPRINT1("  SERVICE_CONTROL_PAUSE received\n");
            UpdateServiceStatus(SERVICE_PAUSED, 0);
            return ERROR_SUCCESS;

        case SERVICE_CONTROL_CONTINUE:
            DPRINT1("  SERVICE_CONTROL_CONTINUE received\n");
            UpdateServiceStatus(SERVICE_RUNNING, 0);
            return ERROR_SUCCESS;

        case SERVICE_CONTROL_INTERROGATE:
            DPRINT1("  SERVICE_CONTROL_INTERROGATE received\n");
            SetServiceStatus(ServiceStatusHandle,
                             &ServiceStatus);
            return ERROR_SUCCESS;

        case SERVICE_CONTROL_SHUTDOWN:
            DPRINT1("  SERVICE_CONTROL_SHUTDOWN received\n");
            UpdateServiceStatus(SERVICE_STOP_PENDING, 1);
            /* Stop listening to RPC Messages */
            RpcMgmtStopServerListening(NULL);
            UpdateServiceStatus(SERVICE_STOPPED, 0);
            return ERROR_SUCCESS;

        default :
            DPRINT1("  Control %lu received\n", dwControl);
            return ERROR_CALL_NOT_IMPLEMENTED;
    }
}

static DWORD
GetBooleanRegValue(
    IN HKEY hKey,
    IN PCWSTR lpSubKey,
    IN PCWSTR lpValue,
    OUT PBOOL pValue)
{
    DWORD dwError, dwType, dwData;
    DWORD cbData = sizeof(dwData);
    HKEY hSubKey = NULL;

    /* Default value */
    *pValue = FALSE;

    dwError = RegOpenKeyExW(hKey,
                            lpSubKey,
                            0,
                            KEY_READ,
                            &hSubKey);
    if (dwError != ERROR_SUCCESS)
    {
        DPRINT("GetBooleanRegValue(): RegOpenKeyExW() has failed to open '%S' key! (Error: %lu)\n",
               lpSubKey, dwError);
        return dwError;
    }

    dwError = RegQueryValueExW(hSubKey,
                               lpValue,
                               0,
                               &dwType,
                               (PBYTE)&dwData,
                               &cbData);
    if (dwError != ERROR_SUCCESS)
    {
        DPRINT("GetBooleanRegValue(): RegQueryValueExW() has failed to query '%S' value! (Error: %lu)\n",
               lpValue, dwError);
        goto Cleanup;
    }
    if (dwType != REG_DWORD)
    {
        DPRINT("GetBooleanRegValue(): The value is not of REG_DWORD type!\n");
        goto Cleanup;
    }

    /* Return the value */
    *pValue = (dwData == 1);

Cleanup:
    RegCloseKey(hSubKey);

    return dwError;
}

BOOL
GetSuppressNewUIValue(VOID)
{
    BOOL bSuppressNewHWUI = FALSE;

    /*
     * Query the SuppressNewHWUI policy registry value. Don't cache it
     * as we want to update our behaviour in consequence.
     */
    GetBooleanRegValue(HKEY_LOCAL_MACHINE,
                       L"Software\\Policies\\Microsoft\\Windows\\DeviceInstall\\Settings",
                       L"SuppressNewHWUI",
                       &bSuppressNewHWUI);
    if (bSuppressNewHWUI)
        DPRINT("GetSuppressNewUIValue(): newdev.dll's wizard UI won't be shown!\n");

    return bSuppressNewHWUI;
}

VOID WINAPI
ServiceMain(DWORD argc, LPTSTR *argv)
{
    HANDLE hThread;
    DWORD dwThreadId;

    UNREFERENCED_PARAMETER(argc);
    UNREFERENCED_PARAMETER(argv);

    DPRINT("ServiceMain() called\n");

    ServiceStatusHandle = RegisterServiceCtrlHandlerExW(ServiceName,
                                                        ServiceControlHandler,
                                                        NULL);
    if (!ServiceStatusHandle)
    {
        DPRINT1("RegisterServiceCtrlHandlerExW() failed! (Error %lu)\n", GetLastError());
        return;
    }

    UpdateServiceStatus(SERVICE_START_PENDING, 1);

    hThread = CreateThread(NULL,
                           0,
                           PnpEventThread,
                           NULL,
                           0,
                           &dwThreadId);
    if (hThread != NULL)
        CloseHandle(hThread);

    UpdateServiceStatus(SERVICE_START_PENDING, 2);

    hThread = CreateThread(NULL,
                           0,
                           RpcServerThread,
                           NULL,
                           0,
                           &dwThreadId);
    if (hThread != NULL)
        CloseHandle(hThread);

    UpdateServiceStatus(SERVICE_START_PENDING, 3);

    hThread = CreateThread(NULL,
                           0,
                           DeviceInstallThread,
                           NULL,
                           0,
                           &dwThreadId);
    if (hThread != NULL)
        CloseHandle(hThread);

    UpdateServiceStatus(SERVICE_RUNNING, 0);

    DPRINT("ServiceMain() done\n");
}

static DWORD
InitializePnPManager(VOID)
{
    BOOLEAN OldValue;
    DWORD dwError;

    DPRINT("UMPNPMGR: InitializePnPManager() started\n");

    /* We need this privilege for using CreateProcessAsUserW */
    RtlAdjustPrivilege(SE_ASSIGNPRIMARYTOKEN_PRIVILEGE, TRUE, FALSE, &OldValue);

    hInstallEvent = CreateEventW(NULL, TRUE, SetupIsActive()/*FALSE*/, NULL);
    if (hInstallEvent == NULL)
    {
        dwError = GetLastError();
        DPRINT1("Could not create the Install Event! (Error %lu)\n", dwError);
        return dwError;
    }

    hNoPendingInstalls = CreateEventW(NULL,
                                      TRUE,
                                      FALSE,
                                      L"Global\\PnP_No_Pending_Install_Events");
    if (hNoPendingInstalls == NULL)
    {
        dwError = GetLastError();
        DPRINT1("Could not create the Pending-Install Event! (Error %lu)\n", dwError);
        return dwError;
    }

    /*
     * Initialize the device-install event list
     */

    hDeviceInstallListNotEmpty = CreateEventW(NULL, FALSE, FALSE, NULL);
    if (hDeviceInstallListNotEmpty == NULL)
    {
        dwError = GetLastError();
        DPRINT1("Could not create the List Event! (Error %lu)\n", dwError);
        return dwError;
    }

    hDeviceInstallListMutex = CreateMutexW(NULL, FALSE, NULL);
    if (hDeviceInstallListMutex == NULL)
    {
        dwError = GetLastError();
        DPRINT1("Could not create the List Mutex! (Error %lu)\n", dwError);
        return dwError;
    }
    InitializeListHead(&DeviceInstallListHead);

    /* Query the SuppressUI registry value and cache it for our whole lifetime */
    GetBooleanRegValue(HKEY_LOCAL_MACHINE,
                       L"System\\CurrentControlSet\\Services\\PlugPlay\\Parameters",
                       L"SuppressUI",
                       &g_IsUISuppressed);
    if (g_IsUISuppressed)
        DPRINT("UMPNPMGR: newdev.dll's wizard UI won't be shown!\n");

    dwError = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                            L"System\\CurrentControlSet\\Enum",
                            0,
                            KEY_ALL_ACCESS,
                            &hEnumKey);
    if (dwError != ERROR_SUCCESS)
    {
        DPRINT1("Could not open the Enum Key! (Error %lu)\n", dwError);
        return dwError;
    }

    dwError = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                            L"System\\CurrentControlSet\\Control\\Class",
                            0,
                            KEY_ALL_ACCESS,
                            &hClassKey);
    if (dwError != ERROR_SUCCESS)
    {
        DPRINT1("Could not open the Class Key! (Error %lu)\n", dwError);
        return dwError;
    }

    DPRINT("UMPNPMGR: InitializePnPManager() done\n");

    return 0;
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
            InitializePnPManager();
            break;

        case DLL_PROCESS_DETACH:
            break;
    }

    return TRUE;
}

/* EOF */
