//+---------------------------------------------------------------------------
//  Copyright (C) 1991-1994, Microsoft Corporation.
//
//  File:       assert.cxx
//
//  Contents:   Debugging output routines
//
//  History:    23-Jul-91   KyleP       Created.
//              09-Oct-91   KevinRo     Major changes and comments added
//              18-Oct-91   vich        moved debug print routines out
//              10-Jun-92   BryanT      Switched to w4crt.h instead of wchar.h
//              30-Sep-93   KyleP       DEVL obsolete
//               7-Oct-94   BruceFo     Ripped out all kernel, non-FLAT,
//                                      DLL-specific, non-Win32 functionality.
//                                      Now it's basically "print to the
//                                      debugger" code.
//
//----------------------------------------------------------------------------

#if DBG == 1

#include <windows.h>
#include <stdio.h>
#include <stdarg.h>

#include "debug.h"

//////////////////////////////////////////////////////////////////////////////

unsigned long Win4InfoLevel = DEF_INFOLEVEL;
unsigned long Win4InfoMask = 0xffffffff;
unsigned long Win4AssertLevel = ASSRT_MESSAGE | ASSRT_BREAK | ASSRT_POPUP;

//////////////////////////////////////////////////////////////////////////////

static int _cdecl w4dprintf(const char *format, ...);
static int _cdecl w4vdprintf(const char *format, va_list arglist);

//////////////////////////////////////////////////////////////////////////////

static CRITICAL_SECTION s_csMessageBuf;
static char g_szMessageBuf[500];		// this is the message buffer

static int _cdecl w4dprintf(const char *format, ...)
{
	int ret;

    va_list va;
    va_start(va, format);
	ret = w4vdprintf(format, va);
    va_end(va);

	return ret;
}


static int _cdecl w4vdprintf(const char *format, va_list arglist)
{
	int ret;

	EnterCriticalSection(&s_csMessageBuf);
	ret = vsprintf(g_szMessageBuf, format, arglist);
	OutputDebugStringA(g_szMessageBuf);
	LeaveCriticalSection(&s_csMessageBuf);
	return ret;
}

//////////////////////////////////////////////////////////////////////////////

//+---------------------------------------------------------------------------
//
//  Function:   _asdprintf
//
//  Synopsis:   Calls vdprintf to output a formatted message.
//
//  History:    18-Oct-91   vich Created
//
//----------------------------------------------------------------------------
inline void __cdecl
_asdprintf(
    char const *pszfmt, ...)
{
    va_list va;
    va_start(va, pszfmt);

    vdprintf(DEB_FORCE, "Assert", pszfmt, va);

    va_end(va);
}

//+---------------------------------------------------------------------------
//
//  Function:   Win4AssertEx, private
//
//  Synopsis:   Display assertion information
//
//  Effects:    Called when an assertion is hit.
//
//----------------------------------------------------------------------------

EXPORTIMP void APINOT
Win4AssertEx(
    char const * szFile,
    int iLine,
    char const * szMessage)
{
    if (Win4AssertLevel & ASSRT_MESSAGE)
    {
        DWORD tid = GetCurrentThreadId();

		_asdprintf("%s File: %s Line: %u, thread id %d\n",
            szMessage, szFile, iLine, tid);
    }

    if (Win4AssertLevel & ASSRT_POPUP)
    {
        int id = PopUpError(szMessage,iLine,szFile);

        if (id == IDCANCEL)
        {
            DebugBreak();
        }
    }
    else if (Win4AssertLevel & ASSRT_BREAK)
    {
        DebugBreak();
    }
}


//+------------------------------------------------------------
// Function:    SetWin4InfoLevel(unsigned long ulNewLevel)
//
// Synopsis:    Sets the global info level for debugging output
// Returns:     Old info level
//
//-------------------------------------------------------------

EXPORTIMP unsigned long APINOT
SetWin4InfoLevel(
    unsigned long ulNewLevel)
{
    unsigned long ul;

    ul = Win4InfoLevel;
    Win4InfoLevel = ulNewLevel;
    return(ul);
}


//+------------------------------------------------------------
// Function:    SetWin4InfoMask(unsigned long ulNewMask)
//
// Synopsis:    Sets the global info mask for debugging output
// Returns:     Old info mask
//
//-------------------------------------------------------------

EXPORTIMP unsigned long APINOT
SetWin4InfoMask(
    unsigned long ulNewMask)
{
    unsigned long ul;

    ul = Win4InfoMask;
    Win4InfoMask = ulNewMask;
    return(ul);
}


//+------------------------------------------------------------
// Function:    SetWin4AssertLevel(unsigned long ulNewLevel)
//
// Synopsis:    Sets the global assert level for debugging output
// Returns:     Old assert level
//
//-------------------------------------------------------------

EXPORTIMP unsigned long APINOT
SetWin4AssertLevel(
    unsigned long ulNewLevel)
{
    unsigned long ul;

    ul = Win4AssertLevel;
    Win4AssertLevel = ulNewLevel;
    return(ul);
}


//+------------------------------------------------------------
// Function:    PopUpError
//
// Synopsis:    Displays a dialog box using provided text,
//              and presents the user with the option to
//              continue or cancel.
//
// Arguments:
//      szMsg --        The string to display in main body of dialog
//      iLine --        Line number of file in error
//      szFile --       Filename of file in error
//
// Returns:
//      IDCANCEL --     User selected the CANCEL button
//      IDOK     --     User selected the OK button
//-------------------------------------------------------------

EXPORTIMP int APINOT
PopUpError(
    char const *szMsg,
    int iLine,
    char const *szFile)
{
    int id;
    static char szAssertCaption[128];
    static char szModuleName[128];

    DWORD tid = GetCurrentThreadId();
    DWORD pid = GetCurrentProcessId();
    char * pszModuleName;

    if (GetModuleFileNameA(NULL, szModuleName, 128))
    {
        pszModuleName = strrchr(szModuleName, '\\');
        if (!pszModuleName)
        {
            pszModuleName = szModuleName;
        }
        else
        {
            pszModuleName++;
        }
    }
    else
    {
        pszModuleName = "Unknown";
    }

    sprintf(szAssertCaption,"Process: %s File: %s line %u, thread id %d.%d",
        pszModuleName, szFile, iLine, pid, tid);

    id = MessageBoxA(NULL,
                     szMsg,
                     szAssertCaption,
                     MB_SETFOREGROUND
						| MB_DEFAULT_DESKTOP_ONLY
						| MB_TASKMODAL
						| MB_ICONEXCLAMATION
						| MB_OKCANCEL);

    //
    // If id == 0, then an error occurred.  There are two possibilities
    // that can cause the error:  Access Denied, which means that this
    // process does not have access to the default desktop, and everything
    // else (usually out of memory).
    //

    if (0 == id)
    {
        if (GetLastError() == ERROR_ACCESS_DENIED)
        {
            //
            // Retry this one with the SERVICE_NOTIFICATION flag on.  That
            // should get us to the right desktop.
            //
            id = MessageBoxA(NULL,
                             szMsg,
                             szAssertCaption,
                             MB_SETFOREGROUND
								| MB_SERVICE_NOTIFICATION
								| MB_TASKMODAL
								| MB_ICONEXCLAMATION
								| MB_OKCANCEL);
        }
    }

    return id;
}


//+------------------------------------------------------------
// Function:    vdprintf
//
// Synopsis:    Prints debug output using a pointer to the
//              variable information. Used primarily by the
//              xxDebugOut macros
//
// Arguements:
//      ulCompMask --   Component level mask used to determine
//                      output ability
//      pszComp    --   String const of component prefix.
//      ppszfmt    --   Pointer to output format and data
//
//-------------------------------------------------------------

static CRITICAL_SECTION s_csDebugPrint;

EXPORTIMP void APINOT
vdprintf(
    unsigned long ulCompMask,
    char const   *pszComp,
    char const   *ppszfmt,
    va_list       pargs)
{
    if ((ulCompMask & DEB_FORCE) == DEB_FORCE ||
        ((ulCompMask | Win4InfoLevel) & Win4InfoMask))
    {
		EnterCriticalSection(&s_csDebugPrint);

        DWORD tid = GetCurrentThreadId();
        DWORD pid = GetCurrentProcessId();
        if ((Win4InfoLevel & (DEB_DBGOUT | DEB_STDOUT)) != DEB_STDOUT)
        {
            if (! (ulCompMask & DEB_NOCOMPNAME))
            {
                w4dprintf("%d.%03d> %s: ", pid, tid, pszComp);
            }
            w4vdprintf(ppszfmt, pargs);
        }

        if (Win4InfoLevel & DEB_STDOUT)
        {
            if (! (ulCompMask & DEB_NOCOMPNAME))
            {
                printf("%d.%03d> %s: ", pid, tid, pszComp);
            }
            vprintf(ppszfmt, pargs);
        }

		LeaveCriticalSection(&s_csDebugPrint);
    }
}

void APINOT InitializeDebugging(void)
{
	InitializeCriticalSection(&s_csMessageBuf);
	InitializeCriticalSection(&s_csDebugPrint);
}

#endif // DBG == 1
