/****************************** Module Header ******************************\
* Module Name: globals.h
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* This module contains all the server's global variables
* One must be executing on the server's context to manipulate
* any of these variables or call any of these functions.  Serializing access
* to them is also a good idea.
*
* History:
* 10-15-90 DarrinM      Created.
\***************************************************************************/

#ifndef _GLOBALS_
#define _GLOBALS_

extern CRITICAL_SECTION gcsUserSrv;
extern BOOL gfAutoEndTask;

extern DWORD gdwRIPFlags;

/*
 * logon process id
 */
extern DWORD gIdLogon;


/*
 * Hard error globals
 */
extern DWORD gdwHardErrorThreadId;
extern HANDLE gNtDllHandle;
extern HANDLE gEventSource;
extern PHARDERRORINFO gphiList;
/*
 * EndTask / Shutdown stuff
 */
extern DWORD gdwThreadEndSession;
extern HANDLE gheventCancel;
extern HANDLE gheventCancelled;
extern PWSTR gpwszaSUCCESS;
extern PWSTR gpwszaSYSTEM_INFORMATION;
extern PWSTR gpwszaSYSTEM_WARNING;
extern PWSTR gpwszaSYSTEM_ERROR;

/*
 * EndTask globals
 */
extern DWORD   gpidWOW;

extern ULONG gSessionId;


#endif // _GLOBALS_
