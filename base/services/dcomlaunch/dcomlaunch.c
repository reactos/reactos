/*
 * PROJECT:     ReactOS RPC Subsystem Service
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     DCOMLAUNCH service
 * COPYRIGHT:   Copyright 2019 Pierre Schweitzer
 */

/* INCLUDES *****************************************************************/

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H
#include <stdarg.h>
#include <windef.h>
#include <winbase.h>
#include <winreg.h>
#include <winsvc.h>

#define NDEBUG
#include <debug.h>

/* GLOBALS ******************************************************************/

static WCHAR ServiceName[] = L"dcomlaunch";

static SERVICE_STATUS_HANDLE ServiceStatusHandle;
static SERVICE_STATUS ServiceStatus;
static HANDLE ShutdownEvent;

/* FUNCTIONS *****************************************************************/

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
            SetEvent(ShutdownEvent);
            UpdateServiceStatus(SERVICE_STOP_PENDING);
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
            SetEvent(ShutdownEvent);
            UpdateServiceStatus(SERVICE_STOP_PENDING);
            return ERROR_SUCCESS;

        default :
            DPRINT1("  Control %lu received\n", dwControl);
            return ERROR_CALL_NOT_IMPLEMENTED;
    }
}

VOID DealWithDeviceEvent();

VOID WINAPI
ServiceMain(DWORD argc, LPTSTR *argv)
{
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

    ShutdownEvent = CreateEventW(NULL, TRUE, FALSE, NULL);

    DealWithDeviceEvent();

    UpdateServiceStatus(SERVICE_RUNNING);

    WaitForSingleObject(ShutdownEvent, INFINITE);
    CloseHandle(ShutdownEvent);

    UpdateServiceStatus(SERVICE_STOPPED);
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
