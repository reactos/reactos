/*
* PROJECT:          ReactOS Update Service
* LICENSE:          GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
* PURPOSE:          Automatic Updates service stub.
*                   Stub is required for some application at the installation phase.
* COPYRIGHT:        Copyright 2018 Denis Malikov (filedem@gmail.com)
*/

#include "wuauserv.h"

/* GLOBALS ******************************************************************/

static WCHAR ServiceName[] = L"wuauserv";

static SERVICE_STATUS_HANDLE ServiceStatusHandle;
static SERVICE_STATUS ServiceStatus;

static HANDLE hStopEvent = NULL;

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
        ServiceStatus.dwWaitHint = 1000;
    else
        ServiceStatus.dwWaitHint = 0;

    if (dwState == SERVICE_RUNNING)
        ServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN;

    SetServiceStatus(ServiceStatusHandle,
                     &ServiceStatus);
    DPRINT1("WU UpdateServiceStatus() called\n");
}

static DWORD WINAPI
ServiceControlHandler(DWORD dwControl,
                      DWORD dwEventType,
                      LPVOID lpEventData,
                      LPVOID lpContext)
{
    switch (dwControl)
    {
        case SERVICE_CONTROL_STOP:
            DPRINT1("WU ServiceControlHandler()  SERVICE_CONTROL_STOP received\n");
            SetEvent(hStopEvent);
            UpdateServiceStatus(SERVICE_STOP_PENDING);
            return ERROR_SUCCESS;

        case SERVICE_CONTROL_PAUSE:
            DPRINT1("WU ServiceControlHandler()  SERVICE_CONTROL_PAUSE received\n");
            UpdateServiceStatus(SERVICE_PAUSED);
            return ERROR_SUCCESS;

        case SERVICE_CONTROL_CONTINUE:
            DPRINT1("WU ServiceControlHandler()  SERVICE_CONTROL_CONTINUE received\n");
            UpdateServiceStatus(SERVICE_RUNNING);
            return ERROR_SUCCESS;

        case SERVICE_CONTROL_INTERROGATE:
            DPRINT1("WU ServiceControlHandler()  SERVICE_CONTROL_INTERROGATE received\n");
            SetServiceStatus(ServiceStatusHandle,
                             &ServiceStatus);
            return ERROR_SUCCESS;

        case SERVICE_CONTROL_SHUTDOWN:
            DPRINT1("WU ServiceControlHandler()  SERVICE_CONTROL_SHUTDOWN received\n");
            SetEvent(hStopEvent);
            UpdateServiceStatus(SERVICE_STOP_PENDING);
            return ERROR_SUCCESS;

        default :
            DPRINT1("WU ServiceControlHandler()  Control %lu received\n");
            return ERROR_CALL_NOT_IMPLEMENTED;
    }
}

VOID WINAPI
ServiceMain(DWORD argc, LPTSTR *argv)
{
    UNREFERENCED_PARAMETER(argc);
    UNREFERENCED_PARAMETER(argv);

    DPRINT("WU ServiceMain() called\n");

    ServiceStatusHandle = RegisterServiceCtrlHandlerExW(ServiceName,
                                                        ServiceControlHandler,
                                                        NULL);
    if (!ServiceStatusHandle)
    {
        DPRINT1("RegisterServiceCtrlHandlerExW() failed! (Error %lu)\n", GetLastError());
        return;
    }

    hStopEvent = CreateEventW(NULL, TRUE, FALSE, NULL);
    if (hStopEvent == NULL)
    {
        DPRINT1("CreateEvent() failed! (Error %lu)\n", GetLastError());
        goto done;
    }

    UpdateServiceStatus(SERVICE_RUNNING);

    WaitForSingleObject(hStopEvent, INFINITE);
    CloseHandle(hStopEvent);

done:
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
