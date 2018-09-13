// Copyright (c) 1996-1999 Microsoft Corporation

// --------------------------------------------------------------------------
//
//  BUTTON.CPP
//
//  This file has the implementation of the button client
//
//  BOGUS:  In theory, just override get_accRole() and get_accState().
//  In reality, have to also override other things, mainly for the Start
//  button. 
//
//  Implements:
//      get_accChildCount
//      get_accChild
//      get_accName
//      get_accRole
//      get_accState
//      get_accDefaultAction
//      get_accKeyboardShortcut
//      accNavigate
//      accDoDefaultAction
//      Next
//      Skip
//
// --------------------------------------------------------------------------

#include "oleacc_p.h"
#include "default.h"
#include "window.h"
#include "client.h"
#include "button.h" 
#include "menu.h"   // because start button has a child that is a menu.


// --------------------------------------------------------------------------
//
//  CreateButtonClient()
//
// --------------------------------------------------------------------------
HRESULT CreateButtonClient(HWND hwnd, long idChildCur, REFIID riid, void** ppvButtonC)
{
    CButton * pbutton;
    HRESULT hr;

    InitPv(ppvButtonC);

    pbutton = new CButton(hwnd, idChildCur);
    if (! pbutton)
        return(E_OUTOFMEMORY);

    hr = pbutton->QueryInterface(riid, ppvButtonC);
    if (!SUCCEEDED(hr))
        delete pbutton;

    return(hr);
}



// --------------------------------------------------------------------------
//
//  CButton::CButton()
//
// --------------------------------------------------------------------------
CButton::CButton(HWND hwnd, LONG idChildCur)
{
    Initialize(hwnd, idChildCur);
}


// --------------------------------------------------------------------------
//
//  SetupChildren()
//
// --------------------------------------------------------------------------
void CButton::SetupChildren(void)
{
    HWND hwndFocus;
    HWND hwndChild;

    if (!InTheShell(m_hwnd, SHELL_TRAY))
    {
        m_cChildren = 0;
        return;
    }

    // check to see if the start button has focus and a menu is shown. if so,
    // then there is one child 
    hwndFocus = MyGetFocus();
    if (m_hwnd == hwndFocus)
    {
        hwndChild = FindWindow (TEXT("#32768"),NULL);
        if (IsWindowVisible(hwndChild))
            m_cChildren = 1;
    }

}

// --------------------------------------------------------------------------
//
//  CButton::get_accName()
//
//  HACK for start button.
//
// --------------------------------------------------------------------------
STDMETHODIMP CButton::get_accName(VARIANT varChild, BSTR* pszName)
{
    InitPv(pszName);

    if (!ValidateChild(&varChild))
        return(E_INVALIDARG);

    if (!InTheShell(m_hwnd, SHELL_TRAY))
        return(CClient::get_accName(varChild, pszName));

    return(HrCreateString(STR_STARTBUTTON, pszName));
}


// --------------------------------------------------------------------------
//
//  CButton::get_accKeyboardShortcut()
//
//  HACK for start button
//
// --------------------------------------------------------------------------
STDMETHODIMP CButton::get_accKeyboardShortcut(VARIANT varChild, BSTR* pszShortcut)
{
    InitPv(pszShortcut);

    if (!ValidateChild(&varChild))
        return(E_INVALIDARG);

    if (!InTheShell(m_hwnd, SHELL_TRAY))
        return(CClient::get_accKeyboardShortcut(varChild, pszShortcut));

    return(HrCreateString(STR_STARTBUTTON_SHORTCUT, pszShortcut));
}

// --------------------------------------------------------------------------
//
//  CButton::get_accChildCount()
//
//  HACK for start button
//
// --------------------------------------------------------------------------
STDMETHODIMP CButton::get_accChildCount(long *pcCount)
{
    SetupChildren();
    *pcCount = m_cChildren;
    return(S_OK);
}

// --------------------------------------------------------------------------
//
//  CButton::get_accChild()
//
//  HACK for start button. If the menu is visible then we'll give that 
//  back, otherwise we'll just fall back on CClient
//
// --------------------------------------------------------------------------
STDMETHODIMP CButton::get_accChild(VARIANT varChild, IDispatch ** ppdispChild)
{
HWND    hwndChild;

    InitPv(ppdispChild);

    if (!InTheShell(m_hwnd, SHELL_TRAY))
        return(CClient::get_accChild(varChild,ppdispChild));

    SetupChildren();
    
    if (m_cChildren > 0)
    {
        hwndChild = FindWindow (TEXT("#32768"),NULL);
        if (IsWindowVisible(hwndChild))
        {
            return (CreateMenuPopupWindow (hwndChild,0L,IID_IDispatch,(void **)ppdispChild));
        }
    }

    return S_FALSE;
}

// --------------------------------------------------------------------------
//
//  CButton::accNavigate()
//
//  HACK for start button
//
// --------------------------------------------------------------------------
STDMETHODIMP CButton::accNavigate(long dwNavDir, VARIANT varStart, VARIANT * pvarEnd)
{
    HWND    hwndChild;
    HWND    hwndNext;

    InitPvar(pvarEnd);

    //
    // Validate--this accepts an HWND id.
    //
    if (!ValidateHwnd(&varStart) ||
        !ValidateNavDir(dwNavDir, varStart.lVal))
        return(E_INVALIDARG);

    if (!InTheShell(m_hwnd, SHELL_TRAY))
        return(CClient::accNavigate(dwNavDir,varStart,pvarEnd));

    // so this is only for the Start button.
    // We want to find the menu that is lowest in the z order
    SetupChildren();
    if ((m_cChildren > 0) && 
        (dwNavDir == NAVDIR_FIRSTCHILD || dwNavDir == NAVDIR_LASTCHILD))
    {
        hwndChild = FindWindow(TEXT("#32768"),NULL);
        if (!hwndChild)
            return(S_FALSE);

        for( ; ; )
        {
            hwndNext = FindWindowEx(NULL,hwndChild,TEXT("#32768"),NULL);
            if (hwndNext && IsWindowVisible(hwndNext))
                hwndChild = hwndNext;
            else
                break;
        }

        if (IsWindowVisible(hwndChild))
            return(GetWindowObject(hwndChild, pvarEnd));
    }

    return(CClient::accNavigate(dwNavDir,varStart,pvarEnd));
}

// --------------------------------------------------------------------------
//
//  CButton::get_accRole()
//
// --------------------------------------------------------------------------
STDMETHODIMP CButton::get_accRole(VARIANT varChild, VARIANT *pvarRole)
{
    long    lStyle;

    InitPvar(pvarRole);

    //
    // Validate parameters
    //
    if (! ValidateChild(&varChild))
        return(E_INVALIDARG);

    pvarRole->vt = VT_I4;

    //
    // Get window style
    //
    lStyle = GetWindowLong(m_hwnd, GWL_STYLE);
    switch (lStyle & BS_TYPEMASK)
    {
        default:
            pvarRole->lVal = ROLE_SYSTEM_PUSHBUTTON;
            break;

        case BS_CHECKBOX:
        case BS_AUTOCHECKBOX:
        case BS_3STATE:
        case BS_AUTO3STATE:
            pvarRole->lVal = ROLE_SYSTEM_CHECKBUTTON;
            break;

        case BS_RADIOBUTTON:
        case BS_AUTORADIOBUTTON:
            pvarRole->lVal = ROLE_SYSTEM_RADIOBUTTON;
            break;

        case BS_GROUPBOX:
            pvarRole->lVal = ROLE_SYSTEM_GROUPING;
            break;
    }

    return(S_OK);
}



// --------------------------------------------------------------------------
//
//  CButton::get_accState()
//
// --------------------------------------------------------------------------
STDMETHODIMP CButton::get_accState(VARIANT varChild, VARIANT *pvarState)
{
    LRESULT lResult;
    HRESULT hr;

    InitPvar(pvarState);

    //
    // Validate parameters && get window client state.
    //
    hr = CClient::get_accState(varChild, pvarState);
    if (!SUCCEEDED(hr))
        return(hr);
    
    Assert(pvarState->vt == VT_I4);

    lResult = SendMessage(m_hwnd, BM_GETSTATE, 0, 0);

    if (lResult & BST_PUSHED)
        pvarState->lVal |= STATE_SYSTEM_PRESSED;

    if (lResult & BST_CHECKED)
        pvarState->lVal |= STATE_SYSTEM_CHECKED;

    if (lResult & BST_INDETERMINATE)
        pvarState->lVal |= STATE_SYSTEM_MIXED;

    if ((GetWindowLong(m_hwnd, GWL_STYLE) & BS_TYPEMASK) == BS_DEFPUSHBUTTON)
        pvarState->lVal |= STATE_SYSTEM_DEFAULT;

    return(S_OK);
}



// --------------------------------------------------------------------------
//
//  CButton::get_accDefaultAction()
//
//  This is the button's name if it is a push button and not disabled.
//
// --------------------------------------------------------------------------
STDMETHODIMP CButton::get_accDefaultAction(VARIANT varChild, BSTR* pszDefAction)
{
    long    lStyle;

    InitPv(pszDefAction);

    //
    // Validate.
    //
    if (!ValidateChild(&varChild))
        return(E_INVALIDARG);

    lStyle = GetWindowLong(m_hwnd, GWL_STYLE);
    if (lStyle & WS_DISABLED)
        return(S_FALSE);

    switch (lStyle & BS_TYPEMASK)
    {
        case BS_PUSHBUTTON:
        case BS_DEFPUSHBUTTON:
        case BS_PUSHBOX:
        case BS_OWNERDRAW:
        case BS_USERBUTTON:
            // Pushing a push button is the default
            return(HrCreateString(STR_BUTTON_PUSH, pszDefAction));

        case BS_CHECKBOX:
        case BS_AUTOCHECKBOX:
            // Toggling a checkbox is the default
            if (SendMessage(m_hwnd, BM_GETSTATE, 0, 0) & BST_CHECKED)
                return(HrCreateString(STR_BUTTON_UNCHECK, pszDefAction));
            else
                return(HrCreateString(STR_BUTTON_CHECK, pszDefAction));
            break;

        case BS_RADIOBUTTON:
        case BS_AUTORADIOBUTTON:
            // Checking a radio button is the default
            return(HrCreateString(STR_BUTTON_CHECK, pszDefAction));

        case BS_3STATE:
        case BS_AUTO3STATE:
            switch (SendMessage(m_hwnd, BM_GETCHECK, 0, 0))
            {
                case 0:
                    return(HrCreateString(STR_BUTTON_CHECK, pszDefAction));

                case 1:
                    return(HrCreateString(STR_BUTTON_HALFCHECK, pszDefAction));

                default:
                    return(HrCreateString(STR_BUTTON_UNCHECK, pszDefAction));
            }
            break;

    }

    return(E_NOT_APPLICABLE);
}



// --------------------------------------------------------------------------
//
//  CButton::accDoDefaultAction()
//
// --------------------------------------------------------------------------
STDMETHODIMP CButton::accDoDefaultAction(VARIANT varChild)
{
    long    lStyle;

    //
    // Validate
    //
    if (!ValidateChild(&varChild))
        return(E_INVALIDARG);

    lStyle = GetWindowLong(m_hwnd, GWL_STYLE);
    if (lStyle & WS_DISABLED)
        return(S_FALSE);

    switch (lStyle & BS_TYPEMASK)
    {
        case BS_PUSHBUTTON:
        case BS_DEFPUSHBUTTON:
            if (InTheShell(m_hwnd, SHELL_TRAY))
            {
                //
                // You can't just click the start button; it won't do
                // anything if the tray isn't active except take focus
                //
                PostMessage(m_hwnd, WM_SYSCOMMAND, SC_TASKLIST, 0L);
                break;
            }
            // FALL THRU

        case BS_PUSHBOX:
        case BS_OWNERDRAW:
        case BS_USERBUTTON:
        case BS_CHECKBOX:
        case BS_AUTOCHECKBOX:
        case BS_RADIOBUTTON:
        case BS_AUTORADIOBUTTON:
        case BS_3STATE:
        case BS_AUTO3STATE:
            PostMessage(m_hwnd, BM_CLICK, 0, 0L);
            return(S_OK);
    }

    return(E_NOT_APPLICABLE);
}

// --------------------------------------------------------------------------
//
//  CButton::Next()
//
//
// --------------------------------------------------------------------------
STDMETHODIMP CButton::Next(ULONG celt, VARIANT *rgvar, ULONG* pceltFetched)
{
    HWND    hwndChild;
    VARIANT* pvar;
    long    cFetched;
    HRESULT hr;

    if (!InTheShell(m_hwnd, SHELL_TRAY))
        return(CClient::Next(celt,rgvar,pceltFetched));

    // Can be NULL
    if (pceltFetched)
        *pceltFetched = 0;

    pvar = rgvar;
    cFetched = 0;
    SetupChildren();

    // we only ever have 1 child
    if (m_idChildCur > 1)
        return (S_FALSE);

    // we only have one child if we have the focus and the menu
    // is visible
    hwndChild = FindWindow(TEXT("#32768"),NULL);
    if (!hwndChild)
        return(S_FALSE);

    if (IsWindowVisible(hwndChild))
    {
        hr = GetWindowObject(hwndChild, pvar);
        if (SUCCEEDED(hr))
        {
            ++pvar;
            ++cFetched;
        }
    }

    //
    // Advance the current position
    //
    m_idChildCur = 1;

    //
    // Fill in the number fetched
    //
    if (pceltFetched)
        *pceltFetched += cFetched;

    //
    // Return S_FALSE if we grabbed fewer items than requested
    //
    return((cFetched < (long)celt) ? S_FALSE : S_OK);
}



// --------------------------------------------------------------------------
//
//  CButton::Skip()
//
// --------------------------------------------------------------------------
STDMETHODIMP CButton::Skip(ULONG celt)
{
    if (!InTheShell (m_hwnd,SHELL_TRAY))
        return (CClient::Skip(celt));

    return (CAccessible::Skip(celt));
}

