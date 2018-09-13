//+---------------------------------------------------------------------
//
//  Copyright (C) Microsoft Corporation, 1998.
//
//   File:      server.cxx
//
//  Contents:   Server App object implementation
//
//  Classes:    CServerApp
//
//  History:   03-Sep-98   tomfakes  Created
//------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_MSHTMSRV_HXX_
#define X_MSHTMSRV_HXX_
#include "mshtmsrv.hxx"
#endif



//+----------------------------------------------------------------------------
//
// Method: constructor
//
//+----------------------------------------------------------------------------
CServerApp::CServerApp() :
    _lRef(1),
    _hTrident(NULL),
    _TridentNormalizeUA(NULL),
    _TridentGetDLText(NULL)
{
}


//+----------------------------------------------------------------------------
//
// Method: destructor
//
//+----------------------------------------------------------------------------
CServerApp::~CServerApp()
{
    Assert(0 == _lRef);
}


//+----------------------------------------------------------------------------
//
// Method: ReleaseApp
//
//+----------------------------------------------------------------------------
LONG
CServerApp::ReleaseApp()
{
    LONG        lRef = InterlockedDecrement(&_lRef);

    if (!lRef)
    {
        delete g_pApp;
        g_pApp = NULL;
    }

    return _lRef;
}

//+----------------------------------------------------------------------------
//
// Method: AddRefApp
//
//+----------------------------------------------------------------------------
HRESULT
CServerApp::AddRefApp()
{
    HRESULT     hr = S_OK;

    if (g_pApp)
    {
        InterlockedIncrement(&(g_pApp->_lRef));
        goto Cleanup;
    }

    g_pApp = new CServerApp;
    if (NULL == g_pApp)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    hr = g_pApp->Initialize();

Cleanup:
    RRETURN(hr);
}


//+----------------------------------------------------------------------------
//
// Method: Initialize
//
//+----------------------------------------------------------------------------
HRESULT
CServerApp::Initialize(void)
{
    HRESULT     hr = E_NOTIMPL;

    _hTrident = LoadLibraryEx(_T("MSHTML.DLL"), NULL, 0);
    if (_hTrident == NULL) 
        goto Cleanup;

    _TridentNormalizeUA = (TridentNormalizeUACall)GetProcAddress(_hTrident, "SvrTri_NormalizeUA");
    _TridentGetDLText = (TridentGetDLTextCall)GetProcAddress(_hTrident, "SvrTri_GetDLText");

    if (_TridentNormalizeUA == NULL || _TridentGetDLText == NULL) 
        goto Cleanup;

    // BUGBUG (tomfakes) Get this url from somewhere else
    lstrcpyA(_szExtensionUrl, "/Trident/mshtmsrv.dll?u=");

    hr = S_OK;

Cleanup:
    RRETURN(hr);
}


//+----------------------------------------------------------------------------
//
// Method: IsHTMLCached
//
//+----------------------------------------------------------------------------
BOOL
CServerApp::IsHTMLCached(LPSTR pszUrl)
{
    return FALSE;
}
