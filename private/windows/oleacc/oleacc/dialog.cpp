// Copyright (c) 1996-1999 Microsoft Corporation

// --------------------------------------------------------------------------
//
//  DIALOG.CPP
//
// --------------------------------------------------------------------------

#include "oleacc_p.h"
#include "default.h"
#include "window.h"
#include "client.h"
#include "dialog.h"




// --------------------------------------------------------------------------
//
//  CreateDialogClient()
//
//  EXTERNAL function for CreateClientObject()
//
// --------------------------------------------------------------------------
HRESULT CreateDialogClient(HWND hwnd, long idChildCur, REFIID riid, void** ppvObject)
{
    CDialog * pdialog;
    HRESULT hr;

    InitPv(ppvObject);

    pdialog = new CDialog(hwnd, idChildCur);
    if (!pdialog)
        return(E_OUTOFMEMORY);

    hr = pdialog->QueryInterface(riid, ppvObject);
    if (!SUCCEEDED(hr))
        delete pdialog;

    return(hr);
}



// --------------------------------------------------------------------------
//
//  CDialog::CDialog()
//
// --------------------------------------------------------------------------
CDialog::CDialog(HWND hwnd, long idChildCur)
{
    Initialize(hwnd, idChildCur);
}



// --------------------------------------------------------------------------
//
//  CDialog::get_accRole()
//
//  Currently does NOT accept child IDs
//
// --------------------------------------------------------------------------
STDMETHODIMP CDialog::get_accRole(VARIANT varChild, VARIANT* pvarRole)
{
    long    lStyle;

    InitPvar(pvarRole);

    if (!ValidateChild(&varChild))
        return(E_INVALIDARG);

    pvarRole->vt = VT_I4;

    lStyle = GetWindowLong(m_hwnd, GWL_STYLE);
    if ((lStyle & WS_CHILD) && (lStyle & DS_CONTROL))
        pvarRole->lVal = ROLE_SYSTEM_PROPERTYPAGE;
    else
        pvarRole->lVal = ROLE_SYSTEM_DIALOG;

    return(S_OK);
}



// --------------------------------------------------------------------------
//
//  CDialog::get_accDefaultAction()
//
//  The default action is the name of the default push button.
//
// --------------------------------------------------------------------------
STDMETHODIMP CDialog::get_accDefaultAction(VARIANT varChild, BSTR* pszDefAction)
{
    HRESULT hr;
    long    idDef;
    HWND    hwndDef;
    IAccessible * poleacc;

    InitPv(pszDefAction);

    //
    // Validate--use ValidateChild so only 0 is allowed.
    //
    if (!ValidateChild(&varChild))
        return(E_INVALIDARG);

    //
    // Get the default ID
    //
    idDef = SendMessageINT(m_hwnd, DM_GETDEFID, 0, 0);
    if (HIWORD(idDef) == DC_HASDEFID)
        idDef &= 0x0000FFFF;
    else
        idDef = IDOK;

    //
    // Get the item with this ID
    //
    hwndDef = GetDlgItem(m_hwnd, idDef);
    if (!hwndDef)
        return(S_FALSE);

    //
    // Get this thing's name.
    //
    poleacc = NULL;
    hr = AccessibleObjectFromWindow(hwndDef, OBJID_CLIENT, IID_IAccessible,
        (void**)&poleacc);
    if (!SUCCEEDED(hr))
        return(hr);

    //
    // varChild is empty of course
    //
    hr = poleacc->get_accName(varChild, pszDefAction);
    poleacc->Release();

    return(hr);
}



// --------------------------------------------------------------------------
//
//  CDialog::accDoDefaultAction()
//
// --------------------------------------------------------------------------
STDMETHODIMP CDialog::accDoDefaultAction(VARIANT varChild)
{
    HRESULT hr;
    long    idDef;
    HWND    hwndDef;
    IAccessible * poleacc;

    //
    // Validate 
    //
    if (!ValidateChild(&varChild))
        return(E_INVALIDARG);

    //
    // Get the default ID
    //
    idDef = SendMessageINT(m_hwnd, DM_GETDEFID, 0, 0);
    if (HIWORD(idDef) == DC_HASDEFID)
        idDef &= 0x0000FFFF;
    else
        idDef = IDOK;

    //
    // Get the child with this ID
    //
    hwndDef = GetDlgItem(m_hwnd, idDef);
    if (!hwndDef)
        return(S_FALSE);

    //
    // Ask the child to do its default action.  Yes, we could send a 
    // WM_COMMAND message directly, but this lets non-push buttons hook
    // into the action.
    //
    poleacc = NULL;
    hr = AccessibleObjectFromWindow(hwndDef, OBJID_CLIENT, IID_IAccessible,
        (void**)&poleacc);
    if (!SUCCEEDED(hr))
        return(hr);

    hr = poleacc->accDoDefaultAction(varChild);
    poleacc->Release();

    return(hr);
}


