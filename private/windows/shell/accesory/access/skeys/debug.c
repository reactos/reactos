/*--------------------------------------------------------------
 *
 * FILE:			DEBUG.C
 *
 * PURPOSE:		Debug Routines using a named pipe to output debug
 *					data.
 *
 * CREATION:		June 1993
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
 *----------- Includes   -------------------------------------------------*/

#include "windows.h"		   /* required for all Windows applications */
#include "debug.h"

//----------- Defines   -------------------------------------------------


#define SKEY_DBG_PIPE_NAME 	TEXT("\\\\.\\PIPE\\test") 			// Pipe name

#ifdef DEBUG

//----------- Variables   -------------------------------------------------
HANDLE	hPipe = 0;
BOOL	bDbgEnable = FALSE;

//----------- Function Prototypes   ----------------------------------------

/*---------------------------------------------------------------
 *
 * FUNCTION		void dbgOut(LPTSTR lpszMsg)
 *
 * TYPE			Debug
 *
 * PURPOSE		Writes the Error message 
 *
 * INPUTS		LPSTR - String to write
 *
 * RETURNS		None
 *
 *---------------------------------------------------------------*/
void dbgOut(LPSTR lpszMsg)
{

	DWORD bytesWritten;

	if(!bDbgEnable)					
		return;

	WriteFile (hPipe, lpszMsg, strlen(lpszMsg)+1,&bytesWritten, NULL);
}

/*---------------------------------------------------------------
 *
 * FUNCTION		void dbgOpen()
 *
 * TYPE			Debug
 *
 * PURPOSE		Open debug pipe
 *
 * INPUTS		None
 *
 * RETURNS		None
 *
 *---------------------------------------------------------------*/
void dbgOpen()
{
	TCHAR  errorBuf[80] = TEXT(""); // Error message buffer.

	if(bDbgEnable)					
		return;

	hPipe = CreateFile				// Connect to the named pipe.
		(
			SKEY_DBG_PIPE_NAME,     // Pipe name.
			GENERIC_WRITE			// Generic access, read/write.
			| GENERIC_READ,
			FILE_SHARE_READ			// Share both read and write.
			| FILE_SHARE_WRITE ,
			NULL,					// No security.
			OPEN_EXISTING,			// Fail if not existing.
			FILE_FLAG_OVERLAPPED,	// Use overlap.
			NULL					// No template.
		);

	bDbgEnable = TRUE;
	dbgOut("Debug Name");
	dbgOut("Opened Debug ------------------------------");
}

/*---------------------------------------------------------------
 *
 * FUNCTION		void dbgClose()
 *
 * TYPE			Debug
 *
 * PURPOSE		Closes the Debug Pipe
 *
 * INPUTS		None
 *
 * RETURNS		None
 *
 *---------------------------------------------------------------*/
void dbgClose()
{
	if(!bDbgEnable)					
		return;


	dbgOut("Closed Debug ------------------------------");
	bDbgEnable = FALSE;
	CloseHandle (hPipe);				// Yes - Close Pipe
	hPipe = 0;							// Clear handle
}

/*---------------------------------------------------------------
 *
 * FUNCTION		void dbgErr()
 *
 * TYPE			Debug
 *
 * PURPOSE		Writes the Error message and the Last Error Message
 *
 * INPUTS		LPSTR - String to write
 *
 * RETURNS		None
 *
 *---------------------------------------------------------------*/
void dbgErr(LPSTR lpStr)
{
	char 	szLastErr[512];
	DWORD 	dwErr = GetLastError();
	LPSTR	lpszMsgBuf;

	if (!bDbgEnable)					
		return;

	FormatMessageA
	(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM,
		NULL,
		dwErr,
		MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US),
		lpszMsgBuf,
		0,
		NULL 
	);

	wsprintfA(szLastErr, "Error    : %s", lpStr);
	dbgOut(szLastErr);
	wsprintfA(szLastErr, "LastError: (%d) %s",dwErr, (LPSTR)lpszMsgBuf);
	dbgOut(szLastErr);
}

#endif 	// DEBUG
