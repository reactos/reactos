/*
 * PROJECT:     ReactOS simple TCP/IP services
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        /base/services/tcpsvcs/tcpsvcs.c
 * PURPOSE:     Provide CharGen, Daytime, Discard, Echo, and Qotd services
 * COPYRIGHT:   Copyright 2005 - 2008 Ged Murphy <gedmurphy@reactos.org>
 *
 */

#include "tcpsvcs.h"

static LPTSTR ServiceName = _T("tcpsvcs");

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
    {ECHO_PORT,    _T("Echo"),    EchoHandler},
    {DISCARD_PORT, _T("Discard"), DiscardHandler},
    {DAYTIME_PORT, _T("Daytime"), DaytimeHandler},
    {QOTD_PORT,    _T("QOTD"),    QotdHandler},
    {CHARGEN_PORT, _T("Chargen"), ChargenHandler}
};


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
               _T("Service state 0x%lu, CheckPoint %lu"),
               pServInfo->servStatus.dwCurrentState,
               pServInfo->servStatus.dwCheckPoint);
    LogEvent(szSet, 0, 0, LOG_FILE);

    if (!SetServiceStatus(pServInfo->hStatus, &pServInfo->servStatus))
        LogEvent(_T("Cannot set service status"), GetLastError(), 0, LOG_ALL);
}


static BOOL
CreateServers(PSERVICEINFO pServInfo)
{
    DWORD dwThreadId[NUM_SERVICES];
    HANDLE hThread[NUM_SERVICES];
    WSADATA wsaData;
    TCHAR buf[256];
    INT i;
    DWORD RetVal;

    if ((RetVal = WSAStartup(MAKEWORD(2, 2), &wsaData)) != 0)
    {
        _stprintf(buf, _T("WSAStartup() failed : %lu\n"), RetVal);
        LogEvent(buf, 0, 100, LOG_ALL);
        return FALSE;
    }

    UpdateStatus(pServInfo, 0, 1);

    LogEvent(_T("\nCreating server Threads"), 0, 0, LOG_FILE);

    /* Create worker threads. */
    for(i = 0; i < NUM_SERVICES; i++)
    {
        _stprintf(buf, _T("Creating thread for %s server"), Services[i].Name);
        LogEvent(buf, 0, 0, LOG_FILE);

        hThread[i] = CreateThread(NULL,
                                  0,
                                  StartServer,
                                  &Services[i],
                                  0,
                                  &dwThreadId[i]);

        if (hThread[i] == NULL)
        {
            _stprintf(buf, _T("\nFailed to start %s server\n"), Services[i].Name);
            LogEvent(buf, GetLastError(), 0, LOG_ALL);
        }

        UpdateStatus(pServInfo, 0, 1);
    }

    LogEvent(_T("Setting service status to running"), 0, 0, LOG_FILE);
    UpdateStatus(pServInfo, SERVICE_RUNNING, 0);

    /* Wait until all threads have terminated. */
    WaitForMultipleObjects(NUM_SERVICES, hThread, TRUE, INFINITE);

    for(i = 0; i < NUM_SERVICES; i++)
    {
        if (hThread[i] != NULL)
            CloseHandle(hThread[i]);
    }

    LogEvent(_T("Detaching Winsock2"), 0, 0, LOG_FILE);
    WSACleanup();

    return 0;
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
            LogEvent(_T("\nSetting the service to SERVICE_STOP_PENDING"), 0, 0, LOG_FILE);
            InterlockedExchange((LONG *)&bShutdown, TRUE);
            pServInfo->servStatus.dwWin32ExitCode = 0;
            pServInfo->servStatus.dwWaitHint = 0;
            UpdateStatus(pServInfo, SERVICE_STOP_PENDING, 1);
            break;

        case SERVICE_CONTROL_PAUSE: /* not yet implemented */
            LogEvent(_T("Setting the service to SERVICE_PAUSED"), 0, 0, LOG_FILE);
            InterlockedExchange((LONG *)&bPause, TRUE);
            UpdateStatus(pServInfo, SERVICE_PAUSED, 0);
            break;

        case SERVICE_CONTROL_CONTINUE:
            LogEvent(_T("Setting the service to SERVICE_RUNNING"), 0, 0, LOG_FILE);
            InterlockedExchange((LONG *)&bPause, FALSE);
            UpdateStatus(pServInfo, SERVICE_RUNNING, 0);
            break;

        case SERVICE_CONTROL_INTERROGATE:
            break;

        default:
            if (dwControl > 127 && dwControl < 256) /* user defined */
                LogEvent(_T("User defined control code"), 0, 0, LOG_FILE);
            else
                LogEvent(_T("ERROR: Bad control code"), 0, 0, LOG_FILE);
            break;
    }
}

VOID WINAPI
ServiceMain(DWORD argc, LPTSTR argv[])
{
    SERVICEINFO servInfo;

    LogEvent (_T("Entering ServiceMain."), 0, 0, LOG_FILE);

    servInfo.servStatus.dwServiceType      = SERVICE_WIN32_OWN_PROCESS;
    servInfo.servStatus.dwCurrentState     = SERVICE_STOPPED;
    servInfo.servStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN | SERVICE_ACCEPT_PAUSE_CONTINUE;
    servInfo.servStatus.dwWin32ExitCode    = ERROR_SERVICE_SPECIFIC_ERROR;
    servInfo.servStatus.dwServiceSpecificExitCode = 0;
    servInfo.servStatus.dwCheckPoint       = 0;
    servInfo.servStatus.dwWaitHint         = 2 * CS_TIMEOUT;

    LogEvent(_T("Registering service control handler"), 0, 0, LOG_FILE);
    servInfo.hStatus = RegisterServiceCtrlHandlerEx(ServiceName,
                                                    (LPHANDLER_FUNCTION_EX)ServerCtrlHandler,
                                                    &servInfo);
    if (!servInfo.hStatus)
        LogEvent(_T("Failed to register service\n"), GetLastError(), 100, LOG_ALL);

    UpdateStatus(&servInfo, SERVICE_START_PENDING, 1);

    if (!CreateServers(&servInfo))
    {
        servInfo.servStatus.dwServiceSpecificExitCode = 1;
        UpdateStatus(&servInfo, SERVICE_STOPPED, 0);
        return;
    }

    LogEvent(_T("Service threads shut down. Set SERVICE_STOPPED status"), 0, 0, LOG_FILE);
    UpdateStatus(&servInfo, SERVICE_STOPPED, 0);

    LogEvent(_T("Leaving ServiceMain\n"), 0, 0, LOG_FILE);
}


int _tmain (int argc, LPTSTR argv [])
{
    SERVICE_TABLE_ENTRY ServiceTable[] =
    {
        {ServiceName, ServiceMain},
        {NULL,        NULL }
    };

    InitLogging();

    if (!StartServiceCtrlDispatcher(ServiceTable))
        LogEvent(_T("failed to start the service control dispatcher"), GetLastError(), 101, LOG_ALL);

    UninitLogging();

    return 0;
}
