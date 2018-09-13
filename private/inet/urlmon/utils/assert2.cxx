//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       assert.cxx
//
//  Functions:  FnAssert
//              DbgDllSetSiftObject
//
//  History:     4-Jan-94   CraigWi     Created
//              16-Jun-94   t-ChriPi    Added DbgDllSetSiftObject
//
//----------------------------------------------------------------------------
#ifdef unix
#include <windows.h>
#endif /* unix */
#include <ole2.h>


STDAPI_(void) Win4AssertEx(
    char const * szFile,
    int iLine,
    char const * szMessage);

//+-------------------------------------------------------------------
//
//  Function:   FnAssert, public
//
//  Synopsis:   Prints a message and optionally stops the program
//
//  Effects:    Simply maps to Win4AssertEx for now.
//
//  History:     4-Jan-94   CraigWi     Created for Win32 OLE2.
//
//--------------------------------------------------------------------

STDAPI FnAssert( LPSTR lpstrExpr, LPSTR lpstrMsg, LPSTR lpstrFileName, UINT iLine )
{
#if DBG == 1
    char szMessage[1024];

    if (lpstrMsg == NULL)
        lstrcpyA(szMessage, lpstrExpr);
    else
        wsprintfA(szMessage, "%s; %s", lpstrExpr, lpstrMsg);

    Win4AssertEx(lpstrFileName, iLine, szMessage);
#endif
    return NOERROR;
}


