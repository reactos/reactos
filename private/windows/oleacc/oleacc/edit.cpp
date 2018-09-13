// Copyright (c) 1996-1999 Microsoft Corporation

// --------------------------------------------------------------------------
//
//  EDIT.CPP
//
//  BOGUS!  This should support ITextDocument or something
//
// --------------------------------------------------------------------------

#include "oleacc_p.h"
#include "default.h"
#include "window.h"
#include "client.h"
#include "edit.h"



// --------------------------------------------------------------------------
//
//  CreateEditClient()
//
// --------------------------------------------------------------------------
HRESULT CreateEditClient(HWND hwnd, long idChildCur, REFIID riid, void** ppvEdit)
{
    CEdit * pedit;
    HRESULT hr;

    InitPv(ppvEdit);

    pedit = new CEdit(hwnd, idChildCur);
    if (!pedit)
        return(E_OUTOFMEMORY);

    hr = pedit->QueryInterface(riid, ppvEdit);
    if (!SUCCEEDED(hr))
        delete pedit;

    return(hr);
}



// --------------------------------------------------------------------------
//
//  CEdit::CEdit()
//
// --------------------------------------------------------------------------
CEdit::CEdit(HWND hwnd, long idChildCur)
{
    Initialize(hwnd, idChildCur);
    m_fUseLabel = TRUE;
}



// --------------------------------------------------------------------------
//
//  CEdit::get_accName()
//
// --------------------------------------------------------------------------
STDMETHODIMP CEdit::get_accName(VARIANT varChild, BSTR* pszName)
{
    InitPv(pszName);

    //
    // Validate parameters
    //
    if (! ValidateChild(&varChild))
        return(E_INVALIDARG);

    // Is this edit in a combo? If so, use the combo's name (which it gets
    // from its label) as our own.

    // Using CompareWindowClasses is safer than checking the ES_COMBOBOX style bit,
    // since that bit is not used when the edit is in a combo in a comboex32.
    // was:   if (GetWindowLong(m_hwnd, GWL_STYLE) & ES_COMBOBOX)
    HWND hwndParent = MyGetAncestor(m_hwnd, GA_PARENT);
    if( hwndParent && CompareWindowClass( hwndParent, CreateComboClient ) )
    {
        IAccessible* pacc = NULL;
        HRESULT hr = AccessibleObjectFromWindow( hwndParent,
                    OBJID_CLIENT, IID_IAccessible, (void**)&pacc );
        if( ! SUCCEEDED( hr ) || ! pacc )
            return S_FALSE;

        VariantInit(&varChild);
        varChild.vt = VT_I4;
        varChild.lVal = CHILDID_SELF;
        hr = pacc->get_accName(varChild, pszName);
        pacc->Release();

        return hr;
    }
    else
        return(CClient::get_accName(varChild, pszName));
}



// --------------------------------------------------------------------------
//
//  CEdit::get_accValue()
//
//  Gets the text contents.
//
// --------------------------------------------------------------------------
STDMETHODIMP CEdit::get_accValue(VARIANT varChild, BSTR* pszValue)
{
    LPTSTR  lpszValue;
    DWORD   dwStyle;

    InitPv(pszValue);

    //
    // Validate parameters
    //
    if (! ValidateChild(&varChild))
        return(E_INVALIDARG);

    // if this is a password edit control, return a NULL pointer
    dwStyle = GetWindowLong (m_hwnd,GWL_STYLE);
    if (dwStyle & ES_PASSWORD)
    {
        return (E_ACCESSDENIED);
    }


    lpszValue = GetTextString(m_hwnd, TRUE);
    if (!lpszValue)
        return(S_FALSE);


    *pszValue = TCharSysAllocString(lpszValue);
    LocalFree((HANDLE)lpszValue);

    if (! *pszValue)
        return(E_OUTOFMEMORY);

    return(S_OK);
}



// --------------------------------------------------------------------------
//
//  CEdit::get_accRole()
//
// --------------------------------------------------------------------------
STDMETHODIMP CEdit::get_accRole(VARIANT varChild, VARIANT *pvarRole)
{
    InitPvar(pvarRole);

    //
    // Validate
    //
    if (! ValidateChild(&varChild))
        return(E_INVALIDARG);

    pvarRole->vt = VT_I4;
    pvarRole->lVal = ROLE_SYSTEM_TEXT;

    return(S_OK);
}




// --------------------------------------------------------------------------
//
//  CEdit::get_accState()
//
// --------------------------------------------------------------------------
STDMETHODIMP CEdit::get_accState(VARIANT varChild, VARIANT* pvarState)
{
    HRESULT hr;
    LONG    lStyle;

    // 
    // Get default client state
    //
    hr = CClient::get_accState(varChild, pvarState);
    if (!SUCCEEDED(hr))
        return(hr);

    //
    // Add on extra styles for edit field
    //
    Assert(pvarState->vt == VT_I4);

    lStyle = GetWindowLong(m_hwnd, GWL_STYLE);
    if (lStyle & ES_READONLY)
        pvarState->lVal |= STATE_SYSTEM_READONLY;
    if (lStyle & ES_PASSWORD)
        pvarState->lVal |= STATE_SYSTEM_PROTECTED;

    return(S_OK);
}



// --------------------------------------------------------------------------
//
//  CEdit::get_accKeyboardShortcut()
//
// --------------------------------------------------------------------------
STDMETHODIMP CEdit::get_accKeyboardShortcut(VARIANT varChild, BSTR* pszShortcut)
{
    InitPv(pszShortcut);

    //
    // Validate parameters
    //
    if (! ValidateChild(&varChild))
        return(E_INVALIDARG);

    // If in a combo, use its shortcut key...
    HWND hwndParent = MyGetAncestor(m_hwnd, GA_PARENT);
    if( hwndParent && CompareWindowClass( hwndParent, CreateComboClient ) )
    {
        IAccessible* pacc = NULL;
        HRESULT hr = AccessibleObjectFromWindow( hwndParent,
                    OBJID_CLIENT, IID_IAccessible, (void**)&pacc );
        if( ! SUCCEEDED( hr ) || ! pacc )
            return S_FALSE;

        VariantInit(&varChild);
        varChild.vt = VT_I4;
        varChild.lVal = CHILDID_SELF;
        hr = pacc->get_accKeyboardShortcut(varChild, pszShortcut);
        pacc->Release();

        return hr;
    }
    else
        return(CClient::get_accKeyboardShortcut(varChild, pszShortcut));
}


// --------------------------------------------------------------------------
//
//  CEdit::put_accValue()
//
// --------------------------------------------------------------------------
STDMETHODIMP CEdit::put_accValue(VARIANT varChild, BSTR szValue)
{
    // Validate parameters
    //
    if (! ValidateChild(&varChild))
        return(E_INVALIDARG);

    LPTSTR  lpszValue;

#ifdef UNICODE

	// If unicode, use the BSTR directly...
	lpszValue = szValue;

#else

	// If not UNICODE, allocate a temp string and convert to multibyte...

    // We may be dealing with DBCS chars - assume worst case where every character is
    // two bytes...
    UINT cchValue = SysStringLen(szValue) * 2;
    lpszValue = (LPTSTR)LocalAlloc(LPTR, (cchValue+1)*sizeof(TCHAR));
    if (!lpszValue)
        return(E_OUTOFMEMORY);

    WideCharToMultiByte(CP_ACP, 0, szValue, -1, lpszValue, cchValue+1, NULL,
        NULL);

#endif


    SendMessage(m_hwnd, WM_SETTEXT, 0, (LPARAM)lpszValue);

#ifndef UNICODE

	// If non-unicode, free the temp string we allocated above
    LocalFree((HANDLE)lpszValue);

#endif

    return(S_OK);
}
