/*
 * PROJECT:     ReactOS simple TCP/IP services
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        base/services/tcpsvcs/tcpsvcs.c
 * PURPOSE:     Provide CharGen, Daytime, Discard, Echo, and Qotd services
 * COPYRIGHT:   Copyright 2005 - 2008 Ged Murphy <gedmurphy@reactos.org>
 *
 */

#include "tcpsvcs.h"

#include <winsvc.h>

static WCHAR ServiceName[] = L"tcpsvcs";

volatile BOOL bShutdown = FALSE;
volatile BOOL bPause = FALSE;

typedef struct _ServiceInfo
{
    SERVICE_STATUS servStatus;
    SERVICE_STATUS_HANDLE hStatus;
} SERVICEINFO, *PSERVICEINFO;

static SERVICES
Services[NUM_SERVICES] =
{
    {ECHO_PORT,    L"Echo",    EchoHandler},
    {DISCARD_PORT, L"Discard", DiscardHandler},
    {DAYTIME_PORT, L"Daytime", DaytimeHandler},
    {QOTD_PORT,    L"QOTD",    QotdHandler},
    {CHARGEN_PORT, L"Chargen", ChargenHandler}
};


static VOID
UpdateStatus(PSERVICEINFO pServInfo,
             DWORD NewStatus,
             DWORD Check)
{
    WCHAR szSet[50];

    if (Check > 0)
        pServInfo->servStatus.dwCheckPoint += Check;
    else
        pServInfo->servStatus.dwCheckPoint = Check;

    if (NewStatus > 0)
        pServInfo->servStatus.dwCurrentState = NewStatus;

    _snwprintf(szSet,
               49,
               L"Service state 0x%lu, CheckPoint %lu",
               pServInfo->servStatus.dwCurrentState,
               pServInfo->servStatus.dwCheckPoint);
    LogEvent(szSet, 0, 0, LOG_FILE);

    if (!SetServiceStatus(pServInfo->hStatus, &pServInfo->servStatus))
        LogEvent(L"Cannot set service status", GetLastError(), 0, LOG_ALL);
}


static BOOL
CreateServers(PSERVICEINFO pServInfo)
{
    DWORD dwThreadId[NUM_SERVICES];
    HANDLE hThread[NUM_SERVICES];
    WSADATA wsaData;
    WCHAR buf[256];
    INT i;
    DWORD RetVal;

    if ((RetVal = WSAStartup(MAKEWORD(2, 2), &wsaData)) != 0)
    {
        swprintf(buf, L"WSAStartup() failed : %lu\n", RetVal);
        LogEvent(buf, 0, 100, LOG_ALL);
        return FALSE;
    }

    UpdateStatus(pServInfo, 0, 1);

    LogEvent(L"\nCreating server Threads", 0, 0, LOG_FILE);

    /* Create worker threads. */
    for (i = 0; i < NUM_SERVICES; i++)
    {
        swprintf(buf, L"Creating thread for %s server", Services[i].lpName);
        LogEvent(buf, 0, 0, LOG_FILE);

        hThread[i] = CreateThread(NULL,
                                  0,
                                  StartServer,
                                  &Services[i],
                                  0,
                                  &dwThreadId[i]);

        if (hThread[i] == NULL)
        {
            swprintf(buf, L"\nError creating %s server thread\n", Services[i].lpName);
            LogEvent(buf, GetLastError(), 0, LOG_ALL);
            return FALSE;
        }

        UpdateStatus(pServInfo, 0, 1);
    }

    LogEvent(L"Setting service status to running", 0, 0, LOG_FILE);
    UpdateStatus(pServInfo, SERVICE_RUNNING, 0);

    /* Wait until all threads have terminated. */
    WaitForMultipleObjects(NUM_SERVICES, hThread, TRUE, INFINITE);

    for (i = 0; i < NUM_SERVICES; i++)
    {
        if (hThread[i] != NULL)
            CloseHandle(hThread[i]);
    }

    LogEvent(L"Detaching Winsock2", 0, 0, LOG_FILE);
    WSACleanup();

    return TRUE;
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
            LogEvent(L"\nSetting the service to SERVICE_STOP_PENDING", 0, 0, LOG_FILE);
            InterlockedExchange((LONG *)&bShutdown, TRUE);
            pServInfo->servStatus.dwWin32ExitCode = 0;
            pServInfo->servStatus.dwWaitHint = 5000;
            UpdateStatus(pServInfo, SERVICE_STOP_PENDING, 1);
            break;

        case SERVICE_CONTROL_PAUSE: /* not yet implemented */
            LogEvent(L"Setting the service to SERVICE_PAUSED", 0, 0, LOG_FILE);
            InterlockedExchange((LONG *)&bPause, TRUE);
            UpdateStatus(pServInfo, SERVICE_PAUSED, 0);
            break;

        case SERVICE_CONTROL_CONTINUE:
            LogEvent(L"Setting the service to SERVICE_RUNNING", 0, 0, LOG_FILE);
            InterlockedExchange((LONG *)&bPause, FALSE);
            UpdateStatus(pServInfo, SERVICE_RUNNING, 0);
            break;

        case SERVICE_CONTROL_INTERROGATE:
            SetServiceStatus(pServInfo->hStatus, &pServInfo->servStatus);
            break;

        default:
            if (dwControl > 127 && dwControl < 256) /* user defined */
                LogEvent(L"User defined control code", 0, 0, LOG_FILE);
            else
                LogEvent(L"ERROR: Bad control code", 0, 0, LOG_FILE);
            break;
    }
}

VOID WINAPI
ServiceMain(DWORD argc, LPWSTR argv[])
{
    SERVICEINFO servInfo;

    LogEvent(L"Entering ServiceMain.", 0, 0, LOG_FILE);

    servInfo.servStatus.dwServiceType      = SERVICE_WIN32_OWN_PROCESS;
    servInfo.servStatus.dwCurrentState     = SERVICE_STOPPED;
    servInfo.servStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN | SERVICE_ACCEPT_PAUSE_CONTINUE;
    servInfo.servStatus.dwWin32ExitCode    = ERROR_SERVICE_SPECIFIC_ERROR;
    servInfo.servStatus.dwServiceSpecificExitCode = 0;
    servInfo.servStatus.dwCheckPoint       = 0;
    servInfo.servStatus.dwWaitHint         = 2 * CS_TIMEOUT;

    LogEvent(L"Registering service control handler", 0, 0, LOG_FILE);
    servInfo.hStatus = RegisterServiceCtrlHandlerExW(ServiceName,
                                                     (LPHANDLER_FUNCTION_EX)ServerCtrlHandler,
                                                     &servInfo);
    if (!servInfo.hStatus)
        LogEvent(L"Failed to register service", GetLastError(), 100, LOG_ALL);

    UpdateStatus(&servInfo, SERVICE_START_PENDING, 1);

    if (!CreateServers(&servInfo))
    {
        LogEvent(L"Error creating servers", GetLastError(), 1, LOG_ALL);
        servInfo.servStatus.dwServiceSpecificExitCode = 1;
        UpdateStatus(&servInfo, SERVICE_STOPPED, 0);
        return;
    }

    LogEvent(L"Service threads shut down. Set SERVICE_STOPPED status", 0, 0, LOG_FILE);
    UpdateStatus(&servInfo, SERVICE_STOPPED, 0);

    LogEvent(L"Leaving ServiceMain\n", 0, 0, LOG_FILE);
}


int _tmain (int argc, LPTSTR argv [])
{
    SERVICE_TABLE_ENTRYW ServiceTable[] =
    {
        {ServiceName, ServiceMain},
        {NULL,        NULL }
    };

    if (InitLogging())
    {
        if (!StartServiceCtrlDispatcherW(ServiceTable))
            LogEvent(L"failed to start the service control dispatcher", GetLastError(), 101, LOG_ALL);

        UninitLogging();
    }

    return 0;
}
