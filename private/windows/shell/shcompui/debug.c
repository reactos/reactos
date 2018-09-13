///////////////////////////////////////////////////////////////////////////////
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995.
//
//  FILE: DEBUG.C
//
//  DESCRIPTION:
//
//    Debug support for SHCOMPUI.DLL.
//
//
//    REVISIONS:
//
//    Date       Description                                         Programmer
//    ---------- --------------------------------------------------- ----------
//    09/15/95   Initial creation.                                   brianau
//
///////////////////////////////////////////////////////////////////////////////

#if defined(DEBUG) || defined(DBG)

#include "debug.h"

#ifdef WIN32
#define DEBUG_BREAK        _try { DebugBreak(); } _except (EXCEPTION_EXECUTE_HANDLER) {;}
#else
#define DEBUG_BREAK        _asm { int 3 }
#endif


///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION: DbgOut
//
// DESCRIPTION:
//
//    Display a message string on the debugger output terminal.
//    The function accepts a variable length printf-style arg list.
//    A terminating newline is appended to the message string.
//
// ARGUMENTS:
//
//    fmt
//       printf-style format string.
//
//    ...
//       Variable-length arg list.
//
// RETURNS:
//
//    Nothing.
//
///////////////////////////////////////////////////////////////////////////////
void DbgOut(LPCTSTR fmt, ...)
{
   TCHAR szBuf[512];

   va_list args;
   va_start(args, fmt);

   ASSERT(NULL != fmt);

   wvsprintf(szBuf, fmt, args);
   lstrcat(szBuf, __TEXT("\r\n"));

   va_end(args);
   OutputDebugString(szBuf);
}



void WINAPI AssertFailed(LPCTSTR pszFile, int line)
{
    LPCTSTR psz;
    TCHAR ach[256];
    static TCHAR szAssertFailed[] = __TEXT("SHCOMPUI: assert %s, line %d\r\n");

    // Strip off path info from filename string, if present.
    //
    for (psz = pszFile + lstrlen(pszFile); psz != pszFile; psz=CharPrev(pszFile, psz))
    {
        if ((CharPrev(pszFile, psz)!= (psz-2)) && *(psz - 1) == TEXT('\\'))
            break;
    }
    DbgOut(szAssertFailed, psz, line);

    DEBUG_BREAK
}



#endif  // #ifdef DBG

