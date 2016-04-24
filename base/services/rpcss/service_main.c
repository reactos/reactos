/*
 * PROJECT:     ReactOS Remote Procedure Call service
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        base/services/rpcss/service_main.c
 * PURPOSE:     Service control code
 * COPYRIGHT:   Copyright 2008 Ged Murphy <gedmurphy@reactos.org>
 *
 */

#include "rpcss.h"

#include <winsvc.h>

#define NDEBUG
#include <debug.h>

extern BOOL RPCSS_Initialize(void);
extern BOOL RPCSS_Shutdown(void);
extern HANDLE exit_event;

static VOID WINAPI ServiceMain(DWORD, LPWSTR *);
static WCHAR ServiceName[] = L"RpcSs";
SERVICE_TABLE_ENTRYW ServiceTable[] =
{
    { ServiceName, ServiceMain },
    { NULL,        NULL }
};

static SERVICE_STATUS ServiceStatus;
static SERVICE_STATUS_HANDLE ServiceStatusHandle;

DWORD WINAPI
ServiceControlHandler(DWORD dwControl,
                      DWORD dwEventType,
                      LPVOID lpEventData,
                      LPVOID lpContext)
{
    switch (dwControl)
    {
        case SERVICE_CONTROL_SHUTDOWN:
        case SERVICE_CONTROL_STOP:
            SetEvent(exit_event);
            return NO_ERROR;

        case SERVICE_CONTROL_INTERROGATE:
            return NO_ERROR;

        default:
            return ERROR_CALL_NOT_IMPLEMENTED;
    }
}

VOID WINAPI
ServiceMain(DWORD argc, LPWSTR argv[])
{
    DWORD dwError;

    ServiceStatusHandle = RegisterServiceCtrlHandlerExW(ServiceName,
                                                        ServiceControlHandler,
                                                        NULL);
    if (!ServiceStatusHandle)
    {
        dwError = GetLastError();
        DPRINT1("RegisterServiceCtrlHandlerW() failed! (Error %lu)\n", dwError);
        return;
    }

    ServiceStatus.dwServiceType      = SERVICE_WIN32_OWN_PROCESS;
    ServiceStatus.dwCurrentState     = SERVICE_START_PENDING;
    ServiceStatus.dwControlsAccepted = 0;
    ServiceStatus.dwWin32ExitCode    = NO_ERROR;
    ServiceStatus.dwServiceSpecificExitCode = 0;
    ServiceStatus.dwCheckPoint       = 0;
    ServiceStatus.dwWaitHint         = 1000;
    SetServiceStatus(ServiceStatusHandle, &ServiceStatus);

    if (RPCSS_Initialize())
    {
        ServiceStatus.dwCurrentState = SERVICE_RUNNING;
        ServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN;
        SetServiceStatus(ServiceStatusHandle, &ServiceStatus);

        WaitForSingleObject(exit_event, INFINITE);

        ServiceStatus.dwCurrentState = SERVICE_STOPPED;
        SetServiceStatus(ServiceStatusHandle, &ServiceStatus);
        RPCSS_Shutdown();
    }
}

int wmain(int argc, LPWSTR argv[])
{
    if (!StartServiceCtrlDispatcherW(ServiceTable))
    {
        DPRINT1("StartServiceCtrlDispatcherW() failed\n");
        return 1;
    }

    return 0;
}
