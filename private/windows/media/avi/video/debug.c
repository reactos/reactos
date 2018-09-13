//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1992, 1994  Microsoft Corporation.  All Rights Reserved.
//
//--------------------------------------------------------------------------;
//
//  debug.c
//
//  Description:
//      This file contains code yanked from several places to provide debug
//      support that works in win 16 and win 32.
//
//
//==========================================================================;

#include <win32.h> // to translate DBG -> DEBUG for NT builds
#ifdef DEBUG

#include <windows.h>
#include <windowsx.h>
#include <stdarg.h>
#define _INC_DEBUG_CODE
#include "debug.h"


//
//  since we don't UNICODE our debugging messages, use the ASCII entry
//  points regardless of how we are compiled.
//
#ifdef _WIN32
    #include <wchar.h>
#else
    #define lstrcpyA		lstrcpy
    #define lstrcatA            lstrcat
    #define lstrlenA            lstrlen
    #define GetProfileIntA      GetProfileInt
    #define OutputDebugStringA  OutputDebugString
    #define wsprintfA           wsprintf
    #define MessageBoxA         MessageBox
#endif

//
//
//
#define cDbgSpecs (sizeof(aszDbgSpecs) / sizeof(aszDbgSpecs[0]))

BOOL    __gfDbgEnabled[cDbgSpecs];     // master enable
UINT    __guDbgLevel[cDbgSpecs];       // current debug level


//--------------------------------------------------------------------------;
//
//  void DbgVPrintF
//
//  Description:
//
//
//  Arguments:
//      LPSTR szFormat:
//
//      va_list va:
//
//  Return (void):
//      No value is returned.
//
//--------------------------------------------------------------------------;

void FAR CDECL DbgVPrintF
(
    LPSTR                   szFormat,
    va_list                 va
)
{
    char                ach[DEBUG_MAX_LINE_LEN];
    BOOL                fDebugBreak = FALSE;
    BOOL                fPrefix     = TRUE;
    BOOL                fCRLF       = TRUE;

    ach[0] = '\0';

    for (;;)
    {
        switch (*szFormat)
        {
            case '!':
                fDebugBreak = TRUE;
                szFormat++;
                continue;

            case '`':
                fPrefix = FALSE;
                szFormat++;
                continue;

            case '~':
                fCRLF = FALSE;
                szFormat++;
                continue;
        }

        break;
    }

    if (fDebugBreak)
    {
        ach[0] = '\007';
        ach[1] = '\0';
    }

    if (fPrefix)
    {
        lstrcatA(ach, DEBUG_MODULE_NAME ": ");
    }

#ifdef _WIN32
    wvsprintfA(ach + lstrlenA(ach), szFormat, va);
#else
    wvsprintf(ach + lstrlenA(ach), szFormat, (LPSTR)va);
#endif

    if (fCRLF)
    {
        lstrcatA(ach, "\r\n");
    }

    OutputDebugStringA(ach);

    if (fDebugBreak)
    {
        DebugBreak();
    }
} // DbgVPrintF()


//--------------------------------------------------------------------------;
//
//  void dprintfS
//
//  Description:
//      dprintfS() is called by the DPFS() macro if DEBUG is defined at compile
//      time. It is recommended that you only use the DPFS() macro to call
//      this function--so you don't have to put #ifdef DEBUG around all
//      of your code.
//
//  Arguments:
//	UINT uDbgSpec:
//
//      UINT uDbgLevel:
//
//      LPSTR szFormat:
//
//  Return (void):
//      No value is returned.
//
//--------------------------------------------------------------------------;

void FAR CDECL dprintfS
(
    UINT		    uDbgSpec,
    UINT                    uDbgLevel,
    LPSTR                   szFormat,
    ...
)
{
    va_list va;

    if (!__gfDbgEnabled[uDbgSpec] || (__guDbgLevel[uDbgSpec] < uDbgLevel))
        return;

    va_start(va, szFormat);
    DbgVPrintF(szFormat, va);
    va_end(va);
} // dprintf()


//--------------------------------------------------------------------------;
//
//  void dprintf
//
//  Description:
//      dprintf() is called by the DPF() macro if DEBUG is defined at compile
//      time. It is recommended that you only use the DPF() macro to call
//      this function--so you don't have to put #ifdef DEBUG around all
//      of your code.
//
//  Arguments:
//      UINT uDbgLevel:
//
//      LPSTR szFormat:
//
//  Return (void):
//      No value is returned.
//
//--------------------------------------------------------------------------;

void FAR CDECL dprintf
(
    UINT                    uDbgLevel,
    LPSTR                   szFormat,
    ...
)
{
    va_list va;

    if (!__gfDbgEnabled[dbgNone] || (__guDbgLevel[dbgNone] < uDbgLevel))
        return;

    va_start(va, szFormat);
    DbgVPrintF(szFormat, va);
    va_end(va);
} // dprintf()


//--------------------------------------------------------------------------;
//
//  BOOL DbgEnable
//
//  Description:
//
//
//  Arguments:
//      BOOL fEnable:
//
//  Return (BOOL):
//      Returns the previous debugging state.
//
//--------------------------------------------------------------------------;

BOOL WINAPI DbgEnable
(
    UINT	uDbgSpec,
    BOOL        fEnable
)
{
    BOOL	fOldState;

    fOldState			= __gfDbgEnabled[uDbgSpec];
    __gfDbgEnabled[uDbgSpec]	= fEnable;

    return (fOldState);
} // DbgEnable()


//--------------------------------------------------------------------------;
//
//  UINT DbgSetLevel
//
//  Description:
//
//
//  Arguments:
//      UINT uLevel:
//
//  Return (UINT):
//      Returns the previous debugging level.
//
//--------------------------------------------------------------------------;

UINT WINAPI DbgSetLevel
(
    UINT	uDbgSpec,
    UINT        uLevel
)
{
    UINT        uOldLevel;

    uOldLevel		    = __guDbgLevel[uDbgSpec];
    __guDbgLevel[uDbgSpec]  = uLevel;

    return (uOldLevel);
} // DbgSetLevel()


//--------------------------------------------------------------------------;
//
//  UINT DbgGetLevel
//
//  Description:
//
//
//  Arguments:
//      None.
//
//  Return (UINT):
//      Returns the current debugging level.
//
//--------------------------------------------------------------------------;

UINT WINAPI DbgGetLevel
(
    UINT    uDbgSpec
)
{
    return (__guDbgLevel[uDbgSpec]);
} // DbgGetLevel()


//--------------------------------------------------------------------------;
//
//  UINT DbgInitializeSpec
//
//  Description:
//
//
//  Arguments:
//      BOOL fEnable:
//
//  Return (UINT):
//      Returns the debugging level that was set.
//
//--------------------------------------------------------------------------;

UINT WINAPI DbgInitializeSpec
(
    UINT	uDbgSpec,
    BOOL	fEnable
)
{
    UINT        uLevel;
    char	szKey[DEBUG_MAX_LINE_LEN];

    lstrcpyA(szKey, DEBUG_MODULE_NAME);
    lstrcatA(szKey, aszDbgSpecs[uDbgSpec]);
	
    uLevel = GetProfileIntA(DEBUG_SECTION, szKey, (UINT)-1);
    if ((UINT)-1 == uLevel)
    {
	//
	//  if the debug key is not present, then force debug output to
	//  be disabled. this way running a debug version of a component
	//  on a non-debugging machine will not generate output unless
	//  the debug key exists.
	//
	uLevel  = 0;
	fEnable = FALSE;
    }

    DbgSetLevel(uDbgSpec, uLevel);
    DbgEnable(uDbgSpec, fEnable);

    return (__guDbgLevel[uDbgSpec]);
} // DbgInitialize()


//--------------------------------------------------------------------------;
//
//  VOID DbgInitialize
//
//  Description:
//
//
//  Arguments:
//      BOOL fEnable:
//
//  Return (UINT):
//      Returns the debugging level that was set.
//
//--------------------------------------------------------------------------;

VOID WINAPI DbgInitialize
(
    BOOL	fEnable
)
{
    UINT	i;

    for (i=0; i<sizeof(__guDbgLevel)/sizeof(__guDbgLevel[0]); i++)
    {
	DbgInitializeSpec(i, fEnable);
    }

    return;
} // DbgInitialize()


//--------------------------------------------------------------------------;
//
//  void _Assert
//
//  Description:
//      This routine is called if the ASSERT macro (defined in debug.h)
//      tests an expression that evaluates to FALSE.  This routine
//      displays an "assertion failed" message box allowing the user to
//      abort the program, enter the debugger (the "retry" button), or
//      ignore the assertion and continue executing.  The message box
//      displays the file name and line number of the _Assert() call.
//
//  Arguments:
//      char *  szFile: Filename where assertion occurred.
//      int     iLine:  Line number of assertion.
//
//--------------------------------------------------------------------------;

#ifndef _WIN32
#pragma warning(disable:4704)
#endif

void WINAPI _Assert
(
    char *  szFile,
    int     iLine
)
{
    static char     ach[300];       // debug output (avoid stack overflow)
    int	            id;
#ifndef _WIN32
    int             iExitCode;
#endif

    wsprintfA(ach, "Assertion failed in file %s, line %d.  [Press RETRY to debug.]", (LPSTR)szFile, iLine);

    id = MessageBoxA(NULL, ach, "Assertion Failed",
            MB_SYSTEMMODAL | MB_ICONHAND | MB_ABORTRETRYIGNORE );

	switch (id)
	{

	case IDABORT:               // Kill the application.
#ifndef _WIN32
        iExitCode = 0;
        _asm
        {
	        mov	ah, 4Ch
	        mov	al, BYTE PTR iExitCode
	        int     21h
        }
#else
        FatalAppExit(0, TEXT("Good Bye"));
#endif // WIN16
		break;

	case IDRETRY:               // Break into the debugger.
		DebugBreak();
		break;

	case IDIGNORE:              // Ignore assertion, continue executing.
		break;
	}
} // _Assert

#ifndef _WIN32
#pragma warning(default:4704)
#endif

#endif // #ifdef DEBUG

