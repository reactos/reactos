/*
 * PROJECT:     ReactOS simple TCP/IP services
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        /base/services/tcpsvcs/tcpsvcs.c
 * PURPOSE:     Provide CharGen, Daytime, Discard, Echo, and Qotd services
 * COPYRIGHT:   Copyright 2005 - 2007 Ged Murphy <gedmurphy@reactos.org>
 *
 */

#include "tcpsvcs.h"

#define DEBUG

volatile BOOL bShutDown = FALSE;
volatile BOOL bPause = FALSE;

static SERVICE_STATUS hServStatus;
static SERVICE_STATUS_HANDLE hSStat;

LPCTSTR LogFileName = _T("C:\\tcpsvcs_log.log");
LPTSTR ServiceName = _T("tcpsvcs");

static SERVICES
Services[NUM_SERVICES] =
{
    {ECHO_PORT,    _T("Echo"),    EchoHandler},
    {DISCARD_PORT, _T("Discard"), DiscardHandler},
    {DAYTIME_PORT, _T("Daytime"), DaytimeHandler},
    {QOTD_PORT,    _T("QOTD"),    QotdHandler},
    {CHARGEN_PORT, _T("Chargen"), ChargenHandler}
};

VOID
LogEvent(LPCTSTR UserMessage,
         DWORD ExitCode,
         BOOL PrintErrorMsg)
{
#ifdef DEBUG
    DWORD eMsgLen;
    DWORD ErrNum = GetLastError();
    LPTSTR lpvSysMsg;
    TCHAR MessageBuffer[512];
    FILE *hLogFile = NULL;

    hLogFile = _tfopen(LogFileName, _T("a"));
    if (hLogFile == NULL) return;

    if (PrintErrorMsg)
    {
        eMsgLen = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                                NULL,
                                ErrNum,
                                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                                (LPTSTR)&lpvSysMsg,
                                0,
                                NULL);

        _stprintf(MessageBuffer,
                  _T("\n%s %s ErrNum = %lu ExitCode = %lu"),
                  UserMessage,
                  lpvSysMsg,
                  ErrNum,
                  ExitCode);

        HeapFree(GetProcessHeap(),
                 0,
                 lpvSysMsg);

    }
    else 
    {
        _stprintf(MessageBuffer,
                  _T("\n%s"),
                  UserMessage);
    }

    _fputts(MessageBuffer, hLogFile);

    fclose(hLogFile);
#endif
    if (ExitCode > 0)
        ExitProcess(ExitCode);
    else
        return;
}


VOID
UpdateStatus(DWORD NewStatus,
             DWORD Check)
{
    TCHAR szSet[50];

    if (Check > 0)
        hServStatus.dwCheckPoint += Check;
    else
        hServStatus.dwCheckPoint = Check;

    if (NewStatus > 0)
        hServStatus.dwCurrentState = NewStatus;

    _sntprintf(szSet, 49, _T("setting service to 0x%lu, CheckPoint %lu"), NewStatus, hServStatus.dwCheckPoint);
    LogEvent(szSet, 0, FALSE);

    if (!SetServiceStatus(hSStat, &hServStatus))
        LogEvent(_T("Cannot set service status"), 101, TRUE);

    return;
}



VOID WINAPI
ServiceMain(DWORD argc, LPTSTR argv[])
{
    LogEvent(_T("Starting service. First log entry."), 0, FALSE);
    LogEvent (_T("Entering ServiceMain."), 0, FALSE);

    hServStatus.dwServiceType      = SERVICE_WIN32_OWN_PROCESS;
    hServStatus.dwCurrentState     = SERVICE_STOPPED;
    hServStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN | SERVICE_ACCEPT_PAUSE_CONTINUE;
    hServStatus.dwWin32ExitCode    = ERROR_SERVICE_SPECIFIC_ERROR;
    hServStatus.dwServiceSpecificExitCode = 0;
    hServStatus.dwCheckPoint       = 0;
    hServStatus.dwWaitHint         = 2 * CS_TIMEOUT;

    hSStat = RegisterServiceCtrlHandler(ServiceName, ServerCtrlHandler);
    if (hSStat == 0)
        LogEvent(_T("Failed to register service\n"), 100, TRUE);

    LogEvent(_T("Control handler registered successfully"), 0, FALSE);
    UpdateStatus(SERVICE_START_PENDING, 1);
    LogEvent(_T("Service status set to SERVICE_START_PENDING"), 0, FALSE);

    if (CreateServers() != 0)
    {
        hServStatus.dwServiceSpecificExitCode = 1;
        UpdateStatus(SERVICE_STOPPED, 0);
        return;
    }

    LogEvent(_T("Service threads shut down. Set SERVICE_STOPPED status"), 0, FALSE);
    UpdateStatus(SERVICE_STOPPED, 0);
    LogEvent(_T("Service status set to SERVICE_STOPPED\n"), 0, FALSE);
    LogEvent(_T("Leaving ServiceMain\n"), 0, FALSE);

    return;
}

VOID WINAPI
ServerCtrlHandler(DWORD Control)
{
    switch (Control)
    {
        case SERVICE_CONTROL_SHUTDOWN:
        case SERVICE_CONTROL_STOP:
            LogEvent(_T("\nSetting the service to SERVICE_STOP_PENDING"), 0, FALSE);
            InterlockedExchange((LONG *)&bShutDown, TRUE);
            hServStatus.dwWin32ExitCode = 0;
            hServStatus.dwWaitHint = 0;
            UpdateStatus(SERVICE_STOP_PENDING, 1);
            break;

        case SERVICE_CONTROL_PAUSE: /* not yet implemented */
            LogEvent(_T("Setting the service to SERVICE_PAUSED"), 0, FALSE);
            InterlockedExchange((LONG *)&bPause, TRUE);
            UpdateStatus(SERVICE_PAUSED, 0);
            break;

        case SERVICE_CONTROL_CONTINUE:
            LogEvent(_T("Setting the service to SERVICE_RUNNING"), 0, FALSE);
            InterlockedExchange((LONG *)&bPause, FALSE);
            UpdateStatus(SERVICE_RUNNING, 0);
            break;

        case SERVICE_CONTROL_INTERROGATE:
            break;

        default:
            if (Control > 127 && Control < 256) /* user defined */
            break;
    }

    return;
}


INT
CreateServers()
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
        LogEvent(buf, RetVal, TRUE);
        return -1;
    }

    UpdateStatus(0, 1); /* increment checkpoint */

    LogEvent(_T("\nCreating server Threads"), 0, FALSE);

    /* Create worker threads. */
    for(i = 0; i < NUM_SERVICES; i++)
    {
        _stprintf(buf, _T("Starting %s server"), Services[i].Name);
        LogEvent(buf, 0, FALSE);

        hThread[i] = CreateThread(NULL,            // default security attributes
                                  0,               // use default stack size
                                  StartServer,     // thread function
                                  &Services[i],    // argument to thread function
                                  0,               // use default creation flags
                                  &dwThreadId[i]); // returns the thread identifier

        if (hThread[i] == NULL)
        {
            _stprintf(buf, _T("\nFailed to start %s server\n"), Services[i].Name);
            /* don't exit process via LogEvent. We want to exit via the server
             * which failed to start, which could mean i=0 */
            LogEvent(buf, 0, TRUE);
        }

        UpdateStatus(0, 1); /* increment checkpoint */
    }

    LogEvent(_T("setting service status to running"), 0, FALSE);

    UpdateStatus(SERVICE_RUNNING, 0);

    /* Wait until all threads have terminated. */
    WaitForMultipleObjects(NUM_SERVICES, hThread, TRUE, INFINITE);

    for(i = 0; i < NUM_SERVICES; i++)
    {
        CloseHandle(hThread[i]);
    }

    LogEvent(_T("Detaching Winsock2"), 0, FALSE);
    WSACleanup();

    return 0;
}

int _tmain (int argc, LPTSTR argv [])
{
    SERVICE_TABLE_ENTRY ServiceTable[] =
    {
        {ServiceName, ServiceMain},
        {NULL,        NULL }
    };

    remove(LogFileName);

    if (!StartServiceCtrlDispatcher(ServiceTable))
        LogEvent(_T("failed to start the service control dispatcher\n"), 100, TRUE);

    return 0;
}
