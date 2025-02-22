/*
 * PROJECT:     ReactOS NetLogon Service
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     NetLogon service RPC server
 * COPYRIGHT:   Eric Kohl 2019 <eric.kohl@reactos.org>
 */

/* INCLUDES *****************************************************************/

#include "precomp.h"

WINE_DEFAULT_DEBUG_CHANNEL(netlogon);


/* GLOBALS ******************************************************************/

HINSTANCE hDllInstance;

static WCHAR ServiceName[] = L"netlogon";

static SERVICE_STATUS_HANDLE ServiceStatusHandle;
static SERVICE_STATUS ServiceStatus;


/* FUNCTIONS *****************************************************************/

static
VOID
UpdateServiceStatus(
    DWORD dwState)
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


static
DWORD
WINAPI
ServiceControlHandler(
    DWORD dwControl,
    DWORD dwEventType,
    LPVOID lpEventData,
    LPVOID lpContext)
{
    TRACE("ServiceControlHandler()\n");

    switch (dwControl)
    {
        case SERVICE_CONTROL_STOP:
            TRACE("  SERVICE_CONTROL_STOP received\n");
            /* Stop listening to incoming RPC messages */
            RpcMgmtStopServerListening(NULL);
            UpdateServiceStatus(SERVICE_STOPPED);
            return ERROR_SUCCESS;

        case SERVICE_CONTROL_PAUSE:
            TRACE("  SERVICE_CONTROL_PAUSE received\n");
            UpdateServiceStatus(SERVICE_PAUSED);
            return ERROR_SUCCESS;

        case SERVICE_CONTROL_CONTINUE:
            TRACE("  SERVICE_CONTROL_CONTINUE received\n");
            UpdateServiceStatus(SERVICE_RUNNING);
            return ERROR_SUCCESS;

        case SERVICE_CONTROL_INTERROGATE:
            TRACE("  SERVICE_CONTROL_INTERROGATE received\n");
            SetServiceStatus(ServiceStatusHandle,
                             &ServiceStatus);
            return ERROR_SUCCESS;

        case SERVICE_CONTROL_SHUTDOWN:
            TRACE("  SERVICE_CONTROL_SHUTDOWN received\n");
            UpdateServiceStatus(SERVICE_STOPPED);
            return ERROR_SUCCESS;

        default :
            TRACE("  Control %lu received\n", dwControl);
            return ERROR_CALL_NOT_IMPLEMENTED;
    }
}


static
DWORD
ServiceInit(VOID)
{
    HANDLE hThread;

    hThread = CreateThread(NULL,
                           0,
                           (LPTHREAD_START_ROUTINE)RpcThreadRoutine,
                           NULL,
                           0,
                           NULL);

    if (!hThread)
    {
        ERR("Can't create PortThread\n");
        return GetLastError();
    }
    else
        CloseHandle(hThread);

    return ERROR_SUCCESS;
}


VOID WINAPI
NlNetlogonMain(
    _In_ INT ArgCount,
    _In_ PWSTR *ArgVector)
{
    DWORD dwError;

    UNREFERENCED_PARAMETER(ArgCount);
    UNREFERENCED_PARAMETER(ArgVector);

    TRACE("NlNetlogonMain(%d %p)\n", ArgCount, ArgVector);

    ServiceStatusHandle = RegisterServiceCtrlHandlerExW(ServiceName,
                                                        ServiceControlHandler,
                                                        NULL);
    if (!ServiceStatusHandle)
    {
        ERR("RegisterServiceCtrlHandlerExW() failed! (Error %lu)\n", GetLastError());
        return;
    }

    UpdateServiceStatus(SERVICE_START_PENDING);

    dwError = ServiceInit();
    if (dwError != ERROR_SUCCESS)
    {
        ERR("Service stopped (dwError: %lu\n", dwError);
        UpdateServiceStatus(SERVICE_STOPPED);
        return;
    }

    UpdateServiceStatus(SERVICE_RUNNING);
}


BOOL
WINAPI
DllMain(
    _In_ HINSTANCE hinstDLL,
    _In_ DWORD fdwReason,
    _In_ PVOID pvReserved)
{
    UNREFERENCED_PARAMETER(pvReserved);

    switch (fdwReason)
    {
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hinstDLL);
            hDllInstance = hinstDLL;
            break;

        case DLL_PROCESS_DETACH:
            break;
    }

    return TRUE;
}

/* EOF */
