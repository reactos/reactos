/*
 * PROJECT:     ReactOS simple TCP/IP services
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        /base/services/tcpsvcs/tcpsvcs.c
 * PURPOSE:     Provide CharGen, Daytime, Discard, Echo, and Qotd services
 * COPYRIGHT:   Copyright 2005 - 2006 Ged Murphy <gedmurphy@gmail.com>
 *
 */
/*
 * TODO:
 * - fix bug when terminating chargen server
 * - log info in the event logger (when it's implemented)
 */


#include "tcpsvcs.h"

//#define NDEBUG
//#include <debug.h>


/*
 * globals
 */
VOID WINAPI ServiceMain(DWORD argc, LPTSTR argv[]);

static SERVICE_STATUS hServStatus;
static SERVICE_STATUS_HANDLE hSStat;

FILE *hLogFile;
BOOL bShutDown = FALSE;
BOOL bPause = FALSE;

LPCTSTR LogFileName = _T("\\tcpsvcs_log.log");
LPTSTR ServiceName = _T("Simp Tcp");
//LPTSTR DisplayName = _T("Simple TCP/IP Services");

static SERVICES
Services[NUM_SERVICES] =
{
    {ECHO_PORT, _T("Echo"), EchoHandler},
    {DISCARD_PORT, _T("Discard"), DiscardHandler},
    {DAYTIME_PORT, _T("Daytime"), DaytimeHandler},
    {QOTD_PORT, _T("QOTD"), QotdHandler},
    {CHARGEN_PORT, _T("Chargen"), ChargenHandler}
};


int
main(void)
{
    SERVICE_TABLE_ENTRY ServiceTable[] =
    {
        {ServiceName, ServiceMain},
        {NULL, NULL}
    };

    //DPRINT("Starting tcpsvcs service. See \system32%s for logs\n", LogFileName);

    if (! StartServiceCtrlDispatcher(ServiceTable))
        LogEvent(_T("failed to start the service control dispatcher\n"), -1, TRUE);

    //DPRINT("Shutdown tcpsvcs service\n");

    return 0;
}


VOID WINAPI
ServiceMain(DWORD argc, LPTSTR argv[])
{
	TCHAR LogFilePath[MAX_PATH + 17];

    if(! GetSystemDirectory(LogFilePath, MAX_PATH))
        return;

    _tcsncat(LogFilePath, LogFileName, 17);

	hLogFile = _tfopen(LogFilePath, _T("a+"));
    if (hLogFile == NULL)
    {
        TCHAR buf[300];

        _sntprintf(buf, 300, _T("Could not open log file: %s\n"), LogFilePath);
        MessageBox(NULL, buf, NULL, MB_OK);
        return;
    }


    LogEvent(_T("Entering ServiceMain\n"), 0, FALSE);

    hServStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    hServStatus.dwCurrentState = SERVICE_START_PENDING;
    hServStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP |
        SERVICE_ACCEPT_SHUTDOWN | SERVICE_ACCEPT_PAUSE_CONTINUE;
    hServStatus.dwWin32ExitCode = ERROR_SERVICE_SPECIFIC_ERROR;
    hServStatus.dwServiceSpecificExitCode = NO_ERROR;
    hServStatus.dwCheckPoint = 0;
    hServStatus.dwWaitHint = 2*CS_TIMEOUT;

    hSStat = RegisterServiceCtrlHandler(ServiceName, ServerCtrlHandler);
    if (hSStat == 0)
        LogEvent(_T("Failed to register service\n"), -1, TRUE);

	LogEvent(_T("Control handler registered successfully\n"), 0, FALSE);
	SetServiceStatus (hSStat, &hServStatus);
	LogEvent(_T("Service status set to SERVICE_START_PENDING\n"), 0, FALSE);

    if (CreateServers() != 0)
    {
        hServStatus.dwCurrentState = SERVICE_STOPPED;
        hServStatus.dwServiceSpecificExitCode = 1;
        SetServiceStatus(hSStat, &hServStatus);
        return;
    }

	LogEvent(_T("Service threads shut down. Set SERVICE_STOPPED status\n"), 0, FALSE);
	/*  We will only return here when the ServiceSpecific function
		completes, indicating system shutdown. */
	UpdateStatus (SERVICE_STOPPED, 0);
	LogEvent(_T("Service status set to SERVICE_STOPPED\n"), 0, FALSE);
	LogEvent(_T("Leaving ServiceMain\n"), 0, FALSE);

	fclose(hLogFile);

	return;

}

VOID WINAPI
ServerCtrlHandler(DWORD Control)
{
    switch (Control)
    {
        case SERVICE_CONTROL_SHUTDOWN: /* fall through */
        case SERVICE_CONTROL_STOP:
			LogEvent(_T("stopping service\n"), 0, FALSE);
            InterlockedExchange((LONG *)&bShutDown, TRUE);
            UpdateStatus(SERVICE_STOP_PENDING, -1);
            break;
        case SERVICE_CONTROL_PAUSE: /* not yet implemented */
			LogEvent(_T("pausing service\n"), 0, FALSE);
            InterlockedExchange((LONG *)&bPause, TRUE);
            break;
        case SERVICE_CONTROL_CONTINUE:
			LogEvent(_T("continuing service\n"), 0, FALSE);
            InterlockedExchange((LONG *)&bPause, FALSE);
            break;
        case SERVICE_CONTROL_INTERROGATE:
            break;
        default:
            if (Control > 127 && Control < 256) /* user defined */
            break;
    }
    UpdateStatus(-1, -1); /* increment checkpoint */
    return;
}


void UpdateStatus (int NewStatus, int Check)
/*  Set a new service status and checkpoint (either specific value or increment) */
{
	if (Check < 0 )
        hServStatus.dwCheckPoint++;
	else
        hServStatus.dwCheckPoint = Check;

	if (NewStatus >= 0)
        hServStatus.dwCurrentState = NewStatus;

	if (! SetServiceStatus (hSStat, &hServStatus))
		LogEvent(_T("Cannot set service status\n"), -1, TRUE);

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

    UpdateStatus(-1, -1); /* increment checkpoint */

    LogEvent(_T("Creating server Threads\n"), 0, FALSE);

    /* Create MAX_THREADS worker threads. */
    for( i=0; i<NUM_SERVICES; i++ )
    {
        _stprintf(buf, _T("Starting %s server....\n"), Services[i].Name);
        LogEvent(buf, 0, FALSE);

        hThread[i] = CreateThread(
            NULL,              // default security attributes
            0,                 // use default stack size
            StartServer,       // thread function
            &Services[i],     // argument to thread function
            0,                 // use default creation flags
            &dwThreadId[i]);   // returns the thread identifier

        /* Check the return value for success. */
        if (hThread[i] == NULL)
        {
            _stprintf(buf, _T("Failed to start %s server....\n"), Services[i].Name);
            /* don't exit process via LogEvent. We want to exit via the server
             * which failed to start, which could mean i=0 */
            LogEvent(buf, 0, TRUE);
            ExitProcess(i);
        }
    }

    LogEvent(_T("setting service status to running\n"), 0, FALSE);

    UpdateStatus(SERVICE_RUNNING, 0);

    /* Wait until all threads have terminated. */
    WaitForMultipleObjects(NUM_SERVICES, hThread, TRUE, INFINITE);

    /* Close all thread handles upon completion. */
    for(i=0; i<NUM_SERVICES; i++)
    {
        CloseHandle(hThread[i]);
    }

    LogEvent(_T("Detaching Winsock2...\n"), 0, FALSE);
    WSACleanup();

    return 0;
}


/*	This is a temperary log system until our eventlog is in place  */

VOID
LogEvent (LPCTSTR UserMessage, INT ExitCode, BOOL PrintErrorMsg)
{
	DWORD eMsgLen, ErrNum = GetLastError ();
	LPTSTR lpvSysMsg;
	TCHAR MessageBuffer[1024];



	if (PrintErrorMsg)
    {
		eMsgLen = FormatMessage (FORMAT_MESSAGE_ALLOCATE_BUFFER |
			FORMAT_MESSAGE_FROM_SYSTEM, NULL,
			ErrNum, MAKELANGID (LANG_NEUTRAL, SUBLANG_DEFAULT),
			(LPTSTR)&lpvSysMsg, 0, NULL);

		_sntprintf(MessageBuffer, 1024, _T("%s %s ErrNum = %lu. ExitCode = %d."),
			UserMessage, lpvSysMsg, ErrNum, ExitCode);
		HeapFree(GetProcessHeap (), 0, lpvSysMsg);
	}
    else
    {
		_sntprintf(MessageBuffer, 1024, _T("%s"), UserMessage);
	}

	_fputts(MessageBuffer, hLogFile);

	if (ExitCode != 0)
		ExitProcess(ExitCode);
	else
		return;
}
