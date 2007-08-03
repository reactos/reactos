/*
 * COPYRIGHT:	See COPYING in the top level directory
 * PROJECT:	ReactOS logoff utility
 * FILE:		base\applications\logoff\logoff.c
 * PURPOSE:	Logoff current session, or another session, potentially on another machine
 * AUTHOR:	30.07.2007 - Frode Lillerud
 */
 
/* Note
 * This application is a lightweight version of shutdown.exe. It is intended to be function-compatible
 * with Windows' system32\logoff.exe commandline application.
 */
 
#define NDEBUG
#include "precomp.h"

//Commandline argument switches
LPTSTR szRemoteServerName = NULL;
BOOL bVerbose;

//---------------------------------------------------------------------- 
//
//Retrieve resource string and output the Usage to the console
//
//----------------------------------------------------------------------
static void PrintUsage() {
	LPTSTR lpUsage = NULL;
	
	if (AllocAndLoadString(&lpUsage, GetModuleHandle(NULL), IDS_USAGE)) {
		_putts(lpUsage);
	}
	
}

//----------------------------------------------------------------------
//
// Writes the last error as both text and error code to the console.
//
//----------------------------------------------------------------------
void DisplayLastError()
{
	int errorCode = GetLastError();
	LPTSTR lpMsgBuf;
	
	// Display the error message to the user
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL,
		errorCode,
		LANG_USER_DEFAULT,
		(LPTSTR) &lpMsgBuf,
		0,
		NULL);
	
	_ftprintf(stderr, lpMsgBuf);
	_ftprintf(stderr, _T("Error code: %d\n"), errorCode);
	
	LocalFree(lpMsgBuf);
}

//----------------------------------------------------------------------
//
//Sets flags based on commandline arguments
//
//----------------------------------------------------------------------
BOOL ParseCommandLine(int argc, TCHAR *argv[])
{
	int i;
	LPTSTR lpIllegalMsg;
	
	//FIXME: Add handling of commandline arguments to select the session number and name, and also name of remote machine
	//Example: logoff.exe 4 /SERVER:Master should logoff session number 4 on remote machine called Master.
	
	for (i = 1; i < argc; i++) {
		switch(argv[i][0]){
		case '-':
		case '/':
			// -v (verbose)
			if (argv[i][1] == 'v') {
				bVerbose = TRUE;
				break; //continue parsing the arguments
			}
			// -? (usage)
			else if(argv[i][1] == '?') {
				return FALSE; //display the Usage
			}			
		default:
			//Invalid parameter detected
			if (AllocAndLoadString(&lpIllegalMsg, GetModuleHandle(NULL), IDS_ILLEGAL_PARAM))
			_putts(lpIllegalMsg); 
			return FALSE;
		}
	}

	return TRUE;
}

//----------------------------------------------------------------------
//
//Main entry for program
//
//----------------------------------------------------------------------
int _tmain(int argc, TCHAR *argv[])
{
	LPTSTR lpLogoffRemote, lpLogoffLocal;

	//
	// Parse command line
	//
	if (!ParseCommandLine(argc, argv)) {	
		PrintUsage();
		return 1;
	}
		
	//
	//Should we log off session on remote server?
	//
	if (szRemoteServerName) {
		if (bVerbose) { 
			if (AllocAndLoadString(&lpLogoffRemote, GetModuleHandle(NULL), IDS_LOGOFF_REMOTE))
			_putts(lpLogoffRemote); 
		}
		
		//FIXME: Add Remote Procedure Call to logoff user on a remote machine
		_ftprintf(stderr, "Remote Procedure Call in logoff.exe has not been implemented");
	}
	//
	//Perform logoff of current session on local machine instead
	//
	else {
		if (bVerbose) {
			//Get resource string, and print it.
			if (AllocAndLoadString(&lpLogoffLocal, GetModuleHandle(NULL), IDS_LOGOFF_LOCAL))
			_putts(lpLogoffLocal); 
		}
		
		//Actual logoff
		if (!ExitWindows(NULL, NULL)) {
			DisplayLastError();
			return 1;
		}
	}
		
	return 0;
}
/* EOF */
