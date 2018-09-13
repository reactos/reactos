//+------------------------------------------------------------------------
//
//  Microsoft Trident
//  Copyright (C) Microsoft Corporation, 1998
//
//  File:       main.cxx
//
//  Contents:   WinMain and associated functions.
//
//  Created:    02/20/98    philco
//-------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_APP_HXX_
#define X_APP_HXX_
#include "app.hxx"
#endif

CHTMLApp theApp;

LPTSTR
GetCmdLine()
{
    #ifdef UNICODE
        LPTSTR pszCmdLine = GetCommandLine();
    #else
        // for multibyte should make it unsigned
        BYTE * pszCmdLine = (BYTE *)GetCommandLine();
    #endif

    if ( *pszCmdLine == TEXT('\"') ) {
        /*
         * Scan, and skip over, subsequent characters until
         * another double-quote or a null is encountered.
         */
        while ( *++pszCmdLine && (*pszCmdLine
             != TEXT('\"')) );
        /*
         * If we stopped on a double-quote (usual case), skip
         * over it.
         */
        if ( *pszCmdLine == TEXT('\"') )
            pszCmdLine++;
    }
    else {
        while (*pszCmdLine > TEXT(' '))
            pszCmdLine++;
    }

    /*
     * Skip past any white space preceeding the second token.
     */
    while (*pszCmdLine && (*pszCmdLine <= TEXT(' '))) {
        pszCmdLine++;
    }

    return (LPTSTR) pszCmdLine;
}

STDAPI
RunHTMLApplication(
        HINSTANCE hinst,
        HINSTANCE hPrevInst,
        LPSTR szCmdLine,
        int nCmdShow)
{
    HRESULT hr = S_OK;
    LPTSTR lpszCmdLine = GetCmdLine();

    hr = theApp.Init(hinst, lpszCmdLine, nCmdShow);
    if (hr)
        goto Cleanup;

    theApp.Run();

Cleanup:
    hr = theApp.Terminate();
    RRETURN(hr);
}
