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

/* FUNCTIONS *****************************************************************/

static DWORD WINAPI
PnpEventThread(LPVOID lpParameter)
{
    PLUGPLAY_CONTROL_USER_RESPONSE_DATA ResponseData = {0, 0, 0, 0};
    DWORD dwRet = ERROR_SUCCESS;
    NTSTATUS Status;
    RPC_STATUS RpcStatus;
    PPLUGPLAY_EVENT_BLOCK PnpEvent, NewPnpEvent;
    ULONG PnpEventSize;

    UNREFERENCED_PARAMETER(lpParameter);

    PnpEventSize = 0x1000;
    PnpEvent = HeapAlloc(GetProcessHeap(), 0, PnpEventSize);
    if (PnpEvent == NULL)
        return ERROR_OUTOFMEMORY;

    for (;;)
    {
        DPRINT("Calling NtGetPlugPlayEvent()\n");

        /* Wait for the next PnP event */
        Status = NtGetPlugPlayEvent(0, 0, PnpEvent, PnpEventSize);

        /* Resize the buffer for the PnP event if it's too small */
        if (Status == STATUS_BUFFER_TOO_SMALL)
        {
            PnpEventSize += 0x400;
            NewPnpEvent = HeapReAlloc(GetProcessHeap(), 0, PnpEvent, PnpEventSize);
            if (NewPnpEvent == NULL)
            {
                dwRet = ERROR_OUTOFMEMORY;
                break;
            }
            PnpEvent = NewPnpEvent;
            continue;
        }

        if (!NT_SUCCESS(Status))
        {
            DPRINT1("NtGetPlugPlayEvent() failed (Status 0x%08lx)\n", Status);
            break;
        }

        /* Process the PnP event */
        DPRINT("Received PnP Event\n");
        if (UuidEqual(&PnpEvent->EventGuid, (UUID*)&GUID_DEVICE_ENUMERATED, &RpcStatus))
        {
            DeviceInstallParams* Params;
            DWORD len;
            DWORD DeviceIdLength;

            DPRINT("Device enumerated: %S\n", PnpEvent->TargetDevice.DeviceIds);

            DeviceIdLength = lstrlenW(PnpEvent->TargetDevice.DeviceIds);
            if (DeviceIdLength)
            {
                /* Queue device install (will be dequeued by DeviceInstallThread) */
                len = FIELD_OFFSET(DeviceInstallParams, DeviceIds) + (DeviceIdLength + 1) * sizeof(WCHAR);
                Params = HeapAlloc(GetProcessHeap(), 0, len);
                if (Params)
                {
                    wcscpy(Params->DeviceIds, PnpEvent->TargetDevice.DeviceIds);
                    InterlockedPushEntrySList(&DeviceInstallListHead, &Params->ListEntry);
                    SetEvent(hDeviceInstallListNotEmpty);
                }
            }
        }
        else if (UuidEqual(&PnpEvent->EventGuid, (UUID*)&GUID_DEVICE_ARRIVAL, &RpcStatus))
        {
//            DWORD dwRecipient;

            DPRINT("Device arrival: %S\n", PnpEvent->TargetDevice.DeviceIds);

//            dwRecipient = BSM_ALLDESKTOPS | BSM_APPLICATIONS;
//            BroadcastSystemMessageW(BSF_POSTMESSAGE,
//                                    &dwRecipient,
//                                    WM_DEVICECHANGE,
//                                    DBT_DEVNODES_CHANGED,
//                                    0);
            SendMessageW(HWND_BROADCAST, WM_DEVICECHANGE, DBT_DEVNODES_CHANGED, 0);
        }
        else if (UuidEqual(&PnpEvent->EventGuid, (UUID*)&GUID_DEVICE_EJECT_VETOED, &RpcStatus))
        {
            DPRINT1("Eject vetoed: %S\n", PnpEvent->TargetDevice.DeviceIds);
        }
        else if (UuidEqual(&PnpEvent->EventGuid, (UUID*)&GUID_DEVICE_KERNEL_INITIATED_EJECT, &RpcStatus))
        {
            DPRINT1("Kernel initiated eject: %S\n", PnpEvent->TargetDevice.DeviceIds);
        }
        else if (UuidEqual(&PnpEvent->EventGuid, (UUID*)&GUID_DEVICE_SAFE_REMOVAL, &RpcStatus))
        {
//            DWORD dwRecipient;

            DPRINT1("Safe removal: %S\n", PnpEvent->TargetDevice.DeviceIds);

//            dwRecipient = BSM_ALLDESKTOPS | BSM_APPLICATIONS;
//            BroadcastSystemMessageW(BSF_POSTMESSAGE,
//                                    &dwRecipient,
//                                    WM_DEVICECHANGE,
//                                    DBT_DEVNODES_CHANGED,
//                                    0);
            SendMessageW(HWND_BROADCAST, WM_DEVICECHANGE, DBT_DEVNODES_CHANGED, 0);
        }
        else if (UuidEqual(&PnpEvent->EventGuid, (UUID*)&GUID_DEVICE_SURPRISE_REMOVAL, &RpcStatus))
        {
//            DWORD dwRecipient;

            DPRINT1("Surprise removal: %S\n", PnpEvent->TargetDevice.DeviceIds);

//            dwRecipient = BSM_ALLDESKTOPS | BSM_APPLICATIONS;
//            BroadcastSystemMessageW(BSF_POSTMESSAGE,
//                                    &dwRecipient,
//                                    WM_DEVICECHANGE,
//                                    DBT_DEVNODES_CHANGED,
//                                    0);
            SendMessageW(HWND_BROADCAST, WM_DEVICECHANGE, DBT_DEVNODES_CHANGED, 0);
        }
        else if (UuidEqual(&PnpEvent->EventGuid, (UUID*)&GUID_DEVICE_REMOVAL_VETOED, &RpcStatus))
        {
            DPRINT1("Removal vetoed: %S\n", PnpEvent->TargetDevice.DeviceIds);
        }
        else if (UuidEqual(&PnpEvent->EventGuid, (UUID*)&GUID_DEVICE_REMOVE_PENDING, &RpcStatus))
        {
            DPRINT1("Removal pending: %S\n", PnpEvent->TargetDevice.DeviceIds);
        }
        else
        {
            DPRINT1("Unknown event, GUID {%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}\n",
                PnpEvent->EventGuid.Data1, PnpEvent->EventGuid.Data2, PnpEvent->EventGuid.Data3,
                PnpEvent->EventGuid.Data4[0], PnpEvent->EventGuid.Data4[1], PnpEvent->EventGuid.Data4[2],
                PnpEvent->EventGuid.Data4[3], PnpEvent->EventGuid.Data4[4], PnpEvent->EventGuid.Data4[5],
                PnpEvent->EventGuid.Data4[6], PnpEvent->EventGuid.Data4[7]);
        }

        /* Dequeue the current PnP event and signal the next one */
        Status = NtPlugPlayControl(PlugPlayControlUserResponse,
                                   &ResponseData,
                                   sizeof(ResponseData));
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("NtPlugPlayControl(PlugPlayControlUserResponse) failed (Status 0x%08lx)\n", Status);
            break;
        }
    }

    HeapFree(GetProcessHeap(), 0, PnpEvent);

    return dwRet;
}


static VOID
UpdateServiceStatus(DWORD dwState)
{
    ServiceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    ServiceStatus.dwCurrentState = dwState;
    ServiceStatus.dwControlsAccepted = 0;
    ServiceStatus.dwWin32ExitCode = 0;
    ServiceStatus.dwServiceSpecificExitCode = 0;
    ServiceStatus.dwCheckPoint = 0;

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
            /* Stop listening to RPC Messages */
            RpcMgmtStopServerListening(NULL);
            UpdateServiceStatus(SERVICE_STOPPED);
            return ERROR_SUCCESS;

        case SERVICE_CONTROL_PAUSE:
            DPRINT1("  SERVICE_CONTROL_PAUSE received\n");
            UpdateServiceStatus(SERVICE_PAUSED);
            return ERROR_SUCCESS;

        case SERVICE_CONTROL_CONTINUE:
            DPRINT1("  SERVICE_CONTROL_CONTINUE received\n");
            UpdateServiceStatus(SERVICE_RUNNING);
            return ERROR_SUCCESS;

        case SERVICE_CONTROL_INTERROGATE:
            DPRINT1("  SERVICE_CONTROL_INTERROGATE received\n");
            SetServiceStatus(ServiceStatusHandle,
                             &ServiceStatus);
            return ERROR_SUCCESS;

        case SERVICE_CONTROL_SHUTDOWN:
            DPRINT1("  SERVICE_CONTROL_SHUTDOWN received\n");
            /* Stop listening to RPC Messages */
            RpcMgmtStopServerListening(NULL);
            UpdateServiceStatus(SERVICE_STOPPED);
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

    UpdateServiceStatus(SERVICE_START_PENDING);

    hThread = CreateThread(NULL,
                           0,
                           PnpEventThread,
                           NULL,
                           0,
                           &dwThreadId);
    if (hThread != NULL)
        CloseHandle(hThread);

    hThread = CreateThread(NULL,
                           0,
                           RpcServerThread,
                           NULL,
                           0,
                           &dwThreadId);
    if (hThread != NULL)
        CloseHandle(hThread);

    hThread = CreateThread(NULL,
                           0,
                           DeviceInstallThread,
                           NULL,
                           0,
                           &dwThreadId);
    if (hThread != NULL)
        CloseHandle(hThread);

    UpdateServiceStatus(SERVICE_RUNNING);

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

    hDeviceInstallListNotEmpty = CreateEventW(NULL, FALSE, FALSE, NULL);
    if (hDeviceInstallListNotEmpty == NULL)
    {
        dwError = GetLastError();
        DPRINT1("Could not create the Event! (Error %lu)\n", dwError);
        return dwError;
    }

    hNoPendingInstalls = CreateEventW(NULL,
                                      TRUE,
                                      FALSE,
                                      L"Global\\PnP_No_Pending_Install_Events");
    if (hNoPendingInstalls == NULL)
    {
        dwError = GetLastError();
        DPRINT1("Could not create the Event! (Error %lu)\n", dwError);
        return dwError;
    }

    InitializeSListHead(&DeviceInstallListHead);

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
