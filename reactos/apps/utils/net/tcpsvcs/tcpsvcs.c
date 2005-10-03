#include <stdio.h>
#include <winsock2.h>
#include <tchar.h>
#include "tcpsvcs.h"

#if 0
/*
 * globals
 */
static SERVICE_STATUS hServStatus;
static SERVICE_STATUS_HANDLE hSStat;
FILE *hLogFile;
BOOL bLogEvents = TRUE;
BOOL ShutDown, PauseFlag;
LPTSTR LogFileName = "tcpsvcs_log.txt";

static SERVICE_TABLE_ENTRY
ServiceTable[2] =
{
    {_T("tcpsvcs"), ServiceMain},
    {NULL, NULL}
};
#endif

static SERVICES
Services[NUM_SERVICES] =
{
    {ECHO_PORT, _T("Echo"), EchoHandler},
    {DISCARD_PORT, _T("Discard"), DiscardHandler},
    {DAYTIME_PORT, _T("Daytime"), DaytimeHandler},
    {QOTD_PORT, _T("QOTD"), QotdHandler},
    {CHARGEN_PORT, _T("Chargen"), ChargenHandler}
};


int main(void)
{
    DWORD dwThreadId[NUM_SERVICES];
    HANDLE hThread[NUM_SERVICES];
    WSADATA wsaData;
    DWORD RetVal;
    INT i;

    if ((RetVal = WSAStartup(MAKEWORD(2, 2), &wsaData)) != 0)
    {
        _tprintf(_T("WSAStartup() failed : %lu\n"), RetVal);
        return -1;
    }

    /* Create MAX_THREADS worker threads. */
    for( i=0; i<NUM_SERVICES; i++ )
    {
        _tprintf(_T("Starting %s server....\n"), Services[i].Name);

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
            _tprintf(_T("Failed to start %s server....\n"), Services[i].Name);
            ExitProcess(i);
        }
    }

    /* Wait until all threads have terminated. */
    WaitForMultipleObjects(NUM_SERVICES, hThread, TRUE, INFINITE);

    /* Close all thread handles upon completion. */
    for(i=0; i<NUM_SERVICES; i++)
    {
        CloseHandle(hThread[i]);
    }
    return 0;
}



/* code to run tcpsvcs as a service */
#if 0
int
main(int argc, char *argv[])
{
    //DPRINT("tcpsvcs: main() started. See tcpsvcs_log.txt for info\n");

    if (!StartServiceCtrlDispatcher(ServiceTable))
        _tprintf(_T("failed to start the service control dispatcher\n"));

    //DPRINT("tcpsvcs: main() done\n");

    return 0;
}


static VOID WINAPI
ServiceMain(DWORD argc, LPTSTR argv[])
{
    DWORD i;

    hLogFile = fopen(LogFileName, _T("w+"));
    if (hLogFile == NULL)
        return;

    LogEvent(_T("Entering ServiceMain"), 0, FALSE);

    hServStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    hServStatus.dwCurrentState = SERVICE_START_PENDING;
    hServStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP |
        SERVICE_ACCEPT_SHUTDOWN | SERVICE_ACCEPT_PAUSE_CONTINUE;
    hServStatus.dwWin32ExitCode = ERROR_SERVICE_SPECIFIC_ERROR;
    hServStatus.dwServiceSpecificExitCode = 0;
    hServStatus.dwCheckPoint = 0;
    hServStatus.dwWaitHint = 2*CS_TIMEOUT;

    hSStat = RegisterServiceCtrlHandler("tcpsvcs", ServerCtrlHandler);
    if (hSStat == 0)
        LogEvent(_T("Failed to register service\n"), 100, TRUE);

    LogEvent(_T("Control handler registered successfully"), 0, FALSE);
    SetServiceStatus (hSStat, &hServStatus);
    LogEvent(_T("Service status set to SERVICE_START_PENDING"), 0, FALSE);

    if (CreateServers() != 0)
    {
        hServStatus.dwCurrentState = SERVICE_STOPPED;
        hServStatus.dwServiceSpecificExitCode = 1;
        SetServiceStatus(hSStat, &hServStatus);
        return;
    }

    LogEvent(_T("Service threads shut down. Set SERVICE_STOPPED status"), 0, FALSE);
    /*  We will only return here when the ServiceSpecific function
        completes, indicating system shutdown. */
    UpdateStatus (SERVICE_STOPPED, 0);
    LogEvent(_T("Service status set to SERVICE_STOPPED"), 0, FALSE);
    fclose(hLogFile);  /*  Clean up everything, in general */
    return;

}

VOID WINAPI
ServerCtrlHandler(DWORD Control)
{
    switch (Control)
    {
        case SERVICE_CONTROL_SHUTDOWN: /* fall through */
        case SERVICE_CONTROL_STOP:
            ShutDown = TRUE;
            UpdateStatus(SERVICE_STOP_PENDING, -1);
            break;
        case SERVICE_CONTROL_PAUSE:
            PauseFlag = TRUE;
            break;
        case SERVICE_CONTROL_CONTINUE:
            PauseFlag = FALSE;
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
    if (Check < 0 ) hServStatus.dwCheckPoint++;
    else            hServStatus.dwCheckPoint = Check;
    if (NewStatus >= 0) hServStatus.dwCurrentState = NewStatus;
    if (!SetServiceStatus (hSStat, &hServStatus))
        LogEvent (_T("Cannot set service status"), 101, TRUE);
    return;
}

INT
CreateServers()
{
    DWORD dwThreadId[NUM_SERVICES];
    HANDLE hThread[NUM_SERVICES];
    INT i;

    UpdateStatus(-1, -1); /* increment checkpoint */

    /* Create MAX_THREADS worker threads. */
    for( i=0; i<NUM_SERVICES; i++ )
    {
        _tprintf(_T("Starting %s server....\n"), Services[i].Name);

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
            _tprintf(_T("Failed to start %s server....\n"), Services[i].Name);
            ExitProcess(i);
        }
    }

    /* Wait until all threads have terminated. */
    WaitForMultipleObjects(NUM_SERVICES, hThread, TRUE, INFINITE);

    /* Close all thread handles upon completion. */
    for(i=0; i<NUM_SERVICES; i++)
    {
        CloseHandle(hThread[i]);
    }
    return 0;
}


/*  LogEvent is similar to the ReportError function used elsewhere
    For a service, however, we ReportEvent rather than write to standard
    error. Eventually, this function should go into the utility
    library.  */
VOID
LogEvent (LPCTSTR UserMessage, DWORD ExitCode, BOOL PrintErrorMsg)
{
    DWORD eMsgLen, ErrNum = GetLastError ();
    LPTSTR lpvSysMsg;
    TCHAR MessageBuffer[512];

    if (PrintErrorMsg) {
        eMsgLen = FormatMessage (FORMAT_MESSAGE_ALLOCATE_BUFFER |
            FORMAT_MESSAGE_FROM_SYSTEM, NULL,
            ErrNum, MAKELANGID (LANG_NEUTRAL, SUBLANG_DEFAULT),
            (LPTSTR)&lpvSysMsg, 0, NULL);

        _stprintf (MessageBuffer, _T("\n%s %s ErrNum = %d. ExitCode = %d."),
            UserMessage, lpvSysMsg, ErrNum, ExitCode);
        HeapFree (GetProcessHeap (), 0, lpvSysMsg);
                /* Explained in Chapter 6. */
    } else {
        _stprintf (MessageBuffer, _T("\n%s ExitCode = %d."),
            UserMessage, ExitCode);
    }

    fputs (MessageBuffer, hLogFile);

    if (ExitCode > 0)
        ExitProcess (ExitCode);
    else
        return;
}

#endif
