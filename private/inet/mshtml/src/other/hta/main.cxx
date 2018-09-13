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

typedef HRESULT STDAPICALLTYPE RUNHTMLAPPLICATIONFN(
        HINSTANCE hinst,
        HINSTANCE hPrevInst,
        LPSTR szCmdLine,
        int nCmdShow);

// Simply a pass-through entry point.  It forwards this call to MSHTML's 
// exported RunHTMLApplication API.
EXTERN_C int PASCAL
WinMain(
        HINSTANCE hinst,
        HINSTANCE hPrevInst,
        LPSTR szCmdLine,
        int nCmdShow)
{
    int nRet = 0;
    HINSTANCE hinstMSHTML = NULL;
    RUNHTMLAPPLICATIONFN *lpfnRunHTMLApp = NULL;
        
    hinstMSHTML = LoadLibraryA("mshtml.dll");
    if (hinstMSHTML)
    {
        lpfnRunHTMLApp = (RUNHTMLAPPLICATIONFN *) GetProcAddress(hinstMSHTML, "RunHTMLApplication");
        if (lpfnRunHTMLApp)
        {
            (*lpfnRunHTMLApp)(hinst, hPrevInst, szCmdLine, nCmdShow);
        }
        FreeLibrary(hinstMSHTML);
    }
    
    return nRet;
}
