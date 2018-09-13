// Copyright (c) 1996-1999 Microsoft Corporation

// --------------------------------------------------------------------------
//
//  DESKTOP.CPP
//
//  Desktop class.
//
// --------------------------------------------------------------------------

#include "oleacc_p.h"
#include "default.h"
#include "client.h"
#include "desktop.h"




// --------------------------------------------------------------------------
//
//  CreateDesktopClient()
//
//  EXTERNAL for CreateClientObject()
//
// --------------------------------------------------------------------------
HRESULT CreateDesktopClient(HWND hwnd, long idChildCur, REFIID riid, void** ppvDesktop)
{
    CDesktop* pdesktop;
    HRESULT   hr;

    InitPv(ppvDesktop);

    pdesktop = new CDesktop(hwnd, idChildCur);
    if (! pdesktop)
        return(E_OUTOFMEMORY);

    hr = pdesktop->QueryInterface(riid, ppvDesktop);
    if (!SUCCEEDED(hr))
        delete pdesktop;

    return(hr);
}



// --------------------------------------------------------------------------
//
//  CDesktop::CDesktop()
//
// --------------------------------------------------------------------------
CDesktop::CDesktop(HWND hwnd, long idChildCur)
{
    Initialize(hwnd, idChildCur);
}



// --------------------------------------------------------------------------
//
//  CDesktop::get_accName()
//
// --------------------------------------------------------------------------
STDMETHODIMP CDesktop::get_accName(VARIANT varChild, BSTR* pszName)
{
    InitPv(pszName);

    if (! ValidateChild(&varChild))
        return(E_INVALIDARG);

    return(HrCreateString(STR_DESKTOP_NAME, pszName));
}



// --------------------------------------------------------------------------
//
//  CDesktop::get_accFocus()
//
// --------------------------------------------------------------------------
STDMETHODIMP CDesktop::get_accFocus(VARIANT* pvarFocus)
{
    return(get_accSelection(pvarFocus));
}



// --------------------------------------------------------------------------
//
//  CDesktop::get_accSelection()
//
// --------------------------------------------------------------------------
STDMETHODIMP CDesktop::get_accSelection(VARIANT* pvar)
{
    HWND    hwnd;

    InitPvar(pvar);

    hwnd = GetForegroundWindow();
    if (! hwnd)
        return(S_FALSE);

    return(GetWindowObject(hwnd, pvar));
}

