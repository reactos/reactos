/*----------------------------------------------------------------------------
/ Title;
/   misc.cpp
/
/ Authors;
/   David De Vorchik (daviddv)
/
/ Notes;
/   Helper functions to help migration of MFC code
/----------------------------------------------------------------------------*/
#include "pch.h"
#pragma hdrstop


/*-----------------------------------------------------------------------------
/ __TRACE
/ -------
/   Helper function to perform formatting and then output debug string
/   given a printf format string and some arguments
/
/ In:
/   pFormat -> formatting information
/   ... = arguments
/
/ Out:
/   HRESULT
/----------------------------------------------------------------------------*/
#if DSUI_DEBUG || defined(_DEBUG)
void __TRACE(LPTSTR pFormat, ...)
{
    TCHAR szBuffer[256];
    va_list va;

    va_start(va, pFormat);
    wvsprintf(szBuffer, pFormat, va);
    va_end(va);
    
    lstrcat(szBuffer, TEXT("\n"));

    OutputDebugString(szBuffer);
}
#endif