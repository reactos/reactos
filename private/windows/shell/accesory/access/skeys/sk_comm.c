/*--------------------------------------------------------------
 *
 * FILE:			SK_COMM.C
 *
 * PURPOSE:		The file contains the Functions responsible for
 *					managing the COMM ports
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

//#define	WINVER 0x0300

// added to be compatible with new windows.h (12/91) and wintric.h 
#define	USECOMM	

#include 	<stdio.h>
#include 	<stdlib.h>
#include	<process.h>

#include	"windows.h"
//#include "winstric.h"				// added for win 3.1 compatibility 1/92

#include	"gide.h"					// Serial Keys Function Proto
#include	"initgide.h"	   			// Serial Keys Function Proto
#include	"debug.h"
#include	"sk_defs.h"
#include	"sk_comm.h"
#include    "drivers.h"
#include	"sk_ex.h"

#define COMMTERMINATE 0xFFFFFFFF     // this 'character' indicates a request to terminate

// Local Function ProtoTypes --------------------------------

static BOOL	OpenComm();
static void	__cdecl ProcessComm(VOID *notUsed);
static int	ReadComm();


// Local Variables ---------------------------------------------------

static DCB	s_dcbCommNew;			// New DCB for comm port
static DCB	s_dcbCommOld;			// Origional DCB for comm port

static OVERLAPPED s_oRead;			// Overlapped structure for reading.

static	HANDLE	s_hFileComm;

static	HANDLE	s_hThreadComm = NULL;

static	HDESK	s_hdeskUser = NULL;

static	DWORD	s_NullTimer;
static	int		s_NullCount=0;

static	HANDLE  s_ahEvents[2] = {NULL, NULL};

#define iEventComm			0
#define iEventExit			1


/*---------------------------------------------------------------
 *
 *	Global Functions -
 *
 *---------------------------------------------------------------*/

 /*---------------------------------------------------------------
 *
 * FUNCTION	void InitComm()
 *
 * TYPE         Global
 *
 * PURPOSE		
 *
 * INPUTS		None
 *
 * RETURNS		None
 *
 *---------------------------------------------------------------*/
BOOL InitComm()
{
    BOOL fOk = TRUE;

	DBG_OUT("InitComm()");

	// Create Event for Overlap File Read 
	s_ahEvents[iEventComm] = CreateEvent(NULL, TRUE, FALSE, NULL);	
	fOk = (NULL != s_ahEvents[iEventComm]);
	if (fOk) 
	{
		s_ahEvents[iEventExit] = CreateEvent(NULL, TRUE, FALSE, NULL);	
		fOk = (NULL != s_ahEvents[iEventExit]);
	}

	if (!fOk)
	{
		TerminateComm();
	}

	return(fOk);
}

/*---------------------------------------------------------------
 *
 * FUNCTION	void TerminateComm()
 *
 *	TYPE		Global
 *
 * PURPOSE		The function is called for the final shutdown of
 *				the comm port.
 *
 * INPUTS		None
 *
 * RETURNS		TRUE - Start Successful
 *				FALSE - Start Failed
 *
 *---------------------------------------------------------------*/
void TerminateComm()
{
	BOOL fOk;
	int i;

	DBG_OUT("TerminateComm()");

	StopComm();

	for (i = 0; i < ARRAY_SIZE(s_ahEvents); ++i)
	{
		if (NULL != s_ahEvents[i])
		{
			fOk = CloseHandle(s_ahEvents[i]);
			DBG_ERR1(fOk, "Unable to Close Event");
			s_ahEvents[i] = NULL;
		}
	}

	return;
}


/*---------------------------------------------------------------
 *
 * FUNCTION	BOOL StartComm()
 *
 *	TYPE		Global
 *
 * PURPOSE		The function is call to start the thread to 
 *				read and process data coming from the comm port.
 *				It will create a thread and an event.  This function
 *				assumes that the comm port is already opened.
 *
 * INPUTS		None
 *
 * RETURNS		TRUE - Start Successful
 *				FALSE - Start Failed
 *
 *---------------------------------------------------------------*/
BOOL StartComm()
{
	BOOL fOk = TRUE;
	DWORD 	Id;

	DBG_OUT("StartComm()");

	// ----------------------------------------------------------
	// Note:	Comm Threads are started and stopped whenever
	// 			the com port is changed. The User logs in or out
	// 			or the comm configuration is changed.
	// ----------------------------------------------------------

	if (NULL == s_hFileComm && // no port currently in use
		(skNewKey.dwFlags & SERKF_AVAILABLE) &&
		(skNewKey.dwFlags & SERKF_SERIALKEYSON))
	{
		if (NULL != s_hThreadComm) 
		{
			// This is an unexpected situation.  We have the comm thread
			// running with no open comm port.  The thread must be hung.
			// Let's close the open handle and forget about it.

			DBG_OUT("StartComm() unexpected (NULL != s_hThreadComm)");
			WaitForSingleObject(s_hThreadComm, 5 * 1000);

			if (NULL != s_hThreadComm)
			{
				DBG_OUT("StartComm() s_hThreadComm abandoned");
				CloseHandle(s_hThreadComm);		
				s_hThreadComm = NULL;
			}
		}


		// skNewKey is used by OpenComm.  We're setting skCurKey to default
		// values in case OpenComm fails.

		skCurKey.iBaudRate = 300;				// No - Reset To Default Values
		skCurKey.iPortState= 0;
		skCurKey.dwFlags   = 0;
		lstrcpy(skCurKey.lpszActivePort, TEXT("COM1"));
		lstrcpy(skCurKey.lpszPort, TEXT("COM1"));

		if (!OpenComm())							// Did Comm Open Ok?
		{
			skNewKey.iBaudRate = 300;				// No - Reset To Default Values
			skNewKey.iPortState= 0;
			skNewKey.dwFlags   = 0;
			lstrcpy(skNewKey.lpszActivePort, TEXT("COM1"));
			lstrcpy(skNewKey.lpszPort, TEXT("COM1"));
			fOk = FALSE;
		}
		else
		{
            // ensure we start with clean events

			ResetEvent(s_ahEvents[iEventComm]);
            ResetEvent(s_ahEvents[iEventExit]);

			memset(&s_oRead, 0, sizeof(OVERLAPPED));	// Init Struct
			s_oRead.hEvent = s_ahEvents[iEventComm];	// Store Event

			// Create thread to handle Processing Comm Port
			s_hThreadComm = (HANDLE)CreateThread(	// Start Service Thread
				0, 0,
				(LPTHREAD_START_ROUTINE) ProcessComm,
				0, 0,&Id);							// argument to thread

			if (NULL == s_hThreadComm)// Is Thread Handle Valid?
			{
				// Close out the Comm Port
				SetCommState(s_hFileComm, &s_dcbCommOld);	// Restore Comm State 
				CloseHandle(s_hFileComm);
				s_hFileComm = NULL;
				skCurKey.iPortState = 0;

				fOk = FALSE;
			}
			else
			{
				// Comm Thread Successfully Started Set The Current Values
				skCurKey.iBaudRate = skNewKey.iBaudRate;
				skCurKey.iPortState	 = 2;
				skCurKey.dwFlags = SERKF_SERIALKEYSON	
									| SERKF_AVAILABLE	
									| SERKF_ACTIVE;

				lstrcpy(skCurKey.lpszActivePort, skNewKey.lpszActivePort);
				lstrcpy(skCurKey.lpszPort, skNewKey.lpszActivePort);

				DBG_OUT("---- Comm Started");
			}
		}
	}
	return(fOk);
}

/*---------------------------------------------------------------
 *
 * FUNCTION	void SuspendComm()
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
void SuspendComm()
{
	DBG_OUT("SuspendComm()");

	if (NULL != s_hThreadComm)
	{
		SuspendThread(s_hThreadComm);
	}
}

/*---------------------------------------------------------------
 *
 * FUNCTION	void ResumeComm()
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
void ResumeComm()
{
	if (s_hThreadComm != NULL)
		ResumeThread(s_hThreadComm);	
}

/*---------------------------------------------------------------
 *
 * FUNCTION	void StopComm()
 *
 *	TYPE		Global
 *
 * PURPOSE		The function is called to stop the thread  
 *				reading and processing data coming from the comm port.
 *
 * INPUTS		None
 *
 * RETURNS		TRUE - Start Successful
 *				FALSE - Start Failed
 *
 *---------------------------------------------------------------*/
void StopComm()
{
	DBG_OUT("StopComm()");

	if (NULL != s_hFileComm)
	{
		skCurKey.dwFlags = SERKF_AVAILABLE;	

		SetEvent(s_ahEvents[iEventExit]);

		if (NULL != s_hThreadComm)
		{
			DWORD dwRet;
			BOOL fOk;

			dwRet = WaitForSingleObject(s_hThreadComm, 5 * 1000);
            DBG_ERR1(WAIT_OBJECT_0 == dwRet, "StopComm() Comm Thread may be hung.");
			CloseHandle(s_hThreadComm);		
			s_hThreadComm = NULL;

			SetCommState(s_hFileComm, &s_dcbCommOld);	// Restore Comm State 
			fOk = CloseHandle(s_hFileComm);			// Close the Comm Port
			DBG_ERR1(fOk, "Unable to Close Comm File");
			s_hFileComm = NULL;

			skCurKey.iPortState	 = 0;
		}
	}
}

/*---------------------------------------------------------------
 *
 * FUNCTION	void SetCommBaud(int Baud)
 *
 *	TYPE		Global
 *
 * PURPOSE		
 *				
 *
 * INPUTS		None
 *
 * RETURNS		TRUE - Start Successful
 *				FALSE - Start Failed
 *
 *---------------------------------------------------------------*/
void SetCommBaud(int Baud)
{
	DBG_OUT("SetCommBaud()");

	switch (Baud)				// Check for Valid Baud Rates
	{
		case 300:
		case 600:
		case 1200:
		case 2400:
		case 4800:
		case 9600:
		case 19200:
			break;				// Baud Ok

		default:
			return;				// Baud Invalid
	}

	skNewKey.iBaudRate = Baud;				// Save Baud

	if (NULL != s_hFileComm)						// Is Comm Port Open?
	{
		s_dcbCommNew.BaudRate = skNewKey.iBaudRate;	// Set new DCB Params
  		SetCommState(s_hFileComm, &s_dcbCommNew);	// State Change Ok?
		skCurKey.iBaudRate = skNewKey.iBaudRate;	// Save New Baud Rate
	}
}

/*---------------------------------------------------------------
 *
 *		Local Functions
 *
/*---------------------------------------------------------------


/*---------------------------------------------------------------
 *
 * FUNCTION     void _CRTAPI1 ProcessComm()
 *
 *	TYPE		Local
 *
 * PURPOSE		The function is the thread the cycles thru reading
 *				processing data coming from the comm port.
 *
 * INPUTS		None
 *
 * RETURNS		None
 *
 *---------------------------------------------------------------*/
static void __cdecl ProcessComm(VOID *notUsed)
{
	int 	c;
	HWINSTA	hwinstaSave;
	HWINSTA	hwinstaUser;
	HDESK	hdeskSave;
	DWORD	dwThreadId;
	BOOL    fCont;

	DBG_OUT("ProcessComm()");

	//------------------------------------------------------
	//
	// Note:
	//	The following code set the input focus to the current
	//  desktop.  It is needed to insure that keyboard and mouse
	//	events  will be passed to the current desktop.
	//
	//------------------------------------------------------

	hwinstaSave = GetProcessWindowStation();
	dwThreadId = GetCurrentThreadId();
	hdeskSave = GetThreadDesktop(dwThreadId);

	hwinstaUser = OpenWindowStation(TEXT("WinSta0"), FALSE, MAXIMUM_ALLOWED);
	SetProcessWindowStation(hwinstaUser);


	serialKeysStartUpInit();				// Initialize the Serial Keys
	fCont = TRUE;

	while (fCont)
	{
		c = ReadComm();						// Read Char from Com Port
		switch (c)
		{
		case 0:
			// Is Character a Null

			// Is Null Timer > 30 Seconds
			if ((GetTickCount() - s_NullTimer) > 30000) 
			{
				s_NullTimer = GetTickCount();	// Yes - Reset Timer
				s_NullCount = 1;				// Reset Null Count
			} else 	{
				
				s_NullCount++;				// No - Inc Null Count
				if (s_NullCount == 3)		// Have we had 3 Null in 30 Sec.?
				{
					// the user is requesting us to reset
					SetCommBaud(300);		

					// DeskSwitch should be unnessary, but if it gets out of sync,
					// this is where we resync

////////////////////DeskSwitchToInput();    // BUGBUG remove this line
					s_NullCount = 0;		// Reset Null Counter
				}
			}
			break;		
			
		case COMMTERMINATE:
			fCont = FALSE;
			break;

		default:
			DeskSwitchToInput();
			serialKeysBegin((UCHAR)c);	// Process Char
			break;

		}
	}

	SetThreadDesktop(hdeskSave);
	SetProcessWindowStation(hwinstaSave);
	CloseDesktop(s_hdeskUser);
	s_hdeskUser = NULL;
	CloseWindowStation(hwinstaUser);

	ExitThread(0);					// Close Thread
}

/*---------------------------------------------------------------
 *
 * FUNCTION	BOOL OpenComm()
 *
 *	TYPE		Local
 *
 * PURPOSE		This Function opens the comm port and sets the new
 *				sets the Device Control Block.
 *
 * INPUTS		None
 *
 * RETURNS		TRUE - Open Ok / FALSE - Open Failed
 *
 *---------------------------------------------------------------*/
static BOOL OpenComm()
{
	BOOL fOk = FALSE;
	COMMTIMEOUTS ctmo;

	DBG_OUT("OpenComm()");

	s_hFileComm = CreateFile(
			skNewKey.lpszActivePort,// FileName (Com Port)
			GENERIC_READ ,			// Access Mode
			0,						// Share Mode
			NULL,					// Address of Security Descriptor
			OPEN_EXISTING,			// How to Create	
			FILE_ATTRIBUTE_NORMAL	// File Attributes
			| FILE_FLAG_OVERLAPPED, // Set for Async File Reads
  			NULL);					// Templet File.

	if (INVALID_HANDLE_VALUE == s_hFileComm)	// File Ok?
	{
		DBG_OUT("- Invalid File");

		s_hFileComm = NULL;
	}
	else
	{
		BOOL fRet;
		COMMPROP cmmp;

		SetupComm(
			s_hFileComm,
			1024,	// size of input buffer
			1024);	// size of output buffer

		memset(&s_dcbCommOld, 0, sizeof(s_dcbCommOld));
		s_dcbCommOld.DCBlength = sizeof(s_dcbCommOld);

		GetCommState(s_hFileComm, &s_dcbCommOld);	// Save Old DCB for restore
		s_dcbCommNew = s_dcbCommOld;	// Copy to New

		// set XoffLim and XonLim based on actual buffer size

		fRet = GetCommProperties(s_hFileComm, &cmmp);
		if (fRet)
		{
			s_dcbCommNew.XoffLim = (WORD)(cmmp.dwCurrentRxQueue / 4);
			s_dcbCommNew.XonLim = (WORD)(cmmp.dwCurrentRxQueue / 4);
		}

		s_dcbCommNew.BaudRate	= skNewKey.iBaudRate; 	// Set new DCB Params
		s_dcbCommNew.ByteSize	= 8;
		s_dcbCommNew.Parity 	= NOPARITY;
		s_dcbCommNew.StopBits	= ONESTOPBIT;
		s_dcbCommNew.fOutX 		= FALSE;  	// XOn/XOff used during transmission
  		s_dcbCommNew.fInX 		= TRUE;	  	// XOn/XOff used during reception 
  		s_dcbCommNew.fNull 		= FALSE;  	// tell windows not to strip nulls 
  		s_dcbCommNew.fBinary	= TRUE;

  		s_dcbCommNew.fOutxCtsFlow	= FALSE;
  		s_dcbCommNew.fOutxDsrFlow	= FALSE;
  		s_dcbCommNew.fDtrControl	= DTR_CONTROL_ENABLE;
  		s_dcbCommNew.fDsrSensitivity   = FALSE;
  		s_dcbCommNew.fErrorChar		= TRUE;
  		s_dcbCommNew.fRtsControl	= RTS_CONTROL_DISABLE;
  		s_dcbCommNew.fAbortOnError	= FALSE;
  		s_dcbCommNew.XonChar		= (char)0x11;
  		s_dcbCommNew.XoffChar		= (char)0x13;
  		s_dcbCommNew.ErrorChar		= '\0';

  		fOk = SetCommState(s_hFileComm, &s_dcbCommNew);

		memset(&ctmo, 0, sizeof(ctmo));
		SetCommTimeouts(s_hFileComm, &ctmo);
	}

	if (!fOk && NULL != s_hFileComm)
	{
		CloseHandle(s_hFileComm);
		s_hFileComm = NULL;
	}

	return(fOk);
}


/*---------------------------------------------------------------
 *
 * FUNCTION	int ReadComm()
 *
 *	TYPE		Local
 *
 * PURPOSE		This Function reads a character from the comm port.
 *				If no character is present it wait on the HEV_COMM
 *				Event untill a character is present
 *
 * INPUTS		None
 *
 * RETURNS		int - Character read (-1 = Error Read)
 *
 *---------------------------------------------------------------*/
static int ReadComm()
{
	int     nRet;
	DWORD	cbRead = 0;
	DWORD	lastError, ComError;
	DWORD	dwRetWait;
	BOOL    fOk;
	BOOL    fExit;

	BOOL	fExitLoop = FALSE;		// Boolean Flag to exit loop.
	UCHAR   uchBuff;
	COMSTAT ComStat;
	
	fExit = (WAIT_OBJECT_0 == WaitForSingleObject(s_ahEvents[iEventExit], 0));

	if (!fExit)
	{
		fOk = ReadFile(s_hFileComm, &uchBuff, 1, &cbRead, &s_oRead);

		if (!fOk)						// Was there a Read Error?
		{
			lastError = GetLastError();	// This var can be useful for debugging
			switch (lastError)			
			{
			// If Error = IO_PENDING, wait til
			// the event hadle signals success,
			case ERROR_IO_PENDING:
				dwRetWait = WaitForMultipleObjects(
					ARRAY_SIZE(s_ahEvents), s_ahEvents, FALSE, INFINITE);

				switch (dwRetWait - WAIT_OBJECT_0)
				{
				case iEventComm:
					// this is the expected event
					GetOverlappedResult(s_hFileComm, &s_oRead, &cbRead, FALSE);

					if (cbRead < 1)			// Did we read bytes;
					{
						// There was some error, return null
						nRet = 0;
					}
					else
					{
						nRet = uchBuff;
					}
					break;

				case iEventExit:
					fExit = TRUE;
					// fall through

				default:
					// this indicates and error and we exit to prevent loop
					nRet = COMMTERMINATE;
					break;
				}
				break;

			default:	
				fOk = ClearCommError(s_hFileComm, &ComError,&ComStat);
				if (fOk)
				{
					nRet = 0;          // return a null
				}
				else
				{
					nRet = COMMTERMINATE;        // terminate
				}
				break;
			}
		}
		else
		{
			if (cbRead < 1)			// Did we read bytes;
			{
				// There was some error, return null
				nRet = 0;
			}
			else
			{
				nRet = uchBuff;
			}

		}
	}
	if (fExit)
	{
		ResetEvent(s_ahEvents[iEventExit]);
		nRet = COMMTERMINATE;
	}
	return(nRet);
}
