/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 1993-1994
*
*  TITLE:       REGDEBUG.C
*
*  VERSION:     4.0
*
*  AUTHOR:      Tracy Sharpe
*
*  DATE:        21 Nov 1993
*
*  Debug routines for the Registry Editor.
*
********************************************************************************
*
*  CHANGE LOG:
*
*  DATE        REV DESCRIPTION
*  ----------- --- -------------------------------------------------------------
*  21 Nov 1993 TCS Original implementation.
*
*******************************************************************************/

#include "pch.h"

#if (!defined(WINNT) && defined(DEBUG)) || (defined(WINNT) && DBG)

#define SIZE_DEBUG_BUFFER               100

/*******************************************************************************
*
*  _DbgPrintf
*
*  DESCRIPTION:
*     Simple implementation of the "debug printf" routine.  Takes the given
*     format string and argument list and outputs the formatted string to the
*     debugger.  Only available in debug builds-- use the DbgPrintf macro
*     defined in REGEDIT.H to access this service or to ignore the printf.
*
*  PARAMETERS:
*     lpFormatString, printf-style format string.
*     ..., variable argument list.
*
*******************************************************************************/

VOID
CDECL
_DbgPrintf(
    PSTR pFormatString,
    ...
    )
{

    va_list arglist;
    CHAR DebugBuffer[SIZE_DEBUG_BUFFER];

    va_start(arglist, pFormatString);

    wvsprintfA(DebugBuffer, pFormatString, arglist);

    OutputDebugStringA(DebugBuffer);
//    MessageBoxA(NULL, DebugBuffer, "RegEdit", MB_OK);

}

#endif
