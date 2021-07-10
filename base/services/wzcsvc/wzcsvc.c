/*
 * PROJECT:          ReactOS System Services
 * LICENSE:          GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * FILE:             base/services/wzcsvc/wzcsvc.c
 * PURPOSE:          ReactOS Wireless Zero Configuration Service
 * COPYRIGHT:        Copyright 2020 Oleg Dubinskiy (oleg.dubinskij2013@yandex.ua)
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

#include <svc.h>

/* GLOBALS ******************************************************************/

#define SERVICE_NAME L"Wireless Zero Configuration Service"

static SERVICE_STATUS_HANDLE ServiceStatusHandle;
static SERVICE_STATUS ServiceStatus;
static WCHAR ServiceName[] = L"wzcsvc";
static HANDLE hStopEvent = NULL;

/* FUNCTIONS ****************************************************************/

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
    DPRINT1("WZC UpdateServiceStatus() called\n");
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
            DPRINT1("WZC ServiceControlHandler()  SERVICE_CONTROL_STOP received\n");
            SetEvent(hStopEvent);
            UpdateServiceStatus(SERVICE_STOP_PENDING);
            return ERROR_SUCCESS;

        case SERVICE_CONTROL_PAUSE:
            DPRINT1("WZC ServiceControlHandler()  SERVICE_CONTROL_PAUSE received\n");
            UpdateServiceStatus(SERVICE_PAUSED);
            return ERROR_SUCCESS;

        case SERVICE_CONTROL_CONTINUE:
            DPRINT1("WZC ServiceControlHandler()  SERVICE_CONTROL_CONTINUE received\n");
            UpdateServiceStatus(SERVICE_RUNNING);
            return ERROR_SUCCESS;

        case SERVICE_CONTROL_INTERROGATE:
            DPRINT1("WZC ServiceControlHandler()  SERVICE_CONTROL_INTERROGATE received\n");
            SetServiceStatus(ServiceStatusHandle,
                             &ServiceStatus);
            return ERROR_SUCCESS;

        case SERVICE_CONTROL_SHUTDOWN:
            DPRINT1("WZC ServiceControlHandler()  SERVICE_CONTROL_SHUTDOWN received\n");
            SetEvent(hStopEvent);
            UpdateServiceStatus(SERVICE_STOP_PENDING);
            return ERROR_SUCCESS;

        default :
            DPRINT1("WZC ServiceControlHandler()  Control %lu received\n");
            return ERROR_CALL_NOT_IMPLEMENTED;
    }
}

VOID WINAPI
WZCSvcMain(DWORD argc, LPTSTR *argv)
{
    UNREFERENCED_PARAMETER(argc);
    UNREFERENCED_PARAMETER(argv);

    DPRINT("WZC ServiceMain() called\n");
    
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

/* See https://www.geoffchappell.com/studies/windows/win32/services/svchost/dll/svchostpushserviceglobals.htm */
VOID WINAPI
SvchostPushServiceGlobals(SVCHOST_GLOBALS *lpGlobals)
{
    UNIMPLEMENTED;
}

/* EOF */
