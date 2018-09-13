///////////////////////////////////////////////////////////////////////////////
/*  File: debug.cpp

    Description: Contains debug output and assertion macros/functions.
        This file was originally copied from the shell's shelldll project
        and modified for the WIN32-only build enviroment of the setfldr 
        project.  Also cleaned it up by adding function headers.

        This code is compiled ONLY when the DEBUG macro is defined.


    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    08/06/96    Copied from shelldll project.                        BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
#include "precomp.hxx" // PCH
#pragma hdrstop

#define DEBUG_BREAK        _try { DebugBreak(); } _except (EXCEPTION_EXECUTE_HANDLER) {;}

#define WINCAPI __cdecl

#ifdef DEBUG

//========== Debug output routines =========================================

UINT dwDebugMask = DM_NONE;

///////////////////////////////////////////////////////////////////////////////
/*  Function: SetDebugMask

    Description: Sets the global variable dwDebugMask which controls 
        debug output and program assertions.

    Arguments:
        mask - See debug.h for mask bit values.

    Returns:
        Returns the previous mask value.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    08/06/96    Copied from shelldll project.                        BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
UINT WINAPI SetDebugMask(UINT mask)
{
    UINT wOld = dwDebugMask;
    dwDebugMask = mask;

    return wOld;
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: GetDebugMask

    Description: Retrieves the contents of the global variable dwDebugMask 
        which controls debug output and program assertions.

    Arguments: None.

    Returns:
        Returns the debug mask value.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    08/06/96    Copied from shelldll project.                        BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
UINT WINAPI GetDebugMask()
{
    return dwDebugMask;
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: AssertFailed

    Description: Called when an assertion fails.  Reports the assertion to the
        debugger terminal then terminates the application.

    Arguments:
        pszFile - Address of file name string identifying file where assertion
            occurred.  This is generally provided by the __FILE__ macro.
            
        line - Line number in file where assertion occurred.  This is generally
            provided by the __LINE__ macro.

    Returns: Nothing.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    08/06/96    Copied from shelldll project.                        BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
void WINAPI AssertFailed(LPCTSTR pszFile, int line)
{
    LPCTSTR psz;
    TCHAR ach[256];
    static TCHAR const szAssertFailed[] = TEXT("SETFLDR: assert %s, l %d\r\n");

    // Strip off path info from filename string, if present.
    //
    if (dwDebugMask & DM_ASSERT)
    {
        for (psz = pszFile + lstrlen(pszFile); psz != pszFile; psz=CharPrev(pszFile, psz))
        {
            if ((CharPrev(pszFile, psz)!= (psz-2)) && *(psz - 1) == TEXT('\\'))
                break;
        }
        wsprintf(ach, szAssertFailed, psz, line);
        OutputDebugString(ach);

        DEBUG_BREAK
    }
}

///////////////////////////////////////////////////////////////////////////////
/*  Function: _AssertMsg

    Description: I'm not sure what this is used for.

    Arguments:

    Returns:

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    08/06/96    Copied from shelldll project.                        BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
void WINCAPI _AssertMsg(BOOL f, LPCTSTR pszMsg, ...)
{
    TCHAR ach[256];

    if (!f && (dwDebugMask & DM_ASSERT))
    {
        va_list ArgList;

        va_start(ArgList, pszMsg);
        wvsprintf(ach, pszMsg, ArgList);
        va_end(ArgList);

        if (0 == (f & DM_NONEWLINE))
            lstrcat(ach, TEXT("\r\n"));

        OutputDebugString(ach);

        DEBUG_BREAK
    }
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: _DebugMsg

    Description: Writes a message to the debugger terminal.  Generation of
        output is controlled through the "mask" argument and the global 
        variable "dwDebugMask".  If the bit(s) set in "mask" are also set in 
        "dwDebugMask", then the message is output.  Otherwise, no output.

    Arguments:
        mask - Encoded with bits describing the condition in which the 
            message is to be output.  See debug.h for bit definitions.

        pszMsg - Message format string (printf-style).

        ... - Variable length argument list.

    Returns: Nothing.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    08/06/96    Copied from shelldll project.                        BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
void WINCAPI _DebugMsg(UINT mask, LPCTSTR pszMsg, ...)
{
    TCHAR ach[5*MAX_PATH+40];  // Handles 5*largest path + slop for message

    if (dwDebugMask & mask)
    {
        va_list ArgList;

        va_start(ArgList, pszMsg);
        __try {
            wvsprintf(ach, pszMsg, ArgList);
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            OutputDebugString(TEXT("SETFLDR: DebugMsg exception: "));
            OutputDebugString(pszMsg);
        }
        va_end(ArgList);
        if (0 == (mask & DM_NOPREFIX))
        {
            TCHAR szThreadID[32];
            wsprintf(szThreadID, TEXT("[%d]"), GetCurrentThreadId());
            OutputDebugString(szThreadID);
            OutputDebugString(TEXT("SETFLDR: "));
        }
        if (0 == (mask & DM_NONEWLINE))
            lstrcat(ach, TEXT("\r\n"));
        OutputDebugString(ach);
    }
}

#endif // DEBUG
