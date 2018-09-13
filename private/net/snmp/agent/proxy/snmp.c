/*++

Copyright (c) 1992-1996  Microsoft Corporation

Module Name:

    snmp.c

Abstract:

    Provides service functionality for Proxy Agent.

Environment:

    User Mode - Win32

Revision History:

    10-May-1996 DonRyan
        Removed banner from Technology Dynamics, Inc.

--*/
 
//--------------------------- WINDOWS DEPENDENCIES --------------------------

//--------------------------- STANDARD DEPENDENCIES -- #include<xxxxx.h> ----

#include <windows.h>

#include <process.h>
#include <stdio.h>

#include <winsvc.h>


//--------------------------- MODULE DEPENDENCIES -- #include"xxxxx.h" ------

#include <snmputil.h>
#include "snmpctrl.h"
#include "..\common\evtlog.h"


//--------------------------- SELF-DEPENDENCY -- ONE #include"module.h" -----

//--------------------------- PUBLIC VARIABLES --(same as in module.h file)--
BOOL        noservice;
DWORD       platformId;
DWORD       WinVersion;
HINSTANCE   g_hInstance;

extern HANDLE g_hTerminationEvent;

//--------------------------- PRIVATE CONSTANTS -----------------------------

//--------------------------- PRIVATE STRUCTS -------------------------------

//--------------------------- PRIVATE VARIABLES -----------------------------

SERVICE_STATUS_HANDLE hService = 0;
SERVICE_STATUS status =
  {SERVICE_WIN32, SERVICE_STOPPED, SERVICE_ACCEPT_STOP, NO_ERROR, 0, 0, 0};


//--------------------------- PRIVATE PROTOTYPES ----------------------------

BOOL agentConfigInit(VOID);


//--------------------------- PRIVATE PROCEDURES ----------------------------

static VOID serviceHandlerFunction(
    IN DWORD dwControl)
    {
    SNMPDBG((SNMP_LOG_TRACE, "SNMP: SVC: serviceHandlerFunction entered, dwControl=%d.\n", dwControl));

    // is it a LogLevel change control?
    if      (SNMP_SERVICE_LOGLEVEL_BASE+SNMP_SERVICE_LOGLEVEL_MIN <= dwControl
          && dwControl <= SNMP_SERVICE_LOGLEVEL_BASE+SNMP_SERVICE_LOGLEVEL_MAX)
        {
        INT nLogLevel;

        nLogLevel = dwControl - SNMP_SERVICE_LOGLEVEL_BASE;

        SNMPDBG((SNMP_LOG_TRACE, "SNMP: SVC: loglevel changed to %d.\n", nLogLevel));

        SnmpSvcSetLogLevel(nLogLevel);
        }

    // is it a LogType change control?
    else if (SNMP_SERVICE_LOGTYPE_BASE+SNMP_SERVICE_LOGTYPE_MIN <= dwControl
          && dwControl <= SNMP_SERVICE_LOGTYPE_BASE+SNMP_SERVICE_LOGTYPE_MAX)
        {
        INT nLogType;

        nLogType = dwControl - SNMP_SERVICE_LOGTYPE_BASE;


        if (!noservice) {
            // make sure console based log is prohibited when compiled as service
            nLogType &= ~SNMP_OUTPUT_TO_CONSOLE;
        }

        SNMPDBG((SNMP_LOG_TRACE, "SNMP: SVC: logtype changed to %d.\n", nLogType));

        SnmpSvcSetLogType(nLogType);
        }

    else if (dwControl == SERVICE_CONTROL_STOP)
        {
        status.dwCurrentState = SERVICE_STOP_PENDING;
        status.dwCheckPoint++;
        status.dwWaitHint = 20000;
        if (!SetServiceStatus(hService, &status))
            {
            SNMPDBG((SNMP_LOG_ERROR, "SNMP: SVC: error %d setting service status to pending.\n", GetLastError()));
            SnmpSvcReportEvent(SNMP_EVENT_FATAL_ERROR, 0, NULL, GetLastError());
            exit(1);
            }
        // set event causing trap thread to terminate, followed by comm thread
        if (!SetEvent(g_hTerminationEvent))
            {
            SNMPDBG((SNMP_LOG_ERROR, "SNMP: SVC: error %s setting termination event.\n", GetLastError()));
            SnmpSvcReportEvent(SNMP_EVENT_FATAL_ERROR, 0, NULL, GetLastError());

            status.dwCurrentState = SERVICE_STOPPED;
            status.dwWin32ExitCode = ERROR_SERVICE_SPECIFIC_ERROR;
            status.dwServiceSpecificExitCode = 1; // OPENISSUE - svc err code
            status.dwCheckPoint = 0;
            status.dwWaitHint = 0;
            if (!SetServiceStatus(hService, &status))
                {
                SNMPDBG((SNMP_LOG_ERROR, "SNMP: SVC: error %d setting service status to stopped.\n", GetLastError()));
                SnmpSvcReportEvent(SNMP_EVENT_FATAL_ERROR, 0, NULL, GetLastError());
                exit(1);
                }
            exit(1);
            }
        }

    else
        //   dwControl == SERVICE_CONTROL_INTERROGATE
        //   dwControl == SERVICE_CONTROL_PAUSE
        //   dwControl == SERVICE_CONTROL_CONTINUE
        //   dwControl == <anything else>
        {
        if (status.dwCurrentState == SERVICE_STOP_PENDING ||
            status.dwCurrentState == SERVICE_START_PENDING)
            {
            status.dwCheckPoint++;
            }

        if (!SetServiceStatus(hService, &status))
            {
            SNMPDBG((SNMP_LOG_ERROR, "SNMP: SVC: error %d updating service status.\n", GetLastError()));
            SnmpSvcReportEvent(SNMP_EVENT_FATAL_ERROR, 0, NULL, GetLastError());
            exit(1);
            }
        }

    } // end serviceHandlerFunction()


static VOID serviceMainFunction(
    IN DWORD dwNumServicesArgs,
    IN LPSTR *lpServiceArgVectors)
    {
    
    SNMPDBG((SNMP_LOG_TRACE, "SNMP: INIT: serviceMainFunction entered.\n"));

    while(dwNumServicesArgs--)
        {
        INT nLogLevel;
        INT nLogType;
        INT temp;

        if      (1 == sscanf(*lpServiceArgVectors, "/loglevel:%d", &temp))
            {
            nLogLevel = temp;

            SNMPDBG((SNMP_LOG_TRACE, "SNMP: INIT: loglevel changed to %d.\n", temp));

            SnmpSvcSetLogLevel(nLogLevel);
            }
        else if (1 == sscanf(*lpServiceArgVectors, "/logtype:%d", &temp))
            {
            nLogType = temp;

            SNMPDBG((SNMP_LOG_TRACE, "SNMP: INIT: logtype changed to %d.\n", temp));

            SnmpSvcSetLogType(nLogType);
            }
        else
            {
            SNMPDBG((SNMP_LOG_TRACE, "SNMP: INIT: argument %s invalid.\n",
                      *lpServiceArgVectors));
            }

        lpServiceArgVectors++;
        } // end while()

    if (!noservice) {

        SNMPDBG((SNMP_LOG_TRACE, "SNMP: INIT: registering service control handler.\n"));

        if ((hService = RegisterServiceCtrlHandler("SNMP", serviceHandlerFunction))
            == 0)
            {
            SNMPDBG((SNMP_LOG_ERROR, "SNMP: INIT: error on RegisterServiceCtrlHander %d\n", GetLastError()));
            SnmpSvcReportEvent(SNMP_EVENT_FATAL_ERROR, 0, NULL, GetLastError());

            status.dwCurrentState = SERVICE_STOPPED;
            status.dwWin32ExitCode = ERROR_SERVICE_SPECIFIC_ERROR;
            status.dwServiceSpecificExitCode = 2; // OPENISSUE - svc err code
            status.dwCheckPoint = 0;
            status.dwWaitHint = 0;
            if (!SetServiceStatus(hService, &status))
                {
                SNMPDBG((SNMP_LOG_ERROR, "SNMP: INIT: error %d setting servicee status to stopped.\n", GetLastError()));
                SnmpSvcReportEvent(SNMP_EVENT_FATAL_ERROR, 0, NULL, GetLastError());
                exit(1);
                }
            else
            exit(1);
            }

        SNMPDBG((SNMP_LOG_TRACE, "SNMP: INIT: setting service status to pending.\n"));

        status.dwCurrentState = SERVICE_START_PENDING;
        status.dwWaitHint = 20000;
        if (!SetServiceStatus(hService, &status))
            {
            SNMPDBG((SNMP_LOG_ERROR, "SNMP: INIT: error %d setting service status to pending.\n", GetLastError()));
            SnmpSvcReportEvent(SNMP_EVENT_FATAL_ERROR, 0, NULL, GetLastError());
            exit(1);
            }
    }

    if (!agentConfigInit())
        {
        SNMPDBG((SNMP_LOG_ERROR, "SNMP: INIT: error on agentConfigInit %d\n", GetLastError()));
        SnmpSvcReportEvent(SNMP_EVENT_FATAL_ERROR, 0, NULL, GetLastError());

        status.dwCurrentState = SERVICE_STOPPED;
        status.dwWin32ExitCode = ERROR_SERVICE_SPECIFIC_ERROR;
        status.dwServiceSpecificExitCode = 3; // OPENISSUE - svc err code
        status.dwCheckPoint = 0;
        status.dwWaitHint = 0;
        if (!SetServiceStatus(hService, &status))
            {
            SNMPDBG((SNMP_LOG_ERROR, "SNMP: INIT: error %d setting service status to stopped.\n", GetLastError()));
            SnmpSvcReportEvent(SNMP_EVENT_FATAL_ERROR, 0, NULL, GetLastError());
            exit(1);
            }
        exit(1);
        }

    // above function will not return until running thread(s) terminate
    
    if (!noservice)
        {
        SNMPDBG((SNMP_LOG_TRACE, "SNMP: INIT: setting service status to stopped.\n"));

        status.dwCurrentState = SERVICE_STOPPED;
        status.dwCheckPoint = 0;
        status.dwWaitHint = 0;
        if (!SetServiceStatus(hService, &status))
            {
            SNMPDBG((SNMP_LOG_ERROR, "SNMP: INIT: error %d setting service status to stopped.\n", GetLastError()));
            SnmpSvcReportEvent(SNMP_EVENT_FATAL_ERROR, 0, NULL, GetLastError());
            exit(1);
            }
        }
    } // end serviceMainFunction()


static BOOL breakHandler(
    IN ULONG ulType)
    {
    UNREFERENCED_PARAMETER(ulType);

    SNMPDBG((SNMP_LOG_TRACE, "SNMP: SVC: break intercepted, cleaning up...\n"));

    SetEvent(g_hTerminationEvent);

    return FALSE;

    } // end breakHandler()


// to simulate service controller functionality when testing as a process
static BOOL StartServiceCtrlLocalDispatcher(LPSERVICE_TABLE_ENTRY junk)
    {
    DWORD p1=0; LPSTR *p2=NULL;

    (*(junk->lpServiceProc))(p1, p2);

    return TRUE;

    } // end StartServiceCtrlDispatcher()

#if defined(NT)
//--------------------------- PUBLIC PROCEDURES -----------------------------
//---- Win95's WinMain and NT's main are mutually exclusive routines---------
INT __cdecl main(
    IN int argc,      //argument count
    IN char *argv[])  //argument vector
    {
    static SERVICE_TABLE_ENTRY serviceStartTable[2] =
        {{"SNMP", serviceMainFunction}, {NULL, NULL}};

    OSVERSIONINFO   OsInfo;

    UNREFERENCED_PARAMETER(argc);
    UNREFERENCED_PARAMETER(argv);

    OsInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

    if (!GetVersionEx(&OsInfo))
        exit(1);

    switch (OsInfo.dwPlatformId)
    {
    case VER_PLATFORM_WIN32_NT:
        noservice = FALSE;
        platformId = VER_PLATFORM_WIN32_NT;
        SNMPDBG((SNMP_LOG_TRACE, "SNMP: INIT: platform is Windows NT.\n"));
        break;

    case VER_PLATFORM_WIN32_WINDOWS:
        noservice = TRUE;
        platformId = VER_PLATFORM_WIN32_WINDOWS;
        SNMPDBG((SNMP_LOG_TRACE, "SNMP: INIT: platform in Windows 95.\n"));
        break;

    default:
        SNMPDBG((SNMP_LOG_ERROR, "SNMP: INIT: unsupported platform 0x%x.\n", OsInfo.dwPlatformId));
        SnmpSvcReportEvent(SNMP_EVENT_INVALID_PLATFORM_ID, 0, NULL, NO_ERROR);
        exit(1);
    }


tryagain:

    if (noservice) {

        // intercept ctrl-c and ctrl-break to allow cleanup to be done

        if (!SetConsoleCtrlHandler(breakHandler, TRUE))
            {
            SNMPDBG((SNMP_LOG_ERROR, "SNMP: INIT: error on SetConsoleCtrlHandler %d\n", GetLastError()));

            //not serious error.
            }
    }

    SNMPDBG((SNMP_LOG_TRACE, "SNMP: INIT: service starting...\n"));

    //start service control dispatcher

    if (noservice) {
        if (!StartServiceCtrlLocalDispatcher(serviceStartTable))
            {
            SNMPDBG((SNMP_LOG_ERROR, "SNMP: INIT: error on StartServiceCtrlDispatch %d\n", GetLastError()));
            exit(1);
            }
    } else {
        if (!StartServiceCtrlDispatcher(serviceStartTable))
            {
            if (GetLastError() == 1063) {
                SNMPDBG((SNMP_LOG_TRACE, "SNMP: INIT: retry with local service control dispatcher.\n"));
                noservice = TRUE;
                goto tryagain;
            }
            SNMPDBG((SNMP_LOG_ERROR, "SNMP: INIT: error on StartServiceCtrlDispatch %d\n", GetLastError()));
            exit(1);
            }
    }

    // above function will not return until running service(s) terminate

    SNMPDBG((SNMP_LOG_TRACE, "SNMP: INIT: service terminated.\n"));
    return 0;

    } // end main()

//-------------------------------- END NT Specific -------------------------------
#else
//------------------------------ Begin Win95 Specific -----------------------------
LRESULT CALLBACK WndProc(
    HWND hWnd,         // window handle
    UINT message,      // type of message
    WPARAM uParam,     // additional information
    LPARAM lParam);    // additional information

#define SNMP_AGENT_EVENT            "SNMP.Agent.Event"

#define RSP_SIMPLE_SERVICE      0x00000001
#define RSP_UNREGISTER_SERVICE  0x00000000

char szAppName[64]; // The name of this application
char szTitle[32];   // The title bar text
char szHelpStr[32]; // Help flag "Help"
char szQuestStr[32];// Abriev. Help Flag "?"
char szCloseStr[32];// Close flag "close"
char szDestroyStr[32];  //Destroy flag "destroy"
char szHelpText1[256];    //Help String
char szHelpText2[64];    //Help String
char szHelpText3[128];    //Help String

HWND hWnd; // Main window handle.

BOOL InitApplication(HINSTANCE hInstance);
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow);
BOOL GetStrings(HINSTANCE hInstance);


//int APIENTRY WinMain
INT APIENTRY WinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR lpCmdLine,
    int nCmdShow)
{

    MSG     msg;
    HANDLE  SnmpAgentEvent, hThread, hKernel32 = NULL;
    DWORD   threadId, LastError;
    BOOL    fKillParent = FALSE, fRegSrvcProc = FALSE;
    FARPROC pRegSrvcProc;

    LPCSTR  event_name = SNMP_AGENT_EVENT;
    DWORD   err;

#if DBG

    SnmpSvcSetLogLevel(SNMP_LOG_VERBOSE);
    SnmpSvcSetLogType(SNMP_OUTPUT_TO_LOGFILE|SNMP_OUTPUT_TO_DEBUGGER);

    SNMPDBG((SNMP_LOG_TRACE,"SNMP: SVC: WinMain entered.\n"));

#endif

    if ((WinVersion = (GetVersion() & 0x000000ff)) == 0x04)
    {
        if ((hKernel32 = GetModuleHandle("kernel32.dll")) == NULL)
        {
            // This should never happen but we'll try and load the library anyway
            if ((hKernel32 = LoadLibrary("kernel32.dll")) == NULL)
                fRegSrvcProc = FALSE;
        }

        if (hKernel32)
        {
            if ((pRegSrvcProc = GetProcAddress(hKernel32,"RegisterServiceProcess")) == NULL)
                fRegSrvcProc = FALSE;
            else
                fRegSrvcProc = TRUE;
        }
    }
    else
        fRegSrvcProc = FALSE;

    //
    // Other instances of regserv running?
    //

    SnmpAgentEvent = OpenEvent(SYNCHRONIZE, FALSE, event_name);
    if (SnmpAgentEvent == NULL) {
        if ( (SnmpAgentEvent = CreateEvent(NULL, FALSE, TRUE, event_name)) == NULL)
        {
            LastError = GetLastError();
            SNMPDBG((SNMP_LOG_ERROR, "SNMP: INIT: Create Event Failed w/ %d\n", LastError));
            return 1;

        }

    } else {
        SnmpAgentEvent = NULL;  // another instance is running.
    }

    if (!GetStrings(hInstance))
    {
        return 1;

    }

    platformId = VER_PLATFORM_WIN32_WINDOWS;

    // we are not on NT... is our command line setup to kill the parent?
    if ( ( lpCmdLine[0]=='-') ||
         ( lpCmdLine[0]=='/') )
    {

        // is it CLOSE or DESTROY??
        if ( _strnicmp ( &lpCmdLine[1], szCloseStr, strlen(szCloseStr) ) ==0 )
            fKillParent = TRUE;
        else if ( _strnicmp ( &lpCmdLine[1], szDestroyStr, strlen(szDestroyStr) ) ==0 )
            fKillParent = TRUE;
        else if (( _strnicmp ( &lpCmdLine[1], szHelpStr, strlen(szHelpStr) )==0 )||
                 ( _strnicmp ( &lpCmdLine[1], szQuestStr, strlen(szQuestStr) )==0 ))
        {   // they have asked for help...
            MessageBox (GetTopWindow(GetDesktopWindow()),
                        szHelpText1,
                        szHelpText2,
                        MB_OK );
            return 0;
        }

    }

    if ((hPrevInstance) ||
        (SnmpAgentEvent == NULL))
    {
        SNMPDBG((SNMP_LOG_ERROR,"SNMP: INIT: service already running\n"));
        if (!fKillParent) // if we are not killing the parent, only one agent at a time...
        {
            HANDLE hParentWin = GetTopWindow(GetDesktopWindow());
            MessageBox (hParentWin,
                        szHelpText3,
                        szHelpText2,
                        MB_OK );
            return 1;
        }

        if (hPrevInstance == NULL)
            if ( (hPrevInstance = (HINSTANCE)FindWindow(szAppName,NULL)) == NULL)
                return 1;
        // else send our parent a die message...
        PostMessage ( (HWND)hPrevInstance, WM_CLOSE, 0,0 );
        return 0;
    }

    if (fKillParent)
        // no parent to kill
        return 2;

    if(!InitApplication(hInstance)) 
    {
        SNMPDBG((SNMP_LOG_ERROR, "SNMP: INIT: Unable to register window class\n"));
        return (FALSE);
    }

    if (!InitInstance(hInstance, SW_HIDE)) {    // ignore nCmdShow
        SNMPDBG((SNMP_LOG_ERROR,"SNMP: INIT: Unable to create main window\n"));
        return (FALSE);
    }

    noservice = TRUE;

    // create synchronize object
    g_hTerminationEvent = CreateEvent(
                            NULL,   // lpEventAttributes
                            TRUE,   // bManualReset
                            FALSE,  // bInitialState
                            NULL    // lpName
                            );

    if ((hThread = CreateThread(NULL, 0,
                                    (LPTHREAD_START_ROUTINE)serviceMainFunction,
                                    NULL, 0, &threadId)) == 0)
        {
        SNMPDBG((SNMP_LOG_ERROR, "SNMP: INIT: error %d creating main service thread.\n", GetLastError()));

        }
    else
        {
        SNMPDBG((SNMP_LOG_TRACE, "SNMP: INIT: created serviceMainFunction tid=0x%lx.\n", threadId));
        }

    if (fRegSrvcProc)
        (*pRegSrvcProc)(GetCurrentProcessId(), RSP_SIMPLE_SERVICE);

    // Acquire and dispatch messages until a WM_QUIT message is received.

    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg); // Translates virtual key codes
        DispatchMessage(&msg);  // Dispatches message to window
    }

    // set event causing trap thread to terminate, followed by comm thread
    if (!SetEvent(g_hTerminationEvent))
        {
        SNMPDBG((SNMP_LOG_ERROR, "SNMP: INIT: error on SetEvent %d\n", GetLastError()));
        }

    SNMPDBG((SNMP_LOG_TRACE, "SNMP: INIT: waiting for service to stop.\n"));
    
    // wait for service thread to exit
    WaitForSingleObject(hThread,INFINITE);

    if (fRegSrvcProc)
        (*pRegSrvcProc)(GetCurrentProcessId(), RSP_UNREGISTER_SERVICE);

    // release event handle
    CloseHandle(g_hTerminationEvent);

    SNMPDBG((SNMP_LOG_TRACE, "SNMP: INIT: SNMP Service exiting 0\n"));
    return(0);

    UNREFERENCED_PARAMETER(lpCmdLine);
}

//
// GetStrings - retrieves resource strings from exe
//

BOOL GetStrings(HINSTANCE hInstance)
{
    DWORD   err, LastError;

    if ((err = LoadString(hInstance, IDS_TITLE_BAR, szTitle, sizeof(szTitle))) == 0)
    {
        goto ErrorExit;
    }

    if ((err = LoadString(hInstance, IDS_APP_NAME, szAppName, sizeof(szAppName))) == 0)
    {
        goto ErrorExit;
    }

    if ((err = LoadString(hInstance, IDS_HELP_STRING, szHelpStr, sizeof(szHelpStr))) == 0)
    {
        goto ErrorExit;
    }

    if ((err = LoadString(hInstance, IDS_QUEST_STRING, szQuestStr, sizeof(szQuestStr))) == 0)
    {
        goto ErrorExit;
    }

    if ((err = LoadString(hInstance, IDS_CLOSE_STRING, szCloseStr, sizeof(szCloseStr))) == 0)
    {
        goto ErrorExit;
    }

    if ((err = LoadString(hInstance, IDS_DESTROY_STRING, szDestroyStr, sizeof(szDestroyStr))) == 0)
    {
        goto ErrorExit;
    }

    if ((err = LoadString(hInstance, IDS_HELP_TEXT1, szHelpText1, sizeof(szHelpText1))) == 0)
    {
        goto ErrorExit;
    }

    if ((err = LoadString(hInstance, IDS_HELP_TEXT2, szHelpText2, sizeof(szHelpText2))) == 0)
    {
        goto ErrorExit;
    }

    if ((err = LoadString(hInstance, IDS_HELP_TEXT3, szHelpText3, sizeof(szHelpText3))) == 0)
    {
        goto ErrorExit;
    }

    return TRUE;


ErrorExit:

    LastError = GetLastError();
    SNMPDBG((SNMP_LOG_ERROR, "SNMP: INIT: LoadString Failed w/ %d\n", LastError));
    return FALSE;


}

//
// InitInstance - save instance handle and create main window
//

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    // Save the instance handle in static variable, which will be used in
    // many subsequence calls from this application to Windows.

    g_hInstance = hInstance; // Store instance handle in our global variable

    // Create a main window for this application instance.

    hWnd = CreateWindow(
        szAppName,
        szTitle,
        WS_EX_TRANSPARENT,                      // Window style.
        0, 0, CW_USEDEFAULT, CW_USEDEFAULT, // Use default positioning CW_USEDEAULT
        NULL,            // Overlapped windows have no parent.
        NULL,            // Use the window class menu.
        hInstance,       // This instance owns this window.
        NULL             // We don't use any data in our WM_CREATE
    );

    // If window could not be created, return "failure"
    if (!hWnd) {
        return (FALSE);
    }

    // Make the window visible; update its client area; and return "success"
    ShowWindow(hWnd, nCmdShow); // Show the window
    UpdateWindow(hWnd);         // Sends WM_PAINT message

    return (TRUE);              // We succeeded...
}


//
// InitApplication - initialize window data and register window class
//
// Return - NULL => failure
//

BOOL InitApplication(HINSTANCE hInstance)
{
    WNDCLASS  wc;
    DWORD     LastError;

    // Fill in window class structure with parameters that describe the
    // main window.

    wc.style         = CS_HREDRAW | CS_VREDRAW;// Class style(s).
    wc.lpfnWndProc   = (WNDPROC)WndProc;       // Window Procedure
    wc.cbClsExtra    = 0;                      // No per-class extra data.
    wc.cbWndExtra    = 0;                      // No per-window extra data.
    wc.hInstance     = hInstance;              // Owner of this class
    wc.hIcon         = NULL;
    wc.hCursor       = NULL;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);// Default color
    wc.lpszMenuName  = szAppName;              // Menu name from .RC
    wc.lpszClassName = szAppName;              // Name to register as

    // Register the window class and return success/failure code.

    if (!RegisterClass(&wc))
    {
        LastError = GetLastError();
        SNMPDBG((SNMP_LOG_ERROR, "SNMP: INIT: RegisterClass failed w/ %d\n", LastError ));
        return FALSE;
    }
    else
        return TRUE;
}

//
// WndProc - process messages
//

LRESULT CALLBACK WndProc(
    HWND hWnd,         // window handle
    UINT message,      // type of message
    WPARAM uParam,     // additional information
    LPARAM lParam)     // additional information
{
    switch (message) 
    {
        case WM_ENDSESSION:
        case WM_QUERYENDSESSION:
            if (lParam == 0)
            {
                SNMPDBG((SNMP_LOG_TRACE, "SNMP: INIT: Received shutdown message\n"));
            }
            if (lParam == 1 ) //EWX_REALLYLOGOFF
            {
                SNMPDBG((SNMP_LOG_TRACE, "SNMP: INIT: Received logoff message\n"));
            }
            return(1);

        case WM_DESTROY:  // message: window being destroyed
            PostQuitMessage(0);
            return(0);

        default:          // Pass it on if unproccessed
            return (DefWindowProc(hWnd, message, uParam, lParam));
    }
}



#endif
