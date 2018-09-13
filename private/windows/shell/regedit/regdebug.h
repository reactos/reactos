/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 1993-1994
*
*  TITLE:       REGDEBUG.H
*
*  VERSION:     4.01
*
*  AUTHOR:      Tracy Sharpe
*
*  DATE:        05 Mar 1994
*
*  Debug routines for the Registry Editor.
*
********************************************************************************
*
*  CHANGE LOG:
*
*  DATE        REV DESCRIPTION
*  ----------- --- -------------------------------------------------------------
*  05 Mar 1994 TCS Original implementation.
*
*******************************************************************************/

#ifndef _INC_REGDEBUG
#define _INC_REGDEBUG

#if (!defined(WINNT) && defined(DEBUG)) || (defined(WINNT) && DBG)

VOID
CDECL
_DbgPrintf(
    PSTR pFormatString,
    ...
    );

#define DbgPrintf(x)                    _DbgPrintf ##x

#else

#define DbgPrintf(x)

#endif

#endif // _INC_REGDEBUG
