/*
 * PROJECT:     ReactOS services
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:
 * PURPOSE:     skeleton service
 * COPYRIGHT:   Copyright 2008 Ged Murphy <gedmurphy@reactos.org>
 *
 */

#include "myservice.h"

volatile BOOL bShutDown = FALSE;
volatile BOOL bPause = FALSE;

LPTSTR ServiceName = _T("skel_service");

typedef struct _ServiceInfo
{
    SERVICE_STATUS servStatus;
    SERVICE_STATUS_HANDLE hStatus;
} SERVICEINFO, *PSERVICEINFO;

/********* To be moved to new file **********/
typedef struct _ServiceData
{
    INT val1;
    INT val2;
} SERVICEDATA, *PSERVICEDATA;

DWORD WINAPI ThreadProc(LPVOID lpParam)
{
    while (!bShutDown)
        Sleep(1000);

    return 0;
}
/*******************************************/


static VOID
UpdateStatus(PSERVICEINFO pServInfo,
             DWORD NewStatus,
             DWORD Check)
{
    TCHAR szSet[50];

    if (Check > 0)
        pServInfo->servStatus.dwCheckPoint += Check;
    else
        pServInfo->servStatus.dwCheckPoint = Check;

    if (NewStatus > 0)
        pServInfo->servStatus.dwCurrentState = NewStatus;

    _sntprintf(szSet,
               49,
               _T("Service state 0x%lx, CheckPoint %lu"),
               pServInfo->servStatus.dwCurrentState,
               pServInfo->servStatus.dwCheckPoint);
    LogEvent(szSet, 0, 0, LOG_FILE);

    if (!SetServiceStatus(pServInfo->hStatus, &pServInfo->servStatus))
        LogEvent(_T("Cannot set service status"), GetLastError(), 0, LOG_ALL);
}

static BOOL
CreateServiceThread(PSERVICEINFO pServInfo)
{
    HANDLE hThread;
    PSERVICEDATA servData;

    UpdateStatus(pServInfo, 0, 1);

    LogEvent(_T("Creating service thread"), 0, 0, LOG_FILE);

    hThread = CreateThread(NULL,
                           0,
                           ThreadProc,
                           &servData,
                           0,
                           NULL);

    if (!hThread)
    {
        LogEvent(_T("Failed to start service thread"), GetLastError(), 101, LOG_ALL);
        return FALSE;
    }

    UpdateStatus(pServInfo, 0, 1);

    LogEvent(_T("setting service status to running"), 0, 0, LOG_FILE);
    UpdateStatus(pServInfo, SERVICE_RUNNING, 0);

    WaitForSingleObject(hThread, INFINITE);

    if (hThread)
        CloseHandle(hThread);

    return TRUE;
}


DWORD WINAPI
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
            LogEvent(_T("\nSetting the service to SERVICE_STOP_PENDING"), 0, 0, LOG_FILE);
            InterlockedExchange((LONG *)&bShutDown, TRUE);
            pServInfo->servStatus.dwWin32ExitCode = 0;
            pServInfo->servStatus.dwWaitHint = 0;
            UpdateStatus(pServInfo, SERVICE_STOP_PENDING, 1);
            return ERROR_SUCCESS;

        case SERVICE_CONTROL_PAUSE:
            LogEvent(_T("Setting the service to SERVICE_PAUSED"), 0, 0, LOG_FILE);
            InterlockedExchange((LONG *)&bPause, TRUE);
            UpdateStatus(pServInfo, SERVICE_PAUSED, 0);
            return ERROR_SUCCESS;

        case SERVICE_CONTROL_CONTINUE:
            LogEvent(_T("Setting the service to SERVICE_RUNNING"), 0, 0, LOG_FILE);
            InterlockedExchange((LONG *)&bPause, FALSE);
            UpdateStatus(pServInfo, SERVICE_RUNNING, 0);
            return ERROR_SUCCESS;

        case SERVICE_CONTROL_INTERROGATE:
            return ERROR_SUCCESS;

        default:
            if (dwControl >= 128 && dwControl <= 255) /* User defined */
            {
                LogEvent(_T("User defined control code"), 0, 0, LOG_FILE);
                return ERROR_SUCCESS;
            }
            else
            {
                LogEvent(_T("ERROR: Bad control code"), 0, 0, LOG_FILE);
                return ERROR_INVALID_SERVICE_CONTROL;
            }
    }
}


VOID WINAPI
ServiceMain(DWORD argc, LPTSTR argv[])
{
    SERVICEINFO servInfo;

    LogEvent(_T("Entering ServiceMain"), 0, 0, LOG_FILE);

    servInfo.servStatus.dwServiceType      = SERVICE_WIN32_OWN_PROCESS;
    servInfo.servStatus.dwCurrentState     = SERVICE_STOPPED;
    servInfo.servStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN | SERVICE_ACCEPT_PAUSE_CONTINUE;
    servInfo.servStatus.dwWin32ExitCode    = ERROR_SERVICE_SPECIFIC_ERROR;
    servInfo.servStatus.dwServiceSpecificExitCode = 0;
    servInfo.servStatus.dwCheckPoint       = 0;
    servInfo.servStatus.dwWaitHint         = 1000;

    LogEvent(_T("Registering service control handler"), 0, 0, LOG_FILE);
    servInfo.hStatus = RegisterServiceCtrlHandlerEx(ServiceName, ServerCtrlHandler, &servInfo);
    if (!servInfo.hStatus)
        LogEvent(_T("Failed to register service"), GetLastError(), 100, LOG_ALL);

    UpdateStatus(&servInfo, SERVICE_START_PENDING, 1);

    if (!CreateServiceThread(&servInfo))
    {
        servInfo.servStatus.dwServiceSpecificExitCode = 1;
        UpdateStatus(&servInfo, SERVICE_STOPPED, 0);
        return;
    }

    LogEvent(_T("Service thread shut down. Set SERVICE_STOPPED status"), 0, 0, LOG_FILE);
    UpdateStatus(&servInfo, SERVICE_STOPPED, 0);

    LogEvent(_T("Leaving ServiceMain"), 0, 0, LOG_FILE);
}


int _tmain(int argc, LPTSTR argv[])
{
    SERVICE_TABLE_ENTRY ServiceTable[] =
    {
        {ServiceName, ServiceMain},
        {NULL,        NULL }
    };

    InitLogging();

    if (!StartServiceCtrlDispatcher(ServiceTable))
        LogEvent(_T("failed to start the service control dispatcher"), GetLastError(), 101, LOG_ALL);

    return 0;
}
