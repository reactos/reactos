/*
 * PROJECT:     ReactOS Print Spooler Service
 * LICENSE:     GNU GPLv2 or any later version as published by the Free Software Foundation
 * PURPOSE:     Main functions
 * COPYRIGHT:   Copyright 2015 Colin Finck <colin@reactos.org>
 */

#include "precomp.h"

SERVICE_STATUS_HANDLE hServiceStatus;
SERVICE_STATUS ServiceStatus;
WCHAR wszServiceName[] = L"Spooler";

static void
_UpdateServiceStatus(DWORD dwNewStatus, DWORD dwCheckPoint)
{
    ServiceStatus.dwCheckPoint = dwCheckPoint;
    ServiceStatus.dwCurrentState = dwNewStatus;
    SetServiceStatus(hServiceStatus, &ServiceStatus);
}

static DWORD WINAPI
_ServiceControlHandlerEx(DWORD dwControl, DWORD dwEventType, LPVOID lpEventData, LPVOID lpContext)
{
    switch (dwControl)
    {
        case SERVICE_CONTROL_SHUTDOWN:
        case SERVICE_CONTROL_STOP:
            _UpdateServiceStatus(SERVICE_STOP_PENDING, 1);
            RpcMgmtStopServerListening(NULL);
            _UpdateServiceStatus(SERVICE_STOPPED, 0);
            return NO_ERROR;

        case SERVICE_CONTROL_INTERROGATE:
            return NO_ERROR;

        default:
            return ERROR_CALL_NOT_IMPLEMENTED;
    }
}

static VOID WINAPI
_ServiceMain(DWORD dwArgc, LPWSTR* lpszArgv)
{
    HANDLE hThread;

    UNREFERENCED_PARAMETER(dwArgc);
    UNREFERENCED_PARAMETER(lpszArgv);

    // Register our service for control
    hServiceStatus = RegisterServiceCtrlHandlerExW(wszServiceName, _ServiceControlHandlerEx, NULL);

    // Report initial SERVICE_START_PENDING status
    ServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN;
    ServiceStatus.dwServiceSpecificExitCode = 0;
    ServiceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    ServiceStatus.dwWaitHint = 4000;
    ServiceStatus.dwWin32ExitCode = NO_ERROR;
    _UpdateServiceStatus(SERVICE_START_PENDING, 0);

    // Create a thread for serving RPC requests
    hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)LrpcThreadProc, NULL, 0, NULL);
    if (!hThread)
    {
        ERR("CreateThread failed with error %u!\n", GetLastError());
        _UpdateServiceStatus(SERVICE_STOPPED, 0);
        return;
    }

    // We don't need the thread handle. Keeping it open blocks the thread from terminating.
    CloseHandle(hThread);

    // Initialize the routing layer in spoolss.dll
    if (!InitializeRouter(hServiceStatus))
    {
        ERR("InitializeRouter failed with error %lu!\n", GetLastError());
        _UpdateServiceStatus(SERVICE_STOPPED, 0);
        return;
    }

    // We're alive!
    _UpdateServiceStatus(SERVICE_RUNNING, 0);
}

int
wmain(int argc, WCHAR* argv[])
{
    SERVICE_TABLE_ENTRYW ServiceTable[] =
    {
        {wszServiceName, _ServiceMain},
        {NULL, NULL}
    };

    UNREFERENCED_PARAMETER(argc);
    UNREFERENCED_PARAMETER(argv);

    StartServiceCtrlDispatcherW(ServiceTable);

    return 0;
}
