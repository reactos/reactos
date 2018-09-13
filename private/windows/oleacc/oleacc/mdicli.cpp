// Copyright (c) 1996-1999 Microsoft Corporation

// --------------------------------------------------------------------------
//
//  MDICLI.CPP
//
//  MDI Client class.
//
// --------------------------------------------------------------------------

#include "oleacc_p.h"
#include "default.h"
#include "window.h"
#include "client.h"
#include "mdicli.h"



// --------------------------------------------------------------------------
//
//  CreateMDIClient()
//
//  EXTERNAL for CreateClientObject()
//
// --------------------------------------------------------------------------
HRESULT CreateMDIClient(HWND hwnd, long idChildCur, REFIID riid, void** ppvMdi)
{
    CMdiClient * pmdicli;
    HRESULT hr;

    InitPv(ppvMdi);

    pmdicli = new CMdiClient(hwnd, idChildCur);
    if (!pmdicli)
        return(E_OUTOFMEMORY);

    hr = pmdicli->QueryInterface(riid, ppvMdi);
    if (!SUCCEEDED(hr))
        delete pmdicli;

    return(hr);
}



// --------------------------------------------------------------------------
//
//  CMdiClient::CMdiClient()
//
// --------------------------------------------------------------------------
CMdiClient::CMdiClient(HWND hwndSelf, long idChild)
{
    Initialize(hwndSelf, idChild);
}



// --------------------------------------------------------------------------
//
//  CMdiClient::get_accName()
//
// --------------------------------------------------------------------------
STDMETHODIMP CMdiClient::get_accName(VARIANT varChild, BSTR* pszName)
{
    InitPv(pszName);

    //
    // Validate -- does NOT accept child IDs (yet)
    //
    if (! ValidateChild(&varChild))
        return(E_INVALIDARG);

    return(HrCreateString(STR_MDICLI_NAME, pszName));
}



// --------------------------------------------------------------------------
//
//  CMdiClient::get_accFocus()
//
//  Both the focus and the selection return back the "active" mdi child.
//
// --------------------------------------------------------------------------
STDMETHODIMP CMdiClient::get_accFocus(VARIANT* pvarFocus)
{
    return(get_accSelection(pvarFocus));
}



// --------------------------------------------------------------------------
//
//  CMdiClient::get_accSelection()
//
//  Both the focus and the selection return back the "active" mdi child.
//
// --------------------------------------------------------------------------
STDMETHODIMP CMdiClient::get_accSelection(VARIANT* pvarSel)
{
    HWND    hwndChild;

    InitPvar(pvarSel);

    hwndChild = (HWND)SendMessage(m_hwnd, WM_MDIGETACTIVE, 0, 0);
    if (!hwndChild)
        return(S_FALSE);

    return(GetWindowObject(hwndChild, pvarSel));
}


