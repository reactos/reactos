// Copyright (c) 1996-1999 Microsoft Corporation

// --------------------------------------------------------------------------
//
//  COMBO.CPP
//
// --------------------------------------------------------------------------

#include "oleacc_p.h"
#include <olectl.h>
#include "default.h"
#include "window.h"
#include "client.h"
#include "combo.h"


STDAPI_(LPTSTR) MyPathFindFileName(LPCTSTR pPath); // in listbox.cpp

HWND IsInComboEx(HWND hwnd); // in listbox.cpp
BOOL IsTridentControl( HWND hWnd, BOOL fCombo, BOOL fComboList ); // inlistbox.cpp

// Variation of HrGetWindowName which never uses a label
// (unlike the original HrGetWindowName which always uses a label if
// the text is an empty string - using label then is not approprite for
// combo value field.)
// Implemented near end of this file. Original is in client.cpp.
HRESULT HrGetWindowNameNoLabel(HWND hwnd, BSTR* pszName);


// --------------------------------------------------------------------------
//
//  CreateComboClient()
//
// --------------------------------------------------------------------------
HRESULT CreateComboClient(HWND hwnd, long idChildCur, REFIID riid, void** ppvCombo)
{
    CCombo * pcombo;
    HRESULT hr;

    InitPv(ppvCombo);
    
    pcombo = new CCombo(hwnd, idChildCur);
    if (!pcombo)
        return(E_OUTOFMEMORY);

    hr = pcombo->QueryInterface(riid, ppvCombo);
    if (!SUCCEEDED(hr))
        delete pcombo;

    return(hr);
}



// --------------------------------------------------------------------------
//
//  CCombo::CCombo()
//
// --------------------------------------------------------------------------
CCombo::CCombo(HWND hwnd, long idChildCur)
{
    LONG    lStyle;

    Initialize(hwnd, idChildCur);

    m_cChildren = CCHILDREN_COMBOBOX;
    m_fUseLabel = TRUE;

    // If in a ComboEx, use its style, instead of our own.
    // Important, because the real Combo will be DROPDOWNLIST (doesn't
    // have edit) when the ComboEx is DROPDOWN (has edit) - the ComboEx
    // supplies an EDIT, but the Combo doesn't know about it.
    HWND hWndEx = IsInComboEx(hwnd);
    if (hWndEx)
    {
        lStyle = GetWindowLong(hWndEx, GWL_STYLE);
    }
    else
    {
        lStyle = GetWindowLong(hwnd, GWL_STYLE);
    }

    switch (lStyle & CBS_DROPDOWNLIST)
    {
        case 0:
            m_cChildren = 0;    // Window not valid!
            break;

        case CBS_SIMPLE:
            m_fHasButton = FALSE;
            m_fHasEdit = TRUE;
            break;

        case CBS_DROPDOWN:
            m_fHasButton = TRUE;
            m_fHasEdit = TRUE;
            break;

        case CBS_DROPDOWNLIST:
            m_fHasButton = TRUE;
            m_fHasEdit = FALSE;
            break;
    }
}


// --------------------------------------------------------------------------
//
//  CCombo::get_accChildCount()
//
//  Since this is a known constant, hand directly to CAccessible.  No
//  need to count up fixed + window children.
//
// --------------------------------------------------------------------------
STDMETHODIMP CCombo::get_accChildCount(long* pcCount)
{
    return(CAccessible::get_accChildCount(pcCount));
}



// --------------------------------------------------------------------------
//
//  CCombo::get_accChild()
//
//  Succeeds for listbox, and for item if editable.  This is because we
//  manipulate our children by ID, since they are known.  
//
// --------------------------------------------------------------------------
STDMETHODIMP CCombo::get_accChild(VARIANT varChild, IDispatch** ppdisp)
{
    COMBOBOXINFO cbi;
    HWND    hwndChild;

    InitPv(ppdisp);

    //
    // Validate
    //
    if (!ValidateChild(&varChild))
        return(E_INVALIDARG);

    if (!MyGetComboBoxInfo(m_hwnd, &cbi))
        return(S_FALSE);

    hwndChild = NULL;

    switch (varChild.lVal)
    {
        case INDEX_COMBOBOX:
            return(E_INVALIDARG);

        case INDEX_COMBOBOX_ITEM:
            hwndChild = cbi.hwndItem;
            break;

        case INDEX_COMBOBOX_LIST:
            hwndChild = cbi.hwndList;
            break;
    }

    if (!hwndChild)
        return(S_FALSE);
    else
        return(AccessibleObjectFromWindow(hwndChild, OBJID_WINDOW, IID_IDispatch,
            (void**)ppdisp));
}



// --------------------------------------------------------------------------
//
//  CCombo::get_accName()
//
// --------------------------------------------------------------------------
STDMETHODIMP CCombo::get_accName(VARIANT varChild, BSTR* pszName)
{
COMBOBOXINFO cbi;

    InitPv(pszName);

    //
    // Validate
    //
    if (!ValidateChild(&varChild))
        return(E_INVALIDARG);

    //
    // The name of the combobox, the edit inside of it, and the dropdown
    // are all the same.  The name of the button is Drop down/Pop up
    //
    if (varChild.lVal != INDEX_COMBOBOX_BUTTON)
    {
        HWND hwndComboEx = IsInComboEx(m_hwnd);
        if( ! hwndComboEx )
        {
            return(CClient::get_accName(varChild, pszName));
        }
        else
        {
            // Special case if we're in a comboex - since we're one level deep,
            // reach up to parent for its name...
            IAccessible * pAcc;
            HRESULT hr = AccessibleObjectFromWindow( hwndComboEx, OBJID_CLIENT, IID_IAccessible, (void **) & pAcc );
            if( hr != S_OK )
                return hr;
            VARIANT varChild;
            varChild.vt = VT_I4;
            varChild.lVal = CHILDID_SELF;
            hr = pAcc->get_accName( varChild, pszName );
            pAcc->Release();
            return hr;
        }
    }
    else
    {
        if (! MyGetComboBoxInfo(m_hwnd, &cbi))
            return(S_FALSE);

        if (IsWindowVisible(cbi.hwndList))
            return (HrCreateString(STR_DROPDOWN_HIDE,pszName));
        else
            return(HrCreateString(STR_DROPDOWN_SHOW, pszName));
    }
}





// --------------------------------------------------------------------------
//
//  CCombo::get_accValue()
//
//  The value of the combobox and the combobox item is the current text of
//  the thing.
//
// --------------------------------------------------------------------------
STDMETHODIMP CCombo::get_accValue(VARIANT varChild, BSTR* pszValue)
{
    InitPv(pszValue);

    //
    // Validate
    //
    if (!ValidateChild(&varChild))
        return(E_INVALIDARG);

    switch (varChild.lVal)
    {
        case INDEX_COMBOBOX:
        case INDEX_COMBOBOX_ITEM:
        {
            // HACK ALERT
            // The IE4 combobox is a superclassed standard combobox,
            // but if I use SendMessageA (which I do) to get the
            // text, I get back garbage. They keep everything
            // in Unicode. It is a bug in the Trident MSHTML 
            // implementation, but even if they fixed it and gave me
            // back an ANSI string, I wouldn't know what code page to
            // use to convert the ANSI string to Unicode - web pages
            // can be in a different language than the one the user's
            // computer uses! Since they already have everything in 
            // Unicode, we decided on a private message that will fill
            // in the Unicode string, and I use that just like I would 
            // normally use WM_GETTEXT.
            // I was going to base this on the classname of the listbox
            // window, which is "Internet Explorer_TridentCmboBx", but
            // the list part of a combo doesn't have a special class
            // name, so instead I am going to base the special case on
            // the file name of the module that owns the window.
            
            // GetWindowModuleFileName(m_hwnd,szModuleName,ARRAYSIZE(szModuleName));
            // lpszModuleName = MyPathFindFileName (szModuleName);
            // if (0 == lstrcmp(lpszModuleName,TEXT("MSHTML.DLL")))
            
            // Update: (BrendanM)
            // GetWindowModuleFilename is broken on Win2k...
            // IsTridentControl goes back to using classnames, and knows
            // how to cope with ComboLBoxes...

            if( IsTridentControl( m_hwnd, TRUE, FALSE ) )
            {
                OLECHAR*    lpszUnicodeText = NULL;
                OLECHAR*    lpszLocalText = NULL;
                HANDLE      hProcess;
                UINT        cch;

                cch = SendMessageINT(m_hwnd, OCM__BASE + WM_GETTEXTLENGTH, 0, 0);

                lpszUnicodeText = (OLECHAR *)SharedAlloc((cch+1)*sizeof(OLECHAR),
                                                         m_hwnd,
                                                         &hProcess);
                lpszLocalText = (OLECHAR*)LocalAlloc(LPTR,(cch+1)*sizeof(OLECHAR));

                if (!lpszUnicodeText || !lpszLocalText)
                    return(E_OUTOFMEMORY);

                cch = SendMessageINT(m_hwnd, OCM__BASE + WM_GETTEXT, cch, (LPARAM)lpszUnicodeText);
                SharedRead (lpszUnicodeText,lpszLocalText,(cch+1)*sizeof(OLECHAR),hProcess);

                *pszValue = SysAllocString(lpszLocalText);

                SharedFree(lpszUnicodeText,hProcess);
                LocalFree(lpszLocalText);
                return (S_OK);
            }
            else
            {
                // If we're a comboex, ask the comboex instead of us...
                HWND hwnd;
                if( ! ( hwnd = IsInComboEx( m_hwnd ) ) )
                    hwnd = m_hwnd;
                    
                // uh-oh - don't want to use HrGetWindowName, since
                // it will look for a label (even though we specify FALSE)
                // if we are in a dialog and out text is "".
                if( ! IsComboEx( hwnd ) )
                {
                    // Regular combo - gettext works for both edit and droplist...
                    return HrGetWindowNameNoLabel( hwnd, pszValue);
                }
                else
                {
                    // comboex - special case for droplist...
                    DWORD dwStyle = GetWindowLong( hwnd, GWL_STYLE );
                    if( ! ( dwStyle & CBS_DROPDOWNLIST ) )
                    {
                        // Not a droplist - can use normal technique...
                        return HrGetWindowNameNoLabel( hwnd, pszValue);
                    }
                    else
                    {
                        // Get the selected item, and get its text...
                        int iSel = SendMessageINT( hwnd, CB_GETCURSEL, 0, 0 );
                        if( iSel == CB_ERR )
                            return S_FALSE; // no item selected

                        int cch = SendMessageINT( hwnd, CB_GETLBTEXTLEN, iSel, 0);

                        // Some apps do not handle CB_GETTEXTLEN correctly, and
                        // always return a small number, like 2.
		                if (cch < 1024)
			                cch = 1024;

                        LPTSTR lpszText;
                        lpszText = (LPTSTR)LocalAlloc(LPTR, (cch+1)*sizeof(TCHAR));
                        if (!lpszText)
                            return(E_OUTOFMEMORY);

                        SendMessage( hwnd, CB_GETLBTEXT, iSel, (LPARAM)lpszText);
                        *pszValue = TCharSysAllocString(lpszText);

                        LocalFree((HANDLE)lpszText);

                        return S_OK;
                    }
                }
            }
        }
    }

    return(E_NOT_APPLICABLE);
}



// --------------------------------------------------------------------------
//
//  CCombo::get_accRole()
//
// --------------------------------------------------------------------------
STDMETHODIMP CCombo::get_accRole(VARIANT varChild, VARIANT* pvarRole)
{
    InitPvar(pvarRole);

    //
    // Validate
    //
    if (!ValidateChild(&varChild))
        return(E_INVALIDARG);

    pvarRole->vt = VT_I4;

    switch (varChild.lVal)
    {
        case INDEX_COMBOBOX:
            pvarRole->lVal = ROLE_SYSTEM_COMBOBOX;
            break;

        case INDEX_COMBOBOX_ITEM:
            if (m_fHasEdit)
                pvarRole->lVal = ROLE_SYSTEM_TEXT;
            else
                pvarRole->lVal = ROLE_SYSTEM_STATICTEXT;
            break;

        case INDEX_COMBOBOX_BUTTON:
            pvarRole->lVal = ROLE_SYSTEM_PUSHBUTTON;
            break;

        case INDEX_COMBOBOX_LIST:
            pvarRole->lVal = ROLE_SYSTEM_LIST;
            break;

        default:
            AssertStr( "Invalid ChildID for child of combo box" );
    }

    return(S_OK);
}



// --------------------------------------------------------------------------
//
//  CCombo::get_accState()
//
//  The state of the combo is the state of the client.
//  The state of the item is the state of the edit field if present; 
//      read-only if static.
//  The state of the button is pushed and/or hottracked.
//  The state of the dropdown is floating (if not simple) and the state
//      of the list window.
//
// --------------------------------------------------------------------------
STDMETHODIMP CCombo::get_accState(VARIANT varChild, VARIANT* pvarState)
{
    COMBOBOXINFO    cbi;
    VARIANT         var;
    IAccessible* poleacc;
    HRESULT         hr;
    HWND            hwndActive;

    InitPvar(pvarState);

    //
    // Validate
    //
    if (!ValidateChild(&varChild))
        return(E_INVALIDARG);

    if (!MyGetComboBoxInfo(m_hwnd, &cbi))
    {
        pvarState->vt = VT_I4;
        pvarState->lVal = STATE_SYSTEM_INVISIBLE;
        return(S_FALSE);
    }

    switch (varChild.lVal)
    {
        case INDEX_COMBOBOX:
            return(CClient::get_accState(varChild, pvarState));

        case INDEX_COMBOBOX_BUTTON:
            pvarState->vt = VT_I4;
            pvarState->lVal = cbi.stateButton;
            break;

        case INDEX_COMBOBOX_ITEM:
            if (!cbi.hwndItem)
            {
                pvarState->vt = VT_I4;
                pvarState->lVal = 0;
                hwndActive = GetForegroundWindow();
                if (hwndActive == MyGetAncestor(m_hwnd, GA_ROOT))
                    pvarState->lVal |= STATE_SYSTEM_FOCUSABLE;
                if (MyGetFocus() == m_hwnd)
                    pvarState->lVal |= STATE_SYSTEM_FOCUSED;
            }
            else
            {
                // Forward state to edit field.
                VariantInit(&var);
                hr = GetWindowObject(cbi.hwndItem, &var);
                goto AskTheChild;
            }
            break;

        case INDEX_COMBOBOX_LIST:
            // Forward state to listbox
            VariantInit(&var);
            hr = GetWindowObject(cbi.hwndList, &var);

AskTheChild:
            if (!SUCCEEDED(hr))
                return(hr);

            Assert(var.vt == VT_DISPATCH);

            //
            // Get the child acc object
            //
            poleacc = NULL;
            hr = var.pdispVal->QueryInterface(IID_IAccessible,
                (void**)&poleacc);
            var.pdispVal->Release();

            if (!SUCCEEDED(hr))
                return(hr);

            //
            // Ask the child its state
            //
            VariantInit(&var);
            hr = poleacc->get_accState(var, pvarState);
            poleacc->Release();
            if (!SUCCEEDED(hr))
                return(hr);
            break;
    }

    return(S_OK);
}



// --------------------------------------------------------------------------
//
//  CCombo::get_accKeyboardShortcut()
//
// --------------------------------------------------------------------------
STDMETHODIMP CCombo::get_accKeyboardShortcut(VARIANT varChild, BSTR* pszShortcut)
{
    TCHAR   szKey[20];

    //
    // Shortcut for combo is label's hotkey.
    // Shortcut for dropdown (if button) is Alt+F4.
    // CWO, 12/5/96, Alt+F4? F4, by itself brings down the combo box,
    //                       but we add "Alt" to the string.  Bad!  Now use 
    //                       down arrow and add Alt to it via HrMakeShortcut()
    //                       As documented in the UI style guide.
    //
    // As always, shortcuts only apply if the container has "focus".  In other
    // words, the hotkey for the combo does nothing if the parent dialog
    // isn't active.  And the hotkey for the dropdown does nothing if the
    // combobox/edit isn't focused.
    //

    InitPv(pszShortcut);

    //
    // Validate parameters
    //
    if (!ValidateChild(&varChild))
        return(E_INVALIDARG);

    if (varChild.lVal == INDEX_COMBOBOX || varChild.lVal == INDEX_COMBOBOX_ITEM)
    {
        HWND hwndComboEx = IsInComboEx(m_hwnd);
        if( ! hwndComboEx )
        {
            return(CClient::get_accKeyboardShortcut(varChild, pszShortcut));
        }
        else
        {
            // Special case if we're in a comboex - since we're one level deep,
            // reach up to parent for its name...
            IAccessible * pAcc;
            HRESULT hr = AccessibleObjectFromWindow( hwndComboEx, OBJID_CLIENT, IID_IAccessible, (void **) & pAcc );
            if( hr != S_OK )
                return hr;
            VARIANT varChild;
            varChild.vt = VT_I4;
            varChild.lVal = CHILDID_SELF;
            hr = pAcc->get_accKeyboardShortcut( varChild, pszShortcut );
            pAcc->Release();
            return hr;
        }
    }
    else if (varChild.lVal == INDEX_COMBOBOX_BUTTON)
    {
        if (m_fHasButton)
        {
            LoadString(hinstResDll, STR_COMBOBOX_LIST_SHORTCUT, szKey,
                ARRAYSIZE(szKey));
            return(HrMakeShortcut(szKey, pszShortcut));
        }
    }

    return(E_NOT_APPLICABLE);
}



// --------------------------------------------------------------------------
//
//  CCombo::get_accDefaultAction()
//
// --------------------------------------------------------------------------
STDMETHODIMP CCombo::get_accDefaultAction(VARIANT varChild, BSTR* pszDef)
{
    COMBOBOXINFO cbi;

    InitPv(pszDef);

    //
    // Validate parameters
    //
    if (!ValidateChild(&varChild))
        return(E_INVALIDARG);

    if ((varChild.lVal != INDEX_COMBOBOX_BUTTON) || !m_fHasButton)
        return(E_NOT_APPLICABLE);

    //
    // Default action of button is to press it.  If pressed already, pressing
    // it will pop dropdown back up.  If not pressed, pressing it will pop
    // dropdown down.
    //
    if (! MyGetComboBoxInfo(m_hwnd, &cbi))
        return(S_FALSE);

    if (IsWindowVisible(cbi.hwndList))
        return(HrCreateString(STR_DROPDOWN_HIDE, pszDef));
    else
        return(HrCreateString(STR_DROPDOWN_SHOW, pszDef));
}



// --------------------------------------------------------------------------
//
//  CCombo::accLocation()
//
// --------------------------------------------------------------------------
STDMETHODIMP CCombo::accLocation(long* pxLeft, long* pyTop, long* pcxWidth,
    long* pcyHeight, VARIANT varChild)
{
    COMBOBOXINFO    cbi;

    InitAccLocation(pxLeft, pyTop, pcxWidth, pcyHeight);

    //
    // Validate
    //
    if (!ValidateChild(&varChild))
        return(E_INVALIDARG);

    if (! varChild.lVal)
        return(CClient::accLocation(pxLeft, pyTop, pcxWidth, pcyHeight, varChild));

    if (! MyGetComboBoxInfo(m_hwnd, &cbi))
        return(S_FALSE);

    switch (varChild.lVal)
    {
        case INDEX_COMBOBOX_BUTTON:
            if (!m_fHasButton)
                return(S_FALSE);

            *pcxWidth = cbi.rcButton.right - cbi.rcButton.left;
            *pcyHeight = cbi.rcButton.bottom - cbi.rcButton.top;

            ClientToScreen(m_hwnd, (LPPOINT)&cbi.rcButton);

            *pxLeft = cbi.rcButton.left;
            *pyTop = cbi.rcButton.top;
            break;

        case INDEX_COMBOBOX_ITEM:
            *pcxWidth = cbi.rcItem.right - cbi.rcItem.left;
            *pcyHeight = cbi.rcItem.bottom - cbi.rcItem.top;
            
            ClientToScreen(m_hwnd, (LPPOINT)&cbi.rcItem);

            *pxLeft = cbi.rcItem.left;
            *pyTop = cbi.rcItem.top;
            break;

        case INDEX_COMBOBOX_LIST:
            MyGetRect(cbi.hwndList, &cbi.rcItem, TRUE);
            *pxLeft = cbi.rcItem.left;
            *pyTop = cbi.rcItem.top;
            *pcxWidth = cbi.rcItem.right - cbi.rcItem.left;
            *pcyHeight = cbi.rcItem.bottom - cbi.rcItem.top;
            break;

        default:
            AssertStr( "Invalid ChildID for child of combo box" );
    }

    return(S_OK);
}



// --------------------------------------------------------------------------
//
//  CCombo::accNavigate()
//
//  Navigates among children of combobox.
//
// --------------------------------------------------------------------------
STDMETHODIMP CCombo::accNavigate(long dwNav, VARIANT varStart, VARIANT* pvarEnd)
{
    COMBOBOXINFO    cbi;
    long            lEnd;

    InitPvar(pvarEnd);

    //
    // Validate parameters
    //
    if ((!ValidateChild(&varStart) && !ValidateHwnd(&varStart)) ||
        !ValidateNavDir(dwNav, varStart.lVal))
        return(E_INVALIDARG);

    if (! MyGetComboBoxInfo(m_hwnd, &cbi))
        return(S_FALSE);

    lEnd = 0;

    if (dwNav == NAVDIR_FIRSTCHILD)
    {
        lEnd =  INDEX_COMBOBOX_ITEM;
        goto GetTheChild;
    }
    else if (dwNav == NAVDIR_LASTCHILD)
    {
        dwNav = NAVDIR_PREVIOUS;
        varStart.lVal = m_cChildren + 1;
    }
    else if (!varStart.lVal)
        return(CClient::accNavigate(dwNav, varStart, pvarEnd));

    //
    // Map HWNDID to normal ID.  We work with both (it is easier).
    //
    if (IsHWNDID(varStart.lVal))
    {
        HWND hWndTemp = HwndFromHWNDID(varStart.lVal);

        if (hWndTemp == cbi.hwndItem)
            varStart.lVal = INDEX_COMBOBOX_ITEM;
        else if (hWndTemp == cbi.hwndList)
            varStart.lVal = INDEX_COMBOBOX_LIST;
        else
            // Don't know what the heck this is
            return(S_FALSE);
    }

    switch (dwNav)
    {
        case NAVDIR_UP:
            if (varStart.lVal == INDEX_COMBOBOX_LIST)
                lEnd = INDEX_COMBOBOX_ITEM;
            break;

        case NAVDIR_DOWN:
            if ((varStart.lVal != INDEX_COMBOBOX_LIST) &&
                IsWindowVisible(cbi.hwndList))
            {
                lEnd = INDEX_COMBOBOX_LIST;
            }
            break;

        case NAVDIR_LEFT:
            if (varStart.lVal == INDEX_COMBOBOX_BUTTON)
                lEnd = INDEX_COMBOBOX_ITEM;
            break;

        case NAVDIR_RIGHT:
            if ((varStart.lVal == INDEX_COMBOBOX_ITEM) &&
               !(cbi.stateButton & STATE_SYSTEM_INVISIBLE))
            {
               lEnd = INDEX_COMBOBOX_BUTTON;
            }   
            break;

        case NAVDIR_PREVIOUS:
            lEnd = varStart.lVal - 1;
            if ((lEnd == INDEX_COMBOBOX_LIST) && !IsWindowVisible(cbi.hwndList))
                --lEnd;
            if ((lEnd == INDEX_COMBOBOX_BUTTON) && !m_fHasButton)
                --lEnd;
            break;

        case NAVDIR_NEXT:
            lEnd = varStart.lVal + 1;
            if (lEnd > m_cChildren)
                lEnd = 0;
            else
            {
                if ((lEnd == INDEX_COMBOBOX_BUTTON) && !m_fHasButton)
                    lEnd++;
                if ((lEnd == INDEX_COMBOBOX_LIST) && !IsWindowVisible(cbi.hwndList))
                    lEnd = 0;
            }
            break;
    }

GetTheChild:
    if (lEnd)
    {
        if ((lEnd == INDEX_COMBOBOX_ITEM) && cbi.hwndItem)
            return(GetWindowObject(cbi.hwndItem, pvarEnd));
        else if ((lEnd == INDEX_COMBOBOX_LIST) && cbi.hwndList)
            return(GetWindowObject(cbi.hwndList, pvarEnd));

        pvarEnd->vt = VT_I4;
        pvarEnd->lVal = lEnd;
        return(S_OK);
    }

    return(S_FALSE);
}




// --------------------------------------------------------------------------
//
//  CCombo::accHitTest()
//
// --------------------------------------------------------------------------
STDMETHODIMP CCombo::accHitTest(long x, long y, VARIANT* pvarEnd)
{
    POINT   pt;
    COMBOBOXINFO cbi;
    RECT    rc;

    InitPvar(pvarEnd);

    if (!MyGetComboBoxInfo(m_hwnd, &cbi))
        return(S_FALSE);

    pt.x = x;
    pt.y = y;

    // Check list first, in case it is a dropdown.
    MyGetRect(cbi.hwndList, &rc, TRUE);
    if (PtInRect(&rc, pt) && IsWindowVisible(cbi.hwndList))
        return(GetWindowObject(cbi.hwndList, pvarEnd));
    else
    {
        ScreenToClient(m_hwnd, &pt);
        MyGetRect(m_hwnd, &rc, FALSE);
        if (! PtInRect(&rc, pt))
            return(S_FALSE);

        if (PtInRect(&cbi.rcButton, pt))
        {
            pvarEnd->vt = VT_I4;
            pvarEnd->lVal = INDEX_COMBOBOX_BUTTON;
        }
        else if (PtInRect(&cbi.rcItem, pt))
        {
            if (m_fHasEdit)
                return(GetWindowObject(cbi.hwndItem, pvarEnd));
            else
            {
                pvarEnd->vt = VT_I4;
                pvarEnd->lVal = INDEX_COMBOBOX_ITEM;
            }
        }
        else
        {
            pvarEnd->vt = VT_I4;
            pvarEnd->lVal = 0;
        }
    }

    return(S_OK);
}



// --------------------------------------------------------------------------
//
//  CCombo::accDoDefaultAction()
//
//  The default action of the button is to toggle the dropdown list up or 
//  down.  Note that we don't just pop up the listbox, we pop it up AND
//  accept amu changes in the selected item..
//
// --------------------------------------------------------------------------
STDMETHODIMP CCombo::accDoDefaultAction(VARIANT varChild)
{
    COMBOBOXINFO    cbi;

    //
    // Validate
    //
    if (!ValidateChild(&varChild))
        return(E_INVALIDARG);

    if ((varChild.lVal == INDEX_COMBOBOX_BUTTON) && m_fHasButton)
    {
        if (!MyGetComboBoxInfo(m_hwnd, &cbi))
            return(S_FALSE);

        if (IsWindowVisible(cbi.hwndList))
            PostMessage(m_hwnd, WM_KEYDOWN, VK_RETURN, 0);
        else
            PostMessage(m_hwnd, CB_SHOWDROPDOWN, TRUE, 0);

        return(S_OK);
    }

    return(E_NOT_APPLICABLE);
}



// --------------------------------------------------------------------------
//
//  CCombo::put_accValue()
//
//  This works if (1) the combo is editable or (2) the text matches a list
//  item exactly.
//
// --------------------------------------------------------------------------
STDMETHODIMP CCombo::put_accValue(VARIANT varChild, BSTR szValue)
{
    //
    // Validate
    //
    if (!ValidateChild(&varChild))
        return(E_INVALIDARG);

    LPTSTR  lpszValue;

#ifdef UNICODE

	// On unicode, no conversion needed...
	lpszValue = szValue;

#else

	// On non-unicode, need to convert to multibyte...

    // We may be dealing with DBCS chars - assume worst case where every character is
    // two bytes...
    UINT cchValue = SysStringLen(szValue) * 2;
    lpszValue = (LPTSTR)LocalAlloc(LPTR, (cchValue+1)*sizeof(TCHAR));
    if (!lpszValue)
        return(E_OUTOFMEMORY);

    WideCharToMultiByte(CP_ACP, 0, szValue, -1, lpszValue, cchValue+1, NULL,
        NULL);

#endif

    //
    // If this is editable, set the text directly.  If this is a dropdown
    // list, select the exact match for this text.
    //
    if (m_fHasEdit)
        SendMessage(m_hwnd, WM_SETTEXT, 0, (LPARAM)lpszValue);
    else
        SendMessage(m_hwnd, CB_SELECTSTRING, (UINT)-1, (LPARAM)lpszValue);

#ifndef UNICODE
	// On non-unicode, free the temp string we alloc'd above...
    LocalFree((HANDLE)lpszValue);
#endif

    return(S_OK);
}



// --------------------------------------------------------------------------
//
//  CCombo::Next()
//
// --------------------------------------------------------------------------
STDMETHODIMP CCombo::Next(ULONG celt, VARIANT* rgvar, ULONG* pceltFetched)
{
    return(CAccessible::Next(celt, rgvar, pceltFetched));
}


// --------------------------------------------------------------------------
//
//  CCombo::Skip()
//
// --------------------------------------------------------------------------
STDMETHODIMP CCombo::Skip(ULONG celt)
{
    return(CAccessible::Skip(celt));
}








// --------------------------------------------------------------------------
//
//  HrGetWindowNameNoLabel()
//
//  This variation of HrGetWindowName (originally from client.cpp)
//  never uses a label. (HrGetWindowName would alway use a label
//  if window text was "" and window was in a dialog. That's not
//  appropriate for getting combo value text, though...)
//
// --------------------------------------------------------------------------
HRESULT HrGetWindowNameNoLabel(HWND hwnd, BSTR* pszName)
{
    LPTSTR  lpText = NULL;

    if( ! IsWindow( hwnd ) )
        return E_INVALIDARG;

    // Look for a name property!
    lpText = GetTextString( hwnd, FALSE );
    if( ! lpText )
        return S_FALSE;

    // Strip out the mnemonic.
    StripMnemonic(lpText);

    // Get a BSTR
    *pszName = TCharSysAllocString( lpText );

    // Free our buffer
    LocalFree( (HANDLE)lpText );

    // Did the BSTR succeed?
    if( ! *pszName )
        return E_OUTOFMEMORY;

    return S_OK;
}
