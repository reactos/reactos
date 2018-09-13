/*--------------------------------------------------------------
 *
 * FILE:			SKeys.c
 *
 * PURPOSE:		The main interface routines between the service
 *					manager and the Serial Keys program.
 *
 * CREATION:		June 1994
 *
 * COPYRIGHT:		Black Diamond Software (C) 1994
 *
 * AUTHOR:			Ronald Moak
 *
 * NOTES:
 *
 * This file, and all others associated with it contains trade secrets
 * and information that is proprietary to Black Diamond Software.
 * It may not be copied copied or distributed to any person or firm
 * without the express written permission of Black Diamond Software.
 * This permission is available only in the form of a Software Source
 * License Agreement.
 *
 * $Header: %Z% %F% %H% %T% %I%
 *
 *--- Includes  ---------------------------------------------------------*/

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <process.h>
#include <tchar.h>
#include "vars.h"
#include "debug.h"

#define DEFDATA	1
#include "sk_defs.h"
#include "sk_comm.h"
#include "sk_reg.h"
#include "sk_dll.h"
#include "sk_login.h"

#include	"sk_ex.h"

#include	"..\access\skeys.h"

#define LONGSTRINGSIZE 1024

#define WAITMAX 0x7FFFFFFF

#define RUNNINGEVENT TEXT("SkeysRunning")

#if defined(DEBUG) && 0
	// give us a long time to startup in case we're debugging
	#define WAITSTARTUP WAITMAX  
#else
	// normal startup time
	#define WAITSTARTUP 60000
#endif


// --- Local Variables  --------------------------------------------------

static SERVICE_STATUS_HANDLE   s_sshStatusHandle;
static SERVICE_STATUS          s_ssStatus;       // current status of the service

PTSTR SERVICENAME = TEXT("SerialKeys");
PTSTR SKEYSUSERINITCMD = TEXT("SKEYS /I");
PTSTR WINLOGONPATH = TEXT("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon");
PTSTR USERINIT = TEXT("Userinit");
PTSTR USERINITCMDSEP = TEXT(",");

DWORD   s_dwServiceCommand;

static HANDLE s_hEventServiceRequest = NULL;
static HANDLE s_hEventServiceRequestReady = NULL;
static HANDLE s_hEventServiceTerminate = NULL;
static HANDLE s_hEventSkeysServiceRunning = NULL;

void DoService();
void DoInit();
void InstallUserInit();
BOOL IsSerialKeysAutoStart();


//--- SCM Function Prototypes  ------------------------------------------------
//
// Note:	The following fuctions manage the connection of the service
//			with the Service Contol Manager.

void	PostEventLog(LPTSTR lpszMsg,DWORD Error);

VOID	ServiceMain(DWORD dwArgc, LPTSTR *ppszArgv);

VOID	StopSerialKeys(LPTSTR lpszMsg);
BOOL	ReportStatusToSCMgr(DWORD dwCurrentState,
                            DWORD dwWin32ExitCode,
                            DWORD dwCheckPoint,
                            DWORD dwWaitHint);

LPHANDLER_FUNCTION ServiceCtrl(DWORD dwCtrlCode);

// Service Routines -----------------------------------------------
//
// Note:	The following fuctions manage the internal control of the
//			Service
static void InitReg();
static BOOL	InitService();
static void PauseService();
static void	ProcessService();
static void	ResumeService();
static void	TerminateService();

static void	ProcessLogout(DWORD dwCtrlType);
static BOOL	InstallLogout();
static BOOL	TerminateLogout();
static void EnableService(BOOL fOn);

int WINAPI _tWinMain(
    HINSTANCE hInstance,	
    HINSTANCE hPrevInstance,
    PTSTR pszCmdLine,	
    int nCmdShow)
{

	if ((TEXT('/') == pszCmdLine[0] || TEXT('-') == pszCmdLine[0]) &&
  		(TEXT('I') == pszCmdLine[1] || TEXT('i') == pszCmdLine[1]))
	{
        DoInit();
	}
	else
	{
		DoService();
	}

	ExitProcess(0);
	return(0);
}

/*---------------------------------------------------------------
 *
 * FUNCTION	DoInit()
 *
 *	TYPE		Global
 *
 * PURPOSE		This function is called to read skeys configuration
 *              from HKEY_CURRENT_USER at logon session startup and
 *              send the information to the service
 *
 * INPUTS		None
 *
 * RETURNS		None
 *
 *---------------------------------------------------------------*/
void DoInit()
{
    HANDLE hEventSkeysServiceRunning;
    BYTE abSd[SECURITY_DESCRIPTOR_MIN_LENGTH];
    PSECURITY_DESCRIPTOR pSD = (PSECURITY_DESCRIPTOR)abSd;
    SECURITY_ATTRIBUTES sa;

    hEventSkeysServiceRunning = NULL;

	if (InitializeSecurityDescriptor(pSD,SECURITY_DESCRIPTOR_REVISION))
	{
	    if (SetSecurityDescriptorDacl(pSD,TRUE,NULL,FALSE))
	    {

			sa.nLength = sizeof(sa);
			sa.bInheritHandle = TRUE;
			sa.lpSecurityDescriptor = pSD;

			hEventSkeysServiceRunning = CreateEvent(
				&sa,	// Security
				TRUE,	// Manual reset?
				FALSE,  // initial state - not signaled
				RUNNINGEVENT);  // name
		}
	}
	
    if (NULL != hEventSkeysServiceRunning)
	{
		DWORD dwWait;

		dwWait = WaitForSingleObject(hEventSkeysServiceRunning, 60 * 1000);

		if (WAIT_OBJECT_0 == dwWait)
		{
			SKEY_SystemParametersInfo((UINT)SK_SPI_INITUSER, 0, NULL, 0);
		}
        CloseHandle(hEventSkeysServiceRunning);
    }
	
	return;
}


/*---------------------------------------------------------------
 *
 *		SCM Interface Functions
 *
/*---------------------------------------------------------------
 *
 * FUNCTION	DoService()
 *
 *	TYPE		Global
 *
 * PURPOSE		all DoService does is call StartServiceCtrlDispatcher
 *				to register the main service thread.  When the
 *				API returns, the service has stopped, so exit.
 *
 * INPUTS		None
 *
 * RETURNS		None
 *
 *---------------------------------------------------------------*/
void DoService()
{
	SERVICE_TABLE_ENTRY dispatchTable[] =
	{
		{ SERVICENAME, (LPSERVICE_MAIN_FUNCTION)ServiceMain },
		{ NULL, NULL }
	};
    BYTE abSd[SECURITY_DESCRIPTOR_MIN_LENGTH];
    PSECURITY_DESCRIPTOR pSD = (PSECURITY_DESCRIPTOR)abSd;
    SECURITY_ATTRIBUTES sa;

    s_hEventServiceRequest = CreateEvent(
		NULL,	// Security
		FALSE,	// Manual reset?
		FALSE,  // initial state - not signaled
		NULL);  // name

    s_hEventServiceRequestReady = CreateEvent(
		NULL,	// Security
		FALSE,	// Manual reset?
		TRUE,  // initial state - signaled (can accept one request even before ready)
		NULL);  // name

    s_hEventServiceTerminate = CreateEvent(
		NULL,	// Security
		TRUE,	// Manual reset?
		FALSE,  // initial state - not signaled
		NULL);  // name

    s_hEventSkeysServiceRunning = NULL;

	if (InitializeSecurityDescriptor(pSD,SECURITY_DESCRIPTOR_REVISION))
	{
	    if (SetSecurityDescriptorDacl(pSD,TRUE,NULL,FALSE))
	    {
			sa.nLength = sizeof(sa);
			sa.bInheritHandle = TRUE;
			sa.lpSecurityDescriptor = pSD;

			s_hEventSkeysServiceRunning = CreateEvent(
				&sa,	// Security
				TRUE,	// Manual reset?
				FALSE,  // initial state - not signaled
				RUNNINGEVENT);  // name
		}
	}

    if (NULL != s_hEventServiceRequest &&
		NULL != s_hEventServiceRequestReady &&
		NULL != s_hEventServiceTerminate &&
		NULL != s_hEventSkeysServiceRunning)
	{
		if (!StartServiceCtrlDispatcher(dispatchTable))
			StopSerialKeys(TEXT("StartServiceCtrlDispatcher failed."));
    }
	else
	{
		StopSerialKeys(TEXT("Unable to create event."));
	}
	
	if (NULL != s_hEventServiceRequest)
	{
	    CloseHandle(s_hEventServiceRequest);
	}

	if (NULL != s_hEventServiceRequestReady)
	{
        CloseHandle(s_hEventServiceRequestReady);
	}

	if (NULL != s_hEventServiceTerminate)
	{
        CloseHandle(s_hEventServiceTerminate);
	}

	if (NULL != s_hEventSkeysServiceRunning)
	{
		ResetEvent(s_hEventSkeysServiceRunning);
        CloseHandle(s_hEventSkeysServiceRunning);
	}
}

/*---------------------------------------------------------------
 *
 * FUNCTION	ServiceMain()
 *
 *	TYPE		Global
 *
 * PURPOSE		this function takes care of actually starting the service,
 *				informing the service controller at each step along the way.
 *				After launching the worker thread, it waits on the event
 *				that the worker thread will signal at its termination.
 *
 * INPUTS		None
 *
 * RETURNS		None
 *
 *---------------------------------------------------------------*/
VOID ServiceMain(DWORD dwArgc, LPTSTR *ppszArgv)
{

	DBG_OPEN();		// Open up the Debug Pipe

	DBG_OUT("ServiceMain()");

	//
	// SERVICE_STATUS members that don't change
	s_ssStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
	s_ssStatus.dwServiceSpecificExitCode = 0;
    
	//
	// register our service control handler:
	s_sshStatusHandle = RegisterServiceCtrlHandler(
			SERVICENAME,
			(LPHANDLER_FUNCTION) ServiceCtrl);

	if (!s_sshStatusHandle)
	{
		TerminateService(GetLastError());
		return;
	}

	// report the status to Service Control Manager.
	ReportStatusToSCMgr(
				SERVICE_START_PENDING,	// service state
				NO_ERROR,				// exit code
				1,						// checkpoint
				WAITSTARTUP);			// wait hint

#if defined(DEBUG) && 0  /////////////////////////////////////////////////
	// This debug code gives us time to attach a debugger

    {
		int i;

		for (i = 0; i < 180; i++)  // 180 sec = 3 min
		{
			Sleep(1000);  // one second
		}
    }
#endif ////////////////////////////////////////////////////////

	InitReg();
	GetUserValues(REG_DEF);

////EnableService(skNewKey.dwFlags & SERKF_SERIALKEYSON);

	if (!InitService())					// Did Service Initiate successfully?
	{
		TerminateService(GetLastError());		// No Terminate With Error
		return;
	}

	ReportStatusToSCMgr(	// report status to service manager.
		SERVICE_RUNNING,	// service state
		NO_ERROR,			// exit code
		0,					// checkpoint
		0);					// wait hint
	
	SetEvent(s_hEventSkeysServiceRunning);

	ProcessService();					// Process the Service
	TerminateService(0);				// Terminate
	return;
}


BOOL IsSerialKeysAutoStart()
{
	BOOL fAutoStart = FALSE;
	BOOL fOk;

	SC_HANDLE   schService = NULL;
	SC_HANDLE   schSCManager = NULL;

	schSCManager = OpenSCManager(   // Open Service Manager
		NULL,                       // machine (NULL == local)
	    NULL,                       // database (NULL == default)
		MAXIMUM_ALLOWED);

	if (NULL != schSCManager)  // Did Open Service succeed?
	{
	    schService = OpenService(
			    schSCManager ,
			    __TEXT("SerialKeys"),
			    MAXIMUM_ALLOWED);

	    if (NULL != schService)
		{
			BYTE abServiceConfig[1024];
			LPQUERY_SERVICE_CONFIG pqsc = (LPQUERY_SERVICE_CONFIG)abServiceConfig;
			DWORD cbBytesNeeded;

			fOk = QueryServiceConfig(
				schService,
				pqsc,
				sizeof(abServiceConfig),
				&cbBytesNeeded);

			if (fOk)
			{
				fAutoStart = (SERVICE_AUTO_START == pqsc->dwStartType);
			}
	        CloseServiceHandle(schService);
		}
        CloseServiceHandle(schSCManager);
    }

	return fAutoStart;
}


void InstallUserInit()
{
	BOOL fOk = FALSE;
    HKEY  hkey;
	LONG lErr;
	DWORD dwType;
	TCHAR szUserinit[LONGSTRINGSIZE];
	DWORD cbData = sizeof(szUserinit);

	lErr = RegOpenKeyEx(
		HKEY_LOCAL_MACHINE,
        WINLOGONPATH,
		REG_OPTION_NON_VOLATILE,
        KEY_ALL_ACCESS,
        &hkey); 
	
    if (ERROR_SUCCESS == lErr)
	{
		lErr = RegQueryValueEx(
                hkey,
                USERINIT,
                0,
                &dwType,
                (LPBYTE)szUserinit,
                &cbData);

		if (ERROR_SUCCESS == lErr)
		{
			// check to see if we are already installed and if we have
			// enough room to install
			// the + 2 allows for the terminating null and for the command seperator char

			if (NULL == _tcsstr(szUserinit, SKEYSUSERINITCMD) &&
				    lstrlen(szUserinit) + lstrlen(SKEYSUSERINITCMD) + 2 < 
					        ARRAY_SIZE(szUserinit))
			{
				lstrcat(szUserinit, USERINITCMDSEP);
				lstrcat(szUserinit, SKEYSUSERINITCMD);

				RegSetValueEx(
					hkey,
					USERINIT,
                    0,
					REG_SZ,
				    (CONST LPBYTE)szUserinit,
				    (lstrlen(szUserinit) + 1) * 
					    sizeof(*szUserinit));
			}
		}
		RegCloseKey(hkey);
	}
    return;
}

void RemoveUserInit()
{
	BOOL fOk = FALSE;
    HKEY  hkey;
	LONG lErr;
	DWORD dwType;
	TCHAR szUserinit[LONGSTRINGSIZE];
	PTSTR pszDest;
	PTSTR pszSrc;
	DWORD cbData = sizeof(szUserinit);

	lErr = RegOpenKeyEx(
		HKEY_LOCAL_MACHINE,
        WINLOGONPATH,
		REG_OPTION_NON_VOLATILE,
        KEY_ALL_ACCESS,
        &hkey); 
	
    if (ERROR_SUCCESS == lErr)
	{
		lErr = RegQueryValueEx(
                hkey,
                USERINIT,
                0,
                &dwType,
                (LPBYTE)szUserinit,
                &cbData);

		if (ERROR_SUCCESS == lErr)
		{

			// check to see if we are already installed
			pszDest = _tcsstr(szUserinit, SKEYSUSERINITCMD);
			if (NULL != pszDest)
			{
				pszSrc =_tcsstr(pszDest, USERINITCMDSEP);
				if (NULL != pszSrc)
				{
					_tcscpy(pszDest, pszSrc+1);
				}
				else
				{
					while(szUserinit < pszDest && *SKEYSUSERINITCMD != *pszDest)
					{
						--pszDest;
					}
					*pszDest = 0;  // null terminate
				}
			}
		}
		RegCloseKey(hkey);
	}
    return;
}

static void EnableService(BOOL fOn)
{
	SC_HANDLE   schService = NULL;
	SC_HANDLE   schSCManager = NULL;

	schSCManager = OpenSCManager(   // Open Service Manager
		NULL,                       // machine (NULL == local)
	    NULL,                       // database (NULL == default)
		MAXIMUM_ALLOWED);

	if (NULL != schSCManager)  // Did Open Service succeed?
	{
	    schService = OpenService(
			    schSCManager ,
			    __TEXT("SerialKeys"),
			    SERVICE_CHANGE_CONFIG | SERVICE_STOP);

	    if (NULL != schService)
		{
			ChangeServiceConfig(
				schService,
				SERVICE_WIN32_OWN_PROCESS,
				(fOn) ? SERVICE_AUTO_START : SERVICE_DEMAND_START,
				SERVICE_NO_CHANGE,		// severity if service fails to start 
				NULL,					// pointer to service binary file name 
				NULL,					// pointer to load ordering group name 
				NULL,					// pointer to variable to get tag identifier 
				NULL,					// pointer to array of dependency names 
				NULL,					// pointer to account name of service 
				NULL,					// pointer to password for service account  
				__TEXT("SerialKeys"));	// name to display 

	        CloseServiceHandle(schService);
		}

        CloseServiceHandle(schSCManager);
    }

	if (fOn)
	{
		InstallUserInit();
	}
	else
	{
		RemoveUserInit();
	}

    return;
}


//---------------------------------------------------------------
//
// FUNCTION	void ServiceCtrl(DWORD dwCtrlCode)
//
//	TYPE		Global
//
// PURPOSE		this function is called by the Service Controller whenever
//				someone calls ControlService in reference to our service.
//
// INPUTS		DWORD dwCtrlCode -
//
// RETURNS		None
//
//-----------------------------------------------------------------
LPHANDLER_FUNCTION ServiceCtrl(DWORD dwCtrlCode)
{
	DWORD	dwState = SERVICE_RUNNING;
	DWORD	dwWait = 0;

	DBG_OUT("ServiceCtrl()");

	// Handle the requested control code.

	switch(dwCtrlCode)
	{
		case SERVICE_CONTROL_PAUSE:			// Pause the service if it is running.
			if (s_ssStatus.dwCurrentState == SERVICE_RUNNING)
			{
				PauseService();
				dwState = SERVICE_PAUSED;
			}
			break;

		case SERVICE_CONTROL_CONTINUE:		// Resume the paused service.
			if (s_ssStatus.dwCurrentState == SERVICE_PAUSED)
			{
				ResumeService();
				dwState = SERVICE_RUNNING;
			}
			break;

		case SERVICE_CONTROL_STOP:			// Stop the service.
			// Report the status, specifying the checkpoint and waithint,
			//  before setting the termination event.
			if (s_ssStatus.dwCurrentState == SERVICE_RUNNING)
			{
				dwState = SERVICE_STOP_PENDING;
				dwWait = 20000;
				SetEvent(s_hEventServiceTerminate);
			}
			break;

		case SERVICE_CONTROL_INTERROGATE:	// Update the service status.
		default:							// invalid control code
			break;
    }
	// send a status response.
    ReportStatusToSCMgr(dwState, NO_ERROR, 0, dwWait);
	 return(0);
}

/*---------------------------------------------------------------
 *
 * FUNCTION	BOOL		ReportStatusToSCMgr()
 *
 *	TYPE		Global
 *
 * PURPOSE		This function is called by the ServMainFunc() and
 *				ServCtrlHandler() functions to update the service's status
 *				to the service control manager.
 *
 * INPUTS		DWORD	dwCurrentState
 *				DWORD	dwWin32ExitCode
 *				DWORD	dwCheckPoint
 *				DWORD	dwWaitHint
 *
 * RETURNS		None
 *
 *---------------------------------------------------------------*/
BOOL ReportStatusToSCMgr(DWORD dwCurrentState,
                    DWORD dwWin32ExitCode,
                    DWORD dwCheckPoint,
                    DWORD dwWaitHint)
{
	BOOL fResult;

#ifdef DEBUG
{
	switch (dwCurrentState)
	{
		case SERVICE_START_PENDING:
			DBG_OUT("ReportStatusToSCMgr(SERVICE_START_PENDING:)");
			break;
		case SERVICE_PAUSED:
			DBG_OUT("ReportStatusToSCMgr(SERVICE_PAUSED:)");
			break;
		case SERVICE_CONTINUE_PENDING:
			DBG_OUT("ReportStatusToSCMgr(SERVICE_CONTINUE_PENDING:)");
			break;
		case SERVICE_STOP_PENDING:
			DBG_OUT("ReportStatusToSCMgr(SERVICE_STOP_PENDING:)");
			break;
		case SERVICE_STOPPED:
			DBG_OUT("ReportStatusToSCMgr(SERVICE_STOPPED:)");
			break;
		case SERVICE_RUNNING:
			DBG_OUT("ReportStatusToSCMgr(SERVICE_RUNNING:)");
			break;

		default:
			DBG_OUT("ReportStatusToSCMgr(ERROR - SERVICE_UNKNOWN)");
			break;
	}
}
#endif


    switch (dwCurrentState)
	{
	case SERVICE_STOPPED:
	case SERVICE_START_PENDING:
	case SERVICE_STOP_PENDING:
		s_ssStatus.dwControlsAccepted = 0;
		break;
    default:
    	s_ssStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP |	SERVICE_ACCEPT_PAUSE_CONTINUE;
		break;
	}

	// These SERVICE_STATUS members are set from parameters.
	s_ssStatus.dwCurrentState	= dwCurrentState;
	s_ssStatus.dwWin32ExitCode	= dwWin32ExitCode;
	s_ssStatus.dwCheckPoint		= dwCheckPoint;
	s_ssStatus.dwWaitHint		= dwWaitHint;

	// Report the status of the service to the service control manager.

	fResult = SetServiceStatus(
		s_sshStatusHandle,				// service reference handle
		&s_ssStatus); 					// SERVICE_STATUS structure

	if (!fResult)
	{
		StopSerialKeys(TEXT("SetServiceStatus")); // If an error occurs, stop the service.
	}
	return fResult;
}

/*---------------------------------------------------------------
 *
 * FUNCTION	void StopSerialKeys(LPTSTR lpszMsg)
 *
 *	TYPE		Global
 *
 * PURPOSE		The StopSerialKeys function can be used by any thread
 *				to report an error, or stop the service.
 *
 * INPUTS		LPTSTR lpszMsg -
 *
 * RETURNS		None
 *
 *---------------------------------------------------------------*/
VOID StopSerialKeys(LPTSTR lpszMsg)
{

	DBG_OUT("StopSerialKeys()");

	PostEventLog(lpszMsg,GetLastError());	// Post to Event Log
	SetEvent(s_hEventServiceTerminate);
}

/*---------------------------------------------------------------
 *
 * FUNCTION	void PostEventLog(LPTSTR lpszMsg, DWORD Error)
 *
 *	TYPE		Local
 *
 * PURPOSE		This function post strings to the Event Log
 *
 * INPUTS		LPTSTR lpszMsg - String to send
 *				DWORD Error		- Error Code (if 0 no error)
 *
 * RETURNS		None
 *
 *---------------------------------------------------------------*/
void PostEventLog(LPTSTR lpszMsg,DWORD Error)
{
	WORD 	ErrType = EVENTLOG_INFORMATION_TYPE;
	WORD	ErrStrings = 0;

	TCHAR   szMsg[256];
	HANDLE  hEventSource;
	LPTSTR  lpszStrings[2];

	DBG_OUT("PostEventLog()");

	lpszStrings[0] = lpszMsg;

	if (Error)
	{
		ErrType = EVENTLOG_ERROR_TYPE;
		ErrStrings = 2;
		wsprintf(szMsg, TEXT("SerialKeys error: %d"), Error);
		lpszStrings[0] = szMsg;
		lpszStrings[1] = lpszMsg;
	}

	hEventSource = RegisterEventSource(NULL,SERVICENAME);

	if (hEventSource != NULL)
	{
		ReportEvent
		(
			hEventSource,		// handle of event source
			ErrType,			// event type
			0,					// event category
			0,					// event ID
			NULL,				// current user's SID
			ErrStrings,			// strings in lpszStrings
			0,					// no bytes of raw data
			lpszStrings,		// array of error strings
			NULL				// no raw data
		);

		(VOID) DeregisterEventSource(hEventSource);
	}
}

/*---------------------------------------------------------------
 *
 *		Internal Service Control Functions
 *
/*---------------------------------------------------------------
 *
 * FUNCTION	void InitService()
 *
 * PURPOSE		This function Initializes the Service & starts the
 *				major threads of the service.
 *
 * INPUTS		None
 *
 * RETURNS		None
 *
 *---------------------------------------------------------------*/
static BOOL InitService()
{

	DBG_OUT("InitService()");

	InstallLogout();

	if (!InitDLL())
		return(FALSE);

	if (!InitLogin())
		return(FALSE);

	if (!InitComm())
		return(FALSE);

	DoServiceCommand(SC_LOG_IN);	// Set ProcessService to Login Serial Keys

	return(TRUE);
}


static void InitReg()
{
	// Set Structure pointers to Buffers
	skNewKey.cbSize = sizeof(skNewKey);
	skNewKey.lpszActivePort = szNewActivePort;
	skNewKey.lpszPort = szNewPort;

	skCurKey.cbSize = sizeof(skCurKey);
	skCurKey.lpszActivePort = szCurActivePort;
	skCurKey.lpszPort = szCurPort;

	// Set Default Values
	skNewKey.dwFlags = SERKF_AVAILABLE;

	skNewKey.iBaudRate = 300;
	skNewKey.iPortState = 2;
	lstrcpy(szNewPort,TEXT("COM1:"));
	lstrcpy(szNewActivePort,TEXT("COM1:"));
}

/*---------------------------------------------------------------
 *
 * FUNCTION	void PauseService()
 *
 * PURPOSE		This function is called to pause the service
 *
 * INPUTS		None
 *
 * RETURNS		None
 *
 *---------------------------------------------------------------*/
static void PauseService()
{
	DBG_OUT("PauseService()");

	SuspendDLL();
	SuspendComm();
	SuspendLogin();
}


/*---------------------------------------------------------------
 *
 * FUNCTION	void DoServiceCommand()
 *
 * PURPOSE		Passes a command to the service thread
 *
 * INPUTS		None
 *
 * RETURNS		None
 *
 *---------------------------------------------------------------*/
void DoServiceCommand(DWORD dwServiceCommand)
{
	DWORD dwWaitRet;

    dwWaitRet = WaitForSingleObject(s_hEventServiceRequestReady, 10*1000);
    
	if (WAIT_OBJECT_0 == dwWaitRet)
	{
		s_dwServiceCommand = dwServiceCommand;
		SetEvent(s_hEventServiceRequest);
	}
    else
	{
		DBG_OUT("DoServiceCommand - wait failed or timed-out, request ignored");
	}
}


/*---------------------------------------------------------------
 *
 * FUNCTION	void ProcessService()
 *
 * PURPOSE		This function is the main service thread for Serial
 *				Keys. 	Is monitors the status of the other theads
 *				and responds to their request.
 *
 * INPUTS		None
 *
 * RETURNS		None
 *
 *---------------------------------------------------------------*/
static void ProcessService()
{
    DWORD dwServiceCommand;
	DWORD dwWaitRet;
	typedef enum {
		iheventServiceRequest,    
		iheventServiceTerminate
	};
	HANDLE ahevent[2] = {s_hEventServiceRequest, s_hEventServiceTerminate};

#ifdef DEBUG
	int Cnt = 0;
#endif

 	DBG_OUT("ProcessService()");
    
    dwWaitRet = WaitForMultipleObjects(ARRAY_SIZE(ahevent), ahevent, 
		FALSE,  // wait all?
		INFINITE);

	//  This loop will terminate when iheventServiceTerminate is signaled or
	//  WaitForMultipleObjects fails

	while (iheventServiceRequest == dwWaitRet - WAIT_OBJECT_0)
	{
		dwServiceCommand = s_dwServiceCommand;
    	SetEvent(s_hEventServiceRequestReady);

		switch (dwServiceCommand)
		{
			case SC_LOG_OUT:				// Login to New User
				DBG_OUT("---- User Logging Out");
				StopComm();			// Stop SerialKey Processing
				if(GetUserValues(REG_DEF))	// Get Default values & Do we Start?
				{
					EnableService(skNewKey.dwFlags & SERKF_SERIALKEYSON);
					StartComm();			// Yes - Process SerialKey
				}
				break;

			case SC_LOG_IN:					// Login to New User
				DBG_OUT("---- User Logging In");
				StopComm();			// Stop SerialKey Processing
				if(GetUserValues(REG_DEF)) 
				{	
					EnableService(skNewKey.dwFlags & SERKF_SERIALKEYSON);
					StartComm();			// Yes - Process SerialKey
				}
				break;

			case SC_CHANGE_COMM: 			// Change Comm Configuration
				DBG_OUT("---- Making Comm Change");
				StopComm();			// Stop SerialKey Processing
				StartComm();				// Restart SerialKey Processing
				break;

			case SC_DISABLE_SKEY:		 	// Disable Serial Keys
				DBG_OUT("---- Disable Serial Keys");
				StopComm();
				break;

			case SC_ENABLE_SKEY:			// Enable Serial Keys
				DBG_OUT("---- Enable Serial Keys");
				StartComm();
				break;
		}
		dwWaitRet = WaitForMultipleObjects(ARRAY_SIZE(ahevent), ahevent, 
			FALSE,  // wait all?
			INFINITE);
	}
}

/*---------------------------------------------------------------
 *
 * FUNCTION	void ResumeService()
 *
 * PURPOSE		This function is called to restore the service
 *
 * INPUTS		None
 *
 * RETURNS		None
 *
 *---------------------------------------------------------------*/
static void ResumeService()
{
	DBG_OUT("ResumeService()");

	ResumeDLL();
	ResumeComm();
	ResumeLogin();
}

//---------------------------------------------------------------
//
// FUNCTION	void TerminateService(DWORD Error)
//
//	TYPE		Local
//
// PURPOSE		This function is called by ServiceMain to terminate
//				the server.  It closes all of the open handles &
//				and reports the service is stopped.
//
// INPUTS		DWORD Error - Any Errors that could abort the
//				Service. 0 = Normal Stop
//
// RETURNS		None
//
//---------------------------------------------------------------

static void TerminateService(DWORD Error)
{
	DBG_OUT("TerminateService()");

	TerminateLogout();						// Remove Logout Monitoring

	TerminateComm();						// Init Comm Thread Shutdown

	TerminateDLL();							// Init DLL Thread Shutdown

	TerminateLogin();						// Init Login Thread Shutdown

	// Loop untill all of the Threads are shut down.

	while (!DoneLogin()) 					// Loop until Login Thread is terminated
		Sleep(250);							// Sleep 

	while (!DoneDLL())	 					// Loop until DLL Thread is terminated
		Sleep(250);							// Sleep 


    // reload registery values to insure we have the current values
	GetUserValues(REG_DEF);

	EnableService(skNewKey.dwFlags & SERKF_SERIALKEYSON);

	// Report the status is stopped
	if (s_sshStatusHandle)
		(VOID)ReportStatusToSCMgr(SERVICE_STOPPED,Error,0,0);

	DBG_CLOSE();

}

/*---------------------------------------------------------------
 *
 *	Logout Functions - Process Logout request
 *
/*---------------------------------------------------------------
 *
 * FUNCTION	void InstallLogout()
 *
 * PURPOSE		This function installs a Control Handler to process
 *				logout events.
 *
 * INPUTS		None
 *
 * RETURNS		None
 *
 *---------------------------------------------------------------*/
static BOOL InstallLogout()
{
	DBG_OUT("InstallLogout()");

	return(SetConsoleCtrlHandler((PHANDLER_ROUTINE)ProcessLogout,TRUE));
}

/*---------------------------------------------------------------
 *
 * FUNCTION	void TerminateLogout()
 *
 * PURPOSE		This function Removes a Control Handler to process
 *				logout events.
 *
 * INPUTS		None
 *
 * RETURNS		None
 *
 *---------------------------------------------------------------*/
static BOOL TerminateLogout()
{
	DBG_OUT("TerminateLogout()");

	return(SetConsoleCtrlHandler((PHANDLER_ROUTINE)ProcessLogout,FALSE));
}

/*---------------------------------------------------------------
 *
 * FUNCTION	void ProcessLogout()
 *
 * PURPOSE		This function processes	logout events.
 *
 * INPUTS		None
 *
 * RETURNS		None
 *
 *---------------------------------------------------------------*/
static void ProcessLogout(DWORD dwCtrlType)
{
	DBG_OUT("ProcessLogout()");

	if (dwCtrlType == CTRL_LOGOFF_EVENT)
	{
		DoServiceCommand(SC_LOG_OUT);

		// we'll do this each time the currently logged in user logs out
	}
}
