/**************************************************************************\
 Dbg.c
 Copyright (C) 1990-96 Bloorview MacMillan Centre
 Toronto, Ontario  M4G 1R8

 Sends debug messages to the TRACEWIN application. This file can be used
 in global hooks, where the code will be injected into every app within
 the system and will send messages from any process space.as the hook is
 being called.


  Note, there is no shared date. Each process receives its own copy of the
  variables.
\**************************************************************************/

#include <windows.h>
#include <stdarg.h>
#include <stdio.h>

#include "dbg.h"

#ifdef __cplusplus
extern "C" 
{
#endif  // __cplusplus

// local prototypes
int dbgOutput( char *szBuf );
BOOL dbgInit( void );

#define MAXBUF 500
// Careful, note inconsistent use of TRACEWND and TRACEWIN
#define TRACEWND_CLASSNAME	TEXT("MfxTraceWindow")
#define TRACEWND_MESSAGE	TEXT("*WM_TRACE_MSG")
#define ID_COPYDATA_TRACEMSG	32774

HWND	hwndTrace = 0;						// Handle to TRACEWIN app
// Use WM_COPYDATA instead
//UINT	wm_trace_msg = WM_NULL;			// Registered during initialization

int Dbg( const char *fmtbuf, ... )
	{
   va_list argptr;						/* -> variable argument list */
	char szBuf[MAXBUF+1];

   va_start( argptr, fmtbuf );      /* get pointer to argument list */
   _vsnprintf( szBuf, MAXBUF, fmtbuf, argptr );
   va_end( argptr );                /* done with variable arguments */

	szBuf[MAXBUF] = '\0';				// make sure it's terminated

	dbgOutput( szBuf );

	return 0;
	}


int xDbg( const char *f, ...)
	{
	return 0;
	}


int dbgOutput( char *szBuf )
	{
	//ATOM	atom;

	if (dbgInit())
		{
		COPYDATASTRUCT cds;

		//atom = GlobalAddAtom( szBuf );
		//SendMessage( hwndTrace, wm_trace_msg, (WPARAM)atom, 0 );
		//GlobalDeleteAtom( atom );

		cds.dwData = ID_COPYDATA_TRACEMSG;
		cds.cbData = strlen( szBuf );
		cds.lpData = (void *)szBuf;
		SendMessage( hwndTrace, WM_COPYDATA, (WPARAM)0, (LPARAM)&cds );
		}
	else
		{
		OutputDebugStringA( szBuf );
		}
	
	return 0;
	}


BOOL dbgInit( void )
	{
	if (hwndTrace)		// only initialize once per process
		return TRUE;

	hwndTrace = FindWindow( TRACEWND_CLASSNAME, 0 );
	if (hwndTrace)
		{
		//wm_trace_msg = RegisterWindowMessage( TRACEWND_MESSAGE );
		return TRUE;
		}
	else
		{
		return FALSE;
		}
	}

#ifdef __cplusplus
}
#endif  // __cplusplus

