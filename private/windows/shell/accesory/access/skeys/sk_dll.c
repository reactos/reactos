/*--------------------------------------------------------------
 *
 * FILE:			SK_DLL.C
 *
 * PURPOSE:		The file contains the Functions responsible for
 *					managing information passed between SerialKeys
 *					and the SerialKeys DLL
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
#include	<process.h>

#include	"windows.h"
#include	"debug.h"
#include	"sk_defs.h"
#include	"sk_reg.h"
#include	"sk_dll.h"
#include	"sk_dllif.h"

#ifdef DEBUG
void 	dbg_Output(LPSTR Header);

#define	DBG_DUMP(Header)	dbg_Output(Header)
#else
#define	DBG_DUMP(Header)	
#endif

// Defines --------------------------------------------------

// Local Function Prototypes ---------------------------------

static void CleanUpDLL();
static void GetCurrentValues();
static void GetNewValues();
static void ProcessDLL();
static BOOL ReadDLL();
static void __cdecl ServiceDLL(VOID *notUsed);
static BOOL WriteDLL();

// Local Variables --------------------------------------------------

static OVERLAPPED	OverLapRd;		// Overlapped structure for reading.
static SKEYDLL		SKeyDLL; 		// Input buffer for pipe.

static BOOL			fExitDLL; 			// Set Exit Flag
static BOOL			fDoneDLL = TRUE;

static	HANDLE		hPipeDLL;
static	HANDLE		hThreadDLL;

static SECURITY_ATTRIBUTES		sa;
static PSECURITY_DESCRIPTOR	pSD;

/*---------------------------------------------------------------
 *
 *	Global Functions 
 *
/*---------------------------------------------------------------
 *
 * FUNCTION	BOOL DoneDLL()
 *
 *	TYPE		Global
 *
 * PURPOSE		Returns the state of the DLL Thread
 *
 * INPUTS		None
 *
 * RETURNS		TRUE - DLL Thread not running
 * 			FALSE - DLL Thread Is running
 *
 *---------------------------------------------------------------*/
BOOL DoneDLL()
{
	return(fDoneDLL);
}

/*---------------------------------------------------------------
 *
 * FUNCTION	BOOL InitDLL()
 *
 *	TYPE		Global
 *
 * PURPOSE		This function creates a thread that monitors when an
 *				application uses the DLL to Get or Set the state
 *				of Serial Keys.  
 *
 * INPUTS		None
 *
 * RETURNS		TRUE - Init ok & Thread installed
 *				FALSE- Thread failed
 *
 *---------------------------------------------------------------*/
BOOL InitDLL()
{
	DWORD Id;

	DBG_OUT("InitDLL()");

	hPipeDLL	= INVALID_HANDLE_VALUE;
	hThreadDLL	= NULL;
	fExitDLL 	= FALSE;

	pSD = (PSECURITY_DESCRIPTOR) LocalAlloc(LPTR,
         SECURITY_DESCRIPTOR_MIN_LENGTH);

	if (!pSD)
		return(FALSE);
		
	if (!InitializeSecurityDescriptor(pSD,SECURITY_DESCRIPTOR_REVISION))
	{
        LocalFree((HLOCAL)pSD);
		 return FALSE;
	}

	if (!SetSecurityDescriptorDacl(pSD,TRUE,NULL,FALSE))
	{
        LocalFree((HLOCAL)pSD);
		 return FALSE;
	}

	sa.nLength = sizeof(SECURITY_ATTRIBUTES);
	sa.bInheritHandle = TRUE;
	sa.lpSecurityDescriptor = pSD;

    hPipeDLL = CreateNamedPipe(
  		SKEY_NAME, 						// Pipe name
		PIPE_ACCESS_DUPLEX,	 			// 2 way pipe.
		PIPE_TYPE_MESSAGE | 
		PIPE_READMODE_MESSAGE | 
		PIPE_WAIT,
		1,							// Maximum instance limit.
		0,							// Buffer sizes.
		0,
		1000 * 30,					// Specify time out.
		&sa);						// Default securities specified.

	if (INVALID_HANDLE_VALUE == hPipeDLL)
	{
		hPipeDLL = NULL;
		DBG_ERR("Unable to Create DLL Named Pipe");
		return FALSE;
	}
	
	fDoneDLL = FALSE;  						// Clear Thread Done Flag

	// Generate thread to handle DLL processing;
	hThreadDLL = (HANDLE)CreateThread(	// Start Service Thread
		0,
		0,
		(LPTHREAD_START_ROUTINE) ServiceDLL,
		0,0,&Id);								// argument to thread

	if (NULL == hThreadDLL)
	{
		DBG_ERR("Unable to Create DLL Thread");
		CleanUpDLL();
		return FALSE;
	}

	return(TRUE);
}

/*---------------------------------------------------------------
 *
 * FUNCTION	void SuspendDLL()
 *
 *	TYPE		Global
 *
 * PURPOSE		The function is called to Pause the thread  
 *				reading and processing data coming from the comm port.
 *
 * INPUTS		None
 *
 * RETURNS		None
 *
 *---------------------------------------------------------------*/
void SuspendDLL()
{
	if (NULL != hThreadDLL)
	{
		SuspendThread(hThreadDLL);	
	}
}

/*---------------------------------------------------------------
 *
 * FUNCTION	void ResumeDLL()
 *
 *	TYPE		Global
 *
 * PURPOSE		The function is called to resume the Paused thread.
 *
 * INPUTS		None
 *
 * RETURNS		None
 *
 *---------------------------------------------------------------*/
void ResumeDLL()
{
	if (NULL != hThreadDLL)
	{
		ResumeThread(hThreadDLL);	
	}
}

/*---------------------------------------------------------------
 *
 * FUNCTION	void TerminateDLL()
 *
 *	TYPE		Global
 *
 * PURPOSE		This function is called to Terminate the DLL Process
 *
 * INPUTS		None
 *
 * RETURNS		None
 *
 *---------------------------------------------------------------*/
void TerminateDLL()
{
	DWORD bytesRead;

	DBG_OUT("TerminateDLL()");

	if (fDoneDLL)
		return;

	fExitDLL = TRUE;					// Set Exit Flag

	CallNamedPipe						// Trigger the DLL to Shutdown
	(
		SKEY_NAME, 						// Pipe name
		&SKeyDLL, sizeof(SKeyDLL),
		&SKeyDLL, sizeof(SKeyDLL),
		&bytesRead, NMPWAIT_NOWAIT
	);
}

/*---------------------------------------------------------------
 *
 *	Local Functions 
 *
/*---------------------------------------------------------------
 *
 * FUNCTION    static void CleanUpDLL()
 *
 *	TYPE		Local
 *
 * PURPOSE		This function cleans up file handles and misc stuff
 *				when the thread is terminated.
 *
 * INPUTS		None
 *
 * RETURNS		None
 *
 *---------------------------------------------------------------*/
static void CleanUpDLL()
{
	BOOL	Stat;

	DBG_OUT("CleanUpDLL()");

	// Close Pipe Handle
	if (NULL != hPipeDLL)
	{
		Stat = CloseHandle(hPipeDLL);
		DBG_ERR1(Stat,"Unable to Close DLL Pipe");
	}

	// Close Thread Handle 
	if (NULL != hThreadDLL)
	{
		Stat = CloseHandle(hThreadDLL);
		DBG_ERR1(Stat,"Unable to Close DLL Thread");
	}

	// Free Security Desciptor
	LocalFree((HLOCAL)pSD);

	hPipeDLL	= NULL;
	hThreadDLL	= NULL;
	fDoneDLL = TRUE;							// Set Thread Done Flag
	DBG_OUT("DLL Service Processing Done");
}

/*---------------------------------------------------------------
 *
 * FUNCTION     void _CRTAPI1 ServiceDLL()
 *
 *	TYPE		Local
 *
 * PURPOSE		This function is a thread that monitors when an
 *				application uses the DLL to Get or Set the state
 *				of Serial Keys.  
 *
 * INPUTS		None
 *
 * RETURNS		None
 *
 *---------------------------------------------------------------*/
static void __cdecl ServiceDLL(VOID *notUsed)
{
	DWORD	retCode;
	DWORD	bytesRead;
	DWORD	bytesWritten;

	DBG_OUT("SericeDLL()");

	while (TRUE)
	{
		if (!ConnectNamedPipe(hPipeDLL,NULL))
		{
			ExitThread(0);
			return;
		}	

		if (fExitDLL)			// Is the Service Done?
		{						// Yes - Close Down Service
			CleanUpDLL();		// Close Handles Etc.
			ExitThread(0);		// Exit The Thread
			return;
		}
		
		retCode = ReadFile(		// Read Message
				hPipeDLL, 
				&SKeyDLL, 
				sizeof(SKeyDLL), 
				&bytesRead, 
				NULL);

		if (!retCode) 			// Pipe is Broken Try & reconnect
			continue;

		ProcessDLL();	  		// Yes - Process incoming buffer

		retCode = WriteFile(	// Write Message
			hPipeDLL, 
			&SKeyDLL, 
			sizeof(SKeyDLL), 
			&bytesWritten, 
			NULL);

		if (!retCode) 			// Pipe is Broken Try & reconnect
			continue;

		DisconnectNamedPipe(hPipeDLL);
	}
}

/*---------------------------------------------------------------
 *
 * FUNCTION	void ProcessDLL()
 *
 *	TYPE		Local
 *
 * PURPOSE		This function process the input buffer received from
 *				the DLL.
 *
 * INPUTS		None
 *
 * RETURNS		None
 *
 *---------------------------------------------------------------*/
static void ProcessDLL()
{
	DWORD	dwService;

	DBG_OUT("ProcessDLL()");

	dwService = SC_CHANGE_COMM;			

	switch (SKeyDLL.Message)			// Validate Message
	{
		case SPI_GETSERIALKEYS:
			if (skCurKey.dwFlags & SERKF_ACTIVE)	// Are We Disabled?
				GetCurrentValues();					// No - Send the actual Values
			else
				GetNewValues();						// Yes - Send the Proposed values
			DBG_DUMP("---Info Sent");
			return;
			
		case SPI_SETSERIALKEYS:
			DBG_DUMP("---Info Received");
			if ((SKeyDLL.dwFlags & SERKF_SERIALKEYSON) &&	// Are We Truning on &
				(SKeyDLL.dwFlags & SERKF_AVAILABLE))		// Is SerialKeys Available
			{
				if (!(skCurKey.dwFlags & SERKF_ACTIVE))		// Are We Disabled?
				{
					dwService = SC_ENABLE_SKEY;				// Yes - Turn SKeys On
					DBG_OUT("Turn Serial Key On");
				}
			}

			if (!(SKeyDLL.dwFlags & SERKF_SERIALKEYSON) &&	// Are We Truning off &
				(SKeyDLL.dwFlags & SERKF_AVAILABLE))		// Is SerialKeys Available
			{
				if (skCurKey.dwFlags & SERKF_ACTIVE) 		// Are We Active?
				{
					dwService = SC_DISABLE_SKEY;  			// Yes - Turn SKeys Off
					DBG_OUT("Turn Serial Key Off");
				}
			}

			skNewKey.iBaudRate	= SKeyDLL.iBaudRate;
			skNewKey.dwFlags 	= SKeyDLL.dwFlags;

#ifdef UNICODE
			MultiByteToWideChar(
				CP_ACP, 0, SKeyDLL.szActivePort, -1,
 				skNewKey.lpszActivePort, MAX_PATH);

			MultiByteToWideChar(
				CP_ACP, 0, SKeyDLL.szPort, -1,
				skNewKey.lpszPort, MAX_PATH);
#else
			lstrcpy(skNewKey.lpszActivePort,SKeyDLL.szActivePort);
			lstrcpy(skNewKey.lpszPort,SKeyDLL.szPort);
#endif

			if (*skNewKey.lpszPort == 0)
			{
                lstrcpy(skNewKey.lpszPort, skNewKey.lpszActivePort);	
			}

			// the calling dll is now responsible for saving the
			// settings because it's running in the user context
			// and can access HKEY_CURRENT_USER.  Here we're
			// running as a service (as the system) and have
			// no HKEY_CURRENT_USER
			
			DoServiceCommand(dwService);

			Sleep(1000);							// Sleep 1 Sec to set values

			if (SKeyDLL.dwFlags & SERKF_SERIALKEYSON) 	// Are We Truning on 
				GetCurrentValues();					// Yes - Send the actual Values
			else
				GetNewValues();						// No - Send the Proposed values

			DBG_DUMP("---Info Sent");
			break;

		default:
			return;
	}
}

/*---------------------------------------------------------------
 *
 * FUNCTION	void GetCurrentValues()
 *
 *	TYPE		Local
 *
 * PURPOSE		
 *				
 *
 * INPUTS		None
 *
 * RETURNS		None
 *
 *---------------------------------------------------------------*/
static void GetCurrentValues()
{
	DBG_OUT("GetCurrentValues()");

#ifdef UNICODE
	WideCharToMultiByte(
		CP_ACP, 0, skCurKey.lpszActivePort, -1, 
		SKeyDLL.szActivePort, sizeof(SKeyDLL.szActivePort), NULL, NULL);

	WideCharToMultiByte(
		CP_ACP, 0, skCurKey.lpszPort, -1, 
		SKeyDLL.szPort, sizeof(SKeyDLL.szPort), NULL, NULL);
#else
	lstrcpy(SKeyDLL.szActivePort,skCurKey.lpszActivePort);
	lstrcpy(SKeyDLL.szPort,skCurKey.lpszPort);
#endif

	SKeyDLL.dwFlags		= skCurKey.dwFlags;
	SKeyDLL.iBaudRate	= skCurKey.iBaudRate;
	SKeyDLL.iPortState	= skCurKey.iPortState;
}

/*---------------------------------------------------------------
 *
 * FUNCTION	void GetNewValues()
 *
 *	TYPE		Local
 *
 * PURPOSE		
 *				
 *
 * INPUTS		None
 *
 * RETURNS		None
 *
 *---------------------------------------------------------------*/
static void GetNewValues()
{
	DBG_OUT("GetNewValues()");


    // BUGBUG a-jimhar 04-03-96 this next line is suspect.  May need to 
	// change 'skCurKey.dwFlags' to 'skNewKey.dwFlags.  This is either
	// a mistake or was done to always return the current flags.

	SKeyDLL.dwFlags		= skCurKey.dwFlags;

	SKeyDLL.iBaudRate	= skNewKey.iBaudRate;
	SKeyDLL.iPortState	= skNewKey.iPortState;

#ifdef UNICODE
	WideCharToMultiByte(
		CP_ACP, 0, skNewKey.lpszActivePort, -1, 
		SKeyDLL.szActivePort, sizeof(SKeyDLL.szActivePort), NULL, NULL);

	WideCharToMultiByte(
		CP_ACP, 0, skNewKey.lpszPort, -1, 
		SKeyDLL.szPort, sizeof(SKeyDLL.szPort), NULL, NULL);
#else
	lstrcpy(SKeyDLL.szActivePort,skNewKey.lpszActivePort);
	lstrcpy(SKeyDLL.szPort,skNewKey.lpszPort);
#endif
}

/*---------------------------------------------------------------
 *
 * FUNCTION	void dbg_Output
 *
 *	TYPE		Debug
 *
 * PURPOSE		Dump the DLL Message Struct
 *
 * INPUTS		None
 *
 * RETURNS		None
 *
 *---------------------------------------------------------------*/
#ifdef DEBUG
void dbg_Output(LPSTR Header)
{
	char	buf[80];

	DBG_OUT(Header);
	wsprintfA(buf,"-- dwFlags (%d)  iBaudRate (%d) Save (%d) iPortState (%d)",
		SKeyDLL.dwFlags,SKeyDLL.iBaudRate,SKeyDLL.iSave,SKeyDLL.iPortState);
	DBG_OUT(buf);
	wsprintfA(buf,"-- ActivePort (%s) Port (%s)",SKeyDLL.szActivePort,SKeyDLL.szPort);
	DBG_OUT(buf);
}
#endif																	