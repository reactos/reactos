/*
 * PROJECT:     ReactOS Remote Procedure Call service
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        /base/services/rpcss/service_main.c
 * PURPOSE:     Service control code
 * COPYRIGHT:   Copyright 2008 Ged Murphy <gedmurphy@reactos.org>
 *
 */

#include "rpcss.h"

BOOL RPCSS_Initialize(void);
BOOL RPCSS_Shutdown(void);

static WCHAR ServiceName[] = L"RpcSs";

typedef struct _ServiceInfo
{
    SERVICE_STATUS servStatus;
    SERVICE_STATUS_HANDLE hStatus;
} SERVICEINFO, *PSERVICEINFO;


static VOID
UpdateStatus(PSERVICEINFO pServInfo,
             DWORD NewStatus,
             DWORD Check)
{
    if (Check > 0)
    {
        pServInfo->servStatus.dwCheckPoint += Check;
    }
    else
    {
        pServInfo->servStatus.dwCheckPoint = Check;
    }

    if (NewStatus > 0)
    {
        pServInfo->servStatus.dwCurrentState = NewStatus;
    }

    SetServiceStatus(pServInfo->hStatus, &pServInfo->servStatus);
}


static BOOL
RunService(PSERVICEINFO pServInfo)
{
    return RPCSS_Initialize();
}

VOID WINAPI
ServerCtrlHandler(DWORD dwControl,
                  DWORD dwEventType,
                  LPVOID lpEventData,
                  LPVOID lpContext)
{
    PSERVICEINFO pServInfo = (PSERVICEINFO)lpContext;

    switch (dwControl)
    {
        case SERVICE_CONTROL_SHUTDOWN:
        case SERVICE_CONTROL_STOP:
            RPCSS_Shutdown();
            pServInfo->servStatus.dwWin32ExitCode = 0;
            pServInfo->servStatus.dwWaitHint = 0;
            UpdateStatus(pServInfo, SERVICE_STOP_PENDING, 1);
            break;

        default:
            break;
    }
}

VOID WINAPI
ServiceMain(DWORD argc, LPWSTR argv[])
{
    SERVICEINFO servInfo;
    HANDLE hThread;
    DWORD dwThreadId;

    servInfo.servStatus.dwServiceType      = SERVICE_WIN32_OWN_PROCESS;
    servInfo.servStatus.dwCurrentState     = SERVICE_STOPPED;
    servInfo.servStatus.dwControlsAccepted = SERVICE_ACCEPT_SHUTDOWN;
    servInfo.servStatus.dwWin32ExitCode    = ERROR_SERVICE_SPECIFIC_ERROR;
    servInfo.servStatus.dwServiceSpecificExitCode = 0;
    servInfo.servStatus.dwCheckPoint       = 0;
    servInfo.servStatus.dwWaitHint         = 1000;

    servInfo.hStatus = RegisterServiceCtrlHandlerExW(ServiceName,
                                                     (LPHANDLER_FUNCTION_EX)ServerCtrlHandler,
                                                     &servInfo);
    if (!servInfo.hStatus) return;

    UpdateStatus(&servInfo, SERVICE_START_PENDING, 1);

    /* Create worker thread */
    hThread = CreateThread(NULL,
                           0,
                           (LPTHREAD_START_ROUTINE)RunService,
                           &servInfo,
                           0,
                           &dwThreadId);
    if (!hThread) return;

    /* Set service status to running */
    UpdateStatus(&servInfo, SERVICE_RUNNING, 0);

    /* Wait until thread has terminated */
    WaitForSingleObject(hThread, INFINITE);

    CloseHandle(hThread);

    UpdateStatus(&servInfo, SERVICE_STOPPED, 0);
}


int wmain(int argc, LPWSTR argv [])
{
    SERVICE_TABLE_ENTRYW ServiceTable[] =
    {
        {ServiceName, ServiceMain},
        {NULL,        NULL }
    };

    return (int)(StartServiceCtrlDispatcherW(ServiceTable) != TRUE);
}
